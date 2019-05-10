#ifndef VTP_HPS_TrgHist_H
#define VTP_HPS_TrgHist_H

#include <stdlib.h>
#include "ModuleFrame.h"
#include "RootHeader.h"

#define UDPATETIME_MAX                  60

#define VTP_HPS_TRIG_INFO_S0_TOTAL      0.0
#define VTP_HPS_TRIG_INFO_S0_PASS       1.0
#define VTP_HPS_TRIG_INFO_S0_TI         2.0
#define VTP_HPS_TRIG_INFO_S1_TOTAL      3.0
#define VTP_HPS_TRIG_INFO_S1_PASS       4.0
#define VTP_HPS_TRIG_INFO_S1_TI         5.0
#define VTP_HPS_TRIG_INFO_P0_TOTAL      6.0
#define VTP_HPS_TRIG_INFO_P0_SUMPASS    7.0
#define VTP_HPS_TRIG_INFO_P0_DIFPASS    8.0
#define VTP_HPS_TRIG_INFO_P0_EDPASS     9.0
#define VTP_HPS_TRIG_INFO_P0_COPPASS    10.0
#define VTP_HPS_TRIG_INFO_P0_PASS       11.0
#define VTP_HPS_TRIG_INFO_P0_TI         12.0
#define VTP_HPS_TRIG_INFO_P1_TOTAL      13.0
#define VTP_HPS_TRIG_INFO_P1_SUMPASS    14.0
#define VTP_HPS_TRIG_INFO_P1_DIFPASS    15.0
#define VTP_HPS_TRIG_INFO_P1_EDPASS     16.0
#define VTP_HPS_TRIG_INFO_P1_COPPASS    17.0
#define VTP_HPS_TRIG_INFO_P1_PASS       18.0
#define VTP_HPS_TRIG_INFO_P1_TI         19.0
#define VTP_HPS_TRIG_INFO_LED           20.0
#define VTP_HPS_TRIG_INFO_COSMIC        21.0
#define VTP_HPS_TRIG_INFO_LED_COSMIC_TI 22.0
#define VTP_HPS_TRIG_INFO_PULSER_TI     23.0
#define VTP_HPS_TRIG_INFO_L1A           24.0

#define CMB_HISTSRC                     3000
#define CMB_ID_SEL_HISTSRC_S0           0
#define CMB_ID_SEL_HISTSRC_S1           1
#define CMB_ID_SEL_HISTSRC_S2           2
#define CMB_ID_SEL_HISTSRC_S3           3
#define CMB_ID_SEL_HISTSRC_P0           4
#define CMB_ID_SEL_HISTSRC_P1           5
#define CMB_ID_SEL_HISTSRC_P2           6
#define CMB_ID_SEL_HISTSRC_P3           7
#define CMB_ID_SEL_HISTSRC_ALL          8

