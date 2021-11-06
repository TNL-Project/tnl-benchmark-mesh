/***************************************************************************
                          tnl-benchmark-mesh.h  -  description
                             -------------------
    begin                : Nov 12, 2017
    copyright            : (C) 2017 by Tomas Oberhuber et al.
    email                : tomas.oberhuber@fjfi.cvut.cz
 ***************************************************************************/

/* See Copyright Notice in tnl/Copyright */

// Implemented by: Jakub Klinkovsky

#pragma once

#include <TNL/Config/parseCommandLine.h>
#include <TNL/Devices/Host.h>
#include <TNL/Devices/Cuda.h>
#include <TNL/Cuda/DeviceInfo.h>

#include "MeshBenchmarks.h"

using namespace TNL;
using namespace TNL::Meshes;
using namespace TNL::Benchmarks;

template< typename CellTopology,
          int SpaceDimension = CellTopology::dimension,
          typename... Params >
bool
setMeshParameters( Params&&... params )
{
   bool status = MeshBenchmarksRunner< MinimalConfig, CellTopology, SpaceDimension, float, int, short int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, SpaceDimension, float, int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, SpaceDimension, float, long int, short int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, SpaceDimension, float, long int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, SpaceDimension, double, int, short int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, SpaceDimension, double, int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, SpaceDimension, double, long int, short int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, SpaceDimension, double, long int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, SpaceDimension, float, int, short int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, SpaceDimension, float, int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, SpaceDimension, float, long int, short int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, SpaceDimension, float, long int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, SpaceDimension, double, int, short int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, SpaceDimension, double, int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, SpaceDimension, double, long int, short int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, SpaceDimension, double, long int, int >::run( std::forward<Params>(params)... );
   return status;
}

bool
resolveCellTopology( Benchmark<> & benchmark,
                     Logging::MetadataMap metadata,
                     const Config::ParameterContainer & parameters )
{
   const String & meshFile = parameters.getParameter< String >( "mesh-file" );
   benchmark.newBenchmark( meshFile, metadata );

   Readers::VTKReader reader( meshFile );
   reader.detectMesh();
   if( reader.getMeshType() != "Meshes::Mesh" ) {
      std::cerr << "The mesh type " << reader.getMeshType() << " is not supported in the VTK reader." << std::endl;
      return false;
   }

   using VTK::EntityShape;
   switch( reader.getCellShape() )
   {
      case EntityShape::Line:
         return setMeshParameters< Topologies::Edge >( benchmark, metadata, parameters );
      case EntityShape::Triangle:
         return setMeshParameters< Topologies::Triangle >( benchmark, metadata, parameters );
      case EntityShape::Quad:
         return setMeshParameters< Topologies::Quadrangle >( benchmark, metadata, parameters );
      case EntityShape::Tetra:
         return setMeshParameters< Topologies::Tetrahedron >( benchmark, metadata, parameters );
//      case EntityShape::Hexahedron:
//         return setMeshParameters< Topologies::Hexahedron >( benchmark, metadata, parameters );
      default:
         std::cerr << "unsupported cell topology: " << getShapeName(reader.getCellShape()) << std::endl;
         return false;
   }
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
   Benchmark<> benchmark( loops, verbose );

   // prepare global metadata
   Logging::MetadataMap metadata = getHardwareMetadata();

   if( ! resolveCellTopology( benchmark, metadata, parameters ) )
      return EXIT_FAILURE;

   if( ! benchmark.save( logFile ) ) {
       std::cerr << "Failed to write the benchmark results to file '" << parameters.getParameter< String >( "log-file" ) << "'." << std::endl;
       return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}
