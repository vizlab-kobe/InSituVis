#include "MultiplePBVR.h"
#include "InverseDistanceWeighting.h"
#include <kvs/UnstructuredVolumeObject>
#include <kvs/PointObject>
#include <kvs/CellByCellMetropolisSampling>
#include <kvs/ParticleBasedRenderer>
#include <kvs/TransferFunction>
#include <kvs/osmesa/Screen>
#include <ParallelImageComposition/Lib/ImageCompositor.h>
#include <mpi.h>
#include <cfloat>
#include <kvs/PolygonObject>
#include <kvs/PolygonImporter>
#include <kvs/StochasticPolygonRenderer>
#include <kvs/StochasticRenderingCompositor>

void MultiplePBVR( const std::vector<float> &p_values, const std::vector<float> &u_values, int ncells, int nnodes, const std::vector<float> &vertex_coords, const std::vector<float> &cell_coords, const std::vector<int> &label, int time, float p_min_value, float p_max_value, float u_min_value, float u_max_value )
{
  int rank, nrank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nrank);

  float global_minx = vertex_coords[0];
  float global_miny = vertex_coords[1];
  float global_minz = vertex_coords[2];
  float global_maxx = vertex_coords[0];
  float global_maxy = vertex_coords[1];
  float global_maxz = vertex_coords[2];

  for( int i = 0; i < nnodes; i++ )
    {
      if( global_minx > vertex_coords[3*i+0])
	global_minx = vertex_coords[3*i+0];
      if( global_miny > vertex_coords[3*i+1])
	global_miny = vertex_coords[3*i+2];
      if( global_minz > vertex_coords[3*i+2])
	global_minz = vertex_coords[3*i+2];  
      if( global_maxx < vertex_coords[3*i+0])
	global_maxx = vertex_coords[3*i+0];
      if( global_maxy < vertex_coords[3*i+1])
	global_maxy = vertex_coords[3*i+1];
      if( global_maxz < vertex_coords[3*i+2])
	global_maxz = vertex_coords[3*i+2];              
    }

  CalculateMinMax( global_minx, global_miny, global_minz, global_maxx, global_maxy, global_maxz );

  local::InverseDistanceWeighting<kvs::Real32> p_idw(nnodes);
  local::InverseDistanceWeighting<kvs::Real32> u_idw(nnodes);
  std::vector<kvs::UInt32> tmp_connection;
 
  int nonode_count = 0;
  int nocell_count = 0;
  for ( int i = 0; i < ncells; i++ )
    {
      if( label[ 8 * i + 0 ] < 0 ){
	nocell_count++;
      }
      else{
	tmp_connection.push_back( label[ 8 * i + 4 ] );
	tmp_connection.push_back( label[ 8 * i + 6 ] );
	tmp_connection.push_back( label[ 8 * i + 7 ] );
	tmp_connection.push_back( label[ 8 * i + 5 ] );
	tmp_connection.push_back( label[ 8 * i + 0 ] );
	tmp_connection.push_back( label[ 8 * i + 2 ] );
	tmp_connection.push_back( label[ 8 * i + 3 ] );
	tmp_connection.push_back( label[ 8 * i + 1 ] );
	float p_value = p_values[i];
	float u_value = u_values[i];
	
	for ( int j = 0; j < 8 ; j++ )
	  {
	    const kvs::UInt32 id = label[ 8 * i + j ];
	    kvs::Vec3 cellCoord( cell_coords[3*i+0], cell_coords[3*i+1], cell_coords[3*i+2]);
	    kvs::Vec3 vertexCoord( vertex_coords[3*id+0], vertex_coords[3*id+1], vertex_coords[3*id+2]);
	    const kvs::Real32 distance = ( cellCoord - vertexCoord ).length();
	    p_idw.insert( id, p_value, distance );
	    u_idw.insert( id, u_value, distance );
	  }
      }
    }
  kvs::ValueArray<kvs::Real32> p_vertex_values = p_idw.serialize();
  kvs::ValueArray<kvs::Real32> u_vertex_values = u_idw.serialize();
  
  kvs::ValueArray<float> coords = kvs::ValueArray<float>( nnodes * 3 );
  for( int i = 0; i < nnodes; i++)
    {
      coords[ 3 * i + 0] = vertex_coords[ 3 * i + 0 ] * 1000 ;
      coords[ 3 * i + 1] = vertex_coords[ 3 * i + 1 ] * 1000 ;
      coords[ 3 * i + 2] = vertex_coords[ 3 * i + 2 ] * 1000 ;
      if( p_vertex_values[ i ] == 0 )
	{
	  nonode_count++;
	}
    }

  kvs::ValueArray<kvs::UInt32> real_connections = kvs::ValueArray<kvs::UInt32>(tmp_connection);

  kvs::UnstructuredVolumeObject* p_volume = new kvs::UnstructuredVolumeObject();
  p_volume->setCellTypeToHexahedra();
  p_volume->setVeclen( 1 );
  //p_volume->setNumberOfNodes( nnodes - nonode_count );
  p_volume->setNumberOfNodes( nnodes );
  p_volume->setNumberOfCells( ncells - nocell_count );
  p_volume->setValues( p_vertex_values );
  p_volume->setCoords( coords );
  p_volume->setConnections( real_connections );
  p_volume->updateMinMaxCoords();
  p_volume->updateMinMaxValues();
  p_volume->setMinMaxValues( p_min_value, p_max_value );

  kvs::UnstructuredVolumeObject* u_volume = new kvs::UnstructuredVolumeObject();
  u_volume->setCellTypeToHexahedra();
  u_volume->setVeclen( 1 );
  //u_volume->setNumberOfNodes( nnodes - nonode_count );
  u_volume->setNumberOfNodes( nnodes );
  u_volume->setNumberOfCells( ncells - nocell_count );
  u_volume->setValues( u_vertex_values );
  u_volume->setCoords( coords );
  u_volume->setConnections( real_connections );
  u_volume->updateMinMaxCoords();
  u_volume->updateMinMaxValues();
  u_volume->setMinMaxValues( u_min_value, u_max_value );
  

  const size_t width = 512;
  const size_t height = 512;
  const size_t npixels = width * height;
  const bool depth_testing = true;
  kvs::ValueArray<kvs::Real32> ensemble_buffer( npixels * 3 );
  ensemble_buffer.fill( 0.0f );
  ParallelImageComposition::ImageCompositor compositor( rank, nrank, MPI_COMM_WORLD );
  compositor.initialize( width, height, depth_testing );

  const size_t repetitions = 20;
  const float step = 0.5f;
  
  kvs::OpacityMap p_omap( 256, p_min_value, p_max_value );
  p_omap.addPoint( p_min_value, 0.0 );
  p_omap.addPoint( p_min_value + 1, 0.1 );
  p_omap.addPoint( (p_max_value - p_min_value) * 0.5 + p_min_value, 0.2 );
  p_omap.addPoint( p_max_value - 1, 0.1 );
  p_omap.addPoint( p_max_value, 0.0 );
  p_omap.create();
  kvs::ColorMap p_cmap( 256, p_min_value, p_max_value );
  p_cmap.addPoint( p_min_value, kvs::RGBColor::White() );
  p_cmap.addPoint( p_max_value, kvs::RGBColor::Red() );  
  p_cmap.create();
  
  kvs::OpacityMap u_omap( 256, u_min_value, u_max_value );
  u_omap.addPoint( u_min_value, 0.0 );
  u_omap.addPoint( 10, 0.2 );
  u_omap.addPoint( u_max_value, 1.0 );
  u_omap.create();
  kvs::ColorMap u_cmap( 256, u_min_value, u_max_value );
  u_cmap.addPoint( u_min_value, kvs::RGBColor::White() );
  u_cmap.addPoint( 10 , kvs::RGBColor::Blue() );
  u_cmap.addPoint( u_max_value, kvs::RGBColor::Blue() );  
  u_cmap.create();
  
  kvs::TransferFunction p_tfunc( p_cmap, p_omap );
  kvs::TransferFunction u_tfunc( u_cmap, u_omap );
  
  kvs::osmesa::Screen screen;
  kvs::StochasticRenderingCompositor rendering_compositor( screen.scene() );
  screen.setBackgroundColor( kvs::RGBColor::White() );
  screen.setGeometry( 0, 0, width, height );
  screen.scene()->camera()->setWindowSize( width, height );
  const kvs::Mat3 R = kvs::Mat3::RotationX( 230 ) * kvs::Mat3::RotationY( 0 )* kvs::Mat3::RotationZ( 10 );
  screen.setEvent(&rendering_compositor );
  screen.create();
  kvs::Light::SetModelTwoSide( true );
  
  kvs::glsl::ParticleBasedRenderer* p_renderer = new kvs::glsl::ParticleBasedRenderer();
  kvs::glsl::ParticleBasedRenderer* u_renderer = new kvs::glsl::ParticleBasedRenderer();
  kvs::StochasticPolygonRenderer* poly_renderer = new kvs::StochasticPolygonRenderer();
  kvs::PolygonObject* poly_object = new kvs::PolygonImporter("/home/ubuntu/realistic-cfd3.stl");
  poly_object->setOpacity( 30 );
  //poly_object->setMinMaxExternalCoords( poly_object->minExternalCoord()*0.1, poly_object->maxExternalCoord()*0.1 );
  poly_object->multiplyXform( kvs::Xform::Rotation( R ) * kvs::Xform::Scaling( 1.3 ) );  
  poly_object->setName("Polygon");
  
  for( size_t i = 0; i < repetitions; i++)
    {
      kvs::Camera* camera = new kvs::Camera();
      camera->setWindowSize( width, height );

      kvs::PointObject* p_object = new kvs::CellByCellMetropolisSampling( camera, p_volume, 1, step, p_tfunc );
      p_object->setName("p_Particle");
      p_object->setMinMaxObjectCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
      p_object->setMinMaxExternalCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
      p_object->multiplyXform( kvs::Xform::Rotation( R ) * kvs::Xform::Scaling( 1.3 ) );

      kvs::PointObject* u_object = new kvs::CellByCellMetropolisSampling( camera, u_volume, 1, step, u_tfunc );
      u_object->setName("u_Particle");
      u_object->setMinMaxObjectCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
      u_object->setMinMaxExternalCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
      u_object->multiplyXform( kvs::Xform::Rotation( R ) * kvs::Xform::Scaling( 1.3 ) );
            
      kvs::PolygonObject* replace_poly_object = new kvs::PolygonObject();
      replace_poly_object->deepCopy( *poly_object );
      replace_poly_object->setName("Polygon");
      if( i != 0 )
	{
	  screen.scene()->replaceObject( "p_Particle", p_object );
	  screen.scene()->replaceObject( "u_Particle", u_object );
	  screen.scene()->replaceObject( "Polygon", replace_poly_object);
	}
      else
	{
	  screen.registerObject( p_object, p_renderer );
	  screen.registerObject( u_object, u_renderer );
	  screen.registerObject( replace_poly_object, poly_renderer);
	}
      
      //screen.draw();
      rendering_compositor.update();
      
      kvs::ValueArray<kvs::UInt8> color_buffer = screen.readbackColorBuffer();
      kvs::ValueArray<kvs::Real32> depth_buffer = screen.readbackDepthBuffer();
      compositor.run( color_buffer, depth_buffer );
      
      const float a = 1.0f / ( i + 1 );      
      for ( size_t j = 0; j < npixels; j++ )
	{
	  const float r = kvs::Real32( color_buffer[ 4 * j + 0 ] );
	  const float g = kvs::Real32( color_buffer[ 4 * j + 1 ] );
	  const float b = kvs::Real32( color_buffer[ 4 * j + 2 ] );
	  ensemble_buffer[ 3 * j + 0 ] = kvs::Math::Mix( ensemble_buffer[ 3 * j + 0 ], r, a );
	  ensemble_buffer[ 3 * j + 1 ] = kvs::Math::Mix( ensemble_buffer[ 3 * j + 1 ], g, a );
	  ensemble_buffer[ 3 * j + 2 ] = kvs::Math::Mix( ensemble_buffer[ 3 * j + 2 ], b, a );
	}
    }
      
  delete p_volume;
  delete u_volume;
  delete poly_object;
  std::vector<kvs::UInt32>().swap(tmp_connection);
  
  kvs::ValueArray<kvs::UInt8> pixels( npixels * 3 );
  for ( size_t i = 0; i < pixels.size(); i++ )
    {
      const int p = kvs::Math::Round( ensemble_buffer[i] );
      pixels[i] = kvs::Math::Clamp( p, 0 , 255 );
    }
  
  if( rank == 0 )
    {
      kvs::ColorImage image = kvs::ColorImage( width , height, pixels );
      std::ostringstream ss;
      ss << std::setw(5) << std::setfill('0') << time;
      std::string num = ss.str();
      std::string name = "./output_result_multiple_pbvr_" +num +".bmp";
      image.write( name );
    }
  
}


void CalculateMinMax( float& min_x, float& min_y, float& min_z, float& max_x, float& max_y, float & max_z )
{
  float recv_min_x = FLT_MAX;
  float recv_min_y = FLT_MAX;
  float recv_min_z = FLT_MAX;
  float recv_max_x = FLT_MIN;
  float recv_max_y = FLT_MIN;
  float recv_max_z = FLT_MIN;
  MPI_Allreduce( &min_x, &recv_min_x, 1, MPI_FLOAT, MPI_MIN, MPI_COMM_WORLD );
  MPI_Allreduce( &min_y, &recv_min_y, 1, MPI_FLOAT, MPI_MIN, MPI_COMM_WORLD );
  MPI_Allreduce( &min_z, &recv_min_z, 1, MPI_FLOAT, MPI_MIN, MPI_COMM_WORLD );
  MPI_Allreduce( &max_x, &recv_max_x, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
  MPI_Allreduce( &max_y, &recv_max_y, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
  MPI_Allreduce( &max_z, &recv_max_z, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
  
  min_x = recv_min_x;
  min_y = recv_min_y;
  min_z = recv_min_z;
  max_x = recv_max_x;
  max_y = recv_max_y;
  max_z = recv_max_z;
}
