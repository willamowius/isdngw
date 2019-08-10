/*
  $Id: routing.cxx,v 1.3 2009/05/22 22:50:33 jan Exp $
  
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

#include <ptlib.h>

#include "routing.hxx"
#include "config.hxx"

/////////////////////
// static variables

Routing::RoutingTable Routing::m_routing;
Routing::InverseRoutingTable Routing::m_inverse_routing;
PString  Routing::m_atlist;


PBoolean Routing::Init()
{
  // first, empty the routing tables
  Free();  

  // fill from directory file
  return InitFromFile();
}

/////////////////////////////////////////
// Add the user from the directory file

PBoolean Routing::InitFromFile()
{
  const PString& directory_file = Config::GetDirectoryFilename();
  if (directory_file.IsEmpty()) {
    PTRACE(0, "No directory file");
    return TRUE;
  }

  if (!PFile::Exists(directory_file)) {
    PTRACE(0, "Cannot find the directory file: " << directory_file);
    return FALSE;
  }

  PConfig dir(directory_file, "");
  PStringList sections = dir.GetSections();
  for (PINDEX i=0; i<sections.GetSize(); i++) {
    PString num = dir.GetString(sections[i], "number", "");
    if (!num)
      AddUser(sections[i], num);
  }

  return TRUE;
}

/////////////////////////////////////
// Add a user in the routing tables

void Routing::AddUser(const PString& _name, const PString& _number)
{
  PTRACE(0, "Adding (name=" << _name << ", number=" << _number << ')');
  PString* name = new PString(_name);
  PString* number = new PString(_number);

  m_routing.SetAt(*number, name);
  m_inverse_routing.SetAt(*name, number);

  if (!m_atlist)
    m_atlist += ';';
  m_atlist += _number;
}

////////////////////////////
// Free the routing tables

void Routing::Free()
{
  m_routing.RemoveAll();
  m_inverse_routing.RemoveAll();
  m_atlist = "";
}


////////////////////////////////////////////////
// Get the H.323 user name of a given ISDN MSN

const PString Routing::GetH323UserByNumber (const PString& nr)
{
  PString* user = m_routing.GetAt(nr);
  if (user==NULL)
    return ""; // throw NotFound("Number not found");
  
  return *user;
}


///////////////////////////////////////////
// Get the ISDN MSN of a given H.323 user

PString Routing::GetNumberByUser (const PString& user)
{
  PString* nr = m_inverse_routing.GetAt(user);
  if (nr==NULL)
    return ""; // throw NotFound("User not found");
  
  return Config::GetDefaultMSN().Left(Config::GetDefaultMSN().GetLength()-nr->GetLength())+*nr;
}
