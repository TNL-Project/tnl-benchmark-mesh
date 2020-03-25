/***************************************************************************
                          MeshBenchmarks.h  -  description
                             -------------------
    begin                : Nov 21, 2017
    copyright            : (C) 2017 by Tomas Oberhuber et al.
    email                : tomas.oberhuber@fjfi.cvut.cz
 ***************************************************************************/

/* See Copyright Notice in tnl/Copyright */

// Implemented by: Jakub Klinkovsky

#pragma once

#include <TNL/Meshes/Grid.h>
#include <TNL/Meshes/Mesh.h>
#include <TNL/Meshes/Geometry/getEntityCenter.h>
#include <TNL/Meshes/Geometry/getEntityMeasure.h>
#include <TNL/Meshes/TypeResolver/TypeResolver.h>
#include <TNL/Pointers/DevicePointer.h>
#include <TNL/Algorithms/ParallelFor.h>
#include <TNL/Algorithms/TemplateStaticFor.h>
#include <TNL/Benchmarks/Benchmarks.h>
#include <TNL/Communicators/NoDistrCommunicator.h>

#ifdef HAVE_CUDA
#include <cuda_profiler_api.h>
#endif

#include "MeshOrdering.h"

#include "MeshConfigs.h"

using namespace TNL;
using namespace TNL::Meshes;
using namespace TNL::Benchmarks;

template< typename Real >
__cuda_callable__
Real
getSimplexMeasure( const TNL::Containers::StaticVector< 1, Real > (& points) [2] )
{
    return getVectorLength( points[0] - points[1] );
}

template< typename Real >
__cuda_callable__
Real
getSimplexMeasure( const TNL::Containers::StaticVector< 2, Real > (& points) [3] )
{
    return getTriangleArea( points[0], points[1] );
}

template< typename Real >
__cuda_callable__
Real
getSimplexMeasure( const TNL::Containers::StaticVector< 3, Real > (& points) [4] )
{
    return getTetrahedronVolume( points[0], points[1], points[2] );
}

template< typename Mesh >
struct MeshBenchmarks
{
   static_assert( std::is_same< typename Mesh::DeviceType, Devices::Host >::value, "The mesh should be loaded on the host." );

   static bool run( Benchmark & benchmark, const String & meshFile )
   {
      // initialization is done at compile-time! (we can't access Mesh::Config::worldDimension at run-time because of linker errors)
      // TODO: fix this! (e.g. make Mesh::getWorldDimension() method)
//      constexpr int worldDimension = Mesh::Config::worldDimension;

      Benchmark::MetadataColumns metadataColumns = {
//         {"mesh-file", meshFile},
         {"config", Mesh::Config::getConfigType()},
         {"topology", getType< typename Mesh::Config::CellTopology >().replace("Topologies::", "")},
//         {"wrld dim", worldDimension},
         {"real", getType< typename Mesh::RealType >()},
         {"gid_t", getType< typename Mesh::GlobalIndexType >()},
         {"lid_t", getType< typename Mesh::LocalIndexType >()},
         {"id_t", getType< typename Mesh::Config::IdType >()},
         {"order", ""},
      };

      Mesh mesh;
      DistributedMeshes::DistributedMesh<Mesh> distributedMesh;
      if( ! loadMesh< Communicators::NoDistrCommunicator >( meshFile, mesh, distributedMesh ) ) {
         std::cerr << "Failed to load mesh from file '" << meshFile << "'." << std::endl;
         return false;
      }

      // natural ordering
      metadataColumns.back() = {"order", "nat"};
      benchmark.setMetadataColumns( metadataColumns );
      dispatchAlgorithms( benchmark, mesh );

      // k-d tree ordering
      metadataColumns.back() = {"order", "kdt"};
      benchmark.setMetadataColumns( metadataColumns );
      using KdTreeOrdering = MeshOrdering< Mesh, KdTreeOrdering >;
      KdTreeOrdering kd;
      kd.reorder( mesh );
      dispatchAlgorithms( benchmark, mesh );

#ifdef HAVE_CUDA
      cudaProfilerStart();
#endif

      // RCM ordering
      metadataColumns.back() = {"order", "rcm"};
      benchmark.setMetadataColumns( metadataColumns );
      using RCMOrdering = MeshOrdering< Mesh, CuthillMcKeeOrdering<> >;
      RCMOrdering rcm;
      rcm.reorder( mesh );
      dispatchAlgorithms( benchmark, mesh );

#ifdef HAVE_CUDA
      cudaProfilerStop();
#endif

      return true;
   }

