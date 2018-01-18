#pragma once
#include "Application.h"
#include "Screen.h"
#include "Model.h"
//#include "MovieInfo.h"


namespace local
{

class MovieScreen : public local::Screen
{
public:
    MovieScreen( local::Application* app );
    void setup( local::Model* model );
    void update( local::Model* model );
};

} // end of namespace local
