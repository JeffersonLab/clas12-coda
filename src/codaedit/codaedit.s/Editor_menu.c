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
 *	CODA Editor's pulldown menus
 *	
 * Author:  Jie Chen, CEBAF Data Acquisition Group
 *
 * Revision History:
 *   $Log: Editor_menu.c,v $
 *   Revision 1.7  1998/07/13 17:53:56  heyes
 *   remove buttons when env var set...
 *
 *   Revision 1.6  1997/11/24 15:09:29  heyes
 *   bombproof strsave
 *
 *   Revision 1.5  1997/08/25 16:08:05  heyes
 *   fix display problems with background
 *
 *   Revision 1.4  1997/07/30 15:17:04  heyes
 *   add more xpm support
 *
 *   Revision 1.3  1997/06/20 17:00:17  heyes
 *   clean up GUI!
 *
 *   Revision 1.2  1997/06/16 12:23:56  heyes
 *   various fixes and nicities!
 *
 *   Revision 1.1.1.2  1996/11/05 17:45:14  chen
 *   coda source
 *
 *	  
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/CascadeB.h>
#include <Xm/ToggleB.h>
#include <Xm/MessageB.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>

#include "MixedLabel.h"

#include "Editor_layout.h"
#include "Editor_graph.h"
#include "Editor_pixmap.h"
#include "Editor_dbaseSel.h"
#include "Editor_configSel.h"
#include "Editor_newConfig.h"
#include "Editor_opendbaseDialog.h"
#include "Editor_menu.h"
#include "Editor_graph_cmd.h"
#include "Editor_rmconfigSel.h"
#include "Editor_converter.h"
#include "Editor_icon_box.h"
#include "Editor_miscinfo.h"
#include "Editor_xmisc.h"
#include "Editor_rmdbaseSel.h"


/*
static char *file_menus[]={
  "New Database...",
  "Open Database...",
  "Remove Database...",
  "Exit"
};
*/

static char *config_menus[] = {
  "New Database...",
  "Open Database...",
  "Remove Database...",
  "New ...",
  "Open ...",
  "Remove ...",
  "Save",
  "Save As ...",
  "Options"
};

static char *edit_menus[]={
  "Delete Component",
  "Delete Connection",
  "Delete All Connections",
  "Clear All",
  "Undo",
  "Redraw",
  "Resize a Component",
  "Connect two columns"
};

static char *op_menus[]={
  "Remove Unused Components",
  "Background/Foreground Color"
};


static char *help_menus[]={
  "About CODA"};

/* default option pairs */
typedef struct _def_option_
{
  char* option_name;
  char* option_val;
} codaDefOption;

static codaDefOption defOptions[] = {
  {"eventLimit", "0"},
  {"dataLimit", "0"},
  {"dataFile", ""},
  {"tokenInterval", "64"},
};

static int numDefOptions = 4;


void
XcodaEditorCreateMenus(Widget menu_bar, int withExit)
{
  Widget pulldown0, pulldown1, pulldown2, pulldown3, pulldown4;
  Widget m_cas0, m_cas1, m_cas2, m_cas3, m_cas4;
  Widget pushb[10], sep, helpb[5], edit_b[20], op_b[10];
  Arg    args[20];
  int    ac, i;
  XmString t;


  /* Create file menu 
  ac = 0;
  pulldown0 = XmCreatePulldownMenu(menu_bar,"_pulldown", NULL, 0);
  ac = 0;

  t = XmStringCreateSimple("File");
  XtSetArg(args[ac], XmNsubMenuId, pulldown0); ac++;
  XtSetArg(args[ac], XmNlabelString, t); ac++;
  XtSetArg(args[ac], XmNmnemonic, 'F'); ac++;
  m_cas0 = XtCreateManagedWidget("m_cas0",xmCascadeButtonWidgetClass,
				 menu_bar,args, ac);
  ac = 0;
  XmStringFree(t);
  for(i=0; i< XtNumber(file_menus)-1; i++){
    t = XmStringCreateSimple(file_menus[i]);
    XtSetArg(args[ac], XmNlabelString,t); ac++;
    pushb[i] = XtCreateManagedWidget("menu_push",xmPushButtonWidgetClass,
				     pulldown0,args, ac);
    switch(i){
    case 0:
      manager.newdbase = pushb[i];
      break;
    case 1:
      manager.opendbase = pushb[i];
      break;
    case 2:
      manager.removedbase = pushb[i];
      break;
    default:
      break;
    }
    ac = 0;
    XmStringFree(t);
  }

  end of file menu */


  /* create configure menu */
  ac = 0;
  pulldown1 = XmCreatePulldownMenu(menu_bar,"_pulldown", NULL, 0);
  ac = 0;

  t = XmStringCreateSimple("File");
  XtSetArg(args[ac], XmNsubMenuId, pulldown1); ac++;
  XtSetArg(args[ac], XmNlabelString, t); ac++;
  XtSetArg(args[ac], XmNmnemonic, 'C'); ac++;
  m_cas1 = XtCreateManagedWidget("m_cas1",xmCascadeButtonWidgetClass,
				 menu_bar,args, ac);
  ac = 0;
  XmStringFree(t);
  for(i=0; i< XtNumber(config_menus) - 1; i++)
  {
    t = XmStringCreateSimple(config_menus[i]);
    XtSetArg(args[ac], XmNlabelString,t); ac++;
    pushb[i] = XtCreateManagedWidget("menu_push",xmPushButtonWidgetClass,
				     pulldown1,args, ac);
    switch(i)
    {
    case 0:
      manager.newdbase = pushb[i];
      break;
    case 1:
      manager.opendbase = pushb[i];
      break;
    case 2:
      manager.removedbase = pushb[i];
      break;
    case 3:
      manager.newconfig = pushb[i];
      break;
    case 4:
      manager.openconfig = pushb[i];
      break;
    case 5:
      manager.removeconfig = pushb[i];
      break;
    case 6:
      manager.saveconfig = pushb[i];
      break;
    case 7:
      manager.saveasconfig = pushb[i];
      break;

    default:
      break;
    }
    ac = 0;
    XmStringFree(t);
  }
  /* create separeator + option menu*/
  sep = XtCreateManagedWidget("sep", xmSeparatorWidgetClass,
			      pulldown1, NULL, 0);
  ac = 0;
  t = XmStringCreateSimple(config_menus[i]);
  XtSetArg(args[ac], XmNlabelString,t); ac++;
  pushb[i] = XtCreateManagedWidget("menu_push",xmPushButtonWidgetClass,
				   pulldown1,args, ac);
  ac = 0;
  XmStringFree(t);
  manager.option = pushb[i];
  if (withExit == 0)
  {
    sep = XtCreateManagedWidget("sep", xmSeparatorWidgetClass,
				pulldown1, NULL, 0);
    ac = 0;
    t = XmStringCreateSimple("Quit");
    XtSetArg(args[ac], XmNlabelString,t); ac++;
    pushb[i] = XtCreateManagedWidget("menu_push",xmPushButtonWidgetClass,
				     pulldown1,args, ac);
    ac = 0;
    XmStringFree(t);
    manager.exit_menu = pushb[i];
  }
  else
  {
    manager.exit_menu = NULL;
  }

  /* create edit menu */
  pulldown2 = XmCreatePulldownMenu(menu_bar,"_pulldown", NULL, 0);
  
  t = XmStringCreateSimple("Edit");
  XtSetArg(args[ac], XmNsubMenuId, pulldown2); ac++;
  XtSetArg(args[ac], XmNlabelString, t); ac++;
  XtSetArg(args[ac], XmNmnemonic, 'E'); ac++;
  m_cas2 = XtCreateManagedWidget("m_cas2",xmCascadeButtonWidgetClass,
				 menu_bar,args, ac);
  ac = 0;
  XmStringFree(t);

  for(i=0; i< XtNumber(edit_menus)-3; i++)
  {
    t = XmStringCreateSimple(edit_menus[i]);
    XtSetArg(args[ac], XmNlabelString, t); ac++;
    edit_b[i] = XtCreateManagedWidget("edit_b", xmPushButtonWidgetClass,
				      pulldown2,args, ac);
    switch(i){
    case 0:
      manager.delete_comp_menu = edit_b[i];
      break;
    case 1:
      manager.delete_arc_menu = edit_b[i];
      break;
    case 2:
      manager.delete_allarc_menu = edit_b[i];
      break;
    case 3:
      manager.delete_all_menu = edit_b[i];
      break;
    case 4:
      manager.undo_menu = edit_b[i];
      break;
    default:
      break;
    }
    ac = 0;
    XmStringFree(t);
  }
  sep = XtCreateManagedWidget("sep", xmSeparatorWidgetClass,
			      pulldown2, NULL, 0);
  ac = 0;
  t = XmStringCreateSimple(edit_menus[i]);
  XtSetArg(args[ac], XmNlabelString, t); ac++;
  edit_b[i] = XtCreateManagedWidget("edit_b", xmPushButtonWidgetClass,
				    pulldown2, args, ac);
  manager.redraw_menu = edit_b[i];
  i++;
  ac = 0;
  XmStringFree(t);

  sep = XtCreateManagedWidget("sep", xmSeparatorWidgetClass,
			      pulldown2, NULL, 0);
  ac = 0;

  t = XmStringCreateSimple(edit_menus[i]);
  XtSetArg(args[ac], XmNlabelString, t); ac++;
  edit_b[i] = XtCreateManagedWidget("edit_b", xmPushButtonWidgetClass,
				    pulldown2, args, ac);
  manager.resize_comp_menu = edit_b[i];
  ac = 0;
  XmStringFree(t);
  i++;

  t = XmStringCreateSimple(edit_menus[i]);
  XtSetArg(args[ac], XmNlabelString, t); ac++;
  edit_b[i] = XtCreateManagedWidget("edit_b", xmPushButtonWidgetClass,
				    pulldown2, args, ac);
  manager.connect2cols_menu = edit_b[i];
  ac = 0;
  XmStringFree(t);
  i++;

  /* register callbacks */
  /* end of view menu */

  ac = 0;
  pulldown3 = XmCreatePulldownMenu(menu_bar,"_pulldown", NULL, 0);
  ac = 0;
  t = XmStringCreateSimple("Options");
  XtSetArg(args[ac], XmNsubMenuId, pulldown3); ac++;
  XtSetArg(args[ac], XmNlabelString, t); ac++;
  XtSetArg(args[ac], XmNmnemonic, 'O'); ac++;
  m_cas3 = XtCreateManagedWidget("m_cas3",xmCascadeButtonWidgetClass,
				 menu_bar,args, ac);
  ac = 0;  
  XmStringFree(t);
  for(i=0;i < XtNumber(op_menus); i++){
    t = XmStringCreateSimple(op_menus[i]);
    XtSetArg(args[ac], XmNlabelString,t); ac++;
    op_b[i] = XtCreateManagedWidget("options",xmPushButtonWidgetClass,
				     pulldown3, args, ac);
    ac = 0;
    XmStringFree(t);    
    switch (i){
    case 0:
      manager.rmvcomp_menu = op_b[i];
      break;
    case 1:
      manager.color_menu = op_b[i];
      break;
    default:
      break;
    }
  }

  ac = 0;
  pulldown4 = XmCreatePulldownMenu(menu_bar,"_pulldown",NULL, 0);
  ac = 0;
  t = XmStringCreateSimple("Help");
  XtSetArg(args[ac], XmNsubMenuId, pulldown4); ac++;
  XtSetArg(args[ac], XmNlabelString, t); ac++;
  XtSetArg(args[ac], XmNmnemonic, 'H'); ac++;
  m_cas4 = XtCreateManagedWidget("m_cas4",xmCascadeButtonWidgetClass,
				 menu_bar,args, ac);
  ac = 0;
  XmStringFree(t);

  for(i=0;i < XtNumber(help_menus); i++){
    t = XmStringCreateSimple(help_menus[i]);
    XtSetArg(args[ac], XmNlabelString,t); ac++;
    helpb[i] = XtCreateManagedWidget("menu_help",xmPushButtonWidgetClass,
				     pulldown4, args, ac);
    ac = 0;
    XmStringFree(t);    
  }

  XtSetArg(args[ac], XmNmenuHelpWidget, m_cas4); ac++;
  XtSetArg(args[ac], XmNspacing, 30); ac++;
  XtSetValues(menu_bar, args, ac);
  ac = 0;

}





