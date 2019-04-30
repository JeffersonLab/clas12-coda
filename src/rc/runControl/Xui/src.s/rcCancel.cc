//-----------------------------------------------------------------------------
// Copyright (c) 1994,1995 Southeastern Universities Research Association,
//                         Continuous Electron Beam Accelerator Facility
//
// This software was developed under a United States Government license
// described in the NOTICE file included as part of this distribution.
//
// CEBAF Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606
//       coda@cebaf.gov  Tel: (804) 249-7030     Fax: (804) 249-5800
//-----------------------------------------------------------------------------
//
// Description:
//      Implementation of Cancel Command Button
//
// Author:  
//      Jie Chen
//      CEBAF Data Acquisition Group
//
// Revision History:
//   $Log: rcCancel.cc,v $
//   Revision 1.1.1.1  1996/10/11 13:39:28  chen
//   run control source
//
//
#include <rcNetStatus.h>
#include <rcButtonPanel.h>
#include <rcAudioOutput.h>
#include "rcCancel.h"

#define RC_CANCEL_NAME (char *)"  Cancel  "
#define RC_CANCEL_MSG  (char *)"Cancel Transition"

rcCancel::rcCancel (Widget parent, rcButtonPanel* panel,
		    rcClientHandler& handler)
:rcComdButton (parent, RC_CANCEL_NAME, RC_CANCEL_MSG, panel, handler)
{
#ifdef _TRACE_OBJECTS
  printf ("              Create rcCancel Class Object\n");
#endif
  // empty
}

rcCancel::~rcCancel (void)
{
#ifdef _TRACE_OBJECTS
  printf ("              Delete rcCancel Class Object\n");
#endif
  // empty
}

void
rcCancel::doit (void)
{
  rcAudio ((char *)"cancel this transition");
#ifdef _TRACE_OBJECTS
  printf("rcCancel::doit: cancel this transition\n");fflush(stdout);
#endif
  assert (stWin_);

  // get network handler first
  rcClient& client = netHandler_.clientHandler ();
  daqData data ((char *)"RCS", (char *)"command", (int)DACANCELTRAN);
  if (client.sendCmdCallback (DACANCELTRAN, data,
			      (rcCallback)&(rcCancel::cancelCallback),
			      (void *)this) != CODA_SUCCESS)
  {
    reportErrorMsg ((char *)"Cannot send download command to the server.");
  }
}

void
rcCancel::undoit (void)
{
  // empty
}

void
rcCancel::cancelCallback (int status, void* arg, daqNetData* data)
{
  rcCancel* obj = (rcCancel *)arg;

#ifdef _TRACE_OBJECTS
  printf("rcCancel::cancelCallback reached\n");fflush(stdout);
#endif

  if (status != CODA_SUCCESS)
  {
    obj->reportErrorMsg ((char *)"Cancel a transaction failed !!!\n");
  }
}

