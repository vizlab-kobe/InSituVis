#include "Program.h"
#include "Input.h"
#include "Process.h"
#include <kvs/PrismaticCell>
#include <KVS.mpi/Lib/Environment.h>
#include <KVS.mpi/Lib/Communicator.h>
#include <InSituVis/Lib/Logger.h>


namespace local
{

int Program::exec( int argc, char** argv )
{
    // Initialize MPI
    kvs::mpi::Environment env( argc, argv );
    kvs::mpi::Communicator world( MPI_COMM_WORLD );

    const int my_rank = world.rank();
    const int master_rank = 0;

    // Logger.
    InSituVis::Logger stdout;
    InSituVis::Logger logger;
    kvs::Indent indent(4);

    // Input parameters.
    local::Input input( argc, argv );
    if ( !input.parse() ) { return 1; }
    if ( my_rank == master_rank )
    {
        input.print( stdout() << "INPUT PARAMETERS" << std::endl, indent );
    }

    // Parallel processing.
    local::Process proc( input, world );
    local::Process::Data data = proc.read();
    local::Process::VolumeList volumes = proc.import( data );
    local::Process::FrameBuffer frame = proc.render( volumes );
    if ( my_rank == master_rank )
    {
        frame.colorImage().write( "output.bmp" );
    }

    // Processing times.
    std::vector<local::Process::Times> all_times = proc.times().gather( world, master_rank );
    local::Process::Times max_times = proc.times().reduce( world, MPI_MAX, master_rank );
    local::Process::Times min_times = proc.times().reduce( world, MPI_MIN, master_rank );
    if ( my_rank == master_rank )
    {
        proc.times().print( stdout() << "PROCESSING TIMES (Rank " << my_rank << ")" << std::endl, indent );
        min_times.print( stdout() << "PROCESSING TIMES (Min)" << std::endl, indent );
        max_times.print( stdout() << "PROCESSING TIMES (Max)" << std::endl, indent );
    }

    return 0;
}

} // end of namespace local
