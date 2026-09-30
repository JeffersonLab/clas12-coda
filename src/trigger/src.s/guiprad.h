
/* guiprad.h */

#define ND 16

enum ETestCommandIdentifiers {
   M_FILE_OPEN,
   M_FILE_SAVE,
   M_FILE_SAVEAS,
   M_FILE_PRINT,
   M_FILE_PRINTSETUP,
   M_FILE_EXIT,

   M_REGISTERS,
   M_DELAYS,
   M_SCALERS,
   M_SCOPE_ASCII,
   M_SCOPE_CANVAS,

   M_VIEW_ENBL_DOCK,
   M_VIEW_ENBL_HIDE,
   M_VIEW_DOCK,
   M_VIEW_UNDOCK,

   M_HELP_CONTENTS,
   M_HELP_SEARCH,
   M_HELP_ABOUT
};


class ScalersDlg;
class DelaysDlg;
class ScopeDlg;
class MyTimer;

class TileFrame : public TGCompositeFrame {

private:
   TGCanvas *fCanvas;

public:
   TileFrame(const TGWindow *p);
   virtual ~TileFrame() { }

   void SetCanvas(TGCanvas *canvas) { fCanvas = canvas; }
   Bool_t HandleButton(Event_t *event);
};


class GUIMainFrame : public TGMainFrame {

private:

   TGDockableFrame    *fMenuDock;
   TGCompositeFrame   *fStatusFrame;
   TGCanvas           *fCanvasWindow;
   TileFrame          *fContainer;
   TGTextEntry        *fTestText;
   TGButton           *fTestButton;
   TGColorSelect      *fColorSel;

   TGMenuBar          *fMenuBar;
   TGPopupMenu        *fMenuFile, *fMenuPRAD, *fMenuView, *fMenuHelp;
   TGPopupMenu        *fCascadeMenu, *fCascade1Menu, *fCascade2Menu;
   TGPopupMenu        *fMenuNew1, *fMenuNew2;
   TGLayoutHints      *fMenuBarLayout, *fMenuBarItemLayout, *fMenuBarHelpLayout;

   TGCompositeFrame    *fHor1;
   TGTextButton        *fExit;
   TGGroupFrame        *fGframe;
   TGNumberEntry       *fNumber;
   TGLabel             *fLabel;

   TGGroupFrame        *fF6;
   TGLayoutHints       *fL3;

   TGTextButton *btConnect, *btDisconnect;

   MyTimer *tt;

   char hostname[80];

public:
   ScalersDlg         *fScalersDlg;
   DelaysDlg          *fDelaysDlg;
   ScopeDlg           *fScopeDlg;

   GUIMainFrame(const TGWindow *p, UInt_t w, UInt_t h, char *host);
   virtual ~GUIMainFrame();

   virtual void CloseWindow();
   virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t);

   void ClearScalersDlg() {fScalersDlg = NULL;}
   void ClearDelaysDlg() {fDelaysDlg = NULL;}
   //void ClearScopeDlg() {fScopeDlg = NULL;}  // sergey: crash here !!!
};



class ScalersDlg : public TGTransientFrame {

private:
   GUIMainFrame       *fMain;
   TGCompositeFrame    *fFrame1, *fF1, *fF2, *fF3, *fF4, *fF5;
   TGGroupFrame        *fF6, *fF7;
   TGButton            *fOkButton, *fCancelButton, *fStartB, *fStopB;
   TGButton            *fBtn1, *fBtn2, *fChk1, *fChk2, *fRad1, *fRad2;
   TGPictureButton     *fPicBut1;
   TGCheckButton       *fCheck1;
   TGCheckButton       *fCheckMulti;
   TGListBox           *fListBox;
   TGComboBox          *fCombo;
   TGTab               *fTab;
   TGTextEntry         *fTxt1, *fTxt2;
   TGLayoutHints       *fL1, *fL2, *fL3, *fL4;
   TRootEmbeddedCanvas *fEc1, *fEc2, *fEc3, *fEc4, *fEc5, *fEc6, *fEc7, *fEc8;
   Int_t                fFirstEntry;
   Int_t                fLastEntry;
   Bool_t               fFillHistos;
   TH1F                *fHpxD1, *fHpxD2, *fHpxD3, *fHpxD4, *fHpxD5, *fHpxD6, *fHpxD7, *fHpxD8;

