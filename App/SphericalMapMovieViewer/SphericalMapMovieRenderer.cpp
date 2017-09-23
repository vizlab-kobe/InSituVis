#include "SphericalMapMovieRenderer.h"
#include <kvs/Camera>
#include <kvs/ImageObject>
#include <kvs/Message>
#include <kvs/IgnoreUnusedVariable>
#include <kvs/OpenGL>
#include <kvs/ShaderSource>


namespace local
{

namespace opencv
{

/*==========================================================================*/
/**
 *  @brief  Constructs a new SphericalMapMovieRenderer class.
 *  @param  type [in] rendering type
 */
/*==========================================================================*/
SphericalMapMovieRenderer::SphericalMapMovieRenderer( const SphericalMapMovieRenderer::Type& type )
{
    m_type = type;
}

/*==========================================================================*/
/**
 *  @brief  Destruct the SphericalMapMovieRenderer class.
 */
/*==========================================================================*/
SphericalMapMovieRenderer::~SphericalMapMovieRenderer()
{
}

/*==========================================================================*/
/**
 *  @brief  Executes the rendering process.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*==========================================================================*/
void SphericalMapMovieRenderer::exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    kvs::IgnoreUnusedVariable( light );

    BaseClass::startTimer();
    local::opencv::MovieObject* video = reinterpret_cast<local::opencv::MovieObject*>( object );

//    video->device().setNextFrameIndex( video->device().nextFrameIndex() - 1 );

    if ( video->device().nextFrameIndex() == video->device().numberOfFrames() )
    {
        video->device().setNextFrameIndex(0);
//        video->device().setNextFrameIndex( video->device().numberOfFrames() - 1 );
    }

    if ( !m_texture.isCreated() )
    {
        this->create_texture( video );
    }

    if ( !m_shader_program.isCreated() )
    {
        this->create_shader_program();
    }

//    if ( m_type == Centering )
//    {
//        this->center_alignment( camera->windowWidth(), camera->windowHeight() );
//    }

//    kvs::OpenGL::SetClearDepth( 1.0 );
    kvs::OpenGL::Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    kvs::OpenGL::WithPushedAttrib p( GL_ALL_ATTRIB_BITS );
    {
        kvs::OpenGL::Disable( GL_DEPTH_TEST );
        kvs::OpenGL::Disable( GL_TEXTURE_1D );
        kvs::OpenGL::Disable( GL_TEXTURE_3D );
        kvs::OpenGL::Enable( GL_TEXTURE_2D );

        kvs::ProgramObject::Binder shader( m_shader_program );
        kvs::Texture::Binder unit( m_texture, 0 );

        const IplImage* frame = video->device().queryFrame();
        const int width = frame->width;
        const int height = frame->height;
        const char* data = frame->imageData; // BGRBGRBGR...
        m_texture.load( width, height, data );

        m_shader_program.setUniform( "spherical_map", 0 );
        m_shader_program.setUniform( "R", object->xform().rotation() );
        m_shader_program.setUniform( "image_size", kvs::Vec2( video->width(), video->height() ) );
        m_shader_program.setUniform( "screen_size", kvs::Vec2( camera->windowWidth(), camera->windowHeight() ) );

        kvs::OpenGL::WithPushedMatrix p1( GL_MODELVIEW );
        p1.loadIdentity();
        {
            kvs::OpenGL::WithPushedMatrix p2( GL_PROJECTION );
            p2.loadIdentity();
            {
                kvs::OpenGL::SetOrtho( m_left, m_right, m_bottom, m_top, -1, 1 );
                kvs::OpenGL::Begin( GL_QUADS );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 0.0, 0.0 ), kvs::Vec2( 0.0, 1.0 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 0.0, 1.0 ), kvs::Vec2( 0.0, 0.0 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 1.0, 1.0 ), kvs::Vec2( 1.0, 0.0 ) );
                kvs::OpenGL::TexCoordVertex( kvs::Vec2( 1.0, 0.0 ), kvs::Vec2( 1.0, 1.0 ) );
                kvs::OpenGL::End();
            }
        }
    }

    BaseClass::stopTimer();
}

void SphericalMapMovieRenderer::create_shader_program()
{
    kvs::ShaderSource vert("spherical_map.vert");
    kvs::ShaderSource frag("spherical_map.frag");
    m_shader_program.build( vert, frag );
}

/*==========================================================================*/
/**
 *  @brief  Creates the texture region on the GPU.
 *  @param  image [in] pointer to the image object
 */
/*==========================================================================*/
void SphericalMapMovieRenderer::create_texture( const local::opencv::MovieObject* movie )
{
    const double width  = movie->width();
    const double height = movie->height();
    m_initial_aspect_ratio = width / height;
    m_left = 0.0;
    m_right = 1.0;
    m_bottom = 0.0;
    m_top = 1.0;

    if ( movie->type() == local::opencv::MovieObject::Gray8 )
    {
        m_texture.setPixelFormat( GL_INTENSITY8, GL_LUMINANCE, GL_UNSIGNED_BYTE );
    }
    else if ( movie->type() == local::opencv::MovieObject::Color24 )
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

//    const IplImage* frame = movie->device().queryFrame();

    kvs::Texture::SetEnv( GL_TEXTURE_ENV_MODE, GL_REPLACE );
    m_texture.setWrapS( GL_REPEAT );
    m_texture.setWrapT( GL_REPEAT );
    m_texture.setMagFilter( GL_LINEAR );
    m_texture.setMinFilter( GL_LINEAR );
//    m_texture.create( frame->width, frame->height );
    m_texture.create( width, height );
}

/*==========================================================================*/
/**
 *  @brief  Calculates centering parameters.
 *  @param  width [in] image width
 *  @param  height [in] image height
 */
/*==========================================================================*/
void SphericalMapMovieRenderer::center_alignment( const double width, const double height )
{
    const double current_aspect_ratio = width / height;
    const double aspect_ratio = current_aspect_ratio / m_initial_aspect_ratio;
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

} // end of namespace opencv

} // end of namespace local
