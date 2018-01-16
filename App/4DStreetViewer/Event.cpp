#include "Event.h"


namespace local
{

Event::Event( local::Model* model, local::View* view ):
    m_model( model ),
    m_view( view )
{
    setEventType(
        kvs::EventBase::KeyPressEvent );
}

void Event::keyPressEvent( kvs::KeyEvent* event )
{
    const kvs::Vec3i& pos = m_model->cameraPosition();

    switch ( event->key() )
    {
    case kvs::Key::Left:
    {
        m_model->setCameraPosition( pos + kvs::Vec3i( -1, 0, 0 ) );
        break;
    }
    case kvs::Key::Right:
    {
        m_model->setCameraPosition( pos + kvs::Vec3i( +1, 0, 0 ) );
        break;
    }
    case kvs::Key::Up:
    {
        m_model->setCameraPosition( pos + kvs::Vec3i( 0, +1, 0 ) );
        break;
    }
    case kvs::Key::Down:
    {
        m_model->setCameraPosition( pos + kvs::Vec3i( 0, -1, 0 ) );
        break;
    }
    case kvs::Key::PageUp:
    {
        m_model->setCameraPosition( pos + kvs::Vec3i( 0, 0, +1 ) );
        break;
    }
    case kvs::Key::PageDown:
    {
        m_model->setCameraPosition( pos + kvs::Vec3i( 0, 0, -1 ) );
        break;
    }
    default:
        break;
    }

    m_view->movieScreen().update( m_model );
}

} // end of namespace local
