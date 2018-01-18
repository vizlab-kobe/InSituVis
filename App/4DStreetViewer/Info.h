#pragma once
#include "Model.h"
#include "Screen.h"
#include <kvs/Label>


namespace local
{

class Info : public kvs::Label
{
private:
    local::Model* m_model;

public:
    Info( local::Screen* screen );
    void setup( local::Model* model ) { m_model = model; }
    void screenUpdated();
};

} // end of namespace local
