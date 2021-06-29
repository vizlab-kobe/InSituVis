/*****************************************************************************/
/**
 *  @file   AdaptiveTimeSelector.h
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include "Adaptor.h"
#include "AdaptiveTimestepController.h"
#include <list>
#include <queue>


namespace InSituVis
{

class AdaptiveTimeSelector : public InSituVis::Adaptor, public InSituVis::AdaptiveTimestepController
{
public:
    using BaseClass = InSituVis::Adaptor;
    using Controller = InSituVis::AdaptiveTimestepController;

public:
    AdaptiveTimeSelector() = default;
    virtual ~AdaptiveTimeSelector() = default;

    virtual void exec( const kvs::UInt32 time_index );

private:
    bool canVis();
    void doVis( const Data& data, const kvs::UInt32 time_index );

private:
    virtual float divergence( const Data& data0, const Data& data1 ) const;
};

} // end of namespace InSituVis

#include "AdaptiveTimeSelector.hpp"
