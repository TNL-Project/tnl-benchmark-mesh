#pragma once

#include <TNL/Meshes/Mesh.h>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/cuthill_mckee_ordering.hpp>

#include "libs/spatial/src/idle_point_multimap.hpp"

struct KdTreeOrdering
{
    // implementation for Mesh and Grid on host
    template< typename MeshEntity, typename Mesh, typename PermutationVector >
    static void
    getPermutations( const Mesh& mesh,
                     PermutationVector& perm,
                     PermutationVector& iperm )
    {
        static_assert( std::is_same< typename Mesh::DeviceType, TNL::Devices::Host >::value, "" );
        static_assert( std::is_same< typename PermutationVector::DeviceType, TNL::Devices::Host >::value, "" );
        using IndexType = typename Mesh::GlobalIndexType;
        using PointType = typename Mesh::PointType;

        const IndexType numberOfEntities = mesh.template getEntitiesCount< MeshEntity >();

        // allocate permutation vectors
        perm.setSize( numberOfEntities );
        iperm.setSize( numberOfEntities );

        spatial::idle_point_multimap< PointType::size, PointType, IndexType > container;

        for( IndexType i = 0; i < numberOfEntities; i++ ) {
            const auto& entity = mesh.template getEntity< MeshEntity >( i );
            const auto center = getEntityCenter( mesh, entity );
            container.insert( std::make_pair( center, i ) );
        }

        container.rebalance();

        IndexType permIndex = 0;

        // in-order traversal of the k-d tree
        for( auto iter = container.cbegin();
             iter != container.cend();
             iter++ )
        {
            perm[ permIndex ] = iter->second;
            iperm[ iter->second ] = permIndex;
            permIndex++;
        }
    }
};

// interface for the boost implementation of the Cuthill-McKee ordering method
// http://www.boost.org/doc/libs/1_64_0/libs/graph/doc/cuthill_mckee_ordering.html
template< bool reverse = true >
struct CuthillMcKeeOrdering
{
    template< typename MeshEntity, typename Mesh, typename PermutationVector >
    static void
    getPermutations( const Mesh& mesh,
                     PermutationVector& perm,
                     PermutationVector& iperm )
    {
        static_assert( std::is_same< typename Mesh::DeviceType, TNL::Devices::Host >::value, "" );
        static_assert( std::is_same< typename PermutationVector::DeviceType, TNL::Devices::Host >::value, "" );

        // The reverse Cuthill-McKee ordering is implemented only for cells,
        // other entities are ordered from the current order of cells exactly
        // as if the mesh was re-initialized from the cell seeds.
        Wrapper< MeshEntity, Mesh >::getPermutations( mesh, perm, iperm );
    }

private:
    template< typename MeshEntity,
              typename Mesh,
              bool is_cell = MeshEntity::getEntityDimension() == Mesh::getMeshDimension() >
    struct Wrapper
    {
        template< typename PermutationVector >
        static void getPermutations( const Mesh& mesh,
                                     PermutationVector& perm,
                                     PermutationVector& iperm )
        {
            reorderCells( mesh, perm, iperm );
        }
    };

    template< typename MeshEntity,
              typename Mesh >
    struct Wrapper< MeshEntity, Mesh, false >
    {
        template< typename PermutationVector >
        static void getPermutations( const Mesh& mesh,
                                     PermutationVector& perm,
                                     PermutationVector& iperm )
        {
            reorderEntities< MeshEntity >( mesh, perm, iperm );
        }
    };

    // TODO: implement reorderCells for grids
    template< typename MeshEntity,
              int Dimension,
              typename Real,
              typename Device,
              typename Index >
    struct Wrapper< MeshEntity, TNL::Meshes::Grid< Dimension, Real, Device, Index >, true >
    {
        template< typename PermutationVector >
        static void getPermutations( const TNL::Meshes::Grid< Dimension, Real, Device, Index >& mesh,
                                     PermutationVector& perm,
                                     PermutationVector& iperm )
        {
            std::cerr << "CuthillMcKeeOrdering is not implemented for grids." << std::endl;
            throw 1;
        }
    };

