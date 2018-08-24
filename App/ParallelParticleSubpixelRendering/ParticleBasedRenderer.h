#pragma once
#include <kvs/ParticleBasedRenderer>
#include <kvs/PointObject>
#include <kvs/ObjectBase>
#include <kvs/Camera>
#include <kvs/Light>


namespace local
{

class ParticleBasedRenderer : public kvs::ParticleBasedRenderer
{
    kvsModule( local::ParticleBasedRenderer, Renderer );
    kvsModuleBaseClass( kvs::ParticleBasedRenderer );

private:
    // Add:
    float m_composition_time;

public:
    ParticleBasedRenderer();

    void exec( kvs::ObjectBase* object, kvs::Camera* camera, kvs::Light* light );

    // Add:
    float compositionTime() const { return m_composition_time; }
    kvs::ValueArray<kvs::UInt8> subpixelizedColorBuffer() const;
    kvs::ValueArray<kvs::Real32> subpixelizedDepthBuffer() const;
    void subpixelAveraging(
        const kvs::ValueArray<kvs::UInt8>& subpixelized_color_buffer,
        const kvs::ValueArray<kvs::Real32>& subpixelized_depth_buffer );
    void drawImage();
    kvs::ValueArray<kvs::UInt8> colorBuffer() { return BaseClass::colorData(); }

private:
    void create_image( const kvs::PointObject* point, const kvs::Camera* camera, const kvs::Light* light );
    void project_particle( const kvs::PointObject* point, const kvs::Camera* camera, const kvs::Light* light );
};

} // end of namespace local
