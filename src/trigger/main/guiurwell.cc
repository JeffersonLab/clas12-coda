
// guiurwell.cxx: gui for URWELL v1495-based trigger

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
#include <TRootCanvas.h>
#include <TText.h>

#include "URWELLTriggerBoardRegs.h"

#include "guiurwell.h"

#include "cratemsgclient.h"
#include "libtcp.h"
#include "libdb.h"

#include "scope.h"

CrateMsgClient *tcp; //sergey: global for now, will find appropriate place later

const char *filetypes[] = { "All files",     "*",
                            "ROOT files",    "*.root",
                            "ROOT macros",   "*.C",
                            0,               0 };

/********************************/
/* MyTimer class implementation */

MyTimer::MyTimer(GUIMainFrame *m, Long_t ms) : TTimer(ms, kTRUE)
{
  fGUIMainFrame = m;
  gSystem->AddTimer(this);
}

Bool_t MyTimer::Notify()
{
  // That function will be called in case of timeout INSTEAD OF
  // standart Notify() function from TTimer class

  //  printf("Timer\n");

  if(fGUIMainFrame->fScalersDlg) fGUIMainFrame->fScalersDlg->ReadVME();
  if(fGUIMainFrame->fDsc2Dlg)    fGUIMainFrame->fDsc2Dlg->ReadVME();
  if(fGUIMainFrame->fDelaysDlg) fGUIMainFrame->fDelaysDlg->ReadVME();
  this->Reset();

  return kTRUE;
}
//------------------------------------------------------------------------------



/**********************************/
/* TileFrame class implementation */

TileFrame::TileFrame(const TGWindow *p) : TGCompositeFrame(p, 10, 10, kHorizontalFrame, GetWhitePixel())
{
   // Create tile view container. Used to show colormap.

   fCanvas = 0;
   SetLayoutManager(new TGTileLayout(this, 8));

   // Handle only buttons 4 and 5 used by the wheel mouse to scroll
   gVirtualX->GrabButton(fId, kButton4, kAnyModifier,
                         kButtonPressMask | kButtonReleaseMask,
                         kNone, kNone);
   gVirtualX->GrabButton(fId, kButton5, kAnyModifier,
                         kButtonPressMask | kButtonReleaseMask,
                         kNone, kNone);
}

Bool_t TileFrame::HandleButton(Event_t *event)
{
   // Handle wheel mouse to scroll.

   Int_t page = 0;
   if (event->fCode == kButton4 || event->fCode == kButton5) {
      if (!fCanvas) return kTRUE;
      if (fCanvas->GetContainer()->GetHeight())
         page = Int_t(Float_t(fCanvas->GetViewPort()->GetHeight() *
                              fCanvas->GetViewPort()->GetHeight()) /
                              fCanvas->GetContainer()->GetHeight());
   }

   if (event->fCode == kButton4) {
      //scroll up
      Int_t newpos = fCanvas->GetVsbPosition() - page;
      if (newpos < 0) newpos = 0;
      fCanvas->SetVsbPosition(newpos);
      return kTRUE;
   }
   if (event->fCode == kButton5) {
      // scroll down
      Int_t newpos = fCanvas->GetVsbPosition() + page;
      fCanvas->SetVsbPosition(newpos);
      return kTRUE;
   }
   return kTRUE;
}



/**************************************/
/* GUIMainFrame class implementation */

GUIMainFrame::GUIMainFrame(const TGWindow *p, UInt_t w, UInt_t h, char *host) : TGMainFrame(p, w, h)
{

   // create VME communication
   strcpy(hostname,host);

   fScalersDlg = NULL;
   fDsc2Dlg = NULL;
   fDelaysDlg = NULL;

   // Create main frame. A TGMainFrame is a top level window.

   // use hierarchical cleaning
   SetCleanup(kDeepCleanup);

   // Create menubar and popup menus. The hint objects are used to place
   // and group the different menu widgets with respect to each other.
   fMenuDock = new TGDockableFrame(this); // create menu dock (upper horizontal bar for 'File' etc)
   AddFrame(fMenuDock, new TGLayoutHints(kLHintsExpandX, 0, 0, 1, 0)); // add menu dock to the main window
   fMenuDock->SetWindowName("GUIURWELL Menu"); // ???

   // create several layouts 
   fMenuBarLayout = new TGLayoutHints(kLHintsTop | kLHintsExpandX);
   fMenuBarItemLayout = new TGLayoutHints(kLHintsTop | kLHintsLeft, 0, 4, 0, 0);
   fMenuBarHelpLayout = new TGLayoutHints(kLHintsTop | kLHintsRight);

   // menu item 'File'
   fMenuFile = new TGPopupMenu(fClient->GetRoot());
   fMenuFile->AddEntry("&Open...", M_FILE_OPEN);
   fMenuFile->AddEntry("&Save", M_FILE_SAVE);
   fMenuFile->AddEntry("S&ave as...", M_FILE_SAVEAS);
   fMenuFile->AddEntry("&Close", -1);
   fMenuFile->AddSeparator();
   fMenuFile->AddEntry("&Print", M_FILE_PRINT);
   fMenuFile->AddEntry("P&rint setup...", M_FILE_PRINTSETUP);
   fMenuFile->AddSeparator();
   fMenuFile->AddEntry("E&xit", M_FILE_EXIT);

   fMenuFile->DisableEntry(M_FILE_SAVEAS);
   fMenuFile->HideEntry(M_FILE_PRINT);

   // menu item 'URWELL'
   fMenuURWELL = new TGPopupMenu(fClient->GetRoot());
   fMenuURWELL->AddLabel("URWELL monitoring and control");
   fMenuURWELL->AddSeparator();
   fMenuURWELL->AddEntry("&Registers", M_REGISTERS);
   fMenuURWELL->AddEntry("&Delays", M_DELAYS);
   fMenuURWELL->AddEntry("&Scalers", M_SCALERS);
   fMenuURWELL->AddEntry("&Dsc2", M_DSC2);
   fMenuURWELL->AddEntry("&Scope (ASCII)", M_SCOPE_ASCII);
   fMenuURWELL->AddEntry("&Scope (Canvas)", M_SCOPE_CANVAS);

   // menu item 'View'
   fMenuView = new TGPopupMenu(gClient->GetRoot());
   fMenuView->AddEntry("&Dock", M_VIEW_DOCK);
   fMenuView->AddEntry("&Undock", M_VIEW_UNDOCK);
   fMenuView->AddSeparator();
   fMenuView->AddEntry("Enable U&ndock", M_VIEW_ENBL_DOCK);
   fMenuView->AddEntry("Enable &Hide", M_VIEW_ENBL_HIDE);
   fMenuView->DisableEntry(M_VIEW_DOCK);

   fMenuDock->EnableUndock(kTRUE);
   fMenuDock->EnableHide(kTRUE);
   fMenuView->CheckEntry(M_VIEW_ENBL_DOCK);
   fMenuView->CheckEntry(M_VIEW_ENBL_HIDE);

   // 'menu item 'Help'
   fMenuHelp = new TGPopupMenu(fClient->GetRoot());
   fMenuHelp->AddEntry("&Contents", M_HELP_CONTENTS);
   fMenuHelp->AddEntry("&Search...", M_HELP_SEARCH);
   fMenuHelp->AddSeparator();
   fMenuHelp->AddEntry("&About", M_HELP_ABOUT);

   // Menu button messages are handled by the main frame (i.e. "this") ProcessMessage() method.
   fMenuFile->Associate(this);
   fMenuURWELL->Associate(this);
   fMenuView->Associate(this);
   fMenuHelp->Associate(this);

   // create menu bar and actually add created above menus to it
   fMenuBar = new TGMenuBar(fMenuDock, 1, 1, kHorizontalFrame);
   fMenuBar->AddPopup("&File", fMenuFile, fMenuBarItemLayout);
   fMenuBar->AddPopup("&URWELL", fMenuURWELL, fMenuBarItemLayout);
   fMenuBar->AddPopup("&View", fMenuView, fMenuBarItemLayout);
   fMenuBar->AddPopup("&Help", fMenuHelp, fMenuBarHelpLayout);

   // add menu bar to the dock
   fMenuDock->AddFrame(fMenuBar, fMenuBarLayout);

   //
   // at that moment we have only top menu bar with menus
   //
   /*
   MapSubwindows();
   Resize();
   MapWindow();
   return;
   */

   
   // Create TGCanvas and a canvas container which uses a tile layout manager
   fCanvasWindow = new TGCanvas(this, 500, 300);
   fContainer = new TileFrame(fCanvasWindow->GetViewPort());
   fContainer->SetCanvas(fCanvasWindow);
   fCanvasWindow->SetContainer(fContainer);
   fContainer->SetCleanup(kDeepCleanup); // use hierarchical cleaning for container
   AddFrame(fCanvasWindow, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 0, 0, 2, 2));
   

   AddFrame(btConnect = new TGTextButton(this, "Connect", 11));
   btConnect->Associate(this);
   btConnect->Resize(90, btConnect->GetDefaultHeight());
   AddFrame(btConnect);

   AddFrame(btDisconnect = new TGTextButton(this, "Disconnect", 12));
   btDisconnect->Associate(this);
   btDisconnect->Resize(90, btDisconnect->GetDefaultHeight());
   btDisconnect->SetEnabled(kFALSE);
   AddFrame(btDisconnect);


   SetWindowName("GUIURWELL");
   MapSubwindows();  // force drawing of all created subwindows; actual drawing is done by MapWindow()

   // we need to use GetDefault...() to initialize the layout algorithm...
   Resize();   // resize to default size
   MapWindow(); // actual window drawing
   Print();

   tt = new MyTimer(this, 1000);
}

GUIMainFrame::~GUIMainFrame()
{
   // Delete all created widgets.

   delete fMenuFile;
   delete fMenuURWELL;
   delete fMenuView;
   delete fMenuHelp;
   delete fContainer;
}

void GUIMainFrame::CloseWindow()
{
   // Got close message for this MainFrame. Terminate the application
   // or returns from the TApplication event loop (depending on the
   // argument specified in TApplication::Run()).

   gApplication->Terminate(0);
}

