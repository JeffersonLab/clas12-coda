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
//      RunControl Help Context Class
//
// Author:  
//      Jie Chen
//      CEBAF Data Acquisition Group
//
// Revision History:
//   $Log: rcHelpContext.cc,v $
//   Revision 1.4  1998/04/08 18:31:16  heyes
//   new look and feel GUI
//
//   Revision 1.3  1997/11/24 21:32:20  rwm
//   Added missing new line.
//
//   Revision 1.2  1996/12/04 18:32:27  chen
//   port to 1.4 on hp and ultrix
//
//   Revision 1.1.1.1  1996/10/11 13:39:28  chen
//   run control source
//
//
#include <rcHelpOverview.h>
#include <rcHelpTextW.h>
#include "rcHelpContext.h"

#define RC_MAX_HELPDOC_LEN 4096

rcHelpTextW* rcHelpContext::helpDisp_ = 0;

char* rcHelpContext::helpdoc_[]=
{
  (char *)"To connect to a Run Control network server:\n\nA dialog box appears after the $Connect$ button has been pressed. Select a database and a session name for a server and click $open$ or $spawn$ button on the dialog box to connect or to start a server. Click on $cancel$ button results no action. An environment variable $MYSQL_HOST$ effects the searching and connecting process to the server.",
  (char *)"To disconnect from a Run Control server:\n\nA warning dialog box appears after the $disconnect$ button has been pressed. Click on $Ok$ button on the dialog to disconnect this GUI from the server.",
  (char *)"To close a Run Control GUI:\n\nA warning dialog appears after the $Close$ menu entry in the $File$ menu has been selected. Click on $Ok$ button of the dialog box to quit this Run Control GUI to leave the Run Control server and all remote processes running.",
  (char *)"To exit from a Run Control GUI and a Run Control server:\n\nA warning dialog appears after the $Exit$ menu entry in the $File$ menu has been selected. Click on $Ok$ button of the dialog box to kill the Run Control server, all remote processes and this Run Control GUI.",
  (char *)"To acquire mastership of a Run Control:\n\nThe lock symbol at the bottom right corner symbolizes the mastership over connected Run Control server. A single unlocked lock denotes that every Run Control GUI has equal access to the server. A single locked lock denotes that one of the other GUIs is the master. A lock with a key denotes that this GUI is the master. Click on the lock symbol to change the status of mastership",
  (char *)"To configure a run:\n\nAn option menu with all selectable run types appears after the $Configure$ button has been pressed. Select one of the run type from the option menu and click $Ok$ button on the dialog. The selected run type will be configured into the Run Control server",
  (char *)"To download a run:\n\nAll remote or local processes in the select run type will undergo download state transition after $Download$ button has been pressed. The processes of ROC will be asked to dynamically load user specified object code. All the other processes with type other than ROC will just go through internal state transition",
  (char *)"To reset a run:\n\nThis is last resort to restore the Run Control server into a consistent dormant state.",
  (char *)"To set event/data limit:\n\nEnter desired event limit or data limit (in unit of Kbytes) in the text entry fields and type return. When number of events or number of data (in Kbytes) reach the event limit or data limit, the Run Control server will end the run automatically. Note: all the Run Control GUIs share one event/data limit. ",
  (char *)"To set run number:\n\nEnter desired run number in the text field entry for the run number.",
  (char *)"To report bug:\n\nTo report any bugs or suggestion for improvements, send e-mail to chen@cebaf.gov"
};


rcHelpContext::rcHelpContext (char* name, int active, int index,
			      rcHelpOverview* parent)
:codaPbtnComd (name, active), parent_ (parent), index_ (index)
{
#ifdef _TRACE_OBJECTS
  printf ("         Create rcHelpContext Class Object\n");
#endif
}

rcHelpContext::~rcHelpContext (void)
{
#ifdef _TRACE_OBJECTS
  printf ("         Delete rcHelpContext Class Object\n");
#endif
}

void
rcHelpContext::doit (void)
{
  char text[RC_MAX_HELPDOC_LEN];
  sprintf(text,"#index%d\n",index_);
  displayHelpText (text);
}

void
rcHelpContext::undoit (void)
{
  // empty
}

void
rcHelpContext::processText (char* src, char* des)
{
  char* p = src;
  char* q = des;

  int i = 0;

  while (*p != '\0') {
    if (*p == '$')
      *q = '"';
    else
      *q = *p;
    q++; p++;
    i++;
    if (i >= RC_MAX_HELPDOC_LEN) {
      fprintf (stderr, "Fatal: Overflow internal data buffer\n");
      break;
    }
  }
  *q = '\0';
}

void
rcHelpContext::displayHelpText (char* text)
{
  if (rcHelpContext::helpDisp_ == 0) {
    rcHelpContext::helpDisp_ = new rcHelpTextW (parent_->dialogBaseWidget(),
						(char *)"helpTextWindow",
						(char *)"Run Control Information");
  }
  rcHelpContext::helpDisp_->setHelpText (text);
}



