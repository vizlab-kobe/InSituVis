#include "Input.h"

namespace local
{

Input::Input( int argc, char** argv ):
    dirname( "" ),
    dimensions( 0, 0, 0 ),
    extension( "mp4" ),
    position( 0, 0, 0 ),
    frame_rate( 5.0f )
{
    m_commandline = kvs::CommandLine( argc, argv );
    m_commandline.addHelpOption();
    m_commandline.addOption( "dir","Directory name.", 1, true );
    m_commandline.addOption( "ext","File extension. (default: mp4)", 1, false );
    m_commandline.addOption( "dims","Dimension of camera array.", 3, true );
    m_commandline.addOption( "pos", "Initial camera position. (default: 0 0 0)", 3, false );
    m_commandline.addOption( "fps", "Frame rate [frames/second]. (default: 5)", 1, false );
}

bool Input::parse()
{
    if ( !m_commandline.parse() ) { return false; }
    if ( m_commandline.hasOption("dir") ) dirname = m_commandline.optionValue<std::string>("dir");
    if ( m_commandline.hasOption("ext") ) extension = m_commandline.optionValue<std::string>("ext");
    if ( m_commandline.hasOption("dims") )
    {
        dimensions[0] = m_commandline.optionValue<int>("dims",0);
        dimensions[1] = m_commandline.optionValue<int>("dims",1);
        dimensions[2] = m_commandline.optionValue<int>("dims",2);
    }
    if ( m_commandline.hasOption("pos") )
    {
        position[0] = m_commandline.optionValue<int>("pos",0);
        position[1] = m_commandline.optionValue<int>("pos",1);
        position[2] = m_commandline.optionValue<int>("pos",2);
    }
    if ( m_commandline.hasOption("fps") ) frame_rate = m_commandline.optionValue<float>("fps");

    return true;
}

void Input::print( std::ostream& os, const kvs::Indent& indent ) const
{
    os << indent << "Directory name: " << dirname << std::endl;
    os << indent << "File extension: " << extension << std::endl;
    os << indent << "Dimension of camera array: " << dimensions << std::endl;
    os << indent << "Initial camera position: " << position << std::endl;
    os << indent << "Frame rate: " << frame_rate << std::endl;
}

} // end of namespace local
