#include "MovieScreen.h"
#include <InSituVis/Lib/SphericalMapMovieRenderer.h>


#include <kvs/IdleEventListener>
class IdleEvent : public kvs::IdleEventListener
{
    void update() { screen()->redraw(); }
};


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

    const size_t movie_width = model->objectPointer()->width();
    const size_t movie_height = model->objectPointer()->height();
//    const float scale = float( movie_width ) / movie_height;

    this->setSize( width, height );
    {
        typedef InSituVis::MovieObject Object;
        typedef InSituVis::SphericalMapMovieRenderer Renderer;

        Object* object = model->object();
        object->setName("Object");

        this->registerObject( object, new Renderer() );
        this->addEvent( new IdleEvent() );
    }

    m_info.setup( model );
}

void MovieScreen::update( local::Model* model )
{
    typedef InSituVis::MovieObject Object;
    Object* object = model->object();
    object->setName("Object");

    scene()->replaceObject( "Object", object );
}

void MovieScreen::show()
{
    local::Screen::show();
    m_info.show();
}

} // end of namespace local
