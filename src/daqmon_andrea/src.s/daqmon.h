
/* daqmon.h */

#include <vector>
#include <strings.h>
using namespace std;

//-------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------
class daqPad {
private:
public:
  
  vector<string> Hname;
  vector<string> Hopt;
  vector<string>  Dopt;
  int Dim;
  int Pad;
  int LogY;
  int LogZ;
  int ColZ;
  int Stat;
  int Marker;
  int MColor;
  double MSize;
  double TSize;
  double ZSize;
  double Maximum;
  double Minimum;
  double RightMargin;
  //----  Graph ----
  TGraph   *graph;
  int GraphSize;
  char Gname[256];
  vector<int> GraphX;
  vector<int> GraphY;
  int GraphSrc;
  int GraphXbin;
  int GraphYbin;
  unsigned int Gtime;
  time_t t0;
  //-------------------
  TH1 *hist1;
  TH2 *hist2;
  TProfile *prof;
  int update_time;
  struct timeval Time;
  
  //--------------------------------------------------------------------------------
  daqPad(char* hname, int pad, int dim,  int Log, int stat, char* hopt, char* dopt) {
    
    string hnameTmp;
    string hoptTmp;
    string doptTmp;

    hnameTmp=string(hname);
    hoptTmp=string(hname);
    doptTmp=string(dopt);

    this->Dim=dim;
    Stat=stat;
    Pad=pad;
    Maximum=0;   Minimum=0; 
    Marker=0;    MColor=0;    MSize=0; TSize=0; ZSize=0;
    graph=NULL;     GraphXbin=0; GraphYbin=0; GraphSrc=0; GraphSize=0; Gtime=0;
    hist1=NULL;hist2=NULL;prof=NULL;
    if (this->Dim==1) {
      LogY=Log;
      LogZ=0;
      ColZ=0;
      RightMargin=0;
   
		     
    } else {
      LogY=0;
      LogZ=Log;
      ColZ=1;
      RightMargin=0.35;
      doptTmp=doptTmp+"COLZ";
    }

    Hname.push_back(hnameTmp);
    Hopt.push_back(hoptTmp);
    Dopt.push_back(doptTmp); 

    printf("Define a new pad for hist = %s opt=%s Dopt=%s Stat=%i Dim=%i me=%p\n",Hname[0].c_str(),Hopt[0].c_str(),Dopt[0].c_str(),Stat,Dim,this);fflush(stdout);
  }
 
  void addHisto(char *hname,char *hopt,char *dopt){
     
    
    string hnameTmp;
    string hoptTmp;
    string doptTmp;

    hnameTmp=string(hname);
    hoptTmp=string(hname);
    doptTmp=string(dopt);
 
  
    if (Dim==1){
      doptTmp=doptTmp+"SAME";
    }
    else{
      doptTmp=doptTmp+"COLZSAME";  
    }
  
    Hname.push_back(hnameTmp);
    Hopt.push_back(hoptTmp);
    Dopt.push_back(doptTmp);



  }
  //--------------------------------------------------------------------------------
  TGraph* timeGraph(char* name,  int size ) {
    GraphSize=size;
    graph = new TGraph(GraphSize);
    GraphX.resize(GraphSize,0); GraphY.resize(GraphSize,0);
    sprintf(Gname,"%s",Hname[0].c_str());
    graph->SetName(Gname);
    t0=time(NULL);
    for (int ti=0; ti<GraphSize; ti++) {
      GraphX[ti]=ti-600;
      GraphY[ti]=0;
      graph->SetPoint(ti,GraphX[ti],GraphY[ti]);
    }
    graph->SetMarkerStyle(20);
    graph->SetMarkerColor(kBlue);
    graph->SetMarkerSize(0.5);

    graph->GetXaxis()->SetTitle("time ");
    graph->GetYaxis()->SetTitle("rate, Hz");
    graph->SetMinimum(0);
    graph->SetTitle(Gname);
    graph->GetXaxis()->SetTimeDisplay(1);
    graph->GetXaxis()->SetLabelSize(0.3);
    time_t start_time = time(NULL);
    //start_time = 3600* (int)(start_time/3600);
    gStyle->SetTimeOffset(start_time);
    //printf("timeGraph:: set time offset: %lu,  time now=%lu\n",start_time,time(0));
    //graph->GetXaxis()->SetTimeFormat("y. %Y %F2000-01-01 00:00:00");

    return graph;
  }
  //--------------------------------------------------------------------------------
  TGraph* timeGraph(char* name,  int size , int src, int chX, int chY ) {
    char name2[128];
    GraphSrc=src; GraphXbin=chX; GraphYbin=chY;
    sprintf(name2,"%s_slot:%d:%d",Hname[0].c_str(),chX,chY);
    TGraph*  gr=timeGraph(name2, size );
    graph->SetTitle(name2);
    return gr;
  }

