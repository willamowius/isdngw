/*
  $Id: connection.cxx,v 1.3 2009/05/22 22:50:33 jan Exp $
  
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

#include <ptlib.h>
#include <h225.h>
#include <h323.h>
#include <h323pdu.h>

#include "pots.hxx"
#include "resource.hxx"
#include "routing.hxx"
#include "endpoint.hxx"
#include "potschannel.hxx"
#include "loopchannel.hxx"
#include "nullchannel.hxx"
#include "connection.hxx"
#include "config.hxx"


//////////////////////////////////////////////////////////
//  Thread safe implementation of the gethostbyname()   //
//  function. Parts of this function are "stolen" from  //
//  the glibc info page "(libc)Host Names". This        //
//  implementation works only with the glibc            //
//////////////////////////////////////////////////////////
//    host_name: host name to be resolved               //
//    host_ip: the resolved IP                          //
//    ip_size: size of the host_ip buffer               //
//////////////////////////////////////////////////////////

static PBoolean GetHostByName (const PString& host_name, char *host_ip, int ip_size) 
{
  struct hostent hostbuf, *hp;
  size_t hstbuflen;
  char *tmphstbuf;
  int res;
  int herr;

  hstbuflen = 1024;
  tmphstbuf = (char*) malloc(hstbuflen);

  while ((res = gethostbyname_r(host_name, &hostbuf, tmphstbuf, hstbuflen, 
                                &hp, &herr)) == ERANGE) {
    // enlarge the buffer
    hstbuflen *= 2;
    tmphstbuf = (char*) realloc(tmphstbuf, hstbuflen);
  }
  
  // check for errors
  if (res)
    return FALSE;

  // convert number to string
  inet_ntop(AF_INET, hostbuf.h_addr, host_ip, ip_size);

  free(tmphstbuf);

  return TRUE;
}


//////////////////////////////////////////////////////////
//  Constructor:                                        //
//////////////////////////////////////////////////////////

GwH323Connection::GwH323Connection (GwH323EndPoint &endpoint, unsigned callReference) 
  : H323Connection(endpoint, callReference),
    m_channel(NULL),
    m_potschannel(NULL),
    m_makecallthread(NULL)
{
}


//////////////////////////////////////////////////////////
//  Destructor: Deletes the PIndirectChannel and        //
//              pots objects. Frees the reserved line.  //
//////////////////////////////////////////////////////////

GwH323Connection::~GwH323Connection ()
{
  if (m_makecallthread)
    m_makecallthread->End();
  
  if (m_channel)
    delete m_channel;

  pots* pots_class = InfoConnections::PotsGet(callToken);
  if (pots_class != NULL)
    delete pots_class;

  InfoConnections::LineFree(callToken);
}


//////////////////////////////////////////////////////////
// Is the remote user (IP) allowed to use the gateway?  //
//////////////////////////////////////////////////////////

PBoolean GwH323Connection::AllowedToUseGateway ()
{
  char *pnt;
  char remote_addr[200];
  char remote_addr_ip[16];
  PBoolean allowed = FALSE;
  PString pstring;
  PStringArray allowed_ips;

  // get the address of the remote endpoint
  pstring = signallingChannel->GetRemoteAddress();
  strcpy(remote_addr, (const char*)pstring.Mid(3));
  pnt = strchr(remote_addr, ':');
  if (pnt != NULL)
    remote_addr [pnt - remote_addr] = '\0';

  // get configuration
  allowed_ips = Config::GetAllowedIPs();

  // convert the symbolic name to the ip address
  // e.g. ovid.telos.de -> 192.168.1.1
  if (GetHostByName(remote_addr, remote_addr_ip, sizeof(remote_addr_ip))) {
    // compare endpoint's address with the allowed IPs
    for (PINDEX i=0; i<allowed_ips.GetSize(); i++)
      if (strncmp(remote_addr_ip, allowed_ips[i], allowed_ips[i].GetLength()) == 0) {
          allowed = TRUE;
          break;
      }
  }
  else  // host is unknown
    strcpy(remote_addr_ip, "unknown");
  
  PTRACE(2, "AllowedToUseGateway() - " << remote_addr << '(' << remote_addr_ip << ") - " << (allowed?"TRUE":"FALSE"));

  // log denied outgoing calls
  if (!allowed) {
    PTRACE(0, "Outgoing call from " << remote_addr << " (" << remote_addr_ip << "): denied");
  }

  return allowed;
}


//////////////////////////////////////////////////////////
//  H.323 -> ISDN                                       //
//////////////////////////////////////////////////////////

H323Connection::AnswerCallResponse
GwH323Connection::OnAnswerCall (const PString &, const H323SignalPDU &setupPDU, H323SignalPDU &)
{
  PString  msn;
  PString  phone_number;

  // verify that caller's host is allowed to make outgoing calls
  if (!AllowedToUseGateway())
    return AnswerCallDenied;
  
  // get the phone number
  const H225_Setup_UUIE & setup = setupPDU.m_h323_uu_pdu.m_h323_message_body;
  const H225_ArrayOf_AliasAddress &dest_adr = setup.m_destinationAddress;
  PINDEX i;
  for (i=0; i<dest_adr.GetSize(); i++)
    if (dest_adr[i].GetTag() == H225_AliasAddress::e_dialedDigits) {
      phone_number = H323GetAliasAddressString(dest_adr[i]);
      break;
    }
  
  // if we couldn't get the phone number, deny the call
  if (i == dest_adr.GetSize()) {
    PTRACE(0, "Unable to find the destination number");
    return AnswerCallDenied;
  }
  
  // get an ISDN line
  if (!InfoConnections::LineGet_Token(callToken))
    return AnswerCallDenied;
  
  // log the connection
  PTRACE(2, "H.323 -> ISDN using " << InfoConnections::GetDevice(callToken));
  
  // get the ISDN MSN associated with the caller
  msn = Config::GetDefaultMSN();
  const H225_ArrayOf_AliasAddress &src_adr = setup.m_sourceAddress;
  for (i=0; i<src_adr.GetSize(); i++)
    if (src_adr[i].GetTag() == H225_AliasAddress::e_h323_ID) {
      PString user = H323GetAliasAddressString(src_adr[i]);
      msn = Routing::GetNumberByUser(user);
      if (!msn.IsEmpty()) {
        PTRACE(0, '"' << user << "\"'s MSN found: " << msn);
        break;
      } else {
        PTRACE(0, "Cannot find the MSN of \"" << user << '"');
      }
    }
  
  // if we couldn't get an h323-ID, deny the call
/*  if (i == src_adr.GetSize()) {
    PTRACE(1, "unable to find the h323-ID of the source");
    return AnswerCallDenied;
  }*/

  // get an ISDN line to make this call
  pots* pots_class = new pots(InfoConnections::GetDevice(callToken), msn);
  if (!pots_class->Init()) {
    PTRACE(0, "Cannot initialize device: " << InfoConnections::GetDevice(callToken));
    InfoConnections::DirectionSet(callToken, InfoConnections::Error);
    return AnswerCallDenied;
  }
  if(!InfoConnections::PotsSet(callToken, pots_class)) {
    PTRACE(0,"GwH323Connection::OnAnswerCall(): Could not PotsSet "<<callToken<<".");
  }
  InfoConnections::DirectionSet(callToken, InfoConnections::ToPots);

  // create the channels
  m_channel = new PIndirectChannel;

  /*

  // if no there is no connection message, use the nullchannel
  const PString& message = Config::GetConnectingMessage();
  if (!message.IsEmpty() && PFile::Exists(message)) {
    LoopChannel* loopchannel = new LoopChannel(message);
    m_channel->Open(loopchannel);
  }
  else {
    NullChannel* nullchannel = new NullChannel;
    m_channel->Open(nullchannel);
  }
  */

  InfoConnections::SetChannel(callToken,m_channel);

  m_makecallthread = new MakeCallThread(*this, phone_number);
  
  return AnswerCallPending;
}


