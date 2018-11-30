#include "ParticleBasedRendering.h"
#include <kvs/StructuredVolumeObject>
#include <kvs/PointObject>
#include <kvs/ColorImage>
#include <kvs/CellByCellMetropolisSampling>
#include <kvs/ParticleBasedRenderer>
#include <kvs/Bounds>
#include <mpi.h>
#include <ParallelImageComposition/Lib/ImageCompositor.h>
#include <kvs/osmesa/Screen>
#include "Composition.h"
extern "C" void ParticleBasedRendering( double values[], int size, int dimx, int dimy, int dimz, double x_coords[], double y_coords[], double z_coords[], int time )
{
    int rank, nnodes;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nnodes);
  
    double min_value = values[0];
    double max_value = values[0];
    double global_minx = x_coords[0];
    double global_miny = y_coords[0];
    double global_minz = z_coords[0];
    double global_maxx = x_coords[dimx - 1];
    double global_maxy = y_coords[dimy - 1];
    double global_maxz = z_coords[dimz - 1];
    double local_minx = global_minx;
    double local_miny = global_miny;
    double local_minz = global_minz;
    double local_maxx = global_maxx;
    double local_maxy = global_maxy;
    double local_maxz = global_maxz;

    for( int i = 0; i < size; i++ )
      {	
	if( min_value > values[i] )
	  min_value = values[i];
	if( max_value < values[i] )
	  max_value = values[i];
      }
    CalculateMinMax( global_minx, global_miny, global_minz, global_maxx, global_maxy, global_maxz, min_value, max_value );

    min_value = -14;
    max_value = 17;
    
    kvs::Vec3 min_coords( global_minx, global_miny, global_minz );
    kvs::Vec3 max_coords( global_maxx, global_maxy, global_maxz );

    /*
    kvs::ValueArray<float> coords( dimx * dimy * dimz * 3 );
    coords.fill(0);
    int index = 0;
    for( int k = 0; k < dimz; k++ )
      {
	for ( int j = 0 ; j < dimy; j++ )
	  {
	    for( int i =0 ; i < dimx; i++ )
	      {
		int num = (i + dimx * j + ( dimx * dimy * k ) );
		coords [ index * 3 + 0 ] = x_coords[ i ];
		coords [ index * 3 + 1 ] = y_coords[ j ];
		coords [ index * 3 + 2 ] = z_coords[ k ];  
		index++;
	      }
	  }
      }
    */

    kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeObject();
    volume->setGridTypeToUniform();
    //volume->setGridTypeToRectilinear();
    volume->setVeclen( 1 );
    volume->setResolution( kvs::Vec3u( dimx, dimy, dimz ) );
    volume->setValues( kvs::ValueArray<double>( values, size )  );
    volume->setMinMaxValues( min_value, max_value );
    //volume->setCoords( coords );
    
    const size_t repetitions = 10;
    const float step = 0.5f;
    const kvs::TransferFunction tfunc( 256 );
    
    const size_t width = 512;
    const size_t height = 512;
    const size_t npixels = width * height;
    const bool depth_testing = true;
    kvs::ValueArray<kvs::Real32> ensemble_buffer( npixels * 3 );
    ensemble_buffer.fill( 0.0f );
    ParallelImageComposition::ImageCompositor compositor( rank, nnodes, MPI_COMM_WORLD );
    compositor.initialize( width, height, depth_testing );


    for( size_t i = 0; i < repetitions; i++)
      {
	kvs::PointObject* object = new kvs::CellByCellMetropolisSampling( volume, 1, step, tfunc );
	kvs::ParticleBasedRenderer* renderer = new kvs::ParticleBasedRenderer();

	kvs::ObjectBase* dummy = new kvs::ObjectBase();
	dummy->setMinMaxExternalCoords( min_coords, max_coords );    
	object->setMinMaxExternalCoords( kvs::Vec3( local_minx, local_miny, local_minz ), kvs::Vec3( local_maxx, local_maxy, local_maxz ) );

    
	const kvs::Mat3 R = kvs::Mat3::RotationX( 30 ) * kvs::Mat3::RotationY( 30 );
	object->multiplyXform( kvs::Xform::Rotation( R ) );
	dummy->multiplyXform( kvs::Xform::Rotation( R ) );

	kvs::osmesa::Screen screen;   
	screen.registerObject( dummy, new kvs::Bounds() );
	screen.registerObject( object, renderer );

	screen.draw(); 
	
	kvs::ValueArray<kvs::UInt8> color_buffer = screen.readbackColorBuffer();
	kvs::ValueArray<kvs::Real32> depth_buffer = screen.readbackDepthBuffer();

	compositor.run( color_buffer, depth_buffer );
	const float a = 1.0f / ( i + 1 );

	for ( int j = 0; j < npixels; j++ )
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
	std::string name = "./Output/output_result_pbvr" +num +".bmp";
	image.write( name );
      }
}
