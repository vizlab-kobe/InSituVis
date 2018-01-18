#include "Model.h"
#include <kvs/File>
#include <kvs/Directory>


namespace local
{

Model::Model( const local::Input& input )
{
    kvs::Directory dir( input.dirname );
    if ( !dir.exists() ) { throw; }
    if ( !dir.isDirectory() ) { throw; }

    const std::string ext = "mp4";
    const kvs::FileList& files = dir.fileList();
    for ( size_t i = 0; i < files.size(); i++ )
    {
        if ( files[i].extension() == ext )
        {
            m_files.push_back( files[i] );
        }
    }

    m_camera_position = input.position;
    m_camera_array_dimensions = input.dimensions;
    m_frame_rate = 5.0f;

    this->setup_object( this->camera_position_index() );
}

Model::Object* Model::object() const
{
    Object* object = new Object();
    object->shallowCopy( *( m_object_pointer.get() ) );
    return object;
}

void Model::setCameraPosition( const kvs::Vec3i& position )
{
    const kvs::Vec3i dims = m_camera_array_dimensions;
    const int x = kvs::Math::Clamp( position.x(), 0, dims.x() - 1 );
    const int y = kvs::Math::Clamp( position.y(), 0, dims.y() - 1 );
    const int z = kvs::Math::Clamp( position.z(), 0, dims.z() - 1 );
    m_camera_position = kvs::Vec3i( x, y, z );
    this->setup_object( this->camera_position_index() );
}

void Model::setup_object( const size_t index )
{
    kvs::File file = m_files[ index ];
    if ( !file.exists() ) { throw; }
    if ( !file.isFile() ) { throw; }

    const std::string filename = file.filePath();
    m_object_pointer = ObjectPointer( new Object( filename ) );
}

size_t Model::camera_position_index() const
{
    const kvs::Vec3i& dims = m_camera_array_dimensions;
    const kvs::Vec3i& pos = m_camera_position;
    return pos.x() + dims.x() * pos.y() + dims.x() * dims.y() * pos.z();
}

} // end of namespace local
