/* scope.cxx */

#include <stdlib.h>

#include <TROOT.h>
#include <TApplication.h>
#include <TVirtualX.h>
#include <TVirtualPadEditor.h>
#include <TGResourcePool.h>
#include <TGListBox.h>
#include <TGListTree.h>
#include <TGFSContainer.h>
#include <TGClient.h>
#include <TGFrame.h>
#include <TGIcon.h>
#include <TGLabel.h>
#include <TGButton.h>
#include <TGTextEntry.h>
#include <TGNumberEntry.h>
#include <TGMsgBox.h>
#include <TGMenu.h>
#include <TGCanvas.h>
#include <TGComboBox.h>
#include <TGTab.h>
#include <TGSlider.h>
#include <TGDoubleSlider.h>
#include <TGFileDialog.h>
#include <TGTextEdit.h>
#include <TGShutter.h>
#include <TGProgressBar.h>
#include <TGColorSelect.h>
#include <TRootEmbeddedCanvas.h>
#include <TCanvas.h>
#include <TColor.h>
#include <TH1.h>
#include <TH2.h>
#include <TRandom.h>
#include <TSystem.h>
#include <TSystemDirectory.h>
#include <TEnv.h>
#include <TFile.h>
#include <TKey.h>
#include <TGDockableFrame.h>
#include <TGFontDialog.h>
#include <TPolyLine.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TGraph.h>
#include <TRootCanvas.h>
#include <TText.h>
#include <TGStatusBar.h>

#include "cratemsgclient.h"
#include "scope.h"

/*NEW*/
#ifdef _DEBUG
#define new DEBUG_NEW
/*
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
*/
#endif

#define TRIGPATTERN_0      0
#define TRIGPATTERN_1      1
#define TRIGPATTERN_X      2

#define TRIGGER_CAP_LEN    128

#define	TEXT_HEIGHT        20 // VSTEP ???

#define TRIG_POLLRATE_MS   100
#define TRIG_LOOPS         1000
#define SCOPE_MIN_X        36.
#define SCOPE_MAX_X        (SCOPE_MIN_X+(Double_t)TRIGGER_CAP_LEN)

#define SIG_LOW            0x1
#define SIG_HIGH           0x2
#define SIG_TRANS          0x4
/*NEW*/

#define SWW                1155 // scope screen width (165. x 7)
#define SHH                640 // scope screen hight
#define VSTEP              20 // pixels per channel


//sergey: global for now, will find appropriate place later
extern CrateMsgClient *tcp;




ScopeTimer::ScopeTimer(ScopeDlg *m, Long_t ms) : TTimer(ms, kTRUE)
{
  fScopeDlg = m;
  gSystem->AddTimer(this);
}

// That function will be called in case of timeout INSTEAD OF
// standart Notify() function from TTimer class
Bool_t ScopeTimer::Notify()
{
  Bool_t repeat;

  //printf("ScopeTimer::Notify() reached\n");fflush(stdout);
  repeat = fScopeDlg->OnTimer();

  // reset timer only if we are in continuous mode or if did not get anything in single trigger mode, otherwise stop it
  if(fScopeDlg->m_bTriggerContinuous || repeat) this->Reset();
  else this->Stop();

  return kTRUE;
}
//------------------------------------------------------------------------------


/*********************************/
/* ScopeDlg class implementation */