    // TODO: implement reorderEntities for grids
    template< typename MeshEntity,
              int Dimension,
              typename Real,
              typename Device,
              typename Index >
    struct Wrapper< MeshEntity, TNL::Meshes::Grid< Dimension, Real, Device, Index >, false >
    {
        template< typename PermutationVector >
        static void getPermutations( const TNL::Meshes::Grid< Dimension, Real, Device, Index >& mesh,
                                     PermutationVector& perm,
                                     PermutationVector& iperm )
        {
            std::cerr << "CuthillMcKeeOrdering is not implemented for grids." << std::endl;
            throw 1;
        }
    };

    template< typename Mesh, typename PermutationVector >
    static void
    reorderCells( const Mesh& mesh,
                  PermutationVector& perm,
                  PermutationVector& iperm )
    {
        using IndexType = typename Mesh::GlobalIndexType;
        const IndexType numberOfCells = mesh.template getEntitiesCount< typename Mesh::Cell >();

        // allocate permutation vectors
        perm.setSize( numberOfCells );
        iperm.setSize( numberOfCells );

        using Graph = boost::adjacency_list< boost::vecS, boost::vecS, boost::undirectedS,
                                boost::property< boost::vertex_color_t, boost::default_color_type,
                                boost::property< boost::vertex_degree_t, int > > >;
        using GraphVertex = boost::graph_traits< Graph >::vertex_descriptor;
        using GraphSizeType = boost::graph_traits< Graph >::vertices_size_type;

        Graph G( numberOfCells );
        for( IndexType K = 0; K < numberOfCells; K++ ) {
            const auto& cell = mesh.template getEntity< Mesh::getMeshDimension() >( K );
            for( typename Mesh::LocalIndexType f = 0; f < cell.template getSubentitiesCount< Mesh::getMeshDimension() - 1 >(); f++ ) {
                const auto F = cell.template getSubentityIndex< Mesh::getMeshDimension() - 1 >( f );
                const auto& face = mesh.template getEntity< Mesh::getMeshDimension() - 1 >( F );
                for( typename Mesh::LocalIndexType c = 0; c < face.template getSuperentitiesCount< Mesh::getMeshDimension() >(); c++ ) {
                    const auto cellIndex = face.template getSuperentityIndex< Mesh::getMeshDimension() >( c );
                    if( cellIndex != K ) {
                        boost::add_edge( K, cellIndex, G );
                        break;
                    }
                }
            }
        }

        boost::property_map< Graph, boost::vertex_index_t >::type
            index_map = get( boost::vertex_index, G );

        std::vector< GraphVertex > graphVertexInvPerm( boost::num_vertices( G ) );
        if( reverse )
            boost::cuthill_mckee_ordering( G, graphVertexInvPerm.rbegin(), get( boost::vertex_color, G ), boost::make_degree_map( G ) );
        else
            boost::cuthill_mckee_ordering( G, graphVertexInvPerm.begin(), get( boost::vertex_color, G ), boost::make_degree_map( G ) );

        IndexType permIndex = 0;
        for( auto i : graphVertexInvPerm ) {
            perm[ permIndex ] = index_map[ i ];
            iperm[ index_map[ i ] ] = permIndex;
            permIndex++;
        }
    }

