/*
  $Id: pots.cxx,v 1.3 2009/05/22 22:50:33 jan Exp $
  
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <signal.h>

#include <iostream>

#include <ptlib.h>

#include "pots.hxx"
#include "audio_delay.hxx"
#include "alaw.hxx"
#include "routing.hxx"
#include "lcr.hxx"
#include "dsp.hxx"
#include "config.hxx"

#include "resource.hxx"
#include "connection.hxx"

// defines from isdn_tty.h

#define DLE 0x10
#define ETX 0x03
#define DC4 0x14


/////////////////////////////////////////////////
//  Wait for a file descriptor (read version)  //
/////////////////////////////////////////////////
//    handle: file handle                      //
//    timeout_sec,                             //
//    timeout_usec: time to wait in sec, usec  //
///////////////////////////////////////////////// 

PBoolean read_ready (int handle, int timeout_sec, int timeout_usec=0)
{
  fd_set         set;
  struct timeval tv;
  
  FD_ZERO(&set);
  FD_SET(handle, &set);
  tv.tv_sec = timeout_sec;
  tv.tv_usec = timeout_usec;

  return select(handle + 1, &set, NULL, NULL, &tv) == 1;
}


/////////////////////////////////////////////////
//  Wait for a file descriptor (write version) //
/////////////////////////////////////////////////
//    timeout_sec,                             //
//    timeout_usec: time to wait in sec, usec  //
///////////////////////////////////////////////// 

PBoolean write_ready (int handle, int timeout_sec, int timeout_usec=0)
{
  fd_set         set;
  struct timeval tv;
  
  FD_ZERO(&set);
  FD_SET(handle, &set);
  tv.tv_sec = timeout_sec;
  tv.tv_usec = timeout_usec;

  return select(handle + 1, NULL, &set, NULL, &tv) == 1;
}


/////////////////////////////////////////////////
//  Read a string from a device.               //
/////////////////////////////////////////////////
//  ret: -1  : error                           //
//       >=0 : number of bytes                 //
/////////////////////////////////////////////////
//    handle: file handle                      //
//    buffer: data buffer                      //
//    nr: buffer size                          //
//    timeout: max time to wait for one byte   //
/////////////////////////////////////////////////

int reads (int handle, char *buffer, int nr, int timeout = 1)
{
  int i;

  do {
    if (!read_ready(handle, timeout))
      return 0;
    if (read(handle, buffer, 1) != 1)
      return -1;
  } 
  while ((*buffer == '\n') || (*buffer == '\r'));
  buffer++;

  for (i=1 ; i < nr-1; i++) {
    if (!read_ready(handle, timeout))
      return i;
    if (read(handle, buffer, 1) != 1) {
      *buffer = '\0';
      return -1;
    }
    if ((*buffer == '\n') || (*buffer == '\r'))  {
      *buffer = '\0';
      return i;
    }
    buffer++;
  }
  *buffer = '\0';
  return i;
}


/////////////////////////////////////////////////
//  Write a string to a device.                //
/////////////////////////////////////////////////
//    handle: file handle                      //
//    buffer: data buffer                      //
/////////////////////////////////////////////////
//  ret: -1  : error                           //
//       >=0 : number of bytes                 //
/////////////////////////////////////////////////

int writes (int handle, const PString& buffer)
{
  int ret;

  ret = write(handle, (const char*)buffer, buffer.GetSize());
  if (ret == -1)
    return -1;
  tcdrain(handle);           

  if (write(handle, "\n", 1) == -1)
    return -1;
  tcdrain(handle);

  return ret; 
}


/////////////////////////////////////////////////
//  Write string "w_buf" to "device", read a   //
//  string from "device" and compare it with   //
//  the string "c_buf".                        //
/////////////////////////////////////////////////
//    r_timeout: read timeout in sec           //
/////////////////////////////////////////////////
//    ret: ok (TRUE), error (FALSE)            //
/////////////////////////////////////////////////
 
PBoolean reads_compare (int device, const PString& c_buf, int r_timeout=1)
{
  char buf[500];
  strcpy(buf, "");
  if (reads(device, buf, sizeof(buf), r_timeout) == -1)
    return FALSE;

  return c_buf == buf;
}

PBoolean writes_reads_compare (int device, const PString& w_buf,
                           const PString& c_buf, int r_timeout=1)
{
  if (!w_buf.IsEmpty())
    if (writes(device, w_buf) < (int)w_buf.GetSize())
      return FALSE;

  return reads_compare(device, c_buf, r_timeout);
}


/////////////////////////////////////////////////
//  Clear the input/output buffers of the      //
//  device.                                    //
/////////////////////////////////////////////////

void flush (int handle)
{
  tcflush(handle, TCOFLUSH);
  tcflush(handle, TCIFLUSH);
}


////////////////////////////////////////////////////////
//  Constructor: create DspEchoComp object            //
////////////////////////////////////////////////////////
//    _modem_device: file name of the ISDN device     //
//    _msn: ISDN MSN number                           //
////////////////////////////////////////////////////////

pots::pots (const PString& _modem_device, const PString& _msn)
{
  m_device_name = _modem_device;
  m_msn = _msn;

  // init echo compensation
  if (Config::UseEchoComp()) { 
    if (Config::UseAGC())
      m_echo_comp = new DspEchoComp(FALSE);
    else
      m_echo_comp = new DspEchoComp(TRUE);
  }
  else
    m_echo_comp = NULL;
  m_rewriteInfo = Config::GetRewriteInfo();
}


////////////////////////////////////////////////////////
//  Destructor: hangup, remove lock file, delete      //
//              DspEchoComp object                    //
////////////////////////////////////////////////////////

pots::~pots()
{
  // hangup
  if (!HangUp())
    PTRACE(1, "ISDN\tUnable to hang up");
  else
    PTRACE(1, "ISDN\tHang up OK");

  flush(m_device);
  writes(m_device, "ATZ");   // i4l defaults (del MSN)

  DeviceClose();

  PTRACE(0, "ISDN\tClosing device \"" << m_device_name << '"');
  close(m_device);
  PTRACE(0, "ISDN\tDevice \"" << m_device_name << "\" closed");
  
  // remove lock file
  if (!m_lockfile_name.IsEmpty())
    PFile::Remove(m_lockfile_name);
  
  // delete echo compensation object
  if (m_echo_comp != NULL)
    delete m_echo_comp;
}


////////////////////////////////////////////////////////
//  Initialize ISDN device and create lock file.      //
////////////////////////////////////////////////////////

PBoolean pots::Init ()
{
  m_isdn_connection = FALSE;
  m_audio_connection = FALSE;

  // create lock file
  m_lockfile_name = "/var/lock/LCK.." + m_device_name.Mid(5);
  PFile lockfile;
  if (PFile::Exists(m_lockfile_name)) {
    PTRACE(1, "ISDN\tDevice \"" << m_lockfile_name << "\" already locked");
    return FALSE;
  }
  if (!lockfile.Open(m_lockfile_name, PFile::WriteOnly)) {
    PTRACE(1, "ISDN\tCannot create lock file: " << m_lockfile_name);
    m_lockfile_name = "";
    return FALSE;
  }
  PString pid;
  pid.sprintf("%11d",PProcess::Current().GetProcessID());
  lockfile << pid;
  lockfile.Close();

  // init modem
  m_buf[0] = 0;

  PTRACE(0, "ISDN\tOpening device \"" << m_device_name << '"');
  if ((m_device = open(m_device_name, O_RDWR|O_NDELAY)) == -1) {
    PTRACE(1,"ISDN\tUnable to open device \"" << m_device_name << '"');
    return FALSE;
  }
  PTRACE(0, "ISDN\tDevice \"" << m_device_name << "\" opened");
  /* Cancel the O_NDELAY flag. */
  int n = fcntl(m_device, F_GETFL, 0);
  fcntl(m_device, F_SETFL, n & ~O_NDELAY);

  DeviceInit();

  writes(m_device, "ATZ");             // i4l defaults
  sleep(1);
  flush(m_device);

  writes(m_device, "ATE0");            // Echo off
  reads(m_device, m_buf, 50);
  strcpy(m_buf, "");
  reads(m_device, m_buf, 50);
  if (strcmp(m_buf, "OK")) {
    PTRACE(1, "ISDN\tATE0 error");
    close(m_device);
    return FALSE;
  }
  flush(m_device);

  // DTR  falling  edge: hang up, return to command mode
  //                     and reset all registers.
  if (!writes_reads_compare (m_device, "AT&D3", "OK"))
    return FALSE; 
  flush (m_device);

  // MSN
  PString  at_msn = "AT&E" + m_msn;
  if (!writes_reads_compare(m_device, at_msn, "OK")) {
    PTRACE(1, "ISDN\t" << at_msn << "error");
    close(m_device);
    return FALSE;
  } 
  flush(m_device);

  // Output buffer size
  if (!writes_reads_compare(m_device, "AT&B2048", "OK")) {
    close(m_device);
    return FALSE;
  } 
  flush(m_device);

  // Audio on
  if (!writes_reads_compare(m_device, "AT+FCLASS=8", "OK")) {
    close(m_device);
    return FALSE; 
  }
  flush(m_device);

  // Audio on, data off
  if (!writes_reads_compare(m_device, "ATS18=1", "OK")) {
    PTRACE(1, "ISDN\tATS18=1 error");
    close(m_device);
    return FALSE; 
  }
  flush(m_device);

  // Layer 2: transparent
  // bug in the man page?
  if (!writes_reads_compare(m_device, "ATS14=4", "OK")) {
    PTRACE(1, "ISDN\tATS14=4 error");
    close(m_device);
    return FALSE;
  } 
  flush(m_device);

  // aLaw
  if (!writes_reads_compare(m_device, "AT+VSM=5", "OK")) {
    PTRACE(1, "ISDN\tAT+VSM=5 error");
    close(m_device);
    return FALSE;
  } 
  flush(m_device);

  // get my MSN
  if (!writes_reads_compare(m_device, "ATS23=1", "OK")) {
    PTRACE(1, "ISDN\tATS23=1 error");
    close(m_device);
    return FALSE;
  } 
  flush(m_device);

  return TRUE;
}