Bool_t GUIMainFrame::ProcessMessage(Long_t msg, Long_t parm1, Long_t)
{
  Int_t ret;
   // Handle messages send to the GUIMainFrame object. E.g. all menu button
   // messages.

   UInt_t board_addr[2];
   board_addr[0] = URWELL_BOARD_ADDRESS_1;
   board_addr[1] = 0;

   char SignalNames0[SCOPE_CHANNELCOUNT][20];
   char SignalNames1[SCOPE_CHANNELCOUNT][20];
   int i, sig = 0;
   for(i=0; i<12; i++) sprintf(SignalNames0[sig++],"TRIG%02d", i);
   for(i=0; i<ND; i++) sprintf(SignalNames0[sig++],"DS1_%02d", i);
   for(i=0; i<ND; i++) sprintf(SignalNames0[sig++],"DS2_%02d", i);
   for(i=0; i<ND; i++) sprintf(SignalNames0[sig++],"DS3_%02d", i);
   for(i=0; i<ND; i++) sprintf(SignalNames0[sig++],"DS4_%02d", i);
   for(i=0; i<ND; i++) sprintf(SignalNames0[sig++],"DS5_%02d", i);
   for(i=0; i<ND; i++) sprintf(SignalNames0[sig++],"DS6_%02d", i);
   for(i=0; i<ND; i++) sprintf(SignalNames0[sig++],"DS7_%02d", i);
   for(i=0; i<ND; i++) sprintf(SignalNames0[sig++],"DS8_%02d", i);
   while(sig<SCOPE_CHANNELCOUNT) sprintf(SignalNames0[sig++],"unused");
   printf("sig=%d\n",sig);

   switch (GET_MSG(msg)) {

      case kC_COMMAND:
         switch (GET_SUBMSG(msg)) {

            case kCM_BUTTON:
	       //printf("Button was pressed, id = %ld\n", parm1);
			  if(parm1 == 11) // Connect
			  {
				printf("Connect reached\n");fflush(stdout);

                tcp = new CrateMsgClient(hostname,6102);
                if(tcp->IsValid())
                {
                  printf("Connected\n");
				  btConnect->SetEnabled(kFALSE);
				  btDisconnect->SetEnabled(kTRUE);

		unsigned short sval = 0xFBFB;
		ret = tcp->Read16((unsigned int)(URWELL_BOARD_ADDRESS_1+0x800C), &sval);
        printf("ret=%d, VME FIRMWARE val=0x%04x\n",ret,sval);
				  
		unsigned int val = 0xFBFBFBFB;
		ret = tcp->Read32((unsigned int)(URWELL_BOARD_ADDRESS_1+0x1000), &val);
        printf("ret=%d, USER FIRMWARE val=0x%08x\n",ret,val);
                }
                else
                {
                  printf("NOT CONNECTED - EXIT\n");
                  exit(0);
                }

                if(fDelaysDlg)
				{
				  fDelaysDlg->ReadVME();
				  fDelaysDlg->UpdateGUI();
				}
                if(fScalersDlg)
				{
				  fScalersDlg->ReadVME();
				  fScalersDlg->UpdateGUI();
				}
                if(fDsc2Dlg)
				{
				  fDsc2Dlg->ReadVME();
				  fDsc2Dlg->UpdateGUI();
				}

			  }
			  else if(parm1 == 12) // Disconnect
			  {
			    printf("Disconnect reached\n");
                tcp->Disconnect();
			    btConnect->SetEnabled(kTRUE);
			    btDisconnect->SetEnabled(kFALSE);
			  }
              break;

            case kCM_MENUSELECT:
               //printf("Pointer over menu entry, id=%ld\n", parm1);
               break;

            case kCM_MENU:
               switch (parm1) {

                  case M_FILE_OPEN:
                     {
                        static TString dir(".");
                        TGFileInfo fi;
                        fi.fFileTypes = filetypes;
                        fi.fIniDir    = StrDup(dir);
                        new TGFileDialog(fClient->GetRoot(), this, kFDOpen, &fi);
                        printf("Open file: %s (dir: %s)\n", fi.fFilename,
                               fi.fIniDir);
                        dir = fi.fIniDir;
                     }
                     break;

                  case M_FILE_SAVE:
                     printf("M_FILE_SAVE\n");
                     break;

                  case M_FILE_PRINT:
                     printf("M_FILE_PRINT\n");
                     printf("Hiding itself, select \"Print Setup...\" to enable again\n");
                     fMenuFile->HideEntry(M_FILE_PRINT);
                     break;

                  case M_FILE_PRINTSETUP:
                     printf("M_FILE_PRINTSETUP\n");
                     printf("Enabling \"Print\"\n");
                     fMenuFile->EnableEntry(M_FILE_PRINT);
                     break;

                  case M_FILE_EXIT:
                     CloseWindow();   // this also terminates theApp
                     break;

                  case M_REGISTERS:
                     new RegistersDlg(fClient->GetRoot(), this);
                     break;

                  case M_DELAYS:
                     fDelaysDlg = new DelaysDlg(fClient->GetRoot(), this, 600, 300);
                     break;

                  case M_SCALERS:
                     fScalersDlg = new ScalersDlg(fClient->GetRoot(), this, 600, 300);
                     break;

                  case M_DSC2:
                     fDsc2Dlg = new Dsc2Dlg(fClient->GetRoot(), this, 600, 300);
                     break;

                  case M_SCOPE_ASCII:
					fScopeDlg = new ScopeDlg(fClient->GetRoot(), this, 800, 800, board_addr, SignalNames0, SignalNames1, 0);
                     break;

                  case M_SCOPE_CANVAS:
					fScopeDlg = new ScopeDlg(fClient->GetRoot(), this, 800, 800, board_addr, SignalNames0, SignalNames1, 1);
                     break;

                  case M_VIEW_ENBL_DOCK:
                     fMenuDock->EnableUndock(!fMenuDock->EnableUndock());
                     if (fMenuDock->EnableUndock()) {
                        fMenuView->CheckEntry(M_VIEW_ENBL_DOCK);
                        fMenuView->EnableEntry(M_VIEW_UNDOCK);
                     } else {
                        fMenuView->UnCheckEntry(M_VIEW_ENBL_DOCK);
                        fMenuView->DisableEntry(M_VIEW_UNDOCK);
                     }
                     break;

                  case M_VIEW_ENBL_HIDE:
                     fMenuDock->EnableHide(!fMenuDock->EnableHide());
                     if (fMenuDock->EnableHide()) {
                        fMenuView->CheckEntry(M_VIEW_ENBL_HIDE);
                     } else {
                        fMenuView->UnCheckEntry(M_VIEW_ENBL_HIDE);
                     }
                     break;

                  case M_VIEW_DOCK:
                     fMenuDock->DockContainer();
                     fMenuView->EnableEntry(M_VIEW_UNDOCK);
                     fMenuView->DisableEntry(M_VIEW_DOCK);
                     break;

                  case M_VIEW_UNDOCK:
                     fMenuDock->UndockContainer();
                     fMenuView->EnableEntry(M_VIEW_DOCK);
                     fMenuView->DisableEntry(M_VIEW_UNDOCK);
                     break;

                  default:
                     break;
               }
            default:
               break;
         }
      default:
         break;
   }

   if (fMenuDock->IsUndocked()) {
      fMenuView->EnableEntry(M_VIEW_DOCK);
      fMenuView->DisableEntry(M_VIEW_UNDOCK);
   } else {
      fMenuView->EnableEntry(M_VIEW_UNDOCK);
      fMenuView->DisableEntry(M_VIEW_DOCK);
   }

   return kTRUE;
}


/***********************************/
/* ScalersDlg class implementation */