/*******************Create popup menus**********************/
static void
popup_edit_menu(Widget w, Widget menu, XButtonPressedEvent* event)
{
  if(event->button != Button3) return;

  XmMenuPosition(menu, event);
  XtManageChild(menu);
}

void
XcodaEditorCreatePopupMenu(Widget parent)
{
  Widget menu,label,sep;
  Widget edit_b[20];
  Arg    args[20];
  int    i, ac = 0;
  XmString t;

  menu = XmCreatePopupMenu(parent,"edit_popup",NULL, 0);

  t = XmStringCreateSimple("Edit Command");
  XtSetArg(args[ac], XmNlabelString, t); ac++;
  label = XtCreateManagedWidget("pop_label",xmLabelWidgetClass,
				menu, args, ac);
  ac = 0;
  sep = XtCreateManagedWidget("sep", xmSeparatorWidgetClass,
			      menu, NULL, 0);

  for(i=0; i< XtNumber(edit_menus)-3; i++)
  {
    t = XmStringCreateSimple(edit_menus[i]);
    XtSetArg(args[ac], XmNlabelString, t); ac++;
    edit_b[i] = XtCreateManagedWidget("edit_b", xmPushButtonWidgetClass,
				      menu, args, ac);
    ac = 0;
    XmStringFree(t);
  }
  sep = XtCreateManagedWidget("sep", xmSeparatorWidgetClass,
			      menu, NULL, 0);
  ac = 0;
  t = XmStringCreateSimple(edit_menus[i]);
  XtSetArg(args[ac], XmNlabelString, t); ac++;
  edit_b[i] = XtCreateManagedWidget("edit_b", xmPushButtonWidgetClass,
				    menu, args, ac);
  ac = 0;
  XmStringFree(t);
  i++;

  sep = XtCreateManagedWidget("sep", xmSeparatorWidgetClass,
			      menu, NULL, 0);
  ac = 0;

  t = XmStringCreateSimple(edit_menus[i]);
  XtSetArg(args[ac], XmNlabelString, t); ac++;
  edit_b[i] = XtCreateManagedWidget("edit_b", xmPushButtonWidgetClass,
				    menu, args, ac);
  ac = 0;
  XmStringFree(t);
  i++;

  t = XmStringCreateSimple(edit_menus[i]);
  XtSetArg(args[ac], XmNlabelString, t); ac++;
  edit_b[i] = XtCreateManagedWidget("edit_b", xmPushButtonWidgetClass,
				    menu, args, ac);
  ac = 0;
  XmStringFree(t);

  {
    extern int hide_controls;
    if (!hide_controls)
    {
      XtAddEventHandler(parent, ButtonPressMask, False, (XtEventHandler)popup_edit_menu, menu);
      
      XtAddCallback(edit_b[0], XmNactivateCallback, XcodaEditorSetDeleteCompMode,
		    (XtPointer)DELETE_NODE_ACTION);
      manager.pop_delete_comp_menu = edit_b[0];
      
      XtAddCallback(edit_b[1], XmNactivateCallback, XcodaEditorSetDeleteArcMode,
		    (XtPointer)DELETE_ARC_ACTION);
      manager.pop_delete_arc_menu = edit_b[1];
      
      XtAddCallback(edit_b[2], XmNactivateCallback, XcodaEditorDeleteAllArcs,
		    (XtPointer)NULL);
      manager.pop_delete_allarc_menu = edit_b[2];
      
      XtAddCallback(edit_b[3], XmNactivateCallback, XcodaEditorDeleteAll,
		    (XtPointer)NULL);
      manager.pop_delete_all_menu = edit_b[3];
      
      XtAddCallback(edit_b[4], XmNactivateCallback, XcodaEditorUndoAction,
		    (XtPointer)UNDO_ACTION);
      manager.pop_undo_menu = edit_b[4];
      
      XtAddCallback(edit_b[5], XmNactivateCallback, XcodaEditorRedraw,
		    (XtPointer)NULL);
      manager.pop_redraw_menu = edit_b[5];
      
      XtAddCallback(edit_b[6], XmNactivateCallback, XcodaEditorResizeComp,
		    (XtPointer)RESIZE_COMP_ACTION);
      manager.pop_resize_comp_menu = edit_b[6];
      
      XtAddCallback(edit_b[7], XmNactivateCallback, XcodaEditorConn2Cols,
		    (XtPointer)CONN_2COLS_ACTION);
      manager.pop_connect2cols_menu = edit_b[7];
    }
  }
}

