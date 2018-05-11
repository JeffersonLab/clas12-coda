
/* daqmon.cc */

#include <sstream>
#include <sys/time.h>
#include <math.h>

#include "daqMainFrame.h"
#include "daqmon.h"
#include "hist_lib.h"
#include "TPaveStats.h"

daqPad*  pad_find(const char *name, daqTab* tab);
daqPad*  pad_find_id(int id, daqTab* tab);

//-------------------------
//---  monitor GLobals  ---
//-------------------------
struct timeval tvA[10], tvB[10]; 
daqMainFrame *dmf;
TCanvas  *c1,*c2,*c3,*c4;
int HIST_UPDATED=0;
int SHOW_TAB=0;
extern int button_EXIT,button_PAUSE;
int button_EXIT=0,button_PAUSE=0;
//vector<hid_t> hids;

daqPad *pad;
daqTab *tab;
vector<daqTab*> tabs;

//--------------------- root client test ------------------------------
extern int CurrentTAB;
int CurrentTAB=0;

vector<int> tab_id;
//int tabBCALr=-1,tabBCALp=-1,tabBCALs=-1,tabFCALr=-1,tabFCALp=-1,tabTOFr=-1,tabSTPSCr=-1,tabTRIGr=-1,tabTAGr=-1,tabTAGd=-1;

TList *ALL_1D_HIST;
TList *ALL_2D_HIST;
TList *ALL_GH_HIST;



#define MAXSTRLNG 100
#define NFLDHIST 5
#define NFLDTMLN 5
#define NFLDGRAF 7
static int ntabs;

int
ReadConfig(char *fileconf)
{
  FILE *fd;
  Int_t ix, iy, i1, i2, i3, i4;
  Int_t ikey, number, norm, ih, iv, iprompt;
  Int_t nread=0, nline, comment, i, j, k;
  Int_t itmlnbin, tmlnalarm;
  Float_t tmln1, tmln2;
  ULong_t tint;
  char str[MAXSTRLNG], str_tmp[MAXSTRLNG], *s, nm[10];

  ntabs = 0;

  if((fd=fopen(fileconf,"r")) != NULL)
  {
    nline = 0;
    do
    {
      /* read one line of config file as character string */
      s = fgets(str,MAXSTRLNG,fd);
      nline++;
      if(s == NULL)
      {
        if(ferror(fd)) /* check for errors */
        {
          printf("ReadConfig(): Error reading config file, line # %i\n",nline);
          perror("ReadConfig(): Error in fgets()");
        }
        if(feof(fd))   /* check for End-Of-File */
        {
          printf("ReadConfig(): Found end of config file, line # %i\n",nline);
        }
        break;         /* Stop reading */
      }
      str[strlen(str)-1] = 0;
      comment = (*str==0 || *str=='#' || *str=='!' || *str==';');
      //printf("String %i: >%s<\n",nline,str);
      if(!comment)
      {
        ikey = *((unsigned long *)str);
        if(!strncmp(str,"CMON_TAB",8))
        {
          ntabs ++;
          printf("Found 'CMON_TAB' key - ntabs set to %d\n",ntabs);
	  nread = sscanf(str,"%*s %s %i %i",str_tmp,&ix,&iy);
          tab  = new daqTab(dmf,str_tmp,ix,iy);
          tabs.push_back(tab);
          printf("Created TAB id=%d, total=%d \n",tab->ID,dmf->get_n_tabs());
          tab_id.push_back(tab->ID);
        }
        else if(!strncmp(str,"CMON_PAD",8))
        {
          printf("Found 'CMON_PAD' key\n");
	  nread = sscanf(str,"%*s %s %i %i %i %i",str_tmp,&i1,&i2,&i3,&i4);
          /*daqPad(char* hname, int pad, int dim, int Log, int stat, char* hopt, char* dopt);*/
	  /*Check if the pad with id i1 is already here, in this tab*/
	  if (tab->hasPad(i1)){
	    pad = tab->getPad(i1); 
	    pad->addHisto(str_tmp,"","");
	    
	  }
          else{	    
	    pad = new daqPad(str_tmp,i1,i2,i3,i4,"","");
	    tab->pads.push_back(pad);
	  
	  }

	  //    tab->pads.push_back(pad);
        }
        else
        {
          printf("Found unknown string >%s< - ignore.\n",str);
        }
	  }
      else
      {
		/*printf("Found comment at line # %i\n",nline)*/;
	  }		  
    } 
	while(1);

    fclose(fd);
  }
  else
  {
    printf("ReadConfig(): cannot open config file >%s< - exit\n",fileconf);
    exit(0);
  }

  return(ntabs);
}





