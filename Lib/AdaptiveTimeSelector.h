/*****************************************************************************/
/**
 *  @file   AdaptiveTimeSelector.h
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include "Adaptor.h"
#include <list>
#include <queue>


namespace InSituVis
{

class AdaptiveTimeSelector : public InSituVis::Adaptor
{
public:
    using BaseClass = InSituVis::Adaptor;
    using Object = BaseClass::Object;
    using Data = std::list<Object>;
    using DataQueue = std::queue<Data>;

private:
    size_t m_calculation_interval = 1; ///< time interval of KL calculation
    size_t m_granularity = 0; ///< granularity for coarse grained sampling
    float m_threshold = 0.0f; ///< threshold value for similarity evalution based on KL divergence

    Data m_data{}; ///< dataset (set of sub-data)
    DataQueue m_data_queue{}; ///< data queue
    Data m_previous_data{}; ///< dataset at previous time-step
    float m_previous_divergence = 0.0f; ///< KL divergence for the previous dataset

public:
    AdaptiveTimeSelector() = default;
    virtual ~AdaptiveTimeSelector() = default;

    size_t calculationInterval() const { return m_calculation_interval; }
    size_t samplingGranularity() const { return m_granularity; }
    float similarityThreshold() const { return m_threshold; }

    void setCalculationInterval( const size_t interval ) { m_calculation_interval = interval; }
    void setSamplingGranularity( const size_t granularity ) { m_granularity = granularity; }
    void setSimilarityThreshold( const float threshold ) { m_threshold = threshold; }

    virtual void put( const Object& object );
    virtual void exec( const kvs::UInt32 time_index );

private:
    void visualize( const Data& data );
    float divergence( const Data& data0, const Data& data1 ) const
    {
        return 1.0f;
    }
};

} // end of namespace InSituVis

#include "AdaptiveTimeSelector.hpp"
