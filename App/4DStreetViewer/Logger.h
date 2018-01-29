#pragma once
#include <vector>
#include <string>


namespace local
{

class Logger
{
private:
    std::vector<float> m_position_change_times;
    std::vector<float> m_ray_change_times;

public:
    Logger() {}

    void pushPositionChangeTime( const float msec ) { m_position_change_times.push_back( msec ); }
    void pushRayChangeTime( const float msec ) { m_ray_change_times.push_back( msec ); }
    void write( const std::string& basename = "log" ) const;
};

} // end of namespace local
