#include "Isosurface_five_p.h"
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

namespace
{

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
}

void Isosurface_five_p( const std::vector<float> &values, int ncells, int nnodes, const std::vector<float> &vertex_coords, const std::vector<float> &cell_coords, const std::vector<int> &label, int time, float min_value, float max_value,   std::string stlpath, float cameraposx, float cameraposy, float cameraposz, const size_t repetitions,float isothr1, float isothr2, float isothr3, float isothr4, float isothr5 )

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

  local::InverseDistanceWeighting<kvs::Real32> idw(nnodes);
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

  const size_t width = 512;
  const size_t height = 512;
  const size_t npixels = width * height;
  const bool depth_testing = true;
  kvs::ValueArray<kvs::Real32> ensemble_buffer( npixels * 3 );
  ensemble_buffer.fill( 0.0f );
  ParallelImageComposition::ImageCompositor compositor( rank, nrank, MPI_COMM_WORLD );
  compositor.initialize( width, height, depth_testing );

  //  const size_t repetitions = 20;
  const float step = 0.5f;

  kvs::OpacityMap omap( 256, min_value, max_value );
  omap.addPoint( min_value, 0.0 );
  omap.addPoint( 99999.0, 0.2 );
  omap.addPoint( 100000.0, 0.3 );
  omap.addPoint( 100001.0, 0.4 );
  omap.addPoint( max_value, 0.0 );
  omap.create();
  kvs::TransferFunction tfunc( omap ); 
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
  
  kvs::PolygonObject* poly_object1 = new kvs::PolygonImporter(stlpath);
  kvs::StochasticPolygonRenderer* poly_renderer1 = new kvs::StochasticPolygonRenderer();

  poly_object1->setOpacity( 30 );
  //poly_object->setMinMaxExternalCoords( poly_object->minExternalCoord()*0.1, poly_object->maxExternalCoord()*0.1 );
  poly_object1->multiplyXform( kvs::Xform::Rotation( R ) * kvs::Xform::Scaling( 1.3 ) );  
  poly_object1->setName("Polygon");


  kvs::StochasticPolygonRenderer* poly_renderer2 = new kvs::StochasticPolygonRenderer();
  kvs::StochasticPolygonRenderer* poly_renderer3 = new kvs::StochasticPolygonRenderer();
  kvs::StochasticPolygonRenderer* poly_renderer4 = new kvs::StochasticPolygonRenderer();
  kvs::StochasticPolygonRenderer* poly_renderer5 = new kvs::StochasticPolygonRenderer();
  kvs::StochasticPolygonRenderer* poly_renderer6 = new kvs::StochasticPolygonRenderer();





  
  for( size_t i = 0; i < repetitions; i++)
    {
      kvs::Camera* camera = new kvs::Camera();
      camera->setWindowSize( width, height );

      kvs::PolygonObject* poly_object2 = new kvs::Isosurface(volume,isothr1,kvs::Isosurface::PolygonNormal);
      poly_object2->setOpacity( 255 );
      kvs::RGBColor color2 = tfunc.colorMap().at(isothr1);
      poly_object2->setColor(color2);
      kvs::PolygonObject* poly_object3 = new kvs::Isosurface(volume,isothr2,kvs::Isosurface::PolygonNormal);
      poly_object3->setOpacity( 255 );
      kvs::RGBColor color3 = tfunc.colorMap().at(isothr2);
      poly_object3->setColor(color3);
      kvs::PolygonObject* poly_object4 = new kvs::Isosurface(volume,isothr3,kvs::Isosurface::PolygonNormal);
      poly_object4->setOpacity( 255 );
      kvs::RGBColor color4 = tfunc.colorMap().at(isothr3);
      poly_object4->setColor(color4);
      kvs::PolygonObject* poly_object5 = new kvs::Isosurface(volume,isothr4,kvs::Isosurface::PolygonNormal);
      poly_object5->setOpacity( 255 );
      kvs::RGBColor color5 = tfunc.colorMap().at(isothr4);
      poly_object5->setColor(color5);
      kvs::PolygonObject* poly_object6 = new kvs::Isosurface(volume,isothr5,kvs::Isosurface::PolygonNormal);
      poly_object6->setOpacity( 255 );
      kvs::RGBColor color6 = tfunc.colorMap().at(isothr5);
      poly_object6->setColor(color6);

      poly_object2->setName("Polygon2");
      poly_object2->setMinMaxObjectCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
      poly_object2->setMinMaxExternalCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
      poly_object2->multiplyXform( kvs::Xform::Rotation( R ) * kvs::Xform::Scaling( 1.3 ) );
      poly_object3->setName("Polygon3");
      poly_object3->setMinMaxObjectCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
      poly_object3->setMinMaxExternalCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
      poly_object3->multiplyXform( kvs::Xform::Rotation( R ) * kvs::Xform::Scaling( 1.3 ) );
      poly_object4->setName("Polygon4");
      poly_object4->setMinMaxObjectCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
      poly_object4->setMinMaxExternalCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
      poly_object4->multiplyXform( kvs::Xform::Rotation( R ) * kvs::Xform::Scaling( 1.3 ) );

      poly_object5->setName("Polygon5");
      poly_object5->setMinMaxObjectCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
      poly_object5->setMinMaxExternalCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
      poly_object5->multiplyXform( kvs::Xform::Rotation( R ) * kvs::Xform::Scaling( 1.3 ) );


      poly_object6->setName("Polygon6");
      poly_object6->setMinMaxObjectCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
      poly_object6->setMinMaxExternalCoords( kvs::Vec3( global_minx, global_miny, global_minz )*1000, kvs::Vec3( global_maxx, global_maxy, global_maxz )*1000 );
      poly_object6->multiplyXform( kvs::Xform::Rotation( R ) * kvs::Xform::Scaling( 1.3 ) );



      kvs::PolygonObject* replace_poly_object1 = new kvs::PolygonObject();
      kvs::PolygonObject* replace_poly_object2 = new kvs::PolygonObject();
      kvs::PolygonObject* replace_poly_object3 = new kvs::PolygonObject();
      kvs::PolygonObject* replace_poly_object4 = new kvs::PolygonObject();
      kvs::PolygonObject* replace_poly_object5 = new kvs::PolygonObject();
      kvs::PolygonObject* replace_poly_object6 = new kvs::PolygonObject();      
      
      replace_poly_object1->deepCopy( *poly_object1 );
      replace_poly_object1->setName("Polygon1");
      replace_poly_object2->deepCopy( *poly_object2 );
      replace_poly_object2->setName("Polygon2");
      replace_poly_object3->deepCopy( *poly_object3 );
      replace_poly_object3->setName("Polygon3");
      replace_poly_object4->deepCopy( *poly_object4 );
      replace_poly_object4->setName("Polygon4");
      replace_poly_object5->deepCopy( *poly_object5 );
      replace_poly_object5->setName("Polygon5");
      replace_poly_object6->deepCopy( *poly_object6 );
      replace_poly_object6->setName("Polygon6");      


      if( i != 0 )
	{
	  screen.scene()->replaceObject( "Polygon1", replace_poly_object1);
	  screen.scene()->replaceObject( "Polygon2", replace_poly_object2);
	  screen.scene()->replaceObject( "Polygon3", replace_poly_object3);
	  screen.scene()->replaceObject( "Polygon4", replace_poly_object4);
	  screen.scene()->replaceObject( "Polygon5", replace_poly_object5);
	  screen.scene()->replaceObject( "Polygon6", replace_poly_object6);
	}
      else
	{

	  screen.registerObject( replace_poly_object1, poly_renderer1);
	  if(poly_object2->numberOfVertices() != 0)
	    screen.registerObject( replace_poly_object2, poly_renderer2);
	  else
	    delete poly_renderer2;
	  if(poly_object3->numberOfVertices() != 0)
	    screen.registerObject( replace_poly_object3, poly_renderer3);
	  else
	    delete poly_renderer3;
	  if(poly_object4->numberOfVertices() != 0)
	    screen.registerObject( replace_poly_object4, poly_renderer4);
	  else
	    delete poly_renderer4;
	  if(poly_object5->numberOfVertices() != 0)
	    screen.registerObject( replace_poly_object5, poly_renderer5);
	  else
	    delete poly_renderer5;
	  if(poly_object6->numberOfVertices() != 0)
	    screen.registerObject( replace_poly_object6, poly_renderer6);
	  else
	    delete poly_renderer6;
			  
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
      
  delete volume;
  delete poly_object1;
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
      std::string name = "./Output/output_result_mix_pbvr_p_" +num +".bmp";
      image.write( name );
    }
  
}
