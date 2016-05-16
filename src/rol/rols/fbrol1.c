
/* fbrol1.c - FASTBUS first readout list */

#if defined(VXWORKS) || defined(Linux_vme)

#define NEW


#undef SSIPC

static int nusertrig, ndone;

#define USE_FASTBUS

#undef DEBUG


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

#ifndef VXWORKS
#include <sys/time.h>
/*
typedef      long long       hrtime_t;
*/
#endif

#ifdef SSIPC
#include <rtworks/ipc.h>
#include "epicsutil.h"
static char ssname[80];
#endif

#include "circbuf.h"

/* from fputil.h */
#define SYNC_FLAG 0x20000000

/* readout list name */
#define ROL_NAME__ "FBROL1"

/* polling mode if needed */
#define POLLING_MODE

/* main TI board */
#define TI_ADDR   0x100000  /* slot 2 */

#ifdef USE_FASTBUS
#define SFI_ADDR  0xe00000
#endif

/* name used by loader */

#ifdef TI_MASTER
#define INIT_NAME fbrol1_master__init
#define TI_READOUT TI_READOUT_EXT_POLL /* Poll for available data, front panel triggers */
#else
#ifdef TI_SLAVE
#define INIT_NAME fbrol1_slave__init
#define TI_READOUT TI_READOUT_TS_POLL /* Poll for available data, triggers from master over fiber */
#else
#define INIT_NAME fbrol1__init
#define TI_READOUT TI_READOUT_EXT_POLL /* Poll for available data, front panel triggers */
#endif
#endif

#include "rol.h"



#ifdef USE_FASTBUS

#include "libsfifb.h"

/* 1881M */
#define BYPASS        0x0
#define PRIMARY_LINK  0x800
#define END_LINK      0x1000
#define MIDDLE_LINK   0x1800
#define MAX_SLOTS     26

enum MODULE_TYPE_t{
        MODULE_NONE,
        MODULE_UNKNOWN,
        MODULE_LRS1881M,
        MODULE_LRS1877,
        MODULE_TS2,
        NUM_MODULE_TYPES
};

int SCANMASK;
int MODULE_TYPE[MAX_SLOTS];
int nhitmod[MAX_SLOTS];
int Nmodules;
int CrateID;
int HeaderID;
int SlotHi, SlotLo, SlotsContiguous;
char myhostname[256];
#endif

void usrtrig(unsigned int EVTYPE, unsigned int EVSOURCE);
void usrtrig_done();

#include "TIPRIMARY_source.h"

extern void *a32slave_window;

/* user code */


static char rcname[5];

#define NBOARDS 22    /* maximum number of VME boards: we have 21 boards, but numbering starts from 1 */
#define MY_MAX_EVENT_LENGTH 3000/*3200*/ /* max words per board */
static unsigned int *tdcbuf;
static unsigned int *sfibuf;

/*#ifdef DMA_TO_BIGBUF*/
/* must be 'rol' members, like dabufp */
extern unsigned int dabufp_usermembase;
extern unsigned int dabufp_physmembase;
/*#endif*/

extern int rocMask; /* defined in roc_component.c */

#define NTICKS 1000 /* the number of ticks per second */
/*temporary here: for time profiling */




#ifndef VXWORKS

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

#endif



void
tsleep(int n)
{
#ifdef VXWORKS
  taskDelay ((sysClkRateGet() / NTICKS) * n);
#else
#endif
}



extern struct TI_A24RegStruct *TIp;
static int ti_slave_fiber_port = 1;

void
titest1()
{
  if(TIp==NULL) {printf("NULL\n");return;}
  printf("0x%08x(%d) 0x%08x(%d)\n",
		 vmeRead32(&TIp->fiberLatencyMeasurement),vmeRead32(&TIp->fiberLatencyMeasurement),
		 vmeRead32(&TIp->fiberAlignment),vmeRead32(&TIp->fiberAlignment));
}

/* for compatibility with hps1.c */
int
getTdcTypes(int *typebyslot)
{
  return(0);
}
int
getTdcSlotNumbers(int *slotnumbers)
{
  return(0);
}



