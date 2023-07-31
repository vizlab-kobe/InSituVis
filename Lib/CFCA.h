/*****************************************************************************/
/**
 *  @file   CameraPathControlledAdaptor_mpi.h
 *  @author Ken Iwata, Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#if defined( KVS_SUPPORT_MPI )
#include <InSituVis/Lib/Adaptor_mpi.h>
#include "EBCFC.h"
#include <list>
#include <queue>



namespace InSituVis
{

namespace mpi
{

class CFCA : public InSituVis::mpi::Adaptor, public InSituVis::EBCFC
{
public:
    using BaseClass = InSituVis::mpi::Adaptor;
    using FrameBuffer = BaseClass::FrameBuffer;
    using Controller = InSituVis::EBCFC;
    using Viewpoint = InSituVis::Viewpoint;
    using Location = Viewpoint::Location;

private:
    bool m_enable_output_image_depth = false;
    bool m_enable_output_evaluation_image = false; ///< if true, all of evaluation images will be output
    bool m_enable_output_evaluation_image_depth = false; ///< if true, all of evaluation depth images will be output
    bool m_enable_output_entropies = false; ///< if true, calculted entropies for all viewpoints will be output
    bool m_enable_output_frame_entropies = false; ///< if true, calculted entropies on the divided framebuffer will be output
    kvs::mpi::StampTimer m_entr_timer{ BaseClass::world() }; ///< timer for entropy evaluation
    kvs::mpi::StampTimer m_focus_timer{ BaseClass::world() }; ///< timer for entropy evaluation
    kvs::mpi::StampTimer m_zoom_timer{ BaseClass::world() }; ///< timer for entropy evaluation

    size_t m_final_time_step = 0;

    size_t m_zoom_level = 1;           // add
    kvs::Vec2ui m_frame_divs{ 20, 20 }; // add ///< number of frame divisions

public:
    CFCA( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ): BaseClass( world, root ) {}
    virtual ~CFCA() = default;

    void setOutputEvaluationImageEnabled( const bool enable = true, const bool enable_depth = false );
    void setOutputEntropiesEnabled( const bool enable = true ) { m_enable_output_entropies = enable; }
    void setOutputFrameEntropiesEnabled( const bool enable = true ) { m_enable_output_frame_entropies = enable; }

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
    virtual void process( const Data& data, const float radius, const kvs::Vec3& focus, const kvs::Quat& rotation ); // add

    std::string outputFinalImageName( const size_t level );

    void outputColorImage(
        const InSituVis::Viewpoint::Location& location,
        const FrameBuffer& frame_buffer,
        const size_t level ); // add

    void outputDepthImage(
        const InSituVis::Viewpoint::Location& location,
        const FrameBuffer& frame_buffer,
        const size_t level );

    void outputEntropies(
        const std::vector<float> entropies );

    void outputPathEntropies(
        const std::vector<float> path_entropies );

    void outputPathPositions(
        const std::vector<float> path_positions );

    // add
    void outputFrameEntropies(
        const std::vector<float> entropies );
    void outputZoomEntropies(
        const std::vector<float> entropies );

// add
private:
    kvs::Vec3 look_at_in_window( const FrameBuffer& frame_buffer );
    kvs::Vec3 look_at_in_window_slide( const FrameBuffer& frame_buffer );
    kvs::Vec3 window_to_object( const kvs::Vec3 win, const Location& location );
    void crop_frame_buffer( const FrameBuffer& frame_buffer, const kvs::Vec2i& indices, FrameBuffer* cropped_frame_buffer );
};

} // end of namespace mpi

} // end of namespace local

#include "CFCA.hpp"

#endif // KVS_SUPPORT_MPI

