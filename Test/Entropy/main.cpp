#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <kvs/StructuredVolumeObject>
#include <kvs/StructuredVolumeImporter>
#include <kvs/PolygonObject>
#include <kvs/Isosurface>
#include <kvs/HydrogenVolumeData>
#include <kvs/Timer>
#include <kvs/OffScreen>
#include <kvs/GrayImage>
#include <kvs/ValueArray>

float DepthEntropy( const size_t width, const size_t height, const kvs::ValueArray<float>& buffer ){
    float entropy = 0.0;

    for( size_t i = 0; i < buffer.size(); i++ ){
        entropy -= buffer[i] * log( buffer[i] );
    }

    return entropy;
}


int main( int argc, char** argv )
{
    std::cout << "OSMesa version: " << kvs::osmesa::Version() << std::endl;

    auto* volume = new kvs::HydrogenVolumeData( { 64, 64, 64 } );

    // Extract isosurfaces as polygon object.
    const auto i = ( volume->maxValue() + volume->minValue() ) * 0.5; // isolevel
    const auto n = kvs::PolygonObject::VertexNormal; // normal type
    const auto d = false; // false: duplicated vertices will be removed
    const auto t = kvs::TransferFunction( 256 ); // transfer function
    auto* object = new kvs::Isosurface( volume, i, n, d, t );
    delete volume;

    kvs::OffScreen screen;
    screen.setSize( 512, 512 );
    screen.registerObject( object );

    kvs::Timer timer( kvs::Timer::Start );
    for ( size_t i = 0; i < 12; i++ )
    {
        std::stringstream num; num << std::setw(3) << std::setfill('0') << i;
        std::string filename = "output_" + num.str() + ".bmp";

        std::cout << "rendering to ... " << std::flush;
        auto R = kvs::Xform::Rotation( kvs::Mat3::RotationY( 30 ) );
        object->multiplyXform( R );
        screen.draw();
        auto width = screen.width();
        auto height = screen.height();
        auto depth_buffer = screen.readbackDepthBuffer();
        auto entropy = DepthEntropy( width, height, depth_buffer );

        kvs::GrayImage depth_image( width, height, depth_buffer );
        depth_image.write( "output_depth_" + num.str() + ".bmp" );
        //screen.capture().write( filename );
        std::cout << filename << "   entropy : " << entropy << std::endl;
    }
    timer.stop();

    std::cout << "Total:   " << timer.sec() << " [sec]" << std::endl;
    std::cout << "Average: " << timer.sec() / 12.0f << " [sec]" << std::endl;

    return 0;
}
