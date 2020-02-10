
#define REMOTE_ET
#define USE_ACTIVEMQ

//  ipcbank2et
//
//  inserts bos bank events into data stream 
//
//  ejw, 19-oct-00


// for posix
#define _POSIX_SOURCE_ 1
#define __EXTENSIONS__


#define EVIO_RECORD_HEADER(myptr, mylen) \
  myptr[0] = mylen + 8; /* Block Size including 8 words header + mylen event size  */ \
  myptr[1] = 1; /* block number */ \
  myptr[2] = 8; /* Header Length = 8 (EV_HDSIZ) */ \
  myptr[3] = 1; /* event count (1 in our case) */ \
  myptr[4] = 0; /* Reserved */ \
  myptr[5] = 0x204; /*evio version (10th bit indicates last block ?)*/ \
  myptr[6] = 1; /* Reserved */ \
  myptr[7] = 0xc0da0100; /* EV_MAGIC */



#define MAXBUF 100000
static int maxbufbytes = MAXBUF*4;;
static unsigned int buf[MAXBUF];
static char old_run_status[80];


#define EVENT_TYPE 31






#ifndef USE_ACTIVEMQ

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

#endif





#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#include "codautil.h"


#ifdef USE_ACTIVEMQ
#include "ipc_lib.h"
#include "MessageActionEVIO2ET.h"
#include "MessageActionControl.h"
#else
// for smartsockets
#include <rtworks/cxxipc.hxx>
// CLAS ipc
#include <clas_ipc_prototypes.h>
#endif




// for i/o


using namespace std;
#include <iomanip>
#include <iostream>

/* BOS swap */
/*sergey: temporary
#include "/usr/local/clas/devel/coda/src/et/main/cinclude/bosRecordSwap.c"
*/

#include <evio.h>

// for et
#include <et.h>


// misc variables
static char *project       	 = (char*)"clastest";
static char *session   		 = (char*)NULL;
static char *host         	 = getenv("HOST");
static char *unique_name       	 = (char*)"ipcbank2et";
static char *unique_id           = (char *) malloc(64);
static int wait_time           	 = 1;
static char *database     	 = (char*)"clasrun";
static char *destination     	 = NULL;
static int done                	 = 0;
static int nev_rec           	 = 0;
static int nev_to_et           	 = 0;
static int nev_no_run            = 0;
static int nev_no_et           	 = 0;
static int run                   = 0;
static int debug                 = 0;
static int lost_connection       = 0;
static time_t last_time          = time(NULL);
static int last_rec              = 0;
static int last_proc             = 0; 
static double rec_rate;
static double proc_rate;
static time_t now;
static char bank_source[132];
static char rcstate_file[128];
static char temp[256];

static char *run_status;
static int run_number = 0;
static int event_number = 0;



// other prototypes
void decode_command_line(int argc, char **argv);


extern "C" {
  //void status_poll_callback(T_IPC_MSG msg);
  //void control_message_callback(T_IPC_CONN, T_IPC_CONN_PROCESS_CB_DATA, T_CB_ARG);

void quit_callback(int sig);
int create_header(int *p, int evlen, int &banksize,
                  int name1, int name2, int nrun, int nevnt, int nphys, int trig);
int add_bank(int *p2ev, int evlen, 
      const char *name, int num, const char *format, int ncol, int nrow, int ndata, int &banksize, int *data);
int va_add_bank(int *p2ev, int evlen, 
      const char *name, int num, const char *format, int ncol, int nrow, int ndata, int &banksize, ...);
}


#ifdef USE_ACTIVEMQ
IpcServer &server = IpcServer::Instance();
#else
TipcSrv &server=TipcSrv::Instance();
#endif



//--------------------------------------------------------------------------

