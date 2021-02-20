/*****************************************************************************/
/**
 *  @file   DistributedViewpoint.h
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include "Viewpoint.h"
#include <kvs/Vector3>


namespace InSituVis
{

/*===========================================================================*/
/**
 *  @brief  Distributed viewpoint class.
 */
/*===========================================================================*/
class DistributedViewpoint : public Viewpoint
{
    using BaseClass = Viewpoint;

public:
    enum DistType
    {
        CubicDist, ///< cubic viewpoint distribution
        SphericalDist ///< spherical viewpoint distribution
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
                    // const kvs::Vec3 rtp = this->index_to_ijk( index ); // polar coordinate
                    // const kvs::Vec3 ijk = this->rtp_to_ijk( rtp ); // ijk coordinate
                    // const kvs::Vec3 xyz = this->ijk_to_xyz( ijk ); // world coordinate
                    const kvs::Vec3 rtp = this->index_to_rtp( index ); // polar coordinate
                    const kvs::Vec3 xyz = this->rtp_to_xyz( rtp ); // world coordinate
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

        switch ( m_dist_type )
        {
        case DistType::CubicDist:
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
            break;
        case DistType::SphericalDist:
            for ( size_t k = 0; k < m_dims[1]; ++k )
            {
                for ( size_t j = 0; j < 2 * (m_dims[0] + m_dims[1] - 2); ++j )
                {
                    const auto point = BaseClass::locator( index );
                    BaseClass::addPoint( point.position, m_dir_type );
                    index++;
                }
            }
            break;
        default:
            break;
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

    kvs::Vec3 index_to_rtp( const size_t index ) const
    {
        const float dt = 1.0f / (m_dims[1] - 1);
        const size_t wt = (m_dims[1] - 1) - index / (2 * (m_dims[0] + m_dims[1] - 2));

        const float dp = 1.0f / (m_dims[0] + m_dims[2] - 2.0f );
        const size_t wp = index % ( 2 * ( m_dims[0] + m_dims[2] - 2 ));

        const float r = ( m_max_coord[0] - m_min_coord[0] ) / 2.0f; // TODO min?
        const float t = wt * dt * kvs::Math::pi;
        const float p = wp * dp * kvs::Math::pi;

        // if (wt == 0)
        // {
        //     const float p0 = kvs::Math::pi * (float(m_dims[1] - 2.0f) / (m_dims[1] - 1.0f)) * wp;
        //     const float r0 = r * std::cos( p0 );
        //     return kvs::Vec3( r0, 0, 0 );
        // }

        // const bool isOrigin = (wp == 0 && wt == (m_dims[1] - 1) / 2);
        // return isOrigin ? kvs::Vec3(0, 0, 0) : kvs::Vec3(r, t, p);

        return kvs::Vec3(r, t, p);

        // const size_t rank = std::round( (std::pow(index, 1.0f / 3.0f)) / 2.0f );

        // const float r = rank * ( m_max_coord[0] - m_min_coord[0] ) / ( m_dims[0] - 1.0f );

    }

    kvs::Vec3 rtp_to_xyz( const kvs::Vec3& rtp ) const
    {
        const float r = rtp[0];
        const float theta = rtp[1];
        const float phi = rtp[2];
        const float sin_theta = std::sin( theta );
        const float sin_phi = std::sin( phi );
        const float cos_theta = std::cos( theta );
        const float cos_phi = std::cos( phi );

        return kvs::Vec3( r * sin_theta * sin_phi, r * cos_theta, r * sin_theta * cos_phi );
    }
};

} // end of namespace InSituVis