ScalersDlg::ScalersDlg(const TGWindow *p, GUIMainFrame *main,
					   UInt_t w, UInt_t h, UInt_t options) : TGTransientFrame(p, main, w, h, options)
{

  fMain = main; // remember mainframe

   // Create a dialog window. A dialog window pops up with respect to its
   // "main" window.

   // use hierarchical cleani
   SetCleanup(kDeepCleanup);

   fFrame1 = new TGHorizontalFrame(this, 60, 20, kFixedWidth);

   fOkButton = new TGTextButton(fFrame1, "&Ok", 1);
   fOkButton->Associate(this);
   fCancelButton = new TGTextButton(fFrame1, "&Cancel", 2);
   fCancelButton->Associate(this);

   fL1 = new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX,
                           2, 2, 2, 2);
   fL2 = new TGLayoutHints(kLHintsBottom | kLHintsRight, 2, 2, 5, 1);

   fFrame1->AddFrame(fOkButton, fL1);
   fFrame1->AddFrame(fCancelButton, fL1);

   fFrame1->Resize(150, fOkButton->GetDefaultHeight());
   AddFrame(fFrame1, fL2);


   //--------- create Tab widget and some composite frames for Tabs

   fTab = new TGTab(this, 300, 300);
   fL3 = new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5);

   TGCompositeFrame *tf;


   /* scalers */
   tf = fTab->AddTab("Scalers");
   tf->SetLayoutManager(new TGHorizontalLayout(tf));

   fF6 = new TGGroupFrame(tf, "Scalers", kVerticalFrame);
   fF6->SetTitlePos(TGGroupFrame::kRight); // right aligned
   tf->AddFrame(fF6, fL3);

   // 14 columns, n rows
   fF6->SetLayoutManager(new TGMatrixLayout(fF6, 0, 14, 1));

   
   char buff1[100];
   int jj;


   // first board

   for(jj=0; jj<ND; jj++)
   {
     // D1
     sprintf(buff1, "  D1[%2d] ->",jj);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
     tbufD1[jj] = new TGTextBuffer(10); //arg: the number of digits
     tbufD1[jj]->AddText(0,"0");
     tentD1[jj] = new TGTextEntry(fF6, tbufD1[jj]);
     tentD1[jj]->Resize(80, tentD1[jj]->GetDefaultHeight()); // 1st arg: the number of pixels
	 tentD1[jj]->SetEnabled(kFALSE);
     tentD1[jj]->SetFont("-adobe-courier-bold-r-*-*-14-*-*-*-*-*-iso8859-1");
     fF6->AddFrame(tentD1[jj]);
   }

   for(jj=0; jj<ND; jj++)
   {
     // D2
     sprintf(buff1, "  D2[%2d] ->",jj);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
     tbufD2[jj] = new TGTextBuffer(10); //arg: the number of digits
     tbufD2[jj]->AddText(0,"0");
     tentD2[jj] = new TGTextEntry(fF6, tbufD2[jj]);
     tentD2[jj]->Resize(80, tentD2[jj]->GetDefaultHeight()); // 1st arg: the number of pixels
	 tentD2[jj]->SetEnabled(kFALSE);
     tentD2[jj]->SetFont("-adobe-courier-bold-r-*-*-14-*-*-*-*-*-iso8859-1");
     fF6->AddFrame(tentD2[jj]);
   }

   for(jj=0; jj<ND; jj++)
   {
     // D3
     sprintf(buff1, "  D3[%2d] ->",jj);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
     tbufD3[jj] = new TGTextBuffer(10); //arg: the number of digits
     tbufD3[jj]->AddText(0,"0");
     tentD3[jj] = new TGTextEntry(fF6, tbufD3[jj]);
     tentD3[jj]->Resize(80, tentD3[jj]->GetDefaultHeight()); // 1st arg: the number of pixels
	 tentD3[jj]->SetEnabled(kFALSE);
     tentD3[jj]->SetFont("-adobe-courier-bold-r-*-*-14-*-*-*-*-*-iso8859-1");
     fF6->AddFrame(tentD3[jj]);
   }

   for(jj=0; jj<ND; jj++)
   {
     // D4
     sprintf(buff1, "  D4[%2d] ->",jj);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
     tbufD4[jj] = new TGTextBuffer(10); //arg: the number of digits
     tbufD4[jj]->AddText(0,"0");
     tentD4[jj] = new TGTextEntry(fF6, tbufD4[jj]);
     tentD4[jj]->Resize(80, tentD4[jj]->GetDefaultHeight()); // 1st arg: the number of pixels
	 tentD4[jj]->SetEnabled(kFALSE);
     tentD4[jj]->SetFont("-adobe-courier-bold-r-*-*-14-*-*-*-*-*-iso8859-1");
     fF6->AddFrame(tentD4[jj]);
   }

   for(jj=0; jj<ND; jj++)
   {
     // D5
     sprintf(buff1, "  D5[%2d] ->",jj);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
     tbufD5[jj] = new TGTextBuffer(10); //arg: the number of digits
     tbufD5[jj]->AddText(0,"0");
     tentD5[jj] = new TGTextEntry(fF6, tbufD5[jj]);
     tentD5[jj]->Resize(80, tentD5[jj]->GetDefaultHeight()); // 1st arg: the number of pixels
	 tentD5[jj]->SetEnabled(kFALSE);
     tentD5[jj]->SetFont("-adobe-courier-bold-r-*-*-14-*-*-*-*-*-iso8859-1");
     fF6->AddFrame(tentD5[jj]);
   }

   for(jj=0; jj<ND; jj++)
   {
     // D6
     sprintf(buff1, "  D6[%2d] ->",jj);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
     tbufD6[jj] = new TGTextBuffer(10); //arg: the number of digits
     tbufD6[jj]->AddText(0,"0");
     tentD6[jj] = new TGTextEntry(fF6, tbufD6[jj]);
     tentD6[jj]->Resize(80, tentD6[jj]->GetDefaultHeight()); // 1st arg: the number of pixels
	 tentD6[jj]->SetEnabled(kFALSE);
     tentD6[jj]->SetFont("-adobe-courier-bold-r-*-*-14-*-*-*-*-*-iso8859-1");
     fF6->AddFrame(tentD6[jj]);
   }

   for(jj=0; jj<ND; jj++)
   {
     // D7
     sprintf(buff1, "  D7[%2d] ->",jj);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
     tbufD7[jj] = new TGTextBuffer(10); //arg: the number of digits
     tbufD7[jj]->AddText(0,"0");
     tentD7[jj] = new TGTextEntry(fF6, tbufD7[jj]);
     tentD7[jj]->Resize(80, tentD7[jj]->GetDefaultHeight()); // 1st arg: the number of pixels
	 tentD7[jj]->SetEnabled(kFALSE);
     tentD7[jj]->SetFont("-adobe-courier-bold-r-*-*-14-*-*-*-*-*-iso8859-1");
     fF6->AddFrame(tentD7[jj]);
   }

   for(jj=0; jj<ND; jj++)
   {
     // D8
     sprintf(buff1, "  D8[%2d] ->",jj);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
     tbufD8[jj] = new TGTextBuffer(10); //arg: the number of digits
     tbufD8[jj]->AddText(0,"0");
     tentD8[jj] = new TGTextEntry(fF6, tbufD8[jj]);
     tentD8[jj]->Resize(80, tentD8[jj]->GetDefaultHeight()); // 1st arg: the number of pixels
	 tentD8[jj]->SetEnabled(kFALSE);
     tentD8[jj]->SetFont("-adobe-courier-bold-r-*-*-14-*-*-*-*-*-iso8859-1");
     fF6->AddFrame(tentD8[jj]);
   }

   for(jj=0; jj<2; jj++)
   {
     // Trig0/1
     sprintf(buff1, "  Trig[%2d] ->",jj);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
     tbufTrig[jj] = new TGTextBuffer(10); //arg: the number of digits
     tbufTrig[jj]->AddText(0,"0");
     tentTrig[jj] = new TGTextEntry(fF6, tbufTrig[jj]);
     tentTrig[jj]->Resize(80, tentTrig[jj]->GetDefaultHeight()); // 1st arg: the number of pixels
	 tentTrig[jj]->SetEnabled(kFALSE);
     tentTrig[jj]->SetFont("-adobe-courier-bold-r-*-*-14-*-*-*-*-*-iso8859-1");
     fF6->AddFrame(tentTrig[jj]);
   }

   
   fF6->Resize(); // resize to default size



   /* histos */
   fFillHistos = kFALSE;
   fHpxD1 = 0;
   fHpxD2 = 0;
   fHpxD3 = 0;
   fHpxD4 = 0;
   fHpxD5 = 0;
   fHpxD6 = 0;
   fHpxD7 = 0;
   fHpxD8 = 0;

   tf = fTab->AddTab("Histos");
   fF3 = new TGCompositeFrame(tf, 60, 20, kHorizontalFrame);
   fStartB = new TGTextButton(fF3, "Start &Filling Hists", 40);
   fStopB  = new TGTextButton(fF3, "&Stop Filling Hists", 41);
   fStartB->Associate(this);
   fStopB->Associate(this);
   fF3->AddFrame(fStartB, fL3);
   fF3->AddFrame(fStopB, fL3);

   fF5 = new TGCompositeFrame(tf, 60, 60, kHorizontalFrame);

   fL4 = new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX |
                           kLHintsExpandY, 5, 5, 5, 5);
   fEc1 = new TRootEmbeddedCanvas("ec1", fF5, 100, 100);
   fF5->AddFrame(fEc1, fL4);
   fEc2 = new TRootEmbeddedCanvas("ec2", fF5, 100, 100);
   fF5->AddFrame(fEc2, fL4);
   fEc3 = new TRootEmbeddedCanvas("ec3", fF5, 100, 100);
   fF5->AddFrame(fEc3, fL4);
   fEc4 = new TRootEmbeddedCanvas("ec4", fF5, 100, 100);
   fF5->AddFrame(fEc4, fL4);
   fEc5 = new TRootEmbeddedCanvas("ec5", fF5, 100, 100);
   fF5->AddFrame(fEc5, fL4);
   fEc6 = new TRootEmbeddedCanvas("ec6", fF5, 100, 100);
   fF5->AddFrame(fEc6, fL4);
   fEc7 = new TRootEmbeddedCanvas("ec7", fF5, 100, 100);
   fF5->AddFrame(fEc7, fL4);
   fEc8 = new TRootEmbeddedCanvas("ec8", fF5, 100, 100);
   fF5->AddFrame(fEc8, fL4);

   tf->AddFrame(fF3, fL3);
   tf->AddFrame(fF5, fL4);

   fEc1->GetCanvas()->SetBorderMode(0);
   fEc2->GetCanvas()->SetBorderMode(0);
   fEc3->GetCanvas()->SetBorderMode(0);
   fEc4->GetCanvas()->SetBorderMode(0);
   fEc5->GetCanvas()->SetBorderMode(0);
   fEc6->GetCanvas()->SetBorderMode(0);
   fEc7->GetCanvas()->SetBorderMode(0);
   fEc8->GetCanvas()->SetBorderMode(0);

   // make tab yellow
   Pixel_t yellow;
   fClient->GetColorByName("yellow", yellow);
   TGTabElement *tabel = fTab->GetTabTab("Tab 3");;
   /*tabel->ChangeBackground(yellow);*/




   TGLayoutHints *fL5 = new TGLayoutHints(kLHintsBottom | kLHintsExpandX |
                                          kLHintsExpandY, 2, 2, 5, 1);
   AddFrame(fTab, fL5);

   MapSubwindows();
   Resize();   // resize to default size

   // position relative to the parent's window
   CenterOnParent();

   SetWindowName("Dialog");

   MapWindow();
   //fClient->WaitFor(this);    // otherwise canvas contextmenu does not work

   {
     // read VME and update GUI for the first time
     ReadVME();
     UpdateGUI();
   }
}


ScalersDlg::~ScalersDlg()
{
   // Delete ScalersDlg widgets.

}

void ScalersDlg::FillHistos()
{
   // Fill histograms till user clicks "Stop Filling" button.


   if (!fHpxD1) {
      fHpxD1 = new TH1F("hpxD1","D1 scalers",ND,0.,16.);
      fHpxD2 = new TH1F("hpxD2","D2 scalers",ND,0.,16.);
      fHpxD3 = new TH1F("hpxD3","D3 scalers",ND,0.,16.);
      fHpxD4 = new TH1F("hpxD4","D4 scalers",ND,0.,16.);
      fHpxD5 = new TH1F("hpxD5","D5 scalers",ND,0.,16.);
      fHpxD6 = new TH1F("hpxD6","D6 scalers",ND,0.,16.);
      fHpxD7 = new TH1F("hpxD7","D7 scalers",ND,0.,16.);
      fHpxD8 = new TH1F("hpxD8","D8 scalers",ND,0.,16.);
      //fHpxpy = new TH2F("hpxpy","py vs px",40,-4,4,40,-4,4);
      fHpxD1->SetFillColor(kRed);
      fHpxD2->SetFillColor(kRed);
      fHpxD3->SetFillColor(kRed);
      fHpxD4->SetFillColor(kRed);
      fHpxD5->SetFillColor(kRed);
      fHpxD6->SetFillColor(kRed);
      fHpxD7->SetFillColor(kRed);
      fHpxD8->SetFillColor(kRed);
   }

   TCanvas *c1 = fEc1->GetCanvas();
   TCanvas *c2 = fEc2->GetCanvas();
   TCanvas *c3 = fEc3->GetCanvas();
   TCanvas *c4 = fEc4->GetCanvas();
   TCanvas *c5 = fEc5->GetCanvas();
   TCanvas *c6 = fEc6->GetCanvas();
   TCanvas *c7 = fEc7->GetCanvas();
   TCanvas *c8 = fEc8->GetCanvas();

   if(fFillHistos)
   {
	 Int_t jj;
	 Double_t xx,ww;

	 fHpxD1->Reset();
	 fHpxD2->Reset();
	 fHpxD3->Reset();
	 fHpxD4->Reset();
	 fHpxD5->Reset();
	 fHpxD6->Reset();
	 fHpxD7->Reset();
	 fHpxD8->Reset();

     // first board
	 for(jj=0; jj<16; jj++)
	 {
	   xx = (Double_t)jj;
	   ww = (Double_t)D1[jj];
	   fHpxD1->Fill(xx,ww);
	 }
	 for(jj=0; jj<16; jj++)
	 {
	   xx = (Double_t)jj;
	   ww = (Double_t)D2[jj];
	   fHpxD2->Fill(xx,ww);
	 }
	 for(jj=0; jj<16; jj++)
	 {
	   xx = (Double_t)jj;
	   ww = (Double_t)D3[jj];
	   fHpxD3->Fill(xx,ww);
	 }
	 for(jj=0; jj<16; jj++)
	 {
	   xx = (Double_t)jj;
	   ww = (Double_t)D4[jj];
	   fHpxD4->Fill(xx,ww);
	 }
	 for(jj=0; jj<16; jj++)
	 {
	   xx = (Double_t)jj;
	   ww = (Double_t)D5[jj];
	   fHpxD5->Fill(xx,ww);
	 }
	 for(jj=0; jj<16; jj++)
	 {
	   xx = (Double_t)jj;
	   ww = (Double_t)D6[jj];
	   fHpxD6->Fill(xx,ww);
	 }
	 for(jj=0; jj<16; jj++)
	 {
	   xx = (Double_t)jj;
	   ww = (Double_t)D7[jj];
	   fHpxD7->Fill(xx,ww);
	 }
	 for(jj=0; jj<16; jj++)
	 {
	   xx = (Double_t)jj;
	   ww = (Double_t)D7[jj];
	   fHpxD7->Fill(xx,ww);
	 }

	 

     c1->cd();
     fHpxD1->Draw();
     c2->cd();
     fHpxD2->Draw();
     c3->cd();
     fHpxD3->Draw();
     c4->cd();
     fHpxD4->Draw();
     c5->cd();
     fHpxD5->Draw();
     c6->cd();
     fHpxD6->Draw();
     c7->cd();
     fHpxD7->Draw();
     c8->cd();
     fHpxD8->Draw();

     c1->Modified();
     c1->Update();
     c2->Modified();
     c2->Update();
     c3->Modified();
     c3->Update();
     c4->Modified();
     c4->Update();
     c5->Modified();
     c5->Update();
     c6->Modified();
     c6->Update();
     c7->Modified();
     c7->Update();
     c8->Modified();
     c8->Update();
     //gSystem->ProcessEvents();  // handle GUI events
   }
}

