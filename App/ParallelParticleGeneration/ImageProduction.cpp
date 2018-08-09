#include "ImageProduction.h"
#include "ParticleGenerator.h"
#include "ParticleRenderer.h"
#include <kvs/PointObject>
#include <kvs/RGBFormulae>
#include <kvs/OpenGL>
#include <kvs/UnstructuredVolumeImporter>
#include <ParallelImageComposition/Lib/ImageCompositor.h>
#include <kvs/Camera>
#include <kvs/UnstructuredVolumeExporter>
#include <kvs/DivergingColorMap>
#include "ParticleBasedRendererGLSL.h"

#include <kvs/osmesa/Screen>
//#include <kvs/egl/Screen>


namespace
{

class Stamper
{
private:
    kvs::Timer& m_timer;
public:
    Stamper( kvs::Timer& timer ): m_timer( timer ) { m_timer.start(); }
    ~Stamper() { m_timer.stop(); }
};

} // end of namespace

kvs::FieldViewData ImageProduction::read( const Input& input, kvs::Timer& timer )
{
    ::Stamper stamper( timer );
    kvs::FieldViewData data( input.filename );
    return data;
}


std::vector<kvs::VolumeObjectBase*> ImageProduction::import( const Input& input, kvs::Timer& timer, const kvs::FieldViewData& data )
{
    ::Stamper stamper( timer );

    this->calculate_min_max( data );
    std::vector<kvs::VolumeObjectBase*> volumes;
    const int nregions = input.regions;
    for ( int i = 0; i < nregions; i++ )
      {
        const int gindex = m_rank + m_nnodes * i;
        kvs::VolumeObjectBase* volume = this->import_volume( data, gindex );
        if ( volume )
        {
            volume->setMinMaxValues( m_min_value, m_max_value );
            volume->setMinMaxExternalCoords( m_min_ext, m_max_ext );
            volumes.push_back( volume );
        }
    }
    return volumes;
}

void ImageProduction::render( const Input& input, kvs::Timer& timer, const std::vector<kvs::VolumeObjectBase*>& volumes, kvs::mpi::Communicator world, double& particle_time, double& transfer_time, double& projection_time, double& createbuffer_time, double& readback_time, double& averaging_time, double& num_particle, double& part_particle)
{
  ::Stamper stamper( timer );
  
  kvs::Timer particle_timer(kvs::Timer::Start );
  kvs::PointObject* object = this->make_point( volumes, input );
  particle_timer.stop();
  particle_time = particle_timer.sec();
  part_particle = object->numberOfSizes();
  if(m_rank != 0)
    {     
      world.send(0, m_rank+100, object->coords() );
      world.send(0, m_rank+200, object->colors() );
      world.send(0, m_rank+300, object->normals() );
      world.send(0, m_rank+400, object->sizes() );
    }
  else
    {
      kvs::Timer transfer_timer(kvs::Timer::Start );
      for(int j =0; j < m_nnodes-1; j++)
	{
	  kvs::Timer transfer_timer(kvs::Timer::Start );
	  kvs::ValueArray<float> receive_coords;
	  kvs::ValueArray<unsigned char> receive_colors;
	  kvs::ValueArray<float> receive_normals;
	    kvs::ValueArray<float> receive_sizes;
	    world.receive( j+1, (j+1)+100, receive_coords );
	    world.receive( j+1, (j+1)+200, receive_colors );
	    world.receive( j+1, (j+1)+300, receive_normals );
	    world.receive( j+1, (j+1)+400, receive_sizes );
	    kvs::PointObject receive_object;
	    receive_object.setCoords(receive_coords);
	    receive_object.setColors(receive_colors);
	    receive_object.setNormals(receive_normals);
	    receive_object.setSizes(receive_sizes);
	    receive_object.updateMinMaxCoords();
	    object->add(receive_object);
	}
      transfer_timer.stop();
      transfer_time = transfer_timer.sec();
      num_particle = object->numberOfSizes();
      if(input.sampling_method ==4)
	object->setColor(kvs::RGBColor::Black() );
      object->write("output.kvsml",false, false);      
      kvs::osmesa::Screen screen;
      screen.setGeometry(0, 0, input.width, input.height);
      screen.scene()->camera()->setWindowSize(input.width, input.height);
      local::StochasticRendererBase* renderer = new local::ParticleBasedRenderer();
      renderer->disableShading();
      renderer->setRepetitionLevel(input.repetitions); 
      //screen.registerObject( object, local::ParticleRenderer( input, m_min_value, m_max_value) );    
      screen.registerObject( object,renderer );    
      screen.setBackgroundColor( kvs::RGBColor::White() );
      //object->multiplyXform( kvs::Xform::Rotation( kvs::Mat3::RotationY( -30 ) ) );
      //object->multiplyXform( kvs::Xform::Rotation( kvs::Mat3::RotationX( -20 ) ) );
      screen.draw();
      //screen.capture();
      screen.capture().write("output.bmp");
      projection_time = renderer->getDrawTime();
      createbuffer_time = renderer->getCreateTime();
      averaging_time = renderer->getAveragingTime();
	
    }

}
kvs::VolumeObjectBase* ImageProduction::import_volume( const kvs::FieldViewData& data, const int gindex )
{
    if ( gindex == 76 || gindex == 91 ) return NULL;
    if ( gindex >= 256 ) return NULL;

    const size_t vindex = 3;
    const int etype = kvs::FieldViewData::Pri;
    if ( data.grid(gindex).nelements[ etype ] > 0 )
    {
        data.setImportingElementType( etype );
        data.setImportingGridIndex( gindex );
        data.setImportingVariableIndex( vindex );
        return new kvs::UnstructuredVolumeImporter( &data );
    }

    return NULL;
}

