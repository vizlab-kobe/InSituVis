#pragma once
#include <functional>
#include <kvs/mpi/Communicator>
#include <kvs/mpi/LogStream>
#include <kvs/mpi/ImageCompositor>
#include <kvs/Timer>
#include <kvs/String>
#include <kvs/OffScreen>
#include <kvs/ColorImage>
#include <kvs/GrayImage>
#include "Importer.h"
#include "OutputDirectory.h"


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
    kvs::mpi::Communicator m_world; ///< MPI communicator
    kvs::mpi::LogStream m_log; ///< MPI log stream
    kvs::mpi::ImageCompositor m_compositor; ///< image compositor
    Screen m_screen; ///< rendering screen (off-screen)
    size_t m_width; ///< width of rendering image
    size_t m_height; ///< height of rendering image
    Util::OutputDirectory m_output_directory; ///< output directory
    bool m_enable_output_image; ///< flag for writing final image data
    bool m_enable_output_subimage; ///< flag for writing sub-volume rendering image
    bool m_enable_output_subimage_depth; ///< flag for writing sub-volume rendering image (depth image)
    bool m_enable_output_subimage_alpha; ///< flag for writing sub-volume rendering image (alpha image)
    bool m_enable_output_subvolume; ///< flag for writing sub-volume data
    Pipeline m_pipeline; ///< visualization pipeline

