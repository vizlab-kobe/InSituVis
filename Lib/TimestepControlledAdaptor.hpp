
namespace InSituVis
{

inline void TimestepControlledAdaptor::exec( const kvs::UInt32 time_index )
{
    Controller::push( BaseClass::objects() );

    const auto index = BaseClass::currentTimeIndex();
    BaseClass::setCurrentTimeIndex( index + 1 );
    BaseClass::clearObjects();
}

inline void TimestepControlledAdaptor::process( const Data& data )
{
    const auto current_index = BaseClass::currentTimeIndex();
    {
        // Reset time index, which is used for output filename,
        // for visualizing the stacked dataset.
        const auto L_crr = Controller::dataQueue().size();
        if ( L_crr > 0 )
        {
            const auto l = BaseClass::visualizationInterval();
            const auto index = current_index - ( L_crr - 1 ) * l;
            BaseClass::setCurrentTimeIndex( index );
        }

        // Stack current time index.
        const auto index = static_cast<float>( BaseClass::currentTimeIndex() );
        BaseClass::indexList().stamp( index );

        // Execute vis. pipeline and rendering.
        BaseClass::execPipeline( data );
        BaseClass::execRendering();
    }
    BaseClass::setCurrentTimeIndex( current_index );
}

} // end of namespace InSituVis
