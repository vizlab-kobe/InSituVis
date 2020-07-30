#pragma once
#include <sys/stat.h>
#include <string>
#include <kvs/Directory>
#include <kvs/String>
#include <kvs/mpi/Communicator>


namespace local
{

inline std::string CreateOutputDirectory(
    kvs::mpi::Communicator& comm,
    const std::string base_dirname ,
    const std::string sub_dirname )
{
    const int root = 0;
    const int rank = comm.rank();
    const std::string sp( "/" );

    if ( rank == root )
    {
        if ( !kvs::Directory::Exists( base_dirname ) )
        {
            if ( mkdir( base_dirname.c_str(), 0777 ) != 0 )
            {
                return "";
            }
        }
    }

    comm.barrier();

    const std::string rank_name = kvs::String::ToString( rank );
    const std::string output_dirname = base_dirname + sp + sub_dirname + rank_name + sp;
    if ( !kvs::Directory::Exists( output_dirname ) )
    {
        if ( mkdir( output_dirname.c_str(), 0777 ) != 0 )
        {
            return "";
        }
    }

    return output_dirname;
}

} // end of namespace local
