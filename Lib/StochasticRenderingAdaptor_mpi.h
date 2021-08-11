#pragma once
#if defined( KVS_SUPPORT_MPI )
#include "Adaptor_mpi.h"
#include <kvs/StochasticRenderingCompositor>


namespace InSituVis
{

namespace mpi
{

class StochasticRenderingAdaptor : public InSituVis::mpi::Adaptor
{
public:
    using BaseClass = InSituVis::mpi::Adaptor;
    using FrameBuffer = BaseClass::FrameBuffer;

    class RenderingCompositor : public kvs::StochasticRenderingCompositor
    {
        using Adaptor = StochasticRenderingAdaptor;
    private:
        Adaptor* m_parent = nullptr;
    public:
        RenderingCompositor( kvs::Scene* scene, Adaptor* parent ):
            kvs::StochasticRenderingCompositor( scene ),
            m_parent( parent ) {}
        void ensembleRenderPass( kvs::EnsembleAverageBuffer& buffer );
    };

private:
    RenderingCompositor m_rendering_compositor{ BaseClass::screen().scene(), this };

public:
    StochasticRenderingAdaptor( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ):
        BaseClass( world, root )
    {
        BaseClass::screen().setEvent( &m_rendering_compositor );
    }
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
    virtual FrameBuffer drawScreen( std::function<void(const FrameBuffer&)> func = [] ( const FrameBuffer& ) {} );
};

} // end of namespace mpi

} // end of namespace InSituVis

#include "StochasticRenderingAdaptor_mpi.hpp"

#endif // KVS_SUPPORT_MPI