void ScalersDlg::CloseWindow()
{
   // Called when window is closed (via the window manager or not).
   // Let's stop histogram filling...
   fFillHistos = kFALSE;
   // Add protection against double-clicks
   fOkButton->SetState(kButtonDisabled);
   fCancelButton->SetState(kButtonDisabled);
   // ... and close the Ged editor if it was activated.
   if (TVirtualPadEditor::GetPadEditor(kFALSE) != 0)
      TVirtualPadEditor::Terminate();
   DeleteWindow();

   fMain->ClearScalersDlg(); // clear pointer to ourself, so FainFrame will stop reading scalers from VME

}


Bool_t ScalersDlg::ProcessMessage(Long_t msg, Long_t parm1, Long_t)
{
   // Process messages coming from widgets associated with the dialog.

   switch (GET_MSG(msg)) {
      case kC_COMMAND:

         switch (GET_SUBMSG(msg)) {
            case kCM_BUTTON:
               switch(parm1) {
                  case 1:
                  case 2:
                     printf("\nTerminating dialog: %s pressed\n",
                            (parm1 == 1) ? "OK" : "Cancel");
                     CloseWindow();
                     break;
                  case 40:  // start histogram filling
                     fFillHistos = kTRUE;
                     //FillHistos();
                     break;
                  case 41:  // stop histogram filling
                     fFillHistos = kFALSE;
                     break;

                 default:
                     break;
               }
               break;
			   /*
            case kCM_RADIOBUTTON:
               switch (parm1) {
                  case 81:
                     fRad2->SetState(kButtonUp);
                     break;
                  case 82:
                     fRad1->SetState(kButtonUp);
                     break;
               }
               break;
			   */
			   /*
            case kCM_CHECKBUTTON:
               switch (parm1) {
                  case 92:
                     fListBox->SetMultipleSelections(fCheckMulti->GetState());
                     break;
                  default:
                     break;
               }
               break;
			   */
            case kCM_TAB:
               printf("Tab item %ld activated\n", parm1);
			   //if(parm1==5)
			   //{
               //  printf("Scalers !!!\n");
			   //}
               break;
            default:
               break;
         }
         break;

      default:
         break;
   }
   return kTRUE;
}

void ScalersDlg::ReadVME()
{

  {
    Int_t jj;
    UInt_t tmp;
    for(jj=0; jj<ND; jj++) D1[jj] = 0;
    for(jj=0; jj<ND; jj++) D2[jj] = 0;
    for(jj=0; jj<ND; jj++) D3[jj] = 0;
    for(jj=0; jj<ND; jj++) D4[jj] = 0;
    for(jj=0; jj<ND; jj++) D5[jj] = 0;
    for(jj=0; jj<ND; jj++) D6[jj] = 0;
    for(jj=0; jj<ND; jj++) D7[jj] = 0;
    for(jj=0; jj<ND; jj++) D8[jj] = 0;
    for(jj=0; jj<2; jj++) TRIG[jj] = 0;

    tmp = 0x0000;
    tcp->Write32(URWELL_BOARD_ADDRESS_1 + URWELL_ENABLE_SCALERS, &tmp);

    /*read trigger scalers*/
    tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_TRIG0_SCALER, &TRIG[0]);
    tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_TRIG1_SCALER, &TRIG[1]);

    /*read channel scalers*/
    for(jj=0; jj<ND; jj++) tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_D1_SCALER_BASE + jj*4, &D1[jj]);
    for(jj=0; jj<ND; jj++) tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_D2_SCALER_BASE + jj*4, &D2[jj]);
    //for(jj=0; jj<ND; jj++) tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_D3_SCALER_BASE + jj*4, &D3[jj]);	
    //for(jj=0; jj<ND; jj++) tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_D4_SCALER_BASE + jj*4, &D4[jj]);	
    //for(jj=0; jj<ND; jj++) tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_D5_SCALER_BASE + jj*4, &D5[jj]);	
    //for(jj=0; jj<ND; jj++) tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_D6_SCALER_BASE + jj*4, &D6[jj]);	
    //for(jj=0; jj<ND; jj++) tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_D7_SCALER_BASE + jj*4, &D7[jj]);	
    //for(jj=0; jj<ND; jj++) tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_D8_SCALER_BASE + jj*4, &D8[jj]);	

    tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_REF_SCALER, &REF1);

    //printf("REF1=%d\n",REF1);
    //for(jj=0; jj<2; jj++) printf("TRIG[%2d]=%d\n",jj,TRIG[jj]);
    //for(jj=0; jj<ND; jj++) printf("D1[%2d]=%d\n",jj,D1[jj]);
    //for(jj=0; jj<ND; jj++) printf("D2[%2d]=%d\n",jj,D2[jj]);
    //for(jj=0; jj<ND; jj++) printf("D3[%2d]=%d\n",jj,D3[jj]);
    //for(jj=0; jj<ND; jj++) printf("D4[%2d]=%d\n",jj,D4[jj]);
    //for(jj=0; jj<ND; jj++) printf("D5[%2d]=%d\n",jj,D5[jj]);
    //for(jj=0; jj<ND; jj++) printf("D6[%2d]=%d\n",jj,D6[jj]);
    //for(jj=0; jj<ND; jj++) printf("D7[%2d]=%d\n",jj,D7[jj]);
    //for(jj=0; jj<ND; jj++) printf("D8[%2d]=%d\n",jj,D8[jj]);

    tmp = 0x0001;
    tcp->Write32(URWELL_BOARD_ADDRESS_1 + URWELL_ENABLE_SCALERS, &tmp);

	// normalize
    if(REF1>0)
    {
      Float_t norm = 40000000./((Float_t)REF1);
      for(jj=0; jj<ND; jj++) D1[jj] = (Int_t)(((Float_t)D1[jj])*norm);
      for(jj=0; jj<ND; jj++) D2[jj] = (Int_t)(((Float_t)D2[jj])*norm);
      for(jj=0; jj<ND; jj++) D3[jj] = (Int_t)(((Float_t)D3[jj])*norm);
      for(jj=0; jj<ND; jj++) D4[jj] = (Int_t)(((Float_t)D4[jj])*norm);
      for(jj=0; jj<ND; jj++) D5[jj] = (Int_t)(((Float_t)D5[jj])*norm);
      for(jj=0; jj<ND; jj++) D6[jj] = (Int_t)(((Float_t)D6[jj])*norm);
      for(jj=0; jj<ND; jj++) D7[jj] = (Int_t)(((Float_t)D7[jj])*norm);
      for(jj=0; jj<ND; jj++) D8[jj] = (Int_t)(((Float_t)D8[jj])*norm);
      TRIG[0] = (Int_t)(((Float_t)TRIG[0])*norm);
      TRIG[1] = (Int_t)(((Float_t)TRIG[1])*norm);
    }


    //printf("AAA1\n");fflush(stdout);
    UpdateGUI();
    //printf("AAA2\n");fflush(stdout);
    FillHistos();
    //printf("AAA3\n");fflush(stdout);
	
  }

}

void ScalersDlg::UpdateGUI()
{
  //printf("ScalersDlg::UpdateGUI reached\n");
   Int_t jj;
   Char_t str[10];
   
   for(jj=0; jj<ND; jj++)
   {
     sprintf(str,"%8d",D1[jj]);
     tentD1[jj]->SetText(str);
   }
   for(jj=0; jj<ND; jj++)
   {
     sprintf(str,"%8d",D2[jj]);
     tentD2[jj]->SetText(str);
   }
   for(jj=0; jj<ND; jj++)
   {
     sprintf(str,"%8d",D3[jj]);
     tentD3[jj]->SetText(str);
   }
   for(jj=0; jj<ND; jj++)
   {
     sprintf(str,"%8d",D4[jj]);
     tentD4[jj]->SetText(str);
   }
   for(jj=0; jj<ND; jj++)
   {
     sprintf(str,"%8d",D5[jj]);
     tentD5[jj]->SetText(str);
   }
   for(jj=0; jj<ND; jj++)
   {
     sprintf(str,"%8d",D6[jj]);
     tentD6[jj]->SetText(str);
   }
   for(jj=0; jj<ND; jj++)
   {
     sprintf(str,"%8d",D7[jj]);
     tentD7[jj]->SetText(str);
   }
   for(jj=0; jj<ND; jj++)
   {
     sprintf(str,"%8d",D8[jj]);
     tentD8[jj]->SetText(str);
   }
   for(jj=0; jj<2; jj++)
   {
     sprintf(str,"%8d",TRIG[jj]);
     tentTrig[jj]->SetText(str);
   }
}







/***********************************/
/* Dsc2Dlg class implementation */

