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
//      Implementation of run type dialog
//
// Author:  
//      Jie Chen
//      CEBAF Data Acquisition Group
//
// Revision History:
//   $Log: rcRunTypeDialog.cc,v $
//   Revision 1.6  1998/06/18 12:20:42  heyes
//   new GUI ready I think
//
//   Revision 1.5  1998/04/08 18:31:33  heyes
//   new look and feel GUI
//
//   Revision 1.4  1997/10/15 16:08:32  heyes
//   embed dbedit, ddmon and codaedit
//
//   Revision 1.3  1997/08/01 18:38:17  heyes
//   nobody will believe this!
//
//   Revision 1.2  1997/06/16 12:26:50  heyes
//   add dbedit and so on
//
//   Revision 1.1.1.1  1996/10/11 13:39:26  chen
//   run control source
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Xm/Form.h>
#include <Xm/PushBG.h>

#include <rcDbaseHandler.h>
#include <rcRunTypeOption.h>
#include <XcodaErrorDialog.h>
#ifdef USE_CREG
#include <codaRegistry.h>
#endif
#include "rcRunTypeDialog.h"
#include "rcXpmComdButton.h"

#include <XcodaFileSelDialog.h>

/*
#define _TRACE_OBJECTS
*/

rcRunTypeDialog::rcRunTypeDialog (Widget parent,
				  char* name,
				  char* title,
				  rcClientHandler& handler)
:XcodaFormDialog (parent, name, title), netHandler_ (handler),
 option_ (0), errDialog_ (0), ok_ (0), cancel_ (0)
{
#ifdef _TRACE_OBJECTS
  printf ("rcRunTypeDialog::rcRunTypeDialog: Create rcRunTypeDialog Class Object, _w=0x%08x !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif
  // empty
}

rcRunTypeDialog::~rcRunTypeDialog (void)
{
#ifdef _TRACE_OBJECTS
  printf ("                   Delete rcRunTypeDialog Class Object\n");
#endif
  // empty
  // option_ and dialog_ will be destroyed by Xt destroy mechanism
}

void
rcRunTypeDialog::createFormChildren (void)
{
  Arg arg[20];
  int ac = 0;

  ac = 0;  
  XtSetValues (_w, arg, ac);
  // create option menu first
  option_ = new rcRunTypeOption (_w, (char *)"runtype", (char *)"Run Type ", netHandler_, this);
  option_->init ();
  
  ac = 0;  
  // create action form
  XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  Widget actionForm = XtCreateManagedWidget ("runTypeActionForm",
				      xmFormWidgetClass, _w, arg, ac);

  XtManageChild (actionForm);

  ac = 0;
  // create push buttons
  rcXpmComdButton *ok     = new rcXpmComdButton(actionForm,(char *)"Ok",    NULL,(char *)"select run type",NULL,netHandler_);
  rcXpmComdButton *cancel = new rcXpmComdButton(actionForm,(char *)"Cancel",NULL,(char *)"cancel",NULL,netHandler_);


  ok->init();
  cancel->init();

  
  ok_     = ok->baseWidget();
  cancel_ = cancel->baseWidget();


  ac = 0;
  XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetValues (cancel_, arg, ac);



  /*sergey: 'Ok' displayed already, do not need following ???
  ac = 0;
  XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (arg[ac], XmNrightWidget, ok_); ac++;
  XtSetValues (ok_, arg, ac);
  */



  ac = 0;


  // set resource for option menu
  XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNtopOffset, 10); ac++;
  XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (arg[ac], XmNbottomWidget, actionForm); ac++;
  XtSetArg (arg[ac], XmNbottomOffset, 10); ac++;
  XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNleftOffset, 10); ac++;
  XtSetValues (option_->baseWidget(), arg, ac);
  ac = 0;



  // add callbacks
  XtAddCallback (ok_, XmNactivateCallback,
		 (XtCallbackProc)&(rcRunTypeDialog::okCallback),
		 (XtPointer)this);

  XtAddCallback (cancel_, XmNactivateCallback,
		 (XtCallbackProc)&(rcRunTypeDialog::cancelCallback),
		 (XtPointer)this);

  // manage all widgets
  option_->manage ();

  // set up default button
  defaultButton (ok_);
}

void
rcRunTypeDialog::startMonitoringRunTypes (void)
{
  option_->startMonitoringRunTypes (); /* sergey: displays 'runTypes' database table here */
}

void
rcRunTypeDialog::endMonitoringRunTypes (void)
{
  option_->endMonitoringRunTypes ();
}

void
rcRunTypeDialog::configure (void)
{
  char *currentruntype = option_->currentRunType();
  if (currentruntype != 0)
  {
    rcClient& client = netHandler_.clientHandler ();
#ifdef _CODA_DEBUG
    printf(">>> rcRunTypeDialog::configure >%s<\n",currentruntype);
#endif
    daqData data ((char *)"RCS", (char *)"command", currentruntype);
    if (client.sendCmdCallback (DACONFIGURE, data,
		 (rcCallback)&(rcRunTypeDialog::configureCallback),
		 (void *)this) != CODA_SUCCESS)
	{
      reportErrorMsg ((char *)"Cannot communication with the RunControl Server\n");
	}

    /*sergey: moved from rcRunTypeOption::currentRunType, otherwise it called every time when currentRunType() called */
    /* NOTE: maybe need to use option_->baseWidget() instead of this->baseWidget(), or it does not matter ??? */
    {
      char cmd[100];
      sprintf(cmd,"c:%s",currentruntype);
#ifdef USE_CREG
      printf("CEDIT 4: >%s<\n",cmd);
      coda_Send(XtDisplay(this->baseWidget()),(char *)"CEDIT",cmd);
      coda_Send(XtDisplay(this->baseWidget()),(char *)"ALLROCS",cmd);
#endif
    }



  }
}



void
rcRunTypeDialog::popup (void)
{
  option_->setAllEntries ();
  XcodaFormDialog::popup (); // popup run type configuration dialog
}


void
rcRunTypeDialog::okCallback (Widget, XtPointer data, XmAnyCallbackStruct *)
{
  rcRunTypeDialog* dialog = (rcRunTypeDialog *)data;

#ifdef _CODA_DEBUG
  printf("rcRunTypeDialog::okCallback\n");fflush(stdout);
#endif

  dialog->popdown ();
  dialog->configure (); /*sergey: triggers rcServer activity*/
}


void
rcRunTypeDialog::cancelCallback (Widget parent, XtPointer data, XmAnyCallbackStruct *)
{
  rcRunTypeDialog* dialog = (rcRunTypeDialog *)data;
  dialog->popdown ();
}


void
rcRunTypeDialog::reportErrorMsg (char* error)
{
  if (!errDialog_)
  {
    errDialog_ = new XcodaErrorDialog (_w,"runTypeError", "Configuration Error");
    errDialog_->init ();
  }

  if (errDialog_->isMapped ()) errDialog_->popdown ();
  errDialog_->setMessage (error);
  errDialog_->popup ();
}


void
rcRunTypeDialog::configureCallback (int status, void* arg, daqNetData* data)
{
  rcRunTypeDialog* obj = (rcRunTypeDialog *)arg;

  if (status != CODA_SUCCESS) obj->reportErrorMsg ((char *)"Configuring a run failed !!!");
}



