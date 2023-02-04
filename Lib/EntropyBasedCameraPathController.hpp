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

inline void EntropyBasedCameraPathController::push( const Data& data )
{
    if( !( this->isFinalStep() ) )
    {
        if ( m_previous_data.empty() )
        {
            // Initial step.
            this->process( data );
            m_previous_data = data;
            m_max_entropies.push( m_max_entropy );
            m_max_positions.push( m_max_position );
            m_max_rotations.push( m_max_rotation );
            m_max_rotations.push( m_max_rotation );
            m_data_queue.push( data );
        }
        else
        {
            if ( this->isCacheEnabled() )
            {
                if( m_data_queue.size() % m_interval == 0 )
                {
                    this->process( data );
                    m_max_entropies.push( m_max_entropy );
                    m_max_positions.push( m_max_position );
                    m_max_rotations.push( m_max_rotation );

                    if( m_max_rotations.size() == 4 )
                    {
                        const auto q1 = m_max_rotations.front(); m_max_rotations.pop();
                        const auto q2 = m_max_rotations.front(); m_max_rotations.pop();
                        const auto q3 = m_max_rotations.front(); m_max_rotations.pop();
                        const auto q4 = m_max_rotations.front(); m_max_rotations.pop();

                        const auto p2 = m_max_positions.front(); m_max_positions.pop();
                        const auto p3 = m_max_positions.front();

                        const auto r2 = p2.length();
                        const auto r3 = p3.length();

                        this->createPath( r2, r3, q1, q2, q3, q4, m_interval );

                        m_data_queue.pop();
                        m_path_entropies.push_back( m_max_entropies.front() );
                        m_max_entropies.pop();
                        m_path_positions.push_back( p2[0] );
                        m_path_positions.push_back( p2[1] );
                        m_path_positions.push_back( p2[2] );

                        for ( size_t i = 0; i < m_interval - 1; i++ )
                        {
                            const auto data_front = m_data_queue.front();
                            const auto [ rad, rot ] = m_path.front();
                            const float radius = rad;
                            const kvs::Quat rotation = rot;
                            this->process( data_front, radius, rotation );
                            m_data_queue.pop();
                            m_path.pop();

                            m_path_entropies.push_back( m_max_entropy );
                            m_path_positions.push_back( m_max_position[0] );
                            m_path_positions.push_back( m_max_position[1] );
                            m_path_positions.push_back( m_max_position[2] );
                        }

                        m_max_rotations.push( q2 );
                        m_max_rotations.push( q3 );
                        m_max_rotations.push( q4 );
                    }
                    m_data_queue.push( data );
                }
                else
                {
                    m_data_queue.push( data );
                }
            }
        }
    }
    else
    {
        const auto q1 = m_max_rotations.front(); m_max_rotations.pop();
        const auto q2 = m_max_rotations.front(); m_max_rotations.pop();
        const auto q3 = m_max_rotations.front(); m_max_rotations.pop();
        const auto q4 = q3;

        const auto p2 = m_max_positions.front(); m_max_positions.pop();
        const auto p3 = m_max_positions.front();

        const auto r2 = p2.length();
        const auto r3 = p3.length();

        this->createPath( r2, r3, q1, q2, q3, q4, m_interval );

        m_data_queue.pop();
        m_path_entropies.push_back( m_max_entropies.front() );
        m_max_entropies.pop();
        m_path_positions.push_back( p2[0] );
        m_path_positions.push_back( p2[1] );
        m_path_positions.push_back( p2[2] );

        for ( size_t i = 0; i < m_interval - 1; i++ )
        {
            const auto data_front = m_data_queue.front();
            const auto [ rad, rot ] = m_path.front();
            const float radius = rad;
            const kvs::Quat rotation = rot;
            this->process( data_front, radius, rotation );
            m_data_queue.pop();
            m_path.pop();

            m_path_entropies.push_back( m_max_entropy );
            m_path_positions.push_back( m_max_position[0] );
            m_path_positions.push_back( m_max_position[1] );
            m_path_positions.push_back( m_max_position[2] );
        }

        while( m_data_queue.size() > 0 )
        {
            std::queue<std::tuple<float, kvs::Quat>> empty;
            m_path.swap( empty );

            for( size_t i = 0; i < m_interval - 1; i++ )
            {
                m_path.push( { r3, q3 } );
            }

            m_data_queue.pop();
            m_path_entropies.push_back( m_max_entropies.front() );
            m_max_entropies.pop();
            m_path_positions.push_back( p3[0] );
            m_path_positions.push_back( p3[1] );
            m_path_positions.push_back( p3[2] );

            for ( size_t i = 0; i < m_interval - 1; i++ )
            {
                const auto data_front = m_data_queue.front();
                const auto [ rad, rot ] = m_path.front();
                const float radius = rad;
                const kvs::Quat rotation = rot;
                this->process( data_front, radius, rotation );
                m_data_queue.pop();
                m_path.pop();

                m_path_entropies.push_back( m_max_entropy );
                m_path_positions.push_back( m_max_position[0] );
                m_path_positions.push_back( m_max_position[1] );
                m_path_positions.push_back( m_max_position[2] );
            }
        }
    }
}

float EntropyBasedCameraPathController::entropy( const FrameBuffer& frame_buffer )
{
    return m_entropy_function( frame_buffer );
}

float EntropyBasedCameraPathController::radiusInterpolation( const float r1, const float r2, const float t )
{
    return ( r2 -r1 ) * t * t * ( 3.0f - 2.0f * t ) + r1;
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
    //float path_calc_time = 0.0f;
    std::queue<std::tuple<float, kvs::Quat>> empty;
    m_path.swap( empty );

    kvs::Timer timer( kvs::Timer::Start );
    for( size_t i = 1; i < point_interval; i++ )
    {
        const float t = static_cast<float>( i ) / static_cast<float>( point_interval );
        const auto r = radiusInterpolation( r2, r3, t );
        const auto q = m_interpolator( q1, q2, q3, q4, t );
        m_path.push( { r, q } );
    }
    timer.stop();
    //path_calc_time += timer.sec();
    float path_calc_time = timer.sec();
    m_path_calc_times.push_back( path_calc_time );
}

} // end of namespace InSituVis
