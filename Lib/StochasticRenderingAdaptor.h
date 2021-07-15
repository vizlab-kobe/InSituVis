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

private:
    ColorBuffer readback_uni_buffer( const Viewpoint::Location& location );
    ColorBuffer readback_omn_buffer( const Viewpoint::Location& location );
    ColorBuffer readback_adp_buffer( const Viewpoint::Location& location );
};

} // end of namespace InSituVis

#include "StochasticRenderingAdaptor.hpp"