/*canvas*/
ScopeDlg::ScopeDlg(const TGWindow *p, TGMainFrame *main, UInt_t w, UInt_t h, UInt_t addr[2],
                   char SignalNames0[SCOPE_CHANNELCOUNT][20], char SignalNames1[SCOPE_CHANNELCOUNT][20], UInt_t options)
{
  int sig;
  int i=0;
  printf("ScopeDlg::ScopeDlg reached\n");fflush(stdout);

  fMain = main; // remember mainframe
  board_address[0] = addr[0];
  board_address[1] = addr[1];

  printf("ScopeDlg::ScopeDlg board addresses set to 0x%08x and 0x%08x\n",board_address[0],board_address[1]);

  sig=0;
  for(int ii=0; ii<SCOPE_CHANNELCOUNT; ii++) if(strcmp(SignalNames0[ii],"unused")) strcpy(m_sSignalNames[sig++],SignalNames0[ii]);
  nused[0]=sig;

  sig=0;
  for(int ii=0; ii<SCOPE_CHANNELCOUNT; ii++) if(strcmp(SignalNames1[ii],"unused")) strcpy(n_sSignalNames[sig++],SignalNames1[ii]);
  nused[1]=sig;

  // Create a dialog window. A dialog window pops up with respect to its "main" window.

  // use hierarchical cleaning
  SetCleanup(kDeepCleanup);

  OnInitDialog();

  if(options&0x1) use_ascii = FALSE;
  else use_ascii = TRUE;

  // ascii version
  if(use_ascii)
  {
    /* top menu */

    fF2 = new TGHorizontalFrame(this, 800, 100);
    fL3 = new TGLayoutHints(kLHintsTop | kLHintsLeft, 2, 2, 2, 2);

    //fF2->SetScrolling(2);

    this->AddFrame(fF2, fL3);

    fPersist = new TGCheckButton(fF2, "Infinite Persistence", 21);
    fPersist->Associate(this);
    fF2->AddFrame(fPersist, fL3);

    fClearButton = new TGTextButton(fF2, " Clear Display ", 1);
    fClearButton->Associate(this);
    fF2->AddFrame(fClearButton, fL3);

    fContinueButton = new TGTextButton(fF2, " Continuous Trigger ", 2);
    fContinueButton->Associate(this);
    fF2->AddFrame(fContinueButton, fL3);

    fSingleButton = new TGTextButton(fF2, " Single Trigger ", 3);
    fSingleButton->Associate(this);
    fF2->AddFrame(fSingleButton, fL3);

    fStopButton = new TGTextButton(fF2, " Stop ", 4);
    fStopButton->Associate(this);
    fStopButton->SetEnabled(kFALSE);
    fF2->AddFrame(fStopButton, fL3);

    fSelectButton = new TGTextButton(fF2, " Multiple Select ", 5);
    fSelectButton->Associate(this);
    fF2->AddFrame(fSelectButton, fL3);


    /* frame for screen */

    fF4 = new TGHorizontalFrame(this, 800, 700);
    fL4 = new TGLayoutHints(kLHintsBottom | kLHintsLeft, 2, 2, 2, 2);
    this->AddFrame(fF4, fL4);
    this->HideFrame(fF4);


    fL5 = new TGLayoutHints(kLHintsBottom | kLHintsRight, 2, 2, 2, 2);
    fF4->AddFrame(fListBox = new TGListBox(fF4, 31), fL5);
    fListBox->Associate(this);


    // draw empty scope for the first time
    for(int i=0; i<SCOPE_CHANNELCOUNT; i++)
    {
      //printf("111\n");fflush(stdout);
      sprintf(fStr[i], "[%03d] %6.6s  x  0 | \0", i,m_sSignalNames[i]);
      //printf("222\n");fflush(stdout);
      for(int j=0; j<TRIGGER_CAP_LEN; j++) strcat(fStr[i],"_");
      //printf("333\n");fflush(stdout);
      printf("[%3d] >%s<\n",i,fStr[i]);
      fListBox->AddEntry(fStr[i], i);
    }
    fListBox->Resize(1200, 680/*580*/); // actual size of the main frame


    // set dialog box title
    SetWindowName("Scope");
    SetIconName("Scope");
    SetClassHints("ScopeDlg", "ScopeDlg");

    // resize & move to center
    MapSubwindows();
    UInt_t width = GetDefaultWidth();
    UInt_t height = GetDefaultHeight();
    Resize(width, height);
    //CenterOnParent();

    MapWindow();
    /*fClient->WaitFor(this);???*/
  }
  else // canvas version
  {
    // At creation time, the canvas size defines the size of the canvas window 
    // (including the window manager's decoration). We'll define precisely the graphics
    // area size of a canvas, making it exactly ww x hh
    Double_t ww = SWW;
    Double_t hh = SHH;
    fCanvas = new TCanvas("c", "Scope", ww, hh);
    printf("Sizes befor: %d %d\n",fCanvas->GetWw(),fCanvas->GetWh());
    fCanvas->SetWindowSize(ww + (ww - fCanvas->GetWw()), hh + (hh - fCanvas->GetWh()));
    printf("Sizes after: %d %d\n",fCanvas->GetWw(),fCanvas->GetWh());

    fCanvas->Range(0.,0.,165.,256.); // define 200x256 drawing area
    fCanvas->SetFillColor(10);
    fCanvas->SetBorderSize(2);



	/* make canvas close application, when canvas is closing - it closing everything !!! */
    //fCanvas->Connect("TCanvas", "Closed()", "TApplication", gApplication, "Terminate()");

    /* when closing ascii, CloseWindow called, when non-ascii - not; following call not needed */
	//fCanvas->Connect("TCanvas", "Closed()", "TApplication", this, "CloseWindow()");


    // get access to TRootCanvas imp
    fRootCanvas = (TRootCanvas *)fCanvas->GetCanvasImp();

    // get access to the menu bar
    TGMenuBar *mbar = fRootCanvas->GetMenuBar();

    // Create status bar in the bottom of the canvas
    int parts[] = { 45, 10, 10, 35 };
    fStatusBar = new TGStatusBar(fRootCanvas, 1130, 20);
    fStatusBar->SetParts(parts, 4);
    TGLayoutHints *fStatusBarLayout = 
      new TGLayoutHints(kLHintsBottom | kLHintsLeft | kLHintsExpandX, 2, 2, 1, 1);
    fRootCanvas->AddFrame(fStatusBar, fStatusBarLayout);
    fRootCanvas->ShowFrame(fStatusBar);
    //fRootCanvas->HideFrame(fStatusBar);


    // cusomize existing status bar
    //sbar = fRootCanvas->GetStatusBar();
    //Int_t parts[3] = {20,30,50};
    //sbar->SetParts(parts,3);


    // set size bigger then original to generate scrollbar
    fCanvas->SetCanvasSize(ww-16, VSTEP*SCOPE_CHANNELCOUNT);
	

    //TGPopupMenu *fff = mbar->GetPopup("Help");
    //printf("fff=0%08x\n",fff);
    //mbar->RemovePopup("Help");

    // menu item 'Scope'
    TGPopupMenu *fMenuScope;
    fMenuScope = new TGPopupMenu(fClient->GetRoot());
    fMenuScope->AddLabel("Scope control");
    fMenuScope->AddSeparator();
    fMenuScope->AddEntry("&Infinite Persistence",21);
    fMenuScope->AddEntry("&Clear Display", 1);
    fMenuScope->AddEntry("&Continuous Trigger", 2);
    fMenuScope->AddEntry("&Single Trigger", 3);
    fMenuScope->AddEntry("&Stop", 4);

    // Menu button messages are handled by the main frame (i.e. "this") ProcessMessage() method.
    fMenuScope->Associate(this);

    // actually add created above menus to menu bar
    mbar->AddPopup("&Scope", fMenuScope, 0);
    mbar->Layout();

    fCanvas->SetEditable(TRUE);







	









    // text attributes
    fText.SetTextFont(20);
    fText.SetTextSize(0.015);
    fText.SetTextAlign(12); // left adjust, vertially centered


    // initially do not draw cursor
    xcursor = 0.;
    for(int i=0; i<SCOPE_CHANNELCOUNT; i++) fCurchar[i] = '0';


	/*
    // polyline attributes
    fPolyLine = new TPolyLine(256);
    fPolyLine->SetFillColor(38);
    fPolyLine->SetLineWidth(2);
    fPolyLine->SetLineColor(kGreen);
	*/
    Double_t x[256], y[256];
    Double_t x0 = SCOPE_MIN_X;
    Double_t y0 = 255.;
    int ii = 0;
    for(int i=0; i<128; i++)
    {
      x[ii++] = x0 + (Double_t)i;
      printf("x1-ii=%d\n",ii);fflush(stdout);
      x[ii++] = x0 + (Double_t)i;
      printf("x2-ii=%d\n",ii);fflush(stdout);
    }
    for(int j=0; j<SCOPE_CHANNELCOUNT; j++)
    {
      printf("y0=%f\n",y0);
      char ch='X';
      sprintf(fStr[j], "[%03d] %6.6s   %c   %c",j,m_sSignalNames[j],ch,fCurchar[j]);
      fText.DrawText(1.,y0+0.5,fStr[j]);
	  /*
      ii = 0;
      for(int i=0; i<128; i++)
      {
        if(i%2)
        {
          y[ii++] = y0 + 0.1;
          y[ii++] = y0 + 0.9;
	    }
	    else
	    {
          y[ii++] = y0 + 0.9;
          y[ii++] = y0 + 0.1;
	    }
      }
      fPolyLine->DrawPolyLine(256,x,y);
	  */
      y0 -= 1.;
    }
	/*
    //draw middle line
    fPolyLine->SetLineColor(kRed);
    x[0] = x0 + 64.;
    x[1] = x0 + 64.;
    y[0] = 256.;
    y[1] = 0.;
    fPolyLine->DrawPolyLine(2,x,y);
	*/

    // need following to activate ExecuteEvent(); right click on drawing area gives
    // TGMainFrame:fMainFrame31, and ExecuteEvent() called; without it right click 
    // gives TCamvas::c, and ExecuteEvent() never called
    Draw();

    fCanvas->SetEditable(FALSE);
    fCanvas->Update();
  }

  fMedian = NULL;
  fCursor = NULL;

  fScopeTimer = new ScopeTimer(this, 100); //1000 = 1sec
}


