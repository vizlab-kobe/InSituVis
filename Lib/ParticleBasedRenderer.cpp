#include "ParticleBasedRenderer.h"
#include <kvs/OpenGL>
#include <kvs/ParticleBasedRenderer>
#include <kvs/Xorshift128>


namespace
{

/*===========================================================================*/
/**
 *  @brief  Returns shuffled array.
 *  @param  values [in] value array
 *  @param  seed   [in] seed value for random number generator
 */
/*===========================================================================*/
template <int Dim, typename T>
kvs::ValueArray<T> ShuffleArray( const kvs::ValueArray<T>& values, kvs::UInt32 seed )
{
    KVS_ASSERT( Dim > 0 );
    KVS_ASSERT( values.size() % Dim == 0 );

    kvs::Xorshift128 rng; rng.setSeed( seed );
    kvs::ValueArray<T> ret;
    if ( values.unique() ) { ret = values; }
    else { ret = values.clone(); }

    T* p = ret.data();
    size_t size = ret.size() / Dim;

    for ( size_t i = 0; i < size; ++i )
    {
        size_t j = rng.randInteger() % ( i + 1 );
        for ( int k = 0; k < Dim; ++k )
        {
            std::swap( p[ i * Dim + k ], p[ j * Dim + k ] );
        }
    }
    return ret;
}

}

