#include <kvs/glut/Application>
#include <kvs/glut/Screen>
#include <kvs/ImageObject>
#include <kvs/ImageRenderer>
#include <kvs/ImageImporter>
#include <kvs/ShaderSource>
#include <InSituVis/Lib/SphericalMapRenderer.h>
#include "MovieObject.h"
#include "MovieRenderer.h"
#include "SphericalMapMovieRenderer.h"
#include <kvs/IdleEventListener>


class IdleEvent : public kvs::IdleEventListener
{
    void update() { screen()->redraw(); }
};

int main( int argc, char** argv )
{
    kvs::ShaderSource::AddSearchPath("../../Lib");

    kvs::glut::Application app( argc, argv );

    local::opencv::MovieObject* object = new local::opencv::MovieObject( argv[1] );
//    local::opencv::MovieRenderer* renderer = new local::opencv::MovieRenderer();
    local::opencv::SphericalMapMovieRenderer* renderer = new local::opencv::SphericalMapMovieRenderer();

//    const size_t width = object->device().frameWidth();
//    const size_t height = object->device().frameHeight();

    kvs::glut::Screen screen( &app );
//    screen.setSize( width, height );
    screen.registerObject( object, renderer );
    screen.addEvent( new IdleEvent() );
    screen.show();

    return app.run();
}
