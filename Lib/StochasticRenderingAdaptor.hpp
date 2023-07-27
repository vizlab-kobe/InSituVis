
namespace InSituVis
{

inline StochasticRenderingAdaptor::ColorBuffer StochasticRenderingAdaptor::drawScreen()
//inline StochasticRenderingAdaptor::ColorBuffer StochasticRenderingAdaptor::drawColorBuffer()
{
    m_rendering_compositor.draw();
    return BaseClass::screen().readbackColorBuffer();
}

} // end of namespace InSituVis
