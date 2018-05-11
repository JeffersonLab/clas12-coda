
/*daqmon_server.cc*/

#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>

#include "root_inc.h"
#include "hist_lib.h"

#include "hbook.h"


#define USE_ET

#ifdef USE_ET

#define MAXEVENTS 10000000
#define MAXBUF 10000000
unsigned int buf[MAXBUF];

#include "evio.h"
#include "evioBankUtil.h"
#include "et.h"
#define TDCLSB 24
#define ET_EVENT_ARRAY_SIZE 100
static char et_name[ET_FILENAME_LENGTH];
static et_stat_id  et_statid;
static et_sys_id   et_sys;
static et_att_id   et_attach;
static int         et_locality, et_init = 0, et_reinit = 0;
static et_event *pe[ET_EVENT_ARRAY_SIZE];
static int done;
static int PrestartCount, prestartEvent=17, endEvent=20;
static int use_et = 0;

#endif

#include "ipc_lib.h"
#include "ipc.h"

#define USE_ROOT
#include "MessageActionDAQMON.h"
#include "MessageActionJSON_DAQMON.h"

IpcServer &server = IpcServer::Instance();


//--------------  RCM MONOTORING -------------------
int *HIST_buffer;
int *skip_event_counter;
int64_t *tiBusyTime;
int *HIST_FLAG;
int *RUN_NUM;
int *RUN_FLAG;
int *buffer_size;

int *FADC_TYPE;
int  RUN_NUM_OLD=0;

//=====================================================
int sig_int = 0;
int sig_hup = 0;
int sig_alarm = 0;
int sig_pipe = 0;  



#ifdef USE_ET

/* ET Initialization */    
int
et_initialize(void)
{
  et_statconfig   sconfig;
  et_openconfig   openconfig;
  int             status;
  struct timespec timeout;

  timeout.tv_sec  = 2;
  timeout.tv_nsec = 0;

  /* Normally, initialization is done only once. However, if the ET
   * system dies and is restarted, and we're running on a Linux or
   * Linux-like operating system, then we need to re-initalize in
   * order to reestablish the tcp connection for communication etc.
   * Thus, we must undo some of the previous initialization before
   * we do it again.
   */
  if(et_init > 0)
  {
    /* unmap shared mem, detach attachment, close socket, free et_sys */
    et_forcedclose(et_sys);
  }
  
  printf("er_et_initialize: start ET stuff\n");

  if(et_open_config_init(&openconfig) != ET_OK)
  {
    printf("ERROR: er ET init: cannot allocate mem to open ET system\n");
    return(-1);;
  }
  et_open_config_setwait(openconfig, ET_OPEN_WAIT);
  et_open_config_settimeout(openconfig, timeout);
  if(et_open(&et_sys, et_name, openconfig) != ET_OK)
  {
    printf("ERROR: er ET init: cannot open ET system\n");
    return(-2);;
  }
  et_open_config_destroy(openconfig);

  /* set level of debug output */
  et_system_setdebug(et_sys, ET_DEBUG_ERROR);

  /* where am I relative to the ET system? */
  et_system_getlocality(et_sys, &et_locality);

  et_station_config_init(&sconfig);
  et_station_config_setselect(sconfig,  ET_STATION_SELECT_ALL);
  et_station_config_setblock(sconfig,   ET_STATION_BLOCKING);
  et_station_config_setuser(sconfig,    ET_STATION_USER_MULTI);
  et_station_config_setrestore(sconfig, ET_STATION_RESTORE_OUT);
  et_station_config_setprescale(sconfig,1);

  if((status = et_station_create(et_sys, &et_statid, "FTOF", sconfig)) < 0)
  {
    if (status == ET_ERROR_EXISTS) {
      printf("er ET init: station exists, will attach\n");
    }
    else
    {
      et_close(et_sys);
      et_station_config_destroy(sconfig);
      printf("ERROR: er ET init: cannot create ET station (status = %d)\n",
        status);
      return(-3);
    }
  }
  et_station_config_destroy(sconfig);

  if (et_station_attach(et_sys, et_statid, &et_attach) != ET_OK) {
    et_close(et_sys);
    printf("ERROR: er ET init: cannot attached to ET station\n");
    return(-4);;
  }

  et_init++;
  et_reinit = 0;
  printf("er ET init: ET fully initialized\n");
  return(0);
}

