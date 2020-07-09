#pragma once
#include <mpi.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <kvs/Timer>


namespace local
{

class StampTimer
{
private:
    kvs::Timer m_timer; ///< timer
    std::vector<float> m_times; ///< times in sec.

public:
    StampTimer()
    {
    }

    void start()
    {
        m_timer.start();
    }

    void stamp()
    {
        m_timer.stop();
        m_times.push_back( m_timer.sec() );
    }

    float last() const
    {
        return m_times.back();
    }

    void allreduce( const MPI_Op op, const MPI_Comm handler = MPI_COMM_WORLD )
    {
        std::vector<float> temp( m_times.size() );
        MPI_Allreduce( &m_times[0], &temp[0], m_times.size(), MPI_FLOAT, op, handler );
        temp.swap( m_times );
    }

    void write( const std::string& filename, std::ios_base::openmode mode = std::ios_base::app )
    {
        std::ofstream ofs( filename.c_str(), std::ios::out | mode );
        for ( auto& t : m_times ) { ofs << t << std::endl; }
    }
};

} // end of namespace local