int
main(int argc,char **argv)
{
  int status;
  char *compname;
  char *comphost;

  // synch with stdio
  ios::sync_with_stdio();

  // decode command line
  decode_command_line(argc,argv);

  // set session name
  if(session==NULL)session=(char*)"clasprod";

  //sergey: use project name as database name
  database = project;

  // create unique_id from uniq_name, node, and et system name
  char *p;
  strcpy(unique_id,unique_name);

  /*
  strcat(unique_id,"_");
  p = strtok(host,".");
  printf("p=>%s<\n",p);
  strcat(unique_id,p);
  */


  /*sergey: used to be _10_clasprod for clon10 and clasprod, do not need it since we have only one ipcbank2et
  strcat(unique_id,"_");
  p = host + strlen(host) - 2;
  strcat(unique_id,p);
  strcat(unique_id,"_");
  strcat(unique_id,session);
  */


  /* found hostname where EB is running, assuming EB name starts from 'EB' */
  get_comp_name_host(getenv("EXPID"), getenv("SESSION"), (char *)"EB", &compname, &comphost);
  cout << "ipcbank2et: name=" << compname << ", host="<< comphost << endl;


#ifdef USE_ACTIVEMQ

  server.AddSendTopic(getenv("EXPID"), getenv("SESSION"), (char *)"control", (char *)"ipcbank2et");
  server.AddRecvTopic(getenv("EXPID"), getenv("SESSION"), (char *)"control", (char *)"*");

  server.AddRecvTopic(getenv("EXPID"), getenv("SESSION"), (char *)"bank2et", (char *)"*");

  server.Open();

  
  MessageActionControl *control = new MessageActionControl((char *)"ipcbank2et");
  control->setDebug(debug);
  server.AddCallback(control);
  
  /*MessageActionEVIO2ET *evio2et = new MessageActionEVIO2ET(session);*/
  MessageActionEVIO2ET *evio2et = new MessageActionEVIO2ET(session,comphost);
  server.AddCallback(evio2et);
#else
  // set ipc parameters and connect to ipc system
  ipc_set_application(project);
  ipc_set_user_status_poll_callback(status_poll_callback);
  ipc_set_control_message_callback(control_message_callback);
  ipc_set_quit_callback(quit_callback);
  status = ipc_init(unique_id,"ipcbank2et");
  if(status<0)
  {
    cerr << "\n?Unable to connect to server...probably duplicate unique id\n"
	 << "   ...check for another ipcbank2et  using ipc_info\n"
	 << "   ...only one connection allowed!" << endl << endl;
    exit(EXIT_FAILURE);
  }
  server.SubjectSubscribe((T_STR)"evt_system",TRUE);
  TipcMt mt((T_STR)"evt_bosbank");
  server.ProcessCbCreate(mt,bosbank_callback,0);
  sprintf(bank_source,"evt_bosbank/%s",session);
  server.SubjectSubscribe(bank_source,TRUE);
#endif

  evio2et->set_debug(1);

  // initialize et system
  evio2et->init_et();

  // get rcstate file name
  sprintf(rcstate_file,"%s/run_log/rcstate_%s.txt",getenv("CLON_PARMS"),session);

  // post startup message
  sprintf(temp,"Process startup:  %s  in project:  %s, session: %s",unique_id,project,session);
  //status = insert_msg("ipcbank2et","online",unique_id,"status",0,"START",0,temp);

  // flush messages and output
#ifndef USE_ACTIVEMQ
  server.Flush();
#endif
  fflush(NULL);

  strcpy(old_run_status, get_run_status(database,session));

  printf("At startup time run state is '%s' (UNIX time=%d)\n",old_run_status,time(NULL));fflush(stdout);

  // main loop
  while(done==0)
  {

    //server.MainLoop((double)wait_time);
    evio2et->set_run_status(get_run_status(database,session));

    //if(debug) printf("Geting run number from database >%s< session >%s<\n",database,session);
    evio2et->set_run_number(get_run_number(database,session));

    //printf("goto sleep, done=%d\n",control->getDone());
    sleep(wait_time);

	/* redo
    // check et
    if((epics2et->et_ok==1)&&(et_alive(et_system_id)==0))
    {
      lost_connection=1;
      done=1;
    }
	*/

    // calc rates every 10 seconds...banks come very slowly!
    now = time(NULL);
    if((now-last_time)>=10)
    {
      int delta=now-last_time;
      rec_rate=(double)(nev_rec-last_rec)/delta;
      proc_rate=(double)(nev_to_et-last_proc)/delta;
      last_time=now;
      last_rec=nev_rec;
      last_proc=nev_to_et;
	  /*
      control->sendStatistics(nev_rec, nev_to_et, rec_rate, proc_rate);
	  */
    }

    done = evio2et->get_done();
	if(done)
	{
      printf("received done=%d from evio2et\n",done);
      break;
	}

    done = control->getDone();
	if(done)
	{
      printf("received done=%d from Control\n",done);
      break;
	}
  }

  printf("exiting ..\n");

  // shutdown messages
  if(lost_connection==1)
  {
    printf("lost connection to ET - exiting\n");
    //insert_msg("ipcbank2et","online",unique_id,"status",1,"WARN",0,"ipcbank2et...lost connection to ET system");
  }
  sprintf(temp,"Process shutdown:  %s",unique_id);
  //status = insert_msg("ipcbank2et","online",unique_id,"status",0,"STOP",0,temp);

  /*redo
  // done
  if(epics2et->et_ok==1) et_forcedclose(et_system_id);
  */

#ifdef USE_ACTIVEMQ
  server.Close();
#else
  ipc_close();
#endif

  exit(EXIT_SUCCESS);
}
       

