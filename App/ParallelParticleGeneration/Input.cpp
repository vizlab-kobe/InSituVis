#include "Input.h"


std::string Input::MethodName( const Method method )
{
    switch ( method )
    {
    case Uniform: return "uniform";
    case Metropolis: return "metoropolis";
    case Rejection: return "rejection";
    case Layered: return "layered";
    case Point: return "Point";
    default: return "unknown";
    }
}

std::string Input::VersionName( const Version version )
{
    switch ( version )
    {
    case Old: return "old";
    case New: return "new";
    default: return "unknown";
    }
}

Input::Input( int argc, char** argv ):
    repetitions( 1 ),
    regions( 1 ),
    step( 0.5f ),
    base_opacity( 0.5f ),
    sampling_method( Uniform ),
    sampling_version( Old ),
    filename( "" ),
    tf_filename( "" ),
    width( 512 ),
    height( 512 )
{
    m_commandline = kvs::CommandLine( argc, argv );
    m_commandline.addHelpOption();
    m_commandline.addValue( "input filename [*.fv]" );
    m_commandline.addOption( "repeats","Number of repetitions (default: 1).", 1, false );
    m_commandline.addOption( "regions","Number of regions (default: 1).", 1, false );
    m_commandline.addOption( "step","Sampling step (defualt: 0.5).", 1, false );
    m_commandline.addOption( "base_opacity","Base opacity (defualt: 0.5).", 1, false );
    m_commandline.addOption( "sampling_method", "Sampling method [0:uniform, 1:metropolis, 2:rejection, 3:layered, 4:point] (default: 0)", 1, false );
    m_commandline.addOption( "sampling_version", "Sampling version [0:old, 1:new] (default: 0)", 1, false );
    m_commandline.addOption( "tf_filename", "input tf_filename [*.kvsml, *.fld, *.inp]", 1, false );
    m_commandline.addOption( "width", "Screen width( default: 512).", 1, false);
    m_commandline.addOption( "height", "Screen height( default: 512).", 1, false);
}

bool Input::parse()
{
    if ( !m_commandline.parse() ) { return false; }
    filename = m_commandline.value<std::string>();
    if ( m_commandline.hasOption("repeats") ) repetitions = m_commandline.optionValue<size_t>("repeats");
    if ( m_commandline.hasOption("regions") ) regions = m_commandline.optionValue<size_t>("regions");
    if ( m_commandline.hasOption("step") ) step = m_commandline.optionValue<float>("step");
    if ( m_commandline.hasOption("base_opacity") ) base_opacity = m_commandline.optionValue<float>("base_opacity");
    if ( m_commandline.hasOption("sampling_method") ) sampling_method = Method( m_commandline.optionValue<int>("sampling_method") );
    if ( m_commandline.hasOption("sampling_version") ) sampling_version = Version( m_commandline.optionValue<int>("sampling_version") );
    if ( m_commandline.hasOption("tf_filename") ) tf_filename = m_commandline.optionValue<std::string>("tf_filename");
    if ( m_commandline.hasOption("width") ) width = m_commandline.optionValue<int>("width");
    if ( m_commandline.hasOption("height") ) height = m_commandline.optionValue<int>("height");
    return true;
}

void Input::print( std::ostream& os, const kvs::Indent& indent ) const
{
    os << indent << "Number of repetitions: " << repetitions << std::endl;
    os << indent << "Number of regions: " << regions << std::endl;
    os << indent << "Sampling step: " << step << std::endl;
    os << indent << "Base opacity: " << base_opacity << std::endl;
    os << indent << "Sampling method: " << MethodName( sampling_method ) << std::endl;
    os << indent << "Sampling version: " << VersionName( sampling_version ) << std::endl;
    os << indent << "Input filename: " << filename << std::endl;
    os << indent << "Screen width: " << width << std::endl;
    os << indent << "Screen height: " << height << std::endl;
}
