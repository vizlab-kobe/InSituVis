#include "StochasticRenderingAdaptor_mpi.h"
#include <cfenv>


namespace InSituVis
{

namespace mpi
{

inline void StochasticRenderingAdaptor::RenderingCompositor::firstRenderPass(
    kvs::EnsembleAverageBuffer& buffer )
{
    m_rend_time = 0.0f;
    m_comp_time = 0.0f;
    Compositor::firstRenderPass( buffer );
}

inline void StochasticRenderingAdaptor::RenderingCompositor::ensembleRenderPass(
    kvs::EnsembleAverageBuffer& buffer )
{
    buffer.bind();

    // Rendering
    kvs::Timer timer_rend( kvs::Timer::Start );
    Compositor::drawEngines();
    timer_rend.stop();
    m_rend_time += m_parent->rendTimer().time( timer_rend );

    // Image composition
    kvs::OpenGL::SetReadBuffer( GL_FRONT );
    kvs::OpenGL::SetPixelStorageMode( GL_PACK_ALIGNMENT, GLint(4) );
    const auto width = m_parent->screen().width();
    const auto height = m_parent->screen().height();
    kvs::ValueArray<kvs::UInt8> color_buffer( width * height * 4 );
    kvs::ValueArray<kvs::Real32> depth_buffer( width * height );
    kvs::OpenGL::ReadPixels( 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, color_buffer.data() );
    kvs::OpenGL::ReadPixels( 0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth_buffer.data() );
    {
        kvs::Timer timer_comp( kvs::Timer::Start );
        if ( !m_parent->imageCompositor().run( color_buffer, depth_buffer ) )
        {
            m_parent->log() << "ERROR: " << "Cannot compose images." << std::endl;
        }
        timer_comp.stop();
        m_comp_time += m_parent->compTimer().time( timer_comp );
    }
    kvs::OpenGL::DrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE, color_buffer.data() );
    kvs::OpenGL::DrawPixels( width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth_buffer.data() );

    buffer.unbind();

    // Adding pixels
    timer_rend.start();
    buffer.add();
    timer_rend.stop();
    m_rend_time += m_parent->rendTimer().time( timer_rend );
}

inline StochasticRenderingAdaptor::FrameBuffer StochasticRenderingAdaptor::drawScreen(
    std::function<void(const FrameBuffer&)> func )
{
    m_rendering_compositor.draw();
    BaseClass::setRendTime( BaseClass::rendTime() + m_rendering_compositor.rendTime() );
    BaseClass::setCompTime( BaseClass::compTime() + m_rendering_compositor.compTime() );

    const auto color_buffer = BaseClass::screen().readbackColorBuffer();
    const auto depth_buffer = BaseClass::screen().readbackDepthBuffer();
    func( { color_buffer, depth_buffer } );

    return { color_buffer, depth_buffer };
}

} // end of namespace mpi

} // end of namespace InSituVis
