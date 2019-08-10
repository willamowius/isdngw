/*
  $Id: endpoint.hxx,v 1.2 2009/05/22 22:50:33 jan Exp $
  
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

#ifndef _ENDPOINT_HXX_
#define _ENDPOINT_HXX_

#include <ptlib.h>
#include <h323.h>
#include "resource.hxx"

class GwH323EndPoint : public H323EndPoint
{
  PCLASSINFO(GwH323EndPoint, H323EndPoint);

public:
  GwH323EndPoint ();

  PBoolean Init ();
  
  PBoolean Call (PString &token);
  
  H323Connection *CreateConnection (unsigned callReference);
  void OnConnectionEstablished (H323Connection &connection,
                                const PString &token);
  void OnConnectionCleared (H323Connection &connection,
                            const PString &token);
  PBoolean OpenAudioChannel (H323Connection &connection,
                         PBoolean isEncoding, unsigned bufferSize,
                         H323AudioCodec &codec);
  void SetEndpointTypeInfo (H225_EndpointType & info) const;


};

#endif /* _ENDPOINT_HXX_ */