ScopeDlg::~ScopeDlg()
{
  printf("ScopeDlg::~ScopeDlg reached\n");fflush(stdout);
   // Delete ScopeDlg widgets.
}


void ScopeDlg::CloseWindow()
{
  printf("ScopeDlg::CloseWindow reached\n");fflush(stdout);
   // Called when window is closed (via the window manager or not).

   // ... and close the Ged editor if it was activated.
   if(TVirtualPadEditor::GetPadEditor(kFALSE) != 0)
   {
     printf("Terminating ..\n");
     TVirtualPadEditor::Terminate();
   }

   if(fScopeTimer) fScopeTimer->Delete();
   DeleteWindow();
   // fMain->ClearScopeDlg(); // clear pointer to ourself, so FainFrame will stop reading/writing VME

}


/* process mouse events
   kNoEvent       =  0,
   kButton1Down   =  1, kButton2Down   =  2, kButton3Down   =  3, kKeyDown  =  4,
   kWheelUp       =  5, kWheelDown     =  6, kButton1Shift  =  7, kButton1ShiftMotion  =  8,
   kButton1Up     = 11, kButton2Up     = 12, kButton3Up     = 13, kKeyUp    = 14,
   kButton1Motion = 21, kButton2Motion = 22, kButton3Motion = 23, kKeyPress = 24,
   kButton1Locate = 41, kButton2Locate = 42, kButton3Locate = 43, kESC      = 27,
   kMouseMotion   = 51, kMouseEnter    = 52, kMouseLeave    = 53,
   kButton1Double = 61, kButton2Double = 62, kButton3Double = 63
*/
void ScopeDlg::ExecuteEvent(Int_t event, Int_t px, Int_t py)
{
  UInt_t nflags;
  Int_t ix,iy;
  Double_t x[2], y[2];
  Double_t x0 = SCOPE_MIN_X;

  printf("ScopeDlg::ExecuteEvent reached\n");fflush(stdout);
  printf("ScopeDlg::ExecuteEvent reached: %d(%d,%d) %d %d\n",event,GET_MSG(event),GET_SUBMSG(event),px,py);
  switch(event)
  {
  case kMouseMotion:
    printf("kMouseMotion\n");fflush(stdout);
    char txt[80];
    x[0] = gPad->AbsPixeltoX(px);
    y[0] = gPad->AbsPixeltoY(py);
    printf("--> %f %f\n",x[0],y[0]);
    /*printf("-->%s<--\n",GetObjectInfo(px,py));-returns string like -->x=80.3995, y=225.6<--*/
    sprintf(txt,"Current mouse position is %d ns",((Int_t)x[0]-TRIGGER_CAP_LEN/2-(Int_t)(SCOPE_MIN_X))*5);
    fStatusBar->SetText(txt, 3);
    break;
  case kButton1Down:
    printf("kButton1Down\n");fflush(stdout);
    x[0] = ((Double_t)px)/6.9;
    y[0] = ((Double_t)py)/((Double_t)VSTEP);
    ix = (Int_t)x[0];
    iy = (Int_t)y[0];
    printf("x[0]=%f y[0]=%f -> iy=%d\n",x[0],y[0],iy);
    OnLButtonDown(nflags,ix,iy);
    break;
  case kButton1Up:
	printf("kButton1Up\n");fflush(stdout);
	break;
  case kButton2Down:
	printf("kButton2Down\n");fflush(stdout);
	break;
  case kButton2Up:
	printf("kButton2Up\n");fflush(stdout);
	break;
  case kButton3Down:
	printf("kButton3Down\n");fflush(stdout);
	break;
  case kButton3Up:
	printf("kButton3Up\n");fflush(stdout);
	break;
  case kWheelUp:
	printf("kWheelUp\n");fflush(stdout);
	break;
  case kWheelDown:
	printf("kWheelDown\n");fflush(stdout);
	break;
  case kMouseLeave:
	printf("kMouseLeave\n");fflush(stdout);
	break;
  default:
	printf("UNKNOWN ACTION !!!!!!!!!!!!!!!!!!!!\n");fflush(stdout);
  }
}