   static void dispatchAlgorithms( Benchmark & benchmark, const Mesh & mesh )
   {
      Algorithms::TemplateStaticFor< int, 1, Mesh::getMeshDimension() + 1, CentersDispatch >::execHost( benchmark, mesh );
      Algorithms::TemplateStaticFor< int, 1, Mesh::getMeshDimension() + 1, MeasuresDispatch >::execHost( benchmark, mesh );
      DualMeasuresDispatch::exec( benchmark, mesh );
      SpheresDispatch::exec( benchmark, mesh );
   }

   template< int EntityDimension >
   struct CentersDispatch
   {
      template< typename M,
                typename = typename std::enable_if< M::template entitiesAvailable< EntityDimension >() >::type >
      static void exec( Benchmark & benchmark, const M & mesh )
      {
         benchmark.setOperation( String("Centers (d = ") + convertToString(EntityDimension) + ")" );
         benchmark_centers< EntityDimension, Devices::Host >( benchmark, mesh );
#ifdef HAVE_CUDA
         benchmark_centers< EntityDimension, Devices::Cuda >( benchmark, mesh );
#endif
      }

      template< typename M,
                typename = typename std::enable_if< ! M::template entitiesAvailable< EntityDimension >() >::type,
                typename = void >
      static void exec( Benchmark & benchmark, const M & mesh )
      {
      }
   };

   template< int EntityDimension >
   struct MeasuresDispatch
   {
      template< typename M,
                typename = typename std::enable_if< M::template entitiesAvailable< EntityDimension >() >::type >
      static void exec( Benchmark & benchmark, const M & mesh )
      {
         benchmark.setOperation( String("Measures (d = ") + convertToString(EntityDimension) + ")" );
         benchmark_measures< EntityDimension, Devices::Host >( benchmark, mesh );
#ifdef HAVE_CUDA
         benchmark_measures< EntityDimension, Devices::Cuda >( benchmark, mesh );
#endif
      }

      template< typename M,
                typename = typename std::enable_if< ! M::template entitiesAvailable< EntityDimension >() >::type,
                typename = void >
      static void exec( Benchmark & benchmark, const M & mesh )
      {
      }
   };

   struct DualMeasuresDispatch
   {
      template< typename M,
                typename = typename std::enable_if<
                              std::is_same< typename M::Config::CellTopology, Topologies::Edge >::value ||
                              std::is_same< typename M::Config::CellTopology, Topologies::Triangle >::value ||
                              std::is_same< typename M::Config::CellTopology, Topologies::Tetrahedron >::value
                           >::type >
      static void exec( Benchmark & benchmark, const M & mesh )
      {
         benchmark.setOperation( "Dual M" );
         benchmark_dual_measures< Devices::Host >( benchmark, mesh );
#ifdef HAVE_CUDA
         benchmark_dual_measures< Devices::Cuda >( benchmark, mesh );
#endif
      }

      template< typename M,
                typename = typename std::enable_if< !(
                              std::is_same< typename M::Config::CellTopology, Topologies::Edge >::value ||
                              std::is_same< typename M::Config::CellTopology, Topologies::Triangle >::value ||
                              std::is_same< typename M::Config::CellTopology, Topologies::Tetrahedron >::value
                           ) >::type,
                typename = void >
      static void exec( Benchmark & benchmark, const M & mesh )
      {
      }
   };

   struct SpheresDispatch
   {
      template< typename M,
                typename = typename std::enable_if<
                              std::is_same< typename M::Config::CellTopology, Topologies::Triangle >::value ||
                              std::is_same< typename M::Config::CellTopology, Topologies::Tetrahedron >::value
                           >::type >
      static void exec( Benchmark & benchmark, const M & mesh )
      {
         benchmark.setOperation( "Spheres" );
         benchmark_spheres< Devices::Host >( benchmark, mesh );
#ifdef HAVE_CUDA
         benchmark_spheres< Devices::Cuda >( benchmark, mesh );
#endif
      }

