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
    using RenderingCompositor = kvs::StochasticRenderingCompositor;

private:
    RenderingCompositor m_rendering_compositor{ BaseClass::screen().scene() };

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

private:
    virtual ColorBuffer drawScreen();
};

} // end of namespace InSituVis

#include "StochasticRenderingAdaptor.hpp"
#include "StochasticRenderingAdaptor_mpi.h"
