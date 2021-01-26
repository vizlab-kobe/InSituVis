#pragma once
#include <kvs/OrthoSlice>
#include <kvs/Isosurface>
#include <kvs/PolygonObject>
#include <kvs/PolygonRenderer>
#include <kvs/PolygonImporter>
#include <kvs/Bounds>
#include <kvs/String>
#include <InSituVis/Lib/Adaptor.h>
#include <InSituVis/Lib/Viewpoint.h>
#include <InSituVis/Lib/DistributedViewpoint.h>
#include <InSituVis/Lib/StampTimer.h>
#include <csignal>
#include <functional>


namespace
{
std::function<void(int)> Dump;
void SigTerm( int sig ) { Dump( sig ); }
}

namespace local
{

class InSituVis : public ::InSituVis::mpi::Adaptor
{
    using BaseClass = ::InSituVis::mpi::Adaptor;
    using Viewpoint = ::InSituVis::DistributedViewpoint;
    using Volume = BaseClass::Volume;
    using Polygon = kvs::PolygonObject;
    using Screen = BaseClass::Screen;

public:
    static Pipeline OrthoSlice();
    static Pipeline Isosurface();

private:
    Viewpoint m_viewpoint; ///< viewpoint
    Polygon m_boundary_mesh; ///< boundary mesh
    ::InSituVis::mpi::StampTimer m_sim_timer;
    ::InSituVis::mpi::StampTimer m_vis_timer;

public:
    InSituVis( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ):
        BaseClass( world, root ),
        m_viewpoint( {3,3,3}, Viewpoint::CubicDist, Viewpoint::SingleDir ), // OK
        //m_viewpoint( {3,3,3}, Viewpoint::CubicDist, Viewpoint::OmniDir ), // OK
        //m_viewpoint( {3,3,3}, Viewpoint::CubicDist, Viewpoint::AdaptiveDir ), // NG
        //m_viewpoint( {3,3,3}, Viewpoint::SphericalDist, Viewpoint::SingleDir ), // OK
        //m_viewpoint( {3,3,3}, Viewpoint::SphericalDist, Viewpoint::OmniDir ), // OK
        //m_viewpoint( {3,3,3}, Viewpoint::SphericalDist, Viewpoint::AdaptiveDir ), // NG
        m_sim_timer( BaseClass::world(), "Sim. time" ),
        m_vis_timer( BaseClass::world(), "Vis. time" )
    {
        m_viewpoint.generate();

        this->setImageSize( 1024, 1024 );
        this->setOutputImageEnabled( true );
        this->setOutputSubImageEnabled( false, false, false ); // color, depth, alpha
        //this->setOutputSubImageEnabled( true, true, true ); // color, depth, alpha
        this->setTimeInterval( 5 );
        this->setViewpoint( m_viewpoint );
        //this->setPipeline( local::InSituVis::OrthoSlice() );
        this->setPipeline( local::InSituVis::Isosurface() );

        // Set signal function for dumping timers.
        ::Dump = [&](int) { this->dumpTimer(); exit(0); };
        std::signal( SIGTERM, ::SigTerm );
    }

    ::InSituVis::mpi::StampTimer& simTimer() { return m_sim_timer; }
    ::InSituVis::mpi::StampTimer& visTimer() { return m_vis_timer; }

    void dumpTimer()
    {
        // For each node
        const std::string rank = kvs::String::From( this->world().rank(), 4, '0' );
        const std::string subdir = BaseClass::outputDirectory().name() + "/";
        m_sim_timer.write( subdir + "sim_time_" + rank +".csv" );
        m_vis_timer.write( subdir + "vis_time_" + rank +".csv" );

        // For root node
        const std::string basedir = BaseClass::outputDirectory().baseDirectoryName() + "/";
        auto sim_time_min = m_sim_timer; sim_time_min.reduceMin();
        auto sim_time_max = m_sim_timer; sim_time_max.reduceMax();
        auto sim_time_ave = m_sim_timer; sim_time_ave.reduceAve();
        auto vis_time_min = m_vis_timer; vis_time_min.reduceMin();
        auto vis_time_max = m_vis_timer; vis_time_max.reduceMax();
        auto vis_time_ave = m_vis_timer; vis_time_ave.reduceAve();
        if ( this->world().isRoot() )
        {
            sim_time_min.write( basedir + "sim_time_min.csv" );
            sim_time_max.write( basedir + "sim_time_max.csv" );
            sim_time_ave.write( basedir + "sim_time_ave.csv" );
            vis_time_min.write( basedir + "vis_time_min.csv" );
            vis_time_max.write( basedir + "vis_time_max.csv" );
            vis_time_ave.write( basedir + "vis_time_ave.csv" );
        }
    }

