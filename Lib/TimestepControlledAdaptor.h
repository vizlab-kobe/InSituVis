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

    void exec( const BaseClass::SimTime sim_time = {} ) override;

private:
    void process( const Data& data ) override;
};

} // end of namespace InSituVis

#include "TimestepControlledAdaptor.hpp"
#include "TimestepControlledAdaptor_mpi.h"