    template< typename MeshEntity, typename Mesh, typename PermutationVector >
    static void
    reorderEntities( const Mesh& mesh,
                     PermutationVector& perm,
                     PermutationVector& iperm )
    {
        using IndexType = typename Mesh::GlobalIndexType;
        const IndexType numberOfEntities = mesh.template getEntitiesCount< MeshEntity >();
        const IndexType numberOfCells = mesh.template getEntitiesCount< typename Mesh::Cell >();

        // allocate permutation vectors
        perm.setSize( numberOfEntities );
        iperm.setSize( numberOfEntities );

        std::unordered_set< IndexType > numberedEntities;
        IndexType permIndex = 0;
        for( IndexType K = 0; K < numberOfCells; K++ ) {
            const auto& cell = mesh.template getEntity< Mesh::getMeshDimension() >( K );
            for( typename Mesh::LocalIndexType e = 0; e < cell.template getSubentitiesCount< MeshEntity::getEntityDimension() >(); e++ ) {
                const auto E = cell.template getSubentityIndex< MeshEntity::getEntityDimension() >( e );
                if( numberedEntities.count( E ) == 0 ) {
                    numberedEntities.insert( E );
                    perm[ permIndex ] = E;
                    iperm[ E ] = permIndex;
                    permIndex++;
                }
            }
        }
    }
};


template< typename Ordering, typename Device >
struct MeshOrderingDeviceWrapper
{
    template< typename MeshEntity, typename MeshConfig, typename PermutationVector >
    static void
    getPermutations( const TNL::Meshes::Mesh< MeshConfig, Device >& mesh,
                     PermutationVector& perm,
                     PermutationVector& iperm )
    {
        using MeshHost = TNL::Meshes::Mesh< MeshConfig, TNL::Devices::Host >;
        using PermutationHost = typename PermutationVector::HostType;
        using MeshHostEntity = typename MeshHost::template EntityType< MeshEntity::getEntityDimension() >;

        const MeshHost meshHost = mesh;
        PermutationHost permHost, ipermHost;
        Ordering::template getPermutations< MeshHostEntity >( meshHost, permHost, ipermHost );
        perm.setLike( permHost );
        iperm.setLike( ipermHost );
        perm = permHost;
        iperm = ipermHost;
    }
};

template< typename Ordering >
struct MeshOrderingDeviceWrapper< Ordering, TNL::Devices::Host >
{
    template< typename MeshEntity, typename MeshConfig, typename PermutationVector >
    static void
    getPermutations( const TNL::Meshes::Mesh< MeshConfig, TNL::Devices::Host >& mesh,
                     PermutationVector& perm,
                     PermutationVector& iperm )
    {
        Ordering::template getPermutations< MeshEntity >( mesh, perm, iperm );
    }
};


// general implementation covering grids
template< typename Mesh,
          typename OrderingMethod = CuthillMcKeeOrdering<> >
class MeshOrdering
{
public:
    void reorder( Mesh& mesh ) {}

    template< int EntityDimension, typename Vector >
    void reorderVector( Vector& vector, bool inverse = false ) const {}

    template< int EntityDimension, typename Matrix >
    void reorderMatrix( const Matrix& matrix1, Matrix& matrix2, bool inverse = false ) const {}

    void reset_vertices() {}
    void reset_faces() {}
    void reset_cells() {}
};

// reordering makes sense only for unstructured meshes
template< typename MeshConfig, typename Device, typename OrderingMethod >
class MeshOrdering< TNL::Meshes::Mesh< MeshConfig, Device >, OrderingMethod >
{
    using Mesh = TNL::Meshes::Mesh< MeshConfig, Device >;
    using PermutationVector = typename Mesh::GlobalIndexVector;
    using Ordering = MeshOrderingDeviceWrapper< OrderingMethod, Device >;

    PermutationVector perm_vertices, iperm_vertices, perm_faces, iperm_faces, perm_cells, iperm_cells;

