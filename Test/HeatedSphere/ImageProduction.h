#pragma once
#include "Input.h"
#include <kvs/VolumeObjectBase>
#include <kvs/FieldViewData>
#include <kvs/Vector3>
#include <kvs/ValueArray>
#include <kvs/ColorImage>
#include <kvs/Timer>
#include <kvs/osmesa/Screen>
#include <KVS.mpi/Lib/Communicator.h>
#include <cfloat>
#include <vector>


class ImageProduction
{
public:

    struct ProcessingTimes
    {
        float reading;
        float importing;
        float mapping;
        float rendering;
        float readback;
        float composition;
    };

    typedef kvs::FieldViewData Data;
    typedef kvs::VolumeObjectBase Volume;
    typedef std::vector<Volume*> VolumeList;
    typedef kvs::ColorImage Image;

private:
    int m_rank;
    int m_nnodes;
    ProcessingTimes m_processing_times;

    kvs::Vec3 m_min_ext;
    kvs::Vec3 m_max_ext;
    kvs::Real32 m_min_value;
    kvs::Real32 m_max_value;

public:
    ImageProduction( const int rank, const int nnodes ):
        m_rank( rank ),
        m_nnodes( nnodes ) {}

    const ProcessingTimes& processingTimes() const { return m_processing_times; }

    Data read( const Input& input );
    VolumeList import( const Input& input, const Data& data );
    Image render( const Input& input, const VolumeList& volumes );

private:
    Volume* import_volume( const Data& data, const int gindex );
    void calculate_min_max( const Data& data );
    void draw_isosurface( kvs::osmesa::Screen& screen, const VolumeList& volumes, Input& input );
    void draw_sliceplane( kvs::osmesa::Screen& screen, const VolumeList& volumes, Input& input );
    void draw_externalfaces( kvs::osmesa::Screen& screen, const VolumeList& volumes, Input& input );
};
