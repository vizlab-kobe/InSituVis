#pragma once
#include "Model.h"
#include "View.h"
#include "Event.h"
#include "Slider.h"
#include "Button.h"
#include "CheckBox.h"
#include "Timer.h"


namespace local
{

class Controller
{
private:
    local::Model* m_model;
    local::View* m_view;
    local::Event m_event;
    local::Slider m_slider;
    local::Button m_button;
    local::CheckBox m_check_box;
    local::Timer m_timer;

public:
    Controller( local::Model* model, local::View* view );
    local::Slider& slider() { return m_slider; }
    local::Button& button() { return m_button; }
    local::CheckBox& checkBox() { return m_check_box; }
};

} // end of namespace local