class VTP_HPS_TrgHist : public TGCompositeFrame
{
public:
  VTP_HPS_TrgHist(const TGWindow *p, ModuleFrame *pModule) : TGCompositeFrame(p, 400, 400)
  {
    static int inst = 0;

    SetLayoutManager(new TGVerticalLayout(this));

    pM = pModule;

    TGCompositeFrame *pTF1;
    TLegend *pLegend;

    AddFrame(pTF1 = new TGHorizontalFrame(this), new TGLayoutHints(kLHintsExpandX | kLHintsTop));
      pTF1->AddFrame(pButtonNormalize = new TGTextButton(pTF1, new TGHotString("Normalize")), new TGLayoutHints(kLHintsCenterY | kLHintsLeft));
        pButtonNormalize->AllowStayDown(kTRUE);
        pButtonNormalize->SetDown(kTRUE);
      pTF1->AddFrame(pButtonAutoUpdate = new TGTextButton(pTF1, new TGHotString("Update Mode: Manual"), BTN_AUTOUPDATE), new TGLayoutHints(kLHintsCenterY | kLHintsLeft));
        pButtonAutoUpdate->SetWidth(80);
        pButtonAutoUpdate->SetEnabled(kTRUE);
        pButtonAutoUpdate->AllowStayDown(kTRUE);
        pButtonAutoUpdate->Associate(this);
      pTF1->AddFrame(pButtonManualUpdate = new TGTextButton(pTF1, new TGHotString("Manual Update"), BTN_MANUALUPDATE), new TGLayoutHints(kLHintsCenterY | kLHintsLeft));
        pButtonManualUpdate->Associate(this);
      pTF1->AddFrame(new TGLabel(pTF1, new TGString("Histogram Source:")), new TGLayoutHints(kLHintsLeft, 2, 2, 4, 0));
      pTF1->AddFrame(pComboHistSrc = new TGComboBox(pTF1, CMB_HISTSRC), new TGLayoutHints(kLHintsCenterY | kLHintsLeft));
        pComboHistSrc->Resize(150,20);
        pComboHistSrc->AddEntry("ALL CLUSTERS", CMB_ID_SEL_HISTSRC_ALL);
        pComboHistSrc->AddEntry("S0  CLUSTERS", CMB_ID_SEL_HISTSRC_S0);
        pComboHistSrc->AddEntry("S1  CLUSTERS", CMB_ID_SEL_HISTSRC_S1);
        pComboHistSrc->AddEntry("S2  CLUSTERS", CMB_ID_SEL_HISTSRC_S2);
        pComboHistSrc->AddEntry("S3  CLUSTERS", CMB_ID_SEL_HISTSRC_S3);
        pComboHistSrc->AddEntry("P0  CLUSTERS", CMB_ID_SEL_HISTSRC_P0);
        pComboHistSrc->AddEntry("P1  CLUSTERS", CMB_ID_SEL_HISTSRC_P1);
        pComboHistSrc->AddEntry("P2  CLUSTERS", CMB_ID_SEL_HISTSRC_P2);
        pComboHistSrc->AddEntry("P3  CLUSTERS", CMB_ID_SEL_HISTSRC_P3);
        pComboHistSrc->Associate(this);
        pComboHistSrc->Select(CMB_ID_SEL_HISTSRC_ALL);
      pTF1->AddFrame(pSliderUpdateTime = new TGHSlider(pTF1, 100, kSlider1 | kScaleBoth, SDR_UPDATETIME), new TGLayoutHints(kLHintsExpandX | kLHintsCenterY | kLHintsLeft));
        pSliderUpdateTime->SetRange(0, UDPATETIME_MAX);
//        pSliderUpdateTime->SetEnabled(kFALSE);
        pSliderUpdateTime->SetPosition(1);
        pSliderUpdateTime->Associate(this);

//    AddFrame(pCanvasRates = new TRootEmbeddedCanvas("c1", this, 1300, 125));//, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
/*
    TGCanvas *pTGCanvas;
    AddFrame(pTGCanvas = new TGCanvas(this), new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
      pTGCanvas->SetContainer(pTF1 = new TGVerticalFrame(pTGCanvas->GetViewPort()));
        pTF1->AddFrame(pCanvas = new TRootEmbeddedCanvas("c1", pTF1, 1300, 2300));//, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
*/

    // Use a resizable frame instead of viewport
    AddFrame(pCanvas = new TRootEmbeddedCanvas("c1", this, 1300, 2300), new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

    gStyle->SetPalette(1, NULL);

    pCanvas->GetCanvas()->Divide(1,3);

    //////////////////////////////////    
    // Trigger Info Canvas
    //////////////////////////////////    
    const char *trigger_info_bins[] = {
      "S0_Total", "S0_Pass", "S0_TI",
      "S1_Total", "S1_Pass", "S1_TI",
      "P0_Total", "P0_SumPass", "P0_DifPass", "P0_EDPass", "P0_CopPass", "P0_Pass", "P0_TI",
      "P1_Total", "P1_SumPass", "P1_DifPass", "P1_EDPass", "P1_CopPass", "P1_Pass", "P1_TI",
      "LED", "COSMIC", "LED_COSMIC_TI",
      "PULSER_TI",
      "L1A"
    };

    pCanvas->GetCanvas()->cd(1);
    pCanvas->GetCanvas()->cd(1)->SetLogy(1);
    pHistTriggerInfo = new TH1F("Trigger Info", "Trigger Info;;Hz", sizeof(trigger_info_bins)/sizeof(trigger_info_bins[0]), 0.0, (double)sizeof(trigger_info_bins)/sizeof(trigger_info_bins[0]));
    pHistTriggerInfo->SetLineColor(kBlack);
    pHistTriggerInfo->SetFillColor(kBlue);
    pHistTriggerInfo->SetNdivisions(sizeof(trigger_info_bins)/sizeof(trigger_info_bins[0]));
    pHistTriggerInfo->SetStats(0);
    for(int i = 0; i < (int)(sizeof(trigger_info_bins)/sizeof(trigger_info_bins[0])); i++)
      pHistTriggerInfo->GetXaxis()->SetBinLabel(i+1, trigger_info_bins[i]);
    pHistTriggerInfo->Draw("B TEXT");

    //////////////////////////////////    
    // Cluster Position Canvas
    //////////////////////////////////    
    pCanvas->GetCanvas()->cd(2);
    pCanvas->GetCanvas()->cd(2)->SetLogz(1);
    pHistPosition = new TH2F("ClusterPosition", "ClusterPosition;X;Y", 46, -22.0, 24.0, 11, -5.0, 6.0);
    pHistPosition->SetStats(0);
    pHistPosition->GetXaxis()->CenterLabels();
    pHistPosition->GetXaxis()->SetNdivisions(46, kFALSE);
    pHistPosition->GetXaxis()->SetTickLength(1);
    pHistPosition->GetYaxis()->CenterLabels();
    pHistPosition->GetYaxis()->SetNdivisions(11, kFALSE);
    pHistPosition->GetYaxis()->SetTickLength(1);
//    pHistPosition->Draw("COLZTEXT");
    pHistPosition->Draw("COLZ");

    int x = 23;
    for(int n = 1; n <= 46; n++)
    {
      pHistPosition->GetXaxis()->SetBinLabel(n,Form("%d", x));
      x--;
      if(x == 0) x--;
    }
    
    pCanvas->GetCanvas()->cd(3)->Divide(3,1);
    //////////////////////////////////    
    // Cluster Energy
    //////////////////////////////////    
    pCanvas->GetCanvas()->cd(3)->cd(1);
    pCanvas->GetCanvas()->cd(3)->cd(1)->SetLogy(1);

    pHistEnergyT = new TH1F("ClusterEnergy", "ClusterEnergy", 1024, 0.0, 8.0*1024.0);
    pHistEnergyT->GetXaxis()->SetTitle("Energy(MeV)");
    pHistEnergyT->GetXaxis()->SetRangeUser(0.0, 8192.0);
    pHistEnergyT->GetYaxis()->SetTitle("Counts");
    pHistEnergyT->GetYaxis()->CenterTitle();
    pHistEnergyT->SetLineColor(kBlue);
    pHistEnergyT->SetStats(0);
    pHistEnergyT->Draw();

    pHistEnergyB = new TH1F("ClusterEnergy", "ClusterEnergy", 1024, 0.0, 8.0*1024.0);
    pHistEnergyB->GetXaxis()->SetTitle("Energy(MeV)");
    pHistEnergyB->GetXaxis()->SetRangeUser(0.0, 8192.0);
    pHistEnergyB->GetYaxis()->SetTitle("Counts");
    pHistEnergyB->GetYaxis()->CenterTitle();
    pHistEnergyB->SetLineColor(kRed);
    pHistEnergyB->SetStats(0);
    pHistEnergyB->Draw("SAME");

    pLegend = new TLegend(0.7,0.8,0.9,0.9);
    pLegend->AddEntry(pHistEnergyT, "Top Clusters");
    pLegend->AddEntry(pHistEnergyB, "Bot Clusters");
    pLegend->Draw();

    //////////////////////////////////    
    // Cluster NHits
    //////////////////////////////////    
    pCanvas->GetCanvas()->cd(3)->cd(2);
    pCanvas->GetCanvas()->cd(3)->cd(2)->SetLogy(1);

    pHistNHitsT = new TH1F("ClusterNHitsTop", "ClusterNHitsTop", 9, 0.0, 9.0);
    pHistNHitsT->GetXaxis()->SetTitle("NHits");
    pHistNHitsT->GetXaxis()->SetRangeUser(0.0, 9.0);
    pHistNHitsT->GetXaxis()->CenterLabels();
    pHistNHitsT->GetYaxis()->SetTitle("Counts");
    pHistNHitsT->GetYaxis()->CenterTitle();
    pHistNHitsT->SetLineColor(kBlue);
    pHistNHitsT->SetStats(0);
    pHistNHitsT->Draw();

    pHistNHitsB = new TH1F("ClusterNHitsBot", "ClusterNHitsBot", 9, 0.0, 9.0);
    pHistNHitsB->GetXaxis()->SetTitle("NHits");
    pHistNHitsB->GetXaxis()->SetRangeUser(0.0, 9.0);
    pHistNHitsB->GetXaxis()->CenterLabels();
    pHistNHitsB->GetYaxis()->SetTitle("Counts");
    pHistNHitsB->GetYaxis()->CenterTitle();
    pHistNHitsB->SetLineColor(kRed);
    pHistNHitsB->SetStats(0);
    pHistNHitsB->Draw("SAME");

    pLegend = new TLegend(0.7,0.8,0.9,0.9);
    pLegend->AddEntry(pHistNHitsT, "Top Clusters");
    pLegend->AddEntry(pHistNHitsB, "Bot Clusters");
    pLegend->Draw();

    //////////////////////////////////    
    // Cluster Latency
    //////////////////////////////////    
    pCanvas->GetCanvas()->cd(3)->cd(3);
    pCanvas->GetCanvas()->cd(3)->cd(3)->SetLogy(1);

    pHistLatencyT = new TH1F("ClusterLatencyTop", "ClusterLatencyTop", 1024, 0.0, 4.0*1024.0);
    pHistLatencyT->GetXaxis()->SetTitle("Latency(ns)");
    pHistLatencyT->GetXaxis()->SetRangeUser(0.0, 4096.0);
    pHistLatencyT->GetYaxis()->SetTitle("Counts");
    pHistLatencyT->GetYaxis()->CenterTitle();
    pHistLatencyT->SetLineColor(kBlue);
    pHistLatencyT->SetStats(0);
    pHistLatencyT->Draw();

    pHistLatencyB = new TH1F("ClusterLatencyBot", "ClusterLatencyBot", 1024, 0.0, 4.0*1024.0);
    pHistLatencyB->GetXaxis()->SetTitle("Latency(ns)");
    pHistLatencyB->GetXaxis()->SetRangeUser(0.0, 4096.0);
    pHistLatencyB->GetYaxis()->SetTitle("Counts");
    pHistLatencyB->GetYaxis()->CenterTitle();
    pHistLatencyB->SetLineColor(kRed);
    pHistLatencyB->SetStats(0);
    pHistLatencyB->Draw("SAME");

    pLegend = new TLegend(0.7,0.8,0.9,0.9);
    pLegend->AddEntry(pHistLatencyT, "Top Clusters");
    pLegend->AddEntry(pHistLatencyB, "Bot Clusters");
    pLegend->Draw();



    pCanvas->GetCanvas()->cd();
    pCanvas->GetCanvas()->Modified();
    pCanvas->GetCanvas()->Update();

    pTimerUpdate = new TTimer(this, 1000*pSliderUpdateTime->GetPosition(), kTRUE);

    HpsMon_HistCtrl       = (volatile unsigned int *)((int)pM->BaseAddr + 0x5700);
    HpsMon_HistSel        = (volatile unsigned int *)((int)pM->BaseAddr + 0x5704);
    HpsMon_HistTime       = (volatile unsigned int *)((int)pM->BaseAddr + 0x5720);
    HpsMon_HistPositionT  = (volatile unsigned int *)((int)pM->BaseAddr + 0x5724);
    HpsMon_HistPositionB  = (volatile unsigned int *)((int)pM->BaseAddr + 0x5728);
    HpsMon_HistEnergyT    = (volatile unsigned int *)((int)pM->BaseAddr + 0x572C);
    HpsMon_HistEnergyB    = (volatile unsigned int *)((int)pM->BaseAddr + 0x5730);
    HpsMon_HistNHitsT     = (volatile unsigned int *)((int)pM->BaseAddr + 0x5734);
    HpsMon_HistNHitsB     = (volatile unsigned int *)((int)pM->BaseAddr + 0x5738);
    HpsMon_HistLatencyT   = (volatile unsigned int *)((int)pM->BaseAddr + 0x573C);
    HpsMon_HistLatencyB   = (volatile unsigned int *)((int)pM->BaseAddr + 0x5740);
    HpsMon_HistHodoT      = (volatile unsigned int *)((int)pM->BaseAddr + 0x5744);
    HpsMon_HistHodoB      = (volatile unsigned int *)((int)pM->BaseAddr + 0x5748);

/*
    HpsSingles0_Pass      = (volatile unsigned int *)((int)pM->BaseAddr + 0x0780);
    HpsSingles0_Tot     = (volatile unsigned int *)((int)pM->BaseAddr + 0x0784);
    HpsSingles1_Pass      = (volatile unsigned int *)((int)pM->BaseAddr + 0x0880);
    HpsSingles1_Tot     = (volatile unsigned int *)((int)pM->BaseAddr + 0x0884);
    HpsPairs0_Pass        = (volatile unsigned int *)((int)pM->BaseAddr + 0x0980);
    HpsPairs0_SumPass     = (volatile unsigned int *)((int)pM->BaseAddr + 0x0984);
    HpsPairs0_DiffPass    = (volatile unsigned int *)((int)pM->BaseAddr + 0x0988);
    HpsPairs0_EDPass      = (volatile unsigned int *)((int)pM->BaseAddr + 0x098C);
    HpsPairs0_CoplanarPass  = (volatile unsigned int *)((int)pM->BaseAddr + 0x0990);
    HpsPairs0_TriggerPass = (volatile unsigned int *)((int)pM->BaseAddr + 0x0994);
    HpsPairs1_Pass        = (volatile unsigned int *)((int)pM->BaseAddr + 0x0A80);
    HpsPairs1_SumPass     = (volatile unsigned int *)((int)pM->BaseAddr + 0x0A84);
    HpsPairs1_DiffPass    = (volatile unsigned int *)((int)pM->BaseAddr + 0x0A88);
    HpsPairs1_EDPass      = (volatile unsigned int *)((int)pM->BaseAddr + 0x0A8C);
    HpsPairs1_CoplanarPass  = (volatile unsigned int *)((int)pM->BaseAddr + 0x0A90);
    HpsPairs1_TriggerPass = (volatile unsigned int *)((int)pM->BaseAddr + 0x0A94);
    HpsScaler_Disable     = (volatile unsigned int *)((int)pM->BaseAddr + 0x0300);
    HpsScaler_Sysclk50    = (volatile unsigned int *)((int)pM->BaseAddr + 0x0304);
    
    HpsScaler_Trig1     = (volatile unsigned int *)((int)pM->BaseAddr + 0x0310);
    HpsScaler_Busy        = (volatile unsigned int *)((int)pM->BaseAddr + 0x0348);
    HpsScaler_BusyCycles    = (volatile unsigned int *)((int)pM->BaseAddr + 0x034C);
    HpsScaler_P2LVDSOut0    = (volatile unsigned int *)((int)pM->BaseAddr + 0x0370);
    HpsScaler_P2LVDSOut1    = (volatile unsigned int *)((int)pM->BaseAddr + 0x0374);
    HpsScaler_P2LVDSOut2    = (volatile unsigned int *)((int)pM->BaseAddr + 0x0378);
    HpsScaler_P2LVDSOut3    = (volatile unsigned int *)((int)pM->BaseAddr + 0x037C);
    HpsScaler_P2LVDSOut4    = (volatile unsigned int *)((int)pM->BaseAddr + 0x0380);
    HpsScaler_P2LVDSOut5    = (volatile unsigned int *)((int)pM->BaseAddr + 0x0384);
    HpsScaler_TrigBusy0   = (volatile unsigned int *)((int)pM->BaseAddr + 0x2110);
    HpsScaler_TrigBusy1   = (volatile unsigned int *)((int)pM->BaseAddr + 0x2114);
    HpsScaler_TrigBusy2   = (volatile unsigned int *)((int)pM->BaseAddr + 0x2118);
    HpsScaler_TrigBusy3   = (volatile unsigned int *)((int)pM->BaseAddr + 0x211C);
    HpsScaler_TrigBusy4   = (volatile unsigned int *)((int)pM->BaseAddr + 0x2120);
    HpsScaler_TrigBusy5   = (volatile unsigned int *)((int)pM->BaseAddr + 0x2124);

    HpsScaler_Cosmic      = (volatile unsigned int *)((int)pM->BaseAddr + 0x0B2C);
    HpsScaler_Led       = (volatile unsigned int *)((int)pM->BaseAddr + 0x0B1C);

    Ti_reset            = (volatile unsigned int *)((int)0x00A80100);
    Ti_LiveTime         = (volatile unsigned int *)((int)0x00A800A8);
    Ti_BusyTime         = (volatile unsigned int *)((int)0x00A800AC);
*/
    // Setup histograms to bin what we've selected by default
    pM->WriteReg32(HpsMon_HistSel, pComboHistSrc->GetSelected()<<11);

    inst++;
  }
  
  virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t)
  {
    switch(GET_MSG(msg))
    {
    case kC_COMMAND:
      switch(GET_SUBMSG(msg))
      {
      case kCM_BUTTON:
        switch(parm1)
        {
          case BTN_AUTOUPDATE:
            if(pButtonAutoUpdate->IsDown())
            {
              pButtonAutoUpdate->SetText(new TGHotString("Update Mode: Auto"));
//              pSliderUpdateTime->SetEnabled(kTRUE);
              pTimerUpdate->Start(1000*pSliderUpdateTime->GetPosition(), kTRUE);
            }
            else
            {
              pButtonAutoUpdate->SetText(new TGHotString("Update Mode: Manual"));
//              pSliderUpdateTime->SetEnabled(kFALSE);
              pTimerUpdate->TurnOff();
            }
            break;
          case BTN_MANUALUPDATE:
            UpdateHistogram();
            break;
          default:
            printf("button id %d pressed\n", (int)parm1);
            break;
        }
        break;

			case kCM_COMBOBOX:
        switch(parm1)
        {
          case CMB_HISTSRC:
            pM->WriteReg32(HpsMon_HistSel, pComboHistSrc->GetSelected()<<11);
            break;

          default:
            printf("combobox id %d pressed\n", (int)parm1);
            break;
        }
        break;

      case kC_HSLIDER:
        switch(parm1)
        {
          case SDR_UPDATETIME:
            pTimerUpdate->TurnOff();
            pTimerUpdate->Start(1000*pSliderUpdateTime->GetPosition(), kTRUE);
            break;
          default:
            printf("slider id %d pressed\n", (int)parm1);
            break;
        }
        break;
      }
      break;
    }
    return kTRUE;
  }

