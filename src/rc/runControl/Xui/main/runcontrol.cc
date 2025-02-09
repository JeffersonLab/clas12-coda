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
//      Main Part of RunControl
//
// Author:  
//      Jie Chen
//      CEBAF Data Acquisition Group
//
// Revision History:
//   $Log: runcontrol.cc,v $
//   Revision 1.34  1998/11/09 17:01:42  timmer
//   Linux port
//
//   Revision 1.33  1998/11/05 20:12:23  heyes
//   reverse status updating to use UDP, fix other stuff
//
//   Revision 1.32  1998/09/01 18:48:43  heyes
//   satisfy Randy's lust for command line options
//
//   Revision 1.31  1998/08/25 17:58:52  rwm
//   Drop reference to XmHTML_TopLevel. Don't start cedit & dbedit.
//
//   Revision 1.30  1998/06/18 12:20:44  heyes
//   new GUI ready I think
//
//   Revision 1.29  1998/06/02 19:51:38  heyes
//   fixed rcServer
//
//   Revision 1.28  1998/05/28 17:47:08  heyes
//   new GUI look
//
//   Revision 1.27  1998/04/08 18:31:35  heyes
//   new look and feel GUI
//
//   Revision 1.26  1998/01/23 15:27:36  heyes
//   commit LINUX changes for Carl
//
//   Revision 1.25  1997/10/20 12:45:54  heyes
//   first tag for B
//
//   Revision 1.24  1997/10/15 16:08:33  heyes
//   embed dbedit, ddmon and codaedit
//
//   Revision 1.23  1997/09/05 12:03:55  heyes
//   almost final
//
//   Revision 1.22  1997/08/25 16:00:38  heyes
//   fix some display problems
//
//   Revision 1.21  1997/08/20 18:38:31  heyes
//   fix up for SunOS
//
//   Revision 1.20  1997/08/18 13:36:38  heyes
//   add bg_pixmap1
//
//   Revision 1.19  1997/08/18 13:26:55  heyes
//   pixmap import
//
//   Revision 1.18  1997/08/01 18:38:19  heyes
//   nobody will believe this!
//
//   Revision 1.17  1997/07/30 15:32:29  heyes
//   clean for Solaris
//
//   Revision 1.16  1997/07/30 14:32:55  heyes
//   add more xpm support
//
//   Revision 1.15  1997/07/22 19:39:11  heyes
//   cleaned up lots of things
//
//   Revision 1.14  1997/07/18 16:54:49  heyes
//   new GUI
//
//   Revision 1.13  1997/07/11 14:07:53  heyes
//   almost working
//
//   Revision 1.11  1997/07/09 17:12:26  heyes
//   add rotated.h
//
//   Revision 1.10  1997/07/08 15:00:53  heyes
//   deep trouble
//
//   Revision 1.8  1997/06/16 13:22:55  heyes
//   clear graph
//
//   Revision 1.7  1997/06/16 12:26:53  heyes
//   add dbedit and so on
//
//   Revision 1.6  1997/06/14 17:29:41  heyes
//   new GUI
//
//   Revision 1.5  1997/06/06 18:51:33  heyes
//   new RC
//
//   Revision 1.3  1996/12/04 18:32:35  chen
//   port to 1.4 on hp and ultrix
//
//   Revision 1.2  1996/10/14 20:13:42  chen
//   add display server messages preference
//
//   Revision 1.1.1.1  1996/10/11 13:39:24  chen
//   run control source
//
//

#include <stdio.h>
#ifdef solaris
#include <libgen.h>
#endif
#include <string.h>
#include <assert.h>
#include <XcodaApp.h>
#include <rcClientHandler.h>
#include <rcTopW.h>
#include <rcComdOption.h>
#include <rcDbaseHandler.h>
#include <rcBackButton.h>
#include <rcHReload.h>
#include <rcHHome.h>

#ifdef Linux
#include <dlfcn.h>
#endif

#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <Xm/MainW.h>

#include "cedit.h" /* in codaedit/ it is codaedit.h !!! */

#ifdef USE_CREG
#include <codaRegistry.h>
#endif

int root_height;
XtAppContext app_context;
static Widget toplevel; //sergey: add 'static'

char *userPath = (char *)"";
/*extern "C" void HTMLhelp(Widget w,char *src);*/
extern "C" int getStartupVisual(Widget shell, Visual **visual, int *depth,
	Colormap *colormap);
#if !defined(Linux) && !defined(Darwin)
extern "C" void bzero(void *,int);
#endif

#ifdef USE_CREG
extern "C" int     codaSendInit (Widget w,char *name);
#endif

char *dollar_coda;

