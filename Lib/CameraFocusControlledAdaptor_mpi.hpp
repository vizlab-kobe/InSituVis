#include <kvs/ColorImage>
#include <kvs/RGBColor>


namespace InSituVis
{

namespace mpi
{

inline bool CameraFocusControlledAdaptor::isEntropyStep()
{
    return BaseClass::timeStep() % ( BaseClass::analysisInterval() * Controller::entropyInterval() ) == 0;
}

inline bool CameraFocusControlledAdaptor::isFinalTimeStep()
{
    return BaseClass::timeStep() == m_final_time_step;
}

inline CameraFocusControlledAdaptor::Location
CameraFocusControlledAdaptor::erpLocation(
    const kvs::Vec3 focus,
    const size_t index,
    const Viewpoint::Direction dir )
{
    const auto rad = Controller::erpRadius();
    const auto rot = Controller::erpRotation();
    const auto p = kvs::Quat::Rotate( kvs::Vec3( { 0.0f, rad,   0.0f } ), rot );
    const auto u = kvs::Quat::Rotate( kvs::Vec3( { 0.0f, 0.0f, -1.0f } ), rot );
    const auto l = kvs::Vec3( { 0.0f, 0.0f, 0.0f } );
    return this->focusedLocation( { index, dir, p, u, rot, l }, focus );
}

// add
inline CameraFocusControlledAdaptor::Location
CameraFocusControlledAdaptor::focusedLocation(
    const Location& location,
    const kvs::Vec3 focus )
{
    const auto p0 = location.look_at - location.position;
    const auto p1 = focus - location.position;
    const auto R = kvs::Quat::RotationQuaternion( p0, p1 );

    auto l = InSituVis::Viewpoint::Location(
        location.direction,
        location.position,
        kvs::Quat::Rotate( location.up_vector, R ), 
        focus );
    l.index = location.index;
    //l.look_at = focus;

    return l;
}

inline bool CameraFocusControlledAdaptor::dump() //mod
{
    bool ret = true;
    bool ret_f = true; // add
    bool ret_z = true;
    if ( BaseClass::world().isRoot() )
    {
        if ( m_entr_timer.title().empty() ) { m_entr_timer.setTitle( "Ent time" ); }
        kvs::StampTimerList entr_timer_list;
        entr_timer_list.push( m_entr_timer );

        const auto basedir = BaseClass::outputDirectory().baseDirectoryName() + "/";
        ret = entr_timer_list.write( basedir + "ent_proc_time.csv" );

        if ( m_focus_timer.title().empty() ) { m_focus_timer.setTitle( "focus time" ); } // add
        kvs::StampTimerList f_timer_list;                                                // add
        f_timer_list.push( m_focus_timer );                                              // add
        ret_f = f_timer_list.write( basedir + "focus_proc_time.csv" );                   // add

        if ( m_zoom_timer.title().empty() ) { m_zoom_timer.setTitle( "zoom time" ); }
        kvs::StampTimerList z_timer_list;
        z_timer_list.push( m_zoom_timer );
        ret_z = z_timer_list.write( basedir + "zoom_proc_time.csv" );
        
        const auto directory = BaseClass::outputDirectory();
        const auto File = [&]( const std::string& name ) { return Controller::logDataFilename( name, directory ); };
        Controller::outputPathCalcTimes( File( "output_path_calc_times" ) );
        Controller::outputViewpointCoords( File( "output_viewpoint_coords" ), BaseClass::viewpoint() );
        Controller::outputNumImages( File( "output_num_images" ), BaseClass::analysisInterval() );
    }

    return BaseClass::dump() && ret && ret_f && ret_z;
}

inline void CameraFocusControlledAdaptor::exec( const BaseClass::SimTime sim_time ) //mod
{
    Controller::setCacheEnabled( BaseClass::isAnalysisStep() );
    Controller::setIsEntStep( this->isEntropyStep() );
    Controller::updataCacheSize();
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

inline void CameraFocusControlledAdaptor::execRendering() //mod
{
    BaseClass::setRendTime( 0.0f );
    BaseClass::setCompTime( 0.0f );
    float save_time = 0.0f;
    float entr_time = 0.0f;
    float focus_time = 0.0f;
    float zoom_time = 0.0f;

    float max_entropy = -1.0f;
    int max_index = 0;

    std::vector<float> entropies;
    std::vector<FrameBuffer> frame_buffers;

    // Auto zooming
    std::vector<float> zoom_entropies;
    std::vector<FrameBuffer> zoom_frame_buffers;
    
    if ( Controller::isEntStep() && !Controller::isErpStep())
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

                /*
                if ( m_enable_output_evaluation_image )
                {
                    this->outputColorImage( location, frame_buffer );
                }

                if ( m_enable_output_evaluation_image_depth )
                {
                    this->outputDepthImage( location, frame_buffer );
                }
                */
            }
            timer.stop();
            entr_time += m_entr_timer.time( timer );
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
        // Controller::setMaxRotation( max_rotation );
        Controller::setMaxEntropy( max_entropy );

        // Calculate camera focus point.
        auto at = max_location.look_at;
        kvs::Timer timer( kvs::Timer::Start );
        if ( BaseClass::world().isRoot() )
        {
            const auto& frame_buffer = frame_buffers[ max_index ];
            const auto at_w = this->look_at_in_window( frame_buffer );
            at = this->window_to_object( at_w, max_location );
        }
        timer.stop();
        focus_time += m_focus_timer.time( timer );

        // Readback frame buffer rendererd from updated location.
        BaseClass::world().broadcast( at.data(), sizeof(float) * 3 );
        Controller::setMaxFocusPoint( at );

        auto location = this->focusedLocation( max_location, at );
        Controller::setMaxPosition( max_position );

        // Zooming
        auto max_zoom_entropy = -1.0f;
        auto estimated_zoom_level = 0;
        auto estimated_zoom_position = max_position;
        for ( size_t level = 0; level < m_zoom_level; level++ )
        {
            // Update camera position.
            timer.start();
            auto t = static_cast<float>( level ) / static_cast<float>( m_zoom_level );
            location.position = ( 1 - t ) * max_position + t * at;
            timer.stop();
            zoom_time += m_zoom_timer.time( timer );

            // Rendering at the updated camera position.
            auto frame_buffer =  BaseClass::readback( location );

            // Output the rendering images and the heatmap of entropies.
            if ( BaseClass::world().isRoot() )
            {
                if ( Controller::isAutoZoomingEnabled() )
                {
                    timer.start();
                    auto zoom_entropy = Controller::entropy( frame_buffer );
                    zoom_entropies.push_back( zoom_entropy );
                    zoom_frame_buffers.push_back( frame_buffer );

                    if ( zoom_entropy > max_zoom_entropy )
                    {
                        max_zoom_entropy = zoom_entropy;
                        estimated_zoom_level = level;
                        estimated_zoom_position = location.position;
                    }
                    timer.stop();
                    zoom_time += m_zoom_timer.time( timer );
                }
                else
                {
                    if ( BaseClass::isOutputImageEnabled() )
                    {
                        timer.start();
                        if ( Controller::isOutpuColorImage() ) this->outputColorImage( location, frame_buffer, level );
                        else {this->outputDepthImage( location, frame_buffer, level );}
                        timer.stop();
                        save_time += BaseClass::saveTimer().time( timer );
                    }
                }
            }
        }

        if ( Controller::isAutoZoomingEnabled() )
        {
            BaseClass::world().broadcast( estimated_zoom_level );
            BaseClass::world().broadcast( estimated_zoom_position.data(), sizeof(float) * 3 );
            Controller::setMaxPosition( estimated_zoom_position );
            Controller::setEstimatedZoomLevel( estimated_zoom_level );
            Controller::setEstimatedZoomPosition( estimated_zoom_position );
            Controller::setMaxRotation( this->rotation( estimated_zoom_position ) );
            
            
            
            if ( BaseClass::world().isRoot() )
            {
                if ( BaseClass::isOutputImageEnabled() )
                {
                    location.position = Controller::estimatedZoomPosition();
                    const auto level = Controller::estimatedZoomLevel();
                    const auto frame_buffer = zoom_frame_buffers[ level ];
                    timer.start();
                    if ( Controller::isOutpuColorImage() ) this->outputColorImage( location, frame_buffer, level );
                    else {this->outputDepthImage( location, frame_buffer, level );}
                    timer.stop();
                    save_time += BaseClass::saveTimer().time( timer );
                }

                this->outputZoomEntropies( zoom_entropies );
            }
        }

    }
    else
    {

        kvs::Timer timer;

        const auto focus = Controller::erpFocus();  // add
        //Controller::setMaxFocusPoint( focus );      // add

        if ( Controller::isAutoZoomingEnabled() )
        {
            auto location = this->erpLocation( focus );
            auto frame_buffer = BaseClass::readback( location );

            // Controller::setEstimatedZoomLevel( 0 );
            // Controller::setEstimatedZoomPosition( location.position );
            timer.start();
            if ( BaseClass::world().isRoot() )
            {
                if ( BaseClass::isOutputImageEnabled() )
                {
                    if ( Controller::isOutpuColorImage() ) this->outputColorImage( location, frame_buffer, 0 );
                    else {this->outputDepthImage( location, frame_buffer, 0 );}
                }
            }
            timer.stop();
            save_time += BaseClass::saveTimer().time( timer );       
        }
        else
        {
            auto location = this->erpLocation( focus );
            //Controller::setMaxPosition( location.position );

            // Zooming
            const auto p = location.position;
            for ( size_t level = 0; level < m_zoom_level; level++ )
            {
                // Update camera position.
                timer.start();
                auto t = static_cast<float>( level ) / static_cast<float>( m_zoom_level );
                location.position = ( 1 - t ) * p + t * focus;
                timer.stop();
                zoom_time += m_zoom_timer.time( timer );

                // Rendering at the updated camera position.
                auto frame_buffer = BaseClass::readback( location );

                // if ( level == 0 )
                // {
                //     const auto path_entropy = Controller::entropy( frame_buffer );
                //     Controller::setMaxEntropy( path_entropy );
                // }

                timer.start();
                if ( BaseClass::world().isRoot() )
                {
                    if ( BaseClass::isOutputImageEnabled() )
                    {
                        if ( Controller::isOutpuColorImage() ) this->outputColorImage( location, frame_buffer, level );
                        else {this->outputDepthImage( location, frame_buffer, level );}
                        
                    }
                }
                timer.stop();
                save_time += BaseClass::saveTimer().time( timer );
            }
        }
    }

