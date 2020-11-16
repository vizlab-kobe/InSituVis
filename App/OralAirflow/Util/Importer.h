#pragma once
#include <vector>
#include <algorithm>

// OpenFOAM related headers
#include "fvMesh.H"
#include "volFields.H"
#include "volPointInterpolation.H"
#include "cellShape.H"
#include "cellModeller.H"

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

    Importer(
        kvs::mpi::Communicator& world,
        const Foam::fvMesh& mesh,
        const Foam::volScalarField& field,
        const CellType type = CellType::Hexahedra )
    {
        this->import( world, mesh, field, type );
    }

    Importer(
        kvs::mpi::Communicator& world,
        const Foam::fvMesh& mesh,
        const Foam::volVectorField& field,
        const CellType type = CellType::Hexahedra )
    {
        this->import( world, mesh, field, type );
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
        case CellType::Pyramid:
            this->import_pyr( mesh, field );
            break;
        case CellType::Prism:
            this->import_prism( mesh, field );
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
        case CellType::Pyramid:
            this->import_pyr( mesh, field );
            break;
        case CellType::Prism:
            this->import_prism( mesh, field );
            break;
        default:
            break;
        }
        return this;
    }

    SuperClass* import(
        kvs::mpi::Communicator& world,
        const Foam::fvMesh& mesh,
        const Foam::volScalarField& field,
        const CellType type = CellType::Hexahedra )
    {
        this->import( mesh, field, type );
        this->update_min_max_values( world );
        this->update_min_max_coords( world );
        return this;
    }

    SuperClass* import(
        kvs::mpi::Communicator& world,
        const Foam::fvMesh& mesh,
        const Foam::volVectorField& field,
        const CellType type = CellType::Hexahedra )
    {
        this->import( mesh, field, type );
        this->update_min_max_values( world );
        this->update_min_max_coords( world );
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
        this->setCellTypeToTetrahedra();
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
        this->setCellTypeToTetrahedra();
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

    SuperClass* import_pyr(
        const Foam::fvMesh& mesh,
        const Foam::volScalarField& field )
    {
        const auto coords = this->calculate_coords( mesh );
        const auto values = this->calculate_values( mesh, field );
        const auto connections = this->calculate_pyr_connections( mesh );
        const auto nnodes = coords.size() / 3;
        const auto ncells = connections.size() / 5;
        this->setCellTypeToPyramid();
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

    SuperClass* import_pyr(
        const Foam::fvMesh& mesh,
        const Foam::volVectorField& field )
    {
        const auto coords = this->calculate_coords( mesh );
        const auto values = this->calculate_values( mesh, field );
        const auto connections = this->calculate_pyr_connections( mesh );
        const auto nnodes = coords.size() / 3;
        const auto ncells = connections.size() / 5;
        this->setCellTypeToPyramid();
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

    SuperClass* import_prism(
        const Foam::fvMesh& mesh,
        const Foam::volScalarField& field )
    {
        const auto coords = this->calculate_coords( mesh );
        const auto values = this->calculate_values( mesh, field );
        const auto connections = this->calculate_prism_connections( mesh );
        const auto nnodes = coords.size() / 3;
        const auto ncells = connections.size() / 6;
        this->setCellTypeToPrism();
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

    SuperClass* import_prism(
        const Foam::fvMesh& mesh,
        const Foam::volVectorField& field )
    {
        const auto coords = this->calculate_coords( mesh );
        const auto values = this->calculate_values( mesh, field );
        const auto connections = this->calculate_prism_connections( mesh );
        const auto nnodes = coords.size() / 3;
        const auto ncells = connections.size() / 6;
        this->setCellTypeToPrism();
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
        Foam::volPointInterpolation p( mesh );
        const Foam::pointScalarField v = p.interpolate( field );
        kvs::ValueArray<kvs::Real32> values( v.size() );
        for ( size_t i = 0; i < values.size(); ++i )
        {
            values[i] = static_cast<kvs::Real32>( v[i] );
        }
        return values;
    }

    kvs::ValueArray<kvs::Real32> calculate_values(
        const Foam::fvMesh& mesh,
        const Foam::volVectorField& field )
    {
        Foam::volPointInterpolation p( mesh );
        const Foam::pointVectorField v = p.interpolate( field );
        kvs::ValueArray<kvs::Real32> values( v.size() );
        for ( size_t i = 0; i < values.size(); ++i )
        {
            values[i] = static_cast<kvs::Real32>( Foam::mag( v[i] ) );
        }
        return values;
    }

    kvs::ValueArray<kvs::UInt32> calculate_tet_connections(
        const Foam::fvMesh& mesh )
    {
        std::vector<kvs::UInt32> connections;
        const auto& tet = *(Foam::cellModeller::lookup("tet"));
        const auto& cell_shapes = mesh.cellShapes();
        const size_t ncells = cell_shapes.size();
        for ( size_t i = 0; i < ncells; ++i )
        {
            const auto& cell_shape = cell_shapes[i];
            const auto& cell_model = cell_shape.model();
            if ( cell_model == tet )
            {
                const auto& id = cell_shape;
                connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[3] ) );
            }
        }
        return kvs::ValueArray<kvs::UInt32>( connections );
    }

    kvs::ValueArray<kvs::UInt32> calculate_pyr_connections(
        const Foam::fvMesh& mesh )
    {
        std::vector<kvs::UInt32> connections;
        const auto& pyr = *(Foam::cellModeller::lookup("pyr"));
        const auto& cell_shapes = mesh.cellShapes();
        const size_t ncells = cell_shapes.size();
        for ( size_t i = 0; i < ncells; ++i )
        {
            const auto& cell_shape = cell_shapes[i];
            const auto& cell_model = cell_shape.model();
            if ( cell_model == pyr )
            {
                const auto& id = cell_shape;
                connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[3] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[4] ) );
            }
        }
        return kvs::ValueArray<kvs::UInt32>( connections );
    }

    kvs::ValueArray<kvs::UInt32> calculate_prism_connections(
        const Foam::fvMesh& mesh )
    {
        std::vector<kvs::UInt32> connections;
        const auto& prism = *(Foam::cellModeller::lookup("prism"));
        const auto& ttwed = *(Foam::cellModeller::lookup("tetWedge"));
        const auto& cell_shapes = mesh.cellShapes();
        const size_t ncells = cell_shapes.size();
        for ( size_t i = 0; i < ncells; ++i )
        {
            const auto& cell_shape = cell_shapes[i];
            const auto& cell_model = cell_shape.model();
            if ( cell_model == prism )
            {
                const auto& id = cell_shape;
                connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[3] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[4] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[5] ) );
            }
            else if ( cell_model == ttwed )
            {
                const auto& id = cell_shape;
                connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[3] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[4] ) );
            }
        }
        return kvs::ValueArray<kvs::UInt32>( connections );
    }

    kvs::ValueArray<kvs::UInt32> calculate_hex_connections(
        const Foam::fvMesh& mesh )
    {
        std::vector<kvs::UInt32> connections;
        const auto& hex = *(Foam::cellModeller::lookup("hex"));
        const auto& wed = *(Foam::cellModeller::lookup("wedge"));
        const auto& cell_shapes = mesh.cellShapes();
        const size_t ncells = cell_shapes.size();
        for ( size_t i = 0; i < ncells; ++i )
        {
            const auto& cell_shape = cell_shapes[i];
            const auto& cell_model = cell_shape.model();
            if ( cell_model == hex )
            {
                const auto& id = cell_shape;
                connections.push_back( static_cast<kvs::UInt32>( id[4] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[5] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[6] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[7] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[3] ) );
            }
            else if ( cell_model == wed )
            {
                const auto& id = cell_shape;
                connections.push_back( static_cast<kvs::UInt32>( id[3] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[4] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[5] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[6] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
                connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
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

