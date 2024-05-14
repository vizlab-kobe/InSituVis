/*****************************************************************************/
/**
 *  @file   CameraFocusControlledAdaptor_mpi.h
 *  @author Taisei Matsushima, Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#if defined( KVS_SUPPORT_MPI )
#include <InSituVis/Lib/Adaptor_mpi.h>
#include "EntropyBasedCameraFocusController.h"
#include <list>
#include <queue>


namespace InSituVis
{

namespace mpi
{

class CameraFocusControlledAdaptor :
        public InSituVis::mpi::Adaptor,
        public InSituVis::EntropyBasedCameraFocusController
{
public:
    using BaseClass = InSituVis::mpi::Adaptor;
    using FrameBuffer = BaseClass::FrameBuffer;
    using Controller = InSituVis::EntropyBasedCameraFocusController;
    using Viewpoint = InSituVis::Viewpoint;
    using Location = Viewpoint::Location;

private:
    bool m_enable_output_image_depth = false;
    kvs::mpi::StampTimer m_entr_timer{ BaseClass::world() }; ///< timer for entropy evaluation
    kvs::mpi::StampTimer m_focus_timer{ BaseClass::world() }; ///< timer for entropy evaluation
    kvs::mpi::StampTimer m_zoom_timer{ BaseClass::world() }; ///< timer for entropy evaluation
    size_t m_final_time_step = 0;

    // add
    size_t m_zoom_level = 1; ///< zoom level
    kvs::Vec2ui m_frame_divs{ 1, 1 }; ///< number of frame divisions

public:
    CameraFocusControlledAdaptor( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ): BaseClass( world, root ) {}
    virtual ~CameraFocusControlledAdaptor() = default;

    kvs::mpi::StampTimer& entrTimer() { return m_entr_timer; }
    kvs::mpi::StampTimer& focusTimer() { return m_focus_timer; }
    kvs::mpi::StampTimer& zoomTimer() { return m_zoom_timer; }
    virtual void exec( const BaseClass::SimTime sim_time = {} );
    virtual bool dump();
    void setFinalTimeStep( const size_t step ) { m_final_time_step = step; }

    // add
    size_t zoomLevel() const { return m_zoom_level; }
    const kvs::Vec2ui& frameDivisions() const { return m_frame_divs; }
    void setZoomLevel( const size_t level ) { m_zoom_level = level; }
    void setFrameDivisions( const kvs::Vec2ui& divs ) { m_frame_divs = divs; }

protected:
    bool isEntropyStep();
    bool isFinalTimeStep();
    Location erpLocation(
        const kvs::Vec3 focus,
        const size_t index = 999999,
        const Viewpoint::Direction dir = Viewpoint::Uni );
    Location focusedLocation(
        const Location& location,
        const kvs::Vec3 focus );

    virtual void execRendering();
    virtual void process( const Data& data );
    virtual void process( const Data& data, const float radius, const kvs::Quaternion& rotation, const kvs::Vec3& focus );

    std::string outputFinalImageName( const size_t level );

    void outputColorImage( const Viewpoint::Location& location, const FrameBuffer& frame_buffer, const size_t level ); // add
    void outputDepthImage( const Viewpoint::Location& location, const FrameBuffer& frame_buffer, const size_t level);

    void outputZoomEntropies(
        const std::vector<float> entropies );

private:
    // add
    kvs::Vec3 look_at_in_window( const FrameBuffer& frame_buffer );
    kvs::Vec3 window_to_object( const kvs::Vec3 win, const Location& location );
    FrameBuffer crop_frame_buffer( const FrameBuffer& frame_buffer, const kvs::Vec2i& indices );
    kvs::Quat rotation( const kvs::Vec3& p );
};

} // end of namespace mpi

} // end of namespace local

#include "CameraFocusControlledAdaptor_mpi.hpp"

#endif // KVS_SUPPORT_MPI
