#include "Program.h"
#include "Input.h"
#include "Process.h"
#include <kvs/PrismaticCell>
#include <KVS.mpi/Lib/Environment.h>
#include <KVS.mpi/Lib/Communicator.h>
#include <InSituVis/Lib/Logger.h>


namespace
{

#include <string>
#include <sstream>
#include <iomanip>
inline std::string ToString( int n, int w, char c = '0' )
{
    std::ostringstream s;
    s << std::setw(w) << std::setfill(c) << n;
    return s.str();
}

void WriteLog(
    std::ostream& os,
    const std::vector<local::Process::Times>& all_times )
{
    // Header.
    os << "Rank,"
       << "Reading time,"
       << "Importing time,"
       << "Mapping time,"
       << "Rendering time,"
       << "Rendering time (creation),"
       << "Rendering time (projection),"
       << "Rendering time (ensemble),"
       << "Readback time,"
       << "Composition time,"
       << std::endl;

    // Body.
    for ( size_t i = 0; i < all_times.size(); i++ )
    {
        const local::Process::Times& times = all_times[i];
        os << i << ","
           << times.reading << ","
           << times.importing << ","
           << times.mapping << ","
           << times.rendering << ","
           << times.rendering_creation << ","
           << times.rendering_projection << ","
           << times.rendering_ensemble << ","
           << times.readback << ","
           << times.composition << "," << std::endl;
    }
}

}

namespace local
{

int Program::exec( int argc, char** argv )
{
    // Initialize MPI
    kvs::mpi::Environment env( argc, argv );
    kvs::mpi::Communicator world( MPI_COMM_WORLD );

    const int my_rank = world.rank();
    const int master_rank = 0;
    const bool is_master = ( my_rank == master_rank );

    // Logger.
    InSituVis::Logger log_stdout;
    InSituVis::Logger log_times( "output_times.log" );
    kvs::Indent indent(4);

    // Input parameters.
    local::Input input( argc, argv );
    if ( !input.parse() ) { return 1; }
    input.print( log_stdout( is_master ) << "INPUT PARAMETERS" << std::endl, indent );

    // Parallel processing.
    log_stdout( is_master ) << "PROCESSING" << std::endl;
    local::Process proc( input, world );

    log_stdout( is_master ) << indent << "Reading ... " << std::flush;
    local::Process::Data data = proc.read();
    log_stdout( is_master ) << "done." << std::endl;

    log_stdout( is_master ) << indent << "Importing ... " << std::flush;
    local::Process::VolumeList volumes = proc.import( data );
    log_stdout( is_master ) << "done." << std::endl;

    log_stdout( is_master ) << indent  << "Rendering ... " << std::flush;
    local::Process::Image image = proc.render( volumes );
    log_stdout( is_master ) << "done." << std::endl;

    // Output final image.
    if ( is_master ) { image.write( "output_image.bmp" ); }

    // Processing times.
    std::vector<local::Process::Times> all_times = proc.times().gather( world, master_rank );
    ::WriteLog( log_times( is_master ), all_times );

    local::Process::Times max_times = proc.times().reduce( world, MPI_MAX, master_rank );
    local::Process::Times min_times = proc.times().reduce( world, MPI_MIN, master_rank );
    proc.times().print( log_stdout( is_master ) << "TIMES (Rank " << my_rank << ")" << std::endl, indent );
    min_times.print( log_stdout( is_master ) << "TIMES (Min)" << std::endl, indent );
    max_times.print( log_stdout( is_master ) << "TIMES (Max)" << std::endl, indent );

    return 0;
}

} // end of namespace local
