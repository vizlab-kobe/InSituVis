#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <string>
#include <cmath>
#include <kvs/Light>
#include <kvs/Camera>
#include <kvs/StructuredVolumeObject>
#include <kvs/StructuredVolumeImporter>
#include <kvs/PolygonObject>
#include <kvs/PolygonRenderer>
#include <kvs/OrthoSlice>
#include <kvs/Isosurface>
#include <kvs/HydrogenVolumeData>
#include <kvs/Timer>
#include <kvs/OffScreen>
#include <kvs/GrayImage>
#include <kvs/Math>
#include <kvs/ValueArray>
#include "../../Lib/Viewpoint.h"
#include "../../Lib/SphericalViewpoint.h"
#include "../../Lib/CubicViewpoint.h"

float DepthEntropy( const size_t width, const size_t height, const kvs::ValueArray<float>& buffer )
{
    const float mean = 0.5;
    const float sigma = 1.0 / 6.0;
    const float sigma2 = sigma * sigma;
    const float dx = 0.0001;

    float entropy = 0.0;
    for( size_t i = 0; i < width * height; i++ ){
        const float p = dx * exp( -1 * ( buffer[i] - mean ) * ( buffer[i] - mean ) / ( 2 * sigma2 ) ) / sqrt( 2 * kvs::Math::pi * sigma2 );
        if( buffer[i] < 1 ){
            entropy -= p * log( p ) / log( 2.0 );
        }
    }

    return entropy;
}

float ColorEntropy( const size_t width, const size_t height, const kvs::ValueArray<unsigned char>& buffer ){
    const float mean = 128.0;
    const float sigma = 128.0 / 6.0;
    const float sigma2 = sigma * sigma;
    const float dx = 0.0001;

    float entropy = 0.0;
    for( size_t i = 0; i < width * height; i++ ){
        const float p = dx * exp( -1 * ( buffer[i] - mean ) * ( buffer[i] - mean ) / ( 2 * sigma2 ) ) / sqrt( 2 * kvs::Math::pi * sigma2 );
        
        entropy -= p * log( p ) / log( 2.0 );

    }

    return entropy;
}

void RegistOrthoSlice( kvs::OffScreen& screen, kvs::HydrogenVolumeData* volume ){
    if ( volume->numberOfCells() == 0 ) { return; }
    
    const auto* mesh = kvs::PolygonObject::DownCast( screen.scene()->object( "BoundaryMesh" ) );
    if ( mesh )
    {
        const auto min_coord = mesh->minExternalCoord();
        const auto max_coord = mesh->maxExternalCoord();
        volume->setMinMaxObjectCoords( min_coord, max_coord );
        volume->setMinMaxExternalCoords( min_coord, max_coord );
    }

    // Setup a transfer function.
    const auto min_value = volume->minValue();
    const auto max_value = volume->maxValue();
    //auto t = kvs::TransferFunction( kvs::ColorMap::CoolWarm() );
    auto t = kvs::TransferFunction( kvs::ColorMap::BrewerSpectral() );
    t.setRange( min_value, max_value );

    // Create new slice objects.
    auto p0 = ( volume->minObjectCoord().y() + volume->maxObjectCoord().y() ) * 0.5f;
    auto a0 = kvs::OrthoSlice::YAxis;
    auto* object0 = new kvs::OrthoSlice( volume, p0, a0, t );
    object0->setName( volume->name() + "Object0");

    auto p1 = ( volume->minObjectCoord().z() + volume->maxObjectCoord().z() ) * 0.5f;
    auto a1 = kvs::OrthoSlice::ZAxis;
    auto* object1 = new kvs::OrthoSlice( volume, p1, a1, t );
    object1->setName( volume->name() + "Object1");

    if ( screen.scene()->hasObject( volume->name() + "Object0") )
    {
        // Update the objects.
        screen.scene()->replaceObject( volume->name() + "Object0", object0 );
        screen.scene()->replaceObject( volume->name() + "Object1", object1 );
    }
    else
    {
        // Register the objects with renderer.
        auto* renderer0 = new kvs::glsl::PolygonRenderer();
        auto* renderer1 = new kvs::glsl::PolygonRenderer();
        renderer0->setTwoSideLightingEnabled( true );
        renderer1->setTwoSideLightingEnabled( true );
        screen.registerObject( object0, renderer0 );
        screen.registerObject( object1, renderer1 );
    }
}