Bool_t ScopeDlg::ProcessMessage(Long_t msg, Long_t parm1, Long_t parm2)
{
  Int_t ret, x, y;
  UInt_t nflags;
  Float_t fx, fy;

  printf("ScopeDlg::ProcessMessage reached\n");fflush(stdout);

  // Process messages coming from widgets associated with the dialog.
  printf("ScopeDlg::ProcessMessage: msg=%d get_msg=%d get_submsg=%d parm1=%d parm2=%d\n",
		 msg,GET_MSG(msg),GET_SUBMSG(msg),parm1,parm2);

  switch (GET_MSG(msg))
  {
    case kC_COMMAND:

    switch (GET_SUBMSG(msg))
    {
      case kCM_MENU:
        switch(parm1)
        {
          case 1:
            printf("Menu 'Clear Display' selected, parm1=%d\n",parm1);
            OnBScopeclear();
            break;
          case 2:
            printf("Menu 'Continuous Trigger' selected, parm1=%d\n",parm1);
			//fContinueButton->SetEnabled(kFALSE);
			//fSingleButton->SetEnabled(kFALSE);
			//fStopButton->SetEnabled(kTRUE);
            OnBScopecontinuous();
            break;
          case 3:
            printf("Menu 'Single Trigger' selected, parm1=%d\n",parm1);
            OnBScopesingle();
            break;
          case 4:
            printf("Menu 'Stop' selected, parm1=%d\n",parm1);
			//fContinueButton->SetEnabled(kTRUE);
			//fSingleButton->SetEnabled(kTRUE);
			//fStopButton->SetEnabled(kFALSE);
            OnBScopestop();
            break;
          case 21:
            printf("Menu 'Infinite Persistence' selected, parm1=%d\n",parm1);
            m_Persist = FALSE;
            break;
          default:
            break;
        }
        break;
			   
      case kCM_BUTTON:
        switch(parm1)
        {
          case 1:
            printf("Button 'Clear Display' pressed, parm1=%d\n",parm1);
            OnBScopeclear();
            break;
          case 2:
            printf("Button 'Continuous Trigger' pressed, parm1=%d\n",parm1);
			fContinueButton->SetEnabled(kFALSE);
			fSingleButton->SetEnabled(kFALSE);
			fStopButton->SetEnabled(kTRUE);
            OnBScopecontinuous();
            break;
          case 3:
            printf("Button 'Single Trigger' pressed, parm1=%d\n",parm1);
            OnBScopesingle();
            break;
          case 4:
            printf("Button 'Stop' pressed, parm1=%d\n",parm1);
			fContinueButton->SetEnabled(kTRUE);
			fSingleButton->SetEnabled(kTRUE);
			fStopButton->SetEnabled(kFALSE);
            OnBScopestop();
            break;
          default:
            break;
        }
        break;
			   
	  case kCM_RADIOBUTTON:
        switch (parm1)
        {
		  case 11:
            printf("Radiobutton pressed, parm1=%d\n",parm1);
			//fRad2->SetState(kButtonUp);
			break;
        }
        break;
	   
	  case kCM_CHECKBUTTON:
        switch (parm1)
        {
		  case 21:
			ret = fPersist->GetState();
			printf("Checkbutton pressed, parm1=%d, tst=%d\n",parm1,ret);
            if(ret) m_Persist = TRUE;
            else    m_Persist = FALSE;
			break;
		  default:
			break;
        }
        break;

	  case kCM_LISTBOX:
		printf("parms: %d %d\n",parm1,parm2);
        switch (parm1)
        {
		  case 31:
			fScroll = fListBox->GetVScrollbar();
			printf("Listbox touched, parms=%d (%d), scroll top at %d\n",
			   parm1,fListBox->GetSelected(),fScroll->GetPosition());
            y = fListBox->GetSelected(); // in ascii mode y means entry number from 0, x and nflags unused
            x = 0; // anything less then SCOPE_MIN_X
            OnLButtonDown(nflags,x,y);


            //TGPosition pos = GetPagePosition();
            //gVirtualX->GetCharacterUp(fx, fy);
            //printf("1: %f %f\n",fx,fy);
			{
              //Int_t mode=0;
              //Int_t ctyp=1;
              //Int_t x;
              //Int_t y;
			  //gVirtualX->RequestLocator(mode,ctyp,x,y);
              //printf("2: %d %d\n",x,y);
			}

			// hungs computer !!!
			//   gVirtualX->GrabPointer(gClient->GetDefaultRoot()->GetId(), kButtonPressMask |
            //              kButtonReleaseMask | kPointerMotionMask, kNone,
			//            gVirtualX->CreateCursor(kWatch), kTRUE, kFALSE);

			break;
	    }

      default:
        break;
    }
    break;

    default:
      break;
  }
  return kTRUE;
}

/*
~sergpozd/scripts/camac_discrim_test 4 100 40 croctest4 0
*/


/***************************NEW*******************************/


//ser ScopeDlg::ScopeDlg(CWnd* pParent /*=NULL*/)
//ser 	: CDialog(ScopeDlg::IDD, pParent)
//ser {
//ser   ;
//ser }


//ser void ScopeDlg::DoDataExchange(CDataExchange* pDX)
//ser {
  //ser CDialog::DoDataExchange(pDX);
  //ser DDX_Control(pDX, IDC_CH_PERSIST, m_Persist);
  //ser DDX_Control(pDX, IDC_SCROLL_SCOPE, m_scrollScope);
//ser }


/////////////////////////////////////////////////////////////////////////////
// ScopeDlg message handlers
// process scroll bar events - to check up-down moves and draw/not draw new lines
void ScopeDlg::OnVScroll(UInt_t nSBCode, UInt_t nPos, /*CScrollBar*/TGScrollBar* pScrollBar)
{
  printf("ScopeDlg::OnVScroll() reached\n");fflush(stdout);
  ;
}

