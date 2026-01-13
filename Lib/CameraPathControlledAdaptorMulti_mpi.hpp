#include <kvs/ColorImage>
#include <kvs/RGBColor>
#include <kvs/Math>
#include <kvs/Quaternion>
#include <kvs/LabColor>
#include <kvs/Stat>
#include <time.h>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>

namespace InSituVis
{
namespace mpi
{

/* =========================
 * Utilities
 * ========================= */
static inline float clampf( float x, float lo, float hi )
{
    return (x < lo) ? lo : (x > hi) ? hi : x;
}

/* =========================
 * Basic step helpers
 * ========================= */
inline bool CameraPathControlledAdaptorMulti::isEntropyStep()
{
    return BaseClass::timeStep() % ( BaseClass::analysisInterval() * Controller::entropyInterval() ) == 0;
}

inline bool CameraPathControlledAdaptorMulti::isFinalTimeStep()
{
    return BaseClass::timeStep() == m_final_time_step;
}

/* =========================
 * Location helpers
 * ========================= */
inline CameraPathControlledAdaptorMulti::Location
CameraPathControlledAdaptorMulti::erpLocation(
    const kvs::Vec3 focus,
    const size_t index,
    const Viewpoint::Direction dir )
{
    const auto rad = Controller::erpRadius();
    const auto rot = Controller::erpRotation();
    const auto p = kvs::Quat::Rotate( kvs::Vec3( { 0.0f, rad,   0.0f } ), rot );
    const auto u = kvs::Quat::Rotate( kvs::Vec3( { 0.0f, 0.0f, -1.0f } ), rot );
    return { index, dir, p, u, rot, focus };
}

inline CameraPathControlledAdaptorMulti::Location
CameraPathControlledAdaptorMulti::focusedLocation(
    const Location& location,
    const kvs::Vec3 focus )
{
    const auto p0 = location.look_at - location.position;
    const auto p1 = focus - location.position;
    const auto R = kvs::Quat::RotationQuaternion( p0, p1 );

    auto l = InSituVis::Viewpoint::Location(
        location.direction,
        location.position,
        kvs::Quat::Rotate( location.up_vector, R ), focus );
    l.index = location.index;
    l.look_at = focus;

    return l;
}

/* =========================
 * Dump / Exec
 * ========================= */
inline bool CameraPathControlledAdaptorMulti::dump()
{
    bool ret = true;
    bool ret_f = true;
    bool ret_z = true;

    if ( BaseClass::world().isRoot() )
    {
        if ( m_entr_timer.title().empty() ) { m_entr_timer.setTitle( "Ent time" ); }
        kvs::StampTimerList entr_timer_list;
        entr_timer_list.push( m_entr_timer );

        const auto basedir = BaseClass::outputDirectory().baseDirectoryName() + "/";
        ret = entr_timer_list.write( basedir + "ent_proc_time.csv" );

        if ( m_focus_timer.title().empty() ) { m_focus_timer.setTitle( "focus time" ); }
        kvs::StampTimerList f_timer_list;
        f_timer_list.push( m_focus_timer );
        ret_f = f_timer_list.write( basedir + "focus_proc_time.csv" );

        if ( m_zoom_timer.title().empty() ) { m_zoom_timer.setTitle( "zoom time" ); }
        kvs::StampTimerList z_timer_list;
        z_timer_list.push( m_zoom_timer );
        ret_z = z_timer_list.write( basedir + "zoom_proc_time.csv" );

        const auto directory = BaseClass::outputDirectory();
        const auto File = [&]( const std::string& name ) { return Controller::logDataFilename( name, directory ); };
        Controller::outputPathCalcTimes( File( "output_path_calc_times" ) );
        Controller::outputViewpointCoords( File( "output_viewpoint_coords" ), BaseClass::viewpoint() );
        Controller::outputNumImages( File( "output_num_images" ), BaseClass::analysisInterval() );
        Controller::outputVideoParams(
            File("output_video_params" ),
            Controller::outputFilenames(),
            Controller::focusEntropies(),
            Controller::focusPathLength(),
            Controller::cameraPathLength()
        );
    }

    return BaseClass::dump() && ret && ret_f && ret_z;
}

inline void CameraPathControlledAdaptorMulti::exec( const BaseClass::SimTime sim_time )
{
    Controller::setCacheEnabled( BaseClass::isAnalysisStep() );
    Controller::setIsEntStep( this->isEntropyStep() );
    Controller::updataCacheSize();

    Controller::push( BaseClass::objects() );

    BaseClass::incrementTimeStep();

    if ( this->isFinalTimeStep() )
    {
        Controller::setIsFinalStep( true );
        const auto dummy = Data();
        Controller::push( dummy );
    }

    BaseClass::clearObjects();
}

/* =========================
 * Maximal location selection (viewpoint)
 * ========================= */
inline std::vector<int> CameraPathControlledAdaptorMulti::getMaximalLocations(
    const std::vector<Viewpoint::Location>& locations,
    const std::vector<float>& entropies )
{
    const int rows    = this->viewPointDimention().y();
    const int columns = this->viewPointDimention().z();

    if ( entropies.size() != static_cast<size_t>(columns * rows) )
    {
        std::cerr << "[ERR] entropy size mismatch: entropies="
                  << entropies.size() << " expected=" << (columns*rows) << "\n";
        return {};
    }

    struct Peak { float entropy; size_t index; };
    std::vector<Peak> local_maxima;

    for (const auto& location : locations)
    {
        const float e = entropies[location.index];
        if (e < 0) continue;

        const int x = static_cast<int>(location.index % columns);
        const int y = static_cast<int>(location.index / columns);

        bool is_local_max = true;
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                if (dx == 0 && dy == 0) continue;

                const int nx = (x + dx + columns) % columns;
                const int ny = y + dy;
                if (ny < 0 || ny >= rows) continue;

                const size_t nidx = static_cast<size_t>(ny * columns + nx);
                if (entropies[nidx] >= e)
                {
                    is_local_max = false;
                    break;
                }
            }
            if (!is_local_max) break;
        }

