/*
  $Id: lcr.hxx,v 1.2 2009/05/22 22:50:33 jan Exp $
  
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

#ifndef _LCR_HXX_
#define _LCR_HXX_

#include <ptlib.h>

#include <unistd.h>   
#include <sys/types.h>
#include <sys/socket.h> 

#if !defined(P_USE_STANDARD_CXX_BOOL) && !defined(P_USE_INTEGER_BOOL)
	typedef int PBoolean;
#endif


class LCR : public PObject
{
  PCLASSINFO(LCR, PObject);

public:
  LCR (const PString& socket_path, const PString& _pbx);
  ~LCR();
  PBoolean GetNumber (const PString& number_in, PString& number_out,
                  const PString& length);
  const PString GetProviderName();
  
private:
  int                   m_sock;                 // isdnrate socket
  struct sockaddr       m_sock_addr;
  PBoolean                  m_isdnrate_ok;          // isdnrate daemon available?
  PString               m_pbx;                  // PBX prefix
  PString               m_provider_info;        // provider informations
};

#endif /* _LCR_HXX_ */
