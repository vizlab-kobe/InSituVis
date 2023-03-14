/*****************************************************************************/
/**
 *  @file   CameraPathControlledAdaptor_mpi.h
 *  @author Ken Iwata, Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#if defined( KVS_SUPPORT_MPI )
#include <InSituVis/Lib/Adaptor_mpi.h>
#include "EntropyBasedCameraPathController.h"
#include <list>
#include <queue>


namespace InSituVis
{

namespace mpi
{

class CameraPathControlledAdaptor : public InSituVis::mpi::Adaptor, public InSituVis::EntropyBasedCameraPathController
{
public:
    using BaseClass = InSituVis::mpi::Adaptor;
    using FrameBuffer = BaseClass::FrameBuffer;
    using Controller = InSituVis::EntropyBasedCameraPathController;

private:
    bool m_enable_output_image_depth = false;
    bool m_enable_output_evaluation_image = false; ///< if true, all of evaluation images will be output
    bool m_enable_output_evaluation_image_depth = false; ///< if true, all of evaluation depth images will be output
    bool m_enable_output_entropies = false; ///< if true, calculted entropies for all viewpoints will be output
    kvs::mpi::StampTimer m_entr_timer{ BaseClass::world() }; ///< timer for entropy evaluation
    size_t m_final_time_step = 0;

public:
    CameraPathControlledAdaptor( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ): BaseClass( world, root ) {}
    virtual ~CameraPathControlledAdaptor() = default;

    void setOutputEvaluationImageEnabled(
        const bool enable = true,
        const bool enable_depth = false );

    void setOutputEntropiesEnabled( const bool enable = true ) { m_enable_output_entropies = enable; }

    kvs::mpi::StampTimer& entrTimer() { return m_entr_timer; }

    virtual void exec( const BaseClass::SimTime sim_time = {} );
    virtual bool dump();
    void setFinalTimeStep( const size_t step ) { m_final_time_step = step; }

protected:
    bool isEntropyStep();
    bool isFinalTimeStep();
    InSituVis::Viewpoint::Location erpLocation(
        const size_t index = 999999,
        const InSituVis::Viewpoint::Direction dir = InSituVis::Viewpoint::Uni );

    virtual void execRendering();
    virtual void process( const Data& data );
    virtual void process( const Data& data , const float radius, const kvs::Quaternion& rotation );

    std::string outputDepthImageName( const Viewpoint::Location& location );

    void outputColorImage(
        const InSituVis::Viewpoint::Location& location,
        const FrameBuffer& frame_buffer );

    void outputDepthImage(
        const InSituVis::Viewpoint::Location& location,
        const FrameBuffer& frame_buffer );

    void outputEntropies(
        const std::vector<float> entropies );

    void outputPathEntropies(
        const std::vector<float> path_entropies );

    void outputPathPositions(
        const std::vector<float> path_positions );

    void outputPathCalcTimes(
        const std::vector<float> path_calc_times );

    void outputViewpointCoords();
};

} // end of namespace mpi

} // end of namespace InSituVis

#include "CameraPathControlledAdaptor_mpi.hpp"

#endif // KVS_SUPPORT_MPI
