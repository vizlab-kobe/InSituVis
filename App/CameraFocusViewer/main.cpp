#include <kvs/Application>
#include <kvs/Screen>
#include <kvs/Directory>
#include <kvs/EventListener>
#include <kvs/Math>
#include <kvs/ffmpeg/MovieObject>
#include <kvs/ffmpeg/MovieRenderer>


int main( int argc, char** argv )
{
    kvs::Application app( argc, argv );
    kvs::Screen screen( &app );

    // Movie files
    const auto dirname = std::string( argv[1] );
    const auto pattern = std::regex(R"(.+\.mp4)");
    const auto files = kvs::Directory( dirname ).fileList( pattern );
    const auto nfiles = int( files.size() );

    // Zoom level
    auto zoom_level = 0;
    auto zoom_in = [&]() { return kvs::Math::Min( zoom_level + 1, nfiles - 1 ); };
    auto zoom_out = [&]() { return kvs::Math::Max( zoom_level - 1, 0 ); };

    // Movie object and renderer.
    using Object = kvs::ffmpeg::MovieObject;
    using Renderer = kvs::ffmpeg::MovieRenderer;
    auto* object = new Object( files[ zoom_level ].filePath() );
    auto* renderer = new Renderer();

    // Create screen.
    auto width = object->width();
    auto height = object->height();
    screen.setSize( width, height );
    screen.create();

    // Disable default mouse and wheel events.
    screen.setEvent( new kvs::InteractorBase );

    // Key-press and timer events.
    const int interval = 1000 / object->frameRate(); // msec
    kvs::EventListener event;
    event.resizeEvent( [&]( int, int )
    {
        if ( renderer->isPlaying() ) { renderer->pause(); }
    } );
    event.keyPressEvent( [&]( kvs::KeyEvent* e )
    {
        auto replace_object = [&] ( const int zl )
        {
            auto* o = new Object( files[ zl ].filePath() );
            o->jumpToFrame( object->currentFrameIndex() );
            screen.scene()->replaceObject( 1, o );
            object = o;
            screen.redraw();
        };

        switch ( e->key() )
        {
        case kvs::Key::s:
        {
            renderer->stop();
            break;
        }
        case kvs::Key::Up:
        {
            const auto is_playing = renderer->isPlaying();
            if ( is_playing ) { renderer->pause(); }
            replace_object( zoom_level = zoom_in() );
            if ( is_playing ) { renderer->play(); }
            break;
        }
        case kvs::Key::Down:
        {
            const auto is_playing = renderer->isPlaying();
            if ( is_playing ) { renderer->pause(); }
            replace_object( zoom_level = zoom_out() );
            if ( is_playing ) { renderer->play(); }
            break;
        }
        case kvs::Key::Space:
        {
            if ( renderer->isPaused() ) { renderer->play(); }
            else if ( renderer->isPlaying() ) { renderer->pause(); }
            else { renderer->play(); }
            break;
        }
        default: break;
        }
    } );
    event.timerEvent( [&]( kvs::TimeEvent* e )
    {
        if ( renderer->isPlaying() ) { screen.redraw(); }
    }, interval );
    screen.addEvent( &event );

    screen.registerObject( object, renderer );
    screen.show();

    return app.run();
}