static void
__download()
{
  unsigned int i1, i2, i3;
  unsigned int i2_init;
  char *ch, tmp[64];

#ifdef POLLING_MODE
  rol->poll = 1;
#else
  rol->poll = 0;
#endif

  printf("\n>>>>>>>>>>>>>>> ROCID=%d, CLASSID=%d <<<<<<<<<<<<<<<<\n",rol->pid,rol->classid);
  printf("CONFFILE >%s<\n\n",rol->confFile);
  printf("LAST COMPILED: %s %s\n", __DATE__, __TIME__);

  printf("USRSTRING >%s<\n\n",rol->usrString);
  gethostname(myhostname, 256);
  HeaderID = 0xdc0adc00 | ((rol->pid) << 20);
  /* if slave, get fiber port number from user string */
#ifdef TI_SLAVE
  ti_slave_fiber_port = 1; /* default */

  ch = strstr(rol->usrString,"fp=");
  if(ch != NULL)
  {
    strcpy(tmp,ch+strlen("fp="));
    printf("tmp >>>>>>>>>>>>>>>>>>>>>%s<<<<<<<<<<<<<<<<<<<<<\n",tmp);
    ti_slave_fiber_port = atoi(tmp);
    printf("ti_slave_fiber_port =%d\n",ti_slave_fiber_port);
    tiSetFiberIn_preInit(ti_slave_fiber_port);
  }
#endif

  /*
  printf("rol1: downloading DDL table ...\n");
  clonbanks_();
  printf("rol1: ... done.\n");
  */

  /**/
  CTRIGINIT;

  /* initialize OS windows and TI board */
#ifdef VXWORKS
  CDOINIT(TIPRIMARY);
#else
  CDOINIT(TIPRIMARY,TIR_SOURCE);
#endif

#if 0
  /* Open Slave Window for SFI Initiated Block transfers */
  vmeOpenSlaveA32(0x18000000,0x00400000);
  /*dmaPUseSlaveWindow(1);*/
#endif
  
  /*usrVmeSlaveDmaMemory(&i1, &i2, &i3);

  i2_init = i2;
  printf("tiprimarytinit: i2_init = 0x%08x\n",i2_init);

  i2_init = (i2_init & 0xFFFFFFF0);
  printf("tiprimarytinit: i2_init = 0x%08x\n",i2_init);

  i2_init = i2_init + 0x10;
  printf("tiprimarytinit: i2_init = 0x%08x\n",i2_init);
  */

  sfibuf = (unsigned int *)a32slave_window /*i2_init*/;
  printf("tiprimarytinit: sfibuf = 0x%08x\n",sfibuf);
  
  /************/
  /* init daq */

  daqInit();
  DAQ_READ_CONF_FILE;



  /*************************************/
  /* redefine TI settings if neseccary */

#ifndef TI_SLAVE
  /* TS 1-6 create physics trigger, no sync event pin, no trigger 2 */
vmeBusLock();
  tiLoadTriggerTable(3);
  tiSetTriggerWindow(7);	// (7+1)*4ns trigger it coincidence time to form trigger type
vmeBusUnlock();
#endif


  /*********************************************************/
  /*********************************************************/

#ifdef USE_FASTBUS
  /* (width + 3)*4 ns, 300 ns width */
vmeBusLock();
  tiSetPromptTriggerWidth(72);
vmeBusUnlock();
#endif


  /* set wide pulse */
vmeBusLock();
/*sergey: WAS tiSetSyncDelayWidth(1,127,1);*/
/*worked for bit pattern latch tiSetSyncDelayWidth(0x54,127,1);*/
vmeBusUnlock();

  /*usrVmeDmaSetConfig(2,5,1);*/ /*A32,2eSST,267MB/s*/
  /*usrVmeDmaSetConfig(2,5,0);*/ /*A32,2eSST,160MB/s*/
  usrVmeDmaSetConfig(2,3,0); /*A32,MBLT*/



  /*
if(rol->pid==18)
{
  usrVmeDmaSetConfig(2,3,0);
}
  */


  /*
  usrVmeDmaSetChannel(1);
  printf("===== Use DMA Channel %d\n\n\n",usrVmeDmaGetChannel());
  */

  tdcbuf = (unsigned int *)i2_from_rol1;



  /******************/
  /* USER code here */


#ifdef USE_FASTBUS

  /* initialize sfi */
vmeBusLock();
  fb_init_1(SFI_ADDR);

  /* build crate map */
  /* Reset sequencer to clear any lingering errors */
  sfiEnableSequencer();
  sfi_error_decode(0);
vmeBusUnlock();

  unsigned int slot;
  /* reset module map */
  for(slot = 0; slot < MAX_SLOTS; slot++)
  {
    MODULE_TYPE[slot] = MODULE_NONE;
  }

  /* read modules */
  SCANMASK = 0x0;
  Nmodules = 0;
  SlotHi = 0;
  SlotLo = MAX_SLOTS;
  printf("Checking Fastbus slots...\n");

  for(slot = 4;slot < MAX_SLOTS; slot++)
  {
    unsigned long mod_id;
    int status;
    /* initialize card */
vmeBusLock();
    status = fpwc(slot, 0x00000000, 0x40000000);
vmeBusUnlock();
    if(status != 0)
    {
      sfi_error_decode(1); /* clear error on sequencer */
      sleep(1);
      continue;
    }
    /* read back CSR0 */
    mod_id = 0;
vmeBusLock();
    status = fprc(slot, 0x00000000, &mod_id);
vmeBusUnlock();
    if(status != 0)
    {
      sfi_error_decode(0); /* clear error on sequencer */
      sleep(1);
      continue;
    }

    switch(mod_id>>16)
    {
    case 0x104F: /* 1881M ADC */
      MODULE_TYPE[slot] = MODULE_LRS1881M;
      SCANMASK |= 0x1<<slot;
      break;
    case 0x103C: /* 1877 TDC */
      MODULE_TYPE[slot] = MODULE_LRS1877;
      SCANMASK |= 0x1<<slot;
      break;
    case 0x0:
      MODULE_TYPE[slot] = MODULE_NONE;
      break;
    default:
      MODULE_TYPE[slot] = MODULE_UNKNOWN;
    }

    if(   (MODULE_TYPE[slot] == MODULE_LRS1881M)
       || (MODULE_TYPE[slot] == MODULE_LRS1877)
      )
    {
      Nmodules++;
      if(slot>SlotHi) SlotHi=slot;
      if(slot<SlotLo) SlotLo=slot;
    }
  }

  sleep(1); /* allow stderr to flush out for cleaner display */
  printf("\nModules detected:%d <%s>\n",Nmodules, myhostname);
  for(slot = 0; slot < MAX_SLOTS; slot++)
  {
    switch(MODULE_TYPE[slot])
    {
    case MODULE_LRS1881M:   printf("\t slot %2d  -- LRS1881M ADC\n", slot); break;
    case MODULE_LRS1877 :   printf("\t slot %2d  -- LRS1877  TDC\n", slot); break;
    }
  }
  printf("SCANMASK=0x%x : ", SCANMASK);
  for(slot = 0; slot < MAX_SLOTS; slot++)
  {
    printf("%d", (SCANMASK>>(MAX_SLOTS-slot-1))&0x1);
  }
  printf("\n\n");

  /* ---- ENABLE MULTI-BLOCK READS ---- */
  SlotsContiguous = 1;
  printf("Multi-block read %s enabled.\n", SlotsContiguous ? "":"not");

  /* Reset/Clear all modules */
  unsigned int csr0, csr1;
  int status;
  for(slot = 0; slot < MAX_SLOTS; slot++)
  {
    switch(MODULE_TYPE[slot])
    {
    case MODULE_LRS1881M:
    case MODULE_LRS1877:
      status = fpwc(slot, 0x00000000, 0x40000000); /* initialize card */
      break;
    default:
      status = 0;
      break;
    }

    if(status != 0) sfi_error_decode(0); /* clear any error on sequencer */
  }

  /* Program LRS1881M ADC modules */
  for(slot = 0; slot < MAX_SLOTS; slot++)
  {
    if(MODULE_TYPE[slot] != MODULE_LRS1881M)
      continue;

	/*
     Try setting sparsification thresholds for this module
     This MUST happen before setting the control registers
     Set1881SparsificationThresholds(myhostname, slot);

     For crates with modules in contiguous slots, we do a multi-block
     read via a RAM list. The modules must be programmed as to where
     they are (first, middle, end) in this case.
	*/

    csr0 = 0x00000104;
    if(SlotsContiguous)
    {
      if(slot==SlotHi) {csr0 |= PRIMARY_LINK; printf("\t *** Multi-block PRIMARY ***: slot %2d \n", slot); }
      else if(slot==SlotLo) {csr0 |= END_LINK; printf("\t ***** Multi-block END *****: slot %2d \n", slot); }
      else {csr0 |= MIDDLE_LINK; printf("\t Multi-block MIDDLE: slot %2d \n", slot); }
    }
    csr1 = 0x00000043;
vmeBusLock();
    status = fpwc(slot, 0x00000000, csr0);       /* CSR0 -- enable gate */
    if(status != 0)sfi_error_decode(0);          /* clear any error on sequencer */
    status = fpwc(slot, 0x00000001, csr1);       /* CSR1 -- Use TR6 and 13bit mode */
    if(status != 0)sfi_error_decode(0);          /* clear any error on sequencer */
    status = fpwc(slot, 0x00000007, 0x00000002); /* CSR7 -- respond to broadcasts */
    if(status != 0)sfi_error_decode(0);          /* clear any error on sequencer */
vmeBusUnlock();
  }

#endif /* USE_FASTBUS */



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

  tiEnableVXSSignals();

#ifdef POLLING_MODE
  CTRIGRSS(TIPRIMARY, TIR_SOURCE, usrtrig, usrtrig_done);
#else
  CTRIGRSA(TIPRIMARY, TIR_SOURCE, usrtrig, usrtrig_done);
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
tiSetBusySource(TI_BUSY_LOOPBACK,0);
  /*tiSetBusySource(TI_BUSY_FP,0);*/
vmeBusUnlock();
#endif



#ifdef USE_FASTBUS
vmeBusLock();
  sfiEnableSequencer();
vmeBusUnlock();

  /*if(nfadc>0)*/
  {
    printf("Set BUSY from FP for FASTBUS\n");
vmeBusLock();
    tiSetBusySource(TI_BUSY_FP, 0);
vmeBusUnlock();
  }
#endif


  /* USER code here */
  /******************/




vmeBusLock();
  tiIntDisable();
vmeBusUnlock();


  /* master and standalone crates, NOT slave */
#ifndef TI_SLAVE

  sleep(1);
vmeBusLock();
  tiSyncReset(1);
vmeBusUnlock();
  sleep(1);
vmeBusLock();
  tiSyncReset(1);
vmeBusUnlock();
  sleep(1);

vmeBusLock();
  ret = tiGetSyncResetRequest();
vmeBusUnlock();
  if(ret)
  {
    printf("ERROR: syncrequest still ON after tiSyncReset(); trying again\n");
    sleep(1);
vmeBusLock();
    tiSyncReset(1);
vmeBusUnlock();
    sleep(1);
  }

vmeBusLock();
  ret = tiGetSyncResetRequest();
vmeBusUnlock();
  if(ret)
  {
    printf("ERROR: syncrequest still ON after tiSyncReset(); try 'tcpClient <rocname> tiSyncReset'\n");
  }
  else
  {
    printf("INFO: syncrequest is OFF now\n");
  }

  printf("holdoff rule 1 set to %d\n",tiGetTriggerHoldoff(1));
  printf("holdoff rule 2 set to %d\n",tiGetTriggerHoldoff(2));

#endif

/* set block level in all boards where it is needed;
   it will overwrite any previous block level settings */

#ifdef TI_SLAVE /* assume that for master and standalone TIs block level is set from config file */
vmeBusLock();
  tiSetBlockLevel(block_level);
vmeBusUnlock();
#endif



vmeBusLock();
  tiStatus(1);
vmeBusUnlock();

  printf("INFO: Prestart1 Executed\n");fflush(stdout);

  *(rol->nevents) = 0;
  rol->recNb = 0;

  return;
}       