    template< typename Vector >
    static void _reorder_vector( Vector& vector, const PermutationVector& perm )
    {
        static_assert( std::is_same< typename Vector::DeviceType, Device >::value, "The vector must live on the same device as the mesh." );
        TNL_ASSERT( vector.getSize() == perm.getSize(),
                    std::cerr << "Mismatched sizes of `vector` and `perm` (" << vector.getSize()
                              << " vs. " << perm.getSize() << ")." << std::endl; );
        using namespace TNL;
        using IndexType = typename Vector::IndexType;
        using RealType = decltype(vector.getElement(0));

        auto kernel = [] __cuda_callable__
           ( IndexType i,
             const RealType* src,
             RealType* dest,
             const typename PermutationVector::RealType* perm )
        {
            dest[ i ] = src[ perm[ i ] ];
        };

        Vector tmp;
        tmp.setLike( vector );

        ParallelFor< Device >::exec( (IndexType) 0, vector.getSize(),
                                     kernel,
                                     vector.getData(),
                                     tmp.getData(),
                                     perm.getData() );
        vector = tmp;
    }

    template< typename Matrix >
    static void _reorder_matrix( const Matrix& matrix1, Matrix& matrix2, const PermutationVector& _perm, const PermutationVector& _iperm )
    {
        // TODO: implement on GPU
        static_assert( std::is_same< typename Matrix::DeviceType, TNL::Devices::Host >::value, "matrix reordering is implemented only for host" );

        static_assert( std::is_same< typename Matrix::DeviceType, Device >::value, "The matrix must live on the same device as the mesh." );

        using namespace TNL;
        using IndexType = typename Matrix::IndexType;

        matrix2.setLike( matrix1 );

        // general multidimensional accessors for permutation indices
        // TODO: this depends on the specific layout of dofs, general reordering of NDArray is needed
        auto perm = [&]( IndexType dof ) {
            TNL_ASSERT_LT( dof, matrix1.getRows(), "invalid dof index" );
            const IndexType i = dof / _perm.getSize();
            return i * _perm.getSize() + _perm[ dof % _perm.getSize() ];
        };
        auto iperm = [&]( IndexType dof ) {
            TNL_ASSERT_LT( dof, matrix1.getRows(), "invalid dof index" );
            const IndexType i = dof / _iperm.getSize();
            return i * _iperm.getSize() + _iperm[ dof % _iperm.getSize() ];
        };

        // set row lengths
        typename Matrix::CompressedRowLengthsVector rowLengths;
        rowLengths.setSize( matrix1.getRows() );
        for( IndexType i = 0; i < matrix1.getRows(); i++ ) {
            const IndexType maxLength = matrix1.getRowLength( perm( i ) );
            const auto row = matrix1.getRow( perm( i ) );
            IndexType length = 0;
            for( IndexType j = 0; j < maxLength; j++ )
                if( row.getElementColumn( j ) < matrix1.getColumns() )
                    length++;
            rowLengths[ i ] = length;
        }
        matrix2.setCompressedRowLengths( rowLengths );

        // set row elements
        for( IndexType i = 0; i < matrix2.getRows(); i++ ) {
            const IndexType rowLength = rowLengths[ i ];

            // extract sparse row
            const auto row1 = matrix1.getRow( perm( i ) );

            // permute
            typename Matrix::IndexType columns[ rowLength ];
            typename Matrix::RealType values[ rowLength ];
            for( IndexType j = 0; j < rowLength; j++ ) {
                columns[ j ] = iperm( row1.getElementColumn( j ) );
                values[ j ] = row1.getElementValue( j );
            }

            // sort
            IndexType indices[ rowLength ];
            for( IndexType j = 0; j < rowLength; j++ )
                indices[ j ] = j;
            auto comparator = [&]( IndexType a, IndexType b ) {
                return columns[ a ] < columns[ b ];
            };
            std::sort( indices, indices + rowLength, comparator );

            typename Matrix::IndexType sortedColumns[ rowLength ];
            typename Matrix::RealType sortedValues[ rowLength ];
            for( IndexType j = 0; j < rowLength; j++ ) {
                sortedColumns[ j ] = columns[ indices[ j ] ];
                sortedValues[ j ] = values[ indices[ j ] ];
            }

            matrix2.setRow( i, sortedColumns, sortedValues, rowLength );
        }
    }

public:
    void reorder( Mesh& mesh )
    {
        // TODO: check if they aren't the same dimension
        Ordering::template getPermutations< typename Mesh::Cell >( mesh, perm_cells, iperm_cells );
        mesh.template reorderEntities< Mesh::getMeshDimension() >( perm_cells, iperm_cells );
        Ordering::template getPermutations< typename Mesh::Face >( mesh, perm_faces, iperm_faces );
        mesh.template reorderEntities< Mesh::getMeshDimension() - 1 >( perm_faces, iperm_faces );
        Ordering::template getPermutations< typename Mesh::Vertex >( mesh, perm_vertices, iperm_vertices );
        mesh.template reorderEntities< 0 >( perm_vertices, iperm_vertices );
    }