static char conffile[256];

//==============================================================================
int
main(int argc, char **argv)
{
  int npad, itab;

  if(argc == 1)
  {
    sprintf(conffile,"%s/daqmon/daqmon.cnf",getenv("CLON_PARMS"));
    printf("Use default config file >%s<\n",conffile);
  }
  else if(argc == 2)
  {
    sprintf(conffile,"%s",argv[1]);
    printf("Use specified config file >%s<\n",conffile);
  }
  else
  {
    printf("Unknown number of args=%d - exit\n",argc);
    exit(0);
  }

  TApplication theApp("App", NULL, NULL);

  //---  Create Main Frame  ---
  dmf = new daqMainFrame(gClient->GetRoot(), 900, 700);
  ALL_GH_HIST=new TList();
  TGraph* gr;

  ReadConfig(conffile);


  /**------------------------------------------------------------------------**/
  /**------------------------------------------------------------------------**/
  /**---                Create Tabs                                       ---**/
  /**------------------------------------------------------------------------**/
  /**------------------------------------------------------------------------**/

  char hist_name[256];



#if 0
  pad = new daqPad("iocpulser1:DISCR_Scalers",2,2,1,0," ","colz");  tab->pads.push_back(pad);
  pad = new daqPad("iocpulser2:DISCR_Scalers",3,2,1,0," ","colz");  tab->pads.push_back(pad);
  //tab->canvas->cd(3);   gPad->AddExec("rate_dsc",".x rate_dsc.C++");
  pad = new daqPad("ioctagctrl:DISCR_Scalers",4,2,1,0," ","colz");  tab->pads.push_back(pad);

  //  graph Cosmic
  pad = new daqPad("Trig1:Cosmic",5,1,0,0," ","colz"); tab->pads.push_back(pad);  
  gr = pad->timeGraph("dummy",600,3,5,1);   tab->canvas->cd(4);   gPad->SetGridy(1);
  if (gr) ALL_GH_HIST->Add(gr);
  //  graph 
  pad = new daqPad("Trig1:Rate_3",6,1,0,0," ","colz"); tab->pads.push_back(pad);  
  gr = pad->timeGraph("dummy",600,3,5,4);   tab->canvas->cd(6);   gPad->SetGridy(1);
  if (gr) ALL_GH_HIST->Add(gr);
#endif





  //------------------------------
  //----  tab:   ERRORS        ---
  //------------------------------
  tab=new daqTab(dmf,"ERR",1,1);
  tabs.push_back(tab);
  printf("Created TAB id=%d, total=%d \n",tab->ID,dmf->get_n_tabs());
  //dmf->fTab->SetEnabled(tab->ID,0);
  pad = new daqPad("ROC_Errors",1,2,0,1,"i","text");  tab->pads.push_back(pad);
  //pad->Maximum=10;
  tab->canvas->cd(1);   gPad->SetGridx(1);   gPad->SetGridy(1);

  //------------------------------------------------------------------------
  printf("Numb Tabs = %d \n",dmf->get_n_tabs());
  printf("Current Tab = %d \n",dmf->fTab->GetCurrent());
  printf("Size of tabs = %d \n",tabs.size());


  //-------------------------------------------------------------------------------
  //                     Book summary hists 
  //-------------------------------------------------------------------------------

  //----------------------  ERROR hist 2D ----------------------
  TRandom *rndm = new TRandom();
  
   const Int_t nx = 9;
   const Int_t ny = 53;
   char *errors[nx]  = {"Time mismatch","DMA timeout","tiBusy","Error_4","Error_5","Error_6","Error_7","Error_8","Error_9"};
   char *crates[ny] = 
{  "FCAL1",  "FCAL2",  "FCAL3",  "FCAL4",  "FCAL5",  "FCAL6",  "FCAL7",  "FCAL8",  "FCAL9",  "FCAL10",  "FCAL11",  "FCAL12",
   "BCAL1","BCAL2","BCAL3","BCAL4","BCAL5","BCAL6","BCAL7","BCAL8","BCAL9","BCAL10","BCAL11","BCAL12",
   "FDC1","FDC2","FDC3","FDC4","FDC5","FDC6","FDC7","FDC8","FDC9","FDC10","FDC11","FDC12","FDC13","FDC14",
   "CDC1","CDC2","CDC3","CDC4",
   "TOF1","TOF2",
   "TRIG1","TRIG2","STPSC1", "ST",
   "TAGM1", "TAGH1", "TAGMH",  "PS1", "PS2"
};
   //c1->SetGrid();
   //c1->SetLeftMargin(0.15);
   //c1->SetBottomMargin(0.15);
   TH2F *GHerrors = new TH2F("ROC_Errors","ROC_Errors",3,0,3,2,0,2);
#if ROOT_VERSION_CODE > ROOT_VERSION(6,0,0)
   GHerrors->SetCanExtend(TH1::kXaxis);
#else
   GHerrors->SetBit(TH1::kCanRebin);
#endif
   GHerrors->SetStats(0);
   rndm->SetSeed();
   for (Int_t ix=0;ix<nx;ix++) {
     for (Int_t iy=0;iy<ny;iy++) {
       double rx = int(rndm->Rndm()*2.);
       //printf("%d %d rx=%f\n",ix,iy,rx);
       //GHerrors->Fill(crates[iy],errors[ix],rx);
       GHerrors->Fill(errors[ix],crates[iy],0);
     }
   }
   GHerrors->SetMarkerSize(0.8);
   GHerrors->SetMarkerColor(2);
   GHerrors->LabelsDeflate("X");
   GHerrors->LabelsDeflate("Y");
   GHerrors->LabelsOption("d");
   GHerrors->GetXaxis()->SetLabelSize(0.03);
   GHerrors->GetYaxis()->SetLabelSize(0.02);
   //GHerrors->Draw("text");
   ALL_GH_HIST->Add(GHerrors);
   
   //----------------------  Time Graph ----------------------
   /*
   const Int_t ngr = 600;
   const Int_t kNMAX = 10000;
   Double_t *Xgr = new Double_t[kNMAX];
   Double_t *Ygr = new Double_t[kNMAX];
   Int_t cursor_gr = 0;
   TGraph *graph1 = new TGraph(ngr); graph1->SetName("DISCR_Rate");
   graph1->SetMarkerStyle(20);
   graph1->SetMarkerColor(kBlue);
   graph1->SetMarkerSize(0.5);
   Double_t xgr = 0;
   printf("Time now=%f\n",xgr);
   ALL_GH_HIST->Add(graph1);
   int np=0;
   for (Int_t i=0;i<ngr;i++) {
     Xgr[i] = xgr-ngr+i;
     Ygr[i] = 0;
   }
   graph1->GetXaxis()->SetTitle("time, sec");
   graph1->GetYaxis()->SetTitle("rate, Hz");
   //graph1->GetXaxis()->SetTimeDisplay(1);
   time_t start_time = time(0);
   start_time = 3600* (int)(start_time/3600);
   gStyle->SetTimeOffset(start_time);
   graph1->GetXaxis()->SetTimeFormat("y. %Y %F2000-01-01 00:00:00");
   */






   /**------------------------------------------------------------------------**/
   /**------------------------------------------------------------------------**/
   /**---                Start threads                                     ---**/
   /**------------------------------------------------------------------------**/
   /**------------------------------------------------------------------------**/


  //---------------- root server -------------------------------------------


  static TThread *thr_hist_trig[20];
  SRV_PARAM srv_trig[20];



  /* starting one thread per tab; if want one per pad, do lop over ii and s_pad++ */
  for(itab=0; itab<ntabs; itab++)
  {
    int CTAB = tab_id[itab];
    int s_pad = 1;
    for(int ii=6; ii<=6; ii++)
    {
      sprintf(srv_trig[ii].REM_HOST,"clondaq%d",ii);

      srv_trig[ii].tab = CTAB;
      srv_trig[ii].canvas = tabs[CTAB]->canvas;
      srv_trig[ii].pad = s_pad; /* s_pad++; */

      sprintf(srv_trig[ii].thread_name,"hist_trig%d_thread",1);
      printf("CREATING hist_client2 %d\n",ii);
      thr_hist_trig[ii] = new TThread(srv_trig[ii].thread_name, hist_client2,(void*) &srv_trig[ii]);
      printf("STARTING hist_client2 %d\n",ii);
      thr_hist_trig[ii]->Run();
      printf("STARTED hist_client2 %d\n\n",ii);
    }
  }




  TThread::Ps();
  printf("\n------------>  all THREADS are STARTED  <------------------\n\n");
  sleep(3);

  gStyle->SetOptFit(11111);




  /**-------------------------------------------------------------------------------**/
  /**-------------------------------------------------------------------------------**/
  /**--                     Main Loop                                             --**/
  /**-------------------------------------------------------------------------------**/
  /**-------------------------------------------------------------------------------**/

  while(1)
  {
    int ID=dmf->CurrentTAB;
    CurrentTAB=dmf->CurrentTAB;
    printf("update_Tab:: current tab = %d\n",ID);

    char Lname[64]; 
    int RunNo=-1;
    int Nevents=-1;

printf("000\n");fflush(stdout);

#if 0
    sprintf(Lname, "Run: %d",RunNo);
    dmf->Lrunnum->ChangeText(Lname);
    sprintf(Lname, "Ev:  %d",Nevents);
    dmf->Levtnum->ChangeText(Lname);
#endif

    //-------------------------------------------------------------------------------------------------------
    //                               TI Busy 
    //-------------------------------------------------------------------------------------------------------
    char CBIN[20];
    int tibusy;
    int ixbin=0;
    //for (int ix=0; ix<nx; ix++) if (!strcmp(errors[ix],"tiBusy") ) { ixbin=ix+1; break; }


	printf("111\n");fflush(stdout);


TThread::Lock();

    for (int ii=0; ii<1; ii++)  //-- TRIG --
    {
      sprintf(CBIN,"TRIG%d",ii);
      int bf=0; for (int iy=0; iy<ny; iy++) if (!strcmp(crates[iy],CBIN) ) { bf=iy+1; break; }
      //printf(" %s %d %d\n",CBIN,ixbin,bf);
      if (bf>0) GHerrors->SetBinContent(ixbin,bf,tibusy);
    }

TThread::UnLock();


	printf("222\n");fflush(stdout);


    //-------------------------------------------------------------------------------------------------------
    //                                 Draw Local hists
    //-------------------------------------------------------------------------------------------------------
    int id,nf=0; 
    TObject *obj;
    TIter next0(ALL_GH_HIST);
    while ((obj = next0()))  
    {
      TH1 *hist = (TH1 *) obj;
      TGraph *graph = (TGraph *) obj;
      daqPad* Gpad_src=NULL;
      TH2* Gh2_src=NULL;
      
      double padscale=sqrt((double)tabs[CurrentTAB]->pads.size());
      gStyle->SetTitleSize(0.03+0.01*padscale,"t");    //-- TITLE size as "fraction of the pad" ---
      gStyle->SetStatW(0.1+0.05*padscale);   // gStyle->SetStatX(0.4); 
      gStyle->SetStatH(0.05+0.025*padscale);  gStyle->SetStatY(0.9);
      //gStyle->SetStatH(-1.);  gStyle->SetStatY(0.9);
      
      daqPad* pad = pad_find(hist->GetName(),tabs[CurrentTAB]);
      if (pad)
	{
	  int n=pad->findHist(hist->GetName());
	  TThread::Lock();
	  tabs[CurrentTAB]->canvas->cd(pad->Pad);
	  printf("qui! \n");fflush(stdout);
	  if (pad->GraphSize==0) {          //----  1D hist -no statistic ----
	    printf("1D hist name=:%s:  stat=%d\n", hist->GetName(),pad->Stat);
	    hist->SetStats(pad->Stat);  
	  }
	  else if (pad->GraphSize>0)  {    //-----  Time Graph -------------
	    Gpad_src = pad_find_id(pad->GraphSrc,tabs[CurrentTAB]); // pad_find("roctof3:DISCR_Scalers",tabs[CurrentTAB]);
	    if (!Gpad_src) { printf("Graph:: src pad not found \n"); }
	    else {
	      
	      Gh2_src=Gpad_src->hist2;
	      if (!Gh2_src)  { printf("Graph:: src hist not found \n"); }
	      else { 
		printf("Graph:: TRY: src=%d, bin=(%d,%d), ptr=%p .... ",pad->GraphSrc,pad->GraphXbin, pad->GraphYbin, Gh2_src);
		double c;
		if (Gh2_src) 
		  c = Gh2_src->GetBinContent(pad->GraphXbin,pad->GraphYbin);  
		
		if (pad->GraphSrc==0) {    printf("Graph:: fill src=%d \n",pad->GraphSrc);
		} else {
		  unsigned long tm=time(NULL); 
		  if (Gh2_src) pad->fillGraph2((double)tm,Gh2_src->GetBinContent(pad->GraphXbin,pad->GraphYbin));
		}
	      }
	    }
	  }
	  
	  if (pad->Maximum>0) hist->SetMaximum(pad->Maximum);
	  if (pad->LogY>0) gPad->SetLogy();
	  
	  if (pad->GraphSize>0  && Gh2_src)  {  //------  Time Graph ----
	    gPad->Clear();
	    if (pad->GraphSrc>0)  {
	      graph=pad->graph;
	    }
	    graph->GetXaxis()->SetTitle("time, sec");
	    graph->GetYaxis()->SetTitle("rate, Hz");
	    graph->GetXaxis()->SetTitleSize(0.06);
	    graph->GetYaxis()->SetTitleSize(0.06);
	    graph->GetXaxis()->SetLabelSize(0.06);
	    graph->GetYaxis()->SetLabelSize(0.06);
	    graph->GetXaxis()->SetTitleOffset(0.8f);
	    graph->GetYaxis()->SetTitleOffset(0.9f);
	    
	    graph->Draw("alp");
	     
	    graph->GetXaxis()->SetTimeDisplay(1);
	    gPad->Update();
	  } else {                    //------   1D  histogram  -------
	    int n=pad->findHist(hist->GetName());
	    hist->SetLineColor(n+1); //A.C. for multiple histograms on same pad
	    hist->Draw(pad->Dopt[n].c_str());	
	    gPad->Update();	  
	    if (pad->Stat==0){
	      hist->SetStats(0);
	      gPad->Update();  
	    }
	    else{
	      hist->SetStats(1);
	      TPaveStats *ps=(TPaveStats*)hist->GetListOfFunctions()->FindObject("stats");
	      ps->SetOptStat(pad->Stat);
	      gPad->Modified();
	      gPad->Update();
	    }


	  }
	  
	  TThread::UnLock();
	  tabs[CurrentTAB]->modified++;
	}
    }   
    
    TThread::Lock();
    tabs[ID]->modified=0;
    tabs[ID]->canvas->Modified();
    tabs[ID]->canvas->Update();
    TThread::UnLock();
    
    printf("-------------------------->  main:: plot done , update canvas done, usleep/process    \n");
    //----- sleep -----
    int TIME=1*1000000; //-- sec --    
    gettimeofday(&tvA[0], NULL);
    time_t t1=time(NULL);
    while (1)
      {
	gettimeofday(&tvB[0], NULL); int idiff=tvB[0].tv_usec - tvA[0].tv_usec +  1000000 * (tvB[0].tv_sec - tvA[0].tv_sec);
	time_t t2=time(NULL);
	button_PAUSE=dmf->v_PAUSE;
	if ( (idiff>TIME  || (dmf->CurrentTAB != ID) || (tabs[dmf->CurrentTAB]->modified>3) ) && button_PAUSE==0 )
	  {
	    break;
	  }
	
	TThread::Lock();
	gSystem->ProcessEvents();
	TThread::UnLock();
	
	if (dmf->v_EXIT)
	  {
	    button_EXIT=1;
	    break;
	  }
	usleep(100);
      }
    if (button_EXIT) break;
  }

  printf("Button EXIT=%d\n",button_EXIT);
  
  exit(0);
}
