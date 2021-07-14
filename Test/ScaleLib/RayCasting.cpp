#include "ExternalFaces.h"
#include "Composition.h"
#include <kvs/StructuredVolumeObject>
#include <kvs/RayCastingRenderer>
#include <kvs/Bounds>
#include <mpi.h>

extern "C" void RayCasting( double values[], int size, int dimx, int dimy, int dimz, double minx, double miny, double minz, double maxx, double maxy, double maxz, int time )
{
    int rank, nnodes;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nnodes);

    double min_value = values[0];
    double max_value = values[0];
    double global_minx = minx;
    double global_miny = miny;
    double global_minz = minz;
    double global_maxx = maxx;
    double global_maxy = maxy;
    double global_maxz = maxz;
    for( int i = 0; i < size; i++ )
      {
        if( min_value > values[i] )
          min_value = values[i];
        if( max_value < values[i] )
          max_value = values[i];
      }

    CalculateMinMax( global_minx, global_miny, global_minz, global_maxx, global_maxy, global_maxz, min_value, max_value );

    kvs::Vec3 min_coords( global_minx, global_miny, global_minz );
    kvs::Vec3 max_coords( global_maxx, global_maxy, global_maxz );

    kvs::StructuredVolumeObject* object = new kvs::StructuredVolumeObject();
    object->setGridTypeToUniform();
    object->setVeclen( 1 );
    object->setResolution( kvs::Vec3u( dimx, dimy, dimz ) );
    object->setValues( kvs::ValueArray<double>( values, size )  );
    object->setMinMaxValues( min_value, max_value );
    
    kvs::Real32 sampling_step = 0.5f;
    kvs::Real32 opaque_value = 0.97f;
    kvs::TransferFunction tfunc( 256 );
    tfunc.setRange( min_value, max_value );
    
    kvs::RayCastingRenderer* renderer = new kvs::RayCastingRenderer(); 
    renderer->setSamplingStep( sampling_step );
    renderer->setOpaqueValue( opaque_value );
    renderer->setTransferFunction( tfunc );


    kvs::ObjectBase* dummy = new kvs::ObjectBase();
    dummy->setMinMaxExternalCoords( min_coords, max_coords );    
    object->setMinMaxObjectCoords( kvs::Vec3( minx, miny, minz ), kvs::Vec3( maxx, maxy, maxz ) );
    object->setMinMaxExternalCoords( kvs::Vec3( minx, miny, minz ), kvs::Vec3( maxx, maxy, maxz ) );
    
    const kvs::Mat3 R = kvs::Mat3::RotationX( 30 ) * kvs::Mat3::RotationY( 30 );
    object->multiplyXform( kvs::Xform::Rotation( R ) );
    dummy->multiplyXform( kvs::Xform::Rotation( R ) );

    kvs::osmesa::Screen screen;   
    screen.registerObject( dummy, new kvs::Bounds() );
    screen.registerObject( object, renderer );

    screen.draw();
    kvs::ValueArray<kvs::UInt8> color_buffer = screen.readbackColorBuffer();
    kvs::ValueArray<kvs::Real32> depth_buffer = screen.readbackDepthBuffer();

    kvs::ValueArray<kvs::UInt8> pixels( depth_buffer.size() * 3 );

    for ( size_t i = 0; i < depth_buffer.size(); i++ )
      {
	pixels[ 3 * i + 0 ] = color_buffer[ 4 * i + 0 ];
	pixels[ 3 * i + 1 ] = color_buffer[ 4 * i + 1 ];
	pixels[ 3 * i + 2 ] = color_buffer[ 4 * i + 2 ];
      }
    kvs::ColorImage tmp_image = kvs::ColorImage( 512, 512, pixels );
    
    std::string rank_num = kvs::String::ToString( rank );
    std::string num = kvs::String::ToString( time );
    
    std::string tmp_name = "output_raycasting" + num + "_rank_"+ rank_num + ".bmp";
    tmp_image.write( tmp_name );

    
    kvs::ColorImage image = Composition( screen, rank, nnodes );
 
    if( rank == 0 )
      {	
	std::string name = "output_result_raycasting_" +num +".bmp";
	image.write( name );
      }
}
