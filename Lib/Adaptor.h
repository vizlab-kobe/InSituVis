/*****************************************************************************/
/**
 *  @file   Adaptor.h
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <functional>
#include <kvs/OffScreen>
#include <kvs/UnstructuredVolumeObject>
#include <kvs/ColorImage>
#include <kvs/GrayImage>
#include <kvs/String>
#include <kvs/Type>
#include <kvs/CubicImage>
#include <kvs/SphericalImage>
#include <kvs/Background>
#include <kvs/RotationMatrix33>
#include <kvs/ObjectManager>
#include <kvs/Coordinate>
#include "Viewpoint.h"
#include "DistributedViewpoint.h"
#include "OutputDirectory.h"
#include "SphericalBuffer.h"

#if defined( KVS_SUPPORT_MPI )
#include <kvs/mpi/Communicator>
#include <kvs/mpi/LogStream>
#include <kvs/mpi/ImageCompositor>
#endif


namespace InSituVis
{

/*===========================================================================*/
/**
 *  @brief  Adaptor class.
 */
/*===========================================================================*/
class Adaptor
{
public:
    using Volume = kvs::UnstructuredVolumeObject;
    using Screen = kvs::OffScreen;
    using Pipeline = std::function<void(Screen&,const Volume&)>;
    using ColorBuffer = kvs::ValueArray<kvs::UInt8>;

private:
    Screen m_screen; ///< rendering screen (off-screen)
    Pipeline m_pipeline; ///< visualization pipeline
    InSituVis::Viewpoint m_viewpoint; ///< rendering viewpoint
    InSituVis::OutputDirectory m_output_directory; ///< output directory
    std::string m_output_filename; ///< basename of output file
    size_t m_image_width; ///< width of rendering image
    size_t m_image_height; ///< height of rendering image
    bool m_enable_output_image; ///< flag for writing final rendering image data
    size_t m_time_counter; ///< time step counter (t)
    size_t m_time_interval; ///< visualization time interval (dt)
    kvs::UInt32 m_current_time_index; ///< current time index
    kvs::UInt32 m_current_space_index; ///< current space index

public:
    Adaptor():
        m_output_filename( "output" ),
        m_image_width( 512 ),
        m_image_height( 512 ),
        m_enable_output_image( true ),
        m_time_counter( 0 ),
        m_time_interval( 1 )
    {
    }

    virtual ~Adaptor() {}

    const std::string& outputFilename() const { return m_output_filename; }
    size_t imageWidth() const { return m_image_width; }
    size_t imageHeight() const { return m_image_height; }
    bool isOutputImageEnabled() const { return m_enable_output_image; }
    std::ostream& log() { return std::cout; }
    Screen& screen() { return m_screen; }
    const InSituVis::Viewpoint& viewpoint() const { return m_viewpoint; }
    InSituVis::OutputDirectory& outputDirectory() { return m_output_directory; }
    size_t timeCounter() const { return m_time_counter; }
    size_t timeInterval() const { return m_time_interval; }

    void setViewpoint( const Viewpoint& viewpoint )
    {
        m_viewpoint = viewpoint;
    }

    void setTimeInterval( const size_t interval )
    {
        m_time_interval = interval;
    }

    void setPipeline( Pipeline pipeline )
    {
        m_pipeline = pipeline;
    }

    void setOutputDirectory( const InSituVis::OutputDirectory& directory )
    {
        m_output_directory = directory;
    }

    void setOutputFilename( const std::string& filename )
    {
        m_output_filename = filename;
    }

    void setImageSize( const size_t width, const size_t height )
    {
        m_image_width = width;
        m_image_height = height;
    }

    void setOutputImageEnabled( const bool enable = true )
    {
        m_enable_output_image = enable;
    }

    virtual bool initialize()
    {
        if ( !m_output_directory.create() )
        {
            this->log() << "ERRROR: " << "Cannot create output directory." << std::endl;
            return false;
        }
        m_screen.setSize( m_image_width, m_image_height );
        m_screen.create();
        return true;
    }

    virtual bool finalize()
    {
        return true;
    }

    virtual void put( const Volume& volume )
    {
        if ( this->canVisualize() )
        {
            if ( volume.numberOfCells() == 0 ) return;
            m_pipeline( m_screen, volume );
        }
    }

