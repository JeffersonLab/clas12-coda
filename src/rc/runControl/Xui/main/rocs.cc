
/* rocs.cc */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef solaris
#include <libgen.h>
#endif

#ifdef Linux
#include <dlfcn.h>
#endif

#include <XcodaApp.h>
#include <rcClientHandler.h>
#include <rcRocW.h>
#include <rcComdOption.h>
#include <rcDbaseHandler.h>
#include <rcBackButton.h>
#include <rcHReload.h>
#include <rcHHome.h>


#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <Xm/MainW.h>
#include <Xm/Paned.h>
#include <Xm/XmP.h>
#include <Xm/SashP.h>

#ifdef USE_CREG
#include <codaRegistry.h>
#endif


#undef DEBUG

/* the number of roc spaces */
#define NSPACE 12

int root_height;
XtAppContext app_context;
Widget toplevel;

char *userPath = (char *)"";
extern "C" int getStartupVisual(Widget shell, Visual **visual, int *depth,
	Colormap *colormap);
#if !defined(Linux) && !defined(Darwin)
extern "C" void bzero(void *,int);
#endif

#ifdef USE_CREG
extern "C" int codaSendInit (Widget w, char *name);
#endif

char *dollar_coda;

static char *fallback_res[] = 
{
  (char *)"rocs.*.fontList:                        -*-helvetica-medium-r-*-*-14-*-*-*-*-*-*-*",
  (char *)"rocs.*.menu_bar.*.fontList:             -*-helvetica-medium-r-*-*-14-*-*-*-*-*-*-*",
  (char *)"rocs*.rcMsgWindow.fontList:             -*-courier-medium-r-normal-*-12-*-*-*-*-*-*-*",
  (char *)"rocs.*.runInfoPanel*status.*.fontList:  -*-times-medium-i-*-*-14-*-*-*-*-*-*-*",
  (char *)"rocs*.runcinfo*.time*.fontList:         -*-times-medium-r-*-*-14-*-*-*-*-*-*-*",
  (char *)"rocs*.runcinfo*.fontList:               -*-times-medium-r-*-*-14-*-*-*-*-*-*-*",
  (char *)"rocs*.helpAboutDialog*.fontList:        -*-helvetica-medium-r-*-*-14-*-*-*-*-*-*-*",
  (char *)"rocs.*.dataLimitUnit.fontList:          -*-helvetica-medium-i-*-*-14-*-*-*-*-*-*-*",
  (char *)"rocs*.helpMsgWindow.fontList:           -*-helvetica-medium-r-*-*-14-*-*-*-*-*-*-*",
  (char *)"rocs*.connectDialogLabel.fontList:      -*-times-medium-i-*-*-14-*-*-*-*-*-*-*",
  (char *)"rocs*.compBootDialogLabel.fontList:     -*-times-medium-i-*-*-14-*-*-*-*-*-*-*",
  (char *)"rocs*.updateDiaLabel.fontList:          -*-times-medium-i-*-*-14-*-*-*-*-*-*-*",
  (char *)"rocs*.analogDialogLabel.fontList:       -*-times-medium-i-*-*-14-*-*-*-*-*-*-*",
  (char *)"rocs*.helpTextW.fontList:               -*-helvetica-medium-r-*-*-14-*-*-*-*-*-*-*",
  (char *)"rocs*.datafilename.fontList:            -*-helvetica-medium-r-*-*-14-*-*-*-*-*-*-*",
  (char *)"rocs*.OutFrame.*.fontList:              -*-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
  (char *)"rocs*.dialogLabel.fontList:             -*-helvetica-medium-r-*-*-14-*-*-*-*-*-*-*",


  /*(char *)"rocs*.iEvRateG.foreground:              RoyalBlue4", does not do enything ..*/
  (char *)"rocs*.iEvRateG.fontList:                -*-times-bold-r-*-*-18-*-*-*-*-*-*-*",
  (char *)"rocs*.iDataRateG.fontList:              -*-times-bold-r-*-*-18-*-*-*-*-*-*-*",
  (char *)"rocs*.dEvRateG.fontList:                -*-times-bold-r-*-*-18-*-*-*-*-*-*-*",
  (char *)"rocs*.dDataRateG.fontList:              -*-times-bold-r-*-*-18-*-*-*-*-*-*-*",


  (char *)"rocs*.OutFrame.height:              200",
  (char *)"rocs*OutForm*sensitive:              False",
  /*
  (char *)"rocs*.Xmhelp.width:                      460",
  (char *)"rocs*.Xmhelp.height:                     550",
  */
  (char *)"rocs*.*foreground:                      white",
  (char *)"rocs*.*background:                      gray20",
  (char *)"rocs*.rcMsgWindow.background:           lightGray",
  (char *)"rocs*.rcMsgWindow*foreground:           black",
  (char *)"rocs*.XmToggleButtonGadget.selectColor: yellow",
  (char *)"rocs*.XmToggleButton.selectColor:       yellow",
  (char *)"rocs*.connectDialog*.foreground:        white ",
  (char *)"rocs*.connectDialog*.background:        gray20",
  (char *)"rocs*.runTypeDialog*.foreground:        white",
  (char *)"rocs*.runTypeDialog*.background:        gray20",
  (char *)"rocs*.runConfigDialog*.foreground:      white",
  (char *)"rocs*.runConfigDialog*.background:      gray20",
  (char *)"rocs*.topShadowColor:                   gray",
  (char *)"rocs*.bottomShadowColor:                black",
  (char *)"rocs*.borderColor:                      gray25",
  (char *)"rocs* runstatusFrame*.borderColor:      blue",
  (char *)"rocs* runstatusFrame*.borderWidth:      2",
  (char *)"rocs*.list*shadowThickness:             2",
  (char *)"rocs*.list.borderWidth:                 4",
  (char *)"rocs.*.initInfoPanelForm.*.Hbar.*.background: lightGrey",
  (char *)"rocs.*.initInfoPanelForm.*.Vbar.*.background: lightGrey",
  (char *)"rocs.*.initInfoPanelForm.*.foreground:        white",
  (char *)"rocs.*.initInfoPanelForm.*.background:        black",
  
  (char *)"rocs.*.runInfoPanel.*.runPanelsessStatFrame.foreground: lightGrey",
  (char *)"rocs.*.runInfoPanel.*.runstatusLabel.fontList:  -*-times-bold-i-*-*-16-*-*-*-*-*-*-*",
  
  (char *)"rocs.*.runInfoPanel.*.runinfoLabel.foreground: lightGrey",
  (char *)"rocs.*.runInfoPanel.*.runinfoLabel.fontList:  -*-times-bold-i-*-*-16-*-*-*-*-*-*-*",
  
  (char *)"rocs.*.runInfoPanel.*.runsprogressLabel.foreground: lightGrey",
  (char *)"rocs.*.runInfoPanel.*.runsprogressLabel.fontList:  -*-times-bold-i-*-*-16-*-*-*-*-*-*-*",
  
  (char *)"rocs.*.runInfoPanel.*.datafn.topShadowColor:   red",
  (char *)"rocs.*.runInfoPanel.*.datafilename.foreground: red",
  (char *)"rocs.*.runInfoPanel.*.datafilename.background: lightGrey",
  (char *)"rocs.*.runInfoPanel.*.datafilename.fontList:  -*-times-bold-i-*-*-16-*-*-*-*-*-*-*",
  
  (char *)"rocs.*.runInfoPanel.*.datacn.topShadowColor:   red",
  (char *)"rocs.*.runInfoPanel.*.conffilename.foreground: red",
  (char *)"rocs.*.runInfoPanel.*.conffilename.background: lightGrey",
  (char *)"rocs.*.runInfoPanel.*.conffilename.fontList:  -*-times-bold-i-*-*-16-*-*-*-*-*-*-*",
 
  (char *)"rocs.*.runInfoPanel.*.evnbFrame2.topShadowColor: red",
  (char *)"rocs.*.runInfoPanel.*.evNumLabel.foreground: red",
  (char *)"rocs.*.runInfoPanel.*.evNumLabel.fontList:  -*-times-bold-r-*-*-16-*-*-*-*-*-*-*",
  
  (char *)"rocs.*.runInfoPanel*simpleInfoPanel.foreground:  blue",
  (char *)"rocs.*.runInfoPanel*limitframe.foreground:       red",
  (char *)"rocs.*.runInfoPanel*limitframe.topShadowColor:   red",
  (char *)"rocs.*.runInfoPanel*eventLimitFrame.foreground:  white",
  (char *)"rocs.*.runInfoPanel*dataLimitFrame.foreground:   white",
  
  (char *)"rocs.*.runInfoPanel.*.iEvDispFrame.topShadowColor: red",
  
  (char *)"rocs.*.runInfoPanel*runNumber.*.background:   lightGrey",
  (char *)"rocs.*.runInfoPanel*runNumber.*.foreground:   black",
  
  (char *)"rocs.*.runInfoPanel*runNumber.*.background:   lightGrey",
  (char *)"rocs.*.runInfoPanel*runNumber.*.foreground:   black",

  (char *)"rocs.*.runInfoPanel*database.*.background:    lightGrey",
  (char *)"rocs.*.runInfoPanel*exptname.*.background:    lightGrey",
  (char *)"rocs.*.runInfoPanel*runType.*.background:     lightGrey",
  (char *)"rocs.*.runInfoPanel*runConfig.*.background:   lightGrey",
  (char *)"rocs.*.runInfoPanel*hostname.*.background:    lightGrey",
  (char *)"rocs.*.runInfoPanel*status.*.background:      lightGrey",
  (char *)"rocs.*.runInfoPanel*status.*.foreground:      black",
  (char *)"rocs.*.runInfoPanel*startTimeG.*.background:   lightGrey",
  (char *)"rocs.*.runInfoPanel*startTimeG.*.foreground:   black",
  (char *)"rocs.*.runInfoPanel*endTimeG.*.background:     lightGrey",
  (char *)"rocs.*.runInfoPanel*endTimeG.*.foreground:     black",
  (char *)"rocs.*.runInfoPanel*eventLimit.*.background:  lightGrey",
  (char *)"rocs.*.runInfoPanel*eventLimit.*.foreground:  black",
  (char *)"rocs.*.runInfoPanel*dataLimit.*.background:   lightGrey",
  (char *)"rocs.*.runInfoPanel*dataLimit.*.foreground:   black",
  (char *)"rocs.*.runInfoPanel*status.*.foreground:      black",
  (char *)"rocs.*.runInfoPanel*timeG.*.background:       lightGrey",
  (char *)"rocs.*.runInfoPanel*timeG.*.foreground:       black",
  (char *)"rocs.*.runInfoPanel*exptname.*.foreground:    RoyalBlue4",
  
  (char *)"rocs.*.runInfoPanel*runType.*.foreground:     red",
  (char *)"rocs.*.runInfoPanel*runType.*.fontList:       -*-times-bold-r-*-*-16-*-*-*-*-*-*-*",
  
  (char *)"rocs.*.runInfoPanel*runConfig.*.foreground:   red",
  (char *)"rocs.*.runInfoPanel*runConfig.*.fontList:     -*-times-bold-r-*-*-16-*-*-*-*-*-*-*",
  
  (char *)"rocs.*.runInfoPanel*exptid.*.foreground:      RoyalBlue4",
  (char *)"rocs.*.runInfoPanel*hostname.*.foreground:    RoyalBlue4",
  (char *)"rocs.*.runInfoPanel*database.*.foreground:    RoyalBlue4",
  (char *)"rocs.*.runInfoPanel*session.*.foreground:     RoyalBlue4",
  (char *)"rocs.*.runInfoPanel*cinfoSubForm.*.alignment: alignment_center",
  (char *)"rocs.*.runInfoPanel*eventNumber.*.background: lightGrey",
  (char *)"rocs.*.runInfoPanel*eventNumber.*.foreground: black",
  
  (char *)"rocs.*.evrateDisplay.background:     lightGrey",
  
  (char *)"rocs.*.datarateDisplay.background:     lightGrey",
  
  (char *)"rocs.*.ratioDisplay.background:     lightGrey",
  
  (char *)"rocs.*.otherDisplay.background:     lightGrey",
  (char *)"rocs.*.runInfoPanel*iEvRate.*.background:     lightGrey",
  (char *)"rocs.*.runInfoPanel*iEvRate.*.background:     lightGrey",
  (char *)"rocs.*.runInfoPanel*iEvRate.*.foreground:     black",
  (char *)"rocs.*.runInfoPanel*dEvRate.*.background:     lightGrey",
  (char *)"rocs.*.runInfoPanel*dEvRate.*.foreground:     black",
  (char *)"rocs.*.runInfoPanel*iDataRate.*.background:   lightGrey",
  (char *)"rocs.*.runInfoPanel*iDataRate.*.foreground:   black",
  (char *)"rocs.*.runInfoPanel*dDataRate.*.background:   lightGrey",
  (char *)"rocs.*.runInfoPanel*dDataRate.*.foreground:   black",
  (char *)"rocs.*.runInfoPanel.*background:              gray20",
  (char *)"rocs.*.runInfoPanel.*foreground:              white",
  (char *)"rocs.*.runInfoPanel*optionPulldown*foreground:white",
  (char *)"rocs.*.runInfoPanel*runtype*foreground:       white",
  (char *)"rocs.*.runInfoPanel*runconfig*foreground:     white",
  (char *)"rocs.*.runInfoPanel*eventNumberG.*.background:lightGrey",
  (char *)"rocs.*.runInfoPanel*eventNumberG.foreground:  RoyalBlue4",
  (char *)"rocs.*.runInfoPanel*eventNumberG.fontList:       -*-times-bold-r-*-*-18-*-*-*-*-*-*-*",
  (char *)"rocs.*.runInfoPanel*eventNumberG.*borderWidth:1",
  (char *)"rocs.*.runInfoPanel*netstatus.*background:    daykGray",
  (char *)"rocs.*.rocTab.tabcolor:                    gray20",
  (char *)"rocs.*.menu_bar.background:                   gray20",
  (char *)"rocs.*.menu_bar.*.foreground:                 white",
  (char *)"rocs.*.XmPushButton*highlightThickness:       0",
  (char *)"rocs.*.XmPushButtonGadget*highlightThickness: 0",
  (char *)"rocs.*.XmTextField*highlightThickness:        0",
  (char *)"rocs.*.XmLabel*highlightThickness:            0",
  (char *)"rocs.*.XmLabelGadget*highlightThickness:      0",
  (char *)"rocs.*.XmToggleButtonGadget*highlightThickness: 0  ",
  (char *)"rocs.*.XmToggleButton*highlightThickness:     0  ",
  (char *)"rocs.*.XmRowColumn*spacing:                   0",
  (char *)"rocs*.scale_red*troughColor:                  RoyalBlue4",
  (char *)"rocs*.scale_green*troughColor:                Green",
  (char *)"rocs*.scale_blue*troughColor:                 Blue",
  (char *)"rocs*.highlightThickness:                     0",
  (char *)"rocs*.XmRowColumn*spacing:                    0",
  (char *)"rocs*.selectColor:                            RoyalBlue4",
  (char *)"rocs*.rocTab.shadowThickness:              2",
  (char *)"rocs*.rocTab.tabWidthPercentage:           10",
  (char *)"rocs*.rocTab.cornerwidth:                  2",
  (char *)"rocs*.rocTab.cornerheight:                 2",
  (char *)"rocs*.rocTab.textmargin:                   4",
  (char *)"rocs*.rocTab.foreground:                   blue",
  (char *)"rocs*.rocTab.tabcolor:                     lightGrey",
  

  /*sergey: testing*/
  (char *)"rocs*.rocFrame.foreground:                   Blue",
  (char *)"rocs*.rocFrame.background:                   LightGreen",
  (char *)"rocs* rocFrame*.borderColor:      blue",
  (char *)"rocs* rocFrame*.borderWidth:      2",
  /*sergey: testing*/



  (char *)"rocs*.top_ruler.background:                   lightGrey",
  (char *)"rocs*.left_ruler.background:                  lightGrey",
  (char *)"rocs*.top_ruler.foreground:                   White",
  (char *)"rocs*.left_ruler.foreground:                  White",
  (char *)"rocs*.top_ruler.tickerColor:                  White",
  (char *)"rocs*.left_ruler.tickerColor:                 White  ",
  (char *)"rocs*.edit_popup.*.background:                White",
  (char *)"rocs*.top_ruler.indicatorColor:               RoyalBlue4",
  (char *)"rocs*.left_ruler.indicatorColor:              RoyalBlue4",
  
  (char *)"rocs*icon_sw*background:                      lightGray",
  (char *)"rocs*icon_sw*foreground:                      black",
  
  (char *)"rocs*htext*background:                        lightGray",
  (char *)"rocs*htext*foreground:                        black",
  (char *)"rocs*anaLogFile.background:                   lightGrey",
  (char *)"rocs*anaLogFile.foreground:                   black",
  (char *)"rocs*anaLogFile.fontList:  -*-times-bold-i-*-*-16-*-*-*-*-*-*-*",

  (char *)"rocs.*.fileFr.topShadowColor:   red",
  (char *)"rocs.*.bootFr.topShadowColor:   red",
  (char *)"rocs.*.updFr.topShadowColor:   red",
  (char *)"rocs.*.monFr.topShadowColor:   red",

  (char *)"rocs*runInfoPanel*statuspanel.*.background:    lightGrey",

  (char *)"rocs*runNumber*backgroundPixmap:               XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*startTime*backgroundPixmap:               XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*endTime*backgroundPixmap:                 XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*eventLimit*backgroundPixmap:              XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*dataLimit*backgroundPixmap:               XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*status*backgroundPixmap:                  XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*time*backgroundPixmap:                    XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*exptname*backgroundPixmap:                XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*runType*backgroundPixmap:                 XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*runConfig*backgroundPixmap:               XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*exptid*backgroundPixmap:                  XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*hostname*backgroundPixmap:                XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*database*backgroundPixmap:                XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*session*backgroundPixmap:                 XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*eventNumber*backgroundPixmap:             XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*iDataRate*backgroundPixmap:               XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*dDataRate*backgroundPixmap:               XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*iEvRate*backgroundPixmap:                 XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*dEvRate*backgroundPixmap:                 XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*top_ruler*backgroundPixmap:               XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*left_ruler*backgroundPixmap:              XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*icon_sw*backgroundPixmap:                 XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*htext*backgroundPixmap:                   XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*initInfoPanelForm*backgroundPixmap:       XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*eventNumberG*backgroundPixmap:            XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*rcMsgWindow*backgroundPixmap:             XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*rcRateDisplay*backgroundPixmap:           XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*rcRateDisplay*foreground:                 black",
  (char *)"rocs*icon_sw*backgroundPixmap:                 XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*icon_box*backgroundPixmap:                XmUNSPECIFIED_PIXMAP",
  (char *)"rocs*icon*backgroundPixmap:                    XmUNSPECIFIED_PIXMAP",
  
  (char *)"rocs.*.RbuttonFrame.topShadowColor:   red",
  (char *)"rocs.*.LbuttonFrame.topShadowColor:   blue",

  NULL,
};

