#pragma once
#include <kvs/StampTimer>
namespace InSituVis { using StampTimer = kvs::StampTimer; }

#if defined( KVS_SUPPORT_MPI )
#include <kvs/mpi/StampTimer>
namespace InSituVis { namespace mpi { using StampTimer = kvs::mpi::StampTimer; } }
#endif



#if 0
#include <kvs/Timer>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#if defined( KVS_SUPPORT_MPI )
#include <kvs/mpi/Communicator>
#endif


namespace InSituVis
{

class StampTimer
{
public:
    using Times = std::vector<float>;
    enum Unit { Sec, MSec, USec, Fps };

private:
    std::string m_title = "";
    Unit m_unit = Sec;
    kvs::Timer m_timer{};
    Times m_times{};

public:
    StampTimer() = default;
    StampTimer( const StampTimer& ) = default;
    StampTimer( const std::string& title ): m_title( title ) {}

    void start()
    {
        m_timer.start();
    }

    void stamp()
    {
        m_timer.stop();
        this->stamp( this->time() );
    }

    void stamp( const kvs::Timer& timer )
    {
        this->stamp( this->time( timer ) );
    }

    void stamp( const float time )
    {
        m_times.push_back( time );
    }

    void setTitle( const std::string& title )
    {
        m_title = title;
    }

    void setUnit( const Unit unit )
    {
        m_unit = unit;
    }

    const std::string& title() const
    {
        return m_title;
    }

    Unit unit() const
    {
        return m_unit;
    }

    const Times& times() const
    {
        return m_times;
    }

    float time() const
    {
        return this->time( m_timer );
    }

    float time( const kvs::Timer& timer ) const
    {
        switch ( m_unit )
        {
        case Unit::Sec: return static_cast<float>( timer.sec() );
        case Unit::MSec: return static_cast<float>( timer.msec() );
        case Unit::USec: return static_cast<float>( timer.usec() );
        case Unit::Fps: return static_cast<float>( timer.fps() );
        default: break;
        }
        return 0.0f;
    }

    size_t numberOfStamps() const
    {
        return m_times.size();
    }

    float last() const
    {
        return m_times.back();
    }

    void print( std::ostream& os, const kvs::Indent& indent = kvs::Indent(0) ) const
    {
        if ( !m_title.empty() ) { os << indent << m_title << std::endl; }
        for ( const auto t : m_times ) { os << indent << t << std::endl; }
    }

    bool write( const std::string& filename, std::ios_base::openmode mode = std::ios_base::app ) const
    {
        std::ofstream ofs( filename.c_str(), std::ios::out | mode );
        if ( !ofs ) { return false; }
        this->print( ofs );
        return true;
    }

protected:
    Times& times() { return m_times; }
};

#if defined( KVS_SUPPORT_MPI )
namespace mpi
{

class StampTimer : public InSituVis::StampTimer
{
public:
    using BaseClass = InSituVis::StampTimer;

private:
    kvs::mpi::Communicator m_comm;

public:
    StampTimer( kvs::mpi::Communicator& comm ): m_comm( comm ) {}
    StampTimer( kvs::mpi::Communicator& comm, const InSituVis::StampTimer& timer ):
        InSituVis::StampTimer( timer ),
        m_comm( comm ) {}
    StampTimer( kvs::mpi::Communicator& comm, const std::string& title ):
        InSituVis::StampTimer( title ),
        m_comm( comm ) {}

    void reduceMin()
    {
        this->reduce( MPI_MIN );
    }

    void reduceMax()
    {
        this->reduce( MPI_MAX );
    }

    void reduceAve()
    {
        this->reduce( MPI_SUM );
        const auto& begin = BaseClass::times().begin();
        const auto& end = BaseClass::times().end();
        const auto size = m_comm.size();
        std::for_each( begin, end, [&] ( float& v ) { v /= size; } );
    }

    void reduce( const MPI_Op op )
    {
        Times times( BaseClass::numberOfStamps() );
        m_comm.allReduce( &(BaseClass::times()[0]), &times[0], times.size(), op );
        times.swap( BaseClass::times() );
    }
};

} // end of namespace mpi
#endif

} // end of namespace InSituVis
#endif
