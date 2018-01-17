#include "Controller.h"
#include <kvs/IdleEventListener>

class IdleEvent : public kvs::IdleEventListener
{
    void update() { screen()->redraw(); }
};

namespace local
{

Controller::Controller( local::Model* model, local::View* view ):
    m_model( model ),
    m_view( view ),
    m_event( model, view ),
    m_slider( model, view ),
    m_button( model, view ),
    m_check_box( model, view )
{
    m_view->movieScreen().addEvent( &m_event );
    m_view->movieScreen().addEvent( new IdleEvent() );

    const size_t widget_width = 150;
    const size_t widget_height = 30;
    m_slider.setMargin( 0 );
    m_slider.setSize( widget_width, widget_height );
    m_button.setSize( widget_width / 2, widget_height );
    m_check_box.setSize( widget_width / 2, widget_height );

    const size_t screen_width = m_view->movieScreen().width();
    const size_t screen_height = m_view->movieScreen().height();
    const size_t margin = 10;
    m_slider.setPosition( screen_width - widget_width - margin, screen_height - widget_height * 2 - margin * 3 );
    m_button.setPosition( screen_width - widget_width - margin, screen_height - widget_height - margin );
    m_check_box.setPosition( screen_width - widget_width / 2, screen_height - widget_height - margin / 2 );

    m_slider.show();
    m_button.show();
    m_check_box.show();
}

} // end of namespace local
