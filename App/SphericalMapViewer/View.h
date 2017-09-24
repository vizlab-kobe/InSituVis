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
    local::Screen m_distorted_image_screen; ///< screen for distorted (equirectanglar) image
    local::Screen m_undistorted_image_screen; ///< screen for undistorted image cropped from equi. image

public:
    View( local::Application* app, local::Model* model );

    local::Screen& distortedImageScreen() { return m_distorted_image_screen; }
    local::Screen& undistortedImageScreen() { return m_undistorted_image_screen; }

    void setup();
    void layout();
    void show();
};

} // end of namespace local
