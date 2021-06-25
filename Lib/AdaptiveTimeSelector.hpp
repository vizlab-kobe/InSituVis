
namespace InSituVis
{

inline void AdaptiveTimeSelector::put( const Object& object )
{
    m_data.push_back( object );
}

inline void AdaptiveTimeSelector::exec( const kvs::UInt32 time_index )
{
    BaseClass::setCurrentTimeIndex( time_index );

    const auto tc = BaseClass::timeCounter();
    if ( tc == 0 ) // t == t0
    {
        this->visualize( m_data );
        m_previous_data = m_data;
        m_previous_divergence = 0.0f;
        m_data.clear();
        return;
    }

    const auto L = m_calculation_interval;
    const auto R = ( m_granularity == 0 ) ? L : m_granularity;
    const auto D_prv = m_previous_divergence;
    const auto D_thr = m_threshold;
    const auto V_prv = m_previous_data;

    // Vis. time-step: t % dt' == 0
    if ( BaseClass::canVisualize() )
    {
        m_data_queue.push( m_data );
        m_data.clear();

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
                    const auto V = m_data_queue.front();
                    if ( i % R == 0 ) { this->visualize( V ); }
                    m_data_queue.pop();
                    i++;
                }
            }

            // Pattern B
            else if ( D_crr >= D_thr )
            {
                while ( !m_data_queue.empty() )
                {
                    const auto V = m_data_queue.front();
                    this->visualize( V );
                    m_data_queue.pop();
                }
            }

            // Pattern C
            else
            {
                for ( size_t i = 0; i < m_data_queue.size() / 2; ++i )
                {
                    const auto V = m_data_queue.front();
                    this->visualize( V );
                    m_data_queue.pop();
                }

                int i = 1;
                while ( !m_data_queue.empty() )
                {
                    const auto V = m_data_queue.front();
                    if ( i % R == 0 ) { this->visualize( V ); }
                    m_data_queue.pop();
                    i++;
                }
            }
            DataQueue().swap( m_data_queue ); // Clear the queue
        }
    }

    BaseClass::incrementTimeCounter();
}

inline void AdaptiveTimeSelector::visualize( const Data& data )
{
    // Execute vis. pipelines for each sub-data
    for ( const auto& sub_data : data )
    {
        BaseClass::execPipeline( sub_data );
    }

    // Render and read-back the framebuffers.
    const auto& vp = BaseClass::viewpoint();
    const auto npoints = vp.numberOfPoints();
    for ( size_t i = 0; i < npoints; ++i )
    {
        BaseClass::setCurrentSpaceIndex( i );

        // Draw and readback framebuffer
        const auto& point = vp.point( i );
        auto color_buffer = BaseClass::readback( point );

        // Output framebuffer to image file
        if ( BaseClass::isOutputImageEnabled() )
        {
            const auto image_size = BaseClass::outputImageSize( point );
            kvs::ColorImage image( image_size.x(), image_size.y(), color_buffer );
            image.write( BaseClass::outputImageName() );
        }
    }
}

} // end of namespace InSituVis
