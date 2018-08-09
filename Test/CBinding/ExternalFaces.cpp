#include "ExternalFaces.h"
#include <kvs/StructuredVolumeObject>
#include <kvs/PolygonObject>
#include <kvs/ExternalFaces>
#include <kvs/ColorImage>
#include <kvs/osmesa/Screen>


extern "C" void ExternalFaces( float values[], int size, int dimx, int dimy, int dimz )
{
    kvs::StructuredVolumeObject* volume = new kvs::StructuredVolumeObject();
    volume->setGridTypeToUniform();
    volume->setVeclen( 1 );
    volume->setResolution( kvs::Vec3u( dimx, dimy, dimz ) );
    volume->setValues( kvs::ValueArray<float>( values, size )  );

    kvs::PolygonObject* object = new kvs::ExternalFaces( volume );
    delete volume;

    const kvs::Mat3 R = kvs::Mat3::RotationX( 30 ) * kvs::Mat3::RotationY( 30 );
    object->multiplyXform( kvs::Xform::Rotation( R ) );

    kvs::osmesa::Screen screen;
    screen.registerObject( object );
    screen.draw();

    kvs::ColorImage image = screen.capture();
    image.write( "output.bmp" );
}
