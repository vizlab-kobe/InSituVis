#pragma once
#include <cfenv>
#include <functional>
#include <kvs/mpi/Communicator>
#include <kvs/mpi/Logger>
#include <kvs/mpi/ImageCompositor>
#include <kvs/Timer>
#include <kvs/String>
#include <kvs/OffScreen>
#include <kvs/ExternalFaces>
#include <kvs/PolygonRenderer>
#include <kvs/Png>
#include "../Util/Importer.h"
#include "../Util/OutputDirectory.h"


namespace local
{

class Visualization
{
public:
    using Volume = kvs::UnstructuredVolumeObject;
    using Screen = kvs::OffScreen;
    using Pipeline = std::function<void(Screen&,Volume&)>;

private:
    kvs::mpi::Communicator m_world;
    kvs::mpi::Logger m_logger;
    kvs::mpi::ImageCompositor m_compositor;
    size_t m_width;
    size_t m_height;
    Util::OutputDirectory m_output_directory;
    bool m_enable_output_volume;
    bool m_enable_output_image;
    bool m_enable_output_subimage;
    Pipeline m_pipeline; ///< visualization pipeline
    Volume* m_volume;
//    std::string m_output_base_dirname;
//    std::string m_output_dirname;

public:
    Visualization( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ):
        m_world( world, root ),
        m_logger( m_world ),
        m_compositor( m_world ),
        m_width( 512 ),
        m_height( 512 ),
        m_enable_output_volume( false ),
        m_enable_output_image( true ),
        m_enable_output_subimage( true ),
        m_volume( nullptr )
    {
        // Default visualization pipeline.
        m_pipeline = [] ( Screen& screen, Volume& volume )
        {
            auto* object = new kvs::ExternalFaces( &volume );
            auto* renderer = new kvs::glsl::PolygonRenderer();
            screen.registerObject( object, renderer );
        };
    }

    kvs::mpi::Communicator& world() { return m_world; }
    std::ostream& log() { return m_logger( m_world.root() ); }

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

        return true;
    }

/*
    bool initialize( const std::string& base_dirname, const std::string& sub_dirname )
    {
        Util::OutputDirectory output_dir( base_dirname, sub_dirname );
        if ( !output_dir.create( m_world ) )
        {
            m_logger( m_world.root() ) << "ERROR: " << "Cannot create output directory." << std::endl;
            return false;
        }

        m_output_base_dirname = output_dir.baseDirectoryName();
        m_output_dirname = output_dir.name();

        const bool depth_testing = true;
        if ( !m_compositor.initialize( m_width, m_height, depth_testing ) )
        {
            m_logger( m_world.root() ) << "ERROR: " << "Cannot initialize image compositor." << std::endl;
            return false;
        }

        return true;
    }
*/

    bool finalize()
    {
        m_compositor.destroy();
        return true;
    }

    void exec( const Foam::Time& time, const Foam::fvMesh& mesh, const Foam::volScalarField& field )
    {
        this->exec_pipeline( time, new Util::Importer( m_world, mesh, field ) );
    }

    void exec( const Foam::Time& time, const Foam::fvMesh& mesh, const Foam::volVectorField& field )
    {
        this->exec_pipeline( time, new Util::Importer( m_world, mesh, field ) );
    }

private:
    void exec_pipeline( const Foam::Time& time, Volume* volume )
    {
        const auto current_time_index = time.timeIndex();
        const std::string output_number = kvs::String::From( current_time_index, 5, '0' );
        const std::string output_basename( "output" );
        const std::string output_filename = output_basename + "_" + output_number;
        const std::string output_dirname = m_output_directory.name();
        const std::string output_base_dirname = m_output_directory.baseDirectoryName();

        kvs::OffScreen screen;
        screen.setSize( m_width, m_height );

        m_pipeline( screen, *volume );
        delete volume;

        fenv_t fe;
        std::feholdexcept( &fe );
        screen.draw();
        std::feupdateenv( &fe );

        auto color_buffer = screen.readbackColorBuffer();
        auto depth_buffer = screen.readbackDepthBuffer();
        m_compositor.run( color_buffer, depth_buffer );

        // Output composite image
        if ( m_enable_output_image )
        {
            kvs::ColorImage image( m_width, m_height, color_buffer );
            image.write( output_base_dirname + "/" + output_filename + ".bmp" );
        }
    }
};

} // end of namespace local
