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


namespace Util
{

class Importer : public kvs::UnstructuredVolumeObject
{
    using SuperClass = kvs::UnstructuredVolumeObject;
    using CellType = SuperClass::CellType;

public:
    Importer(
        const Foam::fvMesh& mesh,
        const Foam::volScalarField& field,
        const CellType type = CellType::Hexahedra )
    {
        this->import( mesh, field, type );
    }

    Importer(
        const Foam::fvMesh& mesh,
        const Foam::volVectorField& field,
        const CellType type = CellType::Hexahedra )
    {
        this->import( mesh, field, type );
    }

    SuperClass* import(
        const Foam::fvMesh& mesh,
        const Foam::volScalarField& field,
        const CellType type = CellType::Hexahedra )
    {
        switch ( type )
        {
        case CellType::Hexahedra:
            this->import_hex( mesh, field );
            break;
        case CellType::Tetrahedra:
            this->import_tet( mesh, field );
            break;
        default:
            break;
        }
        return this;
    }

    SuperClass* import(
        const Foam::fvMesh& mesh,
        const Foam::volVectorField& field,
        const CellType type = CellType::Hexahedra )
    {
        switch ( type )
        {
        case CellType::Hexahedra:
            this->import_hex( mesh, field );
            break;
        case CellType::Tetrahedra:
            this->import_tet( mesh, field );
            break;
        default:
            break;
        }
        return this;
    }

private:
    SuperClass* import_hex(
        const Foam::fvMesh& mesh,
        const Foam::volScalarField& field )
    {
        const auto coords = this->calculate_coords( mesh );
        const auto values = this->calculate_values( mesh, field );
        const auto connections = this->calculate_hex_connections( mesh );
        const auto nnodes = coords.size() / 3;
        const auto ncells = connections.size() / 8;
        this->setCellTypeToHexahedra();
        this->setVeclen( 1 );
        this->setNumberOfNodes( nnodes );
        this->setNumberOfCells( ncells );
        this->setValues( values );
        this->setCoords( coords );
        this->setConnections( connections );
        this->updateMinMaxValues();
        this->updateMinMaxCoords();
        return this;
    }

    SuperClass* import_hex(
        const Foam::fvMesh& mesh,
        const Foam::volVectorField& field )
    {
        const auto coords = this->calculate_coords( mesh );
        const auto values = this->calculate_values( mesh, field );
        const auto connections = this->calculate_hex_connections( mesh );
        const auto nnodes = coords.size() / 3;
        const auto ncells = connections.size() / 8;
        this->setCellTypeToHexahedra();
        this->setVeclen( 1 );
        this->setNumberOfNodes( nnodes );
        this->setNumberOfCells( ncells );
        this->setValues( values );
        this->setCoords( coords );
        this->setConnections( connections );
        this->updateMinMaxValues();
        this->updateMinMaxCoords();
        return this;
    }

    SuperClass* import_tet(
        const Foam::fvMesh& mesh,
        const Foam::volScalarField& field )
    {
        const auto coords = this->calculate_coords( mesh );
        const auto values = this->calculate_values( mesh, field );
        const auto connections = this->calculate_tet_connections( mesh );
        const auto nnodes = coords.size() / 3;
        const auto ncells = connections.size() / 4;
        this->setCellTypeToHexahedra();
        this->setVeclen( 1 );
        this->setNumberOfNodes( nnodes );
        this->setNumberOfCells( ncells );
        this->setValues( values );
        this->setCoords( coords );
        this->setConnections( connections );
        if ( ncells > 0 )
        {
            this->updateMinMaxValues();
            this->updateMinMaxCoords();
        }
        return this;
    }

    SuperClass* import_tet(
        const Foam::fvMesh& mesh,
        const Foam::volVectorField& field )
    {
        const auto coords = this->calculate_coords( mesh );
        const auto values = this->calculate_values( mesh, field );
        const auto connections = this->calculate_tet_connections( mesh );
        const auto nnodes = coords.size() / 3;
        const auto ncells = connections.size() / 4;
        this->setCellTypeToHexahedra();
        this->setVeclen( 1 );
        this->setNumberOfNodes( nnodes );
        this->setNumberOfCells( ncells );
        this->setValues( values );
        this->setCoords( coords );
        this->setConnections( connections );
        if ( ncells > 0 )
        {
            this->updateMinMaxValues();
            this->updateMinMaxCoords();
        }
        return this;
    }

    kvs::ValueArray<kvs::Real32> calculate_coords(
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

    kvs::ValueArray<kvs::Real32> calculate_values(
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
                const auto distance = ( center - vertex ).length();
                idw.insert( id, value, distance );
            }
        }

        return idw.serialize();
    }

    kvs::ValueArray<kvs::Real32> calculate_values(
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
                const auto distance = ( center - vertex ).length();
                idw.insert( id, value, distance );
            }
        }

        return idw.serialize();
    }

    kvs::ValueArray<kvs::UInt32> calculate_tet_connections(
        const Foam::fvMesh& mesh )
    {
        const size_t ncells = mesh.nCells(); // number of cells

        std::vector<kvs::UInt32> connections;
        for ( size_t i = 0; i < ncells; ++i )
        {
            // For only hex cells.
            const auto cell_nnodes = mesh.cellPoints()[i].size();
            const auto cell_nfaces = mesh.cells()[i].size();
//            if ( cell_nfaces == 4 )
//            {
//                std::cout << "cell nnodes = " << cell_nnodes << std::endl;
//                std::cout << "cell nfaces = " << cell_nfaces << std::endl;
//                std::cout << std::endl;
//            }
            if ( cell_nnodes == 4 && cell_nfaces == 4 )
            {
                Foam::label id[4];
                std::copy_n( mesh.cellPoints()[i].begin(), 4, id );

                connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[3] ) );
            }
        }

        return kvs::ValueArray<kvs::UInt32>( connections );
    }

    kvs::ValueArray<kvs::UInt32> calculate_hex_connections(
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
                Foam::label id[8];
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
};

} // end of namespace Util