/*******************Delete all arcs************************************/
static void
popdown_dialog(Widget w, 
			   XtPointer client_data, 
			   XtPointer callback_data)
{
  XtDestroyWidget(XtParent(w));
}
					 
void
XcodaEditorDeleteAllArcs(Widget w, 
						 XtPointer client_data, 
						 XtPointer callback_data) 
{
  Widget warning_dialog;
  XmString yes, no;
  XmString message;
  Arg      args[10];
  int      ac = 0;

  /* reset graph commands */
  XcodaEditorResetGraphCmd();
  message = XmStringCreateSimple("There will be no undo for this, Are you sure?");
  yes = XmStringCreateSimple(" Yes ");
  no = XmStringCreateSimple(" No  ");
  XtSetArg(args[ac], XmNmessageString, message); ac++;
  XtSetArg(args[ac], XmNokLabelString, yes); ac++;
  XtSetArg(args[ac], XmNcancelLabelString, no); ac++;
  
  warning_dialog = XmCreateWarningDialog(sw_geometry.draw_area, "delete_warning", args, ac);

  XmStringFree(yes);
  XmStringFree(no);
  XmStringFree(message);
  XtUnmanageChild(XmMessageBoxGetChild(warning_dialog, XmDIALOG_HELP_BUTTON));
		  
  XtAddCallback(warning_dialog, XmNokCallback, delete_all_arcs, (XtPointer)NULL);
  XtAddCallback(warning_dialog, XmNcancelCallback, popdown_dialog, (XtPointer)NULL);
  XtManageChild(warning_dialog);
  XtPopup(XtParent(warning_dialog), XtGrabNone);
}

void
XcodaEditorDeleteAll(Widget w, 
			  XtPointer client_data, 
			  XtPointer callback_data)
{
  Widget warning_dialog;
  XmString yes, no;
  XmString message;
  Arg      args[10];
  int      ac = 0;

  /* reset graph commands */
  XcodaEditorResetGraphCmd();

  message = XmStringCreateSimple("There will be no undo for this, Are you sure?");
  yes = XmStringCreateSimple(" Yes ");
  no = XmStringCreateSimple(" No  ");
  XtSetArg(args[ac], XmNmessageString, message); ac++;
  XtSetArg(args[ac], XmNokLabelString, yes); ac++;
  XtSetArg(args[ac], XmNcancelLabelString, no); ac++;
  
  warning_dialog = XmCreateWarningDialog(sw_geometry.draw_area, "delete_warning", args, ac);

  XmStringFree(yes);
  XmStringFree(no);
  XmStringFree(message);
  XtUnmanageChild(XmMessageBoxGetChild(warning_dialog, XmDIALOG_HELP_BUTTON));
		  
  XtAddCallback(warning_dialog, XmNokCallback, delete_everything,
		(XtPointer)NULL);
  XtAddCallback(warning_dialog, XmNcancelCallback, popdown_dialog,
		(XtPointer)NULL);
  XtManageChild(warning_dialog);
  XtPopup(XtParent(warning_dialog), XtGrabNone);
}



/**********************File menus*******************************/

/****************************************************************
 *        void XcodaEditorNewConfigCbk()                        *
 * Description:                                                 *
 *    callback function for "New" config menu                   *
 ***************************************************************/
void
XcodaEditorNewConfigCbk (Widget w, 
			      XtPointer clientData, 
			      XtPointer callback_data)
{
  XcodaEditorResetGraphCmd();

  newConfigDialogPopup (sw_geometry.draw_area);
}


/****************************************************************
 *        void XcodaEditorOpenConfigCbk()                       *
 * Description:                                                 *
 *    callback function for "Open" config menu                  *
 ***************************************************************/
void
XcodaEditorOpenConfigCbk (Widget w, 
			       XtPointer clientData, 
			       XtPointer callback_data)
{
  XmAnyCallbackStruct* cbs = (XmAnyCallbackStruct *)callback_data;
#ifdef DEBUG
  printf("XcodaEditorOpenConfigCbk reached\n");
#endif
  XcodaEditorResetGraphCmd();

  /* remove all current graphics */
  delete_everything (w, clientData, cbs);
  initConfigSel (sw_geometry.draw_area);
  configSelPopup ();
}

/****************************************************************
 *        void XcodaEditorRemoveConfigCbk()                     *
 * Description:                                                 *
 *    callback function for "Remove" config menu                  *
 ***************************************************************/
void
XcodaEditorRemoveConfigCbk (Widget w, 
				 XtPointer clientData, 
				 XtPointer callback_data)
{
  XcodaEditorResetGraphCmd();

  initRemoveConfigSel (sw_geometry.draw_area);
  removeConfigSelPopup ();
}


/****************************************************************
 *        void XcodaEditorSaveConfig()                          *
 * Description:                                                 *
 *    callback function for "Save As" file menu                 *
 ***************************************************************/
typedef struct config_widget{
  Widget dialog_;
  Widget input_;
  Widget ok_;
  Widget cancel_;
} ConfigWidgets;

static void
confirm_overwrite(Widget w, 
			      XtPointer data, 
			      XtPointer callback_data)
{
  ConfigWidgets* info = (ConfigWidgets *)data;
  XmAnyCallbackStruct* cbs = (XmAnyCallbackStruct *)callback_data;
  char *config_name;

  if(cbs->reason == XmCR_OK)
  {
    config_name = XmTextFieldGetString(info->input_);
    XcodaEditorWriteToConfig (config_name, &coda_graph);
    XcodaEditorInsertAllComps (&coda_graph);

    /* insert components into icons list */
    XcodaEditorInsertCompToIcons(&coda_graph);
    XcodaEditorUpdateEditableFlag(&coda_graph);

    /* update database the selected configuration */
    selectConfigTable (config_name);
    /* update runTYpe information on the window */
    XcodaEditorShowConfigName (config_name);

    XtFree(config_name);
  }
  XtPopdown (XtParent (w));
  XtPopdown (XtParent (info->dialog_));
}

