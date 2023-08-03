/*****************************************************************************/
/**
 *  @file   EntropyBasedCameraPathController.h
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
    std::queue<kvs::Vec3> m_max_focus_points{}; ///< data queue for m_max_focus_point
    std::queue<kvs::Vec3> m_focus_path{}; ///< 
    std::vector<float> m_focus_path_positions{}; ///< focus points on the interpolated path
    kvs::Vec3 m_erp_focus{ 0.0f, 0.0f, 0.0f }; ///< interpolated focus point
    bool m_enable_output_frame_entropies = false; ///< if true, calculted entropies on the divided framebuffer will be output
    bool m_enable_output_zoom_entropies = false; ///< if true, calculted entropies along the viewing ray will be output

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

protected:
    std::queue<kvs::Vec3>& maxFocusPoints() { return m_max_focus_points; }
    std::queue<kvs::Vec3>& focusPath() { return m_focus_path; }
    std::vector<float>& focusPathPositions() { return m_focus_path_positions; }

    void pushMaxFocusPoints( const kvs::Vec3& point ) { m_max_focus_points.push( point ); }
    void pushFocusPathPositions( const kvs::Vec3& position )
    {
        m_focus_path_positions.push_back( position.x() );
        m_focus_path_positions.push_back( position.y() );
        m_focus_path_positions.push_back( position.z() );
    }

    virtual void process( const Data& data ) {}
    virtual void process( const Data& data, const float radius, const kvs::Vec3& focus, const kvs::Quat& rotation ) {};

    void push( const Data& data );
    void createPath(
        const float r2,
        const float r3,
        const kvs::Vec3& f2,
        const kvs::Vec3& f3,
        const kvs::Quat& q1,
        const kvs::Quat& q2,
        const kvs::Quat& q3,
        const kvs::Quat& q4,
        const size_t point_interval
    );

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
