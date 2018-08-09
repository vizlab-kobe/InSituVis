#include "ImageProduction.h"
#include "ParticleGenerator.h"
#include <kvs/PointObject>
#include <kvs/RGBFormulae>
#include <kvs/OpenGL>
#include <kvs/UnstructuredVolumeImporter>
#include "SubpixelRenderer.h"
#include <kvs/ParticleBasedRenderer>
#include <kvs/DivergingColorMap>

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
        kvs::VolumeObjectBase* volume = this->import_volume( data, (const int)gindex );
        if ( volume )
	  {
            volume->setMinMaxValues( m_min_value, m_max_value );
            volume->setMinMaxExternalCoords( m_min_ext, m_max_ext );
            volumes.push_back( volume );	    
	  }
      }
    return volumes;
}

void ImageProduction::render( const Input& input, kvs::Timer& timer, const std::vector<kvs::VolumeObjectBase*>& volumes ,double& particle_time, double& composition_time, double& render_time, double& num_particle)
{
    ::Stamper stamper( timer );

     kvs::ValueArray<float> accum_buffer( input.width * input.height * 3 );
    accum_buffer.fill( 0.0f );

    // NOTE: A set of particles will be generated for each repeat
    // with the repetition level of 1 in the current implementation.
    // Therefore, the input.repetitions used for rendering image will
    // be forcibily set to 1 here.

    Input temp_input = input;
    
    kvs::Timer composition_timer;
    kvs::Timer render_timer;
  
    kvs::osmesa::Screen screen;
    this->draw_image(screen, volumes, temp_input, particle_time, num_particle, composition_time, render_time );
    
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

void ImageProduction::draw_image( kvs::osmesa::Screen& screen, const std::vector<kvs::VolumeObjectBase*>& volumes, Input& input, double& particle_time, double& num_particle, double& composition_time, double& render_time )
{
  kvs::Timer particle_timer;
  kvs::PointObject* object = new kvs::PointObject();
  screen.setGeometry( 0, 0, input.width, input.height );
  screen.scene()->camera()->setWindowSize( input.width, input.height );
 
  for ( size_t i = 0; i < volumes.size(); i++ )
    {
      const kvs::VolumeObjectBase* input_volume = volumes[i];
        if ( input_volume )
	  {
            kvs::TransferFunction tfunc = kvs::TransferFunction( kvs::RGBFormulae::Hot(256) );
	    //kvs::TransferFunction tfunc = kvs::TransferFunction( kvs::DivergingColorMap::CoolWarm( 256 ) );
	    if ( !input.tf_filename.empty() ) { tfunc = kvs::TransferFunction( input.tf_filename ); }
	    particle_timer.start();
            kvs::PointObject* particles = local::ParticleGenerator( input, input_volume, tfunc, screen.scene()->camera() );
	    particle_timer.stop();
	    particle_time+=particle_timer.sec();
            if ( particles )
	      {
		particles->updateMinMaxCoords();
                object->add( *particles );
                delete particles;
	      }
	  }
    }
  particle_timer.stop();
  if( object ){
    object->setMinMaxObjectCoords( m_min_ext, m_max_ext);
    object->setMinMaxExternalCoords( m_min_ext, m_max_ext);
    num_particle += object->numberOfSizes();
    if(input.sampling_method ==4)
      {
	object->setColor(kvs::RGBColor::Black() );
      }
    local::SubpixelRenderer* renderer = new local::SubpixelRenderer( m_rank, m_nnodes, object, input.sublevel );
    //object->multiplyXform( kvs::Xform::Rotation( kvs::Mat3::RotationY( -30 ) ) );
    //object->multiplyXform( kvs::Xform::Rotation( kvs::Mat3::RotationX( -20 ) ) );
    screen.registerObject( object, renderer );
    screen.setBackgroundColor( kvs::RGBColor::White());
    composition_time = renderer->getCompositionTime();
   
  //std::string num = kvs::String::ToString( m_rank );
  // std::string name = "output_"+num +".bmp";
    screen.draw();
    screen.draw();
    render_time = screen.scene()->renderer()->timer().sec();
    composition_time = renderer->getCompositionTime();
  //screen.capture().write(name);
    if(m_rank ==0 )
      screen.capture().write("output.bmp");
  }
}
