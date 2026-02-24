/*****************************************************************************/
/**
 *  @file   CameraFocusPredefinedControlled2.h
 */
/*****************************************************************************/
#pragma once

#include <InSituVis/Lib/Adaptor.h>
#include <InSituVis/Lib/Viewpoint.h>

#include <kvs/ValueArray>
#include <kvs/Vector3>
#include <kvs/Vector2>
#include <kvs/String>
#include <kvs/Timer>
#include <kvs/ColorImage>
#include <kvs/PointObject>
#include <kvs/SphereGlyph>
#include <kvs/Xform>
#include <kvs/OpenGL>
#include <kvs/Math>

#include <vector>
#include <string>
#include <numeric>
#include <algorithm>
#include <unordered_set>
#include <iostream>
#include <cmath>
#include <cstring>

namespace InSituVis
{

class CameraFocusPredefinedControlledAdaptor : public InSituVis::Adaptor
{
public:
    using BaseClass = InSituVis::Adaptor;
    using Viewpoint = InSituVis::Viewpoint;
    using Location  = Viewpoint::Location;

    // =============================================================
    // Focus mode switch
    // =============================================================
    enum class FocusMode
    {
        Extrema,
        ImageEntropy
    };

    // =============================================================
    // Extrema struct (index space)
    // =============================================================
    struct extremum
    {
        kvs::Vec3 position; // index coords
        float value;
    };

    // =============================================================
    // FrameBuffer (self-defined: Adaptor does NOT provide this type)
    // =============================================================
    struct FrameBuffer
    {
        kvs::ValueArray<kvs::UInt8> color_buffer; // RGBA
        kvs::ValueArray<kvs::Real32> depth_buffer; // 0..1
        size_t width = 0;
        size_t height = 0;
    };

public:
    CameraFocusPredefinedControlledAdaptor() = default;
    virtual ~CameraFocusPredefinedControlledAdaptor() = default;

    // ---- mode ----
    FocusMode focusMode() const { return m_focus_mode; }
    void setFocusMode( const FocusMode mode ) { m_focus_mode = mode; }

    // ---- transforms (index->physical->localCoord) ----
    void setFocusFieldTransform( const kvs::Vec3& origin, const kvs::Vec3& delta )
    {
        m_focus_origin = origin;
        m_focus_delta  = delta;
        m_focus_has_transform = true;
    }

    void setGlobalFieldTransform( const kvs::Vec3& global_min_coord, const kvs::Vec3& global_min_delta )
    {
        m_global_min_coord = global_min_coord;
        m_global_min_delta = global_min_delta;
        m_global_has_transform = true;
    }

    kvs::Vec3 indexToPhysical( const kvs::Vec3& p_index ) const
    {
        return kvs::Vec3(
            m_focus_origin.x() + m_focus_delta.x() * p_index.x(),
            m_focus_origin.y() + m_focus_delta.y() * p_index.y(),
            m_focus_origin.z() + m_focus_delta.z() * p_index.z()
        );
    }

    kvs::Vec3 physicalToLocal( const kvs::Vec3& p_phys ) const
    {
        return ( p_phys - m_global_min_coord ) / m_global_min_delta;
    }

    // ---- visualization xform ----
    void setVisualizationXform( const kvs::Xform& xform )
    {
        m_vis_xform = xform;
        m_has_vis_xform = true;
    }
    bool hasVisualizationXform() const { return m_has_vis_xform; }
    const kvs::Xform& visualizationXform() const { return m_vis_xform; }

    // ---- output ----
    void setOutputImageEnabled( const bool enabled ) { m_enable_output_image = enabled; }

    // ---- image entropy params ----
    void setFrameDivisions( const kvs::Vec2ui& divs ) { m_frame_divs = divs; }
    void setEntropyBins( const size_t bins ) { m_entropy_bins = std::max<size_t>(8, bins); }

    // ---- extrema pipeline ----
    void estimateFocusPoint( const kvs::ValueArray<float>& values, const kvs::Vec3ui& dims );
    void CreateExtremaObject( const std::vector<extremum>& extrema, int i, size_t top_n, const kvs::Vec3ui& dims );

    void setFocusInterpN( const size_t focus_interp_n ){ m_focus_interp_n = focus_interp_n; }
    void setFocusTopN( const size_t focus_top_n ){ m_focus_top_n = focus_top_n; }
    void setZoomPoint( const float zoom ){ m_zoom_t = zoom; }

protected:
    virtual void execRendering() override;

    std::string outputFinalImageName( const size_t level, const size_t cid );
    std::string outputInterpImageName( const size_t prev_id, const size_t curr_id, const size_t s_id );

