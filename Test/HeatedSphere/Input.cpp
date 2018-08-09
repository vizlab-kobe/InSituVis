#include "Input.h"



Input::Input( int argc, char** argv ):
    regions( 1 ),
    filename( "" ),
    tf_filename( "" ),
    width( 512 ),
    height( 512 )
{
    m_commandline = kvs::CommandLine( argc, argv );
    m_commandline.addHelpOption();
    m_commandline.addValue( "input filename [*.fv]" );
    m_commandline.addOption( "regions","Number of regions (default: 1).", 1, false );
    m_commandline.addOption( "tf_filename", "input tf_filename [*.kvsml, *.fld, *.inp]", 1, false );
    m_commandline.addOption( "width", "Screen width( default: 512).", 1, false);
    m_commandline.addOption( "height", "Screen height( default: 512).", 1, false);
}

bool Input::parse()
{
    if ( !m_commandline.parse() ) { return false; }
    filename = m_commandline.value<std::string>();
    if ( m_commandline.hasOption("regions") ) regions = m_commandline.optionValue<size_t>("regions");
    if ( m_commandline.hasOption("tf_filename") ) tf_filename = m_commandline.optionValue<std::string>("tf_filename");
    if ( m_commandline.hasOption("width") ) width = m_commandline.optionValue<int>("width");
    if ( m_commandline.hasOption("height") ) height = m_commandline.optionValue<int>("height");
    return true;
}

void Input::print( std::ostream& os, const kvs::Indent& indent ) const
{
    os << indent << "Number of regions: " << regions << std::endl;
    os << indent << "Input filename: " << filename << std::endl;
    os << indent << "Screen width: " << width << std::endl;
    os << indent << "Screen height: " << height << std::endl;
}
