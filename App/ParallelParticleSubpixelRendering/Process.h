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
#include <kvs/PointObject>
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
        float mapping; ///< mapping time (particle generation time)
        float rendering; ///< rendering time (total time)
        float rendering_projection; ///< rendering time (particle projection time)
        float rendering_subpixel; ///< rendering time (subpixel processing time)
        float readback; ///< readback time
        float composition; ///< image composition time

        ProcessingTimes reduce( kvs::mpi::Communicator& comm, const MPI_Op op, const int rank = 0 ) const;
        std::vector<ProcessingTimes> gather( kvs::mpi::Communicator& comm, const int rank = 0 ) const;
        void print( std::ostream& os, const kvs::Indent& indent ) const;
    };

    typedef kvs::FieldViewData Data;
    typedef kvs::VolumeObjectBase Volume;
    typedef kvs::PointObject Particle;
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
    Image render( const VolumeList& volumes );

private:
    Volume* import_volume( const Data& data, const int gindex );
    void calculate_min_max( const Data& data );
    void mapping( InSituVis::Screen& screen, const VolumeList& volumes, const kvs::TransferFunction& tfunc );
    Particle* generate_particle( const VolumeList& volumes, const kvs::TransferFunction& tfunc );
    Particle* generate_particle( const Volume* volume, const kvs::TransferFunction& tfunc );
};

} // end of namespace local