      template< typename M,
                typename = typename std::enable_if< !(
                              std::is_same< typename M::Config::CellTopology, Topologies::Triangle >::value ||
                              std::is_same< typename M::Config::CellTopology, Topologies::Tetrahedron >::value
                           ) >::type,
                typename = void >
      static void exec( Benchmark & benchmark, const M & mesh )
      {
      }
   };

   template< int EntityDimension, typename Device >
   static void benchmark_centers( Benchmark & benchmark, const Mesh & mesh_src )
   {
      using Real = typename Mesh::RealType;
      using Index = typename Mesh::GlobalIndexType;
      using PointType = typename Mesh::PointType;
      using DeviceMesh = Meshes::Mesh< typename Mesh::Config, Device >;

      const Index entitiesCount = mesh_src.template getEntitiesCount< EntityDimension >();

      const DeviceMesh mesh = mesh_src;
      Pointers::DevicePointer< const DeviceMesh > meshPointer( mesh );
      Containers::Array< PointType, Device, Index > centers;
      centers.setSize( PointType::getSize() * entitiesCount );

      auto kernel_measures = [] __cuda_callable__
         ( Index i,
           const DeviceMesh* mesh,
           PointType* array )
      {
         const auto& entity = mesh->template getEntity< EntityDimension >( i );
         array[ i ] = getEntityCenter( *mesh, entity );
      };

      auto reset = [&]() {
         centers.setValue( 0.0 );
      };

      auto benchmark_func = [&] () {
         Algorithms::ParallelFor< Device >::exec(
               (Index) 0, entitiesCount,
               kernel_measures,
               &meshPointer.template getData< Device >(),
               centers.getData() );
      };

      benchmark.time< Device >( reset,
                                (std::is_same< Device, Devices::Host >::value) ? "CPU" : "GPU",
                                benchmark_func );
   }

   template< int EntityDimension, typename Device >
   static void benchmark_measures( Benchmark & benchmark, const Mesh & mesh_src )
   {
      using Real = typename Mesh::RealType;
      using Index = typename Mesh::GlobalIndexType;
      using DeviceMesh = Meshes::Mesh< typename Mesh::Config, Device >;

      const Index entitiesCount = mesh_src.template getEntitiesCount< EntityDimension >();

      const DeviceMesh mesh = mesh_src;
      Pointers::DevicePointer< const DeviceMesh > meshPointer( mesh );
      Containers::Array< Real, Device, Index > measures;
      measures.setSize( entitiesCount );

      auto kernel_measures = [] __cuda_callable__
         ( Index i,
           const DeviceMesh* mesh,
           Real* array )
      {
         const auto& entity = mesh->template getEntity< EntityDimension >( i );
         array[ i ] = getEntityMeasure( *mesh, entity );
      };

      auto reset = [&]() {
         measures.setValue( 0.0 );
      };

      auto benchmark_func = [&] () {
         Algorithms::ParallelFor< Device >::exec(
               (Index) 0, entitiesCount,
               kernel_measures,
               &meshPointer.template getData< Device >(),
               measures.getData() );
      };

      benchmark.time< Device >( reset,
                                (std::is_same< Device, Devices::Host >::value) ? "CPU" : "GPU",
                                benchmark_func );
   }

