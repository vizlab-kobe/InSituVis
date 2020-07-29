#pragma once
#include <vector>

// OpenFOAM related headers
#include "fvMesh.H"
#include "volFields.H"

// KVS related headers
#include <kvs/UnstructuredVolumeObject>
#include <kvs/ValueArray>


namespace local
{

inline kvs::UnstructuredVolumeObject* CreateUnstructuredVolumeObject(
    const Foam::fvMesh& mesh,
    const Foam::volScalarField& field )
{
    std::vector<float> cell_centered_coords; // cellCoords
    std::vector<float> cell_centered_values; // pValues

    const size_t ncells = mesh.cellPoints().size();
    for ( size_t i = 0; i < ncells; ++i )
    {
        cell_centered_coords.push_back( mesh.C()[i].x() );
        cell_centered_coords.push_back( mesh.C()[i].y() );
        cell_centered_coords.push_back( mesh.C()[i].z() );
        cell_centered_values.push_back( field[i] );
    }

    std::vector<float> coords;
    const size_t nnodes = mesh.points().size();
    for ( size_t i = 0; i < nnodes; ++i )
    {
        coords.push_back( mesh.points()[i].x() );
        coords.push_back( mesh.points()[i].y() );
        coords.push_back( mesh.points()[i].z() );
    }

    auto* volume = new kvs::UnstructuredVolumeObject();

    return volume;
}

} // end of namespace local
