#include "View.h"


namespace local
{

View::View( local::Application* app, local::Model* model ):
    m_model( model ),
    m_movie_screen( app ),
    m_info( &m_movie_screen )
{
    this->setup();
    this->layout();
    this->show();
}

void View::setup()
{
    m_movie_screen.setup( m_model );
    m_info.setup( m_model );
}

void View::layout()
{
    const size_t x = 0;
    const size_t y = 0;
    m_movie_screen.setPosition( x, y );
}

void View::show()
{
    m_movie_screen.show();
    m_info.show();
}

} // end of namespace local