    void computeDerivativesFull(
        const kvs::ValueArray<float>& values,
        const kvs::Vec3ui& dims,
        kvs::ValueArray<float>& fx,
        kvs::ValueArray<float>& fy,
        kvs::ValueArray<float>& fz,
        kvs::ValueArray<float>& fxy,
        kvs::ValueArray<float>& fxz,
        kvs::ValueArray<float>& fyz,
        kvs::ValueArray<float>& fxyz );
    
    void appendParamsCSVRow(
        const std::string& image_relpath,
        float pressure_value,
        const std::vector<float>& fp_from_prev,
        const std::vector<float>& cp_from_prev );

    float dist3_(const kvs::Vec3& a, const kvs::Vec3& b) const;

    // void updateCSVAndPrevCache_ZeroCP(
    //     const std::vector<kvs::Vec3>& curr_focus_worlds,
    //     const std::vector<float>& curr_pressures );

    void updateCSVAndPrevCache_FP_CP(
        const std::vector<kvs::Vec3>& curr_focus_worlds,
        const std::vector<kvs::Vec3>& curr_camera_worlds,
        const std::vector<float>& curr_pressures );

private:
    // ---- readback helpers ----
    FrameBuffer readbackFrameBuffer( const Location& location );

    // ---- entropy helpers ----
    float entropyRGBA( const kvs::ValueArray<kvs::UInt8>& rgba, size_t w, size_t h ) const;

    FrameBuffer cropFrameBuffer( const FrameBuffer& fb, const kvs::Vec2i& ij ) const;
    kvs::Vec3 look_at_in_window( const FrameBuffer& fb ) const;
    kvs::Vec3 window_to_object( const kvs::Vec3 win, const Location& location );

    inline kvs::Vec3 lerpVec3(const kvs::Vec3& a, const kvs::Vec3& b, float t)
    {
        return kvs::Vec3(
            a.x() * (1.0f - t) + b.x() * t,
            a.y() * (1.0f - t) + b.y() * t,
            a.z() * (1.0f - t) + b.z() * t );
    }


private:
    FocusMode m_focus_mode = FocusMode::Extrema;

    bool m_enable_output_image = true;
    kvs::StampTimer m_rend_timer{};
    kvs::StampTimer m_save_timer{};

    // transforms
    kvs::Vec3 m_focus_origin{0,0,0};
    kvs::Vec3 m_focus_delta{1,1,1};
    bool m_focus_has_transform = false;

    kvs::Vec3 m_global_min_coord{0,0,0};
    kvs::Vec3 m_global_min_delta{1,1,1};
    bool m_global_has_transform = false;

    // vis xform
    kvs::Xform m_vis_xform{};
    bool m_has_vis_xform = false;

    // extrema cache
    std::vector<extremum> m_last_maxima;
    std::vector<extremum> m_last_minima;
    std::vector<extremum> m_last_saddles;

    kvs::Vec3ui m_last_dims{0,0,0};

    kvs::Vec3 m_last_extrema_world{0,0,0};
    bool m_has_last_extrema_world = false;

    kvs::Vec3 m_estimated_focus_phys{0,0,0};
    bool m_has_estimated_focus_phys = false;

    // image entropy params
    kvs::Vec2ui m_frame_divs{4,4};
    size_t m_entropy_bins = 256;

    std::vector<kvs::Vec3> m_last_extrema_worlds;
    std::vector<kvs::Vec3> m_focus_candidates_local;
    size_t m_focus_top_n = 5; // 好きな値に

    // ---- interpolation (Extrema focus) ----
    bool m_enable_focus_interpolation = true;   // 必要なら setter で制御
    size_t m_focus_interp_n = 4;                // 「n個」(補間点数)

    // 前タイムステップの top-N 注視点(world)
    std::vector<kvs::Vec3> m_prev_extrema_worlds;
    bool m_has_prev_extrema_worlds = false;

    // ===== output_video_params.csv 出力用 =====
    bool m_enable_output_params_csv = true;
    std::string m_params_csv_path;
    bool m_params_csv_header_written = false;

    // 前stepの topN 注視点(world)
    std::vector<kvs::Vec3> m_prev_focus_worlds;
    std::vector<kvs::Vec3> m_prev_camera_worlds;
    bool m_has_prev_focus = false;

    // 現stepの topN 注視点の評価値（圧力）を保持（estimateFocusPointで埋める）
    std::vector<float> m_curr_focus_pressure;

    // 前stepの「注視点(world)」と「カメラ位置(world)」を候補ごとに保持

    // ズーム係数（あなたの 1/5 をそのままメンバ化しておくと一括で変えられる）
    float m_zoom_t = 2.0f / 5.0f;
};

} // namespace InSituVis

#include "CameraFocusPredefinedControlledAdaptorMulti.hpp"
