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
//      Implementation of Download Command Button
//
// Author:  
//      Jie Chen
//      CEBAF Data Acquisition Group
//
// Revision History:
//   $Log: rcDownload.cc,v $
//   Revision 1.2  1998/06/18 12:20:36  heyes
//   new GUI ready I think
//
//   Revision 1.1.1.1  1996/10/11 13:39:24  chen
//   run control source
//
//

#include <rcNetStatus.h>
#include <rcButtonPanel.h>
#include <rcAudioOutput.h>
#include "rcDownload.h"

/*sergey: for popup gui */
#include <rcInfoPanel.h>
#include <rcRunConfigDialog.h>

#define RC_DOWNLOAD_NAME (char *)" Download "
#define RC_DOWNLOAD_MSG  (char *)"Download object code"

/*
#define DEBUG_MSGS
*/

rcDownload::rcDownload (Widget parent, rcButtonPanel* panel,
			rcClientHandler& handler)
:rcComdButton (parent, RC_DOWNLOAD_NAME, RC_DOWNLOAD_MSG, panel, handler, (char *)"B")
{
  printf ("rcDownload::rcDownload: Create rcDownload Class Object\n");
  // empty
}

rcDownload::~rcDownload (void)
{
  printf ("rcDownload::~rcDownload: Delete rcDownload Class Object\n");
  // empty
}

void
rcDownload::doit (void)
{

#ifdef DEBUG_MSGS
  printf("rcDownload::doit reached\n");
#endif

  rcAudio ((char *)"download a run");

  assert (stWin_);

  /* get network handler first */
  rcClient& client = netHandler_.clientHandler ();


	/*sergey*/
    assert (infoPanel_);
    infoPanel_->runConfigDialog()->popup ();

	/* !!! SERGEY: following DADOWNLOAD command need to be sent from runConfigDialog after popdown,
	  in that case it will be called AFTER we've got config file set */


#if 0

  daqData data ("RCS", "command", (int)DADOWNLOAD);
  /* send DADOWNLOAD command to daqRun.cc; our callback function 'downloadCallback' will be called
     in the END on download transition */
  if(client.sendCmdCallback(DADOWNLOAD, data,
			                (rcCallback)&(rcDownload::downloadCallback),
			                (void *)this) != CODA_SUCCESS)
  {

#ifdef DEBUG_MSGS
    printf("rcDownload::doit 1\n");
#endif
    reportErrorMsg ((char *)"Cannot send download command to the server.");
    rcAudio ("cannot send download command");
  }
  else
  {



#ifdef DEBUG_MSGS
    printf("rcDownload::doit 2\n");
#endif
    stWin_->start ();
#ifdef DEBUG_MSGS
    printf("rcDownload::doit 3\n");
#endif
    bpanel_->deactivateTransitionPanel();
#ifdef DEBUG_MSGS
    printf("rcDownload::doit 4\n");
#endif
  }

#endif /* if 0 */

}

void
rcDownload::undoit (void)
{
#ifdef DEBUG_MSGS
  printf("rcDownload::undoit reached\n");
#endif
  // empty
}

void
rcDownload::downloadCallback (int status, void* arg, daqNetData* data)
{
  rcDownload* obj = (rcDownload *)arg;

/*#ifdef DEBUG_MSGS*/
  printf("---------------- rcDownload::downloadCallback: everything downloaded !\n");
/*#endif*/

  /* stop netStatus updater */
  obj->stWin_->stop ();
  obj->bpanel_->activateTransitionPanel ();

  if (status != CODA_SUCCESS && status != CODA_IGNORED)
  {
    obj->reportErrorMsg ((char *)"Downloading a run failed !!!\n");
    rcAudio ((char *)"downloading failed");
  }  

}