static void
__go()
{
  int ii, jj, id, slot;

  logMsg("INFO: Entering Go 1\n",1,2,3,4,5,6);

#ifndef TI_SLAVE
  /* set sync event interval (in blocks) */
vmeBusLock();
 tiSetSyncEventInterval(0/*10000*//*block_level*/);
vmeBusUnlock();
#endif




  /* always clear exceptions */
  jlabgefClearException(1);

  nusertrig = 0;
  ndone = 0;

  CDOENABLE(TIPRIMARY,TIR_SOURCE,0); /* bryan has (,1,1) ... */

  logMsg("INFO: Go 1 Executed\n",1,2,3,4,5,6);
}


static void
__end()
{
  int iwait=0;
  int blocksLeft=0;
  int id;

  printf("\n\nINFO: End1 Reached\n");fflush(stdout);

  CDODISABLE(TIPRIMARY,TIR_SOURCE,0);

  /* Before disconnecting... wait for blocks to be emptied */
vmeBusLock();
  blocksLeft = tiBReady();
vmeBusUnlock();
  printf(">>>>>>>>>>>>>>>>>>>>>>> %d blocks left on the TI\n",blocksLeft);fflush(stdout);
  if(blocksLeft)
  {
    printf(">>>>>>>>>>>>>>>>>>>>>>> before while ... %d blocks left on the TI\n",blocksLeft);fflush(stdout);
    while(iwait < 10)
	{
      taskDelay(10);
	  if(blocksLeft <= 0) break;
vmeBusLock();
	  blocksLeft = tiBReady();
      printf(">>>>>>>>>>>>>>>>>>>>>>> inside while ... %d blocks left on the TI\n",blocksLeft);fflush(stdout);
vmeBusUnlock();
	  iwait++;
	}
    printf(">>>>>>>>>>>>>>>>>>>>>>> after while ... %d blocks left on the TI\n",blocksLeft);fflush(stdout);
  }



vmeBusLock();
  tiStatus(1);
vmeBusUnlock();

  printf("INFO: End1 Executed\n\n\n");fflush(stdout);

  return;
}


