/*
  $Id: potschannel.cxx,v 1.3 2009/05/22 22:50:33 jan Exp $
  
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

#include <unistd.h>

#include <string>

#include <ptlib.h>
#include <h323.h>

#include "potschannel.hxx"
#include "pots.hxx"
#include "dsp.hxx"
#include "routing.hxx"


DTMF_Data::DTMF_Data(const void* _buf, PINDEX _len)
  : buf(_buf),
    start(0),
    len(_len)
{
}


PotsChannel::PotsChannel (pots *_pots, H323Connection *_connection, PString &token)
  : PChannel(),
    m_pots_class(_pots),
    m_connection(_connection),
    m_dtmf(new DspEncodeDTMF(0.3, 0.2, 8000, 8000))
{
  os_handle = m_pots_class->GetHandle();
  
  if (Config::UseAGC())
    m_gain = new DspGain;
  else
    m_gain = NULL;

  m_dtmf_list.DisallowDeleteObjects();
}


PotsChannel::~PotsChannel ()
{
  if (m_gain != NULL)
    delete m_gain;

  delete m_dtmf;
}


PBoolean PotsChannel::Close ()
{
  return TRUE;
}


PBoolean PotsChannel::IsOpen () const
{
  return m_pots_class->IsAudioOnline();
}


PBoolean PotsChannel::Read (void *buf, PINDEX len)
{
  // ISDN: read audio data
  lastReadCount = 2*m_pots_class->AudioRead (buf, (int) len/2);
	
  if (lastReadCount >= 0) {
	// send ISDN DTMF as H.323 user input
	DTMFDecode();
	return TRUE;
  } else {
    PAssert(m_connection->ClearCall(), "clear call");
    m_pots_class->HangUp();
    SetErrorValues(Miscellaneous, 0);
    return FALSE;
  }
}


PBoolean PotsChannel::Write (const void *buf, PINDEX len)
{
  BYTE new_buf[1000];
//  PTime t1;
  
//  PThread::Current()->Sleep(len/32);
  
  lastWriteCount = 0;

  // replace the voice by the DTMF
  PINDEX start=0;
  PINDEX len2=len;
  DTMF_Data* dtmf;
  while (len2>0) {
    // get the next DTMF data
    m_dtmf_mutex.Wait();
    dtmf = (DTMF_Data*)m_dtmf_list.GetAt(0);
    m_dtmf_mutex.Signal();
    if (dtmf == NULL)
      // no more DTMF to play,
      break;

    PINDEX l;

    if (dtmf->len<=len2) {
      // DTMF buffer shorter than or equal to voice buffer
      l = dtmf->len;
      memcpy(new_buf+start, (BYTE*)dtmf->buf+dtmf->start, l);

      // remove the DTMF from the list
      m_dtmf_mutex.Wait();
      delete (DTMF_Data*)m_dtmf_list.RemoveAt(0);
      m_dtmf_mutex.Signal();

      start += l;
      len2 -= l;
    }
    else {
      // DTMF buffer longer than voice buffer
      l = len2;
      memcpy(new_buf+start, (BYTE*)dtmf->buf+dtmf->start, l);

      dtmf->start += l;
      dtmf->len -= l;

      // modify the DTMF in the list
      m_dtmf_mutex.Wait();
      m_dtmf_list.SetAt(0, dtmf);
      m_dtmf_mutex.Signal();

      len2 = 0; // there is nothing left to play
    }
  }
  
  // complete with voice
  if (len2>0)
    memcpy(new_buf+start, (BYTE*)buf+start, len2);

  if (!WritePots(new_buf, len))
    return FALSE;

//  PTime t2;
//  PTRACE(1,"Write: " << t2-t1);
  
  return TRUE;
}

PBoolean PotsChannel::WritePots(const void* buf, PINDEX len)
{
  short tmp_buf[1000];
  const void *wbuf = (const void*)tmp_buf;

  if (m_gain != NULL)
    m_gain->ConvertArray((short*)buf, tmp_buf, len/2);
  else
    wbuf = buf;

  lastWriteCount += 2*m_pots_class->AudioWrite(wbuf, (int)len/2, TRUE);
  if (lastWriteCount >= 0) {
    return TRUE;
  } else {
    lastWriteCount = 0;
    SetErrorValues(Miscellaneous, 0);
    return FALSE;
  }
}


////////////////////////////////////////////////////////////////////
//  Write DTMF signals to the ISDN channel.                       //
////////////////////////////////////////////////////////////////////
//    button: button to be sent                                   //
////////////////////////////////////////////////////////////////////

PBoolean PotsChannel::DTMFEncode (char button)
{
  int samples;
  const short *data;

  samples = m_dtmf->GetNumberOfSamples();
  data = m_dtmf->GetSound(button);

  if (data == NULL)
    return FALSE;

  m_dtmf_mutex.Wait();
  m_dtmf_list.Append(new DTMF_Data(data, samples*2));
  m_dtmf_mutex.Signal();

  return TRUE;
}


////////////////////////////////////////////////////////////////////
//  When the pots object receives DTMF signals, we send them      //
//  as H.323 user input messages to the remote endpoint.          //
////////////////////////////////////////////////////////////////////  

void PotsChannel::DTMFDecode ()
{
  PString buttons;
  
  m_pots_class->GetDTMF(buttons);
  if (!buttons.IsEmpty())
    if (Config::UseUserInput())
      m_connection->SendUserInput(buttons);
}

