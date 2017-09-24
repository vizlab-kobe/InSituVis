#pragma once
#include "Application.h"
#include "Screen.h"
#include "Model.h"

namespace local
{

class View
{
private:
    local::Model* m_model; ///< pointer to the model
    local::Screen m_distorted_movie_screen; ///< screen for distorted (equirectanglar) movie
    local::Screen m_undistorted_movie_screen; ///< screen for undistorted image cropped from equi. movie

public:
    View( local::Application* app, local::Model* model );

    local::Screen& distortedMovieScreen() { return m_distorted_movie_screen; }
    local::Screen& undistortedMovieScreen() { return m_undistorted_movie_screen; }

    void setup();
    void layout();
    void show();
};

} // end of namespace local
