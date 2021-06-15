#pragma once
#include <kvs/Camera>
#include <kvs/Light>
#include <kvs/ObjectBase>
#include <kvs/PointObject>
#include <kvs/RendererBase>
#include <kvs/Shader>
#include <kvs/Matrix44>
#include <kvs/Module>
#include <kvs/EnsembleAverageBuffer>
#include <kvs/StochasticRenderingEngine>
#include <kvs/ProgramObject>
#include <kvs/VertexBufferObject>


namespace InSituVis
{

namespace internal
{

class StochasticRenderingEngine : public kvs::StochasticRenderingEngine
{
    typedef kvs::StochasticRenderingEngine BaseClass;

public:
    void resetRepetitions() { BaseClass::resetRepetitions(); }
    void countRepetitions() { BaseClass::countRepetitions(); }
    void attachObject( const kvs::ObjectBase* object ) { BaseClass::attachObject( object ); }
    void detachObject() { BaseClass::detachObject(); }
};

class StochasticRendererBase : public kvs::RendererBase
{
    kvsModule( InSituVis::internal::StochasticRendererBase, Renderer );

    friend class StochasticRenderingCompositor;

private:

    size_t m_width;
    size_t m_height;
    size_t m_repetition_level; ///< repetition level
    size_t m_coarse_level; ///< repetition level for the coarse rendering (LOD)
    bool m_enable_lod; ///< flag for LOD rendering
    bool m_enable_refinement; ///< flag for progressive refinement rendering
    kvs::Mat4 m_modelview; ///< modelview matrix used for LOD control
    kvs::Vec3 m_light_position; ///< light position used for LOD control
    kvs::EnsembleAverageBuffer m_ensemble_buffer; ///< ensemble averaging buffer
    kvs::Shader::ShadingModel* m_shader; ///< shading method
    InSituVis::internal::StochasticRenderingEngine* m_engine; ///< rendering engine

    // Add
    double m_creation_time; ///< VBO/shader creation time
    double m_projection_time; ///< particle projection time
    double m_ensemble_time; ///< ensemble averaging time

public:

    StochasticRendererBase( InSituVis::internal::StochasticRenderingEngine* engine );
    virtual ~StochasticRendererBase();

    virtual void exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void release();
    size_t windowWidth() const { return m_width; }
    size_t windowHeight() const { return m_height; }
    size_t repetitionLevel() const { return m_repetition_level; }
    bool isLODControlEnabled() const { return m_enable_lod; }
    bool isRefinementEnabled() const { return m_enable_refinement; }
    void setRepetitionLevel( const size_t repetition_level ) { m_repetition_level = repetition_level; }
    void setEnabledLODControl( const bool enable ) { m_enable_lod = enable; }
    void setEnabledRefinement( const bool enable ) { m_enable_refinement = enable; }
    void enableLODControl() { this->setEnabledLODControl( true ); }
    void enableRefinement() { this->setEnabledRefinement( true ); }
    void disableLODControl() { this->setEnabledLODControl( false ); }
    void disableRefinement() { this->setEnabledRefinement( false ); }
    const kvs::Shader::ShadingModel& shader() const { return *m_shader; }
    const InSituVis::internal::StochasticRenderingEngine& engine() const { return *m_engine; }
    template <typename ShadingType>
    void setShader( const ShadingType shader )
    {
        if ( m_shader ) { delete m_shader; }
        m_shader = new ShadingType( shader );
    }

    double creationTime() const {return m_creation_time; }
    double projectionTime() const { return m_projection_time; }
    double ensembleTime() const { return m_ensemble_time; }

protected:
    kvs::Shader::ShadingModel& shader() { return *m_shader; }
    InSituVis::internal::StochasticRenderingEngine& engine() { return *m_engine; }
};

} // end of namespace internal


class ParticleBasedRenderer : public InSituVis::internal::StochasticRendererBase
{
    kvsModule( InSituVis::ParticleBasedRenderer, Renderer );
    kvsModuleBaseClass( InSituVis::internal::StochasticRendererBase );

public:
    class Engine;

public:
    ParticleBasedRenderer();
    ParticleBasedRenderer( const kvs::Mat4& m, const kvs::Mat4& p, const kvs::Vec4& v );
    bool isShuffleEnabled() const;
    bool isZoomingEnabled() const;
    void setEnabledShuffle( const bool enable );
    void setEnabledZooming( const bool enable );
    void enableShuffle();
    void enableZooming();
    void disableShuffle();
    void disableZooming();
    const kvs::Mat4& initialModelViewMatrix() const;
    const kvs::Mat4& initialProjectionMatrix() const;
    const kvs::Vec4& initialViewport() const;
};

class ParticleBasedRenderer::Engine : public InSituVis::internal::StochasticRenderingEngine
{
private:
    bool m_has_normal; ///< check flag for the normal array
    bool m_enable_shuffle; ///< flag for shuffling particles
    bool m_enable_zooming; ///< flag for zooming particles
    size_t m_random_index; ///< index used for refering the random texture
    kvs::Mat4 m_initial_modelview; ///< initial modelview matrix
    kvs::Mat4 m_initial_projection; ///< initial projection matrix
    kvs::Vec4 m_initial_viewport; ///< initial viewport
    float m_initial_object_depth; ///< initial object depth
    kvs::ProgramObject m_shader_program; ///< zooming shader program
    kvs::VertexBufferObject* m_vbo; ///< vertex buffer objects for each repetition

public:
    Engine();
    Engine( const kvs::Mat4& m, const kvs::Mat4& p, const kvs::Vec4& v );
    virtual ~Engine();
    void release();
    void create( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void update( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void setup( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void draw( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    bool isShuffleEnabled() const { return m_enable_shuffle; }
    bool isZoomingEnabled() const { return m_enable_zooming; }
    void setEnabledShuffle( const bool enable ) { m_enable_shuffle = enable; }
    void setEnabledZooming( const bool enable ) { m_enable_zooming = enable; }
    void enableShuffle() { this->setEnabledShuffle( true ); }
    void enableZooming() { this->setEnabledZooming( true ); }
    void disableShuffle() { this->setEnabledShuffle( false ); }
    void disableZooming() { this->setEnabledZooming( false ); }
    const kvs::Mat4& initialModelViewMatrix() const { return m_initial_modelview; }
    const kvs::Mat4& initialProjectionMatrix() const { return m_initial_projection; }
    const kvs::Vec4& initialViewport() const { return m_initial_viewport; }

private:
    void create_shader_program();
    void create_buffer_object( const kvs::PointObject* point );
};

} // end of namespace InSituVis
