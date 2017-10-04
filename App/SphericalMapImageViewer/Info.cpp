#include "Info.h"
#include <kvs/Font>
#include <kvs/RGBColor>
#include <kvs/Camera>

namespace local
{

Info::Info( Screen* screen, local::Model* model ):
    kvs::Label( screen ),
    m_screen( screen ),
    m_model( model )
{
    kvs::Font font;
    font.setSize( 24 );
    font.setFamilyToSans();
    font.setStyleToBold();
    font.setColor( kvs::RGBColor::White() );
    font.setShadowColor( kvs::RGBColor::Black() );
    font.setEnabledShadow( true );

    setMargin( 10 );
    setFont( font );
}

void Info::screenUpdated()
{
    const size_t width = m_screen->width();
    const size_t height = m_screen->height();
    const kvs::Vec3 pos = kvs::Vec3( 0, 0, 0 );//m_screen->scene()->camera()->position();
    const kvs::Vec3 at = kvs::Vec3( 0, 0, -1 );//m_screen->scene()->camera()->lookAt();
    const kvs::Vec3 dir = ( at - pos ) * m_screen->scene()->object()->xform().rotation().inverted();

    setText( "Filename: %s", m_model->filename().c_str() );
    addText( "Resolution: %d x %d", width, height );
    addText( "Camera position: (%.1f, %.1f, %.1f)", pos.x(), pos.y(), pos.z() );
    addText( "Camera direction: (%.1f, %.1f, %.1f)", dir.x(), dir.y(), dir.z() );
}

} // end of namespace local
