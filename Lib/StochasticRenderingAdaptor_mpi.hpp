#include "StochasticRenderingAdaptor_mpi.h"
#include <cfenv>


namespace InSituVis
{

namespace mpi
{

inline void StochasticRenderingAdaptor::RenderingCompositor::onWindowCreated()
{
//    const auto width = m_parent->screen().width();
//    const auto height = m_parent->screen().height();
//    m_frame_buffer = FrameBuffer( width, height );
//    m_ensemble_buffer = EnsembleBuffer( width * height * 3 );
    kvs::StochasticRenderingCompositor::onWindowCreated();
}

inline void StochasticRenderingAdaptor::RenderingCompositor::firstRenderPass(
    kvs::EnsembleAverageBuffer& buffer )
{
//    m_ensemble_buffer.fill( 0.0f );
//    m_ensembles = 0;
    kvs::StochasticRenderingCompositor::firstRenderPass( buffer );
}

inline void StochasticRenderingAdaptor::RenderingCompositor::ensembleRenderPass(
    kvs::EnsembleAverageBuffer& buffer )
{
    buffer.bind();
    drawEngines();
//   buffer.unbind();
/*
    // Image composition
    auto color_buffer = m_parent->screen().readbackColorBuffer();
    auto depth_buffer = m_parent->screen().readbackDepthBuffer();
    m_parent->imageCompositor().run( color_buffer, depth_buffer );

    // Ensemble averaging
    const auto a = 1.0f / ( m_ensembles + 1 );
    const auto npixels = m_frame_buffer.depth_buffer.size();
    for ( size_t i = 0; i < npixels; ++i )
    {
        const auto r = kvs::Real32( color_buffer[ 4 * i + 0 ] );
        const auto g = kvs::Real32( color_buffer[ 4 * i + 1 ] );
        const auto b = kvs::Real32( color_buffer[ 4 * i + 2 ] );
        m_ensemble_buffer[ 3 * i + 0 ] = kvs::Math::Mix( m_ensemble_buffer[ 3 * i + 0 ], r, a );
        m_ensemble_buffer[ 3 * i + 1 ] = kvs::Math::Mix( m_ensemble_buffer[ 3 * i + 1 ], g, a );
        m_ensemble_buffer[ 3 * i + 2 ] = kvs::Math::Mix( m_ensemble_buffer[ 3 * i + 2 ], b, a );
    }

    ++m_ensembles;
*/

    // Image composition
    auto color_buffer = m_parent->screen().readbackColorBuffer();
    auto depth_buffer = m_parent->screen().readbackDepthBuffer();
    m_parent->imageCompositor().run( color_buffer, depth_buffer );

    const auto width = m_parent->screen().width();
    const auto height = m_parent->screen().height();
    kvs::OpenGL::DrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE, color_buffer.data() );
    kvs::OpenGL::DrawPixels( width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth_buffer.data() );

   buffer.unbind();

//   fenv_t fe;
//   std::feholdexcept( &fe );
   buffer.add();
//   std::feupdateenv( &fe );
//   std::cout << "bb (" << m_parent->world().rank() << ")" << std::endl;

//    kvs::StochasticRenderingCompositor::ensembleRenderPass( buffer );
}

inline void StochasticRenderingAdaptor::RenderingCompositor::lastRenderPass(
    kvs::EnsembleAverageBuffer& buffer )
{
    /*
    auto& color_buffer = m_frame_buffer.color_buffer;
    const auto npixels = m_frame_buffer.depth_buffer.size();
    for ( size_t i = 0; i < npixels; ++i )
    {
        const int r = kvs::Math::Round( m_ensemble_buffer[ 3 * i + 0 ] );
        const int g = kvs::Math::Round( m_ensemble_buffer[ 3 * i + 1 ] );
        const int b = kvs::Math::Round( m_ensemble_buffer[ 3 * i + 2 ] );
        color_buffer[ 4 * i + 0 ] = kvs::Math::Clamp( r, 0 , 255 );
        color_buffer[ 4 * i + 1 ] = kvs::Math::Clamp( g, 0 , 255 );
        color_buffer[ 4 * i + 2 ] = kvs::Math::Clamp( b, 0 , 255 );
        color_buffer[ 4 * i + 3 ] = 255;
    }
    */

    kvs::StochasticRenderingCompositor::lastRenderPass( buffer );
}

inline StochasticRenderingAdaptor::FrameBuffer StochasticRenderingAdaptor::drawScreen(
    std::function<void(const FrameBuffer&)> func )
{
    m_rendering_compositor.draw();
//    return m_rendering_compositor.frameBuffer();

    const auto color_buffer = BaseClass::screen().readbackColorBuffer();
    const auto depth_buffer = BaseClass::screen().readbackDepthBuffer();
    return { color_buffer, depth_buffer };
}

} // end of namespace mpi

} // end of namespace InSituVis