Dsc2Dlg::Dsc2Dlg(const TGWindow *p, GUIMainFrame *main,
					   UInt_t w, UInt_t h, UInt_t options) : TGTransientFrame(p, main, w, h, options)
{

  fMain = main; // remember mainframe

   // Create a dialog window. A dialog window pops up with respect to its
   // "main" window.

   // use hierarchical cleani
   SetCleanup(kDeepCleanup);

   fFrame1 = new TGHorizontalFrame(this, 60, 20, kFixedWidth);

   fOkButton = new TGTextButton(fFrame1, "&Ok", 1);
   fOkButton->Associate(this);
   fCancelButton = new TGTextButton(fFrame1, "&Cancel", 2);
   fCancelButton->Associate(this);

   fL1 = new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX,
                           2, 2, 2, 2);
   fL2 = new TGLayoutHints(kLHintsBottom | kLHintsRight, 2, 2, 5, 1);

   fFrame1->AddFrame(fOkButton, fL1);
   fFrame1->AddFrame(fCancelButton, fL1);

   fFrame1->Resize(150, fOkButton->GetDefaultHeight());
   AddFrame(fFrame1, fL2);

   //--------- create Tab widget and some composite frames for Tabs

   fTab = new TGTab(this, 300, 300);
   fL3 = new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5);

   TGCompositeFrame *tf;


   /* scalers */
   tf = fTab->AddTab("Scalers");
   tf->SetLayoutManager(new TGHorizontalLayout(tf));

   fF6 = new TGGroupFrame(tf, "Scalers", kVerticalFrame);
   fF6->SetTitlePos(TGGroupFrame::kRight); // right aligned
   tf->AddFrame(fF6, fL3);

   // 12 columns, n rows
   fF6->SetLayoutManager(new TGMatrixLayout(fF6, 0, 12, 1));
   char buff1[100];
   int jj;
   for(jj=0; jj<ND; jj++)
   {
     // D1
     sprintf(buff1, "    D1[%2d] ->", jj);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
     tbufD1[jj] = new TGTextBuffer(10); //arg: the number of digits
     tbufD1[jj]->AddText(0,"0");
     tentD1[jj] = new TGTextEntry(fF6, tbufD1[jj]);
     tentD1[jj]->Resize(80, tentD1[jj]->GetDefaultHeight()); // 1st arg: the number of pixels
	 tentD1[jj]->SetEnabled(kFALSE);
     tentD1[jj]->SetFont("-adobe-courier-bold-r-*-*-14-*-*-*-*-*-iso8859-1");
     fF6->AddFrame(tentD1[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
     // D2
     sprintf(buff1, "    D2[%2d] ->", jj);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
     tbufD2[jj] = new TGTextBuffer(10);
     tbufD2[jj]->AddText(0,"0");
     tentD2[jj] = new TGTextEntry(fF6, tbufD2[jj]);
     tentD2[jj]->Resize(80, tentD2[jj]->GetDefaultHeight());
	 tentD2[jj]->SetEnabled(kFALSE);
     tentD2[jj]->SetFont("-adobe-courier-bold-r-*-*-14-*-*-*-*-*-iso8859-1");
     fF6->AddFrame(tentD2[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
     // D3
     sprintf(buff1, "    D3[%2d] ->", jj);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
     tbufD3[jj] = new TGTextBuffer(10);
     tbufD3[jj]->AddText(0,"0");
     tentD3[jj] = new TGTextEntry(fF6, tbufD3[jj]);
     tentD3[jj]->Resize(80, tentD3[jj]->GetDefaultHeight());
     tentD3[jj]->SetFont("-adobe-courier-bold-r-*-*-14-*-*-*-*-*-iso8859-1");
	 tentD3[jj]->SetEnabled(kFALSE);
     fF6->AddFrame(tentD3[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
     // D4
     sprintf(buff1, "    D4[%2d] ->", jj);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
     tbufD4[jj] = new TGTextBuffer(10);
     tbufD4[jj]->AddText(0,"0");
     tentD4[jj] = new TGTextEntry(fF6, tbufD4[jj]);
     tentD4[jj]->Resize(80, tentD4[jj]->GetDefaultHeight());
     tentD4[jj]->SetFont("-adobe-courier-bold-r-*-*-14-*-*-*-*-*-iso8859-1");
	 tentD4[jj]->SetEnabled(kFALSE);
     fF6->AddFrame(tentD4[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
     // D5
     sprintf(buff1, "    D5[%2d] ->", jj);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
     tbufD5[jj] = new TGTextBuffer(10);
     tbufD5[jj]->AddText(0,"0");
     tentD5[jj] = new TGTextEntry(fF6, tbufD5[jj]);
     tentD5[jj]->Resize(80, tentD5[jj]->GetDefaultHeight());
     tentD5[jj]->SetFont("-adobe-courier-bold-r-*-*-14-*-*-*-*-*-iso8859-1");
	 tentD5[jj]->SetEnabled(kFALSE);
     fF6->AddFrame(tentD5[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
     // D6
     sprintf(buff1, "    D6[%2d] ->", jj);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
     tbufD6[jj] = new TGTextBuffer(10);
     tbufD6[jj]->AddText(0,"0");
     tentD6[jj] = new TGTextEntry(fF6, tbufD6[jj]);
     tentD6[jj]->Resize(80, tentD6[jj]->GetDefaultHeight());
     tentD6[jj]->SetFont("-adobe-courier-bold-r-*-*-14-*-*-*-*-*-iso8859-1");
	 tentD6[jj]->SetEnabled(kFALSE);
     fF6->AddFrame(tentD6[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
     // D7
     sprintf(buff1, "    D7[%2d] ->", jj);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
     tbufD7[jj] = new TGTextBuffer(10);
     tbufD7[jj]->AddText(0,"0");
     tentD7[jj] = new TGTextEntry(fF6, tbufD7[jj]);
     tentD7[jj]->Resize(80, tentD7[jj]->GetDefaultHeight());
     tentD7[jj]->SetFont("-adobe-courier-bold-r-*-*-14-*-*-*-*-*-iso8859-1");
	 tentD7[jj]->SetEnabled(kFALSE);
     fF6->AddFrame(tentD7[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
     // D8
     sprintf(buff1, "    D8[%2d] ->", jj);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
     tbufD8[jj] = new TGTextBuffer(10);
     tbufD8[jj]->AddText(0,"0");
     tentD8[jj] = new TGTextEntry(fF6, tbufD8[jj]);
     tentD8[jj]->Resize(80, tentD8[jj]->GetDefaultHeight());
     tentD8[jj]->SetFont("-adobe-courier-bold-r-*-*-14-*-*-*-*-*-iso8859-1");
	 tentD8[jj]->SetEnabled(kFALSE);
     fF6->AddFrame(tentD8[jj]);
   }
   
   fF6->Resize(); // resize to default size




   /* histos */

   fFillHistos = kFALSE;
   fHpxD1 = 0;
   fHpxD2 = 0;
   fHpxD3 = 0;
   fHpxD4 = 0;
   fHpxD5 = 0;
   fHpxD6 = 0;
   fHpxD7 = 0;
   fHpxD8 = 0;

   tf = fTab->AddTab("Histos");
   fF3 = new TGCompositeFrame(tf, 60, 20, kHorizontalFrame);
   fStartB = new TGTextButton(fF3, "Start &Filling Hists", 40);
   fStopB  = new TGTextButton(fF3, "&Stop Filling Hists", 41);
   fStartB->Associate(this);
   fStopB->Associate(this);
   fF3->AddFrame(fStartB, fL3);
   fF3->AddFrame(fStopB, fL3);
   /*
   fF3->AddFrame(fRad1 = new TGRadioButton(fF3, "&Radio 1", 81), fL3);
   fRad1->Associate(this);
   */
   fF3->AddFrame(fChk1 = new TGCheckButton(fF3, "A&ccumulate", 71), fL3);
   fChk1->Associate(this);

   fF5 = new TGCompositeFrame(tf, 60, 60, kHorizontalFrame);

   //fF5->SetLayoutManager(new TGMatrixLayout(fF5, 0, 3, 2));

   fL4 = new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5);
   //fL4 = new TGLayoutHints(kLHintsExpandX | kLHintsExpandY);

   fEc1 = new TRootEmbeddedCanvas("ec1", fF5, 100, 100);
   fF5->AddFrame(fEc1, fL4);
   fEc2 = new TRootEmbeddedCanvas("ec2", fF5, 100, 100);
   fF5->AddFrame(fEc2, fL4);
   fEc3 = new TRootEmbeddedCanvas("ec3", fF5, 100, 100);
   fF5->AddFrame(fEc3, fL4);
   fEc4 = new TRootEmbeddedCanvas("ec4", fF5, 100, 100);
   fF5->AddFrame(fEc4, fL4);
   fEc5 = new TRootEmbeddedCanvas("ec5", fF5, 100, 100);
   fF5->AddFrame(fEc5, fL4);
   fEc6 = new TRootEmbeddedCanvas("ec6", fF5, 100, 100);
   fF5->AddFrame(fEc6, fL4);
   fEc7 = new TRootEmbeddedCanvas("ec7", fF5, 100, 100);
   fF5->AddFrame(fEc7, fL4);
   fEc8 = new TRootEmbeddedCanvas("ec8", fF5, 100, 100);
   fF5->AddFrame(fEc8, fL4);

   tf->AddFrame(fF3, fL3);
   tf->AddFrame(fF5, fL4);


   fEc1->GetCanvas()->SetBorderMode(0);
   fEc2->GetCanvas()->SetBorderMode(0);
   fEc3->GetCanvas()->SetBorderMode(0);
   fEc4->GetCanvas()->SetBorderMode(0);
   fEc5->GetCanvas()->SetBorderMode(0);
   fEc6->GetCanvas()->SetBorderMode(0);
   fEc7->GetCanvas()->SetBorderMode(0);
   fEc8->GetCanvas()->SetBorderMode(0);

   // make tab yellow
   Pixel_t yellow;
   fClient->GetColorByName("yellow", yellow);
   TGTabElement *tabel = fTab->GetTabTab("Tab 3");;
   /*tabel->ChangeBackground(yellow);*/




   TGLayoutHints *fL5 = new TGLayoutHints(kLHintsBottom | kLHintsExpandX | kLHintsExpandY, 2, 2, 5, 1);
   AddFrame(fTab, fL5);

   MapSubwindows();
   Resize();   // resize to default size

   // position relative to the parent's window
   CenterOnParent();

   SetWindowName("Dialog");

   MapWindow();
   //fClient->WaitFor(this);    // otherwise canvas contextmenu does not work

   HistAccumulate = 0;
   {
     // read VME and update GUI for the first time
     ReadVME();
     UpdateGUI();
   }
}


Dsc2Dlg::~Dsc2Dlg()
{
   // Delete Dsc2Dlg widgets.

}

void Dsc2Dlg::FillHistos()
{
   // Fill histograms till user clicks "Stop Filling" button.

   if (!fHpxD1)
   {
	 fHpxD1 = new TH1F("hpxU","D1 scalers",ND,0.,(float)ND);
     fHpxD2 = new TH1F("hpxV","D2 scalers",ND,0.,(float)ND);
     fHpxD3 = new TH1F("hpxW","D3 scalers",ND,0.,(float)ND);
     fHpxD4 = new TH1F("hpxW","D4 scalers",ND,0.,(float)ND);
     fHpxD5 = new TH1F("hpxW","D5 scalers",ND,0.,(float)ND);
     fHpxD6 = new TH1F("hpxW","D6 scalers",ND,0.,(float)ND);
     fHpxD7 = new TH1F("hpxW","D7 scalers",ND,0.,(float)ND);
     fHpxD8 = new TH1F("hpxW","D8 scalers",ND,0.,(float)ND);

     fHpxD1->SetFillColor(kRed);
     fHpxD2->SetFillColor(kRed);
     fHpxD3->SetFillColor(kRed);
     fHpxD4->SetFillColor(kRed);
     fHpxD5->SetFillColor(kRed);
     fHpxD6->SetFillColor(kRed);
     fHpxD7->SetFillColor(kRed);
     fHpxD8->SetFillColor(kRed);
   }

   TCanvas *c1 = fEc1->GetCanvas();
   TCanvas *c2 = fEc2->GetCanvas();
   TCanvas *c3 = fEc3->GetCanvas();
   TCanvas *c4 = fEc4->GetCanvas();
   TCanvas *c5 = fEc5->GetCanvas();
   TCanvas *c6 = fEc6->GetCanvas();
   TCanvas *c7 = fEc7->GetCanvas();
   TCanvas *c8 = fEc8->GetCanvas();

   if(fFillHistos)
   {
	 Int_t jj;
	 Double_t xx,ww;

	 if(!HistAccumulate)
	 {
	   fHpxD1->Reset();
	   fHpxD2->Reset();
	   fHpxD3->Reset();
	   fHpxD4->Reset();
	   fHpxD5->Reset();
	   fHpxD6->Reset();
	   fHpxD7->Reset();
	   fHpxD8->Reset();
	 }

	 for(jj=0; jj<ND; jj++)
	 {
	   xx = (Double_t)jj;
	   ww = (Double_t)D1[jj];
	   fHpxD1->Fill(xx,ww);
	 }

	 for(jj=0; jj<ND; jj++)
	 {
	   xx = (Double_t)jj;
	   ww = (Double_t)D2[jj];
	   fHpxD2->Fill(xx,ww);
	 }

	 for(jj=0; jj<ND; jj++)
	 {
	   xx = (Double_t)jj;
	   ww = (Double_t)D3[jj];
	   fHpxD3->Fill(xx,ww);
	 }

	 for(jj=0; jj<ND; jj++)
	 {
	   xx = (Double_t)jj;
	   ww = (Double_t)D4[jj];
	   fHpxD4->Fill(xx,ww);
	 }

	 for(jj=0; jj<ND; jj++)
	 {
	   xx = (Double_t)jj;
	   ww = (Double_t)D5[jj];
	   fHpxD5->Fill(xx,ww);
	 }

	 for(jj=0; jj<ND; jj++)
	 {
	   xx = (Double_t)jj;
	   ww = (Double_t)D6[jj];
	   fHpxD6->Fill(xx,ww);
	 }

	 for(jj=0; jj<ND; jj++)
	 {
	   xx = (Double_t)jj;
	   ww = (Double_t)D7[jj];
	   fHpxD7->Fill(xx,ww);
	 }

	 for(jj=0; jj<ND; jj++)
	 {
	   xx = (Double_t)jj;
	   ww = (Double_t)D8[jj];
	   fHpxD8->Fill(xx,ww);
	 }
	 

     c1->cd();
     fHpxD1->Draw();
     c2->cd();
     fHpxD2->Draw();
     c3->cd();
     fHpxD3->Draw();
     c4->cd();
     fHpxD4->Draw();
     c5->cd();
     fHpxD5->Draw();
     c6->cd();
     fHpxD6->Draw();
     c7->cd();
     fHpxD7->Draw();
     c8->cd();
     fHpxD8->Draw();

     c1->Modified();
     c1->Update();
     c2->Modified();
     c2->Update();
     c3->Modified();
     c3->Update();
     c4->Modified();
     c4->Update();
     c5->Modified();
     c5->Update();
     c6->Modified();
     c6->Update();
     c7->Modified();
     c7->Update();
     c8->Modified();
     c8->Update();
     //gSystem->ProcessEvents();  // handle GUI events
   }
}

void Dsc2Dlg::CloseWindow()
{
   // Called when window is closed (via the window manager or not).
   // Let's stop histogram filling...
   fFillHistos = kFALSE;
   // Add protection against double-clicks
   fOkButton->SetState(kButtonDisabled);
   fCancelButton->SetState(kButtonDisabled);
   // ... and close the Ged editor if it was activated.
   if (TVirtualPadEditor::GetPadEditor(kFALSE) != 0)
      TVirtualPadEditor::Terminate();
   DeleteWindow();

   fMain->ClearDsc2Dlg(); // clear pointer to ourself, so FainFrame will stop reading scalers from VME

}


Bool_t Dsc2Dlg::ProcessMessage(Long_t msg, Long_t parm1, Long_t)
{
  //Int_t status;

   // Process messages coming from widgets associated with the dialog.

   switch (GET_MSG(msg)) {
      case kC_COMMAND:

         switch (GET_SUBMSG(msg)) {
            case kCM_BUTTON:
               switch(parm1) {
                  case 1:
                  case 2:
                     printf("\nTerminating dialog: %s pressed\n",
                            (parm1 == 1) ? "OK" : "Cancel");
                     CloseWindow();
                     break;
                  case 40:  // start histogram filling
                     fFillHistos = kTRUE;
                     //FillHistos();
                     break;
                  case 41:  // stop histogram filling
                     fFillHistos = kFALSE;
                     break;

                 default:
                     break;
               }
               break;
			   /*
            case kCM_RADIOBUTTON:
               switch (parm1) {
                  case 81:
					 printf("RADIOBUTTON 81: %d\n",fRad1->GetState());
                     if(fRad1->GetState()) fRad1->SetState(kButtonUp);
                     else fRad1->SetState(kButtonDown);
                     break;
                  case 82:
                     fRad1->SetState(kButtonUp);
                     break; 
               }
               break;
			   */
			   
            case kCM_CHECKBUTTON:
               switch (parm1) {
                  case 71:
					 printf("CHECKBUTTON 71\n");
                     HistAccumulate = fChk1->GetState();
					 printf("CHECKBUTTON 71: status=%d\n",HistAccumulate);
                     
                     /*fListBox->SetMultipleSelections(fCheck1->GetState());*/
                     break;

                  default:
                     break;
               }
               break;
			   
            case kCM_TAB:
               printf("Tab item %ld activated\n", parm1);
			   //if(parm1==5)
			   //{
               //  printf("Scalers !!!\n");
			   //}
               break;
            default:
               break;
         }
         break;

      default:
         break;
   }
   return kTRUE;
}

#define NDSC 2

void Dsc2Dlg::ReadVME()
{
  Int_t ii, jj, ndsc;
  UInt_t tmp, addr[NDSC];
  static int ref_old[2];


  ndsc = NDSC;

  addr[0]  = URWELL_DSC2_ADDRESS_SLOT5;
  addr[1]  = URWELL_DSC2_ADDRESS_SLOT7;

  //addr[2]  = URWELL_DSC2_ADDRESS_SLOT14;
  //addr[3]  = URWELL_DSC2_ADDRESS_SLOT15;
  //addr[4]  = URWELL_DSC2_ADDRESS_SLOT17;
  //addr[5]  = URWELL_DSC2_ADDRESS_SLOT18;
  //addr[6]  = URWELL_DSC2_ADDRESS_SLOT19;
  //addr[7]  = URWELL_DSC2_ADDRESS_SLOT20;

  {
    /* should do in dsc2Config, here just in case; will move it to constructor, should be called once */
    tmp = 0x0004;
    for(ii=0; ii<NDSC; ii++) tcp->Write32(addr[ii] + URWELL_DSC2_SCALER_GATE, &tmp);

    tmp = 0x0000;
    for(ii=0; ii<NDSC; ii++) tcp->Write32(addr[ii] + URWELL_DSC2_SCALER_LATCH, &tmp);

    for(jj=0; jj<ND; jj++) D1[jj] = 0;
    for(jj=0; jj<ND; jj++) D2[jj] = 0;
    //for(jj=0; jj<ND; jj++) D3[jj] = 0;
    //for(jj=0; jj<ND; jj++) D4[jj] = 0;
    //for(jj=0; jj<ND; jj++) D5[jj] = 0;
    //for(jj=0; jj<ND; jj++) D6[jj] = 0;
    //for(jj=0; jj<ND; jj++) D7[jj] = 0;
    //for(jj=0; jj<ND; jj++) D8[jj] = 0;


    for(ii=0; ii<NDSC; ii++) tcp->Read32(addr[ii] + URWELL_DSC2_SCALER_REF, &ref[ii]);


    for(jj=0; jj<ND; jj++) {tcp->Read32(addr[0] + URWELL_DSC2_SCALER_BASE + jj*4,     &D1[jj]); refD1[jj]    = ref[0];}
    for(jj=0; jj<ND; jj++) {tcp->Read32(addr[1] + URWELL_DSC2_SCALER_BASE + jj*4,     &D2[jj]); refD2[jj]    = ref[1];}
    //for(jj=0; jj<ND; jj++) {tcp->Read32(addr[2] + URWELL_DSC2_SCALER_BASE + jj*4,     &D3[jj]); refD3[jj]    = ref[2];}
    //for(jj=0; jj<ND; jj++) {tcp->Read32(addr[3] + URWELL_DSC2_SCALER_BASE + jj*4,     &D4[jj]); refD4[jj]    = ref[3];}
    //for(jj=0; jj<ND; jj++) {tcp->Read32(addr[4] + URWELL_DSC2_SCALER_BASE + jj*4,     &D5[jj]); refD5[jj]    = ref[4];}
    //for(jj=0; jj<ND; jj++) {tcp->Read32(addr[5] + URWELL_DSC2_SCALER_BASE + jj*4,     &D6[jj]); refD6[jj]    = ref[5];}
    //for(jj=0; jj<ND; jj++) {tcp->Read32(addr[6] + URWELL_DSC2_SCALER_BASE + jj*4,     &D7[jj]); refD7[jj]    = ref[6];}
    //for(jj=0; jj<ND; jj++) {tcp->Read32(addr[7] + URWELL_DSC2_SCALER_BASE + jj*4,     &D8[jj]); refD8[jj]    = ref[7];}

	
    printf("Scalers: ref[0]=%d, ref[1]=%d, D1=%d %d %d (address=0x%08x)\n",
      ref[0],ref[1],D1[0],D1[1],D1[2],addr[0] + URWELL_DSC2_SCALER_BASE);
    printf("\n REF[0]=%d, REF[1]=%d\n\n",ref[0]-ref_old[0],ref[1]-ref_old[1]);
    ref_old[0] = ref[0];
    ref_old[1] = ref[1];

    // normalize
    if(ref[0]>0)
    {
      Float_t norm;
      /*ERROR: 125000000 is correct only if DSC2_SCALER_REFPRESCALE=1 !!! ???*/
      /* must use (125000000/DSC2_SCALER_REFPRESCALE) !!!???*/
      for(jj=0; jj<ND; jj++) norm = 125000000./((Float_t)refD1[jj]); D1[jj] = (Int_t)(((Float_t)D1[jj])*norm);
      for(jj=0; jj<ND; jj++) norm = 125000000./((Float_t)refD2[jj]); D2[jj] = (Int_t)(((Float_t)D2[jj])*norm);
      //for(jj=0; jj<ND; jj++) norm = 125000000./((Float_t)refD3[jj]); D3[jj] = (Int_t)(((Float_t)D3[jj])*norm);
      //for(jj=0; jj<ND; jj++) norm = 125000000./((Float_t)refD4[jj]); D4[jj] = (Int_t)(((Float_t)D4[jj])*norm);
      //for(jj=0; jj<ND; jj++) norm = 125000000./((Float_t)refD5[jj]); D5[jj] = (Int_t)(((Float_t)D5[jj])*norm);
      //for(jj=0; jj<ND; jj++) norm = 125000000./((Float_t)refD6[jj]); D6[jj] = (Int_t)(((Float_t)D6[jj])*norm);
      //for(jj=0; jj<ND; jj++) norm = 125000000./((Float_t)refD7[jj]); D7[jj] = (Int_t)(((Float_t)D7[jj])*norm);
      //for(jj=0; jj<ND; jj++) norm = 125000000./((Float_t)refD8[jj]); D8[jj] = (Int_t)(((Float_t)D8[jj])*norm);
      printf("Scalers(norm=%f): ref=%d, D1=%d %d %d\n",norm,ref[0],D1[0],D1[1],D1[2]);
    }


    UpdateGUI();
    FillHistos();
  }

}

void Dsc2Dlg::UpdateGUI()
{
   printf("Dsc2Dlg::UpdateGUI reached\n");
   Int_t jj;
   Char_t str[10];
   for(jj=0; jj<ND; jj++)
   {
     sprintf(str,"%8d",D1[jj]);
     tentD1[jj]->SetText(str);
   }
   for(jj=0; jj<ND; jj++)
   {
     sprintf(str,"%8d",D2[jj]);
     tentD2[jj]->SetText(str);
   }
   for(jj=0; jj<ND; jj++)
   {
     sprintf(str,"%8d",D3[jj]);
     tentD3[jj]->SetText(str);
   }
   for(jj=0; jj<ND; jj++)
   {
     sprintf(str,"%8d",D4[jj]);
     tentD4[jj]->SetText(str);
   }
   for(jj=0; jj<ND; jj++)
   {
     sprintf(str,"%8d",D5[jj]);
     tentD5[jj]->SetText(str);
   }
   for(jj=0; jj<ND; jj++)
   {
     sprintf(str,"%8d",D6[jj]);
     tentD6[jj]->SetText(str);
   }
   for(jj=0; jj<ND; jj++)
   {
     sprintf(str,"%8d",D7[jj]);
     tentD7[jj]->SetText(str);
   }
   for(jj=0; jj<ND; jj++)
   {
     sprintf(str,"%8d",D8[jj]);
     tentD8[jj]->SetText(str);
   }

}









/***********************************/
/* DelaysDlg class implementation */

DelaysDlg::DelaysDlg(const TGWindow *p, GUIMainFrame *main,
					   UInt_t w, UInt_t h, UInt_t options) : TGTransientFrame(p, main, w, h, options)
{

  fMain = main; // remember mainframe

   // Create a dialog window. A dialog window pops up with respect to its
   // "main" window.

   // use hierarchical cleani
   SetCleanup(kDeepCleanup);

   fFrame1 = new TGHorizontalFrame(this, 60, 20, kFixedWidth);

   fOkButton = new TGTextButton(fFrame1, "&Ok", 1);
   fOkButton->Associate(this);
   fCancelButton = new TGTextButton(fFrame1, "&Cancel", 2);
   fCancelButton->Associate(this);

   fL1 = new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX,
                           2, 2, 2, 2);
   fL2 = new TGLayoutHints(kLHintsBottom | kLHintsRight, 2, 2, 5, 1);

   fFrame1->AddFrame(fOkButton, fL1);
   fFrame1->AddFrame(fCancelButton, fL1);

   fFrame1->Resize(150, fOkButton->GetDefaultHeight());
   AddFrame(fFrame1, fL2);

   //--------- create Tab widget and some composite frames for Tabs

   fTab = new TGTab(this, 300, 300);
   fL3 = new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5);

   TGCompositeFrame *tf;


   /* delays */
   tf = fTab->AddTab("Delays");
   tf->SetLayoutManager(new TGHorizontalLayout(tf));

   fF6 = new TGGroupFrame(tf, "All values are in ticks (1 tick = 5 ns)", kVerticalFrame);
   fF6->SetTitlePos(TGGroupFrame::kLeft); // right aligned
   tf->AddFrame(fF6, fL3);

   // 14 columns, n rows
   fF6->SetLayoutManager(new TGMatrixLayout(fF6, 0, 14, 1));

   TGNumberFormat::ELimit lim = TGNumberFormat::kNELLimitMinMax;;

   char buff1[100];
   int jj;
   for(jj=0; jj<ND; jj++)
   {
     // D1
     sprintf(buff1, "    D1[%2d] (ticks) ->", jj+1);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
	 tnumD1[jj] = new TGNumberEntry(fF6, 0, 8, 10);
     tnumD1[jj]->SetLimits(lim,0,31 );
	 fF6->AddFrame(tnumD1[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
     // D2
     sprintf(buff1, "    D2[%2d] (ticks) ->", jj+1);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
	 tnumD2[jj] = new TGNumberEntry(fF6, 0, 8, 10);
     tnumD2[jj]->SetLimits(lim,0,31 );
	 fF6->AddFrame(tnumD2[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
     // D3
     sprintf(buff1, "    D3[%2d] (ticks) ->", jj+1);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
	 tnumD3[jj] = new TGNumberEntry(fF6, 0, 8, 10);
     tnumD3[jj]->SetLimits(lim,0,31 );
	 fF6->AddFrame(tnumD3[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
     // D4
     sprintf(buff1, "    D4[%2d] (ticks) ->", jj+1);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
	 tnumD4[jj] = new TGNumberEntry(fF6, 0, 8, 10);
     tnumD4[jj]->SetLimits(lim,0,31 );
	 fF6->AddFrame(tnumD4[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
     // D5
     sprintf(buff1, "    D5[%2d] (ticks) ->", jj+1);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
	 tnumD5[jj] = new TGNumberEntry(fF6, 0, 8, 10);
     tnumD5[jj]->SetLimits(lim,0,31 );
	 fF6->AddFrame(tnumD5[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
     // D6
     sprintf(buff1, "    D6[%2d] (ticks) ->", jj+1);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
	 tnumD6[jj] = new TGNumberEntry(fF6, 0, 8, 10);
     tnumD6[jj]->SetLimits(lim,0,31 );
	 fF6->AddFrame(tnumD6[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
     // D7
     sprintf(buff1, "    D7[%2d] (ticks) ->", jj+1);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
	 tnumD7[jj] = new TGNumberEntry(fF6, 0, 8, 10);
     tnumD7[jj]->SetLimits(lim,0,31 );
	 fF6->AddFrame(tnumD7[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
     // D8
     sprintf(buff1, "    D8[%2d] (ticks) ->", jj+1);
     fF6->AddFrame(new TGLabel(fF6, new TGHotString(buff1)));
	 tnumD8[jj] = new TGNumberEntry(fF6, 0, 8, 10);
     tnumD8[jj]->SetLimits(lim,0,31 );
	 fF6->AddFrame(tnumD8[jj]);
   }

   fF6->Resize(); // resize to default size

   TGLayoutHints *fL5 = new TGLayoutHints(kLHintsBottom | kLHintsExpandX |
                                          kLHintsExpandY, 2, 2, 5, 1);
   AddFrame(fTab, fL5);

   MapSubwindows();
   Resize();   // resize to default size

   // position relative to the parent's window
   CenterOnParent();

   SetWindowName("Dialog");

   MapWindow();
   //fClient->WaitFor(this);    // otherwise canvas contextmenu does not work

   {
     // read VME and update GUI for the first time
     ReadVME();
     UpdateGUI();
   }
}


DelaysDlg::~DelaysDlg()
{
   // Delete DelaysDlg widgets.

}


void DelaysDlg::CloseWindow()
{
   // Called when window is closed (via the window manager or not).
   // Add protection against double-clicks
   fOkButton->SetState(kButtonDisabled);
   fCancelButton->SetState(kButtonDisabled);
   // ... and close the Ged editor if it was activated.
   if (TVirtualPadEditor::GetPadEditor(kFALSE) != 0)
      TVirtualPadEditor::Terminate();
   DeleteWindow();

   fMain->ClearDelaysDlg(); // clear pointer to ourself, so FainFrame will stop reading/writing VME

}


void DelaysDlg::ReadVME()
{
  printf("DelaysDlg::ReadVME reached\n");

  {
    Int_t jj;
    for(jj=0; jj<ND; jj++) D1[jj] = 0;
    for(jj=0; jj<ND; jj++) D2[jj] = 0;
    for(jj=0; jj<ND; jj++) D3[jj] = 0;
    for(jj=0; jj<ND; jj++) D4[jj] = 0;
    for(jj=0; jj<ND; jj++) D5[jj] = 0;
    for(jj=0; jj<ND; jj++) D6[jj] = 0;
    for(jj=0; jj<ND; jj++) D7[jj] = 0;
    for(jj=0; jj<ND; jj++) D8[jj] = 0;

    /* ERROR URWELL_W_DELAY_BASE !!! */
    for(jj=0; jj<ND; jj++) tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_U_DELAY_BASE + jj*4, &D1[jj]);
    for(jj=0; jj<ND; jj++) tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_V_DELAY_BASE + jj*4, &D2[jj]);
    for(jj=0; jj<ND; jj++) tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_W_DELAY_BASE + jj*4, &D3[jj]);
    for(jj=0; jj<ND; jj++) tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_W_DELAY_BASE + jj*4, &D4[jj]);
    for(jj=0; jj<ND; jj++) tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_W_DELAY_BASE + jj*4, &D5[jj]);
    for(jj=0; jj<ND; jj++) tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_W_DELAY_BASE + jj*4, &D6[jj]);
    for(jj=0; jj<ND; jj++) tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_W_DELAY_BASE + jj*4, &D7[jj]);
    for(jj=0; jj<ND; jj++) tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_W_DELAY_BASE + jj*4, &D8[jj]);

	//printf("Delays: %d %d %d\n",U1[0],U1[1],U1[2]);
  }

  WriteVME();
}

Bool_t DelaysDlg::ReadGUI()
{
   printf("DelaysDlg::ReadGUI reached\n");

   Int_t jj;
   for(jj=0; jj<ND; jj++)
   {
     D1GUI[jj] = tnumD1[jj]->GetIntNumber();
   }
   for(jj=0; jj<ND; jj++)
   {
     D2GUI[jj] = tnumD2[jj]->GetIntNumber();
   }
   for(jj=0; jj<ND; jj++)
   {
     D3GUI[jj] = tnumD3[jj]->GetIntNumber();
   }
   for(jj=0; jj<ND; jj++)
   {
     D4GUI[jj] = tnumD4[jj]->GetIntNumber();
   }
   for(jj=0; jj<ND; jj++)
   {
     D5GUI[jj] = tnumD5[jj]->GetIntNumber();
   }
   for(jj=0; jj<ND; jj++)
   {
     D6GUI[jj] = tnumD6[jj]->GetIntNumber();
   }
   for(jj=0; jj<ND; jj++)
   {
     D7GUI[jj] = tnumD7[jj]->GetIntNumber();
   }
   for(jj=0; jj<ND; jj++)
   {
     D8GUI[jj] = tnumD8[jj]->GetIntNumber();
   }
   return kTRUE;
}

void DelaysDlg::WriteVME()
{
  Int_t jj;

  {
    printf("DelaysDlg::WriteDelays reached\n");

    ReadGUI();

    /*ERROR in URWELL !!! */
    for(jj=0; jj<ND; jj++) if(D1[jj] != D1GUI[jj]) tcp->Write32(URWELL_BOARD_ADDRESS_1 + URWELL_U_DELAY_BASE + jj*4, &D1GUI[jj]);
    for(jj=0; jj<ND; jj++) if(D2[jj] != D2GUI[jj]) tcp->Write32(URWELL_BOARD_ADDRESS_1 + URWELL_V_DELAY_BASE + jj*4, &D2GUI[jj]);
    for(jj=0; jj<ND; jj++) if(D3[jj] != D3GUI[jj]) tcp->Write32(URWELL_BOARD_ADDRESS_1 + URWELL_W_DELAY_BASE + jj*4, &D3GUI[jj]);
    for(jj=0; jj<ND; jj++) if(D4[jj] != D3GUI[jj]) tcp->Write32(URWELL_BOARD_ADDRESS_1 + URWELL_W_DELAY_BASE + jj*4, &D4GUI[jj]);
    for(jj=0; jj<ND; jj++) if(D5[jj] != D3GUI[jj]) tcp->Write32(URWELL_BOARD_ADDRESS_1 + URWELL_W_DELAY_BASE + jj*4, &D5GUI[jj]);
    for(jj=0; jj<ND; jj++) if(D6[jj] != D3GUI[jj]) tcp->Write32(URWELL_BOARD_ADDRESS_1 + URWELL_W_DELAY_BASE + jj*4, &D6GUI[jj]);
    for(jj=0; jj<ND; jj++) if(D7[jj] != D3GUI[jj]) tcp->Write32(URWELL_BOARD_ADDRESS_1 + URWELL_W_DELAY_BASE + jj*4, &D7GUI[jj]);
    for(jj=0; jj<ND; jj++) if(D8[jj] != D3GUI[jj]) tcp->Write32(URWELL_BOARD_ADDRESS_1 + URWELL_W_DELAY_BASE + jj*4, &D8GUI[jj]);
  }

}


Bool_t DelaysDlg::UpdateGUI()
{
   printf("DelaysDlg::UpdateGUI reached\n");
   Int_t jj;
   for(jj=0; jj<ND; jj++)
   {
     tnumD1[jj]->SetNumber(D1[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
	 tnumD2[jj]->SetNumber(D2[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
	 tnumD3[jj]->SetNumber(D3[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
	 tnumD4[jj]->SetNumber(D4[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
	 tnumD5[jj]->SetNumber(D5[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
	 tnumD6[jj]->SetNumber(D6[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
	 tnumD7[jj]->SetNumber(D7[jj]);
   }
   for(jj=0; jj<ND; jj++)
   {
	 tnumD8[jj]->SetNumber(D8[jj]);
   }
   return kTRUE;
}


Bool_t DelaysDlg::ProcessMessage(Long_t msg, Long_t parm1, Long_t)
{
   // Process messages coming from widgets associated with the dialog.

   switch (GET_MSG(msg)) {
      case kC_COMMAND:

         switch (GET_SUBMSG(msg)) {
            case kCM_BUTTON:
               switch(parm1) {
                  case 1:
                  case 2:
                     printf("\nTerminating dialog: %s pressed\n",
                            (parm1 == 1) ? "OK" : "Cancel");
                     CloseWindow();
                     break;

                 default:
                     break;
               }
               break;
			   /*
            case kCM_RADIOBUTTON:
               switch (parm1) {
                  case 81:
                     fRad2->SetState(kButtonUp);
                     break;
                  case 82:
                     fRad1->SetState(kButtonUp);
                     break;
               }
               break;
			   */
			   /*
            case kCM_CHECKBUTTON:
               switch (parm1) {
                  case 92:
                     fListBox->SetMultipleSelections(fCheckMulti->GetState());
                     break;
                  default:
                     break;
               }
               break;
			   */
            case kCM_TAB:
               printf("Tab item %ld activated\n", parm1);
			   //if(parm1==5)
			   //{
               //  printf("Scalers !!!\n");
			   //}
               break;
            default:
               break;
         }
         break;

      default:
         break;
   }
   return kTRUE;
}




/*************************************/
/* RegistersDlg class implementation */

// TGNumberEntry widget
const char *const RegistersDlg::numlabel[13] = {
   "Trigger Latency",
   "Trigger Prescale",
   "Two digit real",
   "Three digit real",
   "Four digit real",
   "Real",
   "Degree.min.sec",
   "Min:sec",
   "Hour:min",
   "Hour:min:sec",
   "Day/month/year",
   "Month/day/year",
   "Hex"
};

/*const*/ Double_t RegistersDlg::numinit[13] = {
   1700, 1, 1.00, 1.000, 1.0000, 1.2E-12,
   90 * 3600, 120 * 60, 12 * 60, 12 * 3600 + 15 * 60,
   19991121, 19991121, (Double_t) 0xDEADFACE
};

Double_t min[13] = {0,1,0,0,0,0,0,0,0,0,0,0,0};
Double_t max[13] = {4095,1023,1,1,1,1,1,1,1,1,1,1,1};



RegistersDlg::RegistersDlg(const TGWindow *p, const TGWindow *main)
 : TGTransientFrame(p, main, 10, 10, kHorizontalFrame)
{

  /* get value(s) from hardware */
  UInt_t tmp;
  tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_TRIG0_LATENCY, &tmp);
  numinit[0] = (Double_t)tmp;
  printf("reading trigger_latency=%u (%f)\n",tmp,numinit[0]);
  
  tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_TRIG0_PRESCALE, &tmp);
  numinit[1] = (Double_t)tmp;
  printf("reading trigger_prescale=%u (%f)\n",tmp,numinit[1]);
  

   // build widgets

   // use hierarchical cleaning
   SetCleanup(kDeepCleanup);

   TGGC myGC = *fClient->GetResourcePool()->GetFrameGC();
   TGFont *myfont = fClient->GetFont("-adobe-helvetica-bold-r-*-*-12-*-*-*-*-*-iso8859-1");
   if (myfont) myGC.SetFont(myfont->GetFontHandle());

   fF1 = new TGVerticalFrame(this, 200, 300);
   fL1 = new TGLayoutHints(kLHintsTop | kLHintsLeft, 2, 2, 2, 2);
   AddFrame(fF1, fL1);
   fL2 = new TGLayoutHints(kLHintsCenterY | kLHintsRight, 2, 2, 2, 2);
   for (int i = 0; i < 13; i++) {
      fF[i] = new TGHorizontalFrame(fF1, 200, 30);
      fF1->AddFrame(fF[i], fL2);
      fNumericEntries[i] = new TGNumberEntry(fF[i], numinit[i], 12, i + 20, (TGNumberFormat::EStyle)0, TGNumberFormat::kNEAAnyNumber, TGNumberFormat::kNELLimitMinMax,min[i],max[i]);
                                             //(TGNumberFormat::EStyle) i); // 0,1,2,.. - certain styles, 0-integer
      fNumericEntries[i]->Associate(this);
      fF[i]->AddFrame(fNumericEntries[i], fL2);
      fLabel[i] = new TGLabel(fF[i], numlabel[i], myGC(), myfont->GetFontStruct());
      fF[i]->AddFrame(fLabel[i], fL2);
   }
   fF2 = new TGVerticalFrame(this, 200, 500);
   fL3 = new TGLayoutHints(kLHintsTop | kLHintsLeft, 2, 2, 2, 2);
   AddFrame(fF2, fL3);
   fLowerLimit = new TGCheckButton(fF2, "lower limit:", 4);
   fLowerLimit->Associate(this);
   fF2->AddFrame(fLowerLimit, fL3);
   fLimits[0] = new TGNumberEntry(fF2, 0, 12, 10);
   fLimits[0]->SetLogStep(kFALSE);
   fLimits[0]->Associate(this);
   fF2->AddFrame(fLimits[0], fL3);
   fUpperLimit = new TGCheckButton(fF2, "upper limit:", 5);
   fUpperLimit->Associate(this);
   fF2->AddFrame(fUpperLimit, fL3);
   fLimits[1] = new TGNumberEntry(fF2, 0, 12, 11);
   fLimits[1]->SetLogStep(kFALSE);
   fLimits[1]->Associate(this);
   fF2->AddFrame(fLimits[1], fL3);
   fPositive = new TGCheckButton(fF2, "Positive", 6);
   fPositive->Associate(this);
   fF2->AddFrame(fPositive, fL3);
   fNonNegative = new TGCheckButton(fF2, "Non negative", 7);
   fNonNegative->Associate(this);
   fF2->AddFrame(fNonNegative, fL3);
   fSetButton = new TGTextButton(fF2, " Set ", 2);
   fSetButton->Associate(this);
   fF2->AddFrame(fSetButton, fL3);
   fExitButton = new TGTextButton(fF2, " Close ", 1);
   fExitButton->Associate(this);
   fF2->AddFrame(fExitButton, fL3);

   // set dialog box title
   SetWindowName("Registers");
   SetIconName("Registers");
   SetClassHints("RegistersDlg", "RegistersDlg");
   // resize & move to center
   MapSubwindows();
   UInt_t width = GetDefaultWidth();
   UInt_t height = GetDefaultHeight();
   Resize(width, height);

   CenterOnParent();

   // make the message box non-resizable
   SetWMSize(width, height);
   SetWMSizeHints(width, height, width, height, 0, 0);
   SetMWMHints(kMWMDecorAll | kMWMDecorResizeH | kMWMDecorMaximize |
               kMWMDecorMinimize | kMWMDecorMenu,
               kMWMFuncAll | kMWMFuncResize | kMWMFuncMaximize |
               kMWMFuncMinimize, kMWMInputModeless);

   MapWindow();
   //fClient->WaitFor(this); ?????
}

RegistersDlg::~RegistersDlg()
{
   // dtor
}

void RegistersDlg::CloseWindow()
{
   DeleteWindow();
}

void RegistersDlg::SetLimits()
{
  printf("SetLimits() reached\n");
   Double_t min = fLimits[0]->GetNumber();
   Bool_t low = (fLowerLimit->GetState() == kButtonDown);
   Double_t max = fLimits[1]->GetNumber();
   Bool_t high = (fUpperLimit->GetState() == kButtonDown);
   TGNumberFormat::ELimit lim;
   if (low && high) {
      lim = TGNumberFormat::kNELLimitMinMax;
   } else if (low) {
      lim = TGNumberFormat::kNELLimitMin;
   } else if (high) {
      lim = TGNumberFormat::kNELLimitMax;
   } else {
      lim = TGNumberFormat::kNELNoLimits;
   }
   Bool_t pos = (fPositive->GetState() == kButtonDown);
   Bool_t nneg = (fNonNegative->GetState() == kButtonDown);
   TGNumberFormat::EAttribute attr;
   if (pos) {
      attr = TGNumberFormat::kNEAPositive;
   } else if (nneg) {
      attr = TGNumberFormat::kNEANonNegative;
   } else {
      attr = TGNumberFormat::kNEAAnyNumber;
   }
   for (int i = 0; i < 13; i++) {
      fNumericEntries[i]->SetFormat(fNumericEntries[i]->GetNumStyle(), attr);
      fNumericEntries[i]->SetLimits(lim, min, max);
   }
}

void RegistersDlg::SetTriggerLatency()
{
  UInt_t tmp;

  //printf("SetTriggerLatency() reached\n");
  Double_t trigger_latency = fNumericEntries[0]->GetNumber();
  //printf("trigger_latency = %f\n",trigger_latency);

  tmp = (UInt_t)trigger_latency;
  printf("writing trigger_latency=%u\n",tmp);
  tcp->Write32(URWELL_BOARD_ADDRESS_1 + URWELL_TRIG0_LATENCY, &tmp);

  tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_TRIG0_LATENCY, &tmp);
  printf("reading trigger_latency=%u\n",tmp);
}

void RegistersDlg::SetTriggerPrescale()
{
  UInt_t tmp;

  //printf("SetTriggerPrescale() reached\n");
  Double_t trigger_prescale = fNumericEntries[1]->GetNumber();
  //printf("trigger_prescale = %f\n",trigger_prescale);

  tmp = (UInt_t)trigger_prescale;
  printf("writing trigger_prescale=%u\n",tmp);
  tcp->Write32(URWELL_BOARD_ADDRESS_1 + URWELL_TRIG0_PRESCALE, &tmp);

  tcp->Read32(URWELL_BOARD_ADDRESS_1 + URWELL_TRIG0_PRESCALE, &tmp);
  printf("reading trigger_prescale=%u\n",tmp);
}

Bool_t RegistersDlg::ProcessMessage(Long_t msg, Long_t parm1, Long_t /*parm2*/)
{
  printf("Registers 1: msg=%d(%d), parm1=%d\n",msg,GET_MSG(msg),parm1);
   switch (GET_MSG(msg))
   {
     case kC_COMMAND:
     {
	 printf("Registers 2\n");
       switch (GET_SUBMSG(msg))
       {
         case kCM_BUTTON:
         {
	 printf("Registers 3\n");
           switch (parm1)
           {
             // exit button
             case 1:
             {
               CloseWindow();
               break;
             }
             // set button
             case 2:
             {
               SetLimits();
               break;
             }
           }
           break;
         }
       }
       break;
     }
     case kC_TEXTENTRY:
     {
       printf("TEXTENTRY\n");
       switch (GET_SUBMSG(msg))
       {
         case kTE_TEXTCHANGED:
         {
           printf("TEXTENTRY_TEXTCHANGED\n");
           SetTriggerLatency();
           SetTriggerPrescale();
           break;
		 }
         case kTE_ENTER:
         {
           printf("TEXTENTRY_ENTER\n");
           break;
		 }
         case kTE_TAB:
         {
           printf("TEXTENTRY_TAB\n");
           break;
		 }
         case kTE_KEY:
         {
           break;
           printf("TEXTENTRY_KEY\n");
		 }
	   }
       break;
	 }

   }
   return kTRUE;
}



/****************/
/* Main program */

int main(int argc, char **argv)
{
   TApplication theApp("App", &argc, argv);

   if (gROOT->IsBatch()) {
      fprintf(stderr, "%s: cannot run in batch mode\n", argv[0]);
      return(1);
   }

   if(argc != 2)
   {
     fprintf(stderr, "error: have to specify hostname\n");
     return(1);     
   }

   printf("Trying to connect to >%s<\n",argv[1]);

   GUIMainFrame mainWindow(gClient->GetRoot(), 400, 220, argv[1]);

   theApp.Run();

   return 0;
}
