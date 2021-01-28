#pragma once
#include <kvs/OrthoSlice>
#include <kvs/Isosurface>
#include <kvs/PolygonObject>
#include <kvs/PolygonRenderer>
#include <kvs/PolygonImporter>
#include <kvs/Bounds>
#include <kvs/String>
#include <kvs/StampTimer>
#include <kvs/StampTimerList>
#include <kvs/mpi/StampTimer>
#include <InSituVis/Lib/Adaptor.h>
#include <InSituVis/Lib/Viewpoint.h>
#include <InSituVis/Lib/DistributedViewpoint.h>
//#include <InSituVis/Lib/StampTimer.h>
//#include <InSituVis/Lib/StampTimerTable.h>


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
    kvs::mpi::StampTimer m_sim_timer; ///< timer for simulation process
    kvs::mpi::StampTimer m_imp_timer; ///< timer for imporing process
    kvs::mpi::StampTimer m_vis_timer; ///< timer for visualization process

public:
    InSituVis( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ):
        BaseClass( world, root ),
        m_viewpoint( {3,3,3}, Viewpoint::CubicDist, Viewpoint::SingleDir ), // OK
        //m_viewpoint( {3,3,3}, Viewpoint::CubicDist, Viewpoint::OmniDir ), // OK
        //m_viewpoint( {3,3,3}, Viewpoint::CubicDist, Viewpoint::AdaptiveDir ), // NG
        //m_viewpoint( {3,3,3}, Viewpoint::SphericalDist, Viewpoint::SingleDir ), // OK
        //m_viewpoint( {3,3,3}, Viewpoint::SphericalDist, Viewpoint::OmniDir ), // OK
        //m_viewpoint( {3,3,3}, Viewpoint::SphericalDist, Viewpoint::AdaptiveDir ), // NG
        m_sim_timer( BaseClass::world() ),
        m_imp_timer( BaseClass::world() ),
        m_vis_timer( BaseClass::world() )
    {
        m_viewpoint.generate();

        this->setImageSize( 1024, 1024 );
        this->setOutputImageEnabled( true );
        this->setOutputSubImageEnabled( false, false, false ); // color, depth, alpha
        this->setTimeInterval( 5 );
        this->setViewpoint( m_viewpoint );
        this->setPipeline( local::InSituVis::OrthoSlice() );
        //this->setPipeline( local::InSituVis::Isosurface() );
    }

    kvs::mpi::StampTimer& simTimer() { return m_sim_timer; }
    kvs::mpi::StampTimer& impTimer() { return m_imp_timer; }
    kvs::mpi::StampTimer& visTimer() { return m_vis_timer; }

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

    void importBoundaryMesh( const std::string& filename )
    {
        if ( BaseClass::world().rank() == BaseClass::world().root () )
        {
            m_boundary_mesh = kvs::PolygonImporter( filename );
        }
    }

private:
    virtual bool dump()
    {
        if ( !BaseClass::dump() ) return false;

        // For each node
        m_sim_timer.setTitle( "Sim time" );
        m_imp_timer.setTitle( "Imp time" );
        m_vis_timer.setTitle( "Vis time" );

        const std::string rank = kvs::String::From( this->world().rank(), 4, '0' );
        const std::string subdir = BaseClass::outputDirectory().name() + "/";
        kvs::StampTimerList timer_list;
        timer_list.push( m_sim_timer );
        timer_list.push( m_imp_timer );
        timer_list.push( m_vis_timer );
        if ( !timer_list.write( subdir + "proc_time_" + rank + ".csv" ) ) return false;

        // For root node
        auto sim_time_min = m_sim_timer; sim_time_min.reduceMin();
        auto sim_time_max = m_sim_timer; sim_time_max.reduceMax();
        auto sim_time_ave = m_sim_timer; sim_time_ave.reduceAve();
        auto imp_time_min = m_imp_timer; imp_time_min.reduceMin();
        auto imp_time_max = m_imp_timer; imp_time_max.reduceMax();
        auto imp_time_ave = m_imp_timer; imp_time_ave.reduceAve();
        auto vis_time_min = m_vis_timer; vis_time_min.reduceMin();
        auto vis_time_max = m_vis_timer; vis_time_max.reduceMax();
        auto vis_time_ave = m_vis_timer; vis_time_ave.reduceAve();

        if ( !this->world().isRoot() ) return true;

        sim_time_min.setTitle( "Sim time (min)" );
        sim_time_max.setTitle( "Sim time (max)" );
        sim_time_ave.setTitle( "Sim time (ave)" );
        imp_time_min.setTitle( "Imp time (min)" );
        imp_time_max.setTitle( "Imp time (max)" );
        imp_time_ave.setTitle( "Imp time (ave)" );
        vis_time_min.setTitle( "Vis time (min)" );
        vis_time_max.setTitle( "Vis time (max)" );
        vis_time_ave.setTitle( "Vis time (ave)" );

        timer_list.clear();
        timer_list.push( sim_time_min );
        timer_list.push( sim_time_max );
        timer_list.push( sim_time_ave );
        timer_list.push( imp_time_min );
        timer_list.push( imp_time_max );
        timer_list.push( imp_time_ave );
        timer_list.push( vis_time_min );
        timer_list.push( vis_time_max );
        timer_list.push( vis_time_ave );

        const auto basedir = BaseClass::outputDirectory().baseDirectoryName() + "/";
        return timer_list.write( basedir + "proc_time.csv" );
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
