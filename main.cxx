/*
  $Id: main.cxx,v 1.2 2009/05/22 22:50:33 jan Exp $
  
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

#include <sched.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>         

#include <ptlib.h>
#include <ptlib/svcproc.h>
#include <h323.h>
//#include <gsmcodec.h>

#include "pots.hxx"
#include "resource.hxx"
#include "connection.hxx"
#include "endpoint.hxx"
#include "routing.hxx"
#include "config.hxx"


class PMain : public PServiceProcess
{
  PCLASSINFO(PMain, PServiceProcess);

public: 
  PMain();
  ~PMain();

  void Main();

  int SetRealtimePriority();
  int SetStandardPriority();
  
  PBoolean OnStart()
  { return TRUE; }

  void OnControl() { }  
  
  static void SignalCallback(int);

private:
  PBoolean Init();
  void MainLoop();
  void Clear();

private:
  GwH323EndPoint *      m_endpoint;
  static PBoolean           m_going_down;
};


//////////////////////////////////////////////////////
//  Signal handler                                  //
//////////////////////////////////////////////////////

PBoolean PMain::m_going_down = FALSE;

void PMain::SignalCallback (int rec_signal)
{
  // don't want to get these signals anymore
  signal(SIGTERM, SIG_IGN);
  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);      

  PTRACE(2, "Received signal " << rec_signal);  

  m_going_down = TRUE;  
}

PCREATE_PROCESS(PMain);

//////////////////////////////////////////////////////
//  Constructor                                     //
//////////////////////////////////////////////////////

PMain::PMain ()
  : PServiceProcess("OpenH323", "ISDN - H.323 gateway",
                    MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER),
  m_endpoint(NULL)
{
}

//////////////////////////////////////////////////////
// Destructor: clears all H.323 connections and     //
//             ends the whole gateway               //
//////////////////////////////////////////////////////

PMain::~PMain ()
{
}


//////////////////////////////////////////////////////
//  Entry point of the main thread. This thread is  //
//  listening for incoming ISDN calls.              //
//////////////////////////////////////////////////////

void PMain::Main ()
{
  if (!Init())
    return;

  if (!SetRealtimePriority())
    return;
  
  // call the main loop
  MainLoop();

  if (!SetStandardPriority())
    return;            

  Clear();  
}

PBoolean PMain::Init()
{
  PTrace::SetOptions(PTrace::DateAndTime| /*PTrace::Thread|*/ PTrace::Blocks);
  PTrace::ClearOptions(PTrace::FileAndLine | PTrace::SystemLogStream);

  signal(SIGTERM, SignalCallback);  // catch signals
  signal(SIGINT, SignalCallback);
  signal(SIGQUIT, SignalCallback);


  // initialize the Config class
  PString config_file = "/etc/isdngw/isdngw.conf";
  if (GetArguments().HasOption('i')) {
	  config_file = GetConfigurationFile();
  }
  if (!Config::Start(config_file)) {
    PTRACE(0,"Unable to initialize Config");
    return FALSE;
  }

  // initialize the InfoConnections class
  InfoConnections::Init();

  // create and initialize the GwH323EndPoint object
  m_endpoint = new GwH323EndPoint();
  if (!m_endpoint->Init()) {
    PTRACE(0,"Unable to initialize GwH323EndPoint");
    return FALSE;
  }

  return TRUE;
}

////////////////////////////////////////
//  Give isdngw the highest priority  //
////////////////////////////////////////

int PMain::SetRealtimePriority()
{
  struct sched_param schp;
       
  memset(&schp, 0, sizeof(schp));
  schp.sched_priority = sched_get_priority_max(SCHED_FIFO);

  if (sched_setscheduler(0, SCHED_FIFO, &schp) != 0) {
    PTRACE(0,"error while setting realtime priority");
    return FALSE;
  }
  
  PTRACE(0,"realtime priority OK");
  return TRUE;
}

/////////////////////////////////////////
//  Give isdngw the standard priority  //
/////////////////////////////////////////

int PMain::SetStandardPriority()
{
  struct sched_param schp;
       
  memset(&schp, 0, sizeof(schp));
  schp.sched_priority = sched_get_priority_max(SCHED_OTHER);

  if (sched_setscheduler(0, SCHED_OTHER, &schp) != 0) {
    PTRACE(0,"error while setting standard priority");
    return FALSE;
  }
  
  PTRACE(0,"standard priority OK");
  return TRUE;
}

void PMain::MainLoop()
{
  pots* pots_class = NULL;
  
  PString token;
  for (;;) {
    // check if the gateway has to shutdown
    if (m_going_down) {
      PTRACE(0,"Going down");
      if (pots_class != NULL)
        delete pots_class;
      sleep(5);
      return;
    }

    // if there is no pots object to listen for incoming call,
    // let's create it
    if (pots_class == NULL) {
      while (!InfoConnections::LineGet_Temp(token))
        sleep(1);
      pots_class = new pots(InfoConnections::GetDevice(token), Config::GetDefaultMSN());
      if (!pots_class->Init()) {
        PTRACE(0, "Cannot initialize device: " << InfoConnections::GetDevice(token));
        delete pots_class;
        return;
      }
      if (!pots_class->ListenOn(Routing::GetATList())) {
        PTRACE(0, "ListenOn() error");
        delete pots_class;
        return;
      }
      InfoConnections::PotsSet(token, pots_class);
    }

    // wait 5 seconds for an incoming call
    if (pots_class->WaitForCall(5))
      if (m_endpoint->Call(token))
        // now, the pots object is used for an incoming call,
        // so another one must be used to listen for incoming calls
        pots_class = NULL;
  }    
}

void PMain::Clear()
{
  // clear all H.323 calls.
  m_endpoint->ClearAllCalls();
  sleep(5);      // fixme: hack - needed to close ISDN connections
  delete m_endpoint;  

  // clear the Infoconnections table
  InfoConnections::Clear();

  // clear the configuration
  Config::Stop();
}
