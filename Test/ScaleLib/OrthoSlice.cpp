#include "OrthoSlice.h"
#include "Composition.h"
#include <kvs/StructuredVolumeObject>
#include <kvs/PolygonObject>
#include <kvs/OrthoSlice>
#include <kvs/Bounds>
#include <mpi.h>

extern "C" void OrthoSlice( double values[], int size, int dimx, int dimy, int dimz, double minx, double miny, double minz, double maxx, double maxy, double maxz, int time)
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

    kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeObject();
    volume->setGridTypeToUniform();
    volume->setVeclen( 1 );
    volume->setResolution( kvs::Vec3u( dimx, dimy, dimz ) );
    volume->setValues( kvs::ValueArray<double>( values, size )  );
    volume->setMinMaxValues( min_value, max_value );
    
    const float p = volume->resolution().z() * 0.5f;
    const kvs::OrthoSlice::AlignedAxis a = kvs::OrthoSlice::ZAxis;
    const kvs::TransferFunction t( 256 );
    kvs::PolygonObject* object = new kvs::OrthoSlice( volume, p, a, t );
    delete volume;
    kvs::ObjectBase* dummy = new kvs::ObjectBase();

    dummy->setMinMaxExternalCoords( min_coords, max_coords );    
    object->setMinMaxExternalCoords( kvs::Vec3( minx, miny, minz ), kvs::Vec3( maxx, maxy, maxz ) );

    const kvs::Mat3 R = kvs::Mat3::RotationX( 30 ) * kvs::Mat3::RotationY( 30 );
    object->multiplyXform( kvs::Xform::Rotation( R ) );
    dummy->multiplyXform( kvs::Xform::Rotation( R ) );

    kvs::osmesa::Screen screen;   
    screen.registerObject( dummy, new kvs::Bounds() );
    screen.registerObject( object );
   
    kvs::ColorImage image = Composition( screen, rank, nnodes ); 

    if ( rank == 0 )
      {
	std::string num = kvs::String::ToString( time );
	std::string name = "output_result_orthoslice_" +num +".bmp";
	image.write( name );
      }
}