        if (is_local_max) local_maxima.push_back({ e, location.index });
    }

    std::sort(local_maxima.begin(), local_maxima.end(),
        [](const Peak& a, const Peak& b) { return a.entropy > b.entropy; });

    std::vector<int> maximal_indices;
    maximal_indices.reserve( viewPointCandidateNum() );

    const int nMax = static_cast<int>(local_maxima.size());
    for (int i = 0; i < viewPointCandidateNum(); ++i)
    {
        if (nMax > 0)
        {
            const int pick = (i < nMax) ? i : (nMax - 1);
            maximal_indices.push_back(static_cast<int>(local_maxima[pick].index));
        }
        else
        {
            maximal_indices.push_back(0);
        }
    }

    return maximal_indices;
}

/* =========================
 * Rendering core
 * ========================= */
inline void CameraPathControlledAdaptorMulti::execRendering()
{
    BaseClass::setRendTime( 0.0f );
    BaseClass::setCompTime( 0.0f );

    float save_time  = 0.0f;
    float entr_time  = 0.0f;
    float focus_time = 0.0f;
    float zoom_time  = 0.0f;

    float max_entropy = -1.0f;
    int   max_index   = 0;

    std::vector<float> entropies;
    std::vector<FrameBuffer> frame_buffers;

    // ========= Entropy Step (and not ERP playback) =========
    if ( Controller::isEntStep() && !Controller::isErpStep() )
    {
        std::vector<int> maximal_indices(viewPointCandidateNum(), 0);

        for ( const auto& location : BaseClass::viewpoint().locations() )
        {
            auto frame_buffer = BaseClass::readback( location );

            kvs::Timer timer( kvs::Timer::Start );
            if ( BaseClass::world().isRoot() )
            {
                const auto e = Controller::entropy( frame_buffer );
                entropies.push_back( e );
                frame_buffers.push_back( frame_buffer );

                if ( e > max_entropy )
                {
                    max_entropy = e;
                    max_index = static_cast<int>(location.index);
                }
            }
            timer.stop();
            entr_time += m_entr_timer.time( timer );
        }

        if ( BaseClass::world().isRoot() )
        {
            maximal_indices = this->getMaximalLocations( BaseClass::viewpoint().locations(), entropies );
        }
        else
        {
            maximal_indices.resize(viewPointCandidateNum());
        }

        for ( size_t i = 0; i < viewPointCandidateNum(); i++ )
        {
            BaseClass::world().broadcast(0, maximal_indices[i]);
        }

        // ========= For each selected viewpoint candidate =========
        for ( size_t vp_i = 0; vp_i < viewPointCandidateNum(); vp_i++ )
        {
            BaseClass::world().broadcast( maximal_indices[vp_i] );

            const auto& maximal_location = BaseClass::viewpoint().at( maximal_indices[vp_i] );
            const auto maximal_position  = maximal_location.position;

            Controller::setMaxIndex( maximal_indices[vp_i] );
            Controller::setMaxEntropy( max_entropy );

            std::vector<kvs::Vec3> at( focusPointCandidateNum() );
            kvs::Timer timer( kvs::Timer::Start );

            if ( BaseClass::world().isRoot() )
            {
                const auto& frame_buffer = frame_buffers[ maximal_indices[vp_i] ];
                const auto at_w = this->look_at_in_window( frame_buffer );

                for ( size_t j = 0; j < focusPointCandidateNum(); j++ )
                {
                    at[j] = this->window_to_object( at_w[j], maximal_location );
                }
            }

            timer.stop();
            focus_time += m_focus_timer.time( timer );

            for ( size_t j = 0; j < focusPointCandidateNum(); j++ )
            {
                BaseClass::world().broadcast( at[j].data(), 3 );
            }

            std::vector<Viewpoint::Location> locations;

            for ( size_t fp_j = 0; fp_j < focusPointCandidateNum(); fp_j++ )
            {
                std::vector<float> zoom_entropies;
                std::vector<FrameBuffer> zoom_frame_buffers;

                auto location = this->focusedLocation( maximal_location, at[fp_j] );
                locations.push_back(location);

                if ( !Controller::isAutoZoomingEnabled() )
                {
                    Controller::pushCandPositions( location.position );
                    Controller::pushCandRotations( this->rotation( location.position ) );
                }
                Controller::pushCandFocusPoints( at[fp_j] );

                float max_zoom_entropy = -1.0f;
                int   estimated_zoom_level = 0;
                kvs::Vec3 estimated_zoom_position = maximal_position;

                for ( size_t level = 0; level < m_zoom_level; level++ )
                {
                    timer.start();
                    const float t = static_cast<float>(level) / static_cast<float>(m_zoom_level);
                    locations[fp_j].position = (1 - t) * maximal_position + t * at[fp_j];
                    locations[fp_j].rotation = this->rotation( locations[fp_j].position );
                    locations[fp_j].up_vector = kvs::Quat::Rotate( kvs::Vec3( {0.0f, 0.0f, -1.0f} ), locations[fp_j].rotation );
                    timer.stop();
                    zoom_time += m_zoom_timer.time( timer );

                    auto frame_buffer = BaseClass::readback( locations[fp_j] );

                    if ( BaseClass::world().isRoot() )
                    {
                        if ( Controller::isAutoZoomingEnabled() )
                        {
                            timer.start();
                            const float ze = Controller::entropy( frame_buffer );
                            zoom_entropies.push_back( ze );
                            zoom_frame_buffers.push_back( frame_buffer );

                            if ( ze > max_zoom_entropy )
                            {
                                max_zoom_entropy = ze;
                                estimated_zoom_level = static_cast<int>(level);
                                estimated_zoom_position = locations[fp_j].position;
                            }
                            timer.stop();
                            zoom_time += m_zoom_timer.time( timer );
                        }
                        
                        else
                        {
                            if ( BaseClass::isOutputImageEnabled() )
                            {
                                timer.start();
                                if ( Controller::isOutpuColorImage() ) this->outputColorImage( locations[fp_j], frame_buffer, fp_j, level, 0 );
                                else this->outputDepthImage( locations[fp_j], frame_buffer, fp_j, level, 0 );
                                timer.stop();
                                save_time += BaseClass::saveTimer().time( timer );
                            }
                        }
                    }
                }

                if ( Controller::isAutoZoomingEnabled() )
                {
                    BaseClass::world().broadcast( max_zoom_entropy );
                    BaseClass::world().broadcast( estimated_zoom_level );
                    BaseClass::world().broadcast( estimated_zoom_position.data(), 3 );

                    Controller::pushCandZoomLevels( estimated_zoom_level );
                    Controller::pushCandPositions( estimated_zoom_position );
                    Controller::pushCandRotations( this->rotation( estimated_zoom_position ) );

                    if ( BaseClass::world().isRoot() )
                    {
                        if ( BaseClass::isOutputImageEnabled() )
                        {
                            Controller::pushFocusEntropies( max_zoom_entropy );

                            locations[fp_j].position = estimated_zoom_position;
                            locations[fp_j].rotation = this->rotation( estimated_zoom_position );
                            locations[fp_j].up_vector = kvs::Quat::Rotate( kvs::Vec3( {0.0f, 0.0f, -1.0f} ), locations[fp_j].rotation );
                            const size_t level = static_cast<size_t>(estimated_zoom_level);
                            const auto frame_buffer = zoom_frame_buffers[ level ];

                            Controller::pushOutputFilenames( outputFinalImageName(location, fp_j, level, 0) );

                            timer.start();
                            if ( Controller::isOutpuColorImage() ) this->outputColorImage( locations[fp_j], frame_buffer, fp_j, level, 0 );
                            else this->outputDepthImage( locations[fp_j], frame_buffer, fp_j, level, 0 );
                            timer.stop();
                            save_time += BaseClass::saveTimer().time( timer );
                        }

                        this->outputZoomEntropies( zoom_entropies );
                    }
                }
            }
        }
    }
    else
    {
        // ========= ERP / playback rendering =========
        kvs::Timer timer;

        const auto focus = Controller::erpFocus();

        if ( Controller::isAutoZoomingEnabled() )
        {
            auto location = this->erpLocation( focus );
            auto frame_buffer = BaseClass::readback( location );

            timer.start();
            if ( BaseClass::world().isRoot() )
            {
                const auto& p = location.position;
                const auto& a = location.look_at;
                const auto& u = location.up_vector;

                // std::cerr
                //     << "[OUT][VIEWPOINT]"
                //     << " step=" << BaseClass::timeStep()
                //     << " space=" << location.index
                //     << " cam=(" << p.x() << "," << p.y() << "," << p.z() << ")"
                //     << " look=(" << a.x() << "," << a.y() << "," << a.z() << ")"
                //     << " up=(" << u.x() << "," << u.y() << "," << u.z() << ")"
                //     << "\n";
                if ( BaseClass::isOutputImageEnabled() )
                {
                    if ( Controller::isOutpuColorImage() ) this->outputColorImage( location, frame_buffer, 999999, 0, routeNum() );
                    else this->outputDepthImage( location, frame_buffer, 999999, 0, routeNum() );
                }
            }
            timer.stop();
            save_time += BaseClass::saveTimer().time( timer );
        }
        else
        {
            auto location = this->erpLocation( focus );

            const auto p = location.position;
            for ( size_t level = 0; level < m_zoom_level; level++ )
            {
                timer.start();
                const float t = static_cast<float>( level ) / static_cast<float>( m_zoom_level );
                location.position = ( 1 - t ) * p + t * focus;
                timer.stop();
                zoom_time += m_zoom_timer.time( timer );

                auto frame_buffer = BaseClass::readback( location );

                if ( level == 0 )
                {
                    const auto path_entropy = Controller::entropy( frame_buffer );
                    Controller::setMaxEntropy( path_entropy );
                }

                timer.start();
                if ( BaseClass::world().isRoot() )
                {
                    if ( BaseClass::isOutputImageEnabled() )
                    {
                        if ( Controller::isOutpuColorImage() ) this->outputColorImage( location, frame_buffer, 999999, level, routeNum() );
                        else this->outputDepthImage( location, frame_buffer, 999999, level, routeNum() );
                    }
                }
                timer.stop();
                save_time += BaseClass::saveTimer().time( timer );
            }
        }
    }

    // timers
    m_entr_timer.stamp( entr_time );
    m_focus_timer.stamp( focus_time );
    m_zoom_timer.stamp( zoom_time );
    BaseClass::saveTimer().stamp( save_time );
    BaseClass::rendTimer().stamp( BaseClass::rendTime() );
    BaseClass::compTimer().stamp( BaseClass::compTime() );
}

