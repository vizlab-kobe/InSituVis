#include "Input.h"

namespace local
{

Input::Input( int argc, char** argv ):
    filename( "" )
{
    m_commandline = kvs::CommandLine( argc, argv );
    m_commandline.addHelpOption();
    m_commandline.addOption( "f","Image filename.", 1, true );
}

bool Input::parse()
{
    if ( !m_commandline.parse() ) { return false; }
    if ( m_commandline.hasOption("f") ) filename = m_commandline.optionValue<std::string>("f");

    return true;
}

void Input::print( std::ostream& os, const kvs::Indent& indent ) const
{
    os << indent << "Image filename: " << filename << std::endl;
}

} // end of namespace local
