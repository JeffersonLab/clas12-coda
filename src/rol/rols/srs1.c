
/* srs1.c - first readout list for SRS, runs on regular Linux */


#define NEW


#undef SSIPC

static int nusertrig, ndone;


#undef USE_SRS


#undef DEBUG



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

#include <sys/time.h>


#ifdef SSIPC
#include <rtworks/ipc.h>
#include "epicsutil.h"
static char ssname[80];
#endif

#include "TIpcieLib.h"
#include "tipConfig.h"
#include "daqLib.h"

#include "circbuf.h"

/* from fputil.h */
#define SYNC_FLAG 0x20000000

/* readout list name */
#define ROL_NAME__ "SRS1"

/* polling mode if needed */
#define POLLING_MODE

/* main TI board */
#define TI_ADDR   (21<<19)  /* if 0 - default will be used, assuming slot 21*/



/* name used by loader */

#ifdef TI_MASTER
#define INIT_NAME srs1_master__init
#define TIP_READOUT TIP_READOUT_EXT_POLL /* Poll for available data, front panel triggers */
#else
#ifdef TI_SLAVE
#define INIT_NAME srs1_slave__init
#define TIP_READOUT TIP_READOUT_TS_POLL /* Poll for available data, triggers from master over fiber */
#else
#define INIT_NAME srs1__init
#define TIP_READOUT TIP_READOUT_EXT_POLL /* Poll for available data, front panel triggers */
#endif
#endif

#include "rol.h"

void usrtrig(unsigned int EVTYPE, unsigned int EVSOURCE);
void usrtrig_done();






#define BLOCKLEVEL 1
#define BUFFERLEVEL  8

/* TRIG_MODE = TIP_READOUT_EXT_POLL for polling mode (External front panel input) */
/*           = TIP_READOUT_TS_POLL  for polling mode (Fiber input from TS) */
/* Fix from Bryan 17may16 */
#define TRIG_MODE TIP_READOUT


/*  PULSER_TRIGGER */
/*    is for pedestal and/or debugging. */
/*   Set to 0 to use Front Panel inputs or TS Fiber Connection (TRIG_MODE above)*/
#undef PULSER_TRIGGER


#if 0
#define PULSER_TYPE          0
#define PULSER_FIXED_NUMBER  10
#define PULSER_FIXED_PERIOD  10000/*27%live 6.8kHz 205MB/sec 81usROL1*/
#define PULSER_FIXED_LONG    0
#endif

#if 1
/* PULSER_TYPE   0 - Fixed   1 - Random*/
#define PULSER_TYPE          1
/* PULSER_FIXED  */
#define PULSER_FIXED_NUMBER  BLOCKLEVEL
#define PULSER_FIXED_PERIOD  2
#define PULSER_FIXED_LONG    1
/* PULSER_RANDOM_FREQ where arg sets frequency to 500kHz/(2^(arg-1))*/
#endif

#define PULSER_RANDOM_FREQ   0x7 /*0x6 ~15kHz */



#define DO_SRS_READOUT  1

#ifdef USE_SRS

#include "srsLib.h"

int srsFD[MAX_FEC];
char FEC[MAX_FEC][100];
int nfec=1;

#endif

#define TIR_SOURCE 1
#include "GEN_source.h"



static char rcname[5];

#define NBOARDS 22    /* maximum number of VME boards: we have 21 boards, but numbering starts from 1 */
#define MY_MAX_EVENT_LENGTH 3000/*3200*/ /* max words per board */
static unsigned int tdcbuf[65536];

/*#ifdef DMA_TO_BIGBUF*/
/* must be 'rol' members, like dabufp */
extern unsigned int dabufp_usermembase;
extern unsigned int dabufp_physmembase;
/*#endif*/

extern int rocMask; /* defined in roc_component.c */

#define NTICKS 1000 /* the number of ticks per second */
/*temporary here: for time profiling */





#define ABS(x)      ((x) < 0 ? -(x) : (x))

#define TIMERL_VAR \
  static hrtime_t startTim, stopTim, dTim; \
  static int nTim; \
  static hrtime_t Tim, rmsTim, minTim=10000000, maxTim, normTim=1

