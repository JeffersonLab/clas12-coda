#ifndef __MESSAGE_ACTION_EVIO2ET__
#define __MESSAGE_ACTION_EVIO2ET__

#include "MessageAction.h"


#include "evio.h"
#include "et.h"

/* following macros assume we have 'unsigned int *dabufp' set to the output buffer */

#define BANK_INIT \
  unsigned int *dabufp, *StartOfEvent, *StartOfFrag, *StartOfBank

#define EVENT_OPEN(btag) \
  StartOfEvent =  dabufp++; \
  *dabufp++ = 0x10CC | ((btag & 0xFF) << 16)

#define EVENT_CLOSE \
  *StartOfEvent = dabufp - StartOfEvent - 1

#define FRAG_OPEN(btag,btyp,bnum) \
  StartOfFrag = dabufp++; \
  *dabufp++ = ((btag)<<16) + ((btyp)<<8) + (bnum)

#define FRAG_CLOSE \
  /* writes bank length (in words) into the first word of the bank */ \
  *StartOfFrag = dabufp - StartOfFrag - 1

#define BANK_OPEN(btag,btyp,bnum) \
  StartOfBank = dabufp++; \
  *dabufp++ = ((btag)<<16) + ((btyp)<<8) + (bnum)

#define BANK_CLOSE \
  /* writes bank length (in words) into the first word of the bank */ \
  *StartOfBank = dabufp - StartOfBank - 1



class MessageActionEVIO2ET : public MessageAction {

  private:

    static const int NFORMATS=1;
    std::string formats[NFORMATS] = {"evio2et"};
    int formatid;

    char *session     = (char*)NULL;
    char *destination = NULL;
    char *database    = (char*)"clasrun";
    int done          = 0;
    int debug         = 0;

    int nev_rec         = 0;
    int nev_no_et       = 0;
    int nev_no_run      = 0;
    int nev_to_et       = 0;
    int lost_connection = 0;

    char *run_status;
    int run_number      = 0;
    int event_number    = 0;

    int status, itmp;
    unsigned char *dch[10], data_char[10][256];
    size_t len, ii, jj;
    int *p,*pstart,i,nused,banksize,nhead,buflen;
    std::string msgtype;
    std::string bankname;
    std::string bankformat;
    int32_t banknumber,ncol,nrow,nwrds,ndatawords;

	//smartsockets    int32_t *datawords;
    int32_t datawords[1000];

    char line[128];

  public:

    // et stuff
    int et_ok = 0;
    et_sys_id et_system_id;
    et_openconfig openconfig;
    char et_filename[128];
    et_att_id et_attach_id;
    et_event *et_event_ptr;
    int et_control[ET_STATION_SELECT_INTS];
    unsigned int etbuffersize;


  public:

    MessageActionEVIO2ET()
    {
      debug = 0;
    }

    MessageActionEVIO2ET(int debug_)
    {
      debug = debug_;
    }

    MessageActionEVIO2ET(char *session_)
    {
      session = strdup(session_);
    }

    MessageActionEVIO2ET(char *session_, char *destination_)
    {
      session = strdup(session_);
      destination = strdup(destination_);
    }

    MessageActionEVIO2ET(char *session_, char *destination_, char *database_)
    {
      session = strdup(session_);
      destination = strdup(destination_);
      database = strdup(database_);
    }

    ~MessageActionEVIO2ET(){}


    int check(std::string fmt)
    {
	  /*printf("checkEVIO2ET: fmt >%s<\n",fmt.c_str());*/
      for(int i=0; i<NFORMATS; i++)
	  {
        std::string f = formats[i];
        if( !strncmp(f.c_str(),fmt.c_str(),strlen(f.c_str())) )
		{
          formatid = i;
		  if(debug) printf("FOUND fmt >%s<\n",fmt.c_str());
          return(1);
		}
	  }

      formatid = 0;
      return(0);
    }


    void init_et()
    {
      char ch[256], *chptr;
      et_ok=0;

      // create et file name
      sprintf(et_filename,"/et/%s",session);
      et_open_config_init(&openconfig);

#ifdef REMOTE_ET

      if(destination==NULL)
      {
        printf("destination host not specified, trying env var CLON_EB\n");
        chptr = getenv("CLON_EB");
        if(chptr==NULL)
        {
          printf("ERROR: env var 'CLON_EB' is not set - exit\n");
          exit(0);
        }
	    else
	    {
          destination = strdup(chptr);
          printf("CLON_EB set to '%s', use it as destination host\n",destination);
	    }
      }
      else
      {
        printf("destination host specified as '%s'\n",destination);
      }


      et_open_config_sethost(openconfig, destination);
      et_open_config_gethost(openconfig, ch);

      printf("will use destination (remote) host '%s', et system '%s'\n",ch,et_filename);

      /* do not need it: Carl fixed problem
      et_open_config_setmode(openconfig, ET_DIRECT);
      */

#endif

    }




