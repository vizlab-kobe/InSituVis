#pragma once
#include <kvs/ObjectBase>
#include <kvs/Module>
#include <kvs/opencv/CaptureDevice>
#include <string>


namespace local
{

namespace opencv
{

/*===========================================================================*/
/**
 *  @brief  Movie object.
 */
/*===========================================================================*/
class MovieObject : public kvs::ObjectBase
{
    kvsModuleName( local::opencv::MovieObject );
    kvsModuleCategory( Object );
    kvsModuleBaseClass( kvs::ObjectBase );

public:
    enum PixelType
    {
        Gray8 = 8, ///< 8 bit gray pixel
        Color24 = 24  ///< 24 bit RGB color pixel (8x8x8 bits)
    };

private:
    int m_device_id; ///< capture device ID
    kvs::opencv::CaptureDevice m_device; ///< video capture device
    PixelType m_type; ///< pixel type
    size_t m_width; ///< capture widht
    size_t m_height; ///< capture height
    size_t m_nchannels; ///< number of channels

public:
    MovieObject();
    MovieObject( const std::string& filename );
    virtual ~MovieObject() {}

    ObjectType objectType() const;
    int deviceID() const { return m_device_id; }
    const kvs::opencv::CaptureDevice& device() const { return m_device; }
    PixelType type() const { return m_type; }
    size_t width() const { return m_width; }
    size_t height() const { return m_height; }
    size_t nchannels() const { return m_nchannels; }

public:
    const bool initialize( const std::string& filename );
};

} // end of namespace opencv

} // end of namespace local
