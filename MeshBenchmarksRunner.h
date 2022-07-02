#pragma once

#ifdef HAVE_CUDA
#include <cuda_profiler_api.h>
#endif

#include "MeshConfigs.h"
#include "dispatchBenchmarks.h"

template< template< typename, int, typename, typename, typename > class ConfigTemplate,
          typename CellTopology,
          int SpaceDimension,
          typename Real,
          typename GlobalIndex,
          typename LocalIndex >
struct MeshBenchmarksRunner
{
    // IMPORTANT NOTE:
    // The definition of the method must be separate from its declaration,
    // otherwise the compiler would always do implicit instead of explicit
    // instantiation.
    static bool
    run( Benchmark<> & benchmark,
         const Config::ParameterContainer & parameters );
};

template< template< typename, int, typename, typename, typename > class ConfigTemplate,
          typename CellTopology,
          int SpaceDimension,
          typename Real,
          typename GlobalIndex,
          typename LocalIndex >
bool
MeshBenchmarksRunner< ConfigTemplate,  CellTopology, SpaceDimension, Real, GlobalIndex, LocalIndex >::
run( Benchmark<> & benchmark,
     const Config::ParameterContainer & parameters )
{
   using Config = ConfigTemplate< CellTopology, SpaceDimension, Real, GlobalIndex, LocalIndex >;
   using MeshType = Mesh< Config, Devices::Host >;

   const String & meshFile = parameters.getParameter< String >( "mesh-file" );

   Logging::MetadataColumns metadataColumns = {
      {"mesh-file", meshFile},
      {"config", MeshType::Config::getConfigType()},
      {"topology", removeNamespaces( getType< typename MeshType::Config::CellTopology >() ) },
      {"space dim", std::to_string(MeshType::Config::spaceDimension)},
      {"real", getType< typename MeshType::RealType >()},
      {"gid_t", getType< typename MeshType::GlobalIndexType >()},
      {"lid_t", getType< typename MeshType::LocalIndexType >()},
   };
   benchmark.setMetadataColumns( metadataColumns );

   auto reader = getMeshReader( meshFile, "auto" );
   MeshType mesh;

   try {
      reader->loadMesh( mesh );
   }
   catch( const Meshes::Readers::MeshReaderError& e ) {
      std::cerr << "Failed to load mesh from file '" << meshFile << "'." << std::endl;
      return false;
   }

#ifdef HAVE_CUDA
   cudaProfilerStart();
#endif

   dispatchBenchmarks( benchmark, parameters, mesh, reader );

#ifdef HAVE_CUDA
   cudaProfilerStop();
#endif

   return true;
}

extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, float, int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, float, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, float, long int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, float, long int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, double, int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, double, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, double, long int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, double, long int, int >;

extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, float, int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, float, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, float, long int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, float, long int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, double, int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, double, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, double, long int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, double, long int, int >;

extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Polygon, 2, float, int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Polygon, 2, float, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Polygon, 2, float, long int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Polygon, 2, float, long int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Polygon, 2, double, int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Polygon, 2, double, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Polygon, 2, double, long int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Polygon, 2, double, long int, int >;

extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, float, int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, float, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, float, long int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, float, long int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, double, int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, double, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, double, long int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, double, long int, int >;

extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Polyhedron, 3, float, int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Polyhedron, 3, float, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Polyhedron, 3, float, long int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Polyhedron, 3, float, long int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Polyhedron, 3, double, int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Polyhedron, 3, double, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Polyhedron, 3, double, long int, short int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Polyhedron, 3, double, long int, int >;

extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, float, int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, float, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, float, long int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, float, long int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, double, int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, double, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, double, long int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, double, long int, int >;

extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, float, int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, float, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, float, long int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, float, long int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, double, int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, double, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, double, long int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, double, long int, int >;

extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Polygon, 2, float, int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Polygon, 2, float, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Polygon, 2, float, long int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Polygon, 2, float, long int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Polygon, 2, double, int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Polygon, 2, double, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Polygon, 2, double, long int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Polygon, 2, double, long int, int >;

extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, float, int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, float, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, float, long int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, float, long int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, double, int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, double, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, double, long int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, double, long int, int >;

extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Polyhedron, 3, float, int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Polyhedron, 3, float, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Polyhedron, 3, float, long int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Polyhedron, 3, float, long int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Polyhedron, 3, double, int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Polyhedron, 3, double, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Polyhedron, 3, double, long int, short int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Polyhedron, 3, double, long int, int >;