#define TIMERL_START \
{ \
  startTim = gethrtime(); \
}

#define TIMERL_STOP(whentoprint_macros,histid_macros) \
{ \
  stopTim = gethrtime(); \
  if(stopTim > startTim) \
  { \
    nTim ++; \
    dTim = stopTim - startTim; \
    /*if(histid_macros >= 0)   \
    { \
      uthfill(histi, histid_macros, (int)(dTim/normTim), 0, 1); \
    }*/														\
    Tim += dTim; \
    rmsTim += dTim*dTim; \
    minTim = minTim < dTim ? minTim : dTim; \
    maxTim = maxTim > dTim ? maxTim : dTim; \
    /*logMsg("good: %d %ud %ud -> %d\n",nTim,startTim,stopTim,Tim,5,6);*/ \
    if(nTim == whentoprint_macros) \
    { \
      logMsg("timer: %7llu microsec (min=%7llu max=%7llu rms**2=%7llu)\n", \
                Tim/nTim/normTim,minTim/normTim,maxTim/normTim, \
                ABS(rmsTim/nTim-Tim*Tim/nTim/nTim)/normTim/normTim,5,6); \
      nTim = Tim = 0; \
    } \
  } \
  else \
  { \
    /*logMsg("bad:  %d %ud %ud -> %d\n",nTim,startTim,stopTim,Tim,5,6);*/ \
  } \
}



/*dummy to resolve reference*/
int
getTdcTypes(int *xxx)
{
  return(0);
}
int
getTdcSlotNumbers(int *xxx)
{
  return(0);
}

/*sergey: ??? */
extern struct TI_A24RegStruct *TIp;
/*sergey: ??? */
static int ti_slave_fiber_port = 1;



