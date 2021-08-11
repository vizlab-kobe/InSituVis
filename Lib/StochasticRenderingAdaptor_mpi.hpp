#include "StochasticRenderingAdaptor_mpi.h"
#include <cfenv>


namespace InSituVis
{

namespace mpi
{

inline void StochasticRenderingAdaptor::RenderingCompositor::ensembleRenderPass(
    kvs::EnsembleAverageBuffer& buffer )
{
    buffer.bind();
    drawEngines();

    // Image composition
    kvs::OpenGL::SetReadBuffer( GL_FRONT );
    kvs::OpenGL::SetPixelStorageMode( GL_PACK_ALIGNMENT, GLint(4) );
    const auto width = m_parent->screen().width();
    const auto height = m_parent->screen().height();
    kvs::ValueArray<kvs::UInt8> color_buffer( width * height * 4 );
    kvs::ValueArray<kvs::Real32> depth_buffer( width * height );
    kvs::OpenGL::ReadPixels( 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, color_buffer.data() );
    kvs::OpenGL::ReadPixels( 0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth_buffer.data() );
    m_parent->imageCompositor().run( color_buffer, depth_buffer );
    kvs::OpenGL::DrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE, color_buffer.data() );
    kvs::OpenGL::DrawPixels( width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth_buffer.data() );

    buffer.unbind();
    buffer.add();
}

inline StochasticRenderingAdaptor::FrameBuffer StochasticRenderingAdaptor::drawScreen(
    std::function<void(const FrameBuffer&)> func )
{
    m_rendering_compositor.draw();
    const auto color_buffer = BaseClass::screen().readbackColorBuffer();
    const auto depth_buffer = BaseClass::screen().readbackDepthBuffer();
    return { color_buffer, depth_buffer };
}

} // end of namespace mpi

} // end of namespace InSituVis