static void
saveConfigOverwritePopup(Widget w, 
				     char* filename, 
				     ConfigWidgets* info)
{
  static Widget q_dialog = 0;
  Widget help_b;
  XmString message, yes, no;
  char     temp[256];
  Arg      args[10];
  int      ac = 0;

  sprintf(temp,"Configuration file %s already exits",filename);
  strcat (temp,"\nOver write it ?");
  message = XmStringCreateLtoR(temp,XmSTRING_DEFAULT_CHARSET);

  if (!q_dialog)
  {
    yes = XmStringCreateSimple("Yes");
    no = XmStringCreateSimple("No");

    XtSetArg(args[ac], XmNmessageString, message); ac++;
    XtSetArg(args[ac], XmNtitle, "Over write"); ac++;
    XtSetArg(args[ac], XmNokLabelString, yes); ac++;
    XtSetArg(args[ac], XmNcancelLabelString, no); ac++;
    q_dialog = XmCreateQuestionDialog(w, "over_write",
				      args, ac);
    ac = 0;
    XmStringFree(yes);
    XmStringFree(no);

    help_b = XmMessageBoxGetChild(q_dialog, XmDIALOG_HELP_BUTTON);
    XtUnmanageChild(help_b);
  
    XtAddCallback(q_dialog, XmNokCallback, confirm_overwrite, info);
    XtAddCallback(q_dialog, XmNcancelCallback, confirm_overwrite, info);

    XtManageChild(q_dialog);
  }
  else
  {
    XtUnmanageChild(q_dialog);

    XtSetArg (args[ac], XmNmessageString, message); ac++;
    XtSetValues (q_dialog, args, ac);
    ac = 0;

    XtManageChild (q_dialog);
  }
  /* free filename which is form a text field widget */
  XtFree (filename);
  /* free xmstring */
  XmStringFree (message);
  XtPopup(XtParent(q_dialog),XtGrabExclusive);
}  

     
/*******Save config: ok button callback of the saveConfig cbks *********/
static void
saveConfig (Widget w, 
			XtPointer clientData, 
			XtPointer callback_data)
{
  char *config;
  ConfigWidgets* obj = (ConfigWidgets *)clientData;

  config = XmTextFieldGetString (obj->input_);
  if (!config || !*config)
  {
    pop_error_message ("No Configuration name entered", sw_geometry.draw_area);
    XtFree (config);
    return;
  }
  if (isConfigCreated (config) > 0 )  /* overwrite existing configuration */
  {
    saveConfigOverwritePopup(sw_geometry.draw_area, config, obj);
  }
  else
  {
    XcodaEditorWriteToConfig (config, &coda_graph);
    XcodaEditorInsertAllComps (&coda_graph);

    /* insert components into icons list */
    XcodaEditorInsertCompToIcons(&coda_graph);
    XcodaEditorUpdateEditableFlag(&coda_graph);

    /* update database the selected configuration */
    selectConfigTable (config);
    /* update runTYpe information on the window */
    XcodaEditorShowConfigName (config);

    XtFree (config);
  }
  XtPopdown (XtParent (obj->dialog_));
}

static void
popdownSaveConfigDialog (Widget w, 
				     XtPointer clientData, 
				     XtPointer callback_data)
{
  ConfigWidgets* obj = (ConfigWidgets *)clientData;
  XtPopdown (XtParent (obj->dialog_));
}

/**********************Config Menu: save as... callback*******************/
static ConfigWidgets saveInfoW;

void
XcodaEditorSaveConfig(Widget w, 
			   XtPointer client_data, 
			   XtPointer callback_data)
{
  Widget sep, pushb0, pushb1;
  Widget label, top_label, act_form;
  XmString t;
  Arg    args[20];
  int    ac = 0, len;
  static int saveDialogCreated = 0;

  if(isEmptyGraph(&coda_graph)) return;

  if (!saveDialogCreated)
  {
    saveDialogCreated = 1;
    saveInfoW.dialog_ = XmCreateFormDialog (sw_geometry.draw_area, "Save Config", NULL, 0);

    t = XmStringCreateSimple ("Enter Configuration Name");
    XtSetArg(args[ac], XmNlabelString, t); ac++;
    XtSetArg(args[ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;
    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNtopOffset, 10); ac++;
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNleftOffset, 5); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNrightOffset, 2); ac++;
    top_label = XtCreateManagedWidget("config_top",xmLabelWidgetClass,
				      saveInfoW.dialog_, args, ac);
    ac = 0;
    XmStringFree(t);

    /* create action buttons */
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNleftOffset, 2); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNrightOffset, 2); ac++;
    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNbottomOffset, 2); ac++;
    act_form = XtCreateWidget ("save_act_form", xmFormWidgetClass,
			       saveInfoW.dialog_, args, ac);
    ac = 0;
    
    t = XmStringCreateSimple("  Ok  ");
    XtSetArg(args[ac], XmNlabelString, t); ac++;
    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_FORM);ac++;
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNleftPosition, 10); ac++;
    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNbottomOffset, 5); ac++;
    XtSetArg(args[ac], XmNshowAsDefault, 1); ac++;
    saveInfoW.ok_ = XtCreateManagedWidget("ok", xmPushButtonWidgetClass,
					  act_form, args, ac);
    /* start_save_file */
    ac = 0;
    XmStringFree(t);

    t = XmStringCreateSimple("Cancel");
    XtSetArg(args[ac], XmNlabelString, t); ac++;
    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_FORM);ac++;
    XtSetArg(args[ac], XmNtopOffset, 5); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNrightPosition,90); ac++;
    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNbottomOffset, 10); ac++;
    saveInfoW.cancel_ = XtCreateManagedWidget("cancel", xmPushButtonWidgetClass,
					      act_form, args, ac);
    ac = 0;
    XmStringFree(t);

    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNleftOffset, 2); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNrightOffset, 2); ac++;
    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(args[ac], XmNbottomWidget, act_form); ac++;
    XtSetArg(args[ac], XmNbottomOffset, 10); ac++;
    sep = XtCreateManagedWidget("sep", xmSeparatorWidgetClass,
				saveInfoW.dialog_, args, ac);
    ac = 0;

    /* text input widget */
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNleftOffset, 5); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNrightOffset, 5); ac++;
    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(args[ac], XmNbottomWidget, sep); ac++;
    XtSetArg(args[ac], XmNbottomOffset, 10); ac++;
    saveInfoW.input_ = XtCreateManagedWidget("text_w", xmTextFieldWidgetClass,
					    saveInfoW.dialog_, args, ac);
    ac = 0;

    /* set resource for label widget */
    XtSetArg (args[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg (args[ac], XmNbottomWidget, saveInfoW.input_); ac++;
    XtSetValues (top_label, args, ac);
    ac = 0;

    /* set default button */
    XtSetArg (args[ac], XmNdefaultButton, saveInfoW.ok_); ac++;
    XtSetValues (saveInfoW.dialog_, args, ac);
    ac = 0;

    /* add callbacks for all push buttons */
    XtAddCallback (saveInfoW.ok_, XmNactivateCallback,
		   saveConfig, (XtPointer)&(saveInfoW));
    XtAddCallback (saveInfoW.cancel_, XmNactivateCallback,
		   popdownSaveConfigDialog, (XtPointer)&(saveInfoW));

    XtManageChild(act_form);
    XtManageChild(saveInfoW.dialog_);
  }

  XtPopup(XtParent (saveInfoW.dialog_), XtGrabNone);
}

/****************************************************************
 *        void XcodaEditorSaveDefault()                         *
 * Description:                                                 *
 *    Callback function for "Save" menu entry in the            *
 *   "Configuration"                                            *
 *    Menu. Save Current Drawing to a default configuration file*
 ***************************************************************/

void (*Editor_save_callback)(void *) = NULL;
void *Editor_rc_object = NULL;

