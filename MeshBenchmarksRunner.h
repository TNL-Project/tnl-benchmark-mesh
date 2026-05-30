#pragma once

#ifdef HAVE_CUDA
#include <cuda_profiler_api.h>
#endif

#include "MeshConfigs.h"
#include "dispatchBenchmarks.h"

template< template< typename, int, typename, typename, typename > class ConfigTemplate,
          typename CellTopology,
          typename Real,
          typename GlobalIndex,
          typename LocalIndex >
bool
dispatch( Benchmark & benchmark,
          const Config::ParameterContainer & parameters,
          std::shared_ptr< MeshReader > reader )
{
   using Config = ConfigTemplate< CellTopology, CellTopology::dimension, Real, GlobalIndex, LocalIndex >;
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

   MeshType mesh;
   try {
      reader->loadMesh( mesh );
   }
   catch( const MeshReaderError& e ) {
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

template< template< typename, int, typename, typename, typename > class ConfigTemplate,
          typename Real,
          typename GlobalIndex,
          typename LocalIndex >
bool
resolveCellTopology( Benchmark & benchmark,
                     const Config::ParameterContainer & parameters )
{
   const String & meshFile = parameters.getParameter< String >( "mesh-file" );

   auto reader = getMeshReader( meshFile, "auto" );

   try {
      reader->detectMesh();
   }
   catch( const MeshReaderError& e ) {
      std::cerr << "Failed to detect mesh from file '" << meshFile << "'." << std::endl;
      std::cerr << e.what() << std::endl;
      return false;
   }

   if( reader->getMeshType() != "Meshes::Mesh" ) {
      std::cerr << "The mesh type " << reader->getMeshType() << " is not supported." << std::endl;
      return false;
   }

   using VTK::EntityShape;
   switch( reader->getCellShape() )
   {
//      case EntityShape::Line:
//         return dispatch< ConfigTemplate, Topologies::Edge, Real, GlobalIndex, LocalIndex >( benchmark, parameters, reader );
      case EntityShape::Triangle:
         return dispatch< ConfigTemplate, Topologies::Triangle, Real, GlobalIndex, LocalIndex >( benchmark, parameters, reader );
//      case EntityShape::Quad:
//         return dispatch< ConfigTemplate, Topologies::Quadrangle, Real, GlobalIndex, LocalIndex >( benchmark, parameters, reader );
      case EntityShape::Polygon:
         return dispatch< ConfigTemplate, Topologies::Polygon, Real, GlobalIndex, LocalIndex >( benchmark, parameters, reader );
      case EntityShape::Tetra:
         return dispatch< ConfigTemplate, Topologies::Tetrahedron, Real, GlobalIndex, LocalIndex >( benchmark, parameters, reader );
//      case EntityShape::Hexahedron:
//         return dispatch< ConfigTemplate, Topologies::Hexahedron, Real, GlobalIndex, LocalIndex >( benchmark, parameters, reader );
      case EntityShape::Polyhedron:
         return dispatch< ConfigTemplate, Topologies::Polyhedron, Real, GlobalIndex, LocalIndex >( benchmark, parameters, reader );
      default:
         std::cerr << "unsupported cell topology: " << getShapeName(reader->getCellShape()) << std::endl;
         return false;
   }
}

template< typename Real,
          typename GlobalIndex,
          typename LocalIndex >
struct MeshBenchmarksRunner
{
    static bool
    run( Benchmark & benchmark,
         const Config::ParameterContainer & parameters );
};

template< typename Real,
          typename GlobalIndex,
          typename LocalIndex >
bool
MeshBenchmarksRunner< Real, GlobalIndex, LocalIndex >::
run( Benchmark & benchmark,
     const Config::ParameterContainer & parameters )
{
   return resolveCellTopology< MinimalConfig, Real, GlobalIndex, LocalIndex >( benchmark, parameters ) &&
          resolveCellTopology< FullConfig, Real, GlobalIndex, LocalIndex >( benchmark, parameters );
}


