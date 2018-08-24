#include "Process.h"
#include "ParticleBasedRenderer.h"
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

Process::ProcessingTimes Process::ProcessingTimes::reduce(
    kvs::mpi::Communicator& comm,
    const MPI_Op op,
    const int rank ) const
{
    ProcessingTimes times;
    comm.reduce( rank, this->reading, times.reading, op );
    comm.reduce( rank, this->importing, times.importing, op );
    comm.reduce( rank, this->mapping, times.mapping, op );
    comm.reduce( rank, this->rendering, times.rendering, op );
    comm.reduce( rank, this->rendering_projection, times.rendering_projection, op );
    comm.reduce( rank, this->rendering_subpixel, times.rendering_subpixel, op );
    comm.reduce( rank, this->readback, times.readback, op );
    comm.reduce( rank, this->composition, times.composition, op );
    return times;
}

std::vector<Process::ProcessingTimes> Process::ProcessingTimes::gather(
    kvs::mpi::Communicator& comm,
    const int rank ) const
{
    kvs::ValueArray<float> readings; comm.gather( rank, this->reading, readings );
    kvs::ValueArray<float> importings; comm.gather( rank, this->importing, importings );
    kvs::ValueArray<float> mappings; comm.gather( rank, this->mapping, mappings );
    kvs::ValueArray<float> renderings; comm.gather( rank, this->rendering, renderings );
    kvs::ValueArray<float> renderings_projection; comm.gather( rank, this->rendering_projection, renderings_projection );
    kvs::ValueArray<float> renderings_subpixel; comm.gather( rank, this->rendering_subpixel, renderings_subpixel );
    kvs::ValueArray<float> readbacks; comm.gather( rank, this->readback, readbacks );
    kvs::ValueArray<float> compositions; comm.gather( rank, this->composition, compositions );

    std::vector<ProcessingTimes> times_list;
    for ( size_t i = 0; i < readings.size(); i++ )
    {
        ProcessingTimes times;
        times.reading = readings[i];
        times.importing = importings[i];
        times.mapping = mappings[i];
        times.rendering = renderings[i];
        times.rendering_projection = renderings_projection[i];
        times.rendering_subpixel = renderings_subpixel[i];
        times.readback = readbacks[i];
        times.composition = compositions[i];
        times_list.push_back( times );
    }

    return times_list;
}

void Process::ProcessingTimes::print( std::ostream& os, const kvs::Indent& indent ) const
{
    os << indent << "Reading time: " << this->reading << " [sec]" << std::endl;
    os << indent << "Importing time: " << this->importing << " [sec]" << std::endl;
    os << indent << "Mapping time: " << this->mapping << " [sec]" << std::endl;
    os << indent << "Rendering time: " << this->rendering << " [sec]" << std::endl;
    os << indent << indent << "projection: " << this->rendering_projection << " [sec]" << std::endl;
    os << indent << indent << "  subpixel: " << this->rendering_subpixel << " [sec]" << std::endl;
    os << indent << "Readback time: " << this->readback << " [sec]" << std::endl;
    os << indent << "Composition time: " << this->composition << " [sec]" << std::endl;
}

Process::Data Process::read()
{
    kvs::Timer timer( kvs::Timer::Start );
    Data data( m_input.filename );
    timer.stop();
    m_processing_times.reading = timer.sec();

    return data;
}

