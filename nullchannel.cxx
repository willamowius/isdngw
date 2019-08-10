/*
  $Id: nullchannel.cxx,v 1.2 2009/05/22 22:50:33 jan Exp $
  
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

#include "nullchannel.hxx"

PBoolean NullChannel::Read(void* buf, PINDEX len)
{
  memset(buf,0,len);

  PThread::Current()->Sleep(len/16);
  
  return TRUE;
}

PBoolean NullChannel::Write(const void* buf, PINDEX len)
{
  PThread::Current()->Sleep(len/16);

  return TRUE;
}
