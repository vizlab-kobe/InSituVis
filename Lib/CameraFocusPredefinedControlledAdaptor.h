// /*****************************************************************************/
// /**
//  *  @file   CameraFocusPredefinedControlledAdaptor.h
//  *  @author Kazuya Adachi, Naohisa Sakamoto
//  */
// /*****************************************************************************/
#pragma once
#if defined( KVS_SUPPORT_MPI )
#include <InSituVis/Lib/Adaptor.h>
#include <list>
#include <queue>


namespace InSituVis
{

class CameraFocusPredefinedControlledAdaptor : public InSituVis::Adaptor
{
public:
    using BaseClass = InSituVis::Adaptor;
    using Viewpoint = InSituVis::Viewpoint;
    using Location = Viewpoint::Location;
    struct BlockEntropy
    {
        float entropy;               
        kvs::Vec3ui region_min;
        kvs::Vec3ui region_max;
        kvs::ValueArray<size_t> hist;
    };
    using BlockEntropyList = std::vector<BlockEntropy>;

private:
    bool m_enable_output_image = true;
    kvs::StampTimer m_pipe_timer{}; ///< timer for pipeline execution process
    kvs::StampTimer m_rend_timer{}; ///< timer for rendering process
    kvs::StampTimer m_save_timer{}; ///< timer for image saving process
    kvs::Vec3 m_focus{ 0.0f, 0.0f, 0.0f };
    BlockEntropyList m_blockentorpy_list{};
    std::vector<kvs::Vec3> m_gradients{};
    size_t block_size;

public:
    CameraFocusPredefinedControlledAdaptor() = default;
    virtual ~CameraFocusPredefinedControlledAdaptor() = default;
    kvs::Vec3 isFocusPoint() const { return m_focus; }
    void setFocusPoint( const kvs::Vec3 focus ) { m_focus = focus; }
    void setBlockSize( size_t size ) { block_size = size; }
    void estimateFocusPoint( const kvs::ValueArray<float>& values ,const kvs::Vec3ui& dims);

protected:
    virtual void execRendering();
    size_t directionIndex( const kvs::Vec3& g );
    kvs::ValueArray<size_t> analyzeRegionDistribution( const kvs::Vec3ui& dims,const kvs::Vec3ui& region_min,const kvs::Vec3ui& region_max );
    float computeEntropy( const kvs::ValueArray<size_t>& histogram );
    void computeBlockEntropies( const kvs::Vec3ui& dims );
    void computeGradients(const kvs::ValueArray<float>& values,const kvs::Vec3ui& dims );

};

} // end of namespace local

#include "CameraFocusPredefinedControlledAdaptor.hpp"

#endif 
