/*
  $Id: pots.hxx,v 1.2 2009/05/22 22:50:33 jan Exp $
  
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

#ifndef _POTS_HXX_
#define _POTS_HXX_

#include <ptlib.h>
#include "audio_delay.hxx"
#include "config.hxx"

#define TTY_TIMEOUT     0
#define TTY_CONNECT     1
#define TTY_RINGING     2
#define TTY_BUSY        3
#define TTY_NOCARRIER   4

class DspEchoComp;

class pots : public PObject
{
  PCLASSINFO(pots, PObject);
public:
  class ErrHangUp {
  public:
    ErrHangUp() {}
  };

  class ErrNotConnected {
  public:
    ErrNotConnected() {}
  };

  class ErrRead {
  public:
    ErrRead() {}
  };

  class ErrWrite {
  public:
    ErrWrite() {}
  };

public:
  static PBoolean IsFree(const PString& device);

public:
  pots (const PString&, const PString&);
  ~pots ();
  PBoolean Init ();
  void DeviceInit ();
  void DeviceClose ();
  PBoolean ListenOn (const PString&);
  void LCRouter (PString &);
  PBoolean StartATD (const PString&);
  int ATDReply();
  PBoolean AnswerCall ();
  PBoolean StartAudio ();
  PBoolean IsAudioOnline ();
  PBoolean HangUp ();
  void OnDTMF (unsigned char);
  void GetDTMF (PString&);
  int AudioRead (void*, int);
  void AudioReadFlush ();
  int AudioWrite (const void*, int, PBoolean block=FALSE);
  PBoolean WaitForCall (int);
  const PString& GetCallerNumber ();
  const PString& GetGatewayNumber ();

  int GetHandle() const
  { return m_device; }
  
private:  
  int           m_device;               // handle of the device
  PString       m_device_name;          // device name of the ISDN device
  PString       m_lockfile_name;        // file name of the lock file
  PString       m_msn;                  // ISDN MSN number
  char          m_buf[2000];
  PBoolean          m_isdn_connection;      // ISDN connection establish?
  PBoolean          m_audio_connection;     // recording/playback started?
  PString       m_number_caller;        // remote ISDN number (remote)
  PString       m_number_gateway;       // called ISDN number (local)
  
  DspEchoComp*  m_echo_comp;            // echo compensation object
  
#ifdef USEPWLIB
  PMutex        m_mutex_startaudio;
#endif
  
  PMutex        m_mutex_hangup_read;
  PMutex        m_mutex_hangup_write;

  PMutex        m_dtmf_mutex;
  PString       m_dtmf;                 // received DTMF buttons

  AudioDelay    m_delay;
  PStringToString   m_rewriteInfo;
};


class GwH323Connection;

class MakeCallThread : public PThread
{
  PCLASSINFO(MakeCallThread, PThread);
public:
  MakeCallThread(GwH323Connection&, const PString&);

  void End();
  
protected:
  void Main();

  GwH323Connection&     m_connection;
  PString               m_phone_number;
  PBoolean                  m_end;
};

#endif /* _POTS_HXX_ */
