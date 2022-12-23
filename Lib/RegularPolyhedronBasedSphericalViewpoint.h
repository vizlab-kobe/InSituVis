#pragma once
#include "Viewpoint.h"
#include <kvs/Math>
#include <vector>
#include <algorithm>
#include <fstream>

namespace InSituVis
{

class RegularPolyhedronBasedSphericalViewpoint : public InSituVis::Viewpoint
{
    using BaseClass = InSituVis::Viewpoint;

private:
    kvs::Vec3ui m_dims{ 1, 20, 0 }; // # of spheres, # of faces ( 4, 6, 8, 12, 20 ), level
    kvs::Vec3 m_min_coord{ -12, -12, -12 }; ///< min. coord in world coordinate
    kvs::Vec3 m_max_coord{  12,  12,  12 }; ///< max. coord in world coordinate
    kvs::Vec3 m_base_position{ 0.0f, 12.0f, 0.0f };
    kvs::Vec3 m_base_up_vector{ 0.0f, 0.0f, -1.0f };

public:
    RegularPolyhedronBasedSphericalViewpoint() = default;
    virtual ~RegularPolyhedronBasedSphericalViewpoint() = default;

    const kvs::Vec3ui& dims() const { return m_dims; }
    const kvs::Vec3& minCoord() const { return m_min_coord; }
    const kvs::Vec3& maxCoord() const { return m_max_coord; }
    const kvs::Vec3& basePosition() const { return m_base_position; }
    const kvs::Vec3& baseUpVector() const { return m_base_up_vector; }

    void setDims( const kvs::Vec3ui& dims ) { m_dims = dims; }
    void setMinMaxCoords( const kvs::Vec3& min_coord, const kvs::Vec3& max_coord )
    {
        m_min_coord = min_coord;
        m_max_coord = max_coord;
    }
    void setBasePosition( const kvs::Vec3& base_position ) { m_base_position = base_position; }
    void setBaseUpVector( const kvs::Vec3& base_up_vector ) { m_base_up_vector = base_up_vector; }

