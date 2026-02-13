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

#include <sys/stat.h>
#include <fstream>
#include <kvs/String>

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
    // 複数 Viewpoint がセットされていない場合は従来通り
    if ( m_viewpoints.empty() )
    {
        if ( this->isAnalysisStep() )
        {
            const auto step = static_cast<float>( this->timeStep() );
            m_tstep_list.stamp( step );

            this->execPipeline();
            this->execRendering();
        }

        this->incrementTimeStep();
        this->clearObjects();
        return;
    }

    // 複数 Viewpoint がある場合：同一タイムステップで順に出力
    if ( this->isAnalysisStep() )
    {
        const auto step = static_cast<float>( this->timeStep() );
        m_tstep_list.stamp( step );

        // 現在のサブディレクトリ名を退避（戻すため）
        const std::string original_subdir = m_output_directory.subDirectoryName();

        for ( const auto& vp_name : m_viewpoints )
        {
            const auto& vp        = vp_name.first;
            const auto& save_name = vp_name.second;

            // Viewpoint を差し替え
            this->setViewpoint( vp );

            // 出力ルートを「base/save_name」へ切替
            m_output_directory.setSubDirectoryName( save_name );
            // 必要ならディレクトリ作成
            m_output_directory.create();

            // 各 Viewpoint ごとにパイプライン＆レンダリング
            this->execPipeline();
            this->execRendering();
        }

        // サブディレクトリ名を元に戻す
        m_output_directory.setSubDirectoryName( original_subdir );
        this->incrementTimeStep();
        this->clearObjects();
    }


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
    const std::string base_dir = m_output_directory.baseDirectoryName();
    const int time_step = static_cast<int>( this->timeStep() );
    m_pipeline( m_screen, object, base_dir, time_step );
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

    // ▼ 追加: 出力ディレクトリ "base/params" を用意
    // const std::string base_dir   = m_output_directory.baseDirectoryName();
    // const std::string params_dir = base_dir + "/viewpoints";


    // 変更点：出力ルートを「base/sub を含む name()」にする
    const std::string root_dir   = m_output_directory.name();
    const std::string params_dir = root_dir + "/viewpoints";
    {
        struct stat st;
        if ( ::stat( params_dir.c_str(), &st ) != 0 )
        {
            if ( ::mkdir( params_dir.c_str(), 0755 ) != 0 )
            {
                std::cerr << "Warning: Failed to create directory " << params_dir << "\n";
            }
        }
    }

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
                kvs::ColorImage image( size.x(), size.y(), color_buffer );
                image.write( this->outputImageName( location ) );
            }
            timer_save.stop();
            save_time += m_save_timer.time( timer_save );

            // ▼ 追加: カメラ姿勢の JSON 出力（各 location ごと）
            {
                const auto time  = this->timeStep();

                // 初めだけ視点を登録していく
                if( time == 0 )
                {
                    const auto space = location.index;
                    const auto out_time  = kvs::String::From( time,  6, '0' );
                    const auto out_space = kvs::String::From( space, 6, '0' );

                    const std::string json_basename = m_output_filename + "_" + out_space;
                    const std::string json_path = params_dir + "/" + json_basename + ".json";

                    std::ofstream ofs( json_path );
                    if ( !ofs.is_open() )
                    {
                        std::cerr << "Error: Cannot open " << json_path << " for writing.\n";
                    }
                    else
                    {
                        kvs::Camera* cam = m_screen.scene()->camera();
                        const float fovY = cam->fieldOfView();
                        const float fovX = fovY; // 同値で運用

                        const auto& pos    = location.position;
                        const auto& up     = location.up_vector;
                        const auto& lookAt = location.look_at;
                        const auto& rot    = location.rotation;

                        ofs << "{\n";
                        ofs << "  \"camera_index\": " << int(space) << ",\n";
                        ofs << "  \"camera_position\": [" << pos.x() << ", " << pos.y() << ", " << pos.z() << "],\n";
                        ofs << "  \"up_vector\": [" << up.x() << ", " << up.y() << ", " << up.z() << "],\n";
                        ofs << "  \"look_at\": [" << lookAt.x() << ", " << lookAt.y() << ", " << lookAt.z() << "],\n";
                        ofs << "  \"rotation\": [" << rot.x() << ", " << rot.y() << ", " << rot.z() << ", " << rot.w() << "],\n";
                        ofs << "  \"fovY\": " << fovY << ",\n";
                        ofs << "  \"fovX\": " << fovX << "\n";
                        ofs << "}\n";
                    }

                }

            }
        }
    }

    m_rend_timer.stamp( rend_time );
    m_save_timer.stamp( save_time );
}

inline Adaptor::ColorBuffer Adaptor::drawScreen()
//inline Adaptor::ColorBuffer Adaptor::drawColorBuffer()
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
    const auto u = location.up_vector;
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

        //Draw the scene.
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