/* =========================
 * Process wrappers
 * ========================= */
inline void CameraPathControlledAdaptorMulti::process( const Data& data )
{
    BaseClass::execPipeline( data );
    this->execRendering();
}

inline void CameraPathControlledAdaptorMulti::process(
    const Data& data,
    const float radius,
    const kvs::Quaternion& rotation,
    const kvs::Vec3& focus,
    const int route_num )
{
    const auto current_step = BaseClass::timeStep();

    const auto l = Controller::entropyInterval() -
        (( Controller::focusPointCandidateNum() * Controller::focusPointCandidateNum()
         * Controller::viewPointCandidateNum() * Controller::viewPointCandidateNum()
         * (Controller::entropyInterval()-1) - Controller::path().size())
        % ( Controller::entropyInterval()-1 ) + 1);

    const auto interval = BaseClass::analysisInterval();
    const auto step = current_step - l * interval;

    BaseClass::setTimeStep( step );
    BaseClass::tstepList().stamp( static_cast<float>( step ) );

    Controller::setErpRotation( rotation );
    Controller::setErpRadius( radius );
    Controller::setErpFocus( focus );

    BaseClass::execPipeline( data );
    setRouteNum(route_num);

    this->execRendering();

    BaseClass::setTimeStep( current_step );
}