static void
__download()
{
  int i1, i2, i3;
#ifdef POLLING_MODE
  rol->poll = 1;
#else
  rol->poll = 0;
#endif

  printf("download 1\n");fflush(stdout);


  printf("\n>>>>>>>>>>>>>>> ROCID=%d, CLASSID=%d <<<<<<<<<<<<<<<<\n",rol->pid,rol->classid);
  printf("CONFFILE >%s<\n\n",rol->confFile);
  printf("LAST COMPILED: %s %s\n", __DATE__, __TIME__);

  printf("USRSTRING >%s<\n\n",rol->usrString);

  /* if slave, get fiber port number from user string */

//  ti_slave_fiber_port = 1; /* default */
/*
  ch = strstr(rol->usrString,"fp=");
  if(ch != NULL)
  {
    strcpy(tmp,ch+strlen("fp="));
    printf("tmp >>>>>>>>>>>>>>>>>>>>>%s<<<<<<<<<<<<<<<<<<<<<\n",tmp);
    ti_slave_fiber_port = atoi(tmp);
    printf("ti_slave_fiber_port =%d\n",ti_slave_fiber_port);
    tiSetFiberIn_preInit(ti_slave_fiber_port);
  }
*/
  /*
  printf("srs1: downloading DDL table ...\n");
  clonbanks_();
  printf("srs1: ... done.\n");
  */


  /**/
  CTRIGINIT;
  CDOINIT(GEN,TIR_SOURCE);


  /************/
  /* init daq */

  /*FOR NOW*/
  daqInit();
  DAQ_READ_CONF_FILE;
  

  /*************************************/
  /* redefine TI settings if neseccary */

  tipSetUserSyncResetReceive(1);

#ifndef TI_SLAVE
  /* TS 1-6 create physics trigger, no sync event pin, no trigger 2 */
vmeBusLock();
  tipLoadTriggerTable(0/*was 3*/);
  /*was tiSetTriggerWindow(7);*/	/* (7+1)*4ns trigger it coincidence time to form trigger type */
vmeBusUnlock();
#endif

  /*********************************************************/
  /*********************************************************/


  /******************/
  /* USER code here */

#ifdef USE_SRS

  /*************************************************************
   * Setup SRS 
   */

  nfec=0;

  /* testing before run
  strncpy(FEC[nfec++], "10.0.0.2",100);
  strncpy(FEC[nfec++], "10.0.3.2",100);
  strncpy(FEC[nfec++], "10.0.8.2",100);
  */

  /* from Bryan on May 13, 2016 */

  if (rol->pid == 7) /* clondaq6 */
  {
    strncpy(FEC[nfec++], "10.0.0.2",100);
    strncpy(FEC[nfec++], "10.0.2.2",100);
    strncpy(FEC[nfec++], "10.0.3.2",100);
    strncpy(FEC[nfec++], "10.0.8.2",100);
  }
  else if (rol->pid == 8) /* clondaq8 */
  {
    strncpy(FEC[nfec++], "10.0.4.2",100);
    strncpy(FEC[nfec++], "10.0.5.2",100); 
    strncpy(FEC[nfec++], "10.0.6.2",100); 
    strncpy(FEC[nfec++], "10.0.7.2",100); 
  }
  else
  {
    printf("ERROR1: SRS HAS WRONG PID=%d\n",rol->pid);
    exit(0);
  }

  char hosts[MAX_FEC][100];
  int ifec=0;
  int starting_port = 7000;

  memset(&srsFD, 0, sizeof(srsFD));


  /* was when both were in clondaq6
 for(ifec=0; ifec<4; ifec++)
  {
    sprintf(hosts[ifec],"10.0.0.%d",ifec+3);
    srsSetDAQIP(FEC[ifec], hosts[ifec], starting_port+ifec);
    srsConnect((int*)&srsFD[ifec], hosts[ifec], starting_port+ifec);
  }

  for(ifec=4; ifec<nfec; ifec++)
  {
    sprintf(hosts[ifec],"10.0.4.%d",ifec-1);
    srsSetDAQIP(FEC[ifec], hosts[ifec], starting_port+ifec);
    srsConnect((int*)&srsFD[ifec], hosts[ifec], starting_port+ifec);
  }
  */


  /* Associate FECs with Specific Host IPs and ports */
  if (rol->pid == 7) /* clondaq6 */
  {
    for(ifec=0; ifec<nfec; ifec++)
    {
      sprintf(hosts[ifec],"10.0.0.%d",ifec+3);
      srsSetDAQIP(FEC[ifec], hosts[ifec], starting_port+ifec);
      srsConnect((int*)&srsFD[ifec], hosts[ifec], starting_port+ifec);
    }
  }
  else if (rol->pid == 8) /* clondaq8 */
  {
    for(ifec=0; ifec<nfec; ifec++)
    {
      sprintf(hosts[ifec],"10.0.0.%d",ifec+3);
      srsSetDAQIP(FEC[ifec], hosts[ifec], starting_port+ifec);
      srsConnect((int*)&srsFD[ifec], hosts[ifec], starting_port+ifec);
    }
  }
  else
  {
    printf("ERROR2: SRS HAS WRONG PID=%d\n",rol->pid);
    exit(0);
  }


  /* Configure FEC */
  for(ifec=0; ifec<nfec; ifec++)
  {
    /* Same as call to 
    srsExecConfigFile("config/set_IP10012.txt"); */
    srsSetDTCC(FEC[ifec], 
		 1, // int dataOverEth
		 0, // int noFlowCtrl 
		 2, // int paddingType
		 0, // int trgIDEnable
		 0, // int trgIDAll
		 4, // int trailerCnt
		 0xaa, // int paddingByte
		 0xdd  // int trailerByte
		 );

    /* Same as call to 
	srsExecConfigFile("config/adc_IP10012.txt"); */
    srsConfigADC(FEC[ifec],
		   0xffff, // int reset_mask
		   0, // int ch0_down_mask
		   0, // int ch1_down_mask
		   0, // int eq_level0_mask
		   0, // int eq_level1_mask
		   0, // int trgout_enable_mask
		   0xffff // int bclk_enable_mask
		   );

    /* Same as call to 
	srsExecConfigFile("config/fecCalPulse_IP10012.txt"); */
    srsSetApvTriggerControl(FEC[ifec],
			      4+0, // int mode
			      0, // int trgburst (x+1)*3 time bins
			      0x4, // int freq
			      0x61, // int trgdelay 61(tage total sum) 5f(scintillator trigger) 60(MASTER OR)
			      0x7f, // int tpdelay
			      0x9f // int rosync
			      );
    if(rol->pid == 8)
      {
	srsSetEventBuild(FEC[ifec],
			 0x1ff, // int chEnable
			 550, // int dataLength
			 2, // int mode
			 0, // int eventInfoType
			 0xaa000bb8 | ((ifec+4)<<16) // unsigned int eventInfoData
			 );
      }
    else /* rol->pid == 7 */
      {
	srsSetEventBuild(FEC[ifec],
			 0x1ff, // int chEnable
			 550, // int dataLength
			 2, // int mode
			 0, // int eventInfoType
			 0xaa000bb8 | ((ifec)<<16) // unsigned int eventInfoData
			 );
      }
	
    /* Same as call to 
	srsExecConfigFile("config/apv_IP10012.txt"); */
    srsAPVConfig(FEC[ifec], 
		   0xff, // int channel_mask, 
		   0x03, // int device_mask,
		   0x19, // int mode, 
		   0x80, // int latency, 
		   0x2, // int mux_gain, 
		   0x62, // int ipre, 
		   0x34, // int ipcasc, 
		   0x22, // int ipsf, 
		   0x22, // int isha, 
		   0x22, // int issf, 
		   0x37, // int ipsp, 
		   0x10, // int imuxin, 
		   0x64, // int ical, 
		   0x28, // int vsps,
		   0x3c, // int vfs, 
		   0x1e, // int vfp, 
		   0xef, // int cdrv, 
		   0xf7 // int csel
		   );

    /* Same as call to 
	srsExecConfigFile("/daqfs/home/moffit/work/SRS/test/config/fecAPVreset_IP10012.txt"); */
    srsAPVReset(FEC[ifec]);

    /* Same as call to 
	srsExecConfigFile("config/pll_IP10012.txt"); */
    srsPLLConfig(FEC[ifec], 
		   0xff, // int channel_mask,
		   0, // int fine_delay, 
		   0 // int trg_delay
		   );
      
  }

  for(ifec=0; ifec<nfec; ifec++)
  {
    srsStatus(FEC[ifec],0);
  }
  
#endif

  tipStatus(0);

  sprintf(rcname,"RC%02d",rol->pid);
  printf("rcname >%4.4s<\n",rcname);

#ifdef SSIPC
  sprintf(ssname,"%s_%s",getenv("HOST"),rcname);
  printf("Smartsockets unique name >%s<\n",ssname);
  epics_msg_sender_init(getenv("EXPID"), ssname); /* SECOND ARG MUST BE UNIQUE !!! */
#endif

  logMsg("INFO: User Download Executed\n",1,2,3,4,5,6);
}



