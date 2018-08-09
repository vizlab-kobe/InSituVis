#pragma once
#include <kvs/PointObject>
#include <kvs/VolumeObjectBase>
#include <kvs/TransferFunction>
#include "Input.h"
#include <kvs/Camera>

namespace local
{

  kvs::PointObject* ParticleGenerator( const Input& input, const kvs::VolumeObjectBase* volume, kvs::TransferFunction& tfunc , kvs::Camera* camera, double& generation_time);

} // end of namespace local