static void
__pause()
{
  CDODISABLE(TIPRIMARY,TIR_SOURCE,0);
  logMsg("INFO: Pause Executed\n",1,2,3,4,5,6);
  
} /*end pause */





void
usrtrig(unsigned int EVTYPE, unsigned int EVSOURCE)
{
  int *jw, ind, ind2, i, ii, jj, jjj, blen, len, rlen, itdcbuf, nbytes;
  unsigned int *tdcbuf_save, *tdc;
  unsigned int *dabufp1, *dabufp2;
  int njjloops, slot;
  int nwords;
#ifndef VXWORKS
  TIMERL_VAR;
#endif

#ifdef USE_FASTBUS
  int islot, nw;
  int stat, dCnt, idata;
  unsigned int sync_or_unbuff;
  unsigned int *dma_dabufp;
#endif

  char *chptr, *chptr0;

  /*printf("EVTYPE=%d syncFlag=%d\n",EVTYPE,syncFlag);*/


  if(syncFlag) printf("EVTYPE=%d syncFlag=%d\n",EVTYPE,syncFlag);

  rol->dabufp = (int *) 0;

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

    /* for EVIO format, will dump raw data */
    tdcbuf_save = tdcbuf;

    /*************/
	/* TI stuff */

    /* Set high, the first output port 
    tiSetOutputPort(1,0,0,0);
    */

    /* Grab the data from the TI */
vmeBusLock();
    len = tiReadBlock(tdcbuf,900>>2,1);
vmeBusUnlock();
    if(len<=0)
    {
      printf("ERROR in tiReadBlock : No data or error, len = %d\n",len);
      sleep(1);
    }
    else
    {
	  ;
	  /*
      printf("ti: len=%d\n",len);
      for(jj=0; jj<len; jj++) printf("ti[%2d] 0x%08x\n",jj,LSWAP(tdcbuf[jj]));
	  */

	  
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



#ifndef VXWORKS
TIMERL_START;
#endif







#ifdef USE_FASTBUS

vmeBusLock();
  sfiEnableSequencer();
vmeBusUnlock();
  sync_or_unbuff = 0;
  // Broadcast scan for data
  ii = 0;
  int datascan = 0;
  while (SCANMASK != (datascan&SCANMASK))
  {
    datascan = 0;
vmeBusLock();
    fb_frcm_1(9,0,&datascan,1,0,1,0,0,0);
vmeBusUnlock();
    if(ii++ >50) {
      sync_or_unbuff = 1; // seems to be no data when there should be!
      break;
    }
  }

  dma_dabufp = sfibuf;
  if(ii < 50)
  {
    /* Load next event -- broadcast */
vmeBusLock();
    fb_fwcm_1(0x15,0,0x400,1,0,1,0,0,0);
vmeBusUnlock();

    /* first words in crate data section (MUST BE EVEN NUMBER OF WORDS TO ENFORCE 64-BIT BOUNDARY FOR FOLLOWING DMA)*/
    *dma_dabufp++ = LSWAP(HeaderID + Nmodules);
    *dma_dabufp++ = LSWAP(rol->classid);
	
    /*unsigned int *dmaptr = (unsigned int *)vmeDmaLocalToVmeAdrs((unsigned int)dma_dabufp);*/
    unsigned int *dmaptr = (unsigned int *)usrDmaLocalToVmeAdrs((unsigned int)dma_dabufp);

    dma_dabufp[0]=dma_dabufp[1]=dma_dabufp[2]=0;
    dCnt=0;
	/*printf("befor: dma_dabufp[]: 0x%08x 0x%08x 0x%08x\n",dma_dabufp[0],dma_dabufp[1],dma_dabufp[2]);*/
vmeBusLock();
    stat = fb_frdb_1(SlotHi,0, dmaptr, MAX_EVENT_LENGTH<<2,&dCnt,1,0,1,0, 0x0C, 0,0,1); /*0x0A - D32, 0x0C - D64*/
vmeBusUnlock();
	/*printf("after: dma_dabufp[]: 0x%08x 0x%08x 0x%08x\n",dma_dabufp[0],dma_dabufp[1],dma_dabufp[2]);*/
	/*printf("after: dCnt=%d(%x)\n",dCnt,dCnt);*/
    if(stat != 0)
	{
      printf("ERROR: Read ADC block: stat=0x%x dma_dabufp=0x%08x dmaptr=0x%08x\n",stat,dma_dabufp,dmaptr);
	  /* printf("%d uint words are read\n", dCnt>>2); */
	  /* printf("0x%x  0x%x  0x%x\n",dma_dabufp[0], dma_dabufp[1], dma_dabufp[2]); */
	}
    else if(dma_dabufp[0]==0)
	{
      printf("FASTBUS ERROR - WRONG SLAVE DMA WINDOW, NEED REBOOT !\n");
	}

    dma_dabufp += dCnt>>2;
	/*    fpbr(SlotHi,1000);  */
  }
  else
  {
    /*printf("ERROR: ii=%d\n",ii);*/

    *dma_dabufp++ = LSWAP(0xdc0000ff);
    *dma_dabufp++ = LSWAP(datascan); 
    *dma_dabufp++ = LSWAP(SCANMASK); 
    *dma_dabufp++ = LSWAP(HeaderID);
    for (jj = 0; jj < MAX_SLOTS; jj++)
    {
      switch(MODULE_TYPE[jj])
      {
      case MODULE_LRS1881M:
        if(datascan & (1<<jj)) {
          fb_fwc_1(jj,0,0x400,1,1,0,1,0,0,0);
        }
        break;
      }
    }
  }
/*
  *dma_dabufp++ = LSWAP(0xfabc0005); 
  *dma_dabufp++ = LSWAP(datascan); 
  *dma_dabufp++ = LSWAP(SCANMASK); 
  *dma_dabufp++ = LSWAP(0xfabb0000);
*/
  /* if event is not in sync */
  if(sync_or_unbuff != 0)
  {
    for(ii = 0; ii < MAX_SLOTS; ii++)
    {
      nhitmod[ii]=0;
    }
    int itimeout = 0;
    do
    {
      if(itimeout++ > 100) break;
      datascan = 0;
vmeBusLock();
      fb_frcm_1(9,0,&datascan,1,0,1,0,0,0);
vmeBusUnlock();
	  /*      printf("0x%x  0x%x\n",datascan, SCANMASK); */
      if(datascan & SCANMASK)
      {
        for(ii = 0; ii < MAX_SLOTS; ii++)
        {
          switch(MODULE_TYPE[ii])
          {
          case MODULE_LRS1881M:
            if(datascan & (1<<ii)){
              nhitmod[ii]++;
vmeBusLock();
              fb_fwc_1(ii,0,0x400,1,1,0,1,0,0,0);
vmeBusUnlock();
            }
            break;
          }
        }
      }
    } while(datascan & SCANMASK);
 
    ii = 0 ;
    while(ii <  MAX_SLOTS)
    { 
      if(nhitmod[ii] != 0)
      {
        datascan = 0xdcfe0000 + (ii << 11) + (nhitmod[ii] & 0x7ff);
        *dma_dabufp++ = LSWAP(datascan); 
      }
      ii++ ;
    }
  }

  /* copy data from slave buffer to the output buffer */
  nw = dma_dabufp - sfibuf;
  /*printf("nw=%d\n",nw);*/

  /*
  BANKOPEN(7, BT_UI4, 0);
  */
  BANKOPEN(0xe120,1,rol->pid);
  for(jj=0; jj<nw; jj++) *rol->dabufp++ = sfibuf[jj];
  BANKCLOSE;

#endif /* USE_FASTBUS */


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

    *rol->dabufp ++ = LSWAP((0x11<<27)+nwords); /*block trailer*/

    BANKCLOSE;

#endif
#endif



#ifndef VXWORKS
TIMERL_STOP(100000/block_level,1000+rol->pid);
#endif





#if 0 /* enable/disable sync events processing */


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

vmeBusLock();
      len = tiUploadAll(chptr, 10000);
vmeBusUnlock();
      /*printf("\nTI len=%d\n",len);
      printf(">%s<\n",chptr);*/
      chptr += len;
      nbytes += len;


#ifdef USE_FADC250_HIDE
      if(nfadc>0)
      {
vmeBusLock();
        len = fadc250UploadAll(chptr, 10000);
vmeBusUnlock();
        /*printf("\nFADC len=%d\n",len);
        printf("%s\n",chptr);*/
        chptr += len;
        nbytes += len;
	  }
#endif
	}




    /* read scaler(s) */
    if(syncFlag==1 || EVENT_NUMBER==1)
    {
	  ;
	}


#ifndef TI_SLAVE
    /* print livetite */
    if(syncFlag==1)
	{
      printf("SYNC: livetime\n");

      int livetime, live_percent;
vmeBusLock();
      tiLatchTimers();
      livetime = tiLive(0);
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

      int nblocks;
      nblocks = tiGetNumberOfBlocksInBuffer();
      /*printf(" Blocks ready for readout: %d\n\n",nblocks);*/

      if(nblocks)
	  {
        printf("SYNC ERROR: TI nblocks = %d\n",nblocks);fflush(stdout);
        sleep(10);
	  }
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
  CDOACK(TIPRIMARY,TIR_SOURCE,0);

  return;
}

static void
__status()
{
  return;
}  




#else

void
fbrol1_dummy()
{
  return;
}

#endif
