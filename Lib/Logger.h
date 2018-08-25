#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>


namespace InSituVis
{

namespace internal
{

class NullBuffer : public std::streambuf
{
public:
    int overflow( int c ) { return c; }
};

class NullStream : public std::ostream
{
    NullBuffer m_null_buffer;
public:
    NullStream() : std::ostream( &m_null_buffer ) {}
};

} // end of namespace internal

class Logger
{
private:
    std::string m_filename;
    std::ofstream m_stream;
    internal::NullStream m_null_stream;

public:
    Logger() {}
    Logger( const std::string& filename ): m_filename( filename ), m_stream( filename.c_str() ) {}
    std::ostream& operator ()() { return m_filename.empty() ? std::cout : m_stream; }
    std::ostream& operator ()( const bool enable ) { return ( enable ) ? (*this)() : m_null_stream; }
};

} // end of namespace InSituVis
