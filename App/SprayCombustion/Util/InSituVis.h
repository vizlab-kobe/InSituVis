#pragma once
#include <functional>
#include <string>
#include <sys/stat.h>
#include <kvs/Timer>
#include <kvs/String>
#include <kvs/Directory>
#include <kvs/OffScreen>
#include <kvs/ColorImage>
#include <kvs/GrayImage>
#include <kvs/UnstructuredVolumeObject>


namespace Util
{

/*===========================================================================*/
/**
 *  @brief  In-situ visualization process class.
 */
/*===========================================================================*/
class InSituVis
{
public:
    using Volume = kvs::UnstructuredVolumeObject;
    using Screen = kvs::OffScreen;
    using Pipeline = std::function<void(Screen&,const Volume&)>;

private:
    Screen m_screen; ///< rendering screen (off-screen)
    size_t m_width; ///< width of rendering image
    size_t m_height; ///< height of rendering image
    std::string m_output_dirname; ///< output directory name
    bool m_enable_output_image; ///< flag for writing final image data
    Pipeline m_pipeline; ///< visualization pipeline

public:
    InSituVis():
        m_width( 512 ),
        m_height( 512 ),
        m_output_dirname( "Output" ),
        m_enable_output_image( true )
    {
    }

    Screen& screen() { return m_screen; }
    std::ostream& log() { return std::cout; }

    void setPipeline( Pipeline pipeline )
    {
        m_pipeline = pipeline;
    }

    void setSize( const size_t width, const size_t height )
    {
        m_width = width;
        m_height = height;
    }

    void setOutputDirectoryName( const std::string& dirname )
    {
        m_output_dirname = dirname;
    }

    void setOutputImageEnabled( const bool enable = true )
    {
        m_enable_output_image = enable;
    }

    bool initialize()
    {
        if ( !kvs::Directory::Exists( m_output_dirname ) )
        {
            if ( mkdir( m_output_dirname.c_str(), 0777 ) != 0 )
            {
                kvsMessageError() << "Cannot create " << m_output_dirname << "." << std::endl;
                return false;
            }
        }

        m_screen.setSize( m_width, m_height );
        m_screen.create();

        return true;
    }

    bool finalize()
    {
        return true;
    }

    void exec( const Volume* volume )
    {
        if ( !volume ) return;
        if ( volume->numberOfCells() == 0 ) return;
        m_pipeline( m_screen, *volume );
    }

    //void draw( const kvs::Real64 time_index )
    void draw( const Foam::Time& time )
    {
        const auto current_time_index = time.timeIndex();
        const std::string output_number = kvs::String::From( current_time_index, 6, '0' );
        const std::string output_basename( "output" );
        const std::string output_filename = output_basename + "_" + output_number;

        // Draw image
        m_screen.draw();

        // Read-back framebuffer.
        auto color_buffer = m_screen.readbackColorBuffer();

        // Output framebuffer to image file
        if ( m_enable_output_image )
        {
            const auto filename = m_output_dirname + "/" + output_filename + ".bmp";
            kvs::ColorImage image( m_width, m_height, color_buffer );
            image.write( filename );
        }
    }
};

} // end of namespace Util
