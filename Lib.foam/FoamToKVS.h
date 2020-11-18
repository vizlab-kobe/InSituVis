/*****************************************************************************/
/**
 *  @file   FoamToKVS.h
 *  @author Naohisa Sakamoto
 *  @brief  Data converter from OpenFOAM data to KVS data
 */
/*****************************************************************************/
#pragma once

// KVS headers
#include <kvs/UnstructuredVolumeObject>
#include <kvs/ValueArray>
#if defined( KVS_SUPPORT_MPI )
#include <kvs/mpi/Communicator>
#endif

// OpenFOAM headers
#include "volFields.H"
#include "fvMesh.H"


namespace InSituVis
{

namespace foam
{

/*===========================================================================*/
/**
 *  @brief  FoamToKVS class.
 */
/*===========================================================================*/
class FoamToKVS
{
public:
    using VolumeObject = kvs::UnstructuredVolumeObject;
    using CellType = VolumeObject::CellType;

private:
    bool m_decomposition; ///< flag for decomposing polyhedral cells
    kvs::ValueArray<kvs::Real32> m_coords; ///< coordinate values of each grid point
    kvs::ValueArray<kvs::Real32> m_values; ///< physical quantities of each grid point

public:
    FoamToKVS( const bool decomposition = true );
    FoamToKVS( const Foam::volScalarField& field, const bool decomposition = true );
    FoamToKVS( const Foam::volVectorField& field, const bool decomposition = true );

    void setDecompositionEnabled( const bool enable = true ) { m_decomposition = enable; }
    bool decomposition() const { return m_decomposition; }
    const kvs::ValueArray<kvs::Real32>& coords() const { return m_coords; }
    const kvs::ValueArray<kvs::Real32>& values() const { return m_values; }

    VolumeObject* exec( const Foam::volScalarField& field, const CellType type = CellType::Hexahedra );
    VolumeObject* exec( const Foam::volVectorField& field, const CellType type = CellType::Hexahedra );

#if defined( KVS_SUPPORT_MPI )
    VolumeObject* exec(
        kvs::mpi::Communicator& world,
        const Foam::volScalarField& field,
        const CellType type = CellType::Hexahedra );

    VolumeObject* exec(
        kvs::mpi::Communicator& world,
        const Foam::volVectorField& field,
        const CellType type = CellType::Hexahedra );
#endif // KVS_SUPPORT_MPI

private:
    VolumeObject* import_hex( const Foam::volScalarField& field );
    VolumeObject* import_hex( const Foam::volVectorField& field );
    VolumeObject* import_tet( const Foam::volScalarField& field );
    VolumeObject* import_tet( const Foam::volVectorField& field );
    VolumeObject* import_pyr( const Foam::volScalarField& field );
    VolumeObject* import_pyr( const Foam::volVectorField& field );
    VolumeObject* import_pri( const Foam::volScalarField& field );
    VolumeObject* import_pri( const Foam::volVectorField& field );

    kvs::ValueArray<kvs::Real32> calculate_coords( const Foam::fvMesh& mesh );
    kvs::ValueArray<kvs::Real32> calculate_values( const Foam::volScalarField& field );
    kvs::ValueArray<kvs::Real32> calculate_values( const Foam::volVectorField& field );
    kvs::ValueArray<kvs::UInt32> calculate_hex_connections( const Foam::fvMesh& mesh );
    kvs::ValueArray<kvs::UInt32> calculate_tet_connections( const Foam::fvMesh& mesh );
    kvs::ValueArray<kvs::UInt32> calculate_pyr_connections( const Foam::fvMesh& mesh );
    kvs::ValueArray<kvs::UInt32> calculate_pri_connections( const Foam::fvMesh& mesh );

#if defined( KVS_SUPPORT_MPI )
    void update_min_max_coords( kvs::mpi::Communicator& world, VolumeObject* volume );
    void update_min_max_values( kvs::mpi::Communicator& world, VolumeObject* volume );
#endif // KVS_SUPPORT_MPI
};

#include "FoamToKVS.hpp"

} // end of namespace foam

} // end of namespace InSituVis