////////////////////////////////////////////////////////
//  Initialize devices: DTR, flow control, speed      //
////////////////////////////////////////////////////////

void pots::DeviceInit ()
{
  struct termios tio;

  tcgetattr(m_device, &tio);

  cfsetospeed(&tio, B0);      // DTR
  cfsetispeed(&tio, B0);
  tcsetattr(m_device, TCSANOW, &tio);
  sleep(1);

  cfmakeraw(&tio);
  tio.c_cflag |= (CRTSCTS);
  tio.c_iflag &= ~(IXON|IXOFF|IXANY);
  tio.c_oflag &= ~OPOST;

  cfsetospeed(&tio, B115200); // speed
  cfsetispeed(&tio, B115200);

  tcsetattr(m_device, TCSANOW, &tio);
}


////////////////////////////////////////////////////////
//  Close device by setting speed to 0 (== DTR off).  // 
////////////////////////////////////////////////////////

void pots::DeviceClose ()
{
  struct termios tio;

  tcgetattr(m_device, &tio);
  cfsetospeed(&tio, B0);    // DTR
  cfsetispeed(&tio, B0);
  tcsetattr(m_device, TCSANOW, &tio);
}


////////////////////////////////////////////////////////
//  Set the MSNs we are responsible for.              //
////////////////////////////////////////////////////////

