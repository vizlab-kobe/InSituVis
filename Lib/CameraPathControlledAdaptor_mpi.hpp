#include <kvs/ColorImage>
#include <kvs/RGBColor>


namespace InSituVis
{

namespace mpi
{

inline bool CameraPathControlledAdaptor::isEntropyStep()
{
    return BaseClass::timeStep() % ( BaseClass::analysisInterval() * Controller::entropyInterval() ) == 0;
}

inline bool CameraPathControlledAdaptor::isFinalTimeStep()
{
    return BaseClass::timeStep() == m_final_time_step;
}

inline CameraPathControlledAdaptor::Location
CameraPathControlledAdaptor::erpLocation(
    const size_t index,
    const Viewpoint::Direction dir )
{
    const auto rad = Controller::erpRadius();
    const auto rot = Controller::erpRotation();
    const auto p = kvs::Quat::Rotate( kvs::Vec3( { 0.0f, rad,   0.0f } ), rot );
    const auto u = kvs::Quat::Rotate( kvs::Vec3( { 0.0f, 0.0f, -1.0f } ), rot );
    const auto l = kvs::Vec3( { 0.0f, 0.0f, 0.0f } );
    return { index, dir, p, u, rot, l };
}

inline bool CameraPathControlledAdaptor::dump()
{
    bool ret = true;
    if ( BaseClass::world().isRoot() )
    {
        if ( m_entr_timer.title().empty() ) { m_entr_timer.setTitle( "Ent time" ); }
        kvs::StampTimerList entr_timer_list;
        entr_timer_list.push( m_entr_timer );

        const auto basedir = BaseClass::outputDirectory().baseDirectoryName() + "/";
        ret = entr_timer_list.write( basedir + "ent_proc_time.csv" );

        const auto directory = BaseClass::outputDirectory();
        const auto File = [&]( const std::string& name ) { return Controller::logDataFilename( name, directory ); };
        Controller::outputPathCalcTimes( File( "output_path_calc_times" ) );
        Controller::outputViewpointCoords( File( "output_viewpoint_coords" ), BaseClass::viewpoint() );
        Controller::outputNumImages( File( "output_num_images" ), BaseClass::analysisInterval() );
    }

    return BaseClass::dump() && ret;
}

inline void CameraPathControlledAdaptor::exec( const BaseClass::SimTime sim_time )
{
    Controller::setCacheEnabled( BaseClass::isAnalysisStep() );
    Controller::setIsEntStep( this->isEntropyStep() );
    Controller::push( BaseClass::objects() );

    BaseClass::incrementTimeStep();
    if( this->isFinalTimeStep())
    {
        Controller::setIsFinalStep( true );
        const auto dummy = Data();
        Controller::push( dummy );
    }
    BaseClass::clearObjects();
}

inline void CameraPathControlledAdaptor::execRendering()
{
    BaseClass::setRendTime( 0.0f );
    BaseClass::setCompTime( 0.0f );
    float save_time = 0.0f;
    float entr_time = 0.0f;

    float max_entropy = -1.0f;
    int max_index = 0;

    std::vector<float> entropies;
    std::vector<FrameBuffer> frame_buffers;

    if ( Controller::isEntStep() && !Controller::isErpStep() )
    {
        // Entropy evaluation
        for ( const auto& location : BaseClass::viewpoint().locations() )
        {
            // Draw and readback framebuffer
            auto frame_buffer = BaseClass::readback( location );

            // Output framebuffer to image file at the root node
            kvs::Timer timer( kvs::Timer::Start );
            if ( BaseClass::world().isRoot() )
            {
                const auto entropy = Controller::entropy( frame_buffer );
                entropies.push_back( entropy );
                frame_buffers.push_back( frame_buffer );

                if ( entropy > max_entropy )
                {
                    max_entropy = entropy;
                    max_index = location.index;
                }

                if ( m_enable_output_evaluation_image )
                {
                    this->outputColorImage( location, frame_buffer );
                }

                if ( m_enable_output_evaluation_image_depth )
                {
                    this->outputDepthImage( location, frame_buffer );
                }
            }
            timer.stop();
            entr_time += BaseClass::saveTimer().time( timer );
        }

        // Output entropies (entropy heatmap)
        if ( BaseClass::world().isRoot() )
        {
            if ( Controller::isOutputEntropiesEnabled() )
            {
                const auto basename = "output_entropies_";
                const auto timestep = BaseClass::timeStep();
                const auto directory = BaseClass::outputDirectory();
                const auto filename = Controller::logDataFilename( basename, timestep, directory );
                Controller::outputEntropies( filename, entropies );
            }
        }

        // Distribute the index indicates the max entropy image
        BaseClass::world().broadcast( max_index );
        BaseClass::world().broadcast( max_entropy );
        const auto& max_location = BaseClass::viewpoint().at( max_index );
        const auto max_position = max_location.position;
        const auto max_rotation = max_location.rotation;
        Controller::setMaxIndex( max_index );
        Controller::setMaxPosition( max_position );
        Controller::setMaxRotation( max_rotation );
        Controller::setMaxEntropy( max_entropy );

        // Output the rendering images and the heatmap of entropies.
        kvs::Timer timer( kvs::Timer::Start );
        if ( BaseClass::world().isRoot() )
        {
            if ( BaseClass::isOutputImageEnabled() )
            {
                const auto index = Controller::maxIndex();
                const auto& location = BaseClass::viewpoint().at( index );
                const auto& frame_buffer = frame_buffers[ index ];
                this->outputColorImage( location, frame_buffer );
                //this->outputDepthImage( location, frame_buffer );
            }
        }
        timer.stop();
        save_time += BaseClass::saveTimer().time( timer );
        m_entr_timer.stamp( entr_time );
    }
    else
    {
        const auto location = this->erpLocation();
        auto frame_buffer = BaseClass::readback( location );

        // Output the rendering images.
        kvs::Timer timer( kvs::Timer::Start );
        if ( BaseClass::world().isRoot() )
        {
            if ( BaseClass::isOutputImageEnabled() )
            {
                this->outputColorImage( location, frame_buffer );
                //this->outputDepthImage( location, frame_buffer );
            }
        }
        timer.stop();
        save_time += BaseClass::saveTimer().time( timer );
    }

    BaseClass::saveTimer().stamp( save_time );
    BaseClass::rendTimer().stamp( BaseClass::rendTime() );
    BaseClass::compTimer().stamp( BaseClass::compTime() );
}

inline void CameraPathControlledAdaptor::process( const Data& data )
{
    BaseClass::execPipeline( data );
    this->execRendering();
}

inline void CameraPathControlledAdaptor::process( const Data& data, const float radius, const kvs::Quaternion& rotation )
{
    const auto current_step = BaseClass::timeStep();

    // Reset time step, which is used for output filename,
    // for visualizing the stacked dataset.
    const auto l = Controller::dataQueue().size();
    const auto interval = BaseClass::analysisInterval();
    const auto step = current_step - l * interval;
    BaseClass::setTimeStep( step );
    BaseClass::tstepList().stamp( static_cast<float>( step ) );

    // Execute vis. pipeline and rendering.
    Controller::setErpRotation( rotation );
    Controller::setErpRadius( radius );
    BaseClass::execPipeline( data );
    this->execRendering();

    BaseClass::setTimeStep( current_step );
}

inline std::string CameraPathControlledAdaptor::outputColorImageName( const Viewpoint::Location& location )
{
    const auto time = BaseClass::timeStep();
    const auto sub_time = Controller::subTimeIndex();
    const auto space = location.index;
    const auto output_time = kvs::String::From( time, 6, '0' );
    const auto output_sub_time = kvs::String::From( sub_time, 6, '0' );
    const auto output_space = kvs::String::From( space, 6, '0' );

    const auto output_basename = BaseClass::outputFilename();
    const auto output_filename = output_basename + "_" + output_time + "_" + output_sub_time+ "_" + output_space;
    const auto filename = BaseClass::outputDirectory().baseDirectoryName() + "/" + output_filename + ".bmp";
    // const auto filename = BaseClass::outputDirectory().baseDirectoryName() + "/" + output_filename + ".jpg";
    // const auto filename = BaseClass::outputDirectory().baseDirectoryName() + "/" + output_filename + ".png";
    return filename;
}

inline std::string CameraPathControlledAdaptor::outputDepthImageName( const Viewpoint::Location& location )
{
    const auto time = BaseClass::timeStep();
    const auto space = location.index;
    const auto output_time = kvs::String::From( time, 6, '0' );
    const auto output_space = kvs::String::From( space, 6, '0' );

    const auto output_basename = BaseClass::outputFilename();
    const auto output_filename = output_basename + "_depth_" + output_time + "_" + output_space;
    const auto filename = BaseClass::outputDirectory().baseDirectoryName() + "/" + output_filename + ".bmp";
    return filename;
}

inline void CameraPathControlledAdaptor::outputColorImage(
    const InSituVis::Viewpoint::Location& location,
    const FrameBuffer& frame_buffer )
{
    const auto size = BaseClass::outputImageSize( location );
    const auto buffer = frame_buffer.color_buffer;
    kvs::ColorImage image( size.x(), size.y(), buffer );
    image.write( this->outputColorImageName( location ) );
}

inline void CameraPathControlledAdaptor::outputDepthImage(
    const InSituVis::Viewpoint::Location& location,
    const FrameBuffer& frame_buffer )
{
    const auto size = BaseClass::outputImageSize( location );
    const auto buffer = frame_buffer.depth_buffer;
    kvs::GrayImage image( size.x(), size.y(), buffer );
    image.write( this->outputDepthImageName( location ) );
}

} // end of namespace mpi

} // end of namespace InSituVis
