/*****************************************************************************/
/**
 *  @file   Adaptor.hpp
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#include <kvs/Math>
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
        // Stack current time step.
        const auto step = static_cast<float>( this->timeStep() );
        m_tstep_list.stamp( step );

        // Execute pipeline and rendering.
        this->execPipeline();
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
    for ( auto& pobject : objects )
    {
        auto& object = *( pobject.get() );
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

inline Adaptor::ColorBuffer Adaptor::drawScreen()
{
    m_screen.draw();
    return m_screen.readbackColorBuffer();
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
    const auto p = location.position;
    const auto a = location.look_at;
    const auto p_rtp = location.position_rtp;
    if ( p == a )
    {
        return this->backgroundColorBuffer();
    }
    else
    {
        auto* camera = m_screen.scene()->camera();
        auto* light = m_screen.scene()->light();

        // Backup camera and light info.
        const auto p0 = camera->position();
        const auto a0 = camera->lookAt();
        const auto u0 = camera->upVector();

        // Draw the scene.
        //const auto zero = kvs::Vec3::Zero();
        //const auto pa = a - p;
        //const auto rr = pa.cross( u0 );
        //const auto r = rr == zero ? ( a0 - p0 ).cross( u0 ) : rr;
        //const auto u = r.cross( pa );
        //camera->setPosition( p, a, u );
        //light->setPosition( p );
        //const auto buffer = this->drawScreen();

        //Draw the scene.
        kvs::Vec3 pp_rtp;
        if( p_rtp[1] < kvs::Math::pi / 2 ){
            pp_rtp = p_rtp - kvs::Vec3( { 0, kvs::Math::pi / 2, 0 } );
        }
        else{
            pp_rtp = -1 * ( p_rtp + kvs::Vec3( { 0, kvs::Math::pi / 2, 0 } ) );
        }
        const float pp_x = pp_rtp[0] * std::sin( pp_rtp[1] ) * std::sin( pp_rtp[2] );
        const float pp_y = pp_rtp[0] * std::cos( pp_rtp[1] );
        const float pp_z = pp_rtp[0] * std::sin( pp_rtp[1] ) * std::cos( pp_rtp[2] );
        const auto pp = kvs::Vec3( { pp_x, pp_y, pp_z } );
        const auto u = pp;
        camera->setPosition( p, a, u );
        light->setPosition( p );
        const auto buffer = this->drawScreen();
        

        // Restore camera and light info.
        camera->setPosition( p0, a0, u0 );
        light->setPosition( p0 );

        return buffer;
    }
}

inline Adaptor::ColorBuffer Adaptor::readback_omn_buffer( const Viewpoint::Location& location )
{
    using SphericalColorBuffer = InSituVis::SphericalBuffer<kvs::UInt8>;

    auto* camera = m_screen.scene()->camera();
    auto* light = m_screen.scene()->light();

    // Backup camera and light info.
    const auto fov = camera->fieldOfView();
    const auto front = camera->front();
    const auto cp = camera->position();
    const auto ca = camera->lookAt();
    const auto cu = camera->upVector();
    const auto lp = light->position();
    const auto& p = location.position;

    // Draw the scene.
    camera->setFieldOfView( 90.0 );
    camera->setFront( 0.1 );
    light->setPosition( p );

    SphericalColorBuffer color_buffer( m_screen.width(), m_screen.height() );
    for ( size_t i = 0; i < SphericalColorBuffer::Direction::NumberOfDirections; i++ )
    {
        const auto d = SphericalColorBuffer::Direction(i);
        const auto dir = SphericalColorBuffer::DirectionVector(d);
        const auto up = SphericalColorBuffer::UpVector(d);
        camera->setPosition( p, p + dir, up );
        const auto buffer = this->drawScreen();

        color_buffer.setBuffer( d, buffer );
    }

    // Restore camera and light info.
    camera->setFieldOfView( fov );
    camera->setFront( front );
    camera->setPosition( cp, ca, cu );
    light->setPosition( lp );

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
