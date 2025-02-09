
/* rcRocMenuWindow.cc */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>

#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PanedW.h>
#include <Xm/XmP.h>
#include <Xm/RowColumn.h>
#include <Xm/PushBG.h>
#include <Xm/SeparatoG.h>
#include <Xm/LabelG.h>

#include <codaComd.h>
#include <codaComdXInterface.h>
#include <codaComdList.h>
#include <codaSepComd.h>
#include <XcodaMenuBar.h>
#include <XcodaXpmLabel.h>

#include <rcButtonPanel.h>
#include <rcInfoPanel.h>
#include <rcMsgWindow.h>
#include <rcHelpMsgWin.h>
#include <rcNetStatus.h>
#include <rcMastership.h>
#include <rcComdOption.h>
#include <XmTabs.h>
/*#include <RCLogo.xpm>*/
#ifdef USE_TK
#include <tcl.h>
#include <tk.h>
#endif
#ifdef USE_CREG
#include <codaRegistry.h>
#endif

#include "rcExit.h"
#include "rcClose.h"
#include "rcRocConfig.h" /*sergey*/

#include "rcButtonFeedBack.h"
#include "rcDisplayMsg.h"
#include "rcOnLine.h"
#include "rcAnaLogDialog.h"
#include "rcUpdateIntervalDialog.h"

#include "rcAudioOption.h"
#include "rcWidthOption.h"
#include "rcRcsMsgToDbase.h"
#include "rcTokenIButton.h"

#include "rcCompBootDialog.h"
#include "rcMonitorParmsDialog.h"
#include "rcZoomButton.h"
#include "rcHelpOverview.h"
#include "rcHelpAbout.h"
#include <XcodaErrorDialog.h>




#include "Editor.h"
#include "Editor_configSel.h"
#include "Editor_layout.h"
#include "Editor_pixmap.h"
#include "Editor_graph.h"
#include "Editor_miscinfo.h"
#include "Editor_database.h"
#include "Editor_widget_poll.h"
#include "Editor_cmd_btns.h"
#include "Editor_graph_cmd.h"
#include "Editor_zoom.h"
#include "Editor_icon_box.h"
#include "Editor_drawing.h"
#include "Editor_converter.h"

#include "rcRocMenuWindow.h"

#ifdef NEED_BZERO
extern "C" {
  void bzero(void *,int);
}

#endif

/*
static int pids_[200]; // store process ID
*/

/*global*/
int logging;
/*global*/

extern "C" int ctermlib(int argc, char *argv[]);

/* 'daqComp' described in codaedit.s/Editor.h, and MAX_NUM_COMPS defined there */
/*static*/ int ncomp_;
static daqComp comp[MAX_NUM_COMPS];
static char *type_name[MAX_NUM_COMPS];
static char *bg_name[MAX_NUM_COMPS];
/*static*/ pid_t proc_id[MAX_NUM_COMPS];

static Widget xtermsLabel[200];


rcRocMenuWindow *menu_window;

rcRocMenuWindow::rcRocMenuWindow (Widget parent, char* name, rcClientHandler& handler) : XcodaMenuWindow (parent, name), netHandler_ (handler)
{
#ifdef _TRACE_OBJECTS
  printf ("         Creating rcRocMenuWindow Class Object\n");
#endif

  // register this panel to net handler
  netHandler_.addPanel (this);
  exit_ = 0;
  close_ = 0;

  config_ = 0;

  feedBack_ = 0;
  dispMsg_ = 0;
  online_ = 0;
  anaLogButton_ = 0;
  updateInterval_ = 0;
  bootButton_ = 0;
  monParmsButton_ = 0;
  zoomButton_ = 0;
  netStatus_ = 0;
  rcipanel_ = 0;
  helpMsgWin_ = 0;
  msgw_ = 0;

  audio_ = 0;
  Owidth_ = 0;
  serverMsgToDbase_ = 0;
  tokenIButton_ = 0;

  /*
  ::bzero(pids_,sizeof(pids_));
  setpgid(getpid(),getpid());
  */

  ::bzero(xtermsFrame_,sizeof(xtermsFrame_)); /* will check if it zero or not in 'handle_tab' */

  ncomp_ = 0;

  menu_window = this;

#ifdef _TRACE_OBJECTS
  printf ("         Created rcRocMenuWindow Class Object\n");
#endif
}

rcRocMenuWindow::~rcRocMenuWindow (void)
{
#ifdef _TRACE_OBJECTS
  printf ("         Delete rcRocMenuWindow Class Object\n");
#endif
  // remove this panel from nethandler
  netHandler_.removePanel (this);

//   delete exit_;
//   delete close_;
//   delete config_;
//   delete feedBack_;
//   delete dispMsg_;
//	 delete online_;
//   delete anaLogButton_;
//   delete updateInterval_;
//   delete bootButton_;
//   delete monParmsButton_;
//   delete zoomButton_;
//   delete helpOverview_;
//   delete helpAbout_;


  delete audio_;
  delete Owidth_;
  delete serverMsgToDbase_;
  //delete tokenIButton_;


  // delete network status updater
  if (netStatus_ != 0)
    delete netStatus_;
}





