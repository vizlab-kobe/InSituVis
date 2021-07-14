#include "ExternalFaces.h"
#include "Composition.h"
#include <kvs/StructuredVolumeObject>
#include <kvs/PolygonObject>
#include <kvs/ExternalFaces>
#include <kvs/Bounds>
#include <mpi.h>

extern "C" void ExternalFaces( double values[], int size, int dimx, int dimy, int dimz, double x_coords[], double y_coords[], double z_coords[], int time )
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

    kvs::Vec3 min_coords( global_minx, global_miny, global_minz );
    kvs::Vec3 max_coords( global_maxx, global_maxy, global_maxz );

    kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeObject();    
    volume->setGridTypeToUniform();
    //volume->setGridTypeToRectilinear();
    volume->setVeclen( 1 );
    volume->setResolution( kvs::Vec3u( dimx, dimy, dimz ) );
    volume->setValues( kvs::ValueArray<double>( values, size ) );
    volume->setMinMaxValues( min_value, max_value );
    //volume->setCoords( coords );
    
    kvs::TransferFunction t( 256 );
    t.setRange( volume );
    kvs::PolygonObject* object = new kvs::ExternalFaces( volume, t );
    kvs::ObjectBase* dummy = new kvs::ObjectBase();

    dummy->setMinMaxExternalCoords( min_coords, max_coords );    
    object->setMinMaxExternalCoords( kvs::Vec3( local_minx, local_miny, local_minz ), kvs::Vec3( local_maxx, local_maxy, local_maxz ) );
    delete volume;
    
    //const kvs::Mat3 R = kvs::Mat3::RotationX( 90 ) * kvs::Mat3::RotationY( 10 );
    const kvs::Mat3 R = kvs::Mat3::RotationZ( 90 ) * kvs::Mat3::RotationY( 80 );
    object->multiplyXform( kvs::Xform::Rotation( R ) );
    dummy->multiplyXform( kvs::Xform::Rotation( R ) );

    kvs::osmesa::Screen screen;   
    screen.registerObject( dummy, new kvs::Bounds() );
    screen.registerObject( object );
    
    kvs::ColorImage image = Composition( screen, rank, nnodes );
    if( rank == 0 )
      {
	std::ostringstream ss;
	ss << std::setw(5) << std::setfill('0') << time;  
	std::string num = ss.str();
	std::string name = "./Output/output_result_externalfaces_" +num +".bmp";
	image.write( name );
      }
}
