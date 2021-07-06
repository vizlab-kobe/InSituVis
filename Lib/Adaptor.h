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

    // Simulation time information.
    struct SimTime
    {
        float value = 0.0f;
        size_t index = 0;
        SimTime( float v = 0.0f, size_t i = 0 ): value(v), index(i) {}
    };

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
    size_t m_vis_interval = 1; ///< visualization time interval (l)
    kvs::UInt32 m_time_step = 0; ///< current time step
    kvs::LogStream m_log{}; ///< log stream
    kvs::StampTimer m_step_list{}; ///< time step list
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
    size_t visualizationInterval() const { return m_vis_interval; }
    kvs::StampTimer& stepList() { return m_step_list; }
    kvs::StampTimer& pipeTimer() { return m_pipe_timer; }
    kvs::StampTimer& rendTimer() { return m_rend_timer; }
    kvs::StampTimer& saveTimer() { return m_save_timer; }

    void setViewpoint( const Viewpoint& viewpoint ) { m_viewpoint = viewpoint; }
    void setVisualizationInterval( const size_t interval ) { m_vis_interval = interval; }
    void setPipeline( Pipeline pipeline ) { m_pipeline = pipeline; }
    void setOutputDirectory( const InSituVis::OutputDirectory& directory ) { m_output_directory = directory; }
    void setOutputFilename( const std::string& filename ) { m_output_filename = filename; }
    void setImageSize( const size_t width, const size_t height ) { m_image_width = width; m_image_height = height; }
    void setOutputImageEnabled( const bool enable = true ) { m_enable_output_image = enable; }

    virtual bool initialize();
    virtual bool finalize();
    virtual void put( const Object& Object );
    virtual void exec( const SimTime sim_time );
    virtual bool dump();

protected:
    void execPipeline( const Object& object );
    void execPipeline( const ObjectList& objects );
    void execPipeline();
    void execRendering();

    kvs::UInt32 timeStep() const { return m_time_step; }
    void setTimeStep( const size_t step ) { m_time_step = step; }
    void incrementTimeStep() { m_time_step++; }
    bool canVisualize() const { return m_time_step % m_vis_interval == 0; }
    void clearObjects() { m_objects.clear(); }
    ObjectList& objects() { return m_objects; }

    kvs::Vec2ui outputImageSize( const Viewpoint::Location& location ) const;
    std::string outputImageName( const Viewpoint::Location& location, const std::string& surfix = "" ) const;
    ColorBuffer backgroundColorBuffer() const;
    bool isInsideObject( const kvs::Vec3& position, const kvs::ObjectBase* object ) const;
    ColorBuffer readback( const Viewpoint::Location& location );

private:
    ColorBuffer readback_uni_buffer( const Viewpoint::Location& location );
    ColorBuffer readback_omn_buffer( const Viewpoint::Location& location );
    ColorBuffer readback_adp_buffer( const Viewpoint::Location& location );
};

} // end of namespace InSituVis

#include "Adaptor.hpp"
#include "Adaptor_mpi.h"
