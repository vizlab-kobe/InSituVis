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

inline void EntropyBasedCameraPathTimeStepController::push( const Data& data )
{
    BaseClass::setSubTimeIndex(999999);

    if ( !( this->isFinalStep() ) )
    {
        if ( this->isInitialStep() )
        {
            // Initial step.
            setValidationStep( true );
            this->process( data );
            setValidationStep( false );
            this->setPreviousData( data );
            // this->dataQueue().push( data );
            BaseClass::pushMaxEntropies( BaseClass::maxEntropy() );
            BaseClass::pushMaxPositions( BaseClass::maxPosition() );
            BaseClass::pushMaxRotations( BaseClass::maxRotation() );
            if ( this->isInterpolationMethod() == SQUAD )
            {
                BaseClass::pushMaxRotations( BaseClass::maxRotation() );
            }
            BaseClass::pushNumImages( 1 );
            BaseClass::setIsInitialStep( false );
            var_threshold.push_back( 0.0 );
            this->setPreviousDivergence( 0.0f );
            m_divergences.push_back( m_previous_divergence );
            // BaseClass::setCacheSize(9);
        }
        else
        {
            if ( this->isCacheEnabled() )
            {
                if ( this->isEntStep() )
                {
                    
                    std::cout << this->dataQueue().size() << "," << this->cacheSize() << "," << this->dataQueue().size() << std::endl;

                    const auto D_thr = m_threshold;
                    const auto V_crr = data;
                    const auto V_prv = previousData();
                    auto P_prv = kvs::AnyValueArray();
                    auto P_crr = kvs::AnyValueArray();
                    if( V_prv.size() != 0 & V_crr.size() != 0){
                        P_prv = Volume::DownCast( V_prv.front().get() )->values();
                        P_crr = Volume::DownCast( V_crr.front().get() )->values();
                        // std::cout << "------------------------------------------------------  " << typeid(P_crr).name() << std::endl;
                    }
                    const auto D_crr = this->divergence( P_prv, P_crr );
                    m_divergences.push_back( D_crr );
                    var_threshold.push_back( D_thr );

                    if( this->isThresholdDeterminationMethod() == ADAPTIVE && m_divergences.size() > 10){
                        var_divergences = m_divergences;
                        std::sort(var_divergences.begin(),var_divergences.end());
                        m_threshold = var_divergences[(int)(var_divergences.size()*m_reduction_rate)];
                    }

                    if ( D_crr >= D_thr )
                    {

                        // this->dataQueue().pop();
                        setValidationStep( true );
                        this->process( data );
                        setValidationStep( false );
                        BaseClass::pushMaxEntropies( BaseClass::maxEntropy() );
                        BaseClass::pushMaxPositions( BaseClass::maxPosition() );
                        BaseClass::pushMaxRotations( BaseClass::maxRotation() );
                        if ( this->dataQueue().size() == this->cacheSize() )
                        {
                            this->setIsErpStep( true );
                            BaseClass::createPath();
                            Data data_front;
                            BaseClass::setSubTimeIndex( 0 );
                            size_t num_points = this->path().size();
                            size_t num_images = ( num_points + 1 ) / this->entropyInterval();
                            for ( size_t i = 0; i < num_points; i++ )
                            {
                                if ( this->dataQueue().size() > 0 ) { data_front = this->dataQueue().front(); }
                                else { data_front = data; }
                                const std::pair<float, kvs::Quat> path_front = this->path().front();
                                this->process( data_front, path_front.first, path_front.second );
                                std::cout <<  num_points << ",  " <<  num_images << ", process timing,  " <<  i << std::endl;
                                BaseClass::path().pop();
                                BaseClass::setSubTimeIndex(BaseClass::subTimeIndex() + 1);
                                if ( BaseClass::subTimeIndex() == num_images )
                                {
                                    BaseClass::dataQueue().pop();
                                    BaseClass::pushNumImages( num_images );
                                    BaseClass::setSubTimeIndex( 0 );
                                }
                            }
                            if ( this->dataQueue().size() > 0 ) this->dataQueue().pop();
                            BaseClass::setSubTimeIndex( 0 );
                            BaseClass::pushNumImages( num_images );
                            BaseClass::popMaxPositions();
                            BaseClass::popMaxRotations();
                            BaseClass::setIsErpStep( false );
                            // }
                            // this->dataQueue().push( data );
                            
                        }
                        this->setPreviousData( data );
                    }
                    else
                    {
                        this->dataQueue().push( data );
                        BaseClass::pushMaxPositions( BaseClass::maxPosition() );
                        BaseClass::pushMaxRotations( BaseClass::maxRotation() );
                        // m_entropy_interval = m_entropy_interval + 1;
                        setEntropyInterval(entropyInterval() + 1);
                        BaseClass::createPath();
                        // m_entropy_interval = m_entropy_interval - 1;
                        setEntropyInterval(entropyInterval() - 1);
                        const auto S = this->dataQueue().size();
                        Data data_front;
                        for ( size_t i = 0; i < S  ; i++ )
                        {
                            data_front = this->dataQueue().front();
                            const std::pair<float, kvs::Quat> path_front = this->path().front();
                            this->process( data_front, path_front.first, path_front.second );
                            BaseClass::path().pop();
                            this->dataQueue().pop();
                        }
                        BaseClass::popMaxPositions();
                        BaseClass::popMaxRotations();
                        // BaseClass::pushMaxPositions( BaseClass::maxPosition() );
                        // BaseClass::pushMaxRotations( BaseClass::maxRotation() );
                        // BaseClass::createPath();
                        // this->dataQueue().push( data );
                        // data_front = this->dataQueue().front();
                        // const std::pair<float, kvs::Quat> path_front = this->path().front();
                        // this->process( data_front, path_front.first, path_front.second );
                        // BaseClass::path().pop();
                        // this->dataQueue().pop();
                        this->setPreviousData( data );
                    }  
                    if ( this->isInterpolationMethod() == SQUAD )
                    {
                        this->dataQueue().push( data );
                    }
                }
                else { 
                    this->dataQueue().push( data ); 
                }
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
        BaseClass::createPath();
        Data data_front;
        BaseClass::setSubTimeIndex( 0 );
        size_t num_points = this->path().size();
        size_t num_images = ( num_points + 1 ) / this->entropyInterval();

        for ( size_t i = 0; i < num_points; i++ )
        {
            if ( this->dataQueue().size() > 0 ) { data_front = this->dataQueue().front(); }
            else { data_front = data; }
            const std::pair<float, kvs::Quat> path_front = this->path().front();
            this->process( data_front, path_front.first, path_front.second );
            BaseClass::path().pop();
            BaseClass::setSubTimeIndex( BaseClass::subTimeIndex() + 1 );
            
            if( BaseClass::subTimeIndex() == num_images )
            {
                BaseClass::dataQueue().pop();
                BaseClass::pushNumImages( num_images );
                BaseClass::setSubTimeIndex( 0 );
            }
        }

        if ( this->dataQueue().size() > 0 ) this->dataQueue().pop();
        BaseClass::setSubTimeIndex( 0 );
        BaseClass::pushNumImages( num_images );
        BaseClass::popMaxPositions();
        BaseClass::popMaxRotations();

        while ( this->dataQueue().size() > 0 )
        {
            const auto data_front = this->dataQueue().front();
            this->process( data_front, final_position.length(), final_rotation );
            BaseClass::pushNumImages( 1 );
            BaseClass::dataQueue().pop();
        }
    }
}


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


inline float EntropyBasedCameraPathTimeStepController::divergence( const Values& P0, const Values& P1 )
{
    return m_divergence_function( P0, P1, m_threshold );
}



inline void EntropyBasedCameraPathTimeStepController::outputDivergences(
    const std::string& filename, 
    const std::vector<float>& divergences,
    const std::vector<float>& threshold  )
{
    std::ofstream file( filename );
    {
        file << "Time,Divergence" << std::endl;
        for ( size_t i = 0; i < divergences.size(); i++ )
        {
            file << i*m_interval << "," << divergences[i] << "," << threshold[i] << std::endl;
        }
    }
    file.close();
}

} // end of namespace InSituVis
