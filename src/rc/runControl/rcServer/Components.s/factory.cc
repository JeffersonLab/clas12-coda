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
//      Implementation of factory class
//
// Author:  
//      Jie Chen
//      CEBAF Data Acquisition Group
//
// Revision History:
//   $Log: factory.cc,v $
//   Revision 1.2  1997/01/24 16:36:05  chen
//   change/add Log Component for 1.4
//
//   Revision 1.1.1.1  1996/10/11 13:39:18  chen
//   run control source
//
//
#include <daqActions.h>
#include <daqScriptComp.h>
#include <daqLogComp.h>
#include "factory.h"

factory::factory (int exptid, daqSystem& system)
:exptid_ (exptid), system_ (system)
{
#ifdef _TRACE_OBJECTS
  printf ("Create factory Class Object\n");
#endif
  // empty
}

factory::~factory (void)
{
#ifdef _TRACE_OBJECTS
  printf ("Delete factory Class Object\n");
#endif
  // empty
}

daqSubSystem* 
factory::createSubSystem (char* className)
{
  daqSubSystem* subsys = 0;

  if (system_.locateSystem (className, subsys) == CODA_SUCCESS)
  {
    printf("factory::createSubSystem: className=%s already exist\n",className);
    return subsys;
  }
  else
  {
    printf("factory::createSubSystem: className=%s created\n",className);
    subsys = new daqSubSystem (className, system_);
    return subsys;
  }
}



/*sergey: it seems, following called on 'Connect' click for every component from 'process' table ?*/
    
daqComponent*
factory::createComponent (char* title, int number, char* type, char* node, char* bootString)
{
  daqComponent* comp = 0;
  
  printf("factory::createComponent: title=%s, number=%d, type=%s, node=%s, bootString=%s\n",title,number,type,node,bootString);

  if (system_.has (title,comp) == CODA_SUCCESS)
  {
    return comp;
  }
  else
  {
    daqSubSystem* subsys = createSubSystem (type);

    comp = new netComponent (title, number, exptid_, type, node, bootString, *subsys);
    return comp;
  }
}


daqComponent*
factory::createComponent (daqComponent* comp, char* action, char* script)
{
  char fullname[80];
  static int id = 0;

  int act = codaDaqActions->action (action);
  if (act == CODA_ERROR) return 0;
  
  sprintf (fullname, "%s_%s_%s_%d",comp->title (), action, CODA_USER_SCRIPT, id++);

  // since all comp must be netComponent, the followings are safe
  netComponent* ncomp = (netComponent *)comp;
  daqSubSystem& subsys = ncomp->subSystem ();
  daqSystem& sys = subsys.system ();
  
  // put this component before real net component
  daqComponent *scomp = new daqScriptComp (fullname,
					   act,
					   comp->priority () + 1,
					   script,
					   subsys,
					   *ncomp);
  return scomp;
}

void
factory::subSystemPriority (char* className, int priority)
{
  daqSubSystem* subsys = 0;

  //printf("OOOOO factory::subSystemPriority: className=%s, priority=%d\n",className,priority);
  
  if (system_.locateSystem (className, subsys) == CODA_SUCCESS) 
    subsys->setDefaultPriority (priority);
}