  virtual Bool_t HandleTimer(TTimer *t)
  {
    if(pTimerUpdate->HasTimedOut())
    {
      UpdateHistogram();
      if(pButtonAutoUpdate->IsDown())
        pTimerUpdate->Start(1000*pSliderUpdateTime->GetPosition(), kTRUE);
    }
    return kTRUE;
  }

  void UpdateLatencyHistogram(float scale, Bool_t normalize)
  {
    unsigned int buft[1024], bufb[1024];
    float val;

    pM->BlkReadReg32(HpsMon_HistLatencyT, buft, 1024, CRATE_MSG_FLAGS_NOADRINC);
    pM->BlkReadReg32(HpsMon_HistLatencyB, bufb, 1024, CRATE_MSG_FLAGS_NOADRINC);
    
    pCanvas->GetCanvas()->cd(3)->cd(3);
    pHistLatencyT->Reset();
    pHistLatencyB->Reset();

    if(normalize)
    {
      pHistLatencyT->GetYaxis()->SetTitle("Hz");
      pHistLatencyB->GetYaxis()->SetTitle("Hz");
    }
    else
    {
      pHistLatencyT->GetYaxis()->SetTitle("Counts");
      pHistLatencyB->GetYaxis()->SetTitle("Counts");
    }
    
    for(int i = 0; i < 1024; i++)
    {
      val = (float)buft[i];
      if(normalize) val *= scale;
      pHistLatencyT->Fill(4*i, val);

      val = (float)bufb[i];
      if(normalize) val *= scale;
      pHistLatencyB->Fill(4*i, val);
    }

    pCanvas->GetCanvas()->Modified();
    pCanvas->GetCanvas()->Update();
  }