    m_entr_timer.stamp( entr_time );
    m_focus_timer.stamp( focus_time );
    m_zoom_timer.stamp( zoom_time );
    BaseClass::saveTimer().stamp( save_time );
    BaseClass::rendTimer().stamp( BaseClass::rendTime() );
    BaseClass::compTimer().stamp( BaseClass::compTime() );
}

inline void CameraFocusControlledAdaptor::process( const Data& data )
{
    BaseClass::execPipeline( data );
    this->execRendering();
}

// add
inline void CameraFocusControlledAdaptor::process(
    const Data& data,
    const float radius,
    const kvs::Quaternion& rotation,
    const kvs::Vec3& focus )
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
        Controller::setErpFocus( focus ); // add
        BaseClass::execPipeline( data );
        this->execRendering();

        BaseClass::setTimeStep( current_step );

}

// add
inline std::string CameraFocusControlledAdaptor::outputFinalImageName( const size_t level )
{
    const auto time = BaseClass::timeStep();
    const auto output_time = kvs::String::From( time, 6, '0' );
    const auto output_basename = BaseClass::outputFilename();
    const auto output_zoom_level = kvs::String::From( level, 6, '0' );
    const auto output_filename = output_basename + "_" + output_time + "_" + output_zoom_level;
    const auto filename = BaseClass::outputDirectory().baseDirectoryName() + "/" + output_filename + ".bmp";
    return filename;
}

