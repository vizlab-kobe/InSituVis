/*****************************************************************************/
/**
 *  @file   AdaptiveTimestepController.h
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <queue>
#include <functional>
#include <kvs/VolumeObjectBase>
#include "Adaptor.h"


namespace InSituVis
{

class AdaptiveTimestepController
{
public:
    using Data = InSituVis::Adaptor::ObjectList;
    using DataQueue = std::queue<Data>;

    using Volume = kvs::VolumeObjectBase;
    using Values = Volume::Values;
    using DivergenceFunction = std::function<float(const Values&,const Values&, const float)>;

    static float GaussianKLDivergence( const Values& P0, const Values& P1, const float D_max );

private:
    size_t m_interval = 1; ///< time interval of divergence calculation
    size_t m_granularity = 0; ///< granularity for coarse grained sampling
    float m_threshold = 0.0f; ///< threshold value for divergence evalution

    DataQueue m_data_queue{}; ///< data queue
    Data m_previous_data{}; ///< dataset at previous time-step
    float m_previous_divergence = 0.0f; ///< divergence for the previous dataset
    DivergenceFunction m_divergence_function = GaussianKLDivergence; ///< divergence function

public:
    AdaptiveTimestepController() = default;
    virtual ~AdaptiveTimestepController() = default;

    size_t divergenceInterval() const { return m_interval; }
    float divergenceThreshold() const { return m_threshold; }
    size_t samplingGranularity() const { return m_granularity; }
    const DataQueue& dataQueue() const { return m_data_queue; }

    void setDivergenceInterval( const size_t interval ) { m_interval = interval; }
    void setDivergenceThreshold( const float threshold ) { m_threshold = threshold; }
    void setSamplingGranularity( const size_t granularity ) { m_granularity = granularity; }
    void setDivergenceFunction( DivergenceFunction func ) { m_divergence_function = func; }

protected:
    void push( const Data& data, const kvs::UInt32 time_index );
    virtual bool canPush() { return true; }
    virtual void process( const Data&, const kvs::UInt32 ) {}
    virtual float divergence( const Values& P0, const Values& P1 );
};

} // end of namespace InSituVis

#include "AdaptiveTimestepController.hpp"
