
namespace InSituVis
{

namespace mpi
{

inline void TimestepControlledAdaptor::exec( const BaseClass::SimTime sim_time )
{
    Controller::push( BaseClass::objects() );

//    const auto index = BaseClass::currentTimeIndex();
    const auto index = BaseClass::timeIndex();
//    BaseClass::setCurrentTimeIndex( index + 1 );
    BaseClass::setTimeIndex( index + 1 );
    BaseClass::clearObjects();
}

inline void TimestepControlledAdaptor::process( const Data& data )
{
//    const auto current_index = BaseClass::currentTimeIndex();
    const auto current_index = BaseClass::timeIndex();
    {
        // Reset time index, which is used for output filename,
        // for visualizing the stacked dataset.
        const auto L_crr = Controller::dataQueue().size();
        if ( L_crr > 0 )
        {
            const auto l = BaseClass::visualizationInterval();
            const auto index = current_index - ( L_crr - 1 ) * l;
//            BaseClass::setCurrentTimeIndex( index );
            BaseClass::setTimeIndex( index );
        }

        // Stack current time index.
//        const auto index = static_cast<float>( BaseClass::currentTimeIndex() );
        const auto index = static_cast<float>( BaseClass::timeIndex() );
        BaseClass::indexList().stamp( index );

        // Execute vis. pipeline and rendering.
        BaseClass::execPipeline( data );
        BaseClass::execRendering();
    }
//    BaseClass::setCurrentTimeIndex( current_index );
    BaseClass::setTimeIndex( current_index );
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