static void
__prestart()
{
  int ii, i1, i2, i3;
  int ret;

  /* Clear some global variables etc for a clean start */
  *(rol->nevents) = 0;
  event_number = 0;
  /* was tiEnableVXSSignals();*/

#ifdef POLLING_MODE
  CTRIGRSS(GEN, TIR_SOURCE, usrtrig, usrtrig_done);
#else
  CTRIGRSA(GEN, TIR_SOURCE, usrtrig, usrtrig_done);
#endif

  printf(">>>>>>>>>> next_block_level = %d, block_level = %d, use next_block_level\n",next_block_level,block_level);
  block_level = next_block_level;


  /**************************************************************************/
  /* setting TI busy conditions, based on boards found in Download          */
  /* tiInit() does nothing for busy, tiConfig() sets fiber, we set the rest */
  /* NOTE: if ti is busy, it will not send trigger enable over fiber, since */
  /*       it is the same fiber and busy has higher priority                */

#ifndef TI_SLAVE
vmeBusLock();
tipSetBusySource(TIP_BUSY_FP, 0);
/*FOR NOW !!!!!!!!!!!!!!
tiSetBusySource(TIP_BUSY_LOOPBACK,0);
*/
vmeBusUnlock();
#endif


#ifdef TI_SLAVE
vmeBusLock();
tipSetBusySource(TIP_BUSY_FP_FADC, 0);
  /* tipSetBusySource(0, 1); */
  printf("TIpcie: Setting SyncSrc to HFBR1 and Loopback\n");
  tipSetSyncSource(TIP_SYNC_HFBR1 | TIP_SYNC_LOOPBACK);

  /*sergey: tipSetSyncSource() resets register 0x24 (sync), so we'll loose UserSyncReset enabling, reinstall it*/
  tipSetUserSyncResetReceive(1);

  tipSetInstantBlockLevelChange(1);
vmeBusUnlock();

#endif



#ifdef USE_SRS

 int ifec=0, nframes=0, dCnt=0;
 for(ifec=0; ifec<nfec; ifec++)
   {
     srsStatus(FEC[ifec],0);
   }
 
 /* Check SRS buffers and clear them */
 dCnt = srsCheckAndClearBuffers((int *)&srsFD, nfec,
				(volatile unsigned int *)tdcbuf,
				2*80*1024, 1, &nframes);
 if(dCnt>0)
    {
      printf("SYNC ERROR: SRS had extra data at SyncEvent.\n");
    }
 
#endif /* USE_SRS */


  /*
  if(nfadc>0)
  {
    printf("Set BUSY from SWB for FADCs\n");
vmeBusLock();
    tiSetBusySource(TI_BUSY_SWB,0);
vmeBusUnlock();
  }
  */



  /* USER code here */
  /******************/



#if 0 /* FOR NOW */





vmeBusLock();
  tipIntDisable();
vmeBusUnlock();


  /* master and standalone crates, NOT slave */
#ifndef TI_SLAVE

  sleep(1);
vmeBusLock();
  tipSyncReset(1);
vmeBusUnlock();
  sleep(1);
vmeBusLock();
  tipSyncReset(1);
vmeBusUnlock();
  sleep(1);

vmeBusLock();
  ret = tipGetSyncResetRequest();
vmeBusUnlock();
  if(ret)
  {
    printf("ERROR: syncrequest still ON after tiSyncReset(); trying again\n");
    sleep(1);
vmeBusLock();
    tipSyncReset(1);
vmeBusUnlock();
    sleep(1);
  }

vmeBusLock();
  ret = tipGetSyncResetRequest();
vmeBusUnlock();
  if(ret)
  {
    printf("ERROR: syncrequest still ON after tiSyncReset(); try 'tcpClient <rocname> tiSyncReset'\n");
  }
  else
  {
    printf("INFO: syncrequest is OFF now\n");
  }
  /*FOR NOW
  printf("holdoff rule 1 set to %d\n",tiGetTriggerHoldoff(1));
  printf("holdoff rule 2 set to %d\n",tiGetTriggerHoldoff(2));
  */

#endif

/* set block level in all boards where it is needed;
   it will overwrite any previous block level settings */

/* Fix from Bryan 17may16 */
#ifndef TI_SLAVE /* TI-slave gets its blocklevel from the TI-master (through fiber) */
vmeBusLock();
  tipSetBlockLevel(block_level);
vmeBusUnlock();
#endif






#endif














vmeBusLock();
  tipStatus(1);
vmeBusUnlock();

  printf("INFO: Prestart1 Executed\n");fflush(stdout);

  *(rol->nevents) = 0;
  rol->recNb = 0;

  return;
}       