  void UpdatePositionHistogram(float scale, Bool_t normalize)
  {
    unsigned int buf[1024];
    float rate_top = 0.0, rate_bot = 0.0;
    static bool called=0;

    static TPaveText tt1(0.1,0.9,0.3,1.0,"NDC");
    static TPaveText tt2(0.7,0.91,0.9,0.99,"NDC");
    static TPaveText ttT(-22+13+0.05,6-5,-22+22,7-5-0.05);
    static TPaveText ttB(-22+13+0.05,4-5+0.05,-22+22,5-5);
    static TPaveText ttM(-22+0+0.05,5-5+0.05,-22+13,6-5);
    static TBox bb;
    static TLine ll;

    if (!called)
    {
        called=1;
        bb.SetFillStyle(1001);
        bb.SetFillColor(kWhite);
        bb.SetLineWidth(1);
        bb.SetLineColor(kBlack);
        tt1.SetBorderSize(0);
        tt2.SetBorderSize(0);
        tt1.SetFillColor(kWhite);
        tt2.SetFillColor(kWhite);
        ttT.SetBorderSize(0);
        ttB.SetBorderSize(0);
        ttT.SetFillColor(kWhite);
        ttB.SetFillColor(kWhite);
        ttM.SetBorderSize(0);
        ttM.SetFillColor(kWhite);
        ttM.SetTextColor(kRed);
    }

    float max=0.0;
    
    pM->BlkReadReg32(HpsMon_HistPositionT, &buf[0], 512, CRATE_MSG_FLAGS_NOADRINC);
    pM->BlkReadReg32(HpsMon_HistPositionB, &buf[512], 512, CRATE_MSG_FLAGS_NOADRINC);

    pCanvas->GetCanvas()->cd(2);
    pHistPosition->SetMinimum(0);
    pHistPosition->Reset();

    int x, y;
    for(int i = 0; i < 1024; i++)
    {
      float val = (float)buf[i];

      if(normalize) val *= scale;

      if(val > max) max = val;

      x = (i>>0) & 0x3F;
      y = (i>>6) & 0xF;

      if(x & 0x20) x |= 0xFFFFFFC0;
      if(y & 0x08) y |= 0xFFFFFFF0;

      if(y > 0) rate_top += val;
      else      rate_bot += val;

      pHistPosition->Fill(x, y, val);
    }

    bb.DrawBox(-9+0.05,-1,0,1.97);
    bb.DrawBox(-24,0,24.05,0.97);
    ll.DrawLine(pHistPosition->GetXaxis()->GetXmin(),pHistPosition->GetYaxis()->GetXmin(),
                pHistPosition->GetXaxis()->GetXmax(),pHistPosition->GetYaxis()->GetXmin());
    ll.DrawLine(pHistPosition->GetXaxis()->GetXmin(),pHistPosition->GetYaxis()->GetXmax(),
                pHistPosition->GetXaxis()->GetXmax(),pHistPosition->GetYaxis()->GetXmax());
    ll.DrawLine(pHistPosition->GetXaxis()->GetXmin(),pHistPosition->GetYaxis()->GetXmin(),
                pHistPosition->GetXaxis()->GetXmin(),0);
    ll.DrawLine(pHistPosition->GetXaxis()->GetXmax(),pHistPosition->GetYaxis()->GetXmin(),
                pHistPosition->GetXaxis()->GetXmax(),0);
    ll.DrawLine(pHistPosition->GetXaxis()->GetXmin(),pHistPosition->GetYaxis()->GetXmax(),
                pHistPosition->GetXaxis()->GetXmin(),1);
    ll.DrawLine(pHistPosition->GetXaxis()->GetXmax(),pHistPosition->GetYaxis()->GetXmax(),
                pHistPosition->GetXaxis()->GetXmax(),1);
    ll.DrawLine(pHistPosition->GetXaxis()->GetXmax(),0,0,0);
    ll.DrawLine(pHistPosition->GetXaxis()->GetXmax(),1,0,1);
    ll.DrawLine(pHistPosition->GetXaxis()->GetXmin(),0,-9,0);
    ll.DrawLine(pHistPosition->GetXaxis()->GetXmin(),1,-9,1);
    ll.DrawLine(-9,-1,0,-1);
    ll.DrawLine(-9,2,0,2);
    ll.DrawLine(-9,1,-9,2);
    ll.DrawLine(-9,-1,-9,0);
    ll.DrawLine(0,-1,0,0);
    ll.DrawLine(0,1,0,2);
                
    tt1.Clear();
    tt2.Clear();
    ttT.Clear();
    ttB.Clear();
    ttM.Clear();

    if(normalize)
    {
      tt1.AddText(Form("Total Rate:  %.1E Hz",rate_top+rate_bot));
      tt2.AddText(Form("Total Rate:  %.1f kHz",(rate_top+rate_bot)/1000.0));
      ttT.AddText(Form("%.2f kHz",rate_top/1000.0));
      ttB.AddText(Form("%.2f kHz",rate_bot/1000.0));
      ttM.AddText(Form("MAX SINGLE CRYSTAL = %.2f kHz",(float)max/1000));
    }
    else
    {
      tt1.AddText(Form("Total Count:  %.1E",rate_top+rate_bot));
      ttM.AddText(Form("MAX SINGLE CRYSTAL = %.2f count",(float)max));
    }
    tt1.Draw();
    tt2.Draw();
    ttT.Draw();
    ttB.Draw();
    ttM.Draw();
    pCanvas->GetCanvas()->Modified();
    pCanvas->GetCanvas()->Update();
  }