/* =========================
 * Output helpers
 * ========================= */
inline std::string CameraPathControlledAdaptorMulti::outputFinalImageName(
    const Viewpoint::Location& location,
    const size_t candidateNum,
    const size_t level,
    const size_t from_to )
{
    const auto time  = BaseClass::timeStep();
    const auto space = location.index;

    const auto output_space        = kvs::String::From( space, 6, '0' );
    const auto output_time         = kvs::String::From( time, 6, '0' );
    const auto output_basename     = BaseClass::outputFilename();
    const auto output_candidate    = kvs::String::From( candidateNum, 6, '0' );
    const auto output_zoom_level   = kvs::String::From( level, 6, '0' );
    const auto output_route        = kvs::String::From( from_to, 6, '0' );

    const auto output_filename =
        output_basename + "_" + output_time + "_" + output_candidate + "_" +
        output_zoom_level + "_" + output_route + "_" + output_space;

    return BaseClass::outputDirectory().baseDirectoryName() + "/" + output_filename + ".bmp";
}

inline void CameraPathControlledAdaptorMulti::outputColorImage(
    const InSituVis::Viewpoint::Location& location,
    const FrameBuffer& frame_buffer,
    const size_t candidateNum,
    const size_t level,
    const size_t from_to )
{
    const auto size = BaseClass::outputImageSize( location );
    const auto buffer = frame_buffer.color_buffer;
    kvs::ColorImage image( size.x(), size.y(), buffer );
    image.write( this->outputFinalImageName( location, candidateNum, level, from_to ) );
}

