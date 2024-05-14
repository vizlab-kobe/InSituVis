/*****************************************************************************/
/**
 *  @file   EntropyBasedCameraFocusController.h
 *  @author Taisei Matsushima, Ken Iwata, Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <InSituVis/Lib/EntropyBasedCameraPathController.h>


namespace InSituVis
{

class EntropyBasedCameraFocusController : public EntropyBasedCameraPathController
{
public:
    using BaseClass = EntropyBasedCameraPathController;

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


public:
    EntropyBasedCameraFocusController() = default;
    virtual ~EntropyBasedCameraFocusController() = default;

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

protected:
    std::vector<kvs::Vec3>& maxFocusPoints() { return m_max_focus_points; }
    std::queue<kvs::Vec3>& focusPath() { return m_focus_path; }
    std::queue<kvs::Vec3>& focusPathPositions() { return m_focus_path_positions; }

    void pushMaxFocusPoints( const kvs::Vec3& point ) { m_max_focus_points.push_back( point ); }
    void pushFocusPathPositions( const kvs::Vec3& position )
    {
       m_max_focus_points.push_back( position );
    }
    void popMaxFocusPoints() { m_max_focus_points.erase( m_max_focus_points.begin() ); }

    virtual void process( const Data& data ) {}
    virtual void process( const Data& data, const float radius, const kvs::Quaternion& rotation, const kvs::Vec3& focus ) {};

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
};

} // end of namespace InSituVis

#include "EntropyBasedCameraFocusController.hpp"
