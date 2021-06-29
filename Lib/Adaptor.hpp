/*****************************************************************************/
/**
 *  @file   Adaptor.hpp
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#include <kvs/PointObject>
#include <kvs/LineObject>
#include <kvs/PolygonObject>
#include <kvs/StructuredVolumeObject>
#include <kvs/UnstructuredVolumeObject>

// Termination process.
namespace
{
std::function<void(int)> Dump;
void Terminate( int sig ) { Dump( sig ); }
}

// Shallow copied object pointer.
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

inline Adaptor::Adaptor()
{
    // Set signal function for dumping timers.
    ::Dump = [&](int) { this->dump(); exit(0); };
    std::signal( SIGTERM, ::Terminate ); // (kill pid)
    std::signal( SIGQUIT, ::Terminate ); // Ctrl + \, Ctrl + 4
    std::signal( SIGINT,  ::Terminate ); // Ctrl + c
}

inline bool Adaptor::initialize()
{
    if ( !m_output_directory.create() )
    {
        this->log() << "ERRROR: " << "Cannot create output directory." << std::endl;
        return false;
    }
    m_screen.setSize( m_image_width, m_image_height );
    m_screen.create();
    return true;
}

inline bool Adaptor::finalize()
{
    return this->dump();
}

inline void Adaptor::put( const Adaptor::Object& object )
{
    auto* p = ::ObjectPointer( object ); // pointer to the shallow copied object
    if ( p ) { m_objects.push_back( Object::Pointer( p ) ); }
}

inline void Adaptor::exec( const kvs::UInt32 time_index )
{
    // Visualize the processed volume data.
    this->setCurrentTimeIndex( time_index );
    {
        if ( this->canVisualize() )
        {
            this->doPipeline( m_objects );
            this->doRendering();
        }
        else
        {
            m_pipe_timer.stamp( 0.0f );
            m_rend_timer.stamp( 0.0f );
            m_save_timer.stamp( 0.0f );
        }
    }
    this->incrementTimeCounter();
    this->clearObjects();
}

inline bool Adaptor::dump()
{
    if ( m_pipe_timer.title().empty() ) { m_pipe_timer.setTitle( "Pipe time" ); }
    if ( m_rend_timer.title().empty() ) { m_rend_timer.setTitle( "Rend time" ); }
    if ( m_save_timer.title().empty() ) { m_save_timer.setTitle( "Save time" ); }

    const auto dir = m_output_directory.name() + "/";
    kvs::StampTimerList timer_list;
    timer_list.push( m_pipe_timer );
    timer_list.push( m_rend_timer );
    timer_list.push( m_save_timer );
    return timer_list.write( dir + "vis_proc_time" + ".csv" );
}

inline void Adaptor::doPipeline( const Object& object )
{
    m_pipeline( m_screen, object );
}

inline void Adaptor::doPipeline( const ObjectList& objects )
{
    kvs::Timer timer( kvs::Timer::Start );
    for ( const auto& pobject : objects )
    {
        const auto& object = *( pobject.get() );
        this->doPipeline( object );
    }
    timer.stop();

    const auto pipe_time = m_pipe_timer.time( timer );
    m_pipe_timer.stamp( pipe_time );
}

inline void Adaptor::doPipeline()
{
    this->doPipeline( m_objects );
}

inline void Adaptor::doRendering()
{
    float rend_time = 0.0f;
    float save_time = 0.0f;
    {
        kvs::Timer timer_rend;
        kvs::Timer timer_save;
        const auto npoints = m_viewpoint.numberOfPoints();
        for ( size_t i = 0; i < npoints; ++i )
        {
            this->setCurrentSpaceIndex( i );

            // Draw and readback framebuffer
            timer_rend.start();
            const auto& point = m_viewpoint.point( i );
            auto color_buffer = this->readback( point );
            timer_rend.stop();
            rend_time += m_rend_timer.time( timer_rend );

            // Output framebuffer to image file
            timer_save.start();
            if ( m_enable_output_image )
            {
                const auto image_size = this->outputImageSize( point );
                kvs::ColorImage image( image_size.x(), image_size.y(), color_buffer );
                image.write( this->outputImageName() );
            }
            timer_save.stop();
            save_time += m_save_timer.time( timer_save );
        }
    }
    m_rend_timer.stamp( rend_time );
    m_save_timer.stamp( save_time );
}

inline kvs::Vec2ui Adaptor::outputImageSize( const Viewpoint::Point& point ) const
{
    const auto image_size = kvs::Vec2ui( m_image_width, m_image_height );
    switch ( point.dir_type )
    {
    case Viewpoint::SingleDir: return image_size;
    case Viewpoint::OmniDir: return image_size * kvs::Vec2ui( 4, 3 );
    default:
        const auto* object = m_screen.scene()->objectManager();
        if ( this->isInsideObject( point.position, object ) )
        {
            return image_size * kvs::Vec2ui( 4, 3 );
        }
        break;
    }
    return image_size;
}

inline std::string Adaptor::outputImageName( const std::string& surfix ) const
{
    const auto time = this->currentTimeIndex();
    const auto space = this->currentSpaceIndex();
    const std::string output_time = kvs::String::From( time, 6, '0' );
    const std::string output_space = kvs::String::From( space, 6, '0' );

    const std::string output_basename = m_output_filename;
    const std::string output_filename = output_basename + "_" + output_time + "_" + output_space;
    const std::string filename = m_output_directory.name() + "/" + output_filename + surfix + ".bmp";
    return filename;
}

inline Adaptor::ColorBuffer Adaptor::backgroundColorBuffer() const
{
    const auto color = m_screen.scene()->background()->color();
    const auto width = m_screen.width();
    const auto height = m_screen.height();
    const size_t npixels = width * height;
    ColorBuffer buffer( npixels * 4 );
    for ( size_t i = 0; i < npixels; ++i )
    {
        buffer[ 4 * i + 0 ] = color.r();
        buffer[ 4 * i + 1 ] = color.g();
        buffer[ 4 * i + 2 ] = color.b();
        buffer[ 4 * i + 3 ] = 255;
    }
    return buffer;
}

inline bool Adaptor::isInsideObject( const kvs::Vec3& position, const kvs::ObjectBase* object ) const
{
    const auto min_obj = object->minObjectCoord();
    const auto max_obj = object->maxObjectCoord();
    const auto p_obj = kvs::WorldCoordinate( position ).toObjectCoordinate( object ).position();
    return
        ( min_obj.x() <= p_obj.x() ) && ( p_obj.x() <= max_obj.x() ) &&
        ( min_obj.y() <= p_obj.y() ) && ( p_obj.y() <= max_obj.y() ) &&
        ( min_obj.z() <= p_obj.z() ) && ( p_obj.z() <= max_obj.z() );
}

inline Adaptor::ColorBuffer Adaptor::readback( const Viewpoint::Point& point )
{
    switch ( point.dir_type )
    {
    case Viewpoint::SingleDir:
        return this->readback_plane_buffer( point.position );
    case Viewpoint::OmniDir:
        return this->readback_spherical_buffer( point.position );
    case Viewpoint::AdaptiveDir:
    {
        if ( this->isInsideObject( point.position, m_screen.scene()->objectManager() ) )
        {
            return this->readback_spherical_buffer( point.position );
        }
        return this->readback_plane_buffer( point.position );
    }
    default:
        break;
    }

    return this->backgroundColorBuffer();
}

inline Adaptor::ColorBuffer Adaptor::readback_plane_buffer( const kvs::Vec3& position )
{
    ColorBuffer color_buffer;
    const auto lookat = m_screen.scene()->camera()->lookAt();
    if ( lookat == position )
    {
        color_buffer = this->backgroundColorBuffer();
    }
    else
    {
        const auto p0 = ( m_screen.scene()->camera()->position() - lookat ).normalized();
        const auto p1 = ( position- lookat ).normalized();
        const auto axis = p0.cross( p1 );
        const auto deg = kvs::Math::Rad2Deg( std::acos( p0.dot( p1 ) ) );
        const auto R = kvs::RotationMatrix33<float>( axis, deg );
        const auto up = m_screen.scene()->camera()->upVector() * R;
        m_screen.scene()->camera()->setPosition( position, lookat, up );
        m_screen.scene()->light()->setPosition( position );
        m_screen.draw();
        color_buffer = m_screen.readbackColorBuffer();
    }
    return color_buffer;
}

inline Adaptor::ColorBuffer Adaptor::readback_spherical_buffer( const kvs::Vec3& position )
{
    using SphericalColorBuffer = InSituVis::SphericalBuffer<kvs::UInt8>;

    const auto fov = m_screen.scene()->camera()->fieldOfView();
    const auto front = m_screen.scene()->camera()->front();
    const auto pc = m_screen.scene()->camera()->position();
    const auto pl = m_screen.scene()->light()->position();
    const auto& p = position;

    m_screen.scene()->light()->setPosition( position );
    m_screen.scene()->camera()->setFieldOfView( 90.0 );
    m_screen.scene()->camera()->setFront( 0.1 );

    SphericalColorBuffer color_buffer( m_screen.width(), m_screen.height() );
    for ( size_t i = 0; i < SphericalColorBuffer::Direction::NumberOfDirections; i++ )
    {
        const auto d = SphericalColorBuffer::Direction(i);
        const auto dir = SphericalColorBuffer::DirectionVector(d);
        const auto up = SphericalColorBuffer::UpVector(d);
        m_screen.scene()->camera()->setPosition( p, p + dir, up );
        m_screen.draw();

        const auto buffer = m_screen.readbackColorBuffer();
        color_buffer.setBuffer( d, buffer );
    }

    m_screen.scene()->camera()->setFieldOfView( fov );
    m_screen.scene()->camera()->setFront( front );
    m_screen.scene()->camera()->setPosition( pc );
    m_screen.scene()->light()->setPosition( pl );

    const size_t nchannels = 4; // rgba
    return color_buffer.stitch<nchannels>();
}

} // end of namespace InSituVis
