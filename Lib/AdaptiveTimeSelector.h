/*****************************************************************************/
/**
 *  @file   AdaptiveTimeSelector.h
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include "Adaptor.h"
#include <list>
#include <queue>


namespace InSituVis
{

class AdaptiveTimeSelector : public InSituVis::Adaptor
{
public:
    using BaseClass = InSituVis::Adaptor;
    using Data = std::list<BaseClass::Volume>;
    using DataQueue = std::queue<Data>;

private:
    size_t m_calculation_interval = 1; ///< time interval of KL calculation
    size_t m_granularity = 0; ///< granularity for coarse grained sampling
    float m_threshold = 0.0f; ///< threshold value for similarity evalution based on KL divergence

    Data m_data{};
    DataQueue m_data_queue{};
    Data m_previous_data{};
    float m_previous_divergence = 0.0f;

public:
    AdaptiveTimeSelector() = default;
    virtual ~AdaptiveTimeSelector() = default;

    size_t calculationInterval() const { return m_calculation_interval; }
    size_t samplingGranularity() const { return m_granularity; }
    float similarityThreshold() const { return m_threshold; }

    void setCalculationInterval( const size_t interval )
    {
        m_calculation_interval = interval;
    }

    void setSamplingGranularity( const size_t granularity )
    {
        m_granularity = granularity;
    }

    void setSimilarityThreshold( const float threshold )
    {
        m_threshold = threshold;
    }

    virtual void put( const Volume& volume )
    {
        m_data.push_back( volume );
    }

    virtual void exec( const kvs::UInt32 time_index )
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

private:
    void visualize( const Data& data )
    {
        // Execute vis. pipelines for each sub-volume
        for ( const auto& volume : data )
        {
            BaseClass::execPipeline( volume );
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

    float divergence( const Data& data0, const Data& data1 ) const
    {
        return 1.0f;
    }
};

} // end of namespace InSituVis
