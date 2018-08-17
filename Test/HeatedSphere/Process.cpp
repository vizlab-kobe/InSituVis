#include "Process.h"
#include <kvs/RGBFormulae>
#include <kvs/Camera>
#include <kvs/UnstructuredVolumeImporter>
#include <kvs/Isosurface>
#include <kvs/SlicePlane>
#include <kvs/ExternalFaces>
#include <kvs/Timer>
#include <ParallelImageComposition/Lib/ImageCompositor.h>
#include <cfloat>


namespace local
{

Process::ProcessingTimes Process::ProcessingTimes::reduce( kvs::mpi::Communicator& comm, const MPI_Op op, const int rank ) const
{
    ProcessingTimes times;
    comm.reduce( rank, this->reading, times.reading, op );
    comm.reduce( rank, this->importing, times.importing, op );
    comm.reduce( rank, this->mapping, times.mapping, op );
    comm.reduce( rank, this->rendering, times.rendering, op );
    comm.reduce( rank, this->readback, times.readback, op );
    comm.reduce( rank, this->composition, times.composition, op );
    return times;
}

std::vector<Process::ProcessingTimes> Process::ProcessingTimes::gather( kvs::mpi::Communicator& comm, const int rank ) const
{
    kvs::ValueArray<float> readings; comm.gather( rank, this->reading, readings );
    kvs::ValueArray<float> importings; comm.gather( rank, this->importing, importings );
    kvs::ValueArray<float> mappings; comm.gather( rank, this->mapping, mappings );
    kvs::ValueArray<float> renderings; comm.gather( rank, this->rendering, renderings );
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

Process::FrameBuffer Process::render( const Process::VolumeList& volumes )
{
    // Initialize rendering screen.
    InSituVis::Screen screen;
    screen.setBackgroundColor( kvs::RGBColor::White() );
    screen.setGeometry( 0, 0, m_input.width, m_input.height );
    screen.scene()->camera()->setWindowSize( m_input.width, m_input.height );

    // Mapping parameters.
    kvs::TransferFunction tfunc = kvs::TransferFunction( kvs::RGBFormulae::Hot(256) );
    if ( !m_input.tf_filename.empty() ) { tfunc = kvs::TransferFunction( m_input.tf_filename ); }

    // Mapping.
    kvs::Timer timer( kvs::Timer::Start );
    switch ( m_input.mapping )
    {
    case 0: this->mapping_isosurface( screen, volumes, tfunc ); break;
    case 1: this->mapping_sliceplane( screen, volumes, tfunc ); break;
    case 2: this->mapping_externalfaces( screen, volumes, tfunc ); break;
    default: break;
    }
    timer.stop();
    m_processing_times.mapping = timer.sec();

    // Rendering.
    timer.start();
    screen.draw();
    timer.stop();
    m_processing_times.rendering = timer.sec();

    // Readback.
    timer.start();
    FrameBuffer frame_buffer;
    frame_buffer.color_buffer = screen.readbackColorBuffer();
    frame_buffer.depth_buffer = screen.readbackDepthBuffer();
    timer.stop();
    m_processing_times.readback = timer.sec();

    return frame_buffer;
}

Process::Image Process::compose( const FrameBuffer& frame_buffer )
{
    // MPI parameters.
    const int rank = m_communicator.rank();
    const int nnodes = m_communicator.size();
    const MPI_Comm& comm = m_communicator.handler();

    // Image composition.
    const bool depth_testing = true;
    ParallelImageComposition::ImageCompositor compositor( rank, nnodes, comm );
    compositor.initialize( m_input.width, m_input.height, depth_testing );

    kvs::Timer timer( kvs::Timer::Start );
    kvs::ValueArray<kvs::UInt8> color_buffer = frame_buffer.color_buffer;
    kvs::ValueArray<kvs::Real32> depth_buffer = frame_buffer.depth_buffer;
    compositor.run( color_buffer, depth_buffer );
    timer.stop();
    m_processing_times.composition = timer.sec();

    // Output image.
    const size_t npixels = m_input.width * m_input.height;
    kvs::ValueArray<kvs::UInt8> pixels( npixels * 3 );
    for ( size_t i = 0; i < npixels; i++ )
    {
        pixels[ 3 * i + 0 ] = kvs::Math::Clamp( kvs::Math::Round( color_buffer[ 4 * i + 0 ] ), 0, 255 );
        pixels[ 3 * i + 1 ] = kvs::Math::Clamp( kvs::Math::Round( color_buffer[ 4 * i + 1 ] ), 0, 255 );
        pixels[ 3 * i + 2 ] = kvs::Math::Clamp( kvs::Math::Round( color_buffer[ 4 * i + 2 ] ), 0, 255 );
    }

    return kvs::ColorImage( m_input.width, m_input.height, pixels );
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

void Process::mapping_isosurface(
    InSituVis::Screen& screen,
    const Process::VolumeList& volumes,
    const kvs::TransferFunction& tfunc )
{
    const double iso_level = ( m_max_value - m_min_value ) * 0.2 + m_min_value;
    kvs::PolygonObject::NormalType n = kvs::PolygonObject::PolygonNormal;
    const bool d = true;

    for ( size_t i = 0; i < volumes.size(); i++ )
    {
        const kvs::VolumeObjectBase* input_volume = volumes[i];
        if ( input_volume )
        {
            kvs::PolygonObject* surface = new kvs::Isosurface( input_volume, iso_level, n, d, tfunc );
            surface->setMinMaxObjectCoords( m_min_ext, m_max_ext);
            surface->setMinMaxExternalCoords( m_min_ext, m_max_ext);
            surface->multiplyXform( kvs::Xform::Rotation( kvs::Mat3::RotationY( -30 ) ) );
            surface->multiplyXform( kvs::Xform::Rotation( kvs::Mat3::RotationX( 20 ) ) );
            screen.registerObject( surface );
        }
    }
}

void Process::mapping_sliceplane(
    InSituVis::Screen& screen,
    const Process::VolumeList& volumes,
    const kvs::TransferFunction& tfunc )
{
    const kvs::Vector3f c( ( m_min_ext + m_max_ext)  * 0.4f );
    const kvs::Vector3f p( c );
    const kvs::Vector3f n( 1.0, 0.8, 1.0 );

    for ( size_t i = 0; i < volumes.size(); i++ )
    {
        const kvs::VolumeObjectBase* input_volume = volumes[i];
        if ( input_volume )
        {
            kvs::PolygonObject* slice = new kvs::SlicePlane( input_volume, p, n, tfunc  );
            slice->setMinMaxObjectCoords( m_min_ext, m_max_ext );
            slice->setMinMaxExternalCoords( m_min_ext, m_max_ext );
            slice->multiplyXform( kvs::Xform::Rotation( kvs::Mat3::RotationY( -30 ) ) );
            slice->multiplyXform( kvs::Xform::Rotation( kvs::Mat3::RotationX( 20 ) ) );
            screen.registerObject( slice );
        }
    }
}

void Process::mapping_externalfaces(
    InSituVis::Screen& screen,
    const Process::VolumeList& volumes,
    const kvs::TransferFunction& tfunc )
{
    for ( size_t i = 0; i < volumes.size(); i++ )
    {
        const kvs::VolumeObjectBase* input_volume = volumes[i];
        if ( input_volume )
        {
            kvs::PolygonObject* face = new kvs::ExternalFaces( input_volume );
            face->setMinMaxObjectCoords( m_min_ext, m_max_ext);
            face->setMinMaxExternalCoords( m_min_ext, m_max_ext);
            face->multiplyXform( kvs::Xform::Rotation( kvs::Mat3::RotationY( -30 ) ) );
            face->multiplyXform( kvs::Xform::Rotation( kvs::Mat3::RotationX( 20 ) ) );
            screen.registerObject( face );
        }
    }
}

} // end of namespace local
