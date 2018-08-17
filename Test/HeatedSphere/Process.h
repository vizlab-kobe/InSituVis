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

    struct FrameBuffer
    {
        kvs::ValueArray<kvs::UInt8> color_buffer;
        kvs::ValueArray<kvs::Real32> depth_buffer;
    };

    typedef kvs::FieldViewData Data;
    typedef kvs::VolumeObjectBase Volume;
    typedef std::vector<Volume*> VolumeList;
    typedef kvs::ColorImage Image;

private:
    const local::Input& m_input;
    kvs::mpi::Communicator& m_communicator;
    ProcessingTimes m_processing_times;

    kvs::Vec3 m_min_ext;
    kvs::Vec3 m_max_ext;
    kvs::Real32 m_min_value;
    kvs::Real32 m_max_value;

public:
    Process( const local::Input& input, kvs::mpi::Communicator& communicator ):
        m_input( input ),
        m_communicator( communicator ) {}

    const ProcessingTimes& processingTimes() const { return m_processing_times; }

    Data read();
    VolumeList import( const Data& data );
    FrameBuffer render( const VolumeList& volumes );
    Image compose( const FrameBuffer& frame_buffer );

private:
    Volume* import_volume( const Data& data, const int gindex );
    void calculate_min_max( const Data& data );
    void mapping_isosurface( InSituVis::Screen& screen, const VolumeList& volumes );
    void mapping_sliceplane( InSituVis::Screen& screen, const VolumeList& volumes );
    void mapping_externalfaces( InSituVis::Screen& screen, const VolumeList& volumes );
};

} // end of namespace local
