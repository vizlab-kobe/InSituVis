
namespace InSituVis
{

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
//        screen.draw();
        m_rendering_compositor.update();

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
//        screen.draw();
        m_rendering_compositor.update();

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
