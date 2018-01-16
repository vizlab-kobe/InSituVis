#pragma once
#include "Model.h"
#include "View.h"
#include "Event.h"


namespace local
{

class Controller
{
private:
    local::Model* m_model;
    local::View* m_view;
    local::Event m_event;

public:
    Controller( local::Model* model, local::View* view );
};

} // end of namespace local
