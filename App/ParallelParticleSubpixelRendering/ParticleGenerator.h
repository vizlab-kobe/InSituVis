#if 0
#pragma once
#include <kvs/PointObject>
#include <kvs/VolumeObjectBase>
#include <kvs/TransferFunction>
#include "Input.h"
#include <kvs/Camera>

namespace local
{

  kvs::PointObject* ParticleGenerator( const Input& input, const kvs::VolumeObjectBase* volume, kvs::TransferFunction& tfunc , kvs::Camera* camera);

} // end of namespace local
#endif