// reads vme on timer (about 100ms)
Bool_t ScopeDlg::OnTimer() 
{
  //printf("ScopeDlg::OnTimer() reached\n");fflush(stdout);

  unsigned int status1, status2, tmp;

  /*if(!tcp->m_bConnected) return(FALSE); // do nothing if not connected*/

  if(!tcp->Read32(board_address[0] + ECAL_TRIG_STATUS, &status1))
  {
    tcp->DebugMsg("ERROR: Scope Trigger Failed to Read Status 1");
    return(FALSE);
  }

  if(board_address[1]!=0)
  {
    if(!tcp->Read32(board_address[1] + ECAL_TRIG_STATUS, &status2))
    {
      tcp->DebugMsg("ERROR: Scope Trigger Failed to Read Status 2");
      return(FALSE);
    }
  }

  //printf("-------> status1=0x%08x status2=0x%08x\n",status1,status2);
  if((status1 & 0x2) || (status2 & 0x2))
  {
	//printf("-------> 1\n");
    if(!m_Persist)
    {
      memset(m_iTriggerBuffer_Low, 0, sizeof(m_iTriggerBuffer_Low));
      memset(m_iTriggerBuffer_High, 0, sizeof(m_iTriggerBuffer_High));
      memset(m_iTriggerBuffer_TransLow, 0, sizeof(m_iTriggerBuffer_TransLow));
      memset(m_iTriggerBuffer_TransHigh, 0, sizeof(m_iTriggerBuffer_TransHigh));
      memset(m_iTriggerBuffer, 0, sizeof(m_iTriggerBuffer));
      if(board_address[1]!=0)
      {
        memset(n_iTriggerBuffer_Low, 0, sizeof(n_iTriggerBuffer_Low));
        memset(n_iTriggerBuffer_High, 0, sizeof(n_iTriggerBuffer_High));
        memset(n_iTriggerBuffer_TransLow, 0, sizeof(n_iTriggerBuffer_TransLow));
        memset(n_iTriggerBuffer_TransHigh, 0, sizeof(n_iTriggerBuffer_TransHigh));
        memset(n_iTriggerBuffer, 0, sizeof(n_iTriggerBuffer));
      }
    }
	//printf("-------> 2\n");
    ReadoutScope();
	//printf("-------> 3\n");

    if(m_bTriggerContinuous)
    {
      tmp = 0;
      tcp->Write32(board_address[0] + ECAL_TRIG_STATUS, &tmp);
      if(board_address[1]!=0) tcp->Write32(board_address[1] + ECAL_TRIG_STATUS, &tmp);
      tmp = 1;
      tcp->Write32(board_address[0] + ECAL_TRIG_STATUS, &tmp);
      if(board_address[1]!=0) tcp->Write32(board_address[1] + ECAL_TRIG_STATUS, &tmp);
    }
    else
    {
      ;
    }
    //printf("-------> 4\n");fflush(stdout);
    DrawScope();
    //printf("-------> 5\n");fflush(stdout);
  }
  else
  {
    return(TRUE); // not ready, tell timer to run again
  }

  return(FALSE);
}

// process left mouse button moving cursor
void ScopeDlg::OnMouseMove(UInt_t nFlags, Int_t point_x, Int_t point_y) 
{
  printf("OnMouseMove reached\n");fflush(stdout);
  ;
}

// left button on mouse
// process left button on mouse for 2 cases: move cursor on screen and switch 1-0-x
void ScopeDlg::OnLButtonDown(UInt_t nFlags, Int_t point_x, Int_t point_y) 
{
  printf("OnLButtonDown reached\n");fflush(stdout);
  printf("OnLButtonDown reached, x=%d y=%d SCOPE_MIN_X=%d\n",point_x,point_y,(Int_t)SCOPE_MIN_X);

  if(point_x < (Int_t)SCOPE_MIN_X)  // toggle 0-1-x sequence
  {
    if(point_y>=0 && point_y<256)
    {
      printf("OnLButtonDown: point_y=%d\n",point_y);
      int TrigPatternHit = point_y;
      m_iTriggerPattern[TrigPatternHit] = (m_iTriggerPattern[TrigPatternHit] + 1) % 3;
      UpdateTriggerMasks();
    }
  }
  else // draw cursor
  {
    UpdateCursor(point_x, point_y, TRUE, TRUE);
    printf("ScopeDlg::OnLButtonDown ERROR: point_y=%d\n",point_y);
  }
  DrawScope();
}

