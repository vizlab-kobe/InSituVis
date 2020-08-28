#pragma once
#include <vector>
#include <algorithm>

// OpenFOAM related headers
#include "fvMesh.H"
#include "volFields.H"

// KVS related headers
#include <kvs/mpi/Communicator>
#include <kvs/UnstructuredVolumeObject>
#include <kvs/ValueArray>
#include <kvs/InverseDistanceWeighting>


namespace Util
{

class Importer : public kvs::UnstructuredVolumeObject
{
    using SuperClass = kvs::UnstructuredVolumeObject;

public:
    Importer(
        const Foam::fvMesh& mesh,
        const Foam::volScalarField& field )
    {
        this->import( mesh, field );
    }

    Importer(
        const Foam::fvMesh& mesh,
        const Foam::volVectorField& field )
    {
        this->import( mesh, field );
    }

    Importer(
        kvs::mpi::Communicator& world,
        const Foam::fvMesh& mesh,
        const Foam::volScalarField& field )
    {
        this->import( world, mesh, field );
    }

    Importer(
        kvs::mpi::Communicator& world,
        const Foam::fvMesh& mesh,
        const Foam::volVectorField& field )
    {
        this->import( world, mesh, field );
    }

    SuperClass* import(
        const Foam::fvMesh& mesh,
        const Foam::volScalarField& field )
    {
        const auto coords = this->calculate_coords( mesh );
        const auto values = this->calculate_values( mesh, field );
        const auto connections = this->calculate_connections( mesh );
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

    SuperClass* import(
        const Foam::fvMesh& mesh,
        const Foam::volVectorField& field )
    {
        const auto coords = this->calculate_coords( mesh );
        const auto values = this->calculate_values( mesh, field );
        const auto connections = this->calculate_connections( mesh );
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

    SuperClass* import(
        kvs::mpi::Communicator& world,
        const Foam::fvMesh& mesh,
        const Foam::volScalarField& field )
    {
        this->import( mesh, field );
        this->update_min_max_values( world );
        this->update_min_max_coords( world );
        return this;
    }

    SuperClass* import(
        kvs::mpi::Communicator& world,
        const Foam::fvMesh& mesh,
        const Foam::volVectorField& field )
    {
        this->import( mesh, field );
        this->update_min_max_values( world );
        this->update_min_max_coords( world );
        return this;
    }

private:
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

    kvs::ValueArray<kvs::UInt32> calculate_connections(
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

    void update_min_max_values( kvs::mpi::Communicator& world )
    {
        auto min_value = this->minValue();
        auto max_value = this->maxValue();
        world.allReduce( min_value, min_value, MPI_MIN );
        world.allReduce( max_value, max_value, MPI_MAX );
        this->setMinMaxValues( min_value, max_value );
    }

    void update_min_max_coords( kvs::mpi::Communicator& world )
    {
        auto min_coord = this->minObjectCoord();
        auto max_coord = this->maxObjectCoord();
        world.allReduce( min_coord[0], min_coord[0], MPI_MIN );
        world.allReduce( min_coord[1], min_coord[1], MPI_MIN );
        world.allReduce( min_coord[2], min_coord[2], MPI_MIN );
        world.allReduce( max_coord[0], max_coord[0], MPI_MAX );
        world.allReduce( max_coord[1], max_coord[1], MPI_MAX );
        world.allReduce( max_coord[2], max_coord[2], MPI_MAX );
        this->setMinMaxObjectCoords( min_coord, max_coord );
        this->setMinMaxExternalCoords( min_coord, max_coord );
    }
};

} // end of namespace Util