  void UpdateEnergyHistogram(float scale, Bool_t normalize)
  {
    unsigned int buft[1024], bufb[1024];
    float val;

    pM->BlkReadReg32(HpsMon_HistEnergyT, buft, 1024, CRATE_MSG_FLAGS_NOADRINC);
    pM->BlkReadReg32(HpsMon_HistEnergyB, bufb, 1024, CRATE_MSG_FLAGS_NOADRINC);
    
    pCanvas->GetCanvas()->cd(3)->cd(1);
    pHistEnergyT->Reset();
    pHistEnergyB->Reset();

    if(normalize)
    {
      pHistEnergyT->GetYaxis()->SetTitle("Hz");
      pHistEnergyB->GetYaxis()->SetTitle("Hz");
    }
    else
    {
      pHistEnergyT->GetYaxis()->SetTitle("Counts");
      pHistEnergyB->GetYaxis()->SetTitle("Counts");
    }
    
    for(int i = 0; i < 1024; i++)
    {
      val = (float)bufb[i];
      if(normalize) val *= scale;
      pHistEnergyT->Fill(8*i, val);

      val = (float)buft[i];
      if(normalize) val *= scale;
      pHistEnergyB->Fill(8*i, val);
    }

    pCanvas->GetCanvas()->Modified();
    pCanvas->GetCanvas()->Update();
  }

