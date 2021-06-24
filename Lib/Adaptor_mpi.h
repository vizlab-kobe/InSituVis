/*****************************************************************************/
/**
 *  @file   Adaptor_mpi.h
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include "Adaptor.h"
#if defined( KVS_SUPPORT_MPI )
#include <kvs/mpi/Communicator>
#include <kvs/mpi/LogStream>
#include <kvs/mpi/ImageCompositor>
#include <kvs/mpi/StampTimer>


namespace InSituVis
{

namespace mpi
{

/*===========================================================================*/
/**
 *  @brief  Adaptor class for parallel rendering based on MPI.
 */
/*===========================================================================*/
class Adaptor : public InSituVis::Adaptor
{
public:
    using BaseClass = InSituVis::Adaptor;
    using DepthBuffer = kvs::ValueArray<kvs::Real32>;
    struct FrameBuffer { ColorBuffer color_buffer; DepthBuffer depth_buffer; };

private:
    kvs::mpi::Communicator m_world{}; ///< MPI communicator
    kvs::mpi::LogStream m_log{ m_world }; ///< MPI log stream
    kvs::mpi::ImageCompositor m_compositor{ m_world }; ///< image compositor
    bool m_enable_output_subimage = false; ///< flag for writing sub-volume rendering image
    bool m_enable_output_subimage_depth = false; ///< flag for writing sub-volume rendering image (depth image)
    bool m_enable_output_subimage_alpha = false; ///< flag for writing sub-volume rendering image (alpha image)
    float m_rend_time = 0.0f; ///< rendering time per frame
    float m_comp_time = 0.0f; ///< image composition time per frame
    kvs::mpi::StampTimer m_comp_timer{ m_world }; ///< timer for image composition process

public:
    Adaptor( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ): m_world( world, root ) {}
    virtual ~Adaptor() = default;

    kvs::mpi::Communicator& world() { return m_world; }
    std::ostream& log() { return m_log( m_world.root() ); }
    std::ostream& log( const int rank ) { return m_log( rank ); }

    void setOutputSubImageEnabled(
        const bool enable = true,
        const bool enable_depth = false,
        const bool enable_alpha = false );

    virtual bool initialize();
    virtual bool finalize();
    virtual void exec( const kvs::UInt32 time_index );
    virtual bool dump();

protected:
    void visualize();
    std::string outputFinalImageName();
    DepthBuffer backgroundDepthBuffer();
    FrameBuffer readback( const Viewpoint::Point& point );

private:
    FrameBuffer readback_plane_buffer( const kvs::Vec3& position );
    FrameBuffer readback_spherical_buffer( const kvs::Vec3& position );
};

} // end of namespace mpi

} // end of namespace InSituVis

#include "Adaptor_mpi.hpp"

#endif // KVS_SUPPORT_MPI