inline void CameraPathControlledAdaptorMulti::outputDepthImage(
    const InSituVis::Viewpoint::Location& location,
    const FrameBuffer& frame_buffer,
    const size_t candidateNum,
    const size_t level,
    const size_t from_to )
{
    const auto size = BaseClass::outputImageSize( location );
    const auto buffer = frame_buffer.depth_buffer;
    kvs::GrayImage image( size.x(), size.y(), buffer );
    image.write( this->outputFinalImageName( location, candidateNum, level, from_to ) );
}

/* =========================
 * Focus estimation
 * ========================= */
inline std::vector<kvs::Vec3>
CameraPathControlledAdaptorMulti::look_at_in_window( const BaseClass::FrameBuffer& frame_buffer )
{
    const auto w = BaseClass::imageWidth();
    const auto h = BaseClass::imageHeight();
    const auto cw = w / m_frame_divs.x() + 1;
    const auto ch = h / m_frame_divs.y() + 1;

    auto get_center = [&] ( int i, int j ) -> kvs::Vec2i
    {
        return {
            static_cast<int>( i * cw + cw * 0.5f ),
            static_cast<int>( (h - 1) - static_cast<int>( j * ch + ch * 0.5f ) )
        };
    };

    auto get_depth = [&] ( const BaseClass::FrameBuffer& buffer ) -> float
    {
        const int cx = static_cast<int>( cw * 0.5f );
        const int cy = static_cast<int>( ch * 0.5f );
        const int idx = cx + cy * static_cast<int>(cw);

        const auto& depth_buffer = buffer.depth_buffer;

        if ( idx >= 0 && static_cast<size_t>(idx) < depth_buffer.size() && depth_buffer[idx] < 1.0f )
        {
            return depth_buffer[idx];
        }

        float min_depth = 1.0f;
        for ( const auto d : depth_buffer )
        {
            if ( d < 1.0f ) min_depth = kvs::Math::Min( d, min_depth );
        }
        return min_depth;
    };

    std::vector<float> entropies;
    std::vector<kvs::Vec2i> centers;
    std::vector<kvs::Real32> depthes;

    entropies.reserve( m_frame_divs.x() * m_frame_divs.y() );
    centers.reserve(   m_frame_divs.x() * m_frame_divs.y() );
    depthes.reserve(   m_frame_divs.x() * m_frame_divs.y() );

    for ( size_t j = 0; j < m_frame_divs.y(); ++j )
    {
        for ( size_t i = 0; i < m_frame_divs.x(); ++i )
        {
            const auto ij = kvs::Vec2i( static_cast<int>(i), static_cast<int>(j) );
            const auto cropped = this->crop_frame_buffer( frame_buffer, ij );

            const float e = Controller::entropy( cropped );
            entropies.push_back( e );
            centers.push_back( get_center( static_cast<int>(i), static_cast<int>(j) ) );
            depthes.push_back( get_depth( cropped ) );
        }
    }

    std::vector<kvs::Vec3> focuspoints;
    switch ( Controller::isROIMethod() )
    {
        case max:     focuspoints = biggestEntropyPoint( entropies, centers, depthes ); break;
        case maximum: focuspoints = maximalEntropyPoint( entropies, centers, depthes ); break;
        default:      focuspoints = biggestEntropyPoint( entropies, centers, depthes ); break;
    }

    return focuspoints;
}