static char *fallback_res[] = 
{
  (char *)"runcontrol.*.fontList:                        -*-helvetica-medium-r-*-*-14-*-*-*-*-*-*-*",
  (char *)"runcontrol.*.menu_bar.*.fontList:             -*-helvetica-medium-r-*-*-14-*-*-*-*-*-*-*",
  (char *)"runcontrol*.rcMsgWindow.fontList:             -*-courier-medium-r-normal-*-12-*-*-*-*-*-*-*",
  (char *)"runcontrol.*.runInfoPanel*status.*.fontList:  -*-times-medium-i-*-*-14-*-*-*-*-*-*-*",
  (char *)"runcontrol*.runcinfo*.time*.fontList:         -*-times-medium-r-*-*-14-*-*-*-*-*-*-*",
  (char *)"runcontrol*.runcinfo*.fontList:               -*-times-medium-r-*-*-14-*-*-*-*-*-*-*",
  (char *)"runcontrol*.helpAboutDialog*.fontList:        -*-helvetica-medium-r-*-*-14-*-*-*-*-*-*-*",
  (char *)"runcontrol.*.dataLimitUnit.fontList:          -*-helvetica-medium-i-*-*-14-*-*-*-*-*-*-*",
  (char *)"runcontrol*.helpMsgWindow.fontList:           -*-helvetica-medium-r-*-*-14-*-*-*-*-*-*-*",
  (char *)"runcontrol*.connectDialogLabel.fontList:      -*-times-medium-i-*-*-14-*-*-*-*-*-*-*",
  (char *)"runcontrol*.compBootDialogLabel.fontList:     -*-times-medium-i-*-*-14-*-*-*-*-*-*-*",
  (char *)"runcontrol*.updateDiaLabel.fontList:          -*-times-medium-i-*-*-14-*-*-*-*-*-*-*",
  (char *)"runcontrol*.analogDialogLabel.fontList:       -*-times-medium-i-*-*-14-*-*-*-*-*-*-*",
  (char *)"runcontrol*.helpTextW.fontList:               -*-helvetica-medium-r-*-*-14-*-*-*-*-*-*-*",
  (char *)"runcontrol*.datafilename.fontList:            -*-helvetica-medium-r-*-*-14-*-*-*-*-*-*-*",
  (char *)"runcontrol*.OutFrame.*.fontList:              -*-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
  (char *)"runcontrol*.dialogLabel.fontList:             -*-helvetica-medium-r-*-*-14-*-*-*-*-*-*-*",


  /*(char *)"runcontrol*.iEvRateG.foreground:              RoyalBlue4", does not do enything ..*/
  (char *)"runcontrol*.iEvRateG.fontList:                -*-times-bold-r-*-*-18-*-*-*-*-*-*-*",
  (char *)"runcontrol*.iDataRateG.fontList:              -*-times-bold-r-*-*-18-*-*-*-*-*-*-*",
  (char *)"runcontrol*.dEvRateG.fontList:                -*-times-bold-r-*-*-18-*-*-*-*-*-*-*",
  (char *)"runcontrol*.dDataRateG.fontList:              -*-times-bold-r-*-*-18-*-*-*-*-*-*-*",


  (char *)"runcontrol*.OutFrame.height:              200",
  (char *)"runcontrol*OutForm*sensitive:              False",
  /*
  (char *)"runcontrol*.Xmhelp.width:                      460",
  (char *)"runcontrol*.Xmhelp.height:                     550",
  */
  (char *)"runcontrol*.*foreground:                      white",
  (char *)"runcontrol*.*background:                      gray20",
  (char *)"runcontrol*.rcMsgWindow.background:           lightGray",
  (char *)"runcontrol*.rcMsgWindow*foreground:           black",
  (char *)"runcontrol*.XmToggleButtonGadget.selectColor: yellow",
  (char *)"runcontrol*.XmToggleButton.selectColor:       yellow",
  (char *)"runcontrol*.connectDialog*.foreground:        white ",
  (char *)"runcontrol*.connectDialog*.background:        gray20",
  (char *)"runcontrol*.runTypeDialog*.foreground:        white",
  (char *)"runcontrol*.runTypeDialog*.background:        gray20",
  (char *)"runcontrol*.runConfigDialog*.foreground:      white",
  (char *)"runcontrol*.runConfigDialog*.background:      gray20",
  (char *)"runcontrol*.topShadowColor:                   gray",
  (char *)"runcontrol*.bottomShadowColor:                black",
  (char *)"runcontrol*.borderColor:                      gray25",
  (char *)"runcontrol* runstatusFrame*.borderColor:      blue",
  (char *)"runcontrol* runstatusFrame*.borderWidth:      2",
  (char *)"runcontrol*.list*shadowThickness:             2",
  (char *)"runcontrol*.list.borderWidth:                 4",
  (char *)"runcontrol.*.initInfoPanelForm.*.Hbar.*.background: lightGrey",
  (char *)"runcontrol.*.initInfoPanelForm.*.Vbar.*.background: lightGrey",
  (char *)"runcontrol.*.initInfoPanelForm.*.foreground:        white",
  (char *)"runcontrol.*.initInfoPanelForm.*.background:        black",
  
  (char *)"runcontrol.*.runInfoPanel.*.runPanelsessStatFrame.foreground: lightGrey",
  (char *)"runcontrol.*.runInfoPanel.*.runstatusLabel.fontList:  -*-times-bold-i-*-*-16-*-*-*-*-*-*-*",
  
  (char *)"runcontrol.*.runInfoPanel.*.runinfoLabel.foreground: lightGrey",
  (char *)"runcontrol.*.runInfoPanel.*.runinfoLabel.fontList:  -*-times-bold-i-*-*-16-*-*-*-*-*-*-*",
  
  (char *)"runcontrol.*.runInfoPanel.*.runsprogressLabel.foreground: lightGrey",
  (char *)"runcontrol.*.runInfoPanel.*.runsprogressLabel.fontList:  -*-times-bold-i-*-*-16-*-*-*-*-*-*-*",
  
  (char *)"runcontrol.*.runInfoPanel.*.datafn.topShadowColor:   red",
  (char *)"runcontrol.*.runInfoPanel.*.datafilename.foreground: red",
  (char *)"runcontrol.*.runInfoPanel.*.datafilename.background: lightGrey",
  (char *)"runcontrol.*.runInfoPanel.*.datafilename.fontList:  -*-times-bold-i-*-*-16-*-*-*-*-*-*-*",
  
  (char *)"runcontrol.*.runInfoPanel.*.datacn.topShadowColor:   red",
  (char *)"runcontrol.*.runInfoPanel.*.conffilename.foreground: red",
  (char *)"runcontrol.*.runInfoPanel.*.conffilename.background: lightGrey",
  (char *)"runcontrol.*.runInfoPanel.*.conffilename.fontList:  -*-times-bold-i-*-*-16-*-*-*-*-*-*-*",
 
  (char *)"runcontrol.*.runInfoPanel.*.evnbFrame2.topShadowColor: red",
  (char *)"runcontrol.*.runInfoPanel.*.evNumLabel.foreground: red",
  (char *)"runcontrol.*.runInfoPanel.*.evNumLabel.fontList:  -*-times-bold-r-*-*-16-*-*-*-*-*-*-*",
  
  (char *)"runcontrol.*.runInfoPanel*simpleInfoPanel.foreground:  blue",
  (char *)"runcontrol.*.runInfoPanel*limitframe.foreground:       red",
  (char *)"runcontrol.*.runInfoPanel*limitframe.topShadowColor:   red",
  (char *)"runcontrol.*.runInfoPanel*eventLimitFrame.foreground:  white",
  (char *)"runcontrol.*.runInfoPanel*dataLimitFrame.foreground:   white",
  
  (char *)"runcontrol.*.runInfoPanel.*.iEvDispFrame.topShadowColor: red",
  
  (char *)"runcontrol.*.runInfoPanel*runNumber.*.background:   lightGrey",
  (char *)"runcontrol.*.runInfoPanel*runNumber.*.foreground:   black",
  
  (char *)"runcontrol.*.runInfoPanel*runNumber.*.background:   lightGrey",
  (char *)"runcontrol.*.runInfoPanel*runNumber.*.foreground:   black",

  (char *)"runcontrol.*.runInfoPanel*database.*.background:    lightGrey",
  (char *)"runcontrol.*.runInfoPanel*exptname.*.background:    lightGrey",
  (char *)"runcontrol.*.runInfoPanel*runType.*.background:     lightGrey",
  (char *)"runcontrol.*.runInfoPanel*runConfig.*.background:   lightGrey",
  (char *)"runcontrol.*.runInfoPanel*hostname.*.background:    lightGrey",
  (char *)"runcontrol.*.runInfoPanel*status.*.background:      lightGrey",
  (char *)"runcontrol.*.runInfoPanel*status.*.foreground:      black",
  (char *)"runcontrol.*.runInfoPanel*startTimeG.*.background:   lightGrey",
  (char *)"runcontrol.*.runInfoPanel*startTimeG.*.foreground:   black",
  (char *)"runcontrol.*.runInfoPanel*endTimeG.*.background:     lightGrey",
  (char *)"runcontrol.*.runInfoPanel*endTimeG.*.foreground:     black",
  (char *)"runcontrol.*.runInfoPanel*eventLimit.*.background:  lightGrey",
  (char *)"runcontrol.*.runInfoPanel*eventLimit.*.foreground:  black",
  (char *)"runcontrol.*.runInfoPanel*dataLimit.*.background:   lightGrey",
  (char *)"runcontrol.*.runInfoPanel*dataLimit.*.foreground:   black",
  (char *)"runcontrol.*.runInfoPanel*status.*.foreground:      black",
  (char *)"runcontrol.*.runInfoPanel*timeG.*.background:       lightGrey",
  (char *)"runcontrol.*.runInfoPanel*timeG.*.foreground:       black",
  (char *)"runcontrol.*.runInfoPanel*exptname.*.foreground:    RoyalBlue4",
  
  (char *)"runcontrol.*.runInfoPanel*runType.*.foreground:     red",
  (char *)"runcontrol.*.runInfoPanel*runType.*.fontList:       -*-times-bold-r-*-*-16-*-*-*-*-*-*-*",
  
  (char *)"runcontrol.*.runInfoPanel*runConfig.*.foreground:   red",
  (char *)"runcontrol.*.runInfoPanel*runConfig.*.fontList:     -*-times-bold-r-*-*-16-*-*-*-*-*-*-*",
  
  (char *)"runcontrol.*.runInfoPanel*exptid.*.foreground:      RoyalBlue4",
  (char *)"runcontrol.*.runInfoPanel*hostname.*.foreground:    RoyalBlue4",
  (char *)"runcontrol.*.runInfoPanel*database.*.foreground:    RoyalBlue4",
  (char *)"runcontrol.*.runInfoPanel*session.*.foreground:     RoyalBlue4",
  (char *)"runcontrol.*.runInfoPanel*cinfoSubForm.*.alignment: alignment_center",
  (char *)"runcontrol.*.runInfoPanel*eventNumber.*.background: lightGrey",
  (char *)"runcontrol.*.runInfoPanel*eventNumber.*.foreground: black",
  
  (char *)"runcontrol.*.evrateDisplay.background:     lightGrey",
  
  (char *)"runcontrol.*.datarateDisplay.background:     lightGrey",
  
  (char *)"runcontrol.*.ratioDisplay.background:     lightGrey",
  
  (char *)"runcontrol.*.otherDisplay.background:     lightGrey",
  (char *)"runcontrol.*.runInfoPanel*iEvRate.*.background:     lightGrey",
  (char *)"runcontrol.*.runInfoPanel*iEvRate.*.background:     lightGrey",
  (char *)"runcontrol.*.runInfoPanel*iEvRate.*.foreground:     black",
  (char *)"runcontrol.*.runInfoPanel*dEvRate.*.background:     lightGrey",
  (char *)"runcontrol.*.runInfoPanel*dEvRate.*.foreground:     black",
  (char *)"runcontrol.*.runInfoPanel*iDataRate.*.background:   lightGrey",
  (char *)"runcontrol.*.runInfoPanel*iDataRate.*.foreground:   black",
  (char *)"runcontrol.*.runInfoPanel*dDataRate.*.background:   lightGrey",
  (char *)"runcontrol.*.runInfoPanel*dDataRate.*.foreground:   black",
  (char *)"runcontrol.*.runInfoPanel.*background:              gray20",
  (char *)"runcontrol.*.runInfoPanel.*foreground:              white",
  (char *)"runcontrol.*.runInfoPanel*optionPulldown*foreground:white",
  (char *)"runcontrol.*.runInfoPanel*runtype*foreground:       white",
  (char *)"runcontrol.*.runInfoPanel*runconfig*foreground:     white",
  (char *)"runcontrol.*.runInfoPanel*eventNumberG.*.background:lightGrey",
  (char *)"runcontrol.*.runInfoPanel*eventNumberG.foreground:  RoyalBlue4",
  (char *)"runcontrol.*.runInfoPanel*eventNumberG.fontList:       -*-times-bold-r-*-*-18-*-*-*-*-*-*-*",
  (char *)"runcontrol.*.runInfoPanel*eventNumberG.*borderWidth:1",
  (char *)"runcontrol.*.runInfoPanel*netstatus.*background:    daykGray",
  (char *)"runcontrol.*.scriptTab.tabcolor:                    gray20",
  (char *)"runcontrol.*.menu_bar.background:                   gray20",
  (char *)"runcontrol.*.menu_bar.*.foreground:                 white",
  (char *)"runcontrol.*.XmPushButton*highlightThickness:       0",
  (char *)"runcontrol.*.XmPushButtonGadget*highlightThickness: 0",
  (char *)"runcontrol.*.XmTextField*highlightThickness:        0",
  (char *)"runcontrol.*.XmLabel*highlightThickness:            0",
  (char *)"runcontrol.*.XmLabelGadget*highlightThickness:      0",
  (char *)"runcontrol.*.XmToggleButtonGadget*highlightThickness: 0  ",
  (char *)"runcontrol.*.XmToggleButton*highlightThickness:     0  ",
  (char *)"runcontrol.*.XmRowColumn*spacing:                   0",
  (char *)"runcontrol*.scale_red*troughColor:                  RoyalBlue4",
  (char *)"runcontrol*.scale_green*troughColor:                Green",
  (char *)"runcontrol*.scale_blue*troughColor:                 Blue",
  (char *)"runcontrol*.highlightThickness:                     0",
  (char *)"runcontrol*.XmRowColumn*spacing:                    0",
  (char *)"runcontrol*.selectColor:                            RoyalBlue4",
  (char *)"runcontrol*.scriptTab.shadowThickness:              2",
  (char *)"runcontrol*.scriptTab.tabWidthPercentage:           10",
  (char *)"runcontrol*.scriptTab.cornerwidth:                  2",
  (char *)"runcontrol*.scriptTab.cornerheight:                 2",
  (char *)"runcontrol*.scriptTab.textmargin:                   4",
  (char *)"runcontrol*.scriptTab.foreground:                   blue",
  (char *)"runcontrol*.scriptTab.tabcolor:                     lightGrey",
  
  (char *)"runcontrol*.top_ruler.background:                   lightGrey",
  (char *)"runcontrol*.left_ruler.background:                  lightGrey",
  (char *)"runcontrol*.top_ruler.foreground:                   White",
  (char *)"runcontrol*.left_ruler.foreground:                  White",
  (char *)"runcontrol*.top_ruler.tickerColor:                  White",
  (char *)"runcontrol*.left_ruler.tickerColor:                 White  ",
  (char *)"runcontrol*.edit_popup.*.background:                White",
  (char *)"runcontrol*.top_ruler.indicatorColor:               RoyalBlue4",
  (char *)"runcontrol*.left_ruler.indicatorColor:              RoyalBlue4",
  
  (char *)"runcontrol*icon_sw*background:                      lightGray",
  (char *)"runcontrol*icon_sw*foreground:                      black",
  
  (char *)"runcontrol*htext*background:                        lightGray",
  (char *)"runcontrol*htext*foreground:                        black",
  (char *)"runcontrol*anaLogFile.background:                   lightGrey",
  (char *)"runcontrol*anaLogFile.foreground:                   black",
  (char *)"runcontrol*anaLogFile.fontList:  -*-times-bold-i-*-*-16-*-*-*-*-*-*-*",

  (char *)"runcontrol.*.fileFr.topShadowColor:   red",
  (char *)"runcontrol.*.bootFr.topShadowColor:   red",
  (char *)"runcontrol.*.updFr.topShadowColor:   red",
  (char *)"runcontrol.*.monFr.topShadowColor:   red",

  (char *)"runcontrol*runInfoPanel*statuspanel.*.background:    lightGrey",

  (char *)"runcontrol*runNumber*backgroundPixmap:               XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*startTime*backgroundPixmap:               XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*endTime*backgroundPixmap:                 XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*eventLimit*backgroundPixmap:              XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*dataLimit*backgroundPixmap:               XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*status*backgroundPixmap:                  XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*time*backgroundPixmap:                    XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*exptname*backgroundPixmap:                XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*runType*backgroundPixmap:                 XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*runConfig*backgroundPixmap:               XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*exptid*backgroundPixmap:                  XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*hostname*backgroundPixmap:                XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*database*backgroundPixmap:                XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*session*backgroundPixmap:                 XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*eventNumber*backgroundPixmap:             XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*iDataRate*backgroundPixmap:               XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*dDataRate*backgroundPixmap:               XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*iEvRate*backgroundPixmap:                 XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*dEvRate*backgroundPixmap:                 XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*top_ruler*backgroundPixmap:               XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*left_ruler*backgroundPixmap:              XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*icon_sw*backgroundPixmap:                 XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*htext*backgroundPixmap:                   XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*initInfoPanelForm*backgroundPixmap:       XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*eventNumberG*backgroundPixmap:            XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*rcMsgWindow*backgroundPixmap:             XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*rcRateDisplay*backgroundPixmap:           XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*rcRateDisplay*foreground:                 black",
  (char *)"runcontrol*icon_sw*backgroundPixmap:                 XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*icon_box*backgroundPixmap:                XmUNSPECIFIED_PIXMAP",
  (char *)"runcontrol*icon*backgroundPixmap:                    XmUNSPECIFIED_PIXMAP",
  
  (char *)"runcontrol.*.RbuttonFrame.topShadowColor:   red",
  (char *)"runcontrol.*.LbuttonFrame.topShadowColor:   blue",

  /*
  "runcontrol*HelpWidget*helpHtml*background:               lightGray",
  "runcontrol*HelpWidget*helpHtml*foreground:               black",
  "runcontrol*HelpWidget*fontList:		     -*-helvetica-medium-r-*-*-12-*-*-*-*-*-iso8859-1",
  
  "runcontrol*balloonHelp*background:     yellow",
  "runcontrol*balloonHelp*foreground:     black",
  
  "runcontrol*balloonHelp*fontList:    -*-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
  
  "runcontrol*balloonHelp2*background:     yellow",
  "runcontrol*balloonHelp2*foreground:     black",
  
  "runcontrol*balloonHelp2*fontList:    -*-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
  
  "runcontrol*HelpWidget*maxImageColors: 0",
  "runcontrol*HelpWidget*file.labelString: File",
  "runcontrol*HelpWidget*fileMenu*open.labelString: Open File...",
  "runcontrol*HelpWidget*fileMenu*open.mnemonic: O",
  "runcontrol*HelpWidget*fileMenu*open.accelerator: Ctrl<Key>O",
  "runcontrol*HelpWidget*fileMenu*open.acceleratorText: Ctrl+O",
  "runcontrol*HelpWidget*fileMenu*saveas.labelString: Save File As...",
  "runcontrol*HelpWidget*fileMenu*saveas.mnemonic: S",
  "runcontrol*HelpWidget*fileMenu*reload.labelString: Reload File",
  "runcontrol*HelpWidget*fileMenu*reload.mnemonic: R",
  "runcontrol*HelpWidget*fileMenu*reload.accelerator: Ctrl<Key>R",
  "runcontrol*HelpWidget*fileMenu*reload.acceleratorText: Ctrl+R",
  "runcontrol*HelpWidget*fileMenu*quit.labelString: Exit",
  "runcontrol*HelpWidget*fileMenu*quit.mnemonic: x",
  "runcontrol*HelpWidget*fileMenu*quit.accelerator: Ctrl<Key>X",
  "runcontrol*HelpWidget*fileMenu*quit.acceleratorText: Ctrl+X",
  "runcontrol*HelpWidget*fileMenu*view.labelString: View",
  "runcontrol*HelpWidget*fileMenu*view.mnemonic: V",
  "runcontrol*HelpWidget*viewMenu*viewInfo.labelString: Document Info",
  "runcontrol*HelpWidget*viewMenu*viewInfo.mnemonic: I",
  "runcontrol*HelpWidget*viewMenu*viewSource.labelString: Document Source",
  "runcontrol*HelpWidget*viewMenu*viewSource.mnemonic: S",
  "runcontrol*HelpWidget*viewMenu*viewFonts.labelString: Font Cache Info",
  "runcontrol*HelpWidget*viewMenu*viewFonts.mnemonic: F",
  "runcontrol*HelpWidget*edit.labelString: Edit",
  "runcontrol*HelpWidget*editMenu*find.labelString: Find...",
  "runcontrol*HelpWidget*editMenu*find.mnemonic: F",
  "runcontrol*HelpWidget*editMenu*findAgain.labelString: Find Again",
  "runcontrol*HelpWidget*editMenu*findAgain.mnemonic: A",
  "runcontrol*HelpWidget*option.labelString: Options",
  "runcontrol*HelpWidget*optionMenu*anchorButtons.labelString: Buttoned Anchors",
  "runcontrol*HelpWidget*optionMenu*anchorButtons.mnemonic: B",
  "runcontrol*HelpWidget*optionMenu*highlightOnEnter.labelString: Highlight Anchors",
  "runcontrol*HelpWidget*optionMenu*highlightOnEnter.mnemonic: H",
  "runcontrol*HelpWidget*optionMenu*imageAnchorTracking.labelString: Track Image Anchors",
  "runcontrol*HelpWidget*optionMenu*imageAnchorTracking.mnemonic: I",
  "runcontrol*HelpWidget*optionMenu*anchorTips.labelString: Anchor tooltips",
  "runcontrol*HelpWidget*optionMenu*anchorTips.mnemonic: t",
  "runcontrol*HelpWidget*optionMenu*enableBodyColors.labelString: Body Colors",
  "runcontrol*HelpWidget*optionMenu*enableBodyColors.mnemonic: C",
  "runcontrol*HelpWidget*optionMenu*enableBodyImages.labelString: Body Image",
  "runcontrol*HelpWidget*optionMenu*enableBodyImages.mnemonic: o",
  "runcontrol*HelpWidget*optionMenu*enableDocumentColors.labelString: Allow Document Colors",
  "runcontrol*HelpWidget*optionMenu*enableDocumentColors.mnemonic: l",
  "runcontrol*HelpWidget*optionMenu*enableDocumentFonts.labelString: Allow Document Fonts",
  "runcontrol*HelpWidget*optionMenu*enableDocumentFonts.mnemonic: F",
  "runcontrol*HelpWidget*optionMenu*enableOutlining.labelString: Text Justification",
  "runcontrol*HelpWidget*optionMenu*enableOutlining.mnemonic: J",
  "runcontrol*HelpWidget*optionMenu*strictHTMLChecking.labelString: Strict HTML Checking ",
  "runcontrol*HelpWidget*optionMenu*strictHTMLChecking.mnemonic: S",
  "runcontrol*HelpWidget*optionMenu*warning.labelString: HTML Warnings",
  "runcontrol*HelpWidget*optionMenu*warning.mnemonic: W",
  "runcontrol*HelpWidget*optionMenu*freezeAnimations.labelString: Freeze Animations",
  "runcontrol*HelpWidget*optionMenu*freezeAnimations.mnemonic: r",
  "runcontrol*HelpWidget*optionMenu*imageEnable.labelString: Enable Image Support",
  "runcontrol*HelpWidget*optionMenu*imageEnable.mnemonic: E",
  "runcontrol*HelpWidget*optionMenu*autoImageLoad.labelString: Autoload Images",
  "runcontrol*HelpWidget*optionMenu*autoImageLoad.mnemonic: u",
  "runcontrol*HelpWidget*optionMenu*save.labelString: Save Options",
  "runcontrol*HelpWidget*optionMenu*save.mnemonic: v",
  "runcontrol*HelpWidget*warningMenu*none.labelString: Disable",
  "runcontrol*HelpWidget*warningMenu*none.mnemonic: D",
  "runcontrol*HelpWidget*warningMenu*all.labelString: Show All Warnings",
  "runcontrol*HelpWidget*warningMenu*all.mnemonic: A",
  "runcontrol*HelpWidget*warningMenu*unknownElement.labelString: Unknown HTML element",
  "runcontrol*HelpWidget*warningMenu*unknownElement.mnemonic: U",
  "runcontrol*HelpWidget*warningMenu*bad.labelString: Badly placed tags",
  "runcontrol*HelpWidget*warningMenu*bad.mnemonic: B",
  "runcontrol*HelpWidget*warningMenu*openBlock.labelString: Bad block aparture",
  "runcontrol*HelpWidget*warningMenu*openBlock.mnemonic: p",
  "runcontrol*HelpWidget*warningMenu*closeBlock.labelString: Bad block closure",
  "runcontrol*HelpWidget*warningMenu*closeBlock.mnemonic: c",
  "runcontrol*HelpWidget*warningMenu*openElement.labelString: Unbalanced Terminators",
  "runcontrol*HelpWidget*warningMenu*openElement.mnemonic: T",
  "runcontrol*HelpWidget*warningMenu*nested.labelString: Improper Nested Tags",
  "runcontrol*HelpWidget*warningMenu*nested.mnemonic: I",
  "runcontrol*HelpWidget*warningMenu*violation.labelString: HTML 3.2 Violations",
  "runcontrol*HelpWidget*warningMenu*violation.mnemonic: V",
  "runcontrol*HelpWidget*window.labelString: Window",
  "runcontrol*HelpWidget*windowMenu*lower.labelString: Lower Window",
  "runcontrol*HelpWidget*windowMenu*lower.mnemonic: L",
  "runcontrol*HelpWidget*windowMenu*raise.labelString: Raise Window",
  "runcontrol*HelpWidget*windowMenu*raise.mnemonic: R",
  "runcontrol*HelpWidget*help.labelString: Help",
  "runcontrol*HelpWidget*helpMenu*about.labelString: About XmHTML",
  "runcontrol*HelpWidget*helpMenu*about.mnemonic: A",
  */
  NULL,
};

