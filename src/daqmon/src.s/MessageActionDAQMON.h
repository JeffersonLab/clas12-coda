#ifndef __MESSAGE_ACTION_HIST__
#define __MESSAGE_ACTION_HIST__

#include "MessageAction.h"

#include "hbook.h"
#ifdef USE_ROOT

#endif

class MessageActionHist : public MessageAction {

  private:

    static const int NFORMATS=1;
    std::string formats[NFORMATS] = {"hist"};
    int formatid;

    int error;
    int debug;
    int done;
    int status;
    int statistics;
    std::string myname;
    int len;
	std::string title;

    int packed;
    Hist hist;

    TH1F *phist1;
    TH2F *phist2;


#ifdef USE_ROOT
	TList *histlist;
#endif

  public:

    MessageActionHist(){}


#ifdef USE_ROOT
    MessageActionHist(std::string myname_, int debug_ = 0, TList *histlist_ = NULL)
    {
      myname = myname_;
      done = 0;
      status = 0;
      statistics = 0;
      debug = debug_;
      histlist = histlist_;
    }
#else
    MessageActionHist(std::string myname_, int debug_ = 0)
    {
      myname = myname_;
      done = 0;
      status = 0;
      statistics = 0;
      debug = debug_;
    }
#endif

    ~MessageActionHist(){}

    int check(std::string fmt)
    {
      printf("\n===== MessageActionDAQMON check: testing fmt >%4.4s< (len of fmt is: %i) \n",fmt.c_str(),strlen(fmt.c_str()));
      for(int i=0; i<NFORMATS; i++)
	  {
        std::string f = formats[i];
        if( !strncmp(f.c_str(),fmt.c_str(),strlen(f.c_str())) )
		{
          formatid = i;
	  printf("\n MessageActionDAQMON fmt found! %s %s \n",f.c_str(),fmt.c_str());
          return(1);
		}
	  }

      formatid = 0;
      return(0);
    }


    void decode(IpcConsumer& recv)
    {
      error = 0;

      recv >> packed >> hist.id >> hist.entries >> hist.ntitle >> title;
      hist.title = strdup(title.c_str());

	  std::cout<<"id="<<hist.id<<" ntitle="<<hist.ntitle<<" title="<<hist.title<<std::endl;

      recv >> hist.nbinx >> hist.xmin >> hist.xmax >> hist.xunderflow >> hist.xoverflow >> hist.nbiny;
      hist.dx = (hist.xmax - hist.xmin)/(float)hist.nbinx;

      printf("nbinx=%d nbiny=%d\n",hist.nbinx,hist.nbiny);
      if(hist.nbiny==0) /*1-dim*/
	  {
        hist.buf = (float *) calloc(hist.nbinx,sizeof(float));
        if(hist.buf==NULL)
        {
          printf("ERROR in 1DIM calloc()\n");
          error = 1;
          return;
		}

        for(int ibinx=0; ibinx<hist.nbinx; ibinx++)
        {
          recv >> hist.buf[ibinx];
          //printf("1D[%d] --> %f\n",ibinx,hist.buf[ibinx]);
	    }

	  }
      else /*2-dim*/
	  {
        recv >> hist.ymin >> hist.ymax >> hist.yunderflow >> hist.yoverflow;
        hist.dy = (hist.ymax - hist.ymin)/(float)hist.nbiny;

        hist.buf2 = (float **) calloc(hist.nbinx,sizeof(float*));
        if(hist.buf2==NULL)
        {
          printf("ERROR1 in 2DIM calloc()\n");
          error = 1;
          return;
		}

        for(int i=0; i<hist.nbinx; i++)
        {
          hist.buf2[i] = (float *) calloc(hist.nbiny,sizeof(float));
          if(hist.buf2[i]==NULL)
		  {
            printf("ERROR2 in 2DIM calloc()\n");
            error = 1;
            return;
		  }
		}

		
        for(int ibinx=0; ibinx<hist.nbinx; ibinx++)
        {
          for(int ibiny=0; ibiny<hist.nbiny; ibiny++)
          {
            recv >> hist.buf2[ibinx][ibiny];
            //printf("2D[%d][%d] --> %f\n",ibinx,ibiny,hist.buf2[ibinx][ibiny]);
	      }
	    }
		
	  }

	  printf("\n\n");
    }


    void process()
    {
      //std::cout << "MessageActionHist: process Hist message" << std::endl;      


#ifdef USE_ROOT
      if(error==0) hist2root();
#endif

    }



void hist2root()
{
  Int_t id, ibinx, ibiny;
  Float_t x, y;
  Float_t *content;
  char title[20];

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
	  
      phist1->SetEntries(hist.entries+phist1->GetEntries());
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


};

#endif
