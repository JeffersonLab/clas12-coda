
/* rcRocMenuWindow.h */

#ifndef _RC_ROC_MENU_WINDOW_H
#define _RC_ROC_MENU_WINDOW_H

#include "XcodaMenuWindow.h"
#include "rcClientHandler.h"
#include "rcPanel.h"
#include "XcodaErrorDialog.h"

/*sergey*/
#include "Editor_configSel.h"

class rcMenuComd;
class rcExit;
class rcClose;
class rcRocConfig; /*sergey*/
class rcConfigure;
class rcButtonFeedBack;
class rcDisplayMsg;
class rcOnLine;
class rcAnaLogDialog;

class rcAudioOption;
class rcTokenIButton;
class rcRcsMsgToDbase;

class rcWidthOption;
class rcUpdateIntervalDialog;
class rcCompBootDialog;
class rcMonitorParmsDialog;
class rcZoomButton;
class rcRateGraphButton;

class rcNetStatus;
class rcInfoPanel;
class rcHelpMsgWin;
class rcMsgWindow;

class rcHelpOverview;


/* extern */
class rcRocMenuWindow: public XcodaMenuWindow, public rcPanel
{

public:

  rcRocMenuWindow (Widget parent, char* name, rcClientHandler& handler);
  virtual ~rcRocMenuWindow (void);

  void createXterms (Widget widget, char *name);

  int createTabFrame (char *name,int pid, int *firstxtermframe, int *nxtermframes);

  // inherited config function
  void    config (int status);
  
  void    configBoot();
  void    configMonParms();
  // inherited config online option
  void    configOnlineOption (int online);
  
  // inherited config update interval
  void    configUpdateInterval (int interval);
  
  const Widget dialogBaseWidget (void);
  void  reportErrorMsg (char* msg);

  void    configRcsMsgToDbase (int yes);
  void    configTokenInterval (int interval);

  Widget            rframe_;
  Widget            rform_;
  Widget            rtab_;
  int               numTabs_;
  int               nxterms;

  Widget xtermsFrame_[200];
  Widget xterms[200];

  XcodaErrorDialog  *errDialog_;
  static void handle_tab (Widget w, XtPointer data, XtPointer calldata);
  static void destroyHandler(Widget w,void *data,XEvent *eventPtr,Boolean *b);
  rcCompBootDialog* bootButton_;
  int bootall_;

  /*sergey*/
  void InitConfigSel (Widget parent);
  void ConfigSelPopup (void);

  static void RocsSelectConfig(char *currconfig);


protected:

  //sergey
  static void loadRcDbaseCbk (int status, void* arg, daqNetData* data);

  
  // enter and leaving window event handler
  static void crossEventHandler (Widget w, XtPointer, XEvent*, Boolean);
  virtual Widget createMenuWindow (Widget );
  virtual void   createMenuPanes  (void);

  /*sergey*/
  Widget CreateOptionMenu (Widget parent);
  static void ConfigSelPopdown (void);
  static void ConfigSelOk (Widget w, XtPointer data, /*temporary: XmAnyCallbackStruct*/void* cbs);
  static void ConfigSelCancel (Widget w, XtPointer data, /*temporary: XmAnyCallbackStruct*/void* cbs);



private:

  // tab widgets
  char* tabLabels_[200];

  // network handler
  rcClientHandler& netHandler_;

  //sergey
  //rcClient& client_;
  rcClient* client_;

  // file menu
  rcExit*        exit_;
  rcClose*       close_;

  /*sergey*/
  rcRocConfig*   config_;
  /*sergey*/



  // preference menu
  rcButtonFeedBack* feedBack_;
  rcDisplayMsg*     dispMsg_;

  rcAudioOption*    audio_;
  rcWidthOption*    Owidth_;
  rcRcsMsgToDbase*  serverMsgToDbase_;

  rcOnLine*         online_;
  // option menu
  rcAnaLogDialog*   anaLogButton_;
  rcUpdateIntervalDialog* updateInterval_;

  rcTokenIButton*   tokenIButton_;

  rcMonitorParmsDialog* monParmsButton_;
  rcZoomButton*     zoomButton_;
  rcRateGraphButton* rateGraphButton_;
  // help menu
  rcHelpOverview*  helpOverview_;
  rcMenuComd*      helpAbout_;
  // network communication dynamic status
  rcNetStatus*   netStatus_;
  // initial information panel
  rcInfoPanel*   rcipanel_;
  // help message window (button feedback message window)
  rcHelpMsgWin*  helpMsgWin_;
  // server message window
  rcMsgWindow*   msgw_;
};
#endif