void ImageProduction::calculate_min_max( const kvs::FieldViewData& data )
{
    m_min_ext = kvs::Vec3::All( FLT_MAX );
    m_max_ext = kvs::Vec3::All( FLT_MIN );
    m_min_value = FLT_MAX;
    m_max_value = FLT_MIN;

    const int etype = kvs::FieldViewData::Pri;
    const size_t ngrids = data.numberOfGrids();
    for ( size_t i = 0; i < ngrids; i++ )
    {
        const size_t gindex = i;
        const kvs::FieldViewData::Grid& grid = data.grid( gindex );
        const size_t nelements = grid.nelements[etype];
        if ( nelements > 0 && grid.nelements[1]<=0)
        {
            const size_t vindex = 3;
            const size_t nnodes = grid.nodes.size();
            for ( size_t j = 0; j < nnodes; j++ )
            {
                m_min_ext.x() = kvs::Math::Min( m_min_ext.x(), grid.nodes[j].x );
                m_min_ext.y() = kvs::Math::Min( m_min_ext.y(), grid.nodes[j].y );
                m_min_ext.z() = kvs::Math::Min( m_min_ext.z(), grid.nodes[j].z );
                m_max_ext.x() = kvs::Math::Max( m_max_ext.x(), grid.nodes[j].x );
                m_max_ext.y() = kvs::Math::Max( m_max_ext.y(), grid.nodes[j].y );
                m_max_ext.z() = kvs::Math::Max( m_max_ext.z(), grid.nodes[j].z );

                m_min_value = kvs::Math::Min( m_min_value, grid.variables[vindex].data[j] );
                m_max_value = kvs::Math::Max( m_max_value, grid.variables[vindex].data[j] );
            }
        }
    }
}

 kvs::PointObject* ImageProduction::make_point( const std::vector<kvs::VolumeObjectBase*>& volumes, const Input& input )
{
  kvs::PointObject* object = new kvs::PointObject();

  for ( size_t i = 0; i < volumes.size(); i++ )
    {
        const kvs::VolumeObjectBase* input_volume = volumes[i];
        if ( input_volume )
	  {
	    kvs::TransferFunction tfunc = kvs::TransferFunction( kvs::RGBFormulae::Hot(256) );
	    //kvs::TransferFunction tfunc = kvs::TransferFunction( kvs::DivergingColorMap::CoolWarm( 256 ) );
	  if ( !input.tf_filename.empty() ) { tfunc = kvs::TransferFunction( input.tf_filename ); }
	  kvs::PointObject* particles = local::ParticleGenerator( input, input_volume, tfunc );
	  if ( particles )
	    {
	      particles->updateMinMaxCoords();
	      object->add( *particles );
	      delete particles;
            }
        }
    }
    object->setMinMaxObjectCoords( m_min_ext, m_max_ext);
    object->setMinMaxExternalCoords( m_min_ext, m_max_ext);
    return object;

}