extern XcodaApp *theApplication;

Display *MainDisplay;
rcRocMenuWindow *menW;



void
Xhandler(Widget w, XtPointer p, XEvent *e, Boolean *b)
{  
  if (e->type == DestroyNotify) {
    printf("ROCS:X window was destroyed\n");
    exit(0);
  }
  /*
  return 0;
  */
}


void
messageHandler(char *message)
{
  printf("rocs::messageHandler reached, message >%s< !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",message);

  switch (message[0])
  {
    case 'c':
      printf("rocs::messageHandler: use configuration >%s<\n",&message[2]);
      menW->RocsSelectConfig((char *)&message[2]);  

      /*EditorSelectConfig(&message[2]);*/
      break;
    case 't':
    {
      char name[200];
      int pid;
      sscanf(&message[2],"%d %s",&pid,name);
      /*menW->createTabFrame(name,pid);*/
    }
    break;

    default:
      printf("rocs::messageHandler: unknown message : %s\n",message);
    
  }
}


void warningHandler(char *msg)
{
  return;
}





/* 'w' is 'rframe_', children are 'xtermsFrame_[]' (panedwindow), 2 per tab, subchildren are 'xterms[] */
/* 'we'll define children position and size based on 'frame_' size */
static void
resize (Widget w, XEvent *event, String args[], Cardinal *num_args)
{
  WidgetList         children, subchildren;
  int                ii, jj, kk, nchildren, nsubchildren;
  Dimension          w_width, w_height, x_width, x_height, width, height, pos, sum, prefsize, hmax, hmin, bla;
  short              margin_w, margin_h;
  /*
  XConfigureEvent    *cevent = (XConfigureEvent *) event;
  int                width = cevent->width;
  int                height = cevent->height;
  */


  /* get w's children and sizes */
  XtVaGetValues (w, XmNwidth, &w_width, XmNheight, &w_height, NULL);
  XtVaGetValues (w, XmNchildren, &children, XmNnumChildren, &nchildren, XmNmarginWidth, &margin_w, XmNmarginHeight, &margin_h, NULL);

#ifdef DEBUG
  printf("rocs:resize> main window: nchildren=%d, w_width=%d, w_height=%d, margin_w=%d, margin_h=%d\n",nchildren,w_width,w_height,margin_w,margin_h);
#endif
  for(ii=0; ii<nchildren; ii++)
  {
	/* do not need it, will be set from scratch using parent's widget sizes
    XtVaGetValues (children[ii], XmNwidth, &width, XmNheight, &height, NULL);
    printf("resize: children[%d]: width=%d, height=%d\n",ii,width,height);
	*/

    height = w_height - margin_h*2;
    width = (w_width/2)-((margin_w*3)/2);

    /* xtermsFrame_[] position: top left corner */
    XtVaSetValues (children[ii], XmNx, margin_w, XmNy, margin_h, NULL);

    /* xtermsFrame_[] size */
    XtVaSetValues (children[ii], XmNwidth, width, XmNheight, height, NULL);


    XtVaGetValues (children[ii], XmNchildren, &subchildren, XmNnumChildren, &nsubchildren, NULL);
#ifdef DEBUG
    printf("rocs:resize> panedwindow[%d] id=0x%08x nsubchildren=%d width=%d height=%d\n",ii,children[ii],nsubchildren,width,height);
#endif
    bla = height;
    sum = 0;
    for(jj=0; jj<nsubchildren; jj++)
	{
      XtVaGetValues (subchildren[jj], XmNheight, &height, XmNy, &pos, NULL);
	  
#ifdef DEBUG
      printf("rocs:resize> subchildren[%d]: id=0x%08x, height=%d pos=%d, XmIsSash=%d, XmIsSeparator=%d\n",
        jj,subchildren[jj],height,pos,XmIsSash(subchildren[jj]), XmIsSeparator(subchildren[jj]));
#endif
	  
      if(XmIsSash(subchildren[jj])) /* sash */
	  {
        ;
#ifdef DEBUG
        printf("rocs:resize> subchild[%d] sash, height=%d\n",jj,height);
#endif
	  }
      else if(XmIsSeparator(subchildren[jj])) /* separator */
	  {
        ;
#ifdef DEBUG
        printf("rocs:resize> subchild[%d] separator, height=%d\n",jj,height);
#endif
	  }
      else /* pane */
	  {
        if(height>10) sum += height;
#ifdef DEBUG
        printf("rocs:resize> subchild[%d] pane, height=%d, sum(so far)=%d\n",jj,height,sum);
#endif
	  }
	}

#ifdef DEBUG
    printf("rocs:resize>>>> height=%d, sum=%d, diff=%d\n",bla,sum,bla-sum);
#endif



	/**********************************************************************************************************/
    /* resize panes, keep them equal size for now, should use sash position in future when resizeSash() fixed */

      /* sergey: it seems changing xterms[] sizes (XmNheight, XmNpreferredPaneSize) does not effect how panes resized,
      the only way I found is setting XmNpaneMaximum and XmNpaneMinimum for xterms[], setting it to desired xterms[] height */
	
	bla = bla - 38; /* subtract bla-sum, not sure why 38, can be different on another monitor ... !!! */
	  hmax = (bla/5);
      hmin = (bla/5)-1;
#ifdef DEBUG
      printf("rocs:resize> xterms[%d]: hmax=%d, hmin=%d\n",jj,hmax,hmin);
#endif
	  for(jj=0; jj<menW->nxterms; jj++)
	  {
        XtVaSetValues (menW->xterms[jj], XmNpaneMaximum, hmax, XmNpaneMinimum, hmin, NULL);
        XtVaGetValues (menW->xterms[jj], XmNwidth, &x_width, XmNheight, &x_height, NULL);
#ifdef DEBUG
        printf("rocs:resize> xterms[%d]: x_width=%d, x_height=%d\n",jj,x_width,x_height); 
#endif
	  }

  }

}