extern XcodaApp *theApplication;

int doTk = 0;
Display *MainDisplay;
rcMenuWindow *menW;


void
messageHandler(char *message)
{
  printf("runcontrol::messageHandler reached, message >%s<\n",message);
  
  switch (message[0])
  {
    case 't':
    {
      char name[200];
      int pid;
      sscanf(&message[2],"%d %s",&pid,name);
      /*menW->createTabFrame(name,pid);*/
    }
    break;

    default:
      printf("runcontrol::messageHandler: unknown message : %s\n",message);
    
  }
}

void
warningHandler(char *msg)
{
  ;
}


int
main (int argc, char** argv)
{
  static int once = 0;
  
  dollar_coda = getenv("CODA");
  if (dollar_coda == NULL)
  {
    printf ("$CODA must be set\n");
    exit(0);
  }
  doTk = 1;

  struct rlimit limits;
  getrlimit(RLIMIT_NOFILE,&limits);
  
  limits.rlim_cur = limits.rlim_max;

  setrlimit(RLIMIT_NOFILE,&limits);

  /*__rsd_selectDebugLevels("1");*/
  
  /* set up command option */
  
  rcComdOption::initOptionParser (argc, argv);
  rcComdOption* option = rcComdOption::option ();
  
  option->reportMsgOn ();
  option->parseOptions ();
  
  /* setup database connection handler */
  (void)rcDbaseHandler::dbaseHandler ();

  
  XcodaApp* app = new XcodaApp (argv[0], fallback_res);
  // open a X display
  app->open (&argc, argv);
  
  
  /* create a network handler which register itself to x window event loop */
  app_context = app->appContext ();
  
  rcClientHandler netHandler(app->appContext ());
  
  XtAppSetWarningHandler (app->appContext (), warningHandler);
  
  rcTopW* window  = new rcTopW ((char *)"RunControl", netHandler);
  
  app->initialize (&argc, argv);

  { 
    int x,y;
    unsigned int w,h,bw,dp,ac;
    Arg arg[20];
    Window root;
    /*extern Widget helpBalloon;*/
    char tmp[200];
    
    menW = window->window_;
    
    if (option->autostart_)
    {
      menW->bootall_ = 1;
    }


    /*sergey : !!!!!!!!!!!??????????????
    toplevel = XtParent(XtParent(XtParent(menW->rform_)));
    */
    toplevel = XtParent(menW->rform_);


    printf("\nruncontrol: TOPLEVELS: 0x%08x 0x%08x 0x%08x 0x%08x\n\n",
    XtParent(menW->rform_),
    XtParent(XtParent(menW->rform_)),
    XtParent(XtParent(XtParent(menW->rform_))),
    XtParent(XtParent(XtParent(XtParent(menW->rform_)))) );


    ac = 0;
    /*sergey
    XtSetArg(arg[0], XmNpopdownDelay, 6000);
    XtSetArg(arg[1], XmNpopupDelay,   3000);
    helpBalloon = XmCreateBalloon(toplevel, "balloonHelp2", arg, 2);
    */

/*
#ifdef USE_CREG    
    codaSendInit(toplevel,"RUNCONTROL");
#endif
    MainDisplay = XtDisplay(toplevel);
#endif
*/

#ifdef USE_CREG

    MainDisplay = XtDisplay(toplevel); /* sergey: for rcCLient only !? */

    codaSendInit(toplevel,(char *)"RUNCONTROL");  
    //codaRegisterMsgCallback((void *)messageHandler);
#endif    


	/* sergey: contents of 'help' on right side 
    XGetGeometry(
       XtDisplay(menW->tabChildren_[0]),
       XtWindow(menW->tabChildren_[0]),
       &root,&x,&y,&w,&h,&bw,&dp
    );
    
    {
      char  use_file[1000];
      char  *root = getenv("RC_HELP_ROOT");
      if (root)
      {
	    sprintf(use_file,"%s",root);
      }
      else
      {
	    char *dollar_coda;
	
	    dollar_coda = getenv ("CODA");
	
	    if (dollar_coda == NULL) return 0;
	
	    sprintf(use_file, "%s/common/html/rc/Notice.html", dollar_coda);
      }
      HTMLhelp(menW->tabChildren_[0],use_file);
    }
	*/




#ifdef USE_CREG
    CODASetAppName (XtDisplay(menW->tabChildren_[0]),XtWindow(menW->tabChildren_[0]),(char *)"RC_HTML_WINDOW");
#endif
    XStoreName(XtDisplay(menW->tabChildren_[0]),XtWindow(menW->tabChildren_[0]),(char *)"RC_HTML_WINDOW");

    menW->createTabFrame((char *)"codaedit",0); /* create frame for codaedit; first par must coinside with
     the name in following call from codaedit.c: parent = CODAGetAppWindow(XtDisplay(toplevel),"codaedit_WINDOW"); */

    menW->createTabFrame((char *)"dbedit",0); /* create frame for dbedit */

    menW->createTabFrame((char *)"rocs",0); /* create frame for codaterms */

    ac = 0;
    XtSetArg (arg[ac], XmNresizePolicy, XmRESIZE_ANY); ac++;
    //XtSetArg (arg[ac], XmNresizePolicy, XmNONE); ac++;

#ifdef USE_CREG_hide
    //codaSendInit(toplevel,"RUNCONTROL");  
    codaRegisterMsgCallback((void *)messageHandler);
#endif    

    if (!option->startWide_)
    {
      XResizeWindow(
      XtDisplay(XtParent(menW->rform_)),
      XtWindow(toplevel),
      1727, /*1000_without_rocs*/ /*490*/ /*sergey: initial gui width*/
      1080 /*HeightOfScreen(XtScreen(menW->rform_))*/ /*sergey: initial gui height*/
      );
    }


    {
      char temp2[256],temp3[256];



      
#if 1  
      if (1/*always start codaedit*//*option->startCedit_*/)
      {
        if (option->noEdit_)
        {
          sprintf(temp2, "(echo \"start codaedit\"; sleep 1; %s/codaedit -embed -noedit )&",getenv("CODA_BIN"));
        }
        else
        {
	  sprintf(temp2,"(echo \"start codaedit\"; sleep 1; %s/codaedit -embed )&",getenv("CODA_BIN"));
	  //sprintf(temp2,"(echo \"start codaedit\"; sleep 1;  %s/src/codaedit/%s/bin/codaedit -embed )&",getenv("CODA"),getenv("OSTYPE_MACHINE"));
       }

        printf("Executing >%s<\n",temp2);
        system(temp2);
      }
#endif

      
#if 1
      if (option->startDbedit_)
      {
        sprintf (temp2,"(echo \"start dbedit\";sleep 1; %s/common/scripts/dbedit -embed )&",getenv("CODA"));
        printf("Executing >%s<\n",temp2);
        system(temp2);
      }

#endif


      
#if 1
      if(option->startRocs_)
      {
        if(option->logRocs_)
          sprintf (temp2,"(echo \"start rocs\";sleep 1; %s/rocs -embed -log &>> /data/log/rocs.log)&",getenv("CODA_BIN"));
        else
          sprintf (temp2,"(echo \"start rocs\";sleep 1; %s/rocs -embed )&",getenv("CODA_BIN"));

        system(temp2);
      }
#endif

      
    }

  }

  
  while (1)
  {
    printf("executing main 1\n");fflush(stdout);
    if (theApplication != NULL) app->execute();
    printf("executing main 2\n");fflush(stdout);
    if (theApplication == NULL) return(0);
    printf("executing main 3\n");fflush(stdout);
  }
  

  return(0);
}
