#pragma once
#include <sys/stat.h>
#include <string>
#include <kvs/Directory>
#include <kvs/String>
#include <kvs/mpi/Communicator>


namespace Util
{

class OutputDirectory
{
private:
    std::string m_base_dirname; ///< base directory name (e.g. "Output")
    std::string m_sub_dirname; ///< sub directory name (e.g. "Proc_")
    std::string m_dirname; ///< output directory name (e.g. "Output/Proc_0000")

public:
    OutputDirectory():
        m_base_dirname(""),
        m_sub_dirname("")
    {
    }

    OutputDirectory(
        const std::string& base_dirname,
        const std::string& sub_dirname ):
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

    bool create( kvs::mpi::Communicator& world )
    {
        const int root = 0;
        const int rank = world.rank();
        const std::string sp( "/" );

        if ( rank == root )
        {
            if ( !kvs::Directory::Exists( m_base_dirname ) )
            {
                if ( mkdir( m_base_dirname.c_str(), 0777 ) != 0 )
                {
                    kvsMessageError() << "Cannot create " << m_base_dirname << "." << std::endl;
                    return false;
                }
            }
        }

        world.barrier();

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
};

} // end of namespace Util
