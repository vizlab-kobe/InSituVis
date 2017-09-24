#include <kvs/ShaderSource>
#include "Program.h"

int main( int argc, char** argv )
{
    kvs::ShaderSource::AddSearchPath("../../Lib");
    local::Program program;
    return program.start( argc, argv );
}
