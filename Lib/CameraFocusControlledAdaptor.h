/*****************************************************************************/
/**
 *  @file   CameraFocusControlledAdaptor.h
 *  @author Ken Iwata, Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <InSituVis/Lib/Adaptor.h>
#include "EntropyBasedCameraFocusController.h"
#include <list>
#include <queue>


namespace InSituVis
{

class CameraFocusControlledAdaptor :
        public InSituVis::Adaptor,
        public InSituVis::EntropyBasedCameraFocusController
{
public:
    using BaseClass = InSituVis::Adaptor;
    using FrameBuffer = BaseClass::FrameBuffer;
    using Controller = InSituVis::EntropyBasedCameraFocusController;
    using Viewpoint = InSituVis::Viewpoint;
    using Location = Viewpoint::Location;

private:
    kvs::StampTimer m_entr_timer{}; ///< timer for entropy evaluation
    kvs::StampTimer m_focus_timer{}; ///< timer for entropy evaluation
    kvs::StampTimer m_zoom_timer{}; ///< timer for entropy evaluation
    size_t m_final_time_step = 0;
    size_t m_zoom_level = 1; ///< zoom level
    kvs::Vec2ui m_frame_divs{ 1, 1 }; ///< number of frame divisions

public:
    CameraFocusControlledAdaptor() = default;
    virtual ~CameraFocusControlledAdaptor() = default;

    kvs::StampTimer& entrTimer() { return m_entr_timer; }
    kvs::StampTimer& focusTimer() { return m_focus_timer; }
    kvs::StampTimer& zoomTimer() { return m_zoom_timer; }
    size_t zoomLevel() const { return m_zoom_level; }
    const kvs::Vec2ui& frameDivisions() const { return m_frame_divs; }

    void setFinalTimeStep( const size_t step ) { m_final_time_step = step; }
    void setZoomLevel( const size_t level ) { m_zoom_level = level; }
    void setFrameDivisions( const kvs::Vec2ui& divs ) { m_frame_divs = divs; }
    void setViewpoint( const Viewpoint& viewpoint );

    void exec( const BaseClass::SimTime sim_time = {} ) override;
    bool dump() override;

protected:
    using Controller::process;

    bool isEntropyStep();
    bool isFinalTimeStep();
    Location erpLocation(
        const kvs::Vec3 focus,
        const size_t index = 999999,
        const Viewpoint::Direction dir = Viewpoint::Uni );
    Location focusedLocation(
        const Location& location,
        const kvs::Vec3 focus );

    void execRendering() override;
    void process( const Data& data ) override;
    void process( const Data& data, const float radius, const kvs::Quat& rotation, const kvs::Vec3& focus ) override;

    std::string outputFinalImageName( const size_t level );
    void outputColorImage( const Viewpoint::Location& location, const FrameBuffer& frame_buffer, const size_t level );
    void outputDepthImage( const Viewpoint::Location& location, const FrameBuffer& frame_buffer, const size_t level);

    void outputZoomEntropies(
        const std::vector<float> entropies );

private:
    kvs::Vec3 look_at_in_window( const FrameBuffer& frame_buffer );
    kvs::Vec3 window_to_object( const kvs::Vec3 win, const Location& location );
    FrameBuffer crop_frame_buffer( const FrameBuffer& frame_buffer, const kvs::Vec2i& indices );
    kvs::Quat rotation( const kvs::Vec3& p );
};

} // end of namespace InSituVis

#include "CameraFocusControlledAdaptor.hpp"
