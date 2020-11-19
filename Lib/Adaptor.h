#pragma once
#include <functional>
#include <kvs/OffScreen>
#include <kvs/UnstructuredVolumeObject>
#include <kvs/ColorImage>
#include <kvs/GrayImage>
#include <kvs/String>
#include "OutputDirectory.h"

#if defined( KVS_SUPPORT_MPI )
#include <kvs/mpi/Communicator>
#include <kvs/mpi/LogStream>
#include <kvs/mpi/ImageCompositor>
#endif


namespace InSituVis
{

class Adaptor
{
public:
    using Volume = kvs::UnstructuredVolumeObject;
    using Screen = kvs::OffScreen;
    using Pipeline = std::function<void(Screen&,const Volume&)>;

private:
    Screen m_screen; ///< rendering screen (off-screen)
    Pipeline m_pipeline; ///< visualization pipeline
    InSituVis::OutputDirectory m_output_directory; ///< output directory
    size_t m_image_width; ///< width of rendering image
    size_t m_image_height; ///< height of rendering image
    bool m_enable_output_image; ///< flag for writing final rendering image data

public:
    Adaptor():
        m_image_width( 512 ),
        m_image_height( 512 ),
        m_enable_output_image( true )
    {
    }

    virtual ~Adaptor() {}

    size_t imageWidth() const { return m_image_width; }
    size_t imageHeight() const { return m_image_height; }
    bool isOutputImageEnabled() const { return m_enable_output_image; }
    std::ostream& log() { return std::cout; }
    Screen& screen() { return m_screen; }
    InSituVis::OutputDirectory& outputDirectory() { return m_output_directory; }

    void setPipeline( Pipeline pipeline )
    {
        m_pipeline = pipeline;
    }

    void setOutputDirectory( const InSituVis::OutputDirectory& directory )
    {
        m_output_directory = directory;
    }

    void setImageSize( const size_t width, const size_t height )
    {
        m_image_width = width;
        m_image_height = height;
    }

    void setOutputImageEnabled( const bool enable = true )
    {
        m_enable_output_image = enable;
    }

    virtual bool initialize()
    {
        if ( !m_output_directory.create() )
        {
            this->log() << "ERRROR: " << "Cannot create output directory." << std::endl;
            return false;
        }
        m_screen.setSize( m_image_width, m_image_height );
        m_screen.create();
        return true;
    }

    virtual bool finalize()
    {
        return true;
    }

    virtual void put( const Volume* volume )
    {
        if ( !volume ) return;
        if ( volume->numberOfCells() == 0 ) return;
        m_pipeline( m_screen, *volume );
    }

    virtual void exec( const kvs::UInt32 time_index )
    {
        const std::string output_number = kvs::String::From( time_index, 6, '0' );
        const std::string output_basename( "output" );
        const std::string output_filename = output_basename + "_" + output_number;

        // Draw image
        m_screen.draw();

        // Read-back framebuffer.
        auto color_buffer = m_screen.readbackColorBuffer();

        // Output framebuffer to image file
        if ( m_enable_output_image )
        {
            const auto filename = m_output_directory.name() + "/" + output_filename + ".bmp";
            kvs::ColorImage image( m_image_width, m_image_height, color_buffer );
            image.write( filename );
        }
    }
};

#if defined( KVS_SUPPORT_MPI )
namespace mpi
{

class Adaptor : public InSituVis::Adaptor
{
    using BaseClass = InSituVis::Adaptor;

private:
    kvs::mpi::Communicator m_world; ///< MPI communicator
    kvs::mpi::LogStream m_log; ///< MPI log stream
    kvs::mpi::ImageCompositor m_compositor; ///< image compositor
    bool m_enable_output_subimage; ///< flag for writing sub-volume rendering image
    bool m_enable_output_subimage_depth; ///< flag for writing sub-volume rendering image (depth image)
    bool m_enable_output_subimage_alpha; ///< flag for writing sub-volume rendering image (alpha image)

public:
    Adaptor( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ):
        InSituVis::Adaptor(),
        m_world( world, root ),
        m_log( m_world ),
        m_compositor( m_world ),
        m_enable_output_subimage( false ),
        m_enable_output_subimage_depth( false ),
        m_enable_output_subimage_alpha( false )
    {
    }

    virtual ~Adaptor() {}

    kvs::mpi::Communicator& world() { return m_world; }
    std::ostream& log() { return m_log( m_world.root() ); }
    std::ostream& log( const int rank ) { return m_log( rank ); }

    void setOutputSubImageEnabled(
        const bool enable = true,
        const bool enable_depth = false,
        const bool enable_alpha = false )
    {
        m_enable_output_subimage = enable;
        m_enable_output_subimage_depth = enable_depth;
        m_enable_output_subimage_alpha = enable_alpha;
    }

    virtual bool initialize()
    {
        if ( !BaseClass::outputDirectory().create( m_world ) )
        {
            this->log() << "ERROR: " << "Cannot create output directories." << std::endl;
            return false;
        }

        const bool depth_testing = true;
        const auto width = BaseClass::imageWidth();
        const auto height = BaseClass::imageHeight();
        if ( !m_compositor.initialize( width, height, depth_testing ) )
        {
            this->log() << "ERROR: " << "Cannot initialize image compositor." << std::endl;
            return false;
        }

        BaseClass::screen().setSize( width, height );
        BaseClass::screen().create();

        return true;
    }

    virtual bool finalize()
    {
        return m_compositor.destroy();
    }

    virtual void exec( const kvs::UInt32 time_index )
    {
        const std::string output_number = kvs::String::From( time_index, 6, '0' );
        const std::string output_basename( "output" );
        const std::string output_filename = output_basename + "_" + output_number;
        const std::string output_dirname = BaseClass::outputDirectory().name();
        const std::string output_base_dirname = BaseClass::outputDirectory().baseDirectoryName();

        // Draw image
        BaseClass::screen().draw();

        // Read-back image
        auto color_buffer = BaseClass::screen().readbackColorBuffer();
        auto depth_buffer = BaseClass::screen().readbackDepthBuffer();

        // Output rendering image
        const auto width = BaseClass::imageWidth();
        const auto height = BaseClass::imageHeight();
        if ( m_enable_output_subimage )
        {
            // RGB image
            {
                const auto filename = output_dirname + output_filename + ".bmp";
                kvs::ColorImage image( width, height, color_buffer );
                image.write( filename );
            }

            // Depth image
            if ( m_enable_output_subimage_depth )
            {
                const auto filename = output_dirname + output_basename + "_depth_" + output_number + ".bmp";
                kvs::GrayImage depth_image( width, height, depth_buffer );
                depth_image.write( filename );
            }

            // Alpha image
            if ( m_enable_output_subimage_alpha )
            {
                const auto filename = output_dirname + output_basename + "_alpha_" + output_number + ".bmp";
                kvs::GrayImage alpha_image( width, height, color_buffer, 3 );
                alpha_image.write( filename );
            }
        }

        // Image composition
        if ( !m_compositor.run( color_buffer, depth_buffer ) )
        {
            this->log() << "ERROR: " << "Cannot compose images." << std::endl;
        }

        // Output composite image
        if ( m_world.rank() == m_world.root() )
        {
            if ( BaseClass::isOutputImageEnabled() )
            {
                const auto filename = output_base_dirname + "/" + output_filename + ".bmp";
                kvs::ColorImage image( width, height, color_buffer );
                image.write( filename );
            }
        }
    }
};

} // end of namespace mpi

#endif // KVS_SUPPORT_MPI

} // end of namespace InSituVis
