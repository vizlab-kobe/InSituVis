#pragma once
#include <string>
#include <kvs/CommandLine>
#include <kvs/Indent>

namespace local
{

struct Input
{
private:
    kvs::CommandLine m_commandline;

public:
    std::string filename;

    Input( int argc, char** argv );
    bool parse();
    void print( std::ostream& os, const kvs::Indent& indent ) const;
};

} // end of namespace local
