/*****************************************************************************/
/**
 *  @file   CameraPathTimeStepControlledAdaptor_mpi.h
 *  @author Ken Iwata, Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#if defined( KVS_SUPPORT_MPI )
#include <InSituVis/Lib/Adaptor_mpi.h>
#include "EntropyBasedCameraPathTimeStepController.h"
#include <list>
#include <queue>


namespace InSituVis
{

namespace mpi
{

class CameraPathTimeStepControlledAdaptor :
        public InSituVis::mpi::Adaptor,
        public InSituVis::EntropyBasedCameraPathTimeStepController
{
public:
    using BaseClass = InSituVis::mpi::Adaptor;
    using FrameBuffer = BaseClass::FrameBuffer;
    using Controller = InSituVis::EntropyBasedCameraPathTimeStepController;
    using Viewpoint = InSituVis::Viewpoint;
    using Location = Viewpoint::Location;

private:
    bool m_enable_output_image_depth = false;
    // bool m_enable_output_evaluation_image = true;
    bool m_enable_output_evaluation_image = false;
    bool m_enable_output_evaluation_image_depth = false;
    kvs::mpi::StampTimer m_entr_timer{ BaseClass::world() }; ///< timer for entropy evaluation
    size_t m_final_time_step = 0;

public:
    CameraPathTimeStepControlledAdaptor( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ): BaseClass( world, root ) {}
    virtual ~CameraPathTimeStepControlledAdaptor() = default;

    kvs::mpi::StampTimer& entrTimer() { return m_entr_timer; }

    virtual void exec( const BaseClass::SimTime sim_time = {} );
    virtual bool dump();
    void setFinalTimeStep( const size_t step ) { m_final_time_step = step; }

    float divergence( const Controller::Values& P0, const Controller::Values& P1 );

protected:
    bool isEntropyStep();
    bool isFinalTimeStep();
    Location erpLocation(
        const size_t index = 999999,
        const Viewpoint::Direction dir = Viewpoint::Uni );

    virtual void execRendering();
    virtual void process( const Data& data );
    virtual void process( const Data& data , const float radius, const kvs::Quaternion& rotation );

    std::string outputDepthImageName( const Viewpoint::Location& location );

    void outputColorImage( const Viewpoint::Location& location, const FrameBuffer& frame_buffer );
    void outputDepthImage( const Viewpoint::Location& location, const FrameBuffer& frame_buffer );
};

} // end of namespace mpi

} // end of namespace InSituVis

#include "CameraPathTimeStepControlledAdaptor_mpi.hpp"

#endif // KVS_SUPPORT_MPI
