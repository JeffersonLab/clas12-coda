/*----------------------------------------------------------------------------- * copyright (c) 1991,1992 Southeastern Universities Research Association,
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
 *	availabel command pushbuttons for config editor
 *	
 * Author:  Jie Chen, CEBAF Data Acquisition Group
 *
 * Revision History:
 *   $Log: Editor_cmd_btns.c,v $
 *   Revision 1.4  1997/09/08 15:19:10  heyes
 *   fix dd icon etc
 *
 *   Revision 1.3  1997/08/29 12:25:26  heyes
 *   fixed window positioning
 *
 *   Revision 1.2  1997/06/20 16:59:46  heyes
 *   clean up GUI!
 *
 *   Revision 1.1.1.2  1996/11/05 17:45:04  chen
 *   coda source
 *
 *	  
 */
#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/CascadeB.h>

#include "Editor_pixmap.h"
#include "Editor_layout.h"
#include "Editor_graph.h"

#define EDITOR_MAX_NEW_TYPES 20

/* static variables for user defined types */
static Widget ed_new_types[EDITOR_MAX_NEW_TYPES];
static char   ed_new_type_names[EDITOR_MAX_NEW_TYPES][80];
static int    ed_new_numtypes = 0;

static Widget
XcodaConfigRocButton(Widget parent)
{
  Widget pb;
  Arg    args[5];

  XtSetArg(args[0], XmNlabelType,  XmPIXMAP);
  XtSetArg(args[1], XmNlabelPixmap,btn_pixmaps.roc_btn);
  pb = XtCreateManagedWidget("roc_button", xmPushButtonWidgetClass,
		      parent, args, 2);
  manager.roc_btn = pb;
  return pb;
}

static Widget
XcodaConfigEbButton(Widget parent)
{
  Widget pb;
  Arg    args[5];

  XtSetArg(args[0], XmNlabelType,  XmPIXMAP);
  XtSetArg(args[1], XmNlabelPixmap,btn_pixmaps.eb_btn);
  pb = XtCreateManagedWidget("eb_button", xmPushButtonWidgetClass,
		      parent, args, 2);
  manager.eb_btn = pb;
  return pb;
}



/*sergey*/
static Widget
XcodaConfigEtButton(Widget parent)
{
  Widget pb;
  Arg    args[5];

  XtSetArg(args[0], XmNlabelType,  XmPIXMAP);
  XtSetArg(args[1], XmNlabelPixmap,btn_pixmaps.et_btn);
  pb = XtCreateManagedWidget("eb_button", xmPushButtonWidgetClass,
		      parent, args, 2);
  manager.et_btn = pb;
  return pb;
}

/*sergey*/
static Widget
XcodaConfigEttButton(Widget parent)
{
  Widget pb;
  Arg    args[5];

  XtSetArg(args[0], XmNlabelType,  XmPIXMAP);
  XtSetArg(args[1], XmNlabelPixmap,btn_pixmaps.ett_btn);
  pb = XtCreateManagedWidget("eb_button", xmPushButtonWidgetClass,
		      parent, args, 2);
  manager.ett_btn = pb;
  return pb;
}

/*sergey*/
static Widget
XcodaConfigL3Button(Widget parent)
{
  Widget pb;
  Arg    args[5];

  XtSetArg(args[0], XmNlabelType,  XmPIXMAP);
  XtSetArg(args[1], XmNlabelPixmap,btn_pixmaps.l3_btn);
  pb = XtCreateManagedWidget("eb_button", xmPushButtonWidgetClass,
		      parent, args, 2);
  manager.l3_btn = pb;
  return pb;
}



static Widget
XcodaConfigERButton(Widget parent)
{
  Widget pb;
  Arg    args[5];

  XtSetArg(args[0], XmNlabelType,  XmPIXMAP);
  XtSetArg(args[1], XmNlabelPixmap,btn_pixmaps.er_btn);
  pb = XtCreateManagedWidget("eb_button", xmPushButtonWidgetClass,
		      parent, args, 2);
  manager.er_btn = pb;
  return pb;
}

static Widget
XcodaConfigFIButton(Widget parent)
{
  Widget pb;
  Arg    args[5];

  XtSetArg(args[0], XmNlabelType,  XmPIXMAP);
  XtSetArg(args[1], XmNlabelPixmap,btn_pixmaps.fi_btn);
  pb = XtCreateManagedWidget("eb_button", xmPushButtonWidgetClass,
		      parent, args, 2);
  manager.fi_btn = pb;
  return pb;
}

static Widget
XcodaConfigCFIButton(Widget parent)
{
  Widget pb;
  Arg    args[5];

  XtSetArg(args[0], XmNlabelType,  XmPIXMAP);
  XtSetArg(args[1], XmNlabelPixmap,btn_pixmaps.cfi_btn);
  pb = XtCreateManagedWidget("eb_button", xmPushButtonWidgetClass,
		      parent, args, 2);
  manager.cfi_btn = pb;
  return pb;
}


static Widget
XcodaConfigDBGButton(Widget parent)
{
  Widget pb;
  Arg    args[5];

  XtSetArg(args[0], XmNlabelType,  XmPIXMAP);
  XtSetArg(args[1], XmNlabelPixmap,btn_pixmaps.dbg_btn);
  pb = XtCreateManagedWidget("eb_button", xmPushButtonWidgetClass,
		      parent, args, 2);
  manager.dbg_btn = pb;
  return pb;
}



