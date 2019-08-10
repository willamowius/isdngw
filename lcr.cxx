/*
  $Id: lcr.cxx,v 1.2 2009/05/22 22:50:33 jan Exp $
  
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

#include <unistd.h>   
#include <sys/types.h>
#include <sys/socket.h> 

#include "lcr.hxx"


//////////////////////////////////////////////////////////    
//  Open the isdnrate socket.                           //
//////////////////////////////////////////////////////////
//    socket_path: path of the isdnrate socket          //
//                 (e.g. /tmp/isdnrate)                 //
//    _pbx: phone prefix to get an external line        //      
//////////////////////////////////////////////////////////    

LCR::LCR (const PString& socket_path, const PString& _pbx)
{
  m_isdnrate_ok = FALSE;
  m_pbx = _pbx;

  // establish a connection to the isdnrate daemon
  if ((m_sock = socket (PF_UNIX, SOCK_STREAM, 0)) < 0)
    return;

  m_sock_addr.sa_family = AF_UNIX;
  strcpy(m_sock_addr.sa_data, socket_path);

  if (connect(m_sock, &m_sock_addr,
              strlen(m_sock_addr.sa_data)+sizeof(m_sock_addr.sa_family)) < 0) {
    close (m_sock);
    return;
  }

  m_isdnrate_ok = TRUE;
}


//////////////////////////////////////////////////////////    
//  Close the isdnrate socket.                          //
//////////////////////////////////////////////////////////

LCR::~LCR ()
{ 
  if (m_isdnrate_ok)
    close (m_sock);
}


//////////////////////////////////////////////////////////    
//  Add the call by call prefix for the cheapest        //
//  provider to a given phone number.                   //
//////////////////////////////////////////////////////////    
//    number_in: telephone number of the remote party   //
//    number_out number_in + CbC prefix                 //
//    length_call: length of the call (used for the     //
//                 calculation                          //
//////////////////////////////////////////////////////////    

PBoolean LCR::GetNumber (const PString& number_in, PString& number_out,
                     const PString& length_call)
{
  PString command;              // isdnrate options
  PString int_pbx;              // PBX prefix
  char int_provider[500];       // CbC provider prefix
  PString  int_party;           // call party
  PBoolean use_isdnrate;            // external call (TRUE)
  char *pnt;

  if (!m_isdnrate_ok)
    return FALSE;

  m_provider_info = "";

  // split number
  if (!m_pbx.IsEmpty()) {
    // PBX available?
    if (strncmp (number_in, m_pbx, m_pbx.GetLength()) == 0) {
      // external call (e.g. "0/4912123...")
      int_pbx = m_pbx;
      int_party = number_in.Mid(m_pbx.GetLength(), 
                                number_in.GetLength()-m_pbx.GetLength());
      use_isdnrate = TRUE;
    }
    else {
      // internal call (e.g. "46...")
      int_pbx = "";
      int_party = number_in;
      use_isdnrate = FALSE;
    }    
  }
  else {
    int_pbx = ""; 
    int_party = number_in;
    use_isdnrate = TRUE;
  }


  if (use_isdnrate) {
    // external call
    // send options to the isdnrate daemon
    command = "-l" + length_call + " -b1 -L " + int_party;
    if (write (m_sock, (const char*)command, command.GetLength() + 1) < 0)
      return FALSE;

    // receive the best provider
    if (read (m_sock, int_provider, sizeof (int_provider)) <= 0)
      return FALSE;
 
    // extract the provider prefix
    int_provider[sizeof (int_provider) - 1] = '\0';
    pnt = strpbrk (int_provider, ";");
    if (pnt == NULL)
      return FALSE;
    *pnt = '\0';

    // get the name of the provider
    m_provider_info = (pnt+1);
    m_provider_info.Delete(m_provider_info.Find(";"),1);

    // whole number
    number_out = int_pbx + int_provider + int_party;
  }
  else
    // internal call
    number_out = int_party;
  
  PSYSTEMLOG(Info, "LCR::GetPrefix (" << number_in << ") - " << number_out << ";" << m_provider_info);

  return TRUE;
}


//////////////////////////////////////////////////////////    
//  Return the name of the cheapest provider. Before    //
//  using this function you have to use ::GetNumber().  //
//////////////////////////////////////////////////////////    

const PString LCR::GetProviderName ()
{
  return m_provider_info;
}
