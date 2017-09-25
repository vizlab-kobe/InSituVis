#include "View.h"
#include <kvs/Font>
#include <InSituVis/Lib/MovieObject.h>
#include <InSituVis/Lib/MovieRenderer.h>
#include <InSituVis/Lib/SphericalMapMovieRenderer.h>

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
    m_undistorted_movie_screen( app ),
    m_distorted_movie_info( &m_distorted_movie_screen ),
    m_undistorted_movie_info( &m_undistorted_movie_screen )

{
    this->setup();
    this->layout();
    this->show();
}

void View::setup()
{
    const size_t width = 512;
    const size_t height = 512;
    const size_t movie_width = m_model->objectPointer()->width();
    const size_t movie_height = m_model->objectPointer()->height();
    const float scale = float( movie_width ) / movie_height;

    m_distorted_movie_screen.setTitle( "Source Movie" );
    m_distorted_movie_screen.setSize( width * scale, height );
    {
        typedef InSituVis::MovieRenderer Renderer;
        m_distorted_movie_screen.registerObject( m_model->object(), new Renderer() );
        m_distorted_movie_screen.addEvent( new IdleEvent() );
    }

    m_undistorted_movie_screen.setTitle( "Cropped Movie" );
    m_undistorted_movie_screen.setSize( width, height );
    {
        typedef InSituVis::SphericalMapMovieRenderer Renderer;
        m_undistorted_movie_screen.registerObject( m_model->object(), new Renderer() );
        m_undistorted_movie_screen.addEvent( new IdleEvent() );
    }

    kvs::Font font;
    font.setSize( 24 );
    font.setFamilyToSans();
    font.setStyleToBold();
    font.setColor( kvs::RGBColor::White() );
    font.setShadowColor( kvs::RGBColor::Black() );
    font.setEnabledShadow( true );

    m_distorted_movie_info.setFont( font );
    m_distorted_movie_info.setMargin( 10 );
    m_distorted_movie_info.setText( "Filename: %s", m_model->filename().c_str() );
    m_distorted_movie_info.addText( "Resolution: %d x %d", movie_width, movie_height );

    m_undistorted_movie_info.setFont( font );
    m_undistorted_movie_info.setMargin( 10 );
    m_undistorted_movie_info.setText( "Filename: %s", m_model->filename().c_str() );
    m_undistorted_movie_info.addText( "Resolution: %d x %d", width, height );
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

    m_distorted_movie_info.show();
    m_undistorted_movie_info.show();
}

} // end of namespace local
