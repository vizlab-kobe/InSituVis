#pragma once
#include "Model.h"
#include "View.h"
#include <kvs/EventListener>


namespace local
{

class Event : public kvs::EventListener
{
private:
    local::Model* m_model;
    local::View* m_view;

public:
    Event( local::Model* model, local::View* view );

private:
    void keyPressEvent( kvs::KeyEvent* event );
};

} // end of namespace local