//--------------------------------------------------------------------------



//----------------------------------------------------------------

/*
void
status_poll_callback(T_IPC_MSG msg)
{  
  TipcMsgAppendStr(msg,(T_STR)"Bank source");
  TipcMsgAppendStr(msg,bank_source);

  TipcMsgAppendStr(msg,(T_STR)"nevent received");
  TipcMsgAppendInt4(msg,nev_rec);
  
  TipcMsgAppendStr(msg,(T_STR)"nevent sent to et");
  TipcMsgAppendInt4(msg,nev_to_et);
  
  TipcMsgAppendStr(msg,(T_STR)"nevent no et");
  TipcMsgAppendInt4(msg,nev_no_et);
  
  TipcMsgAppendStr(msg,(T_STR)"nevent no run");
  TipcMsgAppendInt4(msg,nev_no_run);
  
  TipcMsgAppendStr(msg,(T_STR)"wait_time");
  TipcMsgAppendInt4(msg,wait_time);

  return;
}


//-------------------------------------------------------------------


void
quit_callback(int sig)
{
  done=1;

  return;
}


//-------------------------------------------------------------------


void
control_message_callback(T_IPC_CONN conn,
			      T_IPC_CONN_PROCESS_CB_DATA data,
			      T_CB_ARG arg)
{

  T_STR string;


  // get first string
  TipcMsgSetCurrent(data->msg,0);
  TipcMsgNextStr(data->msg,&string);
  
  
  //  event transfer system status request
  if(strcasecmp(string,"evt_status_poll")==0) {

    T_STR srvnode = server.Node();

    TipcMsg status((T_STR)"evt_status");
    status.Dest((T_STR)"/evt_system/status");
    status.Sender(unique_id);
    status << unique_id << host << session << srvnode 
	   << (T_INT4) nev_rec << rec_rate << (T_INT4) nev_to_et << proc_rate
	   << (T_INT4) et_ok << (T_STR)"IPC->et:GRANDCENTRAL";
    server.Send(status,TRUE);
    server.Flush();


  //  don't understand message...ship to smartsockets interpreter
  } else {
    TutCommandParseStr(string);
  }

  return;
}
*/

//----------------------------------------------------------------------


void
decode_command_line(int argc, char**argv)
{
  const char *help = "\nusage:\n\n  ipcbank2et [-a project] [-u unique_name] [-s session]\n"
    "             [-m database] [-d destination] [-w wait_time]\n"
    "             [-debug]\n";

  // loop over all arguments, except the 1st (which is program name)
  int i=1;
  while(i<argc) {
    if(strncasecmp(argv[i],"-h",2)==0){
      cout << help << endl;
      exit(EXIT_SUCCESS);
    }
    else if (strncasecmp(argv[i],"-debug",6)==0){
      debug=1;
      i=i+1;
    }
    else if (strncasecmp(argv[i],"-a",2)==0){
      project=strdup(argv[i+1]);
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-u",2)==0){
      unique_name=strdup(argv[i+1]);
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-s",2)==0){
      session=strdup(argv[i+1]);
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-m",2)==0){
      database=strdup(argv[i+1]);
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-d",2)==0){
      destination=strdup(argv[i+1]);
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-w",2)==0){
      wait_time=atoi(argv[i+1]);
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-",1)==0) {
      cout << "Unknown command line arg: " << argv[i] << argv[i+1] << endl << endl;
      i=i+1;
    }
  }

  return;
}