inline void CameraFocusControlledAdaptor::outputColorImage(
    const InSituVis::Viewpoint::Location& location,
    const FrameBuffer& frame_buffer,
    const size_t level ) // add
{
    const auto size = BaseClass::outputImageSize( location );
    const auto buffer = frame_buffer.color_buffer;
    kvs::ColorImage image( size.x(), size.y(), buffer );
    image.write( this->outputFinalImageName( level ) );
}

inline void CameraFocusControlledAdaptor::outputDepthImage(
    const InSituVis::Viewpoint::Location& location,
    const FrameBuffer& frame_buffer,
    const size_t level )
{
    const auto size = BaseClass::outputImageSize( location );
    const auto buffer = frame_buffer.depth_buffer;
    kvs::GrayImage image( size.x(), size.y(), buffer );
    image.write( this->outputFinalImageName( level ) );
}

// add
inline kvs::Vec3 CameraFocusControlledAdaptor::look_at_in_window( const FrameBuffer& frame_buffer )
{
    const auto w = BaseClass::imageWidth(); // frame buffer width
    const auto h = BaseClass::imageHeight(); // frame buffer height
//    const auto cw = w / m_frame_divs.x(); // cropped frame buffer width
//    const auto ch = h / m_frame_divs.y(); // cropped frame buffer height
//    const auto cw = ( w + 1 ) / m_frame_divs.x(); // cropped frame buffer width
//    const auto ch = ( h + 1 ) / m_frame_divs.y(); // cropped frame buffer height
    const auto cw = w / m_frame_divs.x() + 1; // cropped frame buffer width
    const auto ch = h / m_frame_divs.y() + 1; // cropped frame buffer height

    auto get_center = [&] ( int i, int j ) -> kvs::Vec2i
    {
        return { static_cast<int>( i * cw + cw * 0.5 ) ,static_cast<int>( ( h - 1 ) - static_cast<int>( j * ch + ch * 0.5 ) )};
            //static_cast<int>( i * cw + cw * 0.5 ),
//            static_cast<int>( ( h - 1 ) - ( j * ch + ch * 0.5 ) ) };
            //static_cast<int>( j * ch + ch * 0.5 ) };
    };

    auto get_depth = [&] ( const FrameBuffer& buffer ) -> float
    {
        const auto cx = static_cast<int>( cw * 0.5 );
        const auto cy = static_cast<int>( ch * 0.5 );
        const auto index = cx + cy * cw;

        const auto& depth_buffer = buffer.depth_buffer;
        if ( depth_buffer[ index ] < 1.0f )
        {
            return depth_buffer[ index ];
        }
        else
        {
            float min_depth = 1.0f;
            for ( const auto d : depth_buffer )
            {
                if ( d < 1.0f ) { min_depth = kvs::Math::Min( d, min_depth ); }
            }
            return min_depth;
        }
    };

//    FrameBuffer cropped_buffer;
//    cropped_buffer.color_buffer.allocate( cw * ch * 4 );
//    cropped_buffer.depth_buffer.allocate( cw * ch );

    float max_entropy = -1.0f;
    kvs::Vec2i center{ 0, 0 };
    kvs::Real32 depth{ 0.0f };
    std::vector<float> focus_entropies;
    for ( size_t j = 0; j < m_frame_divs.y(); j++ )
    {
        for ( size_t i = 0; i < m_frame_divs.x(); i++ )
        {
            const auto indices = kvs::Vec2i( static_cast<int>(i), static_cast<int>(j) );
//            this->crop_frame_buffer( frame_buffer, indices, &cropped_buffer );
            const auto cropped_buffer = this->crop_frame_buffer( frame_buffer, indices );
            const auto e = Controller::entropy( cropped_buffer );
            focus_entropies.push_back(e);
            if ( e > max_entropy )
            {
                max_entropy = e;
                center = get_center( i, j );
                depth = get_depth( cropped_buffer );
            }
        }
    }

    if ( Controller::isOutputFrameEntropiesEnabled() )
    {
        const auto basename = "output_frame_entropies_";
        const auto timestep = BaseClass::timeStep();
        const auto directory = BaseClass::outputDirectory();
        const auto filename = Controller::logDataFilename( basename, timestep, directory );
        Controller::outputFrameEntropies( filename, focus_entropies );
    }

    return { static_cast<float>( center.x() ), static_cast<float>( center.y() ), depth };
}