PBoolean pots::ListenOn (const PString& numbers)
{
  if (m_isdn_connection)
    return FALSE;

  PString buf_nr = "AT&L" + numbers;
  PTRACE(1, "ISDN\tListening for calls to "<<numbers);
  if (!writes_reads_compare(m_device, buf_nr, "OK"))
    return FALSE;    // fixme: returns never "OK"

  return TRUE;
}


////////////////////////////////////////////////////////
//  Least Cost Router: add the prefix number of the   //
//  cheapest CbC provider to a phone number.          //
////////////////////////////////////////////////////////
//    phone_numer: (in) number of the remote party    //
//                 (out) remote party + CbC prefix    //
////////////////////////////////////////////////////////

void pots::LCRouter (PString &phone_number)
{
  LCR     *lcr;
  PString  new_phone_number;
  PString  isdnrate_socket;
  PString  provider_name;

  isdnrate_socket = Config::GetLcrIsdnrateSocket();
  if (isdnrate_socket.IsEmpty())
    return;

  lcr = new LCR(isdnrate_socket, Config::GetLcrPbxPrefix());
  if (lcr->GetNumber(phone_number, new_phone_number,176)) {
    phone_number = new_phone_number;
    provider_name = lcr->GetProviderName();

    // log the name of our CbC provider
    if (!provider_name.IsEmpty()) {
      PTRACE(2, "LCR\t using " << provider_name);
	}
  }
  else {
    PTRACE(1, "LCR\tCould not open isdnrate (LCR) socket" << isdnrate_socket);
  }

  delete lcr;
}


