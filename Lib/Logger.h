#pragma once
#include <iostream>
#include <fstream>
#include <string>


namespace InSituVis
{

class Logger
{
private:
    std::string m_filename;
    std::ofstream m_stream;

public:
    Logger() {}
    Logger( const std::string& filename ): m_filename( filename ), m_stream( filename.c_str() ) {}
    std::ostream& operator ()() { return m_filename.empty() ? std::cout : m_stream; }
};

} // end of namespace InSituVis
