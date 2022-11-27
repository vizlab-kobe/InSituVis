/*****************************************************************************/
/**
 *  @file   EntropyBasedCameraPathController.h
 *  @author Ken Iwata, Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <queue>
#include <tuple>
#include <functional>
#include <kvs/VolumeObjectBase>
#include <InSituVis/Lib/Adaptor_mpi.h>
#include <InSituVis/Lib/Viewpoint.h>


namespace InSituVis
{

class EntropyBasedCameraPathController
{
public:
    using BaseClass = InSituVis::mpi::Adaptor;
    using Data = InSituVis::Adaptor::ObjectList;
    using DataQueue = std::queue<Data>;

    using Volume = kvs::VolumeObjectBase;
    using Values = Volume::Values;

    using FrameBuffer = InSituVis::mpi::Adaptor::FrameBuffer;
    using EntropyFunction = std::function<float(const FrameBuffer&)>;
    using Interpolator = std::function<kvs::Quat(const kvs::Quat&, const kvs::Quat&, const kvs::Quat&, const kvs::Quat&, float)>;

    // Entropy functions
    static EntropyFunction LightnessEntropy();
    static EntropyFunction ColorEntropy();
    static EntropyFunction DepthEntropy();
    static EntropyFunction MixedEntropy( EntropyFunction e1, EntropyFunction e2, float p );

    // Interpolation functions
    static Interpolator Slerp();
    static Interpolator Squad();

private:
    size_t m_interval = 1; ///< time interval of entropy calculation

    bool m_cache_enabled = true; ///< flag for data caching
    bool m_final_step = false;
    size_t m_max_index = 0;
    float m_max_entropy;
    kvs::Vec3 m_max_position;
    kvs::Quat m_max_rotation;
    float m_erp_radius;
    kvs::Quat m_erp_rotation;
    std::queue<float> m_max_entropies;
    std::queue<kvs::Vec3> m_max_positions;
    std::queue<kvs::Quat> m_max_rotations;
    std::queue<std::tuple<float, kvs::Quat>> m_path;
    std::vector<float> m_path_positions;
    std::vector<float> m_path_entropies;
    DataQueue m_data_queue{}; ///< data queue
    Data m_previous_data{}; ///< dataset at previous time-step
    EntropyFunction m_entropy_function = MixedEntropy( LightnessEntropy(), DepthEntropy(), 0.5f );
    Interpolator m_interpolator = Squad();

public:
    EntropyBasedCameraPathController() = default;
    virtual ~EntropyBasedCameraPathController() = default;

    size_t entropyInterval() const { return m_interval; }
    size_t maxIndex() const { return m_max_index; }
    float maxEntropy() const { return m_max_entropy; }
    kvs::Vec3 maxPosition() const { return m_max_position; }
    kvs::Quaternion maxRotation() const { return m_max_rotation; }
    float erpRadius() const { return m_erp_radius; }
    kvs::Quaternion erpRotation() const { return m_erp_rotation; }

    void setEntropyInterval( const size_t interval ) { m_interval = interval; }
    void setEntropyFunction( EntropyFunction func ) { m_entropy_function = func; }
    void setEntropyFunctionToLightness() { m_entropy_function = LightnessEntropy(); }
    void setEntropyFunctionToColor() { m_entropy_function = ColorEntropy(); }
    void setEntropyFunctionToDepth() { m_entropy_function = DepthEntropy(); }
    void setEntropyFunctionToMixed( EntropyFunction e1, EntropyFunction e2, float p )
    {
        m_entropy_function = MixedEntropy( e1, e2, p );
    }

    void setInterpolator( Interpolator interpolator ) { m_interpolator = interpolator; }
    void setInterpolatorToSlerp() { m_interpolator = Slerp(); }
    void setInterpolatorToSquad() { m_interpolator = Squad(); }

    void setMaxIndex( const size_t index ) { m_max_index = index; }
    void setMaxEntropy( const float entropy ) { m_max_entropy = entropy; }
    void setMaxPosition( const kvs::Vec3& position ) { m_max_position = position; }
    void setMaxRotation( const kvs::Quaternion& rotation ) { m_max_rotation = rotation; }
    void setErpRadius( const float radius ) { m_erp_radius = radius; }
    void setErpRotation( const kvs::Quaternion& rotation ) { m_erp_rotation = rotation; }

    const DataQueue& dataQueue() const { return m_data_queue; }
    const Data& previousData() const { return m_previous_data; }
    const std::vector<float>& pathPositions() const { return m_path_positions; }
    const std::vector<float>& pathEntropies() const { return m_path_entropies; }
    bool isCacheEnabled() const { return m_cache_enabled; }
    void setCacheEnabled( const bool enabled = true ) { m_cache_enabled = enabled; }
    bool isFinalStep() const { return m_final_step; }
    void setFinalStep( const bool final_step = true ) { m_final_step = final_step; }

protected:
    void push( const Data& data );
    virtual void process( const Data& data ) {}
    virtual void process( const Data& data, const float radius, const kvs::Quaternion& rotation ) {}
    virtual float entropy( const FrameBuffer& frame_buffer );
    float radiusInterpolation( const float r1, const float r2, const float t );

    void createPath(
        const float r2,
        const float r3,
        const kvs::Quaternion& q1,
        const kvs::Quaternion& q2,
        const kvs::Quaternion& q3,
        const kvs::Quaternion& q4,
        const size_t point_interval
    );
};

} // end of namespace InSituVis

#include "EntropyBasedCameraPathController.hpp"
