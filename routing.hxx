/*
  $Id: routing.hxx,v 1.3 2009/05/22 22:50:33 jan Exp $
  
   Copyright (c) 2000 Virtual Net

                    http://www.virtual-net.fr

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   Parts of this file are from isdn2h323, by Marco Budde.
   Copyright (c) 1999-2000 telos EDV Systementwicklung GmbH,
   Hamburg (Germany)          http://www.telos.de
*/

#ifndef _ROUTING_HXX_
#define _ROUTING_HXX_

#include <ptlib.h>
#include "config.hxx"

class Routing
{
public:
  class NotFound
  {
  public:
    NotFound(const PString& what_arg) {}
  };

public:
  static PBoolean Init ();
  static void Free();

  static const PString GetH323UserByNumber (const PString& nr);
  static PString GetNumberByUser (const PString& user);
  static const PString& GetATList()
  { return m_atlist; }
  
private:
  static PBoolean InitFromFile ();
  static void AddUser(const PString&, const PString&);

  PDECLARE_DICTIONARY(RoutingTable, PString, PString)
#if 0
  {
#endif 
  };

  PDECLARE_DICTIONARY(InverseRoutingTable, PString, PString)
#if 0
  {
#endif 
  };
  
  static RoutingTable m_routing;
  static InverseRoutingTable m_inverse_routing;

  static PString  m_atlist;
};

#endif /* _ROUTING_HXX_ */
