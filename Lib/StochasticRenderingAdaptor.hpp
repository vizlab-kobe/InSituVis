
namespace InSituVis
{

inline void StochasticRenderingAdaptor::exec( const SimTime sim_time )
{
    if ( BaseClass::isAnalysisStep() )
    {
        const auto step = static_cast<float>( BaseClass::timeStep() );
        BaseClass::tstepList().stamp( step );

        BaseClass::execPipeline( BaseClass::objects() );
        this->execRendering();
    }

    BaseClass::incrementTimeStep();
    BaseClass::clearObjects();
}

inline void StochasticRenderingAdaptor::execRendering()
{
    float rend_time = 0.0f;
    float save_time = 0.0f;
    {
        kvs::Timer timer_rend;
        kvs::Timer timer_save;
        for ( const auto& location : BaseClass::viewpoint().locations() )
        {
            // Draw and readback framebuffer
            timer_rend.start();
            auto color_buffer = this->readback( location );
            timer_rend.stop();
            rend_time += BaseClass::rendTimer().time( timer_rend );

            // Output framebuffer to image file
            timer_save.start();
            if ( BaseClass::isOutputImageEnabled() )
            {
                const auto size = BaseClass::outputImageSize( location );
                const auto width = size.x();
                const auto height = size.y();
                kvs::ColorImage image( width, height, color_buffer );
                image.write( BaseClass::outputImageName( location ) );
            }
            timer_save.stop();
            save_time += BaseClass::saveTimer().time( timer_save );
        }
    }
    BaseClass::rendTimer().stamp( rend_time );
    BaseClass::saveTimer().stamp( save_time );
}

inline StochasticRenderingAdaptor::ColorBuffer StochasticRenderingAdaptor::readback( const Viewpoint::Location& location )
{
    switch ( location.direction )
    {
    case Viewpoint::Direction::Uni: return this->readback_uni_buffer( location );
    case Viewpoint::Direction::Omni: return this->readback_omn_buffer( location );
    case Viewpoint::Direction::Adaptive: return this->readback_adp_buffer( location );
    default: return BaseClass::backgroundColorBuffer();
    }
}

inline StochasticRenderingAdaptor::ColorBuffer StochasticRenderingAdaptor::readback_uni_buffer( const Viewpoint::Location& location )
{
    const auto p = location.position;
    const auto a = location.look_at;
    if ( p == a )
    {
        return this->backgroundColorBuffer();
    }
    else
    {
        auto& screen = BaseClass::screen();
        auto* camera = screen.scene()->camera();
        auto* light = screen.scene()->light();

        // Backup camera and light info.
        const auto p0 = camera->position();
        const auto a0 = camera->lookAt();
        const auto u0 = camera->upVector();

        // Draw the scene.
        const auto zero = kvs::Vec3::Zero();
        const auto pa = a - p;
        const auto rr = pa.cross( u0 );
        const auto r = rr == zero ? ( a0 - p0 ).cross( u0 ) : rr;
        const auto u = r.cross( pa );
        camera->setPosition( p, a, u );
        light->setPosition( p );
        m_rendering_compositor.draw();

        // Restore camera and light info.
        camera->setPosition( p0, a0, u0 );
        light->setPosition( p0 );

        return screen.readbackColorBuffer();
    }
}

inline StochasticRenderingAdaptor::ColorBuffer StochasticRenderingAdaptor::readback_omn_buffer( const Viewpoint::Location& location )
{
    using SphericalColorBuffer = InSituVis::SphericalBuffer<kvs::UInt8>;

    auto& screen = BaseClass::screen();
    auto* camera = screen.scene()->camera();
    auto* light = screen.scene()->light();

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

    SphericalColorBuffer color_buffer( screen.width(), screen.height() );
    for ( size_t i = 0; i < SphericalColorBuffer::Direction::NumberOfDirections; i++ )
    {
        const auto d = SphericalColorBuffer::Direction(i);
        const auto dir = SphericalColorBuffer::DirectionVector(d);
        const auto up = SphericalColorBuffer::UpVector(d);
        camera->setPosition( p, p + dir, up );
        m_rendering_compositor.draw();

        const auto buffer = screen.readbackColorBuffer();
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

inline StochasticRenderingAdaptor::ColorBuffer StochasticRenderingAdaptor::readback_adp_buffer( const Viewpoint::Location& location )
{
    auto& screen = BaseClass::screen();
    const auto* object = screen.scene()->objectManager();
    return this->isInsideObject( location.position, object ) ?
        this->readback_omn_buffer( location ) :
        this->readback_uni_buffer( location );
}

} // end of namespace InSituVis
