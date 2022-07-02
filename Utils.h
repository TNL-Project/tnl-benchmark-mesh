#pragma once

#include <string>

#include <TNL/Config/ParameterContainer.h>
#include <TNL/Devices/Host.h>
#include <TNL/Devices/Cuda.h>

template< typename Device >
bool checkDevice( const TNL::Config::ParameterContainer& parameters )
{
   const std::string device = parameters.getParameter< std::string >( "devices" );
   if( device == "all" )
      return true;
   if( std::is_same< Device, TNL::Devices::Host >::value && device == "host" )
      return true;
   if( std::is_same< Device, TNL::Devices::Cuda >::value && device == "cuda" )
      return true;
   return false;
}

inline std::string removeNamespaces( const std::string& topology )
{
   std::size_t found = topology.find_last_of("::");
   return topology.substr( found + 1 );
}