/* called by XcodaMenuWindow's init() method */
void
rcRocMenuWindow::createMenuPanes (void)
{
  codaComdList* list;
  rcComdOption* option = rcComdOption::option ();

  printf("rcRocMenuWindow::createMenuPanes reached\n");


  /*************/
  /* file menu */

  config_ = new rcRocConfig ((char *)"Config", 1, (char *)"Alt<Key>c", (char *)"Alt/C", netHandler_);
  close_ = new rcClose ((char *)"Close", 1, (char *)"Alt<Key>c", (char *)"Alt/C", netHandler_);
  exit_ = new rcExit ((char *)"Exit", 1, (char *)"Alt<Key>q", (char *)"Alt/Q", netHandler_);

  list = new codaComdList ();
  list->add (config_);
  list->add (close_);
  list->add (exit_);

  MenuBar->addRegMenuEntry (list, (char *)"File", 'F');
  delete list;


  /*******************/
  /* preference menu */

  feedBack_ = new rcButtonFeedBack ((char *)"Button feedback", 1, (char *)"Alt<Key>b",
				    (char *)"Alt/B", 1, netHandler_);

  if (option->audio ())
    audio_ = new rcAudioOption ((char *)"Audio messages", 1, (char *)"Alt<Key>a",
				(char *)"Alt/A", 1, netHandler_);
  else
    audio_ = new rcAudioOption ((char *)"Audio messages", 1, (char *)"Alt<Key>a",
				(char *)"Alt/A", 0, netHandler_);

  Owidth_ = new rcWidthOption ((char *)"Toggle width", 1, (char *)"Alt<Key>w",
			      (char *)"Alt/W", 0, netHandler_);
  if (option->reportMsg ())
    dispMsg_ = new rcDisplayMsg ((char *)"Show server messages", 1, (char *)"Alt<Key>s",
				 (char *)"Alt/S", 1, netHandler_);
  else
    dispMsg_ = new rcDisplayMsg ((char *)"Show server messages", 1, (char *)"Alt<Key>s",
				 (char *)"Alt/S", 0, netHandler_);


  serverMsgToDbase_ = new rcRcsMsgToDbase ((char *)"Log Server Message", 0,
					   (char *)"Alt<Key>m", (char *)"Alt/M", 0,
					   netHandler_);


  online_ = new rcOnLine ((char *)"Online", 0, (char *)"Alt<Key>l",
			  (char *)"Alt/B", 1, netHandler_);
  list = new codaComdList ();
  list->add (feedBack_);
  list->add (dispMsg_);

  list->add (audio_);
  list->add (serverMsgToDbase_);

  list->add (Owidth_);
  list->add (online_);
  MenuBar->addRegMenuEntry (list, (char *)"Preference", 'P');
  delete list;




  /******************************************************************************/
  /* before return, setup pointers of dynamic panels to related command buttons */

  exit_->netStatusPanel (netStatus_);
  exit_->infoPanel      (rcipanel_);

  /* apply the same technique to close command */
  close_->netStatusPanel (netStatus_);
  close_->infoPanel      (rcipanel_);

  config_->netStatusPanel (netStatus_);
  config_->infoPanel      (rcipanel_);

  /* attach help message window to feedback command */
  feedBack_->helpMsgWindow (helpMsgWin_);

  /* attach message window to display message command */
  dispMsg_->msgWindow (msgw_);
}


/*
extern int root_height;
*/

void
rcRocMenuWindow::handle_tab (Widget w, XtPointer data, XtPointer calldata)
{
  Arg arg[20];
  int ac = 0;
  int state, i, j, tab;
  char* tmp;
  rcRocMenuWindow *self = (rcRocMenuWindow *) data;

  /* get tab data, start on tab 0 */
  static int curr = 0;

  printf("rcRocMenuWindow::handle_tab reached\n");fflush(stdout);

  if (/*(int) sergey*/calldata == 0) return;

  printf("\nrcRocMenuWindow::handle_tab reached, calldata=%d\n",calldata);

  printf("rcRocMenuWindow::handle_tab: old tab number = %d\n",(int)curr);
  printf("rcRocMenuWindow::handle_tab: hide frames %d and %d\n",(int)curr*2,(int)curr*2+1);

  /* hide 'xtermsFrame' related to old tab (currently 2 frames per tab) */
  if(self->xtermsFrame_[curr*2])   XtUnmanageChild(self->xtermsFrame_[curr*2]);
  if(self->xtermsFrame_[curr*2+1]) XtUnmanageChild(self->xtermsFrame_[curr*2+1]);

#ifdef Linux_x86_64
  curr += (int64_t)calldata;
#else
  curr += (int)calldata;
#endif
  /* now 'curr' contains new tab number starting from 0 */  


  tab = curr;
  printf("rcRocMenuWindow::handle_tab: new tab number = %d\n",(int)curr);
  printf("rcRocMenuWindow::handle_tab: show frames %d and %d\n",(int)curr*2,(int)curr*2+1);


  /* show 'xtermsFrame' related to new tab */
  if(self->xtermsFrame_[curr*2])   XtManageChild(self->xtermsFrame_[curr*2]);
  if(self->xtermsFrame_[curr*2+1]) XtManageChild(self->xtermsFrame_[curr*2+1]);


  printf("rcRocMenuWindow::handle_tab: self->numTabs_=%d tab=%d\n",self->numTabs_,tab);

  ac = 0;
  XtSetArg (arg[ac], XtNlefttabs, tab); ac++;
  XtSetArg (arg[ac], XtNrighttabs, self->numTabs_ - tab); ac++;
  XtSetValues (self->rtab_, arg, ac);
}

static void childHandler(int sig)
{
  int status, pid;
  /*
  pid = wait(&status);
  printf("process %d exit with status %d (core dump %d)\n",pid,WEXITSTATUS(status),WCOREDUMP(status));
  */
}

void 
rcRocMenuWindow::destroyHandler(Widget w,void *data,XEvent *eventPtr,Boolean *b)
{
  rcRocMenuWindow *self = (rcRocMenuWindow *) data;
  char temp2[100];

  if (eventPtr->type == DestroyNotify) {
    int ix;
    /*    for (ix=0;ix<30;ix++) {
      if (self->xtermsFrame_[ix] == w) {
	printf("program \"%s\" has unexpectedly quit\n", self->tabLabels_[ix]);
	
	if (strcmp(self->tabLabels_[ix],"cedit") == 0) {
	  sprintf (temp2,"(echo \"start cedit\"; sleep 1; %s/codaedit )&",getenv("CODA_BIN"));
	  system(temp2);
    	}

	if (strcmp(self->tabLabels_[ix],"dbedit") == 0) {
	  sprintf (temp2,"(echo \"start dbedit\";sleep 1; %s/common/scripts/dbedit )&",getenv("CODA"));
	  system(temp2);
    	}

	if (strcmp(self->tabLabels_[ix],"ddmon") == 0) {
	  sprintf (temp2,"(echo \"start ddmon\";sleep 1;  export DD_NAME; DD_NAME=%s;$CODA_BIN/ddmon )&",self->netHandler_.exptname());
	  system(temp2);
	}
      }
      }
    */
  }
}

void
rcRocMenuWindow::crossEventHandler (Widget, XtPointer clientData, XEvent* event, Boolean)
{
  rcRocMenuWindow* obj = (rcRocMenuWindow *)clientData;
  XCrossingEvent* cev = (XCrossingEvent *)event;

  if (obj->helpMsgWin_)
  {
    if (cev->type == EnterNotify) obj->helpMsgWin_->setMessage ((char *)"press a button to change page");
    else                          obj->helpMsgWin_->eraseMessage ();
  }
}


