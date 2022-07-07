#pragma once

#include <TNL/Meshes/Grid.h>
#include <TNL/Meshes/Mesh.h>
#include <TNL/Meshes/Geometry/getEntityCenter.h>
#include <TNL/Meshes/Geometry/getEntityMeasure.h>
#include <TNL/Meshes/Geometry/getDecomposedMesh.h>
#include <TNL/Meshes/Geometry/getPlanarMesh.h>
#include <TNL/Meshes/TypeResolver/resolveMeshType.h>
#include <TNL/Pointers/DevicePointer.h>
#include <TNL/Algorithms/ParallelFor.h>
#include <TNL/Algorithms/staticFor.h>
#include <TNL/Benchmarks/Benchmarks.h>

#include "MemoryInfo.h"
#include "Utils.h"

using namespace TNL;
using namespace TNL::Meshes;
using namespace TNL::Meshes::Readers;
using namespace TNL::Benchmarks;

template< typename Real >
__cuda_callable__
Real
getSimplexMeasure( const TNL::Containers::StaticVector< 1, Real > (& points) [2] )
{
    return TNL::l2Norm( points[0] - points[1] );
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


static void benchmark_reader( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, std::shared_ptr< MeshReader > reader )
{
   if( ! checkDevice< Devices::Host >( parameters ) )
      return;

   auto reset = [&]() {
      reader->reset();
   };

   auto benchmark_func = [&] () {
      reader->detectMesh();
   };

   benchmark.time< Devices::Host >( reset,
                                    "CPU",
                                    benchmark_func );
}

template< typename Mesh >
void benchmark_init( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, std::shared_ptr< MeshReader > reader )
{
   if( ! checkDevice< Devices::Host >( parameters ) )
      return;

   auto reset = [&]() {
      reader->detectMesh();
   };

   auto benchmark_func = [&] () {
      Mesh mesh;
      reader->loadMesh( mesh );
   };

   benchmark.time< Devices::Host >( reset,
                                    "CPU",
                                    benchmark_func );
}

template< typename DeviceFrom,
          typename DeviceTo,
          typename M >
static void benchmark_copy( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh_src )
{
   using MeshFrom = Meshes::Mesh< typename M::Config, DeviceFrom >;
   using MeshTo = Meshes::Mesh< typename M::Config, DeviceTo >;
   using Device = typename std::conditional_t< std::is_same< DeviceFrom, Devices::Host >::value &&
                                               std::is_same< DeviceTo, Devices::Host >::value,
                                               Devices::Host,
                                               Devices::Cuda >;

   // skip benchmarks on devices which the user did not select
   if( ! checkDevice< Device >( parameters ) )
      return;

   const MeshFrom meshFrom = mesh_src;

   auto benchmark_func = [&] () {
      MeshTo meshTo = meshFrom;
   };

   benchmark.time< Device >( [] () {},
                             (std::is_same< Device, Devices::Host >::value) ? "CPU" : "GPU",
                             benchmark_func );
}

template< int EntityDimension, typename Device, typename Mesh >
void benchmark_centers( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const Mesh & mesh_src )
{
   using Index = typename Mesh::GlobalIndexType;
   using PointType = typename Mesh::PointType;
   using DeviceMesh = Meshes::Mesh< typename Mesh::Config, Device >;

   // skip benchmarks on devices which the user did not select
   if( ! checkDevice< Device >( parameters ) )
      return;

   const Index entitiesCount = mesh_src.template getEntitiesCount< EntityDimension >();

   const DeviceMesh mesh = mesh_src;
   Pointers::DevicePointer< const DeviceMesh > meshPointer( mesh );
   Containers::Array< PointType, Device, Index > centers;
   centers.setSize( PointType::getSize() * entitiesCount );

   auto kernel_centers = [] __cuda_callable__
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
            kernel_centers,
            &meshPointer.template getData< Device >(),
            centers.getData() );
   };

   benchmark.time< Device >( reset,
                             (std::is_same< Device, Devices::Host >::value) ? "CPU" : "GPU",
                             benchmark_func );
}

