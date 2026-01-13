// EntropyBasedCameraPathControllerMulti.hpp (debug-removed)

#pragma once
#include <kvs/Math>
#include <kvs/Stat>
#include <kvs/Quaternion>
#include <kvs/LabColor>
#include <time.h>
#include <chrono>
#include <queue>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>

namespace InSituVis
{

inline void EntropyBasedCameraPathControllerMulti::push( const Data& data )
{
    if ( !( BaseClass::isFinalStep() ) )
    {
        if ( BaseClass::isInitialStep() )
        {
            this->process( data );
            BaseClass::pushMaxEntropies( BaseClass::maxEntropy() );

            for( size_t i = 0; i < focusPointCandidateNum() * viewPointCandidateNum(); i++ )
            {
                BaseClass::pushMaxPositions( *candPositions().begin() );
                BaseClass::pushMaxRotations( *candRotations().begin() );
                this->pushMaxFocusPoints( *candFocusPoints().begin() );
                popCandPositions();
                popCandRotations();
                popCandFocusPoints();
                if ( isAutoZoomingEnabled() ) { popCandZoomLevels(); }
            }

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

                    for( size_t i = 0; i < focusPointCandidateNum() * viewPointCandidateNum(); i++ )
                    {
                        BaseClass::pushMaxPositions( *candPositions().begin() );
                        BaseClass::pushMaxRotations( *candRotations().begin() );
                        this->pushMaxFocusPoints( *candFocusPoints().begin() );
                        popCandPositions();
                        popCandRotations();
                        popCandFocusPoints();
                        if ( isAutoZoomingEnabled() ) { popCandZoomLevels(); }
                    }

                    if ( BaseClass::dataQueue().size() == BaseClass::cacheSize() )
                    {
                        BaseClass::setIsErpStep( true );
                        this->createPath();

                        const size_t C4 =
                            focusPointCandidateNum()*focusPointCandidateNum()*
                            viewPointCandidateNum()*viewPointCandidateNum();

                        const size_t num_points = BaseClass::path().size() / C4;
                        const size_t num_images = ( num_points + 1 ) / BaseClass::entropyInterval();

                        Data data_front;

                        for ( int j = 0; j < static_cast<int>(C4); j++ )
                        {
                            for ( size_t i = 0; i < num_points; i++ )
                            {
                                data_front = BaseClass::dataQueue().front();

                                process( data_front,
                                         BaseClass::path().front().first,
                                         BaseClass::path().front().second,
                                         this->focusPath().front(),
                                         j );

                                BaseClass::path().pop();
                                this->focusPath().pop();

                                if ( j < static_cast<int>(C4) - 1 ) { BaseClass::dataQueue().push( data_front ); }
                                BaseClass::dataQueue().pop();

                                BaseClass::pushNumImages( num_images );
                            }
                        }

                        if ( BaseClass::dataQueue().size() > 0 ) { BaseClass::dataQueue().pop(); }
                        BaseClass::pushNumImages( num_images );

                        for ( int i = 0; i < static_cast<int>(focusPointCandidateNum() * viewPointCandidateNum()); i++ )
                        {
                            BaseClass::popMaxPositions();
                            BaseClass::popMaxRotations();
                            popMaxFocusPoints();
                        }

                        BaseClass::setIsErpStep( false );
                    }

                    BaseClass::dataQueue().push( data );
                    if ( isInterpolationMethod() == SLERP ) { BaseClass::dataQueue().pop(); }
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
        // FINAL step

        auto final_positions = BaseClass::maxPositions();
        auto final_rotations = BaseClass::maxRotations();
        auto final_focus_points = maxFocusPoints();

        const size_t C2 = focusPointCandidateNum() * viewPointCandidateNum();

        for ( int i = static_cast<int>(C2); i > 0; i-- )
        {
            BaseClass::pushMaxPositions( final_positions[ final_positions.size() - static_cast<size_t>(i) ] );
            BaseClass::pushMaxRotations( final_rotations[ final_rotations.size() - static_cast<size_t>(i) ] );
            pushMaxFocusPoints( final_focus_points[ final_focus_points.size() - static_cast<size_t>(i) ] );
        }

        BaseClass::setIsErpStep( true );
        this->createPath();

        const size_t C4 =
            focusPointCandidateNum()*focusPointCandidateNum()*
            viewPointCandidateNum()*viewPointCandidateNum();

        const size_t num_points = BaseClass::path().size() / C4;
        const size_t num_images = ( num_points + 1 ) / BaseClass::entropyInterval();

        Data data_front;

        for ( int j = 0; j < static_cast<int>(C4); j++ )
        {
            for ( size_t i = 0; i < num_points; i++ )
            {
                data_front = BaseClass::dataQueue().front();

                process( data_front,
                         BaseClass::path().front().first,
                         BaseClass::path().front().second,
                         this->focusPath().front(),
                         j );

                BaseClass::path().pop();
                this->focusPath().pop();

                BaseClass::dataQueue().push( data_front );
                BaseClass::dataQueue().pop();

                BaseClass::pushNumImages( num_images );
            }
        }

        // dataQueue を全部pop（現状の挙動維持）
        for ( size_t i = 0; i < BaseClass::dataQueue().size(); i++ ) BaseClass::dataQueue().pop();

        if ( BaseClass::dataQueue().size() > 0 ) { BaseClass::dataQueue().pop(); }
        BaseClass::pushNumImages( num_images );

        for ( int i = 0; i < static_cast<int>(C2); i++ )
        {
            BaseClass::popMaxPositions();
            BaseClass::popMaxRotations();
            popMaxFocusPoints();
        }

        for ( size_t i = static_cast<size_t>(C2); i > 0; i-- )
        {
            while ( BaseClass::dataQueue().size() > 0 )
            {
                const auto df = BaseClass::dataQueue().front();

                process( df,
                         final_positions[ final_positions.size() - i ].length(),
                         final_rotations[ final_rotations.size() - i ],
                         final_focus_points[ final_focus_points.size() - i ],
                         static_cast<int>(i) );

                BaseClass::pushNumImages( 1 );
                BaseClass::dataQueue().pop();
            }
        }

        BaseClass::setIsErpStep(false);
    }
}

inline void EntropyBasedCameraPathControllerMulti::createPath() // fin
{
    std::queue<std::pair<float, kvs::Quaternion>> empty;
    BaseClass::path().swap( empty );

    std::queue<kvs::Vec3> empty_focus;
    this->focusPath().swap( empty_focus );

    kvs::Timer timer( kvs::Timer::Start );

    const auto positions   = BaseClass::maxPositions();
    const auto rotations   = BaseClass::maxRotations();
    const auto focuspoints = this->maxFocusPoints();

    const size_t n = 512;

    const int C2 = static_cast<int>(focusPointCandidateNum() * viewPointCandidateNum());
    if ( C2 <= 0 )
    {
        timer.stop();
        BaseClass::pushPathCalcTimes( timer.sec() );
        return;
    }

    for ( int j = 0; j < C2; j++ )
    {
        for ( int k = C2; k < 2 * C2; k++ )
        {
            float l = 0.0f;
            std::vector<kvs::Quat> R{ rotations[j], rotations[k] };

            for ( int i = 0; i < static_cast<int>(n); i++ )
            {
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
            const size_t num_images = 1;
            const size_t num_points = num_images * BaseClass::entropyInterval() - 1;

            for ( int i = 0; i < static_cast<int>(num_points); i++ )
            {
                const auto t = static_cast<float>( i + 1 ) / static_cast<float>( num_points + 1 );
                const auto rad = BaseClass::radiusInterpolation( positions[j].length(), positions[k].length(), t );
                const auto rot = BaseClass::pathInterpolation( R, t );
                const std::pair<float, kvs::Quaternion> elem( rad, rot );
                BaseClass::path().push( elem );

                const auto f = ( 1.0f - t ) * focuspoints[j] + t * focuspoints[k];
                this->focusPath().push( f );
            }

            pushFocusPathLength( (focuspoints[k] - focuspoints[j]).length() );
        }
    }

    timer.stop();

    const auto path_calc_time = timer.sec();
    BaseClass::pushPathCalcTimes( path_calc_time );
}

inline void EntropyBasedCameraPathControllerMulti::outputVideoParams(
    const std::string& filename1,
    const std::vector<std::string>& filename2,
    const std::vector<float>& focus_entropies,
    const std::vector<float>& focus_path_length,
    const std::vector<float>& camera_path_length )
{
    std::ofstream file( filename1 );
    {
        std::cout << "video params create start " << std::endl;

        const size_t Candidate_num = focusPointCandidateNum() * viewPointCandidateNum();
        const size_t CN2 = focusPointCandidateNum()*focusPointCandidateNum()*
                           viewPointCandidateNum()*viewPointCandidateNum();

        file << "Filename,Entropy";
        for ( size_t i = 0; i < Candidate_num; i++ )
        {
            file << ",preFocusPath[" << i << "]";
        }
        for ( size_t i = 0; i < Candidate_num; i++ )
        {
            file << ",PreCameraPath[" << i << "]";
        }
        file << std::endl;

        for ( size_t i = 0; i < filename2.size(); i++ )
        {
            if ( i < Candidate_num )
            {
                file << filename2[i] << "," << focus_entropies[i] << std::endl;
            }
            else
            {
                file << filename2[i] << "," << focus_entropies[i] << ",";

                for ( size_t j = 0; j < Candidate_num; j++ )
                {
                    file << focus_path_length[( i % Candidate_num ) + ( i / Candidate_num - 1 ) * CN2 + j * Candidate_num] << ",";
                }
                for ( size_t j = 0; j < Candidate_num; j++ )
                {
                    file << camera_path_length[( i % Candidate_num ) + ( i / Candidate_num - 1 ) * CN2 + j * Candidate_num] << ",";
                }
                file << std::endl;
            }
        }
    }
    file.close();
}

} // end of namespace InSituVis