  void UpdateNHitsHistogram(float scale, Bool_t normalize)
  {
    unsigned int buft[16], bufb[16];
    float val;

    pM->BlkReadReg32(HpsMon_HistNHitsT, buft, 16, CRATE_MSG_FLAGS_NOADRINC);
    pM->BlkReadReg32(HpsMon_HistNHitsB, bufb, 16, CRATE_MSG_FLAGS_NOADRINC);
    
    pCanvas->GetCanvas()->cd(3)->cd(2);
    pHistNHitsT->Reset();
    pHistNHitsB->Reset();

    if(normalize)
    {
      pHistNHitsT->GetYaxis()->SetTitle("Hz");
      pHistNHitsT->GetYaxis()->SetTitle("Hz");
    }
    else
    {
      pHistNHitsB->GetYaxis()->SetTitle("Counts");
      pHistNHitsB->GetYaxis()->SetTitle("Counts");
    }
    
    for(int i = 0; i < 16; i++)
    {
      val = (float)buft[i];
      if(normalize) val *= scale;
      pHistNHitsT->Fill(i, val);

      val = (float)bufb[i];
      if(normalize) val *= scale;
      pHistNHitsB->Fill(i, val);
    }

    pCanvas->GetCanvas()->Modified();
    pCanvas->GetCanvas()->Update();
  }

