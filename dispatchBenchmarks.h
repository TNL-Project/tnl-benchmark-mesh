#pragma once

#include <set>
#include <sstream>
#include <string>

#include <TNL/Meshes/Grid.h>
#include <TNL/Meshes/Mesh.h>
#include <TNL/Meshes/Geometry/getEntityCenter.h>
#include <TNL/Meshes/Geometry/getEntityMeasure.h>
#include <TNL/Meshes/Geometry/getDecomposedMesh.h>
#include <TNL/Meshes/Geometry/getPlanarMesh.h>
#include <TNL/Meshes/TypeResolver/resolveMeshType.h>
#include <TNL/Pointers/DevicePointer.h>
#include <TNL/Algorithms/parallelFor.h>
#include <TNL/Algorithms/staticFor.h>
#include <TNL/Benchmarks/Benchmarks.h>

#include "MemoryInfo.h"
#include "Utils.h"

using namespace TNL;
using namespace TNL::Meshes;
using namespace TNL::Meshes::Readers;
using namespace TNL::Benchmarks;

static const std::set< std::string > valid_benchmarks = {
   "memory",
   "reader",
   "init",
   "copy",
   "centers",
   "measures",
   "boundary-measures",
   "spheres",
   "decomposition",
   "planar-correction",
};

static std::set< std::string >
parse_comma_list( const Config::ParameterContainer& parameters,
                  const char* parameter,
                  const std::set< std::string >& options )
{
   const String param = parameters.getParameter< String >( parameter );

   if( param == "all" )
      return options;

   std::stringstream ss( param.getString() );
   std::string s;
   std::set< std::string > set;

   while( std::getline( ss, s, ',' ) ) {
      if( ! options.count( s ) )
         throw std::logic_error( std::string("Invalid value in the comma-separated list for the parameter '")
                                 + parameter + "': '" + s + "'. The list contains: '" + param.getString() + "'." );

      set.insert( s );

      if( ss.peek() == ',' )
         ss.ignore();
   }

   return set;
}


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
      Algorithms::parallelFor< Device >(
            0, entitiesCount,
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
      Algorithms::parallelFor< Device >(
            0, entitiesCount,
            kernel_measures,
            &meshPointer.template getData< Device >(),
            measures.getData() );
   };

   benchmark.time< Device >( reset,
                             (std::is_same< Device, Devices::Host >::value) ? "CPU" : "GPU",
                             benchmark_func );
}