void RegistIsosurface( kvs::OffScreen& screen, kvs::HydrogenVolumeData* object ){
    // Extract isosurfaces as polygon object.
    const auto i = ( object->maxValue() + object->minValue() ) * 0.5; // isolevel
    const auto n = kvs::PolygonObject::VertexNormal; // normal type
    const auto d = false; // false: duplicated vertices will be removed
    const auto t = kvs::TransferFunction( 256 ); // transfer function
    auto* object0 = new kvs::Isosurface( object, i, n, d, t );

    screen.registerObject( object0 );
}

int main( int argc, char** argv )
{
    std::cout << "OSMesa version: " << kvs::osmesa::Version() << std::endl;

    std::ofstream writing_file;
    std::string filename = "entropy.txt";
    writing_file.open(filename, std::ios::out);
    std::string writing_text = "entropy";
    writing_file << writing_text << std::endl;

    kvs::OffScreen screen;
    screen.setSize( 512, 512 );
    auto* object = new kvs::HydrogenVolumeData( { 64, 64, 64 } );
    RegistOrthoSlice( screen, object );
    //RegistIsosurface( screen, object );
    
    
    //using Viewpoint = ::InSituVis::CubicViewpoint;
    using Viewpoint = ::InSituVis::SphericalViewpoint;
    auto dims = kvs::Vec3ui( 1, 4, 8 );
    auto dir = Viewpoint::Direction::Uni;
    auto vp = Viewpoint();
    vp.setDims( dims );
    vp.create( dir );
    const auto locations = vp.locations();

    kvs::Timer timer( kvs::Timer::Start );
    for ( size_t i = 0; i < dims[0] * dims[1] * dims[2]; i++ )
    {
        std::stringstream num; num << std::setw(5) << std::setfill('0') << i;
        std::string filename = "output_" + num.str() + ".bmp";

        std::cout << "rendering to ... " << std::flush;

        const auto location = locations[i];
        const auto p = location.position;
        const auto a = location.look_at;
        const auto p_rtp = location.position_rtp;

        auto* camera = screen.scene()->camera();
        auto* light = screen.scene()->light();
            
        // Backup camera and light info.
        const auto p0 = camera->position();
        const auto a0 = camera->lookAt();
        const auto u0 = camera->upVector();
            
        //Draw the scene.
        kvs::Vec3 pp_rtp;
        if( p_rtp[1] > kvs::Math::pi / 2 ){
            pp_rtp = p_rtp - kvs::Vec3( { 0, kvs::Math::pi / 2, 0 } );
        }
        else{
            pp_rtp = p_rtp + kvs::Vec3( { 0, kvs::Math::pi / 2, 0 } );
        }
        const float pp_x = pp_rtp[0] * std::sin( pp_rtp[1] ) * std::sin( pp_rtp[2] );
        const float pp_y = pp_rtp[0] * std::cos( pp_rtp[1] );
        const float pp_z = pp_rtp[0] * std::sin( pp_rtp[1] ) * std::cos( pp_rtp[2] );
        kvs::Vec3 pp;
        if( p_rtp[1] > kvs::Math::pi / 2 ){
            pp = kvs::Vec3( { pp_x, pp_y, pp_z } );
        }
        else{
            pp = -1 * kvs::Vec3( { pp_x, pp_y, pp_z } );
        }
        const auto u = pp;
        camera->setPosition( p, a, u );
        light->setPosition( p );
        screen.draw();
            
        // Restore camera and light info.
        camera->setPosition( p0, a0, u0 );
        light->setPosition( p0 );

        auto width = screen.width();
        auto height = screen.height();
        auto depth_buffer = screen.readbackDepthBuffer();
        auto color_buffer = screen.readbackColorBuffer();
        //auto entropy = DepthEntropy( width, height, depth_buffer );
        //auto entropy = ColorEntropy( width, height, color_buffer );
        auto entropy = DepthEntropy( width, height, depth_buffer ) + ColorEntropy( width, height, color_buffer );




        kvs::GrayImage depth_image( width, height, depth_buffer );
        //depth_image.write( "output_depth_" + num.str() + ".bmp" );
        screen.capture().write( filename );
        std::cout << filename << " | entropy : " << entropy << std::endl;

        std::stringstream en;
        en << entropy;
        writing_text = en.str();
        writing_file << writing_text << std::endl;
    }
    writing_file.close();
    timer.stop();

    std::cout << "Total:   " << timer.sec() << " [sec]" << std::endl;
    std::cout << "Average: " << timer.sec() / 12.0f << " [sec]" << std::endl;

    return 0;
}