Process::VolumeList Process::import( const Process::Data& data )
{
    const int rank = m_communicator.rank();
    const int nnodes = m_communicator.size();

    kvs::Timer timer( kvs::Timer::Start );

    this->calculate_min_max( data );
    VolumeList volumes;
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
        }
    }

    timer.stop();
    m_processing_times.importing = timer.sec();

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
    compositor.initialize( m_input.width * m_input.subpixels, m_input.height * m_input.subpixels, depth_testing );

    // Mapping.
    kvs::TransferFunction tfunc = kvs::TransferFunction( kvs::RGBFormulae::Hot(256) );
    if ( !m_input.tf_filename.empty() ) { tfunc = kvs::TransferFunction( m_input.tf_filename ); }
    this->mapping( screen, volumes, tfunc );

    // Rendering.
    kvs::Timer timer( kvs::Timer::Start );
    screen.draw();
    timer.stop();
    m_processing_times.rendering_projection = timer.sec();
    m_processing_times.rendering = m_processing_times.rendering_projection;

    // Readback.
    timer.start();
    local::ParticleBasedRenderer* renderer = local::ParticleBasedRenderer::DownCast( screen.scene()->renderer() );
    kvs::ValueArray<kvs::UInt8> subpixelized_color_buffer = renderer->subpixelizedColorBuffer();
    kvs::ValueArray<kvs::Real32> subpixelized_depth_buffer = renderer->subpixelizedDepthBuffer();
    timer.stop();
    m_processing_times.readback = timer.sec();

    // Composition.
    timer.start();
    if ( nnodes > 1 ) { compositor.run( subpixelized_color_buffer, subpixelized_depth_buffer ); }
    timer.stop();
    m_processing_times.composition = timer.sec();

    // Subpixel processing.
    timer.start();
    renderer->subpixelAveraging(
        subpixelized_color_buffer,
        subpixelized_depth_buffer );
    timer.stop();
    m_processing_times.rendering_subpixel = timer.sec();
    m_processing_times.rendering += m_processing_times.rendering_subpixel;

    // Draw image
    timer.start();
    renderer->drawImage();
    timer.stop();
    m_processing_times.rendering += timer.sec();

    // Readback.
    timer.start();
    kvs::ValueArray<kvs::UInt8> color_buffer = screen.readbackColorBuffer();
    timer.stop();
    m_processing_times.readback = timer.sec();

    // Output image.
    const size_t npixels = m_input.width * m_input.height;
    kvs::ValueArray<kvs::UInt8> pixels( npixels * 3 );
    for ( size_t i = 0; i < npixels; i++ )
    {
        pixels[ 3 * i + 0 ] = kvs::Math::Clamp( kvs::Math::Round( color_buffer[ 4 * i + 0 ] ), 0, 255 );
        pixels[ 3 * i + 1 ] = kvs::Math::Clamp( kvs::Math::Round( color_buffer[ 4 * i + 1 ] ), 0, 255 );
        pixels[ 3 * i + 2 ] = kvs::Math::Clamp( kvs::Math::Round( color_buffer[ 4 * i + 2 ] ), 0, 255 );
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
    m_processing_times.mapping = timer.sec();

    // Setup particle renderer.
    local::ParticleBasedRenderer* renderer = new local::ParticleBasedRenderer();
    renderer->disableShading();
    renderer->setSubpixelLevel( m_input.subpixels );
    screen.registerObject( particles, renderer );
}

Process::Particle* Process::generate_particle( const Process::VolumeList& volumes, const kvs::TransferFunction& tfunc )
{
    kvs::PointObject* object = new kvs::PointObject();
    for ( size_t i = 0; i < volumes.size(); i++ )
    {
        const kvs::VolumeObjectBase* volume = volumes[i];
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
    const int repetitions = m_input.subpixels * m_input.subpixels;
    switch ( m_input.sampling_method )
    {
    case 0: return new kvs::CellByCellUniformSampling( volume, repetitions, m_input.step, tfunc );
    case 1: return new kvs::CellByCellMetropolisSampling( volume, repetitions, m_input.step, tfunc );
    case 2: return new kvs::CellByCellRejectionSampling( volume, repetitions, m_input.step, tfunc );
    case 3: return new kvs::CellByCellLayeredSampling( volume, repetitions, m_input.step, tfunc );
    case 4:
    {
        kvs::Camera* camera = new kvs::Camera();
        camera->setWindowSize( m_input.width, m_input.height );
        return new ParticleBasedRendering::CellByCellSubpixelPointSampling(
            camera,
            volume,
            m_input.subpixels,
            m_input.step,
            m_input.base_opacity );
    }
    default: break;
    }

    return NULL;
}

} // end of namespace local