void
XcodaEditorSaveDefault(Widget w, 
			    XtPointer client_data, 
			    XtPointer callback_data)
{
  XmAnyCallbackStruct* cbs = (XmAnyCallbackStruct *)callback_data;
  XcodaEditorResetGraphCmd();

  if(isEmptyGraph(&coda_graph))
    return;

  if (currentConfigTable ()) /* already has current configuration */
  {
    XcodaEditorWriteToConfig (currentConfigTable(), &coda_graph);
    XcodaEditorInsertAllComps (&coda_graph);

    /* insert components into icons list */
    XcodaEditorInsertCompToIcons(&coda_graph);
    XcodaEditorUpdateEditableFlag(&coda_graph);

    if (Editor_save_callback)
      (*Editor_save_callback)(Editor_rc_object);
    
  }
  else  /* no default selected */
  {
    XcodaEditorSaveConfig (w, client_data, cbs);
  }
}


/******************************************************************
 *         static void popupSaveQDialog(w, msg, client_data)      *
 * Description:                                                   *
 *     Popup a question dialog to ask user to save or not         *
 *****************************************************************/
static void
popdown_shell(Widget w, 
			  XtPointer client_data, 
			  XtPointer callback_data)
{
  Widget shell = XtParent(XtParent(w));

  XtDestroyWidget(shell);
}

static void
saveAction(Widget w, 
		       XtPointer client_data, 
		       XtPointer callback_data)
{
  XmAnyCallbackStruct* cbs = (XmAnyCallbackStruct *)callback_data;
  popdown_shell(w, client_data, cbs);
  XcodaEditorSaveConfig(w, client_data, cbs);
}

static void
noSaveAction(Widget w, 
			 XtPointer client_data, 
			 XtPointer callback_data)
{
  int type = (int)client_data;
  XmAnyCallbackStruct* cbs = (XmAnyCallbackStruct *)callback_data;

  delete_everything(w, client_data, cbs);
  delete_everyicons( );
  popdown_shell(w,client_data, cbs);

}

