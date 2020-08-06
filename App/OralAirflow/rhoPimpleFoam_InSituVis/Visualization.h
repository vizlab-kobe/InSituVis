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
    using Pipeline = std::function<void(Screen&,const Volume&)>;

private:
    kvs::mpi::Communicator m_world;
    kvs::mpi::Logger m_logger;
    kvs::mpi::ImageCompositor m_compositor;
    int m_root;
    size_t m_width;
    size_t m_height;
    Pipeline m_pipeline; ///< visualization pipeline
    Volume* m_volume;
    std::string m_output_base_dirname;
    std::string m_output_dirname;

public:
    Visualization( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ):
        m_world( world ),
        m_logger( m_world ),
        m_compositor( m_world ),
        m_root( root ),
        m_width( 512 ),
        m_height( 512 ),
        m_volume( nullptr )
    {
        // Default visualization pipeline.
        m_pipeline = [] ( Screen& screen, const Volume& volume )
        {
            auto* object = new kvs::ExternalFaces( &volume );
            auto* renderer = new kvs::glsl::PolygonRenderer();
            screen.registerObject( object, renderer );
        };
    }

    void setPipeline( Pipeline pipeline ) { m_pipeline = pipeline; }
    void setSize( const size_t width, const size_t height ) { m_width = width; m_height = height; }

    bool initialize( const std::string& base_dirname, const std::string& sub_dirname )
    {
        Util::OutputDirectory output_dir( base_dirname, sub_dirname );
        if ( !output_dir.create( m_world ) )
        {
            m_logger( m_root ) << "ERROR: " << "Cannot create output directory." << std::endl;
            return false;
        }

        m_output_base_dirname = output_dir.baseDirectoryName();
        m_output_dirname = output_dir.name();

        const bool depth_testing = true;
        if ( !m_compositor.initialize( m_width, m_height, depth_testing ) )
        {
            m_logger( m_root ) << "ERROR: " << "Cannot initialize image compositor." << std::endl;
            return false;
        }

        return true;
    }

    bool finalize()
    {
        m_compositor.destroy();
        return true;
    }

    void exec( const Foam::fvMesh& mesh, const Foam::volScalarField& field )
    {
        this->exec_pipeline( new Util::Importer( m_world, mesh, field ) );
    }

    void exec( const Foam::fvMesh& mesh, const Foam::volVectorField& field )
    {
        this->exec_pipeline( new Util::Importer( m_world, mesh, field ) );
    }

private:
    void exec_pipeline( Volume* volume )
    {
        kvs::OffScreen screen;
        screen.setSize( m_width, m_height );

//        m_pipeline( screen, volume->clone() );
        m_pipeline( screen, *volume );
        delete volume;

        fenv_t fe;
        std::feholdexcept( &fe );
        screen.draw();
        std::feupdateenv( &fe );

        auto color_buffer = screen.readbackColorBuffer();
        auto depth_buffer = screen.readbackDepthBuffer();
        m_compositor.run( color_buffer, depth_buffer );
    }
};

} // end of namespace local
