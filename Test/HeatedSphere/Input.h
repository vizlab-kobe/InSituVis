#pragma once
#include <string>
#include <kvs/CommandLine>
#include <kvs/Indent>


struct Input
{
private:
    kvs::CommandLine m_commandline;

public:
    size_t regions;
    std::string filename; ///< input filename
    std::string tf_filename;
    int width;
    int height;

    Input( int argc, char** argv );
    bool parse();
    void print( std::ostream& os, const kvs::Indent& indent ) const;
};