void
rcRocMenuWindow::createXterms (Widget widget, char *name)
{
  Widget wid;
  Widget workarea;
  int ac, ii, ix, iy, xpos, ypos, dx, dy;
  Arg arg[20];
  Dimension width, height, height1, height2, hmax, hmin;
  char tmp[200];

  /******************************************************************************************************/
  /* here we'll set xterms[] size, and it will define panedwindow size and size of entire 'rocs' window */

  /* get 'rframe_' size */
  XtVaGetValues (rframe_, XmNwidth, &width, XmNheight, &height, NULL);

  /* if 'width' bigger then 2049, we assume dual head monitors */
  if(width>2049) width = width / 2;

  printf("+++++++++++++++ screen: width=%d height=%d\n",width,height);


  /* get menu tab height, and double it since we have 2 of them (assuming top one the same as bottom, should get it directly ...) */
  XtVaGetValues (rtab_, XmNheight, &height1, NULL);
  printf("+++++++++++++++ menu: height1 = %d\n",height1);
  height1 = height1 * 2; /* 


  /* xterms[] width will be half of full screen, and height 80% minus menu areas divided by 5 */
  width = width / 2;
  height = ((height*4/5)-height1)/5;
  width = width*4/5;
  hmax = height*2;
  hmin = height/2;
  printf("+++++++++++++++ xterms: width=%d height=%d hmax=%d hmin=%d\n",width,height,hmax,hmin);

  for(ii=0; ii<5; ii++)
  {
    ac = 0;
    /*XtSetArg (arg[ac], XmNequalSize,  True); ac++; */
    /*XtSetArg (arg[ac], XmNallowResize,  True); ac++; */
    /*XtSetArg (arg[ac], XmNskipAdjust,  False); ac++; default */
    /*XtSetArg (arg[ac], XmNresizeToPreferred,  True); ac++; */
    /*XtSetArg (arg[ac], XmNpreferredPaneSize, height); ac++; */

    /* following effects panes resizing process */
    //XtSetArg (arg[ac], XmNpaneMaximum, hmax); ac++;
    //XtSetArg (arg[ac], XmNpaneMinimum, hmin); ac++;

    XtSetArg (arg[ac], XtNwidth, width); ac++;
    XtSetArg (arg[ac], XtNheight, height); ac++;
    printf("INITIALLY xterms[%d] width=%d height=prefsize=%d\n",nxterms,width,height);

    xterms[nxterms] = XtCreateManagedWidget ("frame", xmFrameWidgetClass, widget, arg, ac );

    XtVaSetValues (xterms[nxterms], XmNpaneMaximum, hmax, XmNpaneMinimum, hmin, NULL);
    /*XtVaSetValues (xterms[nxterms], XtNwidth, width, XtNheight, height, NULL);*/



	/*label*/

    //strcpy(xtermsTitle[nxterms],comp[nxterms].comp_name);

    ac = 0;
    XtSetArg (arg[ac], XmNframeChildType, XmFRAME_TITLE_CHILD); ac++;
    xtermsLabel[nxterms] = XmCreateLabelGadget (xterms[nxterms], (char *)"title will be here", arg, ac);
    XtManageChild (xtermsLabel[nxterms]);


    /*workarea*/

    sprintf(tmp,"%s%02d_WINDOW",name,ii);
    printf("CREG11 name >%s<\n",tmp);

    ac = 0;
    XtSetArg (arg[ac], XmNframeChildType, XmFRAME_WORKAREA_CHILD); ac++;
    workarea = wid = XtCreateManagedWidget (tmp, xmFrameWidgetClass, xterms[nxterms], arg, ac );
    XtManageChild (workarea);




#ifdef USE_CREG
	/*CODASetAppName (XtDisplay(wid), XtWindow(wid), tmp);*/
    codaSendInit(wid, tmp); /* does embedding, and triggers 'motifHandler'->'resizeHandler' calls in codaRegistry */ 
    /*codaRegisterMsgCallback((void *)messageHandler); need it ???????????????????????????????????? */
    /*XStoreName(XtDisplay(wid),XtWindow(wid),tmp);  need this ??????????????????? */
#endif

    nxterms ++;
  }

  return;
}








/* for example in runcontrol.c: 'menW->createTabFrame("codaedit",0)' */

int
rcRocMenuWindow::createTabFrame (char *name, int pid, int *firstxtermframe, int *nxtermframes)
{
  Widget widget;
  Arg arg[20];
  int ltabs, rtabs, ac;
  
  numTabs_ ++; /* tabs counter, it does NOT include first tab created in rcRocMenuWindow::createMenuWindow() !!! */
  

  printf("rcRocMenuWindow::createTabFrame: numTabs_=%d\n",numTabs_);
  /*
  pids_ [numTabs_] = pid;
  */


  /**************************/
  /* create new frame 'rocs'*/

  ac = 0;
  XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;

  /* thin box around area */
  XtSetArg (arg[ac], XmNshadowType, XmSHADOW_ETCHED_OUT); ac++;
  XtSetArg (arg[ac], XmNshadowThickness, 2); ac++;

  /* use xmFormWidgetClass
  widget = xtermsFrame_[numTabs_*2] = XtCreateManagedWidget ("tabChild", xmFrameWidgetClass, rframe_, arg, ac);
  */

  /* left side */
  printf("rcRocMenuWindow::createTabFrame: left side frame number %d\n",numTabs_*2);
  ac = 0;
  XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  xtermsFrame_[numTabs_*2] = XtCreateManagedWidget ( "rocs", xmPanedWindowWidgetClass, rframe_, arg, ac);
  XtManageChild (xtermsFrame_[numTabs_*2]);


  /* right side */
  printf("rcRocMenuWindow::createTabFrame: right side frame number %d\n",numTabs_*2+1);
  ac = 0;
  XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  xtermsFrame_[numTabs_*2+1] = XtCreateManagedWidget ( "rocs", xmPanedWindowWidgetClass, rframe_, arg, ac);
  XtManageChild (xtermsFrame_[numTabs_*2+1]);


  /* do nothing
  XtUnmapWidget(rframe_);
  XSync(XtDisplay(rframe_),False);
  XtMapWidget(rframe_);
  */


  /*???
  XtAddEventHandler(widget, SubstructureNotifyMask, False, destroyHandler, (XtPointer) this);
  */

  /**********************/
  /* register new frame */

  /* do not need it since we are registering small widgets ???
  char tmp[200];
  sprintf(tmp,"%s_WINDOW",name);
  printf("CREG1 name >%s<\n",tmp);
#ifdef USE_CREG
  CODASetAppName (XtDisplay(xtermsFrame_[numTabs_]),XtWindow(xtermsFrame_[numTabs_]),tmp);
#endif
  XStoreName(XtDisplay(xtermsFrame_[numTabs_]),XtWindow(xtermsFrame_[numTabs_]),tmp);
  */




  /*****************************************************/
  /* create new tab frame inside existing 'rtab_' area */  
  
  tabLabels_[numTabs_] = strdup(name);

  ac = 0;
  XtSetArg (arg[ac], XtNlefttabs, &ltabs); ac++;
  XtSetArg (arg[ac], XtNrighttabs, &rtabs); ac++;
  XtGetValues (rtab_, arg, ac);
  printf("rcRocMenuWindow::createTabFrame: numTabs_=%d ltabs=%d rtabs=%d (numTabs_-ltabs)=%d\n",numTabs_,ltabs,rtabs,numTabs_-ltabs);

  ac = 0;
  XtSetArg (arg[ac], XtNrighttabs, numTabs_-ltabs); ac++;
  XtSetArg (arg[ac], XtNlabels, tabLabels_); ac++;
  XtSetValues (rtab_, arg, ac);

  XtUnmapWidget(rtab_);
  XSync(XtDisplay(rtab_),False);
  XtMapWidget(rtab_);
  
  *firstxtermframe = numTabs_*2;
  *nxtermframes = 2;


  return(0);
}




