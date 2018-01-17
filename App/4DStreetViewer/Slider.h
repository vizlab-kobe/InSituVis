#pragma once
#include "Screen.h"
#include "Model.h"
#include "View.h"
#include <kvs/Slider>


namespace local
{

class Slider : public kvs::Slider
{
private:
    local::Model* m_model;
    local::View* m_view;

public:
    Slider( local::Model* model, local::View* view );
    void sliderMoved();
};

} // end of namespace local
