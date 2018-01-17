#include "Button.h"
#include <InSituVis/Lib/SphericalMapMovieRenderer.h>


namespace local
{

Button::Button( local::Model* model, local::View* view ):
    kvs::PushButton( &(view->movieScreen()) ),
    m_model( model ),
    m_view( view )
{
    this->setCaption("Play");
}

void Button::released()
{
    typedef InSituVis::SphericalMapMovieRenderer Renderer;
    Renderer* renderer = Renderer::DownCast( m_view->movieScreen().scene()->renderer("Renderer") );
    if ( renderer->isEnabledAutoPlay() )
    {
        renderer->setEnabledAutoPlay( false ); // Stop
        this->setCaption("Play");
    }
    else
    {
        renderer->setEnabledAutoPlay( true ); // Play
        this->setCaption("Stop");
    }
}

} // end of namespace local