extern Widget toplevel;










/* called from motif/src.s/XcodaMenuWindow.cc */
/* creates main menu window and first 'rocs' tab; more tabs can be created by calling createTabFrame() */
Widget
rcRocMenuWindow::createMenuWindow (Widget parent)
{
  Widget widget;
  Arg arg[20];
  int ac;
  Dimension width, height;
  XtActionsRec   rec;

  rcComdOption* option = rcComdOption::option ();
  bootall_ = 0;

  // create all widgets

  ac = 0;
  XtSetValues (MenuBar->baseWidget(), arg, ac);

  /* set initial sizes 80% of screen */
  width = (WidthOfScreen(XtScreen(parent)) * 5) / 5;
  height = (HeightOfScreen(XtScreen(parent)) * 5) / 5;

  ac = 0;
  XtSetArg (arg[ac], XtNwidth, width); ac++;
  XtSetArg (arg[ac], XtNheight, height); ac++;
  XtSetValues (parent, arg, ac);
  XtVaGetValues (parent, XtNwidth, &width, XtNheight, &height, NULL);
  printf("?????????? %d %d\n",width,height);


  /* 'formouter' widget will be returned from this function */
  ac = 0;
  XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM/*XmATTACH_FORM*/); ac++;
  XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_WIDGET/*XmATTACH_WIDGET*/); ac++;
  Widget formouter = XtCreateManagedWidget ("rcFormOuter", xmFormWidgetClass, parent, arg, ac);



  ac = 0;
  XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM/*XmATTACH_FORM*/); ac++;
  XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_WIDGET/*XmATTACH_WIDGET*/); ac++;
  XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM/*XmATTACH_FORM*/); ac++;
  XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM/*XmATTACH_FORM*/); ac++;
  rform_ = XtCreateManagedWidget ("rcform", xmFormWidgetClass, formouter, arg, ac);
  

  /* reset the number of tabs */
  numTabs_ = 0;

  /* reset the number of xterms */
  nxterms = 0;



  /*******************/
  /* create tabs area */

  tabLabels_[0] = strdup("rocs0"); /*name of the first rocs tabs, it will be more of them created in rocs.cc*/

  ac = 0;
  XtSetArg (arg[ac], XtNlabels, tabLabels_); ac++;
  XtSetArg (arg[ac], XtNtabWidthPercentage, 0); ac++;
  XtSetArg (arg[ac], XtNlefttabs, 0); ac++;
  XtSetArg (arg[ac], XtNrighttabs, 0); ac++;
  XtSetArg (arg[ac], XtNorientation, XfwfUpTabs); ac++;
  XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNtopOffset, 5); ac++;
  XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNleftOffset, 5); ac++;
  XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNrightOffset, 5); ac++;
  XtSetArg (arg[ac], XmNheight, 30); ac++;
  XtSetArg (arg[ac], XmNshadowType, XmSHADOW_ETCHED_OUT); ac++;
  XtSetArg (arg[ac], XmNshadowThickness, 2); ac++;

  rtab_ = XtCreateManagedWidget ("rocTab", xmTabsWidgetClass, rform_, arg, ac);

  // add all callbacks
  XtAddCallback (rtab_, XtNactivateCallback, handle_tab, (XtPointer)this); /* 'handle_tab' switches tabs when pressed */
  XtAddEventHandler (rtab_, EnterWindowMask | LeaveWindowMask, FALSE, (XtEventHandler)&(crossEventHandler), (XtPointer)this);



  /************************************************************/
  /* create frame area, positioning it just below tab 'rtab_' */
  /* NOTE: frame area is the same for all tabs, contents switched in rcRocMenuWindow::handle_tab !!?? */

  ac = 0;
  XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (arg[ac], XmNtopWidget, rtab_); ac++;
  XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNmarginWidth, 2); ac++;
  XtSetArg (arg[ac], XmNmarginHeight, 2); ac++;
  rframe_ = XtCreateWidget ("rocFrame", xmFormWidgetClass, rform_, arg, ac);
  


  XtManageChild (formouter);
  XtManageChild (rform_);
  XtManageChild (rtab_);
  XtManageChild (rframe_);



  /* do not increase: it does not count current (first) tab 
  numTabs_++;
  */


  /* create xmPaned, xterms will be it's children */

  /* left side */
  ac = 0;
  //XtSetArg (arg[ac], XmNequalSize,  True); ac++;

  //XtSetArg (arg[ac], XmNallowResize,  True); ac++;
  //XtSetArg (arg[ac], XmNresizeToPreferred, False); ac++;
  //XtSetArg (arg[ac], XmNskipAdjust, False); ac++;

  //XtSetArg (arg[ac], XmNresizePolicy, XmRESIZE_ANY); ac++;
  //XtSetArg (arg[ac], XmNrefigureMode, True); ac++;

  XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  xtermsFrame_[0] = XtCreateManagedWidget ( "rocs", xmPanedWindowWidgetClass, rframe_, arg, ac);
  XtManageChild (xtermsFrame_[0]);


  /* right side */
  ac = 0;
  XtSetArg (arg[ac], XmNallowResize,  True); ac++;
  XtSetArg (arg[ac], XmNresizeToPreferred, False); ac++;
  XtSetArg (arg[ac], XmNskipAdjust, False); ac++;

  XtSetArg (arg[ac], XmNresizePolicy, XmRESIZE_ANY); ac++;
  XtSetArg (arg[ac], XmNrefigureMode, True); ac++;

  XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  xtermsFrame_[1] = XtCreateManagedWidget ( "rocs", xmPanedWindowWidgetClass, rframe_, arg, ac);
  XtManageChild (xtermsFrame_[1]);




  return(formouter);
}