// add
inline kvs::Vec3 CameraFocusControlledAdaptor::window_to_object(
    const kvs::Vec3 win,
    const Location& location )
{
    auto* manager = this->screen().scene()->objectManager();
    auto* camera = screen().scene()->camera();

    // Backup camera info.
    const auto p0 = camera->position();
    const auto a0 = camera->lookAt();
    const auto u0 = camera->upVector();
    {
        const auto p = location.position;
        const auto a = location.look_at;
        const auto u = location.up_vector;
        camera->setPosition( p, a, u );
    }

    const auto xv = kvs::Xform( camera->viewingMatrix() );
    const auto xp = kvs::Xform( camera->projectionMatrix() );
    const auto xo = manager->xform();
    const auto xm = xv * xo;

    // Restore camera info.
    camera->setPosition( p0, a0, u0 );

    auto x_to_a = [] ( const kvs::Xform& x, double a[16] )
    {
        const auto m = x.toMatrix();
        a[0] = m[0][0]; a[4] = m[0][1]; a[8]  = m[0][2]; a[12] = m[0][3];
        a[1] = m[1][0]; a[5] = m[1][1]; a[9]  = m[1][2]; a[13] = m[1][3];
        a[2] = m[2][0]; a[6] = m[2][1]; a[10] = m[2][2]; a[14] = m[2][3];
        a[3] = m[3][0]; a[7] = m[3][1]; a[11] = m[3][2]; a[15] = m[3][3];
    };

    double m[16]; x_to_a( xm, m ); // model-view matrix
    double p[16]; x_to_a( xp, p ); // projection matrix
    int v[4]; kvs::OpenGL::GetViewport( v ); // viewport

    kvs::Vec3d obj( 0.0, 0.0, 0.0 );
    kvs::OpenGL::UnProject(
        win.x(), win.y(), win.z(), m, p, v,
        &obj[0], &obj[1], &obj[2] );

    return kvs::Vec3( obj );
}