  void UpdateScalers(Bool_t normalize)
  {
    double singles_pass[2], singles_tot[2];
    double pairs_pass[2], pairs_sumpass[2], pairs_diffpass[2];
    double pairs_edpass[2], pairs_coplanarpass[2], pairs_triggerpass[2];
    double trig1, busy, busycycles, p2lvdsout[6], trigbusy[6];
    double led, cosmic;
    double ref, sysclk;
    static unsigned int ti_live_last = 0, ti_busy_last = 0;
    unsigned int ti_live, ti_busy;
    unsigned int ti_live_delta, ti_busy_delta;
    double ti_dead_time;
/*
    pM->WriteReg32(Ti_reset, 1<<24);

    ti_live = pM->ReadReg32(Ti_LiveTime);
    ti_live_delta = ti_live - ti_live_last;
    ti_live_last = ti_live;

    ti_busy = pM->ReadReg32(Ti_BusyTime);
    ti_busy_delta = ti_busy - ti_busy_last;
    ti_busy_last = ti_busy;

    ti_dead_time = 100.0 * ((double)ti_busy_delta) / ((double)(ti_busy_delta+ti_live_delta));
    
    pM->WriteReg32(HpsScaler_Disable, 1);
    singles_pass[0] = (double)pM->ReadReg32(HpsSingles0_Pass);
    singles_tot[0] = (double)pM->ReadReg32(HpsSingles0_Tot);
    singles_pass[1] = (double)pM->ReadReg32(HpsSingles1_Pass);
    singles_tot[1] = (double)pM->ReadReg32(HpsSingles1_Tot);
    pairs_pass[0] = (double)pM->ReadReg32(HpsPairs0_Pass);
    pairs_sumpass[0] = (double)pM->ReadReg32(HpsPairs0_SumPass);
    pairs_diffpass[0] = (double)pM->ReadReg32(HpsPairs0_DiffPass);
    pairs_edpass[0] = (double)pM->ReadReg32(HpsPairs0_EDPass);
    pairs_coplanarpass[0] = (double)pM->ReadReg32(HpsPairs0_CoplanarPass);
    pairs_triggerpass[0] = (double)pM->ReadReg32(HpsPairs0_TriggerPass);
    pairs_pass[1] = (double)pM->ReadReg32(HpsPairs1_Pass);
    pairs_sumpass[1] = (double)pM->ReadReg32(HpsPairs1_SumPass);
    pairs_diffpass[1] = (double)pM->ReadReg32(HpsPairs1_DiffPass);
    pairs_edpass[1] = (double)pM->ReadReg32(HpsPairs1_EDPass);
    pairs_coplanarpass[1] = (double)pM->ReadReg32(HpsPairs1_CoplanarPass);
    pairs_triggerpass[1] = (double)pM->ReadReg32(HpsPairs1_TriggerPass);
    
    trig1 = (double)pM->ReadReg32(HpsScaler_Trig1);
    busy = (double)pM->ReadReg32(HpsScaler_Busy);
    busycycles = (double)pM->ReadReg32(HpsScaler_BusyCycles);
    p2lvdsout[0] = (double)pM->ReadReg32(HpsScaler_P2LVDSOut0);
    p2lvdsout[1] = (double)pM->ReadReg32(HpsScaler_P2LVDSOut1);
    p2lvdsout[2] = (double)pM->ReadReg32(HpsScaler_P2LVDSOut2);
    p2lvdsout[3] = (double)pM->ReadReg32(HpsScaler_P2LVDSOut3);
    p2lvdsout[4] = (double)pM->ReadReg32(HpsScaler_P2LVDSOut4);
    p2lvdsout[5] = (double)pM->ReadReg32(HpsScaler_P2LVDSOut5);
    trigbusy[0] = (double)pM->ReadReg32(HpsScaler_TrigBusy0);
    trigbusy[1] = (double)pM->ReadReg32(HpsScaler_TrigBusy1);
    trigbusy[2] = (double)pM->ReadReg32(HpsScaler_TrigBusy2);
    trigbusy[3] = (double)pM->ReadReg32(HpsScaler_TrigBusy3);
    trigbusy[4] = (double)pM->ReadReg32(HpsScaler_TrigBusy4);
    trigbusy[5] = (double)pM->ReadReg32(HpsScaler_TrigBusy5);
    
    led = (double)pM->ReadReg32(HpsScaler_Led);
    cosmic = (double)pM->ReadReg32(HpsScaler_Cosmic);
  
    sysclk = (double)pM->ReadReg32(HpsScaler_Sysclk50);
        
    pM->WriteReg32(HpsScaler_Disable, 0);

    if(sysclk <= 0.0)
    {
      printf("Error: UpdateScalers() ref not valid - normalization will not be done\n");
      normalize = kFALSE;
    }
    else
    {
      ref = (50.0E6/501.0) / sysclk;

      singles_pass[0] *= ref;
      singles_tot[0] *= ref;
      singles_pass[1] *= ref;
      singles_tot[1] *= ref;
      pairs_pass[0] *= ref;
      pairs_sumpass[0] *= ref;
      pairs_diffpass[0] *= ref;
      pairs_edpass[0] *= ref;
      pairs_coplanarpass[0] *= ref;
      pairs_triggerpass[0] *= ref;
      pairs_pass[1] *= ref;
      pairs_sumpass[1] *= ref;
      pairs_diffpass[1] *= ref;
      pairs_edpass[1] *= ref;
      pairs_coplanarpass[1] *= ref;
      pairs_triggerpass[1] *= ref;
      
      trig1 *= ref;
      busy *= ref;
      busycycles = ref * busycycles / 250.0E6;
      p2lvdsout[0] = ref * p2lvdsout[0];
      p2lvdsout[1] = ref * p2lvdsout[1];
      p2lvdsout[2] = ref * p2lvdsout[2];
      p2lvdsout[3] = ref * p2lvdsout[3];
      p2lvdsout[4] = ref * p2lvdsout[4];
      p2lvdsout[5] = ref * p2lvdsout[5];
      trigbusy[0] = ref * trigbusy[0] / 250.0E6;
      trigbusy[1] = ref * trigbusy[1] / 250.0E6; 
      trigbusy[2] = ref * trigbusy[2] / 250.0E6;
      trigbusy[3] = ref * trigbusy[3] / 250.0E6;
      trigbusy[4] = ref * trigbusy[4] / 250.0E6;
      trigbusy[5] = ref * trigbusy[5] / 250.0E6;
      
      led *= ref;
      cosmic *= ref;
    }

    pCanvas->GetCanvas()->cd(1);
    pHistTriggerInfo->Reset();
    pHistTriggerInfo->SetTitle(Form("Trigger Rate Info (DeadTime = %.1f%%)", ti_dead_time));

    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_S0_TOTAL,      singles_tot[0]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_S0_PASS,     singles_pass[0]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_S0_TI,       p2lvdsout[0]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_S1_TOTAL,      singles_tot[1]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_S1_PASS,     singles_pass[1]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_S1_TI,       p2lvdsout[1]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_P0_TOTAL,    pairs_pass[0]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_P0_SUMPASS,    pairs_sumpass[0]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_P0_DIFPASS,    pairs_diffpass[0]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_P0_EDPASS,   pairs_edpass[0]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_P0_COPPASS,    pairs_coplanarpass[0]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_P0_PASS,       pairs_triggerpass[0]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_P0_TI,       p2lvdsout[2]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_P1_TOTAL,    pairs_pass[1]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_P1_SUMPASS,    pairs_sumpass[1]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_P1_DIFPASS,    pairs_diffpass[1]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_P1_EDPASS,   pairs_edpass[1]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_P1_COPPASS,    pairs_coplanarpass[1]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_P1_PASS,       pairs_triggerpass[1]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_P1_TI,       p2lvdsout[3]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_LED,       led);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_COSMIC,      cosmic);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_LED_COSMIC_TI, p2lvdsout[4]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_PULSER_TI,   p2lvdsout[5]);
    pHistTriggerInfo->Fill(VTP_HPS_TRIG_INFO_L1A,       trig1);

    pCanvas->GetCanvas()->Modified();
    pCanvas->GetCanvas()->Update();
*/

/*
    static bool called=0;

    static TPaveText tt_col0(0.0,0.0,0.1,1.0,"NDC");
    static TPaveText tt_col1(0.1,0.0,0.2,1.0,"NDC");
    static TPaveText tt_col2(0.2,0.0,0.3,1.0,"NDC");
    static TPaveText tt_col3(0.3,0.0,0.4,1.0,"NDC");
    static TPaveText tt_col4(0.4,0.0,0.5,1.0,"NDC");
    static TPaveText tt_col5(0.5,0.0,0.6,1.0,"NDC");
    static TPaveText tt_col6(0.6,0.0,0.7,1.0,"NDC");
    static TPaveText tt_col7(0.7,0.0,0.8,1.0,"NDC");
    
    if (!called)
    {
      called=1;

      tt_col0.SetBorderSize(0);
      tt_col1.SetBorderSize(0);
      tt_col2.SetBorderSize(0);
      tt_col3.SetBorderSize(0);
      tt_col4.SetBorderSize(0);
      tt_col5.SetBorderSize(0);
      tt_col6.SetBorderSize(0);
      tt_col7.SetBorderSize(0);

      tt_col0.SetFillColor(kWhite);
      tt_col1.SetFillColor(kWhite);
      tt_col2.SetFillColor(kWhite);
      tt_col3.SetFillColor(kWhite);
      tt_col4.SetFillColor(kWhite);
      tt_col5.SetFillColor(kWhite);
      tt_col6.SetFillColor(kWhite);
      tt_col7.SetFillColor(kWhite);
    }
    
    pCanvasRates->GetCanvas()->cd();

    tt_col0.Clear();
    tt_col1.Clear();
    tt_col2.Clear();
    tt_col3.Clear();
    tt_col4.Clear();
    tt_col5.Clear();
    tt_col6.Clear();
    tt_col7.Clear();
    
    tt_col0.SetTextAlign(12);
    tt_col1.SetTextAlign(12);
    tt_col2.SetTextAlign(12);
    tt_col3.SetTextAlign(12);
    tt_col4.SetTextAlign(12);
    tt_col5.SetTextAlign(12);
    tt_col6.SetTextAlign(12);
    tt_col7.SetTextAlign(12);

    tt_col0.SetTextSize(0.1);
    tt_col1.SetTextSize(0.1);
    tt_col2.SetTextSize(0.1);
    tt_col3.SetTextSize(0.1);
    tt_col4.SetTextSize(0.1);
    tt_col5.SetTextSize(0.1);
    tt_col6.SetTextSize(0.1);
    tt_col7.SetTextSize(0.1);

    tt_col0.AddText("SINGLES TRIGGER 0");
    tt_col0.AddText("tot");
    tt_col0.AddText("pass");
    tt_col0.AddText("trig");
    tt_col0.AddText("deadtime");
    tt_col0.AddText("SINGLES TRIGGER 1");
    tt_col0.AddText("tot");
    tt_col0.AddText("pass");
    tt_col0.AddText("trig");
    tt_col0.AddText("deadtime");

    tt_col1.AddText("");
    tt_col1.AddText(Form("%fHz", singles_tot[0]));
    tt_col1.AddText(Form("%fHz", singles_pass[0]));
    tt_col1.AddText(Form("%fHz", p2lvdsout[0]));
    tt_col1.AddText(Form("%f%c", 100.0*trigbusy[0], '%'));
    tt_col1.AddText("");
    tt_col1.AddText(Form("%fHz", singles_tot[1]));
    tt_col1.AddText(Form("%fHz", singles_pass[1]));
    tt_col1.AddText(Form("%fHz", p2lvdsout[1]));
    tt_col1.AddText(Form("%f%c", 100.0*trigbusy[1], '%'));
        
    tt_col2.AddText("PAIR TRIGGER 0");
    tt_col2.AddText("tot");
    tt_col2.AddText("sumpass");
    tt_col2.AddText("diffpass");
    tt_col2.AddText("edpass");
    tt_col2.AddText("coplanarpass");
    tt_col2.AddText("pass");
    tt_col2.AddText("trig");
    tt_col2.AddText("deadtime");
  
    tt_col3.AddText("");
    tt_col3.AddText(Form("%fHz", pairs_pass[0]));
    tt_col3.AddText(Form("%fHz", pairs_sumpass[0]));
    tt_col3.AddText(Form("%fHz", pairs_diffpass[0]));
    tt_col3.AddText(Form("%fHz", pairs_edpass[0]));
    tt_col3.AddText(Form("%fHz", pairs_coplanarpass[0]));
    tt_col3.AddText(Form("%fHz", pairs_triggerpass[0]));
    tt_col3.AddText(Form("%fHz", p2lvdsout[2]));
    tt_col3.AddText(Form("%f%c", 100.0*trigbusy[2], '%'));

    tt_col4.AddText("PAIR TRIGGER 1");
    tt_col4.AddText("tot");
    tt_col4.AddText("sumpass");
    tt_col4.AddText("diffpass");
    tt_col4.AddText("edpass");
    tt_col4.AddText("coplanarpass");
    tt_col4.AddText("pass");
    tt_col4.AddText("trig");
    tt_col4.AddText("deadtime");
    
    tt_col5.AddText("");
    tt_col5.AddText(Form("%fHz", pairs_pass[1]));
    tt_col5.AddText(Form("%fHz", pairs_sumpass[1]));
    tt_col5.AddText(Form("%fHz", pairs_diffpass[1]));
    tt_col5.AddText(Form("%fHz", pairs_edpass[1]));
    tt_col5.AddText(Form("%fHz", pairs_coplanarpass[1]));
    tt_col5.AddText(Form("%fHz", pairs_triggerpass[1]));
    tt_col5.AddText(Form("%fHz", p2lvdsout[3]));
    tt_col5.AddText(Form("%f%c", 100.0*trigbusy[3], '%'));

    tt_col6.AddText("OTHER TRIGGERS");
    tt_col6.AddText("led");
    tt_col6.AddText("cosmic");
    tt_col6.AddText("trig");
    tt_col6.AddText("pulser");
    tt_col6.AddText("DAQ trigger");
    tt_col6.AddText("trigger deadtime");
    
    tt_col7.AddText("");
    tt_col7.AddText(Form("%fHz", led));
    tt_col7.AddText(Form("%fHz", cosmic));
    tt_col7.AddText(Form("%fHz", p2lvdsout[4]));
    tt_col7.AddText(Form("%fHz", p2lvdsout[5]));
    tt_col7.AddText(Form("%fHz", trig1));
    tt_col7.AddText(Form("%f%c", 100.0*busycycles, '%'));
    
    tt_col0.Draw();
    tt_col1.Draw();
    tt_col2.Draw();
    tt_col3.Draw();
    tt_col4.Draw();
    tt_col5.Draw();
    tt_col6.Draw();
    tt_col7.Draw();

    pCanvasRates->GetCanvas()->Modified();
    pCanvasRates->GetCanvas()->Update();
*/
  }

