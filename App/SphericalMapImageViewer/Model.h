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
    std::string m_filename;
    ObjectPointer m_object_pointer;

public:
    Model( const local::Input& input );

    const std::string& filename() const { return m_filename; }
    const ObjectPointer& objectPointer() const { return m_object_pointer; }
    Object* object() const;
};

} // end of namespace local
