#pragma once

#include <TNL/Meshes/Mesh.h>

#include "libs/spatial/src/idle_point_multimap.hpp"

struct KdTreeOrdering
{
    // implementation for Mesh and Grid on host
    template< typename MeshEntity, typename Mesh, typename PermutationArray >
    static void
    getPermutations( const Mesh& mesh,
                     PermutationArray& perm,
                     PermutationArray& iperm )
    {
        static_assert( std::is_same< typename Mesh::DeviceType, TNL::Devices::Host >::value, "" );
        static_assert( std::is_same< typename PermutationArray::DeviceType, TNL::Devices::Host >::value, "" );
        using IndexType = typename Mesh::GlobalIndexType;
        using PointType = typename Mesh::PointType;

        const IndexType numberOfEntities = mesh.template getEntitiesCount< MeshEntity >();

        // allocate permutation vectors
        perm.setSize( numberOfEntities );
        iperm.setSize( numberOfEntities );

        spatial::idle_point_multimap< PointType::getSize(), PointType, IndexType > container;

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

template< bool reverse = true >
struct CuthillMcKeeOrdering
{
    template< typename MeshEntity, typename Mesh, typename PermutationArray >
    static void
    getPermutations( const Mesh& mesh,
                     PermutationArray& perm,
                     PermutationArray& iperm )
    {
        static_assert( std::is_same< typename Mesh::DeviceType, TNL::Devices::Host >::value, "" );
        static_assert( std::is_same< typename PermutationArray::DeviceType, TNL::Devices::Host >::value, "" );

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
        template< typename PermutationArray >
        static void getPermutations( const Mesh& mesh,
                                     PermutationArray& perm,
                                     PermutationArray& iperm )
        {
            reorderCells( mesh, perm, iperm );
        }
    };

    template< typename MeshEntity,
              typename Mesh >
    struct Wrapper< MeshEntity, Mesh, false >
    {
        template< typename PermutationArray >
        static void getPermutations( const Mesh& mesh,
                                     PermutationArray& perm,
                                     PermutationArray& iperm )
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
        template< typename PermutationArray >
        static void getPermutations( const TNL::Meshes::Grid< Dimension, Real, Device, Index >& mesh,
                                     PermutationArray& perm,
                                     PermutationArray& iperm )
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
        template< typename PermutationArray >
        static void getPermutations( const TNL::Meshes::Grid< Dimension, Real, Device, Index >& mesh,
                                     PermutationArray& perm,
                                     PermutationArray& iperm )
        {
            std::cerr << "CuthillMcKeeOrdering is not implemented for grids." << std::endl;
            throw 1;
        }
    };

    template< typename Mesh, typename PermutationArray >
    static void
    reorderCells( const Mesh& mesh,
                  PermutationArray& perm,
                  PermutationArray& iperm )
    {
        using IndexType = typename Mesh::GlobalIndexType;
        const IndexType numberOfCells = mesh.template getEntitiesCount< typename Mesh::Cell >();

        // allocate permutation vectors
        perm.setSize( numberOfCells );
        iperm.setSize( numberOfCells );

        // vector view for the neighbor counts
        const auto neighborCounts = mesh.getNeighborCounts().getConstView();

        // worker array - marker for inserted elements
        TNL::Containers::Array< bool, TNL::Devices::Host, IndexType > marker( numberOfCells );
        marker.setValue( false );
        // worker vector for collecting neighbors
        std::vector< IndexType > neighbors;

        // comparator functor
        auto comparator = [&] ( IndexType a, IndexType b )
        {
            return neighborCounts[ a ] < neighborCounts[ b ];
        };

        // counter for assigning indices
        IndexType permIndex = 0;

        // modifier for the reversed variant
        auto mod = [numberOfCells] ( IndexType i )
        {
            if( reverse )
                return numberOfCells - 1 - i;
            else
                return i;
        };

        // start with a peripheral node
        const IndexType peripheral = argMin( neighborCounts ).first;
        perm[ mod(permIndex) ] = peripheral;
        iperm[ peripheral ] = mod(permIndex);
        permIndex++;
        marker[ peripheral ] = true;

        // Cuthill--McKee
        IndexType i = 0;
        while( permIndex < numberOfCells ) {
            const IndexType k = perm[ mod(i) ];
            // collect all neighbors which were not marked yet
            const IndexType count = neighborCounts[ k ];
            for( IndexType n = 0; n < count; n++ ) {
                const IndexType nk = mesh.getCellNeighborIndex( k, n );
                if( marker[ nk ] == false ) {
                    neighbors.push_back( nk );
                    marker[ nk ] = true;
                }
            }
            // sort collected neighbors with ascending neighbors count
            std::sort( neighbors.begin(), neighbors.end(), comparator );
            // assign an index to the neighbors in this order
            for( auto nk : neighbors ) {
                perm[ mod(permIndex) ] = nk;
                iperm[ nk ] = mod(permIndex);
                permIndex++;
            }
            // next iteration
            i++;
            neighbors.clear();
        }
    }

    template< typename MeshEntity, typename Mesh, typename PermutationArray >
    static void
    reorderEntities( const Mesh& mesh,
                     PermutationArray& perm,
                     PermutationArray& iperm )
    {
        using IndexType = typename Mesh::GlobalIndexType;
        const IndexType numberOfEntities = mesh.template getEntitiesCount< MeshEntity >();
        const IndexType numberOfCells = mesh.template getEntitiesCount< typename Mesh::Cell >();

        // allocate permutation vectors
        perm.setSize( numberOfEntities );
        iperm.setSize( numberOfEntities );

        // worker array - marker for numbered entities
        TNL::Containers::Array< bool, TNL::Devices::Host, IndexType > marker( numberOfEntities );
        marker.setValue( false );

        IndexType permIndex = 0;
        for( IndexType K = 0; K < numberOfCells; K++ ) {
            const auto& cell = mesh.template getEntity< Mesh::getMeshDimension() >( K );
            for( typename Mesh::LocalIndexType e = 0; e < cell.template getSubentitiesCount< MeshEntity::getEntityDimension() >(); e++ ) {
                const auto E = cell.template getSubentityIndex< MeshEntity::getEntityDimension() >( e );
                if( marker[ E ] == false ) {
                    marker[ E ] = true;
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
    template< typename MeshEntity, typename MeshConfig, typename PermutationArray >
    static void
    getPermutations( const TNL::Meshes::Mesh< MeshConfig, Device >& mesh,
                     PermutationArray& perm,
                     PermutationArray& iperm )
    {
        using MeshHost = TNL::Meshes::Mesh< MeshConfig, TNL::Devices::Host >;
        using PermutationHost = typename PermutationArray::template Self< typename PermutationArray::ValueType, TNL::Devices::Host >;
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
    template< typename MeshEntity, typename MeshConfig, typename PermutationArray >
    static void
    getPermutations( const TNL::Meshes::Mesh< MeshConfig, TNL::Devices::Host >& mesh,
                     PermutationArray& perm,
                     PermutationArray& iperm )
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
    using PermutationArray = typename Mesh::GlobalIndexArray;
    using Ordering = MeshOrderingDeviceWrapper< OrderingMethod, Device >;

    PermutationArray perm_vertices, iperm_vertices, perm_faces, iperm_faces, perm_cells, iperm_cells;

// nvcc does not allow __cuda_callable__ lambdas inside private or protected sections
#ifdef __NVCC__
public:
#endif
    template< typename VectorOrView >
    static void _reorder_vector( VectorOrView& vector, const PermutationArray& perm )
    {
        static_assert( std::is_same< typename VectorOrView::DeviceType, Device >::value, "The vector must live on the same device as the mesh." );
        TNL_ASSERT( vector.getSize() == perm.getSize(),
                    std::cerr << "Mismatched sizes of `vector` and `perm` (" << vector.getSize()
                              << " vs. " << perm.getSize() << ")." << std::endl; );
        using namespace TNL;
        using ValueType = typename VectorOrView::ValueType;
        using DeviceType = typename VectorOrView::DeviceType;
        using IndexType = typename VectorOrView::IndexType;
        using Array = Containers::Array< ValueType, DeviceType, IndexType >;

        auto kernel = [] __cuda_callable__
           ( IndexType i,
             const ValueType* src,
             ValueType* dest,
             const typename PermutationArray::ValueType* perm )
        {
            dest[ i ] = src[ perm[ i ] ];
        };

        Array tmp;
        tmp.setLike( vector );

        Algorithms::ParallelFor< Device >::exec( (IndexType) 0, vector.getSize(),
                                                 kernel,
                                                 vector.getData(),
                                                 tmp.getData(),
                                                 perm.getData() );
        vector = tmp;
    }

    template< typename Matrix >
    static void _reorder_matrix( const Matrix& matrix1, Matrix& matrix2, const PermutationArray& _perm, const PermutationArray& _iperm )
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
            // nvcc does not allow lambdas to capture VLAs, even in host code (WTF!?)
            //    error: a variable captured by a lambda cannot have a type involving a variable-length array
            IndexType* _columns = columns;
            auto comparator = [=]( IndexType a, IndexType b ) {
                return _columns[ a ] < _columns[ b ];
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
