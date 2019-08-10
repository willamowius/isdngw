/*
  $Id: resource.hxx,v 1.2 2009/05/22 22:50:33 jan Exp $
  
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

   Some fixes made by Niklas Ögren <niklas.ogren@7l.se>
   http://www.7l.se at 2003-05-09
*/

#ifndef _RESOURCE_HXX_
#define _RESOURCE_HXX_
                              
#include <ptlib.h>


class pots;

class InfoConnections
{
public:
  class NotFound
  {
  public:
    NotFound(const PString& what_arg) {}
  };  
  
public:
  typedef enum { ToPots, FromPots, Unknown, Error } t_direction;
  
  static void Init ();
  static void Clear ();

  static PBoolean LineGet_Token (const PString &token); 
  static PBoolean LineGet_Temp (PString &temp_token);
  static void ChangeToken (const PString &token_old, 
                           const PString &token_new);
  static PBoolean LineFree (const PString &token);
  static PBoolean NoLinesUsed();

  static PString GetDevice (const PString &token);

  static PBoolean PotsSet (const PString &token, pots *_pots_class);
  static pots *PotsGet (const PString &token);

  static PBoolean SetChannel (const PString &token, 
                          PIndirectChannel *_inchannel);
  static PIndirectChannel *GetChannel (const PString &token);

  static PBoolean DirectionSet (const PString &token, const t_direction dir);
  static t_direction DirectionGet (const PString &token);

  static PBoolean WriteStatus2HTML (PBoolean down);

private:
  class Info : public PObject
  {
    PCLASSINFO(Info, PObject);
 
  public:
    PString             device;
    PBoolean                used;
    t_direction         direction;
    pots *              pots_class;
    PString *           token;
    PIndirectChannel *  channel;
  };
  
  PARRAY(InfoArray, Info);
  
  static InfoArray      m_info;
  static PINDEX         m_nr_lines;
  static PMutex         m_mutex;
};

#endif /* _RESOURCE_HXX_ */
