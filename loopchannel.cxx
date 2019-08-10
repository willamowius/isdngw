/*
  $Id: loopchannel.cxx,v 1.2 2009/05/22 22:50:33 jan Exp $
  
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

   Some fixes made by Niklas Ögren <niklas.ogren@7l.se>
   http://www.7l.se at 2003-05-09
*/

#include "loopchannel.hxx"

LoopChannel::LoopChannel(const PFilePath& path)
  : m_file(path,PFile::ReadOnly)
{
  m_file.Open(PFile::ReadOnly,PFile::ModeDefault);
}

PBoolean LoopChannel::Read(void* buf, PINDEX len)
{
  m_delay.Delay(len/2);

  PINDEX start = 0;
  while (!m_file.Read((char*)buf+start,len)) {
    PINDEX count = m_file.GetLastReadCount();
    if (count == 0)
      return FALSE;
    
    start += count;
    len -= count;
    
    m_file.SetPosition(0);
  }
  
  return TRUE;
}

PBoolean LoopChannel::Write(const void* buf, PINDEX len)
{
  PThread::Current()->Sleep(len/16);

  return TRUE;
}
