#include <kvs/Math>
#include <kvs/Stat>
#include <kvs/Quaternion>
#include <kvs/LabColor>
#include <time.h>
#include <chrono>


namespace InSituVis
{

inline EntropyBasedCameraPathController::EntropyFunction
EntropyBasedCameraPathController::LightnessEntropy()
{
    return [] ( const FrameBuffer& frame_buffer )
    {
        kvs::ValueArray<size_t> histogram( 256 );
        histogram.fill( 0 );

        size_t n = 0;
        const auto& depth_buffer = frame_buffer.depth_buffer;
        const auto& color_buffer = frame_buffer.color_buffer;
        const auto length = depth_buffer.size();
        const float Yn = 1.0f;

        auto f = [&] ( const kvs::Real32 t ) -> kvs::Real32 {
            if ( t > 0.008856f ) { return std::pow( t, 1.0f / 3.0f ); }
            else { return 7.787037f * t + 16.0f / 116.0f; }
        };

        auto toLinear = [&] ( const kvs::Real32 C ) -> kvs::Real32 {
            kvs::Real32 Cl = 0;
            if ( C <= 0.04045f ) { Cl = C / 12.92f; }
            else { Cl = std::pow( ( C + 0.055f ) / 1.055f, 2.4f ); }
            return kvs::Math::Clamp( Cl, 0.0f, 1.0f );
        };

        for ( size_t i = 0; i < length; i++ )
        {
            const auto depth = depth_buffer[i];
            //const auto r = color_buffer[ 4 * i ];
            //const auto g = color_buffer[ 4 * i + 1 ];
            //const auto b = color_buffer[ 4 * i + 2 ];

            if ( depth < 1.0f )
            {
                const kvs::Real32 r = static_cast<kvs::Real32>( color_buffer[ 4 * i ] ) / 255.0f;
                const kvs::Real32 g = static_cast<kvs::Real32>( color_buffer[ 4 * i + 1 ] ) / 255.0f;
                const kvs::Real32 b = static_cast<kvs::Real32>( color_buffer[ 4 * i + 2 ] ) / 255.0f;
                const float Y = 0.212639f * toLinear( r ) + 0.715169f * toLinear( g ) + 0.072192f * toLinear( b );
                const kvs::Real32 l = 116.0f * ( f( Y / Yn ) - 16.0f / 116.0f );
                int j = static_cast<int>( l / 100 * 256 );
                if( j > 255 ) j = 255;
                histogram[j] += 1;
                n += 1;
            }
        }

        if ( n == 0 ) { return 0.0f; }

        float entropy = 0.0f;
        const auto log2 = std::log( 2.0f );
        for ( const auto h : histogram )
        {
            const auto p = static_cast<float>( h ) / n;
            if ( p > 0.0f ) { entropy -= p * std::log( p ) / log2; }
        }

        return entropy;
    };
}

inline EntropyBasedCameraPathController::EntropyFunction
EntropyBasedCameraPathController::ColorEntropy()
{
    return [] ( const FrameBuffer& frame_buffer )
    {
        kvs::ValueArray<size_t> histogram_R( 256 );
        kvs::ValueArray<size_t> histogram_G( 256 );
        kvs::ValueArray<size_t> histogram_B( 256 );
        histogram_R.fill( 0 );
        histogram_G.fill( 0 );
        histogram_B.fill( 0 );

        size_t n = 0;
        const auto& depth_buffer = frame_buffer.depth_buffer;
        const auto& color_buffer = frame_buffer.color_buffer;
        const auto length = depth_buffer.size();
        for ( size_t i = 0; i < length; i++ )
        {
            const auto depth = depth_buffer[i];
            const auto color_R = color_buffer[ 4 * i ];
            const auto color_G = color_buffer[ 4 * i + 1 ];
            const auto color_B = color_buffer[ 4 * i + 2 ];

            if ( depth < 1.0f )
            {
                histogram_R[ color_R ] += 1;
                histogram_G[ color_G ] += 1;
                histogram_B[ color_B ] += 1;
                n += 1;
            }
        }

        if ( n == 0 ) { return 0.0f; }

        float entropy_R = 0.0f;
        float entropy_G = 0.0f;
        float entropy_B = 0.0f;
        const auto log2 = std::log( 2.0f );
        for ( const auto h : histogram_R )
        {
            const auto p = static_cast<float>( h ) / n;
            if ( p > 0.0f ) { entropy_R -= p * std::log( p ) / log2; }
        }
        for ( const auto h : histogram_G )
        {
            const auto p = static_cast<float>( h ) / n;
            if ( p > 0.0f ) { entropy_G -= p * std::log( p ) / log2; }
        }
        for ( const auto h : histogram_B )
        {
            const auto p = static_cast<float>( h ) / n;
            if ( p > 0.0f ) { entropy_B -= p * std::log( p ) / log2; }
        }

        const auto entropy = ( entropy_R + entropy_G + entropy_B ) / 3.0f;

        return entropy;
    };
}

inline EntropyBasedCameraPathController::EntropyFunction
EntropyBasedCameraPathController::DepthEntropy()
{
    return [] ( const FrameBuffer& frame_buffer )
    {
        kvs::ValueArray<size_t> histogram( 256 );
        histogram.fill( 0 );

        size_t n = 0;
        const auto& depth_buffer = frame_buffer.depth_buffer;
        for ( const auto depth : depth_buffer )
        {
            if ( depth < 1.0f )
            {
                const auto i = static_cast<int>( depth * 256 );
                histogram[i] += 1;
                n += 1;
            }
        }

        if ( n == 0 ) { return 0.0f; }

        float entropy = 0.0f;
        const auto log2 = std::log( 2.0f );
        for ( const auto h : histogram )
        {
            const auto p = static_cast<float>( h ) / n;
            if ( p > 0.0f ) { entropy -= p * std::log( p ) / log2; }
        }

        return entropy;
    };
}

inline EntropyBasedCameraPathController::EntropyFunction
EntropyBasedCameraPathController::MixedEntropy(
    EntropyFunction e1,
    EntropyFunction e2,
    float p )
{
    return [e1,e2,p] ( const FrameBuffer& frame_buffer ) -> float
    {
        return p * e1( frame_buffer ) + ( 1 - p ) * e2( frame_buffer );
    };
}

inline EntropyBasedCameraPathController::Interpolator
EntropyBasedCameraPathController::Slerp()
{
    return [] (
        const kvs::Quat& q1,
        const kvs::Quat& q2,
        const kvs::Quat& q3,
        const kvs::Quat& q4,
        float t ) -> kvs::Quat
    {
        return kvs::Quat::SphericalLinearInterpolation( q2, q3, t, true, true );
    };
}

inline EntropyBasedCameraPathController::Interpolator
EntropyBasedCameraPathController::Squad()
{
    return [] (
        const kvs::Quat& q1,
        const kvs::Quat& q2,
        const kvs::Quat& q3,
        const kvs::Quat& q4,
        float t ) -> kvs::Quat
    {
        kvs::Quat qq1 = q1;
        kvs::Quat qq2 = q2;
        kvs::Quat qq3 = q3;
        kvs::Quat qq4 = q4;
        if( qq1.dot( qq2 ) < 0 ) qq2 = -qq2;
        if( qq2.dot( qq3 ) < 0 ) qq3 = -qq3;
        if( qq3.dot( qq4 ) < 0 ) qq4 = -qq4;
        return kvs::Quat::SphericalQuadrangleInterpolation( qq1, qq2, qq3, qq4, t, true );
    };
}

inline void EntropyBasedCameraPathController::setOutputEvaluationImageEnabled(
    const bool enable,
    const bool enable_depth )
{
    m_enable_output_evaluation_image = enable;
    m_enable_output_evaluation_image_depth = enable_depth;
}

inline void EntropyBasedCameraPathController::push( const Data& data )
{
    const auto interval = this->entropyInterval();

    if ( !( this->isFinalStep() ) )
    {
        if ( this->previousData().empty() )
        {
            // Initial step.
            this->process( data );
            this->setPreviousData( data );
            this->pushMaxEntropies( this->maxEntropy() );
            this->pushMaxPositions( this->maxPosition() );
            this->pushMaxRotations( this->maxRotation() );
            this->pushMaxRotations( this->maxRotation() );
            this->dataQueue().push( data );
        }
        else
        {
            if ( this->isCacheEnabled() )
            {
                if ( this->dataQueue().size() % interval == 0 )
                {
                    m_number_of_image = 0;
                    this->process( data );
                    this->pushMaxEntropies( this->maxEntropy() );
                    this->pushMaxPositions( this->maxPosition() );
                    this->pushMaxRotations( this->maxRotation() );

                    if ( this->maxRotations().size() == 4 )
                    {
                        const auto q1 = this->maxRotations().front(); this->maxRotations().pop();
                        const auto q2 = this->maxRotations().front(); this->maxRotations().pop();
                        const auto q3 = this->maxRotations().front(); this->maxRotations().pop();
                        const auto q4 = this->maxRotations().front(); this->maxRotations().pop();

                        const auto p2 = this->maxPositions().front(); this->maxPositions().pop();
                        const auto p3 = this->maxPositions().front();

                        const auto r2 = p2.length();
                        const auto r3 = p3.length();

                        this->createPath( r2, r3, q1, q2, q3, q4, interval );

                        if( m_slomo_enabled )
                        {
                            m_is_erp_step = true;
                            const auto data_front = this->dataQueue().front();
                            this->pushPathPositions( p2 );

                            while( this->path().size() > 0 )
                            {
                                m_number_of_image++;
                                const std::pair<float, kvs::Quat> path_front = this->path().front();
                                this->process( data_front, path_front.first, path_front.second );
                                this->path().pop();
                                this->pushPathPositions( this->maxPosition() );
                            }

                            this->dataQueue().pop();
                            m_number_of_images.push_back( m_number_of_image + 1 );
                            m_number_of_image = 0;
                            m_is_erp_step = false;
                        }
                        else
                        {
                            this->dataQueue().pop();
                            this->pushPathEntropies( this->maxEntropies().front() );
                            this->maxEntropies().pop();
                            this->pushPathPositions( p2 );

                            for ( size_t i = 0; i < interval - 1; i++ )
                            {
                                const auto data_front = this->dataQueue().front();
                                const std::pair<float, kvs::Quat> path_front = this->path().front();
                                this->process( data_front, path_front.first, path_front.second );
                                this->dataQueue().pop();
                                this->path().pop();

                                this->pushPathEntropies( this->maxEntropy() );
                                this->pushPathPositions( this->maxPosition() );
                            }
                        }

                        this->pushMaxRotations( q2 );
                        this->pushMaxRotations( q3 );
                        this->pushMaxRotations( q4 );
                    }
                    this->dataQueue().push( data );
                }
                else
                {
                    this->dataQueue().push( data );
                }
            }
        }
    }
    else
    {
        const auto q1 = this->maxRotations().front(); this->maxRotations().pop();
        const auto q2 = this->maxRotations().front(); this->maxRotations().pop();
        const auto q3 = this->maxRotations().front(); this->maxRotations().pop();
        const auto q4 = q3;

        const auto p2 = this->maxPositions().front(); this->maxPositions().pop();
        const auto p3 = this->maxPositions().front();

        const auto r2 = p2.length();
        const auto r3 = p3.length();

        this->createPath( r2, r3, q1, q2, q3, q4, interval );

        if( m_slomo_enabled )
        {
            m_is_erp_step = true;
            const auto data_front = this->dataQueue().front();
            this->pushPathPositions( p2 );

            while( this->path().size() > 0 )
            {
                m_number_of_image++;
                const std::pair<float, kvs::Quat> path_front = this->path().front();
                this->process( data_front, path_front.first, path_front.second );
                this->path().pop();
                this->pushPathPositions( this->maxPosition() );
            }

            this->dataQueue().pop();
            m_number_of_images.push_back( m_number_of_image + 1 );
            m_number_of_image = 0;
            m_is_erp_step = false;
        }
        else
        {
            this->dataQueue().pop();
            this->pushPathEntropies( this->maxEntropies().front() );
            this->maxEntropies().pop();
            this->pushPathPositions( p2 );

            for ( size_t i = 0; i < interval - 1; i++ )
            {
                const auto data_front = this->dataQueue().front();
                const std::pair<float, kvs::Quat> path_front = this->path().front();
                this->process( data_front, path_front.first, path_front.second );
                this->dataQueue().pop();
                this->path().pop();

                this->pushPathEntropies( this->maxEntropy() );
                this->pushPathPositions( this->maxPosition() );
            }
        }

        while ( this->dataQueue().size() > 0 )
        {
            std::queue<std::pair<float, kvs::Quat>> empty;
            this->path().swap( empty );

            for ( size_t i = 0; i < interval - 1; i++ )
            {
                std::pair<float, kvs::Quat> elem( r3, q3 );
                this->path().push( elem );
            }

            this->dataQueue().pop();
            this->pushPathEntropies( this->maxEntropies().front() );
            this->maxEntropies().pop();
            this->pushPathPositions( p3 );

            for ( size_t i = 0; i < m_entropy_interval - 1; i++ )
            {
                const auto data_front = this->dataQueue().front();
                const std::pair<float, kvs::Quat> path_front = this->path().front();
                this->process( data_front, path_front.first, path_front.second );
                this->dataQueue().pop();
                this->path().pop();

                this->pushPathEntropies( this->maxEntropy() );
                this->pushPathPositions( this->maxPosition() );
            }
        }

        m_number_of_images.push_back( m_number_of_image + 1 );
    }
}

inline float EntropyBasedCameraPathController::entropy( const FrameBuffer& frame_buffer )
{
    return m_entropy_function( frame_buffer );
}

inline float EntropyBasedCameraPathController::radiusInterpolation( const float r1, const float r2, const float t )
{
    return ( r2 -r1 ) * t * t * ( 3.0f - 2.0f * t ) + r1;
}

inline kvs::Quat EntropyBasedCameraPathController::pathInterpolation(
    const kvs::Quat& q1,
    const kvs::Quat& q2,
    const kvs::Quat& q3,
    const kvs::Quat& q4,
    const float t )
{
    return m_interpolator( q1, q2, q3, q4, t );
}

inline void EntropyBasedCameraPathController::createPath(
    const float r2,
    const float r3,
    const kvs::Quat& q1,
    const kvs::Quat& q2,
    const kvs::Quat& q3,
    const kvs::Quat& q4,
    const size_t point_interval )
{
    std::queue<std::pair<float, kvs::Quat>> empty;
    this->path().swap( empty );
    
    kvs::Timer timer( kvs::Timer::Start );

    if( m_slomo_enabled )
    {
        const size_t n = 256;
        float length = 0.0f;

        for( size_t i = 0; i < n; i++ )
        {
            const auto t0 = static_cast<float>( i ) / static_cast<float>( n );
            const auto rad0 = this->radiusInterpolation( r2, r3, t0 );
            const auto rot0 = this->pathInterpolation( q1, q2, q3, q4, t0 );
            const auto p0 = kvs::Quat::Rotate( kvs::Vec3( { 0.0f, rad0, 0.0f } ), rot0 );

            const auto t1 = static_cast<float>( i + 1 ) / static_cast<float>( n );
            const auto rad1 = this->radiusInterpolation( r2, r3, t1 );
            const auto rot1 = this->pathInterpolation( q1, q2, q3, q4, t1 );
            const auto p1 = kvs::Quat::Rotate( kvs::Vec3( { 0.0f, rad1, 0.0f } ), rot1 );

            const auto u = p1 - p0;
            length += sqrt( u.dot( u ) );
        }

        const size_t interval = static_cast<size_t>( length / m_viewpoint_interval );

        for ( size_t i = 1; i < interval; i++ )
        {
            const auto t = static_cast<float>( i ) / static_cast<float>( interval );
            const auto rad = this->radiusInterpolation( r2, r3, t );
            const auto rot = this->pathInterpolation( q1, q2, q3, q4, t );
            const std::pair<float, kvs::Quat> elem( rad, rot );
            this->path().push( elem );
        }
    }
    else
    {
        for ( size_t i = 1; i < point_interval; i++ )
        {
            const auto t = static_cast<float>( i ) / static_cast<float>( point_interval );
            const auto rad = this->radiusInterpolation( r2, r3, t );
            const auto rot = this->pathInterpolation( q1, q2, q3, q4, t );
            const std::pair<float, kvs::Quat> elem( rad, rot );
            this->path().push( elem );
        }
    }
    
    timer.stop();

    const auto path_calc_time = timer.sec();
    this->pathCalcTimes().push_back( path_calc_time );
}

inline std::string EntropyBasedCameraPathController::logDataFilename(
    const std::string& basename,
    const InSituVis::OutputDirectory& directory )
{
    return directory.baseDirectoryName() + "/" + basename + ".csv";
}

inline std::string EntropyBasedCameraPathController::logDataFilename(
    const std::string& basename,
    const kvs::UInt32 timestep,
    const InSituVis::OutputDirectory& directory )
{
    const auto output_timestep = kvs::String::From( timestep, 6, '0' );
    const auto output_basename = basename + output_timestep;
    return directory.baseDirectoryName() + "/" + output_basename + ".csv";
}

inline void EntropyBasedCameraPathController::outputEntropies(
    const std::string& filename,
    const std::vector<float>& entropies )
{
    std::ofstream file( filename );
    {
        file << "Index,Entropy" << std::endl;
        for ( size_t i = 0; i < entropies.size(); i++ )
        {
            file << i << "," << entropies[i] << std::endl;
        }
    }
    file.close();
}

inline void EntropyBasedCameraPathController::outputPathEntropies(
    const std::string& filename,
    const size_t analysis_interval )
{
    std::ofstream file( filename );
    {
        file << "Time,Entropy" << std::endl;
        const auto interval = analysis_interval;
        for ( size_t i = 0; i < m_path_entropies.size(); i++ )
        {
            file << interval * i << "," << m_path_entropies[i] << std::endl;
        }
    }
    file.close();
}

inline void EntropyBasedCameraPathController::outputPathPositions(
    const std::string& filename,
    const size_t analysis_interval )
{
    std::ofstream file( filename );
    {
        file << "Time,X,Y,Z" << std::endl;
        const auto interval = analysis_interval;
        for ( size_t i = 0; i < m_path_positions.size() / 3; i++ )
        {
            const auto x = m_path_positions[ 3 * i ];
            const auto y = m_path_positions[ 3 * i + 1 ];
            const auto z = m_path_positions[ 3 * i + 2 ];
            file << interval * i << "," << x << "," << y << "," << z << std::endl;
        }
    }
    file.close();
}

inline void EntropyBasedCameraPathController::outputPathCalcTimes(
    const std::string& filename )
{
    std::ofstream file( filename );
    {
        file << "Calculation time" << std::endl;
        for ( size_t i = 0; i < m_path_calc_times.size(); i++ )
        {
            file << m_path_calc_times[i] << std::endl;
        }
    }
    file.close();
}

inline void EntropyBasedCameraPathController::outputViewpointCoords(
    const std::string& filename,
    const InSituVis::Viewpoint& viewpoint )
{
    std::ofstream file( filename );
    {
        file << "Index,X,Y,Z" << std::endl;
        for ( size_t i = 0; i < viewpoint.numberOfLocations(); i++ )
        {
            const auto l = viewpoint.at( i );
            file << i << "," << l.position.x() << "," << l.position.y() << "," << l.position.z() << std::endl;
        }
    }
    file.close();
}

inline void EntropyBasedCameraPathController::outputNumberOfImages(
    const std::string& filename,
    const size_t analysis_interval )
{
    std::ofstream file( filename );
    {
        file << "Time,The number of images" << std::endl;
        const auto interval = analysis_interval;
        for ( size_t i = 0; i < m_number_of_images.size(); i++ )
        {
            file << interval * i << "," << m_number_of_images[i] << std::endl;
        }
    }
    file.close();
}

} // end of namespace InSituVis
