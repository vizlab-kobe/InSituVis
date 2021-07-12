/*****************************************************************************/
/**
 *  @file   Adaptor.hpp
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#include <kvs/Light>
#include <kvs/Camera>
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

// Shallow-copied object pointer.
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

inline void Adaptor::exec( const SimTime sim_time )
{
    if ( this->isAnalysisStep() )
    {
        const auto step = static_cast<float>( this->timeStep() );
        m_tstep_list.stamp( step );

        this->execPipeline( m_objects );
        this->execRendering();
    }

    this->incrementTimeStep();
    this->clearObjects();
}

inline bool Adaptor::dump()
{
    if ( m_tstep_list.title().empty() ) { m_tstep_list.setTitle( "Time step" ); }
    if ( m_pipe_timer.title().empty() ) { m_pipe_timer.setTitle( "Pipe time" ); }
    if ( m_rend_timer.title().empty() ) { m_rend_timer.setTitle( "Rend time" ); }
    if ( m_save_timer.title().empty() ) { m_save_timer.setTitle( "Save time" ); }

    const auto dir = m_output_directory.name() + "/";
    kvs::StampTimerList timer_list;
    timer_list.push( m_tstep_list );
    timer_list.push( m_pipe_timer );
    timer_list.push( m_rend_timer );
    timer_list.push( m_save_timer );
    return timer_list.write( dir + "vis_proc_time" + ".csv" );
}

inline void Adaptor::execPipeline( const Object& object )
{
    m_pipeline( m_screen, object );
}

inline void Adaptor::execPipeline( const ObjectList& objects )
{
    kvs::Timer timer( kvs::Timer::Start );
    for ( const auto& pobject : objects )
    {
        const auto& object = *( pobject.get() );
        this->execPipeline( object );
    }
    timer.stop();

    const auto pipe_time = m_pipe_timer.time( timer );
    m_pipe_timer.stamp( pipe_time );
}

inline void Adaptor::execPipeline()
{
    this->execPipeline( m_objects );
}

inline void Adaptor::execRendering()
{
    float rend_time = 0.0f;
    float save_time = 0.0f;
    {
        kvs::Timer timer_rend;
        kvs::Timer timer_save;
        for ( const auto& location : m_viewpoint.locations() )
        {
            // Draw and readback framebuffer
            timer_rend.start();
            auto color_buffer = this->readback( location );
            timer_rend.stop();
            rend_time += m_rend_timer.time( timer_rend );

            // Output framebuffer to image file
            timer_save.start();
            if ( m_enable_output_image )
            {
                const auto size = this->outputImageSize( location );
                const auto width = size.x();
                const auto height = size.y();
                kvs::ColorImage image( width, height, color_buffer );
                image.write( this->outputImageName( location ) );
            }
            timer_save.stop();
            save_time += m_save_timer.time( timer_save );
        }
    }
    m_rend_timer.stamp( rend_time );
    m_save_timer.stamp( save_time );
}

inline kvs::Vec2ui Adaptor::outputImageSize( const Viewpoint::Location& location ) const
{
    const auto image_size = kvs::Vec2ui( m_image_width, m_image_height );
    switch ( location.direction )
    {
    case Viewpoint::Direction::Uni: return image_size;
    case Viewpoint::Direction::Omni: return image_size * kvs::Vec2ui( 4, 3 );
    case Viewpoint::Direction::Adaptive:
    {
        const auto* object = m_screen.scene()->objectManager();
        if ( this->isInsideObject( location.position, object ) )
        {
            return image_size * kvs::Vec2ui( 4, 3 );
        }
        break;
    }
    default: break;
    }
    return image_size;
}

inline std::string Adaptor::outputImageName( const Viewpoint::Location& location, const std::string& surfix ) const
{
    const auto time = this->timeStep();
    const auto space = location.index;
    const auto output_time = kvs::String::From( time, 6, '0' );
    const auto output_space = kvs::String::From( space, 6, '0' );

    const auto output_basename = m_output_filename;
    const auto output_filename = output_basename + "_" + output_time + "_" + output_space;
    const auto filename = m_output_directory.name() + "/" + output_filename + surfix + ".bmp";
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

inline Adaptor::ColorBuffer Adaptor::readback( const Viewpoint::Location& location )
{
    switch ( location.direction )
    {
    case Viewpoint::Direction::Uni: return this->readback_uni_buffer( location );
    case Viewpoint::Direction::Omni: return this->readback_omn_buffer( location );
    case Viewpoint::Direction::Adaptive: return this->readback_adp_buffer( location );
    default: return this->backgroundColorBuffer();
    }
}

inline Adaptor::ColorBuffer Adaptor::readback_uni_buffer( const Viewpoint::Location& location )
{
    const auto position = location.position;
    const auto lookat = m_screen.scene()->camera()->lookAt();
    if ( lookat == position )
    {
        return this->backgroundColorBuffer();
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
        return m_screen.readbackColorBuffer();
    }
}

inline Adaptor::ColorBuffer Adaptor::readback_omn_buffer( const Viewpoint::Location& location )
{
    using SphericalColorBuffer = InSituVis::SphericalBuffer<kvs::UInt8>;

    const auto fov = m_screen.scene()->camera()->fieldOfView();
    const auto front = m_screen.scene()->camera()->front();
    const auto pc = m_screen.scene()->camera()->position();
    const auto pl = m_screen.scene()->light()->position();
    const auto& p = location.position;

    m_screen.scene()->light()->setPosition( p );
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

inline Adaptor::ColorBuffer Adaptor::readback_adp_buffer( const Viewpoint::Location& location )
{
    const auto* object = m_screen.scene()->objectManager();
    return this->isInsideObject( location.position, object ) ?
        this->readback_omn_buffer( location ) :
        this->readback_uni_buffer( location );
}

} // end of namespace InSituVis