template< typename Device, typename Mesh >
static void benchmark_boundary_measures( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const Mesh & mesh_src )
{
   using Real = typename Mesh::RealType;
   using Index = typename Mesh::GlobalIndexType;
   using LocalIndex = typename Mesh::LocalIndexType;
   using DeviceMesh = Meshes::Mesh< typename Mesh::Config, Device >;

   // skip benchmarks on devices which the user did not select
   if( ! checkDevice< Device >( parameters ) )
      return;

   const Index facesCount = mesh_src.template getEntitiesCount< Mesh::getMeshDimension() - 1 >();
   const Index cellsCount = mesh_src.template getEntitiesCount< Mesh::getMeshDimension() >();

   const DeviceMesh mesh = mesh_src;
   Pointers::DevicePointer< const DeviceMesh > meshPointer( mesh );
   Containers::Array< Real, Device, Index > boundary_measures;
   boundary_measures.setSize( cellsCount );

   auto kernel_boundary_measures = [] __cuda_callable__
      ( Index fid,
        const DeviceMesh* mesh,
        Real* array )
   {
      const auto& face = mesh->template getEntity< Mesh::getMeshDimension() - 1 >( fid );
      const auto face_measure = getEntityMeasure( *mesh, face );

      const auto cellsCount = face.template getSuperentitiesCount< Mesh::getMeshDimension() >();
      for( LocalIndex c = 0; c < cellsCount; c++ ) {
         const auto cid = face.template getSuperentityIndex< Mesh::getMeshDimension() >( c );
         Algorithms::AtomicOperations< Device >::add( array[ cid ], face_measure );
      }
   };

   auto reset = [&]() {
      boundary_measures.setValue( 0.0 );
   };

   auto benchmark_func = [&] () {
      Algorithms::parallelFor< Device >(
            0, facesCount,
            kernel_boundary_measures,
            &meshPointer.template getData< Device >(),
            boundary_measures.getData() );
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
   Containers::Array< Real, Device, Index > spheres;
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
      TNL_ASSERT_TRUE( false, "local face index not found -- this is a BUG!" );
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
      Algorithms::parallelFor< Device >(
            0, facesCount,
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


static void ReaderDispatch( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, std::shared_ptr< MeshReader > reader )
{
   benchmark.setOperation( String( "Reader" ) );
   benchmark_reader( benchmark, parameters, reader );
}

template< typename Mesh >
static void InitDispatch( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, std::shared_ptr< MeshReader > reader )
{
   benchmark.setOperation( String( "Init" ) );
   benchmark_init< Mesh >( benchmark, parameters, reader );
}

template< typename M >
static void CopyDispatch( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh )
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

template< int EntityDimension, typename M >
static void CentersDispatch( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh )
{
   if constexpr( M::Config::subentityStorage( M::getMeshDimension(), 0 ) )
   {
      benchmark.setOperation( String("Centers (d = ") + convertToString(EntityDimension) + ")" );
      benchmark_centers< EntityDimension, Devices::Host >( benchmark, parameters, mesh );
#ifdef HAVE_CUDA
      benchmark_centers< EntityDimension, Devices::Cuda >( benchmark, parameters, mesh );
#endif
   }
}

template< int EntityDimension, typename M >
static void MeasuresDispatch( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh )
{
   benchmark.setOperation( String("Measures (d = ") + convertToString(EntityDimension) + ")" );
   benchmark_measures< EntityDimension, Devices::Host >( benchmark, parameters, mesh );
#ifdef HAVE_CUDA
   benchmark_measures< EntityDimension, Devices::Cuda >( benchmark, parameters, mesh );
#endif
}

template< typename M >
static void BoundaryMeasuresDispatch( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh )
{
   benchmark.setOperation( "Boundary measures" );
   benchmark_boundary_measures< Devices::Host >( benchmark, parameters, mesh );
#ifdef HAVE_CUDA
   benchmark_boundary_measures< Devices::Cuda >( benchmark, parameters, mesh );
#endif
}

template< typename M >
static void SpheresDispatch( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh )
{
   if constexpr( std::is_same< typename M::Config::CellTopology, Topologies::Triangle >::value ||
                  std::is_same< typename M::Config::CellTopology, Topologies::Tetrahedron >::value )
   {
      benchmark.setOperation( "Spheres" );
      benchmark_spheres< Devices::Host >( benchmark, parameters, mesh );
#ifdef HAVE_CUDA
      benchmark_spheres< Devices::Cuda >( benchmark, parameters, mesh );
#endif
   }
}

template< typename M >
static void DecompositionDispatch( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh_src )
{
   // Polygonal Mesh
   if constexpr( std::is_same< typename M::Config::CellTopology, Topologies::Polygon >::value )
   {
      benchmark.setOperation( String( "Decomposition (c)" ) );
      benchmark_decomposition< EntityDecomposerVersion::ConnectEdgesToCentroid >( benchmark, parameters, mesh_src );

      benchmark.setOperation( String( "Decomposition (p)" ) );
      benchmark_decomposition< EntityDecomposerVersion::ConnectEdgesToPoint >( benchmark, parameters, mesh_src );
   }

   // Polyhedral Mesh
   if constexpr( std::is_same< typename M::Config::CellTopology, Topologies::Polyhedron >::value )
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
}

template< typename M >
static void PlanarDispatch( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const M & mesh_src )
{
   if constexpr( M::Config::spaceDimension == 3 &&
                  (std::is_same< typename M::Config::CellTopology, Topologies::Polygon >::value ||
                  std::is_same< typename M::Config::CellTopology, Topologies::Polyhedron >::value ) )
   {
      benchmark.setOperation( String( "Planar Correction (c)" ) );
      benchmark_planar< EntityDecomposerVersion::ConnectEdgesToCentroid >( benchmark, parameters, mesh_src );

      benchmark.setOperation( String( "Planar Correction (p)" ) );
      benchmark_planar< EntityDecomposerVersion::ConnectEdgesToPoint >( benchmark, parameters, mesh_src );
   }
}


template< typename Mesh >
void dispatchBenchmarks( Benchmark<> & benchmark, const Config::ParameterContainer & parameters, const Mesh & mesh, std::shared_ptr< MeshReader > reader )
{
   const std::set< std::string > benchmarks = parse_comma_list( parameters, "benchmarks", valid_benchmarks );

   if( benchmarks.count( "memory" ) ) {
      // collect memory usage
      benchmark.setOperation( "Memory requirements" );
      MemoryBenchmarkResult meminfo = testMemoryUsage( parameters, mesh );
      auto noop = [](){};
      benchmark.time< Devices::Host >( "CPU", noop, meminfo );
   }

   // generic operations
   if( benchmarks.count( "reader" ) )
      ReaderDispatch( benchmark, parameters, reader );
   if( benchmarks.count( "init" ) )
      InitDispatch< Mesh >( benchmark, parameters, reader );
   if( benchmarks.count( "copy" ) )
      CopyDispatch( benchmark, parameters, mesh );

   // general computations on unstructured mesh
   if( benchmarks.count( "centers" ) ) {
      Algorithms::staticFor< int, 1, Mesh::getMeshDimension() + 1 >(
            [&] ( auto dim ) {
               CentersDispatch< dim >( benchmark, parameters, mesh );
            }
         );
   }
   if( benchmarks.count( "measures" ) ) {
      Algorithms::staticFor< int, 1, Mesh::getMeshDimension() + 1 >(
            [&] ( auto dim ) {
               MeasuresDispatch< dim >( benchmark, parameters, mesh );
            }
         );
   }
   if( benchmarks.count( "boundary-measures" ) )
      BoundaryMeasuresDispatch( benchmark, parameters, mesh );
   if( benchmarks.count( "spheres" ) )
      SpheresDispatch( benchmark, parameters, mesh );

   // computations on polygonal/polyhedral mesh
   if( benchmarks.count( "decomposition" ) )
      DecompositionDispatch( benchmark, parameters, mesh );
   if( benchmarks.count( "planar-correction" ) )
      PlanarDispatch( benchmark, parameters, mesh );
}