public:
    InSituVis( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ):
        m_world( world, root ),
        m_log( m_world ),
        m_compositor( m_world ),
        m_width( 512 ),
        m_height( 512 ),
        m_enable_output_image( true ),
        m_enable_output_subimage( false ),
        m_enable_output_subimage_depth( false ),
        m_enable_output_subimage_alpha( false ),
        m_enable_output_subvolume( false )
    {
    }

    Screen& screen() { return m_screen; }
    kvs::mpi::Communicator& world() { return m_world; }
    std::ostream& log() { return m_log( m_world.root() ); }
    std::ostream& log( const int rank ) { return m_log( rank ); }

    void setPipeline( Pipeline pipeline )
    {
        m_pipeline = pipeline;
    }

    void setSize( const size_t width, const size_t height )
    {
        m_width = width;
        m_height = height;
    }

    void setOutputDirectory( const Util::OutputDirectory& directory )
    {
        m_output_directory = directory;
    }

    void setOutputDirectoryName(
        const std::string& base_dirname ,
        const std::string& sub_dirname )
    {
        m_output_directory.setBaseDirectoryName( base_dirname );
        m_output_directory.setSubDirectoryName( sub_dirname );
    }

    void setOutputImageEnabled( const bool enable = true )
    {
        m_enable_output_image = enable;
    }

    void setOutputSubImageEnabled(
        const bool enable = true,
        const bool enable_depth = false,
        const bool enable_alpha = false )
    {
        m_enable_output_subimage = enable;
        m_enable_output_subimage_depth = enable_depth;
        m_enable_output_subimage_alpha = enable_alpha;
    }

    void setOutputSubVolumeEnabled( const bool enable = true )
    {
        m_enable_output_subvolume = enable;
    }

    bool initialize()
    {
        if ( !m_output_directory.create( m_world ) )
        {
            this->log() << "ERROR: " << "Cannot create output directory." << std::endl;
            return false;
        }

        const bool depth_testing = true;
        if ( !m_compositor.initialize( m_width, m_height, depth_testing ) )
        {
            this->log() << "ERROR: " << "Cannot initialize image compositor." << std::endl;
            return false;
        }

        m_screen.setSize( m_width, m_height );
        m_screen.create();

        return true;
    }

    bool finalize()
    {
        return m_compositor.destroy();
    }

    void exec( const Volume* volume )
    {
        m_pipeline( m_screen, *volume );
    }

    //void draw( const kvs::Real64 time_index )
    void draw( const Foam::Time& time )
    {
        const auto current_time_index = time.timeIndex();
        const std::string output_number = kvs::String::From( current_time_index, 6, '0' );
        const std::string output_basename( "output" );
        const std::string output_filename = output_basename + "_" + output_number;
        const std::string output_dirname = m_output_directory.name();
        const std::string output_base_dirname = m_output_directory.baseDirectoryName();

        // Draw image
        m_screen.draw();

        // Read-back image
        auto color_buffer = m_screen.readbackColorBuffer();
        auto depth_buffer = m_screen.readbackDepthBuffer();

        // Output rendering image
        if ( m_enable_output_subimage )
        {
            // RGB image
            {
                const auto filename = output_dirname + output_filename + ".bmp";
                kvs::ColorImage image( m_width, m_height, color_buffer );
                image.write( filename );
            }

            // Depth image
            if ( m_enable_output_subimage_depth )
            {
                const auto filename = output_dirname + output_basename + "_depth_" + output_number + ".bmp";
                kvs::GrayImage depth_image( m_width, m_height, depth_buffer );
                depth_image.write( filename );
            }

            // Alpha image
            if ( m_enable_output_subimage_alpha )
            {
                const auto filename = output_dirname + output_basename + "_alpha_" + output_number + ".bmp";
                kvs::GrayImage alpha_image( m_width, m_height, color_buffer, 3 );
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
            if ( m_enable_output_image )
            {
                const auto filename = output_base_dirname + "/" + output_filename + ".bmp";
                kvs::ColorImage image( m_width, m_height, color_buffer );
                image.write( filename );
            }
        }
    }

    /*
    void exec( const Foam::Time& time, const Foam::fvMesh& mesh, const Foam::volScalarField& field )
    {
        this->exec_pipeline( time, new Util::Importer( m_world, mesh, field ) );
    }

    void exec( const Foam::Time& time, const Foam::fvMesh& mesh, const Foam::volVectorField& field )
    {
        this->exec_pipeline( time, new Util::Importer( m_world, mesh, field ) );
    }
    */

private:
    /*
    void exec_pipeline( const Foam::Time& time, Volume* volume )
    {
        const auto current_time_index = time.timeIndex();
        const std::string output_number = kvs::String::From( current_time_index, 6, '0' );
        const std::string output_basename( "output" );
        const std::string output_filename = output_basename + "_" + output_number;
        const std::string output_dirname = m_output_directory.name();
        const std::string output_base_dirname = m_output_directory.baseDirectoryName();

        // Output sub-volume data
        if ( m_enable_output_subvolume )
        {
            const auto filename = output_dirname + output_filename + ".kvsml";
            volume->write( filename, false );
        }

        // Execute visualization pipeline
        m_pipeline( m_screen, *volume );
        delete volume;

        // Draw image
        m_screen.draw();

        // Read-back image
        auto color_buffer = m_screen.readbackColorBuffer();
        auto depth_buffer = m_screen.readbackDepthBuffer();

        // Output rendering image
        if ( m_enable_output_subimage )
        {
            // RGB image
            {
                const auto filename = output_dirname + output_filename + ".bmp";
                kvs::ColorImage image( m_width, m_height, color_buffer );
                image.write( filename );
            }

            // Depth image
            if ( m_enable_output_subimage_depth )
            {
                const auto filename = output_dirname + output_basename + "_depth_" + output_number + ".bmp";
                kvs::GrayImage depth_image( m_width, m_height, depth_buffer );
                depth_image.write( filename );
            }

            // Alpha image
            if ( m_enable_output_subimage_alpha )
            {
                const auto filename = output_dirname + output_basename + "_alpha_" + output_number + ".bmp";
                kvs::GrayImage alpha_image( m_width, m_height, color_buffer, 3 );
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
            if ( m_enable_output_image )
            {
                const auto filename = output_base_dirname + "/" + output_filename + ".bmp";
                kvs::ColorImage image( m_width, m_height, color_buffer );
                image.write( filename );
            }
        }
    }
    */
};

} // end of namespace Util
