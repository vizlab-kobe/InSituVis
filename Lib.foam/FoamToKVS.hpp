/*****************************************************************************/
/**
 *  @file   FoamToKVS.hpp
 *  @author Naohisa Sakamoto
 *  @brief  Data converter from OpenFOAM data to KVS data
 */
/*****************************************************************************/

/*===========================================================================*/
/**
 *  @brief  Constructs a new FoamToKVS class.
 *  @param  decomposition [in] flag for polyhedral cell docmposition
 */
/*===========================================================================*/
inline FoamToKVS::FoamToKVS( const bool decomposition ):
    m_decomposition( decomposition )
{
}

/*===========================================================================*/
/**
 *  @brief  Constructs a new FoamToKVS class.
 *  @param  field [in] scalar field
 *  @param  decomposition [in] flag for polyhedral cell docmposition
 *
 *  The coordinates and physical values (scalar values) at the grid points
 *  are calculated from the specified field data (cell centered data).
 */
/*===========================================================================*/
inline FoamToKVS::FoamToKVS(
    const Foam::volScalarField& field,
    const bool decomposition ):
    m_decomposition( decomposition )
{
    m_coords = this->calculate_coords( field.mesh() );
    m_values = this->calculate_values( field );
}

/*===========================================================================*/
/**
 *  @brief  Constructs a new FoamToKVS class.
 *  @param  field [in] vector field
 *  @param  decomposition [in] flag for polyhedral cell docmposition
 *
 *  The coordinates and physical values (scalar values calculated as magnitude
 *  value of the specified vector value) at the grid points are calculated
 *  from the specified field data (cell centered data).
 */
/*===========================================================================*/
inline FoamToKVS::FoamToKVS(
    const Foam::volVectorField& field,
    const bool decomposition ):
    m_decomposition( decomposition )
{
    m_coords = this->calculate_coords( field.mesh() );
    m_values = this->calculate_values( field );
}

/*===========================================================================*/
/**
 *  @brief  Executes data conversion.
 *  @param  field [in] scalar field data in OpenFOAM format
 *  @param  type [in] importing cell type
 *  @return converted volume data in KVS format
 */
/*===========================================================================*/
inline FoamToKVS::VolumeObject FoamToKVS::exec(
    const Foam::volScalarField& field,
    const CellType type )
{
    VolumeObject volume;
    switch ( type )
    {
    case CellType::Hexahedra: this->import_hex( field, &volume ); break;
    case CellType::Tetrahedra: this->import_tet( field, &volume ); break;
    case CellType::Pyramid: this->import_pyr( field, &volume ); break;
    case CellType::Prism: this->import_pri( field, &volume ); break;
    default: break;
    }
    return volume;
}

/*===========================================================================*/
/**
 *  @brief  Executes data conversion.
 *  @param  field [in] vector field data in OpenFOAM format
 *  @param  type [in] importing cell type
 *  @return converted volume data in KVS format
 */
/*===========================================================================*/
inline FoamToKVS::VolumeObject FoamToKVS::exec(
    const Foam::volVectorField& field,
    const CellType type )
{
    VolumeObject volume;
    switch ( type )
    {
    case CellType::Hexahedra: this->import_hex( field, &volume ); break;
    case CellType::Tetrahedra: this->import_tet( field, &volume ); break;
    case CellType::Pyramid: this->import_pyr( field, &volume ); break;
    case CellType::Prism: this->import_pri( field, &volume ); break;
    default: break;
    }
    return volume;
}

#if defined( KVS_SUPPORT_MPI )
/*===========================================================================*/
/**
 *  @brief  Executes data conversion.
 *  @param  world [in] MPI communicator
 *  @param  field [in] scalar field data in OpenFOAM format
 *  @param  type [in] importing cell type
 *  @return converted volume data in KVS format
 */
/*===========================================================================*/
inline FoamToKVS::VolumeObject FoamToKVS::exec(
    kvs::mpi::Communicator& world,
    const Foam::volScalarField& field,
    const CellType type )
{
    auto volume = this->exec( field, type );
    this->update_min_max_coords( world, &volume );
    this->update_min_max_values( world, &volume );
    return volume;
}

/*===========================================================================*/
/**
 *  @brief  Executes data conversion.
 *  @param  world [in] MPI communicator
 *  @param  field [in] vector field data in OpenFOAM format
 *  @param  type [in] importing cell type
 *  @return converted volume data in KVS format
 */
