// scope.h

#define SCOPE_SAMPLEDEPTH		128
#define SCOPE_CHANNELCOUNT		256 /*sergey: was 128*/
#define SCOPE_SIGNALCOUNT		128
#define MAX_INTENSITY_COLOR		255
#define MIN_INTENSITY_COLOR		64
#define SCOPE_COLOR_DEFAULT		255
#define CLOCK_PERIOD_NS			5

/*scope*/
#define ECAL_TRIG_STATUS				0x3000
#define ECAL_TRIG_VALUE7				0x3004
#define ECAL_TRIG_VALUE6			   	0x3008
#define ECAL_TRIG_VALUE5			   	0x300C
#define ECAL_TRIG_VALUE4			   	0x3010
#define ECAL_TRIG_VALUE3			   	0x3014
#define ECAL_TRIG_VALUE2			   	0x3018
#define ECAL_TRIG_VALUE1			   	0x301C
#define ECAL_TRIG_VALUE0			   	0x3020
#define ECAL_TRIG_INGORE7			   	0x3024
#define ECAL_TRIG_INGORE6			   	0x3028
#define ECAL_TRIG_INGORE5			   	0x302C
#define ECAL_TRIG_INGORE4			   	0x3030
#define ECAL_TRIG_INGORE3			   	0x3034
#define ECAL_TRIG_INGORE2			   	0x3038
#define ECAL_TRIG_INGORE1			   	0x303C
#define ECAL_TRIG_INGORE0			   	0x3040
#define ECAL_TRIG_BUFFER			   	0x3044

class ScopeTimer;

class ScopeDlg : public  TGMainFrame {
  
private:
   TGMainFrame       *fMain;
   ScopeTimer          *fScopeTimer;

   TGVerticalFrame      *fF1;
   TGHorizontalFrame    *fF2;
   TGHorizontalFrame    *fF[13];
   TGLayoutHints        *fL1;
   TGLayoutHints        *fL2;
   TGLayoutHints        *fL22;
   TGLayoutHints        *fL3;
   TGLabel              *fLabel1[13];
   TGLabel              *fLabel2[13];
   TGLabel              *fLabel3[13];

   TGVerticalFrame      *fF5;
   TGLayoutHints        *fL5;
   TRootEmbeddedCanvas  *fEc1;
   TCanvas              *fCanvas;
   TRootCanvas          *fRootCanvas;
   Bool_t                use_ascii;

   TGHorizontalFrame    *fF4;
   TGLayoutHints        *fL4;

   TGListBox            *fListBox;
   TGVScrollBar         *fScroll;

   //TGStatusBar          *sbar;
   TGStatusBar          *fStatusBar;

   TText                 fText;
   TPolyLine            *fPolyLine;
   TPolyLine            *fMedian;
   TPolyLine            *fCursor;
   Double_t              xcursor;

   TGCheckButton        *fPersist;
   Bool_t                m_Persist;

   TGButton             *fClearButton, *fContinueButton, *fSingleButton, *fStopButton, *fSelectButton;

   UInt_t board_address[2];
   Int_t  nused[2];

   static const char *const triggerlabel[13];
   static const char *const cursorlabel[13];
   static const char *const channellabel[13];

   char fStr[SCOPE_CHANNELCOUNT][200];
   char fCurchar[SCOPE_CHANNELCOUNT]; // cursor char (0 or 1)

   TGLabel fLabel[SCOPE_CHANNELCOUNT];