void
rcRocMenuWindow::config (int status)
{
  if (status != DA_NOT_CONNECTED)
  {
    serverMsgToDbase_->activate ();
  }
  else
  {
    serverMsgToDbase_->deactivate ();
  }

  if (status >= DA_CONFIGURED) {
    //    zoomButton_->activate ();
  }
  else {
    //    zoomButton_->deactivate ();
  }

  if (status >= DA_DOWNLOADED)
    online_->activate ();
  else
    online_->deactivate ();  
}

void
rcRocMenuWindow::configOnlineOption (int online)
{
  if (online)
    online_->setState (1);
  else
    online_->setState (0);
}

void
rcRocMenuWindow::configUpdateInterval (int interval)
{
  updateInterval_->setUpdateInterval (interval);
}

void
rcRocMenuWindow::configBoot ()
{
  bootButton_->popdown();
  bootButton_->popup();
}

void
rcRocMenuWindow::configMonParms ()
{
  monParmsButton_->popdown();
  monParmsButton_->popup();
}

void
rcRocMenuWindow::configTokenInterval (int interval)
{
  tokenIButton_->setTokenInterval (interval);
}

void
rcRocMenuWindow::configRcsMsgToDbase (int state)
{
  serverMsgToDbase_->setState (state);
}

const Widget
rcRocMenuWindow::dialogBaseWidget (void)
{
  assert (rcipanel_);
  return rcipanel_->baseWidget ();
}

void
rcRocMenuWindow::reportErrorMsg (char* msg)
{
  if (rcRocMenuWindow::errDialog_ == 0) {
    rcRocMenuWindow::errDialog_ = new XcodaErrorDialog (dialogBaseWidget(), "comdError", "Error Dialog");
    rcRocMenuWindow::errDialog_->init ();
  }
  rcRocMenuWindow::errDialog_->setMessage (msg);
  rcRocMenuWindow::errDialog_->popup ();
}









/*******************************************************/
/* sergey: following copied from codaedit and modified */

static editorConfigSel iconfigSel;

void
rcRocMenuWindow::ConfigSelPopup (void)
{
  int  status;
  int  i;
  Arg  arg[10];
  int  ac = 0;
  XmString t;

  printf("rcRocMenuWindow::ConfigSelPopup reached\n");

  if (iconfigSel.managed_) ConfigSelPopdown ();
  
  /* remove old database information */
  for (i = 0; i < iconfigSel.numConfigs_; i++) free (iconfigSel.configs_[i]);

  /* get all database names */
  status = listAllConfigs (iconfigSel.configs_, &(iconfigSel.numConfigs_));

  if (status == 0)
  {
    if (iconfigSel.numConfigs_ > 0)
    {
      for (i = 0; i < iconfigSel.numConfigs_; i++)
      {
	    ac = 0;
	    t = XmStringCreateSimple (iconfigSel.configs_[i]);
	    XtSetArg (arg[ac], XmNlabelString, t); ac++;
	    XtSetValues (iconfigSel.pushb[i], arg, ac);
	    ac = 0;
	    XmStringFree (t);
	    XtManageChild (iconfigSel.pushb[i]);
      }
    }
    else
    {
      t = XmStringCreateSimple ((char *)"no runtypes");
      XtSetArg (arg[ac], XmNlabelString, t); ac++;
      XtSetValues (iconfigSel.pushb[0], arg, ac);
      ac = 0;
      XmStringFree (t);
      /* disable ok button */
      XtSetSensitive (iconfigSel.ok_, FALSE);
    }
  }
  else
  {
    iconfigSel.numConfigs_ = 0;
  }  
    
  XtManageChild (iconfigSel.w_);
  XtPopup (XtParent (iconfigSel.w_), XtGrabNone);
  iconfigSel.managed_ = 1;
}




void
rcRocMenuWindow::ConfigSelPopdown (void)
{
  int i = 0;

  printf("rcRocMenuWindow::ConfigSelPopdown reached\n");fflush(stdout);

  if (iconfigSel.managed_) XtPopdown (XtParent (iconfigSel.w_));
  iconfigSel.managed_ = 0;
  for (i = 0; i < EDITOR_MAX_DATABASES; i++) XtUnmanageChild (iconfigSel.pushb[i]);

  /* reset ok button sensitivity */
  XtSetSensitive (iconfigSel.ok_, TRUE);

  printf("rcRocMenuWindow::ConfigSelPopdown done\n");fflush(stdout);
}







Widget
rcRocMenuWindow::CreateOptionMenu (Widget parent)
{
  Arg arg[20];
  int ac = 0;
  XmString t;
  Widget menu;
  Widget option;
  int    i = 0;
  int ncols = 2;

  printf("rcRocMenuWindow::CreateOptionMenu reached\n");

  ac = 0;
  XtSetArg(arg[ac], XmNpacking, XmPACK_COLUMN); ac++;
  XtSetArg(arg[ac], XmNnumColumns, ncols); ac++;
  menu = XmCreatePulldownMenu (parent, (char *)"optionPullDown", arg, ac);

  ac = 0;
  t = XmStringCreateSimple ((char *)"Run Type ");
  XtSetArg (arg[ac], XmNlabelString, t); ac++;
  XtSetArg (arg[ac], XmNsubMenuId, menu); ac++;
  option = XmCreateOptionMenu (parent, (char *)"configurationOption", arg, ac);
  ac = 0;
  XmStringFree (t);

  t = XmStringCreateSimple ((char *)"              ");
  XtSetArg (arg[ac], XmNlabelString, t); ac++;
  for (i = 0; i < EDITOR_MAX_CONFIGS; i++)
  {
    iconfigSel.pushb[i] = XtCreateWidget ("optionButton", xmPushButtonGadgetClass, menu, arg, ac);
    if (i == 0) XtManageChild (iconfigSel.pushb[i]);
  }
  ac = 0;
  XmStringFree (t);
 
  return(option);
}



