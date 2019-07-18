
/* MessageActionDAQMONJSON.h */

#ifndef __MESSAGE_ACTION_JSON__
#define __MESSAGE_ACTION_JSON__

#include "MessageAction.h"

#include "hbook.h"
#include "json/json.hpp"
#include <string>

#include <unistd.h>
using json = nlohmann::json;

#ifdef USE_ROOT
#endif

class MessageActionJSON : public MessageAction {

  private:

    static const int NFORMATS=1;
    std::string formats[NFORMATS] = {"json"};
    int formatid;

    int error;
    int debug;
    int done;
    int status;
    int statistics;
    std::string myname;
    int len;
    std::string str;


    int packed;
    Hist hist;
    
    char *hnameLocalSplit;
    char hnameLocal[20];
   


    TH1F *phist1;
    TH2F *phist2;


#ifdef USE_ROOT
	TList *histlist;
#endif

  public:

    MessageActionJSON(){}


#ifdef USE_ROOT
    MessageActionJSON(std::string myname_, int debug_ = 0, TList *histlist_ = NULL)
    {
      myname = myname_;
      done = 0;
      status = 0;
      statistics = 0;
      debug = debug_;
      histlist = histlist_;
      gethostname(hnameLocal,20);
      hnameLocalSplit = strtok(hnameLocal,".");
      
    }
#else
    MessageActionJSON(std::string myname_, int debug_ = 0)
    {
      myname = myname_;
      done = 0;
      status = 0;
      statistics = 0;
      debug = debug_;
      gethostname(hnameLocal,20);
      hNameLocalSplit = strtok(hnameLocal,".");
    

    }
#endif

    ~MessageActionJSON(){}

int check(std::string fmt){
 
  if (debug) printf("\n===== MessageActionDAQMONJSON check: testing fmt >%s< <\n",fmt.c_str());

  
  for(int i=0; i<NFORMATS; i++){
    std::string f = formats[i];
    if( !strncmp(f.c_str(),fmt.c_str(),strlen(f.c_str())) ){
      formatid = i;
      if (debug) printf("MessageActionDAQMONJSON found format: %s in string: %s \n",f.c_str(),fmt.c_str());
      return(1);
    }
  }
  formatid = 0;
  return(0);
}



void
decode(IpcConsumer& recv)
{    
  recv >> str;
  if(debug) printf("\nMessageActionJSON received >%s<\n",str.c_str());fflush(stdout);

  json j3 = json::parse(str);
   
  // special iterator member functions for objects
  for (json::iterator it = j3.begin(); it != j3.end(); ++it)
  {
	std::cout<<it.key()<<" "<<it.value()<<std::endl;
	if (j3[it.key()].is_array())
    {
	  std::vector <double> data = j3[it.key()]; //use a double vector
	  std::string htitle = it.key();
	  htitle = std::string(hnameLocalSplit)+":"+htitle;

	  decodeDefaultHist(htitle, data);
	}
  }
}







    void process()
    {
      // std::cout << "MessageActionJSON: process message" << std::endl;      


#ifdef USE_ROOT
      if(error==0) hist2root();
#endif

    }

