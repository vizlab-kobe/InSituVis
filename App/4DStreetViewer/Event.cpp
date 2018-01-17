#include "Event.h"
#include <kvs/Camera>
#include <InSituVis/Lib/SphericalMapMovieRenderer.h>


namespace local
{

Event::Event( local::Model* model, local::View* view ):
    m_model( model ),
    m_view( view )
{
    setEventType(
        kvs::EventBase::MouseDoubleClickEvent |
        kvs::EventBase::KeyPressEvent );
}

void Event::mouseDoubleClickEvent( kvs::MouseEvent* event )
{
    const kvs::Vec3 p = kvs::Vec3( 0, 0, 0 );
    const kvs::Vec3 a = kvs::Vec3( 0, 0, 1 );
    const kvs::Mat3 R = m_view->movieScreen().scene()->object()->xform().rotation().inverted();
    const kvs::Vec3 dir = ( a - p ) * R;

    const int x = kvs::Math::Round( dir.x() );
    const int y = kvs::Math::Round( dir.y() );
    const int z = kvs::Math::Round( dir.z() );
    const kvs::Vec3i& pos = m_model->cameraPosition();
    switch ( event->modifiers() )
    {
    case kvs::Key::ShiftModifier:
    {
        m_model->setCameraPosition( pos - kvs::Vec3i( x, y, z ) );
        break;
    }
    default:
    {
        m_model->setCameraPosition( pos + kvs::Vec3i( x, y, z ) );
        break;
    }
    }

    m_view->movieScreen().update( m_model );
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
    case kvs::Key::Space:
    {
        typedef InSituVis::SphericalMapMovieRenderer Renderer;
        Renderer* renderer = Renderer::DownCast( m_view->movieScreen().scene()->renderer("Renderer") );
        renderer->setEnabledAutoPlay( !renderer->isEnabledAutoPlay() );
        break;
    }
    default:
        break;
    }

    m_view->movieScreen().update( m_model );
}

} // end of namespace local
