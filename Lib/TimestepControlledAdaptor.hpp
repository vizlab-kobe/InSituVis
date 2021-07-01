
namespace InSituVis
{

inline void TimestepControlledAdaptor::execVis( const Data& data, const kvs::UInt32 time_index )
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

inline void TimestepControlledAdaptor::exec( const kvs::UInt32 time_index )
{
    BaseClass::setCurrentTimeIndex( time_index );
    Controller::exec( BaseClass::objects(), time_index );
    BaseClass::incrementTimeCounter();
    BaseClass::clearObjects();
}

} // end of namespace InSituVis
