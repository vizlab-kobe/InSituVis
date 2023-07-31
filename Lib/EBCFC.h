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

class EBCFC : public EntropyBasedCameraPathController
{
public:
    using BaseClass = EntropyBasedCameraPathController;

private:
    kvs::Vec3 m_max_focus_point{ 0.0f, 0.0f, 0.0f }; ///< focus point estimated at the evaluation step
    kvs::Vec3 m_best_location_position{ 0.0f, 0.0f, 0.0f }; //add
    size_t m_best_zoomlevel = 0;//add
    std::queue<kvs::Vec3> m_max_focus_points{}; ///< data queue for m_max_focus_point
    std::queue<kvs::Vec3> m_focus_path{}; ///< 
    std::vector<float> m_focus_path_positions{}; ///< focus points on the interpolated path
    kvs::Vec3 m_erp_focus{ 0.0f, 0.0f, 0.0f }; ///< interpolated focus point

public:

    EBCFC() = default;
    virtual ~EBCFC() = default;

    void setMaxFocusPoint( const kvs::Vec3& focus_point ) { m_max_focus_point = focus_point; }
    kvs::Vec3 maxFocusPoint() const { return m_max_focus_point; }
    void setErpFocus( const kvs::Vec3& erp_focus ) { m_erp_focus = erp_focus; }
    kvs::Vec3 erpFocus() const { return m_erp_focus; }
    //add
    void setBestLocationPosition( const kvs::Vec3& best_location_position ) { m_best_location_position = best_location_position; }
    kvs::Vec3 bestLocationPosition() const { return m_best_location_position; }
    void setBestZoomLevel( const size_t best_zoomlevel ) { m_best_zoomlevel = best_zoomlevel; }
    size_t bestZoomLevel() const { return m_best_zoomlevel; }

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
};

} // end of namespace InSituVis

#include "EBCFC.hpp"