////////////////////////////////////////////////////////
//  Make an outgoing ISDN call.                       //
////////////////////////////////////////////////////////
//    telnumber: phone number of the remote party     //
////////////////////////////////////////////////////////

int pots::ATDReply ()
{
  char buf[500];
  const PString RESPONS_VCON = "VCON";         // 1
  const PString RESPONS_RINGING = "RINGING";   // 2
  const PString RESPONS_BUSY = "BUSY";         // 3
  const PString RESPONS_NOCARRIER = "NO CARRIER"; // 4

  strcpy(buf, "");
  if (reads(m_device, buf, sizeof(buf), 1) == -1)
    return 0;

  if(RESPONS_RINGING == buf) {
    return 2;
  } else if(RESPONS_BUSY == buf) {
    return 3;
  } else if(RESPONS_NOCARRIER == buf) {
    return 4;
  } else if(RESPONS_VCON == buf) {
    m_isdn_connection = TRUE;
    flush(m_device);
    return 1;
  }
  return 0;
}


PBoolean pots::StartATD (const PString& telnumber)
{
  PString buf_nr = telnumber;
  PString dialprefix = Config::GetDialPrefix();

  m_number_caller = telnumber;
  m_number_gateway = m_msn;
  InfoConnections::WriteStatus2HTML (FALSE);

  LCRouter(buf_nr);

  // First we add a dialprefix for all numbers
  buf_nr = dialprefix + buf_nr;

  // Then we rewrite one prefix with something else, including
  // the dialprefix above.
  for (PINDEX i = 0; i < m_rewriteInfo.GetSize(); ++i) {
    PString prefix, insert;

    prefix = m_rewriteInfo.GetKeyAt(i);
    insert = m_rewriteInfo.GetDataAt(i);

    int len = prefix.GetLength();
    if (len == 0 || strncmp(prefix, buf_nr, len) == 0){
      PString result = insert + buf_nr.Mid(len);
      PTRACE(2, "RewritePString: " << buf_nr << " to " << result);
      buf_nr = result;
    }
  }

  //  Then we add ATD.
  buf_nr = "ATD" + buf_nr;

  flush(m_device);
  
  PTRACE(0, "ISDN\tDialing: " << buf_nr);
  if (!writes(m_device, buf_nr)) {
    PTRACE(1, "ISDN\tATD error");
    return FALSE;
  }
  PTRACE(0, "ISDN\tDialing done");
  
  return TRUE;
}


////////////////////////////////////////////////////////
// Answer an incoming ISDN call.                      //
////////////////////////////////////////////////////////

PBoolean pots::AnswerCall ()
{
  flush(m_device);

  if (!writes_reads_compare(m_device, "ATA", "VCON")) {
    HangUp();     // fixme
    return FALSE;
  }
  m_isdn_connection = TRUE;

  PTRACE(0,"ISDN\tAnswering call on \"" << m_device_name << '"');

  return TRUE;
}


////////////////////////////////////////////////////////
//  Wait for an incoming ISDN call. This function     //
//  blocks "timeout" seconds.                         //
////////////////////////////////////////////////////////

PBoolean pots::WaitForCall (int timeout)
{
  flush(m_device);
  PString CNPPrefix = Config::GetCPNPrefix();
  strcpy(m_buf, "");
  while (strncmp(m_buf, "RING", 4)) {
    if (reads(m_device, m_buf, 1000, timeout) < 1)
      return FALSE;
    if (!strncmp(m_buf, "CALLER NUMBER: ", 15)) {
      m_number_caller = m_buf + 15;
      m_number_caller = CNPPrefix + m_number_caller;
    }
  }
  if (m_buf[5] != '\0')
    m_number_gateway = PString(m_buf+5);

  return TRUE;
}


////////////////////////////////////////////////////////
//  Returns a pointer to the caller's phone number.   //
////////////////////////////////////////////////////////

const PString& pots::GetCallerNumber ()
{
  return m_number_caller;
}


////////////////////////////////////////////////////////
//  Returns a pointer to the called phone number      //
//  (one of our local MSNs).                          //
////////////////////////////////////////////////////////

const PString& pots::GetGatewayNumber ()
{
  return m_number_gateway;
}


////////////////////////////////////////////////////////
//  Start audio playback/recording of the ISDN        //
//  device.                                           //
////////////////////////////////////////////////////////

