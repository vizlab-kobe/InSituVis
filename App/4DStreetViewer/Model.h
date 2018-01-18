#pragma once
#include "Input.h"
#include <kvs/SharedPointer>
#include <kvs/FileList>
#include <InSituVis/Lib/MovieObject.h>

namespace local
{

class Model
{
public:
    typedef InSituVis::MovieObject Object;
    typedef kvs::SharedPointer<Object> ObjectPointer;

private:
    kvs::FileList m_files;
    kvs::Vec3i m_camera_position;
    kvs::Vec3i m_camera_array_dimensions;
    ObjectPointer m_object_pointer;
    float m_frame_rate;

public:
    Model( const local::Input& input );

    const kvs::File& file() const { return m_files[this->camera_position_index()]; }
    const std::string filename() const { return m_files[this->camera_position_index()].fileName(); }
    const kvs::Vec3i& cameraPosition() const { return m_camera_position; }
    const kvs::Vec3i& cameraArrayDimensions() const { return m_camera_array_dimensions; }
    const ObjectPointer& objectPointer() const { return m_object_pointer; }
    float frameRate() const { return m_frame_rate; }
    Object* object() const;

    void setCameraPosition( const kvs::Vec3i& position );

private:
    void setup_object( const size_t index );
    size_t camera_position_index() const;
};

} // end of namespace local
