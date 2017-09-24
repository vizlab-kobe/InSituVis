#include "MovieObject.h"


namespace InSituVis
{

/*===========================================================================*/
/**
 *  @brief  Constructs a new MovieObject class.
 */
/*===========================================================================*/
MovieObject::MovieObject():
    m_device_id( CV_CAP_ANY ),
    m_device( new kvs::opencv::CaptureDevice() ),
    m_type( InSituVis::MovieObject::Color24 )
{
}

/*===========================================================================*/
/**
 *  @brief  Constructs a new MovieObject class.
 *  @param  filename [in] filename
 */
/*===========================================================================*/
MovieObject::MovieObject( const std::string& filename ):
    m_device_id( CV_CAP_ANY ),
    m_device( new kvs::opencv::CaptureDevice() )
{
    if ( !this->initialize( filename ) )
    {
        kvsMessageError("Cannot initialize a capture device for s.", filename.c_str() );
    }
}

/*===========================================================================*/
/**
 *  @brief  Returns the object type.
 *  @return object type
 */
/*===========================================================================*/
kvs::ObjectBase::ObjectType MovieObject::objectType() const
{
    return kvs::ObjectBase::Image;
}

void MovieObject::shallowCopy( const MovieObject& other )
{
    BaseClass::operator=( other );
    m_device_id = other.m_device_id;
    m_device = other.m_device;
    m_type = other.m_type;
    m_width = other.m_width;
    m_height = other.m_height;
    m_nchannels = other.m_nchannels;
}

/*===========================================================================*/
/**
 *  @brief  Initialize the video object.
 *  @param  device_id [in] device ID
 *  @return true, if the video object is initialized successfully
 */
/*===========================================================================*/
const bool MovieObject::initialize( const std::string& filename )
{
    if ( !m_device->create( filename ) )
    {
        kvsMessageError("Cannot create a capture device for %s.", filename.c_str() );
        return false;
    }

    const IplImage* frame = m_device->queryFrame();
    if ( !frame )
    {
        kvsMessageError("Cannot query a new frame from the capture device.");
        return false;
    }

    m_width = static_cast<size_t>( frame->width );
    m_height = static_cast<size_t>( frame->height );
    m_nchannels = static_cast<size_t>( frame->nChannels );

    const int depth = frame->depth;
    if ( depth != IPL_DEPTH_8U )
    {
        kvsMessageError("The depth of the grabbed image isn't 'IPL_DEPTH_8U'.");
        return false;
    }

    m_type = m_nchannels == 1 ? Gray8 : Color24;

    return true;
}

} // end of namespace InSituVis
