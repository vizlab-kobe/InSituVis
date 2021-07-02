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
    bool canPush() { return BaseClass::canVisualize(); }
    void process( const Data& data, const kvs::UInt32 time_index );
};

} // end of namespace InSituVis

#include "TimestepControlledAdaptor.hpp"
#include "TimestepControlledAdaptor_mpi.h"