static void
popupSaveQDialog(Widget parent, 
			     XmString msg, 
			     XtPointer client_data)
{
  XmString yes, no, cancel;
  Position ret_x, ret_y;
  Widget   q_dialog, form, sep, label;
  Widget   pushb0, pushb1, pushb2;
  Arg      args[15];
  int      ac = 0;

  yes = XmStringCreateSimple("Yes");
  no = XmStringCreateSimple("No");
  cancel = XmStringCreateSimple("Cancel");

  XtTranslateCoords(parent, 40, 40, &ret_x, &ret_y);

  XtSetArg(args[ac], XmNtitle, "QuestionDialog"); ac++;
  XtSetArg(args[ac], XmNx, ret_x); ac++;
  XtSetArg(args[ac], XmNy, ret_y); ac++;

  printf("Editor_menu:popupSaveQDialog: q_shell\n");fflush(stdout);

  q_dialog = XtCreatePopupShell("q_shell", transientShellWidgetClass,
				parent, args ,ac);
  ac = 0;
    
  form = XtCreateWidget("q_form", xmFormWidgetClass, q_dialog, NULL, 0);

  XtSetArg(args[ac], XmNlabelString, msg); ac++;
  XtSetArg(args[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNtopOffset, 10); ac++;
  XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNleftOffset, 10); ac++;
  XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNrightOffset, 10); ac++;
  label = XtCreateManagedWidget("label",xmLabelWidgetClass, form,
				args, ac);
  ac = 0;

  XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(args[ac], XmNtopWidget, label); ac++;
  XtSetArg(args[ac], XmNtopOffset, 10); ac++;
  XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  sep = XtCreateManagedWidget("sep", xmSeparatorWidgetClass, form,
			      args, ac);
  ac = 0;

  XtSetArg(args[ac], XmNlabelString, yes); ac++;
  XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(args[ac], XmNtopWidget, sep); ac++;
  XtSetArg(args[ac], XmNtopOffset, 10); ac++;
  XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
  XtSetArg(args[ac], XmNleftPosition, 10); ac++;
  XtSetArg(args[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
  XtSetArg(args[ac], XmNrightPosition, 30); ac++;
  XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNbottomOffset, 5); ac++;
  pushb0 = XtCreateManagedWidget("ok", xmPushButtonWidgetClass, form,
				 args, ac);
  XtAddCallback(pushb0, XmNactivateCallback, saveAction, client_data);
  ac = 0;
  XmStringFree(yes);

  XtSetArg(args[ac], XmNlabelString, no); ac++;
  XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(args[ac], XmNtopWidget, sep); ac++;
  XtSetArg(args[ac], XmNtopOffset, 10); ac++;
  XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
  XtSetArg(args[ac], XmNleftPosition, 40); ac++;
  XtSetArg(args[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
  XtSetArg(args[ac], XmNrightPosition, 60); ac++;
  XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNbottomOffset, 5); ac++;
  pushb1 = XtCreateManagedWidget("no", xmPushButtonWidgetClass, form,
				 args, ac);
  XtAddCallback(pushb1, XmNactivateCallback, noSaveAction, client_data);
  ac = 0;
  XmStringFree(no);

  XtSetArg(args[ac], XmNlabelString, cancel); ac++;
  XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(args[ac], XmNtopWidget, sep); ac++;
  XtSetArg(args[ac], XmNtopOffset, 5); ac++;
  XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
  XtSetArg(args[ac], XmNleftPosition, 70); ac++;
  XtSetArg(args[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
  XtSetArg(args[ac], XmNrightPosition, 90); ac++;
  XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNbottomOffset, 5); ac++;
  pushb2 = XtCreateManagedWidget("cancel", xmPushButtonWidgetClass, form,
				 args, ac);
  XtAddCallback(pushb2, XmNactivateCallback, popdown_shell, (XtPointer)NULL);
  ac = 0;
  XmStringFree(cancel);    
  
  XtManageChild(form);
  XtAddGrab(q_dialog, True, False);
  XtPopup(q_dialog, XtGrabNone);  
}

/******************************************************************
 *         void XcodaEditorConfigOption (w, client_data, cbs)     *
 * Description:                                                   *
 *      Callback function for constructing a option for a run     *
 *      type                                                      *
 *****************************************************************/
#define EDITOR_MAX_NUM_OPTIONS 10
typedef struct _editor_config_option_w
{
  Widget holder[EDITOR_MAX_NUM_OPTIONS];
  Widget option[EDITOR_MAX_NUM_OPTIONS];
  Widget value[EDITOR_MAX_NUM_OPTIONS];
  Widget shell;
  Widget label;
  char   runtype[80];
  int    saved;
} editorConfigOptionW;

static editorConfigOptionW opW;
static int opwCreated = 0;
static int numOptions = 0;

static void
saveOptions (Widget w, XtPointer clientData, 
			 XtPointer callback_data)
{
#ifdef DEBUG
  printf("Editor_menu::saveOptions reached\n");
#endif
  editorConfigOptionW *obj = (editorConfigOptionW *)clientData;

  obj->saved = 1;
  XtPopdown (obj->shell);
}

static void
cancelOptionDialog (Widget w, XtPointer clientData, 
				XtPointer callback_data)
{     
#ifdef DEBUG
  printf("Editor_menu::cancelOptionDialog reached\n");
#endif
  editorConfigOptionW *obj = (editorConfigOptionW *)clientData;
  
  obj->saved = 0;
  obj->runtype[0] = '\0';
  XtPopdown (obj->shell);
}

static void
updateOptionWidgets (char* runtype)
{
#ifdef DEBUG
  printf("Editor_menu::updateOptionWidget reached\n");
#endif
  char  temp[128];
  Arg   arg[20];
  int   ac = 0;
  XmString t;
  char** options;
  char** values;
  int i;
  int realnum;

  /* first unmanage all old information */
  for (i = 0; i < EDITOR_MAX_NUM_OPTIONS; i++)
  {
#ifdef DEBUG
    printf("\n\nEditor_menu 1111111111111111\n\n\n\n");
#endif
    XmTextFieldSetString (opW.option[i], "");
    XmTextFieldSetString (opW.value[i], "");
  }

  /* set saved flag to 0 and set widget config name to run type */
  strncpy (opW.runtype, runtype, sizeof (opW.runtype));
#ifdef DEBUG
  printf("\n\nEditor_menu:updateOptionWidgets: >%s< >%s<\n\n\n\n",opW.runtype, runtype);
#endif
  opW.saved = 0;
  
  sprintf (temp, "Options for run type %s", runtype);
  t = XmStringCreateSimple (temp);
  XtSetArg (arg[ac], XmNlabelString, t); ac++;
  XtSetValues (opW.label, arg, ac); 
  ac = 0;
  XmStringFree (t);

  numOptions = getAllOptionInfos (runtype, &options, &values);
  realnum = (numOptions < EDITOR_MAX_NUM_OPTIONS) ? numOptions : EDITOR_MAX_NUM_OPTIONS;
  
  for (i = 0; i < realnum; i++)
  {
    /* update label for option name */
    /* option name */
#ifdef DEBUG
    printf("\n\nEditor_menu 111111111111111122222222222222\n\n\n\n");
#endif
    XmTextFieldSetString (opW.option[i], options[i]);
    /* option value */
    XmTextFieldSetString (opW.value[i], values[i]);
  }

  /* free memory */
  if (numOptions > 0)
  {
    for (i = 0; i < numOptions; i++)
    {
      free (options[i]);
      free (values[i]);
    }
    free (options);
    free (values);
  }
}

void
XcodaEditorResetOptionDialog (void)
{
  int i = 0;

#ifdef DEBUG
  printf("Editor_menu::XcodaEditorResetOptionDialog reached\n");
#endif
  if (opwCreated)
  {
    /* first unmanage all old information */
    for (i = 0; i < EDITOR_MAX_NUM_OPTIONS; i++)
    {
#ifdef DEBUG
      printf("\n\nEditor_menu 1111111111111111333333333333333333\n\n\n\n");
#endif
      XmTextFieldSetString (opW.option[i], "");
      XmTextFieldSetString (opW.value[i], "");
    }

    /* set saved flag to 0 and set widget config name to run type */
    opW.runtype[0] = '\0';
    opW.saved = 0;
    numOptions = 0;
  }
}

void
XcodaEditorSaveConfigOption (char* runtype)
{
  int lenopt, lenval;
  int  i = 0;
  char* value = 0;
  char* option = 0;

#ifdef DEBUG
  printf("Editor_menu::XcodaEditorSaveConfigOption reached\n");
  printf("Editor_menu:insertValToOptionTable ???\n");
  printf("Editor_menu 1: opW.saved=%d, >%s< >%s<\n",opW.saved,opW.runtype, runtype);
#endif

/* sergey: opW.saved is never true,  opW.runtype nevet set ... */
/* if used, segfault in 'XmTextFieldGetString (opW.option[i]);' */
  if (!opW.saved && strcmp (opW.runtype, runtype) == 0)
  {
#ifdef DEBUG
    printf("Editor_menu 2\n");
#endif
    for (i = 0; i < EDITOR_MAX_NUM_OPTIONS; i++)
    {
#ifdef DEBUG
      printf("Editor_menu 3 (%d)\n",i);fflush(stdout);
      printf("\n\nEditor_menu 1111111111111111444444444444444\n\n\n\n");
#endif
      option = XmTextFieldGetString (opW.option[i]);
#ifdef DEBUG
      printf("Editor_menu 31 %d\n",option);fflush(stdout);
#endif
      value =  XmTextFieldGetString (opW.value[i]);
	  printf("Editor_menu 32 %d\n",value);fflush(stdout);
      if (option && (*option))
      {
#ifdef DEBUG
        printf("Editor_menu 4\n");fflush(stdout);
#endif
	    if (value && *value)
        {
#ifdef DEBUG
          printf("Editor_menu 5\n");fflush(stdout);
#endif
          /*insertValToOptionTable (runtype, option, value);*/
	      XtFree (value);
	    }
	    else
		{
#ifdef DEBUG
          printf("Editor_menu 6\n");fflush(stdout);
#endif
	      /*insertValToOptionTable (runtype, option, 0);*/
		}
	    XtFree (option);
      }
    }
  }
}
      
void
XcodaEditorConfigOption (Widget w, XtPointer client_data,
						 XtPointer callback_data)
{
  Widget form, sep;
  Widget ok, cancel, subform, subform1;
  Widget rowcol, rwlabel, rwval;
  Arg    arg[20];
  int    ac = 0;
  XmString t;
  int    i = 0;

#ifdef DEBUG
  printf("Editor_menu::XcodaEditorConfigOption reached\n");
#endif

  if (currentConfigTable () == 0)
  {
    pop_error_message ("Select a run type first.", sw_geometry.draw_area);
    return;
  }

  if (!opwCreated)
  {
    opwCreated = 1;
    opW.saved = 0;  /* options will be saved */

    form = XmCreateFormDialog (sw_geometry.draw_area,
			       "optionDialog",
			       NULL, 0);
    /* create label */
    t = XmStringCreateSimple ("Options for run type                ");
    XtSetArg (arg[ac], XmNlabelString, t); ac++;
    XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNtopOffset, 5); ac++;
    XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNleftOffset, 2); ac++;
    XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNrightOffset, 2); ac++;
    opW.label = XtCreateManagedWidget ("dialogLabel", xmLabelGadgetClass,
				       form, arg, ac);
    ac = 0;
    XmStringFree (t);


    /* create ok/cancel buttons */
    XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNbottomOffset, 5); ac++;
    XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNleftOffset, 2); ac++;
    XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNrightOffset, 2); ac++;
    subform = XtCreateWidget ("subform", xmFormWidgetClass,
			      form, arg, ac);
    ac = 0;

    t = XmStringCreateSimple ("  Ok  ");
    XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
    XtSetArg (arg[ac], XmNleftPosition, 15); ac++;
    XtSetArg (arg[ac], XmNlabelString, t); ac++;
    XtSetArg (arg[ac], XmNdefaultButtonShadowThickness, 1); ac++;
    ok = XtCreateManagedWidget ("ok", xmPushButtonGadgetClass,
				subform, arg, ac);
    ac = 0;
    XmStringFree (t);

    t = XmStringCreateSimple ("Dismiss");
    XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNbottomOffset, 5); ac++;
    XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNtopOffset, 5); ac++;
    XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
    XtSetArg (arg[ac], XmNrightPosition, 85); ac++;
    XtSetArg (arg[ac], XmNlabelString, t); ac++;
    cancel = XtCreateManagedWidget ("cancel", xmPushButtonGadgetClass,
				    subform, arg, ac);
    ac = 0;
    XmStringFree (t);
    
    /* create a separator */
    XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg (arg[ac], XmNbottomWidget, subform); ac++;
    XtSetArg (arg[ac], XmNbottomOffset, 5); ac++;
    XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;    
    XtSetArg (arg[ac], XmNrightOffset, 2) ;ac++;
    XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNleftOffset, 2); ac++;
    XtSetArg (arg[ac], XmNorientation, XmHORIZONTAL); ac++;
    sep = XtCreateManagedWidget ("sep", xmSeparatorWidgetClass,
				 form, arg, ac);
    ac = 0;

    /* create a row column widget */
    XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg (arg[ac], XmNbottomWidget, sep); ac++;
    XtSetArg (arg[ac], XmNbottomOffset, 5); ac++;
    XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg (arg[ac], XmNtopWidget, opW.label); ac++;
    XtSetArg (arg[ac], XmNtopOffset, 5); ac++;
    XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;    
    XtSetArg (arg[ac], XmNrightOffset, 2); ac++;
    XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNleftOffset, 2); ac++;
    XtSetArg (arg[ac], XmNorientation, XmVERTICAL); ac++;
    XtSetArg (arg[ac], XmNnumColumns, 1); ac++;
    rowcol = XtCreateWidget ("rowcol", xmRowColumnWidgetClass,
			     form, arg, ac);
    ac = 0;

    subform1= XtCreateWidget ("holder", xmFormWidgetClass,
			      rowcol, NULL, 0);
      
    t = XmStringCreateSimple ("Option Name");
    XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNleftOffset, 2); ac++;
    XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
    XtSetArg (arg[ac], XmNrightPosition, 45); ac++;
    XtSetArg (arg[ac], XmNlabelString, t); ac++;
    XtSetArg (arg[ac], XmNrecomputeSize, FALSE); ac++;
    XtSetArg (arg[ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;
    rwlabel = XtCreateManagedWidget ("label",
				     xmLabelGadgetClass,
				     subform1, arg, ac);
    ac = 0;
    XmStringFree (t);

    t = XmStringCreateSimple ("Option Value");
    XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
    XtSetArg (arg[ac], XmNleftPosition, 55); ac++;
    XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNrightOffset, 2); ac++;
    XtSetArg (arg[ac], XmNlabelString, t); ac++;
    XtSetArg (arg[ac], XmNalignment, XmALIGNMENT_END); ac++;
    rwval = XtCreateManagedWidget ("entry",
				   xmLabelGadgetClass,
				   subform1, arg, ac);
    ac = 0;
    XmStringFree (t);


    for (i = 0; i < EDITOR_MAX_NUM_OPTIONS; i++)
    {
      opW.holder[i] = XtCreateWidget ("holder", xmFormWidgetClass,
				      rowcol, NULL, 0);
      
      XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
      XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
      XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
      XtSetArg (arg[ac], XmNleftOffset, 2); ac++;
      XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
      XtSetArg (arg[ac], XmNrightPosition, 45); ac++;
      opW.option[i] = XtCreateManagedWidget ("optionName",
					     xmTextFieldWidgetClass,
					     opW.holder[i], arg, ac);
      ac = 0;
    
      XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
      XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
      XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
      XtSetArg (arg[ac], XmNleftPosition, 55); ac++;
      XtSetArg (arg[ac], XmNleftOffset, 2); ac++;
      XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
      XtSetArg (arg[ac], XmNrightOffset, 2); ac++;
      opW.value[i] = XtCreateManagedWidget ("entry",
					    xmTextFieldWidgetClass,
					    opW.holder[i], arg, ac);
      ac = 0;

      XtManageChild (opW.holder[i]);
    }
    
    XtSetArg (arg[ac], XmNdefaultButton, ok); ac++;
    XtSetValues (form, arg, ac);
    ac = 0;

    XtManageChild (subform);
    XtManageChild (subform1);
    XtManageChild (rowcol);
    XtManageChild (form);

    opW.shell = XtParent (form);

    /* add callback for ok and cancel */
    XtAddCallback (ok, XmNactivateCallback, saveOptions,
		   (XtPointer)&opW);
    XtAddCallback (cancel, XmNactivateCallback, cancelOptionDialog,
		   (XtPointer)&opW);
  }

  /* update contents of the display */
  updateOptionWidgets (currentConfigTable ());
  XtPopup (opW.shell, XtGrabNone);
}
    

