
namespace InSituVis
{

inline void AdaptiveTimeSelector::exec( const kvs::UInt32 time_index )
{
    BaseClass::setCurrentTimeIndex( time_index );

    const auto tc = BaseClass::timeCounter();
    if ( tc == 0 ) // t == t0
    {
        BaseClass::doPipeline();
        BaseClass::doRendering();
        m_previous_data = BaseClass::objects();
        m_previous_divergence = 0.0f;
        BaseClass::incrementTimeCounter();
        BaseClass::clearObjects();
        return;
    }

    const auto L = m_interval;
    const auto R = ( m_granularity == 0 ) ? L : m_granularity;
    const auto D_prv = m_previous_divergence;
    const auto D_thr = m_threshold;
    const auto V_prv = m_previous_data;

    // Vis. time-step: t % dt' == 0
    if ( BaseClass::canVisualize() )
    {
        m_data_queue.push( BaseClass::objects() );
        BaseClass::clearObjects();

        // KL time-step
        if ( m_data_queue.size() >= L )
        {
            const auto V_crr = m_data_queue.back();
            const auto D_crr = this->divergence( V_prv, V_crr );
            m_previous_data = V_crr;

            // Pattern A
            if ( D_prv < D_thr && D_crr < D_thr )
            {
                int counter = 1;
                while ( !m_data_queue.empty() )
                {
                    const auto index = time_index - m_data_queue.size() + 1;
                    BaseClass::setCurrentTimeIndex( index );
                    const auto V = m_data_queue.front();
                    if ( counter % R == 0 )
                    {
                        BaseClass::doPipeline( V );
                        BaseClass::doRendering();
                    }
                    m_data_queue.pop();
                    counter++;
                }
            }

            // Pattern B
            else if ( D_crr >= D_thr )
            {
                while ( !m_data_queue.empty() )
                {
                    const auto index = time_index - m_data_queue.size() + 1;
                    BaseClass::setCurrentTimeIndex( index );
                    const auto V = m_data_queue.front();
                    BaseClass::doPipeline( V );
                    BaseClass::doRendering();
                    m_data_queue.pop();
                }
            }

            // Pattern C
            else
            {
                const auto queue_size = m_data_queue.size();
                for ( size_t i = 0; i < queue_size / 2; ++i )
                {
                    const auto index = time_index - m_data_queue.size() + 1;
                    BaseClass::setCurrentTimeIndex( index );
                    const auto V = m_data_queue.front();
                    BaseClass::doPipeline( V );
                    BaseClass::doRendering();
                    m_data_queue.pop();
                }

                int counter = 1;
                while ( !m_data_queue.empty() )
                {
                    const auto index = time_index - m_data_queue.size() + 1;
                    BaseClass::setCurrentTimeIndex( index );
                    const auto V = m_data_queue.front();
                    if ( counter % R == 0 )
                    {
                        BaseClass::doPipeline( V );
                        BaseClass::doRendering();
                    }
                    m_data_queue.pop();
                    counter++;
                }
            }
            DataQueue().swap( m_data_queue ); // Clear the queue
        }
    }

    BaseClass::incrementTimeCounter();
}

float AdaptiveTimeSelector::divergence( const Data& data0, const Data& data1 ) const
{
    return 0.0f;
}

} // end of namespace InSituVis
