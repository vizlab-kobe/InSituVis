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
        const std::vector<kvs::Quat>& q,
        float t ) -> kvs::Quat
    {
        return kvs::Quat::SphericalLinearInterpolation( q[0], q[1], t, true, true );
    };
}

inline EntropyBasedCameraPathController::Interpolator
EntropyBasedCameraPathController::Squad()
{
    return [] (
        const std::vector<kvs::Quat>& q,
        float t ) -> kvs::Quat
    {
        return kvs::Quat::SphericalQuadrangleInterpolation( q[0], q[1], q[2], q[3], t, true );
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
    m_sub_time_index = 999999;

    if ( !( this->isFinalStep() ) )
    {
        if ( this->isInitialStep() )
        {
            // Initial step.
            this->process( data );
            this->pushMaxEntropies( this->maxEntropy() );
            this->pushMaxPositions( this->maxPosition() );
            this->pushMaxRotations( this->maxRotation() );
            this->pushNumImages( 1 );
            this->setIsInitialStep( false );
        }
        else
        {
            if ( this->isCacheEnabled() )
            {
                if ( this->isEntStep() )
                {
                    this->process( data );
                    this->pushMaxEntropies( this->maxEntropy() );
                    this->pushMaxPositions( this->maxPosition() );
                    this->pushMaxRotations( this->maxRotation() );

                    if ( this->dataQueue().size() == this->cacheSize() )
                    {
                        this->setIsErpStep( true );
                        this->createPath();
                        Data data_front;
                        m_sub_time_index = 0;
                        size_t num_points = this->path().size();
                        size_t num_images = ( num_points + 1 ) / this->entropyInterval();

                        for ( size_t i = 0; i < num_points; i++ )
                        {
                            if ( this->dataQueue().size() > 0 ) { data_front = this->dataQueue().front(); }
                            else { data_front = data; }
                            const std::pair<float, kvs::Quat> path_front = this->path().front();
                            this->process( data_front, path_front.first, path_front.second );
                            this->path().pop();
                            m_sub_time_index += 1;
                            
                            if ( m_sub_time_index == num_images )
                            {
                                this->dataQueue().pop();
                                this->pushNumImages( num_images );
                                m_sub_time_index = 0;
                            }
                        }

                        if ( this->dataQueue().size() > 0 ) this->dataQueue().pop();
                        m_sub_time_index = 0;
                        this->pushNumImages( num_images );
                        this->popMaxPositions();
                        this->popMaxRotations();
                        this->setIsErpStep( false );
                    }
                    this->dataQueue().push( data );
                }
                else { this->dataQueue().push( data ); }
            }
        }
    }
    else
    {
        const auto final_position = this->maxPositions().back();
        const auto final_rotation = this->maxRotations().back();
        this->pushMaxPositions( final_position );
        this->pushMaxRotations( final_rotation );

        this->setIsErpStep( true );
        this->createPath();
        Data data_front;
        m_sub_time_index = 0;
        size_t num_points = this->path().size();
        size_t num_images = ( num_points + 1 ) / this->entropyInterval();

        for ( size_t i = 0; i < num_points; i++ )
        {
            if ( this->dataQueue().size() > 0 ) { data_front = this->dataQueue().front(); }
            else { data_front = data; }
            const std::pair<float, kvs::Quat> path_front = this->path().front();
            this->process( data_front, path_front.first, path_front.second );
            this->path().pop();
            m_sub_time_index += 1;
            
            if( m_sub_time_index == num_images )
            {
                this->dataQueue().pop();
                this->pushNumImages( num_images );
                m_sub_time_index = 0;
            }
        }

        if ( this->dataQueue().size() > 0 ) this->dataQueue().pop();
        m_sub_time_index = 0;
        this->pushNumImages( num_images );
        this->popMaxPositions();
        this->popMaxRotations();

        while ( this->dataQueue().size() > 0 )
        {
            const auto data_front = this->dataQueue().front();
            this->process( data_front, final_position.length(), final_rotation );
            this->pushNumImages( 1 );
            this->dataQueue().pop();
        }
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
    const std::vector<kvs::Quat>& q,
    const float t )
{
    return m_interpolator( q, t );
}

inline void EntropyBasedCameraPathController::createPath()
{
    std::queue<std::pair<float, kvs::Quat>> empty;
    this->path().swap( empty );
    
    kvs::Timer timer( kvs::Timer::Start );

    const auto positions = this->maxPositions();
    const auto rotations = this->maxRotations();

    const size_t n = 256;
    float l = 0.0f;

    for ( size_t i = 0; i < n; i++ )
    {
        const auto t0 = static_cast<float>( i ) / static_cast<float>( n );
        const auto rad0 = this->radiusInterpolation( positions[0].length(), positions[1].length(), t0 );
        const auto rot0 = this->pathInterpolation( rotations, t0 );
        const auto p0 = kvs::Quat::Rotate( kvs::Vec3( { 0.0f, rad0, 0.0f } ), rot0 );

        const auto t1 = static_cast<float>( i + 1 ) / static_cast<float>( n );
        const auto rad1 = this->radiusInterpolation( positions[0].length(), positions[1].length(), t1 );
        const auto rot1 = this->pathInterpolation( rotations, t1 );
        const auto p1 = kvs::Quat::Rotate( kvs::Vec3( { 0.0f, rad1, 0.0f } ), rot1 );

        const auto u = p1 - p0;
        l += u.length();
    }

    const size_t num_images = static_cast<size_t>( l / ( m_entropy_interval * m_delta ) ) + 1;
    const size_t num_points = num_images * m_entropy_interval - 1;

    for ( size_t i = 0; i < num_points; i++ )
    {
        const auto t = static_cast<float>( i + 1 ) / static_cast<float>( num_points + 1 );
        const auto rad = this->radiusInterpolation( positions[0].length(), positions[1].length(), t );
        const auto rot = this->pathInterpolation( rotations, t );
        const std::pair<float, kvs::Quat> elem( rad, rot );
        this->path().push( elem );
    }

    timer.stop();

    const auto path_calc_time = timer.sec();
    this->pushPathCalcTimes( path_calc_time );
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

inline void EntropyBasedCameraPathController::outputMaxEntropies(
    const std::string& filename )
{
    std::ofstream file( filename );
    {
        file << "Entropy" << std::endl;
        const auto max_entropies = this->maxEntropies();
        for ( size_t i = 0; i < max_entropies.size(); i++ )
        {
            file << max_entropies[i] << std::endl;
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
        const auto path_calc_times = this->pathCalcTimes();
        for ( size_t i = 0; i < path_calc_times.size(); i++ )
        {
            file << path_calc_times[i] << std::endl;
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

inline void EntropyBasedCameraPathController::outputNumImages(
    const std::string& filename,
    const size_t interval )
{
    std::ofstream file( filename );
    {
        file << "Time,The number of images" << std::endl;
        const auto num_images = this->numImages();
        for ( size_t i = 0; i < num_images.size(); i++ )
        {
            file << interval * i << "," << num_images[i] << std::endl;
        }
    }
    file.close();
}

} // end of namespace InSituVis
