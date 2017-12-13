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
#include <TNL/param-types.h>
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

   static TNL::String getType()
   {
      return TNL::String( "Meshes::FullConfig< " ) +
             Cell::getType() + ", " +
             TNL::String( WorldDimension ) + ", " +
             TNL::getType< Real >() + ", " +
             TNL::getType< GlobalIndex >() + ", " +
             TNL::getType< LocalIndex >() + ", " +
             TNL::getType< Id >() + " >";
   }

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
    * Storage of boundary tags of mesh entities. Necessary for the mesh traverser.
    */
   template< typename EntityTopology >
   static constexpr bool boundaryTagsStorage( EntityTopology )
   {
       return true;
   }
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

   static TNL::String getType()
   {
      return TNL::String( "Meshes::MinimalConfig< " ) +
             Cell::getType() + ", " +
             TNL::String( WorldDimension ) + ", " +
             TNL::getType< Real >() + ", " +
             TNL::getType< GlobalIndex >() + ", " +
             TNL::getType< LocalIndex >() + ", " +
             TNL::getType< Id >() + " >";
   }

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
    * Storage of boundary tags of mesh entities. Necessary for the mesh traverser.
    */
   template< typename EntityTopology >
   static constexpr bool boundaryTagsStorage( EntityTopology )
   {
      return false;
   }
};