//////////////////////////////////////////////////////////
//  ISDN -> H.323                                       //
//////////////////////////////////////////////////////////

PBoolean GwH323Connection::OnOutgoingCall (const H323SignalPDU & connectPDU)
{ 
  PTRACE(2, "ISDN -> H.323 using " << InfoConnections::GetDevice(callToken));

  // get the pots object and answer the incoming ISDN call
  pots* pots_class = InfoConnections::PotsGet(callToken);
  if (!pots_class->AnswerCall())
    return FALSE;

  pots_class->StartAudio();

  // create the channels
  m_channel = new PIndirectChannel();
  m_potschannel = new PotsChannel(pots_class, this, callToken);
  m_channel->Open(m_potschannel);

  InfoConnections::SetChannel(callToken, m_channel);

  return TRUE;
}


//////////////////////////////////////////////////////////
//  When a connection receives user input, this         //
//  functions is called. The function sends the user    //
//  input as ISDN DTMF.                                 //
//////////////////////////////////////////////////////////

void GwH323Connection::OnUserInputString (const PString &value)
{
  // send UserInput as ISDN DTMF
  if (m_potschannel)
    for (PINDEX i=0; i < value.GetSize()-1; i++)
      m_potschannel->DTMFEncode(value[i]);
}


//////////////////////////////////////////////////////////
// This member is called by the pots class to indicate  //
// if the new connection was established or not.        //
//////////////////////////////////////////////////////////

void GwH323Connection::MakeCallValue(int success)
{
  m_makecallthread = NULL;

  switch (success) {
  case TTY_CONNECT: {
    PString token = GetCallToken();

    pots* pots_class = InfoConnections::PotsGet(token);
    if (!pots_class) {
      PTRACE(0,"No pots class found");
      ClearCall(EndedByCallerAbort);      
    }

    pots_class->StartAudio();

    m_potschannel = new PotsChannel(pots_class, this, token);
    m_channel->Open(m_potschannel);
    InfoConnections::SetChannel(callToken, m_channel);
    // Telling other endpoint that we should establish the call
    AnsweringCall(AnswerCallNow);
    break;
  }
  case TTY_BUSY: {
    ClearCall(EndedByRemoteBusy);
    break;
  }
  case TTY_TIMEOUT: {
    ClearCall(EndedByNoAnswer);
    break;
  }
  case TTY_NOCARRIER:
  default: {
    ClearCall(EndedByCallerAbort);
    break;
  }
  } // end switch
}


//////////////////////////////////////////////////////////
// Changes the alias of the gateway given to the called //
// H.323 enpoint.                                       //
//////////////////////////////////////////////////////////

PBoolean GwH323Connection::OnSendSignalSetup (H323SignalPDU &setup_pdu) 
{
  pots* my_pots = InfoConnections::PotsGet(GetCallToken());
  if (my_pots) {
    PString calling_number = my_pots->GetCallerNumber();
    if (!calling_number.IsEmpty() && calling_number != "0")
      SetLocalPartyName(calling_number);
    else
      SetLocalPartyName("Unknown phone number");

    return TRUE;
  }

  return FALSE;
}
