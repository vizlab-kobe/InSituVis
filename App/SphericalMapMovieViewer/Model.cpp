#include "Model.h"
#include <kvs/File>

namespace local
{

Model::Model( const local::Input& input )
{
    kvs::File file( input.filename );
    if ( !file.exists() ) { throw; }
    if ( !file.isFile() ) { throw; }

    m_filename = file.filePath();
    m_object_pointer = ObjectPointer( new InSituVis::MovieObject( m_filename ) );
}

Model::Object* Model::object() const
{
    Object* object = new Object();
    object->shallowCopy( *( m_object_pointer.get() ) );
    return object;
}

} // end of namespace local
