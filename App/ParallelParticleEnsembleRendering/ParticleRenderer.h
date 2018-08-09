#pragma once
#include "Input.h"
#include <kvs/StochasticRendererBase>
#include <kvs/ParticleBasedRenderer>

namespace local
{

  kvs::StochasticRendererBase* ParticleRenderer( const Input& input, float min_value, float max_value );
  //kvs::glsl::ParticleBasedRenderer* ParticleRenderer(const Input& input, float min_value, float max_value );
} // end of namespace local
