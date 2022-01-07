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

public:
    CubicViewpoint() = default;
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
            const auto ijk = kvs::Vec3( { i, j, k } );
            const auto d = kvs::Vec3( m_dims ) - kvs::Vec3::Constant(1);
            return ( ijk / d ) * ( m_max_coord - m_min_coord ) + m_min_coord;
        };

        auto index_to_rtp = [&] ( const size_t index ) -> kvs::Vec3 {
            const auto xyz = index_to_xyz( index );
            const float x = xyz[0];
            const float y = xyz[1];
            const float z = xyz[2];
            float r, t, p;
            r = sqrt( xyz.dot( xyz ) );
            if( r == 0 ){
                t = 0;
                p = 0;
            }
            else{
                t = acos( y / r );
                if( ( x == 0 ) && ( z == 0 ) ){
                    p = 0;
                }
                else{
                    if( x >= 0 ){
                        p = acos( z / sqrt( z * z + x * x ) );
                    }
                    else{
                        p = -1 * acos( z / sqrt( z * z + x * x ) );
                    }
                }
            }
            if( p < 0 ){
                p += 2 * kvs::Math::pi;
            }

            return kvs::Vec3( r, t, p );
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

        BaseClass::clear();

        const kvs::Vec3 l = { 0, 0, 0 };

        for ( size_t index = 0; index < m_dims[0] * m_dims[1] * m_dims[2]; ++index)
        {
            const auto p = index_to_xyz( index );
            const auto p_rtp = index_to_rtp( index );
            const auto u = calc_up_vector( p_rtp );
            BaseClass::add( { d, p, u, l } );
        }
    }
};

} // end of namespace InSituVis
