#pragma once
#include <kvs/Timer>
#include <vector>
#include <string>


namespace InSituVis
{

class Timer
{
    struct Stamp
    {
        std::string label;
        float sec;
        Stamp( const std::string& l, float s ): label(l), sec(s) {}
    };

private:
    std::vector<Stamp> m_stamps;
    kvs::Timer m_timer;

public:
    Timer() {}

    void start() { m_timer.start(); }
    void stop() { m_timer.stop(); }

    void stamp( const std::string& label, const int trials = 1 )
    {
        Stamp stamp( label, m_timer.sec() / trials );
        m_stamps.push_back( stamp );
    }

    void print( std::ostream& os, const kvs::Indent& indent = kvs::Indent(0) ) const
    {
        std::vector<Stamp>::const_iterator stamp = m_stamps.begin();
        while ( stamp != m_stamps.end() )
        {
            os << indent << stamp->label << " : " << stamp->sec << " [sec]" << std::endl;
            stamp++;
        }
    }
};

} // end of namespace InSituVis