    void exec( const kvs::UInt32 time_index )
    {
        if ( !BaseClass::screen().scene()->hasObject( "BoundaryMesh") )
        {
            if ( m_boundary_mesh.numberOfVertices() > 0 )
            {
                // Register the bounding box at the root rank.
                auto* object = new Polygon();
                object->shallowCopy( m_boundary_mesh );
                object->setName( "BoundaryMesh" );
                BaseClass::screen().registerObject( object, new kvs::Bounds() );
            }
        }

        BaseClass::exec( time_index );
    }

    bool finalize()
    {
        this->dumpTimer();
        return BaseClass::finalize();
    }

    void importBoundaryMesh( const std::string& filename )
    {
        if ( BaseClass::world().rank() == BaseClass::world().root () )
        {
            m_boundary_mesh = kvs::PolygonImporter( filename );
        }
    }
};

inline InSituVis::Pipeline InSituVis::OrthoSlice()
{
    return [&] ( Screen& screen, const Volume& volume )
    {
        // Setup a transfer function.
        const auto min_value = volume.minValue();
        const auto max_value = volume.maxValue();
        auto t = kvs::TransferFunction( kvs::ColorMap::CoolWarm() );
        t.setRange( min_value, max_value );

        // Create new slice objects.
        auto py = ( volume.minObjectCoord().y() + volume.maxObjectCoord().y() ) * 0.5f;
        auto ay = kvs::OrthoSlice::YAxis;
        auto* object_y = new kvs::OrthoSlice( &volume, py, ay, t );
        object_y->setName( volume.name() + "ObjectY");

        auto pz = ( volume.minObjectCoord().z() + volume.maxObjectCoord().z() ) * 0.5f;
        auto az = kvs::OrthoSlice::ZAxis;
        auto* object_z = new kvs::OrthoSlice( &volume, pz, az, t );
        object_z->setName( volume.name() + "ObjectZ");

        kvs::Light::SetModelTwoSide( true );
        if ( screen.scene()->hasObject( volume.name() + "ObjectY") )
        {
            // Update the objects.
            screen.scene()->replaceObject( volume.name() + "ObjectY", object_y );
            screen.scene()->replaceObject( volume.name() + "ObjectZ", object_z );
        }
        else
        {
            // Register the objects with renderer.
            screen.registerObject( object_y, new kvs::glsl::PolygonRenderer() );
            screen.registerObject( object_z, new kvs::glsl::PolygonRenderer() );
        }
    };
}

inline InSituVis::Pipeline InSituVis::Isosurface()
{
    return [&] ( Screen& screen, const Volume& volume )
    {
        // Setup a transfer function.
        const auto min_value = volume.minValue();
        const auto max_value = volume.maxValue();
        auto t = kvs::TransferFunction( kvs::ColorMap::CoolWarm() );
        t.setRange( min_value, max_value );

        // Create new object
        auto i = ( min_value + max_value ) * 0.5f;
        auto n = kvs::Isosurface::PolygonNormal;
        auto d = true;
        auto* object = new kvs::Isosurface( &volume, i, n, d, t );
        object->setName( volume.name() + "Object");

        // Register object and renderer to screen
        kvs::Light::SetModelTwoSide( true );
        if ( screen.scene()->hasObject( volume.name() + "Object") )
        {
            // Update the objects.
            screen.scene()->replaceObject( volume.name() + "Object", object );
        }
        else
        {
            // Register the objects with renderer.
            screen.registerObject( object, new kvs::glsl::PolygonRenderer() );
        }
    };
}

} // end of namspace local
