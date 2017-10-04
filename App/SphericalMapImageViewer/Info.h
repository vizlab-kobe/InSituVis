#pragma once
#include "Screen.h"
#include "Model.h"
#include <kvs/Label>

namespace local
{

class Info : public kvs::Label
{
private:
    Screen* m_screen;
    local::Model* m_model;

public:
    Info( Screen* screen, local::Model* model );

private:
    void screenUpdated();
};

} // end of namespace local