int 
gotControlEvent(et_event **pe, int size)
{
  int i;
    
  for (i=0; i<size; i++)
  {
    if ((pe[i]->control[0] == 17) || (pe[i]->control[0] == 20))
    {
      return(1);
    }
  }
  return(0);
}

#endif


void
ctrl_c(int m)
{
  sig_int=1;
  printf("\n CTRL-C pressed...\n");
  exit(1);
  //   printf("\n CTRL-C pressed, cancel timer=%d\n",alarm(1)); 
}

struct hist_param {
    int ic;
    TSocket *sock;
    char host_name[128];
};

int Debug = 0;

void *hist_serv_thread(void* arg);

void
usage(char* name)
{
  printf("\nusage: %s -h[elp]   -d[debug]=LEVEL \n",name);
}

int
STREQ(char*s1,const char*s2)
{
  if (strncasecmp(s1,s2,strlen(s2))) return 0;
  else return 1;
}



#undef DEBUG


TList *ALL_1D_HIST;

int
main(int argc, char *argv[])
{  
  char *substr1, *substr2;
  static int HIST_INIT;
  int runnum, nfile, handler, iev;
  char filename[1024];
  char *ch, chrunnum[32];
  int datasaved[100];
  int fragtag;
  int fragnum;
  int banktag;
  int banknum;
  int data;
  unsigned int bitpattern;

  pthread_t  hist_thread, rate_thread;
  TThread *th_hist_srv = NULL;

  ALL_1D_HIST = new TList();

#ifdef USE_ET
  GET_PUT_INIT;

  if(argc==2)
  {
    printf("Will try to attach to ET system or read file >%s<\n",argv[1]);

    if(!strncmp(argv[1],"/et/",4))
    {
      /* check if file exist */
      FILE *fd;
      fd = fopen(argv[1],"r");
      if(fd!=NULL)
	  {
        fclose(fd);
        strncpy(et_name,argv[1],ET_FILENAME_LENGTH);
        printf("attach to ET system >%s<\n",et_name);
	  }
      else
	  {
        printf("ET system >%s< does not exist - exit\n",argv[1]);
        exit(0);
	  }
	  /*
      if (!et_alive(et_sys))
      {
        printf("ERROR: not attached to ET system\n");
        et_reinit = 1;
        exit(0);
      }
	  */
      if(et_initialize() != 0)
      {
        printf("ERROR: cannot initalize ET system\n");
        exit(0);
      }

      done = 0;
      use_et = 1;
    }

    if(use_et)
    {
      runnum = 1; /* temporary fake it, must extract from et */
      printf("run number is %d\n",runnum);
    }
    else
    {
      strcpy(chrunnum,argv[1]);
      ch = strchr(chrunnum,'0');
      ch[6] = '\0';
      runnum = atoi(ch);
      printf("run number is %s (%d)\n",ch,runnum);
      nfile = 0;
    }

    iev = 0;
  }


  TH1F *h1pulse[128];
  TH1F *h1ped[128];
  TH1F *h1adc[128];
  char htitle[80];

  char hnameLocal[20];
  char *hnameLocalSplit;
  gethostname(hnameLocal,20);
  hnameLocalSplit=strtok(hnameLocal,".");

  for(int ii=0; ii<128; ii++)
  {
    sprintf(htitle,"%s:fadcpulse_%03d",hnameLocalSplit,ii);
    h1pulse[ii]= new TH1F(htitle, htitle, 100, 0.0, 100.0);
    ALL_1D_HIST->Add(h1pulse[ii]);

    sprintf(htitle,"%s:fadcped_%03d",hnameLocalSplit,ii);
    h1ped[ii]= new TH1F(htitle, htitle, 100, 0.0, 500.0);
    ALL_1D_HIST->Add(h1ped[ii]);

    sprintf(htitle,"%s:fadcadc_%03d",hnameLocalSplit,ii);
    h1adc[ii]= new TH1F(htitle, htitle, 100, 1.0, 2001.0);
    ALL_1D_HIST->Add(h1adc[ii]);
  }

#endif

  /*
  for(int ii=1; ii<argc; ii++)
  {
    printf("arg(%d)=%s -> ",ii,argv[ii]);fflush(stdout);
    
    //-------------------------------------------------------------------
    if(STREQ(argv[ii],"-d"))   { //-- port 
      Debug = 1;
      if ((substr1=strstr(argv[ii],"="))) {
	    printf("found /=/ |%s|\n",substr1);
	    Debug = atoi(&substr1[1]);
      }
    }
    //-------------------------------------------------------------------
    //-------------------------------------------------------------------
    if(STREQ(argv[ii],"-h"))   { usage(argv[0]); exit(0); }
  }
  */

  printf("11\n");fflush(stdout);

  //-----------------------
  signal(SIGINT,ctrl_c);  
  //-----------------------


  printf("12\n");fflush(stdout);  

  /*sergey*/
  {
    pthread_t id;
    pthread_attr_t attr;
    pthread_attr_init(&attr); /* initialize attr with default attributes */
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_create(&id, &attr, hist_serv_thread, NULL);
  }
  /*sergey*/

  printf("13\n");fflush(stdout);  


  // connect to ipc server
  server.AddSendTopic(getenv("EXPID"),getenv("SESSION"),NULL,NULL);
  server.AddRecvTopic(getenv("EXPID"),getenv("SESSION"),"*","*");
  server.Open();

  MessageActionHist *hist = new MessageActionHist((char *)"hbook_recv",Debug,ALL_1D_HIST);
  server.AddCallback(hist);
  MessageActionJSON *scaler = new MessageActionJSON((char *)"scaler_recv",Debug,ALL_1D_HIST);
  server.AddCallback(scaler);
  
 


  while(1)
  {
    //sleep(1);

#ifdef USE_ET

	int status, nevents, etstart, etstop, i, maxevents, iet, ind, ind1, idn, nbytes, mm, nn, ind_data, sec;
    size_t len;
    unsigned int *bufptr, word;
    float x1,x2,y1,y2,ww,tmpx,tmpy,ttt,ref;

  if(use_et)
  {
    /* get chunk of events */
    status = et_events_get(et_sys, et_attach, pe, ET_SLEEP, NULL, ET_EVENT_ARRAY_SIZE, &nevents);

    /* if no events or error ... */
    if ((nevents < 1) || (status != ET_OK))
    {
      /* if status == ET_ERROR_EMPTY or ET_ERROR_BUSY, no reinit is necessary */
      
      /* will wake up with ET_ERROR_WAKEUP only in threaded code */
      if (status == ET_ERROR_WAKEUP)
      {
        printf("status = ET_ERROR_WAKEUP\n");
      }
      else if (status == ET_ERROR_DEAD)
      {
        printf("status = ET_ERROR_DEAD\n");
        et_reinit = 1;
      }
      else if (status == ET_ERROR)
      {
        printf("error in et_events_get, status = ET_ERROR \n");
        et_reinit = 1;
      }
      done = 1;
    }
    else /* if we got events */
    {
      /* by default (no control events) write everything */
      etstart = 0;
      etstop  = nevents - 1;
      
      /* if we got control event(s) */
      if (gotControlEvent(pe, nevents))
      {
        /* scan for prestart and end events */
        for (i=0; i<nevents; i++)
        {
	      if (pe[i]->control[0] == prestartEvent)
          {
	        printf("Got Prestart Event!!\n");
	        /* look for first prestart */
	        if (PrestartCount == 0)
            {
	          /* ignore events before first prestart */
	          etstart = i;
	          if (i != 0)
              {
	            printf("ignoring %d events before prestart\n",i);
	          }
	        }
            PrestartCount++;
	      }
	      else if (pe[i]->control[0] == endEvent)
          {
	        /* ignore events after last end event & quit */
            printf("Got End event\n");
            etstop = i;
	        done = 1;
	      }
        }
      }
	}
    maxevents = iev + etstop; 
    iet = 0;
  }
  else
  {
    sprintf(filename,"%s.%d",argv[1],nfile++);
    printf("opening data file >%s<\n",filename);
    status = evOpen(filename,"r",&handler);
    printf("status=%d\n",status);
    if(status!=0)
    {
      printf("evOpen error %d - exit\n",status);
while(1) sleep(1);
      break;
    }
    maxevents = MAXEVENTS;
  }






  while(iev<maxevents)
  {
    iev ++;

    if(!(iev%1000)) printf("\n\n\nEvent %d\n\n",iev);
#ifdef DEBUG
    printf("\n\n\nEvent %d\n\n",iev);
#endif



	if(use_et)
	{
      int handle1;
      if(iet >= nevents)
	  {
        printf("ERROR: iev=%d, nevents=%d\n",iet,nevents);
        exit(0);
	  }
      et_event_getlength(pe[iet], &len); /*get event length from et*/
	  /*printf("len1=%d\n",len);*/

      /*copy event from et to the temporary buffer
      memcpy((char *)buf, (char *)pe[iet]->pdata, len);
      bufptr = &buf[8];
	  */
      bufptr = (unsigned int *)pe[iet]->pdata;
      bufptr += 8;

	  /*
	  printf("buf: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
			 bufptr[0],bufptr[1],bufptr[2],bufptr[3],bufptr[4],
			 bufptr[5],bufptr[6],bufptr[7],bufptr[8],bufptr[9]);
	  */

      iet ++;

    }
    else
    {
      status = evRead(handler, buf, MAXBUF);
      if(status < 0)
	  {
	    if(status==EOF)
	    {
          printf("evRead: end of file after %d events - exit\n",iev);
          break;
	    }
	    else
	    {
          printf("evRead error=%d after %d events - exit\n",status,iev);
          break;
	    }
      }
      bufptr = buf;
    }




	  /* get trigger bits */
      fragtag = 37;
      fragnum = -1;
      banktag = 0xe10a;
      banknum = 0;
      if((ind1 = evLinkBank(bufptr, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data)) > 0)
      {
        unsigned char *end;
        printf("FOUND TS BANK\n");
        b08 = (unsigned char *) &bufptr[ind_data];
        end = b08 + nbytes;
        unsigned int *iptr, iarray[10];
        iptr = iarray;
        while(b08<end)
        {
          GET32(data);
          *iptr++ = (unsigned int)data;
          printf("data=0x%08x\n",data);
		}
        bitpattern = iarray[4];
        printf("BITMASK=0x%08x\n",bitpattern);
	  }


	  for(fragtag=1; fragtag<=36; fragtag++)
	  {

        //if(bitpattern!=0x01000000) break;

        fragnum = -1;
        banktag = 0xe101;
        banknum = 0;

	    /* ADC raw mode bank */
        if((ind1 = evLinkBank(bufptr, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data)) > 0)
        {
          unsigned char *end;
          unsigned long long time;
          int crate,slot,trig,nchan,chan,nsamples,notvalid,edge,val,count,ncol1,nrow1;
          int oldslot = 100;
          int baseline, sum, channel, summing_in_progress;
#ifdef DEBUG
          printf("0xe101: fragtag=%d, ind1=%d, nbytes=%d\n",fragtag,ind1,nbytes);
#endif
          b08 = (unsigned char *) &bufptr[ind_data];
          end = b08 + nbytes;
#ifdef DEBUG
          printf("ind1=%d, nbytes=%d (from 0x%08x to 0x%08x)\n",ind1,nbytes,b08,end);
#endif

          while(b08<end)
          {
#ifdef DEBUG
            printf("begin while: b08=0x%08x\n",b08);
#endif
            GET8(slot);
            GET32(trig);
            GET64(time);
            GET32(nchan);
#ifdef DEBUG
            printf("slot=%d, trig=%d, time=%lld nchan=%d\n",slot,trig,time,nchan);
#endif
            for(nn=0; nn<nchan; nn++)
	        {
              GET8(chan);

              GET32(nsamples);
#ifdef DEBUG
              printf("  chan=%d, nsamples=%d\n",chan,nsamples);
#endif
              baseline = sum = summing_in_progress = 0;
              for(mm=0; mm<nsamples; mm++)
	          {
	            GET16(data);
                datasaved[mm] = data;

			    printf("mmm=%d data=%d\n",mm,data);
                if(mm<10) baseline += data;

                if(mm==10)
			    {
                  baseline = baseline / 10;
#ifdef DEBUG
                  printf("slot=%d chan=%d baseline=%d\n",slot,chan,baseline);
#endif
                  printf("slot=%d chan=%d baseline=%d\n",slot,chan,baseline);
			    }

                if(mm>15 && mm<100)
                {
                  if(summing_in_progress==0 && data>(baseline+20))
			      {
                    printf("open summing at mm=%d, data=%d, sum=%d\n",mm,data,sum);
                    summing_in_progress = 1;
                    sum += (datasaved[mm-3]-baseline);
                    sum += (datasaved[mm-2]-baseline);
                    sum += (datasaved[mm-1]-baseline);
                    printf("sum1=%d\n",sum);
			      }

                  if(summing_in_progress>0 && data<(baseline+20))
			      {
                    printf("close summing at mm=%d, sum=%d\n",mm,sum);
                    summing_in_progress = -1;
			      }

                  if(summing_in_progress>0)
			      {
                    sum += (datasaved[mm]-baseline);
                    printf("sum=%d (mm=%d)\n",sum,mm);
			      }
			    }

	          }



              if(slot==3&&chan==3)
              {
		        /* fill raw adc pulse hist only if there was a pulse */
                if(sum>100)
		        {
                  for(mm=0; mm<nsamples; mm++)
	              {
                    tmpx = (float)mm+0.5;
                    ww = (float)datasaved[mm];
#ifdef DEBUG
				    printf("slot=%d chan=%d bin=%f ww=%f\n",slot,chan,tmpx,ww);
#endif
				    printf("sum=%d slot=%d chan=%d bin=%f ww=%f\n",sum,slot,chan,tmpx,ww);
                    h1pulse[fragtag]->Fill(tmpx, ww);
		          }
		        }
			  }


		      if(slot==3&&chan==3)
		      {
			    ww = 1.;

                /* adc pedestal */
                tmpx = (float)baseline;
			    /*printf("idn2=%d\n",idn);*/
			    /*printf("slot %d chan %d -> idn=%d tmpx=%f\n",slot,chan,idn,tmpx);*/
                h1ped[fragtag]->Fill(tmpx, ww);

                /* adc spectra */
                tmpx = (float)sum;
			    /*printf("idn3=%d\n",idn);*/
			    /*printf("slot %d chan %d -> idn=%d tmpx=%f\n",slot,chan,idn,tmpx);*/
                h1adc[fragtag]->Fill(tmpx, ww);
		      }
 



            }
#ifdef DEBUG
            printf("end loop: b08=0x%08x\n",b08);
#endif
          }
        }

      }

    }





  if(use_et)
  {
    /* put et events back into system */
    status = et_events_put(et_sys, et_attach, pe, nevents);            
    if (status != ET_OK)
    {
	  printf("error in et_events_put, status = %i \n",status);
      et_reinit = 1;
      done = 1;
    }	
  }
  else
  {
    printf("evClose after %d events\n",iev);fflush(stdout);
    evClose(handler);
  }

  if(iev>=MAXEVENTS) break;


#endif

  }



  return(0);
}



