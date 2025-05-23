

/* rol1.c - 'standard' first readout list */

#if defined(VXWORKS) || defined(Linux_vme)

#define NEW

#undef SSIPC

static int nusertrig, ndone;

#undef DMA_TO_BIGBUF /*if want to dma directly to the big buffers*/

#define USE_FADC250
#define USE_DSC2
#define USE_V1190
#define USE_SSP
#define USE_SSP_RICH
#define USE_VSCM
#define USE_DCRB
#undef USE_VETROC
#undef USE_FLP
#define USE_VFTDC
#undef USE_SIS3801
#define USE_HD


//#define USE_ED



/* if event rate goes higher then 10kHz, with random triggers we have wrong
slot number reported in GLOBAL HEADER and/or GLOBAL TRAILER words; to work
around that problem temporary patches were applied - until fixed (Sergey) */
#define SLOTWORKAROUND

//#define DEBUG


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
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

#include "daqLib.h"
#include "moLib.h"
#include "v851.h"
#include "sdLib.h"
#include "vscmLib.h"
#include "dcrbLib.h"
#include "sspLib.h"
#include "sspConfig.h"
#include "fadcLib.h"
#include "fadc250Config.h"
#include "vetrocLib.h"
#include "tiLib.h"
#include "tiConfig.h"
#include "dsc2Lib.h"
#include "dsc2Config.h"

#include "circbuf.h"

/* from fputil.h */
#define SYNC_FLAG 0x20000000

/* readout list name */
#define ROL_NAME__ "ROL1"

/* polling mode if needed */
#define POLLING_MODE

/* main TI board */
#define TI_ADDR   (21<<19)  /* if 0 - default will be used, assuming slot 21*/



/* name used by loader */

#ifdef TI_MASTER
#define INIT_NAME rol1_master__init
#define TI_READOUT TI_READOUT_EXT_POLL /* Poll for available data, front panel triggers */
#else
#ifdef TI_SLAVE
#define INIT_NAME rol1_slave__init
#define TI_READOUT TI_READOUT_TS_POLL /* Poll for available data, triggers from master over fiber */
#else
#define INIT_NAME rol1__init
#define TI_READOUT TI_READOUT_EXT_POLL /* Poll for available data, front panel triggers */
#endif
#endif

#include "rol.h"

void usrtrig(unsigned int EVTYPE, unsigned int EVSOURCE);
void usrtrig_done();

#include "TIPRIMARY_source.h"



/* user code */

/*
#include "uthbook.h"
#include "coda.h"
#include "tt.h"
*/
#include "scaler7201.h"
#include "c792Lib.h"
#include "tdc1190.h"
#include "vscmLib.h"


#ifdef USE_SSP
#include "sspLib.h"
static int nssp;   /* Number of SSPs found with sspInit(..) */
static int ssp_not_ready_errors[21];
#endif

static char rcname[5];

#define NBOARDS 22    /* maximum number of VME boards: we have 21 boards, but numbering starts from 1 */
#define MY_MAX_EVENT_LENGTH 3000/*3200*/ /* max words per board */
static unsigned int *tdcbuf;

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
titest2()
{
  printf("roc >%4.4s<, next_block_level=%d, current block level = %d\n",rcname,tiGetNextBlockLevel(),tiGetCurrentBlockLevel());
}



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



/*
#ifdef USE_V1190
*/
static int tdctypebyslot[NBOARDS];
static int error_flag[NBOARDS];
static int ndsc2=0, ndsc2_daq=0;
static int ntdcs;


int
getTdcTypes(int *typebyslot)
{
  int jj;
  for(jj=0; jj<NBOARDS; jj++) typebyslot[jj] = tdctypebyslot[jj];
  return(ntdcs);
}



#ifdef SLOTWORKAROUND
static int slotnums[NBOARDS];
int
getTdcSlotNumbers(int *slotnumbers)
{
  int jj;
  for(jj=0; jj<NBOARDS; jj++) slotnumbers[jj] = slotnums[jj];
  return(ntdcs);
}
#endif

/*
#endif
*/

#ifdef USE_VFTDC
#include "vfTDCLib.h"
static int nvftdc = 0;
static unsigned int MAXVFTDCWORDS = 0;
#endif

#ifdef USE_VETROC
#include "vetrocLib.h"
extern int nvetroc;                   /* Number of VETROCs in Crate */
static int VETROC_SLOT;
static int VETROC_ROFLAG = 1;           /* 0-noDMA, 1-board-by-board DMA, 2-chainedDMA */

/* for the calculation of maximum data words in the block transfer */
static unsigned int MAXVETROCWORDS = 10000;
static unsigned int vetrocSlotMask; /* bit=slot (starting from 0) */
#endif

#ifdef USE_DCRB
#include "dcrbLib.h"
extern int ndcrb;                     /* Number of DCs in Crate */
static int DCRB_SLOT;
static int DCRB_ROFLAG = 1;           /* 0-noDMA, 1-board-by-board DMA, 2-chainedDMA */

/* for the calculation of maximum data words in the block transfer */
static unsigned int MAXDCRBWORDS = 10000;
static unsigned int dcrbSlotMask; /* bit=slot (starting from 0) */
#endif

#ifdef USE_DC
#include "dcLib.h"
extern int ndc;                     /* Number of DCs in Crate */
static int DC_SLOT;
static int DC_ROFLAG = 1;           /* 0-noDMA, 1-board-by-board DMA, 2-chainedDMA */

/* for the calculation of maximum data words in the block transfer */
static unsigned int MAXDCWORDS = 10000;
static unsigned int dcSlotMask; /* bit=slot (starting from 0) */
#endif

#ifdef USE_SSP
static unsigned int sspSlotMask = 0; /* bit=slot (starting from 0) */
static int SSP_SLOT;
#endif

#ifdef USE_FLP
#include "flpLib.h"
static int nflp;
#endif

#ifdef USE_HD
#include "hdLib.h"
static int hd_found = 0;
#define HELICITY_DECODER_ADDR 0x00980000
#define HDMAXWORDS 1024
static uint8_t fiber_input;
static uint8_t cu_input;
static uint8_t cu_output;   
#endif

#ifdef USE_VSCM
static unsigned int vscmSlotMask = 0; /* bit=slot (starting from 0) */
static int nvscm1 = 0;
static int VSCM_ROFLAG = 1;
#endif


static int sd_found = 0;


#ifdef USE_FADC250

#include "fadcLib.h"
extern int fadcBlockError; /* defined in fadcLib.c */

#define DIST_ADDR  0xEA00	  /*  base address of FADC signal distribution board  (A16)  */


unsigned int fadcSlotMask   = 0;    /* bit=slot (starting from 0) */
static int nfadc;                 /* Number of FADC250s verified with the library */
static int NFADC;                   /* The Maximum number of tries the library will
                                     * use before giving up finding FADC250s */
static int FA_SLOT;                 /* We'll use this over and over again to provide
				                     * us access to the current FADC slot number */ 

static int FADC_ROFLAG           = 2;  /* 0-noDMA, 1-board-by-board DMA, 2-chainedDMA */

/* for the calculation of maximum data words in the block transfer */
static unsigned int MAXFADCWORDS = 0;
static unsigned int MAXTIWORDS  = 0;
static unsigned int MAXVSCMWORDS  = 100000;


/* IC lookup tables */

/*tdc's in slots 19 and 20*/
int ic_tdc_high[2][128] = {
0*256
};

int ic_tdc_low[2][128] = {
0*256
};



/*adc's in slots 3-10 and 13-18*/

char *
getFadcPedsFilename(int rocid)
{
  char *dir = NULL;
  /*char *expid = NULL;*/
  static char fname[1024];

  if((dir=getenv("CLAS")) == NULL)
  {
    printf("ERROR: environment variable CLAS is not defined - exit\n");
    return(NULL);
  }
  /*
  if((expid=getenv("EXPID")) == NULL)
  {
    printf("ERROR: environment variable EXPID is not defined - exit\n");
    return(NULL);
  }
  */
  sprintf(fname,"%s/parms/peds/%s/fadc%02d.ped",dir,expid,rocid);

  return(fname);
}

#endif

#ifdef USE_SIS3801
#include "sis3801.h"
static unsigned long run_trig_count = 0;
static int nsis;
unsigned int addr;
#define MASK    0x00000000   /* unmask all 32 channels (0-enable,1-disable) */

/* general settings */
void
sis3801config(int id, int mode)
{
  sis3801control(id, DISABLE_EXT_NEXT);
  sis3801reset(id);
  sis3801clear(id);
  sis3801setinputmode(id,mode);
  sis3801enablenextlogic(id);
  sis3801control(id, ENABLE_EXT_DIS);
}

static int mode = 2;

#endif


static unsigned int maxA32Address;
static unsigned int fadcA32Address = 0x09000000;
static unsigned int vfTDCA32Address = 0x09000000;


