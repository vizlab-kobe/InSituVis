#pragma once
#include "Viewpoint.h"
#include <kvs/Vector3>


namespace InSituVis
{

class DistributedViewpoint : public Viewpoint
{
    using BaseClass = Viewpoint;

public:
    enum DistType
    {
        CubicDist,
        SphericalDist
    };

private:
    kvs::Vec3ui m_dims; ///< grid resolution
    DistType m_dist_type; ///< viewpoint distribution type
    DirType m_dir_type; ///< viewpoint direction type
    kvs::Vec3 m_min_coord; ///< min. coord in world coordinate
    kvs::Vec3 m_max_coord; ///< max. coord in world coordinate

public:
    DistributedViewpoint(
        const kvs::Vec3ui& dims,
        const DistType dist_type = DistType::CubicDist,
        const DirType dir_type = DirType::SingleDir ):
        m_dims( dims ),
        m_dist_type( dist_type ),
        m_dir_type( dir_type ),
        m_min_coord( -12, -12, -12 ),
        m_max_coord(  12,  12,  12 )
    {
        BaseClass::setNumberOfPointsCounter(
            [&]()
            {
                const size_t npoints = m_dims.x() * m_dims.y() * m_dims.z();
                return npoints;
            } );

        switch ( dist_type )
        {
        case DistType::CubicDist:
            BaseClass::setPointLocator(
                [&] ( const size_t index )
                {
                    const kvs::Vec3 ijk = this->index_to_ijk( index ); // ijk coordinate
                    const kvs::Vec3 xyz = this->ijk_to_xyz( ijk ); // world coordinate
                    return BaseClass::Point( xyz, m_dir_type );
                } );
            break;
        case DistType::SphericalDist:
            BaseClass::setPointLocator(
                [&] ( const size_t index )
                {
                    const kvs::Vec3 rtp = this->index_to_ijk( index ); // polor coordinate
                    const kvs::Vec3 ijk = this->rtp_to_ijk( rtp ); // ijk coordinate
                    const kvs::Vec3 xyz = this->ijk_to_xyz( ijk ); // world coordinate
                    return BaseClass::Point( xyz, m_dir_type );
                } );
            break;
        default:
            break;
        }
    }

    void generate()
    {
        BaseClass::clearPoints();

        size_t index = 0;
        for ( size_t k = 0; k < m_dims[2]; ++k )
        {
            for ( size_t j = 0; j < m_dims[1]; ++j )
            {
                for ( size_t i = 0; i < m_dims[0]; ++i )
                {
                    const auto point = BaseClass::locator( index );
                    BaseClass::addPoint( point.position, m_dir_type );
                    index++;
                }
            }
        }
    }

private:
    size_t ijk_to_index( const kvs::Vec3ui& ijk ) const
    {
        const size_t i = ijk[0];
        const size_t j = ijk[1];
        const size_t k = ijk[2];
        return i + j * m_dims[0] + k * m_dims[0] * m_dims[1];
    }

    kvs::Vec3 index_to_ijk( const size_t index ) const
    {
        const size_t i = index % m_dims[0];
        const size_t j = index / m_dims[0] % m_dims[1];
        const size_t k = index / ( m_dims[0] * m_dims[1] );
        return kvs::Vec3( i, j, k );
    }

    kvs::Vec3 rtp_to_ijk( const kvs::Vec3& rtp ) const
    {
        const float r = rtp[0];
        const float theta = ( rtp[1] / ( m_dims[1] - 1 ) ) * 2.0f * kvs::Math::pi;
        const float phi = ( rtp[2] / ( m_dims[2] - 1 ) ) * 2.0f * kvs::Math::pi;
        const float sin_theta = std::sin( theta );
        const float sin_phi = std::sin( phi );
        const float cos_theta = std::cos( theta );
        const float cos_phi = std::cos( phi );
        return kvs::Vec3( r * sin_theta * cos_phi, r * sin_theta * sin_phi, r * cos_theta );
    }

    kvs::Vec3 ijk_to_xyz( const kvs::Vec3& ijk ) const
    {
        const kvs::Vec3 one = kvs::Vec3::Constant(1);
        return ( ijk / ( kvs::Vec3( m_dims ) - one ) ) * ( m_max_coord - m_min_coord ) + m_min_coord;
    }
};

} // end of namespace InSituVis