    virtual void exec( const kvs::UInt32 time_index )
    {
        this->setCurrentTimeIndex( time_index );

        if ( this->canVisualize() )
        {
            const auto npoints = m_viewpoint.numberOfPoints();
            for ( size_t i = 0; i < npoints; ++i )
            {
                this->setCurrentSpaceIndex( i );

                // Draw and readback framebuffer
                const auto& point = m_viewpoint.point( i );
                auto color_buffer = this->readback( point );

                // Output framebuffer to image file
                if ( m_enable_output_image )
                {
                    const auto image_size = this->outputImageSize( point );
                    kvs::ColorImage image( image_size.x(), image_size.y(), color_buffer );
                    image.write( this->outputImageName() );
                }
            }
        }

        this->incrementTimeCounter();
    }

protected:
    kvs::UInt32 currentTimeIndex() const { return m_current_time_index; }
    kvs::UInt32 currentSpaceIndex() const { return m_current_space_index; }
    void setCurrentTimeIndex( const size_t index ) { m_current_time_index = index; }
    void setCurrentSpaceIndex( const size_t index ) { m_current_space_index = index; }
    void incrementTimeCounter() { m_time_counter++; }
    void decrementTimeCounter() { m_time_counter--; }
    bool canVisualize() const { return m_time_counter % m_time_interval == 0; }

    kvs::Vec2ui outputImageSize( const Viewpoint::Point& point ) const
    {
        switch ( point.dir_type )
        {
        case Viewpoint::OmniDir:
            return kvs::Vec2ui( m_image_width * 4, m_image_height * 3 );
        default:
            const auto* object = m_screen.scene()->objectManager();
            if ( this->isInsideObject( point.position, object ) )
            {
                return kvs::Vec2ui( m_image_width * 4, m_image_height * 3 );
            }
            break;
        }
        return kvs::Vec2ui( m_image_width, m_image_height );
    }

    std::string outputImageName( const std::string& surfix = "" ) const
    {
        const auto time = this->currentTimeIndex();
        const auto space = this->currentSpaceIndex();
        const std::string output_time = kvs::String::From( time, 6, '0' );
        const std::string output_space = kvs::String::From( space, 6, '0' );

        const std::string output_basename = m_output_filename;
        const std::string output_filename = output_basename + "_" + output_time + "_" + output_space;
        const std::string filename = m_output_directory.name() + "/" + output_filename + surfix + ".bmp";
        return filename;
    }

    ColorBuffer backgroundColorBuffer() const
    {
        const auto color = m_screen.scene()->background()->color();
        const auto width = m_screen.width();
        const auto height = m_screen.height();
        const size_t npixels = width * height;
        ColorBuffer buffer( npixels * 4 );
        for ( size_t i = 0; i < npixels; ++i )
        {
            buffer[ 4 * i + 0 ] = color.r();
            buffer[ 4 * i + 1 ] = color.g();
            buffer[ 4 * i + 2 ] = color.b();
            buffer[ 4 * i + 3 ] = 255;
        }
        return buffer;
    }

    bool isInsideObject( const kvs::Vec3& position, const kvs::ObjectBase* object ) const
    {
        const auto min_obj = object->minObjectCoord();
        const auto max_obj = object->maxObjectCoord();
        const auto p_obj = kvs::WorldCoordinate( position ).toObjectCoordinate( object ).position();
        return
            ( min_obj.x() <= p_obj.x() ) && ( p_obj.x() <= max_obj.x() ) &&
            ( min_obj.y() <= p_obj.y() ) && ( p_obj.y() <= max_obj.y() ) &&
            ( min_obj.z() <= p_obj.z() ) && ( p_obj.z() <= max_obj.z() );
    }

private:
    ColorBuffer readback( const Viewpoint::Point& point )
    {
        switch ( point.dir_type )
        {
        case Viewpoint::SingleDir:
            return this->readback_plane_buffer( point.position );
        case Viewpoint::OmniDir:
            return this->readback_spherical_buffer( point.position );
        case Viewpoint::AdaptiveDir:
        {
            if ( this->isInsideObject( point.position, m_screen.scene()->objectManager() ) )
            {
                return this->readback_spherical_buffer( point.position );
            }
            return this->readback_plane_buffer( point.position );
        }
        default:
            break;
        }

        return this->backgroundColorBuffer();
    }

