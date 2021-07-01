/*****************************************************************************/
/**
 *  @file   TimestepControlledAdaptor.h
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

class TimestepControlledAdaptor : public InSituVis::Adaptor, public InSituVis::AdaptiveTimestepController
{
public:
    using BaseClass = InSituVis::Adaptor;
    using Controller = InSituVis::AdaptiveTimestepController;

public:
    TimestepControlledAdaptor() = default;
    virtual ~TimestepControlledAdaptor() = default;

    virtual void exec( const kvs::UInt32 time_index );

private:
    bool canVis() { return BaseClass::canVisualize(); }
    void execVis( const Data& data, const kvs::UInt32 time_index );

private:
    virtual float divergence( const Data& data0, const Data& data1 ) const { return 0.0f; }
};

} // end of namespace InSituVis

#include "TimestepControlledAdaptor.hpp"
