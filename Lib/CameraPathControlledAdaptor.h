/*****************************************************************************/
/**
 *  @file   CameraPathControlledAdaptor.h
 *  @author Ken Iwata, Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <InSituVis/Lib/Adaptor.h>
#include "EntropyBasedCameraPathController.h"
#include <list>
#include <queue>


namespace InSituVis
{

class CameraPathControlledAdaptor :
        public InSituVis::Adaptor,
        public InSituVis::EntropyBasedCameraPathController
{
public:
    using BaseClass = InSituVis::Adaptor;
    using FrameBuffer = BaseClass::FrameBuffer;
    using Controller = InSituVis::EntropyBasedCameraPathController;
    using Viewpoint = InSituVis::Viewpoint;
    using Location = Viewpoint::Location;

private:
    kvs::StampTimer m_entr_timer{}; ///< timer for entropy evaluation
    size_t m_final_time_step = 0;

public:
    CameraPathControlledAdaptor() = default;
    virtual ~CameraPathControlledAdaptor() = default;

    kvs::StampTimer& entrTimer() { return m_entr_timer; }

    void exec( const BaseClass::SimTime sim_time = {} ) override;
    bool dump() override;
    void setFinalTimeStep( const size_t step ) { m_final_time_step = step; }

protected:
    bool isEntropyStep();
    bool isFinalTimeStep();
    Location erpLocation(
        const size_t index = 999999,
        const Viewpoint::Direction dir = Viewpoint::Uni );

    void execRendering() override;
    void process( const Data& data ) override;
    void process( const Data& data , const float radius, const kvs::Quat& rotation ) override;

    std::string outputColorImageName( const Viewpoint::Location& location );
    std::string outputDepthImageName( const Viewpoint::Location& location );

    void outputColorImage( const Viewpoint::Location& location, const FrameBuffer& frame_buffer );
    void outputDepthImage( const Viewpoint::Location& location, const FrameBuffer& frame_buffer );
};

} // end of namespace InSituVis

#include "CameraPathControlledAdaptor.hpp"
