#pragma once
#include "Input.h"
#include <kvs/SharedPointer>
#include <InSituVis/Lib/MovieObject.h>

namespace local
{

class Model
{
public:
    typedef InSituVis::MovieObject Object;
    typedef kvs::SharedPointer<Object> ObjectPointer;

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
