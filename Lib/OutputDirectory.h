/*****************************************************************************/
/**
 *  @file   OutputDirectory.h
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <sys/stat.h>
#include <string>
#include <kvs/Directory>
#include <kvs/String>
#if defined( KVS_USE_MPI )
#include <kvs/mpi/Communicator>
#endif // KVS_USE_MPI


namespace InSituVis
{

/*===========================================================================*/
/**
 *  @brief  Output directory class.
 */
/*===========================================================================*/
class OutputDirectory
{
private:
    std::string m_base_dirname = "/data2/tomoya/CUBE/Output"; ///< base directory name (e.g. "Output")
    std::string m_sub_dirname = "Process"; ///< sub directory name (e.g. "Process")
    std::string m_dirname = ""; ///< output directory name (e.g. "Output/Process0000")

public:
    OutputDirectory(
        const std::string& base_dirname = "/data2/tomoya/CUBE/Output",
        const std::string& sub_dirname = "Process" ):
        m_base_dirname( base_dirname ),
        m_sub_dirname( sub_dirname )
    {
    }

    const std::string& baseDirectoryName() const { return m_base_dirname; }
    const std::string& subDirectoryName() const { return m_sub_dirname; }
    const std::string& name() const { return m_dirname; }

    void setBaseDirectoryName( const std::string& dirname )
    {
        m_base_dirname = dirname;
    }

    void setSubDirectoryName( const std::string& dirname )
    {
        m_sub_dirname = dirname;
    }

    bool create()
    {
        if ( !kvs::Directory::Exists( m_base_dirname ) )
        {
            if ( mkdir( m_base_dirname.c_str(), 0777 ) != 0 )
            {
                kvsMessageError() << "Cannot create " << m_base_dirname << "." << std::endl;
                return false;
            }
        }

        m_dirname = m_base_dirname;
        return true;
    }

#if defined( KVS_USE_MPI )
    bool create( kvs::mpi::Communicator& world )
    {
        const int root = 0;
        const int rank = world.rank();
        const std::string sp( "/" );

        // Create base directory.
        if ( rank == root )
        {
            if ( !this->create() ) { return false; }
        }

        world.barrier();

        // Create sub-directories.
        const std::string rank_name = kvs::String::From( rank, 4, '0' );
        const std::string output_dirname = m_base_dirname + sp + m_sub_dirname + rank_name + sp;
        if ( !kvs::Directory::Exists( output_dirname ) )
        {
            if ( mkdir( output_dirname.c_str(), 0777 ) != 0 )
            {
                kvsMessageError() << "Cannot create " << output_dirname << "." << std::endl;
                return false;
            }
        }

        m_dirname = output_dirname;
        return true;
    }
#endif
};

} // end of namespace InSituVis
