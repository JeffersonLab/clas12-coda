
/* fadc1.c */

#if defined(VXWORKS) || defined(Linux_vme)

#define NEW


/* HPS firmware:
Board Firmware Rev/ID = 0x02c1 : ADC Processing Rev = 0x0a03

TI init:
tcpClient adcecal1 'tiInit(0xa80000,3,0)'
coda_roc_gef -s clasprod -o "adcecal1 ROC" -i
*/


#ifndef VXWORKS
#undef DMA_TO_BIGBUF /*if want to dma directly to the big buffers*/
#endif

/* fadc1.c - first readout list for VXS crates with FADC250 and new TI */

#define USE_FADC250

/* if ADC1182 is used, do not use FADC250 */
#ifdef USE_ADC1182
#undef USE_FADC250
#undef DMA_TO_BIGBUF
#endif


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

#include "circbuf.h"

/* from fputil.h */
#define SYNC_FLAG 0x20000000

/* readout list name */
#define ROL_NAME__ "FADC1"

/* polling mode if needed */
#define POLLING_MODE

/* main TI board */
#define TI_ADDR   (21<<19)  /* if 0 - default will be used, assuming slot 21*/




#ifdef USE_ADC1182

#define INIT_NAME fadc1_1182__init
#define TI_READOUT TI_READOUT_EXT_POLL /* Poll for available data, front panel triggers */

#else

/* name used by loader */
#ifdef TI_MASTER
#define INIT_NAME fadc1_master__init
#define TI_READOUT TI_READOUT_EXT_POLL /* Poll for available data, front panel triggers */
#else
#ifdef TI_SLAVE
#define INIT_NAME fadc1_slave__init
#define TI_READOUT TI_READOUT_TS_POLL /* Poll for available data, triggers from master over fiber */
#else
#define INIT_NAME fadc1__init
#define TI_READOUT TI_READOUT_EXT_POLL /* Poll for available data, front panel triggers */
#endif
#endif

#endif




#include "rol.h"

void usrtrig(unsigned int EVTYPE, unsigned int EVSOURCE);
void usrtrig_done();

#include "TIPRIMARY_source.h"



/* user code */

#include "uthbook.h"
#include "coda.h"
#include "tt.h"
#include "scaler7201.h"
#include "adc792.h"
#include "tdc1190.h"

static char rcname[5];

#define NBOARDS 22    /* maximum number of VME boards: we have 21 boards, but numbering starts from 1 */
#define MY_MAX_EVENT_LENGTH 3000/*3200*/ /* max words per board */
static unsigned int *tdcbuf;





#ifdef USE_ADC1182
#define NADCS 1
static int nadcs;
static unsigned long adcadr[4] = {0x180000, 0, 0, 0};
#endif


/*#ifdef DMA_TO_BIGBUF*/
/* must be 'rol' members, like dabufp */
extern unsigned int dabufp_usermembase;
extern unsigned int dabufp_physmembase;
/*#endif*/

extern int rocMask; /* defined in roc_component.c */

#define NTICKS 1000 /* the number of ticks per second */
/*temporary here: for time profiling */




#ifndef VXWORKS

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

void
titest1()
{
  if(TIp==NULL) {printf("NULL\n");return;}
  printf("0x%08x(%d) 0x%08x(%d)\n",
		 vmeRead32(&TIp->fiberLatencyMeasurement),vmeRead32(&TIp->fiberLatencyMeasurement),
		 vmeRead32(&TIp->fiberAlignment),vmeRead32(&TIp->fiberAlignment));
}



#ifdef USE_FADC250

/*ctp cluster coincidence window*/
static unsigned int cluster_window = 1; /*0: 4 ns, 1: 8ns, ..., max 32ns - coincidence window on FADC hits */

/*FADC trigger reporting sum scale factor*/
static unsigned int fa_sumScaleFactor = 7; /*lsb will be: 0-20fC, 1-40fC, 2-80, 3-160, ...: range is 5 bits (0-31); Scale Factor for the total sum reported to the CTP i.e. Sum/(2^sumScaleFactor) (<15)*/


void
changeClusterWindow()
{
  cluster_window += 1;
  if(cluster_window>7) cluster_window=0;
  printf("set CTP cluster_window to %d (%d ns)\n",cluster_window,(cluster_window+1)*4);
}

