#pragma once
#include "Model.h"
#include "View.h"
#include <kvs/PushButton>


namespace local
{

class Button : public kvs::PushButton
{
private:
    local::Model* m_model;
    local::View* m_view;

public:
    Button( local::Model* model, local::View* view );
    void released();
};

} // end of namespace local