/*===========================================================================*/
inline FoamToKVS::VolumeObject FoamToKVS::exec(
    kvs::mpi::Communicator& world,
    const Foam::volVectorField& field,
    const CellType type )
{
    auto volume = this->exec( field, type );
    this->update_min_max_coords( world, &volume );
    this->update_min_max_values( world, &volume );
    return volume;
}
#endif // KVS_SUPPORT_MPI

inline void FoamToKVS::import_hex(
    const Foam::volScalarField& field,
    VolumeObject* volume )
{
    const auto coords = m_coords.empty() ? this->calculate_coords( field.mesh() ) : m_coords;
    const auto values = m_values.empty() ? this->calculate_values( field ) : m_values;
    const auto connections = this->calculate_hex_connections( field.mesh() );
    const auto nnodes = coords.size() / 3;
    const auto ncells = connections.size() / 8;

    volume->setCellTypeToHexahedra();
    volume->setVeclen( 1 );
    volume->setNumberOfNodes( nnodes );
    volume->setNumberOfCells( ncells );
    volume->setValues( values );
    volume->setCoords( coords );
    volume->setConnections( connections );
    if ( ncells > 0 )
    {
        volume->updateMinMaxValues();
        volume->updateMinMaxCoords();
    }
}

inline void FoamToKVS::import_hex(
    const Foam::volVectorField& field,
    VolumeObject* volume )
{
    const auto coords = m_coords.empty() ? this->calculate_coords( field.mesh() ) : m_coords;
    const auto values = m_values.empty() ? this->calculate_values( field ) : m_values;
    const auto connections = this->calculate_hex_connections( field.mesh() );
    const auto nnodes = coords.size() / 3;
    const auto ncells = connections.size() / 8;

    volume->setCellTypeToHexahedra();
    volume->setVeclen( 1 );
    volume->setNumberOfNodes( nnodes );
    volume->setNumberOfCells( ncells );
    volume->setValues( values );
    volume->setCoords( coords );
    volume->setConnections( connections );
    if ( ncells > 0 )
    {
        volume->updateMinMaxValues();
        volume->updateMinMaxCoords();
    }
}

inline void FoamToKVS::import_tet(
    const Foam::volScalarField& field,
    VolumeObject* volume )
{
    const auto coords = m_coords.empty() ? this->calculate_coords( field.mesh() ) : m_coords;
    const auto values = m_values.empty() ? this->calculate_values( field ) : m_values;
    const auto connections = this->calculate_tet_connections( field.mesh() );
    const auto nnodes = coords.size() / 3;
    const auto ncells = connections.size() / 4;

    volume->setCellTypeToTetrahedra();
    volume->setVeclen( 1 );
    volume->setNumberOfNodes( nnodes );
    volume->setNumberOfCells( ncells );
    volume->setValues( values );
    volume->setCoords( coords );
    volume->setConnections( connections );
    if ( ncells > 0 )
    {
        volume->updateMinMaxValues();
        volume->updateMinMaxCoords();
    }
}

inline void FoamToKVS::import_tet(
    const Foam::volVectorField& field,
    VolumeObject* volume )
{
    const auto coords = m_coords.empty() ? this->calculate_coords( field.mesh() ) : m_coords;
    const auto values = m_values.empty() ? this->calculate_values( field ) : m_values;
    const auto connections = this->calculate_tet_connections( field.mesh() );
    const auto nnodes = coords.size() / 3;
    const auto ncells = connections.size() / 4;

    volume->setCellTypeToTetrahedra();
    volume->setVeclen( 1 );
    volume->setNumberOfNodes( nnodes );
    volume->setNumberOfCells( ncells );
    volume->setValues( values );
    volume->setCoords( coords );
    volume->setConnections( connections );
    if ( ncells > 0 )
    {
        volume->updateMinMaxValues();
        volume->updateMinMaxCoords();
    }
}

inline void FoamToKVS::import_pyr(
    const Foam::volScalarField& field,
    VolumeObject* volume )
{
    const auto coords = m_coords.empty() ? this->calculate_coords( field.mesh() ) : m_coords;
    const auto values = m_values.empty() ? this->calculate_values( field ) : m_values;
    const auto connections = this->calculate_pyr_connections( field.mesh() );
    const auto nnodes = coords.size() / 3;
    const auto ncells = connections.size() / 5;

    volume->setCellTypeToPyramid();
    volume->setVeclen( 1 );
    volume->setNumberOfNodes( nnodes );
    volume->setNumberOfCells( ncells );
    volume->setValues( values );
    volume->setCoords( coords );
    volume->setConnections( connections );
    if ( ncells > 0 )
    {
        volume->updateMinMaxValues();
        volume->updateMinMaxCoords();
    }
}

