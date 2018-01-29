#pragma once
#include <kvs/Program>
#include "Logger.h"

namespace local
{

class Program : public kvs::Program
{
    static local::Logger m_logger;
public:
    static local::Logger& Logger() { return m_logger; }

    Program();

    int exec( int argc, char** argv );
};

} // end of namespace local
