/*****************************************************************************/
/**
 *  @file   CameraPathControlledAdaptor_mpi.h
 *  @author Ken Iwata, Naohisa Sakamoto
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

class CameraFocusControlledAdaptor : public InSituVis::mpi::Adaptor, public InSituVis::EntropyBasedCameraFocusController
{
public:
    using BaseClass = InSituVis::mpi::Adaptor;
    using FrameBuffer = BaseClass::FrameBuffer;
    using Controller = InSituVis::EntropyBasedCameraFocusController;

    using Viewpoint = InSituVis::Viewpoint; // add
    using Location = Viewpoint::Location;   // add

private:
    bool m_enable_output_image_depth = false;
    bool m_enable_output_evaluation_image = false; ///< if true, all of evaluation images will be output
    bool m_enable_output_evaluation_image_depth = false; ///< if true, all of evaluation depth images will be output
    kvs::mpi::StampTimer m_entr_timer{ BaseClass::world() }; ///< timer for entropy evaluation
    kvs::mpi::StampTimer m_focus_timer{ BaseClass::world() }; ///< timer for entropy evaluation
    size_t m_final_time_step = 0;

    int m_zoom_divid = 5;           // add
    kvs::Vec2i m_ndivs{ 20, 20 };   // add ///< number of divisions for frame buffer
    size_t m_distance_position = 3; // add

public:
    CameraFocusControlledAdaptor( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ): BaseClass( world, root ) {}
    virtual ~CameraFocusControlledAdaptor() = default;

    void setOutputEvaluationImageEnabled(
        const bool enable = true,
        const bool enable_depth = false );

    kvs::mpi::StampTimer& entrTimer() { return m_entr_timer; }
    kvs::mpi::StampTimer& focusTimer() { return m_focus_timer; }

    virtual void exec( const BaseClass::SimTime sim_time = {} );
    virtual bool dump();
    void setFinalTimeStep( const size_t step ) { m_final_time_step = step; }

    // add
    void setNumberOfDivisions( const kvs::Vec2i& ndivs ) { m_ndivs = ndivs; }
    void setDistancePosition( const size_t split ){ m_distance_position = split; }

protected:
    bool isEntropyStep();
    bool isFinalTimeStep();

    virtual void execRendering();
    virtual void process( const Data& data );
    std::string outputFinalImageName( const int space );

    // add
    virtual void process( const Data& data, const float radius, const kvs::Vec3& focus, const kvs::Quat& rotation );

    void outputColorImage(
        const InSituVis::Viewpoint::Location& location,
        const FrameBuffer& frame_buffer,
        const int space ); // add

    void outputDepthImage(
        const InSituVis::Viewpoint::Location& location,
        const FrameBuffer& frame_buffer );

    void outputEntropyTable(
        const std::vector<float> entropies );

    void outputPathEntropies(
        const std::vector<float> path_entropies );

    void outputPathPositions(
        const std::vector<float> path_positions );

    // add
    void outputFocusEntropyTable(
        const std::vector<float> entropies );

// add
private:
    kvs::Vec3 look_at_in_window( const FrameBuffer& frame_buffer );
    kvs::Vec3 window_to_object( const kvs::Vec3 win, const Location& location );
    Location update_location( const Location& location, const kvs::Vec3 at );

    void crop_frame_buffer(
        const FrameBuffer& frame_buffer,
        const kvs::Vec2i& indices,
        FrameBuffer* cropped_frame_buffer );
};

} // end of namespace mpi

} // end of namespace local

#include "CameraFocusControlledAdaptor_mpi.hpp"

#endif // KVS_SUPPORT_MPI
