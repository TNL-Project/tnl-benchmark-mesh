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

#include <TNL/Config/ConfigDescription.h>
#include <TNL/Config/ParameterContainer.h>
#include <TNL/Devices/Host.h>
#include <TNL/Devices/Cuda.h>
#include <TNL/Devices/CudaDeviceInfo.h>

#include "MeshBenchmarks.h"

using namespace TNL;
using namespace TNL::Meshes;
using namespace TNL::benchmarks;

template< typename CellTopology,
          int WorldDimension = CellTopology::dimension,
          typename... Params >
bool
setMeshParameters( Params&&... params )
{
   bool status = MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, float, int, short int, void >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, float, int, short int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, float, int, short int, long int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, float, int, int, void >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, float, int, int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, float, int, int, long int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, float, long int, short int, void >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, float, long int, short int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, float, long int, short int, long int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, float, long int, int, void >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, float, long int, int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, float, long int, int, long int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, double, int, short int, void >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, double, int, short int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, double, int, short int, long int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, double, int, int, void >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, double, int, int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, double, int, int, long int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, double, long int, short int, void >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, double, long int, short int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, double, long int, short int, long int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, double, long int, int, void >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, double, long int, int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< MinimalConfig, CellTopology, WorldDimension, double, long int, int, long int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, float, int, short int, void >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, float, int, short int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, float, int, short int, long int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, float, int, int, void >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, float, int, int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, float, int, int, long int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, float, long int, short int, void >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, float, long int, short int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, float, long int, short int, long int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, float, long int, int, void >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, float, long int, int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, float, long int, int, long int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, double, int, short int, void >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, double, int, short int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, double, int, short int, long int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, double, int, int, void >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, double, int, int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, double, int, int, long int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, double, long int, short int, void >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, double, long int, short int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, double, long int, short int, long int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, double, long int, int, void >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, double, long int, int, int >::run( std::forward<Params>(params)... ) &&
                 MeshBenchmarksRunner< FullConfig, CellTopology, WorldDimension, double, long int, int, long int >::run( std::forward<Params>(params)... );
   return status;
}

bool
resolveCellTopology( Benchmark & benchmark,
                     Benchmark::MetadataMap metadata,
                     const String & meshFile )
{
   benchmark.newBenchmark( meshFile, metadata );

   Readers::VTKReader reader;
   if( ! reader.detectMesh( meshFile ) )
      return false;
   if( reader.getMeshType() != "Meshes::Mesh" ) {
      std::cerr << "The mesh type " << reader.getMeshType() << " is not supported in the VTK reader." << std::endl;
      return false;
   }

   using Readers::EntityShape;
   switch( reader.getCellShape() )
   {
      case EntityShape::Line:
         return setMeshParameters< Topologies::Edge >( benchmark, metadata, meshFile );
      case EntityShape::Triangle:
         return setMeshParameters< Topologies::Triangle >( benchmark, metadata, meshFile );
      case EntityShape::Quad:
         return setMeshParameters< Topologies::Quadrilateral >( benchmark, metadata, meshFile );
      case EntityShape::Tetra:
         return setMeshParameters< Topologies::Tetrahedron >( benchmark, metadata, meshFile );
//      case EntityShape::Hexahedron:
//         return setMeshParameters< Topologies::Hexahedron >( benchmark, metadata, meshFile );
      default:
         std::cerr << "unsupported cell topology: " << reader.getCellShape() << std::endl;
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

   if( ! parseCommandLine( argc, argv, conf_desc, parameters ) ) {
       conf_desc.printUsage( argv[ 0 ] );
       return 1;
   }

   Devices::Host::setup( parameters );
   Devices::Cuda::setup( parameters );

   const String & logFileName = parameters.getParameter< String >( "log-file" );
   const String & outputMode = parameters.getParameter< String >( "output-mode" );
   const int loops = parameters.getParameter< int >( "loops" );
   const int verbose = parameters.getParameter< int >( "verbose" );
   const String & meshFile = parameters.getParameter< String >( "mesh-file" );

   // open log file
   auto mode = std::ios::out;
   if( outputMode == "append" )
       mode |= std::ios::app;
   std::ofstream logFile( logFileName.getString(), mode );

   // init benchmark and common metadata
   Benchmark benchmark( loops, verbose );

   // prepare global metadata
   const int cpu_id = 0;
   Devices::CacheSizes cacheSizes = Devices::Host::getCPUCacheSizes( cpu_id );
   String cacheInfo = String( cacheSizes.L1data ) + ", "
                       + String( cacheSizes.L1instruction ) + ", "
                       + String( cacheSizes.L2 ) + ", "
                       + String( cacheSizes.L3 );
#ifdef HAVE_CUDA
   const int activeGPU = Devices::CudaDeviceInfo::getActiveDevice();
   const String deviceArch = String( Devices::CudaDeviceInfo::getArchitectureMajor( activeGPU ) ) + "." +
                             String( Devices::CudaDeviceInfo::getArchitectureMinor( activeGPU ) );
#endif
   Benchmark::MetadataMap metadata {
       { "host name", Devices::Host::getHostname() },
       { "architecture", Devices::Host::getArchitecture() },
       { "system", Devices::Host::getSystemName() },
       { "system release", Devices::Host::getSystemRelease() },
       { "start time", Devices::Host::getCurrentTime() },
       { "CPU model name", Devices::Host::getCPUModelName( cpu_id ) },
       { "CPU cores", Devices::Host::getNumberOfCores( cpu_id ) },
       { "CPU threads per core", Devices::Host::getNumberOfThreads( cpu_id ) / Devices::Host::getNumberOfCores( cpu_id ) },
       { "CPU max frequency (MHz)", Devices::Host::getCPUMaxFrequency( cpu_id ) / 1e3 },
       { "CPU cache sizes (L1d, L1i, L2, L3) (kiB)", cacheInfo },
#ifdef HAVE_CUDA
       { "GPU name", Devices::CudaDeviceInfo::getDeviceName( activeGPU ) },
       { "GPU architecture", deviceArch },
       { "GPU CUDA cores", Devices::CudaDeviceInfo::getCudaCores( activeGPU ) },
       { "GPU clock rate (MHz)", (double) Devices::CudaDeviceInfo::getClockRate( activeGPU ) / 1e3 },
       { "GPU global memory (GB)", (double) Devices::CudaDeviceInfo::getGlobalMemory( activeGPU ) / 1e9 },
       { "GPU memory clock rate (MHz)", (double) Devices::CudaDeviceInfo::getMemoryClockRate( activeGPU ) / 1e3 },
       { "GPU memory ECC enabled", Devices::CudaDeviceInfo::getECCEnabled( activeGPU ) },
#endif
   };

   if( ! resolveCellTopology( benchmark, metadata, meshFile ) )
      return EXIT_FAILURE;

   if( ! benchmark.save( logFile ) ) {
       std::cerr << "Failed to write the benchmark results to file '" << parameters.getParameter< String >( "log-file" ) << "'." << std::endl;
       return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}