    ColorBuffer readback_plane_buffer( const kvs::Vec3& position )
    {
        ColorBuffer color_buffer;
        const auto lookat = m_screen.scene()->camera()->lookAt();
        if ( lookat == position )
        {
            color_buffer = this->backgroundColorBuffer();
        }
        else
        {
            const auto p0 = ( m_screen.scene()->camera()->position() - lookat ).normalized();
            const auto p1 = ( position- lookat ).normalized();
            const auto axis = p0.cross( p1 );
            const auto deg = kvs::Math::Rad2Deg( std::acos( p0.dot( p1 ) ) );
            const auto R = kvs::RotationMatrix33<float>( axis, deg );
            const auto up = m_screen.scene()->camera()->upVector() * R;
            m_screen.scene()->camera()->setPosition( position, lookat, up );
            m_screen.scene()->light()->setPosition( position );
            m_screen.draw();
            color_buffer = m_screen.readbackColorBuffer();
        }
        return color_buffer;
    }

    ColorBuffer readback_spherical_buffer( const kvs::Vec3& position )
    {
        using SphericalColorBuffer = InSituVis::SphericalBuffer<kvs::UInt8>;

        const auto fov = m_screen.scene()->camera()->fieldOfView();
        const auto front = m_screen.scene()->camera()->front();
        const auto pc = m_screen.scene()->camera()->position();
        const auto pl = m_screen.scene()->light()->position();
        const auto& p = position;

        m_screen.scene()->light()->setPosition( position );
        m_screen.scene()->camera()->setFieldOfView( 90.0 );
        m_screen.scene()->camera()->setFront( 0.1 );

        SphericalColorBuffer color_buffer( m_screen.width(), m_screen.height() );
        for ( size_t i = 0; i < SphericalColorBuffer::Direction::NumberOfDirections; i++ )
        {
            const auto d = SphericalColorBuffer::Direction(i);
            const auto dir = SphericalColorBuffer::DirectionVector(d);
            const auto up = SphericalColorBuffer::UpVector(d);
            m_screen.scene()->camera()->setPosition( p, p + dir, up );
            m_screen.draw();

            const auto buffer = m_screen.readbackColorBuffer();
            color_buffer.setBuffer( d, buffer );
        }

        m_screen.scene()->camera()->setFieldOfView( fov );
        m_screen.scene()->camera()->setFront( front );
        m_screen.scene()->camera()->setPosition( pc );
        m_screen.scene()->light()->setPosition( pl );

        const size_t nchannels = 4; // rgba
        return color_buffer.stitch<nchannels>();
    }
};

#if defined( KVS_SUPPORT_MPI )
namespace mpi
{

/*===========================================================================*/
/**
 *  @brief  Adaptor class for parallel rendering based on MPI.
 */
/*===========================================================================*/
class Adaptor : public InSituVis::Adaptor
{
public:
    using BaseClass = InSituVis::Adaptor;
    using DepthBuffer = kvs::ValueArray<kvs::Real32>;
    struct FrameBuffer { ColorBuffer color_buffer; DepthBuffer depth_buffer; };

private:
    kvs::mpi::Communicator m_world; ///< MPI communicator
    kvs::mpi::LogStream m_log; ///< MPI log stream
    kvs::mpi::ImageCompositor m_compositor; ///< image compositor
    bool m_enable_output_subimage; ///< flag for writing sub-volume rendering image
    bool m_enable_output_subimage_depth; ///< flag for writing sub-volume rendering image (depth image)
    bool m_enable_output_subimage_alpha; ///< flag for writing sub-volume rendering image (alpha image)

public:
    Adaptor( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ):
        InSituVis::Adaptor(),
        m_world( world, root ),
        m_log( m_world ),
        m_compositor( m_world ),
        m_enable_output_subimage( false ),
        m_enable_output_subimage_depth( false ),
        m_enable_output_subimage_alpha( false )
    {
    }

    virtual ~Adaptor() {}

    kvs::mpi::Communicator& world() { return m_world; }
    std::ostream& log() { return m_log( m_world.root() ); }
    std::ostream& log( const int rank ) { return m_log( rank ); }

    void setOutputSubImageEnabled(
        const bool enable = true,
        const bool enable_depth = false,
        const bool enable_alpha = false )
    {
        m_enable_output_subimage = enable;
        m_enable_output_subimage_depth = enable_depth;
        m_enable_output_subimage_alpha = enable_alpha;
    }

    virtual bool initialize()
    {
        if ( !BaseClass::outputDirectory().create( m_world ) )
        {
            this->log() << "ERROR: " << "Cannot create output directories." << std::endl;
            return false;
        }

        const bool depth_testing = true;
        const auto width = BaseClass::imageWidth();
        const auto height = BaseClass::imageHeight();
        if ( !m_compositor.initialize( width, height, depth_testing ) )
        {
            this->log() << "ERROR: " << "Cannot initialize image compositor." << std::endl;
            return false;
        }

        BaseClass::screen().setSize( width, height );
        BaseClass::screen().create();

        return true;
    }

