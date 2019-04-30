/*-----------------------------------------------------------------------------
 * Copyright (c) 1991,1992 Southeastern Universities Research Association,
 *                         Continuous Electron Beam Accelerator Facility
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 * CEBAF Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606
 * Email: coda@cebaf.gov  Tel: (804) 249-7101  Fax: (804) 249-7363
 *-----------------------------------------------------------------------------
 * 
 * Description:
 *	Implementation of Editor_configSel
 *	
 * Author:  Jie Chen
 * CEBAF Data Acquisition Group
 *
 * Revision History:
 *   $Log: Editor_configSel.c,v $
 *   Revision 1.6  1997/11/25 16:57:19  rwm
 *   Zapped compiler warning.
 *
 *   Revision 1.5  1997/06/20 16:59:47  heyes
 *   clean up GUI!
 *
 *   Revision 1.4  1997/06/16 13:22:42  heyes
 *   clear graph
 *
 *   Revision 1.3  1997/06/16 12:23:37  heyes
 *   various fixes and nicities!
 *
 *   Revision 1.2  1996/11/13 14:55:40  chen
 *   allow different readout list in different configuration
 *
 *   Revision 1.1.1.2  1996/11/05 17:45:30  chen
 *   coda source
 *
 *	  
 */

#include <stdio.h>
#include <stdlib.h>

#include <Xm/Xm.h>
#include <Xm/PushBG.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/SeparatoG.h>

#include "Editor.h"
#include "Editor_configSel.h"
#include "Editor_graph.h"
#include "Editor_graph_cmd.h"
#include "Editor_miscinfo.h"
#include "Editor_xmisc.h"
#include "Editor_converter.h"


#undef DEBUG

Widget menu;
static editorConfigSel  iconfigSel;

static Widget
createOptionMenu (Widget parent)
{
  Arg arg[20];
  int ac = 0;
  XmString t;
  Widget option;
  int    i = 0;

  printf(">>> iconfigSel.numConfigs_=%d\n",iconfigSel.numConfigs_);

  /* sergey: make it possible to set the number of rows in popup menu on flight */
  ac = 0;
  XtSetArg(arg[ac], XmNpacking, XmPACK_COLUMN); ac++;
  XtSetArg(arg[ac], XmNnumColumns, 1); ac++; /*sergey: will be changed on-flight based on the number of configs*/
  menu = XmCreatePulldownMenu (parent, "optionPullDown", arg, ac);

  ac = 0;
  t = XmStringCreateSimple ("Run Type ");
  XtSetArg (arg[ac], XmNlabelString, t); ac++;
  XtSetArg (arg[ac], XmNsubMenuId, menu); ac++;
  option = XmCreateOptionMenu (parent, "configurationOption", arg, ac);
  ac = 0;
  XmStringFree (t);

  t = XmStringCreateSimple ("              ");
  XtSetArg (arg[ac], XmNlabelString, t); ac++;
  for (i = 0; i < EDITOR_MAX_CONFIGS; i++)
  {
    iconfigSel.pushb[i] = XtCreateWidget ("optionButton", xmPushButtonGadgetClass, menu, arg, ac);
    if (i == 0) XtManageChild (iconfigSel.pushb[i]);
  }
  ac = 0;
  XmStringFree (t);
  
  return option;
}


/*args: (Widget,XtPointer,XtPointer), see Intrinsic.h */

static void
configSelOk (Widget w, XtPointer data, /*XmAnyCallbackStruct* cbs*/XtPointer callback_data)
{
  XmAnyCallbackStruct* cbs = (XmAnyCallbackStruct*)callback_data;
  int  i = 0;
  char* currconfig;
  Arg arg[20];
  int ac = 0;
  Widget curr;
  char  temp[128];
  rcNetComp* daq_list[MAX_NUM_COMPS];
  int        num_comps;
  ConfigInfo* configs[EDITOR_MAX_CONFIGS];
  int        num_configs;

  editorConfigSel* sel = (editorConfigSel *)data;

  /* popdown dialog first */
  configSelPopdown ();
  
  /* get current selection */
  XtSetArg (arg[ac], XmNmenuHistory, &curr); ac++;
  XtGetValues (sel->option_, arg, ac);
  ac = 0;
  
#ifdef DEBUG
  printf("configSelOk: sel->numConfigs_ = %d\n",sel->numConfigs_); fflush(stdout);
#endif
  for (i = 0; i < sel->numConfigs_; i++)
  {
    if (curr == sel->pushb[i]) break;
  }
  currconfig = sel->configs_[i];

  XcodaEditorResetGraphCmd();
  /* tell database handler */
  if (selectConfigTable (currconfig) < 0) {
    XcodaEditorShowConfigName (0);
    sprintf (temp, "Cannot select configuration %s", currconfig);
    pop_error_message (temp, w);
    return;
  }
  else
  {
#ifdef DEBUG
	printf("configSelOk: currconfig >%s<\n",currconfig); fflush(stdout);
#endif

    XcodaEditorShowConfigName (currconfig);
    if (constructRcnetCompsWithConfig (currconfig, 
				       daq_list, &num_comps,
				       configs, &num_configs) == 0) {
      XcodaEditorConstructGraphFromConfig(&coda_graph, daq_list, num_comps,
					  configs, num_configs);
      (*coda_graph.redisplay)(&coda_graph, sw_geometry.draw_area, 
			      cbs->event);

      /* free all resources */
      for (i = 0; i < num_comps; i++) freeRcNetComp (daq_list[i]);
      for (i = 0; i < num_configs; i++) freeConfigInfo (configs[i]);
    }
  }
}


