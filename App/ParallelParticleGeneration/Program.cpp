#include "Program.h"
#include "Input.h"
#include "ImageProduction.h"
#include <kvs/Timer>
#include <kvs/ShaderSource>
#include <kvs/ColorImage>
#include <KVS.mpi/Lib/Environment.h>
#include <KVS.mpi/Lib/Communicator.h>
#include <kvs/PrismaticCell>
#include <fstream>
#include <kvs/File>
std::stringstream log_label;
std::stringstream log_data;
void log(){
  log_label << "width,";
  log_label << "height,";
  log_label << "repeat,";
  log_label << "base_opacity,";
  log_label << "max part volume,";
  log_label << "sum volume,";
  log_label << "max cells,";
  log_label << "sum cells,";
  log_label << "parallel number,";
  log_label << "part particles,";
  log_label << "number of particles,";
  log_label << "data read time,";
  log_label << "import time,";
  log_label << "particle time,";
  log_label << "transfer time,";
  log_label << "projection time,";
  log_label << "createbuffer time,";
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
  kvs::ShaderSource::AddSearchPath("../../Lib");
  kvs::Timer timer;
    kvs::Indent indent(4);

    Input input( argc, argv );
    if ( !input.parse() ) { return 1; }

    // Initialize MPI
    kvs::mpi::Environment env( argc, argv );
    kvs::mpi::Communicator world( MPI_COMM_WORLD );
    const int rank = world.rank();
    const int nnodes = world.size();

    if ( rank == 0 )
    {
        input.print( std::cout << "INPUT PARAMETERS" << std::endl, indent );
        std::cout << std::endl;
    }

    ImageProduction proc( rank, nnodes );

    // Read data
    kvs::FieldViewData data = proc.read( input, timer );

    float read_time = timer.sec();
    world.reduce( 0, read_time, read_time, MPI_MAX );
    if ( rank == 0 )
    {
      std::cout << "Read time : " << read_time << " [sec]" << std::endl;
    }

    // Import object
    std::vector<kvs::VolumeObjectBase*> volumes = proc.import( input, timer, data );

    float import_time = timer.sec();
    world.reduce( 0, import_time, import_time, MPI_MAX );
    if ( rank == 0 )
    {
        std::cout << "Import time : " << import_time << " [sec]" << std::endl;
    }

    int ncells =0;
    kvs::Real32 v_part=0;
    for( size_t i=0; i< volumes.size(); i++){
      if(volumes[i]){
        ncells += volumes[i]->numberOfCells();
	kvs::PrismaticCell cell((kvs::UnstructuredVolumeObject*)volumes[i]);
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
    world.reduce( 0, ncells, ncells, MPI_MAX);
    world.reduce( 0, v_part, v_part, MPI_MAX);
    // Ensemble averaging with image composition

    double particle_time = 0;
    double transfer_time = 0;
    double render_time = 0;
    double num_particle = 0;
    double part_particle = 0;
    double projection_time = 0;
    double createbuffer_time =0;
    double readback_time = 0;
    double averaging_time = 0;

    proc.render( input, timer, volumes, world, particle_time, transfer_time, projection_time, createbuffer_time, readback_time, averaging_time, num_particle, part_particle);
    world.reduce( 0, particle_time, particle_time, MPI_MAX );
    world.reduce( 0, part_particle, part_particle, MPI_MAX );
    render_time = projection_time + readback_time+ averaging_time;
    if ( rank == 0 )
    {     
      log_data << input.width << ",";
      log_data << input.height << ",";
      log_data << input.repetitions << ",";
      log_data << input.base_opacity << ",";
      log_data << v_part << ",";
      log_data << sum_part << ",";
      log_data << ncells << ",";
      log_data << sum_cells << ",";
      log_data << nnodes << ",";
      log_data << part_particle << ",";
      log_data << num_particle << ",";
      log_data << read_time << ",";
      log_data << import_time << ",";
      log_data << particle_time << ",";
      log_data << transfer_time << ",";
      log_data << projection_time <<",";
      log_data << createbuffer_time <<",";
      log_data << readback_time <<",";
      log_data << averaging_time <<",";
      log_data << render_time << ",";
      log();
    }

    return 0;
}
