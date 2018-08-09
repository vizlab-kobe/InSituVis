#pragma once
#include <string>
#include <kvs/CommandLine>
#include <kvs/Indent>


struct Input
{
private:
    kvs::CommandLine m_commandline;

public:
    enum Method
    {
        Uniform = 0,
        Metropolis,
        Rejection,
        Layered,
	Point,
    };

    enum Version
    {
        Old = 0,
        New
    };

    static std::string MethodName( const Method method );
    static std::string VersionName( const Version version );

public:
  size_t repetitions; ///< number of repetitions
  size_t regions;
  float step; ///< step length
  float base_opacity; ///< base opacity
  Method sampling_method; ///< sampling method
  Version sampling_version; ///< sampling version
  std::string filename; ///< input filename
  std::string tf_filename;
  int width;
  int height;
  Input( int argc, char** argv );
  bool parse();
  void print( std::ostream& os, const kvs::Indent& indent ) const;
};

