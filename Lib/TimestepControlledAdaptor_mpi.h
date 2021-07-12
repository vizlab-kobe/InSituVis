/*****************************************************************************/
/**
 *  @file   TimestepControlledAdaptor_mpi.h
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#if defined( KVS_SUPPORT_MPI )
#include "Adaptor_mpi.h"
#include "AdaptiveTimestepController.h"
#include <list>
#include <queue>


namespace InSituVis
{

namespace mpi
{

class TimestepControlledAdaptor : public InSituVis::mpi::Adaptor, public InSituVis::AdaptiveTimestepController
{
public:
    using BaseClass = InSituVis::mpi::Adaptor;
    using Controller = InSituVis::AdaptiveTimestepController;

public:
    TimestepControlledAdaptor( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ): BaseClass( world, root ) {}
    virtual ~TimestepControlledAdaptor() = default;

    virtual void exec( const BaseClass::SimTime sim_time = {} );

private:
    void process( const Data& data );
    float divergence( const Controller::Values& P0, const Controller::Values& P1 );
};

} // end of namespace mpi

} // end of namespace InSituVis

#include "TimestepControlledAdaptor_mpi.hpp"

#endif // KVS_SUPPORT_MPI
