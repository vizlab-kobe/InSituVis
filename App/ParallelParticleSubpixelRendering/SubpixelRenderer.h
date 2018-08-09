#pragma once

#include <kvs/VolumeRendererBase>
#include <kvs/ParticleBuffer>
#include <kvs/Module>
#include <kvs/Deprecated>
#include "Input.h"

namespace local
{
  class ParticleBufferCompositor;
  
  class SubpixelRenderer : public kvs::VolumeRendererBase
  {
    
    //kvsModule( kvs::ParticleBasedRenderer, Renderer );
    kvsModuleBaseClass( kvs::VolumeRendererBase );  

  protected:

    // Reference data (NOTE: not allocated in thie class).
    const kvs::PointObject* m_ref_point; ///< pointer to the point data

    bool m_enable_rendering; ///< rendering flag
    size_t m_subpixel_level; ///< number of divisions in a pixel
    kvs::ParticleBuffer* m_buffer; ///< particle buffer
 
  private:
    int m_rank;
    int m_nnodes;
    double m_composition_time;
    
  public:

    //ParticleBasedRenderer();
    //ParticleBasedRenderer( const kvs::PointObject* point, const size_t subpixel_level = 1 );
    //virtual ~ParticleBasedRenderer();

    SubpixelRenderer(int rank, int nnodes );
    SubpixelRenderer(int rank, int nnodes, const kvs::PointObject* point, const size_t subpixel_level = 1 );
    virtual ~SubpixelRenderer();
    void exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );
    void attachPointObject( const kvs::PointObject* point ) { m_ref_point = point; }
    void setSubpixelLevel( const size_t subpixel_level ) { m_subpixel_level = subpixel_level; }
    const kvs::ParticleBuffer* particleBuffer() const { return m_buffer; }
    size_t subpixelLevel() const { return m_subpixel_level; }
    void enableRendering() { m_enable_rendering = true; }
    void disableRendering() { m_enable_rendering = false; }
    double getCompositionTime() { return m_composition_time; }

  protected:

    bool createParticleBuffer( const size_t width, const size_t height, const size_t subpixel_level );
    void cleanParticleBuffer();
    void deleteParticleBuffer();

  private:

    void create_image( const kvs::PointObject* point, const kvs::Camera* camera, const kvs::Light* light );
    void project_particle( const kvs::PointObject* point, const kvs::Camera* camera, const kvs::Light* light );
    kvs::ValueArray<kvs::UInt8> get_color( const kvs::PointObject* point ) const;
    void createImage( kvs::ValueArray<kvs::UInt8>* color, kvs::ValueArray<kvs::Real32>* depth, kvs::ValueArray<kvs::UInt8> color_buffer, kvs::ValueArray<kvs::Real32> depth_buffer );

  };
}; // end of namespace local