void
changeScaleFactor()
{
  fa_sumScaleFactor += 1;
  if(fa_sumScaleFactor>15) fa_sumScaleFactor=0;
  printf("set FADC trigger reporting sum scale factor to %d (%d fC)\n",fa_sumScaleFactor,(fa_sumScaleFactor+1)*20);
}

#include "fadcLib.h"

#define DIST_ADDR  0xEA00	  /*  base address of FADC signal distribution board  (A16)  */
#define FADC_THRESHOLD       10  /* Threshold for data readout */


unsigned int fadcSlotMask   = 0;    /* bit=slot (starting from 0) */
static int nfadc;                 /* Number of FADC250s verified with the library */
static int NFADC;                   /* The Maximum number of tries the library will
                                     * use before giving up finding FADC250s */
static int FA_SLOT;                 /* We'll use this over and over again to provide
				                     * us access to the current FADC slot number */ 

static int FADC_ROFLAG           = 1;  /* 0-noDMA, 1-board-by-board DMA, 2-chainedDMA */

/* for the calculation of maximum data words in the block transfer */
static unsigned int MAXFADCWORDS = 0;
static unsigned int MAXTIWORDS  = 0;


/* IC lookup tables */

/*tdc's in slots 19 and 20*/
int ic_tdc_high[2][128] = {
0*256
};

int ic_tdc_low[2][128] = {
0*256
};



/*adc's in slots 3-10 and 13-18*/

unsigned int ic_adc_ped[21][16];
unsigned short ic_adc_thres[21][16];
unsigned int offset;


int
abcReadPeds(int rocid)
{
  FILE *fd = NULL;
  char *dir = NULL;
  char *expid = NULL;
  char fname[1024];
  int slot, chan, ped, status;
  float sigma;


  printf("Opening pedestal file\n");
  if((dir=getenv("CLAS")) == NULL)
  {
    printf("ERROR: environment variable CLAS is not defined - exit\n");
    return(-1);
  }
  if((expid=getenv("EXPID")) == NULL)
  {
    printf("ERROR: environment variable EXPID is not defined - exit\n");
    return(-1);
  }
  sprintf(fname,"%s/parms/peds/%s/fadc%02d.ped",dir,expid,rocid);
  fd = fopen(fname,"r");
  if(fd==NULL)
  {
    printf("ttfa: ERROR: cannot open pedestal file >%s<\n",fname);
  }
  else
  {
    printf("ttfa: pedestal file >%s< is opened for writing\n",fname);


    for(slot=0; slot<19; slot++)
    {
      for(chan=0; chan<16; chan++)
      {
        ic_adc_ped[slot][chan] = 0;
        ic_adc_thres[slot][chan] = 0;
      }
    }

    while( (status=fscanf(fd,"%d %d %d %f %d\n",&slot,&chan,&ped,&sigma,&offset)) > 0 )
    {
      printf("status=%d -> slot=%2d chan=%2d ped=%5d sigma=%6.3f\n",status,slot,chan,ped,sigma);
      if(slot>=0&&slot<19&&chan>=0&&chan<16)
      {
        /* set trigger threshold */
        ic_adc_ped[slot][chan] = ped + offset;

        /* set readout threshold as pedestal + some offset */
        ic_adc_thres[slot][chan] = ped + 6;
      }
      else printf("bad slot=%d or chan=%d\n",slot,chan);
    }

    if(status==EOF) printf("EOF reached\n");
    else            printf("fscanf() returned error %d\n",status);

    fclose(fd);
    printf("ttfa: pedestal file >%s< is closed\n",fname);
  }

  return(0);
}

#endif