PBoolean pots::StartAudio ()
{
#ifdef USEPWLIB      
  if (m_mutex_startaudio.WillBlock)  // avoid dead lock
    return TRUE;
  PWaitAndSignal lmutex(m_mutex_startaudio);
#endif

  if (m_audio_connection)
    return TRUE;

  flush(m_device);

  PTRACE(3,"ISDN\tStarting audio");

  // Audio connection?
  if (!writes_reads_compare(m_device, "AT+VRX+VTX", "CONNECT")) {
    HangUp();
    return FALSE;
  }
  if (!writes_reads_compare(m_device, "", "CONNECT")) {
    HangUp();
    return FALSE;
  }
  m_audio_connection = TRUE; 
  
  return TRUE;
}


////////////////////////////////////////////////////////
//  Audio playback/recording started?                 //
////////////////////////////////////////////////////////

PBoolean pots::IsAudioOnline ()
{
  return m_audio_connection;
}

////////////////////////////////////////////////////////
// Hangup the connection.                             //
////////////////////////////////////////////////////////

PBoolean pots::HangUp ()
{
  PWaitAndSignal l1mutex(m_mutex_hangup_write);
  PWaitAndSignal l2mutex(m_mutex_hangup_read);

  if (IsAudioOnline()) {
    tcflush(m_device, TCOFLUSH);
    writes(m_device, "\x010\x014");     // send "DC4: end of audio"
    tcdrain(m_device);                  // wait for EOF to be sent
    sleep(1);
    tcflush(m_device, TCIFLUSH);        // drop result/received audio data
    m_audio_connection = FALSE;
  }

  if (m_isdn_connection) {
    tcflush(m_device, TCIOFLUSH); 
    if (!writes_reads_compare(m_device, "ATH", "OK"))
      return FALSE;              // fixme: sometimes we get NO CARRIER, why?
    m_isdn_connection = FALSE;
    m_number_caller = "";
    m_number_gateway = "";
  }

  return TRUE;
}


////////////////////////////////////////////////////////
//  This function is called whenever a DTMF signal    //
//  is received by this class.                        //
////////////////////////////////////////////////////////

void pots::OnDTMF (unsigned char ch)
{
  PWaitAndSignal mutex(m_dtmf_mutex);

  m_dtmf += ch;
}


////////////////////////////////////////////////////////
//  Return the received DTMF buttons.                 //
////////////////////////////////////////////////////////
//    buttons: received buttons                       //
////////////////////////////////////////////////////////

void pots::GetDTMF (PString &buttons)
{
  PWaitAndSignal mutex(m_dtmf_mutex);

  buttons = m_dtmf;
  m_dtmf = "";
}


////////////////////////////////////////////////////////
//  Read samples from the ISDN device.                //
////////////////////////////////////////////////////////
//    buffer: pointer to the sample buffer            //
//    len: number of samples to be read               //
////////////////////////////////////////////////////////
// -1  : error                                        //
// >=0 : number of samples read                       //
////////////////////////////////////////////////////////

int pots::AudioRead (void *buffer, int len)
{
  short *buffer_ptr = (short*)buffer;
  unsigned char isdn_buffer[len];

  PWaitAndSignal lmutex(m_mutex_hangup_read);

  if (!m_audio_connection)
    return -1; // throw ErrNotConnected();

  int read_len = len;
  int was_dle = 0;
  while (read_len) {
    int nb_read = read(m_device, isdn_buffer, read_len);
    if (nb_read <= 0)
      return -1; // throw ErrRead();

    int nb_write = 0;
    for (int i=0; i<nb_read; i++) {
      if (was_dle) {
        was_dle = 0;

        switch (isdn_buffer[i]) {
        case DLE:
          PTRACE(3,"--- DLE");
          // normal audio data
          buffer_ptr[nb_write] = ALAW2INT(0x10);
          if (m_echo_comp != NULL)
            m_echo_comp->Out(buffer_ptr[nb_write]);
          nb_write++;
          break;
        
        case ETX:
          PTRACE(3,"--- ETX");
          PTRACE(3,"ISDN\thang up detected on \"" << m_device_name << '"');
          return -1; // throw ErrHangUp();

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '#':
        case '*':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
          // touch-tone
          PTRACE(3,"--- DTMF(" << isdn_buffer[i] << ')');
          OnDTMF(isdn_buffer[i]);
          break;

        default:
          PTRACE(3,"--- ???");
          return -1; // throw ErrRead();
        }
      }
      else if (isdn_buffer[i] == DLE) {
        PTRACE(3,"DLE");
        was_dle = 1;
      }
      else {
        // normal audio data
        buffer_ptr[nb_write] = ALAW2INT(isdn_buffer[i]);
        if (m_echo_comp != NULL)
          m_echo_comp->Out(buffer_ptr[nb_write]);
        nb_write++;
      }
    }
    read_len -= nb_write;
    buffer_ptr += nb_write;
  }
  return len;
}