    template< int EntityDimension, typename Vector >
    void reorderVector( Vector& vector, bool inverse = false ) const
    {
        if( EntityDimension == 0 ) {
            TNL_ASSERT( perm_vertices.getSize() > 0,
                        std::cerr << "You must call `::reorder( mesh )` before calling this method." << std::endl; );
            if( inverse == false )
                _reorder_vector( vector, perm_vertices );
            else
                _reorder_vector( vector, iperm_vertices );
        }
        else if( EntityDimension == Mesh::getMeshDimension() - 1 ) {
            TNL_ASSERT( perm_faces.getSize() > 0,
                        std::cerr << "You must call `::reorder( mesh )` before calling this method." << std::endl; );
            if( inverse == false )
                _reorder_vector( vector, perm_faces );
            else
                _reorder_vector( vector, iperm_faces );
        }
        else if( EntityDimension == Mesh::getMeshDimension() ) {
            TNL_ASSERT( perm_cells.getSize() > 0,
                        std::cerr << "You must call `::reorder( mesh )` before calling this method." << std::endl; );
            if( inverse == false )
                _reorder_vector( vector, perm_cells );
            else
                _reorder_vector( vector, iperm_cells );
        }
    }

    template< int EntityDimension, typename Matrix >
    void reorderMatrix( const Matrix& matrix1, Matrix& matrix2, bool inverse = false ) const
    {
        if( EntityDimension == 0 ) {
            TNL_ASSERT( perm_vertices.getSize() > 0,
                        std::cerr << "You must call `::reorder( mesh )` before calling this method." << std::endl; );
            if( inverse == false )
                _reorder_matrix( matrix1, matrix2, perm_vertices, iperm_vertices );
            else
                _reorder_matrix( matrix1, matrix2, iperm_vertices, perm_vertices );
        }
        else if( EntityDimension == Mesh::getMeshDimension() - 1 ) {
            TNL_ASSERT( perm_faces.getSize() > 0,
                        std::cerr << "You must call `::reorder( mesh )` before calling this method." << std::endl; );
            if( inverse == false )
                _reorder_matrix( matrix1, matrix2, perm_faces, iperm_faces );
            else
                _reorder_matrix( matrix1, matrix2, iperm_faces, perm_faces );
        }
        else if( EntityDimension == Mesh::getMeshDimension() ) {
            TNL_ASSERT( perm_cells.getSize() > 0,
                        std::cerr << "You must call `::reorder( mesh )` before calling this method." << std::endl; );
            if( inverse == false )
                _reorder_matrix( matrix1, matrix2, perm_cells, iperm_cells );
            else
                _reorder_matrix( matrix1, matrix2, iperm_cells, perm_cells );
        }
    }

    void reset_vertices()
    {
        perm_vertices.setSize( 0 );
        iperm_vertices.setSize( 0 );
    }

    void reset_faces()
    {
        perm_faces.setSize( 0 );
        iperm_faces.setSize( 0 );
    }

    void reset_cells()
    {
        perm_cells.setSize( 0 );
        iperm_cells.setSize( 0 );
    }
};