static void
__download()
{
  int i1, i2, i3;
#ifdef USE_FADC250

  int id, isl, ichan;
  unsigned short iflag;
  int fadc_mode = 1, iFlag = 0;
  int ich, NSA, NSB;

  /*ctp params*/
  unsigned int cluster_threshold = 0; /* 8 bits: (MeV) cluster energy sum threshold */

  /*ssp params*/
  unsigned int ssp_addr_a24 = 0xBE0000;

  unsigned int coincidence_window = 15; /*4: +/-16ns time window for cluster pairs */

  unsigned int energy_sum_max = 255; /*255: no cut (cut on pair)*/
  unsigned int pair_energy_difference_max = 255; /*255: no cut*/
  unsigned int pair_energy_difference_threshold = 255; /*255: no cut*/
  unsigned int energy_max = 255; /*255: no cut (cut on individual cluster)*/
  unsigned int energy_min = 10/*10*/; /*not cut (cut on individual cluster)*/
  unsigned int cluster_delay = 128; /*128: 512ns delay critical parameter for internal algorithm to work*/
  unsigned int trigger_latency = 511; /*2us delay for sync->trigger out of SSP...real latency to FADC trigger input will be about 3.2us*/

  /*
  0:    will trigger on a cluster pair
  0x10: will trigger on a cluster found in top half of calorimeter
  0x20: will trigger on a cluster found in bottom half of calorimeter
  0x30: will trigger on a cluster found in top OR bottom half of calorimeter
  */
  unsigned int trigger_cut_enable_mask = 0x30;


#endif
#ifdef POLLING_MODE
  rol->poll = 1;
#else
  rol->poll = 0;
#endif

  printf("\n>>>>>>>>>>>>>>> ROCID=%d, CLASSID=%d <<<<<<<<<<<<<<<<\n",rol->pid,rol->classid);
  printf("CONFFILE >%s<\n\n",rol->confFile);

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



  /*************************************/
  /* redefine TI settings if neseccary */

  /* tiInit() does nothing for busy, tiConfig() sets fiber, we set the rest */
#ifndef TI_SLAVE
  tiSetBusySource(TI_BUSY_LOOPBACK,0);
#endif
  tiSetBusySource(TI_BUSY_SWB,0);




/*tiAddRocSWA();*/
tiRemoveRocSWA();



printf("***4***\n");
tiStatus(1); /* Ben & William Testing */


  /* for timing measurements in FADC250s */
  tiSetTriggerHoldoff(1,5,0);   /* No more than 1 trigger within 80 ns */
printf("***5***\n");
tiStatus(1); /* Ben & William Testing */
  tiSetTriggerHoldoff(4,41,0);  /* No more than 4 triggers within 656 ns */
printf("***6***\n");
tiStatus(1); /* Ben & William Testing */

  /* Allow set/clear control of sync reset - for s/w pulse control */
  tiSetUserSyncResetReceive(1);
printf("***7***\n");
tiStatus(1); /* Ben & William Testing */





  usrVmeDmaSetConfig(2,5,1); /*A32,2eSST,267MB/s*/
  /*usrVmeDmaSetConfig(2,5,0);*/ /*A32,2eSST,160MB/s*/
  /*usrVmeDmaSetConfig(2,3,0);*/ /*A32,MBLT*/

  tdcbuf = (unsigned int *)i2_from_rol1;



  /******************/
  /* USER code here */


#ifdef USE_ADC1182
  nadcs = adc1182init(adcadr, NADCS);
#endif

#ifdef USE_FADC250


  /*HPS stuff
#ifdef TI_SLAVE
  abcReadPeds(2);
#else
  abcReadPeds(1);
#endif
  */
  abcReadPeds(rol->pid);


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
  /* Trigger Source */
  iFlag |= (1<<2);    /* VXS */
  /* Clock Source */
  /*iFlag |= (1<<5);*/    /* VXS */
  iFlag |= (0<<5);  /* Internal Clock Source */
#ifndef VXWORKS
  vmeSetQuietFlag(1); /* skip the errors associated with BUS Errors */
#endif
  faInit((unsigned int)(3<<19),(1<<19),NFADC,iFlag); /* start from 0x00180000, increment 0x00080000 */

  nfadc = faGetNfadc(); /* acual number of FADC boards found  */
#ifndef VXWORKS
  vmeSetQuietFlag(0); /* Turn the error statements back on */
#endif

  /* Calculate the maximum number of words per block transfer (assuming Pulse mode)
   *   MAX = NFADC * block_level * (EvHeader + TrigTime*2 + Pulse*2*chan) 
   *         + 2*32 (words for byte alignment) 
   */
  MAXFADCWORDS = NFADC * block_level * (1+2+100/*FADC_WINDOW_WIDTH*/*16) + 2*32;
  
  printf("**************************************************\n");
  printf("* Calculated MAX FADC words per block = %d\n",MAXFADCWORDS);
  printf("**************************************************\n");

  /* Check these numbers, compared to our buffer size.. */
  if( (MAXFADCWORDS+MAXTIWORDS)*4 > MAX_EVENT_LENGTH )
  {
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf(" WARNING.  Event buffer size is smaller than the expected data size\n");
    printf("     Increase the size of MAX_EVENT_LENGTH and recompile!\n");
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  }


  if(nfadc==1) FADC_ROFLAG = 1; /*no chainedDMA if one board only*/

  if(FADC_ROFLAG==2) faEnableMultiBlock(1);
  else faDisableMultiBlock();

  /* configure all modules based on config file */
  fadc250Config("");

  /* Additional Configuration for each module */
  fadcSlotMask=0;
  for(id=0; id<nfadc; id++) 
  {
    FA_SLOT = faSlot(id);      /* Grab the current module's slot number */
    fadcSlotMask |= (1<<FA_SLOT); /* Add it to the mask */
	printf("=======================> fadcSlotMask=0x%08x\n",fadcSlotMask);

	{
      unsigned int PL, PTW, NSB, NSA, NP;
      faGetProcMode(FA_SLOT, &fadc_mode, &PL, &PTW, &NSB, &NSA, &NP);
      printf("slot %d, fadc_mode=%d\n",FA_SLOT,fadc_mode);
	}
    if(fadc_mode==1)
    {
      /* Set the threshold for data readout */
      faSetThreshold(FA_SLOT,FADC_THRESHOLD,0);

	  /* do this for zero-suppressed runs using raw mode 
      faSetThresholdAll(FA_SLOT,ic_adc_thres[FA_SLOT]);
*/
    }
    else
    {
      /*fadc_mode=2-pulse modes, fadc_mode=3-integral mode*/
      faSetThresholdAll(FA_SLOT,ic_adc_thres[FA_SLOT]);
    }

    /* Bus errors to terminate block transfers (preferred) */
    faEnableBusError(FA_SLOT);

    /* Set the Block level */
    faSetBlockLevel(FA_SLOT, block_level);



#ifdef NEW
    /*****************/
    /*trigger-related*/
    faResetMGT(FA_SLOT,1);

    /*faSetThreshold(FA_SLOT,400,0xFFFF);*/

    NSA = faGetNSA(FA_SLOT);
    NSB = faGetNSB(FA_SLOT);
    printf("slot %d -> nsa=%d nsb=%d\n",FA_SLOT,NSA,NSB);

    for(ichan=0; ichan<16; ichan++)
    {

      /* Set the individual channel pedestals for the data that is sent to the CTP */
      /* pedestal is for trigger chain only, must be integral, so we'll multiply
		 value from the file by (NSA+NSB) */
	  faSetChannelPedestal(FA_SLOT,ichan, ic_adc_ped[FA_SLOT][ichan]*(NSA+NSB) );
      printf("  slot=%2d chan=%2d ped=%5d\n",FA_SLOT,ichan,faGetChannelPedestal(FA_SLOT, ichan));

      faSetChannelGain(FA_SLOT,ichan,0.5); /* MUST BE IN CONFIG FILE */
    }
#endif


  }


  /* read back and print trigger pedestals */
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
  sdInit(1); /* Initialize the SD library; 1-ignore firmware version */
  sdSetActiveVmeSlots(fadcSlotMask); /* Use the fadcSlotMask to configure the SD */
  sdStatus();


#endif



  logMsg("INFO: User Download Executed\n",1,2,3,4,5,6);
}