////////////////////////////////////////////////////////
//  Flush the recording buffer of the ISDN device.    //
////////////////////////////////////////////////////////

void pots::AudioReadFlush ()
{
  tcflush(m_device, TCIFLUSH);   
}


////////////////////////////////////////////////////////
//  Write samples to the ISDN device.                 //
////////////////////////////////////////////////////////
//    buffer: pointer to the sample buffer            //
//    len: number of samples to be written            //
//    block: should the function block some msecs?    //
////////////////////////////////////////////////////////
// -1  : error                                        //
// >=0 : number of samples written                    //
////////////////////////////////////////////////////////

int pots::AudioWrite (const void* buffer, int len, PBoolean block)
{
  const short* pnt = (const short*)buffer;
  unsigned char new_pnt[len*2];

  PWaitAndSignal lmutex(m_mutex_hangup_write);

  if (! IsAudioOnline())
    return -1; // throw ErrNotConnected();

  // convert audio from OpenH323 to ISDN4Linux
  int c=0;
  for (int i=0; i<len; i++) {
    short tmp_s = pnt[i];
    unsigned char tmp_uc;
    
    if (m_echo_comp != NULL)
      m_echo_comp->In(tmp_s);
    
    tmp_uc = INT2ALAW(tmp_s);
    if (tmp_uc == DLE)
      new_pnt[c++] = 0x10;
    
    new_pnt[c++] = tmp_uc;
  }

  m_delay.Delay(len);
  int rval = write(m_device, new_pnt, c);
  if (rval == c)
    return len;
  
  if (rval == -1) {
    PTRACE(2,"ISDN\tWrite error: " << strerror(errno));
  }

  return -1; // throw ErrWrite();
}


PBoolean pots::IsFree(const PString& device)
{
	PString lockfile = "/var/lock/LCK.." + device.Mid(5);
	if (PFile::Exists(lockfile)) {
		PFile file;
		if (file.Open(lockfile, PFile::ReadOnly)) {
			pid_t pid;
			file >> pid;
			file.Close();
			int result = kill(pid, 0);		// check if process exists (no signal sent)
			if((result < 0) && (errno == ESRCH)) {
				PTRACE(0, "ISDN\tRemoving stale lock file for device " << device);
				PFile::Remove(lockfile);
				return TRUE;	// free
			}
			return FALSE;	// not free
		}
	}
	return TRUE;	// free
}

MakeCallThread::MakeCallThread(GwH323Connection& connection, const PString& phone_number)
  : PThread(10000),
    m_connection(connection),
    m_phone_number(phone_number),
    m_end(FALSE)
{
  Resume();
  //Main();
}

void MakeCallThread::Main()
{
  int rval = TTY_TIMEOUT;
  int timeout;
  PBoolean breakwhile = FALSE;

  pots* pots_class = InfoConnections::PotsGet(m_connection.GetCallToken());

  pots_class->StartATD(m_phone_number);

  for (timeout = Config::GetATDTimeout(); !breakwhile && timeout && !m_end ; timeout--) {
    switch (pots_class->ATDReply()) {
    case 0:
      PTRACE(1,"ISDN\tWaiting for answer...");
      break;
    case 1:
      PTRACE(0,"ISDN\tRemote party answered");
      rval = TTY_CONNECT;
      breakwhile = TRUE;
      break;
    case 2:
      PTRACE(1,"ISDN\tAlert from remote party");
      m_connection.AnsweringCall(H323Connection::AnswerCallPending);
      break;
    case 3:
      PTRACE(0,"ISDN\tBUSY from other end");
      breakwhile = TRUE;
      rval = TTY_BUSY;
      break;
    case 4:
      PTRACE(0,"ISDN\tNO CARRIER from other end");
      breakwhile = TRUE;
      rval = TTY_NOCARRIER;
      break;
    default:
      break;
    } // end switch
  } // end for

  if(!m_end)
    m_connection.MakeCallValue(rval);
}

void MakeCallThread::End()
{
  m_end = TRUE;
}
