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

public:
    SphericalViewpoint() = default;
    virtual ~SphericalViewpoint() = default;

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
        auto index_to_rtp = [&] ( const size_t index ) -> kvs::Vec3 {
            const size_t layer = static_cast<size_t>( index / ( m_dims[1] * m_dims[2] ) ) + 1;

            const float dt = 1.0f / ( m_dims[1] - 1 );
            const size_t wt = ( index % ( m_dims[1] * m_dims[2] ) ) / m_dims[2];

            const float dp = 2.0f / ( m_dims[2] );
            const size_t wp = index % m_dims[2];

            const float r = layer * ( m_max_coord[0] - m_min_coord[0] ) / ( 2.0f * m_dims[0] );
            const float t = wt * dt * kvs::Math::pi;
            const float p = wp * dp * kvs::Math::pi;

            return kvs::Vec3( r, t, p );
        };

        auto rtp_to_xyz = [&] ( const kvs::Vec3& rtp ) -> kvs::Vec3 {
            const float r = rtp[0];
            const float theta = rtp[1];
            const float phi = rtp[2];
            const float sin_theta = std::sin( theta );
            const float sin_phi = std::sin( phi );
            const float cos_theta = std::cos( theta );
            const float cos_phi = std::cos( phi );

            return kvs::Vec3( r * sin_theta * sin_phi, r * cos_theta, r * sin_theta * cos_phi );
        };

        BaseClass::clear();

        size_t index = 0;
        const kvs::Vec3 l = { 0, 0, 0 };
        for ( size_t k = 0; k < m_dims[1]; ++k )
        {
            for ( size_t j = 0; j < 2 * (m_dims[0] + m_dims[1] - 2); ++j )
            {
                const auto rtp = index_to_rtp( index );
                const auto p = rtp_to_xyz( rtp );
                BaseClass::add( { d, p, l } );
                index++;
            }
        }
    }
};

} // end of namespace InSituVis

