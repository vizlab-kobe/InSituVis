#include "Program.h"
#include "Input.h"
#include "ImageProduction.h"
#include <kvs/Timer>
#include <kvs/ShaderSource>
#include <kvs/ColorImage>
#include <KVS.mpi/Lib/Environment.h>
#include <KVS.mpi/Lib/Communicator.h>
#include <kvs/PrismaticCell>
#include <kvs/TetrahedralCell>
#include <fstream>
#include <kvs/File>

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
  kvs::ShaderSource::AddSearchPath("../../Lib");
  kvs::ShaderSource::AddSearchPath("../../../../ParticleBasedRendering/Lib");
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
    world.reduce( 0, ncells, ncells, MPI_MAX);
    world.reduce( 0, v_part, v_part, MPI_MAX);
    if( rank ==0){
      std::cout <<"max cells: "<< ncells << std::endl;
      std::cout <<"sum cells: "<< sum_cells << std::endl;
      std::cout <<"max volume: "<< v_part << std::endl;
      std::cout <<"sum volume: "<< sum_part << std::endl;
    }
    // Ensemble averaging with image composition
    double composition_time = 0;
    double projection_time = 0;
    double readback_time = 0;
    double averaging_time = 0;
    double render_time = 0;
    kvs::ColorImage image = proc.render( input, timer, volumes, composition_time, projection_time, readback_time, averaging_time);
    
    //world.reduce( 0, render_time, render_time, MPI_MAX );
    world.reduce( 0, projection_time, projection_time, MPI_MAX );
    world.reduce( 0, readback_time, readback_time, MPI_MAX );
    world.reduce( 0, averaging_time, averaging_time, MPI_MAX );
    world.reduce( 0, composition_time, composition_time, MPI_MAX );
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
