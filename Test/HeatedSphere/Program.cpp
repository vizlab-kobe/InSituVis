#include "Program.h"
#include "Input.h"
#include "Process.h"
#include <KVS.mpi/Lib/Environment.h>
#include <KVS.mpi/Lib/Communicator.h>
#include <InSituVis/Lib/Logger.h>


int Program::exec( int argc, char** argv )
{
    InSituVis::Logger logger;
    kvs::Indent indent(4);

    local::Input input( argc, argv );
    if ( !input.parse() ) { return 1; }

    // Initialize MPI
    kvs::mpi::Environment env( argc, argv );
    kvs::mpi::Communicator world( MPI_COMM_WORLD );

    const int my_rank = world.rank();
    const int master_rank = 0;
    if ( my_rank == master_rank )
    {
        input.print( logger() << "INPUT PARAMETERS" << std::endl, indent );
    }

    // Parallel processing
    local::Process proc( input, world );
    local::Process::Data data = proc.read();
    local::Process::VolumeList volumes = proc.import( data );
    local::Process::FrameBuffer frame = proc.render( volumes );
    local::Process::Image image = proc.compose( frame );
    if ( my_rank == master_rank )
    {
        image.write( "output.bmp" );
    }

    const float read_time = proc.processingTimes().reading;
    const float import_time = proc.processingTimes().importing;
    const float map_time = proc.processingTimes().mapping;
    const float render_time = proc.processingTimes().rendering;
    const float readback_time = proc.processingTimes().readback;
    const float composition_time = proc.processingTimes().composition;
    float max_read_time = 0;
    float max_import_time = 0;
    float max_map_time = 0;
    float max_render_time = 0;
    float max_readback_time = 0;
    float max_composition_time = 0;
    world.reduce( 0, read_time, max_read_time, MPI_MAX );
    world.reduce( 0, import_time, max_import_time, MPI_MAX );
    world.reduce( 0, map_time, max_map_time, MPI_MAX );
    world.reduce( 0, render_time, max_render_time, MPI_MAX );
    world.reduce( 0, readback_time, max_readback_time, MPI_MAX );
    world.reduce( 0, composition_time, max_composition_time, MPI_MAX );
    if ( my_rank == master_rank )
    {
        logger() << "PROCESSING TIMES" << std::endl;
        logger() << indent << "Max. read time: " << max_read_time << " [sec]" << std::endl;
        logger() << indent << "Max. import time: " << max_import_time << " [sec]" << std::endl;
        logger() << indent << "Max. map time: " << max_map_time << " [sec]" << std::endl;
        logger() << indent << "Max. render time: " << max_render_time << " [sec]" << std::endl;
        logger() << indent << "Max. readback time: " << max_readback_time << " [sec]" << std::endl;
        logger() << indent << "Max. composition time: " << max_composition_time << " [sec]" << std::endl;
    }

    return 0;
}
