#include "Process.h"
#include <cmath>
#include <cfloat>
#include <kvs/RGBFormulae>
#include <kvs/Camera>
#include <kvs/PointObject>
#include <kvs/UnstructuredVolumeImporter>
#include <kvs/CellByCellUniformSampling>
#include <kvs/CellByCellMetropolisSampling>
#include <kvs/CellByCellRejectionSampling>
#include <kvs/CellByCellLayeredSampling>
#include <ParallelImageComposition/Lib/ImageCompositor.h>
#include <ParticleBasedRendering/Lib/CellByCellSubpixelPointSampling.h>
#include <InSituVis/Lib/ParticleBasedRenderer.h>


namespace local
{

Process::Times Process::Times::reduce(
    kvs::mpi::Communicator& comm,
    const MPI_Op op,
    const int rank ) const
{
    Times times;
    comm.reduce( rank, this->reading, times.reading, op );
    comm.reduce( rank, this->importing, times.importing, op );
    comm.reduce( rank, this->mapping, times.mapping, op );
    comm.reduce( rank, this->rendering, times.rendering, op );
    comm.reduce( rank, this->rendering_creation, times.rendering_creation, op );
    comm.reduce( rank, this->rendering_projection, times.rendering_projection, op );
    comm.reduce( rank, this->rendering_ensemble, times.rendering_ensemble, op );
    comm.reduce( rank, this->readback, times.readback, op );
    comm.reduce( rank, this->composition, times.composition, op );
    return times;
}

std::vector<Process::Times> Process::Times::gather(
    kvs::mpi::Communicator& comm,
    const int rank ) const
{
    kvs::ValueArray<float> readings; comm.gather( rank, this->reading, readings );
    kvs::ValueArray<float> importings; comm.gather( rank, this->importing, importings );
    kvs::ValueArray<float> mappings; comm.gather( rank, this->mapping, mappings );
    kvs::ValueArray<float> renderings; comm.gather( rank, this->rendering, renderings );
    kvs::ValueArray<float> renderings_creation; comm.gather( rank, this->rendering_creation, renderings_creation );
    kvs::ValueArray<float> renderings_projection; comm.gather( rank, this->rendering_projection, renderings_projection );
    kvs::ValueArray<float> renderings_ensemble; comm.gather( rank, this->rendering_ensemble, renderings_ensemble );
    kvs::ValueArray<float> readbacks; comm.gather( rank, this->readback, readbacks );
    kvs::ValueArray<float> compositions; comm.gather( rank, this->composition, compositions );

    std::vector<Times> times_list;
    for ( size_t i = 0; i < readings.size(); i++ )
    {
        Times times;
        times.reading = readings[i];
        times.importing = importings[i];
        times.mapping = mappings[i];
        times.rendering = renderings[i];
        times.rendering_creation = renderings_creation[i];
        times.rendering_projection = renderings_projection[i];
        times.rendering_ensemble = renderings_ensemble[i];
        times.readback = readbacks[i];
        times.composition = compositions[i];
        times_list.push_back( times );
    }

    return times_list;
}

void Process::Times::print( std::ostream& os, const kvs::Indent& indent ) const
{
    os << indent << "Reading time: " << this->reading << " [sec]" << std::endl;
    os << indent << "Importing time: " << this->importing << " [sec]" << std::endl;
    os << indent << "Mapping time: " << this->mapping << " [sec]" << std::endl;
    os << indent << "Rendering time: " << this->rendering << " [sec]" << std::endl;
    os << indent << indent << "  creation: " << this->rendering_creation << " [sec]" << std::endl;
    os << indent << indent << "projection: " << this->rendering_projection << " [sec]" << std::endl;
    os << indent << indent << "  ensemble: " << this->rendering_ensemble << " [sec]" << std::endl;
    os << indent << "Readback time: " << this->readback << " [sec]" << std::endl;
    os << indent << "Composition time: " << this->composition << " [sec]" << std::endl;
}

Process::Stats Process::Stats::reduce( kvs::mpi::Communicator& comm, const MPI_Op op, const int rank ) const
{
    Stats stats;
    comm.reduce( rank, this->nregions, stats.nregions, op );
    comm.reduce( rank, this->ncells, stats.ncells, op );
    comm.reduce( rank, this->nparticles, stats.nparticles, op );
    return stats;
}

std::vector<Process::Stats> Process::Stats::gather( kvs::mpi::Communicator& comm, const int rank ) const
{
    kvs::ValueArray<int> nregions; comm.gather( rank, this->nregions, nregions );
    kvs::ValueArray<int> ncells; comm.gather( rank, this->ncells, ncells );
    kvs::ValueArray<int> nparticles; comm.gather( rank, this->nparticles, nparticles );

    std::vector<Stats> stats_list;
    for ( size_t i = 0; i < nregions.size(); i++ )
    {
        Stats stats;
        stats.nregions = nregions[i];
        stats.ncells = ncells[i];
        stats.nparticles = nparticles[i];
        stats_list.push_back( stats );
    }

    return stats_list;
}

void Process::Stats::print( std::ostream& os, const kvs::Indent& indent ) const
{
    os << indent << "Number of regions: " << this->nregions << std::endl;
    os << indent << "Number of cells: " << this->ncells <<std::endl;
    os << indent << "Number of particles: " << this->nparticles << std::endl;
}

Process::Process( const local::Input& input, kvs::mpi::Communicator& communicator ):
    m_input( input ),
    m_communicator( communicator )
{
    m_stats.nregions = input.regions;
}

Process::Data Process::read()
{
    kvs::Timer timer( kvs::Timer::Start );
    Data data( m_input.filename );
    timer.stop();
    m_times.reading = timer.sec();

    return data;
}

Process::VolumeList Process::import( const Process::Data& data )
{
    const int rank = m_communicator.rank();
    const int nnodes = m_communicator.size();

    kvs::Timer timer( kvs::Timer::Start );

    this->calculate_min_max( data );
    VolumeList volumes;
    size_t ncells = 0;
    const int nregions = m_input.regions;
    for ( int i = 0; i < nregions; i++ )
    {
        const int gindex = rank + nnodes * i;
        Volume* volume = this->import_volume( data, gindex );
        if ( volume )
        {
            volume->setMinMaxValues( m_min_value, m_max_value );
            volume->setMinMaxExternalCoords( m_min_ext, m_max_ext );
            volumes.push_back( volume );

            ncells += volume->numberOfCells();
        }
    }

    timer.stop();
    m_times.importing = timer.sec();
    m_stats.ncells = ncells;

    return volumes;
}

Process::Image Process::render( const Process::VolumeList& volumes )
{
    // Initialize rendering screen.
    InSituVis::Screen screen;
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.setGeometry( 0, 0, m_input.width, m_input.height );
    screen.scene()->camera()->setWindowSize( m_input.width, m_input.height );

    // Image composititor.
    const int rank = m_communicator.rank();
    const int nnodes = m_communicator.size();
    const MPI_Comm& comm = m_communicator.handler();
    const bool depth_testing = true;
    ParallelImageComposition::ImageCompositor compositor( rank, nnodes, comm );
    compositor.initialize( m_input.width, m_input.height, depth_testing );

    // Ensemble averaging buffer.
    kvs::ValueArray<kvs::Real32> ensemble_buffer( m_input.width * m_input.height * 3 );
    ensemble_buffer.fill( 0.0f );

    // Transfer function.
    kvs::TransferFunction tfunc = kvs::TransferFunction( kvs::RGBFormulae::Hot(256) );
    if ( !m_input.tf_filename.empty() ) { tfunc = kvs::TransferFunction( m_input.tf_filename ); }

    // Ensemble averaging process.
    float mapping = 0;
    float rendering = 0;
    float rendering_creation = 0;
    float rendering_projection = 0;
    float rendering_ensemble = 0;
    float readback = 0;
    float composition = 0;
    const size_t nrepeats = m_input.repetitions;
    for ( size_t i = 0; i < nrepeats; i++ )
    {
        // Mapping.
        this->mapping( screen, volumes, tfunc );
        mapping += m_times.mapping;

        // Rendering.
        kvs::Timer timer( kvs::Timer::Start );
        screen.draw();
        timer.stop();
        const InSituVis::ParticleBasedRenderer* renderer = InSituVis::ParticleBasedRenderer::DownCast( screen.scene()->renderer() );
        rendering += timer.sec();
        rendering_creation += renderer->creationTime();
        rendering_projection += renderer->projectionTime();

        // Readback.
        timer.start();
        kvs::ValueArray<kvs::UInt8> color_buffer = screen.readbackColorBuffer();
        kvs::ValueArray<kvs::Real32> depth_buffer = screen.readbackDepthBuffer();
        timer.stop();
        readback += timer.sec();

        // Composition.
        timer.start();
        if ( nnodes > 1 ) { compositor.run( color_buffer, depth_buffer ); }
        timer.stop();
        composition += timer.sec();

        // Ensemble averaging.
        timer.start();
        const float a = 1.0f / ( i + 1 );
        const int npixels = depth_buffer.size();
        for ( int j = 0; j < npixels; j++ )
        {
            const float r = kvs::Real32( color_buffer[ 4 * j + 0 ] );
            const float g = kvs::Real32( color_buffer[ 4 * j + 1 ] );
            const float b = kvs::Real32( color_buffer[ 4 * j + 2 ] );
            ensemble_buffer[ 3 * j + 0 ] = kvs::Math::Mix( ensemble_buffer[ 3 * j + 0 ], r, a );
            ensemble_buffer[ 3 * j + 1 ] = kvs::Math::Mix( ensemble_buffer[ 3 * j + 1 ], g, a );
            ensemble_buffer[ 3 * j + 2 ] = kvs::Math::Mix( ensemble_buffer[ 3 * j + 2 ], b, a );
        }
        timer.stop();
        rendering_ensemble += timer.sec();
        rendering += timer.sec();
    }

    // Processing times.
    m_times.mapping = mapping;
    m_times.rendering = rendering;
    m_times.rendering_creation = rendering_creation;
    m_times.rendering_projection = rendering_projection;
    m_times.rendering_ensemble = rendering_ensemble;
    m_times.readback = readback;
    m_times.composition = composition;

    // Output image.
    kvs::ValueArray<kvs::UInt8> pixels( m_input.width * m_input.height * 3 );
    for ( size_t i = 0; i < pixels.size(); i++ )
    {
        const int p = kvs::Math::Round( ensemble_buffer[i] );
        pixels[i] = kvs::Math::Clamp( p, 0 , 255 );
    }

    return Image( m_input.width, m_input.height, pixels );
}

Process::Volume* Process::import_volume( const Process::Data& data, const int gindex )
{
    if ( gindex == 76 || gindex == 91 ) return NULL;
    if ( gindex >= 256 ) return NULL;

    const size_t vindex = 3;
    const int etype = kvs::FieldViewData::Pri;
    if ( data.grid(gindex).nelements[ etype ] > 0 )
    {
        data.setImportingElementType( etype );
        data.setImportingGridIndex( gindex );
        data.setImportingVariableIndex( vindex );
        return new kvs::UnstructuredVolumeImporter( &data );
    }

    return NULL;
}

void Process::calculate_min_max( const Process::Data& data )
{
    m_min_ext = kvs::Vec3::All( FLT_MAX );
    m_max_ext = kvs::Vec3::All( FLT_MIN );
    m_min_value = FLT_MAX;
    m_max_value = FLT_MIN;

    const int etype = kvs::FieldViewData::Pri;
    const size_t ngrids = data.numberOfGrids();
    for ( size_t i = 0; i < ngrids; i++ )
    {
        const size_t gindex = i;
        const kvs::FieldViewData::Grid& grid = data.grid( gindex );
        const size_t nelements = grid.nelements[etype];
        if ( nelements > 0 && grid.nelements[1] <= 0 )
        {
            const size_t vindex = 3;
            const size_t nnodes = grid.nodes.size();
            for ( size_t j = 0; j < nnodes; j++ )
            {
                m_min_ext.x() = kvs::Math::Min( m_min_ext.x(), grid.nodes[j].x );
                m_min_ext.y() = kvs::Math::Min( m_min_ext.y(), grid.nodes[j].y );
                m_min_ext.z() = kvs::Math::Min( m_min_ext.z(), grid.nodes[j].z );
                m_max_ext.x() = kvs::Math::Max( m_max_ext.x(), grid.nodes[j].x );
                m_max_ext.y() = kvs::Math::Max( m_max_ext.y(), grid.nodes[j].y );
                m_max_ext.z() = kvs::Math::Max( m_max_ext.z(), grid.nodes[j].z );

                m_min_value = kvs::Math::Min( m_min_value, grid.variables[vindex].data[j] );
                m_max_value = kvs::Math::Max( m_max_value, grid.variables[vindex].data[j] );
            }
        }
    }
}

void Process::mapping(
    InSituVis::Screen& screen,
    const Process::VolumeList& volumes,
    const kvs::TransferFunction& tfunc )
{
    // Generate particles.
    kvs::Timer timer( kvs::Timer::Start );
    Particle* particles = this->generate_particle( volumes, tfunc );
    timer.stop();
    m_times.mapping = timer.sec();
    m_stats.nparticles = particles->coords().size() / 3;

    // Setup particle renderer.
    InSituVis::ParticleBasedRenderer* renderer = new InSituVis::ParticleBasedRenderer();
    renderer->disableShading();
    screen.registerObject( particles, renderer );
}

Process::Particle* Process::generate_particle( const Process::VolumeList& volumes, const kvs::TransferFunction& tfunc )
{
    kvs::PointObject* object = new kvs::PointObject();
    for ( size_t i = 0; i < volumes.size(); i++ )
    {
        const Volume* volume = volumes[i];
        if ( !volume ) { continue; }

        Particle* particles = this->generate_particle( volume, tfunc );
        if ( !particles ) { continue; }

        particles->updateMinMaxCoords();
        object->add( *particles );

        delete particles;
    }

    if ( object )
    {
        if ( m_input.sampling_method == 4 )
        {
            object->setColor( kvs::RGBColor::Black() );
        }

        object->setMinMaxObjectCoords( m_min_ext, m_max_ext);
        object->setMinMaxExternalCoords( m_min_ext, m_max_ext);
    }

    return object;
}

Process::Particle* Process::generate_particle( const Process::Volume* volume, const kvs::TransferFunction& tfunc )
{
    switch ( m_input.sampling_method )
    {
    case 0: return new kvs::CellByCellUniformSampling( volume, m_input.repetitions, m_input.step, tfunc );
    case 1: return new kvs::CellByCellMetropolisSampling( volume, m_input.repetitions, m_input.step, tfunc );
    case 2: return new kvs::CellByCellRejectionSampling( volume, m_input.repetitions, m_input.step, tfunc );
    case 3: return new kvs::CellByCellLayeredSampling( volume, m_input.repetitions, m_input.step, tfunc );
    case 4:
    {
        const size_t subpixel_level = std::sqrt( m_input.repetitions );
        kvs::Camera* camera = new kvs::Camera();
        camera->setWindowSize( m_input.width, m_input.height );
        return new ParticleBasedRendering::CellByCellSubpixelPointSampling(
            camera,
            volume,
            subpixel_level,
            m_input.step,
            m_input.base_opacity );
    }
    default: break;
    }

    return NULL;
}

} // end of namespace local