/*global*/
extern int logging;
/*global*/

static int embedded;

int
main(int argc, char** argv)
{
  Widget widget0, widget;
  int ac, ii, jj, ix, iy, xpos, ypos, dx, dy, status;
  Arg arg[20];
  int firstxtermframe, nxtermframes;
  char tmp[128], tmp1[128];
  XtActionsRec   rec;
  WidgetList     children;
  int            nchildren;

  
  for(ii=0; ii<argc; ii++) printf("argv[%d] >%s<\n",ii,argv[ii]);
  embedded = 0;
  logging = 0;
  if(argc==2)
  {
    if(!strncmp(argv[1],"-embed",6)) embedded = 1;
  }
  else if(argc==3)
  {
    if(!strncmp(argv[1],"-embed",6)) embedded = 1;

    if( (!strncmp(argv[2],"-log",4)) || (!strncmp(argv[2],"-logs",5)) ) logging = 1;
  }


  if (getenv("CODA") == NULL)
  {
    printf ("$CODA must be set\n");
    exit(0);
  }

  struct rlimit limits;
  getrlimit(RLIMIT_NOFILE,&limits);
  
  limits.rlim_cur = limits.rlim_max;

  setrlimit(RLIMIT_NOFILE,&limits);
  
  /* set up command option */
  
  rcComdOption::initOptionParser (argc, argv);
  rcComdOption* option = rcComdOption::option ();
  
  option->reportMsgOn ();
  option->parseOptions ();
  
  /* setup database connection handler */
  (void)rcDbaseHandler::dbaseHandler ();
  
  XcodaApp* app = new XcodaApp (argv[0], fallback_res);

  /* open a X display ??? */
  app->open (&argc, argv);
  
  /* create a network handler which register itself to x window event loop */
  app_context = app->appContext ();
  
  rcClientHandler netHandler (app->appContext ());

  XtAppSetWarningHandler (app->appContext (), warningHandler);



  rcRocW* window  = new rcRocW ((char *)"Rocs", netHandler);

  
  app->initialize (&argc, argv);

    
  menW = window->window_;

  if (option->autostart_) menW->bootall_ = 1;

  toplevel = XtParent(XtParent(XtParent(menW->rform_)));

  MainDisplay = XtDisplay(toplevel);


#if 1
  /* set resizing for 'rframe_' */
  rec.string = (char *)"resize";
  rec.proc = resize;
  XtAppAddActions (app_context, &rec, 1);
  XtOverrideTranslations(menW->rframe_, XtParseTranslationTable ("<Configure>: resize()")); /* call resize() when parent window resized */
  XtOverrideTranslations(menW->rframe_, XtParseTranslationTable ("<Expose>: resize()")); /* call resize() when exposed (for example tabs switched) */
#endif

  menW->createXterms (menW->xtermsFrame_[0], (char *)"00_");
  menW->createXterms (menW->xtermsFrame_[1], (char *)"01_");




  for(jj=1; jj<NSPACE; jj++)
  {
    sprintf(tmp1,"rocs%d",jj);
    menW->createTabFrame(tmp1,0, &firstxtermframe, &nxtermframes);
    for(ii=firstxtermframe; ii<firstxtermframe+nxtermframes; ii++)
    {
      sprintf(tmp,"%02d_",ii);
      printf("%s: xtermsFrame number %d, window prefix >%s<\n",tmp1,ii,tmp);
      menW->createXterms (menW->xtermsFrame_[ii], tmp);
      XtUnmanageChild (menW->xtermsFrame_[ii]);
    }
  }





  /* at that point all xtermsFrame_[]'s in first tab have to be XtManageChild(), all others XtUnmanageChild(),
     otherwise have to click tab2 and then tab0 to display xterms on tab0 ... */


  ac = 0;
  XtSetArg (arg[ac], XmNresizePolicy, XmRESIZE_ANY); ac++;

#ifdef USE_CREG_HIDE /* done inside 'createXterms' for every individual window */
    printf("CREG 009: ALLROCS\n");
    codaSendInit(toplevel,"ALLROCS");
    codaRegisterMsgCallback((void *)messageHandler);
#endif    


  if (!option->startWide_)
  {
    Dimension width, height;
    /* set initial sizes 80% of screen */
    width = WidthOfScreen(XtScreen(menW->rform_));
    height = HeightOfScreen(XtScreen(menW->rform_));
    if(width>2049) width = width / 2;

    width = (width * 4) / 5;
    height = (height * 4) / 5;
    XResizeWindow(XtDisplay(XtParent(menW->rform_)),XtWindow(toplevel),width,height);
  }




#ifdef USE_CREG

  if(embedded)
  {
    printf("EMBEDDED !!!!!!!!!!!!!!!!!!!!!!!\n");

    static char *embedded_name = (char *)"rocs";

    int ac, ix;
    Arg args[10];
    Window parent;
    Widget w;
    char name[100];
    char cmd[100];
    parent = 0;


    printf("CREG 1\n");
    parent = 0;
    if (embedded)
    {
	  printf("wwwwwwwwwwwwwwwwwwwww CREG wwwwwwwwwwwwwww (-embed)\n");

      sprintf(name,"%s_WINDOW",embedded_name);
      printf("name >%s<\n",name);
      
      parent = CODAGetAppWindow(XtDisplay(toplevel),name);
      printf("parent=0x%08x\n",parent);
    }

    printf("CREG 2\n");
    if (parent)
    {
      printf("CREG 3\n");
      ac = 0;
      XtSetArg(args[ac], XmNx, 3000); ac++;
      XtSetValues (toplevel, args, ac);
      XtRealizeWidget(toplevel);
      XWithdrawWindow(XtDisplay(toplevel), XtWindow(toplevel),0);

      sprintf(cmd,"r:0x%08x 0x%08x",XtWindow(toplevel),parent);      
      printf("cmd >%s<\n",cmd);

      /* second parameter is the same as in parent's call 'codaSendInit(toplevel,"RUNCONTROL")' */
      coda_Send(XtDisplay(toplevel),(char *)"RUNCONTROL",cmd);

	  /* was in codaedit:*/
      codaSendInit(toplevel,(char *)"ALLROCS"); /* does NOT triggers 'motifHandler'->'resizeHandler' calls in codaRegistry */ 
      codaRegisterMsgCallback((void *)messageHandler);
      XtAddEventHandler(toplevel, StructureNotifyMask, False, Xhandler, NULL); /*Xhandler will exit if window was destroyed*/

    }
    else
    {
      printf("CREG 4\n");

      ac = 0;
      XtSetArg(args[ac], XmNoverrideRedirect,False); ac++;
      XtSetValues (toplevel, args, ac);

	  /* codaEditor() etc stuff was here */
    }

  }

#endif



  while (1)
  {
	printf("111\n");
    if (theApplication != NULL) app->execute();
	printf("222\n");
    if (theApplication == NULL) return(0);
	printf("333\n");
  }

  return(0);
}
