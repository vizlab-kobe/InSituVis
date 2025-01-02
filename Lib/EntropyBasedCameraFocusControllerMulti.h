/*****************************************************************************/
/**
 *  @file   EntropyBasedCameraFocusControllerMulti.h
 *  @author Taisei Matsushima, Ken Iwata, Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <InSituVis/Lib/EntropyBasedCameraPathController.h>


namespace InSituVis
{

class EntropyBasedCameraFocusControllerMulti : public EntropyBasedCameraPathController
{
public:
    using BaseClass = EntropyBasedCameraPathController;
    //add
    enum ROIMethod { max, maximum };
    
private:
    kvs::Vec3 m_max_focus_point{ 0.0f, 0.0f, 0.0f }; ///< focus point estimated at the evaluation step
    std::vector<kvs::Vec3> m_max_focus_points{}; ///< data queue for m_max_focus_point
    std::queue<kvs::Vec3> m_focus_path{}; ///< 
    std::queue<kvs::Vec3> m_focus_path_positions{}; ///< focus points on the interpolated path
    kvs::Vec3 m_erp_focus{ 0.0f, 0.0f, 0.0f }; ///< interpolated focus point
    bool m_enable_output_frame_entropies = false; ///< if true, calculted entropies on the divided framebuffer will be output
    bool m_enable_output_zoom_entropies = false; ///< if true, calculted entropies along the viewing ray will be output

    bool m_enable_auto_zooming = false; ///< if true, auto-zooming fuctionality will be available
    bool m_image_type = true;
    kvs::Vec3 m_estimated_zoom_position{ 0.0f, 0.0f, 0.0f }; ///< estimated zoom position along the viewing ray
    size_t m_estimated_zoom_level = 0; ///< estimated zoom level
    //add
    std::vector<kvs::Vec3> m_cand_positions{}; 
    std::vector<kvs::Quat> m_cand_rotations{}; 
    std::vector<kvs::Vec3> m_cand_focus_points{};
    std::vector<size_t> m_cand_zoom_levels{};
    size_t m_candidate_num = 1;
    std::vector<std::string> m_output_filenames{};
    std::vector<float> m_focus_entropies{};
    std::vector<float> m_focus_path_length{};
    std::vector<float> m_camera_path_length{};

    //add
    ROIMethod m_ROI_method = ROIMethod::max;
public:
    EntropyBasedCameraFocusControllerMulti() = default;
    virtual ~EntropyBasedCameraFocusControllerMulti() = default;

    void setMaxFocusPoint( const kvs::Vec3& focus_point ) { m_max_focus_point = focus_point; }
    kvs::Vec3 maxFocusPoint() const { return m_max_focus_point; }
    void setErpFocus( const kvs::Vec3& erp_focus ) { m_erp_focus = erp_focus; }
    kvs::Vec3 erpFocus() const { return m_erp_focus; }

    void setOutputFrameEntropiesEnabled( const bool enable = true ) { m_enable_output_frame_entropies = enable; }
    void setOutputZoomEntropiesEnabled( const bool enable = true ) { m_enable_output_zoom_entropies = enable; }
    bool isOutputFrameEntropiesEnabled() const { return m_enable_output_frame_entropies; }
    bool isOutputZoomEntropiesEnabled() const { return m_enable_output_zoom_entropies; }

    void setAutoZoomingEnabled( const bool enable = true ) { m_enable_auto_zooming = enable; }
    void setOutputColorImage( const bool color = true ) { m_image_type = color; }
    void setEstimatedZoomPosition( const kvs::Vec3& position ) { m_estimated_zoom_position = position; }
    void setEstimatedZoomLevel( const size_t level ) { m_estimated_zoom_level = level; }
    bool isAutoZoomingEnabled() { return m_enable_auto_zooming; }
    bool isOutpuColorImage() { return m_image_type; }
    kvs::Vec3 estimatedZoomPosition() const { return m_estimated_zoom_position; }
    size_t estimatedZoomLevel() const { return m_estimated_zoom_level; }
    size_t candidateNum() const { return m_candidate_num; }
    void setCandidateNum( const size_t candidateNum ) { m_candidate_num = candidateNum; }

    //add
    void setROIMethod( ROIMethod method ){ m_ROI_method = method; }
    ROIMethod isROIMethod(){ return m_ROI_method; }

protected:
    std::vector<kvs::Vec3>& maxFocusPoints() { return m_max_focus_points; }
    std::queue<kvs::Vec3>& focusPath() { return m_focus_path; }
    std::queue<kvs::Vec3>& focusPathPositions() { return m_focus_path_positions; }

    //add
    std::vector<std::string>& outputFilenames() {return m_output_filenames; }
    std::vector<float>& focusEntropies() { return m_focus_entropies; }
    std::vector<float>& focusPathLength() { return m_focus_path_length; }
    std::vector<float>& cameraPathLength(){ return m_camera_path_length; }
    void pushOutputFilenames( const std::string filename ) { m_output_filenames.push_back( filename ); }
    void pushFocusEntropies( const float entropy ) { m_focus_entropies.push_back( entropy ); }
    void pushFocusPathLength( const float path_length ) { m_focus_path_length.push_back( path_length ); }
    void pushCameraPathLength( const float path_length ) { m_camera_path_length.push_back( path_length ); }

    std::vector<kvs::Vec3>& candPositions() { return m_cand_positions; }
    void pushCandPositions( const kvs::Vec3& position ) { m_cand_positions.push_back( position ); }
    void popCandPositions() { m_cand_positions.erase( m_cand_positions.begin() ); }
    std::vector<kvs::Quat>& candRotations() { return m_cand_rotations; }
    void pushCandRotations( const kvs::Quat& rotation ) { m_cand_rotations.push_back( rotation ); }
    void popCandRotations() { m_cand_rotations.erase( m_cand_rotations.begin() ); }
    std::vector<kvs::Vec3>& candFocusPoints() { return m_cand_focus_points; }
    void pushCandFocusPoints( const kvs::Vec3& focus ) { m_cand_focus_points.push_back( focus ); }
    void popCandFocusPoints() { m_cand_focus_points.erase( m_cand_focus_points.begin() ); }
    std::vector<size_t>& candZoomLevels() { return m_cand_zoom_levels; }
    void pushCandZoomLevels( const size_t& zoom_level ) { m_cand_zoom_levels.push_back( zoom_level ); }
    void popCandZoomLevels() { m_cand_zoom_levels.erase( m_cand_zoom_levels.begin() ); }    

    void pushMaxFocusPoints( const kvs::Vec3& point ) { m_max_focus_points.push_back( point ); }
    void pushFocusPathPositions( const kvs::Vec3& position )
    {
       m_max_focus_points.push_back( position );
    }
    void popMaxFocusPoints() { m_max_focus_points.erase( m_max_focus_points.begin() ); }

    virtual void process( const Data& data ){}
    virtual void process( const Data& data, const float radius, const kvs::Quaternion& rotation, const kvs::Vec3& focus, const int route_num ){}

    virtual void push( const Data& data );
    virtual void createPath();

    void outputFrameEntropies(
        const std::string& filename,
        const std::vector<float>& entropies )
    {
        BaseClass::outputEntropies( filename, entropies );
    }

    void outputZoomEntropies(
        const std::string& filename,
        const std::vector<float>& entropies )
    {
        BaseClass::outputEntropies( filename, entropies );
    }
    void outputVideoParams(
        const std::string& filename1,
        const std::vector<std::string>& filename2,
        const std::vector<float>& focus_entropies,
        const std::vector<float>& focus_path_length,
        const std::vector<float>& camera_path_length
    );
};

} // end of namespace InSituVis

#include "EntropyBasedCameraFocusControllerMulti.hpp"
