/*
  $Id: resource.cxx,v 1.3 2009/05/22 22:50:33 jan Exp $
  
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

#include "pots.hxx"
#include "resource.hxx"
#include "config.hxx"

InfoConnections::InfoArray      InfoConnections::m_info;
PINDEX                          InfoConnections::m_nr_lines;
PMutex                          InfoConnections::m_mutex;

//////////////////////////////////////////////////////////
//  Get the file names of the available ISDN devices    //
//  and initialize the resource structure.              //
//  The Config class must be initialized before         //
//  calling this member.                                //
//////////////////////////////////////////////////////////

void InfoConnections::Init ()
{
  const PStringArray& devices = Config::GetDevices();
  
  m_info.SetSize(m_nr_lines = devices.GetSize());
  for (PINDEX i=0; i<devices.GetSize(); i++) {
    Info* info = new Info;
    info->device = devices[i];
    info->token = new PString();
    info->used = FALSE;
    info->pots_class = NULL;
    info->channel = NULL;
    info->direction = Unknown;
    m_info.SetAt(i,info);
  }
}


//////////////////////////////////////////////////////////
// Destructor: free all used objects                    //
//////////////////////////////////////////////////////////

void InfoConnections::Clear ()
{
  PWaitAndSignal lmutex(m_mutex);

  WriteStatus2HTML(TRUE);
}


//////////////////////////////////////////////////////////
//  Get a free line.                                    //
//////////////////////////////////////////////////////////
//    token: (in) an identifier                         //
//////////////////////////////////////////////////////////

PBoolean InfoConnections::LineGet_Token (const PString &token)
{
  PWaitAndSignal lmutex(m_mutex);
  
  for (PINDEX i=0; i<m_nr_lines; i++)
    if ((!m_info[i].used) && (pots::IsFree(m_info[i].device))) {
      m_info[i].used = TRUE;
      *(m_info[i].token) = token;
      m_info[i].direction = Unknown;

      WriteStatus2HTML(FALSE);

      return TRUE;
    }

  PTRACE(0, "No free line");
  return FALSE;
}


//////////////////////////////////////////////////////////
//  Get a free line.                                    //
//////////////////////////////////////////////////////////
//    token: (out) an identifier (created by this       //
//           function)                                  //
//////////////////////////////////////////////////////////

PBoolean InfoConnections::LineGet_Temp (PString &temp_token)
{
  PWaitAndSignal lmutex(m_mutex);

  for (PINDEX i=0; i<m_nr_lines; i++)
    if ((!m_info[i].used) && (pots::IsFree(m_info[i].device))) {
      m_info[i].used = TRUE;
      *(m_info[i].token) = m_info[i].device;
      temp_token = m_info[i].device;
      m_info[i].direction = Unknown;

      WriteStatus2HTML (FALSE);

      return TRUE;
    }
  
  return FALSE;
}


//////////////////////////////////////////////////////////
//  Free a reserved line.                               //
//////////////////////////////////////////////////////////
//    token: identifier of the line                     //
//////////////////////////////////////////////////////////

PBoolean InfoConnections::LineFree (const PString &token)
{
  PWaitAndSignal lmutex(m_mutex);

  for (PINDEX i=0; i<m_nr_lines; i++)
    if (*(m_info[i].token) == token) {
      *(m_info[i].token) = "";
      m_info[i].used = FALSE;
      m_info[i].pots_class = NULL;
      m_info[i].channel = NULL;

      if (m_info[i].direction != Error)
        m_info[i].direction = Unknown;

      WriteStatus2HTML(FALSE);

      return TRUE;
    }

  return FALSE;
}


//////////////////////////////////////////////////////////
//   Change the token of a reserved line.               //
//////////////////////////////////////////////////////////
//    token_old: old token                              //
//    token_new: new token                              //
//////////////////////////////////////////////////////////

void InfoConnections::ChangeToken (const PString &token_old, 
                                   const PString &token_new)
{
  PWaitAndSignal lmutex(m_mutex);
  
  for (PINDEX i=0; i<m_nr_lines; i++)
    if (*(m_info[i].token) == token_old)
      *(m_info[i].token) = token_new;
}


//////////////////////////////////////////////////////////
//  Write the status of the lines to a HTML file.       //
//////////////////////////////////////////////////////////
//    down: gateway is down (TRUE)                      //
//////////////////////////////////////////////////////////

PBoolean InfoConnections::WriteStatus2HTML (PBoolean down)
{
  pots * my_pots;
  const PString& filename(Config::GetStatusFilename());
  if (filename.IsEmpty())
    return FALSE;
  
  PFile out(filename, PFile::WriteOnly);

  out << "<HTML>\n<HEAD>\n<TITLE>Status of the ISDN - "
      << "H.323 gateway</TITLE>\n"
      << "<META HTTP-EQUIV=\"expires\" CONTENT=\"30\">\n"
      << "</HEAD>\n<BODY BGCOLOR=#FFFFFF>\n<CENTER>\n"
      << "<H1>Status of the ISDN - H.323 gateway</H1>\n";

  if (down)
    out << "<BLINK>The gateway is down.</BLINK><BR>\n";
  else {
    out << "\n<TABLE BORDER=0 CELLSPACING=0 "
        << "CELLPADDING=10 BGCOLOR=#DDDDDD WIDTH=90%%>\n"
        << "<TR BGCOLOR=#CCCCCC><TH>Device<TH>Status\n";

    for (PINDEX i=0; i<m_nr_lines; i++) {
      out << "<TR><TD WIDTH=30%%><TT>"
          << m_info[i].device
          << "</TT><TD WIDTH=60%%>";
               
      switch (m_info[i].direction) {
      case Unknown :
        if (m_info[i].used)
          out << "Waiting for incoming calls.\n";
        else
          out << "Free\n";
        break;
        
      case ToPots:
	my_pots = m_info[i].pots_class;
        out << "H.323 -&gt; ISDN <br>\n";
	out << my_pots->GetGatewayNumber();
	out << " -&gt; ";
	out << my_pots->GetCallerNumber();
	out << "<br>\n(" <<* (m_info[i].token) << ")";
        break;

      case FromPots:
	my_pots = m_info[i].pots_class;
        out << "ISDN -&gt; H.323 <br>\n";
	out << my_pots->GetCallerNumber();
	out << " -&gt; ";
	out << my_pots->GetGatewayNumber();
	out << "<br>\n(" <<* (m_info[i].token) << ")";
        break;

      case Error:
        out << "<B><BLINK>Error</BLINK></B>\n";
        break;
      }
    }

    out << "</TABLE>\n"
        << Config::GetHTML()
        << "<BR>\n";
  }

  const PString& mail = Config::GetAdmin();
  if (!mail.IsEmpty())
    out << "The administrator of this gateway is <A HREF=\"mailto:" 
        << mail << "\">" << mail << "</A>\n";

  out << "</CENTER>\n<BR><FONT FACE=\"Helvetica,Arial\" SIZE=-1>\n<B>isdngw</B> by <A HREF=\"http://www.virtual-net.fr/\">Virtual Net</A><BR>\n";
  out << "Based on <B>isdn2h323</B> by <A HREF=\"http://www.telos.de/\">telos Systementwicklung GmbH</A><BR>\n";
  out << "This program uses the <B>OpenH323</B> library by <A HREF=\"http://www.openh323.org/\">Equivalence Pty. Ltd.</A>\n";
  out << "</FONT>\n</BODY>\n</HTML>\n";

  return TRUE;
}


//////////////////////////////////////////////////////////
//  Are all lines available?                            //
//////////////////////////////////////////////////////////
//  yes (TRUE)                                          //
//////////////////////////////////////////////////////////

PBoolean InfoConnections::NoLinesUsed ()
{
  PWaitAndSignal lmutex(m_mutex);

  for (PINDEX i=0; i<m_nr_lines; i++)
    if ((m_info[i].used) && (*(m_info[i].token) != m_info[i].device))
      return FALSE;

  return TRUE;
}


//////////////////////////////////////////////////////////
//  Get the name of a device.                           //
//////////////////////////////////////////////////////////
//    token: identifier of the line                     //
//////////////////////////////////////////////////////////

PString InfoConnections::GetDevice (const PString &token)
{
  PWaitAndSignal lmutex(m_mutex);

  for (PINDEX i=0; i<m_nr_lines; i++)
    if (m_info[i].used && (*(m_info[i].token) == token))
      return m_info[i].device;

  return ""; // throw NotFound("token not found");
}


//////////////////////////////////////////////////////////
//  Set the pointer to the pots object.                 //
//////////////////////////////////////////////////////////
//    token: identifier of the line                     //
//    _pots_class: pointer to the pots object           //
//////////////////////////////////////////////////////////

PBoolean InfoConnections::PotsSet (const PString &token, 
                               pots *_pots_class)
{
  PWaitAndSignal lmutex(m_mutex);

  for (PINDEX i=0; i<m_nr_lines; i++)
    if ((m_info[i].used) && (*(m_info[i].token) == token)){
      m_info[i].pots_class = _pots_class;
      return TRUE;
    }

  return FALSE;
}


//////////////////////////////////////////////////////////
//  Get the pointer to the pots object.                 //
//////////////////////////////////////////////////////////
//    token: identifier of the line                     //
//////////////////////////////////////////////////////////

pots *InfoConnections::PotsGet (const PString &token)
{
  PWaitAndSignal lmutex(m_mutex);

  for (PINDEX i=0; i<m_nr_lines; i++)
    if (m_info[i].used && (*(m_info[i].token) == token))
      return m_info[i].pots_class;
     
  return NULL;
}


//////////////////////////////////////////////////////////
//  Set the pointer to the PChannel object.             //
//////////////////////////////////////////////////////////
//    token: identifier of the line                     //
//    _inchannel: pointer to the PChannel               //
//                object                                //
//////////////////////////////////////////////////////////

PBoolean InfoConnections::SetChannel (const PString &token, 
                                  PIndirectChannel *_channel)
{
  PWaitAndSignal lmutex(m_mutex);
  
  for (PINDEX i=0; i<m_nr_lines; i++)
    if ((m_info[i].used) && (*(m_info[i].token) == token)) {
      m_info[i].channel = _channel;
      return TRUE;
    }
      
  return FALSE;
}


//////////////////////////////////////////////////////////
//  Get the pointer to the PChannel object.             //
//////////////////////////////////////////////////////////
//    token: identifier of the line                     //
//////////////////////////////////////////////////////////

PIndirectChannel *InfoConnections::GetChannel (const PString &token)
{
  PWaitAndSignal lmutex(m_mutex);

  for (PINDEX i=0; i<m_nr_lines; i++)
    if (*(m_info[i].token) == token)
      return m_info[i].channel;
      
  return NULL;
}

//////////////////////////////////////////////////////////
//  Set the direction of the line.                      //
//////////////////////////////////////////////////////////
//    token: identifier of the line                     //
//    dir: direction (ToPots, FromPots, Unknown, Error) //
//////////////////////////////////////////////////////////

PBoolean InfoConnections::DirectionSet (const PString &token, const t_direction dir)
{
  PWaitAndSignal lmutex(m_mutex);

  for (PINDEX i=0; i<m_nr_lines; i++)
    if ((m_info[i].used) &&(*(m_info[i].token) == token)) {
      m_info[i].direction = dir;
      WriteStatus2HTML (FALSE);
      return TRUE;
    }
      
  return FALSE;
}


//////////////////////////////////////////////////////////
//  Get the direction of the line.                      //
//////////////////////////////////////////////////////////
//    token: identifier of the line                     //
//////////////////////////////////////////////////////////

InfoConnections::t_direction InfoConnections::DirectionGet (const PString &token)
{
  PWaitAndSignal lmutex(m_mutex);

  for (PINDEX i=0; i<m_nr_lines; i++)
    if (*(m_info[i].token) == token)
      return m_info[i].direction;
      
  return Unknown;
}