void
rcRocMenuWindow::RocsSelectConfig(char *currconfig)
{
  rcComdOption* option = rcComdOption::option ();
  int ii, ncomp_old, jj, kk, ret, i = 0;
  char temp[128], temp2[128];
  rcNetComp* daq_list[MAX_NUM_COMPS];
  int        num_comps;
  ConfigInfo* configs[EDITOR_MAX_CONFIGS];
  int        num_configs;
  pid_t pid;
  int myargc;
  char *myargv[20];
  char *mysqlhost;
  char *expid;
  char *session;

  mysqlhost = option->msqldhost();
  expid = option->dbasename();
  session = option->session();
  printf("\n>>> Will use expid(dbasename) >%s<, session name >%s<\n\n",expid,session);

  /* cleanup previously opened components if any */
  if(ncomp_)
  {
    printf("rcRocMenuWindow::ConfigSelOk: CLEANUP !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    for(ii=0; ii<ncomp_; ii++)
	{
      ret = kill(proc_id[ii], SIGTERM);
      printf("[%2d] pid=%d, kill() returned %d\n",ii,proc_id[ii],ret);
	}
  }
  ncomp_ = 0;

  XcodaEditorResetGraphCmd();

  /* tell database handler */
  if (selectConfigTable (currconfig) < 0)
  {
    /*XcodaEditorShowConfigName (0);*/
    printf("Cannot select configuration %s", currconfig);fflush(stdout);
    return;
  }
  else
  {
	printf("ConfigSelOk: currconfig >%s<\n",currconfig); fflush(stdout);

    /*XcodaEditorShowConfigName (currconfig);*/
    /* in following call sometimes have popup gui with message:
    Get all components failed: Lost connection to MySQL server during query
      it comes from Editor_converter.c calling Editor_database.c
      ( routine createRcNetCompsFromDbase() )
	*/
    if ( (ret=constructRcnetCompsWithConfig (currconfig, 
											 daq_list, &num_comps,
											 configs, &num_configs)) == 0)
    {
	  /*	  
      printf("\nConfigSelOk: num_comps=%d\n",num_comps);
      for(ii=0; ii<num_comps; ii++)
	  {
        printf("   [%d] comp name >%s<, node name >%s< type %d, id_num=%d, status=%d, boot_string >%s<\n",
          ii,daq_list[ii]->daq.comp_name,daq_list[ii]->daq.node_name,
			   daq_list[ii]->daq.type,daq_list[ii]->daq.id_num,daq_list[ii]->daq.status,daq_list[ii]->daq.boot_string);
		printf("       code >%s< >%s< >%s<\n",daq_list[ii]->daq.code[0],daq_list[ii]->daq.code[1],daq_list[ii]->daq.code[2]);
	  }
      printf("\nConfigSelOk: num_configs=%d\n",num_configs);
	  */
      for(ii=0; ii<num_configs; ii++)
	  {
		
        printf("  Comp[%d] name >%s<\n",ii,configs[ii]->comp_name);
        printf("      code0 >%s<\n",configs[ii]->code[0]);
        printf("      code1 >%s<\n",configs[ii]->code[1]);
        printf("      code2 >%s<\n",configs[ii]->code[2]);
        printf("      code3 >%s<\n",configs[ii]->code[3]);
        printf("   num_inputs=%d\n",configs[ii]->num_inputs);
        for(jj=0; jj<configs[ii]->num_inputs; jj++)
		{
          printf("     input:comp_name >%s<, input:port_name >%s<\n",
				 configs[ii]->inputs[jj]->comp_name,configs[ii]->inputs[jj]->port_name);
        }
        printf("   num_outputs=%d\n",configs[ii]->num_outputs);
        for(jj=0; jj<configs[ii]->num_outputs; jj++)
		{
          printf("     output:comp_name >%s<, output:port_name >%s<\n",
				 configs[ii]->outputs[jj]->comp_name,configs[ii]->outputs[jj]->port_name);
        }
		

        /* skip coda_xxx, we will not start xterm for it */
        if(!strncmp(configs[ii]->comp_name,"coda_",5))
		{
          printf("Skip component >%s<\n",configs[ii]->comp_name);
          continue;
		}

        /* skip file_xxx, we will not start xterm for it */
        if(!strncmp(configs[ii]->comp_name,"file_",5))
		{
          printf("Skip component >%s<\n",configs[ii]->comp_name);
          continue;
		}

        /* fill structure which will be used to start processes */
        comp[ncomp_].comp_name = configs[ii]->comp_name; /* component unique name */
        /*get the rest from 'daq_list'*/
        ncomp_old = ncomp_;
        for(jj=0; jj<num_comps; jj++)
		{
          if( !strcmp(daq_list[jj]->daq.comp_name, comp[ncomp_].comp_name) )
		  {
            comp[ncomp_].type = daq_list[jj]->daq.type;               /* component type                  */
            comp[ncomp_].node_name = daq_list[jj]->daq.node_name;     /* default address                 */
            comp[ncomp_].id_num = daq_list[jj]->daq.id_num;           /* unique id number within a class */
            comp[ncomp_].status = 0/*daq_list[jj]->daq.status*/;           /* component status from RC        */
            comp[ncomp_].boot_string = daq_list[jj]->daq.boot_string; /* boot string, how to start       */

		    /* backward conversion from number to name, see Editor_converter.c, change it here it changed there !!!!!!!!!! */
            if(comp[ncomp_].type == CODA_ROC)           {type_name[ncomp_] = (char *)"ROC";      bg_name[ncomp_] = (char *)"lightgreen";}
			else if(comp[ncomp_].type == CODA_EB)       {type_name[ncomp_] = (char *)"EB";       bg_name[ncomp_] = (char *)"yellow";}
			else if(comp[ncomp_].type == CODA_ET)       {type_name[ncomp_] = (char *)"ET";       bg_name[ncomp_] = (char *)"yellow";}
            else if(comp[ncomp_].type == CODA_ETT)      {type_name[ncomp_] = (char *)"ETT";      bg_name[ncomp_] = (char *)"yellow";}
            else if(comp[ncomp_].type == CODA_SRO)      {type_name[ncomp_] = (char *)"SRO";      bg_name[ncomp_] = (char *)"yellow";}
            else if(comp[ncomp_].type == CODA_TRIG)     {type_name[ncomp_] = (char *)"TS";       bg_name[ncomp_] = (char *)"lightblue";}  
            else if(comp[ncomp_].type == CODA_RCS)      {type_name[ncomp_] = (char *)"RCS";      bg_name[ncomp_] = (char *)"white";}
            else if(comp[ncomp_].type == CODA_ER)       {type_name[ncomp_] = (char *)"ER";       bg_name[ncomp_] = (char *)"yellow";}
            else if(comp[ncomp_].type == CODA_SPR)       {type_name[ncomp_] = (char *)"SPR";       bg_name[ncomp_] = (char *)"white";}
            else if(comp[ncomp_].type == CODA_L3)       {type_name[ncomp_] = (char *)"L3";       bg_name[ncomp_] = (char *)"yellow";}
            else if(comp[ncomp_].type == CODA_LOG)      {type_name[ncomp_] = (char *)"LOG";      bg_name[ncomp_] = (char *)"white";}
            else if(comp[ncomp_].type == CODA_SC)       {type_name[ncomp_] = (char *)"SC";       bg_name[ncomp_] = (char *)"white";}
            else if(comp[ncomp_].type == CODA_FILE)     {type_name[ncomp_] = (char *)"FILE";     bg_name[ncomp_] = (char *)"white";}
            else if(comp[ncomp_].type == CODA_CODAFILE) {type_name[ncomp_] = (char *)"CODAFILE"; bg_name[ncomp_] = (char *)"white";}
            else if(comp[ncomp_].type == CODA_DEBUG)    {type_name[ncomp_] = (char *)"DEBUG";    bg_name[ncomp_] = (char *)"white";}
            else if(comp[ncomp_].type == CODA_MON)      {type_name[ncomp_] = (char *)"MON";      bg_name[ncomp_] = (char *)"white";}
            else if(comp[ncomp_].type == CODA_NONE)     {type_name[ncomp_] = (char *)"NONE";     bg_name[ncomp_] = (char *)"white";}
            else                                        {type_name[ncomp_] = (char *)"UNKNOWN";  bg_name[ncomp_] = (char *)"white";}

            /* set window title*/
			sprintf(temp,"%s on %s",comp[ncomp_].comp_name,comp[ncomp_].node_name);
            XmString t = XmStringCreateSimple(temp);
            XtVaSetValues (xtermsLabel[ncomp_], XmNlabelString, t, NULL);
            XmStringFree (t);

            ncomp_ ++;
            if(ncomp_ >= MAX_NUM_COMPS)
			{
              printf("MAX_NUM_COMPS=%d is not enough, increase it in Editor.h !!!\n");fflush(stdout);
              exit(0);
			}
            break;
		  }
		}
        if(ncomp_ == ncomp_old)
		{
          printf("ERROR: cannot find component >%s< - exit\n",comp[ncomp_].comp_name);fflush(stdout);
          ncomp_ = 0;
		  return;
	    }
	  }



	  /* we are not interest of the fate of our children, so we are letting kernel to recycle <defunct> after child killed */
      /* otherwise we'd call 'wait' or 'waitpid' */
      signal(SIGCHLD, SIG_IGN);

	  /*ncomp_=1;*/

      /* start components */
      printf("\nFollowing %d components will be started:\n\n",ncomp_);
      for(kk=0; kk<ncomp_; kk++)
	  {
        printf("   [%2d] name  >%s<\n",kk,comp[kk].comp_name);
        printf("   [%2d] host  >%s<\n",kk,comp[kk].node_name);
        printf("   [%2d] id     %d\n",kk,comp[kk].id_num);
        printf("   [%2d] boot  >%s<\n",kk,comp[kk].boot_string);
        printf("   [%2d] type   %d >%s<\n",kk,comp[kk].type, type_name[kk]);
        printf("   [%2d] status %d\n\n",kk,comp[kk].status);

        pid = fork();

        if(pid < 0)
        {
          printf("FORK ERROR - exit\n");fflush(stdout);
          exit(0);
        }
        else if(pid == 0) /* child */
		{
          printf("[%2d] child process id = %d\n",kk,getpid());
          /*printf("[%d] child only: parent id = %d, process id = %d\n", kk, getppid(), getpid());*/

          /* second param is process name as it will be in 'ps -ef' !!! */
          myargc = 0;
          myargv[myargc++] = strdup( "cterm" );

		  /*
          myargv[myargc++] = strdup( "-geometry" );
          myargv[myargc++] = strdup( "80x11" );
		  */

		  
          if(logging)
		  {
            myargv[myargc++] = strdup( "-l" );
            myargv[myargc++] = strdup( "-lf" );
            sprintf(temp,"/data/log/%s.log",comp[kk].comp_name);
            printf("log filename >%s<\n",temp);
            myargv[myargc++] = strdup( temp );
		  }
		  
          myargv[myargc++] = strdup( "-bg" );
          myargv[myargc++] = strdup( bg_name[kk] );

          myargv[myargc++] = strdup( "-into_name" );
          sprintf(temp,"%02d_%02d",kk/5,kk%5);
          printf("kk=%d -> window index >%s<\n\n",kk,temp);
          myargv[myargc++] = strdup( temp );

          myargv[myargc++] = strdup( "-expect" );

 	  /*unsetenv DISPLAY disables X forwarding, and ssh connects faster*/
          sprintf(temp,"unsetenv DISPLAY; ssh %s:%s",comp[kk].node_name,comp[kk].node_name);
          //sprintf(temp,"ssh %s:%s",comp[kk].node_name,comp[kk].node_name);

          myargv[myargc++] = strdup( temp );

          sprintf(temp,"stop_coda_process -p %s -match \"%s %s\"",comp[kk].boot_string,comp[kk].comp_name,type_name[kk]);
          myargv[myargc++] = strdup( temp );

          sprintf(temp,"%s -m %s -e %s -s %s -o \"%s %s\":%s %s",comp[kk].boot_string,mysqlhost,expid,session,comp[kk].comp_name,type_name[kk],comp[kk].comp_name,type_name[kk]);
          myargv[myargc++] = strdup( temp );


          printf("myargc=%d, myargv >",myargc);
		  for(ii=0; ii<myargc; ii++) printf("%s ",myargv[ii]);
          printf("<\n");

		  /* use following while-sleep if execl commented out, otherwise nested forking will occur !
		  while(1) sleep(1);
		  */

		  /*
          sprintf(temp2,"%s/cterm",getenv("CODA_BIN"));
          printf("command >%s<\n",temp2);
          execl(temp2,
                myargv[0], myargv[1], myargv[2], myargv[3], myargv[4],
                myargv[5], myargv[6], myargv[7], myargv[8], myargv[9], myargv[10],
                (char *)NULL);
		  */

          ctermlib(myargc, myargv);

		}
        else
		{
          proc_id[kk] = pid; /* remember child's pid */
          /*printf("[%d] parent only: pid=%d\n",kk,pid)*/;
		}

	  }


	  printf("OUT OF FORK LOOP\n");fflush(stdout);
      for(i=0; i<ncomp_; i++)
	  {
        printf("[%2d] pid=%d\n",i,proc_id[i]);
	  }

	  /* if have ssh error 'ssh_exchange_identification: Connection closed by remote host', increase following
grep MaxStartups /etc/ssh/sshd_config
	  */

	  /*	  
      system("codaterm -geometry 80x11 -into 00_00 -expect 'ssh adcecal5:adcecal5' 'stop_coda_process -p coda_roc_gef -match \"adcecal0 ROC\"' 'coda_roc_gef -s clastest -o \"adcecal0 ROC\":ROC'  &");
	  */



	  /*
      XcodaEditorConstructGraphFromConfig(&coda_graph, daq_list, num_comps,
					  configs, num_configs);
      (*coda_graph.redisplay)(&coda_graph, sw_geometry.draw_area, 
			      cbs->event);
	  */



      /* free all resources */
      for (i = 0; i < num_comps; i++) freeRcNetComp (daq_list[i]);
      for (i = 0; i < num_configs; i++) freeConfigInfo (configs[i]);
    }
	else
	{
      printf("rcRocMenuWindow::RocsSelectConfig: ERROR in constructRcnetCompsWithConfig(), returned %d\n",ret);fflush(stdout);
      printf("rcRocMenuWindow::RocsSelectConfig: rocs windows were NOT created !!!\n");fflush(stdout);
	}



  }
  /*
  printf("sleep ..\n");fflush(stdout);
  sleep(3);
  printf("slept !!\n");fflush(stdout);
  */
}



void
rcRocMenuWindow::ConfigSelOk (Widget w, XtPointer data, /*temporary: XmAnyCallbackStruct*/void* cbs)
{
  Widget curr;
  Arg arg[20];
  int i, ac = 0;
  char *currconfig;

  printf("rcRocMenuWindow::ConfigSelOk reached\n");

  /* popdown dialog first */
  ConfigSelPopdown ();

  /* get current selection */
  editorConfigSel* sel = (editorConfigSel *)data;
  XtSetArg (arg[ac], XmNmenuHistory, &curr); ac++;
  XtGetValues (sel->option_, arg, ac);
  ac = 0;  
  for (i = 0; i < sel->numConfigs_; i++)
  {
    if (curr == sel->pushb[i]) break;
  }
  currconfig = sel->configs_[i];

  RocsSelectConfig(currconfig);  
}



void
rcRocMenuWindow::ConfigSelCancel (Widget w, XtPointer data, /*temporary: XmAnyCallbackStruct*/void* cbs)
{
  printf("rcRocMenuWindow::ConfigSelCancel reached\n");

  ConfigSelPopdown ();
}

void
rcRocMenuWindow::InitConfigSel (Widget parent)
{
  Arg arg[20];
  int ac = 0;
  XmString t;
  Widget actionForm, sep;
  static int configSelInited = 0;

  printf("rcRocMenuWindow::InitConfigSel reached\n");

  if (configSelInited) 
  {
    printf("rcRocMenuWindow::InitConfigSel: already inited - do nothing\n");
    return;
  }

  iconfigSel.numConfigs_ = 0;
  iconfigSel.managed_ = 0;
  iconfigSel.w_ = 0;
  iconfigSel.option_ = 0;
  iconfigSel.ok_ = 0;
  iconfigSel.cancel_ = 0;

  /* create all widgets */
  XtSetArg (arg[ac], XmNtitle, "Run Type Selection"); ac++;
  XtSetArg (arg[ac], XmNautoUnmanage, FALSE); ac++;
  iconfigSel.w_ = XmCreateFormDialog (parent, (char *)"editorconfigSel",
				     arg, ac);
  ac = 0;

  /* create action form */
  XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNbottomOffset, 5); ac++;
  XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNleftOffset, 2); ac++;
  XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNrightOffset, 2); ac++;
  XtSetArg (arg[ac], XmNwidth, 300); ac++;
  actionForm = XtCreateWidget ("runTypeActionForm",
			       xmFormWidgetClass, iconfigSel.w_,
			       arg, ac);
  ac = 0;

  /* create push buttons */
  t = XmStringCreateSimple ((char *)"   Ok   ");
  XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
  XtSetArg (arg[ac], XmNleftPosition, 10); ac++;
  XtSetArg (arg[ac], XmNlabelString, t); ac++;
  XtSetArg (arg[ac], XmNshowAsDefault, 1); ac++;
  iconfigSel.ok_ = XtCreateManagedWidget ("runTypeOk", xmPushButtonGadgetClass,
					  actionForm, arg, ac);
  ac = 0;
  XmStringFree (t);

  t = XmStringCreateSimple ((char *)" Cancel ");
  XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNtopOffset, 5); ac++;
  XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNbottomOffset, 5); ac++;
  XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
  XtSetArg (arg[ac], XmNrightPosition, 90); ac++;
  XtSetArg (arg[ac], XmNlabelString, t); ac++;
  iconfigSel.cancel_ = XtCreateManagedWidget ("cancel", xmPushButtonGadgetClass,
					      actionForm, arg, ac);
  ac = 0;
  XmStringFree (t);

  /* create a separator */
  XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++; 
  XtSetArg (arg[ac], XmNleftOffset, 5); ac++;
  XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNrightOffset, 5); ac++;
  XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (arg[ac], XmNbottomWidget, actionForm); ac++;
  XtSetArg (arg[ac], XmNbottomOffset, 10); ac++;
  XtSetArg (arg[ac], XmNorientation, XmHORIZONTAL); ac++;
  sep = XtCreateManagedWidget ("sep", xmSeparatorGadgetClass, iconfigSel.w_,
			       arg, ac);
  ac = 0;
					  
  
  /* create option menu */
  iconfigSel.option_ = CreateOptionMenu (iconfigSel.w_);
  XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNtopOffset, 20); ac++;
  XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNleftOffset, 10); ac++;
  XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (arg[ac], XmNbottomWidget, sep); ac++;
  XtSetArg (arg[ac], XmNbottomOffset, 10); ac++;
  XtSetValues (iconfigSel.option_, arg, ac);
  ac = 0;


  printf("rcRocMenuWindow::InitConfigSel: Adding callbacks\n");fflush(stdout);


  /* add callbacks */
  XtAddCallback (iconfigSel.ok_, XmNactivateCallback, (XtCallbackProc)&(rcRocMenuWindow::ConfigSelOk), (XtPointer)&(iconfigSel));
  XtAddCallback (iconfigSel.cancel_, XmNactivateCallback, (XtCallbackProc)&(rcRocMenuWindow::ConfigSelCancel), (XtPointer)&(iconfigSel));


  XtManageChild (actionForm);
  XtManageChild (iconfigSel.option_);
  
  /* set default button */
  XtSetArg (arg[ac], XmNdefaultButton, iconfigSel.ok_); ac++;
  XtSetValues (iconfigSel.w_, arg, ac);
  ac = 0;

  /* set inited flag */
  configSelInited = 1;
}
