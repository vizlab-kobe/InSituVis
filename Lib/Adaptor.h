/*****************************************************************************/
/**
 *  @file   Adaptor.h
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <functional>
#include <csignal>
#include <list>
#include <kvs/OffScreen>
#include <kvs/ObjectBase>
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
#include <kvs/LogStream>
#include <kvs/StampTimer>
#include <kvs/StampTimerList>
#include "Viewpoint.h"
#include "DistributedViewpoint.h"
#include "OutputDirectory.h"
#include "SphericalBuffer.h"


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
    using Screen = kvs::OffScreen;
    using Object = kvs::ObjectBase;
    using ObjectList = std::list<Object::Pointer>;
    using Pipeline = std::function<void(Screen&,const Object&)>;
    using ColorBuffer = kvs::ValueArray<kvs::UInt8>;

private:
    Screen m_screen{}; ///< rendering screen (off-screen)
    ObjectList m_objects{}; ///< object list
    Pipeline m_pipeline{}; ///< visualization pipeline
    InSituVis::Viewpoint m_viewpoint{}; ///< rendering viewpoint
    InSituVis::OutputDirectory m_output_directory{}; ///< output directory
    std::string m_output_filename = "output"; ///< basename of output file
    size_t m_image_width = 512; ///< width of rendering image
    size_t m_image_height = 512; ///< height of rendering image
    bool m_enable_output_image = true; ///< flag for writing final rendering image data
    size_t m_time_counter = 0; ///< time step counter (t)
    size_t m_time_interval = 1; ///< visualization time interval (dt)
    kvs::UInt32 m_current_time_index = 0; ///< current time index
    kvs::UInt32 m_current_space_index = 0; ///< current space index
    kvs::LogStream m_log{}; ///< log stream
    kvs::StampTimer m_pipe_timer{}; ///< timer for pipeline execution process
    kvs::StampTimer m_rend_timer{}; ///< timer for rendering process
    kvs::StampTimer m_save_timer{}; ///< timer for image saving process

public:
    Adaptor();
    virtual ~Adaptor() = default;

    const std::string& outputFilename() const { return m_output_filename; }
    size_t imageWidth() const { return m_image_width; }
    size_t imageHeight() const { return m_image_height; }
    bool isOutputImageEnabled() const { return m_enable_output_image; }
    std::ostream& log() { return m_log(); }
    std::ostream& log( const bool enable ) { return m_log( enable ); }
    Screen& screen() { return m_screen; }
    const InSituVis::Viewpoint& viewpoint() const { return m_viewpoint; }
    InSituVis::OutputDirectory& outputDirectory() { return m_output_directory; }
    size_t timeCounter() const { return m_time_counter; }
    size_t timeInterval() const { return m_time_interval; }
    kvs::StampTimer& pipeTimer() { return m_pipe_timer; }
    kvs::StampTimer& rendTimer() { return m_rend_timer; }
    kvs::StampTimer& saveTimer() { return m_save_timer; }

    void setViewpoint( const Viewpoint& viewpoint ) { m_viewpoint = viewpoint; }
    void setTimeInterval( const size_t interval ) { m_time_interval = interval; }
    void setPipeline( Pipeline pipeline ) { m_pipeline = pipeline; }
    void setOutputDirectory( const InSituVis::OutputDirectory& directory ) { m_output_directory = directory; }
    void setOutputFilename( const std::string& filename ) { m_output_filename = filename; }
    void setImageSize( const size_t width, const size_t height ) { m_image_width = width; m_image_height = height; }
    void setOutputImageEnabled( const bool enable = true ) { m_enable_output_image = enable; }

    virtual bool initialize();
    virtual bool finalize();
    virtual void put( const Object& Object );
    virtual void exec( const kvs::UInt32 time_index );
    virtual bool dump();

protected:
    void doPipeline( const Object& object );
    void doPipeline( const ObjectList& objects );
    void doPipeline();
    void doRendering();

    kvs::UInt32 currentTimeIndex() const { return m_current_time_index; }
    kvs::UInt32 currentSpaceIndex() const { return m_current_space_index; }
    void setCurrentTimeIndex( const size_t index ) { m_current_time_index = index; }
    void setCurrentSpaceIndex( const size_t index ) { m_current_space_index = index; }
    void incrementTimeCounter() { m_time_counter++; }
    void decrementTimeCounter() { m_time_counter--; }
    bool canVisualize() const { return m_time_counter % m_time_interval == 0; }
    void clearObjects() { m_objects.clear(); }
    ObjectList& objects() { return m_objects; }

    kvs::Vec2ui outputImageSize( const Viewpoint::Point& point ) const;
    std::string outputImageName( const std::string& surfix = "" ) const;
    ColorBuffer backgroundColorBuffer() const;
    bool isInsideObject( const kvs::Vec3& position, const kvs::ObjectBase* object ) const;
    ColorBuffer readback( const Viewpoint::Point& point );

private:
    ColorBuffer readback_plane_buffer( const kvs::Vec3& position );
    ColorBuffer readback_spherical_buffer( const kvs::Vec3& position );
};

} // end of namespace InSituVis

#include "Adaptor.hpp"
#include "Adaptor_mpi.h"