extern Widget top_form;

void
EditorSelectConfig (char *confn)
{
  int  i = 0;
  char* currconfig;
  Arg arg[20];
  int ac = 0;
  Widget curr;
  char  temp[128];
  rcNetComp* daq_list[MAX_NUM_COMPS];
  int        num_comps;
  ConfigInfo* configs[EDITOR_MAX_CONFIGS];
  int        num_configs;

#ifdef DEBUG
  printf("EditorSelectConfig: confn >%s<\n",confn);
#endif

  currconfig = confn;

  XcodaEditorResetGraphCmd();

  delete_everything (0, 0, 0);
  initConfigSel (sw_geometry.draw_area);

  /* tell database handler */
  if (selectConfigTable (currconfig) < 0)
  {
    XcodaEditorShowConfigName (0);
    sprintf (temp, "Cannot select configuration %s", currconfig);
    pop_error_message (temp, top_form);
    return;
  }
  else
  {
    XcodaEditorShowConfigName (currconfig);
    if (constructRcnetCompsWithConfig (currconfig, 
				       daq_list, &num_comps,
				       configs, &num_configs) == 0)
    {
      XcodaEditorConstructGraphFromConfig(&coda_graph, daq_list, num_comps,
					  configs, num_configs);
      (*coda_graph.redisplay)(&coda_graph, sw_geometry.draw_area, 
			      0);
      /* free all resources */
      for (i = 0; i < num_comps; i++) freeRcNetComp (daq_list[i]);
      for (i = 0; i < num_configs; i++) freeConfigInfo (configs[i]);
    }
  }
}

/*args: (Widget,XtPointer,XtPointer), see Intrinsic.h */

static void
configSelCancel (Widget w, XtPointer data, /*XmAnyCallbackStruct**/XtPointer cbs)
{
  configSelPopdown ();
}


void
initConfigSel (Widget parent)
{
  Arg arg[20];
  int ac = 0;
  XmString t;
  Widget actionForm, sep;
  static int configSelInited = 0;
  
#ifdef DEBUG
  printf("initConfigSel reached\n");
#endif

  if (configSelInited) 
  {
#ifdef DEBUG
    printf("initConfigSel: already inited - do nothing\n");
#endif
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
  iconfigSel.w_ = XmCreateFormDialog (parent, "editorconfigSel",
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
  t = XmStringCreateSimple ("   Ok   ");
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

  t = XmStringCreateSimple (" Cancel ");
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
  iconfigSel.option_ = createOptionMenu (iconfigSel.w_);
  XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNtopOffset, 20); ac++;
  XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (arg[ac], XmNleftOffset, 10); ac++;
  XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (arg[ac], XmNbottomWidget, sep); ac++;
  XtSetArg (arg[ac], XmNbottomOffset, 10); ac++;
  XtSetValues (iconfigSel.option_, arg, ac);
  ac = 0;

  /* add callbacks */
  XtAddCallback (iconfigSel.ok_, XmNactivateCallback, configSelOk,
		 (XtPointer)&(iconfigSel));
  XtAddCallback (iconfigSel.cancel_, XmNactivateCallback, configSelCancel,
		 (XtPointer)&(iconfigSel));


  XtManageChild (actionForm);
  XtManageChild (iconfigSel.option_);
  
  /* set default button */
  XtSetArg (arg[ac], XmNdefaultButton, iconfigSel.ok_); ac++;
  XtSetValues (iconfigSel.w_, arg, ac);
  ac = 0;

  /* set inited flag */
  configSelInited = 1;
}


void
configSelPopup (void)
{
  int  status;
  int  i;
  Arg  arg[10];
  int  ac = 0;
  XmString t;
  int ncols;

  if (iconfigSel.managed_) 
    configSelPopdown ();
  
  /* remove old database information */
  for (i = 0; i < iconfigSel.numConfigs_; i++) free (iconfigSel.configs_[i]);

  /* get all database names */
  status = listAllConfigs (iconfigSel.configs_, &(iconfigSel.numConfigs_));
  /*printf("=== iconfigSel.numConfigs_=%d\n",iconfigSel.numConfigs_);*/

  ncols = iconfigSel.numConfigs_ / 16 + 1;
  /*printf("setting ncols in pulldown menu to %d\n",ncols);*/
  ac = 0;
  XtSetArg(arg[ac], XmNnumColumns, ncols); ac++;
  XtSetValues (menu, arg, ac);


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
      t = XmStringCreateSimple ("no runtypes");
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
configSelPopdown (void)
{
  int i = 0;

  if (iconfigSel.managed_) XtPopdown (XtParent (iconfigSel.w_));
  iconfigSel.managed_ = 0;
  for (i = 0; i < EDITOR_MAX_DATABASES; i++)
  {
    XtUnmanageChild (iconfigSel.pushb[i]);
  }
  /* reset ok button sensitivity */
  XtSetSensitive (iconfigSel.ok_, TRUE);
}
