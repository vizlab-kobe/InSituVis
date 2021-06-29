
namespace InSituVis
{

inline bool AdaptiveTimeSelector::canVis()
{
    return BaseClass::canVisualize();
}

inline void AdaptiveTimeSelector::doVis( const Data& data, const kvs::UInt32 time_index )
{
    // Reset time index, which is used for output filename, for visualizing the stacked dataset.
    if ( Controller::dataQueue().size() > 0 )
    {
        const auto index = time_index - Controller::dataQueue().size() + 1;
        BaseClass::setCurrentTimeIndex( index );
    }

    // Execute vis. pipeline and rendering.
    BaseClass::doPipeline( data );
    BaseClass::doRendering();
}

inline void AdaptiveTimeSelector::exec( const kvs::UInt32 time_index )
{
    BaseClass::setCurrentTimeIndex( time_index );
    Controller::exec( BaseClass::objects(), time_index );
    BaseClass::incrementTimeCounter();
    BaseClass::clearObjects();
}

float AdaptiveTimeSelector::divergence( const Data& data0, const Data& data1 ) const
{
    return 0.0f;
}

} // end of namespace InSituVis
