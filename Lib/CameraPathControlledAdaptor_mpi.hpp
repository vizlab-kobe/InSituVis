#include <kvs/ColorImage>
#include <kvs/RGBColor>


namespace InSituVis
{

namespace mpi
{

inline void CameraPathControlledAdaptor::setOutputEvaluationImageEnabled(
    const bool enable,
    const bool enable_depth )
{
    m_enable_output_evaluation_image = enable;
    m_enable_output_evaluation_image_depth = enable_depth;
}

inline bool CameraPathControlledAdaptor::isEntropyStep()
{
    return BaseClass::timeStep() % ( BaseClass::analysisInterval() * Controller::entropyInterval() ) == 0;
}

inline bool CameraPathControlledAdaptor::isFinalTimeStep()
{
    return BaseClass::timeStep() == m_final_time_step;
}

inline bool CameraPathControlledAdaptor::dump()
{
    bool ret = true;
    if ( BaseClass::world().isRoot() )
    {
        if ( m_entr_timer.title().empty() ) { m_entr_timer.setTitle( "Ent time" ); }
        kvs::StampTimerList timer_list;
        timer_list.push( m_entr_timer );

        const auto basedir = BaseClass::outputDirectory().baseDirectoryName() + "/";
        ret = timer_list.write( basedir + "ent_proc_time.csv" );

        this->outputPathEntropies( Controller::pathEntropies() );
        this->outputPathPositions( Controller::pathPositions() );
    }

    return BaseClass::dump() && ret;
}

inline void CameraPathControlledAdaptor::exec( const BaseClass::SimTime sim_time )
{
    Controller::setCacheEnabled( BaseClass::isAnalysisStep() );
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

    if ( this->isEntropyStep() )
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

                //this->outputColorImage( location, frame_buffer );
            }
            timer.stop();
            entr_time += BaseClass::saveTimer().time( timer );
        }

        // Distribute the index indicates the max entropy image
        BaseClass::world().broadcast( max_index );
        BaseClass::world().broadcast( max_entropy );
        const auto max_position = BaseClass::viewpoint().at( max_index ).position;
        const auto max_rotation = BaseClass::viewpoint().at( max_index ).rotation;
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
                this->outputEntropies( entropies );
            }
        }
        timer.stop();
        save_time += BaseClass::saveTimer().time( timer );
    }
    else
    {
        auto radius = Controller::erpRadius();
        auto rotation = Controller::erpRotation();
        const size_t i = 999999;
        const auto d = InSituVis::Viewpoint::Direction::Uni;
        const auto p = kvs::Quat::Rotate( kvs::Vec3( { 0.0f, radius, 0.0f } ), rotation );
        const auto u = kvs::Quat::Rotate( kvs::Vec3( { 0.0f, 0.0f, -1.0f } ), rotation );
        const auto l = kvs::Vec3( { 0.0f, 0.0f, 0.0f } );
        const auto location = InSituVis::Viewpoint::Location( i, d, p, u, rotation, l );
        auto frame_buffer = BaseClass::readback( location );
        const auto path_entropy = Controller::entropy( frame_buffer );
        Controller::setMaxEntropy( path_entropy );
        Controller::setMaxPosition( p );

        kvs::Timer timer( kvs::Timer::Start );
        if ( BaseClass::world().rank() == BaseClass::world().root() )
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
    m_entr_timer.stamp( entr_time );
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
    {
        // Reset time step, which is used for output filename,
        // for visualizing the stacked dataset.
        const auto L_crr = Controller::dataQueue().size();
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

inline void CameraPathControlledAdaptor::outputColorImage(
    const InSituVis::Viewpoint::Location& location,
    const FrameBuffer& frame_buffer )
{
    const auto size = BaseClass::outputImageSize( location );
    const auto buffer = frame_buffer.color_buffer;
    kvs::ColorImage image( size.x(), size.y(), buffer );
    image.write( BaseClass::outputFinalImageName( location ) );
}

inline void CameraPathControlledAdaptor::outputDepthImage(
    const InSituVis::Viewpoint::Location& location,
    const FrameBuffer& frame_buffer )
{
    const auto size = BaseClass::outputImageSize( location );
    const auto buffer = frame_buffer.depth_buffer;
    kvs::GrayImage image( size.x(), size.y(), buffer );
    image.write( BaseClass::outputFinalImageName( location ) );
}

inline void CameraPathControlledAdaptor::outputEntropies(
    const std::vector<float> entropies )
{
    const auto time = BaseClass::timeStep();
    const auto output_time = kvs::String::From( time, 6, '0' );
    const auto output_filename =  "output_entropies_" + output_time;
    const auto filename = BaseClass::outputDirectory().baseDirectoryName() + "/" + output_filename + ".csv";
    std::ofstream file( filename );
    file << "Index,Entropy" << std::endl;
    
    for( size_t i = 0; i < entropies.size(); i++ )
    {
        file << i << "," << entropies[i] << std::endl;
    }

    file.close();
}

inline void CameraPathControlledAdaptor::outputPathEntropies(
    const std::vector<float> path_entropies )
{
    const auto output_filename =  "output_path_entropies";
    const auto filename = BaseClass::outputDirectory().baseDirectoryName() + "/" + output_filename + ".csv";
    std::ofstream file( filename );
    const auto interval = BaseClass::analysisInterval();
    file << "Time,Entropy" << std::endl;
    
    for( size_t i = 0; i < path_entropies.size(); i++ )
    {
        file << interval * i << "," << path_entropies[i] << std::endl;
    }

    file.close();
}

inline void CameraPathControlledAdaptor::outputPathPositions(
    const std::vector<float> path_positions )
{
    const auto output_filename =  "output_path_positions";
    const auto filename = BaseClass::outputDirectory().baseDirectoryName() + "/" + output_filename + ".csv";
    std::ofstream file( filename );
    const auto interval = BaseClass::analysisInterval();
    file << "Time,X,Y,Z" << std::endl;
    
    for( size_t i = 0; i < path_positions.size() / 3; i++ )
    {
        const auto x = path_positions[ 3 * i ];
        const auto y = path_positions[ 3 * i + 1 ];
        const auto z = path_positions[ 3 * i + 2 ];
        file << interval * i << "," << x << "," << y << "," << z << std::endl;
    }

    file.close();
}

} // end of namespace mpi

} // end of namespace InSituVis