inline void FoamToKVS::import_pyr(
    const Foam::volVectorField& field,
    VolumeObject* volume )
{
    const auto coords = m_coords.empty() ? this->calculate_coords( field.mesh() ) : m_coords;
    const auto values = m_values.empty() ? this->calculate_values( field ) : m_values;
    const auto connections = this->calculate_pyr_connections( field.mesh() );
    const auto nnodes = coords.size() / 3;
    const auto ncells = connections.size() / 5;

    volume->setCellTypeToPyramid();
    volume->setVeclen( 1 );
    volume->setNumberOfNodes( nnodes );
    volume->setNumberOfCells( ncells );
    volume->setValues( values );
    volume->setCoords( coords );
    volume->setConnections( connections );
    if ( ncells > 0 )
    {
        volume->updateMinMaxValues();
        volume->updateMinMaxCoords();
    }
}

inline void FoamToKVS::import_pri(
    const Foam::volScalarField& field,
    VolumeObject* volume )
{
    const auto coords = m_coords.empty() ? this->calculate_coords( field.mesh() ) : m_coords;
    const auto values = m_values.empty() ? this->calculate_values( field ) : m_values;
    const auto connections = this->calculate_pri_connections( field.mesh() );
    const auto nnodes = coords.size() / 3;
    const auto ncells = connections.size() / 6;

    volume->setCellTypeToPrism();
    volume->setVeclen( 1 );
    volume->setNumberOfNodes( nnodes );
    volume->setNumberOfCells( ncells );
    volume->setValues( values );
    volume->setCoords( coords );
    volume->setConnections( connections );
    if ( ncells > 0 )
    {
        volume->updateMinMaxValues();
        volume->updateMinMaxCoords();
    }
}

inline void FoamToKVS::import_pri(
    const Foam::volVectorField& field,
    VolumeObject* volume )
{
    const auto coords = m_coords.empty() ? this->calculate_coords( field.mesh() ) : m_coords;
    const auto values = m_values.empty() ? this->calculate_values( field ) : m_values;
    const auto connections = this->calculate_pri_connections( field.mesh() );
    const auto nnodes = coords.size() / 3;
    const auto ncells = connections.size() / 6;

    volume->setCellTypeToPrism();
    volume->setVeclen( 1 );
    volume->setNumberOfNodes( nnodes );
    volume->setNumberOfCells( ncells );
    volume->setValues( values );
    volume->setCoords( coords );
    volume->setConnections( connections );
    if ( ncells > 0 )
    {
        volume->updateMinMaxValues();
        volume->updateMinMaxCoords();
    }
}

/*===========================================================================*/
/**
 *  @brief  Calculates coordinate value array from the specified mesh data.
 *  @param  mesh [in] mesh data
 */
/*===========================================================================*/
inline kvs::ValueArray<kvs::Real32> FoamToKVS::calculate_coords( const Foam::fvMesh& mesh )
{
    std::vector<kvs::Real32> coords;

    // Copy the coordinate values of each grid point from the specified mesh data.
    const size_t nnodes = mesh.nPoints();
    for ( size_t i = 0; i < nnodes; ++i )
    {
        coords.push_back( mesh.points()[i].x() );
        coords.push_back( mesh.points()[i].y() );
        coords.push_back( mesh.points()[i].z() );
    }


    // If the decomposition flag is enabled, additionaly copy the coordinate values
    // at the cell centered point of the polyhedral cell.
    if ( m_decomposition )
    {
        const auto& ukn = *(Foam::cellModeller::lookup("unknown"));
        const auto& cell_shapes = mesh.cellShapes();
        const size_t ncells = cell_shapes.size();
        for ( size_t i = 0; i < ncells; ++i )
        {
            const auto& cell_shape = cell_shapes[i];
            const auto& cell_model = cell_shape.model();
            if ( cell_model == ukn )
            {
                const auto& center = mesh.C()[i];
                coords.push_back( center.x() );
                coords.push_back( center.y() );
                coords.push_back( center.z() );
            }
        }
    }

    return kvs::ValueArray<kvs::Real32>( coords );
}

