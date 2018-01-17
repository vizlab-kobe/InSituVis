#include "CheckBox.h"
#include <InSituVis/Lib/SphericalMapMovieRenderer.h>


namespace local
{

CheckBox::CheckBox( local::Model* model, local::View* view ):
    kvs::CheckBox( &(view->movieScreen()) ),
    m_model( model ),
    m_view( view )
{
    typedef InSituVis::SphericalMapMovieRenderer Renderer;
    Renderer* renderer = Renderer::DownCast( m_view->movieScreen().scene()->renderer("Renderer") );

    this->setCaption("Loop");
    this->setState( renderer->isEnabledLoopPlay() );
}

void CheckBox::stateChanged()
{
    typedef InSituVis::SphericalMapMovieRenderer Renderer;
    Renderer* renderer = Renderer::DownCast( m_view->movieScreen().scene()->renderer("Renderer") );
    renderer->setEnabledLoopPlay( this->state() );
}

} // end of namespace local
