#pragma once
#include "Input.h"
#include <kvs/VolumeObjectBase>
#include <kvs/FieldViewData>
#include <kvs/Vector3>
#include <kvs/ValueArray>
#include <kvs/ColorImage>
#include <kvs/Timer>
#include <kvs/osmesa/Screen>
#include <cfloat>
#include <vector>
#include <KVS.mpi/Lib/Communicator.h>
#include <InSituVis/Lib/Screen.h>


namespace local
{

class Process
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
    Process( const int rank, const int nnodes ):
        m_rank( rank ),
        m_nnodes( nnodes ) {}

    const ProcessingTimes& processingTimes() const { return m_processing_times; }

    Data read( const local::Input& input );
    VolumeList import( const local::Input& input, const Data& data );
    Image render( const local::Input& input, const VolumeList& volumes );

private:
    Volume* import_volume( const Data& data, const int gindex );
    void calculate_min_max( const Data& data );
    void draw_isosurface( InSituVis::Screen& screen, const VolumeList& volumes, const local::Input& input );
    void draw_sliceplane( InSituVis::Screen& screen, const VolumeList& volumes, const local::Input& input );
    void draw_externalfaces( InSituVis::Screen& screen, const VolumeList& volumes, const local::Input& input );
};

} // end of namespace local