/* =========================
 * window -> object
 * ========================= */
inline kvs::Vec3 CameraPathControlledAdaptorMulti::window_to_object(
    const kvs::Vec3& win,
    const Location& location )
{
    auto* manager = this->screen().scene()->objectManager();
    auto* camera  = screen().scene()->camera();

    // Backup
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

    // Restore
    camera->setPosition( p0, a0, u0 );

    auto x_to_a = [] ( const kvs::Xform& x, double a[16] )
    {
        const auto m = x.toMatrix();
        a[0] = m[0][0]; a[4] = m[0][1]; a[8]  = m[0][2]; a[12] = m[0][3];
        a[1] = m[1][0]; a[5] = m[1][1]; a[9]  = m[1][2]; a[13] = m[1][3];
        a[2] = m[2][0]; a[6] = m[2][1]; a[10] = m[2][2]; a[14] = m[2][3];
        a[3] = m[3][0]; a[7] = m[3][1]; a[11] = m[3][2]; a[15] = m[3][3];
    };

    double m[16]; x_to_a( xm, m );
    double p[16]; x_to_a( xp, p );
    int v[4]; kvs::OpenGL::GetViewport( v );

    kvs::Vec3d obj( 0.0, 0.0, 0.0 );
    kvs::OpenGL::UnProject(
        win.x(), win.y(), win.z(), m, p, v,
        &obj[0], &obj[1], &obj[2] );

    return kvs::Vec3( obj );
}