static void
__end()
{
  int iwait=0;
  int blocksLeft=0;
  int id;
  int ifec=0;

  printf("\n\nINFO: End1 Reached\n");fflush(stdout);

  CDODISABLE(GEN,TIR_SOURCE,0);

  /* Before disconnecting... wait for blocks to be emptied */
vmeBusLock();
  blocksLeft = tipBReady();
vmeBusUnlock();
  printf(">>>>>>>>>>>>>>>>>>>>>>> %d blocks left on the TI\n",blocksLeft);fflush(stdout);
  if(blocksLeft)
  {
    printf(">>>>>>>>>>>>>>>>>>>>>>> before while ... %d blocks left on the TI\n",blocksLeft);fflush(stdout);
    while(iwait < 10)
	{
      sleep(1);
	  if(blocksLeft <= 0) break;
vmeBusLock();
	  blocksLeft = tipBReady();
      printf(">>>>>>>>>>>>>>>>>>>>>>> inside while ... %d blocks left on the TI\n",blocksLeft);fflush(stdout);
vmeBusUnlock();
	  iwait++;
	}
    printf(">>>>>>>>>>>>>>>>>>>>>>> after while ... %d blocks left on the TI\n",blocksLeft);fflush(stdout);
  }


#ifdef USE_SRS
  for(ifec=0; ifec<nfec; ifec++)
  {
	srsTrigDisable(FEC[ifec]);
    srsStatus(FEC[ifec],0);
  }
#endif


vmeBusLock();
  tipStatus(1);
vmeBusUnlock();

  printf("INFO: End1 Executed\n\n\n");fflush(stdout);

  return;
}


