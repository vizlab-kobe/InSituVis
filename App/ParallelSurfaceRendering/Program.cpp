#include "Program.h"
#include "Input.h"
#include "Process.h"
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
    const local::Input& input,
    const std::vector<local::Process::Times>& all_times,
    const std::vector<local::Process::Stats>& all_stats )
{
    os << "Number of processes," << all_times.size() << "," << std::endl;
    os << "Image width," << input.width << "," << std::endl;
    os << "Image height," << input.height << "," << std::endl;
    os << "Processing times [sec]," << std::endl;

    // Header.
    os << "Rank,"
       << "Reading time,"
       << "Importing time,"
       << "Mapping time,"
       << "Rendering time,"
       << "Readback time,"
       << "Composition time,"
       << "Number of regions,"
       << "Number of cells,"
       << "Number of polygons,"
       << std::endl;

    // Body.
    for ( size_t i = 0; i < all_times.size(); i++ )
    {
        const local::Process::Times& times = all_times[i];
        const local::Process::Stats& stats = all_stats[i];
        os << i << ","
           << times.reading << ","
           << times.importing << ","
           << times.mapping << ","
           << times.rendering << ","
           << times.readback << ","
           << times.composition << ","
           << stats.nregions << ","
           << stats.ncells << ","
           << stats.npolygons << "," << std::endl;
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
    InSituVis::Logger log_cout;
    InSituVis::Logger log_file( "output_log.csv" );
    kvs::Indent indent(4);

    // Input parameters.
    local::Input input( argc, argv );
    if ( !input.parse() ) { return 1; }
    input.print( log_cout( is_master ) << "INPUT PARAMETERS" << std::endl, indent );

    // Parallel processing.
    log_cout( is_master ) << "PROCESSING" << std::endl;
    local::Process proc( input, world );

    log_cout( is_master ) << indent << "Reading ... " << std::flush;
    local::Process::Data data = proc.read();
    log_cout( is_master ) << "done." << std::endl;

    log_cout( is_master ) << indent << "Importing ... " << std::flush;
    local::Process::VolumeList volumes = proc.import( data );
    log_cout( is_master ) << "done." << std::endl;

    log_cout( is_master ) << indent  << "Rendering ... " << std::flush;
    local::Process::FrameBuffer frame = proc.render( volumes );
    log_cout( is_master ) << "done." << std::endl;

    // Output particle image.
    frame.colorImage().write( "output_image_" + ::ToString( my_rank, 3 ) + ".bmp" );

    log_cout( is_master ) << indent << "Composition ... " << std::flush;
    local::Process::Image image = proc.compose( frame );
    log_cout( is_master ) << "done." << std::endl;

    // Output final image.
    if ( is_master ) { image.write( "output_image.bmp" ); }

    // Processing times and stats.
    std::vector<local::Process::Times> all_times = proc.times().gather( world, master_rank );
    std::vector<local::Process::Stats> all_stats = proc.stats().gather( world, master_rank );
    ::WriteLog( log_file( is_master ), input, all_times, all_stats );

    local::Process::Stats sum_stats = proc.stats().reduce( world, MPI_SUM, master_rank );
    local::Process::Times max_times = proc.times().reduce( world, MPI_MAX, master_rank );
    local::Process::Times min_times = proc.times().reduce( world, MPI_MIN, master_rank );
    proc.stats().print( log_cout( is_master ) << "STATS (Rank " << my_rank << ")" << std::endl, indent );
    proc.times().print( log_cout( is_master ) << "TIMES (Rank " << my_rank << ")" << std::endl, indent );
    sum_stats.print( log_cout( is_master ) << "STATS (Total)" << std::endl, indent );
    min_times.print( log_cout( is_master ) << "TIMES (Min)" << std::endl, indent );
    max_times.print( log_cout( is_master ) << "TIMES (Max)" << std::endl, indent );

    return 0;
}

} // end of namespace local
