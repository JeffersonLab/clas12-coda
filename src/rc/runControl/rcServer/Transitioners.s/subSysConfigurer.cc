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
//      subSysConfigurer Class Implementation
//
// Author:  
//      Jie Chen
//      CEBAF Data Acquisition Group
//
// Revision History:
//   $Log: subSysConfigurer.cc,v $
//   Revision 1.1.1.1  1996/10/11 13:39:22  chen
//   run control source
//
//
#include "subSysConfigurer.h"

subSysConfigurer::subSysConfigurer (daqSubSystem* subsys)
:subSysTransitioner (subsys)
{
#ifdef _TRACE_OBJECTS
  printf ("subSysConfigurer::subSysConfigurer: Create subSysConfigurer Class Object\n");
#endif
  // empty
}

subSysConfigurer::~subSysConfigurer (void)
{
#ifdef _TRACE_OBJECTS
  printf ("subSysConfigurer::~subSysConfigurer: Delete subSysConfigurer Class Object\n");
#endif
  // empty
}

int
subSysConfigurer::action (void) const
{
  return CODA_CONFIGURE_ACTION;
}

void
subSysConfigurer::executeItem (daqComponent* comp)
{
#ifdef _TRACE_OBJECTS
  printf("subSysConfigurer::executeItem reached\n");fflush(stdout);
#endif
  comp->configure ();
}

int
subSysConfigurer::failureState (void)
{
  return CODA_BOOTED;
}

int
subSysConfigurer::successState (void)
{
  return CODA_CONFIGURED;
}
