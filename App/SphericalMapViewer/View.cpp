#include "View.h"
#include <kvs/ImageObject>
#include <kvs/ImageRenderer>
#include <InSituVis/Lib/SphericalMapRenderer.h>

namespace local
{

View::View( local::Application* app, local::Model* model ):
    m_model( model ),
    m_distorted_image_screen( app ),
    m_undistorted_image_screen( app )
{
    this->setup();
    this->layout();
    this->show();
}

void View::setup()
{
    m_distorted_image_screen.setTitle( "Equirectanglar image" );
    {
        typedef kvs::ImageRenderer Renderer;
        m_distorted_image_screen.registerObject( m_model->object(), new Renderer() );
    }

    m_undistorted_image_screen.setTitle( "Undistorted image" );
    {
        typedef InSituVis::SphericalMapRenderer Renderer;
        m_undistorted_image_screen.registerObject( m_model->object(), new Renderer() );
    }
}

void View::layout()
{
    // Top-left on the desktop.
    {
        const size_t x = 0;
        const size_t y = 0;
        m_distorted_image_screen.setPosition( x, y );
    }

    // Right side of the distorted image screen.
    {
        const size_t x = m_distorted_image_screen.x() + m_distorted_image_screen.width();
        const size_t y = m_distorted_image_screen.y();
        m_undistorted_image_screen.setPosition( x, y );
    }
}

void View::show()
{
    m_distorted_image_screen.show();
    m_undistorted_image_screen.show();
}

} // end of namespace local
