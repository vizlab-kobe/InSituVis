#include <kvs/PointObject>
#include <kvs/LineObject>
#include <kvs/PolygonObject>
#include <kvs/StructuredVolumeObject>
#include <kvs/UnstructuredVolumeObject>


namespace
{

inline kvs::ObjectBase* GeometryObjectPointer(
    const kvs::GeometryObjectBase* geometry )
{
    switch ( geometry->geometryType() )
    {
    case kvs::GeometryObjectBase::Point:
    {
        using Geom = kvs::PointObject;
        auto* ret = new Geom();
        ret->shallowCopy( *Geom::DownCast( geometry ) );
        return ret;
    }
    case kvs::GeometryObjectBase::Line:
    {
        using Geom = kvs::LineObject;
        auto* ret = new Geom();
        ret->shallowCopy( *Geom::DownCast( geometry ) );
        return ret;
    }
    case kvs::GeometryObjectBase::Polygon:
    {
        using Geom = kvs::PolygonObject;
        auto* ret = new Geom();
        ret->shallowCopy( *Geom::DownCast( geometry ) );
        return ret;
    }
    default: return nullptr;
    }
}

inline kvs::ObjectBase* VolumeObjectPointer(
    const kvs::VolumeObjectBase* volume )
{
    switch ( volume->volumeType() )
    {
    case kvs::VolumeObjectBase::Structured:
    {
        using Volume = kvs::StructuredVolumeObject;
        auto* ret = new Volume();
        ret->shallowCopy( *Volume::DownCast( volume ) );
        return ret;
    }
    case kvs::VolumeObjectBase::Unstructured:
    {
        using Volume = kvs::UnstructuredVolumeObject;
        auto* ret = new Volume();
        ret->shallowCopy( *Volume::DownCast( volume ) );
        return ret;
    }
    default: return nullptr;
    }
}

inline kvs::ObjectBase* ObjectPointer( const kvs::ObjectBase& object )
{
    switch ( object.objectType() )
    {
    case kvs::ObjectBase::Geometry:
    {
        using Geom = kvs::GeometryObjectBase;
        return GeometryObjectPointer( Geom::DownCast( &object ) );
    }
    case kvs::ObjectBase::Volume:
    {
        using Volume = kvs::VolumeObjectBase;
        return VolumeObjectPointer( Volume::DownCast( &object ) );
    }
    default: return nullptr;
    }
}

} // end of namespace


namespace InSituVis
{

inline void AdaptiveTimeSelector::put( const Object& object )
{
    auto* p = ::ObjectPointer( object ); // pointer to the shallow copied object
    if ( p ) { m_data.push_back( Object::Pointer( p ) ); }
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
        BaseClass::incrementTimeCounter();
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
                int counter = 1;
                while ( !m_data_queue.empty() )
                {
                    BaseClass::setCurrentTimeIndex( time_index - m_data_queue.size() + 1 );
                    const auto V = m_data_queue.front();
                    if ( counter % R == 0 ) { this->visualize( V ); }
                    m_data_queue.pop();
                    counter++;
                }
            }

            // Pattern B
            else if ( D_crr >= D_thr )
            {
                while ( !m_data_queue.empty() )
                {
                    BaseClass::setCurrentTimeIndex( time_index - m_data_queue.size() + 1 );
                    const auto V = m_data_queue.front();
                    this->visualize( V );
                    m_data_queue.pop();
                }
            }

            // Pattern C
            else
            {
                const auto queue_size = m_data_queue.size();
                for ( size_t i = 0; i < queue_size / 2; ++i )
                {
                    BaseClass::setCurrentTimeIndex( time_index - m_data_queue.size() + 1 );
                    const auto V = m_data_queue.front();
                    this->visualize( V );
                    m_data_queue.pop();
                }

                int counter = 1;
                while ( !m_data_queue.empty() )
                {
                    BaseClass::setCurrentTimeIndex( time_index - m_data_queue.size() + 1 );
                    const auto V = m_data_queue.front();
                    if ( counter % R == 0 ) { this->visualize( V ); }
                    m_data_queue.pop();
                    counter++;
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
        const auto& object = *( sub_data.get() );
        BaseClass::doPipeline( object );
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

float AdaptiveTimeSelector::divergence( const Data& data0, const Data& data1 ) const
{
    return 0.0f;
}

} // end of namespace InSituVis
