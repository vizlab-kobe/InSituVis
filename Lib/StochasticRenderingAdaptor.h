/*****************************************************************************/
/**
 *  @file   StochasticRenderingAdaptor.h
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include "Adaptor.h"
#include <kvs/StochasticRenderingCompositor>


namespace InSituVis
{

class StochasticRenderingAdaptor : public InSituVis::Adaptor
{
public:
    using BaseClass = InSituVis::Adaptor;
    using ColorBuffer = InSituVis::Adaptor::ColorBuffer;

private:
    kvs::StochasticRenderingCompositor m_rendering_compositor{ BaseClass::screen().scene() };

public:
    StochasticRenderingAdaptor() { BaseClass::screen().setEvent( &m_rendering_compositor ); }
    virtual ~StochasticRenderingAdaptor() = default;

    void setRepetitionLevel( const size_t level )
    {
        m_rendering_compositor.setRepetitionLevel( level );
    }

    size_t repetitionLevel() const
    {
        return m_rendering_compositor.repetitionLevel();
    }

    virtual void exec( const SimTime sim_time = {} );

private:
    void execRendering();
    ColorBuffer readback( const Viewpoint::Location& location );
    ColorBuffer readback_uni_buffer( const Viewpoint::Location& location );
    ColorBuffer readback_omn_buffer( const Viewpoint::Location& location );
    ColorBuffer readback_adp_buffer( const Viewpoint::Location& location );
};

} // end of namespace InSituVis

#include "StochasticRenderingAdaptor.hpp"
