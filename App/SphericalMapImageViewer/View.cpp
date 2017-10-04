#include "View.h"
#include <kvs/Font>
#include <kvs/ImageObject>
#include <kvs/ImageRenderer>
#include <InSituVis/Lib/SphericalMapImageRenderer.h>

namespace local
{

View::View( local::Application* app, local::Model* model ):
    m_model( model ),
    m_distorted_image_screen( app ),
    m_undistorted_image_screen( app ),
    m_distorted_image_info( &m_distorted_image_screen ),
    m_undistorted_image_info( &m_undistorted_image_screen, model )
{
    this->setup();
    this->layout();
    this->show();
}

void View::setup()
{
    const size_t width = 512;
    const size_t height = 512;
    const size_t image_width = m_model->objectPointer()->width();
    const size_t image_height = m_model->objectPointer()->height();
    const float scale = float( image_width ) / image_height;

    m_distorted_image_screen.setTitle( "Source Image" );
    m_distorted_image_screen.setSize( width * scale, height );
    {
        typedef kvs::ImageRenderer Renderer;
        m_distorted_image_screen.registerObject( m_model->object(), new Renderer() );
    }

    m_undistorted_image_screen.setTitle( "Cropped Image" );
    m_undistorted_image_screen.setSize( width, height );
    {
        typedef InSituVis::SphericalMapImageRenderer Renderer;
        m_undistorted_image_screen.registerObject( m_model->object(), new Renderer() );
    }

    kvs::Font font;
    font.setSize( 24 );
    font.setFamilyToSans();
    font.setStyleToBold();
    font.setColor( kvs::RGBColor::White() );
    font.setShadowColor( kvs::RGBColor::Black() );
    font.setEnabledShadow( true );

    m_distorted_image_info.setFont( font );
    m_distorted_image_info.setMargin( 10 );
    m_distorted_image_info.setText( "Filename: %s", m_model->filename().c_str() );
    m_distorted_image_info.addText( "Resolution: %d x %d", image_width, image_height );

//    m_undistorted_image_info.setFont( font );
//    m_undistorted_image_info.setMargin( 10 );
//    m_undistorted_image_info.setText( "Filename: %s", m_model->filename().c_str() );
//    m_undistorted_image_info.addText( "Resolution: %d x %d", width, height );
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

    m_distorted_image_info.show();
    m_undistorted_image_info.show();
}

} // end of namespace local
