#include "QCriterionIsosurface.h"
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
#include <kvs/UnstructuredQCriterion>

void QCriterionIsosurface( const std::vector<float> &values, int ncells, int nnodes, const std::vector<float> &vertex_coords, const std::vector<float> &cell_coords, const std::vector<int> &label, int time, float min_value, float max_value )
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

  local::InverseDistanceWeighting<kvs::Vec3> idw( nnodes );
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
	kvs::Vec3 value( values[ 3 * i + 0], values[ 3 * i + 1 ] , values[ 3 * i + 2]  );
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
      if( vertex_values[  3 * i ] == 0 )
	{
	  nonode_count++;
	}
    }

  kvs::ValueArray<kvs::UInt32> real_connections = kvs::ValueArray<kvs::UInt32>(tmp_connection);
  kvs::UnstructuredVolumeObject* volume = new kvs::UnstructuredVolumeObject();
  volume->setCellTypeToHexahedra();
  volume->setVeclen( 3 );
  //volume->setNumberOfNodes( nnodes - nonode_count );
  volume->setNumberOfNodes( nnodes );
  volume->setNumberOfCells( ncells - nocell_count );
  volume->setValues( vertex_values );
  volume->setCoords( coords );
  volume->setConnections( real_connections );
  volume->updateMinMaxCoords();

  kvs::UnstructuredQCriterion * q_volume = new kvs::UnstructuredQCriterion( volume );

  kvs::ValueArray<float> tmp_values = q_volume->values().asValueArray<float>();
  min_value = 10;
  max_value = 0;
  for( size_t i = 0; i < tmp_values.size() ; i++)
    {
      if( vertex_values[ 3 * i ] == 0  )
	{
	  continue;
	}
      if( min_value > tmp_values[i])
	min_value = tmp_values[i];
      if( max_value < tmp_values[i])
	max_value = tmp_values[i];
    }
    
  
  CalculateValues( min_value, max_value );
  q_volume->setMinMaxValues( min_value, max_value );
   
  const size_t width = 512;
  const size_t height = 512;
  const size_t npixels = width * height;
  const bool depth_testing = true;
  kvs::ValueArray<kvs::Real32> ensemble_buffer( npixels * 3 );
  ensemble_buffer.fill( 0.0f );
  ParallelImageComposition::ImageCompositor compositor( rank, nrank, MPI_COMM_WORLD );
  compositor.initialize( width, height, depth_testing );

  kvs::TransferFunction tfunc( 256 );
  tfunc.setRange( min_value, max_value );

  
  kvs::osmesa::Screen screen;
  kvs::StochasticRenderingCompositor rendering_compositor( screen.scene() );
  rendering_compositor.setRepetitionLevel( 30 );
  screen.setBackgroundColor( kvs::RGBColor::White() );
  screen.setGeometry( 0, 0, width, height );
  screen.scene()->camera()->setWindowSize( width, height );
  const kvs::Mat3 R = kvs::Mat3::RotationX( 230 ) * kvs::Mat3::RotationY( 0 )* kvs::Mat3::RotationZ( 10 );
  screen.setEvent(&rendering_compositor );
  screen.create();
  kvs::Light::SetModelTwoSide( true );

  kvs::StochasticPolygonRenderer* renderer = new kvs::StochasticPolygonRenderer();
  kvs::StochasticPolygonRenderer* poly_renderer = new kvs::StochasticPolygonRenderer();
  kvs::PolygonObject* poly_object = new kvs::PolygonImporter("/home/ubuntu/realistic-cfd3.stl");
  poly_object->setOpacity( 30 );
  poly_object->multiplyXform( kvs::Xform::Rotation( R ) * kvs::Xform::Scaling( 3.0 ) );  
  poly_object->setName("Polygon");
  
  //const double iso_level = ( q_volume->maxValue() - q_volume->minValue() ) * 0.5 + q_volume->minValue();
  const double iso_level = 100;
  kvs::PolygonObject::NormalType n = kvs::PolygonObject::PolygonNormal;
  const bool d = true;

  std::cout << "object make " << std::endl;
  kvs::PolygonObject* object = new kvs::Isosurface( q_volume, iso_level, n, d, tfunc );
  object->setName("Surface");
  object->setMinMaxObjectCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
  object->setMinMaxExternalCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
  object->multiplyXform( kvs::Xform::Rotation( R ) * kvs::Xform::Scaling( 3.0 ) );
  object->print( std::cout );
  screen.registerObject( object, renderer );
  screen.registerObject( poly_object, poly_renderer);
      
  //screen.draw();
  std::cout << "draw " << std::endl;
  rendering_compositor.update();
      
  kvs::ValueArray<kvs::UInt8> color_buffer = screen.readbackColorBuffer();
  kvs::ValueArray<kvs::Real32> depth_buffer = screen.readbackDepthBuffer();
  compositor.run( color_buffer, depth_buffer );
  
  for ( size_t j = 0; j < npixels; j++ )
    {
      ensemble_buffer[ 3 * j + 0 ] = kvs::Real32( color_buffer[ 4 * j + 0 ] );
      ensemble_buffer[ 3 * j + 1 ] = kvs::Real32( color_buffer[ 4 * j + 1 ] );
      ensemble_buffer[ 3 * j + 2 ] = kvs::Real32( color_buffer[ 4 * j + 2 ] );
    }
    
      
  //delete volume;
  //delete q_volume;
  //delete poly_object;
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
      std::string name = "./output_result_qriterion_isosurface_" +num +".bmp";
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

void CalculateValues( float& min_value, float& max_value )
{
  float recv_min_value = FLT_MAX;
  float recv_max_value = FLT_MIN;

  MPI_Allreduce( &min_value, &recv_min_value, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
  MPI_Allreduce( &max_value, &recv_max_value, 1, MPI_FLOAT, MPI_MIN, MPI_COMM_WORLD );

  min_value = recv_min_value;
  max_value = recv_max_value;

}
