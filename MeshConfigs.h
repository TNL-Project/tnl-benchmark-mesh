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
          typename LocalIndex = GlobalIndex,
          typename Id = void >
struct FullConfig
{
   using CellTopology = Cell;
   using RealType = Real;
   using GlobalIndexType = GlobalIndex;
   using LocalIndexType = LocalIndex;
   using IdType = Id;

   static constexpr int worldDimension = WorldDimension;
   static constexpr int meshDimension = Cell::dimension;

   static TNL::String getConfigType()
   {
      return "Full";
   }

   /****
    * Storage of mesh entities.
    */
   static constexpr bool entityStorage( int dimension )
   {
      /****
       * Vertices and cells must always be stored.
       */
      return true;
   }

   /****
    * Storage of subentities of mesh entities.
    */
   template< typename EntityTopology >
   static constexpr bool subentityStorage( EntityTopology, int SubentityDimension )
   {
      /****
       *  Subvertices of all stored entities must always be stored
       */
      return entityStorage( EntityTopology::dimension ) && entityStorage( SubentityDimension );
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
      return entityStorage( EntityTopology::dimension ) && entityStorage( SuperentityDimension );
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
          typename LocalIndex = GlobalIndex,
          typename Id = void >
struct MinimalConfig
{
   using CellTopology = Cell;
   using RealType = Real;
   using GlobalIndexType = GlobalIndex;
   using LocalIndexType = LocalIndex;
   using IdType = Id;

   static constexpr int worldDimension = WorldDimension;
   static constexpr int meshDimension = Cell::dimension;

   static TNL::String getConfigType()
   {
      return "Minimal";
   }

   /****
    * Storage of mesh entities.
    */
   static constexpr bool entityStorage( int dimension )
   {
      return ( dimension == 0 || dimension == meshDimension - 1 || dimension == meshDimension );
   }

   /****
    * Storage of subentities of mesh entities.
    */
   template< typename EntityTopology >
   static constexpr bool subentityStorage( EntityTopology, int SubentityDimension )
   {
      return entityStorage( EntityTopology::dimension ) &&
             ( SubentityDimension == 0 || ( SubentityDimension == meshDimension - 1 && EntityTopology::dimension == meshDimension ) );
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
      return false;
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
