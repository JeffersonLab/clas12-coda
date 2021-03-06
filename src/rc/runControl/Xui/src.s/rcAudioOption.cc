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
//      Implementation of audio option toggle button
//
// Author:  
//      Jie Chen
//      CEBAF Data Acquisition Group
//
// Revision History:
//   $Log: rcAudioOption.cc,v $
//   Revision 1.2  1996/12/04 18:32:24  chen
//   port to 1.4 on hp and ultrix
//
//   Revision 1.1.1.1  1996/10/11 13:39:28  chen
//   run control source
//
//
#if defined (_CODA_2_0_T) || defined (_CODA_2_0)
#include <rcComdOption.h>
#include "rcAudioOption.h"

rcAudioOption::rcAudioOption (char* name, int active,
			      char* acc, char* acc_text,
			      int state,
			      rcClientHandler& handler)
:rcMenuTogComd (name, active, acc, acc_text, state, handler)
{
#ifdef _TRACE_OBJECTS
  printf ("               Create rcAudioOption class object\n");
#endif
}

rcAudioOption::~rcAudioOption (void)
{
#ifdef _TRACE_OBJECTS
  printf ("               Delete rcAudioOption class object\n");
#endif
  // mshWin_ is just a pointer
}

void
rcAudioOption::doit (void)
{
  rcComdOption* option = rcComdOption::option ();

  if (state () > 0)  // after button pressed
    option->audioOn ();
  else
    option->audioOff ();
}

void
rcAudioOption::undoit (void)
{
  // empty
}
#endif
