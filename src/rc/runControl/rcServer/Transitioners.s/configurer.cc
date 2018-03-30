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
//      Configurer Implementation
//
// Author:  
//      Jie Chen
//      CEBAF Data Acquisition Group
//
// Revision History:
//   $Log: configurer.cc,v $
//   Revision 1.1.1.1  1996/10/11 13:39:21  chen
//   run control source
//
//

#include <daqSubSystem.h>
#include "configurer.h"

#define _TRACE_OBJECTS

configurer::configurer (daqSystem* system)
:transitioner (system)
{
#ifdef _TRACE_OBJECTS
  printf ("configurer::configurer: Create configurer Class Object\n");
#endif
  // empty
}

configurer::~configurer (void)
{
#ifdef _TRACE_OBJECTS
  printf ("configurer::~configurer: Delete configurer Class Object\n");
#endif
  // empty
}



int
configurer::transitionFinished (int fstate, int successState)
{
#ifdef _TRACE_OBJECTS
  printf("configurer::transitionFinished reached, returns (%d == %d)\n",fstate,successState);
#endif







  return(fstate == successState);

#if 0
#ifdef NEW_STUFF
  /*sergey: do the same as transitioner::transitionFinished */
  return(fstate == successState);
#else
  if (fstate != CODA_DISCONNECTED)
    return 1;
  return 0;
#endif
#endif

}


int
configurer::action (void)
{
#ifdef _TRACE_OBJECTS
  printf("configurer::action\n");fflush(stdout);
#endif
  return CODA_CONFIGURE_ACTION;
}

void
configurer::executeItem (daqSubSystem* subsys)
{
#ifdef _TRACE_OBJECTS
  printf("configurer::executeItem reached\n");fflush(stdout);
#endif
  subsys->configure (); /* calls daqSubSystem::configure() ????????????? */

  /*
netComponent::configure reached
codaCompClnt::codaDaCompConfigure reached (name >ROC85<)

   and after loop over all components:

netComponent::state() reached
netComponent::state: Component ROC85 at state 4
configurer::successState (CODA_CONFIGURED)
configurer::transitionFinished reached, returns (4 == 4)
   */

}

int
configurer::failureState (void)
{
#ifdef _TRACE_OBJECTS
  printf("configurer::failureState\n");
#endif
  return CODA_BOOTED;
}

int
configurer::successState (void)
{
#ifdef _TRACE_OBJECTS
  printf("configurer::successState (always returns CODA_CONFIGURED)\n");
#endif
  return CODA_CONFIGURED;
}

int
configurer::transitionState (void)
{
#ifdef _TRACE_OBJECTS
  printf("configurer::transitionState\n");
#endif

  /* sergey: return CODA_BOOTED triggers Download button to show up, CODA_DORMANT does not */
  return CODA_BOOTED;
  //return CODA_DORMANT;
}
