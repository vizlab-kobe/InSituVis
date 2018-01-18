#pragma once
#include "Application.h"
#include "Model.h"
#include "MovieInfo.h"
#include "MovieScreen.h"
#include <kvs/Label>


namespace local
{

class View
{
private:
    local::Model* m_model; ///< pointer to the model
    local::MovieScreen m_movie_screen;
    local::MovieInfo m_info;

public:
    View( local::Application* app, local::Model* model );
    local::MovieScreen& movieScreen() { return m_movie_screen; }
    void setup();
    void layout();
    void show();
};

} // end of namespace local
