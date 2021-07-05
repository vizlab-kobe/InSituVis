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
        auto ijk_to_xyz = [&] ( const kvs::Vec3& ijk ) -> kvs::Vec3 {
            const auto d = kvs::Vec3( m_dims ) - kvs::Vec3::Constant(1);
            return ( ijk / d ) * ( m_max_coord - m_min_coord ) + m_min_coord;
        };

        BaseClass::clear();

        const kvs::Vec3 l = { 0, 0, 0 };
        for ( size_t k = 0; k < m_dims[2]; ++k )
        {
            for ( size_t j = 0; j < m_dims[1]; ++j )
            {
                for ( size_t i = 0; i < m_dims[0]; ++i )
                {
                    const auto p = ijk_to_xyz( { i, j, k } );
                    BaseClass::add( { d, p, l } );
                }
            }
        }
    }
};

} // end of namespace InSituVis