template< int EntityDimension, typename Device, typename Mesh >
static void benchmark_measures( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const Mesh & mesh_src )
{
   using Real = typename Mesh::RealType;
   using Index = typename Mesh::GlobalIndexType;
   using DeviceMesh = Meshes::Mesh< typename Mesh::Config, Device >;

   // skip benchmarks on devices which the user did not select
   if( ! checkDevice< Device >( parameters ) )
      return;

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

template< typename Device, typename Mesh >
static void benchmark_dual_measures( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const Mesh & mesh_src )
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

   // skip benchmarks on devices which the user did not select
   if( ! checkDevice< Device >( parameters ) )
      return;

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
//      constexpr auto facesCount = Mesh::Cell::template getSubentitiesCount< Mesh::getMeshDimension() - 1 >();
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

template< typename Device, typename Mesh >
static void benchmark_spheres( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const Mesh & mesh_src )
{
   static_assert( std::is_same< typename Mesh::Config::CellTopology, Topologies::Triangle >::value ||
                  std::is_same< typename Mesh::Config::CellTopology, Topologies::Tetrahedron >::value,
                  "The algorithm works only on triangles and tetrahedrons." );

   using Real = typename Mesh::RealType;
   using Index = typename Mesh::GlobalIndexType;
   using LocalIndex = typename Mesh::LocalIndexType;
   using DeviceMesh = Meshes::Mesh< typename Mesh::Config, Device >;

   // skip benchmarks on devices which the user did not select
   if( ! checkDevice< Device >( parameters ) )
      return;

   const Index verticesCount = mesh_src.template getEntitiesCount< 0 >();
   const Index facesCount = mesh_src.template getEntitiesCount< Mesh::getMeshDimension() - 1 >();

   const DeviceMesh mesh = mesh_src;
   Pointers::DevicePointer< const DeviceMesh > meshPointer( mesh );
   Containers::Vector< Real, Device, Index > spheres;
   spheres.setSize( verticesCount );

   auto getLocalFaceIndex = [] __cuda_callable__
      ( const typename DeviceMesh::Cell & cell,
        const Index i )
   {
//      constexpr auto facesCount = Mesh::Cell::template getSubentitiesCount< 0 >();
      constexpr auto facesCount = Mesh::Cell::template SubentityTraits< Mesh::getMeshDimension() - 1 >::count;
      for( LocalIndex f = 0; f < facesCount; f++ ) {
         const auto fid = cell.template getSubentityIndex< Mesh::getMeshDimension() - 1 >( f );
         if( fid == i ) {
            return f;
         }
      }
      TNL_ASSERT( false,
                  std::cerr << "local face index not found -- this is a BUG!" << std::endl; );
      return (LocalIndex) 0;
   };

   auto kernel_spheres = [getLocalFaceIndex] __cuda_callable__
      ( Index fid,
        const DeviceMesh* mesh,
        Real* array )
   {
      const auto& face = mesh->template getEntity< Mesh::getMeshDimension() - 1 >( fid );
      const auto face_measure = getEntityMeasure( *mesh, face );

      const auto cellsCount = face.template getSuperentitiesCount< Mesh::getMeshDimension() >();
      for( LocalIndex c = 0; c < cellsCount; c++ ) {
         const auto cid = face.template getSuperentityIndex< Mesh::getMeshDimension() >( c );
         const auto& cell = mesh->template getEntity< Mesh::getMeshDimension() >( cid );
         // specialized version for simplices (assuming that opposite vertex and face have the same local index)
         const auto v = getLocalFaceIndex( cell, fid );
         const auto vid = cell.template getSubentityIndex< 0 >( v );
         Algorithms::AtomicOperations< Device >::add( array[ vid ], face_measure );
      }
   };

   auto reset = [&]() {
      spheres.setValue( 0.0 );
   };

   auto benchmark_func = [&] () {
      Algorithms::ParallelFor< Device >::exec(
            (Index) 0, facesCount,
            kernel_spheres,
            &meshPointer.template getData< Device >(),
            spheres.getData() );
   };

   benchmark.time< Device >( reset,
                             (std::is_same< Device, Devices::Host >::value) ? "CPU" : "GPU",
                             benchmark_func );
}

template< EntityDecomposerVersion DecomposerVersion,
          EntityDecomposerVersion SubDecomposerVersion = EntityDecomposerVersion::ConnectEdgesToPoint,
          typename M >
static void benchmark_decomposition( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh_src )
{
   // skip benchmarks on devices which the user did not select
   if( ! checkDevice< Devices::Host >( parameters ) )
      return;

   auto benchmark_func = [&] () {
      auto meshBuilder = decomposeMesh< DecomposerVersion, SubDecomposerVersion >( mesh_src );
   };

   benchmark.time< Devices::Host >( "CPU",
                                    benchmark_func );
}

template< EntityDecomposerVersion DecomposerVersion,
          typename M,
          std::enable_if_t< M::Config::spaceDimension == 3 &&
                           (std::is_same< typename M::Config::CellTopology, Topologies::Polygon >::value ||
                            std::is_same< typename M::Config::CellTopology, Topologies::Polyhedron >::value ), bool > = true >
static void benchmark_planar( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh_src )
{
   if( ! checkDevice< Devices::Host >( parameters ) )
      return;

   auto benchmark_func = [&] () {
      auto meshBuilder = planarCorrection< DecomposerVersion >( mesh_src );
   };

   benchmark.time< Devices::Host >( "CPU",
                                    benchmark_func );
}


struct ReaderDispatch
{
   static void exec( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, std::shared_ptr< MeshReader > reader )
   {
      benchmark.setOperation( String( "Reader" ) );
      benchmark_reader( benchmark, parameters, reader );
   }
};

template< typename Mesh >
struct InitDispatch
{
   static void exec( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, std::shared_ptr< MeshReader > reader )
   {
      benchmark.setOperation( String( "Init" ) );
      benchmark_init< Mesh >( benchmark, parameters, reader );
   }
};

struct CopyDispatch
{
   template< typename M >
   static void exec( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh )
   {
      benchmark.setOperation( String("Copy CPU->CPU") );
      benchmark_copy< Devices::Host, Devices::Host >( benchmark, parameters, mesh );
#ifdef HAVE_CUDA
      benchmark.setOperation( String("Copy CPU->GPU") );
      benchmark_copy< Devices::Host, Devices::Cuda >( benchmark, parameters, mesh );
      benchmark.setOperation( String("Copy GPU->CPU") );
      benchmark_copy< Devices::Cuda, Devices::Host >( benchmark, parameters, mesh );
      benchmark.setOperation( String("Copy GPU->GPU") );
      benchmark_copy< Devices::Cuda, Devices::Cuda >( benchmark, parameters, mesh );
#endif
   }
};

template< int EntityDimension >
struct CentersDispatch
{
   template< typename M,
             typename = typename std::enable_if< M::Config::subentityStorage( M::getMeshDimension(), 0 ) >::type >
   static void exec( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh )
   {
      benchmark.setOperation( String("Centers (d = ") + convertToString(EntityDimension) + ")" );
      benchmark_centers< EntityDimension, Devices::Host >( benchmark, parameters, mesh );
#ifdef HAVE_CUDA
      benchmark_centers< EntityDimension, Devices::Cuda >( benchmark, parameters, mesh );
#endif
   }

   template< typename M,
             typename = typename std::enable_if< ! M::Config::subentityStorage( M::getMeshDimension(), 0 ) >::type,
             typename = void >
   static void exec( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh )
   {
   }
};

template< int EntityDimension >
struct MeasuresDispatch
{
   template< typename M,
             typename = typename std::enable_if< M::Config::subentityStorage( M::getMeshDimension(), 0 ) >::type >
   static void exec( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh )
   {
      benchmark.setOperation( String("Measures (d = ") + convertToString(EntityDimension) + ")" );
      benchmark_measures< EntityDimension, Devices::Host >( benchmark, parameters, mesh );
#ifdef HAVE_CUDA
      benchmark_measures< EntityDimension, Devices::Cuda >( benchmark, parameters, mesh );
#endif
   }

   template< typename M,
             typename = typename std::enable_if< ! M::Config::subentityStorage( M::getMeshDimension(), 0 ) >::type,
             typename = void >
   static void exec( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh )
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
   static void exec( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh )
   {
      benchmark.setOperation( "Dual M" );
      benchmark_dual_measures< Devices::Host >( benchmark, parameters, mesh );
#ifdef HAVE_CUDA
      benchmark_dual_measures< Devices::Cuda >( benchmark, parameters, mesh );
#endif
   }

   template< typename M,
             typename = typename std::enable_if< !(
                           std::is_same< typename M::Config::CellTopology, Topologies::Edge >::value ||
                           std::is_same< typename M::Config::CellTopology, Topologies::Triangle >::value ||
                           std::is_same< typename M::Config::CellTopology, Topologies::Tetrahedron >::value
                        ) >::type,
             typename = void >
   static void exec( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh )
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
   static void exec( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh )
   {
      benchmark.setOperation( "Spheres" );
      benchmark_spheres< Devices::Host >( benchmark, parameters, mesh );
#ifdef HAVE_CUDA
      benchmark_spheres< Devices::Cuda >( benchmark, parameters, mesh );
#endif
   }

   template< typename M,
             typename = typename std::enable_if< !(
                           std::is_same< typename M::Config::CellTopology, Topologies::Triangle >::value ||
                           std::is_same< typename M::Config::CellTopology, Topologies::Tetrahedron >::value
                        ) >::type,
             typename = void >
   static void exec( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh )
   {
   }
};

struct DecompositionDispatch
{
   // Polygonal Mesh
   template< typename M,
             std::enable_if_t< std::is_same< typename M::Config::CellTopology, Topologies::Polygon >::value, bool > = true >
   static void exec( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh_src )
   {
      benchmark.setOperation( String( "Decomposition (c)" ) );
      benchmark_decomposition< EntityDecomposerVersion::ConnectEdgesToCentroid >( benchmark, parameters, mesh_src );

      benchmark.setOperation( String( "Decomposition (p)" ) );
      benchmark_decomposition< EntityDecomposerVersion::ConnectEdgesToPoint >( benchmark, parameters, mesh_src );
   }

   // Polyhedral Mesh
   template< typename M,
             std::enable_if_t< std::is_same< typename M::Config::CellTopology, Topologies::Polyhedron >::value, bool  > = true >
   static void exec( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh_src )
   {
      benchmark.setOperation( String( "Decomposition (cc)" ) );
      benchmark_decomposition< EntityDecomposerVersion::ConnectEdgesToCentroid,
                               EntityDecomposerVersion::ConnectEdgesToCentroid >( benchmark, parameters, mesh_src );

      benchmark.setOperation( String( "Decomposition (cp)" ) );
      benchmark_decomposition< EntityDecomposerVersion::ConnectEdgesToCentroid,
                               EntityDecomposerVersion::ConnectEdgesToPoint >( benchmark, parameters, mesh_src );

      benchmark.setOperation( String( "Decomposition (pc)" ) );
      benchmark_decomposition< EntityDecomposerVersion::ConnectEdgesToPoint,
                               EntityDecomposerVersion::ConnectEdgesToCentroid >( benchmark, parameters, mesh_src );

      benchmark.setOperation( String( "Decomposition (pp)" ) );
      benchmark_decomposition< EntityDecomposerVersion::ConnectEdgesToPoint,
                               EntityDecomposerVersion::ConnectEdgesToPoint >( benchmark, parameters, mesh_src );
   }

   // Other than Polygonal and Polyhedral Mesh
   template< typename M,
             std::enable_if_t< ! std::is_same< typename M::Config::CellTopology, Topologies::Polygon >::value &&
                               ! std::is_same< typename M::Config::CellTopology, Topologies::Polyhedron >::value, bool  > = true >
   static void exec( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh_src )
   {
   }
};

struct PlanarDispatch
{
   template< typename M,
             std::enable_if_t< M::Config::spaceDimension == 3 &&
                              (std::is_same< typename M::Config::CellTopology, Topologies::Polygon >::value ||
                               std::is_same< typename M::Config::CellTopology, Topologies::Polyhedron >::value ), bool > = true >
   static void exec( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh_src )
   {
      benchmark.setOperation( String( "Planar Correction (c)" ) );
      benchmark_planar< EntityDecomposerVersion::ConnectEdgesToCentroid >( benchmark, parameters, mesh_src );

      benchmark.setOperation( String( "Planar Correction (p)" ) );
      benchmark_planar< EntityDecomposerVersion::ConnectEdgesToPoint >( benchmark, parameters, mesh_src );
   }

   template< typename M,
             std::enable_if_t< M::Config::spaceDimension < 3 ||
                              (! std::is_same< typename M::Config::CellTopology, Topologies::Polygon >::value &&
                               ! std::is_same< typename M::Config::CellTopology, Topologies::Polyhedron >::value ), bool > = true >
   static void exec( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh_src )
   {
   }
};


template< typename Mesh >
void dispatchBenchmarks( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const Mesh & mesh, std::shared_ptr< MeshReader > reader )
{
   // collect memory usage
   benchmark.setOperation( "Memory requirements" );
   MemoryBenchmarkResult meminfo = testMemoryUsage( parameters, mesh );
   auto noop = [](){};
   benchmark.time< Devices::Host >( "CPU", noop, meminfo );

   // generic operations
   ReaderDispatch::exec( benchmark, parameters, reader );
   InitDispatch< Mesh >::exec( benchmark, parameters, reader );
   CopyDispatch::exec( benchmark, parameters, mesh );

   // general computations on unstructured mesh
   Algorithms::staticFor< int, 1, Mesh::getMeshDimension() + 1 >(
         [&] ( auto dim ) {
            CentersDispatch< dim >::exec( benchmark, parameters, mesh );
         }
      );
   Algorithms::staticFor< int, 1, Mesh::getMeshDimension() + 1 >(
         [&] ( auto dim ) {
            MeasuresDispatch< dim >::exec( benchmark, parameters, mesh );
         }
      );
   DualMeasuresDispatch::exec( benchmark, parameters, mesh );
   SpheresDispatch::exec( benchmark, parameters, mesh );

   // computations on polygonal/polyhedral mesh
   DecompositionDispatch::exec( benchmark, parameters, mesh );
   PlanarDispatch::exec( benchmark, parameters, mesh );
}
