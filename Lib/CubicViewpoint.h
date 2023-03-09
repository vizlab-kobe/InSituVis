#pragma once
#include "Viewpoint.h"


namespace InSituVis
{

class CubicViewpoint : public InSituVis::Viewpoint
{
    using BaseClass = InSituVis::Viewpoint;

private:
    kvs::Vec3ui m_dims{ 1, 1, 1 }; // grid resolution
    kvs::Vec3 m_min_coord{ -12, -12, -12 }; ///< min. coord in world coordinate
    kvs::Vec3 m_max_coord{  12,  12,  12 }; ///< max. coord in world coordinate
    kvs::Vec3 m_base_position{ 0.0f, 12.0f, 0.0f };
    kvs::Vec3 m_base_up_vector{ 0.0f, 0.0f, -1.0f };

public:
    CubicViewpoint() = default;
    CubicViewpoint(
        const kvs::Vec3ui& dims,
        const Direction dir = Direction::Uni ):
        m_dims( dims )
    {
        this->create( dir );
    }
    virtual ~CubicViewpoint() = default;

    const kvs::Vec3ui& dims() const { return m_dims; }
    const kvs::Vec3& minCoord() const { return m_min_coord; }
    const kvs::Vec3& maxCoord() const { return m_max_coord; }

    void setDims( const kvs::Vec3ui& dims ) { m_dims = dims; }
    void setMinMaxCoords( const kvs::Vec3& min_coord, const kvs::Vec3& max_coord )
    {
        m_min_coord = min_coord;
        m_max_coord = max_coord;
    }

    void create( const Direction d = Direction::Uni )
    {
        auto index_to_xyz = [&] ( const size_t index ) -> kvs::Vec3 {
            const size_t i = index % m_dims[0];
            const size_t j = ( index / m_dims[0] ) % m_dims[1];
            const size_t k = index / ( m_dims[0] * m_dims[1] );
            const auto ijk = kvs::Vec3( i, j, k );
            const auto d = kvs::Vec3( m_dims ) - kvs::Vec3::Constant(1);
            return ( ijk / d ) * ( m_max_coord - m_min_coord ) + m_min_coord;
        };

        auto xyz_to_rtp = [&] ( const kvs::Vec3& xyz ) -> kvs::Vec3 {
            const float x = xyz[0];
            const float y = xyz[1];
            const float z = xyz[2];

            const float r = sqrt( x * x + y * y + z * z );
            const float t = std::acos( y / r );
            const float p = std::atan2( x, z );

            return kvs::Vec3( r, t, p );
        };

        auto calc_rotation = [&] ( const kvs::Vec3& xyz ) -> kvs::Quaternion {
            const auto rtp = xyz_to_rtp( xyz );
            const float phi = rtp[2];
            const auto axis = kvs::Vec3( { 0.0f, 1.0f, 0.0f } );
            auto q_phi = kvs::Quaternion( axis, phi );
            const auto q_theta = kvs::Quaternion::RotationQuaternion( m_base_position, xyz );
            return q_theta * q_phi;
        };

        BaseClass::clear();

        const kvs::Vec3 l = { 0, 0, 0 };

        for ( size_t index = 0; index < m_dims[0] * m_dims[1] * m_dims[2]; ++index)
        {
            const auto p = index_to_xyz( index );
            const auto q = calc_rotation( p );
            const auto u = kvs::Quaternion::Rotate( m_base_up_vector, q );
            BaseClass::add( { index, d, p, u, q, l } );
        }
    }
};

} // end of namespace InSituVis