namespace InSituVis
{

namespace internal
{

/*===========================================================================*/
/**
 *  @brief  Constructs a new StochasticRendererBase class.
 *  @param  engine [in] pointer to the stochastic rendering engine
 */
/*===========================================================================*/
StochasticRendererBase::StochasticRendererBase( InSituVis::internal::StochasticRenderingEngine* engine ):
    m_width( 0 ),
    m_height( 0 ),
    m_repetition_level( 1 ),
    m_coarse_level( 1 ),
    m_enable_lod( false ),
    m_enable_refinement( false ),
    m_shader( new kvs::Shader::Lambert() ),
    m_engine( engine ),
    m_creation_time( 0 ),
    m_projection_time( 0 ),
    m_ensemble_time( 0 )
{
}

/*===========================================================================*/
/**
 *  @brief  Destroys the StochasticRendererBase class.
 */
/*===========================================================================*/
StochasticRendererBase::~StochasticRendererBase()
{
    if ( m_shader ) delete m_shader;
    if ( m_engine ) delete m_engine;
}

/*===========================================================================*/
/**
 *  @brief  Releases object resouces from GPU.
 */
/*===========================================================================*/
void StochasticRendererBase::release()
{
    KVS_ASSERT( m_engine );

    if ( m_engine->object() )
    {
        m_engine->detachObject();
        m_engine->release();
    }
}

/*===========================================================================*/
/**
 *  @brief  Executes the rendering process.
 *  @param  object [in] pointer to the object
 *  @param  camera [in] pointer to the camra
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void StochasticRendererBase::exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    startTimer();
    kvs::OpenGL::WithPushedAttrib p( GL_ALL_ATTRIB_BITS );

    // Add: Timer.
    kvs::Timer timer;
    m_creation_time = 0;
    m_projection_time = 0;
    m_ensemble_time = 0;

    const size_t width = camera->windowWidth();
    const size_t height = camera->windowHeight();
    const bool window_created = m_width == 0 && m_height == 0;
    if ( window_created )
    {
        m_width = width;
        m_height = height;
        m_ensemble_buffer.create( width, height );
        m_ensemble_buffer.clear();
        m_modelview = kvs::OpenGL::ModelViewMatrix();
        m_light_position = light->position();
        m_engine->setShader( &shader() );
        m_engine->setRepetitionLevel( m_repetition_level );
        m_engine->setShadingEnabled( kvs::RendererBase::isShadingEnabled() );

        // Add: VBO/shader craation time.
        timer.start();
        {
            m_engine->create( object, camera, light );
        }
        timer.stop();
        m_creation_time = timer.sec();
    }

    const bool window_resized = m_width != width || m_height != height;
    if ( window_resized )
    {
        m_width = width;
        m_height = height;
        m_ensemble_buffer.release();
        m_ensemble_buffer.create( width, height );
        m_ensemble_buffer.clear();
        m_engine->update( object, camera, light );
    }

    const bool object_changed = m_engine->object() != object;
    if ( object_changed )
    {
        m_ensemble_buffer.clear();
        m_engine->release();
        m_engine->setShader( &shader() );
        m_engine->setShadingEnabled( kvs::RendererBase::isShadingEnabled() );
        m_engine->create( object, camera, light );
    }

    // LOD control.
    size_t repetitions = m_repetition_level;
    kvs::Vec3 light_position = light->position();
    kvs::Mat4 modelview = kvs::OpenGL::ModelViewMatrix();
    if ( m_light_position != light_position || m_modelview != modelview )
    {
        if ( m_enable_lod )
        {
            repetitions = m_coarse_level;
        }
        m_light_position = light_position;
        m_modelview = modelview;
        m_ensemble_buffer.clear();
    }

    // Setup engine.
    const bool reset_count = !m_enable_refinement;
    if ( reset_count ) m_engine->resetRepetitions();
    m_engine->setup( object, camera, light );

    // Ensemble rendering.
    if ( reset_count ) m_ensemble_buffer.clear();
    for ( size_t i = 0; i < repetitions; i++ )
    {
        m_ensemble_buffer.bind();

        // Add: projection time.
        timer.start();
        {
            m_engine->draw( object, camera, light );
        }
        timer.stop();
        m_projection_time += timer.sec();

        m_engine->countRepetitions();
        m_ensemble_buffer.unbind();

        // Add: ensemble time
        timer.start();
        {
            m_ensemble_buffer.add();
        }
        timer.stop();
        m_ensemble_time += timer.sec();
    }
    m_ensemble_buffer.draw();

    kvs::OpenGL::Finish();
    stopTimer();
}

} // end of namespace internal

/*===========================================================================*/
/**
 *  @brief  Constructs a new ParticleBasedRenderer class.
 */
/*===========================================================================*/
ParticleBasedRenderer::ParticleBasedRenderer():
    internal::StochasticRendererBase( new Engine() )
{
}

/*===========================================================================*/
/**
 *  @brief  Constructs a new ParticleBasedRenderer class.
 *  @param  m [in] initial modelview matrix
 *  @param  p [in] initial projection matrix
 *  @param  v [in] initial viewport
 */
/*===========================================================================*/
ParticleBasedRenderer::ParticleBasedRenderer( const kvs::Mat4& m, const kvs::Mat4& p, const kvs::Vec4& v ):
    internal::StochasticRendererBase( new Engine( m, p, v ) )
{
}

/*===========================================================================*/
/**
 *  @brief  Returns true if the particle shuffling is enabled
 *  @return true, if the shuffling is enabled
 */
/*===========================================================================*/
bool ParticleBasedRenderer::isShuffleEnabled() const
{
    return static_cast<const Engine&>( engine() ).isShuffleEnabled();
}

/*===========================================================================*/
/**
 *  @brief  Returns true if the particle zooming is enabled
 *  @return true, if the zooming is enabled
 */
/*===========================================================================*/
bool ParticleBasedRenderer::isZoomingEnabled() const
{
    return static_cast<const Engine&>( engine() ).isZoomingEnabled();
}

/*===========================================================================*/
/**
 *  @brief  Sets enable-flag for the particle shuffling.
 *  @param  enable [in] enable-flag
 */
/*===========================================================================*/
void ParticleBasedRenderer::setEnabledShuffle( const bool enable )
{
    static_cast<Engine&>( engine() ).setEnabledShuffle( enable );
}

/*===========================================================================*/
/**
 *  @brief  Sets enable-flag for the particle zooming.
 *  @param  enable [in] enable-flag
 */
/*===========================================================================*/
void ParticleBasedRenderer::setEnabledZooming( const bool enable )
{
    static_cast<Engine&>( engine() ).setEnabledZooming( enable );
}

/*===========================================================================*/
/**
 *  @brief  Enable the particle shuffling.
 */
/*===========================================================================*/
void ParticleBasedRenderer::enableShuffle()
{
    static_cast<Engine&>( engine() ).enableShuffle();
}

/*===========================================================================*/
/**
 *  @brief  Enable the particle zooming.
 */
/*===========================================================================*/
void ParticleBasedRenderer::enableZooming()
{
    static_cast<Engine&>( engine() ).enableZooming();
}

/*===========================================================================*/
/**
 *  @brief  Disable the particle shuffling.
 */
/*===========================================================================*/
void ParticleBasedRenderer::disableShuffle()
{
    static_cast<Engine&>( engine() ).disableShuffle();
}

/*===========================================================================*/
/**
 *  @brief  Disable the particle zooming.
 */
/*===========================================================================*/
void ParticleBasedRenderer::disableZooming()
{
    static_cast<Engine&>( engine() ).disableZooming();
}

/*===========================================================================*/
/**
 *  @brief  Returns the initial modelview matrix.
 *  @param  initial modelview matrix
 */
/*===========================================================================*/
const kvs::Mat4& ParticleBasedRenderer::initialModelViewMatrix() const
{
    return static_cast<const Engine&>( engine() ).initialModelViewMatrix();
}

/*===========================================================================*/
/**
 *  @brief  Returns the initial projection matrix.
 *  @param  initial projection matrix
 */
/*===========================================================================*/
const kvs::Mat4& ParticleBasedRenderer::initialProjectionMatrix() const
{
    return static_cast<const Engine&>( engine() ).initialProjectionMatrix();
}

/*===========================================================================*/
/**
 *  @brief  Returns the initial viewport.
 *  @param  initial viewport
 */
/*===========================================================================*/
const kvs::Vec4& ParticleBasedRenderer::initialViewport() const
{
    return static_cast<const Engine&>( engine() ).initialViewport();
}


/*===========================================================================*/
/**
 *  @brief  Constructs a new Engine class.
 */
/*===========================================================================*/
ParticleBasedRenderer::Engine::Engine():
    m_has_normal( false ),
    m_enable_shuffle( true ),
    m_enable_zooming( true ),
    m_random_index( 0 ),
    m_initial_modelview( kvs::Mat4::Zero() ),
    m_initial_projection( kvs::Mat4::Zero() ),
    m_initial_viewport( kvs::Vec4::Zero() ),
    m_initial_object_depth( 0 ),
    m_vbo( NULL )
{
}

/*===========================================================================*/
/**
 *  @brief  Constructs a new Engine class.
 *  @param  m [in] initial modelview matrix
 *  @param  p [in] initial projection matrix
 *  @param  v [in] initial viewport
 */
/*===========================================================================*/
ParticleBasedRenderer::Engine::Engine( const kvs::Mat4& m, const kvs::Mat4& p, const kvs::Vec4& v ):
    m_has_normal( false ),
    m_enable_shuffle( true ),
    m_random_index( 0 ),
    m_initial_modelview( m ),
    m_initial_projection( p ),
    m_initial_viewport( v ),
    m_initial_object_depth( 0 ),
    m_vbo( NULL )
{
}

/*===========================================================================*/
/**
 *  @brief  Destroys the Engine class.
 */
/*===========================================================================*/
ParticleBasedRenderer::Engine::~Engine()
{
    if ( m_vbo ) delete [] m_vbo;
}

/*===========================================================================*/
/**
 *  @brief  Releases the GPU resources.
 */
/*===========================================================================*/
void ParticleBasedRenderer::Engine::release()
{
    KVS_ASSERT( m_vbo );

    if ( m_shader_program.isCreated() )
    {
        m_shader_program.release();
        for ( size_t i = 0; i < repetitionLevel(); i++ ) m_vbo[i].release();
    }
}

/*===========================================================================*/
/**
 *  @brief  Create shaders, VBO, and framebuffers.
 *  @param  point [in] pointer to the point object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void ParticleBasedRenderer::Engine::create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    kvs::PointObject* point = kvs::PointObject::DownCast( object );
    m_has_normal = point->normals().size() > 0;
    if ( !m_has_normal ) setShadingEnabled( false );

    // Create resources.
    attachObject( object );
    createRandomTexture();
    this->create_shader_program();
    this->create_buffer_object( point );

    // Initial values for calculating the object depth.
    if ( kvs::Math::IsZero( m_initial_modelview[3][3] ) )
    {
        m_initial_modelview = kvs::OpenGL::ModelViewMatrix();
    }

    if ( kvs::Math::IsZero( m_initial_projection[3][3] ) )
    {
        m_initial_projection = kvs::OpenGL::ProjectionMatrix();
    }

    if ( kvs::Math::IsZero( m_initial_viewport[2] ) )
    {
        m_initial_viewport[2] = static_cast<float>( camera->windowWidth() );
        m_initial_viewport[3] = static_cast<float>( camera->windowHeight() );
    }

    const kvs::Vec4 I( point->objectCenter(), 1.0f );
    const kvs::Vec4 O = m_initial_projection * m_initial_modelview * I;
    m_initial_object_depth = O.z();
}

/*===========================================================================*/
/**
 *  @brief  Update.
 *  @param  point [in] pointer to the point object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void ParticleBasedRenderer::Engine::update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    m_initial_viewport[2] = static_cast<float>( camera->windowWidth() );
}

/*===========================================================================*/
/**
 *  @brief  Setup.
 *  @param  point [in] pointer to the point object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void ParticleBasedRenderer::Engine::setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    // The repetition counter must be reset here.
    resetRepetitions();

    kvs::OpenGL::Enable( GL_DEPTH_TEST );
    kvs::OpenGL::Enable( GL_VERTEX_PROGRAM_POINT_SIZE );
    m_random_index = m_shader_program.attributeLocation("random_index");

    const kvs::Mat4 M = kvs::OpenGL::ModelViewMatrix();
    const kvs::Mat4 P = kvs::OpenGL::ProjectionMatrix();
    m_shader_program.bind();
    m_shader_program.setUniform( "ModelViewMatrix", M );
    m_shader_program.setUniform( "ProjectionMatrix", P );
    m_shader_program.unbind();
}

/*===========================================================================*/
/**
 *  @brief  Draw an ensemble.
 *  @param  point [in] pointer to the point object
 *  @param  camera [in] pointer to the camera
 *  @param  light [in] pointer to the light
 */
/*===========================================================================*/
void ParticleBasedRenderer::Engine::draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light )
{
    kvs::PointObject* point = kvs::PointObject::DownCast( object );

    kvs::VertexBufferObject::Binder bind1( m_vbo[ repetitionCount() ] );
    kvs::ProgramObject::Binder bind2( m_shader_program );
    kvs::Texture::Binder bind3( randomTexture() );
    {
        const kvs::Mat4& m0 = m_initial_modelview;
        const float scale0 = kvs::Vec3( m0[0][0], m0[1][0], m0[2][0] ).length();
        const float width0 = m_initial_viewport[2];
        const float height0 = m_initial_viewport[3];

        const kvs::Mat4 m = kvs::OpenGL::ModelViewMatrix();
        const float scale = kvs::Vec3( m[0][0], m[1][0], m[2][0] ).length();
        const float width = static_cast<float>( camera->windowWidth() );
        const float height = static_cast<float>( camera->windowHeight() );

        const float Cr = ( width / width0 ) * ( height / height0 );
        const float Cs = scale / scale0;
        const float D0 = m_initial_object_depth;
        const float object_scale = Cr * Cs;
        const float object_depth = object_scale * D0;

        m_shader_program.setUniform( "object_scale", object_scale );
        m_shader_program.setUniform( "object_depth", object_depth );
        m_shader_program.setUniform( "random_texture", 0 );
        m_shader_program.setUniform( "random_texture_size_inv", 1.0f / randomTextureSize() );
        m_shader_program.setUniform( "screen_scale", kvs::Vec2( width * 0.5f, height * 0.5f ) );

        const size_t nvertices = point->numberOfVertices();
        const size_t rem = nvertices % repetitionLevel();
        const size_t quo = nvertices / repetitionLevel();
        const size_t count = quo + ( repetitionCount() < rem ? 1 : 0 );
        const size_t coord_size = count * sizeof(kvs::Real32) * 3;
        const size_t color_size = count * sizeof(kvs::UInt8) * 3;

        // Enable coords.
        KVS_GL_CALL( glEnableClientState( GL_VERTEX_ARRAY ) );
        KVS_GL_CALL( glVertexPointer( 3, GL_FLOAT, 0, (GLbyte*)NULL + 0 ) );

        // Enable colors.
        KVS_GL_CALL( glEnableClientState( GL_COLOR_ARRAY ) );
        KVS_GL_CALL( glColorPointer( 3, GL_UNSIGNED_BYTE, 0, (GLbyte*)NULL + coord_size ) );

        // Enable normals.
        if ( m_has_normal )
        {
            KVS_GL_CALL( glEnableClientState( GL_NORMAL_ARRAY ) );
            KVS_GL_CALL( glNormalPointer( GL_FLOAT, 0, (GLbyte*)NULL + coord_size + color_size ) );
        }

        // Enable random index.
        KVS_GL_CALL( glEnableVertexAttribArray( m_random_index ) );
        KVS_GL_CALL( glVertexAttribPointer( m_random_index, 2, GL_UNSIGNED_SHORT, GL_FALSE, 0, (GLubyte*)NULL + 0 ) );

        // Draw.
        KVS_GL_CALL( glDrawArrays( GL_POINTS, 0, count ) );

        // Disable coords.
        KVS_GL_CALL( glDisableClientState( GL_VERTEX_ARRAY ) );

        // Disable colors.
        KVS_GL_CALL( glDisableClientState( GL_COLOR_ARRAY ) );

        // Disable normals.
        if ( m_has_normal )
        {
            KVS_GL_CALL( glDisableClientState( GL_NORMAL_ARRAY ) );
        }

        // Disable random index.
        KVS_GL_CALL( glDisableVertexAttribArray( m_random_index ) );
    }
    //    countRepetitions();
}

/*===========================================================================*/
/**
 *  @brief  Creates shader program.
 */
/*===========================================================================*/
void ParticleBasedRenderer::Engine::create_shader_program()
{
    kvs::ShaderSource vert( "PBR_zooming.vert" );
    kvs::ShaderSource frag( "PBR_zooming.frag" );

    if ( isShadingEnabled() )
    {
        switch ( shader().type() )
        {
        case kvs::Shader::LambertShading: frag.define("ENABLE_LAMBERT_SHADING"); break;
        case kvs::Shader::PhongShading: frag.define("ENABLE_PHONG_SHADING"); break;
        case kvs::Shader::BlinnPhongShading: frag.define("ENABLE_BLINN_PHONG_SHADING"); break;
        default: break; // NO SHADING
        }

        if ( kvs::OpenGL::Boolean( GL_LIGHT_MODEL_TWO_SIDE ) == GL_TRUE )
        {
            frag.define("ENABLE_TWO_SIDE_LIGHTING");
        }
    }

    if ( isZoomingEnabled() )
    {
        vert.define("ENABLE_PARTICLE_ZOOMING");
        frag.define("ENABLE_PARTICLE_ZOOMING");
    }

    m_shader_program.build( vert, frag );
    m_shader_program.bind();
    m_shader_program.setUniform( "shading.Ka", shader().Ka );
    m_shader_program.setUniform( "shading.Kd", shader().Kd );
    m_shader_program.setUniform( "shading.Ks", shader().Ks );
    m_shader_program.setUniform( "shading.S",  shader().S );
    m_shader_program.unbind();
}

/*===========================================================================*/
/**
 *  @brief  Creates buffer objects.
 *  @param  point [in] pointer to the point object
 */
/*===========================================================================*/
void ParticleBasedRenderer::Engine::create_buffer_object( const kvs::PointObject* point )
{
    KVS_ASSERT( point->coords().size() == point->colors().size() );

    kvs::ValueArray<kvs::Real32> coords = point->coords();
    kvs::ValueArray<kvs::UInt8> colors = point->colors();
    kvs::ValueArray<kvs::Real32> normals = point->normals();
    if ( m_enable_shuffle )
    {
        kvs::UInt32 seed = 12345678;
        coords = ::ShuffleArray<3>( point->coords(), seed );
        colors = ::ShuffleArray<3>( point->colors(), seed );
        if ( m_has_normal )
        {
            normals = ::ShuffleArray<3>( point->normals(), seed );
        }
    }

    if ( !m_vbo ) m_vbo = new kvs::VertexBufferObject [ repetitionLevel() ];

    const size_t nvertices = point->numberOfVertices();
    const size_t rem = nvertices % repetitionLevel();
    const size_t quo = nvertices / repetitionLevel();
    for ( size_t i = 0; i < repetitionLevel(); i++ )
    {
        const size_t count = quo + ( i < rem ? 1 : 0 );
        const size_t first = quo * i + kvs::Math::Min( i, rem );
        const size_t coord_size = count * sizeof(kvs::Real32) * 3;
        const size_t color_size = count * sizeof(kvs::UInt8) * 3;
        const size_t normal_size = m_has_normal ? count * sizeof(kvs::Real32) * 3 : 0;
        const size_t byte_size = coord_size + color_size + normal_size;
        m_vbo[i].create( byte_size );

        m_vbo[i].bind();
        m_vbo[i].load( coord_size, coords.data() + first * 3, 0 );
        m_vbo[i].load( color_size, colors.data() + first * 3, coord_size );
        if ( m_has_normal )
        {
            m_vbo[i].load( normal_size, normals.data() + first * 3, coord_size + color_size );
        }
        m_vbo[i].unbind();
    }
}

} // end of namespace InSituVis
