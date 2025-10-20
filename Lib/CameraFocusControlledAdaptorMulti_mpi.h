/*****************************************************************************/
/**
 *  @file   CameraFocusControlledAdaptorMulti_mpi.h
 *  @author Taisei Matsushima, Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#if defined( KVS_USE_MPI )
#include <InSituVis/Lib/Adaptor_mpi.h>
#include "EntropyBasedCameraFocusControllerMulti.h"
#include <list>
#include <queue>


namespace InSituVis
{

namespace mpi
{

class CameraFocusControlledAdaptorMulti :
        public InSituVis::mpi::Adaptor,
        public InSituVis::EntropyBasedCameraFocusControllerMulti
{
public:
    using BaseClass = InSituVis::mpi::Adaptor;
    using FrameBuffer = BaseClass::FrameBuffer;
    using Controller = InSituVis::EntropyBasedCameraFocusControllerMulti;
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
    int m_route_num;

    kvs::Vec2ui m_frame_divs{ 1, 1 }; ///< number of frame divisions
    kvs::Real32 m_depth = 0.5;

public:
    CameraFocusControlledAdaptorMulti( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ): BaseClass( world, root ) {}
    virtual ~CameraFocusControlledAdaptorMulti() = default;

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
    void setDepth( const kvs::Real32& depth ){ m_depth = depth; }
    const kvs::Real32& depth() const { return m_depth; } 

    //add
    int routeNum() const { return m_route_num; }
    void setRouteNum( const int route ){ m_route_num = route; }

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
    virtual void process( const Data& data, const float radius, const kvs::Quaternion& rotation, const kvs::Vec3& focus, const int route_num );

    std::string outputFinalImageName( const size_t numImages, const size_t level, const size_t from_to );

    void outputColorImage( const Viewpoint::Location& location, const FrameBuffer& frame_buffer, const size_t numImages, const size_t level, const size_t route_num ); // add
    void outputDepthImage( const Viewpoint::Location& location, const FrameBuffer& frame_buffer, const size_t numImages, const size_t level, const size_t route_num );

    void outputZoomEntropies(
        const std::vector<float> entropies );

private:
    // add
    std::vector<kvs::Vec3> look_at_in_window( const FrameBuffer& frame_buffer );
    kvs::Vec3 window_to_object( const kvs::Vec3& win, const Location& location );
    FrameBuffer crop_frame_buffer( const FrameBuffer& frame_buffer, const kvs::Vec2i& indices );
    kvs::Quat rotation( const kvs::Vec3& p );
    std::vector<kvs::Vec3> biggestEntropyPoint(  std::vector<float> entropies,  std::vector<kvs::Vec2i> centers,  std::vector<kvs::Real32> depthes );
    std::vector<kvs::Vec3> maximalEntropyPoint(std::vector<float> entropies,  std::vector<kvs::Vec2i> centers,  std::vector<kvs::Real32> depthes );

};

} // end of namespace mpi

} // end of namespace local

#include "CameraFocusControlledAdaptorMulti_mpi.hpp"

#endif // KVS_USE_MPI
