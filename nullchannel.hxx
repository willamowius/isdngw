/*
  $Id: nullchannel.hxx,v 1.2 2009/05/22 22:50:33 jan Exp $
  
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
*/

#ifndef _NULLCHANNEL_HXX_
#define _NULLCHANNEL_HXX_

#include <ptlib.h>

#if !defined(P_USE_STANDARD_CXX_BOOL) && !defined(P_USE_INTEGER_BOOL)
	typedef int PBoolean;
#endif


class NullChannel : public PChannel
{
  PCLASSINFO(NullChannel, PChannel);

public:
  NullChannel() { }

  PBoolean IsOpen() const { return TRUE; }
  PBoolean Read (void *buf, PINDEX len);
  PBoolean Write (const void *buf, PINDEX len);
};

#endif /* _NULLCHANNEL_HXX_ */