   /*new*/
protected:
   void OnVScroll(UInt_t nSBCode, UInt_t nPos, /*CScrollBar*/TGScrollBar* pScrollBar);
   void OnLButtonDown(UInt_t nFlags, Int_t point_x, Int_t point_y);
   BOOL OnInitDialog();
   void OnPaint();
   void OnMouseMove(UInt_t nFlags, Int_t point_x, Int_t point_y);
   void OnBScopecontinuous();
   void OnBScopesingle();
   void OnBScopestop();
   void OnBScopeclear();
// Overrides
//virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   /*new*/


public:
   ScopeDlg(const TGWindow *p, TGMainFrame *main, UInt_t w, UInt_t h, UInt_t addr[2],
            char SignalNames0[SCOPE_CHANNELCOUNT][20], char SignalNames1[SCOPE_CHANNELCOUNT][20], UInt_t options = kVerticalFrame);
   virtual ~ScopeDlg();
   virtual void CloseWindow();
   virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t parm2);

   void    ExecuteEvent(Int_t event, Int_t px, Int_t py);
   Int_t   DistancetoPrimitive(Int_t, Int_t) { return 0; }

    Bool_t OnTimer();
   /*new*/
   //ser ScopeDlg(CWnd* pParent = NULL);   // standard constructor
    BOOL ReadScope(UInt_t addr, UInt_t *buf, Int_t len);
	BOOL ReadoutScope(void);
	BOOL UpdateTriggerMasks();
	BOOL m_bTriggerContinuous;
	void DrawScope();
	void UpdateCursor(int x, int y, BOOL updatePosition, BOOL eraseold);
	int m_iTriggerPattern[SCOPE_CHANNELCOUNT];
	int m_iCursorSample;

	/*TString*/char m_sSignalNames[SCOPE_CHANNELCOUNT][20];
	/*TString*/char n_sSignalNames[SCOPE_CHANNELCOUNT][20];

	unsigned int GetColor(unsigned int intensity, unsigned int odd);



    /* first board */
	unsigned int m_iTriggerBuffer_Low[SCOPE_CHANNELCOUNT][SCOPE_SAMPLEDEPTH];
	unsigned int m_iTriggerBuffer_High[SCOPE_CHANNELCOUNT][SCOPE_SAMPLEDEPTH];
	unsigned int m_iTriggerBuffer_TransLow[SCOPE_CHANNELCOUNT][SCOPE_SAMPLEDEPTH];
	unsigned int m_iTriggerBuffer_TransHigh[SCOPE_CHANNELCOUNT][SCOPE_SAMPLEDEPTH];
	unsigned int m_iTriggerBuffer[SCOPE_CHANNELCOUNT][SCOPE_SAMPLEDEPTH];	/* for ascii version */


    /* second board */
	unsigned int n_iTriggerBuffer_Low[SCOPE_CHANNELCOUNT][SCOPE_SAMPLEDEPTH];
	unsigned int n_iTriggerBuffer_High[SCOPE_CHANNELCOUNT][SCOPE_SAMPLEDEPTH];
	unsigned int n_iTriggerBuffer_TransLow[SCOPE_CHANNELCOUNT][SCOPE_SAMPLEDEPTH];
	unsigned int n_iTriggerBuffer_TransHigh[SCOPE_CHANNELCOUNT][SCOPE_SAMPLEDEPTH];
	unsigned int n_iTriggerBuffer[SCOPE_CHANNELCOUNT][SCOPE_SAMPLEDEPTH];	/* for ascii version */





	/* were local in ReadoutScope() - seg fault .. */
  unsigned int bits[SCOPE_CHANNELCOUNT/32], lastbits[SCOPE_CHANNELCOUNT/32];
  unsigned int ScopeTraces[SCOPE_SAMPLEDEPTH*SCOPE_CHANNELCOUNT/32];
  unsigned int ScopeTracesTmp[SCOPE_SAMPLEDEPTH*SCOPE_CHANNELCOUNT/32];

// Dialog Data
//	enum { IDD = IDD_SCOPE_DIALOG };
//ser	/*CScrollBar*/TGScrollBar	m_scrollScope;
	/*new*/

  //ClassDef(ScopeDlg,0)   // scope with dialog
};

class ScopeTimer : public TTimer
{
private:
  ScopeDlg   *fScopeDlg;

public:
  ScopeTimer(ScopeDlg *m, Long_t ms);
  Bool_t  Notify();
};