    void create( const Direction d = Direction::Uni )
    {
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

        auto calc_midpoint = [&] ( const kvs::Vec3& a, const kvs::Vec3& b ) -> kvs::Vec3 {
            auto x = ( a.x() + b.x() ) * 0.5f;
            auto y = ( a.y() + b.y() ) * 0.5f;
            auto z = ( a.z() + b.z() ) * 0.5f;
            return kvs::Vec3( { x, y, z } );
        };

        auto output_coords_connections = [&] ( 
            const std::vector<kvs::Vec3>& coords, 
            const std::vector<kvs::Vec3ui>& connections )
        {
            std::ofstream coord( "coords.csv" );
            std::ofstream connection( "connections.csv" );

            for( size_t i = 0; i < coords.size(); i++ )
            {
                coord << coords[i].x() << "," << coords[i].y() << "," << coords[i].z() << std::endl;
            }

            for( size_t i = 0; i < connections.size(); i++ )
            {
                connection << connections[i].x() << "," << connections[i].y() << "," << connections[i].z() << std::endl;
            }

            coord.close();
            connection.close();
        };
        
        std::vector<kvs::Vec3> vertices;
        std::vector<kvs::Vec3ui> faces;
        const float tau = ( 1.0f + sqrt( 5.0f ) ) * 0.5f; // golden number
        const float taui = ( -1.0f + sqrt( 5.0f ) ) * 0.5f; // tau inverse
        auto g = kvs::Vec3( 0.0f, 1.0f, 0.0f );
        auto rq = kvs::Quaternion( 0.0f, 0.0f, 0.0f, 1.0f );
        const kvs::Vec3 l = { 0, 0, 0 };
        BaseClass::clear();

        switch( m_dims[1] )
        {
            case 4:
                vertices.push_back( kvs::Vec3( { -1.0f, -1.0f,  1.0f } ) );
                vertices.push_back( kvs::Vec3( {  1.0f,  1.0f,  1.0f } ) );
                vertices.push_back( kvs::Vec3( { -1.0f,  1.0f, -1.0f } ) );
                vertices.push_back( kvs::Vec3( {  1.0f, -1.0f, -1.0f } ) );

                for( size_t i = 0; i < vertices.size(); i++ ) vertices[i].normalize();

                faces.push_back( kvs::Vec3ui( {  0,  1,  2 } ) );
                faces.push_back( kvs::Vec3ui( {  0,  1,  3 } ) );
                faces.push_back( kvs::Vec3ui( {  1,  2,  3 } ) );
                faces.push_back( kvs::Vec3ui( {  0,  2,  3 } ) );

                g = kvs::Vec3( { 1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f } ); g.normalize();
                rq = kvs::Quaternion::RotationQuaternion( g, kvs::Vec3( { 0.0f, 1.0f, 0.0f } ) );

                break;
            
            case 6:
                vertices.push_back( kvs::Vec3( {  0.0f,  1.0f,  0.0f } ) );
                vertices.push_back( kvs::Vec3( {  1.0f,  1.0f, -1.0f } ) );
                vertices.push_back( kvs::Vec3( { -1.0f,  1.0f, -1.0f } ) );
                vertices.push_back( kvs::Vec3( { -1.0f,  1.0f,  1.0f } ) );
                vertices.push_back( kvs::Vec3( {  1.0f,  1.0f,  1.0f } ) );
                vertices.push_back( kvs::Vec3( {  0.0f,  0.0f, -1.0f } ) );
                vertices.push_back( kvs::Vec3( { -1.0f,  0.0f,  0.0f } ) );
                vertices.push_back( kvs::Vec3( {  0.0f,  0.0f,  1.0f } ) );
                vertices.push_back( kvs::Vec3( {  1.0f,  0.0f,  0.0f } ) );
                vertices.push_back( kvs::Vec3( {  1.0f, -1.0f, -1.0f } ) );
                vertices.push_back( kvs::Vec3( { -1.0f, -1.0f, -1.0f } ) );
                vertices.push_back( kvs::Vec3( { -1.0f, -1.0f,  1.0f } ) );
                vertices.push_back( kvs::Vec3( {  1.0f, -1.0f,  1.0f } ) );
                vertices.push_back( kvs::Vec3( {  0.0f, -1.0f,  0.0f } ) );

                for( size_t i = 0; i < vertices.size(); i++ ) vertices[i].normalize();

                faces.push_back( kvs::Vec3ui( {  0,  1,  2 } ) );
                faces.push_back( kvs::Vec3ui( {  0,  2,  3 } ) );
                faces.push_back( kvs::Vec3ui( {  0,  3,  4 } ) );
                faces.push_back( kvs::Vec3ui( {  0,  1,  4 } ) );
                faces.push_back( kvs::Vec3ui( {  1,  2,  5 } ) );
                faces.push_back( kvs::Vec3ui( {  2,  3,  6 } ) );
                faces.push_back( kvs::Vec3ui( {  3,  4,  7 } ) );
                faces.push_back( kvs::Vec3ui( {  1,  4,  8 } ) );
                faces.push_back( kvs::Vec3ui( {  1,  5,  9 } ) );
                faces.push_back( kvs::Vec3ui( {  2,  5, 10 } ) );
                faces.push_back( kvs::Vec3ui( {  2,  6, 10 } ) );
                faces.push_back( kvs::Vec3ui( {  3,  6, 11 } ) );
                faces.push_back( kvs::Vec3ui( {  3,  7, 11 } ) );
                faces.push_back( kvs::Vec3ui( {  4,  7, 12 } ) );
                faces.push_back( kvs::Vec3ui( {  4,  8, 12 } ) );
                faces.push_back( kvs::Vec3ui( {  1,  8,  9 } ) );
                faces.push_back( kvs::Vec3ui( {  5,  9, 10 } ) );
                faces.push_back( kvs::Vec3ui( {  6, 10, 11 } ) );
                faces.push_back( kvs::Vec3ui( {  7, 11, 12 } ) );
                faces.push_back( kvs::Vec3ui( {  8,  9, 12 } ) );
                faces.push_back( kvs::Vec3ui( {  9, 10, 13 } ) );
                faces.push_back( kvs::Vec3ui( { 10, 11, 13 } ) );
                faces.push_back( kvs::Vec3ui( { 11, 12, 13 } ) );
                faces.push_back( kvs::Vec3ui( {  9, 12, 13 } ) );

                g = kvs::Vec3( { 0.0f, 1.0f, -2.0f / 3.0f } ); g.normalize();
                rq = kvs::Quaternion::RotationQuaternion( g, kvs::Vec3( { 0.0f, 1.0f, 0.0f } ) );

                break;
            
            case 8:
                vertices.push_back( kvs::Vec3( {  0.0f,  1.0f,  0.0f } ) );
                vertices.push_back( kvs::Vec3( {  1.0f,  0.0f, -1.0f } ) );
                vertices.push_back( kvs::Vec3( { -1.0f,  0.0f, -1.0f } ) );
                vertices.push_back( kvs::Vec3( { -1.0f,  0.0f,  1.0f } ) );
                vertices.push_back( kvs::Vec3( {  1.0f,  0.0f,  1.0f } ) );
                vertices.push_back( kvs::Vec3( {  0.0f, -1.0f,  0.0f } ) );

                for( size_t i = 0; i < vertices.size(); i++ ) vertices[i].normalize();

                faces.push_back( kvs::Vec3ui( {  0,  1,  2 } ) );
                faces.push_back( kvs::Vec3ui( {  0,  2,  3 } ) );
                faces.push_back( kvs::Vec3ui( {  0,  3,  4 } ) );
                faces.push_back( kvs::Vec3ui( {  0,  1,  4 } ) );
                faces.push_back( kvs::Vec3ui( {  1,  2,  5 } ) );
                faces.push_back( kvs::Vec3ui( {  2,  3,  5 } ) );
                faces.push_back( kvs::Vec3ui( {  3,  4,  5 } ) );
                faces.push_back( kvs::Vec3ui( {  1,  4,  5 } ) );

                g = kvs::Vec3( { 0.0f, 1.0f / 3.0f, -2.0f / 3.0f } ); g.normalize();
                rq = kvs::Quaternion::RotationQuaternion( g, kvs::Vec3( { 0.0f, 1.0f, 0.0f } ) );

                break;
            
            case 12:
                vertices.push_back( kvs::Vec3( {  taui,   tau,  0.0f } ) );
                vertices.push_back( kvs::Vec3( { -taui,   tau,  0.0f } ) );
                vertices.push_back( kvs::Vec3( {  1.0f,  1.0f, -1.0f } ) );
                vertices.push_back( kvs::Vec3( { -1.0f,  1.0f, -1.0f } ) );
                vertices.push_back( kvs::Vec3( { -1.0f,  1.0f,  1.0f } ) );
                vertices.push_back( kvs::Vec3( {  1.0f,  1.0f,  1.0f } ) );
                vertices.push_back( kvs::Vec3( {  0.0f,  taui,   tau } ) );
                vertices.push_back( kvs::Vec3( {  0.0f, -taui,   tau } ) );
                vertices.push_back( kvs::Vec3( {   tau,  0.0f,  taui } ) );
                vertices.push_back( kvs::Vec3( {   tau,  0.0f, -taui } ) );
                vertices.push_back( kvs::Vec3( {  0.0f,  taui,  -tau } ) );
                vertices.push_back( kvs::Vec3( {  0.0f, -taui,  -tau } ) );
                vertices.push_back( kvs::Vec3( {  -tau,  0.0f, -taui } ) );
                vertices.push_back( kvs::Vec3( {  -tau,  0.0f,  taui } ) );
                vertices.push_back( kvs::Vec3( {  1.0f, -1.0f, -1.0f } ) );
                vertices.push_back( kvs::Vec3( { -1.0f, -1.0f, -1.0f } ) );
                vertices.push_back( kvs::Vec3( { -1.0f, -1.0f,  1.0f } ) );
                vertices.push_back( kvs::Vec3( {  1.0f, -1.0f,  1.0f } ) );
                vertices.push_back( kvs::Vec3( {  taui,  -tau,  0.0f } ) );
                vertices.push_back( kvs::Vec3( { -taui,  -tau,  0.0f } ) );

                vertices.push_back( vertices[ 0] + vertices[ 1] + vertices[ 4] + vertices[ 6] + vertices[ 5] );
                vertices.push_back( vertices[ 0] + vertices[ 1] + vertices[ 3] + vertices[10] + vertices[ 2] );
                vertices.push_back( vertices[ 0] + vertices[ 2] + vertices[ 9] + vertices[ 8] + vertices[ 5] );
                vertices.push_back( vertices[ 1] + vertices[ 3] + vertices[12] + vertices[13] + vertices[ 4] );
                vertices.push_back( vertices[ 5] + vertices[ 6] + vertices[ 7] + vertices[17] + vertices[ 8] );
                vertices.push_back( vertices[ 2] + vertices[ 9] + vertices[14] + vertices[11] + vertices[10] );
                vertices.push_back( vertices[ 3] + vertices[10] + vertices[11] + vertices[15] + vertices[12] );
                vertices.push_back( vertices[ 4] + vertices[ 6] + vertices[ 7] + vertices[16] + vertices[13] );
                vertices.push_back( vertices[ 8] + vertices[ 9] + vertices[14] + vertices[18] + vertices[17] );
                vertices.push_back( vertices[12] + vertices[13] + vertices[16] + vertices[19] + vertices[15] );
                vertices.push_back( vertices[ 7] + vertices[16] + vertices[19] + vertices[18] + vertices[17] );
                vertices.push_back( vertices[11] + vertices[14] + vertices[18] + vertices[19] + vertices[15] );
                
                for( size_t i = 0; i < vertices.size(); i++ ) vertices[i].normalize();

                faces.push_back( kvs::Vec3ui( {  20,  0,  1 } ) );
                faces.push_back( kvs::Vec3ui( {  20,  1,  4 } ) );
                faces.push_back( kvs::Vec3ui( {  20,  4,  6 } ) );
                faces.push_back( kvs::Vec3ui( {  20,  6,  5 } ) );
                faces.push_back( kvs::Vec3ui( {  20,  5,  0 } ) );
                faces.push_back( kvs::Vec3ui( {  21,  0,  1 } ) );
                faces.push_back( kvs::Vec3ui( {  21,  1,  3 } ) );
                faces.push_back( kvs::Vec3ui( {  21,  3, 10 } ) );
                faces.push_back( kvs::Vec3ui( {  21, 10,  2 } ) );
                faces.push_back( kvs::Vec3ui( {  21,  2,  0 } ) );
                faces.push_back( kvs::Vec3ui( {  22,  0,  2 } ) );
                faces.push_back( kvs::Vec3ui( {  22,  2,  9 } ) );
                faces.push_back( kvs::Vec3ui( {  22,  9,  8 } ) );
                faces.push_back( kvs::Vec3ui( {  22,  8,  5 } ) );
                faces.push_back( kvs::Vec3ui( {  22,  5,  0 } ) );
                faces.push_back( kvs::Vec3ui( {  23,  1,  3 } ) );
                faces.push_back( kvs::Vec3ui( {  23,  3, 12 } ) );
                faces.push_back( kvs::Vec3ui( {  23, 12, 13 } ) );
                faces.push_back( kvs::Vec3ui( {  23, 13,  4 } ) );
                faces.push_back( kvs::Vec3ui( {  23,  4,  1 } ) );
                faces.push_back( kvs::Vec3ui( {  24,  5,  6 } ) );
                faces.push_back( kvs::Vec3ui( {  24,  6,  7 } ) );
                faces.push_back( kvs::Vec3ui( {  24,  7, 17 } ) );
                faces.push_back( kvs::Vec3ui( {  24, 17,  8 } ) );
                faces.push_back( kvs::Vec3ui( {  24,  8,  5 } ) );
                faces.push_back( kvs::Vec3ui( {  25,  2,  9 } ) );
                faces.push_back( kvs::Vec3ui( {  25,  9, 14 } ) );
                faces.push_back( kvs::Vec3ui( {  25, 14, 11 } ) );
                faces.push_back( kvs::Vec3ui( {  25, 11, 10 } ) );
                faces.push_back( kvs::Vec3ui( {  25, 10,  2 } ) );
                faces.push_back( kvs::Vec3ui( {  26,  3, 10 } ) );
                faces.push_back( kvs::Vec3ui( {  26, 10, 11 } ) );
                faces.push_back( kvs::Vec3ui( {  26, 11, 15 } ) );
                faces.push_back( kvs::Vec3ui( {  26, 15, 12 } ) );
                faces.push_back( kvs::Vec3ui( {  26, 12,  3 } ) );
                faces.push_back( kvs::Vec3ui( {  27,  4,  6 } ) );
                faces.push_back( kvs::Vec3ui( {  27,  6,  7 } ) );
                faces.push_back( kvs::Vec3ui( {  27,  7, 16 } ) );
                faces.push_back( kvs::Vec3ui( {  27, 16, 13 } ) );
                faces.push_back( kvs::Vec3ui( {  27, 13,  4 } ) );
                faces.push_back( kvs::Vec3ui( {  28,  8,  9 } ) );
                faces.push_back( kvs::Vec3ui( {  28,  9, 14 } ) );
                faces.push_back( kvs::Vec3ui( {  28, 14, 18 } ) );
                faces.push_back( kvs::Vec3ui( {  28, 18, 17 } ) );
                faces.push_back( kvs::Vec3ui( {  28, 17,  8 } ) );
                faces.push_back( kvs::Vec3ui( {  29, 12, 13 } ) );
                faces.push_back( kvs::Vec3ui( {  29, 13, 16 } ) );
                faces.push_back( kvs::Vec3ui( {  29, 16, 19 } ) );
                faces.push_back( kvs::Vec3ui( {  29, 19, 15 } ) );
                faces.push_back( kvs::Vec3ui( {  29, 15, 12 } ) );
                faces.push_back( kvs::Vec3ui( {  30,  7, 16 } ) );
                faces.push_back( kvs::Vec3ui( {  30, 16, 19 } ) );
                faces.push_back( kvs::Vec3ui( {  30, 19, 18 } ) );
                faces.push_back( kvs::Vec3ui( {  30, 18, 17 } ) );
                faces.push_back( kvs::Vec3ui( {  30, 17,  7 } ) );
                faces.push_back( kvs::Vec3ui( {  31, 11, 14 } ) );
                faces.push_back( kvs::Vec3ui( {  31, 14, 18 } ) );
                faces.push_back( kvs::Vec3ui( {  31, 18, 19 } ) );
                faces.push_back( kvs::Vec3ui( {  31, 19, 15 } ) );
                faces.push_back( kvs::Vec3ui( {  31, 15, 11 } ) );

                g = vertices[0] + vertices[1] + vertices[20]; g.normalize();
                rq = kvs::Quaternion::RotationQuaternion( g, kvs::Vec3( { 0.0f, 1.0f, 0.0f } ) );
                
                break;
            
            case 20:
                vertices.push_back( kvs::Vec3( {  0.0f,  1.0f,   tau } ) );
                vertices.push_back( kvs::Vec3( {  1.0f,   tau,  0.0f } ) );
                vertices.push_back( kvs::Vec3( { -1.0f,   tau,  0.0f } ) );
                vertices.push_back( kvs::Vec3( {   tau,  0.0f,  1.0f } ) );
                vertices.push_back( kvs::Vec3( {  0.0f,  1.0f,  -tau } ) );
                vertices.push_back( kvs::Vec3( {  -tau,  0.0f,  1.0f } ) );
                vertices.push_back( kvs::Vec3( {  0.0f, -1.0f,   tau } ) );
                vertices.push_back( kvs::Vec3( {   tau,  0.0f, -1.0f } ) );
                vertices.push_back( kvs::Vec3( {  -tau,  0.0f, -1.0f } ) );
                vertices.push_back( kvs::Vec3( {  1.0f,  -tau,  0.0f } ) );
                vertices.push_back( kvs::Vec3( {  0.0f, -1.0f,  -tau } ) );
                vertices.push_back( kvs::Vec3( { -1.0f,  -tau,  0.0f } ) );

                for( size_t i = 0; i < vertices.size(); i++ ) vertices[i].normalize();

                faces.push_back( kvs::Vec3ui( {  0,  1,  2 } ) );
                faces.push_back( kvs::Vec3ui( {  0,  1,  3 } ) );
                faces.push_back( kvs::Vec3ui( {  1,  2,  4 } ) );
                faces.push_back( kvs::Vec3ui( {  0,  2,  5 } ) );
                faces.push_back( kvs::Vec3ui( {  0,  3,  6 } ) );
                faces.push_back( kvs::Vec3ui( {  1,  3,  7 } ) );
                faces.push_back( kvs::Vec3ui( {  1,  4,  7 } ) );
                faces.push_back( kvs::Vec3ui( {  2,  4,  8 } ) );
                faces.push_back( kvs::Vec3ui( {  2,  5,  8 } ) );
                faces.push_back( kvs::Vec3ui( {  0,  5,  6 } ) );
                faces.push_back( kvs::Vec3ui( {  3,  6,  9 } ) );
                faces.push_back( kvs::Vec3ui( {  3,  7,  9 } ) );
                faces.push_back( kvs::Vec3ui( {  4,  7, 10 } ) );
                faces.push_back( kvs::Vec3ui( {  4,  8, 10 } ) );
                faces.push_back( kvs::Vec3ui( {  5,  8, 11 } ) );
                faces.push_back( kvs::Vec3ui( {  5,  6, 11 } ) );
                faces.push_back( kvs::Vec3ui( {  7,  9, 10 } ) );
                faces.push_back( kvs::Vec3ui( {  8, 10, 11 } ) );
                faces.push_back( kvs::Vec3ui( {  6,  9, 11 } ) );
                faces.push_back( kvs::Vec3ui( {  9, 10, 11 } ) );

                g = kvs::Vec3( { 0.0f, ( 1.0f + 2.0f * tau ) / 3.0f, tau / 3.0f } ); g.normalize();
                rq = kvs::Quaternion::RotationQuaternion( g, kvs::Vec3( { 0.0f, 1.0f, 0.0f } ) );

                break;
            
            default:
                const auto index = 0;
                const auto q = kvs::Quaternion( 0.0f, 0.0f, 0.0f, 1.0f );
                BaseClass::add( { index, d, m_base_position, m_base_up_vector, q, l } );
                return;
        }

        for( size_t i = 0; i < m_dims[2]; i++ )
        {
            std::vector<kvs::Vec3ui> new_faces;

            for( size_t j = 0; j < faces.size(); j++ )
            {
                const auto face = faces[j];
                std::vector<unsigned int> mid_indices;
                
                for( size_t k = 0; k < 3; k++ )
                {
                    auto mid = calc_midpoint( vertices[face[k]], vertices[face[( k + 1 ) % 3]] );
                    mid.normalize();

                    std::vector<kvs::Vec3>::iterator itr;
                    itr = std::find( vertices.begin(), vertices.end(), mid );

                    if ( itr == vertices.end() )
                    {
                        const size_t mid_index = vertices.size();
                        vertices.push_back( mid );
                        mid_indices.push_back( mid_index );
                    }
                    else
                    {
                        const size_t mid_index = std::distance( vertices.begin(), itr );
                        mid_indices.push_back( mid_index );
                    }
                }

                new_faces.push_back( kvs::Vec3ui( { face[0], mid_indices[0], mid_indices[2]}));
                new_faces.push_back( kvs::Vec3ui( { face[1], mid_indices[0], mid_indices[1]}));
                new_faces.push_back( kvs::Vec3ui( { face[2], mid_indices[1], mid_indices[2]}));
                new_faces.push_back( kvs::Vec3ui( { mid_indices[0], mid_indices[1], mid_indices[2]}));
            }

            faces.swap( new_faces );
            new_faces.clear();
        }

        std::vector<kvs::Vec3> coords;

        for( size_t i = 0; i < m_dims[0]; i++ )
        {
            const float r = ( m_max_coord[0] - m_min_coord[0] ) * 0.5f * ( i + 1 ) /  m_dims[0];
        
            for( size_t j = 0; j < vertices.size(); j++ )
            {
                const auto index = vertices.size() * i + j;
                const auto xyz = kvs::Vec3( { vertices[j].x() * r, vertices[j].y() * r, vertices[j].z() * r } );
                const auto p = kvs::Quaternion::Rotate( xyz, rq );
                const auto rtp = xyz_to_rtp( p );
                const auto q = calc_rotation( p );
                const auto u = kvs::Quaternion::Rotate( m_base_up_vector, q );
                coords.push_back( p );
                BaseClass::add( { index, d, p, u, q, l } );
            }
        }

        //output_coords_connections( coords, faces );
    }
};

} // end of namespace InSituVis