    virtual bool finalize()
    {
        return m_compositor.destroy();
    }

    virtual void exec( const kvs::UInt32 time_index )
    {
        BaseClass::setCurrentTimeIndex( time_index );

        if ( BaseClass::canVisualize() )
        {
            const auto npoints = BaseClass::viewpoint().numberOfPoints();
            for ( size_t i = 0; i < npoints; ++i )
            {
                BaseClass::setCurrentSpaceIndex( i );

                // Draw and readback framebuffer
                const auto& point = BaseClass::viewpoint().point( i );
                auto frame_buffer = this->readback( point );

                // Output framebuffer to image file
                if ( m_world.rank() == m_world.root() )
                {
                    if ( BaseClass::isOutputImageEnabled() )
                    {
                        const auto image_size = BaseClass::outputImageSize( point );
                        kvs::ColorImage image( image_size.x(), image_size.y(), frame_buffer.color_buffer );
//                        image.write( this->outputImageName() );
                        image.write( this->outputFinalImageName() );
                    }
                }
            }
        }
        BaseClass::incrementTimeCounter();
    }

private:
    std::string outputFinalImageName()
    {
        const auto time = BaseClass::currentTimeIndex();
        const auto space = BaseClass::currentSpaceIndex();
        const std::string output_time = kvs::String::From( time, 6, '0' );
        const std::string output_space = kvs::String::From( space, 6, '0' );

        const std::string output_basename = BaseClass::outputFilename();
        const std::string output_filename = output_basename + "_" + output_time + "_" + output_space;
//        const std::string filename = m_output_directory.name() + "/" + output_filename + surfix + ".bmp";
        const std::string filename = BaseClass::outputDirectory().baseDirectoryName() + "/" + output_filename + ".bmp";
        return filename;
    }

    DepthBuffer backgroundDepthBuffer()
    {
        const auto width = BaseClass::screen().width();
        const auto height = BaseClass::screen().height();
        DepthBuffer buffer( width * height );
        buffer.fill(0);
        return buffer;
    }

    FrameBuffer readback( const Viewpoint::Point& point )
    {
        switch ( point.dir_type )
        {
        case Viewpoint::SingleDir:
            return this->readback_plane_buffer( point.position );
        case Viewpoint::OmniDir:
            return this->readback_spherical_buffer( point.position );
        case Viewpoint::AdaptiveDir:
        {
            if ( BaseClass::isInsideObject( point.position, BaseClass::screen().scene()->objectManager() ) )
            {
                return this->readback_spherical_buffer( point.position );
            }
            else
            {
                return this->readback_plane_buffer( point.position );
            }
        }
        default:
            break;
        }

        FrameBuffer frame_buffer;
        frame_buffer.color_buffer = BaseClass::backgroundColorBuffer();
        frame_buffer.depth_buffer = this->backgroundDepthBuffer();
        return frame_buffer;
    }

    FrameBuffer readback_plane_buffer( const kvs::Vec3& position )
    {
        FrameBuffer frame_buffer;
        const auto lookat = BaseClass::screen().scene()->camera()->lookAt();
        if ( lookat == position )
        {
            frame_buffer.color_buffer = BaseClass::backgroundColorBuffer();
            frame_buffer.depth_buffer = this->backgroundDepthBuffer();
        }
        else
        {
            // Draw image
            const auto p0 = ( BaseClass::screen().scene()->camera()->position() - lookat ).normalized();
            const auto p1 = ( position- lookat ).normalized();
            const auto axis = p0.cross( p1 );
            const auto deg = kvs::Math::Rad2Deg( std::acos( p0.dot( p1 ) ) );
            const auto R = kvs::RotationMatrix33<float>( axis, deg );
            const auto up = BaseClass::screen().scene()->camera()->upVector() * R;
            BaseClass::screen().scene()->camera()->setPosition( position, lookat, up );
            BaseClass::screen().scene()->light()->setPosition( position );
            BaseClass::screen().draw();

            // Read-back image
            auto color_buffer = BaseClass::screen().readbackColorBuffer();
            auto depth_buffer = BaseClass::screen().readbackDepthBuffer();

            // Output rendering image (partial rendering image)
            if ( m_enable_output_subimage )
            {
                // RGB image
                const auto width = BaseClass::imageWidth();
                const auto height = BaseClass::imageHeight();
                kvs::ColorImage image( width, height, color_buffer );
                image.write( BaseClass::outputImageName( "_color") );

                // Depth image
                if ( m_enable_output_subimage_depth )
                {
                    kvs::GrayImage depth_image( width, height, depth_buffer );
                    depth_image.write( BaseClass::outputImageName( "_depth" ) );
                }

                // Alpha image
                if ( m_enable_output_subimage_alpha )
                {
                    kvs::GrayImage alpha_image( width, height, color_buffer, 3 );
                    alpha_image.write( BaseClass::outputImageName( "_alpha" ) );
                }
            }

            // Image composition
            if ( !m_compositor.run( color_buffer, depth_buffer ) )
            {
                this->log() << "ERROR: " << "Cannot compose images." << std::endl;
            }

            frame_buffer.color_buffer = color_buffer;
            frame_buffer.depth_buffer = depth_buffer;
        }

        return frame_buffer;
    }

