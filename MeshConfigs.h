/***************************************************************************
                          MeshConfigs.h  -  description
                             -------------------
    begin                : Dec 12, 2017
    copyright            : (C) 2017 by Tomas Oberhuber et al.
    email                : tomas.oberhuber@fjfi.cvut.cz
 ***************************************************************************/

/* See Copyright Notice in tnl/Copyright */

// Implemented by: Jakub Klinkovsky

#pragma once

#include <TNL/String.h>
#include <TNL/Meshes/Topologies/SubentityVertexMap.h>

template< typename Cell,
          int WorldDimension = Cell::dimension,
          typename Real = double,
          typename GlobalIndex = int,
          typename LocalIndex = GlobalIndex >
struct FullConfig
{
   using CellTopology = Cell;
   using RealType = Real;
   using GlobalIndexType = GlobalIndex;
   using LocalIndexType = LocalIndex;

   static constexpr int worldDimension = WorldDimension;
   static constexpr int meshDimension = Cell::dimension;

   static TNL::String getConfigType()
   {
      return "Full";
   }

   /****
    * Storage of subentities of mesh entities.
    */
   template< typename EntityTopology >
   static constexpr bool subentityStorage( EntityTopology, int SubentityDimension )
   {
      return true;
   }

   /****
    * Storage of subentity orientations of mesh entities.
    * It must be false for vertices and cells.
    */
   template< typename EntityTopology >
   static constexpr bool subentityOrientationStorage( EntityTopology, int SubentityDimension )
   {
      return false;
   }

   /****
    * Storage of superentities of mesh entities.
    */
   template< typename EntityTopology >
   static constexpr bool superentityStorage( EntityTopology, int SuperentityDimension )
   {
      return true;
   }

   /****
    * Storage of mesh entity tags. Boundary tags are necessary for the mesh traverser.
    */
   template< typename EntityTopology >
   static constexpr bool entityTagsStorage( EntityTopology )
   {
      return true;
   }

   /****
    * Storage of the dual graph.
    *
    * If enabled, links from vertices to cells must be stored.
    */
   static constexpr bool dualGraphStorage()
   {
      return true;
   }

   /****
    * Cells must have at least this number of common vertices to be considered
    * as neighbors in the dual graph.
    */
   static constexpr int dualGraphMinCommonVertices = meshDimension;
};

template< typename Cell,
          int WorldDimension = Cell::dimension,
          typename Real = double,
          typename GlobalIndex = int,
          typename LocalIndex = GlobalIndex >
struct MinimalConfig
{
   using CellTopology = Cell;
   using RealType = Real;
   using GlobalIndexType = GlobalIndex;
   using LocalIndexType = LocalIndex;

   static constexpr int worldDimension = WorldDimension;
   static constexpr int meshDimension = Cell::dimension;

   static TNL::String getConfigType()
   {
      return "Minimal";
   }

   /****
    * Storage of subentities of mesh entities.
    */
   template< typename EntityTopology >
   static constexpr bool subentityStorage( EntityTopology, int SubentityDimension )
   {
      return SubentityDimension == 0 || ( SubentityDimension == meshDimension - 1 && EntityTopology::dimension == meshDimension );
   }

   /****
    * Storage of subentity orientations of mesh entities.
    * It must be false for vertices and cells.
    */
   template< typename EntityTopology >
   static constexpr bool subentityOrientationStorage( EntityTopology, int SubentityDimension )
   {
      return false;
   }

   /****
    * Storage of superentities of mesh entities.
    */
   template< typename EntityTopology >
   static constexpr bool superentityStorage( EntityTopology, int SuperentityDimension )
   {
      return ( EntityTopology::dimension == 0 || EntityTopology::dimension == meshDimension - 1 ) && SuperentityDimension == meshDimension;
   }

   /****
    * Storage of mesh entity tags. Boundary tags are necessary for the mesh traverser.
    */
   template< typename EntityTopology >
   static constexpr bool entityTagsStorage( EntityTopology )
   {
//      return false;
       // NOTE: needed for reorderEntities (could be optimized)
      using FaceTopology = typename TNL::Meshes::Topologies::Subtopology< CellTopology, meshDimension - 1 >::Topology;
      return superentityStorage( FaceTopology(), meshDimension ) &&
             ( EntityTopology::dimension >= meshDimension - 1 || subentityStorage( FaceTopology(), EntityTopology::dimension ) );
   }

   /****
    * Storage of the dual graph.
    *
    * If enabled, links from vertices to cells must be stored.
    */
   static constexpr bool dualGraphStorage()
   {
//      return false;
       // NOTE: needed for MeshOrdering (ordering could be pre-generated)
      return true;
   }

   /****
    * Cells must have at least this number of common vertices to be considered
    * as neighbors in the dual graph.
    */
   static constexpr int dualGraphMinCommonVertices = meshDimension;
};
