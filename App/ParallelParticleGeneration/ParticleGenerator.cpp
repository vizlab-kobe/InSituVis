#include "ParticleGenerator.h"
#include <kvs/CellByCellUniformSampling>
#include <kvs/CellByCellMetropolisSampling>
#include <kvs/CellByCellRejectionSampling>
#include <kvs/CellByCellLayeredSampling>
#include <math.h>
#include <ParticleBasedRendering/Lib/CellByCellSubpixelPointSampling.h>

namespace local
{

kvs::PointObject* ParticleGenerator( const Input& input, const kvs::VolumeObjectBase* volume, kvs::TransferFunction& tfunc )
{
    kvs::PointObject* object = NULL;
    size_t subpixel_level = sqrt(input.repetitions);
    
    kvs::Camera* camera = new kvs::Camera();
    camera->setWindowSize(input.width, input.height );
    
    if ( input.sampling_method == 0 )
    {
      object = new kvs::CellByCellUniformSampling( volume, input.repetitions, input.step, tfunc );  
    }
    else if ( input.sampling_method == 1 )
    {
      object = new kvs::CellByCellMetropolisSampling( volume, input.repetitions,input.step, tfunc ); 
    }
    else if ( input.sampling_method == 2 )
    {
      object = new kvs::CellByCellRejectionSampling( volume, input.repetitions ,input.step, tfunc );
    }
    else if ( input.sampling_method == 3 )
      {
	object = new kvs::CellByCellLayeredSampling( volume, input.repetitions, input.step, tfunc );
      }
    else if ( input.sampling_method == 4 )
      {
	object = new ParticleBasedRendering::CellByCellSubpixelPointSampling(camera, volume, subpixel_level, input.step, input.base_opacity);
      }
    return object;
}

} // end of namespace local
