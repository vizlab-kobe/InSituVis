namespace InSituVis
{

namespace mpi
{

inline void Adaptor::setOutputSubImageEnabled(
    const bool enable,
    const bool enable_depth,
    const bool enable_alpha )
{
    m_enable_output_subimage = enable;
    m_enable_output_subimage_depth = enable_depth;
    m_enable_output_subimage_alpha = enable_alpha;
}

inline bool Adaptor::initialize()
{
    if ( !BaseClass::outputDirectory().create( m_world ) )
    {
        this->log() << "ERROR: " << "Cannot create output directories." << std::endl;
        return false;
    }

    const bool depth_testing = true;
    const auto width = BaseClass::imageWidth();
    const auto height = BaseClass::imageHeight();
    if ( !m_compositor.initialize( width, height, depth_testing ) )
    {
        this->log() << "ERROR: " << "Cannot initialize image compositor." << std::endl;
        return false;
    }

    BaseClass::screen().setSize( width, height );
    BaseClass::screen().create();

    return true;
}

inline bool Adaptor::finalize()
{
    if ( m_compositor.destroy() )
    {
        return BaseClass::finalize();
    }
    return false;
}

inline void Adaptor::exec( const BaseClass::SimTime sim_time )
{
    // Visualize the processed object.
    if ( this->canVisualize() )
    {
        // Stack current time index.
        const auto index = static_cast<float>( BaseClass::timeIndex() );
        BaseClass::indexList().stamp( index );

        this->execPipeline();
        this->execRendering();
    }

    const auto index = BaseClass::timeIndex();
    BaseClass::setTimeIndex( index + 1 );
    BaseClass::clearObjects();
}

inline bool Adaptor::dump()
{
    auto& index_list = BaseClass::indexList();
    auto& pipe_timer = BaseClass::pipeTimer();
    auto& rend_timer = BaseClass::rendTimer();
    auto& save_timer = BaseClass::saveTimer();
    auto& comp_timer = m_comp_timer;
    if ( index_list.title().empty() ) { index_list.setTitle( "Time index" ); }
    if ( pipe_timer.title().empty() ) { pipe_timer.setTitle( "Pipe time" ); }
    if ( rend_timer.title().empty() ) { rend_timer.setTitle( "Rend time" ); }
    if ( save_timer.title().empty() ) { save_timer.setTitle( "Save time" ); }
    if ( comp_timer.title().empty() ) { comp_timer.setTitle( "Comp time" ); }

    const std::string rank = kvs::String::From( this->world().rank(), 4, '0' );
    const std::string subdir = BaseClass::outputDirectory().name() + "/";
    kvs::StampTimerList timer_list;
    timer_list.push( index_list );
    timer_list.push( pipe_timer );
    timer_list.push( rend_timer );
    timer_list.push( save_timer );
    timer_list.push( comp_timer );
    if ( !timer_list.write( subdir + "vis_proc_time_" + rank + ".csv" ) ) return false;

    using Time = kvs::mpi::StampTimer;
    Time pipe_time_min( this->world(), pipe_timer ); pipe_time_min.reduceMin();
    Time pipe_time_max( this->world(), pipe_timer ); pipe_time_max.reduceMax();
    Time pipe_time_ave( this->world(), pipe_timer ); pipe_time_ave.reduceAve();
    Time rend_time_min( this->world(), rend_timer ); rend_time_min.reduceMin();
    Time rend_time_max( this->world(), rend_timer ); rend_time_max.reduceMax();
    Time rend_time_ave( this->world(), rend_timer ); rend_time_ave.reduceAve();
    Time save_time_min( this->world(), save_timer ); save_time_min.reduceMin();
    Time save_time_max( this->world(), save_timer ); save_time_max.reduceMax();
    Time save_time_ave( this->world(), save_timer ); save_time_ave.reduceAve();
    Time comp_time_min( this->world(), comp_timer ); comp_time_min.reduceMin();
    Time comp_time_max( this->world(), comp_timer ); comp_time_max.reduceMax();
    Time comp_time_ave( this->world(), comp_timer ); comp_time_ave.reduceAve();

    if ( !this->world().isRoot() ) return true;

    pipe_time_min.setTitle( pipe_timer.title() + " (min)" );
    pipe_time_max.setTitle( pipe_timer.title() + " (max)" );
    pipe_time_ave.setTitle( pipe_timer.title() + " (ave)" );
    rend_time_min.setTitle( rend_timer.title() + " (min)" );
    rend_time_max.setTitle( rend_timer.title() + " (max)" );
    rend_time_ave.setTitle( rend_timer.title() + " (ave)" );
    save_time_min.setTitle( save_timer.title() + " (min)" );
    save_time_max.setTitle( save_timer.title() + " (max)" );
    save_time_ave.setTitle( save_timer.title() + " (ave)" );
    comp_time_min.setTitle( comp_timer.title() + " (min)" );
    comp_time_max.setTitle( comp_timer.title() + " (max)" );
    comp_time_ave.setTitle( comp_timer.title() + " (ave)" );

    timer_list.clear();
    timer_list.push( index_list );
    timer_list.push( pipe_time_min );
    timer_list.push( pipe_time_max );
    timer_list.push( pipe_time_ave );
    timer_list.push( rend_time_min );
    timer_list.push( rend_time_max );
    timer_list.push( rend_time_ave );
    timer_list.push( save_time_min );
    timer_list.push( save_time_max );
    timer_list.push( save_time_ave );
    timer_list.push( comp_time_min );
    timer_list.push( comp_time_max );
    timer_list.push( comp_time_ave );

    const auto basedir = BaseClass::outputDirectory().baseDirectoryName() + "/";
    return timer_list.write( basedir + "vis_proc_time.csv" );
}

inline void Adaptor::execRendering()
{
    m_rend_time = 0.0f;
    m_comp_time = 0.0f;
    float save_time = 0.0f;
    {
        const auto npoints = BaseClass::viewpoint().numberOfLocations();
        for ( size_t i = 0; i < npoints; ++i )
        {
            // Draw and readback framebuffer
            const auto& location = BaseClass::viewpoint().at( i );
            auto frame_buffer = this->readback( location );

            // Output framebuffer to image file at the root node
            kvs::Timer timer( kvs::Timer::Start );
            if ( m_world.rank() == m_world.root() )
            {
                if ( BaseClass::isOutputImageEnabled() )
                {
                    const auto image_size = BaseClass::outputImageSize( location );
                    kvs::ColorImage image( image_size.x(), image_size.y(), frame_buffer.color_buffer );
                    image.write( this->outputFinalImageName( location ) );
                }
            }
            timer.stop();
            save_time += BaseClass::saveTimer().time( timer );
        }
    }
    BaseClass::saveTimer().stamp( save_time );
    BaseClass::rendTimer().stamp( m_rend_time );
    m_comp_timer.stamp( m_comp_time );
}

inline std::string Adaptor::outputFinalImageName( const Viewpoint::Location& location )
{
    const auto time = BaseClass::timeIndex();
    const auto space = location.index;
    const auto output_time = kvs::String::From( time, 6, '0' );
    const auto output_space = kvs::String::From( space, 6, '0' );

    const auto output_basename = BaseClass::outputFilename();
    const auto output_filename = output_basename + "_" + output_time + "_" + output_space;
    const auto filename = BaseClass::outputDirectory().baseDirectoryName() + "/" + output_filename + ".bmp";
    return filename;
}

inline Adaptor::DepthBuffer Adaptor::backgroundDepthBuffer()
{
    const auto width = BaseClass::screen().width();
    const auto height = BaseClass::screen().height();
    DepthBuffer buffer( width * height );
    buffer.fill(0);
    return buffer;
}

inline Adaptor::FrameBuffer Adaptor::readback( const Viewpoint::Location& location )
{
    FrameBuffer frame_buffer;

    switch ( location.direction )
    {
    case Viewpoint::Direction::Uni:
    {
        frame_buffer = this->readback_plane_buffer( location );
        break;
    }
    case Viewpoint::Direction::Omni:
    {
        frame_buffer = this->readback_spherical_buffer( location );
        break;
    }
    case Viewpoint::Direction::Adaptive:
    {
        const auto* object = BaseClass::screen().scene()->objectManager();
        frame_buffer = BaseClass::isInsideObject( location.position, object ) ?
            this->readback_spherical_buffer( location ) :
            this->readback_plane_buffer( location );
        break;
    }
    default:
    {
        frame_buffer.color_buffer = BaseClass::backgroundColorBuffer();
        frame_buffer.depth_buffer = this->backgroundDepthBuffer();
        break;
    }
    }

    return frame_buffer;
}

inline Adaptor::FrameBuffer Adaptor::readback_plane_buffer( const Viewpoint::Location& location )
{
    FrameBuffer frame_buffer;

    kvs::Timer timer_rend;
    kvs::Timer timer_comp;

    const auto* camera = BaseClass::screen().scene()->camera();
    const auto* light = BaseClass::screen().scene()->light();

    const auto position = location.position;
    const auto lookat = camera->lookAt();
    if ( lookat == position )
    {
        timer_rend.start();
        frame_buffer.color_buffer = BaseClass::backgroundColorBuffer();
        frame_buffer.depth_buffer = this->backgroundDepthBuffer();
        timer_rend.stop();
        m_rend_time += BaseClass::rendTimer().time( timer_rend );
    }
    else
    {
        timer_rend.start();

        // Backup camera and light info.
        const auto cp = camera->position();
        const auto cu = camera->upVector();
        const auto lp = light->position();
        {
            // Draw image
            //
            // BUGS INCLUDED
            // {
            const auto p0 = ( camera->position() - lookat ).normalized();
            const auto p1 = ( position - lookat ).normalized();
            const auto axis = p0.cross( p1 );
            const auto deg = kvs::Math::Rad2Deg( std::acos( p0.dot( p1 ) ) );
            const auto R = kvs::RotationMatrix33<float>( axis, deg );
            const auto up = camera->upVector() * R;
            // }
            //
            BaseClass::screen().scene()->camera()->setPosition( position, lookat, up );
            BaseClass::screen().scene()->light()->setPosition( position );
            BaseClass::screen().draw();
        }

        // Read-back image
        auto color_buffer = BaseClass::screen().readbackColorBuffer();
        auto depth_buffer = BaseClass::screen().readbackDepthBuffer();

        // Restore camera and light info.
//            BaseClass::screen().scene()->camera()->setPosition( cp, lookat, cu );
//            BaseClass::screen().scene()->light()->setPosition( lp );

        timer_rend.stop();
        m_rend_time += BaseClass::rendTimer().time( timer_rend );

        // Output rendering image (partial rendering image)
        if ( m_enable_output_subimage )
        {
            // RGB image
            const auto width = BaseClass::imageWidth();
            const auto height = BaseClass::imageHeight();
            kvs::ColorImage image( width, height, color_buffer );
            image.write( BaseClass::outputImageName( location, "_color") );

            // Depth image
            if ( m_enable_output_subimage_depth )
            {
                kvs::GrayImage depth_image( width, height, depth_buffer );
                depth_image.write( BaseClass::outputImageName( location, "_depth" ) );
            }

            // Alpha image
            if ( m_enable_output_subimage_alpha )
            {
                kvs::GrayImage alpha_image( width, height, color_buffer, 3 );
                alpha_image.write( BaseClass::outputImageName( location, "_alpha" ) );
            }
        }

        // Image composition
        timer_comp.start();
        if ( !m_compositor.run( color_buffer, depth_buffer ) )
        {
            this->log() << "ERROR: " << "Cannot compose images." << std::endl;
        }
        timer_comp.stop();
        m_comp_time += m_comp_timer.time( timer_comp );

        frame_buffer.color_buffer = color_buffer;
        frame_buffer.depth_buffer = depth_buffer;
    }

    return frame_buffer;
}

inline Adaptor::FrameBuffer Adaptor::readback_spherical_buffer( const Viewpoint::Location& location )
{
    using SphericalColorBuffer = InSituVis::SphericalBuffer<kvs::UInt8>;
    using SphericalDepthBuffer = InSituVis::SphericalBuffer<kvs::Real32>;

    kvs::Timer timer_rend;
    kvs::Timer timer_comp;

    // Color and depth buffers.
    SphericalColorBuffer color_buffer( BaseClass::screen().width(), BaseClass::screen().height() );
    SphericalDepthBuffer depth_buffer( BaseClass::screen().width(), BaseClass::screen().height() );

    // Backup camera and light info.
    const auto fov = BaseClass::screen().scene()->camera()->fieldOfView();
    const auto front = BaseClass::screen().scene()->camera()->front();
//        const auto pc = BaseClass::screen().scene()->camera()->position();
//        const auto pl = BaseClass::screen().scene()->light()->position();
    const auto cp = BaseClass::screen().scene()->camera()->position();
    const auto cl = BaseClass::screen().scene()->camera()->lookAt();
    const auto cu = BaseClass::screen().scene()->camera()->upVector();
    const auto lp = BaseClass::screen().scene()->light()->position();
    {
        // Draw images.
        BaseClass::screen().scene()->light()->setPosition( location.position );
        BaseClass::screen().scene()->camera()->setFieldOfView( 90.0 );
        BaseClass::screen().scene()->camera()->setFront( 0.1 );

        float rend_time = 0.0f;
        float comp_time = 0.0f;

        const auto& p = location.position;
        for ( size_t i = 0; i < SphericalColorBuffer::Direction::NumberOfDirections; i++ )
        {
            timer_rend.start();

            const auto d = SphericalColorBuffer::Direction(i);
            const auto dir = SphericalColorBuffer::DirectionVector(d);
            const auto up = SphericalColorBuffer::UpVector(d);
            BaseClass::screen().scene()->camera()->setPosition( p, p + dir, up );
            BaseClass::screen().draw();

            auto cbuffer = BaseClass::screen().readbackColorBuffer();
            auto dbuffer = BaseClass::screen().readbackDepthBuffer();

            timer_rend.stop();
            rend_time += BaseClass::rendTimer().time( timer_rend );

            // Output rendering image (partial rendering image) for each direction
            if ( m_enable_output_subimage )
            {
                // RGB image
                const auto dname = SphericalColorBuffer::DirectionName(d);
                const auto width = BaseClass::imageWidth();
                const auto height = BaseClass::imageHeight();
                kvs::ColorImage image( width, height, cbuffer );
                image.write( BaseClass::outputImageName( location, "_color_" + dname ) );

                // Depth image
                if ( m_enable_output_subimage_depth )
                {
                    kvs::GrayImage depth_image( width, height, dbuffer );
                    depth_image.write( BaseClass::outputImageName( location, "_depth_" + dname ) );
                }

                // Alpha image
                if ( m_enable_output_subimage_alpha )
                {
                    kvs::GrayImage alpha_image( width, height, cbuffer, 3 );
                    alpha_image.write( BaseClass::outputImageName( location, "_alpha_" + dname ) );
                }
            }

            // Image composition
            timer_comp.start();
            if ( !m_compositor.run( cbuffer, dbuffer ) )
            {
                this->log() << "ERROR: " << "Cannot compose images." << std::endl;
            }
            timer_comp.stop();
            comp_time += m_comp_timer.time( timer_comp );

            color_buffer.setBuffer( d, cbuffer );
            depth_buffer.setBuffer( d, dbuffer );
        }

        m_rend_time = rend_time;
        m_comp_time = comp_time;
    }
    // Restore camera and light info.
    BaseClass::screen().scene()->camera()->setFieldOfView( fov );
    BaseClass::screen().scene()->camera()->setFront( front );
    BaseClass::screen().scene()->camera()->setPosition( cp, cl, cu );
    BaseClass::screen().scene()->light()->setPosition( lp );

    // Return frame buffer
    FrameBuffer frame_buffer;
    frame_buffer.color_buffer = color_buffer.stitch<4>();
    frame_buffer.depth_buffer = depth_buffer.stitch<1>();
    return frame_buffer;
}

} // end of namespace mpi

} // end of namespace InSituVis
