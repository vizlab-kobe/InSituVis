#include "MovieInfo.h"
#include <kvs/Font>


namespace local
{

MovieInfo::MovieInfo( local::Screen* screen ):
    kvs::Label( screen ),
    m_model( NULL )
{
    kvs::Font font;
    font.setSize( 24 );
    font.setFamilyToSans();
    font.setStyleToBold();
    font.setColor( kvs::RGBColor::White() );
    font.setShadowColor( kvs::RGBColor::Black() );
    font.setEnabledShadow( true );

    this->setFont( font );
    this->setMargin( 10 );
}

void MovieInfo::screenUpdated()
{
    if ( m_model )
    {
        const kvs::Vec3i dims = m_model->cameraArrayDimensions();
        const kvs::Vec3i pos = m_model->cameraPosition();
        this->setText( "Filename: %s", m_model->filename().c_str() );
        this->addText( "Image resolution: %d x %d", screen()->width(), screen()->height() );
        this->addText( "Camera array dimensions: %d x %d x %d", dims.x(), dims.y(), dims.z() );
        this->addText( "Camera position: (%d, %d, %d)", pos.x(), pos.y(), pos.z() );
    }
}

} // end of namespace local
