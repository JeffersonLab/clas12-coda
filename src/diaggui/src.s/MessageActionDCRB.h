#ifndef __MESSAGE_ACTION_HIST__
#define __MESSAGE_ACTION_HIST__

#include "MessageAction.h"

#include "hbook.h"
#ifdef USE_ROOT
#include "dc_scalers_ipc.h"
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

#ifdef USE_ROOT
	dc_scalers_app *pdc_scalers_app;
#endif

  public:

    MessageActionHist(){}


#ifdef USE_ROOT
    MessageActionHist(std::string myname_, int debug_ = 0, dc_scalers_app *pdc_scalers_app_ = NULL)
    {
      myname = myname_;
      done = 0;
      status = 0;
      statistics = 0;
      debug = debug_;
      pdc_scalers_app = pdc_scalers_app_;
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
	  printf("check: testing fmt >%4.4s<\n",fmt.c_str());
      for(int i=0; i<NFORMATS; i++)
	  {
        std::string f = formats[i];
        if( !strncmp(f.c_str(),fmt.c_str(),strlen(f.c_str())) )
		{
          formatid = i;
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
          printf("1D[%d] --> %f\n",ibinx,hist.buf[ibinx]);
	    }

	  }
      else /*2-dim*/
	  {
        recv >> hist.ymin >> hist.ymax >> hist.yunderflow >> hist.yoverflow;
        hist.dy = (hist.ymax - hist.ymin)/(float)hist.nbiny;

        hist.buf2 = (float **) calloc(hist.nbinx,sizeof(float));
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
            printf("2D[%d][%d] --> %f\n",ibinx,ibiny,hist.buf2[ibinx][ibiny]);
	      }
	    }
		
	  }

	  printf("\n\n");
    }


    void process()
    {
      //std::cout << "MessageActionHist: process Hist message" << std::endl;      


#ifdef USE_ROOT
      if(error==0) pdc_scalers_app->hist2root(hist);
#endif

    }



};

#endif
