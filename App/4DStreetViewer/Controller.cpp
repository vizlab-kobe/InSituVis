#include "Controller.h"

namespace local
{

Controller::Controller( local::Model* model, local::View* view ):
    m_model( model ),
    m_view( view ),
    m_event( model, view )
{
    m_view->movieScreen().addEvent( &m_event );
}

} // end of namespace local