    void connect_et()
    {
      int status;
      sigset_t sigblock;

      et_ok=0;

      printf("trying to connect to ET system\n");

      // open et system
      if(et_open(&et_system_id, et_filename, openconfig)!=ET_OK)
      {
        printf("ERROR: Cannot connect to ET - return\n");
        done = 1; /* force loop exit in main program assuming that process manager will start us soon and we'll try to connect again */
        return;
      }

      // get max normal event size
      et_system_geteventsize(et_system_id, (size_t *)&etbuffersize);
      printf("INFO: event size = %d\n",etbuffersize);


      // block signals to THIS thread and any thread created by this thread
      // needed to keep signals from et threads
      sigfillset(&sigblock);
      pthread_sigmask(SIG_BLOCK, &sigblock, NULL);


      // attach to existing station
      status = et_station_attach(et_system_id, ET_GRANDCENTRAL, &et_attach_id);
      if(status!=ET_OK)
      {
        et_forcedclose(et_system_id);
		std::cerr << "Unable to attach to grandcentral station" << std::endl;
        done = 1; /* force loop exit in main program assuming that process manager will start us soon and we'll try to connect again */
        return;
      }
  

      // unblock signals
      pthread_sigmask(SIG_UNBLOCK,&sigblock,NULL);


      // success
      et_ok=1; 
      std::cout << "...now connected to ET system: " << et_filename << ",   station: grandcentral" << std::endl;
    }


    void set_run_status(char *run_status_)
	{
      run_status = strdup(run_status_);
	}

    void set_run_number(int run_number_)
	{
      run_number = run_number_;
	}

    void set_debug(int debug_)
	{
      debug = debug_;
	}

    int get_done()
	{
      return(done);
	}



    void process()
    {
      //std::cout << "MessageActionEVIO2ET: process EVIO2ET message" << std::endl;      

    }


