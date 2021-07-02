#pragma once
#include <kvs/OrthoSlice>
#include <kvs/Isosurface>
#include <kvs/PolygonRenderer>
#include <kvs/UnstructuredVolumeObject>
#include <kvs/Bounds>
#include <kvs/StampTimer>
#include <kvs/StampTimerList>
#include <kvs/Math>
#include <InSituVis/Lib/Adaptor.h>
#include <InSituVis/Lib/Viewpoint.h>
#include <InSituVis/Lib/DistributedViewpoint.h>

#define IN_SITU_VIS__ADAPTIVE_TIMESTEP_CONTROLL

#if defined( IN_SITU_VIS__ADAPTIVE_TIMESTEP_CONTROLL )
#include <InSituVis/Lib/TimestepControlledAdaptor.h>
namespace { using Adaptor = InSituVis::TimestepControlledAdaptor; }
#else
namespace { using Adaptor = InSituVis::Adaptor; }
#endif


namespace local
{

class InSituVis : public ::Adaptor
{
    using BaseClass = ::Adaptor;
    using Object = BaseClass::Object;
    using Volume = kvs::UnstructuredVolumeObject;
    using Screen = BaseClass::Screen;

public:
    static Pipeline OrthoSlice();
    static Pipeline Isosurface();

private:
    kvs::StampTimer m_sim_timer{}; ///< timer for simulation process
    kvs::StampTimer m_cnv_timer{}; ///< timer for conversion process
    kvs::StampTimer m_vis_timer{}; ///< timer for visualization process

public:
    InSituVis(): BaseClass()
    {
        // Common parameters.
        enum { Ortho, Iso } pipeline_type = Ortho; // 'Ortho' or 'Iso'
        enum { Single, Dist } viewpoint_type = Single; // 'Single' or 'Dist'
        this->setImageSize( 1024, 1024 );
        this->setOutputImageEnabled( true );

        // Time intervals.
        this->setVisualizationInterval( 3 ); // l: vis. time interval
#if defined( IN_SITU_VIS__ADAPTIVE_TIMESTEP_CONTROLL )
        this->setDivergenceInterval( 4 ); // L: div. time interval
        this->setSamplingGranularity( 2 ); // R: granularity for the pattern A
        this->setDivergenceThreshold( 0.8 );
#endif

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

    kvs::StampTimer& simTimer() { return m_sim_timer; }
    kvs::StampTimer& cnvTimer() { return m_cnv_timer; }
    kvs::StampTimer& visTimer() { return m_vis_timer; }

    bool dump()
    {
        if ( !BaseClass::dump() ) return false;

        m_sim_timer.setTitle( "Sim time" );
        m_cnv_timer.setTitle( "Cnv time" );
        m_vis_timer.setTitle( "Vis time" );

        const auto dir = BaseClass::outputDirectory().name() + "/";
        kvs::StampTimerList timer_list;
        timer_list.push( m_sim_timer );
        timer_list.push( m_cnv_timer );
        timer_list.push( m_vis_timer );
        return timer_list.write( dir + "proc_time" + ".csv" );
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
            kvs::Light::SetModelTwoSide( true );
            auto* renderer_y = new kvs::glsl::PolygonRenderer();
            auto* renderer_z = new kvs::glsl::PolygonRenderer();
            renderer_y->setTwoSideLightingEnabled( true );
            renderer_z->setTwoSideLightingEnabled( true );
            screen.registerObject( object_y, renderer_y );
            screen.registerObject( object_z, renderer_z );
            screen.registerObject( object_z, new kvs::Bounds() );
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
        auto i = kvs::Math::Mix( min_value, max_value, 0.9 );
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
            screen.registerObject( surface, new kvs::Bounds() );
        }
    };
}

} // end of namspace local