/*=================used in hist_serv_thread====================================================*/
void *
hist_serv(void* arg)
{
  printf("hist_serv reached\n");
  TMessage mess1(kMESS_OBJECT);  
  TMessage *mess;
  int DBG_h=10;
  int DBG_a=10;
  struct hist_param *param = (struct hist_param *) arg;
  TSocket *sd = param->sock;
  int ic=param->ic;
  int id;
  TThread::Printf(" New Client TThread --> hist_serv sock=%d ic=%d <--",sd,ic);
  prctl(PR_SET_NAME,"hist_serv_thread");


  while (sig_int==0)
  {
    int ret = sd->Recv(mess);
    if (Debug>DBG_a) printf("hist_serv():: Received message, ret(len)=%d  mess=%p \n",ret,mess);
    if (ret<=0)
    { 
      printf("hist_serv():: Error recv, Close socket %lu \n",(long int)sd);
      sd->Close();
      return NULL;
    }

    if (mess && mess->What() == kMESS_STRING)
    {
      char *hname=NULL;
      char cmd[256];
      mess->ReadString(cmd, 256);

      //if (Debug>DBG_a)
	  {
        printf("hist_serv()::   Client %d: %s\n",ic, cmd);

	  }

      if (!strncmp(cmd,"Update",6))
	  {
        printf("hist_serv():: Client %d: %s\n",ic, cmd); 
	  }

      if (!strncmp(cmd,"Update",6)  || !strncmp(cmd,"Hist:",5) )
      {
printf("hist_serv(): 11\n");fflush(stdout);

        if (!strncmp(cmd,"Hist:",5) )  hname=&cmd[5];

printf("hist_serv(): 12\n");fflush(stdout);

	    mess1.Reset();              // re-use TMessage object
	    int icc=0;
	    TObject *obj;

printf("hist_serv(): 13\n");fflush(stdout);

	    if (Debug>DBG_a) printf("hist_serv():: RUN loop\n");fflush(stdout);

printf("hist_serv(): 14\n");fflush(stdout);




	    TIter next1(ALL_1D_HIST);
	    while ((obj = next1()))
        {
	  printf("hist_serv(): 15\n");fflush(stdout);
	  mess1.Reset();

          const char *hh = obj->GetName();

          { printf("hist_serv():: request=%s hname=%s hh=%s ",cmd,hname,hh); obj->Print(); }
          if ( strncmp(cmd,"Update",6) &&  strcmp(hname,hh) ) continue; // NOT update and NOT hist
          { printf("hist_serv():: SEND hist: request=%s hname=%s hh=%s ",cmd,hname,hh); }

          mess1.WriteObject(obj);     // write object in message buffer 
          int ret = sd->Send(mess1); 
          if (ret<=0)
          {
            printf("hist_serv():: Error send, Close socket %lu \n",(long int)sd);
            sd->Close(); 
            return NULL;
		  }
		}
/* test: sending one hist */


#if 0

	    //---  send RUN  hist
	    TIter next0(histptr->ALL_RUN_HIST);
	    while ((obj = next0()))
        {
		  mess1.Reset(); 
		  icc++; 
          const char *hh = obj->GetName();
          if (Debug>DBG_h) { printf("hist_serv():: ICC=%d request=%s hname=%s hh=%s ",icc,cmd,hname,hh); obj->Print(); }
          if ( strncmp(cmd,"Update",6) &&  strcmp(hname,hh) ) continue;
          if (Debug>DBG_h) { printf("hist_serv():: SEND hist: request=%s hname=%s hh=%s ",cmd,hname,hh); }
          mess1.WriteObject(obj);     // write object in message buffer 
          int ret = sd->Send(mess1); 
          if (ret<=0)
          {
            printf("hist_serv():: Error send, Close socket %lu \n",(long int)sd);
            sd->Close(); 
            return NULL;
          }
	    } 
	      
	    if (Debug>DBG_a) printf("hist_serv():: 1D loop\n");
	    //---  send 1D  hist
	    TIter next1(histptr->ALL_1D_HIST);
	    while ((obj = next1()))
        {
		  mess1.Reset(); 
		  icc++; 
          const char *hh = obj->GetName();
          if (Debug>DBG_h) { printf("hist_serv():: ICC=%d request=%s hname=%s hh=%s ",icc,cmd,hname,hh); obj->Print(); }
          if ( strncmp(cmd,"Update",6) &&  strcmp(hname,hh) ) continue;
          if (Debug>DBG_h) { printf("hist_serv():: SEND hist: request=%s hname=%s hh=%s ",cmd,hname,hh); }
          mess1.WriteObject(obj);     // write object in message buffer 
          int ret = sd->Send(mess1); 
          if (ret<=0)
          {
            printf("hist_serv():: Error send, Close socket %lu \n",(long int)sd);
            sd->Close(); 
            return NULL;
          }
	    } 
	    if (Debug>DBG_a) printf("hist_serv():: FIT loop\n");

	    //---  send FIT  hist
	    TIter next1f(histptr->ALL_FIT_HIST);
	    while ((obj = next1f()))
        {
		  mess1.Reset(); 
		  icc++; 
          const char *hh = obj->GetName();
          if (Debug>DBG_h) { printf("hist_serv():: ICC=%d request=%s hname=%s hh=%s ",icc,cmd,hname,hh); obj->Print(); }
          if ( strncmp(cmd,"Update",6) &&  strcmp(hname,hh) ) continue;
          if (Debug>DBG_h) { printf("hist_serv():: SEND hist: request=%s hname=%s hh=%s ",cmd,hname,hh); }
		  mess1.WriteObject(obj);     // write object in message buffer 
		  int ret = sd->Send(mess1); 
		  if (ret<=0)
          {
		    printf("hist_serv():: Error send, Close socket %lu \n",(long int)sd);
		    sd->Close(); 
		    return NULL;
		  }
	    } 
	    if (Debug>DBG_a) printf("hist_serv():: 2D loop\n");

	    //---  send 2D  hist
	    TIter next2(histptr->ALL_2D_HIST);
	    while ((obj = next2()))
        {
		  mess1.Reset(); 
		  icc++;
          const char *hh = obj->GetName();
          if (Debug>DBG_h) { printf("hist_serv():: ICC=%d request=%s hname=%s hh=%s ",icc,cmd,hname,hh); obj->Print(); }
          if ( strncmp(cmd,"Update",6) &&  strcmp(hname,hh) ) continue;
          if (Debug>DBG_h) { printf("hist_serv():: SEND hist: request=%s hname=%s hh=%s ",cmd,hname,hh); }
		  mess1.WriteObject(obj);     // write object in message buffer 
		  int ret = sd->Send(mess1); 
		  if (ret<=0)
          {
		    printf("hist_serv():: Error send, Close socket %lu \n",(long int)sd);
		    sd->Close(); 
		    return NULL;
		  }
	    }  //-- obj loop 2D hist
#endif  



      }
      else if (!strncmp(cmd,"Finished",8))
      {
	    printf("hist_serv():: Client Finished \n");
	    printf("hist_serv():: Close socket %lu ic=%d \n",(long int)sd,ic);
	    sd->Close(); 
	    return NULL;
      }
    }
    else if (mess->What() == kMESS_OBJECT)
    {
      printf("hist_serv():: !!!!!!!!!!!!!!  got object of class: %s\n", mess->GetClass()->GetName());
      TThread::Lock();
      TH1 *h = (TH1 *)mess->ReadObject(mess->GetClass());
      TThread::UnLock();
            
      h->Print();
      delete h;       // delete histogram
    }
    else
    {
      printf("hist_serv():: *** Unexpected message ***\n");
    }
    delete mess;
    sd->Send("END");          // tell clients we are finished
  }  //--- while sig_int==0 

  printf("hist_serv()::===> EXIT hist_serv() client \n");
  return NULL;
}


