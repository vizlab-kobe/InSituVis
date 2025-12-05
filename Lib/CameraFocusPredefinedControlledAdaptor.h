/*****************************************************************************/
/**
 *  @file   CameraFocusPredefinedControlledAdaptor.h
 *  @author Kazuya Adachi, Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
//#if defined( KVS_SUPPORT_MPI )
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
        kvs::ValueArray<float> hist;
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
    size_t block_size = 10;
    size_t bin_size = 4;
    float alpha_deg = 30.0;
    bool m_is_initial_step = true;
    std::queue<kvs::Vec3> m_focus_path{};
    std::queue<std::pair<float, kvs::Quat>> m_path{};
    std::vector<kvs::Vec3> m_max_positions{}; ///< data queue for m_max_position
    std::vector<kvs::Quat> m_max_rotations{}; ///< data queue for m_max_rotation
    std::vector<kvs::Vec3> m_max_focus_points{};

public:
    CameraFocusPredefinedControlledAdaptor() = default;
    virtual ~CameraFocusPredefinedControlledAdaptor() = default;
    kvs::Vec3 isFocusPoint() const { return m_focus; }
    void setFocusPoint( const kvs::Vec3 focus ) { m_focus = focus; }
    void setBlockSize( size_t size ) { block_size = size; }
    void setBinSize( size_t size ) { bin_size = size; }
    void setAlpha_Deg( float alpha ) { alpha_deg = alpha; }
    void estimateFocusPoint( const kvs::ValueArray<float>& values ,const kvs::Vec3ui& dims);
    void CreatePointObject(const std::vector<BlockEntropy>& blocks);
    float radiusInterpolation( const float r1, const float r2, const float t );
    bool isInitialStep() const { return m_is_initial_step; }
    void setIsInitialStep( const bool is_initial_step ) { m_is_initial_step = is_initial_step; }
    std::vector<kvs::Vec3>& maxPositions() { return m_max_positions; }
    std::vector<kvs::Quat>& maxRotations() { return m_max_rotations; }
    std::vector<kvs::Vec3>& maxFocusPoints() { return m_max_focus_points; }
    void pushMaxPositions( const kvs::Vec3& position ) { m_max_positions.push_back( position ); }
    void pushMaxRotations( const kvs::Quat& rotation )
    {
        if( m_max_rotations.size() > 0 )
        {
            if( rotation.dot( m_max_rotations.back() ) < 0 )
            {
                m_max_rotations.push_back( -rotation );
            }
            else { m_max_rotations.push_back( rotation ); }
        }
        else { m_max_rotations.push_back( rotation ); }
    }
    void pushMaxFocusPoints( const kvs::Vec3& point ) { m_max_focus_points.push_back( point ); }
    void popMaxPositions() { m_max_positions.erase( m_max_positions.begin() ); }
    void popMaxRotations() { m_max_rotations.erase( m_max_rotations.begin() ); }
    void popMaxFocusPoints() { m_max_focus_points.erase( m_max_focus_points.begin() ); }
    

protected:
    virtual void execRendering();
    std::vector<kvs::Vec3> buildSphereDirections(size_t t);
    kvs::ValueArray<float> analyzeRegionDistribution( const kvs::Vec3ui& dims,const kvs::Vec3ui& region_min,const kvs::Vec3ui& region_max );
    float computeEntropy( const kvs::ValueArray<float>& histogram );
    void computeBlockEntropies( const kvs::Vec3ui& dims );
    void computeGradients(const kvs::ValueArray<float>& values,const kvs::Vec3ui& dims );
    std::queue<kvs::Vec3>& focusPath() { return m_focus_path; }
    std::queue<std::pair<float, kvs::Quat>>& path() { return m_path; }
    void createPath();
    std::string outputFinalImageName( const size_t level );

};

} // end of namespace local

#include "CameraFocusPredefinedControlledAdaptor.hpp"

//#endif 