   template< typename Device >
   static void benchmark_dual_measures( Benchmark & benchmark, const Mesh & mesh_src )
   {
      static_assert( std::is_same< typename Mesh::Config::CellTopology, Topologies::Edge >::value ||
                     std::is_same< typename Mesh::Config::CellTopology, Topologies::Triangle >::value ||
                     std::is_same< typename Mesh::Config::CellTopology, Topologies::Tetrahedron >::value,
                     "The algorithm works only on simplices." );

      using Real = typename Mesh::RealType;
      using Index = typename Mesh::GlobalIndexType;
      using LocalIndex = typename Mesh::LocalIndexType;
      using PointType = typename Mesh::PointType;
      using DeviceMesh = Meshes::Mesh< typename Mesh::Config, Device >;

      const Index entitiesCount = mesh_src.template getEntitiesCount< Mesh::getMeshDimension() >();

      const DeviceMesh mesh = mesh_src;
      Pointers::DevicePointer< const DeviceMesh > meshPointer( mesh );
      Containers::Array< Real, Device, Index > measures;
      measures.setSize( entitiesCount );

      auto kernel_measures = [] __cuda_callable__
         ( Index i,
           const DeviceMesh* mesh,
           Real* array )
      {
         const auto& entity = mesh->template getEntity< Mesh::getMeshDimension() >( i );
//         constexpr auto facesCount = Mesh::Cell::template getSubentitiesCount< Mesh::getMeshDimension() - 1 >();
         constexpr auto facesCount = Mesh::Cell::template SubentityTraits< Mesh::getMeshDimension() - 1 >::count;
         PointType centers[ facesCount ];

         for( LocalIndex f = 0; f < facesCount; f++ ) {
            const auto fid = entity.template getSubentityIndex< Mesh::getMeshDimension() - 1 >( f );
            const auto& face = mesh->template getEntity< Mesh::getMeshDimension() - 1 >( fid );
            const auto _cells = face.template getSuperentitiesCount< Mesh::getMeshDimension() >();
            if( _cells == 1 )
               // boundary face - take the face center instead of the neighbor
               centers[ f ] = getEntityCenter( *mesh, face );
            else for( LocalIndex c = 0; c < _cells; c++ ) {
               const auto cid = face.template getSuperentityIndex< Mesh::getMeshDimension() >( c );
               if( cid != i ) {
                  const auto& cell = mesh->template getEntity< Mesh::getMeshDimension() >( cid );
                  centers[ f ] = getEntityCenter( *mesh, cell );
               }
            }
         }

         array[ i ] = getSimplexMeasure( centers );
      };

      auto reset = [&]() {
         measures.setValue( 0.0 );
      };

      auto benchmark_func = [&] () {
         Algorithms::ParallelFor< Device >::exec(
               (Index) 0, entitiesCount,
               kernel_measures,
               &meshPointer.template getData< Device >(),
               measures.getData() );
      };

      benchmark.time< Device >( reset,
                                (std::is_same< Device, Devices::Host >::value) ? "CPU" : "GPU",
                                benchmark_func );
   }