/*=====================================================================*/
void *
hist_serv_thread(void *arg)
{
  printf("hist_serv_thread reached\n");

  const int MAX_CLNT = 20;
  TSocket *sd, *soc[MAX_CLNT]={NULL};
  TThread *th_hist = NULL;  //--- better joint ??? =>array
  TServerSocket *ss;
  struct hist_param param;

  TThread::Printf(" New TThread --> hist_serv <-- Enter Lock()");    

  while(!sig_int)
  {
    ss = new TServerSocket(32765, kTRUE);
    if (ss->IsValid()) break;
    printf("hist_serv_thread:: Error Open TServerSocket, err=%d\n", ss->GetErrorCode());
    delete ss;
    sleep(5);
  }

  while (sig_int==0)
  {
    printf("hist_serv_thread:: ................  NEW wait ......\n");
        
    sd = ss->Accept();
    if (sd<=0)
    {
      printf("Error Accept sd=%lu, Close connection \n",(long int)sd);
      sd->Close();
      continue;
    }
    printf("hist_serv_thread:: ................  NEW client ......\n");
        
    int ifound=0;

    for (int ic=0;ic<MAX_CLNT;ic++)
    {
      if (!soc[ic] || !soc[ic]->IsValid())
      {
        if (sd->Send("go 0")<=0) sd->Close();
        else
        { 
          ifound = 1;
          param.ic = ic;
          param.sock = sd;
          soc[ic] = sd;   //-- not used -- for count only 
          printf("hist_serv_thread():: accept ic = %d \n",ic);
          //---  start new thread ----
          th_hist = new TThread("hist_serv", hist_serv, &param);
          th_hist->Run();
          printf("New Thread hist_serv() started \n");
          break;
        }
      }
    }

    if (!ifound)
    {
      printf("hist_serv_thread():: only accept %d clients connections !!\n",MAX_CLNT);
      sd->Send("Exceed the Number of Clients");
      sd->Close();
    }
    continue;
  }

  printf("===> EXIT hist_serv_thread() \n");
  return NULL;
}
