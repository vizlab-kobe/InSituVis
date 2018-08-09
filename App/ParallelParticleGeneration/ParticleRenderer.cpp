#include "ParticleRenderer.h"
#include <kvs/ParticleBasedRenderer>
#include <ParticleBasedRendering/Lib/AdaptiveParticleBasedRenderer.h>
#include <kvs/TransferFunction>
#include <kvs/RGBFormulae>

namespace local
{
  kvs::glsl::ParticleBasedRenderer* ParticleRenderer( const Input& input, float min_value, float max_value )
  //kvs::ParticleBasedRenderer* ParticleRenderer( const Input& input, float min_value, float max_value )
{
  float ka = 0.5;
  float kd = 0.5;
  float ks = 0.3;
  float s = 100.0;
  kvs::glsl::ParticleBasedRenderer* renderer = new kvs::glsl::ParticleBasedRenderer();
  //kvs::ParticleBasedRenderer* renderer = new kvs::ParticleBasedRenderer();
  renderer->setRepetitionLevel(input.repetitions);
  //renderer->setSubpixelLevel( 10 );
  //renderer->setShader( kvs::Shader::BlinnPhong( ka, kd, ks, s ) );
  renderer->disableShading();
  return renderer;
}

} // end of namespace local
