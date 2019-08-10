/*
  $Id: connection.hxx,v 1.2 2009/05/22 22:50:33 jan Exp $
  
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

#ifndef _CONNECTION_HXX_
#define _CONNECTION_HXX_
                       
#include <ptlib.h>
#include <h323.h>
#include "resource.hxx"

class GwH323EndPoint;  
class PotsChannel; 
class pots;
class MakeCallThread;

class GwH323Connection : public H323Connection
{
  PCLASSINFO(GwH323Connection, H323Connection);

public:
  GwH323Connection (GwH323EndPoint &endpoint, unsigned callReference);
  ~GwH323Connection ();
  
  AnswerCallResponse OnAnswerCall (const PString &, const H323SignalPDU &, H323SignalPDU &);
  PBoolean OnOutgoingCall (const H323SignalPDU &connectPDU);
  void OnUserInputString (const PString &value);

  void MakeCallValue(int);

  PBoolean OnSendSignalSetup(H323SignalPDU &my_pdu);
  
private:
  PBoolean AllowedToUseGateway();

private:
  PIndirectChannel *    m_channel;
  PotsChannel *         m_potschannel;

  MakeCallThread *      m_makecallthread;
};

#endif /* _CONNECTION_HXX_ */
