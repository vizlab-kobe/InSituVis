/*****************************************************************************/
/**
 *  @file   EntropyBasedCameraPathTimeStepController.h
 *  @author Kauzya Adachi, Taisei Matsushima, Ken Iwata, Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <InSituVis/Lib/EntropyBasedCameraPathController.h>


namespace InSituVis
{

class EntropyBasedCameraPathTimeStepController : public EntropyBasedCameraPathController
{
public:
    using BaseClass = EntropyBasedCameraPathController;
    using Data = InSituVis::Adaptor::ObjectList;
    using DataQueue = std::queue<Data>;
    using Volume = kvs::VolumeObjectBase;
    using Values = Volume::Values;
    using DivergenceFunction = std::function<float(const Values&,const Values&, const float)>;
    
        // divergence function
    static float GaussianKLDivergence( const Values& P0, const Values& P1, const float D_max );

private:
    size_t m_interval = 10; ///< time interval of entropy calculation
    float m_threshold = 0.0f; ///< threshold value for divergence evalution
    float m_previous_divergence = 0.0f; ///< divergence for the previous dataset
    DivergenceFunction m_divergence_function = GaussianKLDivergence; ///< divergence function
    Data m_previous_data{}; ///< dataset at previous time-step
    kvs::Quaternion m_previous_rotation{};
    kvs::Vec3 m_previous_position{};
    std::vector<float> m_path_positions{};

    bool m_enable_output_divergence = false;
    std::vector<float> m_divergences;
    std::vector<float> var_divergences;
    std::vector<float> var_threshold;
    size_t m_dataqueue_size = 0;
    bool m_validation_step = false;

public:
    EntropyBasedCameraPathTimeStepController() = default;
    virtual ~EntropyBasedCameraPathTimeStepController() = default;

    float divergenceThreshold() const { return m_threshold; }

    void setDivergenceThreshold( const float threshold ) { m_threshold = threshold; }
    void setDivergenceFunction( DivergenceFunction func ) { m_divergence_function = func; }

    // void setEntropyInterval( const size_t interval ) { m_interval = interval; }
    // void setEntropyFunction( EntropyFunction func ) { m_entropy_function = func; }
    // void setEntropyFunctionToLightness() { m_entropy_function = LightnessEntropy(); }
    // void setEntropyFunctionToColor() { m_entropy_function = ColorEntropy(); }
    // void setEntropyFunctionToDepth() { m_entropy_function = DepthEntropy(); }
    // void setEntropyFunctionToMixed( EntropyFunction e1, EntropyFunction e2, float p )
    // {
    //     m_entropy_function = MixedEntropy( e1, e2, p );
    // }
    void setValidationStep( bool validation_step ){ m_validation_step = validation_step; }
    bool isValidationStep(){ return m_validation_step; }

    const Data& previousData() const { return m_previous_data; }
    const kvs::Quaternion& previousRoation() const { return m_previous_rotation; }
    const kvs::Vec3& previousPosition() const { return m_previous_position; }
    const float previousDivergence() const { return m_previous_divergence; }
    const size_t dataQueueSize() const { return m_dataqueue_size; }
    const std::vector<float>& pathPositions() const { return m_path_positions; }


    ///
    void setOutputDivergenceEnabled( const bool enable = true ) { m_enable_output_divergence = enable; }////
    bool isOutputDivergenceEnabled() const { return m_enable_output_divergence; }////

    const std::vector<float>& divergences() const { return m_divergences; }


protected:
    void setPreviousData( const Data& data ) { m_previous_data = data; }
    void setPreviousDivergence( const float divergence ) { m_previous_divergence = divergence; }
    void setPreviousPosition( const kvs::Vec3& position ) { m_previous_position = position; }
    void setPreviousRotation( const kvs::Quaternion& rotation ) { m_previous_rotation = rotation; }

    virtual void process( const Data& data ) {}
    virtual void process( const Data& data, const float radius, const kvs::Quat& rotation ) {}
    std::vector<float>& pathPositions() { return m_path_positions; }
    
    virtual void push( const Data& data );
    // void createPath(
    //     const float r2,
    //     const float r3,
    //     const kvs::Quat& q1,
    //     const kvs::Quat& q2,
    //     const kvs::Quat& q3,
    //     const kvs::Quat& q4,
    //     const size_t point_interval
    // );
    void pushPathPositions( const kvs::Vec3& position )
    {
        m_path_positions.push_back( position.x() );
        m_path_positions.push_back( position.y() );
        m_path_positions.push_back( position.z() );
    }

    virtual float divergence( const Values& P0, const Values& P1 );

    // std::string logDataFilename(
    //     const std::string& basename,
    //     const InSituVis::OutputDirectory& directory );

    // std::string logDataFilename(
    //     const std::string& basename,
    //     const kvs::UInt32 timestep,
    //     const InSituVis::OutputDirectory& directory );


    // void outputPathEntropies(
    //     const std::string& filename,
    //     const size_t analysis_interval );

    // void outputPathPositions(
    //     const std::string& filename,
    //     const size_t analysis_interval );

    ///
    void outputDivergences(
        const std::string& filename,
        const std::vector<float>& divergences );
};

} // end of namespace InSituVis

#include "EntropyBasedCameraPathTimeStepController.hpp"