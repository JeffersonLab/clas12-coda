
/* daqMainFrame.h */

#include "TGDockableFrame.h"
#include "TGMenu.h"
#include "TGMdiDecorFrame.h"
#include "TG3DLine.h"
#include "TGMdiFrame.h"
#include "TGMdiMainFrame.h"
#include "TGMdiMenu.h"
#include "TGListBox.h"
#include "TGNumberEntry.h"
#include "TGScrollBar.h"
#include "TGComboBox.h"
#include "TGuiBldHintsEditor.h"
#include "TGuiBldNameFrame.h"
#include "TGFrame.h"
#include "TGFileDialog.h"
#include "TGShutter.h"
#include "TGButtonGroup.h"
#include "TGCanvas.h"
#include "TGFSContainer.h"
#include "TGuiBldEditor.h"
#include "TGColorSelect.h"
#include "TGButton.h"
#include "TGFSComboBox.h"
#include "TGLabel.h"
#include "TGMsgBox.h"
#include "TRootGuiBuilder.h"
#include "TGTab.h"
#include "TGListView.h"
#include "TGStatusBar.h"
#include "TGListTree.h"
#include "TGuiBldGeometryFrame.h"
#include "TGToolBar.h"
#include "TGuiBldDragManager.h"
#include "TGObject.h"
#include <TApplication.h>
#include <TSystem.h>
#include <TCanvas.h>
#include "TThread.h"
#include "TRootEmbeddedCanvas.h"
#include <TStyle.h>
#include <TGaxis.h>
#include <TExec.h>
#include <TH2.h>
#include <TProfile.h>
#include <TRandom.h>
#include <TGraph.h>

#include "Riostream.h"



//---------------------------------------------------------
//
//---------------------------------------------------------
class daqMainFrame : public TGMainFrame {

private:
  TGMainFrame *fMain;
  TGCompositeFrame *fComp1,*fComp;

public:

  TGTab *fTab;

  daqMainFrame(const TGWindow *p, UInt_t w, UInt_t h);
 ~daqMainFrame() { }

  TCanvas* add_tab(/*TGString**/  char* name);

  int ViewMyFrame(int ID);
  Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t parm2);

  int get_n_tabs() {
    return fTab->GetNumberOfTabs();
  }
  void set_tab_name(char *name ) {
    fTab->SetText(name);
  }
  void set_tab_name(char *name ,int tab) {
    fTab->SetTab(tab);
    fTab->SetText(name);
  }
  void set_tab_name(char *name ,char* old_name) {
    fTab->SetTab(old_name);
    fTab->SetText(name);
  }
  
  //-----------------------------
  //  Global VARs
  //-----------------------------
  TGTextButton *fPause;
  TGLabel *Lrunnum, *Levtnum;

  enum CommandsId {
    b_SEND = 100,
    b_SAVE,
    b_PAUSE,
    b_EXIT
  };
  int v_UPDATE, v_SEND, v_PAUSE, v_EXIT, v_SAVE;
  //-----------------------------
  int  CurrentTAB;

};
