#pragma once
#include "Input.h"
#include <kvs/SharedPointer>
#include <kvs/ImageObject>

namespace local
{

class Model
{
public:
    typedef kvs::ImageObject Object;
    typedef kvs::SharedPointer<kvs::ImageObject> ObjectPointer;

private:
    ObjectPointer m_object_pointer;

public:
    Model( const local::Input& input );

    const ObjectPointer& objectPointer() const { return m_object_pointer; }
    Object* object() const;
};

} // end of namespace local
