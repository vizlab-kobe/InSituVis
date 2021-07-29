#include "StochasticRenderingAdaptor_mpi.h"


namespace InSituVis
{

namespace mpi
{

inline void StochasticRenderingAdaptor::RenderingCompositor::ensembleRenderingPass(
    kvs::EnsembleAverageBuffer& buffer )
{
    buffer.bind();
    drawEngines();
    buffer.unbind();
    {
        // Image composition
        auto color_buffer = m_parent->screen().readbackColorBuffer();
        auto depth_buffer = m_parent->screen().readbackDepthBuffer();
        m_parent->imageCompositor().run( color_buffer, depth_buffer );

        const auto width = m_parent->imageWidth();
        const auto height = m_parent->imageHeight();
        kvs::OpenGL::DrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE, color_buffer.data() );
        kvs::OpenGL::DrawPixels( width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth_buffer.data() );
    }
//    buffer.unbind();
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