static void
__download()
{
  int i1, i2, i3;
  char *ch, tmp[256];
  int ret;
  char *myhost = getenv("HOST");

#ifdef USE_FADC250
  int ii, id, isl, ichan, slot;
  unsigned short iflag;
  int fadc_mode = 1, iFlag = 0;
  int ich, NSA, NSB;
#endif
#ifdef POLLING_MODE
  rol->poll = 1;
#else
  rol->poll = 0;
#endif

  printf("\n>>>>>>>>>>>>>>> ROCID=%d, CLASSID=%d <<<<<<<<<<<<<<<<\n",rol->pid,rol->classid);
  printf("CONFFILE >%s<\n\n",rol->confFile);
  printf("LAST COMPILED: %s %s\n", __DATE__, __TIME__);

  printf("USRSTRING >%s<\n\n",rol->usrString);



  /*TEST*/
  //UDP_user_request(MSGERR, myhost, "MY_ERROR_MESSAGE");
  /*TEST*/



  /* if slave, get fiber port number from user string */
#ifdef TI_SLAVE
  ti_slave_fiber_port = 1; /* default */

  ch = strstr(rol->usrString,"fp=");
  if(ch != NULL)
  {
    strcpy(tmp,ch+strlen("fp="));
    printf("FP >>>>>>>>>>>>>>>>>>>>>%s<<<<<<<<<<<<<<<<<<<<<\n",tmp);
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



  /************/
  /* init daq */

  daqInit();
  DAQ_READ_CONF_FILE;


  /*************************************/
  /* redefine TI settings if neseccary */
  
  tiSetUserSyncResetReceive(1);
  


#ifndef TI_SLAVE
  /* TS 1-6 create physics trigger, no sync event pin, no trigger 2 */
vmeBusLock();
  tiLoadTriggerTable(3);
  tiSetTriggerWindow(7);	// (7+1)*4ns trigger it coincidence time to form trigger type
vmeBusUnlock();
#endif


  /*********************************************************/
  /*********************************************************/



  /* set wide pulse */
vmeBusLock();
/*sergey: WAS tiSetSyncDelayWidth(1,127,1);*/
/*worked for bit pattern latch tiSetSyncDelayWidth(0x54,127,1);*/
vmeBusUnlock();

  usrVmeDmaSetConfig(2,5,1); /*A32,2eSST,267MB/s*/
  /*usrVmeDmaSetConfig(2,5,0);*/ /*A32,2eSST,160MB/s*/
  /*usrVmeDmaSetConfig(2,3,0);*/ /*A32,MBLT*/



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


vmeBusLock();
  ret = sdInit(1);  /* Initialize the SD library; will use 'sd_found' later */
  if(ret >= 0)
  {
    sd_found = 1;
    SD_READ_CONF_FILE;
  }
  else
  {
    printf("\n\nsdInit returns %d, probably SD does not installed in that crate ..\n\n\n",ret);
    sd_found = 0;
  }
vmeBusUnlock();



#ifdef USE_DSC2
  printf("DSC2 Download() starts =========================\n");

#ifndef VXWORKS
  vmeSetQuietFlag(1); /* skip the errors associated with BUS Errors */
#endif
vmeBusLock();
  dsc2Init(0x100000,0x80000,20,0);
vmeBusUnlock();
#ifndef VXWORKS
  vmeSetQuietFlag(0); /* Turn the error statements back on */
#endif

  ndsc2 = dsc2GetNdsc();
  if(ndsc2>0)
  {
    DSC2_READ_CONF_FILE;
    maxA32Address = dsc2GetA32MaxAddress();
    fadcA32Address = maxA32Address + DSC_MAX_A32_MEM;
    vfTDCA32Address = maxA32Address + DSC_MAX_A32_MEM;
    ndsc2_daq = dsc2GetNdsc_daq();
  }
  else
  {
    ndsc2_daq = 0;
  }
  printf("dsc2: %d boards set to be readout by daq\n",ndsc2_daq);
  printf("DSC2 Download() ends =========================\n\n");
#endif



#ifdef USE_FADC250
  printf("FADC250 Download() starts =========================\n");

  /* Here, we assume that the addresses of each board were set according
   to their geographical address (slot number): on FADC250 it must be set by jumpers,
   while some other boards (TI, DSC2 etc) can set it automatically if jumpers are set to 0

   * Slot  2:  (2<<19) = 0x00100000

   * Slot  3:  (3<<19) = 0x00180000
   * Slot  4:  (4<<19) = 0x00200000
   * Slot  5:  (5<<19) = 0x00280000
   * Slot  6:  (6<<19) = 0x00300000
   * Slot  7:  (7<<19) = 0x00380000
   * Slot  8:  (8<<19) = 0x00400000
   * Slot  9:  (9<<19) = 0x00480000
   * Slot 10: (10<<19) = 0x00500000

   * Slot 11: (11<<19) = 0x00580000
   * Slot 12: (12<<19) = 0x00600000

   * Slot 13: (13<<19) = 0x00680000
   * Slot 14: (14<<19) = 0x00700000
   * Slot 15: (15<<19) = 0x00780000
   * Slot 16: (16<<19) = 0x00800000
   * Slot 17: (17<<19) = 0x00880000
   * Slot 18: (18<<19) = 0x00900000
   * Slot 19: (19<<19) = 0x00980000
   * Slot 20: (20<<19) = 0x00A00000

   * Slot 21: (21<<19) = 0x00A80000

   */

  NFADC = 16 + 2; /* 16 slots + 2 (for the switch slots) */

  /* NOTE: starting from 'fadcA32Base' address, memory chunks size=FA_MAX_A32_MEM(=0x800000)
							will be used for every board in A32Blk space:
adc      A32BLK address
1        0x09000000
2        0x09800000
3        0x0A000000
4        0x0A800000
5        0x0B000000
6        0x0B800000
7        0x0C000000
8        0x0C800000
9        0x0D000000
10       0x0D800000
11       0x0E000000
12       0x0E800000
13       0x0F000000
14       0x0F800000
15       0x10000000
16       0x10800000

DSC2: the same as FADCs

CAEN BOARDS IN A32 SPACE MUST BE USING ADDRESSES FROM 0x11000000 AND ABOVE !!!

v1495: 0x11xx0000, where xx follows the same scheme as FADCs
v1190: 0x11xx0000, where xx follows the same scheme as FADCs

*/

  /* Setup the iFlag.. flags for FADC initialization */
  iFlag = 0;
  /* base address */
  iFlag = (DIST_ADDR)<<10;
  /* Sync Source */
  iFlag |= (1<<0);    /* VXS */


  if(sd_found)
  {
    printf("Assume SD usage for FADCs\n");

    /* Trigger Source */
    iFlag |= (1<<2);    /* VXS */
    /* Clock Source */
    /*iFlag |= (1<<5);*/    /* VXS */
    iFlag |= (0<<5);  /* Internal Clock Source */
  }
  else
  {
    printf("Assume SDC usage for FADCs\n");

    /* Trigger Source - have to do it to make faInit() configure for SDC board */
    iFlag |= (1<<1);    /* Front Panel */

    /* Clock Source */
    iFlag |= (1<<4);    /* Front Panel */
    /*iFlag |= (0<<5);*/  /* Internal Clock Source */

    /* SDC address */
    iFlag |= (0xea<<8);
  }



#ifndef VXWORKS
  vmeSetQuietFlag(1); /* skip the errors associated with BUS Errors */
#endif

  faSetA32BaseAddress(fadcA32Address);
vmeBusLock();
  faInit((unsigned int)(3<<19),(1<<19),NFADC,iFlag); /* start from 0x00180000, increment 0x00080000 */
vmeBusUnlock();

  faGetMinA32MB(0);
  faGetMaxA32MB(0);

  nfadc = faGetNfadc(); /* acual number of FADC boards found  */
#ifndef VXWORKS
  vmeSetQuietFlag(0); /* Turn the error statements back on */
#endif

  if(nfadc>0)
  {
    if(nfadc==1) FADC_ROFLAG = 1; /*no chainedDMA if one board only*/
    if(!sd_found) FADC_ROFLAG = 1; /*no chainedDMA if no SD*/

    if(FADC_ROFLAG==2) faEnableMultiBlock(1);
    else faDisableMultiBlock();

    /* configure all modules based on config file */
    FADC_READ_CONF_FILE;

    /* Additional Configuration for each module */
    fadcSlotMask=0;
    for(id=0; id<nfadc; id++) 
    {
      FA_SLOT = faSlot(id);      /* Grab the current module's slot number */
      fadcSlotMask |= (1<<FA_SLOT); /* Add it to the mask */
      printf("=======================> fadcSlotMask=0x%08x",fadcSlotMask);

      /*
      if(rol->pid==7)
      {
        fadcSlotMask = 0x0017e7f8;
        printf("=======================> new fadcSlotMask=0x%08x",fadcSlotMask);
      }
*/

	  {
        unsigned int PL, PTW, NSB, NSA, NP;
vmeBusLock();
        faGetProcMode(FA_SLOT, &fadc_mode, &PL, &PTW, &NSB, &NSA, &NP);
vmeBusUnlock();
        printf(", slot %d, fadc_mode=%d\n",FA_SLOT,fadc_mode);
	  }

      /* Bus errors to terminate block transfers (preferred) */
vmeBusLock();
      faEnableBusError(FA_SLOT);
vmeBusUnlock();

#ifdef NEW
      /*****************/
      /*trigger-related*/
vmeBusLock();
      faResetMGT(FA_SLOT,1);
      faSetTrigOut(FA_SLOT, 7);
vmeBusUnlock();
#endif

	  /*****************/
	  /*****************/
    }


    /* 1) Load FADC pedestals from file for trigger path.
       2) Offset FADC threshold for each channel based on pedestal for both readout and trigger */
	if(rol->pid>36 && rol->pid!=46 && rol->pid!=37 && rol->pid!=39 && rol->pid!=58)
    {
vmeBusLock();
      faGLoadChannelPedestals(getFadcPedsFilename(rol->pid), 1);
vmeBusUnlock();
    }
    /* read back and print trigger pedestals */
/*
    printf("\n\nTrigger pedestals readback\n");
    for(id=0; id<nfadc; id++) 
    {
      FA_SLOT = faSlot(id);
      for(ichan=0; ichan<16; ichan++)
      {
        printf("  slot=%2d chan=%2d ped=%5d\n",FA_SLOT,ichan,faGetChannelPedestal(FA_SLOT, ichan));
      }
    }
    printf("\n\n");
*/
  }


/*
STATUS for FADC in slot 18 at VME (Local) base address 0x900000 (0xa16b1000)
---------------------------------------------------------------------- 
 Board Firmware Rev/ID = 0x020e : ADC Processing Rev = 0x0907
 Alternate VME Addressing: Multiblock Enabled
   A32 Enabled at VME (Local) base 0x0f800000 (0xa95b1000)
   Multiblock VME Address Range 0x10800000 - 0x11000000

 Signal Sources: 
   Ref Clock : Internal
   Trig Src  : VXS (Async)
   Sync Reset: VXS (Async)

 Configuration: 
   Internal Clock ON
   Bus Error ENABLED
   MultiBlock transfer ENABLED (Last Board  - token via VXS)

 ADC Processing Configuration: 
   Channel Disable Mask = 0x0000
   Mode = 1  (ENABLED)
   Lookback (PL)    = 1360 ns   Time Window (PTW) = 400 ns
   Time Before Peak = 12 ns   Time After Peak   = 24 ns
   Max Peak Count   = 1 
   Playback Mode    = 0 

  CSR       Register = 0x00001800
  Control 1 Register = 0x10b00338 
  Control 2 Register = 0x00000000 - Disabled
  Internal Triggers (Live) = 0
  Trigger   Scaler         = 0
  Events in FIFO           = 0  (Block level = 1)
  MGT Status Register      = 0x00000400 
  BERR count (from module) = 0
*/

  /***************************************
   *   SD SETUP
   ***************************************/
vmeBusLock();
  /*sd_found = sdInit(1); moved before anything else*/   /* Initialize the SD library */
  if(sd_found)
  {
    sdSetActiveVmeSlots(fadcSlotMask); /* Use the fadcSlotMask to configure the SD */
    sdStatus();
    /*sdSetTrigoutLogic(0, 2);*/ /* Enable SD trigout as OR by default */
  }
vmeBusUnlock();

  printf("FADC250 Download() ends =========================\n\n");
#endif








#ifdef USE_V1190
  printf("V1190 Download() starts =========================\n");

vmeBusLock();
  ntdcs = tdc1190Init(0x11100000,0x80000,20,0);
  if(ntdcs>0) TDC_READ_CONF_FILE;
vmeBusUnlock();

  for(ii=0; ii<ntdcs; ii++)
  {
    slot = tdc1190Slot(ii);
    tdctypebyslot[slot] = tdc1190Type(ii);
    printf(">>> id=%d slot=%d type=%d\n",ii,slot,tdctypebyslot[slot]);
  }


#ifdef SLOTWORKAROUND
  for(ii=0; ii<ntdcs; ii++)
  {
vmeBusLock();
    slot = tdc1190GetGeoAddress(ii);
vmeBusUnlock();
    slotnums[ii] = slot;
    printf("[%d] slot %d\n",ii,slotnums[ii]);
  }
#endif


  /* if TDCs are present, set busy from P2 */
  if(ntdcs>0)
  {
    printf("Set BUSY from P2 for TDCs\n");
vmeBusLock();
    tiSetBusySource(TI_BUSY_P2,0);
vmeBusUnlock();
  }

  for(ii=0; ii<ntdcs; ii++)
  {
vmeBusLock();
    tdc1190Clear(ii);
vmeBusUnlock();
    error_flag[ii] = 0;
  }

  printf("V1190 Download() ends =========================\n\n");
#endif


#ifdef USE_VSCM
  printf("VSCM Download() starts =========================\n");
#ifndef VXWORKS
  vmeSetQuietFlag(1); /* skip the errors associated with BUS Errors */
#endif
vmeBusLock();
  nvscm1 = vscmInit((unsigned int)(3<<19),(1<<19),20,0);
vmeBusUnlock();
#ifndef VXWORKS
  vmeSetQuietFlag(0); /* Turn the error statements back on */
#endif

  if(VSCM_ROFLAG==2 && nvscm1==1) VSCM_ROFLAG = 1; /*no chainedDMA if one board only*/
  /*
  if(VSCM_ROFLAG==2) faEnableMultiBlock(1);
  else faDisableMultiBlock();
  */

  vscmSlotMask=0;
  for(ii=0; ii<nvscm1; ii++)
  {
    slot = vscmSlot(ii);      /* Grab the current module's slot number */
    vscmSlotMask |= (1<<slot); /* Add it to the mask */
  }

  printf("VSCM Download() ends =========================\n\n");
#endif




#ifdef USE_DC
  printf("DC Download() starts =========================\n");

#ifndef VXWORKS
  vmeSetQuietFlag(1); /* skip the errors associated with BUS Errors */
#endif

vmeBusLock();
  ndc = dcInit((3<<19), 0x80000, 16+2, 7); /* 7 boards from slot 4, 7 boards from slot 13 */
  if(ndc>0)
  {
    dcGSetDAC(/*20*/10); /* threshold in mV */
    dcGSetCalMask(0,0x3f);
    dcGSetProcMode(4000/*2000*/,4000/*2000*/,32);
  }
vmeBusUnlock();

#ifndef VXWORKS
  vmeSetQuietFlag(0); /* Turn the error statements back on */
#endif

  if(ndc==1) DC_ROFLAG = 1; /*no chainedDMA if one board only*/
  if((ndc>0) && (DC_ROFLAG==2)) dcEnableMultiBlock(1);
  else if(ndc>0) dcDisableMultiBlock();

  /* Additional Configuration for each module */
  dcSlotMask=0;
  for(ii=0; ii<ndc; ii++) 
  {
    DC_SLOT = dcSlot(ii);      /* Grab the current module's slot number */
    dcSlotMask |= (1<<DC_SLOT); /* Add it to the mask */
	printf("=======================> dcSlotMask=0x%08x\n",dcSlotMask);

  }


  /* DC stuff */

  for(id=0; id<ndc; id++) 
  {
    DC_SLOT = dcSlot(id);
vmeBusLock();
    dcTriggerPulseWidth(DC_SLOT, 8000);
	dcLinkReset(DC_SLOT);
vmeBusUnlock();

    /* will try to reset 5 times */
    for(ii=0; ii<5; ii++)
	{
      if(dcLinkStatus(DC_SLOT)) break;
	  printf("Reseting link at slot %d\n",DC_SLOT);
vmeBusLock();
      dcLinkReset(DC_SLOT);
vmeBusUnlock();
	}
  }


  for(id=0; id<ndc; id++)
  {
    DC_SLOT = dcSlot(id);
    if(dcLinkStatus(DC_SLOT)) printf("Link at slot %d is UP\n",DC_SLOT);
    else printf("Link at slot %d is DOWN\n",DC_SLOT);
  }
  printf("DC Download() ends =========================\n\n");
#endif



#ifdef USE_FLP
  printf("FLP Download() starts =========================\n");
vmeBusLock();
  flpInit(0x00900000, 0); /* FLP in slot 18 */
  nflp = flpGetNflp();

  if(nflp>0)
  {
    FLP_READ_CONF_FILE;

  flpEnableOutput(0);
  flpEnableOutput(1);
  flpEnableIntPulser(0);
  flpEnableIntPulser(1);
  flpStatus(0);

  }
vmeBusUnlock();



/*
  flpEnableOutput(0);
  flpEnableOutput(1);
  flpEnableIntPulser(0);
  flpEnableIntPulser(1);
  flpStatus(0);

  flpSetOutputVoltages(0, 3.7, 3.7, 4.7);
  flpSetOutputVoltages(1, 3.7, 3.7, 4.7);

  flpGetOutputVoltages(1, &v1, &v2, &v3);
  printf ("output voltage from 1 %3.2f, %3.2f, %3.2f\n", v1, v2, v3);

  flpGetOutputVoltages(0, &v1, &v2, &v3);
  printf ("output voltage %from 0 3.2f, %3.2f, %3.2f\n", v1, v2, v3);
  flpSetPulserPeriod(0, 200000);

  flpSetPulserPeriod(1, 200000);
  flpStatus(0);
*/

#endif


#ifdef USE_SIS3801
  printf("SIS3801 Download() starts =========================\n");

vmeBusLock();

  mode = 2; /* Control Inputs mode = 2  */
  //nsis = sis3801Init(0x11800000, 0x100000, 2, mode);
  nsis = sis3801Init(0x800000, 0x100000, 2, mode);
  /*nsis = sis3801Init(0x10000000, 0x1000000, 2, mode);*/
  printf("nsis=%d\n",nsis);
  /*if(nsis>0) TDC_READ_CONF_FILE;*/

  for(id = 0; id < nsis; id++)
  {
    sis3801config(id, mode);
    sis3801control(id, DISABLE_EXT_NEXT);

    printf("    Status = 0x%08x\n",sis3801status(id));
  }

#if 0
  /* Set up the 0th scaler as the interrupt source */
  /* 2nd arg: vector = 0 := use default */
  scalIntInit(0, 0);

  /* Connect service routine */
  scalIntConnect(myISR, 0);
#endif

vmeBusUnlock();

  printf("SIS3801 Download() ends =========================\n\n");
#endif


#ifdef USE_HD

  /* Initialize the library and module with its internal clock*/
  ret = hdInit(HELICITY_DECODER_ADDR, HD_INIT_VXS, HD_INIT_EXTERNAL_FIBER, 0);
  if(ret==1) hd_found = 1;
  else       hd_found = 0;

  if(hd_found)
  {
    hdSetA32(0x10000000);
    hdStatus(1);
  }

#endif


  sprintf(rcname,"RC%02d",rol->pid);
  printf("rcname >%4.4s<\n",rcname);

#ifdef SSIPC
  sprintf(ssname,"%s_%s",getenv("HOST"),rcname);
  printf("Smartsockets unique name >%s<\n",ssname);
  epics_msg_sender_init(expid, ssname); /* SECOND ARG MUST BE UNIQUE !!! */
#endif

  logMsg("INFO: User Download Executed\n",1,2,3,4,5,6);
}





static void
__prestart()
{
  int ii, i1, i2, i3;
  int ret;
#ifdef USE_FADC250
  int id, isl, ichan, slot;
  unsigned short iflag;
  int iFlag = 0;
  int ich;
  unsigned short aa = 0;
  unsigned short bb;
  unsigned short thr = 400;
#endif
#ifdef USE_HD
  uint8_t hd_clock, hd_clock_ret;
#endif

  /* Clear some global variables etc for a clean start */
  *(rol->nevents) = 0;
  event_number = 0;

  /*TEST*/
  //UDP_cancel_errors();
  /*TEST*/

  tiEnableVXSSignals();

#ifdef POLLING_MODE
  CTRIGRSS(TIPRIMARY, TIR_SOURCE, usrtrig, usrtrig_done);
#else
  CTRIGRSA(TIPRIMARY, TIR_SOURCE, usrtrig, usrtrig_done);
#endif

  printf(">>>>>>>>>> next_block_level = %d, block_level = %d, use %d\n",next_block_level,block_level,next_block_level);
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


#ifdef USE_FADC250
  /* if FADCs are present, set busy from SD board */
  if(nfadc>0)
  {
    printf("Set BUSY from SWB for FADCs\n");
vmeBusLock();
    if(sd_found) tiSetBusySource(TI_BUSY_SWB,0);
    else tiSetBusySource(TI_BUSY_FP_FADC,0);
vmeBusUnlock();
  }
#endif



#ifdef USE_VSCM
  printf("VSCM Prestart() start =========================\n");

  /* if VSCMs are present, set busy from SD board */
  if(nvscm1>0)
  {
    printf("Set BUSY from SWB for FADCs\n");

vmeBusLock();
    tiSetBusySource(TI_BUSY_SWB,0);
	sdSetActiveVmeSlots(vscmSlotMask);

    /*sdSetTrigoutLogic(0, 2);*/

vmeBusUnlock();

    VSCM_READ_CONF_FILE;
/*
    printf("vscmPrestart ...\n"); fflush(stdout);
//   vscmPrestart("VSCMConfig_ben_cosmic.txt");
    vscmPrestart("VSCMConfig.txt");
    printf("vscmPrestart done\n"); fflush(stdout);
*/
  }


  printf("VSCM Prestart() ends =========================\n\n");
#endif


#ifdef USE_VFTDC
  printf("VFTDC Prestart() starts =========================\n");

  /*
  printf("\nUse vfTDCA32Address=0x%08x\n",vfTDCA32Address);
  vfTDCSetA32BaseAddress(vfTDCA32Address);
  */

vmeBusLock();
  nvftdc = vfTDCInit(3<<19, 1<<19, 20,
  //nvftdc = vfTDCInit(20<<19, 1<<19, 1,
                     VFTDC_INIT_VXS_SYNCRESET |
                     VFTDC_INIT_VXS_TRIG      |
                     VFTDC_INIT_VXS_CLKSRC/*VFTDC_INIT_INT_CLKSRC*/);
vmeBusUnlock();



  printf("nvftdc=%d\n",nvftdc);
  if(nvftdc>0)
  {
    int window_width   = 255/*255*/; /* 200*4ns = 800ns, maximum 255 */
    int window_latency = 2027 /*2047*/; /* 2175*4ns = 8700ns, maximum 2047 */ /* changed from 2027 to 2011 Rafo*/

	for(ii=0; ii<nvftdc; ii++)
	{
      slot = vfTDCSlot(ii);
vmeBusLock();
      vfTDCSetWindowParameters(slot, window_latency, window_width);
      vfTDCStatus(slot,1);
vmeBusUnlock();
	}
  }


  printf("VFTDC Prestart() ends =========================\n\n");
#endif


#ifdef USE_VETROC
  printf("VETROC Prestart() starts =========================\n");

#ifndef VXWORKS
  vmeSetQuietFlag(1); /* skip the errors associated with BUS Errors */
#endif

vmeBusLock();
  nvetroc = vetrocInit((3<<19), 0x80000, 16+2, 0x111); /* 7 boards from slot 4, 7 boards from slot 13 */
  if(nvetroc>0)
  {
    vetrocGSetProcMode(/*4000*/2000,/*4000*/2000);
  }
vmeBusUnlock();

#ifndef VXWORKS
  vmeSetQuietFlag(0); /* Turn the error statements back on */
#endif

  if(nvetroc==1) VETROC_ROFLAG = 1; /*no chainedDMA if one board only*/
  if((nvetroc>0) && (VETROC_ROFLAG==2)) vetrocEnableMultiBlock(1);
  else if(nvetroc>0) vetrocDisableMultiBlock();

  /* Additional Configuration for each module */
  vetrocSlotMask=0;
  for(ii=0; ii<nvetroc; ii++) 
  {
    VETROC_SLOT = vetrocSlot(ii);      /* Grab the current module's slot number */
    vetrocSlotMask |= (1<<VETROC_SLOT); /* Add it to the mask */
	printf("=======================> vetrocSlotMask=0x%08x\n",vetrocSlotMask);

  }

  /* VETROC stuff */
  for(id=0; id<nvetroc; id++) 
  {
    VETROC_SLOT = vetrocSlot(id);
vmeBusLock();
    vetrocTriggerPulseWidth(VETROC_SLOT, 8000);
    vetrocLinkReset(VETROC_SLOT);
vmeBusUnlock();
  }

  printf("VETROC Prestart() ends =========================\n\n");
#endif




#ifdef USE_DCRB
  printf("DCRB Prestart() starts =========================\n");

#ifndef VXWORKS
  vmeSetQuietFlag(1); /* skip the errors associated with BUS Errors */
#endif

vmeBusLock();
  ndcrb = dcrbInit((3<<19), 0x80000, 20, 7); /* 7 boards from slot 3, 7 boards from slot 14 */
  if(ndcrb>0)
  {
    DCRB_READ_CONF_FILE;

    /*dcrbGSetDAC(-30); MUST COME FROM CONFIG FILE */ /* threshold in mV */

    /* last param is double-hit resolution in ns */
    /*dcrbGSetProcMode(2000,2000,1000); MUST COME FROM CONFIG FILE */
  }
vmeBusUnlock();

#ifndef VXWORKS
  vmeSetQuietFlag(0); /* Turn the error statements back on */
#endif

  if(ndcrb==1) DCRB_ROFLAG = 1; /*no chainedDMA if one board only*/
  if((ndcrb>0) && (DCRB_ROFLAG==2)) dcrbEnableMultiBlock(1);
  else if(ndcrb>0) dcrbDisableMultiBlock();

  /* Additional Configuration for each module */
  dcrbSlotMask=0;
  for(ii=0; ii<ndcrb; ii++) 
  {
    DCRB_SLOT = dcrbSlot(ii);      /* Grab the current module's slot number */
    dcrbSlotMask |= (1<<DCRB_SLOT); /* Add it to the mask */
	printf("=======================> dcrbSlotMask=0x%08x\n",dcrbSlotMask);

  }


  /* DCRB stuff */

  for(id=0; id<ndcrb; id++) 
  {
    DCRB_SLOT = dcrbSlot(id);
vmeBusLock();
//    dcrbTriggerPulseWidth(DCRB_SLOT, 8000);
	dcrbLinkReset(DCRB_SLOT);
vmeBusUnlock();

    /* will try to reset 5 times
    for(ii=0; ii<5; ii++)
	{
      if(dcrbLinkStatus(DCRB_SLOT)) break;
	  printf("Reseting link at slot %d\n",DCRB_SLOT);
vmeBusLock();
      dcrbLinkReset(DCRB_SLOT);
vmeBusUnlock();
	}
    */

  }


  for(id=0; id<ndcrb; id++)
  {
    DCRB_SLOT = dcrbSlot(id);
    if(dcrbLinkStatus(DCRB_SLOT)) printf("Link at slot %d is UP\n",DCRB_SLOT);
    else printf("Link at slot %d is DOWN\n",DCRB_SLOT);
  }


  /* SD Trigout when DCRB multiplicity >= 1
  if(sd_found)
  {
    sdSetTrigoutLogic(0, 1);
  }
*/

  printf("DCRB Prestart() ends =========================\n\n");
#endif




  /*****************************************************************/
  /*****************************************************************/

#ifdef USE_FADC250
  printf("FADC250 Prestart() starts =========================\n");

  /* Program/Init VME Modules Here */
  for(id=0; id<nfadc; id++)
  {
    FA_SLOT = faSlot(id);
vmeBusLock();
    if(sd_found) faSetClockSource(FA_SLOT,2);
    else         faSetClockSource(FA_SLOT,/*0*/1); /* 0-internal, 1-front panel*/
vmeBusUnlock();
  }

  sleep(1);

  for(id=0; id<nfadc; id++)
  {
    FA_SLOT = faSlot(id);
vmeBusLock();
    faSoftReset(FA_SLOT,0); /*0-soft reset, 1-soft clear*/
vmeBusUnlock();

#ifdef NEW
    if(!faGetMGTChannelStatus(FA_SLOT))
    {
vmeBusLock();
      faResetMGT(FA_SLOT,1);
      faResetMGT(FA_SLOT,0);
vmeBusUnlock();
    }
#endif

vmeBusLock();
    faResetToken(FA_SLOT);
    faResetTriggerCount(FA_SLOT);
    faStatus(FA_SLOT,0);
    faPrintThreshold(FA_SLOT);
vmeBusUnlock();
  }

  /*  Enable FADC */
  for(id=0; id<nfadc; id++) 
  {
    FA_SLOT = faSlot(id);
    /*faSetMGTTestMode(FA_SLOT,0);*/
    /*faChanDisable(FA_SLOT,0xffff);enabled in download*/
vmeBusLock();
    faEnable(FA_SLOT,0,0);
    /*faSetCompression(FA_SLOT,0);*/
vmeBusUnlock();
  }

  faGStatus(0);

  printf("FADC250 Prestart() ends =========================\n\n");
  sleep(2);
#endif



#ifdef USE_SSP
  printf("SSP Prestart() starts =========================\n");


//////////////////////////////////////
//////// RICH LV cycle test //////////
#if 0
  if(rol->pid==84)
  {
    printf("*** Power cycling RICH2 LV ***\n");
    system("caput B_DET_RICH2_LV:OFF 1");
    usleep(1000000);
    system("caput B_DET_RICH2_LV:ON 1");
    usleep(3000000);
    printf("*** Done ***\n");
  }
#endif
//////////////////////////////////////


  memset(ssp_not_ready_errors, 0, sizeof(ssp_not_ready_errors));

 /*****************
  *   SSP SETUP - must do sspInit() after master TI clock is stable, so do it in Prestart
  *****************/
  iFlag  = SSP_INIT_MODE_DISABLED; /* Disabled, initially */
  iFlag |= SSP_INIT_SKIP_FIRMWARE_CHECK;
  iFlag |= SSP_INIT_MODE_VXS;
  iFlag |= SSP_INIT_REBOOT_FPGA;
//  iFlag |= SSP_INIT_FIBER0_ENABLE;         /* Enable hps1gtp fiber ports */
//  iFlag |= SSP_INIT_FIBER1_ENABLE;         /* Enable hps1gtp fiber ports */
//  iFlag |= SSP_INIT_GTP_FIBER_ENABLE_MASK; /* Enable all fiber port data to GTP */
  /*iFlag|= SSP_INIT_NO_INIT;*/ /* does not configure SSPs, just set pointers */
  nssp=0;
vmeBusLock();
  nssp = sspInit(0, 0, 0, iFlag); /* Scan for, and initialize all SSPs in crate */
vmeBusUnlock();
  printf("rol1: found %d SSPs (using iFlag=0x%08x)\n",nssp,iFlag);

  if(nssp>0)
  {
    int rich_firmware = 0;
    int firmware_type;

    SSP_READ_CONF_FILE;
    sspSlotMask=0;
    for(id=0; id<nssp; id++)
    {
      SSP_SLOT = sspSlot(id);      /* Grab the current module's slot number */

      firmware_type = sspGetFirmwareType_Shadow(SSP_SLOT);
      if(firmware_type == SSP_CFG_SSPTYPE_HALLBRICH)
      {
        rich_firmware = 1;
      }

      sspSlotMask |= (1<<SSP_SLOT); /* Add it to the mask */
      printf("=======================> sspSlotMask=0x%08x\n",sspSlotMask);

      printf("Setting SSP %d, slot %d\n",id,sspSlot(id));
    }

    if(rich_firmware==1)
    {
      printf("\nWE ARE IN RICH CRATE\n\n");
vmeBusLock();
      tiSetBusySource(TI_BUSY_SWB,0);
      sdSetActiveVmeSlots(sspSlotMask);
vmeBusUnlock();
    }
    else
    {
      printf("\nWE ARE NOT IN RICH CRATE\n\n");
    }
  }

  printf("SSP Prestart() ends =========================\n\n");
#endif /* USE_SSP */

  /* USER code here */
  /******************/




vmeBusLock();
  tiIntDisable();
vmeBusUnlock();

#ifdef USE_DSC2
  printf("DSC2 Prestart() starts =========================\n");
  /* dsc2 configuration */
  if(ndsc2>0) DSC2_READ_CONF_FILE;
  printf("DSC2 Prestart() ends =========================\n\n");
#endif




  /* master and standalone crates, NOT slave */
#ifndef TI_SLAVE

vmeBusLock();
printf("CLOCK251?\n");
vfTDCGetClockSource(19);
vfTDCGetClockSource(20);
printf("CLOCK251!\n");
vmeBusUnlock();

  sleep(1);
vmeBusLock();
  tiSyncReset(1);
vmeBusUnlock();
  sleep(1);

vmeBusLock();
printf("CLOCK252?\n");
vfTDCGetClockSource(19);
vfTDCGetClockSource(20);
printf("CLOCK252!\n");
vmeBusUnlock();


vmeBusLock();
  tiSyncReset(1);
vmeBusUnlock();
  sleep(1);



  /* USER RESET - use it because 'SYNC RESET' produces too short pulse, still need 'SYNC RESET' above because 'USER RESET'
  does not do everything 'SYNC RESET' does (in paticular does not reset event number) */
vmeBusLock();
  tiUserSyncReset(1,1);
  tiUserSyncReset(0,1);
vmeBusUnlock();




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



#if 0
  if(!sd_found)
  {
    /* added by Ben Raydo - please fix with correct SDC sync pulse...*/
    faEnableSoftSync(0);
    faSync(0);
  }
#endif





/* set block level in all boards where it is needed;
   it will overwrite any previous block level settings */




#if 0
#ifdef TI_SLAVE /* assume that for master and standalone TIs block level is set from config file */
vmeBusLock();
  tiSetBlockLevel(block_level);
vmeBusUnlock();
#endif
#endif
 printf("tiCurrentBlockLevel = %d, block_level = %d\n",tiGetCurrentBlockLevel(),block_level);





#ifdef USE_V1190
  for(ii=0; ii<ntdcs; ii++)
  {
vmeBusLock();
    tdc1190SetBLTEventNumber(ii, block_level);
vmeBusUnlock();
  }
#endif
#ifdef USE_FADC250
    /* Calculate the maximum number of words per block transfer (assuming Pulse mode)
     *   MAX = NFADC * block_level * (EvHeader + TrigTime*2 + Pulse*2*chan) 
     *         + 2*32 (words for byte alignment) 
     */
    MAXFADCWORDS = NFADC * block_level * (1+2+100/*FADC_WINDOW_WIDTH*/*16) + 2*32;
  
    printf("**************************************************\n");
    printf("* Calculated MAXFADCWORDS per block = %d\n",MAXFADCWORDS);
    printf("**************************************************\n");

    /* Check these numbers, compared to our buffer size.. */
    if( (MAXFADCWORDS+MAXTIWORDS)*4 > MAX_EVENT_LENGTH )
    {
      printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      printf(" WARNING.  Event buffer size (%d bytes) is smaller than the expected data size (%d bytes)\n",
        MAX_EVENT_LENGTH,(MAXFADCWORDS+MAXTIWORDS)*4);
      printf("     Increase the size of MAX_EVENT_LENGTH and recompile!\n");
      printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    }

    for(id=0; id<nfadc; id++) 
    {
      slot = faSlot(id);
vmeBusLock();
      faSetBlockLevel(slot, block_level);
vmeBusUnlock();
    }



//#ifdef USE_ED
    /* sergey: set fadc250 internal busy parameters */
	printf("\n\n== Setting fadc250 internal busy parameters\n\n");
    for(id=0; id<nfadc; id++) 
    {
      slot = faSlot(id);
vmeBusLock();

      /*the maximum number of unacknowledged triggers before
		module stops accepting incoming triggers*/
      faSetTriggerStopCondition(slot, 9); /* halld - 9 */ /*2000/nsamples-3 ???*/

	  /*the maximum number of unacknowledged triggers before module asserts BUSY*/
      faSetTriggerBusyCondition(slot, 3); /* halld - 3 */

      faStatus(slot,0);
vmeBusUnlock();
    }
	printf("\n\n== Done setting fadc250 internal busy parameters\n\n");
//#endif



#endif


#ifdef USE_VSCM
  for(ii=0; ii<nvscm1; ii++)
  {
    slot = vscmSlot(ii);
vmeBusLock();
    vscmSetBlockLevel(slot, block_level);
vmeBusUnlock();
  }
#endif
#ifdef USE_SSP
  for(id=0; id<nssp; id++)
  {
    slot = sspSlot(id);
vmeBusLock();
    sspSetBlockLevel(slot, block_level);
    sspGetBlockLevel(slot);
vmeBusUnlock();
  }
#endif


#ifdef USE_VFTDC
  if(nvftdc>0)
  {
    /* change block level is all modules */
	for(ii=0; ii<nvftdc; ii++)
	{
      slot = vfTDCSlot(ii);
vmeBusLock();
      vfTDCSetBlockLevel(slot, block_level);
vmeBusUnlock();
	}
    MAXVFTDCWORDS = nvftdc * block_level * 128*16 + 2*32; /*MUST DO IT RIGHT !!!*/
  }
#endif



#ifdef USE_VETROC
  for(id=0; id<nvetroc; id++)
  {
    slot = vetrocSlot(id);
vmeBusLock();
    vetrocSetBlockLevel(slot, block_level);
/*    vetrocGetBlockLevel(slot);*/
vmeBusUnlock();
  }
#endif

#ifdef USE_DC
  for(id=0; id<ndc; id++)
  {
    slot = dcSlot(id);
vmeBusLock();
    dcSetBlockLevel(slot, block_level);
/*    dcGetBlockLevel(slot);*/
vmeBusUnlock();
  }
#endif

#ifdef USE_DCRB
  for(id=0; id<ndcrb; id++)
  {
    slot = dcrbSlot(id);
vmeBusLock();
    dcrbSetBlockLevel(slot, block_level);
/*    dcrbGetBlockLevel(slot);*/
vmeBusUnlock();
  }
#endif


#ifdef USE_SIS3801

vmeBusLock();
  for(id = 0; id < nsis; id++)
  {
    /*sis3801clear(id);*/
    sis3801config(id, mode);
    sis3801control(id, DISABLE_EXT_NEXT);
  }
vmeBusUnlock();

#endif



#ifdef USE_HD

  if(hd_found)
  {
    /* Setting data input (0x100 = 2048 ns) and
       trigger latency (1000*8ns = 8000 ns) processing delays */
    hdSetProcDelay(0x100, 1000);

    /* Enable the module decoder, well before triggers are enabled */
    hdEnableDecoder();

    /*set i/o signals inversion if needed; 3 parameters have following meaning: fiber_input, cu_input, cu_output
     (0 means no inversion, 1 means inversion)*/

    //ret = hdSetHelicityInversion(0, 0, 0); /*reproduces old (v7) firmware (before Jan 31, 2024)*/

    /* signals on fiber inputs are reversed because of polarity-flipping
       optical fanout in counting room, so we do (1,0,1); if optical fanout
       replaced with non-flipping one, will change it to (0,0,1)*/
    ret = hdSetHelicityInversion(1, 0, 1);

    ret = hdGetHelicityInversion(&fiber_input, &cu_input, &cu_output);
    printf("\nHelicity Decoder inversion settings: fiber_input=%d, cu_input=%d, cu_output=%d (0-no flip, 1-flip)\n\n",
           fiber_input,cu_input,cu_output);

    /* set tsettle filtering:
     *     0   Disabled
     *     1   4 clock cycles
     *     2   8 clock cycles
     *     3   16 clock cycles
     *     4   24 clock cycles
     *     5   32 clock cycles
     *     6   64 clock cycles
     *     7   128 clock cycles */
    hd_clock = 3;
    ret = hdSetTSettleFilter(hd_clock); 
    ret = hdGetTSettleFilter(&hd_clock_ret);
    printf("\nHD T-settle filtering: set %d, read back %d\n\n",hd_clock,hd_clock_ret);

    hdStatus(0);
  }

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
__end()
{
  int iwait=0;
  int blocksLeft=0;
  int id, slot;

  printf("\n\nINFO: End1 Reached\n");fflush(stdout);

#ifdef USE_SIS3801

  for(id = 0; id < nsis; id++)
  {
vmeBusLock();
    sis3801control(id, DISABLE_EXT_NEXT);
vmeBusUnlock();
    printf("    Status = 0x%08x\n",sis3801status(id));
  }
#if 0
  scalIntDisable();
#endif

#endif


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


#ifdef USE_FADC250
  /* FADC Disable */
  for(id=0; id<nfadc; id++) 
  {
    FA_SLOT = faSlot(id);
vmeBusLock();
    faDisable(FA_SLOT,0);
    faStatus(FA_SLOT,0);
vmeBusUnlock();
  }

vmeBusLock();
  sdStatus();
vmeBusUnlock();
#endif


#ifdef USE_VFTDC
  for(id=0; id<nvftdc; id++)
  {
    slot = vfTDCSlot(id);
vmeBusLock();
    vfTDCStatus(slot,1);
vmeBusUnlock();
  }
#endif


#ifdef USE_HD

  if(hd_found)
  {
    hdDisable();
    hdStatus(0);
  }

#endif


vmeBusLock();
  tiStatus(1);
vmeBusUnlock();

  printf("INFO: End1 Executed\n\n\n");fflush(stdout);

  return;
}


static void
__pause()
{
  int id;

#ifdef USE_SIS3801

  for(id=0; id<nsis; id++)
  {
vmeBusLock();
    sis3801clear(id);
    sis3801control(id, DISABLE_EXT_NEXT);
vmeBusUnlock();
  }

#endif

  CDODISABLE(TIPRIMARY,TIR_SOURCE,0);
  logMsg("INFO: Pause Executed\n",1,2,3,4,5,6);
  
} /*end pause */


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


#ifdef USE_FADC250

#ifdef USE_ED
  for(id=0; id<nfadc; id++) 
  {
    FA_SLOT = faSlot(id);
    faArmStatesStorage(FA_SLOT);
  }
#endif

  /*  Enable FADC - old place
  for(id=0; id<nfadc; id++) 
  {
    FA_SLOT = faSlot(id);
    faChanDisable(FA_SLOT,0x0);
  }
  sleep(1);
  */

  if(!sd_found)
  {
    int portMask = 0;

    for(id=0; id<nfadc; id++) portMask |= (1<<id);
    printf("Configuring SDC card with portMask=0x%08x for %d FADC boards\n",portMask,nfadc);

	faSDC_Config(1, portMask);
  }

  /*  Send Sync Reset to FADC */
  /*if(!sd_found) faSDC_Sync();*/

#endif

#ifdef USE_V1190
  for(jj=0; jj<ntdcs; jj++)
  {
vmeBusLock();
    tdc1190Clear(jj);
vmeBusUnlock();
    error_flag[jj] = 0;
  }
  taskDelay(100);

#endif



#ifdef USE_VSCM
  for(ii=0; ii<nvscm1; ii++)
  {
    for(jj=0; jj<8; jj++)
    {
vmeBusLock();
      fssrSCR(vscmSlot(ii), jj);
vmeBusUnlock();
	}
  }

  /* Reset the Token */
  if(VSCM_ROFLAG==2)
  {
	for(ii=0; ii<nvscm1; ii++)
    {
	  slot = vscmSlot(ii);
vmeBusLock();
      vscmResetToken(slot);
vmeBusUnlock();
	}
  }

	for(ii=0; ii<nvscm1; ii++)
    {
	  slot = vscmSlot(ii);
vmeBusLock();
	  vscmStat(slot);
vmeBusUnlock();
	}


  /*
  printf("\n\nFSSR Status:\n\n");
  for(ii=0; ii<nvscm1; ii++)
  {
    for(jj=0; jj<8; jj++)
    {
      fssrStatus(vscmSlot(ii), jj);
	}
  }
  printf("\n\n\n\n");
  */

#endif

#ifdef USE_DC

  for(ii=0; ii<ndc; ii++)
  {
    DC_SLOT = dcSlot(ii);
vmeBusLock();
    dcClear(DC_SLOT);
vmeBusUnlock();
  }

#endif

#ifdef USE_DCRB
  for(ii=0; ii<ndcrb; ii++)
  {
    DCRB_SLOT = dcrbSlot(ii);
vmeBusLock();
    dcrbClear(DCRB_SLOT);
    dcrbEnableLinkErrorCounts(DCRB_SLOT);
vmeBusUnlock();
  }
#endif

#ifdef USE_VETROC
  for(ii=0; ii<nvetroc; ii++)
  {
    VETROC_SLOT = vetrocSlot(ii);
vmeBusLock();
    vetrocClear(VETROC_SLOT);
vmeBusUnlock();
  }
#endif

#ifdef USE_DSC2
  for(ii=0; ii<ndsc2_daq; ii++)
  {
    slot = dsc2Slot(ii);
vmeBusLock();
    dsc2ResetScalersGroupA(slot);
    dsc2ResetScalersGroupB(slot);
vmeBusUnlock();
  }
#endif

#ifdef USE_SIS3801
  run_trig_count = 0;
  for(id=0; id<nsis; id++)
  {
vmeBusLock();
    sis3801control(id, DISABLE_EXT_NEXT);
    sis3801clear(id);
vmeBusUnlock();
  }
#if 0
  /* Enable interrupts */
  scalIntEnable(0x1);
#endif

#endif


#ifdef USE_HD

  if(hd_found)
  {
    /* Enable/Set Block Level on modules, if needed, here */
    hdSetBlocklevel(block_level);

    hdEnable();
    hdStatus(0);
  }

#endif


  /* always clear exceptions */
  jlabgefClearException(1);

  nusertrig = 0;
  ndone = 0;

  CDOENABLE(TIPRIMARY,TIR_SOURCE,0); /* bryan has (,1,1) ... */

  logMsg("INFO: Go 1 Executed\n",1,2,3,4,5,6);
}



void
usrtrig(unsigned int EVTYPE, unsigned int EVSOURCE)
{
  int *jw, ind, ind2, i, ii, jj, kk, jjj, blen, len, lentot, rlen, itdcbuf, nbytes, nblocks;
  unsigned int *tdcbuf_save, *tdc, utmp;
  unsigned int *dabufp1, *dabufp2;
  int njjloops, slot, type;
  int dready = 0, tdctimeout = 0, sistimeout = 0, siswasread = 0;
  int nwords;
#ifndef VXWORKS
  TIMERL_VAR;
#endif
#ifdef USE_FADC250
  unsigned int datascan, mask;
  unsigned short *dabufp16, *dabufp16_save;
  int id;
  int dCnt, idata;
  int status, stat, itime, gbready;
#endif
#ifdef USE_V1190
  int nev, rlenbuf[22];
  unsigned long tdcslot, tdcchan, tdcval, tdc14, tdcedge, tdceventcount;
  unsigned long tdceventid, tdcbunchid, tdcwordcount, tdcerrorflags;
  unsigned int *tdchead;
#ifdef SLOTWORKAROUND
  unsigned long tdcslot_h, tdcslot_t, remember_h;
#endif
#endif
#ifdef DMA_TO_BIGBUF
  unsigned int pMemBase, uMemBase, mSize;
#endif
  char *chptr, *chptr0;

#ifdef DEBUG
  printf("\n\n\nEVTYPE=%d syncFlag=%d\n",EVTYPE,syncFlag);
#endif

  if(syncFlag) printf("EVTYPE=%d syncFlag=%d\n",EVTYPE,syncFlag);

  rol->dabufp = NULL;

  /*
usleep(100);
  */
  /*
  sleep(1);
  */

#ifdef USE_SIS3801
  run_trig_count++;
  if(run_trig_count==1)
  {
    printf("First event - sis3801: nsis=%d, ENABLE_EXT_NEXT\n",nsis);
vmeBusLock();
    for(id = 0; id < nsis; id++)
    {
      sis3801control(id, ENABLE_EXT_NEXT);
    }
vmeBusUnlock();

/* read and trash whatever in buffers */


    for(ii = 0; ii < nsis; ii++)
    {
vmeBusLock();
      while( ! (sis3801status(ii) & FIFO_EMPTY) )
      {
        tdcbuf[0] = 10000;
        len = sis3801read(ii, tdcbuf);
        printf("INFO(FIRST EVENT): sis3801[%d] returned %d bytes\n",ii,len);
	  }
vmeBusUnlock();
	}

  }

#endif





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


#if 0
    /* nblocks always '1' in following */
vmeBusLock();
    nblocks = tiGetNumberOfBlocksInBuffer();
vmeBusUnlock();
    printf("TI nblocks(1) = %d\n",nblocks);fflush(stdout);

    printf("start sleeping 1 ..\n");fflush(stdout);
    sleep(1);
    printf(".. end sleeping 1\n");fflush(stdout);


vmeBusLock();
    nblocks = tiGetNumberOfBlocksInBuffer();
vmeBusUnlock();
    printf("TI nblocks(11) = %d\n",nblocks);fflush(stdout);
#endif


    /*
adcecal3_ts - works perfect
     */



#if 1
    /* Grab the data from the TI */
    tdcbuf = tdcbuf_save;
    len = 0;
vmeBusLock();
    len = tiReadBlock(tdcbuf,2048,1);
vmeBusUnlock();
    //printf("TI nwords(1) = %d\n\n",len);fflush(stdout);
    //for(jj=0; jj<len; jj++) printf("ti[%2d] 0x%08x\n",jj,LSWAP(tdcbuf[jj]));
    lentot=len;

vmeBusLock();
    nblocks = tiGetNumberOfBlocksInBuffer();
vmeBusUnlock();
    //if(nblocks!=0) printf("TI nblocks(2) = %d\n",nblocks);fflush(stdout);
    if(nblocks>7/*buffer_level*/) printf("TI nblocks(2) = %d\n",nblocks);fflush(stdout);
#endif




#if 0
    tdcbuf = tdcbuf_save;
    lentot = 0;
    while(nblocks>0)
    {
      len = 0;
vmeBusLock();
      len = tiReadBlock(tdcbuf,2048,1);
vmeBusUnlock();
      printf("TI nwords(2) = %d\n\n",len);fflush(stdout);
      for(jj=0; jj<len; jj++) printf("ti[%2d] 0x%08x\n",jj,LSWAP(tdcbuf[jj]));

vmeBusLock();
      nblocks = tiGetNumberOfBlocksInBuffer();
vmeBusUnlock();
      printf("TI nblocks(22) = %d\n",nblocks);fflush(stdout);

      tdcbuf += len;
      lentot += len;
    }

    printf("start sleeping 2 ..\n");fflush(stdout);
    sleep(1);
    printf(".. end sleeping 2\n");fflush(stdout);


vmeBusLock();
    nblocks = tiGetNumberOfBlocksInBuffer();
vmeBusUnlock();
    printf("TI nblocks(3) = %d\n",nblocks);fflush(stdout);


    printf("\n\n");fflush(stdout);
#endif








    if(lentot!=block_level*4+4) printf("ERROR: TI nwords = %d\n",lentot);fflush(stdout);
    if(lentot<=0)
    {
      printf("ERROR in tiReadBlock : No data or error, len = %d\n",lentot);
      sleep(1);
    }
    else
    {
	  
#ifdef DEBUG
      //if((len != 163) && (len != 164))
      {
        //printf("ERROR ROL1 TI: len=%d\n",lentot);
        for(jj=0; jj<lentot; jj++) printf("ti[%2d] 0x%08x\n",jj,LSWAP(tdcbuf[jj]));
      }
#endif

      BANKOPEN(0xe10A,1,rol->pid);
      for(jj=0; jj<lentot; jj++) *rol->dabufp++ = tdcbuf_save[jj];
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




#ifdef USE_V1190

    tdcbuf = tdcbuf_save;
    if(ntdcs>0)
    {


      /*check if we have 'block_level' events in every board*/
      for(jj=0; jj<ntdcs; jj++)
      {
vmeBusLock();
        nev = tdc1190Dready(jj);
vmeBusUnlock();
        if(nev < block_level)
	{
          printf("WARN: v1190/v1290[%2d] has %d events - wait\n",jj,nev);fflush(stdout);
	}
      }



vmeBusLock();
      tdc1190ReadStart(tdcbuf, rlenbuf);
vmeBusUnlock();


/*#if 0*/


      /*
      rlenbuf[0] = tdc1190ReadBoard(0, tdcbuf);
      rlenbuf[1] = tdc1190ReadBoard(1, &tdcbuf[rlenbuf[0]]);
      */

      /*check if anything left in event buffer; if yes, print warning message and clear event buffer
      for(jj=0; jj<ntdcs; jj++)
      {
        nev = tdc1190Dready(jj);
        if(nev > 0)
	{
          printf("WARN: v1290[%2d] has %d events - clear it\n",jj,nev);
          tdc1190Clear(jj);
	}
      }
      for(ii=0; ii<rlenbuf[0]; ii++) tdcbuf[ii] = LSWAP(tdcbuf[ii]);
      */

      itdcbuf = 0;
      njjloops = ntdcs;

      BANKOPEN(0xe10B,1,rol->pid);

      for(ii=0; ii<njjloops; ii++)
      {
        rlen = rlenbuf[ii];
	/*
        printf("rol1(TDCs): ii=%d, rlen=%d\n",ii,rlen);
	*/

	/*	  
#ifdef DEBUG
        level = tdc1190GetAlmostFullLevel(ii);
        iii = tdc1190StatusAlmostFull(ii);
        logMsg("ii=%d, rlen=%d, almostfull=%d level=%d\n",ii,rlen,iii,level,5,6);
#endif
	*/	  

        if(rlen <= 0) continue;

        tdc = &tdcbuf[itdcbuf];
        itdcbuf += rlen;


#ifdef SLOTWORKAROUND
	/* go through current board and fix slot number */
        for(jj=0; jj<rlen; jj++)
	{
          utmp = LSWAP(tdc[jj]);

          if( ((utmp>>27)&0x1F) == 8 ) /* GLOBAL HEADER */
	  {
            slot = utmp&0x1f;
            if( slot != slotnums[ii] )
	    {
              /*printf("ERROR: old=0x%08x: WRONG slot=%d IN GLOBAL HEADER, must be %d - fixed\n",utmp,slot,slotnums[ii]);*/
              utmp = (utmp & 0xFFFFFFE0) | slotnums[ii];
              /*printf("new=0x%08x\n",utmp);*/
              tdc[jj] = LSWAP(utmp);
            }
	  }
          else if( ((utmp>>27)&0x1F) == 0x10 ) /* GLOBAL TRAILER */
	  {
            slot = utmp&0x1f;
            if( slot != slotnums[ii] )
	    {
              /*printf("ERROR: old=0x%08x: WRONG slot=%d IN GLOBAL TRAILER, must be %d - fixed\n",utmp,slot,slotnums[ii]);*/
              utmp = (utmp & 0xFFFFFFE0) | slotnums[ii];
              /*printf("new=0x%08x\n",utmp);*/
              tdc[jj] = LSWAP(utmp);
            }
	  }
        }
#endif

        for(jj=0; jj<rlen; jj++) *rol->dabufp ++ = tdc[jj];

      } /*for(ii=0; ii<njjloops; ii++)*/

      BANKCLOSE;


/*#endif*/ /*if 0*/


    } /*if(ntdcs>0)*/


#endif /* USE_V1190 */



#ifdef USE_SIS3801

    if(nsis>0)
    {
      /* get status from all boards */
      status = 0;
      for(ii=0; ii<nsis; ii++)
      {
vmeBusLock();
        status |= sis3801status(ii);
vmeBusUnlock();

	  }

      /* if at least one board is full, reset */
      if(status & FIFO_FULL)
      {
        printf("SIS3801 IS FULL - CLEAN IT UP AND START AGAIN\n");fflush(stdout);

        for(id=0; id<nsis; id++)
        {
vmeBusLock();
          sis3801config(id, mode);
          sis3801control(id, ENABLE_EXT_NEXT);
vmeBusUnlock();
          printf("    Status = 0x%08x\n",sis3801status(id));
        }
	  }
      else
	  {
        siswasread = 0;
        for(ii=0; ii<nsis; ii++)
        {
          sistimeout = 0;
          dready = 0;
          while((dready == 0) && (sistimeout++ < 10))
          {
vmeBusLock();
            dready = (sis3801status(ii) & FIFO_EMPTY) ? 0 : 1;
vmeBusUnlock();
          }

          if(dready == 0)
		  {
            /*printf("NOT READY\n");fflush(stdout)*/;
		  }
          else
          {
            //printf("READY =======================================\n");fflush(stdout);
            tdcbuf[0] = 10000;
vmeBusLock();
            len = sis3801read(ii, tdcbuf);
vmeBusUnlock();
            if(len>=10000) printf("WARN: sis3801[%d] returned %d bytes\n",ii,len);
            len = len >> 2;
            /*printf("\nsis3801[%d]: read %d words\n",ii,len);fflush(stdout);
            for(jj = 0; jj <len; jj++)
	        {
	          if((jj%4) == 0) printf("\n%4d: ", jj);
	          printf(" 0x%08x ",tdcbuf[jj]);
	        }
            printf("\n");
			*/
            BANKOPEN(0xe125,1,/*rol->pid*/ii);
            for(jj=0; jj<len; jj++) *rol->dabufp++ = LSWAP(tdcbuf[jj]);
            BANKCLOSE;

            siswasread = 1;
		  }

		  /*
          ret = scaler7201readHLS(scaler0, ring0, nHLS);
          if(ret==0) printRingFull = 1;
          else if(ret==-1 && printRingFull==1)
          {
            printf("scaler: ring0 is full\n",0,0,0,0,0,0);
            printRingFull = 0;
          }
		  */
        }

#ifdef USE_DSC2
        if(siswasread)
		{
	      if(ndsc2_daq>0)
	      {
            BANKOPEN(0xe115,1,rol->pid);
            for(jj=0; jj<ndsc2_daq; jj++)
            {
              slot = dsc2Slot_daq(jj);
vmeBusLock();
              /* in following argument 4 set to 0xFF means latch and read everything, 0x3F - do not latch and read everything */
              nwords = dsc2ReadScalers(slot, tdcbuf, 0x10000, 0xFF, 1);
              /*printf("nwords=%d, nwords = 0x%08x 0x%08x 0x%08x 0x%08x\n",nwords,tdcbuf[0],tdcbuf[1],tdcbuf[2],tdcbuf[3]);*/
vmeBusUnlock();
              /* unlike other boards, dcs2 scaler readout already swapped in 'dsc2ReadScalers', so swap it back, because
              rol2.c expects big-endian format*/
              for(kk=0; kk<nwords; kk++) *rol->dabufp ++ = LSWAP(tdcbuf[kk]);
            }
            BANKCLOSE;
	      }
		}
#endif

	  }
    }
#endif



#ifdef USE_SSP_RICH
    ///////////////////////////////////////
    // SSP_CFG_SSPTYPE_HALLBRICH Readout //
    ///////////////////////////////////////
    tdcbuf = tdcbuf_save;
    dCnt=0;
    for(ii=0; ii<nssp; ii++)
    {
      slot = sspSlot(ii);
      type = sspGetFirmwareType_Shadow(slot);
      
      if(type == SSP_CFG_SSPTYPE_HALLBRICH)
      {
#ifdef DEBUG
      printf("Calling sspBReady(%d) ...\n", slot); fflush(stdout);
#endif
        for(itime=0; itime<100000; itime++) 
        {
          vmeBusLock();
          gbready = sspBReady(slot);
          vmeBusUnlock();
          
          if(gbready)
            break;
#ifdef DEBUG
          else
            printf("SSP NOT READY (slot=%d)\n",slot);
#endif
        }

        if(!gbready)
        {
          printf("SSP NOT READY (slot=%d)\n",slot);
          
          ssp_not_ready_errors[slot]++;
        }
#ifdef DEBUG
        else
          printf("SSP IS READY (slot=%d)\n",slot);
#endif
/*
        sspPrintEbStatus(slot);
        printf(" ");
*/
        vmeBusLock();
        len = sspReadBlock(slot,&tdcbuf[dCnt],0x10000,1);
        vmeBusUnlock();
      
/*
        printf("ssp tdcbuf[%2d]:", len);
        for(jj=0;jj<(len>40?40:len);jj++)
          printf(" 0x%08x",tdcbuf[jj]);
        
        printf(" ");
        sspPrintEbStatus(slot);
        printf("\n");
*/

        dCnt += len;
      }
    }

    if(dCnt>0)
    {
      for(ii=0; ii<nssp; ii++)
      {
        slot = sspSlot(ii);
        type = sspGetFirmwareType_Shadow(slot);
        if( (type == SSP_CFG_SSPTYPE_HALLBRICH) && (ssp_not_ready_errors[slot]) )
        {
          printf("SSP Read Errors:");
          for(ii=0; ii<nssp; ii++)
          {
            slot = sspSlot(ii);
            type = sspGetFirmwareType_Shadow(slot);
            if(type == SSP_CFG_SSPTYPE_HALLBRICH)
              printf(" %4d", ssp_not_ready_errors[slot]);
          }
          printf("\n");
          break;
        }
      }
    }

    if(dCnt>0)
    {
      BANKOPEN(0xe123,1,rol->pid);
      for(jj=0; jj<dCnt; jj++) *rol->dabufp++ = tdcbuf[jj];
      BANKCLOSE;
    }

#endif /* USE_SSP_RICH */

#if 0
#ifdef USE_SSP
    if(nssp>0)
    {
#ifdef DEBUG
      printf("Calling sspGBReady ...\n");fflush(stdout);
#endif
      for(itime=0; itime<100000; itime++) 
	    {
vmeBusLock();
	      gbready = sspGBReady();
vmeBusUnlock();
	      stat = (gbready == sspSlotMask);
	      if (stat>0) 
	      {
	        break;
	      }
#ifdef DEBUG
		    else
		    {
          printf("SSP NOT READY: gbready=0x%08x, expect 0x%08x\n",gbready,sspSlotMask);
		    }
#endif
	    }


#ifdef DEBUG
      printf("SSP IS READY: gbready=0x%08x, expect 0x%08x\n",gbready,sspSlotMask);
#endif

vmeBusLock();
      len = sspReadBlock(0,tdcbuf,0x10000,1);
vmeBusUnlock();
	  
/*
      printf("ssp tdcbuf[%2d]: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
          len,tdcbuf[0],tdcbuf[1],tdcbuf[2],tdcbuf[3],tdcbuf[4],tdcbuf[5],tdcbuf[6]);
*/

	  
      if(len>0)
      {
        BANKOPEN(0xe10C,1,rol->pid);
        for(jj=0; jj<len; jj++) *rol->dabufp++ = tdcbuf[jj];
        BANKCLOSE;
      }
	  
    }
#endif /* USE_SSP */
#endif

#ifdef USE_FADC250

    /* Configure Block Type... temp fix for 2eSST trouble with token passing */
    tdcbuf = tdcbuf_save;
    dCnt=0;
    if(nfadc>0)
    {


#ifdef DEBUG
	  printf("FADC250 readout starts\n");fflush(stdout);
#endif

/*COMMENT OUT FOLLOWING 'FOR' LOOP FOR SPEED UP !!!*/
      for(itime=0; itime<200000/*100000*/; itime++) 
	  {
vmeBusLock();
	    gbready = faGBready();
vmeBusUnlock();
	    stat = (gbready == fadcSlotMask);
	    if (stat>0) 
	    {
	      break;
	    }
	  }

      if(stat>0)
	  {
        BANKOPEN(0xe109,1,rol->pid);

        FA_SLOT = faSlot(0);
        if(FADC_ROFLAG==2)
        {

#ifdef DMA_TO_BIGBUF
 		  /*
          printf("dabufp_usermembase=0x%08x\n",dabufp_usermembase);
          printf("dabufp_physmembase=0x%08x\n",dabufp_physmembase);
           */
 /*1ms->*/
          uMemBase = dabufp_usermembase;
          pMemBase = dabufp_physmembase;
          mSize = 0x100000;
 	      /*printf("desired: 0x%08x 0x%08x 0x%08x\n",pMemBase,uMemBase,mSize);*/
          usrChangeVmeDmaMemory(pMemBase, uMemBase, mSize);
 
          usrVmeDmaMemory(&pMemBase, &uMemBase, &mSize);
 	      /*printf("actual: 0x%08x 0x%08x 0x%08x\n",pMemBase,uMemBase,mSize);*/
 /*->1us*/
 
 
 /*25us->*/
vmeBusLock();
          dCnt = faReadBlock(FA_SLOT,rol->dabufp,/*(dmaMemSize/4)*/MAXFADCWORDS,FADC_ROFLAG);
vmeBusUnlock();
 /*->25us*/
#ifdef DEBUG
  		  printf("dCnt=%d\n",dCnt);
          for(jjj=0; jjj<dCnt; jjj++) printf(" [%3d]  0x%08x\n",jjj,rol->dabufp[jjj]);
#endif
          rol->dabufp += dCnt;

          usrRestoreVmeDmaMemory();
          usrVmeDmaMemory(&pMemBase, &uMemBase, &mSize);
 	      /*printf("restored: 0x%08x 0x%08x 0x%08x\n",pMemBase,uMemBase,mSize);*/

#else

#ifdef DEBUG
          printf("fadc1: Starting DMA, dmaMemSize=%d(0x%08x) bytes\n",dmaMemSize,dmaMemSize);fflush(stdout);
#endif

vmeBusLock();
          dCnt = faReadBlock(FA_SLOT,tdcbuf,/*(dmaMemSize/4)*/MAXFADCWORDS,FADC_ROFLAG);
vmeBusUnlock();

          if(fadcBlockError)
		  {
            printf("fadc1 ERROR: Finished DMA, fadcBlockError=%d\n",fadcBlockError);fflush(stdout);
            printf("MAXFADCWORDS=%d can be too small\n",MAXFADCWORDS);fflush(stdout);
		  }
#ifdef DEBUG
          printf("fadc1: Finished DMA, dCnt*4=%d bytes, fadcBlockError=%d\n",dCnt*4,fadcBlockError);fflush(stdout);
#endif
		  
          if((dCnt*4) >= dmaMemSize)
		  {
            printf("ERROR !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");fflush(stdout);
            printf("ERROR !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");fflush(stdout);
            printf("ERROR: increase dmaMemSize above %d bytes by calling usrVmeDmaSetMemSize(size)\n",dCnt*4);fflush(stdout);
            printf("       and recompile rol1 (see TIPRIMARY_source.h)\n");fflush(stdout);
            printf("ERROR !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");fflush(stdout);
            printf("ERROR !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");fflush(stdout);
            exit(0);
          }
		  
#endif


        }
        else
		{




          for(jj=0; jj<nfadc; jj++)
		  {
#ifdef DEBUG
            printf("fadc1: [%d] Starting DMA\n",jj);fflush(stdout);
#endif
#ifdef DMA_TO_BIGBUF

            uMemBase = dabufp_usermembase;
            pMemBase = dabufp_physmembase;
            mSize = 0x100000;
            usrChangeVmeDmaMemory(pMemBase, uMemBase, mSize);


vmeBusLock();
	        len = faReadBlock(faSlot(jj),rol->dabufp,MAXFADCWORDS,FADC_ROFLAG);
vmeBusUnlock();
            rol->dabufp += len;
            dCnt += len;

            usrRestoreVmeDmaMemory();
#else
vmeBusLock();
	        len = faReadBlock(faSlot(jj),&tdcbuf[dCnt],MAXFADCWORDS,FADC_ROFLAG);
vmeBusUnlock();
            dCnt += len;
#endif

			/*
			if(len!=21244)
			{
			printf("!!!!!!!!!!!!!!!!!!! ERROR len=%d\n",len);
            faStatus(jj,0);
            printf("FADC board %d: len=%d dCnt=%d\n",jj,len,dCnt);
            for(jjj=0; jjj<len; jjj++) printf(" [%3d]  0x%08x (tag=0x%02x)\n",jjj,LSWAP(tdcbuf[(dCnt-len)+jjj]),((LSWAP(tdcbuf[(dCnt-len)+jjj])>>27)&0x1F));
            printf("End of FADCs data\n");
			}
			*/
		  }




	    }





	    if(dCnt<=0)
	    {
	      printf("FADCs: No data or error.  dCnt = %d (slots from %d)\n",dCnt,FA_SLOT);
          dCnt=0;
	    }
	    else
	    {
#ifndef DMA_TO_BIGBUF
#ifdef DEBUG
          printf("fadc: moving %d words to dabufp starting from address 0x%lx\n",dCnt,rol->dabufp);fflush(stdout);
#endif
          for(jj=0; jj<dCnt; jj++) *rol->dabufp++ = tdcbuf[jj];
#ifdef DEBUG
          printf("fadc: ending dabufp address 0x%lx\n",rol->dabufp);fflush(stdout);
#endif
#endif
        }

        BANKCLOSE;

	  }
      else 
	  {
	    printf ("FADCs: no events   stat=%d  intcount = %d   gbready = 0x%08x  fadcSlotMask = 0x%08x\n",
		  stat,tiGetIntCount(),gbready,fadcSlotMask);
        printf("Missing slots:");
        for(jj=1; jj<21; jj++)
		{
          mask = 1<<jj;
          if((fadcSlotMask&mask) && !(gbready&mask)) printf("%3d",jj);
		}
        printf("\n");fflush(stdout);




		{
          printf("\n============= trying to read troubled FADCs ===================\n");fflush(stdout);

          for(jj=1; jj<21; jj++)
		  {
            mask = 1<<jj;
            if((fadcSlotMask&mask) && !(gbready&mask))
			{
              printf("FADC in slot %3d:\n",jj);fflush(stdout);
vmeBusLock();
              faStatus(jj,0);
	          len = faReadBlock(jj,tdcbuf,MAXFADCWORDS,1);
vmeBusUnlock();
              printf("ERROR: Printing %d words from FADC %d\n",len,jj);
              for(jjj=0; jjj<len; jjj++) printf(" [%3d]  0x%08x (tag=0x%02x)\n",jjj,LSWAP(tdcbuf[jjj]),((LSWAP(tdcbuf[jjj])>>27)&0x1F));
              printf("End of FADC data\n");
			}
		  }
          printf("\n============= finished reading troubled FADCs ===================\n");
		}






	  }

      /* Reset the Token */
      if(FADC_ROFLAG==2)
	  {
/*2us->*/
	    for(id=0; id<nfadc; id++)
	    {
	      FA_SLOT = faSlot(id);
vmeBusLock();
	      faResetToken(FA_SLOT);
vmeBusUnlock();
	    }
/*->2us*/
	  }

#ifdef DEBUG
	  printf("FADC250 readout ends\n");fflush(stdout);
#endif

    }

#endif /* USE_FADC250 */





#ifdef USE_VSCM

    /* Configure Block Type... temp fix for 2eSST trouble with token passing */
    tdcbuf = tdcbuf_save;
    dCnt=0;
    if(nvscm1 != 0)
    {

#ifdef DEBUG
      printf("Calling vscmGBReady ...\n");fflush(stdout);
#endif
      for(itime=0; itime<100000; itime++) 
	  {
vmeBusLock();
	    gbready = vscmGBReady();
vmeBusUnlock();
	    stat = (gbready == vscmSlotMask);
	    if (stat>0) 
	    {
	      break;
	    }
#ifdef DEBUG
		else
		{
          printf("VSCM NOT READY: gbready=0x%08x, expect 0x%08x\n",gbready,vscmSlotMask);
		}
#endif
	  }


#ifdef DEBUG
	  
	  /* print fifo info */
      printf("FIFOs info:\n");fflush(stdout);
      for(jj=0; jj<nvscm1; jj++)
	  {
vmeBusLock();
        vscmStat(vscmSlot(jj));
vmeBusUnlock();
	  }
      printf("mask=0x%08x gbready=0x%08x stat=%d\n",vscmSlotMask,gbready,stat);
	  
#endif
	  
      if(stat>0)
	  {
        BANKOPEN(0xe104,1,rol->pid);

        if(VSCM_ROFLAG==2)
        {
          slot = vscmSlot(0);
#ifdef DMA_TO_BIGBUF
          uMemBase = dabufp_usermembase;
          pMemBase = dabufp_physmembase;
          mSize = 0x100000;
          usrChangeVmeDmaMemory(pMemBase, uMemBase, mSize);
 
          usrVmeDmaMemory(&pMemBase, &uMemBase, &mSize);
 


vmeBusLock();
          dCnt = vscmReadBlock(slot,rol->dabufp,500000/*MAXVSCMWORDS*/,VSCM_ROFLAG);
vmeBusUnlock();
#ifdef DEBUG
		  printf("readout ends, len=%d\n",dCnt);
          /*if(len>12) len=12;*/
          vscmPrintFifo(rol->dabufp,dCnt);
#endif
          rol->dabufp += dCnt;
		  /*  
 		  printf("dCnt=%d, data: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",dCnt,
 		  rol->dabufp[0],rol->dabufp[1],rol->dabufp[2],rol->dabufp[3],rol->dabufp[4],rol->dabufp[5]);
 		  */
          usrRestoreVmeDmaMemory();
          usrVmeDmaMemory(&pMemBase, &uMemBase, &mSize);
#else

vmeBusLock();
	      dCnt = vscmReadBlock(slot,tdcbuf,500000/*MAXVSCMWORDS*/,VSCM_ROFLAG);
vmeBusUnlock();

#ifdef DEBUG
		  printf("readout ends, len=%d\n",dCnt);
          /*if(len>12) len=12;*/
          vscmPrintFifo(tdcbuf,dCnt);
#endif




#endif
        }
        else
		{
#ifdef DEBUG
		  printf("readout, VSCM_ROFLAG=%d\n",VSCM_ROFLAG);
#endif
          for(jj=0; jj<nvscm1; jj++)
		  {
#ifdef DMA_TO_BIGBUF

            uMemBase = dabufp_usermembase;
            pMemBase = dabufp_physmembase;
            mSize = 0x100000;
            usrChangeVmeDmaMemory(pMemBase, uMemBase, mSize);


vmeBusLock();
	        len = vscmReadBlock(vscmSlot(jj),rol->dabufp,MAXVSCMWORDS,VSCM_ROFLAG);
vmeBusUnlock();

#ifdef DEBUG
			printf("readout ends, len=%d\n",len);
            /*if(len>12) len=12;*/
            vscmPrintFifo(rol->dabufp,len);
#endif

			/*			
vscmPrintFifo(rol->dabufp,len);
			*/

            rol->dabufp += len;
            dCnt += len;

            usrRestoreVmeDmaMemory();

#else

vmeBusLock();
	        len = vscmReadBlock(vscmSlot(jj),&tdcbuf[dCnt],MAXVSCMWORDS,VSCM_ROFLAG);
vmeBusUnlock();

            if(len>(MAXVSCMWORDS-10))
			{
              printf("VSCM data from slot %d too large (%d words), increase MAXVSCMWORDS !\n",vscmSlot(jj),len);
			}

#ifdef DEBUG
			printf("readout ends, len=%d\n",len);
            /*if(len>12) len=12;*/
            vscmPrintFifo(&tdcbuf[dCnt],len);
#endif

            dCnt += len;
#endif

#ifdef DEBUG
            printf("[%d] len=%d dCnt=%d\n",jj,len,dCnt);
#endif
		  }
	    }

	    if(dCnt<=0)
	    {
	      printf("VSCM: No data or error.  dCnt = %d (slots from %d)\n",dCnt,slot);
          dCnt=0;
	    }
	    else
	    {
#ifndef DMA_TO_BIGBUF
          for(jj=0; jj<dCnt; jj++) *rol->dabufp++ = tdcbuf[jj];
#endif
	    }

        BANKCLOSE;
	  }
      else 
	  {
	    printf ("VSCMs: no events   stat=%d  intcount = %d   gbready = 0x%08x  vscmSlotMask = 0x%08x\n",
		  stat,tiGetIntCount(),gbready,vscmSlotMask);
        printf("Missing slots:");
        for(jj=1; jj<21; jj++)
		{
          mask = 1<<jj;
          if((vscmSlotMask&mask) && !(gbready&mask)) printf("%3d",jj);
		}
        printf("\n");
	  }

      /* Reset the Token */
      if(VSCM_ROFLAG==2)
	  {
	    for(ii=0; ii</*1*/nvscm1; ii++)
	    {
	      slot = vscmSlot(ii);
vmeBusLock();
	      vscmResetToken(slot);
vmeBusUnlock();
	    }
	  }

    }

#endif /* USE_VSCM */




#ifdef USE_DC



    /* Configure Block Type... temp fix for 2eSST trouble with token passing */
    tdcbuf = tdcbuf_save;
    dCnt=0;
    if(ndc != 0)
    {
      stat = 0;
      for(itime=0; itime<100000; itime++) 
	  {
vmeBusLock();
	    gbready = dcGBready();
vmeBusUnlock();
	    stat = (gbready == dcSlotMask);
	    if (stat>0) 
	    {
          printf("expected mask 0x%08x, got 0x%08x\n",dcSlotMask,gbready);
	      break;
	    }
	  }
      if(stat==0) printf("dc not ready !!!\n");


      if(stat>0)
	  {
        BANKOPEN(0xe105,1,rol->pid);

        DC_SLOT = dcSlot(0);
        if(DC_ROFLAG==2)
        {
#ifdef DMA_TO_BIGBUF
          uMemBase = dabufp_usermembase;
          pMemBase = dabufp_physmembase;
          mSize = 0x100000;
          usrChangeVmeDmaMemory(pMemBase, uMemBase, mSize);
 
          usrVmeDmaMemory(&pMemBase, &uMemBase, &mSize);
 
vmeBusLock();
 	      dCnt = dcReadBlock(DC_SLOT,rol->dabufp,MAXDCWORDS,DC_ROFLAG);
vmeBusUnlock();
          rol->dabufp += dCnt;
          usrRestoreVmeDmaMemory();
#else
vmeBusLock();
	      dCnt = dcReadBlock(DC_SLOT,tdcbuf,MAXDCWORDS,DC_ROFLAG);
vmeBusUnlock();
#endif
        }
        else
		{
          for(jj=0; jj<ndc; jj++)
	      {
            DC_SLOT = dcSlot(jj);
	        /*dcPrintScalers(DC_SLOT);*/
#ifdef DMA_TO_BIGBUF
            uMemBase = dabufp_usermembase;
            pMemBase = dabufp_physmembase;
            mSize = 0x100000;
            usrChangeVmeDmaMemory(pMemBase, uMemBase, mSize);

vmeBusLock();
            len = dcReadBlock(DC_SLOT, rol->dabufp, MAXDCWORDS, DC_ROFLAG);
vmeBusUnlock();
/*#ifdef DEBUG*/
            printf("DC: slot=%d, nw=%d, data-> 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
			  DC_SLOT,len,LSWAP(rol->dabufp[0]),LSWAP(rol->dabufp[1]),LSWAP(rol->dabufp[2]),
			  LSWAP(rol->dabufp[3]),LSWAP(rol->dabufp[4]),LSWAP(rol->dabufp[5]),LSWAP(rol->dabufp[6]));
/*#endif*/
            rol->dabufp += len;
            dCnt += len;

            usrRestoreVmeDmaMemory();

#else

vmeBusLock();
            len = dcReadBlock(DC_SLOT, &tdcbuf[dCnt], MAXDCWORDS, DC_ROFLAG);
vmeBusUnlock();
/*#ifdef DEBUG*/
            printf("DC: slot=%d, nw=%d, data-> 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
			  DC_SLOT,len,LSWAP(tdcbuf[dCnt+0]),LSWAP(tdcbuf[dCnt+1]),LSWAP(tdcbuf[dCnt+2]),
              LSWAP(tdcbuf[dCnt+3]),LSWAP(tdcbuf[dCnt+4]),LSWAP(tdcbuf[dCnt+5]),LSWAP(tdcbuf[dCnt+6]));
/*#endif*/
            dCnt += len;
#endif
	      }
		}

	    if(dCnt<=0)
	    {
	      printf("DC: No data or error.  dCnt = %d\n",dCnt);
          dCnt=0;
	    }
        else
	    {
#ifndef DMA_TO_BIGBUF
          for(jj=0; jj<dCnt; jj++) *rol->dabufp++ = tdcbuf[jj];
#endif
	    }

        BANKCLOSE;
	  }
	  else
	  {
	    printf ("DCs: no events   stat=%d  intcount = %d   gbready = 0x%08x  dcSlotMask = 0x%08x\n",
		  stat,tiGetIntCount(),gbready,dcSlotMask);
        printf("Missing slots:");
        for(jj=1; jj<21; jj++)
		{
          mask = 1<<jj;
          if((dcSlotMask&mask) && !(gbready&mask)) printf("%3d",jj);
		}
        printf("\n");
	  }

      /* Reset the Token */
      if(DC_ROFLAG==2)
	  {
	    for(jj=0; jj<ndc; jj++)
	    {
	      DC_SLOT = dcSlot(jj);
vmeBusLock();
	      /*dcResetToken(DC_SLOT);not implemented yet !!!!!!!!!!*/
vmeBusUnlock();
	    }
	  }

    }

#endif /*USE_DC*/





#ifdef USE_DCRB



    /* Configure Block Type... temp fix for 2eSST trouble with token passing */
    tdcbuf = tdcbuf_save;
    dCnt=0;
    if(ndcrb != 0)
    {
      stat = 0;
      for(itime=0; itime<100000; itime++) 
	  {
vmeBusLock();
	    gbready = dcrbGBready();
vmeBusUnlock();
	    stat = (gbready == dcrbSlotMask);
	    if (stat>0) 
	    {
          if(dcrbSlotMask!=gbready) printf("expected mask 0x%08x, got 0x%08x\n",dcrbSlotMask,gbready);
	      break;
	    }
	  }
      if(stat==0) printf("dcrb not ready !!!\n");


      if(stat>0)
      {
        BANKOPEN(0xe105,1,rol->pid);

        DCRB_SLOT = dcrbSlot(0);
        if(DCRB_ROFLAG==2)
        {
#ifdef DMA_TO_BIGBUF
          uMemBase = dabufp_usermembase;
          pMemBase = dabufp_physmembase;
          mSize = 0x100000;
          usrChangeVmeDmaMemory(pMemBase, uMemBase, mSize);
 
          usrVmeDmaMemory(&pMemBase, &uMemBase, &mSize);
 
vmeBusLock();
 	      dCnt = dcrbReadBlock(DCRB_SLOT,rol->dabufp,MAXDCRBWORDS,DCRB_ROFLAG);
vmeBusUnlock();
          rol->dabufp += dCnt;
          usrRestoreVmeDmaMemory();
#else
vmeBusLock();
	      dCnt = dcrbReadBlock(DCRB_SLOT,tdcbuf,MAXDCRBWORDS,DCRB_ROFLAG);
vmeBusUnlock();
#endif
        }
        else
	{
          for(jj=0; jj<ndcrb; jj++)
	  {
            DCRB_SLOT = dcrbSlot(jj);
#ifdef DMA_TO_BIGBUF
            uMemBase = dabufp_usermembase;
            pMemBase = dabufp_physmembase;
            mSize = 0x100000;
            usrChangeVmeDmaMemory(pMemBase, uMemBase, mSize);

vmeBusLock();
            len = dcrbReadBlock(DCRB_SLOT, rol->dabufp, MAXDCRBWORDS, DCRB_ROFLAG);
vmeBusUnlock();

#ifdef DEBUG
            printf("DCRB: slot=%d, nw=%d, data-> 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
			  DCRB_SLOT,len,LSWAP(rol->dabufp[0]),LSWAP(rol->dabufp[1]),LSWAP(rol->dabufp[2]),
			  LSWAP(rol->dabufp[3]),LSWAP(rol->dabufp[4]),LSWAP(rol->dabufp[5]),LSWAP(rol->dabufp[6]));
#endif
            rol->dabufp += len;
            dCnt += len;

            usrRestoreVmeDmaMemory();

#else

vmeBusLock();
            len = dcrbReadBlock(DCRB_SLOT, &tdcbuf[dCnt], MAXDCRBWORDS, DCRB_ROFLAG);
vmeBusUnlock();
#ifdef DEBUG
            printf("DC: slot=%d, len=%d\n",DCRB_SLOT,len);
            for(i=0; i<len; i++)
            {
              printf("  [%3d] 0x%08x\n",i,LSWAP(tdcbuf[dCnt+i]));
            }
#endif
            dCnt += len;
#endif
	  }
	}

	if(dCnt<=0)
	{
	  printf("DCRB: No data or error.  dCnt = %d\n",dCnt);
          dCnt=0;
	}
        else
	{
#ifndef DMA_TO_BIGBUF
          for(jj=0; jj<dCnt; jj++) *rol->dabufp++ = tdcbuf[jj];
#endif
	}

        BANKCLOSE;
      }
      else
      {
	    printf ("DCRBs: no events   stat=%d  intcount = %d   gbready = 0x%08x  dcrbSlotMask = 0x%08x\n",
		  stat,tiGetIntCount(),gbready,dcrbSlotMask);
        printf("Missing slots:");
        for(jj=1; jj<21; jj++)
		{
          mask = 1<<jj;
          if((dcrbSlotMask&mask) && !(gbready&mask)) printf("%3d",jj);
		}
        printf("\n");
        for(jj=1; jj<21; jj++)
		{
          mask = 1<<jj;
          if((dcrbSlotMask&mask) && !(gbready&mask)) dcrbStatus(jj, 0);
		}
	  }


      /* Reset the Token */
      if(DCRB_ROFLAG==2)
	  {
	    for(jj=0; jj<ndcrb; jj++)
	    {
	      DCRB_SLOT = dcrbSlot(jj);
vmeBusLock();
	      /*dcResetToken(DC_SLOT);not implemented yet !!!!!!!!!!*/
vmeBusUnlock();
	    }
	  }

    }

#endif /*USE_DCRB*/




#ifdef USE_VFTDC

  tdcbuf = tdcbuf_save;
  dCnt = 0;
  for(jj=0; jj<MAXVFTDCWORDS+10; jj++) tdcbuf[jj] = 0;
  for(ii=0; ii<nvftdc; ii++)
  {
    slot = vfTDCSlot(ii);

vmeBusLock();
    gbready = vfTDCBReady(slot);
vmeBusUnlock();
    tdctimeout = 0;
    while(gbready==0 && tdctimeout<100)
    {
vmeBusLock();
      gbready = vfTDCBReady(slot);
vmeBusUnlock();
      tdctimeout++; 
    }
    if(tdctimeout>=100)
    {
      printf("%s: Data not ready in vfTDC, tdctimeout=%d\n",__FUNCTION__,tdctimeout);
vmeBusLock();
      vfTDCStatus(slot,1);
vmeBusUnlock();
    }
	else
	{
      //printf("%s: Data ready in vfTDC, tdctimeout=%d\n",__FUNCTION__,tdctimeout);

vmeBusLock();
      len = vfTDCReadBlock(slot, &tdcbuf[dCnt], MAXVFTDCWORDS, 1);
vmeBusUnlock();
      if(len<=0)
      {
        printf("%s: No vfTDC data or error in slot %d.  len = %d\n",__FUNCTION__,slot,len);
        vfTDCStatus(slot,1);
      }
	  else
	  {
        //printf("--> slot=%d, dCnt=%d, len=%d, data 0x%08x 0x%08x 0x%08x ...\n",slot,dCnt,len,tdcbuf[dCnt],tdcbuf[dCnt+1],tdcbuf[dCnt+2]);
        dCnt += len;
	  }

	} /* no timeout - should have data */

  } /* loop over slots */

  if(dCnt>0)
  {
    BANKOPEN(0xe131,1,rol->pid);
    //printf("dCnt=%d\n",dCnt);
    for(jj=0; jj<dCnt; jj++)
	{
      //printf("data[%5d]=0x%08x (0x%08x)\n",jj,tdcbuf[jj],LSWAP(tdcbuf[jj]));
      *rol->dabufp++ = tdcbuf[jj];
	}
	//exit(0);
    BANKCLOSE;
  }

#endif




#ifdef USE_HD

  if(hd_found)
  {
    timeout = 0;
    tdcbuf = tdcbuf_save;
    dCnt=0;
vmeBusLock();
    gbready = hdBReady(0);
vmeBusUnlock();
    while((gbready!=1) && (timeout<1000))
    {
      timeout++;
vmeBusLock();
      gbready = hdBReady(0);
vmeBusUnlock();
    }

    if(timeout>=1000)
    {
      printf("ROL1 ERROR: TIMEOUT waiting for Helicity Decoder Block Ready\n");fflush(stdout);
    }
    else
    {
      /*printf("ROL1 INFO: Helicity Decoder Block Ready\n");fflush(stdout);*/

vmeBusLock();
      len = hdReadBlock(&tdcbuf[dCnt], HDMAXWORDS, 1);
vmeBusUnlock();

      if(len<=0)
      {
	printf("ROL1 ERROR or NO data from hdReadBlock(...) = %d\n",len);fflush(stdout);
      }
      else if(len>HDMAXWORDS)
      {
        printf("ROL1 ERROR in hdReadBlock(...): returned %d(0x%08x) which is bigger then HDMAXWORDS=%d\n",
	       len,len,HDMAXWORDS);fflush(stdout);
      }
      else
      {
        /*printf("ROL1 INFO: hdReadBlock(...) returned %d\n",len);fflush(stdout);*/
	dCnt += len;
      }
    }

    if(dCnt>0)
    {
      /*for(jj=0; jj<dCnt; jj++) printf(" data[%3d] = 0x%08x\n",jj,tdcbuf[jj]);*/
      BANKOPEN(0xe133,1,rol->pid);
      for(jj=0; jj<dCnt; jj++) *rol->dabufp++ = tdcbuf[jj];
      BANKCLOSE;
    }
	
  }

#endif





#ifdef USE_VETROC
    /* Configure Block Type... temp fix for 2eSST trouble with token passing */
    tdcbuf = tdcbuf_save;
    dCnt=0;
    if(nvetroc != 0)
    {
      stat = 0;
      for(itime=0; itime<100000; itime++) 
	  {
vmeBusLock();
	    gbready = vetrocGBready();
vmeBusUnlock();
	    stat = (gbready == vetrocSlotMask);
	    if (stat>0) 
	    {
          printf("expected mask 0x%08x, got 0x%08x\n",vetrocSlotMask,gbready);
	      break;
	    }
	  }
      if(stat==0) printf("vetroc not ready !!!\n");


      if(stat>0)
	  {
        BANKOPEN(0xe105,1,rol->pid);

        VETROC_SLOT = vetrocSlot(0);
        if(VETROC_ROFLAG==2)
        {
#ifdef DMA_TO_BIGBUF
          uMemBase = dabufp_usermembase;
          pMemBase = dabufp_physmembase;
          mSize = 0x100000;
          usrChangeVmeDmaMemory(pMemBase, uMemBase, mSize);
 
          usrVmeDmaMemory(&pMemBase, &uMemBase, &mSize);
 
vmeBusLock();
 	      dCnt = vetrocReadBlock(VETROC_SLOT,rol->dabufp,MAXVETROCWORDS,VETROC_ROFLAG);
vmeBusUnlock();
          rol->dabufp += dCnt;
          usrRestoreVmeDmaMemory();
#else
vmeBusLock();
	      dCnt = vetrocReadBlock(VETROC_SLOT,tdcbuf,MAXVETROCWORDS,VETROC_ROFLAG);
vmeBusUnlock();
#endif
        }
        else
		{
          for(jj=0; jj<nvetroc; jj++)
	      {
            VETROC_SLOT = vetrocSlot(jj);
#ifdef DMA_TO_BIGBUF
            uMemBase = dabufp_usermembase;
            pMemBase = dabufp_physmembase;
            mSize = 0x100000;
            usrChangeVmeDmaMemory(pMemBase, uMemBase, mSize);

vmeBusLock();
            len = vetrocReadBlock(VETROC_SLOT, rol->dabufp, MAXVETROCWORDS, VETROC_ROFLAG);
vmeBusUnlock();
/*#ifdef DEBUG*/
            printf("VETROC: slot=%d, nw=%d, data-> 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
			  VETROC_SLOT,len,LSWAP(rol->dabufp[0]),LSWAP(rol->dabufp[1]),LSWAP(rol->dabufp[2]),
			  LSWAP(rol->dabufp[3]),LSWAP(rol->dabufp[4]),LSWAP(rol->dabufp[5]),LSWAP(rol->dabufp[6]));
/*#endif*/
            rol->dabufp += len;
            dCnt += len;

            usrRestoreVmeDmaMemory();
#else
vmeBusLock();
            len = vetrocReadBlock(VETROC_SLOT, &tdcbuf[dCnt], MAXVETROCWORDS, VETROC_ROFLAG);
vmeBusUnlock();
/*#ifdef DEBUG*/
            printf("VETROC: slot=%d, nw=%d, data-> 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
			  VETROC_SLOT,len,LSWAP(tdcbuf[dCnt+0]),LSWAP(tdcbuf[dCnt+1]),LSWAP(tdcbuf[dCnt+2]),
              LSWAP(tdcbuf[dCnt+3]),LSWAP(tdcbuf[dCnt+4]),LSWAP(tdcbuf[dCnt+5]),LSWAP(tdcbuf[dCnt+6]));
/*#endif*/
            dCnt += len;
#endif
	      }
		}

	    if(dCnt<=0)
	    {
	      printf("VETROC: No data or error.  dCnt = %d\n",dCnt);
          dCnt=0;
	    }
        else
	    {
#ifndef DMA_TO_BIGBUF
          for(jj=0; jj<dCnt; jj++) *rol->dabufp++ = tdcbuf[jj];
#endif
	    }

        BANKCLOSE;
	  }
	  else
	  {
	    printf ("VETROCs: no events   stat=%d  intcount = %d   gbready = 0x%08x  VETROCSlotMask = 0x%08x\n",
		  stat,tiGetIntCount(),gbready,vetrocSlotMask);
        printf("Missing slots:");
        for(jj=1; jj<21; jj++)
		{
          mask = 1<<jj;
          if((vetrocSlotMask&mask) && !(gbready&mask)) printf("%3d",jj);
		}
        printf("\n");
	  }

      /* Reset the Token */
      if(VETROC_ROFLAG==2)
	  {
	    for(jj=0; jj<nvetroc; jj++)
	    {
	      VETROC_SLOT = vetrocSlot(jj);
vmeBusLock();
	      /*vetrocResetToken(DC_SLOT);not implemented yet !!!!!!!!!!*/
vmeBusUnlock();
	    }
	  }
    }
#endif /*USE_VETROC*/


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

      nwords = 6; /* UPDATE THAT IF THE NUMBER OF WORDS CHANGED BELOW !!! */
      *rol->dabufp ++ = LSWAP((0x14<<27)+nwords); /*head data*/

      /* COUNT DATA WORDS FROM HERE */
      *rol->dabufp ++ = 0; /*version  number */
      *rol->dabufp ++ = LSWAP(RUN_NUMBER); /*run  number */
      *rol->dabufp ++ = LSWAP(event_number); /*event number */
      if(ii==(block_level-1))
	  {
        *rol->dabufp ++ = LSWAP(time(0)); /*event unix time */
        *rol->dabufp ++ = LSWAP(EVTYPE);  /*event type */
        *rol->dabufp ++ = 0;              /*reserved for L3 info*/
	  }
      else
	  {
        *rol->dabufp ++ = 0;
        *rol->dabufp ++ = 0;
        *rol->dabufp ++ = 0;
	  }
      /* END OF DATA WORDS */

	}

    nwords = ((int)rol->dabufp-(int)dabufp1)/4 + 1;

    *rol->dabufp ++ = LSWAP((0x11<<27)+nwords); /*block trailer*/

    BANKCLOSE;

#endif




#ifndef VXWORKS
TIMERL_STOP(100000/block_level,1000+rol->pid);
#endif





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

vmeBusLock();
      len = tiUploadAll(chptr, 10000);
vmeBusUnlock();
      /*printf("\nTI len=%d\n",len);
      printf(">%s<\n",chptr);*/
      chptr += len;
      nbytes += len;


#ifdef USE_FADC250
      if(nfadc>0)
      {
vmeBusLock();
        len = fadc250UploadAll(chptr, 14000);
vmeBusUnlock();
        /*printf("\nFADC len=%d\n",len);
        printf("%s\n",chptr);*/
        chptr += len;
        nbytes += len;
	  }
#endif

#ifdef USE_V1190_HIDE
	  if(ntdcs>0)
	  {
vmeBusLock();
        len = tdc1190UploadAll(chptr, 10000);
vmeBusUnlock();
        /*printf("\nTDC len=%d\n",len);
        printf("%s\n",chptr);*/
        chptr += len;
        nbytes += len;
	  }
#endif

#ifdef USE_DSC2
	  if(ndsc2>0)
	  {
vmeBusLock();
        len = dsc2UploadAll(chptr, 10000);
vmeBusUnlock();
        /*printf("\nDSC2 len=%d\n",len);
        printf("%s\n",chptr);*/
        chptr += len;
        nbytes += len;
	  }
#endif

#ifdef USE_DCRB
	  if(ndcrb>0)
	  {
vmeBusLock();
        len = dcrbUploadAll(chptr, 10000);
vmeBusUnlock();
        /*printf("\nDCRB len=%d\n",len);
        printf("%s\n",chptr);*/
        chptr += len;
        nbytes += len;
	  }
#endif

#ifdef USE_VSCM
	  if(nvscm1>0)
	  {
vmeBusLock();
        len = vscmUploadAll(chptr, 65535);
vmeBusUnlock();
        /*printf("\nVSCM len=%d\n",len);
        printf("%s\n",chptr);*/
        chptr += len;
        nbytes += len;
	  }
#endif


#ifdef USE_SSP
      if(nssp>0)
      {
vmeBusLock();
        len = sspUploadAll(chptr, 300000);
vmeBusUnlock();
        /*printf("\nSSP len=%d\n",len);
        printf("%s\n",chptr);*/
        chptr += len;
        nbytes += len;
	  }
#endif


#if 0
	  /* temporary for crates with GTP */
      if(rol->pid==37||rol->pid==39)
	  {
#define TEXT_STR  1000
        char *roc;
        int  ii, kk, stt = 0;
        char result[TEXT_STR];      /* string for messages from tcpClientCmd */
        char exename[200];          /* command to be executed by tcpServer */

        if(rol->pid==37) roc = "hps1gtp";
        else             roc = "hps2gtp";

        sprintf(exename,"gtpUploadAllPrint()");

        /*printf("gtptest1: roc >%s< exename >%s<\n",roc,exename);*/

        memset(result,0,TEXT_STR);
        tcpClientCmd(roc, exename, result);

        len = strlen(result) - 2; /* 'result' has 2 extra chars in the end we do not want ????? */
        /*printf("gtptest1: len=%d, >%s<",len,result);*/

        strncpy(chptr,result,len);
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

	  /*ADD PADDING AND \4 HERE !!!!!!!!!!!!*/

      nwords = nbytes/4;
      rol->dabufp += nwords;

      BANKCLOSE;

      printf("SYNC: read boards configurations - done\n");
    }












	


    /* read scaler(s) */
    if(syncFlag==1 || EVENT_NUMBER==1)
    {
      printf("SYNC: read scalers\n");

#ifdef USE_DSC2
	  /*printf("ndsc2_daq=%d\n",ndsc2_daq);*/
	  if(ndsc2_daq>0)
	  {
        BANKOPEN(0xe115,1,rol->pid);
        for(jj=0; jj<ndsc2_daq; jj++)
        {
          slot = dsc2Slot_daq(jj);
vmeBusLock();
          /* in following argument 4 set to 0xFF means latch and read everything, 0x3F - do not latch and read everything */
          nwords = dsc2ReadScalers(slot, tdcbuf, 0x10000, 0xFF, 1);
          /*printf("nwords=%d, nwords = 0x%08x 0x%08x 0x%08x 0x%08x\n",nwords,tdcbuf[0],tdcbuf[1],tdcbuf[2],tdcbuf[3]);*/
vmeBusUnlock();

#ifdef SSIPC
/*
	      {
            int status, mm;
            unsigned int dd[72];
            for(mm=0; mm<72; mm++) dd[mm] = tdcbuf[mm];
            status = epics_msg_send("hallb_dsc2_hps2_slot2","uint",72,dd);
	      }
*/
#endif
          /* unlike other boards, dcs2 scaler readout already swapped in 'dsc2ReadScalers', so swap it back, because
          rol2.c expects big-endian format*/
          for(ii=0; ii<nwords; ii++) *rol->dabufp ++ = LSWAP(tdcbuf[ii]);
        }
        BANKCLOSE;
	  }

#endif
      printf("SYNC: read scalers - done\n");
	}


#ifndef TI_SLAVE
    /* print livetime */
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

  
  nusertrig ++;
  
  //printf("usrtrig called %d times\n",nusertrig);fflush(stdout);
  
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
  
  ndone ++;
  //printf("_done called %d times\n",ndone);fflush(stdout);
  
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
fadc1_dummy()
{
  return;
}

#endif