  int findHist(const char *hname){
    int ret=-1;
    printf("daqPad::findHist called (pad: %p), nHists there: %i\n",this,Hname.size());fflush(stdout);
    for (int ii=0;ii<Hname.size();ii++){
      printf("daqPad::findHist search for %s and try %s \n",hname,Hname[ii].c_str());fflush(stdout);
      if (strncasecmp(Hname[ii].c_str(),hname,strlen(hname))==0){
	return ii;
      }
    }
    return ret;
  }

  //--------------------------------------------------------------------------------
  int fillGraph(double X , double Y ) {
    for (Int_t i=0;i<GraphSize-1;i++) {
      GraphX[i] = GraphX[i+1];
      GraphY[i] = GraphY[i+1];
    }
    GraphX[GraphSize-1] =  Gtime++; //X;
    GraphY[GraphSize-1] = Y;
  }
  //--------------------------------------------------------------------------------
  int fillGraph2(double X , double Y ) {
    //printf("fillGraph2: x=%f y=%f \n",X,Y);
    for (Int_t i=0;i<GraphSize-1;i++) {
      GraphX[i] = GraphX[i+1];
      GraphY[i] = GraphY[i+1];
      graph->SetPoint(i,GraphX[i],GraphY[i]);
    }
    GraphX[GraphSize-1] = X-t0; // Gtime++;   // X;
    GraphY[GraphSize-1] = Y;
    graph->SetPoint(GraphSize-1,GraphX[GraphSize-1],GraphY[GraphSize-1]);
    //printf("tv: sec=%lu  usec=%lu\n",Time.tv_sec,Time.tv_usec);
  }
  //--------------------------------------------------------------------------------
  ~daqPad() {
    delete graph;
  }
};

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
class daqTab {
 private:
 public:
  int ID;
  char name[128];
  int nx,ny, max_pads;
  TCanvas *canvas;
  //  vector<hid_t> hids;
  vector<string> pattern;
  vector<string> system;
  vector<daqPad*> pads;
  int modified;
  int update_time;  //-- sec --
  int Print() {
    char fname[512];
      sprintf(fname,"~/ROOT_HIST/ROOT-%d/%s.pdf",234,name);
      //fMain->c1[Nmmod]->Print(name);
      //sprintf(name,"ROOTTB2008/ROOT-%d/MonitorModule%dPedest.gif",RunNo_local,DEPFET[Nmmod]->ID);
      //mainWin->c1[Nmmod]->Print(name);
  }
  int Divide(int nx0 , int ny0 ) {    
    nx=nx0;
    ny=ny0;
    max_pads=nx*ny;
    canvas->Divide(nx,ny);    
  }
  int Create(daqMainFrame *fMain, /*TGString**/ char*  name) { 
    canvas=fMain->add_tab(name);
    sprintf(name,"%s",name);
    int cID = fMain->fTab->GetCurrent(); 
    //fMain->fTab->SetTab(name,kTRUE);
    //ID=fMain->fTab->GetCurrent(); 
    ID=fMain->fTab->GetNumberOfTabs()-1;
    printf("New TAB ID=(%d,%d) Current=%d\n",ID,fMain->fTab->GetNumberOfTabs()-1,cID);
    //fMain->fTab->SetTab(cID,kTRUE);
    modified=0;
    update_time=1;
  }

  daqTab(daqMainFrame *fMain, /*TGString**/ char*  name) {    
    Create(fMain, name);
    nx=1; ny=1; max_pads=1;
  }
  daqTab(daqMainFrame *fMain, char*  name, int nx0 , int ny0 ) {    
    Create(fMain, name);
    Divide(nx0,ny0);
  }

  int hasPad(int id){
    int ret=0;
    for (int ii=0;ii<pads.size();ii++){
      if (pads[ii]->Pad==id) return 1;
    }
    return ret;
  }

  

  daqPad* getPad(int id){
    daqPad* pad=NULL;
    int ii=this->hasPad(id);
    if (hasPad(id)==0){
      printf("Error no pad on this tab with id $i\n",id);
      return NULL;
    }
    for (int ii=0;ii<pads.size();ii++){
      if (pads[ii]->Pad==id){
	pad=pads[ii];
	break;
      }
    }
    
    printf("getPad returns: ii=%i and pad pointer=%p \n",ii,pad);
    return pad;
  }

  ~daqTab() { }

};
