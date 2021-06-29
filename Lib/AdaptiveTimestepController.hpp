
namespace InSituVis
{

inline void AdaptiveTimestepController::exec( const Data& data, const kvs::UInt32 time_index )
{
    if ( m_data_queue.empty() && m_previous_data.empty() ) // initial step
    {
        this->doVis( data, time_index );
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

        // Vis. time-step: t % dt' == 0
        if ( this->canVis() )
        {
            m_data_queue.push( data );

            // KL time-step
            if ( m_data_queue.size() >= L )
            {
                const auto V_crr = m_data_queue.back();
                const auto D_crr = this->divergence( V_prv, V_crr );
                m_previous_data = V_crr;

                // Pattern A
                if ( D_prv < D_thr && D_crr < D_thr )
                {
                    int i = 1;
                    while ( !m_data_queue.empty() )
                    {
                        if ( i % R == 0 )
                        {
                            this->doVis( m_data_queue.front(), time_index );
                        }
                        m_data_queue.pop();
                        i++;
                    }
                }

                // Pattern B
                else if ( D_crr >= D_thr )
                {
                    while ( !m_data_queue.empty() )
                    {
                        this->doVis( m_data_queue.front(), time_index );
                        m_data_queue.pop();
                    }
                }

                // Pattern C
                else
                {
                    const auto queue_size = m_data_queue.size();
                    for ( size_t i = 0; i < queue_size / 2; ++i )
                    {
                        this->doVis( m_data_queue.front(), time_index );
                        m_data_queue.pop();
                    }

                    int i = 1;
                    while ( !m_data_queue.empty() )
                    {
                        if ( i % R == 0 )
                        {
                            this->doVis( m_data_queue.front(), time_index );
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

} // end of namespace InSituVis
