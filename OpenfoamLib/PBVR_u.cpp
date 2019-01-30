#include "PBVR_u.h"
#include "InverseDistanceWeighting.h"
#include <kvs/UnstructuredVolumeObject>
#include <kvs/Isosurface>
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
#include <kvs/Timer>

void PBVR_u( const std::vector<float> &values, int ncells, int nnodes, const std::vector<float> &vertex_coords, const std::vector<float> &cell_coords, const std::vector<int> &label, int time, float min_value, float max_value )
{
  float conversion_time = 0.0;
  float vis_time = 0.0;
  float output_time = 0.0;
  kvs::Timer timer;
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

  local::InverseDistanceWeighting<kvs::Real32> idw(nnodes);
  std::vector<kvs::UInt32> tmp_connection;
 
  int nonode_count = 0;
  int nocell_count = 0;

  timer.start();
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
	float value = values[i];
	for ( int j = 0; j < 8 ; j++ )
	  {
	    const kvs::UInt32 id = label[ 8 * i + j ];
	    kvs::Vec3 cellCoord( cell_coords[3*i+0], cell_coords[3*i+1], cell_coords[3*i+2]);
	    kvs::Vec3 vertexCoord( vertex_coords[3*id+0], vertex_coords[3*id+1], vertex_coords[3*id+2]);
	    const kvs::Real32 distance = ( cellCoord - vertexCoord ).length();
	    idw.insert( id, value, distance );
	  }
      }
    }
  kvs::ValueArray<kvs::Real32> vertex_values = idw.serialize();
  
  kvs::ValueArray<float> coords = kvs::ValueArray<float>( nnodes * 3 );
  for( int i = 0; i < nnodes; i++)
    {
      coords[ 3 * i + 0] = vertex_coords[ 3 * i + 0 ] * 1000 ;
      coords[ 3 * i + 1] = vertex_coords[ 3 * i + 1 ] * 1000 ;
      coords[ 3 * i + 2] = vertex_coords[ 3 * i + 2 ] * 1000 ;
      if( vertex_values[ i ] == 0 )
	{
	  nonode_count++;
	}
    }

  kvs::ValueArray<kvs::UInt32> real_connections = kvs::ValueArray<kvs::UInt32>(tmp_connection);
  kvs::UnstructuredVolumeObject* volume = new kvs::UnstructuredVolumeObject();
  volume->setCellTypeToHexahedra();
  volume->setVeclen( 1 );
  //volume->setNumberOfNodes( nnodes - nonode_count );
  volume->setNumberOfNodes( nnodes );
  volume->setNumberOfCells( ncells - nocell_count );
  volume->setValues( vertex_values );
  volume->setCoords( coords );
  volume->setConnections( real_connections );
  volume->updateMinMaxCoords();
  volume->updateMinMaxValues();
  volume->setMinMaxValues( min_value, max_value );
  timer.stop();
  conversion_time = timer.sec();

  const size_t width = 512;
  const size_t height = 512;
  const size_t npixels = width * height;
  const bool depth_testing = true;
  kvs::ValueArray<kvs::Real32> ensemble_buffer( npixels * 3 );
  ensemble_buffer.fill( 0.0f );
  ParallelImageComposition::ImageCompositor compositor( rank, nrank, MPI_COMM_WORLD );
  compositor.initialize( width, height, depth_testing );

  const size_t repetitions = 50;
  const float step = 0.5f;

  kvs::TransferFunction tfunc( 256 );
  tfunc.setRange( min_value, max_value );

  kvs::osmesa::Screen screen;
  kvs::StochasticRenderingCompositor rendering_compositor( screen.scene() );
  screen.setBackgroundColor( kvs::RGBColor::White() );
  screen.setGeometry( 0, 0, width, height );
  screen.scene()->camera()->setWindowSize( width, height );
  const kvs::Mat3 R = kvs::Mat3::RotationX( 230 ) * kvs::Mat3::RotationY( 0 )* kvs::Mat3::RotationZ( 10 );
  screen.setEvent(&rendering_compositor );
  screen.create();
  kvs::Light::SetModelTwoSide( true );
  
  kvs::glsl::ParticleBasedRenderer* renderer = new kvs::glsl::ParticleBasedRenderer();
  kvs::StochasticPolygonRenderer* poly_renderer = new kvs::StochasticPolygonRenderer();
  kvs::PolygonObject* poly_object = new kvs::PolygonImporter("/home/ubuntu/realistic-cfd3.stl");
  poly_object->setOpacity( 30 );
  //poly_object->setMinMaxExternalCoords( poly_object->minExternalCoord()*0.1, poly_object->maxExternalCoord()*0.1 );
  poly_object->multiplyXform( kvs::Xform::Rotation( R ) * kvs::Xform::Scaling( 1.3 ) );
  //poly_object->multiplyXform( kvs::Xform::Rotation( R ) * kvs::Xform::Scaling( 2.6 ) );  
  poly_object->setName("Polygon");


  timer.start();
  for( size_t i = 0; i < repetitions; i++)
    {
      kvs::Camera* camera = new kvs::Camera();
      camera->setWindowSize( width, height );
      kvs::PointObject* object = new kvs::CellByCellMetropolisSampling( camera, volume, 1, step, tfunc );
      object->setName("Particle");
      object->setMinMaxObjectCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
      object->setMinMaxExternalCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
      object->multiplyXform( kvs::Xform::Rotation( R ) * kvs::Xform::Scaling( 1.3 ) );
      //object->multiplyXform( kvs::Xform::Rotation( R ) * kvs::Xform::Scaling( 2.6 ) );
      
      kvs::PolygonObject* replace_poly_object = new kvs::PolygonObject();
      replace_poly_object->deepCopy( *poly_object );
      replace_poly_object->setName("Polygon");
      if( i != 0 )
	{
	  screen.scene()->replaceObject( "Particle", object );
	  screen.scene()->replaceObject( "Polygon", replace_poly_object);
	}
      else
	{
	  screen.registerObject( object, renderer );
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
  timer.stop();
  vis_time = timer.sec();
  
  delete volume;
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
      std::string name = "./Output/output_result_mix_pbvr_u_" +num +".bmp";
      timer.start();
      image.write( name );
      timer.stop();
      output_time = timer.sec();
    }
  
  MPI_Allreduce( &conversion_time, &conversion_time, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
  MPI_Allreduce( &vis_time, &vis_time, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
  MPI_Allreduce( &output_time, &output_time, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );

  if( rank == 0)
    {
      std::cout << "conversion vis time : " << conversion_time << std::endl;
      std::cout << "visualization time : " << vis_time << std::endl;
      std::cout << "output image time : " << output_time << std::endl;
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
