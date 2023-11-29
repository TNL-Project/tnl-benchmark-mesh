#pragma once

#include <TNL/Config/parseCommandLine.h>
#include <TNL/Devices/Host.h>
#include <TNL/Devices/Cuda.h>

#include "MeshBenchmarksRunner.h"

using namespace TNL;
using namespace TNL::Meshes;
using namespace TNL::Benchmarks;

template< typename... Params >
bool
setMeshParameters( Params&&... params )
{
   bool status = MeshBenchmarksRunner< float, int, short int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< float, int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< float, long int, short int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< float, long int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< double, int, short int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< double, int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< double, long int, short int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< double, long int, int >::run( std::forward<Params>(params)... );
   return status;
}

static std::string
get_comma_list( const std::set< std::string >& options )
{
   std::string result;
   for( const std::string& item : valid_benchmarks )
      if( result.empty() )
         result += item;
      else
         result += ", " + item;
   return result;
}

void
setupConfig( Config::ConfigDescription & config )
{
   config.addDelimiter( "Benchmark settings:" );
   config.addEntry< String >( "log-file", "Log file name.", "tnl-benchmark-mesh.log");
   config.addEntry< String >( "output-mode", "Mode for opening the log file.", "overwrite" );
   config.addEntryEnum( "append" );
   config.addEntryEnum( "overwrite" );
   config.addEntry< int >( "loops", "Number of iterations for every computation.", 10 );
   config.addEntry< int >( "verbose", "Verbose mode.", 1 );
   config.addRequiredEntry< String >( "mesh-file", "Path of the mesh to load for the benchmark." );
   config.addEntry< String >( "devices", "Run benchmarks on these devices.", "all" );
   config.addEntryEnum( "all" );
   config.addEntryEnum( "host" );
   #ifdef HAVE_CUDA
   config.addEntryEnum( "cuda" );
   #endif
   config.addEntry< String >( "benchmarks", "Comma-separated list of benchmarks to run. Options: " + get_comma_list( valid_benchmarks ), "all" );

   config.addDelimiter( "Device settings:" );
   Devices::Host::configSetup( config );
   Devices::Cuda::configSetup( config );
}

int
main( int argc, char* argv[] )
{
   Config::ParameterContainer parameters;
   Config::ConfigDescription conf_desc;

   setupConfig( conf_desc );

   if( ! parseCommandLine( argc, argv, conf_desc, parameters ) )
       return 1;

   Devices::Host::setup( parameters );
   Devices::Cuda::setup( parameters );

   const String & logFileName = parameters.getParameter< String >( "log-file" );
   const String & outputMode = parameters.getParameter< String >( "output-mode" );
   const int loops = parameters.getParameter< int >( "loops" );
   const int verbose = parameters.getParameter< int >( "verbose" );

   // open log file
   auto mode = std::ios::out;
   if( outputMode == "append" )
       mode |= std::ios::app;
   std::ofstream logFile( logFileName.getString(), mode );

   // init benchmark and common metadata
   Benchmark<> benchmark( logFile, loops, verbose );

   // write global metadata into a separate file
   std::map< std::string, std::string > metadata = getHardwareMetadata();
   writeMapAsJson( metadata, logFileName + ".log", ".metadata.json" );

   if( ! setMeshParameters( benchmark, parameters ) )
      return EXIT_FAILURE;

   return EXIT_SUCCESS;
}