static void
__prestart()
{
  int ii, i1, i2, i3;
#ifdef USE_FADC250
  int id, isl, ichan;
  unsigned short iflag;
  int iFlag = 0;
  int ich;
  unsigned short aa = 0;
  unsigned short bb;
  unsigned short thr = 400;
#endif

  /* Clear some global variables etc for a clean start */
  *(rol->nevents) = 0;

  tiEnableVXSSignals();

#ifdef POLLING_MODE
  CTRIGRSS(TIPRIMARY, TIR_SOURCE, usrtrig, usrtrig_done);
#else
  CTRIGRSA(TIPRIMARY, TIR_SOURCE, usrtrig, usrtrig_done);
#endif

  sprintf(rcname,"RC%02d",rol->pid);
  printf("rcname >%4.4s<\n",rcname);


#ifdef USE_FADC250

  /* Program/Init VME Modules Here */
  for(id=0; id<nfadc; id++)
  {
    FA_SLOT = faSlot(id);
    faSetClockSource(FA_SLOT,2);
  }

  sleep(1);

  for(id=0; id<nfadc; id++)
  {
    FA_SLOT = faSlot(id);
    faSoftReset(FA_SLOT,0); /*0-soft reset, 1-soft clear*/

#ifdef NEW
    if(!faGetMGTChannelStatus(FA_SLOT))
    {
      faResetMGT(FA_SLOT,1);
      faResetMGT(FA_SLOT,0);
    }
#endif

    faResetToken(FA_SLOT);
    faResetTriggerCount(FA_SLOT);
    faStatus(FA_SLOT,0);
    faPrintThreshold(FA_SLOT);
  }

  tiStatus(1);

  /*  Enable FADC */
  for(id=0; id<nfadc; id++) 
  {
    FA_SLOT = faSlot(id);
    /*faSetMGTTestMode(FA_SLOT,0);*/
    faSetBlockLevel(FA_SLOT,block_level); /* done above !!??*/
    /*faChanDisable(FA_SLOT,0xffff);enabled in download*/
    faEnable(FA_SLOT,0,0);
  }


  sleep(2);

#endif

  /* USER code here */
  /******************/

printf("\n\n***111***\n");
tiStatus(1);
sleep(1);
printf("\n\n***112***\n");
tiStatus(1);

  tiIntDisable();

printf("\n\n***113***\n");
tiStatus(1);
sleep(1);
printf("\n\n***114***\n");
tiStatus(1);

  /* master and standalone crates, NOT slave */
#ifndef TI_SLAVE

printf("\n\n***11***\n");
tiStatus(1);
  sleep(5);
printf("\n\n***12***\n");
tiStatus(1);
  tiSyncReset(1);
  sleep(5);


printf("\n\n***13***\n");
tiStatus(1);
  sleep(1);
printf("\n\n***14***\n");
tiStatus(1);
  tiSyncReset(1);
printf("\n\n***15***\n");
tiStatus(1);
  sleep(1);
printf("\n\n***16***\n");
tiStatus(1);


  if(tiGetSyncResetRequest())
  {
    printf("ERROR: syncrequest still ON after tiSyncReset(); trying again\n");
    sleep(1);
    tiSyncReset(1);
    sleep(1);
  }

printf("\n\n***17***\n");
tiStatus(1);

  if(tiGetSyncResetRequest())
  {
    printf("ERROR: syncrequest still ON after tiSyncReset(); try 'tcpClient <rocname> tiSyncReset'\n");
  }
  else
  {
    printf("INFO: syncrequest is OFF now\n");
  }
printf("\n\n***18***\n");
tiStatus(1);

  printf("holdoff rule 1 set to %d\n",tiGetTriggerHoldoff(1));
  printf("holdoff rule 2 set to %d\n",tiGetTriggerHoldoff(2));
printf("\n\n***19***\n");
tiStatus(1);

#endif


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

  CDODISABLE(TIPRIMARY,TIR_SOURCE,0);

  /* Before disconnecting... wait for blocks to be emptied */
  blocksLeft = tiBReady();
  if(blocksLeft)
  {
    printf("... end:  Blocks left on the TI (%d)\n",blocksLeft);
    while(iwait < 100)
	{
	  if(blocksLeft <= 0) break;
	  blocksLeft = tiBReady();
	  iwait++;
	}
    printf("... end:  Blocks left on the TI (%d)\n",blocksLeft);
  }


#ifdef USE_FADC250
  /* FADC Disable */
  for(id=0;id<nfadc;id++) 
  {
    FA_SLOT = faSlot(id);
    faDisable(FA_SLOT,0);
    faStatus(FA_SLOT,0);
  }
  sdStatus();
#endif

  tiStatus(1);

  logMsg("INFO: User End Executed\n",1,2,3,4,5,6);

  return;
}

