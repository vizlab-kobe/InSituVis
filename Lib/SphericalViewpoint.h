#pragma once
#include "Viewpoint.h"
#include <kvs/Math>


namespace InSituVis
{

class SphericalViewpoint : public InSituVis::Viewpoint
{
    using BaseClass = InSituVis::Viewpoint;

private:
    kvs::Vec3ui m_dims{ 1, 1, 1 }; // grid resolution
    kvs::Vec3 m_min_coord{ -12, -12, -12 }; ///< min. coord in world coordinate
    kvs::Vec3 m_max_coord{  12,  12,  12 }; ///< max. coord in world coordinate
    kvs::Vec3 m_base_position{ 0.0f, 12.0f, 0.0f };

public:
    SphericalViewpoint() = default;
    virtual ~SphericalViewpoint() = default;

    const kvs::Vec3ui& dims() const { return m_dims; }
    const kvs::Vec3& minCoord() const { return m_min_coord; }
    const kvs::Vec3& maxCoord() const { return m_max_coord; }
    const kvs::Vec3& basePosition() const { return m_base_position; }

    void setDims( const kvs::Vec3ui& dims ) { m_dims = dims; }
    void setMinMaxCoords( const kvs::Vec3& min_coord, const kvs::Vec3& max_coord )
    {
        m_min_coord = min_coord;
        m_max_coord = max_coord;
    }
    void setBasePosition( const kvs::Vec3& base_position ) { m_base_position = base_position; }

    void create( const Direction d = Direction::Uni )
    {
        auto index_to_rtp = [&] ( const size_t index ) -> kvs::Vec3 {
            const double max_theta = kvs::Math::pi * 0.99f;
            const double min_theta = kvs::Math::pi * 0.01f;

            const size_t layer = static_cast<size_t>( index / ( m_dims[1] * m_dims[2] ) ) + 1;

            const float dt = 1.0f / ( m_dims[1] - 1 );
            const size_t wt = ( index % ( m_dims[1] * m_dims[2] ) ) / m_dims[2];

            const float dp = 2.0f / ( m_dims[2] );
            const size_t wp = index % m_dims[2];

            const float r = layer * ( m_max_coord[0] - m_min_coord[0] ) / ( 2.0f * m_dims[0] );
            const float t = kvs::Math::Clamp( wt * dt * kvs::Math::pi, min_theta, max_theta );
            const float p = wp * dp * kvs::Math::pi;

            return kvs::Vec3( r, t, p );
        };

        auto index_to_xyz = [&] ( const size_t index ) -> kvs::Vec3 {
            const auto rtp = index_to_rtp( index );
            const float r = rtp[0];
            const float theta = rtp[1];
            const float phi = rtp[2];
            const float sin_theta = std::sin( theta );
            const float sin_phi = std::sin( phi );
            const float cos_theta = std::cos( theta );
            const float cos_phi = std::cos( phi );

            return kvs::Vec3( r * sin_theta * sin_phi, r * cos_theta, r * sin_theta * cos_phi );
        };

        auto calc_up_vector = [&] ( const kvs::Vec3& rtp ) -> kvs::Vec3 {
            kvs::Vec3 p;
            if( rtp[1] > kvs::Math::pi / 2 ){
                p = rtp - kvs::Vec3( { 0, kvs::Math::pi / 2, 0 } );
            }
            else{
                p = rtp + kvs::Vec3( { 0, kvs::Math::pi / 2, 0 } );
            }
            const float p_x = p[0] * std::sin( p[1] ) * std::sin( p[2] );
            const float p_y = p[0] * std::cos( p[1] );
            const float p_z = p[0] * std::sin( p[1] ) * std::cos( p[2] );
            kvs::Vec3 u;
            if( rtp[1] > kvs::Math::pi / 2 ){
                u = kvs::Vec3( { p_x, p_y, p_z } );
            }
            else{
                u = -1 * kvs::Vec3( { p_x, p_y, p_z } );
            }

            return u;
        };

        auto calc_rotation = [&] ( const size_t index ) -> kvs::Quaternion {
            const auto rtp = index_to_rtp( index );
            //const float theta = rtp[1];
            const float phi = rtp[2];
            const auto axis = kvs::Vec3( { 0.0f, 1.0f, 0.0f } );
            auto q_phi = kvs::Quaternion( axis, phi );

            const auto xyz = index_to_xyz( index );
            const auto q_theta = kvs::Quaternion::RotationQuaternion( m_base_position, xyz );

            return q_theta * q_phi;
        };

        BaseClass::clear();

        const kvs::Vec3 l = { 0, 0, 0 };
        for ( size_t index = 0; index < m_dims[0] * m_dims[1] * m_dims[2]; ++index)
        {
            const auto p = index_to_xyz( index );
            const auto p_rtp = index_to_rtp( index );
            const auto u = calc_up_vector( p_rtp );
            const auto q = calc_rotation( index );
            BaseClass::add( { index, d, p, u, q, l } );
        }
    }
};

} // end of namespace InSituVis

