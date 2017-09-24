#include "Model.h"
#include <kvs/File>
#include <kvs/ImageImporter>

namespace local
{

Model::Model( const local::Input& input )
{
    kvs::File file( input.filename );
    if ( !file.exists() ) { throw; }
    if ( !file.isFile() ) { throw; }

    m_object_pointer = ObjectPointer( new kvs::ImageImporter( file.filePath() ) );
}

Model::Object* Model::object() const
{
    Object* object = new Object();
    object->shallowCopy( *( m_object_pointer.get() ) );
    return object;
}

} // end of namespace local