static void
__pause()
{
  CDODISABLE(TIPRIMARY,TIR_SOURCE,0);
  logMsg("INFO: Pause Executed\n",1,2,3,4,5,6);
  
} /*end pause */

static void
__go()
{
  int jj, id;

  logMsg("INFO: Entering Go 1\n",1,2,3,4,5,6);

#ifdef USE_FADC250


  /*  Enable FADC - old place
  for(id=0; id<nfadc; id++) 
  {
    FA_SLOT = faSlot(id);
    faChanDisable(FA_SLOT,0x0);
  }
  sleep(1);
  */

  
  /*  Send Sync Reset to FADC */
  /*faSDC_Sync();*/
#endif

#ifdef USE_ADC1182
  /* adc1182 reset */
  for(jj=0; jj<nadcs; jj++) adc1182reset(jj);
#endif

  CDOENABLE(TIPRIMARY,TIR_SOURCE,0); /* bryan has (,1,1) ... */

  logMsg("INFO: Go 1 Executed\n",1,2,3,4,5,6);
}

#define NSSP 8

void
usrtrig(unsigned int EVTYPE, unsigned int EVSOURCE)
{
  int *jw, ind, ind2, ii, jj, jjj, blen, len, nwords, dCnt, rlen, itdcbuf, nbytes;
  unsigned int *tdcbuf_save, *tdc;
  unsigned int *dabufp1, *dabufp2, sspbuf[NSSP];
#ifndef VXWORKS
  TIMERL_VAR;
#endif
#ifdef USE_FADC250
  unsigned int datascan, mask;
  int njjloops;
  unsigned short *dabufp16, *dabufp16_save;
  int id;
  int idata;
  int stat, itime, gbready;
#endif
#ifdef DMA_TO_BIGBUF
    unsigned int pMemBase, uMemBase, mSize;
#endif

  /*printf("EVTYPE=%d syncFlag=%d\n",EVTYPE,syncFlag);*/

  rol->dabufp = (int *) 0;

  CEOPEN(EVTYPE, BT_BANKS);

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


#ifndef VXWORKS
TIMERL_START;
#endif

    /* for EVIO format, will dump raw data */
    tdcbuf_save = tdcbuf;

    /*************/
	/* TI stuff */

    /* Set high, the first output port */
    tiSetOutputPort(1,0,0,0);

    /* Grab the data from the TI */
    len = tiReadBlock(tdcbuf,900>>2,1);
    if(len<=0)
    {
      /*printf("TIreadout : No data or error, len = %d\n",len)*/;
    }
    else
    {
	  ;
	  /*
      printf("ti: len=%d\n",len);
      for(jj=0; jj<len; jj++) printf("ti[%2d] 0x%08x\n",jj,LSWAP(tdcbuf[jj]));
	  */
      /* *rol->dabufp++ = LSWAP(tdcbuf[jj]);*/

    }

    /* Turn off all output ports */
    tiSetOutputPort(0,0,0,0);

	/* TI stuff */
    /*************/

#ifdef DEBUG
    printf("fadc1: start fadc processing\n");fflush(stdout);
#endif












#ifdef USE_FADC250

    /* Configure Block Type... temp fix for 2eSST trouble with token passing */
    tdcbuf = tdcbuf_save;
    dCnt=0;
    if(nfadc != 0)
    {

#ifndef VXWORKS
TIMERL_START;
#endif

/*COMMENT OUT FOLLOWING 'FOR' LOOP FOR SPEED UP !!!*/
      for(itime=0; itime<100000; itime++) 
	  {
	    gbready = faGBready();
	    stat = (gbready == fadcSlotMask);
	    if (stat>0) 
	    {
	      break;
	    }
	  }









goto a234;
#ifdef TI_MASTER
	nwords = sspReadFifo(sspbuf);	
    sspbuf[7] = time(0);
	/*
    printf("sspbuf[%2d]: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
      nwords,sspbuf[0],sspbuf[1],sspbuf[2],sspbuf[3],sspbuf[4],sspbuf[5],sspbuf[6]);
	*/
#endif
a234:



      if(stat>0)
	  {
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
 

goto a345;
#ifdef TI_MASTER
          for(jjj=0; jjj<NSSP; jjj++) *rol->dabufp++ = sspbuf[jjj];
#endif
a345:
 
 /*25us->*/
 	      dCnt = faReadBlock(FA_SLOT,rol->dabufp,500000/*MAXFADCWORDS*/,FADC_ROFLAG);
          rol->dabufp += dCnt;
 /*->25us*/
		  /*  
 		  printf("dCnt=%d, data: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",dCnt,
 		  rol->dabufp[0],rol->dabufp[1],rol->dabufp[2],rol->dabufp[3],rol->dabufp[4],rol->dabufp[5]);
 		  */
          usrRestoreVmeDmaMemory();
          usrVmeDmaMemory(&pMemBase, &uMemBase, &mSize);
 	      /*printf("restored: 0x%08x 0x%08x 0x%08x\n",pMemBase,uMemBase,mSize);*/
#else

#ifdef DEBUG
          printf("fadc1: Starting DMA\n");fflush(stdout);
#endif

	      dCnt = faReadBlock(FA_SLOT,tdcbuf,500000/*MAXFADCWORDS*/,FADC_ROFLAG);

#ifdef DEBUG
          printf("fadc1: Finished DMA, dCnt=%d\n",dCnt);fflush(stdout);
#endif

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

goto a3456;
#ifdef TI_MASTER
            /* report ssp data before first fadc board */
            if(jj==0) for(jjj=0; jjj<NSSP; jjj++) *rol->dabufp++ = sspbuf[jjj];
#endif
a3456:

	        len = faReadBlock(faSlot(jj),rol->dabufp,10000/*MAXFADCWORDS*/,FADC_ROFLAG);
            rol->dabufp += len;
            dCnt += len;

            usrRestoreVmeDmaMemory();
#else
	        len = faReadBlock(faSlot(jj),&tdcbuf[dCnt],10000/*MAXFADCWORDS*/,FADC_ROFLAG);
            dCnt += len;
#endif
#ifdef DEBUG
            printf("fadc1: [%d] len=%d dCnt=%d\n",jj,len,dCnt);
#endif
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

goto a4321;
#ifdef TI_MASTER
          for(jjj=0; jjj<NSSP; jjj++) *rol->dabufp++ = sspbuf[jjj];
#endif
a4321:

          for(jj=0; jj<dCnt; jj++) *rol->dabufp++ = tdcbuf[jj];
#endif
	    }
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
        printf("\n");
	  }

      /* Reset the Token */
      if(FADC_ROFLAG==2)
	  {
/*2us->*/
	    for(id=0; id<nfadc; id++)
	    {
	      FA_SLOT = faSlot(id);
	      faResetToken(FA_SLOT);
	    }
/*->2us*/
	  }

#ifndef VXWORKS
TIMERL_STOP(100000/block_level,1000+rol->pid);
#endif

    }

