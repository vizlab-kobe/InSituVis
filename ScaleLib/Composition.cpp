#include "Composition.h"
#include <ParallelImageComposition/Lib/ImageCompositor.h>
#include <mpi.h>
#include <cfloat>
kvs::ColorImage Composition( kvs::osmesa::Screen& screen, int rank, int nnodes )
{
  
  ParallelImageComposition::ImageCompositor compositor( rank, nnodes, MPI_COMM_WORLD );
  const size_t width = screen.width();
  const size_t height = screen.height();
  const size_t npixels = width * height;
  const bool depth_testing = true;

  screen.draw();
  compositor.initialize( width, height, depth_testing );  
  kvs::ValueArray<kvs::UInt8> color_buffer = screen.readbackColorBuffer();
  kvs::ValueArray<kvs::Real32> depth_buffer = screen.readbackDepthBuffer();

  compositor.run( color_buffer, depth_buffer );

  kvs::ValueArray<kvs::UInt8> pixels( npixels * 3 );
  pixels.fill( 0 );
  if( rank == 0 )
    {
      for ( size_t i = 0; i < npixels; i++ )
	{
	  pixels[ 3 * i + 0 ] = kvs::Math::Clamp( kvs::Math::Round( color_buffer[ 4 * i + 0 ] ), 0, 255 );
	  pixels[ 3 * i + 1 ] = kvs::Math::Clamp( kvs::Math::Round( color_buffer[ 4 * i + 1 ] ), 0, 255 );
	  pixels[ 3 * i + 2 ] = kvs::Math::Clamp( kvs::Math::Round( color_buffer[ 4 * i + 2 ] ), 0, 255 );
	}	
    }
 
  return kvs::ColorImage( width, height, pixels );

}

void CalculateMinMax( double& min_x, double& min_y, double& min_z, double& max_x, double& max_y, double& max_z, double& min_value, double& max_value ) 
{
  
  double recv_min_x = FLT_MAX;
  double recv_min_y = FLT_MAX;
  double recv_min_z = FLT_MAX;
  double recv_max_x = FLT_MIN;
  double recv_max_y = FLT_MIN;
  double recv_max_z = FLT_MIN;
  double recv_min_value = FLT_MAX;
  double recv_max_value = FLT_MIN;
  MPI_Allreduce( &min_x, &recv_min_x, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD );  
  MPI_Allreduce( &min_y, &recv_min_y, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD );  
  MPI_Allreduce( &min_z, &recv_min_z, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD );  
  MPI_Allreduce( &max_x, &recv_max_x, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD );  
  MPI_Allreduce( &max_y, &recv_max_y, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD );
  MPI_Allreduce( &max_z, &recv_max_z, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD );  
  MPI_Allreduce( &min_value, &recv_min_value, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD );  
  MPI_Allreduce( &max_value, &recv_max_value, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD );  
  min_x = recv_min_x;
  min_y = recv_min_y;
  min_z = recv_min_z;
  max_x = recv_max_x;
  max_y = recv_max_y;
  max_z = recv_max_z;
  min_value = recv_min_value;
  max_value = recv_max_value;

}
