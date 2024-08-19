#include <kvs/Math>
#include <kvs/Stat>
#include <kvs/Quaternion>
#include <kvs/LabColor>
#include <time.h>
#include <chrono>



namespace InSituVis
{

inline void EntropyBasedCameraFocusController::push( const Data& data )
{
   BaseClass::setSubTimeIndex(999999);

    if ( !( BaseClass::isFinalStep() ) )
    {
        if ( BaseClass::isInitialStep() )
        {
            // Initial step.
            this->process( data );
            BaseClass::pushMaxEntropies( BaseClass::maxEntropy() );
            BaseClass::pushMaxPositions( BaseClass::maxPosition() );
            BaseClass::pushMaxRotations( BaseClass::maxRotation() );
            if ( this->isInterpolationMethod() == SQUAD )
            {
                this->pushMaxRotations( this->maxRotation() );
            }
            BaseClass::pushNumImages( 1 );
            BaseClass::setIsInitialStep( false );
            this->pushMaxFocusPoints( this->maxFocusPoint() ); // add
        }
        else
        {
            if ( BaseClass::isCacheEnabled() )
            {
                if ( BaseClass::isEntStep() )
                {
                    this->process( data );
                    BaseClass::pushMaxEntropies( BaseClass::maxEntropy() );
                    BaseClass::pushMaxPositions( BaseClass::maxPosition() );
                    BaseClass::pushMaxRotations( BaseClass::maxRotation() );
                    this->pushMaxFocusPoints( this->maxFocusPoint() ); // add

                    if ( BaseClass::dataQueue().size() == BaseClass::cacheSize() )
                    {
                        BaseClass::setIsErpStep( true );
                        this->createPath();
                        Data data_front;
                        BaseClass::setSubTimeIndex( 0 );
                        size_t num_points = BaseClass::path().size();
                        size_t num_images = ( num_points + 1 ) / BaseClass::entropyInterval();
                    

                        for ( size_t i = 0; i < num_points; i++ )
                        {
                            if ( BaseClass::dataQueue().size() > 0 ) { data_front = BaseClass::dataQueue().front(); }
                            else { data_front = data; }
                            const std::pair<float, kvs::Quaternion> path_front = BaseClass::path().front();
                            this->process( data_front, path_front.first, path_front.second, this->focusPath().front() ); //mod
                            BaseClass::path().pop();
                            this->focusPath().pop(); // add
                            BaseClass::setSubTimeIndex(BaseClass::subTimeIndex() + 1);
                            
                            if ( BaseClass::subTimeIndex() == num_images )
                            {
                                BaseClass::dataQueue().pop();
                                BaseClass::pushNumImages( num_images );
                                BaseClass::setSubTimeIndex( 0 );
                            }
                        }

                        if ( BaseClass::dataQueue().size() > 0 ) BaseClass::dataQueue().pop();
                        BaseClass::setSubTimeIndex( 0 );
                        BaseClass::pushNumImages( num_images );
                        BaseClass::popMaxPositions();
                        BaseClass::popMaxRotations();
                        this->popMaxFocusPoints();
                        BaseClass::setIsErpStep( false );
                    }
                    BaseClass::dataQueue().push( data );
                }
                else { BaseClass::dataQueue().push( data ); }
            }
        }
    }
    else
    {
        const auto final_position = BaseClass::maxPositions().back();
        const auto final_rotation = BaseClass::maxRotations().back();
        const auto final_focus_point = this->maxFocusPoints().back();
        BaseClass::pushMaxPositions( final_position );
        BaseClass::pushMaxRotations( final_rotation );
        this->pushMaxFocusPoints( final_focus_point );

        BaseClass::setIsErpStep( true );
        this->createPath();
        Data data_front;
        BaseClass::setSubTimeIndex( 0 );
        size_t num_points = BaseClass::path().size();
        size_t num_images = ( num_points + 1 ) / BaseClass::entropyInterval();

        for ( size_t i = 0; i < num_points; i++ )
        {
            if ( BaseClass::dataQueue().size() > 0 ) { data_front = BaseClass::dataQueue().front(); }
            else { data_front = data; }
            const std::pair<float, kvs::Quaternion> path_front = BaseClass::path().front();
            this->process( data_front, path_front.first, path_front.second, this->focusPath().front() ); //mod
            BaseClass::path().pop();
            this->focusPath().pop(); // add
            BaseClass::setSubTimeIndex( BaseClass::subTimeIndex() + 1 );
            
            if( BaseClass::subTimeIndex() == num_images )
            {
                BaseClass::dataQueue().pop();
                BaseClass::pushNumImages( num_images );
                BaseClass::setSubTimeIndex( 0 );
            }
        }

        if ( BaseClass::dataQueue().size() > 0 ) BaseClass::dataQueue().pop();
        BaseClass::setSubTimeIndex( 0 );
        BaseClass::pushNumImages( num_images );
        BaseClass::popMaxPositions();
        BaseClass::popMaxRotations();
        this->popMaxFocusPoints();

        while ( BaseClass::dataQueue().size() > 0 )
        {
            const auto data_front = BaseClass::dataQueue().front();
            this->process( data_front, final_position.length(), final_rotation, final_focus_point );
            BaseClass::pushNumImages( 1 );
            BaseClass::dataQueue().pop();
        }
    }
}


inline void EntropyBasedCameraFocusController::createPath()
{
    std::queue<std::pair<float, kvs::Quaternion>> empty;
    BaseClass::path().swap( empty );
    
    std::queue<kvs::Vec3> empty_focus;     // add
    this->focusPath().swap( empty_focus ); // add
    kvs::Timer timer( kvs::Timer::Start );

    const auto positions = BaseClass::maxPositions();
    const auto rotations = BaseClass::maxRotations();
    const auto focuspoints = this->maxFocusPoints();
    const size_t n = 512;
    float l = 0.0f;

    for ( size_t i = 0; i < n; i++ )
    {
        const auto t0 = static_cast<float>( i ) / static_cast<float>( n );
        const auto rad0 = BaseClass::radiusInterpolation( positions[0].length(), positions[1].length(), t0 );
        const auto rot0 = BaseClass::pathInterpolation( rotations, t0 );
        const auto p0 = kvs::Quaternion::Rotate( kvs::Vec3( { 0.0f, rad0, 0.0f } ), rot0 );

        const auto t1 = static_cast<float>( i + 1 ) / static_cast<float>( n );
        const auto rad1 = BaseClass::radiusInterpolation( positions[0].length(), positions[1].length(), t1 );
        const auto rot1 = BaseClass::pathInterpolation( rotations, t1 );
        const auto p1 = kvs::Quaternion::Rotate( kvs::Vec3( { 0.0f, rad1, 0.0f } ), rot1 );
        const auto u = p1 - p0;
        l += u.length();
    }

    const size_t num_images = static_cast<size_t>( l / ( BaseClass::entropyInterval() * BaseClass::delta() ) ) + 1;
    const size_t num_points = num_images * BaseClass::entropyInterval() - 1;
    
    
    for ( size_t i = 0; i < num_points; i++ )
    {
        const auto t = static_cast<float>( i + 1 ) / static_cast<float>( num_points + 1 );
        const auto rad = BaseClass::radiusInterpolation( positions[0].length(), positions[1].length(), t );
        const auto rot = BaseClass::pathInterpolation( rotations, t );
        const std::pair<float, kvs::Quaternion> elem( rad, rot );
        BaseClass::path().push( elem );
        const auto f = ( 1.0f - t ) * focuspoints[0] + t * focuspoints[1]; // add
        this->focusPath().push( f );              // add
    }

    timer.stop();

    const auto path_calc_time = timer.sec();
    BaseClass::pushPathCalcTimes( path_calc_time );
}

} // end of namespace InSituVis
