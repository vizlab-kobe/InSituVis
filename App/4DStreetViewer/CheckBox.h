#pragma once
#include "Model.h"
#include "View.h"
#include <kvs/CheckBox>

namespace local
{

class CheckBox : public kvs::CheckBox
{
private:
    local::Model* m_model;
    local::View* m_view;

public:
    CheckBox( local::Model* model, local::View* view );
    void stateChanged();
};

} // end of namespace local