   TGTextBuffer *tbufD1[ND], *tbufD2[ND], *tbufD3[ND], *tbufD4[ND], *tbufD5[ND], *tbufD6[ND], *tbufD7[ND], *tbufD8[ND];
   TGTextEntry *tentD1[ND], *tentD2[ND], *tentD3[ND], *tentD4[ND], *tentD5[ND], *tentD6[ND], *tentD7[ND], *tentD8[ND];
   TGTextBuffer *tbufTrig[NTRIG];
   TGTextEntry *tentTrig[NTRIG];

   UInt_t D1[16], D2[16], D3[16], D4[16], D5[16], D6[16], D7[16], D8[16];
   UInt_t REF1;
   UInt_t TRIG[NTRIG];

   void FillHistos();

public:
   ScalersDlg(const TGWindow *p, GUIMainFrame *main, UInt_t w, UInt_t h,
               UInt_t options = kVerticalFrame);
   virtual ~ScalersDlg();

   virtual void CloseWindow();
   virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t parm2);

   virtual void ReadVME();
   virtual void UpdateGUI();
};



class DelaysDlg : public TGTransientFrame {

private:
   GUIMainFrame        *fMain;
   TGCompositeFrame    *fFrame1, *fF1, *fF2, *fF3, *fF4, *fF5;
   TGGroupFrame        *fF6, *fF7;
   TGButton            *fOkButton, *fCancelButton, *fStartB, *fStopB;
   TGButton            *fBtn1, *fBtn2, *fChk1, *fChk2, *fRad1, *fRad2;
   TGPictureButton     *fPicBut1;
   TGCheckButton       *fCheck1;
   TGCheckButton       *fCheckMulti;
   TGListBox           *fListBox;
   TGComboBox          *fCombo;
   TGTab               *fTab;
   TGTextEntry         *fTxt1, *fTxt2;
   TGLayoutHints       *fL1, *fL2, *fL3, *fL4;
   TRootEmbeddedCanvas *fEc1, *fEc2;
   Int_t                fFirstEntry;
   Int_t                fLastEntry;

   TGNumberEntry *tnumD1[ND], *tnumD2[ND], *tnumD3[ND], *tnumD4[ND], *tnumD5[ND], *tnumD6[ND], *tnumD7[ND], *tnumD8[ND];

   UInt_t D1[ND], D2[ND], D3[ND], D4[ND], D5[ND], D6[ND], D7[ND], D8[ND];
   UInt_t D1GUI[ND], D2GUI[ND], D3GUI[ND], D4GUI[ND], D5GUI[ND], D6GUI[ND], D7GUI[ND], D8GUI[ND];

public:
   DelaysDlg(const TGWindow *p, GUIMainFrame *main, UInt_t w, UInt_t h,
               UInt_t options = kVerticalFrame);
   virtual ~DelaysDlg();
   virtual void CloseWindow();
   virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t parm2);

   virtual void ReadVME();
   virtual void WriteVME();
   virtual Bool_t ReadGUI();
   virtual Bool_t UpdateGUI();
};






class RegistersDlg : public TGTransientFrame {

private:
   TGVerticalFrame      *fF1;
   TGVerticalFrame      *fF2;
   TGHorizontalFrame    *fF[NTRIG];
   TGLayoutHints        *fL1;
   TGLayoutHints        *fL2;
   TGLayoutHints        *fL3;
   TGLabel              *fLabel[NTRIG];
   TGNumberEntry        *fNumericEntries[NTRIG];
   TGCheckButton        *fLowerLimit;
   TGCheckButton        *fUpperLimit;
   TGNumberEntry        *fLimits[2];
   TGCheckButton        *fPositive;
   TGCheckButton        *fNonNegative;
   TGButton             *fSetButton;
   TGButton             *fExitButton;

   static const char *const numlabel[NTRIG];
   static /*const*/ Double_t numinit[NTRIG];

public:
   RegistersDlg(const TGWindow *p, const TGWindow *main);
   virtual ~RegistersDlg();
   virtual void CloseWindow();

   void SetLimits();
   void SetTriggerLatency();
   virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t);
};


class MyTimer : public TTimer
{
private:
  GUIMainFrame   *fGUIMainFrame;   //display to which this timer belongs

public:
  MyTimer(GUIMainFrame *m, Long_t ms);
  Bool_t  Notify();
};