    FrameBuffer readback_spherical_buffer( const kvs::Vec3& position )
    {
        using SphericalColorBuffer = InSituVis::SphericalBuffer<kvs::UInt8>;
        using SphericalDepthBuffer = InSituVis::SphericalBuffer<kvs::Real32>;

        const auto fov = BaseClass::screen().scene()->camera()->fieldOfView();
        const auto front = BaseClass::screen().scene()->camera()->front();
        const auto pc = BaseClass::screen().scene()->camera()->position();
        const auto pl = BaseClass::screen().scene()->light()->position();
        const auto& p = position;

        BaseClass::screen().scene()->light()->setPosition( position );
        BaseClass::screen().scene()->camera()->setFieldOfView( 90.0 );
        BaseClass::screen().scene()->camera()->setFront( 0.1 );

        SphericalColorBuffer color_buffer( BaseClass::screen().width(), BaseClass::screen().height() );
        SphericalDepthBuffer depth_buffer( BaseClass::screen().width(), BaseClass::screen().height() );
        for ( size_t i = 0; i < SphericalColorBuffer::Direction::NumberOfDirections; i++ )
        {
            const auto d = SphericalColorBuffer::Direction(i);
            const auto dir = SphericalColorBuffer::DirectionVector(d);
            const auto up = SphericalColorBuffer::UpVector(d);
            BaseClass::screen().scene()->camera()->setPosition( p, p + dir, up );
            BaseClass::screen().draw();

            auto cbuffer = BaseClass::screen().readbackColorBuffer();
            auto dbuffer = BaseClass::screen().readbackDepthBuffer();

            // Output rendering image (partial rendering image) for each direction
            if ( m_enable_output_subimage )
            {
                // RGB image
                const auto dname = SphericalColorBuffer::DirectionName(d);
                const auto width = BaseClass::imageWidth();
                const auto height = BaseClass::imageHeight();
                kvs::ColorImage image( width, height, cbuffer );
                image.write( BaseClass::outputImageName( "_color_" + dname ) );

                // Depth image
                if ( m_enable_output_subimage_depth )
                {
                    kvs::GrayImage depth_image( width, height, dbuffer );
                    depth_image.write( BaseClass::outputImageName( "_depth_" + dname ) );
                }

                // Alpha image
                if ( m_enable_output_subimage_alpha )
                {
                    kvs::GrayImage alpha_image( width, height, cbuffer, 3 );
                    alpha_image.write( BaseClass::outputImageName( "_alpha_" + dname ) );
                }
            }

            // Image composition
            if ( !m_compositor.run( cbuffer, dbuffer ) )
            {
                this->log() << "ERROR: " << "Cannot compose images." << std::endl;
            }

            color_buffer.setBuffer( d, cbuffer );
            depth_buffer.setBuffer( d, dbuffer );
        }

        BaseClass::screen().scene()->camera()->setFieldOfView( fov );
        BaseClass::screen().scene()->camera()->setFront( front );
        BaseClass::screen().scene()->camera()->setPosition( pc );
        BaseClass::screen().scene()->light()->setPosition( pl );

        FrameBuffer frame_buffer;
        frame_buffer.color_buffer = color_buffer.stitch<4>();
        frame_buffer.depth_buffer = depth_buffer.stitch<1>();
        return frame_buffer;
    }
};

} // end of namespace mpi

#endif // KVS_SUPPORT_MPI

} // end of namespace InSituVis