/*===========================================================================*/
/**
 *  @brief  Calculates physical value array from the specified field data.
 *  @param  field [in] scalar field data
 */
/*===========================================================================*/
inline kvs::ValueArray<kvs::Real32> FoamToKVS::calculate_values(
    const Foam::volScalarField& field )
{
    std::vector<kvs::Real32> values;

    Foam::volPointInterpolation p( field.mesh() );
    const Foam::pointScalarField v = p.interpolate( field );
    for ( int i = 0; i < v.size(); ++i )
    {
        values.push_back( static_cast<kvs::Real32>( v[i] ) );
    }

    if ( m_decomposition )
    {
        const auto& ukn = *(Foam::cellModeller::lookup("unknown"));
        const auto& cell_shapes = field.mesh().cellShapes();
        const size_t ncells = cell_shapes.size();
        for ( size_t i = 0; i < ncells; ++i )
        {
            const auto& cell_shape = cell_shapes[i];
            const auto& cell_model = cell_shape.model();
            if ( cell_model == ukn )
            {
                const auto value = static_cast<kvs::Real32>( field[i] );
                values.push_back( value );
            }
        }
    }

    return kvs::ValueArray<kvs::Real32>( values );
}

/*===========================================================================*/
/**
 *  @brief  Calculates physical value array from the specified field data.
 *  @param  field [in] vector field data
 */
/*===========================================================================*/
inline kvs::ValueArray<kvs::Real32> FoamToKVS::calculate_values(
    const Foam::volVectorField& field )
{
    Foam::volPointInterpolation p( field.mesh() );
    const Foam::pointVectorField v = p.interpolate( field );

    std::vector<kvs::Real32> values;
    for ( int i = 0; i < v.size(); ++i )
    {
        values.push_back( static_cast<kvs::Real32>( Foam::mag( v[i] ) ) );
    }

    if ( m_decomposition )
    {
        const auto& ukn = *(Foam::cellModeller::lookup("unknown"));
        const auto& cell_shapes = field.mesh().cellShapes();
        const size_t ncells = cell_shapes.size();
        for ( size_t i = 0; i < ncells; ++i )
        {
            const auto& cell_shape = cell_shapes[i];
            const auto& cell_model = cell_shape.model();
            if ( cell_model == ukn )
            {
                const auto value = static_cast<kvs::Real32>( Foam::mag( field[i] ) );
                values.push_back( value );
            }
        }
    }

    return kvs::ValueArray<kvs::Real32>( values );
}

