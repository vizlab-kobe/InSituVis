#pragma once
#include <kvs/OrthoSlice>
#include <kvs/Isosurface>
#include <kvs/PolygonObject>
#include <kvs/PolygonRenderer>
#include <kvs/PolygonImporter>
#include <kvs/UnstructuredVolumeObject>
#include <kvs/Bounds>
#include <kvs/String>
#include <kvs/StampTimer>
#include <kvs/StampTimerList>
#include <kvs/Math>
#include <kvs/mpi/StampTimer>
#include <InSituVis/Lib/Adaptor.h>
#include <InSituVis/Lib/Viewpoint.h>
#include <InSituVis/Lib/DistributedViewpoint.h>
#include <InSituVis/Lib/TimestepControlledAdaptor.h>


namespace local
{

class InSituVis : public ::InSituVis::mpi::Adaptor
{
    using BaseClass = ::InSituVis::mpi::Adaptor;
    using Object = BaseClass::Object;
    using Volume = kvs::UnstructuredVolumeObject;
    using Screen = BaseClass::Screen;

public:
    static Pipeline OrthoSlice();
    static Pipeline Isosurface();

private:
    kvs::PolygonObject m_boundary_mesh; ///< boundary mesh
    kvs::mpi::StampTimer m_sim_timer{ BaseClass::world() }; ///< timer for sim. process
    kvs::mpi::StampTimer m_imp_timer{ BaseClass::world() }; ///< timer for impor process
    kvs::mpi::StampTimer m_vis_timer{ BaseClass::world() }; ///< timer for vis. process

public:
    InSituVis( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ): BaseClass( world, root )
    {
        // Common parameters.
        enum { Ortho, Iso } pipeline_type = Ortho; // 'Ortho' or 'Iso'
        enum { Single, Dist } viewpoint_type = Single; // 'Single' or 'Dist'
        this->setImageSize( 1024, 1024 );
        this->setOutputImageEnabled( true );
        this->setOutputSubImageEnabled( false, false, false ); // color, depth, alpha

        // Time interval.
        this->setTimeInterval( 5 ); // vis. time interval

        // Set visualization pipeline.
        switch ( pipeline_type )
        {
        case Ortho:
            this->setPipeline( local::InSituVis::OrthoSlice() );
            break;
        case Iso:
            this->setPipeline( local::InSituVis::Isosurface() );
            break;
        default: break;
        }

        // Set viewpoint(s)
        switch ( viewpoint_type )
        {
        case Single:
        {
            using Viewpoint = ::InSituVis::Viewpoint;
            const auto p = kvs::Vec3( 0, 0, 12 );
            this->setViewpoint( Viewpoint{ p } );
            break;
        }
        case Dist:
        {
            using Viewpoint = ::InSituVis::DistributedViewpoint;
            const auto dim = kvs::Vec3ui( 3, 3, 3 );
            const auto dist = Viewpoint::CubicDist;
            //const auto dist = Viewpoint::SphericalDist;
            const auto dir = Viewpoint::SingleDir;
            //const auto dir = Viewpoint::OmniDir;
            //const auto dir = Viewpoint::AdaptiveDir;
            this->setViewpoint( Viewpoint{ dim, dist, dir } );
        }
        default: break;
        }
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
                auto* object = new kvs::PolygonObject();
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

    bool dump()
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
    return [&] ( Screen& screen, const Object& object )
    {
        const auto& volume = dynamic_cast<const Volume&>( object );
        if ( volume.numberOfCells() == 0 ) { return; }

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
            auto* renderer_y = new kvs::glsl::PolygonRenderer();
            auto* renderer_z = new kvs::glsl::PolygonRenderer();
            renderer_y->setTwoSideLightingEnabled( true );
            renderer_z->setTwoSideLightingEnabled( true );
            screen.registerObject( object_y, renderer_y );
            screen.registerObject( object_z, renderer_z );
        }
    };
}

inline InSituVis::Pipeline InSituVis::Isosurface()
{
    return [&] ( Screen& screen, const Object& object )
    {
        const auto& volume = dynamic_cast<const Volume&>( object );
        if ( volume.numberOfCells() == 0 ) { return; }

        // Setup a transfer function.
        const auto min_value = volume.minValue();
        const auto max_value = volume.maxValue();
        auto t = kvs::TransferFunction( kvs::ColorMap::CoolWarm() );
        t.setRange( min_value, max_value );

        // Create new object
        auto i = kvs::Math::Mix( min_value, max_value, 0.5 );
        auto n = kvs::Isosurface::PolygonNormal;
        auto d = true;
        auto* surface = new kvs::Isosurface( &volume, i, n, d, t );
        surface->setName( volume.name() + "Object");

        // Register object and renderer to screen
        kvs::Light::SetModelTwoSide( true );
        if ( screen.scene()->hasObject( volume.name() + "Object") )
        {
            // Update the objects.
            screen.scene()->replaceObject( volume.name() + "Object", surface );
        }
        else
        {
            // Register the objects with renderer.
            auto* renderer = new kvs::glsl::PolygonRenderer();
            renderer->setTwoSideLightingEnabled( true );
            screen.registerObject( surface, renderer );
        }
    };
}

} // end of namspace local