static void
__pause()
{
  CDODISABLE(GEN,TIR_SOURCE,0);
  logMsg("INFO: Pause Executed\n",1,2,3,4,5,6);
  
} /*end pause */


static void
__go()
{
  int ii, jj, id, slot;

  logMsg("INFO: Entering Go 1\n",1,2,3,4,5,6);

/* #ifdef DOSELFSYNC */
/* #ifdef TI_SLAVE */
/*   printf("TIpcie: SyncReseting itself - temporary work-around\n"); */
/* vmeBusLock(); */
/*   tipSyncReset(0); */
/*   usleep(100); */
/* vmeBusUnlock(); */
/* #endif */
/* #endif */

#ifndef TI_SLAVE
  /* set sync event interval (in blocks) */
vmeBusLock();
 tipSetSyncEventInterval(0/*10000*//*block_level*/);
vmeBusUnlock();
/* Fix from Bryan 17may16 */
#else
/* Block level was broadcasted from TI-Master / TS */
 block_level = tipGetCurrentBlockLevel();
 printf("rocGo: Block Level set to %d\n",block_level);
 tipSetBlockBufferLevel(0);

#endif

#ifdef USE_SRS
  int ifec=0;

  for(ifec=0; ifec<nfec; ifec++)
  {
    srsTrigEnable(FEC[ifec]);
    srsStatus(FEC[ifec],0);
  }

#endif

  tipStatus(1);

  nusertrig = 0;
  ndone = 0;

  CDOENABLE(GEN,TIR_SOURCE,0); /* bryan has (,1,1) ... */

  logMsg("INFO: Go 1 Executed\n",1,2,3,4,5,6);
}



