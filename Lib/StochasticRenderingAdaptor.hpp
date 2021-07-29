
namespace InSituVis
{

inline StochasticRenderingAdaptor::ColorBuffer StochasticRenderingAdaptor::drawScreen()
{
    m_rendering_compositor.draw();
    return BaseClass::screen().readbackColorBuffer();
}

} // end of namespace InSituVis
