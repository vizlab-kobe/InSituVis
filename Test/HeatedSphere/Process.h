#pragma once
#include "Input.h"
#include <vector>
#include <kvs/Indent>
#include <kvs/VolumeObjectBase>
#include <kvs/FieldViewData>
#include <kvs/Vector3>
#include <kvs/ValueArray>
#include <kvs/ColorImage>
#include <kvs/Timer>
#include <kvs/TransferFunction>
#include <KVS.mpi/Lib/Communicator.h>
#include <InSituVis/Lib/Screen.h>


namespace local
{

class Process
{
public:
    struct ProcessingTimes
    {
        float reading; ///< rendering time
        float importing; ///< importing time
        float mapping; ///< mappint time
        float rendering; ///< rendering time
        float readback; ///< readback time
        float composition; ///< image composition time

        ProcessingTimes reduce( kvs::mpi::Communicator& comm, const MPI_Op op, const int rank = 0 ) const;
        std::vector<ProcessingTimes> gather( kvs::mpi::Communicator& comm, const int rank = 0 ) const;
        void print( std::ostream& os, const kvs::Indent& indent ) const;
    };

    struct FrameBuffer
    {
        int width; ///< buffer width
        int height; ///< buffer height
        kvs::ValueArray<kvs::UInt8> color_buffer; ///< color frame buffer
        kvs::ValueArray<kvs::Real32> depth_buffer; ///< depth frame buffer
    };

    typedef kvs::FieldViewData Data;
    typedef kvs::VolumeObjectBase Volume;
    typedef std::vector<Volume*> VolumeList;
    typedef kvs::ColorImage Image;

private:
    const local::Input& m_input; ///< input parameters
    kvs::mpi::Communicator& m_communicator; ///< MPI communicator
    ProcessingTimes m_processing_times; ////< processing times
    kvs::Vec3 m_min_ext; ///< min. external coords
    kvs::Vec3 m_max_ext; ///< max. external coords
    kvs::Real32 m_min_value; ///< min. value
    kvs::Real32 m_max_value; ///< max. value

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
    void mapping_isosurface( InSituVis::Screen& screen, const VolumeList& volumes, const kvs::TransferFunction& tfunc );
    void mapping_sliceplane( InSituVis::Screen& screen, const VolumeList& volumes, const kvs::TransferFunction& tfunc );
    void mapping_externalfaces( InSituVis::Screen& screen, const VolumeList& volumes, const kvs::TransferFunction& tfunc );
};

} // end of namespace local
