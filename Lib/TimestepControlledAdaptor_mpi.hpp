
namespace InSituVis
{

namespace mpi
{

inline void TimestepControlledAdaptor::exec( const kvs::UInt32 time_index )
{
    BaseClass::setCurrentTimeIndex( time_index );
    Controller::push( BaseClass::objects(), time_index );
    BaseClass::incrementTimeCounter();
    BaseClass::clearObjects();
}

inline void TimestepControlledAdaptor::process( const Data& data, const kvs::UInt32 time_index )
{
    // Reset time index, which is used for output filename, for visualizing the stacked dataset.
    if ( Controller::dataQueue().size() > 0 )
    {
        const auto l = BaseClass::visualizationInterval();
        const auto index = time_index - ( Controller::dataQueue().size() - 1 ) * l;
        BaseClass::setCurrentTimeIndex( index );
    }

    // Execute vis. pipeline and rendering.
    BaseClass::execPipeline( data );
    BaseClass::execRendering();
}

inline float TimestepControlledAdaptor::divergence(
    const Controller::Values& P0,
    const Controller::Values& P1 )
{
    auto D = Controller::divergence( P0, P1 );
    BaseClass::world().allReduce( D, D, MPI_MAX );
    return D;
}

} // end of namespace mpi

} // end of namespace InSituVis
