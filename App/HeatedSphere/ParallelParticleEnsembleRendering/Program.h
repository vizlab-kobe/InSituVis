#pragma once
#include <kvs/Program>


namespace local
{

class Program : public kvs::Program
{
    int exec( int argc, char** argv );
};

} // end of namespace local