  void UpdateHistogram(Bool_t bReadout = kTRUE)
  {
    pM->WriteReg32(HpsMon_HistCtrl, 0x00);  // disable histograms

    Bool_t normalize = pButtonNormalize->IsDown();
    
    unsigned int ref = pM->ReadReg32(HpsMon_HistTime);
    float scale = ref;
    if(normalize && (scale <= 0.0))
    {
      printf("SSP cluster histogram reference time invalid. not normalizing data.\n");
      normalize = kFALSE;
    }
    else
    {
      scale = scale * 256.0f / 250.0E6;
      scale = 1.0f / scale;
    }


    UpdateLatencyHistogram(scale, normalize);
    UpdatePositionHistogram(scale, normalize);
    UpdateEnergyHistogram(scale, normalize);
    UpdateNHitsHistogram(scale, normalize);

    pM->WriteReg32(HpsMon_HistCtrl, 0xFFFFFFFF);  // enable histograms

    // scalers
    UpdateScalers(pButtonNormalize->IsDown());
  }

private:

  enum Buttons
  {
    BTN_AUTOUPDATE    = 1002,
    BTN_MANUALUPDATE  = 1003,
    SDR_UPDATETIME    = 1100
  };
  
  volatile unsigned int *HpsMon_HistCtrl;
  volatile unsigned int *HpsMon_HistSel;
  volatile unsigned int *HpsMon_HistTime;
  volatile unsigned int *HpsMon_HistPositionT;
  volatile unsigned int *HpsMon_HistPositionB;
  volatile unsigned int *HpsMon_HistEnergyT;
  volatile unsigned int *HpsMon_HistEnergyB;
  volatile unsigned int *HpsMon_HistNHitsT;
  volatile unsigned int *HpsMon_HistNHitsB;
  volatile unsigned int *HpsMon_HistLatencyT;
  volatile unsigned int *HpsMon_HistLatencyB;
  volatile unsigned int *HpsMon_HistHodoT;
  volatile unsigned int *HpsMon_HistHodoB;

  ModuleFrame       *pM;

  TTimer          *pTimerUpdate;

  TRootEmbeddedCanvas *pCanvas;
  TRootEmbeddedCanvas *pCanvasRates;

  TH1F            *pHistLatencyT, *pHistLatencyB;
  TH1F            *pHistEnergyT, *pHistEnergyB;
  TH1F            *pHistNHitsT, *pHistNHitsB;
  TH2F            *pHistPosition;
  TH1F            *pHistTriggerInfo;

  TGSlider          *pSliderUpdateTime;

  TGTextButton      *pButtonAutoUpdate;
  TGTextButton      *pButtonManualUpdate;
  TGTextButton      *pButtonNormalize;

  TGComboBox        *pComboHistSrc;
};

#endif