BOOL ScopeDlg::OnInitDialog() 
{
  int i;

  printf("OnInitDialog reached\n");fflush(stdout);

  for(i=0; i<SCOPE_CHANNELCOUNT; i++) m_iTriggerPattern[i] = TRIGPATTERN_X;

  memset(m_iTriggerBuffer_Low, 0, sizeof(m_iTriggerBuffer_Low));
  memset(m_iTriggerBuffer_High, 0, sizeof(m_iTriggerBuffer_High));
  memset(m_iTriggerBuffer_TransLow, 0, sizeof(m_iTriggerBuffer_TransLow));
  memset(m_iTriggerBuffer_TransHigh, 0, sizeof(m_iTriggerBuffer_TransHigh));
  memset(m_iTriggerBuffer, 0, sizeof(m_iTriggerBuffer));
  if(board_address[1]!=0)
  {
    memset(n_iTriggerBuffer_Low, 0, sizeof(n_iTriggerBuffer_Low));
    memset(n_iTriggerBuffer_High, 0, sizeof(n_iTriggerBuffer_High));
    memset(n_iTriggerBuffer_TransLow, 0, sizeof(n_iTriggerBuffer_TransLow));
    memset(n_iTriggerBuffer_TransHigh, 0, sizeof(n_iTriggerBuffer_TransHigh));
    memset(n_iTriggerBuffer, 0, sizeof(n_iTriggerBuffer));
  }

  m_iCursorSample = TRIGGER_CAP_LEN / 2;

  TString str;
  str.Format("Cursor = %dns", (int)((m_iCursorSample - TRIGGER_CAP_LEN / 2) * CLOCK_PERIOD_NS));
  m_Persist = FALSE;

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

//callback to repaint, redraw scope and texts
void ScopeDlg::OnPaint() 
{
  printf("OnPaint reached\n");fflush(stdout);
  ;
}

// chouse line color
unsigned int ScopeDlg::GetColor(unsigned int intensity, unsigned int odd)
{
  //printf("GetColor reached\n");fflush(stdout);
  return(0);
}



/* draws scope based on following arrays, representing TRANSITIONS:
   m_iTriggerBuffer_Low[][] - from low to low
   m_iTriggerBuffer_High[][] - from high to high
   m_iTriggerBuffer_TransHigh[][] - from low to high
   m_iTriggerBuffer_TransLow[][] - from high to low
*/
void ScopeDlg::DrawScope()
{
  //printf("ScopeDlg::DrawScope reached\n");fflush(stdout);

  int ii, i, j, icursor, y = 50;
  char ch;
  Double_t xx[300], yy[300];
  Double_t xmed[2], ymed[2];
  Double_t xcur[2], ycur[2];
  Double_t x0 = SCOPE_MIN_X; // left edge of the drawing area
  Double_t y0 = 255.; // bottom of the upper line
  Double_t xstep = 1.;
  Double_t ystep = 1.;

  /*SERGEY: TESTING*/
  //fCanvas->DontCallClose();

  // for graphic mode, draw background boxes and handle persistency
  if(!use_ascii)
  {
    //printf("fCanvas=%d\n",fCanvas);fflush(stdout);
    if (fCanvas->GetCanvasImp()==NULL) {printf("fCancas window has been closed\n");fflush(stdout);};

    fCanvas->SetEditable(TRUE);
    fCanvas->Clear();

    // polyline attributes
    fPolyLine = new TPolyLine(SCOPE_CHANNELCOUNT);
    //fPolyLine->SetFillColor(38);
    fPolyLine->SetLineWidth(1);
    fPolyLine->SetLineColor(kGreen);

    // median line attributes
    if(fMedian) delete fMedian;
    fMedian = new TPolyLine(2);
    fMedian->SetLineWidth(1);
    fMedian->SetLineColor(kRed);

    // cursor line attributes
    if(fCursor) delete fCursor;
    fCursor = new TPolyLine(2);
    fCursor->SetLineWidth(1);
    fCursor->SetLineColor(kBlue);
  }


  y = 0; // same as 'i' ???
  //for(i=m_scrollScope.GetScrollPos(); i<(m_scrollScope.GetScrollPos()+32); i++) //draw visible are only
  for(i=0; i<SCOPE_CHANNELCOUNT; i++) //256
  {
    int ii;

    if(m_iTriggerPattern[i] == TRIGPATTERN_0)      ch = '0';
    else if(m_iTriggerPattern[i] == TRIGPATTERN_1) ch = '1';
    else                                           ch = 'X';

    j = (Int_t)(xcursor - SCOPE_MIN_X);
    if(m_iTriggerBuffer[i][j]) fCurchar[i] = '1';
    else                       fCurchar[i] = '0';

    sprintf(fStr[i], "[%03d] %6.6s  %c  %c | ", i,m_sSignalNames[i],ch,fCurchar[i]);


    ii = 0;
    for(j=0; j<TRIGGER_CAP_LEN; j++) //128
    {
      //??? int low = m_iTriggerBuffer_Low[i][j] + m_iTriggerBuffer_TransLow[i][j];
      //??? int high = m_iTriggerBuffer_High[i][j] + m_iTriggerBuffer_TransHigh[i][j];
      //??? int trans = m_iTriggerBuffer_TransHigh[i][j] + m_iTriggerBuffer_TransLow[i][j];

      unsigned int low_color = GetColor(SCOPE_COLOR_DEFAULT, i & 0x1);
      unsigned int high_color = GetColor(SCOPE_COLOR_DEFAULT, i & 0x1);
      unsigned int trans_color = GetColor(SCOPE_COLOR_DEFAULT, i & 0x1);

      // draw actial lines
      if(use_ascii)
	  {
        if(m_iTriggerBuffer[i][j]) strcat(fStr[i],"\257");
        else                       strcat(fStr[i],"_");
	  }
	  else
	  {
		if(j==0) // starting point
		{
          xx[ii] = x0;
          xx[ii+1] = x0 + xstep;
          if(m_iTriggerBuffer[i][j]) {yy[ii] = y0 + 0.9; yy[ii+1] = y0 + 0.9;}
		  else                       {yy[ii] = y0 + 0.1; yy[ii+1] = y0 + 0.1;}
          ii+=2;
		}
		else
		{
          if(m_iTriggerBuffer[i][j-1]==m_iTriggerBuffer[i][j]) // if same as previous, add one point
		  {
            xx[ii] = xx[ii-1] + xstep;
			if(m_iTriggerBuffer[i][j]) yy[ii] = y0 + 0.9; // top 
			else                       yy[ii] = y0 + 0.1; // bottom
			ii++;
		  }
		  else // add 2 point
	      {
            xx[ii] = xx[ii-1];
            xx[ii+1] = xx[ii] + xstep;
            if(m_iTriggerBuffer[i][j-1] && !m_iTriggerBuffer[i][j]) // down&bottom
			{
              yy[ii] = y0 + 0.1;
              yy[ii+1] = y0 + 0.1;
			}
            else // up&top
			{
              yy[ii] = y0 + 0.9;
              yy[ii+1] = y0 + 0.9;
			}
            ii+=2;
		  }
		}

	  }
    }


	if(use_ascii)
	{
	  // update text in entry[i]
      TGLBEntry *entry = fListBox->GetEntry(i);
      if(entry->InheritsFrom(TGTextLBEntry::Class()))
      {
        const char *text;
        text = ((TGTextLBEntry*)entry)->GetText()->GetString();
        //printf("2: [%3d] >%s<\n",i,text);
        ((TGTextLBEntry*)entry)->SetText(new TGString(fStr[i]));
      }
	}
	else
	{
      fText.DrawText(1.,y0+0.5,fStr[i]);
      fPolyLine->DrawPolyLine(ii,xx,yy);
      y0 -= ystep; 
	}

    y++;
  }


  if(use_ascii)
  {
    // update ListBox window
    fListBox->Layout();
  }
  else
  {
    // draw middle line
    xmed[0] = xmed[1] = SCOPE_MIN_X + (Double_t)TRIGGER_CAP_LEN/2.;
	ymed[0] = 256.;
    ymed[1] = 0.;
    fMedian->DrawPolyLine(2,xmed,ymed);

    // draw cursor
    if(xcursor > 0.)
	{
      xcur[0] = xcur[1] = xcursor;
      ycur[0] = 256.;
      ycur[1] = 0.;
      fCursor->DrawPolyLine(2,xcur,ycur);
	}

    Draw();
    fCanvas->SetEditable(FALSE);
    fCanvas->Update();
  }

  //if(m_bTriggerContinuous) printf("Continuous mode !!!\n");
}




// update cursor position and update info on the left
void ScopeDlg::UpdateCursor(int ix, int iy, BOOL updatePosition, BOOL eraseold)
{
  printf("ScopeDlg::UpdateCursor reached\n");fflush(stdout);
  printf("ScopeDlg::UpdateCursor reached, ix=%d, iy=%d\n",ix,iy);

  Double_t xx[2], yy[2];
  if(use_ascii)
  {
    /*do nothing*/;
  }
  else
  {
    //draw cursor in mouse position - must redraw whole picture to remove old cursor
    xcursor = (Double_t)ix;
    char txt[80]; 
    sprintf(txt,"Cursor %d ns",(ix-TRIGGER_CAP_LEN/2-(Int_t)(SCOPE_MIN_X))*5);
    fStatusBar->SetText(txt, 2);
  }

}

// continous mode trigger, sets flag for timer
void ScopeDlg::OnBScopecontinuous() 
{
  unsigned int tmp;

  printf("ScopeDlg::OnBScopecontinuous reached\n");fflush(stdout);

  m_bTriggerContinuous = TRUE;
  if(UpdateTriggerMasks())
  {
    /*if(tcp->m_bConnected)*/
	{
      tmp = 0;
      tcp->Write32(board_address[0] + ECAL_TRIG_STATUS, &tmp);
      if(board_address[1]!=0) tcp->Write32(board_address[1] + ECAL_TRIG_STATUS, &tmp);
      tmp = 1;
      tcp->Write32(board_address[0] + ECAL_TRIG_STATUS, &tmp);
      if(board_address[1]!=0) tcp->Write32(board_address[1] + ECAL_TRIG_STATUS, &tmp);
	}
    fScopeTimer->Start();
  }
}

// same as before, without flag
void ScopeDlg::OnBScopesingle() 
{
  unsigned int tmp;

  printf("ScopeDlg::OnBScopesingle reached\n");fflush(stdout);

  m_bTriggerContinuous = FALSE;
  if(UpdateTriggerMasks())
  {
   /*if(tcpvme->m_bConnected)*/
	{
      tmp = 0;
      tcp->Write32(board_address[0] + ECAL_TRIG_STATUS, &tmp);
      if(board_address[1]!=0) tcp->Write32(board_address[1] + ECAL_TRIG_STATUS, &tmp);
      tmp = 1;
      tcp->Write32(board_address[0] + ECAL_TRIG_STATUS, &tmp);
      if(board_address[1]!=0) tcp->Write32(board_address[1] + ECAL_TRIG_STATUS, &tmp);
    }
    fScopeTimer->Start();
  }
}

// process stop
void ScopeDlg::OnBScopestop()
{
  printf("ScopeDlg::OnBScopestop reached\n");fflush(stdout);

  m_bTriggerContinuous = FALSE;
}

// process clear
void ScopeDlg::OnBScopeclear() 
{
  printf("ScopeDlg::OnBScopeclear reached\n");fflush(stdout);

  memset(m_iTriggerBuffer_Low, 0, sizeof(m_iTriggerBuffer_Low));
  memset(m_iTriggerBuffer_High, 0, sizeof(m_iTriggerBuffer_High));
  memset(m_iTriggerBuffer_TransLow, 0, sizeof(m_iTriggerBuffer_TransLow));
  memset(m_iTriggerBuffer_TransHigh, 0, sizeof(m_iTriggerBuffer_TransHigh));
  memset(m_iTriggerBuffer, 0, sizeof(m_iTriggerBuffer));
  DrawScope();
}

// writes from gui to vme
BOOL ScopeDlg::UpdateTriggerMasks()
{
  unsigned int bit_masks[8], ignore_masks[8];

  printf("ScopeDlg::UpdateTriggerMasks reached\n");fflush(stdout);

  memset(bit_masks, 0, sizeof(bit_masks));
  memset(ignore_masks, 0, sizeof(ignore_masks));
  for(int i=0; i<SCOPE_CHANNELCOUNT; i++)
  {
    int word = i / 32;
    int bit = i % 32;

    if(m_iTriggerPattern[i] == TRIGPATTERN_X) ignore_masks[word] |= 1<<bit;
    else if(m_iTriggerPattern[i] == TRIGPATTERN_1) bit_masks[word] |= 1<<bit;
  }

  /*if(tcp->m_bConnected)*/
  {
    if(!tcp->Write32(board_address[0] + ECAL_TRIG_VALUE7, &bit_masks[7])) return FALSE;
    if(!tcp->Write32(board_address[0] + ECAL_TRIG_VALUE6, &bit_masks[6])) return FALSE;
    if(!tcp->Write32(board_address[0] + ECAL_TRIG_VALUE5, &bit_masks[5])) return FALSE;
    if(!tcp->Write32(board_address[0] + ECAL_TRIG_VALUE4, &bit_masks[4])) return FALSE;
    if(!tcp->Write32(board_address[0] + ECAL_TRIG_VALUE3, &bit_masks[3])) return FALSE;
    if(!tcp->Write32(board_address[0] + ECAL_TRIG_VALUE2, &bit_masks[2])) return FALSE;
    if(!tcp->Write32(board_address[0] + ECAL_TRIG_VALUE1, &bit_masks[1])) return FALSE;
    if(!tcp->Write32(board_address[0] + ECAL_TRIG_VALUE0, &bit_masks[0])) return FALSE;

    if(!tcp->Write32(board_address[0] + ECAL_TRIG_INGORE7, &ignore_masks[7])) return FALSE;
    if(!tcp->Write32(board_address[0] + ECAL_TRIG_INGORE6, &ignore_masks[6])) return FALSE;
    if(!tcp->Write32(board_address[0] + ECAL_TRIG_INGORE5, &ignore_masks[5])) return FALSE;
    if(!tcp->Write32(board_address[0] + ECAL_TRIG_INGORE4, &ignore_masks[4])) return FALSE;
    if(!tcp->Write32(board_address[0] + ECAL_TRIG_INGORE3, &ignore_masks[3])) return FALSE;
    if(!tcp->Write32(board_address[0] + ECAL_TRIG_INGORE2, &ignore_masks[2])) return FALSE;
    if(!tcp->Write32(board_address[0] + ECAL_TRIG_INGORE1, &ignore_masks[1])) return FALSE;
    if(!tcp->Write32(board_address[0] + ECAL_TRIG_INGORE0, &ignore_masks[0])) return FALSE;

    if(board_address[1]!=0) 
	{
      if(!tcp->Write32(board_address[1] + ECAL_TRIG_VALUE7, &bit_masks[7])) return FALSE;
      if(!tcp->Write32(board_address[1] + ECAL_TRIG_VALUE6, &bit_masks[6])) return FALSE;
      if(!tcp->Write32(board_address[1] + ECAL_TRIG_VALUE5, &bit_masks[5])) return FALSE;
      if(!tcp->Write32(board_address[1] + ECAL_TRIG_VALUE4, &bit_masks[4])) return FALSE;
      if(!tcp->Write32(board_address[1] + ECAL_TRIG_VALUE3, &bit_masks[3])) return FALSE;
      if(!tcp->Write32(board_address[1] + ECAL_TRIG_VALUE2, &bit_masks[2])) return FALSE;
      if(!tcp->Write32(board_address[1] + ECAL_TRIG_VALUE1, &bit_masks[1])) return FALSE;
      if(!tcp->Write32(board_address[1] + ECAL_TRIG_VALUE0, &bit_masks[0])) return FALSE;

      if(!tcp->Write32(board_address[1] + ECAL_TRIG_INGORE7, &ignore_masks[7])) return FALSE;
      if(!tcp->Write32(board_address[1] + ECAL_TRIG_INGORE6, &ignore_masks[6])) return FALSE;
      if(!tcp->Write32(board_address[1] + ECAL_TRIG_INGORE5, &ignore_masks[5])) return FALSE;
      if(!tcp->Write32(board_address[1] + ECAL_TRIG_INGORE4, &ignore_masks[4])) return FALSE;
      if(!tcp->Write32(board_address[1] + ECAL_TRIG_INGORE3, &ignore_masks[3])) return FALSE;
      if(!tcp->Write32(board_address[1] + ECAL_TRIG_INGORE2, &ignore_masks[2])) return FALSE;
      if(!tcp->Write32(board_address[1] + ECAL_TRIG_INGORE1, &ignore_masks[1])) return FALSE;
      if(!tcp->Write32(board_address[1] + ECAL_TRIG_INGORE0, &ignore_masks[0])) return FALSE;
	}
  }

  return TRUE;
}

BOOL ScopeDlg::ReadScope(UInt_t addr, UInt_t *buf, Int_t len)
{
  //printf("ScopeDlg::ReadScope reached\n");fflush(stdout);
  //printf("ScopeDlg::ReadScope reached, len=%d\n",len);
  unsigned int val;
  tcp->Read32(addr + 0x2000, &val);
  //printf("'Global' Firmware Revision: V%u.%u\n", (unsigned int)(val>>8), (unsigned int)(val & 0xFF));

  len=len/8;
  //printf("---------------------------- len=%d\n",len); /*128*/


  /* Break into pieces - 5500 CPU otherwise drops connection with larger transfers...*/
  if(!(tcp->Read32(addr + ECAL_TRIG_BUFFER, &buf[len*0], len, CRATE_MSG_FLAGS_NOADRINC))) return FALSE;
  if(!(tcp->Read32(addr + ECAL_TRIG_BUFFER, &buf[len*1], len, CRATE_MSG_FLAGS_NOADRINC))) return FALSE;
  if(!(tcp->Read32(addr + ECAL_TRIG_BUFFER, &buf[len*2], len, CRATE_MSG_FLAGS_NOADRINC))) return FALSE;
  if(!(tcp->Read32(addr + ECAL_TRIG_BUFFER, &buf[len*3], len, CRATE_MSG_FLAGS_NOADRINC))) return FALSE;
  if(!(tcp->Read32(addr + ECAL_TRIG_BUFFER, &buf[len*4], len, CRATE_MSG_FLAGS_NOADRINC))) return FALSE;
  if(!(tcp->Read32(addr + ECAL_TRIG_BUFFER, &buf[len*5], len, CRATE_MSG_FLAGS_NOADRINC))) return FALSE;
  if(!(tcp->Read32(addr + ECAL_TRIG_BUFFER, &buf[len*6], len, CRATE_MSG_FLAGS_NOADRINC))) return FALSE;
  if(!(tcp->Read32(addr + ECAL_TRIG_BUFFER, &buf[len*7], len, CRATE_MSG_FLAGS_NOADRINC))) return FALSE;

  return TRUE;
}

BOOL ScopeDlg::ReadoutScope()
{
  int i,j,k,m,n;
  int i1,i2,j1,j2;

  //printf("ScopeDlg::ReadoutScope reached\n");fflush(stdout);

  memset(ScopeTraces, 0xAA, sizeof(ScopeTraces));
  memset(ScopeTracesTmp, 0xAA, sizeof(ScopeTracesTmp));

  if(!ReadScope(board_address[0], ScopeTraces, SCOPE_SAMPLEDEPTH*SCOPE_CHANNELCOUNT/32)) return FALSE;
  if(board_address[1]!=0)
  {
    if(!ReadScope(board_address[1], ScopeTracesTmp, SCOPE_SAMPLEDEPTH*SCOPE_CHANNELCOUNT/32)) return FALSE;

    /* copy used part of second board into ubused part of the first board */
    i1 = nused[0] / 32;
    i2 = nused[0] % 32;
    j1 = nused[1] / 32;
    j2 = nused[1] % 32;
    printf("NUSED: %d %d -> %d %d,   %d %d\n",nused[0],nused[1],i1,i2,j1,j2); /* NUSED: 142 62 -> 4 14,   1 30 */
    for(i=0; i<SCOPE_SAMPLEDEPTH; i++) /*128*/
    {
      for(k=0; k<(j1+1); k++) /* 'j1+1' 32-bit words from second board */
	  {
        ScopeTraces[i*8+k+(i1+1)] = ScopeTracesTmp[i*8+k];
	  }
	}

  }

  //printf("ScopeDlg::ReadoutScope fills\n");
  {
    for(i=0; i<SCOPE_SAMPLEDEPTH; i++) //128
    {
	  
      /* 'vertical' 256-vector for i-th channel */
      for(k=0; k<8; k++)
	  {
        bits[k] = ScopeTraces[i*8+k];
	  }
	  
      for(j=0; j<SCOPE_CHANNELCOUNT; j++) //256
      {
        unsigned int signal, lastsignal;
        unsigned int bit = j % 32;
        signal = bits[j/32] & (1<<bit);

        if(i) lastsignal = lastbits[j/32] & (1<<bit);
        if(i)
        {
          if(lastsignal && signal)        m_iTriggerBuffer_High[j][i]++;
          else if(!lastsignal && signal)  m_iTriggerBuffer_TransHigh[j][i]++;
          else if(!lastsignal && !signal) m_iTriggerBuffer_Low[j][i]++;
          else                            m_iTriggerBuffer_TransLow[j][i]++;
        }
        else
        {
          if(signal) m_iTriggerBuffer_High[j][i]++;
          else       m_iTriggerBuffer_Low[j][i]++;
        }

        /*ascii version*/
        if(signal) m_iTriggerBuffer[j][i]=1;
        else       m_iTriggerBuffer[j][i]=0;

      }

      memcpy(lastbits, bits, sizeof(bits));
    }
    return TRUE;
  }
  return FALSE;
}
