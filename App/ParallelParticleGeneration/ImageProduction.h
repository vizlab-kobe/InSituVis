#pragma once
#include <KVS.mpi/Lib/Communicator.h>
#include "Input.h"
#include <kvs/VolumeObjectBase>
#include <kvs/FieldViewData>
#include <kvs/Vector3>
#include <kvs/ValueArray>
#include <kvs/ColorImage>
#include <kvs/Timer>
#include <cfloat>
#include <vector>
#include <kvs/PointObject>

class ImageProduction
{
private:
    int m_rank;
    int m_nnodes;
    kvs::Vec3 m_min_ext;
    kvs::Vec3 m_max_ext;
    kvs::Real32 m_min_value;
    kvs::Real32 m_max_value;

public:
    ImageProduction( const int rank, const int nnodes ):
        m_rank( rank ),
        m_nnodes( nnodes ) {}

    kvs::FieldViewData read( const Input& input, kvs::Timer& timer );
    std::vector<kvs::VolumeObjectBase*> import( const Input& input, kvs::Timer& timer, const kvs::FieldViewData& data );
    void render( const Input& input, kvs::Timer& timer, const std::vector<kvs::VolumeObjectBase*>& volumes, kvs::mpi::Communicator world, double& particle_time, double& transfer_time, double& projection_time,double& createbuffer_time, double& readback_time, double& averaging_time, double& num_particle, double& part_particle);
    

private:
    kvs::VolumeObjectBase* import_volume( const kvs::FieldViewData& data, const int gindex );
    void calculate_min_max( const kvs::FieldViewData& data );
    kvs::PointObject* make_point( const std::vector<kvs::VolumeObjectBase*>& volumes, const Input& input );
};
