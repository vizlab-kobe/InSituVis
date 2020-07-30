#pragma once
#include <vector>

// OpenFOAM related headers
#include "fvMesh.H"
#include "volFields.H"

// KVS related headers
#include <kvs/UnstructuredVolumeObject>
#include <kvs/ValueArray>
#include <kvs/InverseDistanceWeighting>


namespace local
{

inline kvs::UnstructuredVolumeObject* CreateUnstructuredVolumeObject(
    const Foam::fvMesh& mesh,
    const Foam::volScalarField& field )
{
    std::vector<float> cell_centered_coords; // cellCoords
    std::vector<float> cell_centered_values; // pValues
    std::vector<int> cell_connections; // cell_points, label

    const size_t ncells = mesh.cellPoints().size(); // == mesh.nCells();
    for ( size_t i = 0; i < ncells; ++i )
    {
        cell_centered_coords.push_back( mesh.C()[i].x() );
        cell_centered_coords.push_back( mesh.C()[i].y() );
        cell_centered_coords.push_back( mesh.C()[i].z() );
        cell_centered_values.push_back( field[i] );

        // For only hex cells.
        const auto cell_nnodes = mesh.cellPoints()[i].size();
        const auto cell_nfaces = mesh.cells()[i].size();
        if ( cell_nnodes == 8 && cell_nfaces == 6 )
        {
            for ( auto& id : mesh.cellPoints()[i] )
            {
                cell_connections.push_back( static_cast<int>( id ) );
            }
        }
        else
        {
            for ( size_t j = 0; j < 8; ++j )
            {
                cell_connections.push_back( -1 );
            }
        }
    }

    std::vector<float> coords;
    const size_t nnodes = mesh.points().size(); // == mesh.nPoints();
    for ( size_t i = 0; i < nnodes; ++i )
    {
        coords.push_back( mesh.points()[i].x() );
        coords.push_back( mesh.points()[i].y() );
        coords.push_back( mesh.points()[i].z() );
    }

    int nocell_count = 0;
    std::vector<kvs::UInt32> connections;
    kvs::InverseDistanceWeighting<kvs::Real32> idw( nnodes );
    for ( size_t i = 0; i < ncells; ++i )
    {
        if ( cell_connections[ 8 * i ] < 0 ) { nocell_count++; }
        else
        {
            connections.push_back( static_cast<kvs::UInt32>( cell_connections[ 8 * i + 4 ] ) );
            connections.push_back( static_cast<kvs::UInt32>( cell_connections[ 8 * i + 6 ] ) );
            connections.push_back( static_cast<kvs::UInt32>( cell_connections[ 8 * i + 7 ] ) );
            connections.push_back( static_cast<kvs::UInt32>( cell_connections[ 8 * i + 5 ] ) );
            connections.push_back( static_cast<kvs::UInt32>( cell_connections[ 8 * i + 0 ] ) );
            connections.push_back( static_cast<kvs::UInt32>( cell_connections[ 8 * i + 2 ] ) );
            connections.push_back( static_cast<kvs::UInt32>( cell_connections[ 8 * i + 3 ] ) );
            connections.push_back( static_cast<kvs::UInt32>( cell_connections[ 8 * i + 1 ] ) );

            const auto x0 = cell_centered_coords[ 3 * i + 0 ];
            const auto y0 = cell_centered_coords[ 3 * i + 1 ];
            const auto z0 = cell_centered_coords[ 3 * i + 2 ];
            const auto center = kvs::Vec3( x0, y0, z0 );
            const auto value = cell_centered_values[i];
            for ( size_t j = 0; j < 8; ++j )
            {
                const auto id = static_cast<kvs::UInt32>( cell_connections[ 8 * i + j ] );
                const auto xj = coords[ 3 * id + 0 ];
                const auto yj = coords[ 3 * id + 1 ];
                const auto zj = coords[ 3 * id + 2 ];
                const auto distance = ( center - kvs::Vec3( xj, yj, zj ) ).length();
                idw.insert( id, value, distance );
            }
        }
    }

    auto values = idw.serialize();

//    int nonode_count = 0;
    for ( size_t i = 0; i < nnodes; ++i )
    {
        coords[ 3 * i + 0 ] *= 1000;
        coords[ 3 * i + 1 ] *= 1000;
        coords[ 3 * i + 2 ] *= 1000;
//        if ( kvs::Math::IsZero( values[i] ) ) { nonode_count++; }
    }

    auto* volume = new kvs::UnstructuredVolumeObject();
    volume->setCellTypeToHexahedra();
    volume->setVeclen( 1 );
    volume->setNumberOfNodes( nnodes );
    volume->setNumberOfCells( ncells - nocell_count );
    volume->setValues( values );
    volume->setCoords( kvs::ValueArray<kvs::Real32>( coords ) );
    volume->setConnections( kvs::ValueArray<kvs::UInt32>( connections ) );
    volume->updateMinMaxValues();
    volume->updateMinMaxCoords();

    return volume;
}

} // end of namespace local
