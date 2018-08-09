#pragma once
#include <kvs/PointObject>
#include <kvs/VolumeObjectBase>
#include <kvs/TransferFunction>
#include "Input.h"


namespace local
{

kvs::PointObject* ParticleGenerator( const Input& input, const kvs::VolumeObjectBase* volume, kvs::TransferFunction& tfunc );

} // end of namespace local
