#ifndef dc_scalers_app_H
#define dc_scalers_app_H

/* dc_scalers_ipc.h */

class dc_scalers_app : public TGMainFrame
{
  private:
    int connect_to_server();
    int read_scalers();

    unsigned int dc_scalers[6][3][14][96];
    unsigned int dc_ref[6][3][14];
    Double_t dc_tot[6][3][2];

    void draw_scalers();
    void normalize_scalers();
    void sum_scalers();
    void Honeycomb(TH2Poly *pH, Double_t xstart, Double_t ystart, Double_t a);
    void SetThreshold(unsigned int thr);
    Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t);
    Bool_t HandleTimer(TTimer *t);

    TGSlider *pSliderThreshold;
    TGLabel *pLabelThreshold;
    TTimer *pTimerUpdate;
    CrateMsgClient *crate_dc[6][3];
    TCanvas *pTC;

 public:
    TMutex mutex;
    TH2Poly *pH[6];
    TRootEmbeddedCanvas *pCanvas;
    TH1F *phist1[6];
    TH2F *phist2[6];

  public:
    dc_scalers_app(const TGWindow *p, UInt_t w, UInt_t h);
    ~dc_scalers_app();

    void button_init();
    void DoExit();
    void button_Save();
    void refresh_scalers();
    void hist2root(Hist hist);
};

#endif
