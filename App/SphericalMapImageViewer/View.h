#pragma once
#include "Application.h"
#include "Screen.h"
#include "Model.h"
#include "Info.h"
#include <kvs/Label>

namespace local
{

class View
{
private:
    local::Model* m_model; ///< pointer to the model
    local::Screen m_distorted_image_screen; ///< screen for distorted (equirectanglar) image
    local::Screen m_undistorted_image_screen; ///< screen for undistorted image cropped from equi. image
    kvs::Label m_distorted_image_info;
//    kvs::Label m_undistorted_image_info;
    local::Info m_undistorted_image_info;

public:
    View( local::Application* app, local::Model* model );

    local::Screen& distortedImageScreen() { return m_distorted_image_screen; }
    local::Screen& undistortedImageScreen() { return m_undistorted_image_screen; }
    kvs::Label& distortedImageInfo() { return m_distorted_image_info; }
//    kvs::Label& undistortedImageInfo() { return m_undistorted_image_info; }
    local::Info& undistortedImageInfo() { return m_undistorted_image_info; }

    void setup();
    void layout();
    void show();
};

} // end of namespace local
