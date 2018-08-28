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
    size_t regions; ///< number of regions
    int mapping; ///< mapping method
    std::string filename; ///< input filename
    std::string tf_filename; ///< input transfer function filename
    int width; ///< screen width
    int height; ///< screen height

    Input( int argc, char** argv );
    bool parse();
    void print( std::ostream& os, const kvs::Indent& indent ) const;
};

} // end of namespace local
