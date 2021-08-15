#include <kvs/Math>
#include <kvs/Stat>


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

inline float AdaptiveTimestepController::GaussianKLDivergence(
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

inline void AdaptiveTimestepController::push( const Data& data )
{
    if ( m_data_queue.empty() && m_previous_data.empty() )
    {
        // Initial step.
        this->process( data );
        m_previous_data = data;
        m_previous_divergence = 0.0f;
    }
    else
    {
        const auto L = m_interval;
        const auto R = ( m_granularity == 0 ) ? L : m_granularity;
        const auto D_prv = m_previous_divergence;
        const auto D_thr = m_threshold;
        const auto V_prv = m_previous_data;

        if ( this->isCacheEnabled() )
        {
            m_data_queue.push( data );

            if ( m_data_queue.size() >= L )
            {
                const auto V_crr = m_data_queue.back();
                const auto P_prv = Volume::DownCast( V_prv.front().get() )->values();
                const auto P_crr = Volume::DownCast( V_crr.front().get() )->values();
                const auto D_crr = this->divergence( P_prv, P_crr );
                m_previous_data = V_crr;
                m_previous_divergence = D_crr;

                //std::cout << "D_prv: " << D_prv << std::endl;
                //std::cout << "D_crr: " << D_crr << std::endl;

                // Pattern A
                if ( D_prv < D_thr && D_crr < D_thr )
                {
                    //std::cout << "\tPttern A" << std::endl;

                    int i = 1;
                    while ( !m_data_queue.empty() )
                    {
                        if ( i % R == 0 )
                        {
                            this->process( m_data_queue.front() );
                        }
                        m_data_queue.pop();
                        i++;
                    }
                }

                // Pattern B
                else if ( D_crr >= D_thr )
                {
                    //std::cout << "\tPttern B" << std::endl;

                    while ( !m_data_queue.empty() )
                    {
                        this->process( m_data_queue.front() );
                        m_data_queue.pop();
                    }
                }

                // Pattern C
                else
                {
                    //std::cout << "\tPttern C" << std::endl;

                    const auto queue_size = m_data_queue.size();
                    for ( size_t i = 0; i < queue_size / 2; ++i )
                    {
                        this->process( m_data_queue.front() );
                        m_data_queue.pop();
                    }

                    int i = 1;
                    while ( !m_data_queue.empty() )
                    {
                        if ( i % R == 0 )
                        {
                            this->process( m_data_queue.front() );
                        }
                        m_data_queue.pop();
                        i++;
                    }
                }
                DataQueue().swap( m_data_queue ); // Clear the queue
            }
        }
    }
}

inline float AdaptiveTimestepController::divergence( const Values& P0, const Values& P1 )
{
    return m_divergence_function( P0, P1, m_threshold );
}

} // end of namespace InSituVis