/* =========================
 * Crop framebuffer
 * ========================= */
inline CameraPathControlledAdaptorMulti::FrameBuffer
CameraPathControlledAdaptorMulti::crop_frame_buffer(
    const FrameBuffer& frame_buffer,
    const kvs::Vec2i& indices )
{
    KVS_ASSERT( indices[0] < static_cast<int>( m_frame_divs[0] ) );
    KVS_ASSERT( indices[1] < static_cast<int>( m_frame_divs[1] ) );

    const auto w  = BaseClass::imageWidth();
    const auto h  = BaseClass::imageHeight();
    const auto cw = w / m_frame_divs.x() + 1;
    const auto ch = h / m_frame_divs.y() + 1;
    const auto ow = cw * indices[0];
    const auto oh = ch * indices[1];

    const auto ww = ow + cw;
    const auto hh = oh + ch;

    const auto aw = ( w >= ww ) ? cw : cw - ( ww - w );
    const auto ah = ( h >= hh ) ? ch : ch - ( hh - h );

    FrameBuffer cropped;
    cropped.color_buffer.allocate( aw * ah * 4 );
    cropped.depth_buffer.allocate( aw * ah );

    auto* dst_c = cropped.color_buffer.data();
    auto* dst_d = cropped.depth_buffer.data();

    const auto offset = ow + oh * w;
    const auto* src_c = frame_buffer.color_buffer.data() + offset * 4;
    const auto* src_d = frame_buffer.depth_buffer.data() + offset;

    for ( size_t j = 0; j < ah; j++ )
    {
        std::memcpy( dst_c, src_c, aw * 4 * sizeof( kvs::UInt8 ) );
        dst_c += aw * 4;
        src_c += w * 4;

        std::memcpy( dst_d, src_d, aw * sizeof( kvs::Real32 ) );
        dst_d += aw;
        src_d += w;
    }

    return cropped;
}

/* =========================
 * Rotation (safe)
 * ========================= */
