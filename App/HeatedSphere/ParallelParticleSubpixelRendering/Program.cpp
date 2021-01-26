#include "Program.h"
#include "Input.h"
#include "Process.h"
#include <kvs/PrismaticCell>
#include <KVS.mpi/Lib/Environment.h>
#include <KVS.mpi/Lib/Communicator.h>
#include <kvs/LogStream>


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

std::string BaseName(
    const std::string& prefix,
    const local::Input& input,
    const kvs::mpi::Communicator& comm )
{
    const std::string nprocs = ToString( comm.size(), 4 );
    const std::string width = ToString( input.width, 4 );
    const std::string height = ToString( input.height, 4 );
    const std::string subpixels = ToString( input.subpixels, 4 );
    return prefix + "_" + nprocs + "_" + width + "x" + height + "_" + subpixels;
}

void WriteLog(
    std::ostream& os,
    kvs::mpi::Communicator& comm,
    const int rank,
    const local::Input& input,
    const local::Process& proc )
{
    std::vector<local::Process::Times> all_times = proc.times().gather( comm, rank );
    std::vector<local::Process::Stats> all_stats = proc.stats().gather( comm, rank );
    local::Process::Stats sum_stats = proc.stats().reduce( comm, MPI_SUM, rank );
    local::Process::Stats min_stats = proc.stats().reduce( comm, MPI_MIN, rank );
    local::Process::Stats max_stats = proc.stats().reduce( comm, MPI_MAX, rank );
    local::Process::Times min_times = proc.times().reduce( comm, MPI_MIN, rank );
    local::Process::Times max_times = proc.times().reduce( comm, MPI_MAX, rank );

    os << "Number of processes," << all_times.size() << "," << std::endl;
    os << "Image width," << input.width << "," << std::endl;
    os << "Image height," << input.height << "," << std::endl;
    os << "Subpixels," << input.subpixels << "," << std::endl;
    os << "Total number of regions," << sum_stats.nregions << "," << std::endl;
    os << "Total number of cells," << sum_stats.ncells << "," << std::endl;
    os << "Total number of particles," << sum_stats.nparticles << "," << std::endl;

    // Header.
    os << "Processing times [sec]," << std::endl;
    os << "Rank,"
       << "Reading time,"
       << "Importing time,"
       << "Mapping time,"
       << "Rendering time,"
       << "Rendering time (projection),"
       << "Rendering time (subpixel),"
       << "Readback time,"
       << "Composition time,"
       << "Number of regions,"
       << "Number of cells,"
       << "Number of particles,"
       << std::endl;

    // Min times.
    os << "min" << ","
       << min_times.reading << ","
       << min_times.importing << ","
       << min_times.mapping << ","
       << min_times.rendering << ","
       << min_times.rendering_projection << ","
       << min_times.rendering_subpixel << ","
       << min_times.readback << ","
       << min_times.composition << ","
       << min_stats.nregions << ","
       << min_stats.ncells << ","
       << min_stats.nparticles << ","
       << std::endl;

    // Max times.
    os << "max" << ","
       << max_times.reading << ","
       << max_times.importing << ","
       << max_times.mapping << ","
       << max_times.rendering << ","
       << max_times.rendering_projection << ","
       << max_times.rendering_subpixel << ","
       << max_times.readback << ","
       << max_times.composition << ","
       << max_stats.nregions << ","
       << max_stats.ncells << ","
       << max_stats.nparticles << ","
       << std::endl;

    // Processing times for each rank.
    for ( size_t i = 0; i < all_times.size(); i++ )
    {
        const local::Process::Times& times = all_times[i];
        const local::Process::Stats& stats = all_stats[i];
        os << i << ","
           << times.reading << ","
           << times.importing << ","
           << times.mapping << ","
           << times.rendering << ","
           << times.rendering_projection << ","
           << times.rendering_subpixel << ","
           << times.readback << ","
           << times.composition << ","
           << stats.nregions << ","
           << stats.ncells << ","
           << stats.nparticles << ","
           << std::endl;
    }
}

}

namespace local
{

int Program::exec( int argc, char** argv )
{
    // Input parameters.
    local::Input input( argc, argv );
    if ( !input.parse() ) { return 1; }

    // Initialize MPI
    kvs::mpi::Environment env( argc, argv );
    kvs::mpi::Communicator world( MPI_COMM_WORLD );

    const int my_rank = world.rank();
    const int master_rank = 0;
    const bool is_master = ( my_rank == master_rank );

    // Logger.
    const kvs::Indent indent(4);
    const std::string basename = ::BaseName( "result", input, world );
    kvs::LogStream log_file( basename + "_log.csv" );
    kvs::LogStream log_cout;

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
    local::Process::Image image = proc.render( volumes );
    log_cout( is_master ) << "done." << std::endl;

    // Output final image.
    if ( is_master ) { image.write( basename + "_image.bmp" ); }

    // Processing times.
    proc.times().print( log_cout( is_master ) << "RESULT (Rank " << my_rank << ")" << std::endl, indent );
    proc.stats().print( log_cout( is_master ), indent );
    ::WriteLog( log_file( is_master ), world, master_rank, input, proc );

    return 0;
}

} // end of namespace local