    void decode(IpcConsumer& recv)
    {
      //std::cout << std::endl << "MessageActionEVIO2ET: decode EVIO2ET message" << std::endl;      




      BANK_INIT;

      // for record segment header
      int nevnt  = 0;
      int nphys  = 0;
      int trig   = 0;

      // constants for head bank
      int nvers  = 0;
      int type   = EVENT_TYPE;
      int rocst  = 0;
      int evcls  = 0;
      int presc  = 0;

      // total events received
      nev_rec++;

      // check et
      if(et_ok==0)
      {
        connect_et();
        if(et_ok==0)
        {
          nev_no_et++;
          return;
        }
      }
      else if(et_alive(et_system_id)==0)
      {
        nev_no_et++;
        lost_connection = 1;
		printf("et_alive: lost connection - exit\n");
        done = 1; /* force loop exit in main program assuming that process manager will start us soon and we'll try to connect again */
        return;
      }



      /* sergey: get run state 
      run_status = get_run_status(database,session);
      */



#if 0 /* redo it, removing dependence to epicsutil.a !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

      strcpy((char *)data_char[0],run_status);
      if(debug) printf("run_status: data_char[0]>%s<\n",data_char[0]);
      dch[0] = data_char[0];
      status = epics_msg_send("hallb_run_status","string",1,dch);


      /*
      strcpy((char *)data_char[0],get_run_operators(database,session));
      if(debug) printf("run_operators: data_char[0]>%s<\n",data_char[0]);
      dch[0] = data_char[0];
      status = epics_msg_send("hallb_run_operators","string",1,dch);
      */

      if(strcasecmp(run_status,old_run_status)!=0)
      {
        printf("Run state changed from '%s' to '%s' (UNIX time=%d)\n",old_run_status,run_status,time(NULL));fflush(stdout);
        strcpy(old_run_status,run_status);
      }

      // no entry unless in 'active' state
      if(strncasecmp(run_status,"active",6)!=0)
      {
        if(debug) printf("bosbank_callback: session '%s' is in state '%s', state is not 'active' - set nev_no_run=%d and return\n",
                          session,run_status,nev_no_run);

        itmp = 0;
        status = epics_msg_send("hallb_daq_status","int",1,&itmp);

        nev_no_run++;
	    return;
      }

      itmp = 1;
      status = epics_msg_send("ts_status_ttl","int",1,&itmp);


      // get run number
      if(debug) printf("Obtained run number %d\n",run_number);

      status = epics_msg_send("hallb_run_number","int",1,&run_number);

#endif


      // get free event 
      status = et_event_new(et_system_id, et_attach_id, &et_event_ptr, ET_ASYNC, NULL, etbuffersize);
      if(status!=ET_OK)
      {
	    printf("error in et_event_new - return\n");
        if(debug!=0) std::cerr << "?unable to get event, status is: " << status << std::endl;
        nev_no_et++;
        return;
      }

      // set control words
      for (int ii=0; ii<ET_STATION_SELECT_INTS; ii++) et_control[ii] = 0;
      et_control[0] = type;
      et_event_setcontrol(et_event_ptr, et_control, ET_STATION_SELECT_INTS);

      // set pointer, reset counts, etc.
      et_event_getdata(et_event_ptr,(void**)&p);
      pstart = p;
      nused=0;
      nhead=0;


      /**************************/
      /* creating evio2et fragment */

      int handle1;
      unsigned int *ptr;

      ptr = (unsigned int *)et_event_ptr->pdata;
      et_event_getlength(et_event_ptr, &len); /*get event length from et*/

printf("new et event length=%d\n",len);




	  /* SEE HPS_HACK in coda_ebc.c !!!!!!!!! */
#if 0
      status = evOpenBuffer((char *)pstart, MAXBUF, "w", &handle1);
      if(status!=0) printf("evOpenBuffer returns %d\n",status);
#else
#endif

      pstart += 8; /* skip record header */
      dabufp = (unsigned int *)pstart;

      if(debug) printf("Open event\n");

      EVENT_OPEN(EVENT_TYPE);

      if(debug) printf("Open fragment tag=129\n");

      FRAG_OPEN(129,0xe,0); /* use 'rocid' = 129, it never used in hardware ROCs */

      /*
      // create segment header, then update pointer and counters
      status = create_header(p,etbuffersize-nused,nhead,'RUNP','ARMS',run,nevnt,nphys,trig);
      if(status==0)
      {
        p+=nhead;
        nused+=nhead;
      }  
      */

      if(debug) printf("Open bank tag=0xe112\n");

      BANK_OPEN(0xe112,1,0); /* open HEAD bank */

	  /*
      // head bank (pstart' points to data area)
      status = va_add_bank(p,etbuffersize-nused,"HEAD",0,"I",8,1,8,banksize,
        nvers,run,nevnt,(int)time(NULL),type,rocst,evcls,presc);
      if(status==0)
      {
        p+=banksize;
        nused+=banksize;
      }
      */

      int nwords = 5; /* UPDATE THAT IF THE NUMBER OF WORDS CHANGED BELOW !!! */
      int event_type = EVENT_TYPE;
      event_number ++;

      //
      // does not need headers - there are no rol2
      //
      //  *dabufp ++ = (0x12<<27)+(event_number&0x7FFFFFF); /*event header*/
      //  *dabufp ++ = (0x14<<27)+nwords; /*head data*/
      //

      *dabufp ++ = 0; /*version  number */
      *dabufp ++ = run_number; /*run  number */
      *dabufp ++ = event_number; /*event number */
      *dabufp ++ = time(0); /*event unix time */
      *dabufp ++ = event_type; /*event type */

      if(debug) printf("Close bank tag=0xe112\n");

      BANK_CLOSE;

      // extract banks from message and insert into event
      int fptr = 1;

      int banknum = 0;
      while(1/*was fptr<msg.NumFields()*/)
      {

        //recv >> bankname >> banknumber >> bankformat >> ncol >> nrow >> nwrds >> datawords >> GetSize(&ndatawords) /*>> Check(msg)*/;



	    try {
          recv >> bankname;
		}
        catch (const MessageEOFException& ex) {
		  printf("\nEnd Of Message reached !!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");
          break;
	    }
        catch (const std::exception& ex) {
          printf("EXCEPTION %s\n", ex.what());
          exit(0);
	    }



        recv >> banknumber >> bankformat >> ncol >> nrow >> nwrds;
        if(debug) std::cout<<"bankname="<<bankname<<", banknumber="<<banknumber<<", bankformat="<<bankformat;
        if(debug) std::cout<<", ncol="<<ncol<<", nrow="<<nrow<<", nwrds="<<nwrds<<std::endl;

        if(debug) printf("RECV1\n");fflush(stdout);
        recv >> datawords;
        if(debug) printf("RECV2\n");fflush(stdout);
        recv >> GetSize(&ndatawords);
        if(debug) printf("RECV3\n");fflush(stdout);


        if((debug==1)&&(nwrds!=ndatawords))
        {
          std::cerr << "Bank data inconsistent...nwrds,ndatawords are: " << nwrds << ", " << ndatawords << std::endl;
        }
		else
		{
          if(debug) printf("GOT IT: ndatawords=%d\n",ndatawords);fflush(stdout);
		}

	    /*
        status = add_bank(p,etbuffersize-nused,bankname,banknumber,bankformat,ncol,nrow,nwrds,banksize,(int*)&datawords[0]);
        if(status==0)
        {
          p+=banksize;
          nused+=banksize;
        }
	    */

        if(debug) printf("Open bank tag=0xe114 num=%d\n",banknum);

        BANK_OPEN(0xe114,3,banknum); /* open EPICS data bank */

        char *str = (char *)dabufp;
        char sss[1024];
        str[0] = '\n'; /* one 'cr' in the beginning to make evio2xml output looks better ... */
        str[1] = '\0';
        int nch;

        float *ptr;
        char *ch;
        ptr = (float *)datawords;
        ch = (char *)(ptr+1);

        for(ii=0; ii<nrow; ii++)
        {
          //printf("val=%f, name >",ptr[0]);

          sprintf(sss,"%16.6f %32.32s\n",ptr[0],(char *)&ptr[1]);
          strcat(str,sss);

          //for(jj=0; jj<(ncol-1)*4; jj++)
          //{
          //  printf("%c",ch[jj]);
          //}
          //printf("<\n");
          ptr += 9;
          ch = (char *)(ptr+1);

          //printf("in loop str >%s<\n",str);
        }

        if(debug) printf("final str >%s<\n",str);

        nch = strlen(str) + 1; /* strlen() exclude terminal character ! */
        int nw_full = (nch+5)/4;
        dabufp += nw_full;
        int nch_full = nw_full*4;
		int nch_round = ((nch+3)/4)*4; /* complete last word and recount chars */

        /* fill unused chars with '\0', last one with '\4' */
        for(int kk=nch; kk<nch_full-1; kk++) {str[kk] = '\4';printf("str[%d]=4\n",kk);}
        str[nch_full-1] = '\4';printf("str[%d]=4\n",nch_full-1);

        int padding = nch_round - nch;
		printf("nch=%d, nch_round=%d, nch_full=%d, nw_full=%d, padding=%d -> strlen=%d\n",
			   nch,nch_round,nch_full,nw_full,padding, strlen(str));
        *(StartOfBank+1) |= (padding&0x3)<<14;

        //printf("banknum=%d\n",banknum);
        //printf("  ncol%d nrow=%d nwrds=%d\n",ncol,nrow,nwrds);

        if(debug) printf("Close bank tag=0xe114 num=%d\n",banknum);

        BANK_CLOSE;

        banknum ++;

        //fptr+=7;
		//break;/*TEMP !!!!!!!!!!!!!!!!!!*/
      }

      if(debug) printf("Close fragment\n");fflush(stdout);

      FRAG_CLOSE;

      if(debug) printf("Close event\n");fflush(stdout);

      EVENT_CLOSE;





	  /* SEE HPS_HACK in coda_ebc.c !!!!!!! */
#if 0
	  /*see
evWrite() -> evWriteImpl() -> writeEventToBuffer()
have to set p[0] = len ???????? - will be used inside 
	  */
      status = evWrite(handle1, (uint32_t *)pstart);
      if(status!=0) printf("evWrite returns %d\n",status);
      evGetBufferLength(handle1, (uint32_t *)&len);
printf("get buffer length=%d\n",len);
      status = evClose(handle1);
      if(status!=0) printf("evClose returns %d\n",status);

      evGetBufferLength(handle1, (uint32_t *)&len);
printf("get buffer length=%d\n",len);


#else

      EVIO_RECORD_HEADER(p, pstart[0]+1);

#endif

      printf("p[0]=%d, pstart[0]=%d\n",p[0],pstart[0]);

	  len = (pstart[0]+1+8)<<2;
printf("filled et event length=%d\n",len);

     /* pstart[0] contains exclusive event length, add 1 for itself and 8 for record header */
      et_event_setlength(et_event_ptr, len);

	  /*check*/
      et_event_getlength(et_event_ptr, &len); /*get event length from et*/
      printf("check et event length=%d\n",len);


#ifdef REMOTE_ET


      /*****************************************************************/
      /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
      /* sergey: we assume that local host is sparc and destination is */
      /* intel, so we'll swap BOS record and set ET endian equal to    */
      /* ET_ENDIAN_LITTLE=1; in future it must be done such way so it  */
      /* will check the necessity of swapping                          */
      /*****************************************************************/


      /****************** do it only if necessary !!!!!!!!!!!!!!!!!!!! 
      BOSrecordSwap((unsigned int *)pstart, (unsigned int *)pstart);

      {
        int endian;
        et_event_getendian(et_event_ptr,&endian);
        et_event_setendian(et_event_ptr,ET_ENDIAN_LITTLE);
        et_event_getendian(et_event_ptr,&endian);
      }
      */


#endif

      // insert event into ET system
      if(et_alive(et_system_id)==1)
      {
        status = et_event_put(et_system_id,et_attach_id,et_event_ptr);
        if(status==ET_OK)
        {
          printf("Successfully put event into ET system, status=%d\n",status);
          nev_to_et++;
        }
        else
        {
          if(debug==1) std::cerr << "?unable to put event, status is: " << status << std::endl;
          nev_no_et++;
        }
      }



  
	}



};

#endif
