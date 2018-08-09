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
#include <kvs/osmesa/Screen>

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
    kvs::ColorImage render( const Input& input, kvs::Timer& timer, const std::vector<kvs::VolumeObjectBase*>& volumes, double& composition_time, double& projcetion_time, double& readback_time, double& averaging_time );

private:
    kvs::VolumeObjectBase* import_volume( const kvs::FieldViewData& data, const int gindex );
    void calculate_min_max( const kvs::FieldViewData& data );
    void draw_isosurface(kvs::osmesa::Screen& screen, const std::vector<kvs::VolumeObjectBase*>& volumes, Input& input, double& projection_time );
    void draw_sliceplane(kvs::osmesa::Screen& screen, const std::vector<kvs::VolumeObjectBase*>& volumes, Input& input, double& projection_time );
    void draw_externalfaces(kvs::osmesa::Screen& screen, const std::vector<kvs::VolumeObjectBase*>& volumes, Input& input, double& projection_time );
};
