#include "MovieScreen.h"
#include <InSituVis/Lib/SphericalMapMovieRenderer.h>


namespace local
{

MovieScreen::MovieScreen( local::Application* app ):
    local::Screen( app ),
    m_info( this )
{
    this->setTitle( "4D Street Viewer" );
}

void MovieScreen::setup( local::Model* model )
{
    const size_t width = 512;
    const size_t height = 512;
    this->setSize( width, height );
    {
        typedef InSituVis::MovieObject Object;
        typedef InSituVis::SphericalMapMovieRenderer Renderer;

        Object* object = model->object();
        object->setName("Object");

        Renderer* renderer = new Renderer();
        renderer->setName("Renderer");
        renderer->setEnabledAutoPlay( false );
        renderer->setEnabledLoopPlay( false );

        this->registerObject( object, renderer );
    }

    m_info.setup( model );
}

void MovieScreen::update( local::Model* model )
{
    typedef InSituVis::MovieObject Object;
    const int index = static_cast<Object*>( scene()->object("Object") )->device().nextFrameIndex();

    Object* object = model->object();
    object->setName("Object");
    object->device().setNextFrameIndex( index );

    scene()->replaceObject( "Object", object );
}

void MovieScreen::show()
{
    local::Screen::show();
    m_info.show();
}

} // end of namespace local
