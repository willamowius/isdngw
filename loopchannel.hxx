/*
  $Id: loopchannel.hxx,v 1.2 2009/05/22 22:50:33 jan Exp $
  
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

#ifndef _LOOPCHANNEL_HXX_
#define _LOOPCHANNEL_HXX_

#include <ptlib.h>
#include "audio_delay.hxx"

class LoopChannel : public PChannel
{
  PCLASSINFO(LoopChannel, PChannel);

public:
  LoopChannel(const PFilePath&);
  
  PBoolean IsOpen() const { return TRUE; }
  PBoolean Read (void *buf, PINDEX len);
  PBoolean Write (const void *buf, PINDEX len);

protected:
  PFile m_file;

  AudioDelay m_delay;
};

#endif /* _LOOPCHANNEL_HXX_ */