static Widget
XcodaConfigDDButton(Widget parent)
{
  Widget pb;
  Arg    args[5];

  XtSetArg(args[0], XmNlabelType,  XmPIXMAP);
  XtSetArg(args[1], XmNlabelPixmap,btn_pixmaps.dd_btn);
  pb = XtCreateManagedWidget("dd_button", xmPushButtonWidgetClass,
		      parent, args, 2);
  manager.dd_btn = pb;
  return pb;
}



static Widget
XcodaConfigNoneButton(Widget parent)
{
  Widget pb;
  Arg    args[5];

  XtSetArg(args[0], XmNlabelType,  XmPIXMAP);
  XtSetArg(args[1], XmNlabelPixmap,btn_pixmaps.trash);
  pb = XtCreateManagedWidget("none_button", xmPushButtonWidgetClass,
		      parent, args, 2);
  manager.none_btn = pb;
  return pb;
}

static void
newTypeSelected (Widget w, XtPointer data, 
			     XmAnyCallbackStruct *cbs)
{
  int index = (int)data;

  printf ("index is %d\n", index);
}

static Widget
XcodaConfigTrigButton(Widget parent)
{
  Widget pb;
  Arg    args[5];

  XtSetArg(args[0], XmNlabelType,  XmPIXMAP);
  XtSetArg(args[1], XmNlabelPixmap,btn_pixmaps.trig_btn);
  pb = XtCreateManagedWidget("roc_button", xmPushButtonWidgetClass,
		      parent, args, 2);
  manager.trig_btn = pb;
  return pb;
}

/* add new type to the type list */
void
XcodaEditorAddNewType (char* type)
{
  Arg arg[10];
  int ac = 0;
  XmString t;

  if (ed_new_numtypes >= EDITOR_MAX_NEW_TYPES)
  {
    fprintf (stderr, "Fatal: Maximum number of new defined types is %d\n",
	     EDITOR_MAX_NEW_TYPES);
    return;
  }

  t = XmStringCreateSimple (type);
  XtSetArg (arg[ac], XmNlabelString, t); ac++;
  XtSetValues (ed_new_types[ed_new_numtypes], arg, ac); ac++;
  ac = 0;
  XmStringFree (t);

  strncpy (ed_new_type_names[ed_new_numtypes], type, 
	   sizeof (ed_new_type_names[ed_new_numtypes]));
  ed_new_numtypes++;
}

/* return form widget which serves as a container of all command buttons*/
Widget
XcodaEditorNewButtons(Widget parent)
{
  Widget form;
  Widget pushb[20];
  Arg    args[20];
  int    ac;

  ac = 0;
  form = XtCreateWidget("cmd_form",xmFormWidgetClass,parent, NULL, 0);
  ac = 0;

  /* icons on left will shows up in the same order as in following */
  pushb[ 0] = XcodaConfigTrigButton(form);
  pushb[ 1] = XcodaConfigRocButton(form);
  pushb[ 2] = XcodaConfigEbButton(form);
  pushb[ 3] = XcodaConfigEtButton(form); /*sergey*/
  pushb[ 4] = XcodaConfigEttButton(form); /*sergey*/
  pushb[ 5] = XcodaConfigL3Button(form); /*sergey*/
  pushb[ 6] = XcodaConfigERButton(form);
  pushb[ 7] = XcodaConfigCFIButton(form);
  pushb[ 8] = XcodaConfigFIButton(form);
  pushb[ 9] = XcodaConfigDDButton(form);
  pushb[10] = XcodaConfigDBGButton(form);
  pushb[11] = XcodaConfigNoneButton(form);


  XtSetArg(args[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetValues(pushb[0], args, ac);
  ac = 0;

  XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(args[ac], XmNtopWidget, pushb[0]); ac++;
  XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetValues(pushb[1], args, ac);
  ac = 0;

  XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(args[ac], XmNtopWidget, pushb[1]); ac++;
  XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetValues(pushb[2], args, ac);
  ac = 0;

  XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(args[ac], XmNtopWidget, pushb[2]); ac++;
  XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetValues(pushb[3], args, ac);
  ac = 0;

  XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(args[ac], XmNtopWidget, pushb[3]); ac++;
  XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetValues(pushb[4], args, ac);
  ac = 0;

  XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(args[ac], XmNtopWidget, pushb[4]); ac++;
  XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetValues(pushb[5], args, ac);
  ac = 0;

  XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(args[ac], XmNtopWidget, pushb[5]); ac++;
  XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetValues(pushb[6], args, ac);
  ac = 0;

  XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(args[ac], XmNtopWidget, pushb[6]); ac++;
  XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetValues(pushb[7], args, ac);
  ac = 0;

  XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(args[ac], XmNtopWidget, pushb[7]); ac++;
  XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetValues(pushb[8], args, ac);
  ac = 0;

  XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(args[ac], XmNtopWidget, pushb[8]); ac++;
  XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetValues(pushb[9], args, ac);
  ac = 0;

  XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(args[ac], XmNtopWidget, pushb[9]); ac++;
  XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetValues(pushb[10], args, ac);
  ac = 0;

  XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg(args[ac], XmNtopWidget, pushb[10]); ac++;
  XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_FORM); ac++; /*sergey: that line in last piece only */
  XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetValues(pushb[11], args, ac);
  ac = 0;
  

  XtManageChild(form);
  
  return(form);
}





