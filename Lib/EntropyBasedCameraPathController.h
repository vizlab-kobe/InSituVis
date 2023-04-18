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
#include <InSituVis/Lib/OutputDirectory.h>


namespace InSituVis
{

class EntropyBasedCameraPathController
{
public:
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
    bool m_final_step = false; ///< flag for checking whether the current step is final step
    size_t m_max_index = 0; ///< index of the estimated camera at the evaluation step
    float m_max_entropy = 0.0f; ///< entropy at the estimated camera
    kvs::Vec3 m_max_position{ 0.0f, 12.0f, 0.0f }; ///< position of the estimated camera
    kvs::Quat m_max_rotation{ 0.0f, 0.0f, 0.0f, 1.0f }; ///< rotation of the estimated camera
    float m_erp_radius = 12.0f; ///< distance between the intepolated camera position and the orign
    kvs::Quat m_erp_rotation{ 0.0f, 0.0f, 0.0f, 1.0f }; ///< rotation at the interpolated camera position
    std::queue<float> m_max_entropies{}; ///< data queue for m_max_entropy
    std::queue<kvs::Vec3> m_max_positions{}; ///< data queue for m_max_position
    std::queue<kvs::Quat> m_max_rotations{}; ///< data queue for m_max_rotation
    std::queue<std::tuple<float, kvs::Quat>> m_path{}; ///< {radius,rotation} on the interpolated path
    std::vector<float> m_path_positions{}; ///< positions on the interpolated path
    std::vector<float> m_path_entropies{}; ///< entropies on the interpolated path
    std::vector<float> m_path_calc_times{}; ///< path calculation times
    DataQueue m_data_queue{}; ///< data queue
    Data m_previous_data{}; ///< dataset at previous time-step
    EntropyFunction m_entropy_function = MixedEntropy( LightnessEntropy(), DepthEntropy(), 0.5f ); ///< entropy function
    Interpolator m_interpolator = Squad(); ///< path interpolator
    bool m_enable_output_evaluation_image = false; ///< if true, all of evaluation images will be output
    bool m_enable_output_evaluation_image_depth = false; ///< if true, all of evaluation depth images will be output
    bool m_enable_output_entropies = false; ///< if true, calculted entropies for all viewpoints will be output

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
    const std::vector<float>& pathCalcTimes() const { return m_path_calc_times; }
    bool isCacheEnabled() const { return m_cache_enabled; }
    void setCacheEnabled( const bool enabled = true ) { m_cache_enabled = enabled; }
    bool isFinalStep() const { return m_final_step; }
    void setFinalStep( const bool final_step = true ) { m_final_step = final_step; }

    void setOutputEvaluationImageEnabled( const bool enable = true, const bool enable_depth = false );
    void setOutputEntropiesEnabled( const bool enable = true ) { m_enable_output_entropies = enable; }

    bool isOutputEvaluationImageEnabled() const { return m_enable_output_evaluation_image; }
    bool isOutputEvaluationDepathImageEnabled() const { return m_enable_output_evaluation_image_depth; }
    bool isOutputEntropiesEnabled() const { return m_enable_output_entropies; }

protected:
    void setPreviousData( const Data& data ) { m_previous_data = data; }
    DataQueue& dataQueue() { return m_data_queue; }
    std::queue<float>& maxEntropies() { return m_max_entropies; }
    std::queue<kvs::Vec3>& maxPositions() { return m_max_positions; }
    std::queue<kvs::Quat>& maxRotations() { return m_max_rotations; }
    std::queue<std::tuple<float, kvs::Quat>>& path() { return m_path; }
    std::vector<float>& pathEntropies() { return m_path_entropies; }
    std::vector<float>& pathPositions() { return m_path_positions; }
    std::vector<float>& pathCalcTimes() { return m_path_calc_times; }

    void pushMaxEntropies( const float entropy ) { m_max_entropies.push( entropy ); }
    void pushMaxPositions( const kvs::Vec3& position ) { m_max_positions.push( position ); }
    void pushMaxRotations( const kvs::Quat& rotation ) { m_max_rotations.push( rotation ); }

    void pushPathEntropies( const float entropy ) { m_path_entropies.push_back( entropy ); }
    void pushPathPositions( const kvs::Vec3& position )
    {
        m_path_positions.push_back( position.x() );
        m_path_positions.push_back( position.y() );
        m_path_positions.push_back( position.z() );
    }

    virtual void process( const Data& data ) {}
    virtual void process( const Data& data, const float radius, const kvs::Quat& rotation ) {}
    virtual float entropy( const FrameBuffer& frame_buffer );
    float radiusInterpolation( const float r1, const float r2, const float t );
    kvs::Quat pathInterpolation( const kvs::Quat& q1, const kvs::Quat& q2, const kvs::Quat& q3, const kvs::Quat& q4, const float t );

    void push( const Data& data );
    void createPath(
        const float r2,
        const float r3,
        const kvs::Quat& q1,
        const kvs::Quat& q2,
        const kvs::Quat& q3,
        const kvs::Quat& q4,
        const size_t point_interval
    );

    std::string logDataFilename(
        const std::string& basename,
        const InSituVis::OutputDirectory& directory );

    std::string logDataFilename(
        const std::string& basename,
        const kvs::UInt32 timestep,
        const InSituVis::OutputDirectory& directory );

    void outputEntropies(
        const std::string& filename,
        const std::vector<float>& entropies );

    void outputPathEntropies(
        const std::string& filename,
        const size_t analysis_interval );

    void outputPathPositions(
        const std::string& filename,
        const size_t analysis_interval );

    void outputPathCalcTimes(
        const std::string& filename );

    void outputViewpointCoords(
        const std::string& filename,
        const InSituVis::Viewpoint& viewpoint );
};

} // end of namespace InSituVis

#include "EntropyBasedCameraPathController.hpp"
