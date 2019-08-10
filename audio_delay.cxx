/*
  $Id: audio_delay.cxx,v 1.1.1.1 2004/09/19 17:43:31 jan Exp $
  
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

#include <ptlib.h>

#include "audio_delay.hxx"

AudioDelay::AudioDelay()
  : delay_conversion(8) // == 8000Hz / ( 1000 ( ms / s ) )
{
  firstTime = TRUE;
  NumAllOctets = 0;
}

void AudioDelay::Restart()
{
  firstTime = TRUE;
}

int AudioDelay::Delay(const int NumOctets)
{


  int delaytime, delaytime1;
  if (firstTime) {
    firstTime = FALSE;
    previousTime = PTime();
    NumAllOctets = NumOctets;
    return 0;
  }

  // NumAllOctets += NumOctets;
  delaytime1 = NumAllOctets / delay_conversion; // >>3

  PTime now;
  PTimeInterval delay = now - previousTime;
  delaytime =  delaytime1 - int( delay.GetMilliSeconds() ) ;


  //PTRACE(2,"Delaying: " << delaytime1 << '\t' << int( delay.GetMilliSeconds()) << '\t' << delaytime << '\t' << NumAllOctets  );

  if (delaytime > 0)
  {
#ifdef P_LINUX
    usleep(delaytime * 1000);
#else
    PThread::Current()->Sleep(delaytime);
#endif
  }

  NumAllOctets += NumOctets;


  return delaytime;
}
