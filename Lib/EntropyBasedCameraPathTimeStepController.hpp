#include <kvs/Math>
#include <kvs/Stat>
#include <kvs/Quaternion>
#include <kvs/LabColor>
#include <time.h>
#include <chrono>
#include <kvs/AnyValueArray>

namespace
{

template <typename T>
inline float Divergence(
    const kvs::ValueArray<T>& p0,
    const kvs::ValueArray<T>& p1,
    const float D_max )
{
    auto m0 = T(0);
    auto m1 = T(0);
    auto s0 = kvs::Stat::StdDev( p0, kvs::Stat::OnlineVar<T>, &m0 );
    auto s1 = kvs::Stat::StdDev( p1, kvs::Stat::OnlineVar<T>, &m1 );
    if ( kvs::Math::IsZero( s0 ) || kvs::Math::IsZero( s1 ) )
    {
        if ( kvs::Math::Equal( s0, s1 ) && kvs::Math::Equal( m0, m1 ) )
        {
            return 0.0f;
        }
        return D_max;
    }

    const auto a = std::log( s1 / s0 );
    const auto b = s0 * s0 + ( m0 - m1 ) * ( m0 - m1 );
    const auto c = 2.0 * s1 * s1;
    return a + b / c - 0.5f;
}

}

namespace InSituVis
{

inline float EntropyBasedCameraPathTimeStepController::GaussianKLDivergence(
    const Values& P0,
    const Values& P1,
    const float D_max )
{
    KVS_ASSERT( P0.typeID() == P1.typeID() );

    switch ( P0.typeID() )
    {
    case kvs::Type::TypeReal32:
    {
        const auto p0 = P0.asValueArray<kvs::Real32>();
        const auto p1 = P1.asValueArray<kvs::Real32>();
        return ::Divergence( p0, p1, D_max );
    }
    case kvs::Type::TypeReal64:
    {
        const auto p0 = P0.asValueArray<kvs::Real64>();
        const auto p1 = P1.asValueArray<kvs::Real64>();
        return ::Divergence( p0, p1, D_max );
    }
    default: return 0.0f;
    }
}

inline EntropyBasedCameraPathTimeStepController::EntropyFunction
EntropyBasedCameraPathTimeStepController::LightnessEntropy()
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

inline EntropyBasedCameraPathTimeStepController::EntropyFunction
EntropyBasedCameraPathTimeStepController::ColorEntropy()
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

inline EntropyBasedCameraPathTimeStepController::EntropyFunction
EntropyBasedCameraPathTimeStepController::DepthEntropy()
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

inline EntropyBasedCameraPathTimeStepController::EntropyFunction
EntropyBasedCameraPathTimeStepController::MixedEntropy(
    EntropyFunction e1,
    EntropyFunction e2,
    float p )
{
    return [e1,e2,p] ( const FrameBuffer& frame_buffer ) -> float
    {
        return p * e1( frame_buffer ) + ( 1 - p ) * e2( frame_buffer );
    };
}

inline EntropyBasedCameraPathTimeStepController::Interpolator
EntropyBasedCameraPathTimeStepController::Slerp()
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

inline EntropyBasedCameraPathTimeStepController::Interpolator
EntropyBasedCameraPathTimeStepController::Squad()
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

inline void EntropyBasedCameraPathTimeStepController::setOutputEvaluationImageEnabled(
    const bool enable,
    const bool enable_depth )
{
    m_enable_output_evaluation_image = enable;
    m_enable_output_evaluation_image_depth = enable_depth;
}

inline void EntropyBasedCameraPathTimeStepController::push( const Data& data )
{
    const auto interval = this->entropyInterval();
    if ( !( this->isFinalStep() ) )
    {
        
        if ( this->previousData().empty() )
        {
            // Initial step.
            setValidationStep( true );
            this->process( data );
            setValidationStep( false );
            this->setPreviousData( data );
            this->dataQueue().push( data );
            this->setPreviousPosition( this->maxPosition() );
            this->setPreviousRotation( this->maxRotation() );
            this->setPreviousDivergence( 0.0f );
            m_divergences.push_back( m_previous_divergence );
        }
        else
        {
            const auto D_thr = m_threshold;
            const auto V_prv = this->previousData();
            if ( this->isCacheEnabled() )
            {
                if ( this->dataQueue().size() % interval == 0 ){
                    const auto V_crr = data;
                    const auto V_prv = previousData();
                    const auto P_prv = Volume::DownCast( V_prv.front().get() )->values(); //m_previous_dataの更新
                    const auto P_crr = Volume::DownCast( V_crr.front().get() )->values(); //dataQueue().push( data )
                    const auto D_crr = this->divergence( P_prv, P_crr );
                    m_divergences.push_back( D_crr );
                    if ( D_crr >= D_thr )
                    {
                        this->dataQueue().pop();
                        setValidationStep( true );
                        this->process( data );
                        setValidationStep( false );
                        const auto q1 = this->previousRoation();
                        const auto q2 = this->maxRotation(); 
                        this->setPreviousPosition( this->maxPosition() );
                        this->setPreviousRotation( q2 );
                        this->pushPathPositions( this->previousPosition() );
                        const auto S = this->dataQueue().size();
                        for ( size_t i = 0; i < S  ; i++ )
                        {
                            const auto q = kvs::Quat::SphericalLinearInterpolation( q1, q2, static_cast<float>( i + 1 ) / ( S + 1 ), true, true );
                            const auto data_front = this->dataQueue().front();
                            this->process( data_front, 12, q );
                            this->dataQueue().pop();
                            this->pushPathPositions( this->maxPosition() );
                        }
                        this->setPreviousData( data );
                    }
                    else
                    {
                        const auto S = this->dataQueue().size();
                        for ( size_t i = 0; i < S  ; i++ )
                        {
                            this->dataQueue().pop();
                        }
                        this->setPreviousData( data );
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
        while ( this->dataQueue().size() > 0 )
        {
            const auto data_front = this->dataQueue().front();
            this->process( data_front, 12, this->maxRotation() );
            this->dataQueue().pop();
            this->pushPathPositions( this->maxPosition() );
        }
    }
}



inline float EntropyBasedCameraPathTimeStepController::entropy( const FrameBuffer& frame_buffer )
{
    return m_entropy_function( frame_buffer );
}

inline float EntropyBasedCameraPathTimeStepController::radiusInterpolation( const float r1, const float r2, const float t )
{
    return ( r2 -r1 ) * t * t * ( 3.0f - 2.0f * t ) + r1;
}

inline kvs::Quat EntropyBasedCameraPathTimeStepController::pathInterpolation(
    const kvs::Quat& q1,
    const kvs::Quat& q2,
    const kvs::Quat& q3,
    const kvs::Quat& q4,
    const float t )
{
    return m_interpolator( q1, q2, q3, q4, t );
}

inline void EntropyBasedCameraPathTimeStepController::createPath(
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
    for ( size_t i = 1; i < point_interval; i++ )
    {
        const auto t = static_cast<float>( i ) / static_cast<float>( point_interval );
        const auto r = this->radiusInterpolation( r2, r3, t );
        const auto q = this->pathInterpolation( q1, q2, q3, q4, t );
        const std::pair<float, kvs::Quat> elem( r, q );
        this->path().push( elem );
    }
    timer.stop();

    const auto path_calc_time = timer.sec();
    this->pathCalcTimes().push_back( path_calc_time );
}

inline float EntropyBasedCameraPathTimeStepController::divergence( const Values& P0, const Values& P1 )
{
    return m_divergence_function( P0, P1, m_threshold );
}

inline std::string EntropyBasedCameraPathTimeStepController::logDataFilename(
    const std::string& basename,
    const InSituVis::OutputDirectory& directory )
{
    return directory.baseDirectoryName() + "/" + basename + ".csv";
}

inline std::string EntropyBasedCameraPathTimeStepController::logDataFilename(
    const std::string& basename,
    const kvs::UInt32 timestep,
    const InSituVis::OutputDirectory& directory )
{
    const auto output_timestep = kvs::String::From( timestep, 6, '0' );
    const auto output_basename = basename + output_timestep;
    return directory.baseDirectoryName() + "/" + output_basename + ".csv";
}

inline void EntropyBasedCameraPathTimeStepController::outputEntropies(
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

inline void EntropyBasedCameraPathTimeStepController::outputPathEntropies(
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

inline void EntropyBasedCameraPathTimeStepController::outputPathPositions(
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

inline void EntropyBasedCameraPathTimeStepController::outputPathCalcTimes(
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

inline void EntropyBasedCameraPathTimeStepController::outputViewpointCoords(
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

inline void EntropyBasedCameraPathTimeStepController::outputDivergences(
    const std::string& filename, 
    const std::vector<float>& divergences )
{
    std::ofstream file( filename );
    {
        file << "Time,Divergence" << std::endl;
        for ( size_t i = 0; i < divergences.size(); i++ )
        {
            file << i*m_interval << "," << divergences[i] << std::endl;
        }
    }
    file.close();
}

} // end of namespace InSituVis
