/*
  $Id: config.hxx,v 1.2 2009/05/22 22:50:33 jan Exp $
  
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

#ifndef _CONFIG_HXX_
#define _CONFIG_HXX_

// version of the gateway
#define MAJOR_VERSION 0
#define MINOR_VERSION 4
#define BUILD_TYPE    ReleaseCode
#define BUILD_NUMBER  0

#include <ptlib.h>

#if !defined(P_USE_STANDARD_CXX_BOOL) && !defined(P_USE_INTEGER_BOOL)
	typedef int PBoolean;
#endif


class TempUserInformations : public PObject {
  PCLASSINFO(TempUserInformations,PObject);
public:
  PString number;
  PString h323user;
  PString gateway;
};

PARRAY(TempArray, TempUserInformations);

//class ofstream;

class Config
{
public:
  static PBoolean Start(const PString & file);
  static void Stop();

  static PBoolean ReadConfig(const PString & file);
  static void Free();

  static const PString& GetH323Name()
  { return m_name; }
  
  static const PString& GetListenerInterface()
  { return m_listener_interface; }

  static const int GetListenerPort()
  { return m_listener_port; }

  static const PString& GetRASInterface()
  { return m_ras_interface; }
  
  static const PBoolean NeedGatekeeper()
  { return m_need_gatekeeper; }

  static const PString& GetGatekeeper()
  { return m_gatekeeper; }
  
  static const PString& GetAdmin()
  { return m_admin; }
  
  static const PString& GetDefaultMSN()
  { return m_default_msn; }
  
  static PBoolean UseAGC()
  { return m_agc; }
  
  static PBoolean UseEchoComp()
  { return m_echo_comp; }
  
  static const PString& GetStatusFilename()
  { return m_status_filename; }
  
  static PString GetLcrIsdnrateSocket()
  { return m_lcr_pbx_prefix; }
  
  static PString GetLcrPbxPrefix()
  { return m_lcr_pbx_prefix; }
  
  static int GetATDTimeout()
  { return m_ATD_timeout; }
  
  static const PStringArray& GetDevices()
  { return m_devices; }
  
  static const PStringArray& GetAllowedIPs()
  { return m_allowed_ips; }
  
  static const PStringArray& GetISDNPrefix()
  { return m_prefix; }

  static const PString& GetDirectoryFilename()
  { return m_directory_filename; }

  static const PString& GetConnectingMessage()
  { return m_connecting_message; }
  
  static const PString& GetHTML()
  { return m_html; }

  static PBoolean UseUserInput()
  { return m_user_input; }


  static const PString& GetCPNPrefix()
  { return m_cpn_prefix; }

  static const PString& GetDialPrefix()
  { return m_dial_prefix; }

  static const PStringToString& GetRewriteInfo()
  { return m_rewriteInfo; }


private:
  static PString        m_name;
  static PString        m_ras_interface;
  static PString        m_listener_interface;
  static int            m_listener_port;
  static PBoolean           m_need_gatekeeper;
  static PString        m_gatekeeper;
  static PString        m_admin;
  static PString        m_default_msn;
  static PBoolean           m_agc;
  static PBoolean           m_echo_comp;
  static PString        m_status_filename;
  static PString        m_lcr_isdnrate;
  static PString        m_lcr_pbx_prefix;
  static int            m_ATD_timeout;

  static PString        m_directory_filename;

  static PString        m_connecting_message;
  
  static PStringArray   m_devices;

  static PStringArray   m_allowed_ips;

  static PStringArray   m_prefix;

  static PString        m_html;

  static ofstream*      m_stream;

  static PBoolean           m_user_input;
  static PString        m_cpn_prefix;
  static PString        m_dial_prefix;
  static PStringToString m_rewriteInfo;
};





#endif /* _CONFIG_HXX_ */
