#include "ParticleGenerator.h"
#include <kvs/CellByCellUniformSampling>
#include <kvs/CellByCellMetropolisSampling>
#include <kvs/CellByCellRejectionSampling>
#include <kvs/CellByCellLayeredSampling>
#include <ParticleBasedRendering/Lib/CellByCellPointSampling.h>

namespace local
{

  kvs::PointObject* ParticleGenerator( const Input& input, const kvs::VolumeObjectBase* volume, kvs::TransferFunction& tfunc, kvs::Camera* camera )
{
    kvs::PointObject* object = NULL;
    
    const int repeats = input.sublevel * input.sublevel;
    
    if ( input.sampling_method == 0 )
      {
	object = new kvs::CellByCellUniformSampling( camera, volume, repeats, input.step, tfunc);  
	//object = new kvs::CellByCellUniformSampling( camera, volume, input.sublevel, input.step, tfunc);  
      }
    else if ( input.sampling_method == 1 )
      {
	object = new kvs::CellByCellMetropolisSampling( camera, volume, repeats,input.step, tfunc);
	//object = new kvs::CellByCellMetropolisSampling( camera, volume, input.sublevel ,input.step, tfunc);
      }
    else if ( input.sampling_method == 2 )
      {
	object = new kvs::CellByCellRejectionSampling( camera, volume, repeats ,input.step, tfunc);
	//object = new kvs::CellByCellRejectionSampling( camera, volume, input.sublevel, input.step, tfunc);
      }
    else if ( input.sampling_method == 3 )
      {
	object = new kvs::CellByCellLayeredSampling( camera, volume, repeats, input.step, tfunc);
	//object = new kvs::CellByCellLayeredSampling( camera, volume, input.sublevel, input.step, tfunc);
      }
    else if ( input.sampling_method == 4 )
      {
	const float base_opacity = input.base_opacity;
	object = new ParticleBasedRendering::CellByCellPointSampling(camera, volume, repeats, input.step, base_opacity);
      }

    return object;
}

} // end of namespace local
