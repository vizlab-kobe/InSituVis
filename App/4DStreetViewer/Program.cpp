#include "Program.h"
#include "Input.h"
#include "Model.h"
#include "View.h"
#include "Controller.h"
#include <cstdlib>


namespace
{

void Exit()
{
    local::Program::Logger().write("log");
}

}

namespace local
{

local::Logger Program::m_logger;

Program::Program()
{
    atexit( ::Exit );
}

int Program::exec( int argc, char** argv )
{
    local::Application app( argc, argv );

    local::Input input( argc, argv );
    if ( !input.parse() ) { return 1; }

    local::Model model( input );
    local::View view( &app, &model );
    local::Controller controller( &model, &view );

    return app.run();
}

} // end of namespace local
