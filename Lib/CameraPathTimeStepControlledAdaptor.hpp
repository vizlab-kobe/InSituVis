#include <kvs/ColorImage>
#include <kvs/RGBColor>


namespace InSituVis
{


inline bool CameraPathTimeStepControlledAdaptor::isEntropyStep()
{
    return BaseClass::timeStep() % ( BaseClass::analysisInterval() * Controller::entropyInterval() ) == 0;
}

inline bool CameraPathTimeStepControlledAdaptor::isFinalTimeStep()
{
    return BaseClass::timeStep() == m_final_time_step;
}

inline CameraPathTimeStepControlledAdaptor::Location
CameraPathTimeStepControlledAdaptor::erpLocation(
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


inline bool CameraPathTimeStepControlledAdaptor::dump()
{
    bool ret = true;
    if ( m_entr_timer.title().empty() ) { m_entr_timer.setTitle( "Ent time" ); }
    kvs::StampTimerList entr_timer_list;
    entr_timer_list.push( m_entr_timer );

    const auto basedir = BaseClass::outputDirectory().baseDirectoryName() + "/";
    ret = entr_timer_list.write( basedir + "ent_proc_time.csv" );

    const auto interval = BaseClass::analysisInterval();
    const auto directory = BaseClass::outputDirectory();
    const auto File = [&]( const std::string& name ) { return Controller::logDataFilename( name, directory ); };
    // Controller::outputPathEntropies( File( "output_path_entropies" ), interval );
    // Controller::outputPathPositions( File( "output_path_positions"), interval );
    Controller::outputPathCalcTimes( File( "output_path_calc_times" ) );
    Controller::outputDivergences("Output/output_divergences.csv",Controller::divergences(),Controller::threshold());
    Controller::outputViewpointCoords( File( "output_viewpoint_coords" ), BaseClass::viewpoint() );


    return BaseClass::dump() && ret;
}

inline void CameraPathTimeStepControlledAdaptor::exec( const BaseClass::SimTime sim_time )
{
    Controller::setCacheEnabled( BaseClass::isAnalysisStep() );
    Controller::setIsEntStep( this->isEntropyStep() );
    Controller::updataCacheSize();
    
    Controller::push( BaseClass::objects() );

    BaseClass::incrementTimeStep();
    if( this->isFinalTimeStep())
    {
        Controller::setFinalStep( true );
        const auto dummy = Data();
        Controller::push( dummy );
    }
    BaseClass::clearObjects();
}

inline void CameraPathTimeStepControlledAdaptor::execRendering()
{
    BaseClass::setRendTime( 0.0f );
    BaseClass::setCompTime( 0.0f );
    float save_time = 0.0f;
    float entr_time = 0.0f;

    float max_entropy = -1.0f;
    int max_index = 0;

    std::vector<float> entropies;
    std::vector<FrameBuffer> frame_buffers;

    // if ( this->isEntropyStep() )
    if ( Controller::isValidationStep() )
    {
        // Entropy evaluation
        for ( const auto& location : BaseClass::viewpoint().locations() )
        {
            // Draw and readback framebuffer
            auto frame_buffer = BaseClass::readback( location ); 

            // Output framebuffer to image file at the root node
            kvs::Timer timer( kvs::Timer::Start );

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
            
            timer.stop();
            entr_time += BaseClass::saveTimer().time( timer );
        }

        // Distribute the index indicates the max entropy image //並列計算関係

        const auto max_position = BaseClass::viewpoint().at( max_index ).position;
        const auto max_rotation = BaseClass::viewpoint().at( max_index ).rotation;
        Controller::setMaxIndex( max_index );
        Controller::setMaxPosition( max_position );
        Controller::setMaxRotation( max_rotation );
        Controller::setMaxEntropy( max_entropy );

        // Output the rendering images and the heatmap of entropies.
        kvs::Timer timer( kvs::Timer::Start );

        if ( BaseClass::isOutputImageEnabled() )
        {
            const auto index = Controller::maxIndex();
            const auto& location = BaseClass::viewpoint().at( index );
            const auto& frame_buffer = frame_buffers[ index ];
            this->outputColorImage( location, frame_buffer );
            //this->outputDepthImage( location, frame_buffer );
        }

        if ( Controller::isOutputEntropiesEnabled() )
        {
            const auto basename = "output_entropies_";
            const auto timestep = BaseClass::timeStep();
            const auto directory = BaseClass::outputDirectory();
            const auto filename = Controller::logDataFilename( basename, timestep, directory );
            Controller::outputEntropies( filename, entropies );
        }
        
        timer.stop();
        save_time += BaseClass::saveTimer().time( timer );
    }
    else
    {
        const auto location = this->erpLocation();
        auto frame_buffer = BaseClass::readback( location );
        const auto path_entropy = Controller::entropy( frame_buffer );
        Controller::setMaxEntropy( path_entropy );
        Controller::setMaxPosition( location.position );

        // Output the rendering images.
        kvs::Timer timer( kvs::Timer::Start );
        
        if ( BaseClass::isOutputImageEnabled() )
        {
            this->outputColorImage( location, frame_buffer );
            //this->outputDepthImage( location, frame_buffer );
        }
        
        timer.stop();
        save_time += BaseClass::saveTimer().time( timer );
    }
    m_entr_timer.stamp( entr_time );
    BaseClass::saveTimer().stamp( save_time );
    BaseClass::rendTimer().stamp( BaseClass::rendTime() );
    BaseClass::compTimer().stamp( BaseClass::compTime() );
}

inline void CameraPathTimeStepControlledAdaptor::process( const Data& data )
{
    BaseClass::execPipeline( data );
    this->execRendering();
}

inline void CameraPathTimeStepControlledAdaptor::process( const Data& data, const float radius, const kvs::Quaternion& rotation )
{
    const auto current_step = BaseClass::timeStep();
    {
        // Reset time step, which is used for output filename,
        // for visualizing the stacked dataset.
        const auto L_crr = Controller::dataQueue().size();
        // const auto L_crr = Controller::dataQueueSize() - 1;
        if ( L_crr > 0 )
        {
            const auto l = BaseClass::analysisInterval();
            const auto step = current_step - L_crr * l;
            BaseClass::setTimeStep( step );
        }

        // Stack current time step.
        const auto step = static_cast<float>( BaseClass::timeStep() );
        BaseClass::tstepList().stamp( step );

        // Execute vis. pipeline and rendering.
        Controller::setErpRotation( rotation );
        Controller::setErpRadius( radius );
        BaseClass::execPipeline( data );
        this->execRendering();
    }
    BaseClass::setTimeStep( current_step );
}

inline std::string CameraPathTimeStepControlledAdaptor::outputDepthImageName( const Viewpoint::Location& location )
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

inline void CameraPathTimeStepControlledAdaptor::outputColorImage(
    const InSituVis::Viewpoint::Location& location,
    const FrameBuffer& frame_buffer )
{
    const auto size = BaseClass::outputImageSize( location );
    const auto buffer = frame_buffer.color_buffer;
    kvs::ColorImage image( size.x(), size.y(), buffer );
    image.write( BaseClass::outputFinalImageName( location ) );
}

inline void CameraPathTimeStepControlledAdaptor::outputDepthImage(
    const InSituVis::Viewpoint::Location& location,
    const FrameBuffer& frame_buffer )
{
    const auto size = BaseClass::outputImageSize( location );
    const auto buffer = frame_buffer.depth_buffer;
    kvs::GrayImage image( size.x(), size.y(), buffer );
    image.write( this->outputDepthImageName( location ) );
}


} // end of namespace InSituVis
