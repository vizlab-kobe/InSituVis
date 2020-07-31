#pragma once
#include <vector>
#include <algorithm>

// OpenFOAM related headers
#include "fvMesh.H"
#include "volFields.H"

// KVS related headers
#include <kvs/UnstructuredVolumeObject>
#include <kvs/ValueArray>
#include <kvs/InverseDistanceWeighting>

//#include "InverseDistanceWeighting.h"


namespace Util
{

namespace internal
{

inline kvs::ValueArray<kvs::Real32> CalculateCoords(
    const Foam::fvMesh& mesh )
{
    const size_t nnodes = mesh.nPoints(); // number of nodes

    kvs::ValueArray<float> coords( nnodes * 3 );
    for ( size_t i = 0; i < nnodes; ++i )
    {
        coords[ 3 * i + 0 ] = mesh.points()[i].x();
        coords[ 3 * i + 1 ] = mesh.points()[i].y();
        coords[ 3 * i + 2 ] = mesh.points()[i].z();
    }

    return coords;
}

inline kvs::ValueArray<kvs::Real32> CalculateValues(
    const Foam::fvMesh& mesh,
    const Foam::volScalarField& field )
{
    const size_t ncells = mesh.nCells(); // number of cells
    const size_t nnodes = mesh.nPoints(); // number of nodes

    // Calculate values on each grid point.
    kvs::InverseDistanceWeighting<kvs::Real32> idw( nnodes );
    for ( size_t i = 0; i < ncells; ++i )
    {
        const auto& c = mesh.C()[i];
        const kvs::Vec3 center( c.x(), c.y() , c.z() );
        const float value = static_cast<float>( field[i] );
        for ( auto& id : mesh.cellPoints()[i] )
        {
            const auto& p = mesh.points()[id];
            const kvs::Vec3 vertex( p.x(), p.y(), p.z() );
            const auto distance = ( center -vertex ).length();
            idw.insert( id, value, distance );
        }
    }

    return idw.serialize();
}

inline kvs::ValueArray<kvs::Real32> CalculateValues(
    const Foam::fvMesh& mesh,
    const Foam::volVectorField& field )
{
    const size_t ncells = mesh.nCells(); // number of cells
    const size_t nnodes = mesh.nPoints(); // number of nodes

    // Calculate values on each grid point.
    kvs::InverseDistanceWeighting<kvs::Real32> idw( nnodes );
    for ( size_t i = 0; i < ncells; ++i )
    {
        const auto& c = mesh.C()[i];
        const kvs::Vec3 center( c.x(), c.y() , c.z() );
        const float value = static_cast<float>( Foam::mag( field[i] ) );
        for ( auto& id : mesh.cellPoints()[i] )
        {
            const auto& p = mesh.points()[id];
            const kvs::Vec3 vertex( p.x(), p.y(), p.z() );
            const auto distance = ( center -vertex ).length();
            idw.insert( id, value, distance );
        }
    }

    return idw.serialize();
}

inline kvs::ValueArray<kvs::UInt32> CalculateConnections(
    const Foam::fvMesh& mesh )
{
    const size_t ncells = mesh.nCells(); // number of cells

    std::vector<kvs::UInt32> connections;
    for ( size_t i = 0; i < ncells; ++i )
    {
        // For only hex cells.
        const auto cell_nnodes = mesh.cellPoints()[i].size();
        const auto cell_nfaces = mesh.cells()[i].size();
        if ( cell_nnodes == 8 && cell_nfaces == 6 )
        {
            kvs::UInt32 id[8];
            std::copy_n( mesh.cellPoints()[i].begin(), 8, id );
/*
            connections.push_back( static_cast<kvs::UInt32>( id[4] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[5] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[6] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[7] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[3] ) );
*/
/*
            connections.push_back( static_cast<kvs::UInt32>( id[3] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[7] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[6] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[4] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[5] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
*/
            connections.push_back( static_cast<kvs::UInt32>( id[4] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[6] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[7] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[5] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[3] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
        }
    }

    return kvs::ValueArray<kvs::UInt32>( connections );
}

}

inline kvs::UnstructuredVolumeObject* CreateUnstructuredVolumeObject(
    const Foam::fvMesh& mesh,
    const Foam::volScalarField& field )
{
    const auto coords = internal::CalculateCoords( mesh );
    const auto values = internal::CalculateValues( mesh, field );
    const auto connections = internal::CalculateConnections( mesh );

    const auto nnodes = coords.size() / 3;
    const auto ncells = connections.size() / 8;

    auto* volume = new kvs::UnstructuredVolumeObject();
    volume->setCellTypeToHexahedra();
    volume->setVeclen( 1 );
    volume->setNumberOfNodes( nnodes );
    volume->setNumberOfCells( ncells );
    volume->setValues( values );
    volume->setCoords( coords );
    volume->setConnections( connections );
    volume->updateMinMaxValues();
    volume->updateMinMaxCoords();

    return volume;
}

inline kvs::UnstructuredVolumeObject* CreateUnstructuredVolumeObject(
    const Foam::fvMesh& mesh,
    const Foam::volVectorField& field )
{
    const auto coords = internal::CalculateCoords( mesh );
    const auto values = internal::CalculateValues( mesh, field );
    const auto connections = internal::CalculateConnections( mesh );

    const auto nnodes = coords.size() / 3;
    const auto ncells = connections.size() / 8;

    auto* volume = new kvs::UnstructuredVolumeObject();
    volume->setCellTypeToHexahedra();
    volume->setVeclen( 1 );
    volume->setNumberOfNodes( nnodes );
    volume->setNumberOfCells( ncells );
    volume->setValues( values );
    volume->setCoords( coords );
    volume->setConnections( connections );
    volume->updateMinMaxValues();
    volume->updateMinMaxCoords();

    return volume;
}

} // end of namespace Util
