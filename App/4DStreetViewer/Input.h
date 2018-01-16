#pragma once
#include <string>
#include <kvs/Vector3>
#include <kvs/CommandLine>
#include <kvs/Indent>

namespace local
{

struct Input
{
private:
    kvs::CommandLine m_commandline;

public:
    std::string dirname;
    kvs::Vec3i dimensions;
    std::string extension;
    kvs::Vec3i position;

    Input( int argc, char** argv );
    bool parse();
    void print( std::ostream& os, const kvs::Indent& indent ) const;
};

} // end of namespace local
