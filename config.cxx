/*
  $Id: config.cxx,v 1.2 2009/05/22 22:50:33 jan Exp $
  
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
#include <ptlib/sockets.h>
#include <ptlib/svcproc.h>
#include <fstream>

#include "config.hxx"
#include "routing.hxx"

// define the default values.

#define DEFAULT_ADMIN ""
#define DEFAULT_AGC FALSE
#define DEFAULT_ATD_TIMEOUT "30"
#define DEFAULT_CONNECTING_MESSAGE ""
#define DEFAULT_DIRECTORY_FILENAME ""
#define DEFAULT_ECHO_COMPENSATION FALSE
#define DEFAULT_ISDN_RATE ""
#define DEFAULT_LISTENER_INTERFACE "0.0.0.0"
#define DEFAULT_LISTENER_PORT "1720"
#define DEFAULT_LOG_LEVEL "0"
#define DEFAULT_NAME "ISDN Gateway"
#define DEFAULT_NEED_GATEKEEPER TRUE
#define DEFAULT_GATEKEEPER ""
#define DEFAULT_MSN ""
#define DEFAULT_PBX_PREFIX ""
#define DEFAULT_STATUS_FILENAME ""
#define DEFAULT_TRACES ""
#define DEFAULT_USER_INPUT TRUE
#define DEFAULT_CPN_PREFIX ""
#define DEFAULT_DIAL_PREFIX ""

PString         Config::m_admin;
PBoolean            Config::m_agc;
PStringArray    Config::m_allowed_ips;
int             Config::m_ATD_timeout;
PString         Config::m_connecting_message;
PString         Config::m_default_msn;
PStringArray    Config::m_devices;
PString         Config::m_directory_filename;
PBoolean            Config::m_echo_comp;
PString         Config::m_html;
PString         Config::m_lcr_isdnrate;
PString         Config::m_lcr_pbx_prefix;
PString         Config::m_ras_interface;
PString         Config::m_listener_interface;
int             Config::m_listener_port;
PString         Config::m_name;
PBoolean            Config::m_need_gatekeeper;
PString         Config::m_gatekeeper;
PStringArray    Config::m_prefix;
PString         Config::m_status_filename;
ofstream*       Config::m_stream = NULL;
PBoolean            Config::m_user_input;
PString         Config::m_cpn_prefix;
PString         Config::m_dial_prefix;
PStringToString Config::m_rewriteInfo;

////////////////////////////////////
// First read of the configuration

PBoolean Config::Start(const PString & file)
{
  if (!ReadConfig(file))
    return FALSE;

  return TRUE;
}

/////////////////////////////////////////
// Last call to the configuration class

void Config::Stop()
{
  Free();
}

///////////////////////////////////////////////
// Read the configuration from "/etc/isdngw"

PBoolean Config::ReadConfig (const PString & file)
{
  PConfig config(file, "system");

  // get system configuration
  config.SetDefaultSection("system");
  m_admin               = config.GetString("admin", DEFAULT_ADMIN);
  m_directory_filename  = config.GetString("directory filename", DEFAULT_DIRECTORY_FILENAME);
  m_name                = config.GetString("name", DEFAULT_NAME);
  m_status_filename     = config.GetString("status filename", DEFAULT_STATUS_FILENAME);

  config.SetDefaultSection("network");
  m_gatekeeper          = config.GetString("gatekeeper", DEFAULT_GATEKEEPER);
  m_listener_interface  = config.GetString("listener interface", DEFAULT_LISTENER_INTERFACE);
  m_listener_port       = config.GetString("listener port", DEFAULT_LISTENER_PORT).AsInteger();
  m_need_gatekeeper     = config.GetBoolean("need gatekeeper", DEFAULT_NEED_GATEKEEPER);
  m_ras_interface       = config.GetString("ras interface", "");

  config.SetDefaultSection("isdn");
  m_ATD_timeout         = config.GetString("atd timeout", DEFAULT_ATD_TIMEOUT).AsInteger();
  m_default_msn         = config.GetString("default msn", DEFAULT_MSN);
  m_lcr_isdnrate        = config.GetString("isdn rate", DEFAULT_ISDN_RATE);
  m_lcr_pbx_prefix      = config.GetString("pbx prefix", DEFAULT_PBX_PREFIX);
  m_cpn_prefix          = config.GetString("cpn prefix", DEFAULT_CPN_PREFIX);
  m_dial_prefix         = config.GetString("dial prefix", DEFAULT_DIAL_PREFIX);

  config.SetDefaultSection("h323");
  m_agc                 = config.GetBoolean("agc", DEFAULT_AGC);
  m_echo_comp           = config.GetBoolean("echo compensation", DEFAULT_ECHO_COMPENSATION);
  m_user_input          = config.GetBoolean("user input", DEFAULT_USER_INPUT);

  config.SetDefaultSection("messages");
  m_connecting_message  = config.GetString("connecting", DEFAULT_CONNECTING_MESSAGE);

  
  // get log configuration
  config.SetDefaultSection("log");
  PString logfile_path = config.GetString("traces", DEFAULT_TRACES);
  PTrace::SetStream(NULL);
  if (m_stream != NULL)
    delete m_stream;
  if (!logfile_path.IsEmpty()) {
    m_stream = new ofstream(logfile_path, ios::app);
    PTrace::SetStream(m_stream);
  }
  PTrace::SetLevel(config.GetString("level",DEFAULT_LOG_LEVEL).AsInteger());

  // get variable length parameters
  PStringList sections = config.GetSections();

  for (PINDEX i=0; i<sections.GetSize(); i++) {
    if (sections[i] *= "system")
      continue;
    else if (sections[i] *= "log")
      continue;
    else if (sections[i] *= "network")
      continue;
    else if (sections[i] *= "isdn")
      continue;
    else if (sections[i] *= "h323")
      continue;
    else if (sections[i] *= "messages")
      continue;
    else if (sections[i] *= "devices") {
      config.SetDefaultSection("devices");

      PStringList keys = config.GetKeys();
      m_devices.RemoveAll();
      for (PINDEX j=0; j<keys.GetSize(); j++)
        m_devices.Append(new PString(config.GetString(keys[j])));
    }
    else if (sections[i] *= "allowed ip") {
      config.SetDefaultSection("allowed ip");
      
      PStringList keys = config.GetKeys();
      m_allowed_ips.RemoveAll();
      for (PINDEX j=0; j<keys.GetSize(); j++)
        m_allowed_ips.Append(new PString(config.GetString(keys[j])));
    }
    else if (sections[i] *= "prefixes") {
      config.SetDefaultSection("prefixes");
      
      PStringList keys = config.GetKeys();
      m_prefix.RemoveAll();
      for (PINDEX j=0; j<keys.GetSize(); j++)
        m_prefix.Append(new PString(config.GetString(keys[j])));
    }
    else if (sections[i] *= "isdnrewrite") {
      config.SetDefaultSection("isdnrewrite");
      
      PStringToString keys = config.GetAllKeyValues();
      m_rewriteInfo = keys;
    }
    else {
      PTRACE(0, "Unknown section in config file (" << sections[i] << ')');
      PSYSTEMLOG(Error, "Unknown section in config file (" << sections[i] << ')');
      return FALSE;
    }    
  }

  if (m_default_msn.IsEmpty()) {
    PTRACE (0, "No default ISDN MSN given");
    PSYSTEMLOG(Error, "No default ISDN MSN given");
    return FALSE;
  }

  if (m_listener_interface.IsEmpty()) {
    PTRACE (0, "No listener interface given");
    PSYSTEMLOG(Error, "No listener interface given");
    return FALSE;
  }

  if (m_ras_interface.IsEmpty()) {
    PTRACE (0, "No RAS interface given, using listener interface");
    PSYSTEMLOG(Info, "No ras interface given, using listener interface");
    m_ras_interface = m_listener_interface;
  }
  
  // initialize routing table

  if (!Routing::Init())
    return FALSE;
  
  // create HTML status and PTRACE() messages

  m_html = "\n<BR><BR>\n<H1>Current configuration</H1>\n<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=10 BGCOLOR=#DDDDDD WIDTH=90%>\n";

  if (!m_name.IsEmpty()) {
    PTRACE(2, "local user name: " << m_name); 
    m_html += "<TR><TD BGCOLOR=#CCCCCC WIDTH=30%><B>Local user name</B><TD WIDTH=60%>"
      + m_name + "\n";
  }

  m_html += "<TR><TD BGCOLOR=#CCCCCC><B>Automatic gain control</B><TD>";
  if (m_agc == true) {
    PTRACE(2, "automatic gain control: enabled");
    m_html += "enabled\n";
  } 
  else {
    PTRACE(2, "automatic gain control: disabled");
    m_html += "disabled\n";
  }

  m_html += "<TR><TD BGCOLOR=#CCCCCC><B>Echo compensation</B><TD>";
  if (m_echo_comp == true) {
    PTRACE(2, "echo compensation: enabled");
    m_html += "enabled\n";
  }
  else {
    PTRACE(2, "echo compensation: disabled");         
    m_html += "disabled\n";
  }

  PTRACE(2, "default ISDN number: " << m_default_msn);
  m_html += "<TR><TD BGCOLOR=#CCCCCC><B>Default ISDN MSN</B><TD>" + m_default_msn + '\n';

  if (!m_prefix.IsEmpty()) {
    PString prefixes;
    for(PINDEX i=0; i<m_prefix.GetSize(); i++) {
      if (i>0)
        prefixes += ' ';
      prefixes += m_prefix[i];
    }
    
    PTRACE(2, "ISDN prefixes: " << prefixes);
    m_html += "<TR><TD BGCOLOR=#CCCCCC><B>ISDN prefixes</B><TD>" + prefixes + '\n';
  }

  m_html += "<TR><TD BGCOLOR=#CCCCCC WIDTH=30%><B>Least cost router</B><TD WIDTH=60%>";
  if (!m_lcr_isdnrate.IsEmpty()) {
    PTRACE(2, "Least cost router: enabled");
    m_html += "enabled\n";
  }
  else {
    PTRACE(2, "Least cost router: disabled");
    m_html += "disabled\n";
  }

  PTRACE(2, "ATD timeout: " << m_ATD_timeout << "s.");
  m_html += "<TR><TD BGCOLOR=#CCCCCC><B>ATD timeout</B><TD>";
  m_html.sprintf("%ds.\n", m_ATD_timeout);

  m_html += "</TABLE>\n";

  return TRUE;
}

///////////////////////////////////////////////
// Frees the memory used by the configuration

void Config::Free()
{
  m_devices.RemoveAll();
  m_allowed_ips.RemoveAll();
  m_prefix.RemoveAll();

  Routing::Free();
}

