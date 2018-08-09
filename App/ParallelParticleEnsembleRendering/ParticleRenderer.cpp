#include "ParticleRenderer.h"
#include <kvs/ParticleBasedRenderer>
#include <ParticleBasedRendering/Lib/AdaptiveParticleBasedRenderer.h>
#include <kvs/TransferFunction>
#include <kvs/RGBFormulae>

namespace local
{

  kvs::StochasticRendererBase* ParticleRenderer( const Input& input, float min_value, float max_value )
  //kvs::ParticleBasedRenderer* ParticleRenderer( const Input& input, float min_value, float max_value )
  //kvs::glsl::ParticleBasedRenderer* ParticleRenderer( const Input& input, float min_value, float max_value )
{
  float ka = 0.5;
  float kd = 0.5;
  float ks = 0.3;
  float s = 100.0;
  //  kvs::StochasticRendererBase* renderer = new kvs::ParticleBasedRenderer();
  if ( input.sampling_method == Input::Point )
    {
      typedef ParticleBasedRendering::AdaptiveParticleBasedRenderer Renderer;
      Renderer* adaptive_renderer = new Renderer( min_value , max_value);
      adaptive_renderer->setBaseOpacity( input.base_opacity );
      adaptive_renderer->setSamplingStep( input.step );
      
      if ( !input.tf_filename.empty() )
	{
	  const kvs::TransferFunction tfunc( input.tf_filename );
	  adaptive_renderer->setTransferFunction( tfunc );
	}
      else
	{
	  const kvs::TransferFunction tfunc( kvs::RGBFormulae::Hot(256) );
	  //const kvs::TransferFunction tfunc( 256 );
	  adaptive_renderer->setTransferFunction( tfunc );
	}
      adaptive_renderer->setShader( kvs::Shader::BlinnPhong( ka, kd, ks, s) );
      return adaptive_renderer;
    }

  kvs::glsl::ParticleBasedRenderer* renderer = new kvs::glsl::ParticleBasedRenderer();
  
  //renderer->setShader( kvs::Shader::BlinnPhong( ka, kd, ks, s ) );
  renderer->disableShading();
  //return new kvs::glsl::ParticleBasedRenderer();
  return renderer;
}

} // end of namespace local