/******************************************************************
 *         void XcodaEditorNewDbaseCbk(w, client_data, cbs)      *
 * Description:                                                   *
 *     Callback function for New DatabaseButton in the File Menu  *
 *****************************************************************/
typedef struct _newdbase_info
{
  Widget dialog_;
  Widget input_;
  Widget ok_;
  Widget cancel_;
} newDbaseDialog;

static void
openNewDatabase (Widget w, 
			     XtPointer clientData,
			     XtPointer callback_data)
{
  char* str;
  char  msg[80];
  newDbaseDialog* obj = (newDbaseDialog *)clientData;

#ifdef DEBUG
  printf("Editor_menu::openNewDatabase reached\n");
#endif

  str = XmTextFieldGetString (obj->input_);
  if (!str || !*str) {
    pop_error_message ("Need new database name", sw_geometry.draw_area);
    XtFree (str);
    return;
  }
  if (createNewDatabase (str) < 0) {
    sprintf (msg, "Cannot create database %s",str);
    XtFree (str);
    pop_error_message (msg, sw_geometry.draw_area);
    return;
  }
  XcodaEditorShowDatabaseName (str);
  /* enable editing */
  XcodaEditorEnableInput ();
  XtFree (str);
  XtPopdown (XtParent (obj->dialog_));
}

static void
popdownNdbaseDialog (Widget w, 
				 XtPointer clientData, 
				 XtPointer callback_data)
{
  newDbaseDialog* obj = (newDbaseDialog *)clientData;

  XtPopdown (XtParent (obj->dialog_));
}

