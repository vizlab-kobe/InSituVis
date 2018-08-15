#include "Program.h"
#include "Input.h"
#include "ImageProduction.h"
#include <kvs/Timer>
#include <kvs/ColorImage>
#include <kvs/PrismaticCell>
#include <kvs/TetrahedralCell>
#include <kvs/File>
#include <KVS.mpi/Lib/Environment.h>
#include <KVS.mpi/Lib/Communicator.h>

#include <fstream>

std::stringstream log_label;
std::stringstream log_data;
void log(){
    log_label << "width,";
    log_label << "height,";
    log_label << "max part volume,";
    log_label << "sum volume,";
    log_label << "max cells,";
    log_label << "sum cells,";
    log_label << "parallel number,";
    log_label << "data read time,";
    log_label << "import time,";
    log_label << "composition time,";
    log_label << "projection time,";
    log_label << "readback time,";
    log_label << "averaging time,";
    log_label << "render time,";
    kvs::File file( "log.csv" );
    if( file.exists() )
    {
        std::ofstream ofs( "log.csv", std::ios::app);
        ofs << log_data.str() << std::endl;
    }
    else 
    {
        std::ofstream ofs( "log.csv", std::ios::out);
        ofs << log_label.str() << std::endl;
        ofs << log_data.str() << std::endl;
    }
}

int Program::exec( int argc, char** argv )
{
   kvs::Timer timer;
    kvs::Indent indent(4);

    Input input( argc, argv );
    if ( !input.parse() ) { return 1; }

    // Initialize MPI
    kvs::mpi::Environment env( argc, argv );
    kvs::mpi::Communicator world( MPI_COMM_WORLD );
    const int rank = world.rank();
    const int nnodes = world.size();
    const int master_rank = 0;
    if ( rank == master_rank )
    {
        input.print( std::cout << "INPUT PARAMETERS" << std::endl, indent );
        std::cout << std::endl;
    }

    ImageProduction proc( rank, nnodes );

    // Read data
    timer.start();
    kvs::FieldViewData data = proc.read( input );
    timer.stop();

    float read_time = timer.sec();
    float recv_read_time = 0;
//    world.reduce( 0, read_time, read_time, MPI_MAX );
    world.reduce( 0, read_time, recv_read_time, MPI_MAX );
    read_time = recv_read_time;
    if ( rank == 0 )
    {
        std::cout << "Read time : " << read_time << " [sec]" << std::endl;
    }

    // Import object
    timer.start();
    std::vector<kvs::VolumeObjectBase*> volumes = proc.import( input, data );
    timer.stop();

    float import_time = timer.sec();
    float recv_import_time = 0;
//    world.reduce( 0, import_time, import_time, MPI_MAX );
    world.reduce( 0, import_time, recv_import_time, MPI_MAX );
    import_time = recv_import_time;
    if ( rank == 0 )
    {
        std::cout << "Import time : " << import_time << " [sec]" << std::endl;
    }

    int ncells =0;
    kvs::Real32 v_part=0;
    for(size_t i = 0; i < volumes.size(); i++)
    {
        if(volumes[i]){
            ncells += volumes[i]->numberOfCells();
            kvs::TetrahedralCell cell((kvs::UnstructuredVolumeObject*)volumes[i]);
            for ( size_t t = 0; t < volumes[i]->numberOfCells(); t++ )
            {
                cell.bindCell( t );
                v_part += cell.volume();
            }
        }
    }
    int sum_cells =0;
    kvs::Real32 sum_part =0;
    world.reduce( 0, ncells, sum_cells, MPI_SUM);
    world.reduce( 0, v_part, sum_part, MPI_SUM);

    int recv_ncells = 0;
    world.reduce( 0, ncells, recv_ncells, MPI_MAX);
    ncells = recv_ncells;

    kvs::Real32 recv_v_part = 0;
    world.reduce( 0, v_part, recv_v_part, MPI_MAX);
    v_part = recv_v_part;

    if( rank ==0){
        std::cout <<"max cells: "<< ncells << std::endl;
        std::cout <<"sum cells: "<< sum_cells << std::endl;
        std::cout <<"max volume: "<< v_part << std::endl;
        std::cout <<"sum volume: "<< sum_part << std::endl;
    }

    // Ensemble averaging with image composition
//    double composition_time = 0;
//    double projection_time = 0;
//    double readback_time = 0;
//    double averaging_time = 0;
//    double render_time = 0;
//    kvs::ColorImage image = proc.render( input, timer, volumes, composition_time, projection_time, readback_time, averaging_time);
//    double send_composition_time = 0;
//    double send_projection_time = 0;
//    double send_readback_time = 0;
//    double send_averaging_time = 0;
//    double send_render_time = 0;
//    kvs::ColorImage image = proc.render( input, volumes, send_composition_time, send_projection_time, send_readback_time, send_averaging_time);
    kvs::ColorImage image = proc.render( input, volumes );
    double send_composition_time = proc.processingTimes().composition;
    double send_projection_time = proc.processingTimes().rendering;
    double send_readback_time = proc.processingTimes().readback;
    double send_averaging_time = 0;
    double send_render_time = 0;

    double composition_time = 0;
    double projection_time = 0;
    double readback_time = 0;
    double averaging_time = 0;
    double render_time = 0;
    world.reduce( 0, send_render_time, render_time, MPI_MAX );
    world.reduce( 0, send_projection_time, projection_time, MPI_MAX );
    world.reduce( 0, send_readback_time, readback_time, MPI_MAX );
    world.reduce( 0, send_averaging_time, averaging_time, MPI_MAX );
    world.reduce( 0, send_composition_time, composition_time, MPI_MAX );
    render_time = projection_time + readback_time + averaging_time;

    if ( rank == 0 )
    {
        std::cout << "Render time : " << render_time << " [sec]" << std::endl;
        std::cout << "Composition time : " << composition_time << " [sec]" << std::endl;
        image.write( "output.bmp" );
        log_data << input.width << ",";
        log_data << input.height << ",";
        log_data << v_part << ",";
        log_data << sum_part << ",";
        log_data << ncells << ",";
        log_data << sum_cells << ",";
        log_data << nnodes << ",";
        log_data << read_time << ",";
        log_data << import_time << ",";
        log_data << composition_time << ",";
        log_data << projection_time << ",";
        log_data << readback_time << ",";
        log_data << averaging_time << ",";
        log_data << render_time << ",";
        log();
    }
    return 0;
}
