namespace InSituVis
{

inline void EntropyBasedCameraFocusController::push( const Data& data )
{
    const auto interval = BaseClass::entropyInterval();

    if ( !( BaseClass::isFinalStep() ) )
    {
        if ( BaseClass::previousData().empty() )
        {
            // Initial step.
            this->process( data );
            BaseClass::setPreviousData( data );
            BaseClass::pushMaxEntropies( BaseClass::maxEntropy() );
            BaseClass::pushMaxPositions( BaseClass::maxPosition() );
            BaseClass::pushMaxRotations( BaseClass::maxRotation() );
            BaseClass::pushMaxRotations( BaseClass::maxRotation() );
            BaseClass::dataQueue().push( data );
            this->pushMaxFocusPoints( this->maxFocusPoint() ); // add
        }
        else
        {
            if ( BaseClass::isCacheEnabled() )
            {
                if ( BaseClass::dataQueue().size() % interval == 0 )
                {
                    this->process( data );
                    BaseClass::pushMaxEntropies( BaseClass::maxEntropy() );
                    BaseClass::pushMaxPositions( BaseClass::maxPosition() );
                    BaseClass::pushMaxRotations( BaseClass::maxRotation() );
                    this->pushMaxFocusPoints( this->maxFocusPoint() ); // add

                    if ( BaseClass::maxRotations().size() == 4 )
                    {
                        const auto q1 = BaseClass::maxRotations().front(); BaseClass::maxRotations().pop();
                        const auto q2 = BaseClass::maxRotations().front(); BaseClass::maxRotations().pop();
                        const auto q3 = BaseClass::maxRotations().front(); BaseClass::maxRotations().pop();
                        const auto q4 = BaseClass::maxRotations().front(); BaseClass::maxRotations().pop();

                        const auto p2 = BaseClass::maxPositions().front(); BaseClass::maxPositions().pop();
                        const auto p3 = BaseClass::maxPositions().front();

                        const auto r2 = p2.length();
                        const auto r3 = p3.length();

                        const auto f2 = this->maxFocusPoints().front(); this->maxFocusPoints().pop(); // add
                        const auto f3 = this->maxFocusPoints().front();                               // add
                        this->createPath( r2, r3, f2, f3, q1, q2, q3, q4, interval );                 // mod

                        BaseClass::dataQueue().pop();
                        BaseClass::pushPathEntropies( BaseClass::maxEntropies().front() );
                        BaseClass::maxEntropies().pop();
                        BaseClass::pushPathPositions( p2 );
                        this->pushFocusPathPositions( f2 ); // add

                        for ( size_t i = 0; i < interval - 1; i++ )
                        {
                            const auto data_front = BaseClass::dataQueue().front();
                            const auto [ radius, rotation ] = BaseClass::path().front();
                            const auto focus = this->focusPath().front();         // add
                            this->process( data_front, radius, focus, rotation ); // mod
                            BaseClass::dataQueue().pop();
                            BaseClass::path().pop();
                            this->focusPath().pop(); // add

                            BaseClass::pushPathEntropies( BaseClass::maxEntropy() );
                            BaseClass::pushPathPositions( BaseClass::maxPosition() );
                            this->pushFocusPathPositions( this->maxFocusPoint() ); // add
                        }

                        BaseClass::pushMaxRotations( q2 );
                        BaseClass::pushMaxRotations( q3 );
                        BaseClass::pushMaxRotations( q4 );
                    }
                    BaseClass::dataQueue().push( data );
                }
                else
                {
                    BaseClass::dataQueue().push( data );
                }
            }
        }
    }
    else
    {
        const auto q1 = BaseClass::maxRotations().front(); BaseClass::maxRotations().pop();
        const auto q2 = BaseClass::maxRotations().front(); BaseClass::maxRotations().pop();
        const auto q3 = BaseClass::maxRotations().front(); BaseClass::maxRotations().pop();
        const auto q4 = q3;

        const auto p2 = BaseClass::maxPositions().front(); BaseClass::maxPositions().pop();
        const auto p3 = BaseClass::maxPositions().front();

        const auto r2 = p2.length();
        const auto r3 = p3.length();

        const auto f2 = this->maxFocusPoints().front(); this->maxFocusPoints().pop(); // add
        const auto f3 = this->maxFocusPoints().front();                               // add
        this->createPath( r2, r3, f2, f3, q1, q2, q3, q4, interval );                 // mod

        BaseClass::dataQueue().pop();
        BaseClass::pushPathEntropies( BaseClass::maxEntropies().front() );
        BaseClass::maxEntropies().pop();
        BaseClass::pushPathPositions( p2 );
        this->pushFocusPathPositions( f2 ); // add

        for ( size_t i = 0; i < interval - 1; i++ )
        {
            const auto data_front = BaseClass::dataQueue().front();
//            const auto [ radius, rotation ] = BaseClass::path().front();
            const auto radius = BaseClass::path().front().first;
            const auto rotation = BaseClass::path().front().second;
            const auto focus = this->focusPath().front();         // add
            this->process( data_front, radius, focus, rotation ); // mod
            BaseClass::dataQueue().pop();
            BaseClass::path().pop();
            this->focusPath().pop(); // add

            BaseClass::pushPathEntropies( BaseClass::maxEntropy() );
            BaseClass::pushPathPositions( BaseClass::maxPosition() );
            this->pushFocusPathPositions( this->maxFocusPoint() ); // add
        }

        while ( BaseClass::dataQueue().size() > 0 )
        {
            std::queue<std::tuple<float, kvs::Quat>> empty;
            BaseClass::path().swap( empty );

            std::queue<kvs::Vec3> empty_focus;     // add
            this->focusPath().swap( empty_focus ); // add

            for ( size_t i = 0; i < interval - 1; i++ )
            {
                BaseClass::path().push( { r3, q3 } );
                this->focusPath().push( f3 ); // add
            }

            BaseClass::dataQueue().pop();
            BaseClass::pushPathEntropies( BaseClass::maxEntropies().front() );
            BaseClass::maxEntropies().pop();
            BaseClass::pushPathPositions( p3 );
            this->pushFocusPathPositions( f3 ); // add

            for ( size_t i = 0; i < interval - 1; i++ )
            {
                const auto data_front = BaseClass::dataQueue().front();
                //const auto [ radius, rotation ] = BaseClass::path().front();
                const auto radius = BaseClass::path().front().first;
                const auto rotation = BaseClass::path().front().second;
                const auto focus = m_focus_path.front();              // add
                this->process( data_front, radius, focus, rotation ); // mod
                BaseClass::dataQueue().pop();
                BaseClass::path().pop();
                this->focusPath().pop(); // add

                BaseClass::pushPathEntropies( BaseClass::maxEntropy() );
                BaseClass::pushPathPositions( BaseClass::maxPosition() );
                this->pushFocusPathPositions( this->maxFocusPoint() ); // add
            }
        }
    }
}

inline void EntropyBasedCameraFocusController::createPath(
    const float r2,
    const float r3,
    const kvs::Vec3& f2,
    const kvs::Vec3& f3,
    const kvs::Quat& q1,
    const kvs::Quat& q2,
    const kvs::Quat& q3,
    const kvs::Quat& q4,
    const size_t point_interval )
{
    std::queue<std::tuple<float, kvs::Quat>> empty;
    BaseClass::path().swap( empty );

    std::queue<kvs::Vec3> empty_focus;     // add
    this->focusPath().swap( empty_focus ); // add

    kvs::Timer timer( kvs::Timer::Start );
    for ( size_t i = 1; i < point_interval; i++ )
    {
        const auto t = static_cast<float>( i ) / static_cast<float>( point_interval );
        const auto r = BaseClass::radiusInterpolation( r2, r3, t );
        const auto q = BaseClass::pathInterpolation( q1, q2, q3, q4, t );
        BaseClass::path().push( { r, q } );

        const auto f = ( 1.0 - t ) * f2 + t * f3; // add
        this->focusPath().push( f );              // add
    }
    timer.stop();

    const auto path_calc_time = timer.sec();
    BaseClass::pathCalcTimes().push_back( path_calc_time );
}

} // end of namespace InSituVis
