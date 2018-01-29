#include "Logger.h"
#include <iostream>
#include <fstream>


namespace
{

float WriteCSV( const std::string& filename, const std::vector<float>& values )
{
    std::ofstream ofs( filename.c_str() );
    if ( !ofs.is_open() ) { return false; }

    float average = 0.0f;
    std::vector<float>::const_iterator value = values.begin();
    while ( value != values.end() )
    {
        const float v = *(value++);
        ofs << v << "," << std::endl;
        average += v;
    }
    average /= values.size();

    ofs.close();

    return average;
}

}

namespace local
{

void Logger::write( const std::string& basename ) const
{
    const float t_position = ::WriteCSV( basename + "_position_change_time.csv", m_position_change_times );
    const float t_ray = ::WriteCSV( basename + "_ray_change_time.csv", m_ray_change_times );
    std::cout << "Position change time: " << t_position << " [msec]" << std::endl;
    std::cout << "Ray change time: " << t_ray << " [msec]" << std::endl;
}

} // end of namespace local
