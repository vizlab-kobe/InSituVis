
namespace InSituVis
{

inline void TimestepControlledAdaptor::exec( const BaseClass::SimTime sim_time )
{
    Controller::push( BaseClass::objects() );

    BaseClass::incrementTimeStep();
    BaseClass::clearObjects();
}

inline void TimestepControlledAdaptor::process( const Data& data )
{
    const auto current_step = BaseClass::timeStep();
    {
        // Reset time step, which is used for output filename,
        // for visualizing the stacked dataset.
        const auto L_crr = Controller::dataQueue().size();
        if ( L_crr > 0 )
        {
            const auto l = BaseClass::analysisInterval();
            const auto step = current_step - ( L_crr - 1 ) * l;
            BaseClass::setTimeStep( step );
        }

        // Stack current time step.
        const auto step = static_cast<float>( BaseClass::timeStep() );
        BaseClass::tstepList().stamp( step );

        // Execute vis. pipeline and rendering.
        BaseClass::execPipeline( data );
        BaseClass::execRendering();
    }
    BaseClass::setTimeStep( current_step );
}

} // end of namespace InSituVis