#endif /*FADC250 */







#ifdef USE_ADC1182

    /* wait for all ADCs to finish conversion */
    for(ii=0; ii<nadcs; ii++) while( adc1182cip(ii) );
    dCnt = 0;
    for(ii=0; ii<nadcs; ii++)
    {
      for(jj=0; jj<8; jj++)
      {
        tdcbuf[dCnt++] = adc1182read(ii, jj);
      }
	}
    for(jj=0; jj<dCnt; jj++) *rol->dabufp++ = tdcbuf[jj];

    /* reset all ADC boards */
    for(ii=0; ii<nadcs; ii++) adc1182reset(ii);

#endif

#ifndef VXWORKS
TIMERL_STOP(1000,0);
#endif

#if 0
    /* for physics sync event, make sure all board buffers are empty */
    if(syncFlag==1)
    {
      int scan_flag = 0;
      unsigned short slot, nev16;

	  /*
	  if(scan_flag) logMsg("SYNC: scan_flag=0x%08x\n",scan_flag,2,3,4,5,6);
	  */
      ind2 = bosMopen_(jw,"PTRN",rol->pid,1,1);
      jw[ind2+1] = SYNC_FLAG + scan_flag;
      rol->dabufp += bosMclose_(jw,ind2,1,1);
    }
#endif


  }

  /* close event */
  CECLOSE;

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