void
XcodaEditorNewDbaseEntry (Widget w)
{
  static newDbaseDialog ndbaseDialog;
  static int ndbaseCreated = 0;
  Widget act_form, sep, topLabel;
  Arg    arg[20];
  int    ac = 0;
  XmString t;

  XcodaEditorResetGraphCmd();

  if (!ndbaseCreated) {
    ndbaseCreated = 1;
    ndbaseDialog.dialog_ = XmCreateFormDialog (w,
					       "New Database", NULL, 0);
    
    /* create action form */
    XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNbottomOffset, 5); ac++;
    XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNleftOffset, 2); ac++;
    XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNrightOffset, 2); ac++;
    act_form = XtCreateWidget ("newDbaseActForm", xmFormWidgetClass,
			       ndbaseDialog.dialog_, arg, ac);
    ac = 0;

    t = XmStringCreateSimple ("  Ok   ");
    XtSetArg (arg[ac], XmNlabelString, t); ac++;
    XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
    XtSetArg (arg[ac], XmNleftPosition, 10); ac++;
    XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNshowAsDefault, 1); ac++;
    ndbaseDialog.ok_ = XtCreateManagedWidget ("okButton",
					      xmPushButtonWidgetClass,
					      act_form, arg, ac);
    ac = 0;
    XmStringFree (t);

    t = XmStringCreateSimple ("Cancel");
    XtSetArg (arg[ac], XmNlabelString, t); ac++;
    XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
    XtSetArg (arg[ac], XmNrightPosition, 90); ac++;
    XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNtopOffset, 5); ac++;
    XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNbottomOffset, 5); ac++;
    ndbaseDialog.cancel_ = XtCreateManagedWidget ("cancelButton",
						  xmPushButtonWidgetClass,
						  act_form, arg, ac);
    ac = 0;
    XmStringFree (t);

    /* create a seperator */
    XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNleftOffset, 2); ac++;
    XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNrightOffset, 2); ac++;
    XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg (arg[ac], XmNbottomWidget, act_form); ac++;
    XtSetArg (arg[ac], XmNbottomOffset, 10); ac++;
    XtSetArg (arg[ac], XmNorientation, XmHORIZONTAL); ac++;
    sep = XtCreateManagedWidget ("sep", xmSeparatorWidgetClass,
				 ndbaseDialog.dialog_, arg, ac);
    ac = 0;

    /* create text input field */
    XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNleftOffset, 5); ac++;
    XtSetArg (arg[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNrightOffset, 5); ac++;
    XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg (arg[ac], XmNbottomWidget, sep); ac++;
    XtSetArg (arg[ac], XmNbottomOffset, 10); ac++;
    XtSetArg (arg[ac], XmNcolumns, 40); ac++;
    ndbaseDialog.input_ = XtCreateManagedWidget ("ndabaseInput", xmTextFieldWidgetClass,
						 ndbaseDialog.dialog_, arg, ac);
    ac = 0;

    /* create top label */
    t = XmStringCreateSimple ("New Database Name:");
    XtSetArg (arg[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNleftOffset, 5); ac++;
    XtSetArg (arg[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg (arg[ac], XmNbottomWidget, ndbaseDialog.input_); ac++;
    XtSetArg (arg[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg (arg[ac], XmNtopOffset, 5); ac++;
    XtSetArg (arg[ac], XmNlabelString, t); ac++;
    topLabel = XtCreateManagedWidget ("ndbaseLabel", xmLabelWidgetClass,
				      ndbaseDialog.dialog_, arg, ac);
    ac = 0;
    XmStringFree (t);

    /* set default button */
    XtSetArg (arg[ac], XmNdefaultButton, ndbaseDialog.ok_); ac++;
    XtSetValues (ndbaseDialog.dialog_, arg, ac);
    ac = 0;

    /* add callbacks */
    XtAddCallback (ndbaseDialog.ok_, XmNactivateCallback, 
		   openNewDatabase, (XtPointer)&(ndbaseDialog));
    XtAddCallback (ndbaseDialog.cancel_, XmNactivateCallback,
		   popdownNdbaseDialog, (XtPointer)&(ndbaseDialog));

    XtManageChild (act_form);
    XtManageChild (ndbaseDialog.dialog_);
  }
  XtPopup (XtParent (ndbaseDialog.dialog_), XtGrabNone);
}

void
XcodaEditorNewDbaseCbk(Widget w, 
			    XtPointer client_data, 
				XtPointer callback_data)
{
  if (isEmptyGraph (&coda_graph))
    XcodaEditorNewDbaseEntry (sw_geometry.draw_area);
  else 
    opendbaseDialogPopup (sw_geometry.draw_area, 0);
}



/******************************************************************
 *         void XcodaEditorOpenDbaseCbk(w, client_data, cbs)     *
 * Description:                                                   *
 *     Callback function for Open Database Button in the File Menu*
 *****************************************************************/
void
XcodaEditorOpenDbaseCbk(Widget w, 
			     XtPointer client_data, 
			     XtPointer callback_data)
{
  XcodaEditorResetGraphCmd();
  
  if (isEmptyGraph (&coda_graph) && isEmptyIconList ()) {
    initDbaseSel (sw_geometry.draw_area);
    dbaseSelPopup ();
  }
  else 
    opendbaseDialogPopup (sw_geometry.draw_area, 1);
}

/**********************************************************************
 *         void XcodaEditorRemoveDbaseCbk(w, client_data, cbs)        *
 * Description:                                                       *
 *     Callback function for Remove Database Button in the File Menu  *
 *********************************************************************/
void
XcodaEditorRemoveDbaseCbk(Widget w, 
			       XtPointer client_data, 
			       XtPointer callback_data)
{
  XcodaEditorResetGraphCmd();

  initRemoveDbaseSel (sw_geometry.draw_area);
  removeDbaseSelPopup ();
}

/****************************************************************
 *         void XcodaEditorCleanmDbase(w, cdata, cbs)           *
 * Description:                                                 *
 *      Remove all unused components from database              *
 ***************************************************************/
void
XcodaEditorCleanDbase (Widget w, 
			    XtPointer client_data, 
			    XtPointer callback_data)
{
  /* check all the components inside icon list
     forward task to iconlist
   */
  compressIconList();
}

/*********************Exit Menu CAllback*************************/
static void
exit_coda_editor(Widget w, 
			     XtPointer client_data, 
			     XtPointer callback_data)
{
  XmAnyCallbackStruct* cbs = (XmAnyCallbackStruct *)callback_data;

  XFreeGC(xgc.dpy, xgc.erase_gc);
  XFreeGC(xgc.dpy, xgc.r_gc);
  XFreeGC(xgc.dpy, xgc.xor_gc);
  XFreeGC(xgc.dpy, xgc.hl_gc);
  XFreeFont(xgc.dpy, xgc.font[0]);
  XFreeFont(xgc.dpy, xgc.font[2]);
  XFreeFont(xgc.dpy, xgc.font[3]);
  XFreeFont(xgc.dpy, xgc.font[4]);
  XFreeFont(xgc.dpy, xgc.font[5]);
  XFreeCursor(xgc.dpy, coda_graph.delete_cursor);
  XFreeCursor(xgc.dpy, coda_graph.delete_arc_cursor);
  XFreeCursor(xgc.dpy, coda_graph.working_cursor);
  XFreeCursor(xgc.dpy, coda_graph.finger_cursor);
  XFreePixmap(xgc.dpy, btn_pixmaps.roc);
  XFreePixmap(xgc.dpy, btn_pixmaps.eb);
  XFreePixmap(xgc.dpy, btn_pixmaps.ana);
  XFreePixmap(xgc.dpy, btn_pixmaps.trig);
  XFreePixmap(xgc.dpy, btn_pixmaps.ebana);
  XFreePixmap(xgc.dpy, btn_pixmaps.hl_roc);  
  XFreePixmap(xgc.dpy, btn_pixmaps.hl_eb);  
  XFreePixmap(xgc.dpy, btn_pixmaps.hl_ana);  
  XFreePixmap(xgc.dpy, btn_pixmaps.hl_trig);  
  XFreePixmap(xgc.dpy, btn_pixmaps.hl_ebana);
  XFreePixmap(xgc.dpy, btn_pixmaps.eth_input);   
  XFreePixmap(xgc.dpy, btn_pixmaps.eth_input_sel);   
  XFreePixmap(xgc.dpy, btn_pixmaps.move_node);   
  XFreePixmap(xgc.dpy, btn_pixmaps.move_node_sel);   
  XFreePixmap(xgc.dpy, btn_pixmaps.roc_btn);  
  XFreePixmap(xgc.dpy, btn_pixmaps.eb_btn);  
  XFreePixmap(xgc.dpy, btn_pixmaps.ana_btn);  
  XFreePixmap(xgc.dpy, btn_pixmaps.ebana_btn);
  XFreePixmap(xgc.dpy, btn_pixmaps.trig_btn); 
  XFreePixmap(xgc.dpy, btn_pixmaps.icon); 
  
  XFreePixmap(xgc.dpy, sw_geometry.bg_pixmap);

  delete_everything(w, client_data, cbs);
  /* free head and tail etc */
  free (coda_graph.comp_list_head);
  free (coda_graph.comp_list_tail);
  free (coda_graph.arc_list_head);
  free (coda_graph.arc_list_tail);
  delete_everyicons();

  /* close database */
  closeDatabase ();

  exit(0);
}
  
void
XcodaEditorExitMenuCbk(Widget w, 
			    XtPointer client_data, 
			    XtPointer callback_data)
{
  Widget dialog, help_w;
  XmString message, yes, no;
  Arg      args[10];
  int      ac = 0;

  /* reset toggle part of graph commands */
  XcodaEditorResetGraphCmd();

  message = XmStringCreateSimple("Do you really want to quit ?");
  yes = XmStringCreateSimple("Yes");
  no  = XmStringCreateSimple("No");
  XtSetArg(args[ac], XmNmessageString, message); ac++;
  XtSetArg(args[ac], XmNokLabelString, yes); ac++;
  XtSetArg(args[ac], XmNcancelLabelString, no); ac++;
  XtSetArg(args[ac], XmNtitle,"Exit Popup"); ac++;
  dialog = XmCreateQuestionDialog(XtParent(XtParent(w)),"exit_dialog",args, ac);
  ac = 0;
  XmStringFree(message);
  XmStringFree(yes);
  XmStringFree(no);
  help_w = XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON);
  XtSetSensitive(help_w, False);
  
  XtAddCallback(dialog, XmNokCallback, exit_coda_editor,(XtPointer)NULL);
  XtAddCallback(dialog, XmNcancelCallback, popdown_dialog, (XtPointer)NULL);
  
  XtManageChild(dialog);
  XtPopup(XtParent(dialog), XtGrabNone);
}

    
