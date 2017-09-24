#include "MovieRenderer.h"
#include <kvs/Camera>
#include <kvs/Message>
#include <kvs/IgnoreUnusedVariable>
#include <kvs/Vector2>
#include <kvs/OpenGL>

namespace InSituVis
{

/*===========================================================================*/
/**
 *  @brief  Constructs a new MovieRenderer class.
 *  @param  type [in] video type
 */
/*===========================================================================*/
MovieRenderer::MovieRenderer( const Type type ):
    m_type( type )
{
}

/*===========================================================================*/
/**
 *  @brief  Destructs the MovieRenderer class.
 */
/*===========================================================================*/
MovieRenderer::~MovieRenderer()
{
}

/*===========================================================================*/
/**
 *  @brief  Renders the grabbed frame.
 *  @param  object [in] pointer to the video object
 *  @param  camera [in] pointer to the camera in KVS
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void MovieRenderer::exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    kvs::IgnoreUnusedVariable( light );

    BaseClass::startTimer();
    InSituVis::MovieObject* video = reinterpret_cast<InSituVis::MovieObject*>( object );

    video->device().setNextFrameIndex( video->device().nextFrameIndex() - 1 );

    if ( video->device().nextFrameIndex() == video->device().numberOfFrames() )
    {
        video->device().setNextFrameIndex(0);
//        video->device().setNextFrameIndex( video->device().numberOfFrames() - 1 );
    }

    const IplImage* frame = video->device().queryFrame();
//    if ( !frame ) return;

    kvs::OpenGL::WithPushedAttrib p( GL_ALL_ATTRIB_BITS );

    if ( !m_texture.isValid() )
    {
        this->create_texture( video );
    }

    kvs::OpenGL::Disable( GL_DEPTH_TEST );
    kvs::OpenGL::Enable( GL_TEXTURE_2D );

    switch ( m_type )
    {
    case MovieRenderer::Centering:
    {
        this->centering( camera->windowWidth(), camera->windowHeight() );
        break;
    }
    default: break;
    }

    const int width = frame->width;
    const int height = frame->height;
    const char* data = frame->imageData; // BGRBGRBGR...
    m_texture.bind();
    m_texture.load( width, height, data );

    kvs::OpenGL::WithPushedMatrix p1( GL_MODELVIEW );
    p1.loadIdentity();
    {
        kvs::OpenGL::WithPushedMatrix p2( GL_PROJECTION );
        p2.loadIdentity();
        {
            kvs::OpenGL::SetOrtho( m_left, m_right, m_bottom, m_top, -1, 1 );
            kvs::OpenGL::Begin( GL_QUADS );
/* mirror */
/*
            kvs::OpenGL::TexCoordVertex( kvs::Vec2( 0.0f, 0.0f ), kvs::Vec2( 1.0f, 1.0f ) );
            kvs::OpenGL::TexCoordVertex( kvs::Vec2( 0.0f, 1.0f ), kvs::Vec2( 1.0f, 0.0f ) );
            kvs::OpenGL::TexCoordVertex( kvs::Vec2( 1.0f, 1.0f ), kvs::Vec2( 0.0f, 0.0f ) );
            kvs::OpenGL::TexCoordVertex( kvs::Vec2( 1.0f, 0.0f ), kvs::Vec2( 0.0f, 1.0f ) );
*/
/* normal */
            kvs::OpenGL::TexCoordVertex( kvs::Vec2( 0.0f, 0.0f ), kvs::Vec2( 0.0f, 1.0f ) );
            kvs::OpenGL::TexCoordVertex( kvs::Vec2( 0.0f, 1.0f ), kvs::Vec2( 0.0f, 0.0f ) );
            kvs::OpenGL::TexCoordVertex( kvs::Vec2( 1.0f, 1.0f ), kvs::Vec2( 1.0f, 0.0f ) );
            kvs::OpenGL::TexCoordVertex( kvs::Vec2( 1.0f, 0.0f ), kvs::Vec2( 1.0f, 1.0f ) );

            kvs::OpenGL::End();
        }
    }

    m_texture.unbind();

    kvs::OpenGL::SetClearDepth( 1000 );
    BaseClass::stopTimer();
}

/*===========================================================================*/
/**
 *  @brief  Create texture.
 *  @param  video [in] pointer to the video object
 */
/*===========================================================================*/
void MovieRenderer::create_texture( const InSituVis::MovieObject* movie )
{
    const double width  = movie->width();
    const double height = movie->height();
    m_initial_aspect_ratio = width / height;
    m_left = 0.0;
    m_right = 1.0;
    m_bottom = 0.0;
    m_top = 1.0;

    if ( movie->type() == InSituVis::MovieObject::Gray8 )
    {
        m_texture.setPixelFormat( GL_INTENSITY8, GL_LUMINANCE, GL_UNSIGNED_BYTE );
    }
    else if ( movie->type() == InSituVis::MovieObject::Color24 )
    {
#ifdef GL_BGR_EXT
        m_texture.setPixelFormat( GL_RGB8, GL_BGR_EXT, GL_UNSIGNED_BYTE );
#else
        m_texture.setPixelFormat( GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE );
#endif
    }
    else
    {
        kvsMessageError("Unknown pixel color type.");
    }

    const IplImage* frame = movie->device().queryFrame();
    m_texture.create( frame->width, frame->height );
}

/*===========================================================================*/
/**
 *  @brief  Renders the grabbed frame in the center of the window.
 *  @param  width [in] window width
 *  @param  height [in] window height
 */
/*===========================================================================*/
void MovieRenderer::centering( const double width, const double height )
{
    double current_aspect_ratio = width / height;
    double aspect_ratio = current_aspect_ratio / m_initial_aspect_ratio;
    if( aspect_ratio >= 1.0 )
    {
        m_left = ( 1.0 - aspect_ratio ) * 0.5;
        m_right = ( 1.0 + aspect_ratio ) * 0.5;
        m_bottom = 0.0;
        m_top = 1.0;
    }
    else
    {
        m_left = 0.0;
        m_right = 1.0;
        m_bottom = ( 1.0 - 1.0 / aspect_ratio ) * 0.5;
        m_top = ( 1.0 + 1.0 / aspect_ratio ) * 0.5;
    }
}

} // end of namespace InSituVis