inline kvs::ValueArray<kvs::UInt32> FoamToKVS::calculate_hex_connections(
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

inline kvs::ValueArray<kvs::UInt32> FoamToKVS::calculate_tet_connections(
    const Foam::fvMesh& mesh )
{
    std::vector<kvs::UInt32> connections;
    const auto& tet = *(Foam::cellModeller::lookup("tet"));
    const auto& ukn = *(Foam::cellModeller::lookup("unknown"));
    const auto& cell_shapes = mesh.cellShapes();
    const auto& owner = mesh.faceOwner();
    const size_t ncells = cell_shapes.size();
    size_t ukn_cells = 0;
    for ( size_t i = 0; i < ncells; ++i )
    {
        const auto& cell_shape = cell_shapes[i];
        const auto& cell_model = cell_shape.model();
        if ( cell_model == tet )
        {
            const auto& id = cell_shape;
            connections.push_back( static_cast<kvs::UInt32>( id[3] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
        }
        else if ( cell_model == ukn )
        {
            if ( m_decomposition )
            {
                const auto new_id = mesh.nPoints() + ukn_cells;
                const auto& faces = mesh.cells()[i];
                for ( const auto face : faces )
                {
                    const auto& f = mesh.faces()[ face ];
                    const bool is_owner = ( owner[ face ] == int(i) );

                    const auto ntris = f.nTriangles( mesh.points() );
                    Foam::faceList tri_faces( ntris );
                    Foam::label trii = 0;
                    f.triangles( mesh.points(), trii, tri_faces );
                    for ( const auto& id : tri_faces )
                    {
                        connections.push_back( static_cast<kvs::UInt32>( new_id ) );
                        if ( is_owner )
                        {
                            connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
                            connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
                            connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
                        }
                        else
                        {
                            connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
                            connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
                            connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
                        }
                    }
                }
                ukn_cells++;
            }
        }
    }
    return kvs::ValueArray<kvs::UInt32>( connections );
}

inline kvs::ValueArray<kvs::UInt32> FoamToKVS::calculate_pyr_connections(
    const Foam::fvMesh& mesh )
{
    std::vector<kvs::UInt32> connections;
    const auto& pyr = *(Foam::cellModeller::lookup("pyr"));
    const auto& ukn = *(Foam::cellModeller::lookup("unknown"));
    const auto& cell_shapes = mesh.cellShapes();
    const auto& owner = mesh.faceOwner();
    const size_t ncells = cell_shapes.size();
    size_t ukn_cells = 0;
    for ( size_t i = 0; i < ncells; ++i )
    {
        const auto& cell_shape = cell_shapes[i];
        const auto& cell_model = cell_shape.model();
        if ( cell_model == pyr )
        {
            const auto& id = cell_shape;
            connections.push_back( static_cast<kvs::UInt32>( id[4] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[3] ) );
        }
        else if ( cell_model == ukn )
        {
            if ( m_decomposition )
            {
                const auto new_id = mesh.nPoints() + ukn_cells;
                const auto& faces = mesh.cells()[i];
                for ( const auto face : faces )
                {
                    const auto& f = mesh.faces()[ face ];
                    const bool is_owner = ( owner[ face ] == int(i) );

                    Foam::label ntris = 0;
                    Foam::label nquads = 0;
                    f.nTrianglesQuads( mesh.points(), ntris, nquads );

                    Foam::faceList tri_faces( ntris );
                    Foam::faceList quad_faces( nquads );

                    Foam::label trii = 0;
                    Foam::label quadi = 0;
                    f.trianglesQuads( mesh.points(), trii, quadi, tri_faces, quad_faces );

                    for ( const auto& id : quad_faces )
                    {
                        connections.push_back( static_cast<kvs::UInt32>( new_id ) );
                        if ( is_owner )
                        {
                            connections.push_back( static_cast<kvs::UInt32>( id[3] ) );
                            connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
                            connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
                            connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
                        }
                        else
                        {
                            connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
                            connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
                            connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
                            connections.push_back( static_cast<kvs::UInt32>( id[3] ) );
                        }
                    }
                }
                ukn_cells++;
            }
        }
    }
    return kvs::ValueArray<kvs::UInt32>( connections );
}

inline kvs::ValueArray<kvs::UInt32> FoamToKVS::calculate_pri_connections(
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
            connections.push_back( static_cast<kvs::UInt32>( id[3] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[4] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[5] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
        }
        else if ( cell_model == ttwed )
        {
            const auto& id = cell_shape;
            connections.push_back( static_cast<kvs::UInt32>( id[3] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[4] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[0] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[1] ) );
            connections.push_back( static_cast<kvs::UInt32>( id[2] ) );
        }
    }
    return kvs::ValueArray<kvs::UInt32>( connections );
}

#if defined( KVS_SUPPORT_MPI )
void FoamToKVS::update_min_max_coords(
    kvs::mpi::Communicator& world,
    VolumeObject* volume )
{
    kvs::Vec3 global_min( 0, 0, 0 );
    kvs::Vec3 global_max( 0, 0, 0 );
    const auto min_coord = volume->minObjectCoord();
    const auto max_coord = volume->maxObjectCoord();
    world.allReduce( min_coord[0], global_min[0], MPI_MIN );
    world.allReduce( min_coord[1], global_min[1], MPI_MIN );
    world.allReduce( min_coord[2], global_min[2], MPI_MIN );
    world.allReduce( max_coord[0], global_max[0], MPI_MAX );
    world.allReduce( max_coord[1], global_max[1], MPI_MAX );
    world.allReduce( max_coord[2], global_max[2], MPI_MAX );
    volume->setMinMaxObjectCoords( global_min, global_max );
    volume->setMinMaxExternalCoords( global_min, global_max );
}

void FoamToKVS::update_min_max_values(
    kvs::mpi::Communicator& world,
    VolumeObject* volume )
{
    double global_min = 0.0;
    double global_max = 0.0;
    const auto min_value = volume->minValue();
    const auto max_value = volume->maxValue();
    world.allReduce( min_value, global_min, MPI_MIN );
    world.allReduce( max_value, global_max, MPI_MAX );
    volume->setMinMaxValues( global_min, global_max );
}
#endif // KVS_SUPPORT_MPI
