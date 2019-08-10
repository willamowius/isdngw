/*
  $Id: endpoint.cxx,v 1.3 2009/05/22 22:50:33 jan Exp $
  
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
#include <ptlib/svcproc.h>

#include <h225.h>
#include <h323.h>
//#include <gsmcodec.h>

#include "config.hxx"
#include "pots.hxx"
#include "resource.hxx"
#include "connection.hxx"
#include "endpoint.hxx"
#include "routing.hxx"


GwH323EndPoint::GwH323EndPoint ()
{
  terminalType = e_GatewayOnly;
  t35CountryCode = 61; // country code for France
}


H323Connection *GwH323EndPoint::CreateConnection (unsigned callReference)
{
  return new GwH323Connection(*this, callReference);
}    


void GwH323EndPoint::OnConnectionEstablished (H323Connection &connection,
                                              const PString &token)
{
  PTRACE(2, "connection established: " << token);
}


void GwH323EndPoint::OnConnectionCleared (H323Connection &connection,
                                          const PString &token)
{
  PTRACE(2, "connection cleared: " << token);

  pots* p = InfoConnections::PotsGet(token);

  if (p != NULL)
    p->HangUp();
  else
    PTRACE(2, "cannot find pots");
}


PBoolean GwH323EndPoint::OpenAudioChannel (H323Connection &connection,
                                       PBoolean isEncoding,
                                       unsigned bufferSize,
                                       H323AudioCodec &codec)
{
  // Disable the silence detection.
  // Attach a GwPIndirectChannel to the H323AudioCodec object.
  codec.SetSilenceDetectionMode(H323AudioCodec::NoSilenceDetection);

  PChannel* channel = NULL;

  channel = InfoConnections::GetChannel(connection.GetCallToken());

  if (channel)
    return codec.AttachChannel(channel, FALSE);
  else {
    PTRACE(0, "No channel to attach to the codec");
    return FALSE;
  }
}


////////////////////////////////////////////////////////////////////  
//  Initialize the H.323 endpoint: available codecs, the          //
//  gateway's H.323 user name, register with a gatekeeper.        //
////////////////////////////////////////////////////////////////////  

PBoolean GwH323EndPoint::Init ()
{ 
  // Should we use the UserInput capability (DTMF)
  if (Config::UseUserInput())
    H323_UserInputCapability::AddAllCapabilities(capabilities, 0, 1);

  AddAllCapabilities(0, 0, "*");

  // set our user name
  SetLocalUserName(Config::GetH323Name());

  H323ListenerTCP* listener = new H323ListenerTCP(*this, Config::GetListenerInterface(), Config::GetListenerPort());
  if (!StartListener(listener)) {
    PTRACE(0, "Could not start H.323 listener on " << Config::GetListenerInterface() << ':' << Config::GetListenerPort());
    return FALSE;
  }

  if (Config::NeedGatekeeper()) {
    PTRACE(2, "A gatekeeper must be found");
    if (Config::GetGatekeeper().IsEmpty()) {
      PTRACE(2, "No gatekeeper given in the configuration => discovery");
      if (DiscoverGatekeeper(new H323TransportUDP(*this))) {
        PTRACE(2, "Gatekeeper found, registration OK");
        return TRUE;
      }
    }
    else
      PTRACE(2, "Try to register with " << Config::GetGatekeeper());
      if(SetGatekeeper(Config::GetGatekeeper(),new H323TransportUDP(*this, Config::GetRASInterface()))) {
        PTRACE(2, "Registration OK");
        return TRUE;
      }
    PTRACE(0, "Could not register with a gatekeeper");
    PSYSTEMLOG(Error, "Could not register with a gatekeeper...");
    return FALSE;
  }

  return TRUE;
}


////////////////////////////////////////////////////////////////////  
//  Start an outgoing H.323 call.                                 //
////////////////////////////////////////////////////////////////////  
//    token_old: Identifier created by the main object.           //
//               This identifier is used by the InfoConnections   //
//               object.                                          //
////////////////////////////////////////////////////////////////////  

PBoolean GwH323EndPoint::Call (PString &token_old)
{
  PString token_new;
  PString user;

  pots *pots_class = InfoConnections::PotsGet(token_old);
  if (pots_class == NULL) {
    PTRACE(1, "Unknown call token: \"" << token_old << '"');
    return FALSE;
  }
  InfoConnections::DirectionSet (token_old, InfoConnections::FromPots);

  // get H.323 routing informations (user, gateway)
  user = Routing::GetH323UserByNumber(pots_class->GetGatewayNumber());
  if (user.IsEmpty()) {
    return FALSE;
  }

  MakeCall(user, token_new);

  InfoConnections::ChangeToken(token_old, token_new);

  return TRUE;
}


////////////////////////////////////////////////////////////////////  
//  Register the gateway's prefixes with the gatekeeper.          //
////////////////////////////////////////////////////////////////////  

void GwH323EndPoint::SetEndpointTypeInfo (H225_EndpointType &info) const
{
  H323EndPoint::SetEndpointTypeInfo(info);

  const PStringArray& isdn_prefix = Config::GetISDNPrefix();

  if (isdn_prefix.GetSize() < 1)
    return;

  H225_AliasAddress             alias;
  H225_SupportedPrefix          prefix;
  H225_ArrayOf_SupportedPrefix  a_prefix;

  a_prefix.SetSize(isdn_prefix.GetSize());

  for (PINDEX i=0; i < isdn_prefix.GetSize(); i++) {
    alias.SetTag(H225_AliasAddress::e_dialedDigits);
    (PASN_IA5String &) alias = isdn_prefix[i];
    prefix.m_prefix = alias;
    a_prefix[i] = prefix;
  }

  H225_VoiceCaps voicecaps;
  voicecaps.SetTag(H225_VoiceCaps::e_supportedPrefixes);
  voicecaps.m_supportedPrefixes = a_prefix;
  
  H225_SupportedProtocols protocols;
  protocols.SetTag(H225_SupportedProtocols::e_voice);
  (H225_VoiceCaps&)protocols = voicecaps; 

  H225_ArrayOf_SupportedProtocols a_protocols;
  a_protocols.SetSize (1);
  a_protocols[0] = protocols;

  H225_GatewayInfo gateway;
  gateway.IncludeOptionalField(H225_GatewayInfo::e_protocol);
  gateway.m_protocol = a_protocols; 

  info.m_gateway = gateway;
}
