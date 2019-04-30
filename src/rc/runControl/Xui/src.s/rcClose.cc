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
//      Implementation of rcClose Class
//
// Author:  
//      Jie Chen
//      CEBAF Data Acquisition Group
//
// Revision History:
//   $Log: rcClose.cc,v $
//   Revision 1.2  1999/12/09 21:29:01  rwm
//   Do not do clean memory on linux - bug in Xlibs?
//
//   Revision 1.1.1.1  1996/10/11 13:39:25  chen
//   run control source
//
//
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <XcodaApp.h>

#include <codaComdXInterface.h>

#include <rcCloseDialog.h>
#include <rcNetStatus.h>
#include <rcInfoPanel.h>
#include "rcClose.h"

/*sergey*/
extern Widget toplevel;
#ifdef USE_CREG
#include <codaRegistry.h>
#endif
#include "Editor.h"
extern int ncomp_;
extern pid_t proc_id[MAX_NUM_COMPS];
/*sergey*/


rcClose::rcClose (char* name, int active,
		  char* acc, char* acc_text,
		  rcClientHandler& handler)
:rcMenuComd (name, active, acc, acc_text, handler), dialog_ (0)
{
#ifdef _TRACE_OBJECTS
  printf ("         Create rcClose Class Object\n");
#endif
}

rcClose::~rcClose (void)
{
#ifdef _TRACE_OBJECTS
  printf ("         Delete rcClose Class Object\n");
#endif
  // dialog_ will be destroyed by Xt Mechanism
}



/*sergey*/
extern Widget toplevel;
#ifdef USE_CREG
#include <codaRegistry.h>
#endif

void
rcClose::doit (void)
{
  int ii, ret;

  if (netHandler_.connected ()) {
    if (!dialog_) {
      dialog_ = new rcCloseDialog (this, (char *)"closeDialog", (char *)"Close Dialog",
				   netHandler_);
      dialog_->init ();
      dialog_->alignment (XmALIGNMENT_CENTER);
      // set message
      char temp[256];
      sprintf (temp, "This leaves RunControl server still running\n");
      strcat  (temp, "Are you sure?");
      dialog_->setMessage (temp);
    }
    if (dialog_->isMapped ())
      dialog_->popdown ();
    dialog_->popup ();
  }
  else
  {
	/*sergey*/
    /* cleanup previously opened components if any */
    if(ncomp_)
    {
      printf("rcClose::doit: CLEANUP !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      for(ii=0; ii<ncomp_; ii++)
	  {
        ret = kill(proc_id[ii], SIGTERM);
        printf("[%2d] pid=%d, kill() returned %d\n",ii,proc_id[ii],ret);
	  }
    }
    ncomp_ = 0;
	/*sergey*/

#if 0
    /*sergey: testing*/
    {
      char cmd[100];
      sprintf(cmd,"b:%s","blablabla");
printf("CTERM: >%s<\n",cmd);
#ifdef USE_CREG
      /*coda_Send(XtDisplay(toplevel),"CTERM",cmd);*/
      coda_Send(XtDisplay(toplevel),"00_00_WINDOW",cmd);
      coda_Send(XtDisplay(toplevel),"00_01_WINDOW",cmd);
#endif
    }
#endif

	/*
    close ();
	*/

  }
}

void
rcClose::undoit (void)
{
  // empty
}

void
rcClose::close (void)
{
  if (netSt_)
    netSt_->stop ();
  if (rcipanel_)
    rcipanel_->stop ();
  if (netHandler_.connected ())
    // disconnect from the server
    netHandler_.disconnect ();

#ifndef linux
  // clean up memory
  theApplication->cleanMemory ();
#else
  exit(EXIT_SUCCESS);
#endif
}