// add
inline CameraFocusControlledAdaptor::FrameBuffer
CameraFocusControlledAdaptor::crop_frame_buffer(
    const FrameBuffer& frame_buffer,
    const kvs::Vec2i& indices )
{
    KVS_ASSERT( indices[0] < static_cast<int>( m_frame_divs[0] ) );
    KVS_ASSERT( indices[1] < static_cast<int>( m_frame_divs[1] ) );

    const auto w = BaseClass::imageWidth(); // frame buffer width
    const auto h = BaseClass::imageHeight(); // frame buffer height
//    const auto cw = w / m_frame_divs.x(); // cropped frame buffer width
//    const auto ch = h / m_frame_divs.y(); // cropped frame buffer height
    const auto cw = w / m_frame_divs.x() + 1; // cropped frame buffer width
    const auto ch = h / m_frame_divs.y() + 1; // cropped frame buffer height
    const auto ow = cw * indices[0]; // offset width for frame buffer
    const auto oh = ch * indices[1]; // offset height for frame buffer

    // Adjust cropped width and height (aw,ah).
    const auto ww = ow + cw;
    const auto hh = oh + ch;

    const auto aw = ( w >= ww ) ? cw : cw - ( ww - w );
    const auto ah = ( h >= hh ) ? ch : ch - ( hh - h );

    // Cropped frame buffer.
    FrameBuffer cropped_buffer;
    cropped_buffer.color_buffer.allocate( aw * ah * 4 );
    cropped_buffer.depth_buffer.allocate( aw * ah );

    auto* dst_color_buffer = cropped_buffer.color_buffer.data();
    auto* dst_depth_buffer = cropped_buffer.depth_buffer.data();
    const auto offset = ow + oh * w;
    const auto* src_color_buffer = frame_buffer.color_buffer.data() + offset * 4;
    const auto* src_depth_buffer = frame_buffer.depth_buffer.data() + offset;
//    for ( size_t j = 0; j < ch; j++ )
    for ( size_t j = 0; j < ah; j++ )
    {
//        std::memcpy( dst_color_buffer, src_color_buffer, cw * 4 * sizeof( kvs::UInt8 ) );
        std::memcpy( dst_color_buffer, src_color_buffer, aw * 4 * sizeof( kvs::UInt8 ) );
//        dst_color_buffer += cw * 4;
        dst_color_buffer += aw * 4;
        src_color_buffer += w * 4;

//        std::memcpy( dst_depth_buffer, src_depth_buffer, cw * sizeof( kvs::Real32 ) );
        std::memcpy( dst_depth_buffer, src_depth_buffer, aw * sizeof( kvs::Real32 ) );
//        dst_depth_buffer += cw;
        dst_depth_buffer += aw;
        src_depth_buffer += w;
    }

    return cropped_buffer;
}

inline kvs::Quat CameraFocusControlledAdaptor::rotation( const kvs::Vec3& position )
{
    auto xyz_to_rtp = [&] ( const kvs::Vec3& xyz ) -> kvs::Vec3
    {
        const float x = xyz[0];
        const float y = xyz[1];
        const float z = xyz[2];
        const float r = sqrt( x * x + y * y + z * z );
        const float t = std::acos( y / r );
        const float p = std::atan2( x, z );
        return kvs::Vec3( r, t, p );
    };

    const auto base = kvs::Vec3{ 0.0f, 12.0f, 0.0f };
    const auto axis = kvs::Vec3{ 0.0f, 1.0f, 0.0f };
    const auto rtp = xyz_to_rtp( position );
    const auto phi = rtp[2];
    const auto q_phi = kvs::Quat( axis, phi );
    const auto q_theta = kvs::Quat::RotationQuaternion( base, position );
    return q_theta * q_phi;
}

//add
inline void CameraFocusControlledAdaptor::outputZoomEntropies(
    const std::vector<float> zoom_entropies )
{
    const auto time = BaseClass::timeStep();
    const auto output_time = kvs::String::From( time, 6, '0' );
    const auto output_filename = "output_zoom_entropies" + output_time;
    const auto filename = BaseClass::outputDirectory().baseDirectoryName() + "/" + output_filename + ".csv";

    std::ofstream file( filename );
    {
        file << "Zoomlevel,Entropy" << std::endl;
        for ( size_t i = 0; i < zoom_entropies.size(); i++ )
        {
            file << i << "," << zoom_entropies[i] << std::endl;
        }
    }
    file.close();
}

} // end of namespace mpi

} // end of namespace InSituVis
