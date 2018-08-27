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
    comm.reduce( rank, this->transmission, times.transmission, op );
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
    kvs::ValueArray<float> transmissions; comm.gather( rank, this->transmission, transmissions );
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
        times.transmission = transmissions[i];
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
    os << indent << "Ttransmission time: " << this->transmission << " [sec]" << std::endl;
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

kvs::ColorImage Process::FrameBuffer::colorImage() const
{
    const size_t npixels = this->width * this->height;
    kvs::ValueArray<kvs::UInt8> pixels( npixels * 3 );
    for ( size_t i = 0; i < npixels; i++ )
    {
        pixels[ 3 * i + 0 ] = kvs::Math::Clamp( kvs::Math::Round( this->color_buffer[ 4 * i + 0 ] ), 0, 255 );
        pixels[ 3 * i + 1 ] = kvs::Math::Clamp( kvs::Math::Round( this->color_buffer[ 4 * i + 1 ] ), 0, 255 );
        pixels[ 3 * i + 2 ] = kvs::Math::Clamp( kvs::Math::Round( this->color_buffer[ 4 * i + 2 ] ), 0, 255 );
    }

    return kvs::ColorImage( this->width, this->height, pixels );
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

Process::FrameBuffer Process::render( const Process::VolumeList& volumes )
{
    // Initialize rendering screen.
    InSituVis::Screen screen;
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.setGeometry( 0, 0, m_input.width, m_input.height );
    screen.scene()->camera()->setWindowSize( m_input.width, m_input.height );

    // Mapping.
    kvs::TransferFunction tfunc = kvs::TransferFunction( kvs::RGBFormulae::Hot(256) );
    if ( !m_input.tf_filename.empty() ) { tfunc = kvs::TransferFunction( m_input.tf_filename ); }
    this->mapping( screen, volumes, tfunc );

    // Rendering.
    kvs::Timer timer( kvs::Timer::Start );
    screen.draw();
    timer.stop();
    const InSituVis::ParticleBasedRenderer* renderer = InSituVis::ParticleBasedRenderer::DownCast( screen.scene()->renderer() );
    m_times.rendering = timer.sec();
    m_times.rendering_creation = renderer->creationTime();
    m_times.rendering_projection = renderer->projectionTime();
    m_times.rendering_ensemble = renderer->ensembleTime();

    // Readback.
    timer.start();
    FrameBuffer frame_buffer;
    frame_buffer.width = m_input.width;
    frame_buffer.height = m_input.height;
    frame_buffer.color_buffer = screen.readbackColorBuffer();
    frame_buffer.depth_buffer = screen.readbackDepthBuffer();
    timer.stop();
    m_times.readback = timer.sec();

    return frame_buffer;
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
    // Particle generation.
    kvs::Timer timer( kvs::Timer::Start );
    Particle* particles = this->generate_particle( volumes, tfunc );
    timer.stop();
    m_times.mapping = timer.sec();
    m_stats.nparticles = particles->coords().size() / 3;

    // Particle transmission.
    const int nnodes = m_communicator.size();
    const int my_rank = m_communicator.rank();
    const int master_rank = 0;
    timer.start();
    if ( my_rank != master_rank )
    {
        // Send particles to master rank
        m_communicator.send( master_rank, my_rank+100, particles->coords() );
        m_communicator.send( master_rank, my_rank+200, particles->colors() );
        m_communicator.send( master_rank, my_rank+300, particles->normals() );
        m_communicator.send( master_rank, my_rank+400, particles->sizes() );
    }
    else
    {
        // Receive particles from other nodes
        for ( int j = 0; j < nnodes - 1; j++ )
        {
            const int rank = j + 1;
            kvs::ValueArray<kvs::Real32> coords; m_communicator.receive( rank, rank + 100, coords );
            kvs::ValueArray<kvs::UInt8> colors; m_communicator.receive( rank, rank + 200, colors );
            kvs::ValueArray<kvs::Real32> normals; m_communicator.receive( rank, rank + 300, normals );
            kvs::ValueArray<kvs::Real32> sizes; m_communicator.receive( rank, rank + 400, sizes );
            kvs::PointObject receive_particles;
            receive_particles.setCoords( coords );
            receive_particles.setColors( colors );
            receive_particles.setNormals( normals );
            receive_particles.setSizes( sizes );
            receive_particles.updateMinMaxCoords();
            particles->add( receive_particles );
        }
    }
    timer.stop();
    m_times.transmission = timer.sec();

    // Setup particle renderer.
    InSituVis::ParticleBasedRenderer* renderer = new InSituVis::ParticleBasedRenderer();
    renderer->disableShading();
    renderer->setRepetitionLevel( m_input.repetitions );
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