inline kvs::Quat CameraPathControlledAdaptorMulti::rotation( const kvs::Vec3& position )
{
    auto xyz_to_rtp = [&] ( const kvs::Vec3& xyz ) -> kvs::Vec3
    {
        const float x = xyz[0];
        const float y = xyz[1];
        const float z = xyz[2];

        const float r2 = x*x + y*y + z*z;
        const float r  = std::sqrt(r2);

        if (r < 1.0e-12f)
        {
            return kvs::Vec3( 0.0f, 0.0f, 0.0f );
        }

        const float c = y / r;
        const float c_clamped = clampf(c, -1.0f, 1.0f);

        const float t = std::acos( c_clamped );
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

/* =========================
 * Zoom entropy output
 * ========================= */
inline void CameraPathControlledAdaptorMulti::outputZoomEntropies(
    const std::vector<float> zoom_entropies )
{
    const auto time = BaseClass::timeStep();
    const auto output_time = kvs::String::From( time, 6, '0' );
    const auto output_filename = "output_zoom_entropies" + output_time;
    const auto filename = BaseClass::outputDirectory().baseDirectoryName() + "/" + output_filename + ".csv";

    std::ofstream file( filename );
    file << "Zoomlevel,Entropy\n";
    for ( size_t i = 0; i < zoom_entropies.size(); i++ )
    {
        file << i << "," << zoom_entropies[i] << "\n";
    }
    file.close();
}

/* =========================
 * ROI selection helpers
 * ========================= */
inline std::vector<kvs::Vec3> CameraPathControlledAdaptorMulti::biggestEntropyPoint(
    std::vector<float> entropies,
    std::vector<kvs::Vec2i> centers,
    std::vector<kvs::Real32> depthes )
{
    std::vector<kvs::Vec3> focuspoints;
    std::vector<kvs::Vec2i> top_centers;
    std::vector<kvs::Real32> top_depthes;

    for ( size_t i = 0; i < focusPointCandidateNum(); i++ )
    {
        const int maxIndex = static_cast<int>( std::distance(entropies.begin(), std::max_element(entropies.begin(), entropies.end())) );

        if ( i > 0 && depthes[maxIndex] == 1.0f )
        {
            top_depthes.push_back( top_depthes.back() );
            top_centers.push_back( top_centers.back() );
        }
        else
        {
            top_depthes.push_back( depthes[maxIndex] );
            top_centers.push_back( centers[maxIndex] );
        }

        focuspoints.push_back( { static_cast<float>(top_centers[i].x()),
                                 static_cast<float>(top_centers[i].y()),
                                 static_cast<float>(top_depthes[i]) } );

        entropies.erase( entropies.begin() + maxIndex );
        centers.erase( centers.begin() + maxIndex );
        depthes.erase( depthes.begin() + maxIndex );
    }

    return focuspoints;
}

inline std::vector<kvs::Vec3> CameraPathControlledAdaptorMulti::maximalEntropyPoint(
    std::vector<float> entropies,
    std::vector<kvs::Vec2i> centers,
    std::vector<kvs::Real32> depthes )
{
    std::vector<kvs::Vec3> focuspoints;
    std::vector<kvs::Vec2i> top_centers;
    std::vector<kvs::Real32> top_depthes;
    std::vector<float> top_entropies;

    const int dx[] = {-1,-1,-1, 0,0, 1,1,1};
    const int dy[] = {-1, 0, 1,-1,1,-1,0,1};

    const int m = static_cast<int>(m_frame_divs.y());
    const int n = static_cast<int>(m_frame_divs.x());

    std::vector<std::vector<float>> matrix(m, std::vector<float>(n));
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < n; ++j)
            matrix[i][j] = entropies[i * n + j];

    const int rows = m;
    const int cols = n;

    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            bool isLocalMaxima = true;
            for (int k = 0; k < 8; ++k)
            {
                const int ni = i + dx[k];
                const int nj = j + dy[k];
                if (ni >= 0 && ni < rows && nj >= 0 && nj < cols)
                {
                    if (matrix[i][j] <= matrix[ni][nj])
                    {
                        isLocalMaxima = false;
                        break;
                    }
                }
            }

            if (isLocalMaxima)
            {
                top_centers.push_back( centers[i*n + j] );
                top_depthes.push_back( depthes[i*n + j] );
                top_entropies.push_back( entropies[i*n + j] );
            }
        }
    }

    if ( top_depthes.empty() )
    {
        for ( size_t i = 0; i < focusPointCandidateNum(); i++ )
        {
            focuspoints.push_back( { std::ceil(frameDivisions().x()/2.0f),
                                     std::ceil(frameDivisions().y()/2.0f),
                                     depth() } );
        }
        return focuspoints;
    }

    for ( size_t i = 0; i < focusPointCandidateNum(); i++ )
    {
        if ( !top_depthes.empty() )
        {
            const int maxIndex = static_cast<int>( std::distance(top_entropies.begin(), std::max_element(top_entropies.begin(), top_entropies.end())) );
            focuspoints.push_back( { static_cast<float>(top_centers[maxIndex].x()),
                                     static_cast<float>(top_centers[maxIndex].y()),
                                     static_cast<float>(top_depthes[maxIndex]) } );

            top_entropies.erase( top_entropies.begin() + maxIndex );
            top_centers.erase( top_centers.begin() + maxIndex );
            top_depthes.erase( top_depthes.begin() + maxIndex );
        }
        else
        {
            focuspoints.push_back( focuspoints.back() );
        }
    }

    return focuspoints;
}

} // end of namespace mpi
} // end of namespace InSituVis
