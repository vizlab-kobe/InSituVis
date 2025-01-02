#include <kvs/Math>
#include <kvs/Stat>
#include <kvs/Quaternion>
#include <kvs/LabColor>
#include <time.h>
#include <chrono>

namespace InSituVis
{

inline void EntropyBasedCameraFocusControllerMulti::push( const Data& data )
{
//    BaseClass::setSubTimeIndex(999999);

    if ( !( BaseClass::isFinalStep() ) )
    {
        if ( BaseClass::isInitialStep() )
        {
            // Initial step.
            this->process( data );
            BaseClass::pushMaxEntropies( BaseClass::maxEntropy() );
            for( size_t i = 0;i<candidateNum();i++ ){
                BaseClass::pushMaxPositions( *candPositions().begin() );
                BaseClass::pushMaxRotations( *candRotations().begin() );
                this->pushMaxFocusPoints( *candFocusPoints().begin() );
                popCandPositions();
                popCandRotations();
                popCandFocusPoints();
                if(isAutoZoomingEnabled())popCandZoomLevels();
            }
                        // if ( this->isInterpolationMethod() == SQUAD )
            // {
            //     this->pushMaxRotations( this->maxRotation() );
            // }
            BaseClass::pushNumImages( 1 );
            BaseClass::setIsInitialStep( false );
        }
        else
        {
            if ( BaseClass::isCacheEnabled() )
            {
                if ( BaseClass::isEntStep() )
                {
                    process( data );
                    for(size_t i = 0;i<candidateNum();i++ ){
                        BaseClass::pushMaxPositions( *candPositions().begin() );
                        BaseClass::pushMaxRotations( *candRotations().begin() );
                        this->pushMaxFocusPoints( *candFocusPoints().begin() );
                        popCandPositions();
                        popCandRotations();
                        popCandFocusPoints();
                        if(isAutoZoomingEnabled())popCandZoomLevels();
                    }
                    if ( BaseClass::dataQueue().size() == BaseClass::cacheSize() )
                    {
                        BaseClass::setIsErpStep( true );
                        this->createPath();
                        Data data_front;
                        // BaseClass::setSubTimeIndex( 0 );
                        size_t num_points = (BaseClass::path().size())/(candidateNum()*candidateNum());
                        size_t num_images = ( num_points + 1 ) / BaseClass::entropyInterval();
                
                        for(int j = 0; j<static_cast<int>(candidateNum()*candidateNum()); j++){
                            for (size_t i = 0; i < num_points; i++ )
                            {
                                if ( BaseClass::dataQueue().size() > 0 ) { data_front = BaseClass::dataQueue().front(); }
                                else { data_front = data; }
                                const std::pair<float, kvs::Quaternion> path_front = BaseClass::path().front();
                                process( data_front, path_front.first, path_front.second, this->focusPath().front(), j  ); 
                                BaseClass::path().pop();
                                this->focusPath().pop();
                                // BaseClass::setSubTimeIndex(BaseClass::subTimeIndex() + 1);
                                if ( j < static_cast<int>(candidateNum()*candidateNum() ) - 1 )BaseClass::dataQueue().push( data_front );
                                BaseClass::dataQueue().pop();
                                BaseClass::pushNumImages( num_images );
                                // if ( BaseClass::subTimeIndex() == num_images )//毎回くる？
                                // {   
                                //     if ( j != static_cast<int>(candidateNum()*candidateNum() ) - 1 )BaseClass::dataQueue().push(BaseClass::dataQueue().front() );
                                //     BaseClass::dataQueue().pop();
                                //     BaseClass::pushNumImages( num_images );
                                //     BaseClass::setSubTimeIndex( 0 );
                                // }
                            }
                        }

                        if ( BaseClass::dataQueue().size() > 0 ) BaseClass::dataQueue().pop();
                        // BaseClass::setSubTimeIndex( 0 );
                        BaseClass::pushNumImages( num_images );
                            for(int i=0;i<static_cast<int>(candidateNum());i++){
                            BaseClass::popMaxPositions();
                            BaseClass::popMaxRotations();
                            popMaxFocusPoints();
                            }
                        BaseClass::setIsErpStep( false );
                    }
                    BaseClass::dataQueue().push( data );
                    if( isInterpolationMethod() == SLERP  ) BaseClass::dataQueue().pop();
                }
                else { BaseClass::dataQueue().push( data ); }
            }
        }
    }
    else{       
        auto final_positions = BaseClass::maxPositions();
        auto final_rotations = BaseClass::maxRotations();
        auto final_focus_points = maxFocusPoints();

        for(int i = static_cast<int>(candidateNum());i>0;i-- ){
            BaseClass::pushMaxPositions( final_positions[final_positions.size() - i] );
            BaseClass::pushMaxRotations( final_rotations[final_rotations.size() - i] );
            pushMaxFocusPoints( final_focus_points[final_focus_points.size() - i] );
        }
        BaseClass::setIsErpStep( true );
        this->createPath();
        Data data_front;
        // BaseClass::setSubTimeIndex( 0 );
        size_t num_points = (BaseClass::path().size())/(candidateNum()*candidateNum());
        size_t num_images = ( num_points + 1 ) / BaseClass::entropyInterval();

        for(int j = 0;j<static_cast<int>(candidateNum()*candidateNum()); j++){
            for ( size_t i = 0; i < num_points; i++ )
            {
                if ( BaseClass::dataQueue().size() > 0 ) { data_front = BaseClass::dataQueue().front(); }
                else { data_front = data; }
                const std::pair<float, kvs::Quaternion> path_front = BaseClass::path().front();
                process( data_front, path_front.first, path_front.second, focusPath().front(), j ); 
                BaseClass::path().pop();
                this->focusPath().pop();
                // BaseClass::setSubTimeIndex(BaseClass::subTimeIndex() + 1);
                
                // if ( BaseClass::subTimeIndex() == num_images )
                // {
                    BaseClass::pushNumImages( num_images );
                    // BaseClass::setSubTimeIndex( 0 );
                // }
            }
        }
        if ( BaseClass::dataQueue().size() > 0 ) BaseClass::dataQueue().pop();
        // BaseClass::setSubTimeIndex( 0 );
        BaseClass::pushNumImages( num_images );
        for(int i=0;i<static_cast<int>(candidateNum());i++){
        BaseClass::popMaxPositions();
        BaseClass::popMaxRotations();
        popCandFocusPoints();
        }
        for(size_t i = static_cast<int>(candidateNum());i>0;i-- ){
                while ( BaseClass::dataQueue().size() > 0 )
                {
                    const auto data_front = BaseClass::dataQueue().front();
                    process( data_front, final_positions[final_positions.size() - i].length(), final_rotations[final_positions.size() - i], final_focus_points[final_positions.size() - i], i );
                    BaseClass::pushNumImages( 1 );
                    BaseClass::dataQueue().pop();
                }
            }
        }
}


inline void EntropyBasedCameraFocusControllerMulti::createPath() //fin
{
    std::queue<std::pair<float, kvs::Quaternion>> empty;
    BaseClass::path().swap( empty );
    
    std::queue<kvs::Vec3> empty_focus;     // add
    this->focusPath().swap( empty_focus ); // add
    kvs::Timer timer( kvs::Timer::Start );

    const auto positions = BaseClass::maxPositions(); //2queued if SLERP
    const auto rotations = BaseClass::maxRotations(); //
    const auto focuspoints = this->maxFocusPoints();
    const size_t n = 512;

    for( int j=0; j<static_cast<int>(candidateNum()); j++){
        for( int k=static_cast<int>(candidateNum()); k<2*(static_cast<int>(candidateNum())); k++ ){
            float l = 0.0f;
            std::vector<kvs::Quat> R{rotations[j],rotations[k]};
            for ( int i = 0; i < n; i++ ) {
                
                const auto t0 = static_cast<float>( i ) / static_cast<float>( n );
                const auto rad0 = BaseClass::radiusInterpolation( positions[j].length(), positions[k].length(), t0 );
                const auto rot0 = BaseClass::pathInterpolation( R, t0 );
                const auto p0 = kvs::Quaternion::Rotate( kvs::Vec3( { 0.0f, rad0, 0.0f } ), rot0 );

                const auto t1 = static_cast<float>( i + 1 ) / static_cast<float>( n );
                const auto rad1 = BaseClass::radiusInterpolation( positions[j].length(), positions[k].length(), t1 );
                const auto rot1 = BaseClass::pathInterpolation( R, t1 );
                const auto p1 = kvs::Quaternion::Rotate( kvs::Vec3( { 0.0f, rad1, 0.0f } ), rot1 );
                const auto u = p1 - p0;
                l += u.length();
            }
            pushCameraPathLength( l );
            // const size_t num_images = static_cast<size_t>( l / ( BaseClass::entropyInterval() * BaseClass::delta() ) ) + 1;
            const size_t num_images = 1;
            const size_t num_points = num_images * BaseClass::entropyInterval() - 1;
            for ( int i = 0; i < num_points; i++ )
            {
                const auto t = static_cast<float>( i + 1 ) / static_cast<float>( num_points + 1 );
                const auto rad = BaseClass::radiusInterpolation( positions[j].length(), positions[k].length(), t );
                const auto rot = BaseClass::pathInterpolation( R, t );
                const std::pair<float, kvs::Quaternion> elem( rad, rot );
                BaseClass::path().push( elem );
                const auto f = ( 1.0f - t ) * focuspoints[j] + t * focuspoints[k]; // add
                this->focusPath().push( f ); 
            }        // add
            pushFocusPathLength( (focuspoints[k]-focuspoints[j]).length() );
        }
    }
    timer.stop();

    const auto path_calc_time = timer.sec();
    BaseClass::pushPathCalcTimes( path_calc_time );
}

inline void EntropyBasedCameraFocusControllerMulti::outputVideoParams(
    const std::string& filename1,
    const std::vector<std::string>& filename2,
    const std::vector<float>& focus_entropies,
    const std::vector<float>& focus_path_length,
    const std::vector<float>& camera_path_length
    )  
{
    std::ofstream file( filename1 );
    {
        size_t CN2 = candidateNum()*candidateNum();
        size_t t=-1;
        file << "Filename,Entropy";
        for(size_t i =0; i <  candidateNum(); i++ ){
            file << ",preFocusPath[" << i << "]";
        }
        for(size_t i =0; i <  candidateNum(); i++ ){
            file << ",PreCameraPath[" << i << "]";
        }
        file<<std::endl;
        for ( size_t i = 0; i < filename2.size(); i++ )
        {
            if( i < candidateNum() ){
                file << filename2[i] << "," << focus_entropies[i] <<std::endl;
            }
            else{
                file << filename2[i] << "," << focus_entropies[i] << ",";
                for(size_t j = 0; j<candidateNum(); j++ ){
                    file << focus_path_length[( i%candidateNum() ) + (i/candidateNum() -1 )*CN2 + j*candidateNum()] << ","; 
                } 
                for(size_t j = 0; j<candidateNum(); j++ ){
                    file << camera_path_length[( i%candidateNum() ) + (i/candidateNum() -1 )*CN2 + j*candidateNum()] << ","; 
                } 
                file<<std::endl;
            }
        }
    }
    file.close();
}


} // end of namespace InSituVis