    /*Special flag is to handle specific cases (as FT mapping)*/

void
hist2root(int specialFlag=0)
{
  Int_t id, ibinx, ibiny;
  Float_t x, y;
  Float_t *content;
  char title[40];

  printf("hist2root reached, histlist=%p\n",histlist);

  //TObject *obj = NULL;
  if(histlist==NULL)
  {
    return;
  }
  else
  {
    auto obj = histlist->FindObject(hist.title);

  /* if histogram is NOT in histlist, create and put it in, otherwise get pointer to it */
  if(obj!=NULL)
  {
    printf("FOUND obj=%p contains name >%s<\n",obj,hist.title);fflush(stdout);
    if(hist.nbiny==0/*obj->InheritsFrom("TH1F")*/)
	{
      printf("hist2root: found 1D\n");fflush(stdout);
      phist1 = (TH1F *)obj;
      //phist2 = NULL;
	}
    else /*if(obj->InheritsFrom(TH2::Class()))*/
	{
      printf("hist2root: found 2D\n");fflush(stdout);
      //phist1 = NULL;
      phist2 = (TH2F *)obj;
	}
	/*
    else
	{
      printf("UNKNOWN CLASS\n");fflush(stdout);
	}
	*/
  }
  else
  {
    if(hist.nbiny==0)
	{
      printf("hist2root: creating 1D\n");
      phist1 = new TH1F(hist.title, hist.title, hist.nbinx, hist.xmin, hist.xmax);
      histlist->Add(phist1);
	}
    else
	{
      printf("hist2root: creating 2D\n");
      phist2 = new TH2F(hist.title, hist.title, hist.nbinx, hist.xmin, hist.xmax, hist.nbiny, hist.ymin, hist.ymax);
      histlist->Add(phist2);
	}
  } 


  id = hist.id;
  if(hist.nbinx>0)
  {
    printf("hist2root: [id=%d] nbinx=%d nbiny=%d\n",id,hist.nbinx,hist.nbiny);

    if(hist.nbiny == 0) /* 1-dim */
    {
	  
      //phist1->SetEntries(hist.entries+phist1->GetEntries());
      phist1->SetEntries(hist.entries); //these are scalers, hence don't sum with previous data.
      content = phist1->GetArray();
      content[0] += (Float_t)hist.xunderflow;
      content[hist.nbinx+1] += (Float_t)hist.xoverflow;
      for(ibinx=0; ibinx<hist.nbinx; ibinx++)
	  {
        content[ibinx+1] += (Float_t)hist.buf[ibinx];
        //printf("content[%d]=%f\n",ibinx,content[ibinx+1]);
	  }
	  
	  /*
      for(ibinx=0; ibinx<hist.nbinx; ibinx++)
	  {
        printf("content[%d]=%f\n",ibinx,(float)hist.buf[ibinx]);
        phist1->Fill((float)ibinx, (float)hist.buf[ibinx]);
	  }
	  */

    }
    else /* 2-dim */
    {
	  /*
      phist2->SetEntries(hist.entries+phist2->GetEntries());
      content = phist2->GetArray();
      content[0] += (float)hist.xunderflow;
      content[hist.nbinx+1] += (float)hist.xoverflow;
      for(ibinx=0; ibinx<hist.nbinx; ibinx++)
	  {
        content[ibinx+1] += (float)hist.buf[ibinx];
        printf("content[%d]=%f\n",ibinx,content[ibinx+1]);
	  }
	  */


      printf("hist2root: 2-dim hist !!!\n"); fflush(stdout);
      phist2->Reset(); //these are scalers, hence reset
	  for(ibinx=0; ibinx<hist.nbinx; ibinx++)
      {
        x = hist.xmin + hist.dx*(Float_t)ibinx;
        for(ibiny=0; ibiny<hist.nbiny; ibiny++)
        {
          y = hist.ymin + hist.dy*(Float_t)ibiny;

          //printf("hist2root: 2DIM[%f][%f] => %f\n",x,y,(Float_t)hist.buf2[ibinx][ibiny]); fflush(stdout);
          phist2->Fill(x,y,(Float_t)hist.buf2[ibinx][ibiny]);
	    }
	  }
	  

    }
  }
  } /*netlist==NULL*/
}

void decodeDefaultHist(std::string title, std::vector <double> &data)
{
  if (debug) printf("decodeDefaultHist \n");
  hist.ntitle=title.length();
  hist.title=strdup(title.c_str());
  hist.nbiny=0;
  hist.nbinx=data.size();
  hist.xmin=-0;5;
  hist.xmax=data.size()-0.5;
  hist.dx=1;
  
  hist.buf = (float *) calloc(hist.nbinx,sizeof(float));
  if(hist.buf==NULL){
    printf("ERROR in 1DIM calloc()\n");
    error = 1;
    return;
  }
  
  for(int ibinx=0; ibinx<hist.nbinx; ibinx++)
  {
    hist.buf[ibinx]=data[ibinx];
  }
  
}



};

#endif

