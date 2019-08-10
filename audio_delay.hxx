/*
  $Id: audio_delay.hxx,v 1.2 2009/05/22 22:50:33 jan Exp $
  
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

#ifndef _AUDIO_DELAY_HXX_
#define _AUDIO_DELAY_HXX_

#if !defined(P_USE_STANDARD_CXX_BOOL) && !defined(P_USE_INTEGER_BOOL)
	typedef int PBoolean;
#endif

#include <ptlib.h>

class AudioDelay : public PObject
{
  PCLASSINFO(AudioDelay, PObject);

  const int delay_conversion; // = 8; // == 8000Hz / ( 1000 ( ms / s ) )

  public:
    AudioDelay();
    int Delay(const int NumOctets);
    void Restart();
    int  GetError();

  protected:
    PTime  previousTime;
    PBoolean   firstTime;
    int    NumAllOctets;
};

#endif /* _AUDIO_DELAY_HXX_ */