void
usrtrig(unsigned int EVTYPE, unsigned int EVSOURCE)
{
  int *jw, ind, ind2, i, ii, jj, jjj, blen, len, rlen, nbytes;
  unsigned int *tdc;
  unsigned int *dabufp1, *dabufp2;
  int njjloops, slot;
  int nwords, dCnt;
  TIMERL_VAR;
  char *chptr, *chptr0;

  /*printf("EVTYPE=%d syncFlag=%d\n",EVTYPE,syncFlag);*/



  if(syncFlag) printf("EVTYPE=%d syncFlag=%d\n",EVTYPE,syncFlag);

  rol->dabufp = NULL;

  /*
usleep(100);
  */
  /*
  sleep(1);
  */

  CEOPEN(EVTYPE, BT_BANKS); /* reformatted on CODA_format.c !!! */

  if((syncFlag<0)||(syncFlag>1))         /* illegal */
  {
    printf("Illegal1: syncFlag=%d EVTYPE=%d\n",syncFlag,EVTYPE);
  }
  else if((syncFlag==0)&&(EVTYPE==0))    /* illegal */
  {
    printf("Illegal2: syncFlag=%d EVTYPE=%d\n",syncFlag,EVTYPE);
  }
  else if((syncFlag==1)&&(EVTYPE==0))    /* force_sync (scaler) events */
  {
    ;
/*
!!! we are geting here on End transition: syncFlag=1 EVTYPE=0 !!!
*/
  }
  else if((syncFlag==0)&&(EVTYPE==15)) /* helicity strob events */
  {
    ;
  }
  else           /* physics and physics_sync events */
  {


TIMERL_START;

    /*************/
	/* TI stuff */

    /* Set high, the first output port 
    tiSetOutputPort(1,0,0,0);
    */

    /* Grab the data from the TI */
    len = tipReadBlock(tdcbuf,1024,0);
    if(len<=0)
    {
      printf("ERROR in tipReadBlock : No data or error, len = %d\n",len);
      sleep(1);

      if(TRIG_MODE==TIP_READOUT_EXT_POLL) tipSetBlockLimit(1); /* Effectively... stop triggers */

    }
    else
    {
	  ;
	  
      printf("ti: len=%d\n",len);
      for(jj=0; jj<len; jj++) printf("ti[%2d] 0x%08x (0x%08x)\n",jj,tdcbuf[jj],LSWAP(tdcbuf[jj]));
	  

	  
      BANKOPEN(0xe10A,1,rol->pid);
      for(jj=0; jj<len; jj++) *rol->dabufp++ = LSWAP(tdcbuf[jj]);
      BANKCLOSE;
	  
    }
    /* Turn off all output ports 
    tiSetOutputPort(0,0,0,0);
    */
	/* TI stuff */
    /*************/




#ifdef DEBUG
    printf("fadc1: start fadc processing\n");fflush(stdout);
#endif





/*usleep(1);*/ /*1-55us, 50-104us, 70-126us*/




#ifdef USE_SRS

    dCnt=0;

  /************************************************************
   * SRS READOUT
   */

  int ifec=0;
  int nframes=0;
  int SRS_FEC_BASE=0;

  if(rol->pid == 8) /* clondaq8 */
    {
      SRS_FEC_BASE = 4;
    }
  for(ifec=0; ifec<nfec; ifec++)
  {
    dCnt=0;
    BANKOPEN(0xe11f,1,5+ifec+SRS_FEC_BASE);

    dCnt = srsReadBlock(srsFD[ifec],
			   (volatile unsigned int *)rol->dabufp,
			   2*80*1024, block_level, &nframes);
    if(dCnt<=0)
	{
	  printf("**************************************************\n");
	  printf("No SRS data or error.  dCnt = %d\n",dCnt);
	  printf("**************************************************\n");
	}
    else
	{
	  /* Bump data buffer by the amount of data received */
	  rol->dabufp += dCnt;
	}

    BANKCLOSE;
  }

#endif


#if 0
#ifndef TI_SLAVE

  /* create HEAD bank if master and standalone crates, NOT slave */

	event_number = (EVENT_NUMBER) * block_level - block_level;

    BANKOPEN(0xe112,1,0);

	dabufp1 = rol->dabufp;

    *rol->dabufp ++ = LSWAP((0x10<<27)+block_level); /*block header*/

    for(ii=0; ii<block_level; ii++)
	{
      event_number ++;
	  /*
	  printf(">>>>>>>>>>>>> %d %d\n",(EVENT_NUMBER),event_number);
      sleep(1);
	  */
      *rol->dabufp ++ = LSWAP((0x12<<27)+(event_number&0x7FFFFFF)); /*event header*/

      nwords = 5; /* UPDATE THAT IF THE NUMBER OF WORDS CHANGED BELOW !!! */
      *rol->dabufp ++ = LSWAP((0x14<<27)+nwords); /*head data*/
      *rol->dabufp ++ = 0; /*version  number */
      *rol->dabufp ++ = LSWAP(RUN_NUMBER); /*run  number */
      *rol->dabufp ++ = LSWAP(event_number); /*event number */
      if(ii==(block_level-1))
	  {
        *rol->dabufp ++ = LSWAP(time(0)); /*event unix time */
        *rol->dabufp ++ = LSWAP(EVTYPE); /*event type */
	  }
      else
	  {
        *rol->dabufp ++ = 0;
        *rol->dabufp ++ = 0;
	  }
	}

    nwords = ((int)rol->dabufp-(int)dabufp1)/4+1;
	/*printf("nwords=%d\n",nwords);*/

    *rol->dabufp ++ = LSWAP((0x11<<27)+nwords); /*block trailer*/

    BANKCLOSE;

#endif
#endif



TIMERL_STOP(100000/block_level,1000+rol->pid);






#if 1 /* enable/disable sync events processing */


    /* read boards configurations */
    if(syncFlag==1 || EVENT_NUMBER==1)
    {
      printf("SYNC: read boards configurations\n");

      BANKOPEN(0xe10E,3,rol->pid);
      chptr = chptr0 =(char *)rol->dabufp;
      nbytes = 0;

      /* add one 'return' to make evio2xml output nicer */
      *chptr++ = '\n';
      nbytes ++;

#ifdef TIP_UPLOADALL_NOTDEFINED      
vmeBusLock();
      len = tipUploadAll(chptr, 10000);
vmeBusUnlock();
#endif

/*printf("\nTI len=%d\n",len);
      printf(">%s<\n",chptr);*/
      chptr += len;
      nbytes += len;

#ifdef USE_SRS_HIDE
      if(nfec>0)
      {
vmeBusLock();
        len = srsUploadAll(chptr, 10000);
vmeBusUnlock();
        /*printf("\nFADC len=%d\n",len);
        printf("%s\n",chptr);*/
        chptr += len;
        nbytes += len;
	  }
#endif

      /* 'nbytes' does not includes end_of_string ! */
      chptr[0] = '\n';
      chptr[1] = '\n';
      chptr[2] = '\n';
      chptr[3] = '\n';
      nbytes = (((nbytes+1)+3)/4)*4;
      chptr0[nbytes-1] = '\0';

      nwords = nbytes/4;
      rol->dabufp += nwords;

      BANKCLOSE;

      printf("SYNC: read boards configurations - done\n");
	}





    /* read scaler(s) */
    if(syncFlag==1 || EVENT_NUMBER==1)
    {
      printf("SYNC: read scalers\n");
	}


#ifndef TI_SLAVE
    /* print livetite */
    if(syncFlag==1)
	{
      printf("SYNC: livetime\n");

      int livetime, live_percent;
vmeBusLock();
      tipLatchTimers();
      livetime = tipLive(0);
vmeBusUnlock();
      live_percent = livetime/10;
	  printf("============= Livetime=%3d percent\n",live_percent);
#ifdef SSIPC
	  {
        int status;
        status = epics_msg_send("hallb_livetime","int",1,&live_percent);
	  }
#endif
      printf("SYNC: livetime - done\n");
	}
#endif


    /* for physics sync event, make sure all board buffers are empty */
    if(syncFlag==1)
    {
      printf("SYNC: make sure all board buffers are empty\n");

      /* Check TIpcie */
      int nblocks;
      nblocks = tipGetNumberOfBlocksInBuffer();
      /*printf(" Blocks ready for readout: %d\n\n",nblocks);*/
      
      if(nblocks > 1) /* TIpcie Blocks decrement on readout acknowledge */
	{
	  printf("SYNC ERROR: TI nblocks = %d\n",nblocks);fflush(stdout);
	  sleep(10);
	}

#ifdef USE_SRS
      /* Check SRS */
      dCnt = srsCheckAndClearBuffers(&srsFD, nfec,
				     (volatile unsigned int *)tdcbuf,
				     2*80*1024, block_level, &nframes);
      if(dCnt>0)
	{
	  printf("SYNC ERROR: SRS had extra data at SyncEvent.\n");
	}
	  
#endif /* USE_SRS */
      
      printf("SYNC: make sure all board buffers are empty - done\n");
	}


#endif /* if 0 */







  }

  /* close event */
  CECLOSE;

  /*
  nusertrig ++;
  printf("usrtrig called %d times\n",nusertrig);fflush(stdout);
  */
  return;
}





void
usrtrig_done()
{
  return;
}

void
__done()
{
  /*
  ndone ++;
  printf("_done called %d times\n",ndone);fflush(stdout);
  */
  /* from parser */
  poolEmpty = 0; /* global Done, Buffers have been freed */

  /* Acknowledge tir register */
  CDOACK(GEN,TIR_SOURCE,0);

  // PULSER ACTION, if needeD
#ifdef PULSER_TRIGGER
  if(PULSER_TYPE==0)
  {
    tipSoftTrig(1,PULSER_FIXED_NUMBER,PULSER_FIXED_PERIOD,PULSER_FIXED_LONG);
  }
#endif

  return;
}


static void
__status()
{
  return;
}  


int 
rocClose()
{
  printf("--- Execute rocClose ---\n");
#ifdef USE_SRS
  int ifec=0;

  for(ifec=0; ifec<nfec; ifec++)
    srsTrigDisable(FEC[ifec]);

  sleep(1);

  for(ifec=0; ifec<nfec; ifec++)
  {
    if(srsFD[ifec]) close(srsFD[ifec]);
  }
#endif
}

