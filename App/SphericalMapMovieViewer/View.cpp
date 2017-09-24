#include "View.h"
#include "MovieObject.h"
#include "MovieRenderer.h"
#include "SphericalMapMovieRenderer.h"

#include <kvs/IdleEventListener>
class IdleEvent : public kvs::IdleEventListener
{
    void update() { screen()->redraw(); }
};

namespace local
{

View::View( local::Application* app, local::Model* model ):
    m_model( model ),
    m_distorted_movie_screen( app ),
    m_undistorted_movie_screen( app )
{
    this->setup();
    this->layout();
    this->show();
}

void View::setup()
{
    m_distorted_movie_screen.setTitle( "Equirectanglar movie" );
    {
        typedef local::opencv::MovieRenderer Renderer;
        m_distorted_movie_screen.registerObject( m_model->object(), new Renderer() );
        m_distorted_movie_screen.addEvent( new IdleEvent() );
    }

    m_undistorted_movie_screen.setTitle( "Undistorted movie" );
    {
        typedef local::opencv::SphericalMapMovieRenderer Renderer;
        m_undistorted_movie_screen.registerObject( m_model->object(), new Renderer() );
        m_undistorted_movie_screen.addEvent( new IdleEvent() );
    }
}

void View::layout()
{
    // Top-left on the desktop.
    {
        const size_t x = 0;
        const size_t y = 0;
        m_distorted_movie_screen.setPosition( x, y );
    }

    // Right side of the distorted movie screen.
    {
        const size_t x = m_distorted_movie_screen.x() + m_distorted_movie_screen.width();
        const size_t y = m_distorted_movie_screen.y();
        m_undistorted_movie_screen.setPosition( x, y );
    }
}

void View::show()
{
    m_distorted_movie_screen.show();
    m_undistorted_movie_screen.show();
}

} // end of namespace local
