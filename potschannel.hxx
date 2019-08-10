/*
  $Id: potschannel.hxx,v 1.2 2009/05/22 22:50:33 jan Exp $
  
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

#ifndef _INCHANNEL_HXX_
#define _INCHANNEL_HXX_

#include <ptlib.h>
#include <h323.h>

#if !defined(P_USE_STANDARD_CXX_BOOL) && !defined(P_USE_INTEGER_BOOL)
	typedef int PBoolean;
#endif

class pots;
class DspGain;
class DspEncodeDTMF;

class DTMF_Data : public PObject
{
  PCLASSINFO(DTMF_Data,PObject);
public:
  DTMF_Data(const void*, PINDEX);
  
public:
  const void* buf;
  PINDEX start;
  PINDEX len;
};

class PotsChannel : public PChannel
{
  PCLASSINFO(PotsChannel, PChannel);

public:
  PotsChannel (pots *_pots, H323Connection *_connection,
                 PString &_token);
  ~PotsChannel();
  
  PBoolean Close(); 
  PBoolean IsOpen() const;
  PBoolean Read (void *buf, PINDEX len);
  PBoolean Write (const void *buf, PINDEX len);
  PBoolean WritePots (const void *buf, PINDEX len);
  PBoolean DTMFEncode (char button);
  
private:
  void DTMFDecode();
  
  pots *                m_pots_class;   // ISDN class
  H323Connection *      m_connection;   // the associated connection
  PMutex                m_dtmf_mutex;   // ISDN write mutex

  DspGain *             m_gain;         // AGC class
  
  PLIST(DTMF_List, DTMF_Data);
  DTMF_List             m_dtmf_list;
  DspEncodeDTMF *       m_dtmf;         // DTMF encoder class
};

#endif /* _INCHANNEL_HXX_ */
