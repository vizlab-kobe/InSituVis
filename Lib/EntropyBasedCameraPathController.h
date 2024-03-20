/*****************************************************************************/
/**
 *  @file   EntropyBasedCameraPathController.h
 *  @author Ken Iwata, Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <queue>
#include <utility>
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
    using Interpolator = std::function<kvs::Quat(const std::vector<kvs::Quat>&, float)>;

    enum InterpolationMethod { SLERP, SQUAD };

    // Entropy functions
    static EntropyFunction LightnessEntropy();
    static EntropyFunction ColorEntropy();
    static EntropyFunction DepthEntropy();
    static EntropyFunction MixedEntropy( EntropyFunction e1, EntropyFunction e2, float p );

    // Interpolation functions
    static Interpolator Slerp();
    static Interpolator Squad();

private:
    size_t m_cache_size = 0;
    float m_delta = 1.0f;
    size_t m_entropy_interval = 1; ///< time interval of entropy calculation
    bool m_cache_enabled = true; ///< flag for data caching
    bool m_is_initial_step = true; ///< flag for checking whether the current step is initial step
    bool m_is_final_step = false; ///< flag for checking whether the current step is final step
    bool m_is_ent_step = false;
    bool m_is_erp_step = false;
    size_t m_max_index = 0; ///< index of the estimated camera at the evaluation step
    float m_max_entropy = 0.0f; ///< entropy at the estimated camera
    kvs::Vec3 m_max_position{ 0.0f, 12.0f, 0.0f }; ///< position of the estimated camera
    kvs::Quat m_max_rotation{ 0.0f, 0.0f, 0.0f, 1.0f }; ///< rotation of the estimated camera
    float m_erp_radius = 12.0f; ///< distance between the intepolated camera position and the orign
    kvs::Quat m_erp_rotation{ 0.0f, 0.0f, 0.0f, 1.0f }; ///< rotation at the interpolated camera position
    size_t m_sub_time_index = 0;
    std::vector<float> m_max_entropies{}; ///< data queue for m_max_entropy
    std::vector<kvs::Vec3> m_max_positions{}; ///< data queue for m_max_position
    std::vector<kvs::Quat> m_max_rotations{}; ///< data queue for m_max_rotation
    std::queue<std::pair<float, kvs::Quat>> m_path{}; ///< {radius,rotation} on the interpolated path
    std::vector<float> m_path_calc_times{}; ///< path calculation times
    std::vector<size_t> m_num_images{};
    DataQueue m_data_queue{}; ///< data queue
    EntropyFunction m_entropy_function = MixedEntropy( LightnessEntropy(), DepthEntropy(), 0.5f ); ///< entropy function
    Interpolator m_interpolator = Slerp(); ///< path interpolator
    bool m_enable_output_evaluation_image = false; ///< if true, all of evaluation images will be output
    bool m_enable_output_evaluation_image_depth = false; ///< if true, all of evaluation depth images will be output
    bool m_enable_output_entropies = false; ///< if true, calculated entropies for all viewpoints will be output

public:
    EntropyBasedCameraPathController() = default;
    virtual ~EntropyBasedCameraPathController() = default;

    size_t cacheSize() const { return m_cache_size; }
    float delta() const { return m_delta; }
    size_t entropyInterval() const { return m_entropy_interval; }
    size_t maxIndex() const { return m_max_index; }
    float maxEntropy() const { return m_max_entropy; }
    kvs::Vec3 maxPosition() const { return m_max_position; }
    kvs::Quaternion maxRotation() const { return m_max_rotation; }
    float erpRadius() const { return m_erp_radius; }
    kvs::Quaternion erpRotation() const { return m_erp_rotation; }
    size_t subTimeIndex() const { return m_sub_time_index; }

    void setCacheSize( const size_t cache_size ) { m_cache_size = cache_size; }
    void setDelta( const float delta ) { m_delta = delta; }
    void setEntropyInterval( const size_t interval ) { m_entropy_interval = interval; }
    void setEntropyFunction( EntropyFunction func ) { m_entropy_function = func; }
    void setEntropyFunctionToLightness() { m_entropy_function = LightnessEntropy(); }
    void setEntropyFunctionToColor() { m_entropy_function = ColorEntropy(); }
    void setEntropyFunctionToDepth() { m_entropy_function = DepthEntropy(); }
    void setEntropyFunctionToMixed( EntropyFunction e1, EntropyFunction e2, float p )
    {
        m_entropy_function = MixedEntropy( e1, e2, p );
    }

    void setInterpolator( InterpolationMethod interpolation_method )
    {
        if ( interpolation_method == InterpolationMethod::SLERP ) { this->setInterpolatorToSlerp(); }
        else if ( interpolation_method == InterpolationMethod::SQUAD ) { this->setInterpolatorToSquad(); }
    }

    void setInterpolatorToSlerp()
    {
        m_interpolator = Slerp();
        m_entropy_interval = m_cache_size + 1;
    }

    void setInterpolatorToSquad()
    {
        m_interpolator = Squad();
        if( m_cache_size > 0 )
        {
            if( m_cache_size % 2 == 0 ) m_cache_size -= 1;
        }
        else { m_cache_size = 1; }
        m_entropy_interval = ( m_cache_size + 1 ) / 2;
        this->pushMaxRotations( this->maxRotation() );
    }

    void setMaxIndex( const size_t index ) { m_max_index = index; }
    void setMaxEntropy( const float entropy ) { m_max_entropy = entropy; }
    void setMaxPosition( const kvs::Vec3& position ) { m_max_position = position; }
    void setMaxRotation( const kvs::Quaternion& rotation ) { m_max_rotation = rotation; }
    void setErpRadius( const float radius ) { m_erp_radius = radius; }
    void setErpRotation( const kvs::Quaternion& rotation ) { m_erp_rotation = rotation; }
    void setSubTimeIndex( const size_t index ) { m_sub_time_index = index; }

    const DataQueue& dataQueue() const { return m_data_queue; }
    bool isCacheEnabled() const { return m_cache_enabled; }
    void setCacheEnabled( const bool enabled = true ) { m_cache_enabled = enabled; }
    bool isInitialStep() const { return m_is_initial_step; }
    void setIsInitialStep( const bool is_initial_step ) { m_is_initial_step = is_initial_step; }
    bool isFinalStep() const { return m_is_final_step; }
    void setIsFinalStep( const bool is_final_step ) { m_is_final_step = is_final_step; }
    bool isEntStep() const { return m_is_ent_step; }
    void setIsEntStep( const bool is_ent_step ) { m_is_ent_step = is_ent_step; }
    bool isErpStep() const { return m_is_erp_step; }
    void setIsErpStep( const bool is_erp_step ) { m_is_erp_step = is_erp_step; }

    void setOutputEvaluationImageEnabled( const bool enable = true, const bool enable_depth = false );
    void setOutputEntropiesEnabled( const bool enable = true ) { m_enable_output_entropies = enable; }

    bool isOutputEvaluationImageEnabled() const { return m_enable_output_evaluation_image; }
    bool isOutputEvaluationDepathImageEnabled() const { return m_enable_output_evaluation_image_depth; }
    bool isOutputEntropiesEnabled() const { return m_enable_output_entropies; }

protected:
    DataQueue& dataQueue() { return m_data_queue; }
    std::vector<float>& maxEntropies() { return m_max_entropies; }
    std::vector<kvs::Vec3>& maxPositions() { return m_max_positions; }
    std::vector<kvs::Quat>& maxRotations() { return m_max_rotations; }
    std::queue<std::pair<float, kvs::Quat>>& path() { return m_path; }
    std::vector<float>& pathCalcTimes() { return m_path_calc_times; }
    std::vector<size_t>& numImages() { return m_num_images; }

    void pushMaxEntropies( const float entropy ) { m_max_entropies.push_back( entropy ); }
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
    void pushPathCalcTimes( const float path_calc_time ) { m_path_calc_times.push_back( path_calc_time ); }
    void pushNumImages( const size_t num_images ){ m_num_images.push_back( num_images ); }

    void popMaxPositions() { m_max_positions.erase( m_max_positions.begin() ); }
    void popMaxRotations() { m_max_rotations.erase( m_max_rotations.begin() ); }

    virtual void process( const Data& data ) {}
    virtual void process( const Data& data, const float radius, const kvs::Quat& rotation ) {}
    virtual float entropy( const FrameBuffer& frame_buffer );
    float radiusInterpolation( const float r1, const float r2, const float t );
    kvs::Quat pathInterpolation( const std::vector<kvs::Quat>& q, const float t );

    void push( const Data& data );
    void createPath();

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

    void outputMaxEntropies(
        const std::string& filename );

    void outputPathCalcTimes(
        const std::string& filename );

    void outputViewpointCoords(
        const std::string& filename,
        const InSituVis::Viewpoint& viewpoint );

    void outputNumImages(
        const std::string& filename,
        const size_t interval );
};

} // end of namespace InSituVis

#include "EntropyBasedCameraPathController.hpp"