   template< typename Device >
   static void benchmark_spheres( Benchmark & benchmark, const Mesh & mesh_src )
   {
      static_assert( std::is_same< typename Mesh::Config::CellTopology, Topologies::Triangle >::value ||
                     std::is_same< typename Mesh::Config::CellTopology, Topologies::Tetrahedron >::value,
                     "The algorithm works only on triangles and tetrahedrons." );

      using Real = typename Mesh::RealType;
      using Index = typename Mesh::GlobalIndexType;
      using LocalIndex = typename Mesh::LocalIndexType;
      using DeviceMesh = Meshes::Mesh< typename Mesh::Config, Device >;

      const Index entitiesCount = mesh_src.template getEntitiesCount< 0 >();

      const DeviceMesh mesh = mesh_src;
      Pointers::DevicePointer< const DeviceMesh > meshPointer( mesh );
      Containers::Array< Real, Device, Index > spheres;
      spheres.setSize( entitiesCount );

//      auto hasSubvertex = [] __cuda_callable__
//         ( const typename DeviceMesh::Face & face,
//           const Index i )
//      {
////         constexpr auto verticesCount = Mesh::Face::template getSubentitiesCount< 0 >();
//         constexpr auto verticesCount = Mesh::Face::template SubentityTraits< 0 >::count;
//         for( LocalIndex v = 0; v < verticesCount; v++ ) {
//            const auto vid = face.template getSubentityIndex< 0 >( v );
//            if( vid == i )
//               return true;
//         }
//         return false;
//      };

      auto getLocalVertexIndex = [] __cuda_callable__
         ( const typename DeviceMesh::Cell & cell,
           const Index i )
      {
//         constexpr auto verticesCount = Mesh::Cell::template getSubentitiesCount< 0 >();
         constexpr auto verticesCount = Mesh::Cell::template SubentityTraits< 0 >::count;
         for( LocalIndex v = 0; v < verticesCount; v++ ) {
            const auto vid = cell.template getSubentityIndex< 0 >( v );
            if( vid == i ) {
               return v;
            }
         }
         TNL_ASSERT( false,
                     std::cerr << "local vertex index not found -- this is a BUG!" << std::endl; );
         return (LocalIndex) 0;
      };

      auto kernel_spheres = [getLocalVertexIndex] __cuda_callable__
         ( Index i,
           const DeviceMesh* mesh,
           Real* array )
      {
         Real s = 0.0;
         const auto& vertex = mesh->template getEntity< 0 >( i );
         const auto cellsCount = vertex.template getSuperentitiesCount< Mesh::getMeshDimension() >();
         for( LocalIndex c = 0; c < cellsCount; c++ ) {
            const auto cid = vertex.template getSuperentityIndex< Mesh::getMeshDimension() >( c );
            const auto& cell = mesh->template getEntity< Mesh::getMeshDimension() >( cid );
            // general version, but very slow
////            constexpr auto facesCount = Mesh::Cell::template getSubentitiesCount< Mesh::getMeshDimension() - 1 >();
//            constexpr auto facesCount = Mesh::Cell::template SubentityTraits< Mesh::getMeshDimension() - 1 >::count;
//            for( LocalIndex f = 0; f < facesCount; f++ ) {
//               const auto fid = cell.template getSubentityIndex< Mesh::getMeshDimension() - 1 >( f );
//               const auto& face = mesh->template getEntity< Mesh::getMeshDimension() - 1 >( fid );
//               if( ! hasSubvertex( face, i ) )
//                  s += getEntityMeasure( *mesh, face );
//            }
            // specialized version for simplices (assuming that opposite vertex and face have the same local index)
            const auto f = getLocalVertexIndex( cell, i );
            const auto fid = cell.template getSubentityIndex< Mesh::getMeshDimension() - 1 >( f );
            const auto& face = mesh->template getEntity< Mesh::getMeshDimension() - 1 >( fid );
            s += getEntityMeasure( *mesh, face );
         }
         array[ i ] = s;
      };

      auto reset = [&]() {
         spheres.setValue( 0.0 );
      };

      auto benchmark_func = [&] () {
         Algorithms::ParallelFor< Device >::exec(
               (Index) 0, entitiesCount,
               kernel_spheres,
               &meshPointer.template getData< Device >(),
               spheres.getData() );
      };

      benchmark.time< Device >( reset,
                                (std::is_same< Device, Devices::Host >::value) ? "CPU" : "GPU",
                                benchmark_func );
   }
};

template< template< typename, int, typename, typename, typename, typename > class ConfigTemplate,
          typename CellTopology,
          int WorldDimension,
          typename Real,
          typename GlobalIndex,
          typename LocalIndex,
          typename Id >
struct MeshBenchmarksRunner
{
    // IMPORTANT NOTE:
    // The definition of the method must be separate from its declaration,
    // otherwise the compiler would always do implicit instead of explicit
    // instantiation.
    static bool
    run( Benchmark & benchmark,
         Benchmark::MetadataMap metadata,
         const String & meshFile );
};

template< template< typename, int, typename, typename, typename, typename > class ConfigTemplate,
          typename CellTopology,
          int WorldDimension,
          typename Real,
          typename GlobalIndex,
          typename LocalIndex,
          typename Id >
bool
MeshBenchmarksRunner< ConfigTemplate,  CellTopology, WorldDimension, Real, GlobalIndex, LocalIndex, Id >::
run( Benchmark & benchmark,
     Benchmark::MetadataMap metadata,
     const String & meshFile )
{
   using Config = ConfigTemplate< CellTopology, WorldDimension, Real, GlobalIndex, LocalIndex, Id >;
   using MeshType = Mesh< Config, Devices::Host >;
   return MeshBenchmarks< MeshType >::run( benchmark, meshFile );
}

extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, float, int, short int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, float, int, short int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, float, int, short int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, float, int, int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, float, int, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, float, int, int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, float, long int, short int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, float, long int, short int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, float, long int, short int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, float, long int, int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, float, long int, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, float, long int, int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, double, int, short int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, double, int, short int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, double, int, short int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, double, int, int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, double, int, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, double, int, int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, double, long int, short int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, double, long int, short int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, double, long int, short int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, double, long int, int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, double, long int, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Edge, 1, double, long int, int, long int >;

extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, float, int, short int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, float, int, short int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, float, int, short int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, float, int, int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, float, int, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, float, int, int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, float, long int, short int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, float, long int, short int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, float, long int, short int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, float, long int, int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, float, long int, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, float, long int, int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, double, int, short int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, double, int, short int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, double, int, short int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, double, int, int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, double, int, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, double, int, int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, double, long int, short int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, double, long int, short int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, double, long int, short int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, double, long int, int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, double, long int, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Triangle, 2, double, long int, int, long int >;

extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, float, int, short int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, float, int, short int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, float, int, short int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, float, int, int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, float, int, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, float, int, int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, float, long int, short int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, float, long int, short int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, float, long int, short int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, float, long int, int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, float, long int, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, float, long int, int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, double, int, short int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, double, int, short int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, double, int, short int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, double, int, int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, double, int, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, double, int, int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, double, long int, short int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, double, long int, short int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, double, long int, short int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, double, long int, int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, double, long int, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Quadrilateral, 2, double, long int, int, long int >;

extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, float, int, short int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, float, int, short int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, float, int, short int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, float, int, int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, float, int, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, float, int, int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, float, long int, short int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, float, long int, short int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, float, long int, short int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, float, long int, int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, float, long int, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, float, long int, int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, double, int, short int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, double, int, short int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, double, int, short int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, double, int, int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, double, int, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, double, int, int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, double, long int, short int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, double, long int, short int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, double, long int, short int, long int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, double, long int, int, void >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, double, long int, int, int >;
extern template struct MeshBenchmarksRunner< FullConfig, Topologies::Tetrahedron, 3, double, long int, int, long int >;

extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, float, int, short int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, float, int, short int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, float, int, short int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, float, int, int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, float, int, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, float, int, int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, float, long int, short int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, float, long int, short int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, float, long int, short int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, float, long int, int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, float, long int, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, float, long int, int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, double, int, short int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, double, int, short int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, double, int, short int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, double, int, int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, double, int, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, double, int, int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, double, long int, short int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, double, long int, short int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, double, long int, short int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, double, long int, int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, double, long int, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Edge, 1, double, long int, int, long int >;

extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, float, int, short int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, float, int, short int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, float, int, short int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, float, int, int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, float, int, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, float, int, int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, float, long int, short int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, float, long int, short int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, float, long int, short int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, float, long int, int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, float, long int, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, float, long int, int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, double, int, short int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, double, int, short int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, double, int, short int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, double, int, int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, double, int, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, double, int, int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, double, long int, short int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, double, long int, short int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, double, long int, short int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, double, long int, int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, double, long int, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Triangle, 2, double, long int, int, long int >;

extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, float, int, short int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, float, int, short int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, float, int, short int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, float, int, int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, float, int, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, float, int, int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, float, long int, short int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, float, long int, short int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, float, long int, short int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, float, long int, int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, float, long int, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, float, long int, int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, double, int, short int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, double, int, short int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, double, int, short int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, double, int, int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, double, int, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, double, int, int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, double, long int, short int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, double, long int, short int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, double, long int, short int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, double, long int, int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, double, long int, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Quadrilateral, 2, double, long int, int, long int >;

extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, float, int, short int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, float, int, short int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, float, int, short int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, float, int, int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, float, int, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, float, int, int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, float, long int, short int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, float, long int, short int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, float, long int, short int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, float, long int, int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, float, long int, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, float, long int, int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, double, int, short int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, double, int, short int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, double, int, short int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, double, int, int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, double, int, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, double, int, int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, double, long int, short int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, double, long int, short int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, double, long int, short int, long int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, double, long int, int, void >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, double, long int, int, int >;
extern template struct MeshBenchmarksRunner< MinimalConfig, Topologies::Tetrahedron, 3, double, long int, int, long int >;
