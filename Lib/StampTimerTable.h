#pragma once
#include "StampTimer.h"
#include <vector>
#include <string>
#include <kvs/Math>


namespace InSituVis
{

class StampTimerTable
{
public:
    using StampTimerList = std::vector<StampTimer>;

private:
    size_t m_nrows = 0; ///< max number of rows
    StampTimerList m_stamp_timers{}; ///< stamp timer list

public:
    StampTimerTable() = default;

    void push( const StampTimer& timer )
    {
        m_nrows = kvs::Math::Max( m_nrows, timer.numberOfStamps() );
        m_stamp_timers.push_back( timer );
    }

    void clear()
    {
        m_nrows = 0;
        m_stamp_timers.clear();
        m_stamp_timers.shrink_to_fit();
    }

    void print( std::ostream& os, const std::string delim = ", " ) const
    {
        const size_t nrows = m_nrows;
        const size_t ncols = m_stamp_timers.size();
        if ( ncols == 0 ) { return; }

        if ( this->has_title() )
        {
            for ( size_t j = 0; j < ncols - 1; ++j )
            {
                const auto& title = m_stamp_timers[j].title();
                os << title << delim;
            }
            const auto& title = m_stamp_timers[ ncols - 1 ].title();
            os << title << std::endl;
        }

        for ( size_t i = 0; i < nrows; ++i )
        {
            for ( size_t j = 0; j < ncols - 1; ++j )
            {
                const auto& col = m_stamp_timers[j];
                if ( i < col.numberOfStamps() ) { os << col.times()[i] << delim; }
                else { os << delim; }
            }
            const auto& col = m_stamp_timers[ ncols - 1 ];
            if ( i < col.numberOfStamps() ) { os << col.times()[i] << std::endl; }
            else { os << std::endl; }
        }
    }

    bool write(
        const std::string& filename,
        const std::string delim = ", ",
        std::ios_base::openmode mode = std::ios_base::app ) const
    {
        std::ofstream ofs( filename.c_str(), std::ios::out | mode );
        if ( !ofs ) { return false; }
        this->print( ofs, delim );
        return true;
    }

private:
    bool has_title() const
    {
        for ( const auto& t : m_stamp_timers )
        {
            if ( !t.title().empty() ) { return true; }
        }
        return false;
    }
};

} // end of namespace InSituVis
