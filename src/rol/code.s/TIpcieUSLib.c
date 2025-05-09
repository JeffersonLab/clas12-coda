/*----------------------------------------------------------------------------*/
/**
 * @mainpage
 * <pre>
 *  Copyright 2022, Jefferson Science Associates, LLC.
 *  Subject to the terms in the LICENSE file found in the top-level directory.
 *
 *    Authors: Bryan Moffit
 *             moffit@jlab.org                   Jefferson Lab, MS-12B3
 *             Phone: (757) 269-5660             12000 Jefferson Ave.
 *             Fax:   (757) 269-5800             Newport News, VA 23606
 *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *     Primitive trigger control for Intel CPUs running Linux using the TJNAF
 *     Trigger Interface (TI) PCIexpress card Ultrascale+ version.
 *
 * </pre>
 *----------------------------------------------------------------------------*/

#define _GNU_SOURCE

/* #define ALLOCMEM */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stdint.h>
#include "TIpcieUSLib.h"

/* Mutex to guard TI read/writes */
pthread_mutex_t   tipcieusMutex = PTHREAD_MUTEX_INITIALIZER;
#define TIPUSLOCK     if(pthread_mutex_lock(&tipcieusMutex)<0) perror("pthread_mutex_lock");
#define TIPUSUNLOCK   if(pthread_mutex_unlock(&tipcieusMutex)<0) perror("pthread_mutex_unlock");

/* Global Variables */
volatile struct TIPCIEUS_RegStruct  *TIPUSp=NULL;    /* pointer to TI memory map */
volatile unsigned int *TIPUSpd=NULL;             /* pointer to TI DMA memory */
volatile unsigned int *TIPUSpj=NULL;             /* pointer to TI JTAG memory */
static unsigned long tipusDmaAddrBase=0;
static void          *tipusMappedBase;
static void          *tipusJTAGMappedBase;

static int tipusUseDma=0;
int tipusMaster=1;                               /* Whether or not this TIPUS is the Master */
int tipusCrateID=0x59;                           /* Crate ID */
int tipusBlockLevel=0;                           /* Current Block level for TIPUS */
int tipusNextBlockLevel=0;                       /* Next Block level for TIPUS */
int tipusBlockBufferLevel=0;                     /**< Current Block Buffer level for TIPUS */
unsigned int        tipusIntCount    = 0;
unsigned int        tipusAckCount    = 0;
unsigned int        tipusDaqCount    = 0;       /* Block count from previous update (in daqStatus) */
unsigned int        tipusReadoutMode = 0;
unsigned int        tipusTriggerSource = 0;     /* Set with tipusSetTriggerSource(...) */
unsigned int        tipusSlaveMask   = 0;       /* TIPUS Slaves (mask) to be used with TIPUS Master */
int                 tipusDoAck       = 0;
int                 tipusNeedAck     = 0;
static BOOL         tipusIntRunning  = FALSE;   /* running flag */
static VOIDFUNCPTR  tipusIntRoutine  = NULL;    /* user intererrupt service routine */
static int          tipusIntArg      = 0;       /* arg to user routine */
static unsigned int tipusIntLevel    = TIPUS_INT_LEVEL;       /* VME Interrupt level */
static unsigned int tipusIntVec      = TIPUS_INT_VEC;  /* default interrupt vector */
static VOIDFUNCPTR  tipusAckRoutine  = NULL;    /* user trigger acknowledge routine */
static int          tipusAckArg      = 0;       /* arg to user trigger ack routine */
static int          tipusReadoutEnabled = 1;    /* Readout enabled, by default */
static int          tipusFiberLatencyMeasurement = 0; /* Measured fiber latency */
static int          tipusSyncEventFlag = 0;     /* Sync Event/Block Flag */
static int          tipusSyncEventReceived = 0; /* Indicates reception of sync event */
static int          tipusBlockSyncFlag = 0;     /* Sync Event Flag in Trigger Block previous readout */
static int          tipusNReadoutEvents = 0;    /* Number of events to readout from crate modules */
static int          tipusDoSyncResetRequest =0; /* Option to request a sync reset during readout ack */
static int          tipusSlaveFiberIn=1;        /* Which Fiber port to use when in Slave mode *//*sergey*/
static int          tipusFakeTriggerBank=1;
static int          tipusSyncResetType=TIPUS_SYNCCOMMAND_SYNCRESET_4US;  /* Set default SyncReset Type to Fixed 4 us */
static int          tipusVersion     = 0x0;     /* Firmware version */
int                 tipusFiberLatencyOffset = 0xbf; /* Default offset for fiber latency */
static int          tipusDoIntPolling= 1;       /* Decision to use library polling thread routine */
static int tipusFD;
/*sergey*/
static int tipusClockOutputMode = 0; /*software setting is disabled by default*/
/*sergey*/

static unsigned int tipusTrigPatternData[16]=   /* Default Trigger Table to be loaded */
  { /* TS#1,2,3,4,5,6 generates Trigger1 (physics trigger),
       No Trigger2 (playback trigger),
       No SyncEvent;
    */
    0x43424100, 0x47464544, 0x4b4a4948, 0x4f4e4d4c,
    0x53525150, 0x57565554, 0x5b5a5958, 0x5f5e5d5c,
    0x63626160, 0x67666564, 0x6b6a6968, 0x6f6e6d6c,
    0x73727170, 0x77767574, 0x7b7a7978, 0x7f7e7d7c,
  };

/* Pointer to local storage of TI block data */
static unsigned int *tipusBlockData = NULL;
static int tipusBlockDataSize = 0;

/* Polling routine prototypes (static) */
static void tipusPoll(void);
static void tipusStartPollingThread(void);
/* polling thread pthread and pthread_attr */
pthread_attr_t tipuspollthread_attr;
pthread_t      tipuspollthread=0;

static int FiberMeas();

/**
 * @defgroup PreInit Pre-Initialization
 * @defgroup SlavePreInit Slave Pre-Initialization
 *   @ingroup PreInit
 * @defgroup Config Initialization/Configuration
 * @defgroup MasterConfig Master Configuration
 *   @ingroup Config
 * @defgroup SlaveConfig Slave Configuration
 *   @ingroup Config
 * @defgroup Status Status
 * @defgroup MasterStatus Master Status
 *   @ingroup Status
 * @defgroup Readout Data Readout
 * @defgroup MasterReadout Master Data Readout
 *   @ingroup Readout
 * @defgroup IntPoll Interrupt/Polling
 * @defgroup Deprec Deprecated - To be removed
 */

/**
 * @ingroup PreInit
 * @brief Set the Fiber Latency Offset to be used during initialization
 *
 * @param flo fiber latency offset
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusSetFiberLatencyOffset_preInit(int flo)
{
  if((flo<0) || (flo>0x1ff))
    {
      printf("%s: ERROR: Invalid Fiber Latency Offset (%d)\n",
	     __FUNCTION__,flo);
      return ERROR;
    }

  tipusFiberLatencyOffset = flo;

  return OK;
}

/**
 * @ingroup PreInit
 * @brief Set the CrateID to be used during initialization
 *
 * @param cid Crate ID
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusSetCrateID_preInit(int cid)
{
  if((cid<0) || (cid>0xff))
    {
      printf("%s: ERROR: Invalid Crate ID (%d)\n",
	     __FUNCTION__,cid);
      return ERROR;
    }

  tipusCrateID = cid;

  return OK;
}


/*sergey*/
int
tipusSetFiberIn_preInit(int port)
{
  if((port!=1) && (port!=5))
    {
      printf("%s: ERROR: Invalid Slave Fiber In Port (%d)\n",
	     __FUNCTION__,port);
      return ERROR;
    }

  tipusSlaveFiberIn=port;

  return OK;
}

int
tipusClockOutput_preInit(int mode)
{
  if(mode==0)
  {
    tipusClockOutputMode = TIPUS_CLOCK_OUTPUT_SET_16MHZ;
    printf("Clock output is set to  16.667 MHz (250/15 MHz, duty cycle 47%/53%)\n");
  }
  else if(mode==1)
  {
    tipusClockOutputMode = TIPUS_CLOCK_OUTPUT_SET_25MHZ;
    printf("Clock output is set to  25.000 MHz (250/10 MHz, duty cycle 50%)\n");
  }
  else if(mode==2)
  {
    tipusClockOutputMode = TIPUS_CLOCK_OUTPUT_SET_41MHZ;
    printf("Clock output is set to  41.667 MHz (250/6 MHz,  duty cycle 50%)\n");
  }
  else if(mode==3)
  {
    tipusClockOutputMode = TIPUS_CLOCK_OUTPUT_SET_125MHZ;
    printf("Clock output is set to 125.000 MHz (250/2 MHz,  duty cycle 50%, default)\n");
  }
  else
  {
    tipusClockOutputMode = 0;
    printf("Clock output software setting is disabled\n");
  }

  return(0);
}

int
tipusGetClockOutput()
{
  int val, rval=0;

  if(TIPUSp==NULL)
  {
    printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
    return ERROR;
  }

  TIPUSLOCK;
  val = (tipusRead(&TIPUSp->clock) & TIPUS_CLOCK_OUTPUT_ENABLE_MASK) >> 10;
  rval = (tipusRead(&TIPUSp->clock) & TIPUS_CLOCK_OUTPUT_GET_MASK) >> 12;
  TIPUSUNLOCK;

  if(val != 2)  printf("Clock output software control is DISABLED\n");
  else
  {
    if(rval==0)      printf("Clock output is set to  16.667 MHz (250/15 MHz, duty cycle 47%/53%)\n");
    else if(rval==1) printf("Clock output is set to  25.000 MHz (250/10 MHz, duty cycle 50%)\n");
    else if(rval==2) printf("Clock output is set to  41.667 MHz (250/6 MHz,  duty cycle 50%)\n");
    else if(rval==3) printf("Clock output is set to 125.000 MHz (250/2 MHz,  duty cycle 50%, default)\n");
  }

  return rval;
}

void
tipusTest()
{
  tipusWrite(&TIPUSp->clock, 0x30000c00);
}

/*sergey*/




/**
 *  @ingroup Config
 *  @brief Initialize the Tipus register space into local memory,
 *  and setup registers given user input
 *
 *  @param  mode  Readout/Triggering Mode
 *     - 0 External Trigger - Interrupt Mode
 *     - 1 TI/TImaster Trigger - Interrupt Mode
 *     - 2 External Trigger - Polling Mode
 *     - 3 TI/TImaster Trigger - Polling Mode
 *
 *  @param iFlag Initialization bit mask
 *     - 0   Do not initialize the board, just setup the pointers to the registers
 *     - 1   Ignore firmware check
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

//float demon=0;

int
tipusInit(unsigned int mode, int iFlag)
{
  unsigned int rval, prodID;
  unsigned int firmwareInfo;
  int noBoardInit=0, noFirmwareCheck=0;

  if(iFlag&TIPUS_INIT_NO_INIT)
    {
      noBoardInit = 1;
    }
  if(iFlag&TIPUS_INIT_SKIP_FIRMWARE_CHECK)
    {
      noFirmwareCheck=1;
    }
  if(iFlag&TIPUS_INIT_USE_DMA)
    {
      tipusUseDma=1;
    }
  else
    {
      tipusUseDma=0;
    }

  /* Pointer should already be set up tipusOpen */
  if(TIPUSp==NULL)
    {
      printf("%s: Pointer not initialized.  Calling tipusOpen()\n",
	     __FUNCTION__);
      if(tipusOpen()==ERROR)
	return -1;
    }

  /* Check if TI board is readable */
  /* Read the boardID reg */
  rval = tipusRead(&TIPUSp->boardID);

  if (rval == ERROR)
    {
      printf("%s: ERROR: Tipcieus card not addressable\n",__FUNCTION__);
      TIPUSp=NULL;
      return(-1);
    }
  else
    {
      /* Check that it is a TI */
      if(((rval&TIPUS_BOARDID_TYPE_MASK)>>16) != TIPUS_BOARDID_TYPE_TI)
	{
	  printf("%s: ERROR: Invalid Board ID: 0x%x (rval = 0x%08x)\n",
		 __FUNCTION__,
		 (rval&TIPUS_BOARDID_TYPE_MASK)>>16,rval);
	  TIPUSp=NULL;
	  return(ERROR);
	}

      /* Get the "production" type bits.  2=modTI, 1=production, 0=prototype */
      prodID = (rval&TIPUS_BOARDID_PROD_MASK)>>16;

    }

  if(!noBoardInit)
    {
      if(tipusMaster==0) /* Reload only on the TI Slaves */
      {
	/* tiReload(); */
	/* taskDelay(60); */
      }
      tipusDisableTriggerSource(0);
    }

#ifdef SUPPORT_FIRMWARE_CHECK
  /* Get the Firmware Information and print out some details */
  firmwareInfo = tipusGetFirmwareVersion();
  if(firmwareInfo>0)
    {
      int supportedVersion = TIPUS_SUPPORTED_FIRMWARE;
      int supportedType    = TIPUS_SUPPORTED_TYPE;
      int tipusFirmwareType   = (firmwareInfo & TIPUS_FIRMWARE_TYPE_MASK)>>12;

      tipusVersion = firmwareInfo&0xFFF;
      printf("  ID: 0x%x \tFirmware (type - revision): 0x%X - 0x%03X\n",
	     (firmwareInfo&TIPUS_FIRMWARE_ID_MASK)>>16, tipusFirmwareType, tipusVersion);

      if(tipusFirmwareType != supportedType)
	{
	  if(noFirmwareCheck)
	    {
	      printf("%s: WARN: Firmware type (%d) not supported by this driver.\n  Supported type = %d  (IGNORED)\n",
		     __FUNCTION__,tipusFirmwareType,supportedType);
	    }
	  else
	    {
	      printf("%s: ERROR: Firmware Type (%d) not supported by this driver.\n  Supported type = %d\n",
		     __FUNCTION__,tipusFirmwareType,supportedType);
	      TIPUSp=NULL;
	      return ERROR;
	    }
	}

      if(tipusVersion != supportedVersion)
	{
	  if(noFirmwareCheck)
	    {
	      printf("%s: WARN: Firmware version (0x%x) not supported by this driver.\n  Supported version = 0x%x  (IGNORED)\n",
		     __FUNCTION__,tipusVersion,supportedVersion);
	    }
	  else
	    {
	      printf("%s: ERROR: Firmware version (0x%x) not supported by this driver.\n  Supported version = 0x%x\n",
		     __FUNCTION__,tipusVersion,supportedVersion);
	      TIPUSp=NULL;
	      return ERROR;
	    }
	}
    }
  else
    {
      printf("%s:  ERROR: Invalid firmware 0x%08x\n",
	     __FUNCTION__,firmwareInfo);
      return ERROR;
    }
#endif /* SUPPORT_FIRMWARE_CHECK */

  /* Check if we should exit here, or initialize some board defaults */
  if(noBoardInit)
    {
      return OK;
    }

  tipusReset();

#ifdef OLDDMA
  if(tipusUseDma)
    {
      printf("%s: Configuring DMA\n",__FUNCTION__);
      tipusDmaConfig(1, 0, 2); /* set up the DMA, 1MB, 32-bit, 256 byte packet */
      tipusDmaSetAddr(tipusDmaAddrBase,0);
    }
  else
    {
      printf("%s: Configuring FIFO\n",__FUNCTION__);
      tipusEnableFifo();
    }
#else
  /* Try taking this out */
  /* tipusEnableFifo(); */
#endif /* OLDDMA */

  /* Set some defaults, dependent on Master/Slave status */
  switch(mode)
    {
    case TIPUS_READOUT_EXT_INT:
    case TIPUS_READOUT_EXT_POLL:

      printf("... Configure as TI Master...\n");
      /* Master (Supervisor) Configuration: takes in external triggers */
      tipusMaster = 1;

      /* Clear the Slave Mask */
      tipusSlaveMask = 0;

      /* Self as busy source */
      tipusSetBusySource(TIPUS_BUSY_LOOPBACK,1);

      /* Onboard Clock Source */
      tipusSetClockSource(TIPUS_CLOCK_INTERNAL);
      /* Loopback Sync Source */
      tipusSetSyncSource(TIPUS_SYNC_LOOPBACK);
      break;

    case TIPUS_READOUT_TS_INT:
    case TIPUS_READOUT_TS_POLL:

      printf("... Configure as TI Slave...\n");
      /* Slave Configuration: takes in triggers from the Master (supervisor) */
      tipusMaster = 0;

      /* BUSY  */
      tipusSetBusySource(0,1);


      if(tipusSlaveFiberIn==1)
      {
        /* Enable HFBR#1 */
        tipusEnableFiber(1);
        /* HFBR#1 Clock Source */
        tipusSetClockSource(1);
        /* HFBR#1 Sync Source */
        tipusSetSyncSource(TIPUS_SYNC_HFBR1);
        /* HFBR#1 Trigger Source */
        tipusSetTriggerSource(TIPUS_TRIGGER_HFBR1);
      }
      else if(tipusSlaveFiberIn==5)
      {
        printf("CLOCK1=%d\n",tipusGetClockSource());
        /* Enable HFBR#5 */
        tipusEnableFiber(5);

        printf("CLOCK2=%d\n",tipusGetClockSource());
        /* HFBR#5 Clock Source */
        tipusSetClockSource(5);

        printf("CLOCK3=%d\n",tipusGetClockSource());fflush(stdout);
        /* HFBR#5 Sync Source */
        tipusSetSyncSource(TIPUS_SYNC_HFBR5);

        printf("CLOCK4=%d\n",tipusGetClockSource());fflush(stdout);
        /* HFBR#5 Trigger Source */
        tipusSetTriggerSource(TIPUS_TRIGGER_HFBR5);

        printf("CLOCK5=%d\n",tipusGetClockSource());fflush(stdout);
      }

      break;

    default:
      printf("%s: ERROR: Invalid TI Mode %d\n",
	     __FUNCTION__,mode);
      return ERROR;
    }
  tipusReadoutMode = mode;

  /* Setup some Other Library Defaults */
  if(tipusMaster!=1)
  {



      printf("CLOCK6=%d\n",tipusGetClockSource());


      tipusSetFiberDelay(0x50, 0xcf);/*sergey: for TS as master*/
      //tipusSetFiberDelay(0x70, 0xcf);/*sergey: for SVT1 as master*/

      /*sergey
      if(FiberMeas() == ERROR)
	{
	  printf("%s: Fiber Measurement failure.  Check fiber and/or fiber port,\n",
		 __FUNCTION__);
	  return ERROR;
	}
      */



      printf("CLOCK7=%d\n",tipusGetClockSource());

      tipusWrite(&TIPUSp->syncWidth, 0x24);
      // TI IODELAY reset
      tipusWrite(&TIPUSp->reset,TIPUS_RESET_IODELAY);
      usleep(10);

      printf("CLOCK8=%d\n",tipusGetClockSource());//=1



      // TI Sync auto alignment
      if(tipusSlaveFiberIn==1)
      {
        tipusWrite(&TIPUSp->reset,TIPUS_RESET_AUTOALIGN_HFBR1_SYNC);
      }
      else
      {
tipusSetSyncSource(0); /*sergey*/
        tipusWrite(&TIPUSp->reset,TIPUS_RESET_AUTOALIGN_HFBR5_SYNC);
tipusSetSyncSource(TIPUS_SYNC_HFBR5); /*sergey*/
      }
      usleep(10);



      printf("CLOCK10=%d\n",tipusGetClockSource());//=3

      // TI auto fiber delay measurement
      tipusWrite(&TIPUSp->reset,TIPUS_RESET_MEASURE_LATENCY);
      usleep(10);

      printf("CLOCK11=%d\n",tipusGetClockSource());

      // TI auto alignement fiber delay
      tipusWrite(&TIPUSp->reset,TIPUS_RESET_FIBER_AUTO_ALIGN);
      usleep(10);
  }
  else
  {
      // TI IODELAY reset
      tipusWrite(&TIPUSp->reset,TIPUS_RESET_IODELAY);
      usleep(10);

      // TI Sync auto alignment
      tipusWrite(&TIPUSp->reset,TIPUS_RESET_AUTOALIGN_HFBR1_SYNC);
      usleep(10);

      // Perform a trigger link reset
      tipusTrigLinkReset();
      usleep(10);
  }



  printf("CLOCK12=%d\n",tipusGetClockSource());

  /* Setup a default Sync Delay and Pulse width */
  if(tipusMaster==1)
    tipusSetSyncDelayWidth(0x54, 0x2f, 0);

  printf("CLOCK13=%d\n",tipusGetClockSource());

  /* Set default sync delay (fiber compensation) */
  if(tipusMaster==1)
    tipusWrite(&TIPUSp->fiberSyncDelay,
	       (tipusFiberLatencyOffset<<16)&TIPUS_FIBERSYNCDELAY_LOOPBACK_SYNCDELAY_MASK);

  printf("CLOCK14=%d\n",tipusGetClockSource());

  /* Set Default Block Level to 1, and default crateID */
  if(tipusMaster==1)
    tipusSetBlockLevel(1);

  printf("CLOCK15=%d\n",tipusGetClockSource());

  tipusSetCrateID(tipusCrateID);

  printf("CLOCK16=%d\n",tipusGetClockSource());

  /* Set Event format for CODA 3.0 */
  tipusSetEventFormat(3);

  printf("CLOCK17=%d\n",tipusGetClockSource());

  /* Set Default Trig1 and Trig2 delay=16ns (0+1)*16ns, width=64ns (15+1)*4ns */
  tipusSetTriggerPulse(1,0,15,0);
  tipusSetTriggerPulse(2,0,15,0);

  printf("CLOCK18=%d\n",tipusGetClockSource());

  /* Set the default prescale factor to 0 for rate/(0+1) */
  tipusSetPrescale(0);

  printf("CLOCK19=%d\n",tipusGetClockSource());

  /* MGT reset */
  if(tipusMaster==1)
  {
    tipusResetMGT();
  }

  printf("CLOCK20=%d\n",tipusGetClockSource());

  /* Set this to 1 (ROC Lock mode), by default. */
  tipusSetBlockBufferLevel(1);

  printf("CLOCK21=%d\n",tipusGetClockSource());

  /* Disable all TS Inputs */
  tipusDisableTSInput(TIPUS_TSINPUT_ALL);

  printf("CLOCK99=%d\n",tipusGetClockSource());

  return OK;
}

int
tipusCheckAddresses()
{
  unsigned long offset=0, expected=0, base=0;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  printf("%s:\n\t ---------- Checking TI address space ---------- \n",__FUNCTION__);

  base = (unsigned long) &TIPUSp->boardID;

  offset = ((unsigned long) &TIPUSp->trigsrc) - base;
  expected = 0x20;
  if(offset != expected)
    printf("%s: ERROR TIPUSp->triggerSource not at offset = 0x%lx (@ 0x%lx)\n",
	   __FUNCTION__,expected,offset);

  offset = ((unsigned long) &TIPUSp->syncWidth) - base;
  expected = 0x80;
  if(offset != expected)
    printf("%s: ERROR TIPUSp->syncWidth not at offset = 0x%lx (@ 0x%lx)\n",
	   __FUNCTION__,expected,offset);

  offset = ((unsigned long) &TIPUSp->adr24) - base;
  expected = 0xD0;
  if(offset != expected)
    printf("%s: ERROR TIPUSp->adr24 not at offset = 0x%lx (@ 0x%lx)\n",
	   __FUNCTION__,expected,offset);

  offset = ((unsigned long) &TIPUSp->reset) - base;
  expected = 0x100;
  if(offset != expected)
    printf("%s: ERROR TIPUSp->reset not at offset = 0x%lx (@ 0x%lx)\n",
	   __FUNCTION__,expected,offset);

  return OK;
}

/**
 * @ingroup Status
 * @brief Print some status information of the TI to standard out
 *
 * @param pflag if pflag>0, print out raw registers
 *
 */

void
tipusStatus(int pflag)
{
  struct TIPCIEUS_RegStruct *ro;
  int iinp, iblock, ifiber;
  unsigned int blockStatus[5], nblocksReady, nblocksNeedAck;
  unsigned int fibermask;
  unsigned long TIBase;
  unsigned long long int l1a_count=0;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return;
    }

  ro = (struct TIPCIEUS_RegStruct *) malloc(sizeof(struct TIPCIEUS_RegStruct));
  if(ro == NULL)
    {
      printf("%s: ERROR allocating memory for TI register structure\n",
	     __FUNCTION__);
      return;
    }

  /* latch live and busytime scalers */
  tipusLatchTimers();
  l1a_count    = tipusGetEventCounter();
  tipusGetCurrentBlockLevel();

  TIPUSLOCK;
  ro->boardID      = tipusRead(&TIPUSp->boardID);
#ifdef OLD_FIBER_REG
  ro->fiber        = tipusRead(&TIPUSp->fiber);
#endif
  ro->intsetup     = tipusRead(&TIPUSp->intsetup);
  ro->trigDelay    = tipusRead(&TIPUSp->trigDelay);
  ro->__adr32      = tipusRead(&TIPUSp->__adr32);
  ro->blocklevel   = tipusRead(&TIPUSp->blocklevel);
  ro->dataFormat   = tipusRead(&TIPUSp->dataFormat);
  ro->vmeControl   = tipusRead(&TIPUSp->vmeControl);
  ro->trigsrc      = tipusRead(&TIPUSp->trigsrc);
  ro->sync         = tipusRead(&TIPUSp->sync);
  ro->busy         = tipusRead(&TIPUSp->busy);
  ro->clock        = tipusRead(&TIPUSp->clock);
  ro->trig1Prescale = tipusRead(&TIPUSp->trig1Prescale);
  ro->blockBuffer  = tipusRead(&TIPUSp->blockBuffer);

  ro->tsInput      = tipusRead(&TIPUSp->tsInput);

  ro->output       = tipusRead(&TIPUSp->output);
  ro->syncEventCtrl= tipusRead(&TIPUSp->syncEventCtrl);
  ro->blocklimit   = tipusRead(&TIPUSp->blocklimit);
  ro->fiberSyncDelay = tipusRead(&TIPUSp->fiberSyncDelay);

  ro->GTPStatusA   = tipusRead(&TIPUSp->GTPStatusA);
  ro->GTPStatusB   = tipusRead(&TIPUSp->GTPStatusB);

  /* Latch scalers first */
  tipusWrite(&TIPUSp->reset,TIPUS_RESET_SCALERS_LATCH);
  ro->livetime     = tipusRead(&TIPUSp->livetime);
  ro->busytime     = tipusRead(&TIPUSp->busytime);

  ro->inputCounter = tipusRead(&TIPUSp->inputCounter);

  for(iblock=0;iblock<4;iblock++)
    blockStatus[iblock] = tipusRead(&TIPUSp->blockStatus[iblock]);

  blockStatus[4] = tipusRead(&TIPUSp->adr24);

  ro->nblocks      = tipusRead(&TIPUSp->nblocks);

  ro->GTPtriggerBufferLength = tipusRead(&TIPUSp->GTPtriggerBufferLength);

  ro->rocEnable    = tipusRead(&TIPUSp->rocEnable);
  TIPUSUNLOCK;

  TIBase = (unsigned long)TIPUSp;

  printf("\n");
  printf("STATUS for Tipcieus\n");
  printf("--------------------------------------------------------------------------------\n");

  if(tipusMaster)
    printf(" Configured as a TI Master\n");
  else
    printf(" Configured as a TI Slave\n");

  printf(" Readout Count: %d\n",tipusIntCount);
  printf("     Ack Count: %d\n",tipusAckCount);
  printf("     L1A Count: %llu\n",l1a_count);
  printf("   Block Limit: %d   %s\n",ro->blocklimit,
	 (ro->blockBuffer & TIPUS_BLOCKBUFFER_BUSY_ON_BLOCKLIMIT)?"* Finished *":"");
  printf("   Block Count: %d\n",ro->nblocks & TIPUS_NBLOCKS_COUNT_MASK);

  if(pflag>0)
    {
      printf("\n");
      printf(" Registers (offset):\n");
      printf("  boardID        (0x%04lx) = 0x%08x\n", (unsigned long)&TIPUSp->boardID - TIBase, ro->boardID);
#ifdef OLD_FIBER_REG
      printf("  fiber          (0x%04lx) = 0x%08x\n", (unsigned long)(&TIPUSp->fiber) - TIBase, ro->fiber);
#endif
      printf("  intsetup       (0x%04lx) = 0x%08x\t", (unsigned long)(&TIPUSp->intsetup) - TIBase, ro->intsetup);
      printf("  trigDelay      (0x%04lx) = 0x%08x\n", (unsigned long)(&TIPUSp->trigDelay) - TIBase, ro->trigDelay);
      printf("  __adr32        (0x%04lx) = 0x%08x\t", (unsigned long)(&TIPUSp->__adr32) - TIBase, ro->__adr32);
      printf("  blocklevel     (0x%04lx) = 0x%08x\n", (unsigned long)(&TIPUSp->blocklevel) - TIBase, ro->blocklevel);
      printf("  vmeControl     (0x%04lx) = 0x%08x\n", (unsigned long)(&TIPUSp->vmeControl) - TIBase, ro->vmeControl);
      printf("  trigger        (0x%04lx) = 0x%08x\t", (unsigned long)(&TIPUSp->trigsrc) - TIBase, ro->trigsrc);
      printf("  sync           (0x%04lx) = 0x%08x\n", (unsigned long)(&TIPUSp->sync) - TIBase, ro->sync);
      printf("  busy           (0x%04lx) = 0x%08x\t", (unsigned long)(&TIPUSp->busy) - TIBase, ro->busy);
      printf("  clock          (0x%04lx) = 0x%08x\n", (unsigned long)(&TIPUSp->clock) - TIBase, ro->clock);
      printf("  blockBuffer    (0x%04lx) = 0x%08x\t", (unsigned long)(&TIPUSp->blockBuffer) - TIBase, ro->blockBuffer);

      printf("  output         (0x%04lx) = 0x%08x\n", (unsigned long)(&TIPUSp->output) - TIBase, ro->output);
      printf("  fiberSyncDelay (0x%04lx) = 0x%08x\n", (unsigned long)(&TIPUSp->fiberSyncDelay) - TIBase, ro->fiberSyncDelay);

      printf("  GTPStatusA     (0x%04lx) = 0x%08x\t", (unsigned long)(&TIPUSp->GTPStatusA) - TIBase, ro->GTPStatusA);
      printf("  GTPStatusB     (0x%04lx) = 0x%08x\n", (unsigned long)(&TIPUSp->GTPStatusB) - TIBase, ro->GTPStatusB);

      printf("  livetime       (0x%04lx) = 0x%08x\t", (unsigned long)(&TIPUSp->livetime) - TIBase, ro->livetime);
      printf("  busytime       (0x%04lx) = 0x%08x\n", (unsigned long)(&TIPUSp->busytime) - TIBase, ro->busytime);
      printf("  GTPTrgBufLen   (0x%04lx) = 0x%08x\t", (unsigned long)(&TIPUSp->GTPtriggerBufferLength) - TIBase, ro->GTPtriggerBufferLength);
      printf("  rocEnable      (0x%04lx) = 0x%08x\n", (unsigned long)(&TIPUSp->rocEnable) - TIBase, ro->rocEnable);
    }
  printf("\n");

  if((!tipusMaster) && (tipusBlockLevel==0))
    {
      printf(" Block Level not yet received\n");
    }
  else
    {
      printf(" Block Level = %d ", tipusBlockLevel);
      if(tipusBlockLevel != tipusNextBlockLevel)
	printf("(To be set = %d)\n", tipusNextBlockLevel);
      else
	printf("\n");
    }

  printf(" Block Buffer Level = ");
  if(ro->vmeControl & TIPUS_VMECONTROL_USE_LOCAL_BUFFERLEVEL)
    {
      printf("%d -Local- ",
	     ro->blockBuffer & TIPUS_BLOCKBUFFER_BUFFERLEVEL_MASK);
    }
  else
    {
      printf("%d -Broadcast- ",
	     (ro->dataFormat & TIPUS_DATAFORMAT_BCAST_BUFFERLEVEL_MASK) >> 24);
    }

  printf("(%s)\n",(ro->vmeControl & TIPUS_VMECONTROL_BUSY_ON_BUFFERLEVEL)?
	 "Busy Enabled":"Busy not enabled");

  if(tipusMaster)
    {
      if((ro->syncEventCtrl & TIPUS_SYNCEVENTCTRL_NBLOCKS_MASK) == 0)
	printf(" Sync Events DISABLED\n");
      else
	printf(" Sync Event period  = %d blocks\n",
	       ro->syncEventCtrl & TIPUS_SYNCEVENTCTRL_NBLOCKS_MASK);
    }

  printf("\n");
#ifdef OLD_FIBER_REG
  printf(" Fiber Status         1   \n");
  printf("                    ----- \n");
  printf("  Connected          ");
  for(ifiber=0; ifiber<1; ifiber++)
    {
      printf("%s   ",
	     (ro->fiber & TIPUS_FIBER_CONNECTED_TI(ifiber+1))?"YES":"   ");
    }
  printf("\n");
  if(tipusMaster)
    {
      printf("  Trig Src Enabled   ");
      for(ifiber=0; ifiber<1; ifiber++)
	{
	  printf("%s   ",
		 (ro->fiber & TIPUS_FIBER_TRIGSRC_ENABLED_TI(ifiber+1))?"YES":"   ");
	}
    }
  printf("\n\n");
#endif

  if(tipusMaster)
    {
      if(tipusSlaveMask)
	{
	  printf(" TI Slaves Configured on HFBR (0x%x) = ",tipusSlaveMask);
	  fibermask = tipusSlaveMask;
	  for(ifiber=0; ifiber<1; ifiber++)
	    {
	      if( fibermask & (1<<ifiber))
		printf(" %d",ifiber+1);
	    }
	  printf("\n");
	}
      else
	printf(" No TI Slaves Configured on HFBR\n");

    }

  printf(" Clock Source (%d) = \n",ro->clock & TIPUS_CLOCK_MASK);
  switch(ro->clock & TIPUS_CLOCK_MASK)
    {
    case TIPUS_CLOCK_INTERNAL:
      printf("   Internal\n");
      break;

    case TIPUS_CLOCK_HFBR5:
      printf("   HFBR #5 Input\n");
      if((ro->clock & TIPUS_CLOCK_BRIDGE)>0) printf("    (bridge mode is ON)\n");
      break;

    case TIPUS_CLOCK_HFBR1:
      printf("   HFBR #1 Input\n");
      break;

    case TIPUS_CLOCK_FP:
      printf("   Front Panel\n");
      break;

    default:
      printf("   UNDEFINED!\n");
    }

  if(tipusTriggerSource&TIPUS_TRIGSRC_SOURCEMASK)
    {
      if(ro->trigsrc)
	printf(" Trigger input source (%s) =\n",
	       (ro->blockBuffer & TIPUS_BLOCKBUFFER_BUSY_ON_BLOCKLIMIT)?"DISABLED on Block Limit":
	       "ENABLED");
      else
	printf(" Trigger input source (DISABLED) =\n");
      if(tipusTriggerSource & TIPUS_TRIGSRC_P0)
	printf("   P0 Input\n");
      if(tipusTriggerSource & TIPUS_TRIGSRC_HFBR1)
	printf("   HFBR #1 Input\n");
      if(tipusTriggerSource & TIPUS_TRIGSRC_HFBR5)
	printf("   HFBR #5 Input\n");
      if(tipusTriggerSource & TIPUS_TRIGSRC_LOOPBACK)
	printf("   Loopback\n");
      if(tipusTriggerSource & TIPUS_TRIGSRC_FPTRG)
	printf("   Front Panel TRG\n");
      if(tipusTriggerSource & TIPUS_TRIGSRC_VME)
	printf("   PCI Command\n");
      if(tipusTriggerSource & TIPUS_TRIGSRC_TSINPUTS)
	printf("   Front Panel TS Inputs\n");
      if(tipusTriggerSource & TIPUS_TRIGSRC_TSREV2)
	printf("   Trigger Supervisor (rev2)\n");
      if(tipusTriggerSource & TIPUS_TRIGSRC_PULSER)
	printf("   Internal Pulser\n");
      if(tipusTriggerSource & TIPUS_TRIGSRC_PART_1)
	printf("   TS Partition 1 (HFBR #1)\n");
      if(tipusTriggerSource & TIPUS_TRIGSRC_PART_2)
	printf("   TS Partition 2 (HFBR #1)\n");
      if(tipusTriggerSource & TIPUS_TRIGSRC_PART_3)
	printf("   TS Partition 3 (HFBR #1)\n");
      if(tipusTriggerSource & TIPUS_TRIGSRC_PART_4)
	printf("   TS Partition 4 (HFBR #1)\n");
    }
  else
    {
      printf(" No Trigger input sources\n");
    }

  if(ro->sync&TIPUS_SYNC_SOURCEMASK)
    {
      printf(" Sync source = \n");
      if(ro->sync & TIPUS_SYNC_P0)
	printf("   P0 Input\n");
      if(ro->sync & TIPUS_SYNC_HFBR1)
	printf("   HFBR #1 Input\n");
      if(ro->sync & TIPUS_SYNC_HFBR5)
	printf("   HFBR #5 Input\n");
      if(ro->sync & TIPUS_SYNC_FP)
	printf("   Front Panel Input\n");
      if(ro->sync & TIPUS_SYNC_LOOPBACK)
	printf("   Loopback\n");
      if(ro->sync & TIPUS_SYNC_USER_SYNCRESET_ENABLED)
	printf("   User SYNCRESET Receieve Enabled\n");
    }
  else
    {
      printf(" No SYNC input source configured\n");
    }

  if(ro->busy&TIPUS_BUSY_SOURCEMASK)
    {
      printf(" BUSY input source = \n");
      if(ro->busy & TIPUS_BUSY_SWA)
	printf("   Switch Slot A    %s\n",(ro->busy&TIPUS_BUSY_MONITOR_SWA)?"** BUSY **":"");
      if(ro->busy & TIPUS_BUSY_SWB)
	printf("   Switch Slot B    %s\n",(ro->busy&TIPUS_BUSY_MONITOR_SWB)?"** BUSY **":"");
      if(ro->busy & TIPUS_BUSY_P2)
	printf("   P2 Input         %s\n",(ro->busy&TIPUS_BUSY_MONITOR_P2)?"** BUSY **":"");
      if(ro->busy & TIPUS_BUSY_TRIGGER_LOCK)
	printf("   Trigger Lock     \n");
      if(ro->busy & TIPUS_BUSY_FP_FTDC)
	printf("   Front Panel TDC  %s\n",(ro->busy&TIPUS_BUSY_MONITOR_FP_FTDC)?"** BUSY **":"");
      if(ro->busy & TIPUS_BUSY_FP_FADC)
	printf("   Front Panel ADC  %s\n",(ro->busy&TIPUS_BUSY_MONITOR_FP_FADC)?"** BUSY **":"");
      if(ro->busy & TIPUS_BUSY_FP)
	printf("   Front Panel      %s\n",(ro->busy&TIPUS_BUSY_MONITOR_FP)?"** BUSY **":"");
      if(ro->busy & TIPUS_BUSY_LOOPBACK)
	printf("   Loopback         %s\n",(ro->busy&TIPUS_BUSY_MONITOR_LOOPBACK)?"** BUSY **":"");
      if(ro->busy & TIPUS_BUSY_HFBR1)
	printf("   HFBR #1          %s\n",(ro->busy&TIPUS_BUSY_MONITOR_HFBR1)?"** BUSY **":"");
      if(ro->busy & TIPUS_BUSY_HFBR2)
	printf("   HFBR #2          %s\n",(ro->busy&TIPUS_BUSY_MONITOR_HFBR2)?"** BUSY **":"");
      if(ro->busy & TIPUS_BUSY_HFBR3)
	printf("   HFBR #3          %s\n",(ro->busy&TIPUS_BUSY_MONITOR_HFBR3)?"** BUSY **":"");
      if(ro->busy & TIPUS_BUSY_HFBR4)
	printf("   HFBR #4          %s\n",(ro->busy&TIPUS_BUSY_MONITOR_HFBR4)?"** BUSY **":"");
      if(ro->busy & TIPUS_BUSY_HFBR5)
	printf("   HFBR #5          %s\n",(ro->busy&TIPUS_BUSY_MONITOR_HFBR5)?"** BUSY **":"");
      if(ro->busy & TIPUS_BUSY_HFBR6)
	printf("   HFBR #6          %s\n",(ro->busy&TIPUS_BUSY_MONITOR_HFBR6)?"** BUSY **":"");
      if(ro->busy & TIPUS_BUSY_HFBR7)
	printf("   HFBR #7          %s\n",(ro->busy&TIPUS_BUSY_MONITOR_HFBR7)?"** BUSY **":"");
      if(ro->busy & TIPUS_BUSY_HFBR8)
	printf("   HFBR #8          %s\n",(ro->busy&TIPUS_BUSY_MONITOR_HFBR8)?"** BUSY **":"");
    }
  else
    {
      printf(" No BUSY input source configured\n");
    }

  if(ro->tsInput & TIPUS_TSINPUT_MASK)
    {
      printf(" Front Panel TS Inputs Enabled: ");
      for(iinp=0; iinp<6; iinp++)
	{
	  if( (ro->tsInput & TIPUS_TSINPUT_MASK) & (1<<iinp))
	    printf(" %d",iinp+1);
	}
      printf("\n");
    }
  else
    {
      printf(" All Front Panel TS Inputs Disabled\n");
    }

  if(tipusMaster)
    {
      printf("\n");
      printf(" Trigger Rules:\n");
      tipusPrintTriggerHoldoff(pflag);
    }

  if(tipusMaster)
    {
      if(ro->rocEnable & TIPUS_ROCENABLE_SYNCRESET_REQUEST_ENABLE_MASK)
	{
	  printf(" SyncReset Request ENABLED from ");

	  if(ro->rocEnable & (1 << 10))
	    {
	      printf("SELF ");
	    }

	  for(ifiber=0; ifiber<1; ifiber++)
	    {
	      if(ro->rocEnable & (1 << (ifiber + 1 + 10)))
		{
		  printf("%d ", ifiber + 1);
		}
	    }

	  printf("\n");
	}
      else
	{
	  printf(" SyncReset Requests DISABLED\n");
	}

      printf("\n");
      tipusSyncResetRequestStatus(1);
    }
  printf("\n");

  if(ro->intsetup&TIPUS_INTSETUP_ENABLE)
    printf(" Interrupts ENABLED\n");
  else
    printf(" Interrupts DISABLED\n");
  printf("   Level = %d   Vector = 0x%02x\n",
	 (ro->intsetup&TIPUS_INTSETUP_LEVEL_MASK)>>8, (ro->intsetup&TIPUS_INTSETUP_VECTOR_MASK));

  printf(" Blocks ready for readout: %d\n",(ro->blockBuffer&TIPUS_BLOCKBUFFER_BLOCKS_READY_MASK)>>8);
  if(tipusMaster)
    {
      printf(" Slave Block Status:   %s\n",
	     (ro->busy&TIPUS_BUSY_MONITOR_TRIG_LOST)?"** Waiting for Trigger Ack **":"");
      /* TI slave block status */
      fibermask = tipusSlaveMask;
      for(ifiber=0; ifiber<1; ifiber++)
	{
	  if( fibermask & (1<<ifiber) )
	    {
	      if( (ifiber % 2) == 0)
		{
		  nblocksReady   = blockStatus[ifiber/2] & TIPUS_BLOCKSTATUS_NBLOCKS_READY0;
		  nblocksNeedAck = (blockStatus[ifiber/2] & TIPUS_BLOCKSTATUS_NBLOCKS_NEEDACK0)>>8;
		}
	      else
		{
		  nblocksReady   = (blockStatus[(ifiber-1)/2] & TIPUS_BLOCKSTATUS_NBLOCKS_READY1)>>16;
		  nblocksNeedAck = (blockStatus[(ifiber-1)/2] & TIPUS_BLOCKSTATUS_NBLOCKS_NEEDACK1)>>24;
		}
	      printf("  Fiber %d  :  Blocks ready / need acknowledge: %d / %d\n",
		     ifiber+1,nblocksReady, nblocksNeedAck);
	    }
	}

      /* TI master block status */
      nblocksReady   = (blockStatus[4] & TIPUS_BLOCKSTATUS_NBLOCKS_READY1)>>16;
      nblocksNeedAck = (blockStatus[4] & TIPUS_BLOCKSTATUS_NBLOCKS_NEEDACK1)>>24;
      printf("  Loopback :  Blocks ready / need acknowledge: %d / %d\n",
	     nblocksReady, nblocksNeedAck);

    }
  printf(" Input counter %d\n",ro->inputCounter);
  printf(" useDma %d\n", tipusUseDma);

  printf("--------------------------------------------------------------------------------\n");
  printf("\n\n");

  if(ro)
    free(ro);
}

/**
 * @ingroup SlaveConfig
 * @brief This routine provides the ability to switch the port that the TI Slave
 *     receives its Clock, SyncReset, and Trigger.
 *     If the TI has already been configured to use this port, nothing is done.
 *
 *   @param port
 *      -  1  - Port 1
 *
 * @return OK if successful, ERROR otherwise
 *
 */
int
tipusSetSlavePort(int port)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Slave.\n",__FUNCTION__);
      return ERROR;
    }

  if((port!=1) && (port!=5))
    {
      printf("%s: ERROR: Invalid port specified (%d).  Must be 1 or 5 for TI Slave.\n",
	     __FUNCTION__,port);
      return ERROR;
    }

  if(port==tipusSlaveFiberIn)
    {
      printf("%s: INFO: TI Slave already configured to use port %d.\n",
	     __FUNCTION__,port);
      return OK;
    }

  TIPUSLOCK;
  tipusSlaveFiberIn=port;
  TIPUSUNLOCK;


  if(tipusSlaveFiberIn==1)
  {
    /* Enable HFBR#1 */
    tipusEnableFiber(1);
    /* HFBR#1 Clock Source */
    tipusSetClockSource(1);
    /* HFBR#1 Sync Source */
    tipusSetSyncSource(TIPUS_SYNC_HFBR1);
    /* HFBR#1 Trigger Source */
    tipusSetTriggerSource(TIPUS_TRIGGER_HFBR1);
  }
  else if(tipusSlaveFiberIn==5)
  {
    /* Enable HFBR#5 */
    tipusEnableFiber(5);
    /* HFBR#5 Clock Source */
    tipusSetClockSource(5);
    /* HFBR#5 Sync Source */
    tipusSetSyncSource(TIPUS_SYNC_HFBR5);
    /* HFBR#5 Trigger Source */
    tipusSetTriggerSource(TIPUS_TRIGGER_HFBR5);
  }


  /* Measure and apply fiber compensation */
  /*sergey
  if(FiberMeas() == ERROR)
    return ERROR;
  */

  /* TI IODELAY reset */
  TIPUSLOCK;
  tipusWrite(&TIPUSp->reset,TIPUS_RESET_IODELAY);
  usleep(10);

  /* TI Sync auto alignment */
  if(tipusSlaveFiberIn==1)
    tipusWrite(&TIPUSp->reset,TIPUS_RESET_AUTOALIGN_HFBR1_SYNC);
  else
    tipusWrite(&TIPUSp->reset,TIPUS_RESET_AUTOALIGN_HFBR5_SYNC);
  usleep(10);

  /* TI auto fiber delay measurement */
  tipusWrite(&TIPUSp->reset,TIPUS_RESET_MEASURE_LATENCY);
  usleep(10);

  /* TI auto alignement fiber delay */
  tipusWrite(&TIPUSp->reset,TIPUS_RESET_FIBER_AUTO_ALIGN);
  usleep(10);
  TIPUSUNLOCK;

  printf("%s: INFO: TI Slave configured to use port %d.\n",
	 __FUNCTION__,port);
  return OK;
}

int
tipusGetSlavePort()
{
  return tipusSlaveFiberIn;
}



/**
 * @ingroup Status
 * @brief Print a summary of all fiber port connections to potential TI Slaves
 *
 * @param  pflag
 *   -  0  - Default output
 *   -  1  - Print Raw Registers
 *
 */

void
tipusSlaveStatus(int pflag)
{
  int iport=0, ibs=0, ifiber=0;
  unsigned int TIBase;
  unsigned int hfbr_tiID[8] = {1,2,3,4,5,6,7};
  unsigned int master_tiID;
  unsigned int blockStatus[5];
  unsigned int fiber=0, busy=0, trigsrc=0;
  int nblocksReady=0, nblocksNeedAck=0, slaveCount=0;
  int blocklevel=0;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return;
    }

  TIPUSLOCK;
  for(iport=0; iport<1; iport++)
    {
      hfbr_tiID[iport] = tipusRead(&TIPUSp->hfbr_tiID[iport]);
    }
  master_tiID = tipusRead(&TIPUSp->master_tiID);
#ifdef OLD_FIBER_REG
  fiber       = tipusRead(&TIPUSp->fiber);
#endif
  busy        = tipusRead(&TIPUSp->busy);
  trigsrc     = tipusRead(&TIPUSp->trigsrc);
  for(ibs=0; ibs<4; ibs++)
    {
      blockStatus[ibs] = tipusRead(&TIPUSp->blockStatus[ibs]);
    }
  blockStatus[4] = tipusRead(&TIPUSp->adr24);

  blocklevel = (tipusRead(&TIPUSp->blocklevel) & TIPUS_BLOCKLEVEL_CURRENT_MASK)>>16;

  TIPUSUNLOCK;

  TIBase = (unsigned long)TIPUSp;

  if(pflag>0)
    {
      printf(" Registers (offset):\n");
      /* printf("  TIBase     (0x%08x)\n",(unsigned int)(TIBase-tiA24Offset)); */
      printf("  busy           (0x%04lx) = 0x%08x\n", (unsigned long)(&TIPUSp->busy) - TIBase, busy);
#ifdef OLD_FIBER_REG
      printf("  fiber          (0x%04lx) = 0x%08x\n", (unsigned long)(&TIPUSp->fiber) - TIBase, fiber);
#endif
      printf("  hfbr_tiID[0]   (0x%04lx) = 0x%08x\t", (unsigned long)(&TIPUSp->hfbr_tiID[0]) - TIBase, hfbr_tiID[0]);
      printf("  hfbr_tiID[1]   (0x%04lx) = 0x%08x\n", (unsigned long)(&TIPUSp->hfbr_tiID[1]) - TIBase, hfbr_tiID[1]);
      printf("  hfbr_tiID[2]   (0x%04lx) = 0x%08x\t", (unsigned long)(&TIPUSp->hfbr_tiID[2]) - TIBase, hfbr_tiID[2]);
      printf("  hfbr_tiID[3]   (0x%04lx) = 0x%08x\n", (unsigned long)(&TIPUSp->hfbr_tiID[3]) - TIBase, hfbr_tiID[3]);
      printf("  hfbr_tiID[4]   (0x%04lx) = 0x%08x\t", (unsigned long)(&TIPUSp->hfbr_tiID[4]) - TIBase, hfbr_tiID[4]);
      printf("  hfbr_tiID[5]   (0x%04lx) = 0x%08x\n", (unsigned long)(&TIPUSp->hfbr_tiID[5]) - TIBase, hfbr_tiID[5]);
      printf("  hfbr_tiID[6]   (0x%04lx) = 0x%08x\t", (unsigned long)(&TIPUSp->hfbr_tiID[6]) - TIBase, hfbr_tiID[6]);
      printf("  hfbr_tiID[7]   (0x%04lx) = 0x%08x\n", (unsigned long)(&TIPUSp->hfbr_tiID[7]) - TIBase, hfbr_tiID[7]);
      printf("  master_tiID    (0x%04lx) = 0x%08x\t", (unsigned long)(&TIPUSp->master_tiID) - TIBase, master_tiID);

      printf("\n");
    }

  printf("TI-Master Port STATUS Summary\n");
  printf("                                                     Block Status\n");
  printf("Port  ROCID   Connected   TrigSrcEn   Busy Status   Ready / NeedAck  Blocklevel\n");
  printf("--------------------------------------------------------------------------------\n");
  /* Master first */
  /* Slot and Port number */
  printf("L     ");

  /* Port Name */
  printf("%5d      ",
	 (master_tiID&TIPUS_ID_CRATEID_MASK)>>8);

  /* Connection Status */
  printf("%s      %s       ",
	 "YES",
	 (trigsrc & TIPUS_TRIGSRC_LOOPBACK)?"ENABLED ":"DISABLED");

  /* Busy Status */
  printf("%s       ",
	 (busy & TIPUS_BUSY_MONITOR_LOOPBACK)?"BUSY":"    ");

  /* Block Status */
  nblocksReady   = (blockStatus[4] & TIPUS_BLOCKSTATUS_NBLOCKS_READY1)>>16;
  nblocksNeedAck = (blockStatus[4] & TIPUS_BLOCKSTATUS_NBLOCKS_NEEDACK1)>>24;
  printf("  %3d / %3d",nblocksReady, nblocksNeedAck);
  printf("        %3d",blocklevel);
  printf("\n");

  /* Slaves last */
  for(iport=1; iport<2; iport++)
    {
      /* Only continue of this port has been configured as a slave */
      if((tipusSlaveMask & (1<<(iport-1)))==0) continue;

      /* Slot and Port number */
      printf("%d     ", iport);

      /* Port Name */
      printf("%5d      ",
	     (hfbr_tiID[iport-1]&TIPUS_ID_CRATEID_MASK)>>8);

      /* Connection Status */
      printf("%s      %s       ",
	     (fiber & TIPUS_FIBER_CONNECTED_TI(iport))?"YES":"NO ",
	     (fiber & TIPUS_FIBER_TRIGSRC_ENABLED_TI(iport))?"ENABLED ":"DISABLED");

      /* Busy Status */
      printf("%s       ",
	     (busy & TIPUS_BUSY_MONITOR_FIBER_BUSY(iport))?"BUSY":"    ");

      /* Block Status */
      ifiber=iport-1;
      if( (ifiber % 2) == 0)
	{
	  nblocksReady   = blockStatus[ifiber/2] & TIPUS_BLOCKSTATUS_NBLOCKS_READY0;
	  nblocksNeedAck = (blockStatus[ifiber/2] & TIPUS_BLOCKSTATUS_NBLOCKS_NEEDACK0)>>8;
	}
      else
	{
	  nblocksReady   = (blockStatus[(ifiber-1)/2] & TIPUS_BLOCKSTATUS_NBLOCKS_READY1)>>16;
	  nblocksNeedAck = (blockStatus[(ifiber-1)/2] & TIPUS_BLOCKSTATUS_NBLOCKS_NEEDACK1)>>24;
	}
      printf("  %3d / %3d",nblocksReady, nblocksNeedAck);

      printf("        %3d",(hfbr_tiID[iport-1]&TIPUS_ID_BLOCKLEVEL_MASK)>>16);

      printf("\n");
      slaveCount++;
    }
  printf("\n");
  printf("Total Slaves Added = %d\n",slaveCount);

}

/**
 * @ingroup Status
 * @brief Get the Firmware Version
 *
 * @return Firmware Version if successful, ERROR otherwise
 *
 */
int
tipusGetFirmwareVersion()
{
#ifdef MAPJTAG
  unsigned int rval=0;
  int delay=10000;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  /* reset the VME_to_JTAG engine logic */
  tipusWrite(&TIPUSp->reset,TIPUS_RESET_JTAG);
  usleep(delay);

  /* Reset FPGA JTAG to "reset_idle" state */
  tipusJTAGWrite(0x3c,0);
  usleep(delay);

  /* enable the user_code readback */
  tipusJTAGWrite(0x26c,0x3c8);
  usleep(delay);

  /* shift in 32-bit to FPGA JTAG */
  tipusJTAGWrite(0x7dc,0);
  usleep(delay);

  /* Readback the firmware version */
  rval = tipusJTAGRead(0x00);
  TIPUSUNLOCK;

  return rval;
#else
  printf("%s: ERROR: JTAG Not supported\n",
	 __func__);
  return ERROR;
#endif /* MAPJTAG */

}

/**
 * @ingroup Status
 * @brief Get the Module Serial Number
 *
 * @param rSN  Pointer to string to pass Serial Number
 *
 * @return SerialNumber if successful, ERROR otherwise
 *
 */
unsigned int
tipusGetSerialNumber(char **rSN)
{
#ifdef MAPJTAG
  unsigned int rval=0;
  char retSN[10];
  int delay=10000;

  memset(retSN,0,sizeof(retSN));

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->reset,TIPUS_RESET_JTAG);           /* reset */
  usleep(delay);
  tipusJTAGWrite(0x83c,0);     /* Reset_idle */
  usleep(delay);
  tipusJTAGWrite(0xbec,0xFD); /* load the UserCode Enable */
  usleep(delay);
  tipusJTAGWrite(0xfdc,0);   /* shift in 32-bit of data */
  usleep(delay);
  rval = tipusJTAGRead(0x800);
  TIPUSUNLOCK;

  if(rSN!=NULL)
    {
      sprintf(retSN,"TI-%d",rval&0x7ff);
      strcpy((char *)rSN,retSN);
    }


  printf("%s: TI Serial Number is %s (0x%08x)\n",
	 __FUNCTION__,retSN,rval);

  return rval;
#else
  printf("%s: ERROR: JTAG Not supported\n",
	 __func__);
  return ERROR;
#endif /* MAPJTAG */

}

int
tipusPrintTempVolt()
{
#ifdef MAPJTAG
  unsigned int rval=0;
  unsigned int Temperature;
  int delay=10000;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->reset,TIPUS_RESET_JTAG);           /* reset */
  usleep(delay);

  tipusJTAGWrite(0x3c,0x0); // Reset_idle
  usleep(delay);

  tipusJTAGWrite(0x26c,0x3f7); // load the UserCode Enable
  usleep(delay);

  tipusJTAGWrite(0x7dc,0x04000000);     // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x1f1c);

  //second read is required to get the correct DRP value
  tipusJTAGWrite(0x7dc,0x04000000);     // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x7dc);

  printf("%s: FPGA silicon temperature readout is %x \n",
	 __FUNCTION__,rval);

  Temperature = 504*((rval >>6) & 0x3ff)/1024-273;
  printf("\tThe temperature is : %d \n", Temperature);

  // maximum temperature readout
  tipusJTAGWrite(0x7dc,0x04200000);    // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x7dc);

  //second read is required to get the correct DRP value
  tipusJTAGWrite(0x7dc,0x04200000);    // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x7dc);

  printf ("%s: FPGA silicon max. temperature readout is %x\n",
	  __FUNCTION__,rval);
  Temperature = 504*((rval >>6) & 0x3ff)/1024-273;
  printf("\tThe max. temperature is : %d \n", Temperature);

  // minimum temperature readout
  tipusJTAGWrite(0x7dc,0x04240000);    // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x7dc);

  //second read is required to get the correct DRP value
  tipusJTAGWrite(0x7dc,0x04240000);    // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x7dc);

  printf("%s: FPGA silicon min. temperature readout is %x\n",
	 __FUNCTION__,rval);
  Temperature = 504*((rval >>6) & 0x3ff)/1024-273;
  printf ("\tThe min. temperature is : %d \n", Temperature);

  TIPUSUNLOCK;
  return OK;

  // VccInt readout
  tipusJTAGWrite(0x7dc,0x04010000);    // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x7dc);

  //second read is required to get the correct DRP value
  tipusJTAGWrite(0x7dc,0x04010000);    // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x7dc);

  printf("%s: FPGA silicon VccInt readout is %x\n",
	 __FUNCTION__,rval);
  Temperature = 3000*((rval >>6) & 0x3ff)/1024;
  printf ("\tThe VccInt is : %d mV \n", Temperature);

  // maximum VccInt readout
  tipusJTAGWrite(0x7dc,0x04210000);    // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x7dc);

  //second read is required to get the correct DRP value
  tipusJTAGWrite(0x7dc,0x04210000);    // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x7dc);
  printf("%s: FPGA silicon Max. VccInt readout is %x\n",
	 __FUNCTION__,rval);
  Temperature = 3000*((rval >>6) & 0x3ff)/1024;
  printf("\tThe Max. VccInt is : %d mV \n", Temperature);

  // minimum VccInt readout
  tipusJTAGWrite(0x7dc,0x04250000);    // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x7dc);

  //second read is required to get the correct DRP value
  tipusJTAGWrite(0x7dc,0x04250000);    // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x7dc);
  printf("%s: FPGA silicon Min. VccInt readout is %x\n",
	 __FUNCTION__,rval);
  Temperature = 3000*((rval >>6) & 0x3ff)/1024;
  printf("\tThe Min. VccInt is : %d mV \n", Temperature);

  // VccAux readout
  tipusJTAGWrite(0x7dc,0x04020000);    // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x7dc);

  //second read is required to get the correct DRP value
  tipusJTAGWrite(0x7dc,0x04020000);    // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x7dc);

  printf("%s: FPGA silicon VccAux readout is %x\n",
	 __FUNCTION__,rval);
  Temperature = 3000*((rval >>6) & 0x3ff)/1024;
  printf("\tThe VccAux is : %d mV \n", Temperature);

  // maximum VccAux readout
  tipusJTAGWrite(0x7dc,0x04220000);    // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x7dc);

  //second read is required to get the correct DRP value
  tipusJTAGWrite(0x7dc,0x04220000);    // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x7dc);

  printf("%s: FPGA silicon Max. VccAux readout is %x\n",
	 __FUNCTION__,rval);
  Temperature = 3000*((rval >>6) & 0x3ff)/1024;
  printf("\tThe Max. VccAux is : %d mV \n", Temperature);

  // minimum VccAux readout
  tipusJTAGWrite(0x7dc,0x04260000);    // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x7dc);

  //second read is required to get the correct DRP value
  tipusJTAGWrite(0x7dc,0x04260000);    // shift in 32-bit of data
  usleep(delay);
  rval = tipusJTAGRead(0x7dc);

  printf("%s: FPGA silicon Min. VccAux readout is %x\n",
	 __FUNCTION__,rval);
  Temperature = 3000*((rval >>6) & 0x3ff)/1024;
  printf("\tThe Min. VccAux is : %d mV \n", Temperature);

  TIPUSUNLOCK;

  return OK;
#else
  printf("%s: ERROR: JTAG Not supported\n",
	 __func__);
  return ERROR;
#endif /* MAPJTAG */
}

/**
 * @ingroup MasterConfig
 * @brief Resync the 250 MHz Clock
 *
 * @return OK if successful, ERROR otherwise
 *
 */
int
tipusClockResync()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }


  TIPUSLOCK;
  tipusWrite(&TIPUSp->syncCommand,TIPUS_SYNCCOMMAND_AD9510_RESYNC);
  TIPUSUNLOCK;

  printf ("%s: \n\t AD9510 ReSync ! \n",__FUNCTION__);
  return OK;

}

/**
 * @ingroup Config
 * @brief Perform a soft reset of the TI
 *
 * @return OK if successful, ERROR otherwise
 *
 */
int
tipusReset()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->reset,TIPUS_RESET_SOFT);
  TIPUSUNLOCK;
  return OK;
}

/**
 * @ingroup Config
 * @brief Set the crate ID
 *
 * @return OK if successful, ERROR otherwise
 *
 */
int
tipusSetCrateID(unsigned int crateID)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(crateID>0xff)
    {
      printf("%s: ERROR: Invalid crate id (0x%x)\n",__FUNCTION__,crateID);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->boardID,
	     (tipusRead(&TIPUSp->boardID) & ~TIPUS_BOARDID_CRATEID_MASK)  | crateID);
  TIPUSUNLOCK;

  /*sergey
  printf("tipusSetCrateID: set crateID=%d\n",crateID);
  usleep(100);
  printf("tipusSetCrateID: get crateID=%d\n",tipusRead(&TIPUSp->boardID)&TIPUS_BOARDID_CRATEID_MASK);
  sergey*/

  return OK;

}

/**
 * @ingroup Status
 * @brief Get the crate ID of the selected port
 *
 * @param  port
 *       - 0 - Self
 *       - 1 - Fiber port 1 (If Master)
 *
 * @return port Crate ID if successful, ERROR otherwise
 *
 */
int
tipusGetCrateID(int port)
{
  int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if((port<0) || (port>1))
    {
      printf("%s: ERROR: Invalid port (%d)\n",
	     __FUNCTION__,port);
    }

  TIPUSLOCK;
  if(port==0)
    {
      rval = (tipusRead(&TIPUSp->master_tiID) & TIPUS_ID_CRATEID_MASK)>>8;
	  printf("tipusGetCrateID(self)=%d\n",rval);
    }
  else
    {
      rval = (tipusRead(&TIPUSp->hfbr_tiID[port-1]) & TIPUS_ID_CRATEID_MASK)>>8;
	  printf("tipusGetCrateID(port=%d)=%d\n",port,rval);
    }
  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Get the trigger sources enabled bits of the selected port
 *
 * @param  port
 *       - 0 - Self
 *       - 1-8 - Fiber port 1-8  (If Master)
 *
 * @return bitmask of rigger sources enabled if successful, otherwise ERROR
 *         bitmask
 *         - 0 - P0
 *         - 1 - Fiber 1
 *         - 2 - Loopback
 *         - 3 - TRG (FP)
 *         - 4  - VME
 *         - 5 - TS Inputs (FP)
 *         - 6 - TS (rev 2)
 *         - 7 - Internal Pulser
 *
 */
int
tipusGetPortTrigSrcEnabled(int port)
{
  int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if((port<0) || (port>8))
    {
      printf("%s: ERROR: Invalid port (%d)\n",
	     __FUNCTION__,port);
    }

  TIPUSLOCK;
  if(port==0)
    {
      rval = (tipusRead(&TIPUSp->master_tiID) & TIPUS_ID_TRIGSRC_ENABLE_MASK);
    }
  else
    {
      rval = (tipusRead(&TIPUSp->hfbr_tiID[port-1]) & TIPUS_ID_TRIGSRC_ENABLE_MASK);
    }
  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Get the blocklevel of the TI-Slave on the selected port
 * @param port
 *       - 1 - Fiber port 1
 *
 * @return port blocklevel if successful, ERROR otherwise
 *
 */
int
tipusGetSlaveBlocklevel(int port)
{
  int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(port!=1)
    {
      printf("%s: ERROR: Invalid port (%d)\n",
	     __FUNCTION__,port);
    }

  TIPUSLOCK;
  rval = (tipusRead(&TIPUSp->hfbr_tiID[port-1]) & TIPUS_ID_BLOCKLEVEL_MASK)>>16;
  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup MasterConfig
 * @brief Set the number of events per block
 * @param blockLevel Number of events per block
 * @return OK if successful, ERROR otherwise
 *
 */
int
tipusSetBlockLevel(int blockLevel)
{
  return tipusBroadcastNextBlockLevel(blockLevel);
}

/**
 * @ingroup MasterConfig
 * @brief Broadcast the next block level (to be changed at the end of
 * the next sync event, or during a call to tiSyncReset(1).
 *
 * @see tiSyncReset(1)
 * @param blockLevel block level to broadcats
 *
 * @return OK if successful, ERROR otherwise
 *
 */

int
tipusBroadcastNextBlockLevel(int blockLevel)
{
  unsigned int trigger=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if( (blockLevel>TIPUS_BLOCKLEVEL_MASK) || (blockLevel==0) )
    {
      printf("%s: ERROR: Invalid Block Level (%d)\n",__FUNCTION__,blockLevel);
      return ERROR;
    }

  if(!tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Master.\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  trigger = tipusRead(&TIPUSp->trigsrc);

  if(!(trigger & TIPUS_TRIGSRC_VME)) /* Turn on the VME trigger, if not enabled */
    tipusWrite(&TIPUSp->trigsrc, TIPUS_TRIGSRC_VME | trigger);

  tipusWrite(&TIPUSp->triggerCommand, TIPUS_TRIGGERCOMMAND_SET_BLOCKLEVEL | blockLevel);

  if(!(trigger & TIPUS_TRIGSRC_VME)) /* Turn off the VME trigger, if it was initially disabled */
    tipusWrite(&TIPUSp->trigsrc, trigger);

  TIPUSUNLOCK;

  tipusGetNextBlockLevel();

  return OK;

}

/**
 * @ingroup Status
 * @brief Get the block level that will be updated on the end of the block readout.
 *
 * @return Next Block Level if successful, ERROR otherwise
 *
 */

int
tipusGetNextBlockLevel()
{
  unsigned int reg_bl=0;
  int bl=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  reg_bl = tipusRead(&TIPUSp->blocklevel);
  bl = (reg_bl & TIPUS_BLOCKLEVEL_RECEIVED_MASK)>>24;
  tipusNextBlockLevel = bl;

  tipusBlockLevel = (reg_bl & TIPUS_BLOCKLEVEL_CURRENT_MASK)>>16;
  TIPUSUNLOCK;

  return bl;
}

/**
 * @ingroup Status
 * @brief Get the current block level
 *
 * @return Next Block Level if successful, ERROR otherwise
 *
 */
int
tipusGetCurrentBlockLevel()
{
  unsigned int reg_bl=0;
  int bl=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  reg_bl = tipusRead(&TIPUSp->blocklevel);
  bl = (reg_bl & TIPUS_BLOCKLEVEL_CURRENT_MASK)>>16;
  tipusBlockLevel = bl;
  tipusNextBlockLevel = (reg_bl & TIPUS_BLOCKLEVEL_RECEIVED_MASK)>>24;
  TIPUSUNLOCK;

  return bl;
}

/**
 * @ingroup Config
 * @brief Set TS to instantly change blocklevel when broadcast is received.
 *
 * @param enable Option to enable or disable this feature
 *       - 0: Disable
 *        !0: Enable
 *
 * @return OK if successful, ERROR otherwise
 *
 */
int
tipusSetInstantBlockLevelChange(int enable)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  if(enable)
    tipusWrite(&TIPUSp->vmeControl,
	       tipusRead(&TIPUSp->vmeControl) | TIPUS_VMECONTROL_BLOCKLEVEL_UPDATE);
  else
    tipusWrite(&TIPUSp->vmeControl,
	       tipusRead(&TIPUSp->vmeControl) & ~TIPUS_VMECONTROL_BLOCKLEVEL_UPDATE);
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup Status
 * @brief Get Status of instant blocklevel change when broadcast is received.
 *
 * @return 1 if enabled, 0 if disabled , ERROR otherwise
 *
 */
int
tipusGetInstantBlockLevelChange()
{
  int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = (tipusRead(&TIPUSp->vmeControl) & TIPUS_VMECONTROL_BLOCKLEVEL_UPDATE)>>21;
  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Set the trigger source
 *     This routine will set a library variable to be set in the TI registers
 *     at a call to tiIntEnable.
 *
 *  @param trig - integer indicating the trigger source
 *         - 0: P0
 *         - 1: HFBR#1
 *         - 2: Front Panel (TRG)
 *         - 3: Front Panel TS Inputs
 *         - 4: TS (rev2)
 *         - 5: Random
 *         - 6-9: TS Partition 1-4
 *         - 10: HFBR#5
 *         - 11: Pulser Trig 2 then Trig1 after specified delay
 *
 * @return OK if successful, ERROR otherwise
 *
 */
int
tipusSetTriggerSource(int trig)
{
  unsigned int trigenable=0;

  if(TIPUSp==NULL)
  {
    printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
    return ERROR;
  }

  if( (trig>10) || (trig<0) )
  {
    printf("%s: ERROR: Invalid Trigger Source (%d).  Must be between 0 and 10.\n",__FUNCTION__,trig);
    return ERROR;
  }


  if(!tipusMaster)
  {
    /* Setup for TI Slave */
    trigenable = 0; //sergey: was 'trigenable = TIPUS_TRIGSRC_VME;'

    if((trig>=6) && (trig<=9)) /* TS partition specified */
    {

      /* sergey: copied from tiLib.c, not sure if applicable here
      if(tipusSlaveFiberIn!=1)
      {
	printf("%s: WARN: Partition triggers NOT USED on Fiber Port 5.\n",__FUNCTION__);
        trigenable |= TIPUS_TRIGSRC_HFBR5;
      }
      */

      trigenable |= TIPUS_TRIGSRC_HFBR1;
      switch(trig)
      {
	case TIPUS_TRIGGER_PART_1:
	  trigenable |= TIPUS_TRIGSRC_PART_1;
	  break;

	case TIPUS_TRIGGER_PART_2:
	  trigenable |= TIPUS_TRIGSRC_PART_2;
	  break;

	case TIPUS_TRIGGER_PART_3:
	  trigenable |= TIPUS_TRIGSRC_PART_3;
	  break;

	case TIPUS_TRIGGER_PART_4:
	  trigenable |= TIPUS_TRIGSRC_PART_4;
	  break;
      }
    }
    else
    {
      if(tipusSlaveFiberIn==1)
      {
	trigenable |= TIPUS_TRIGSRC_HFBR1;
      }
      else if(tipusSlaveFiberIn==5)
      {
	trigenable |= TIPUS_TRIGSRC_HFBR5;
      }

      if( ((trig != TIPUS_TRIGGER_HFBR1) && (tipusSlaveFiberIn == 1)) ||
	  ((trig != TIPUS_TRIGGER_HFBR5) && (tipusSlaveFiberIn == 5)) )
      {
	printf("%s: WARN:  Only valid trigger source for TI Slave is HFBR%d (trig = %d)",
		 __FUNCTION__, tipusSlaveFiberIn,
		 (tipusSlaveFiberIn==1)?TIPUS_TRIGGER_HFBR1:TIPUS_TRIGGER_HFBR5);
	printf("  Ignoring specified trig (%d)\n",trig);
      }
    }
  }
  else
  {
    /* Setup for TI Master */

    /* Set VME and Loopback by default */
    trigenable  = TIPUS_TRIGSRC_VME;
    trigenable |= TIPUS_TRIGSRC_LOOPBACK;

    switch(trig)
    {
      case TIPUS_TRIGGER_P0:
	trigenable |= TIPUS_TRIGSRC_P0;
        break;

      case TIPUS_TRIGGER_HFBR1:
	trigenable |= TIPUS_TRIGSRC_HFBR1;
        break;

      case TIPUS_TRIGGER_FPTRG:
	trigenable |= TIPUS_TRIGSRC_FPTRG;
        break;

      case TIPUS_TRIGGER_TSINPUTS:
	trigenable |= TIPUS_TRIGSRC_TSINPUTS;
        break;

      case TIPUS_TRIGGER_TSREV2:
	trigenable |= TIPUS_TRIGSRC_TSREV2;
        break;

      case TIPUS_TRIGGER_PULSER:
	trigenable |= TIPUS_TRIGSRC_PULSER;
        break;

      case TIPUS_TRIGGER_TRIG21:
	trigenable |= TIPUS_TRIGSRC_PULSER;
        trigenable |= TIPUS_TRIGSRC_TRIG21;
	break;

      default:
	printf("%s: ERROR: Invalid Trigger Source (%d) for TI Master\n",__FUNCTION__,trig);
	return ERROR;
    }
  }

  tipusTriggerSource = trigenable;
  printf("%s: INFO: tipusTriggerSource = 0x%x\n",__FUNCTION__,tipusTriggerSource);

  return OK;
}


/**
 * @ingroup Config
 * @brief Set trigger sources with specified trigmask
 *    This routine is for special use when tiSetTriggerSource(...) does
 *    not set all of the trigger sources that is required by the user.
 *
 * @param trigmask bits:
 *        -         0:  P0
 *        -         1:  HFBR #1
 *        -         2:  TI Master Loopback
 *        -         3:  Front Panel (TRG) Input
 *        -         4:  VME Trigger
 *        -         5:  Front Panel TS Inputs
 *        -         6:  TS (rev 2) Input
 *        -         7:  Random Trigger
 *        -         8:  FP/Ext/GTP
 *        -         9:  P2 Busy
 *        -        10:  HFBR #5
 *        -        11:  Pulser Trig2 with delayed Trig1 (only compatible with 2 and 7)
 *
 * @return OK if successful, ERROR otherwise
 *
 */
int
tipusSetTriggerSourceMask(int trigmask)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  /* Check input mask */
  if(trigmask>TIPUS_TRIGSRC_SOURCEMASK)
    {
      printf("%s: ERROR: Invalid trigger source mask (0x%x).\n",
	     __FUNCTION__,trigmask);
      return ERROR;
    }

  tipusTriggerSource = trigmask;

  return OK;
}

/**
 * @ingroup Config
 * @brief Enable trigger sources
 * Enable trigger sources set by
 *                          tiSetTriggerSource(...) or
 *                          tiSetTriggerSourceMask(...)
 * @sa tiSetTriggerSource
 * @sa tiSetTriggerSourceMask
 *
 * @return OK if successful, ERROR otherwise
 *
 */
int
tipusEnableTriggerSource()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(tipusTriggerSource==0)
    {
      printf("%s: WARN: No Trigger Sources Enabled\n",__FUNCTION__);
    }
  else
    {
      printf("%s: INFO: Writing Trigger Sources %d into hardware\n",__FUNCTION__,tipusTriggerSource);
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->trigsrc, tipusTriggerSource);
  TIPUSUNLOCK;

  return OK;

}

/**
 * @ingroup Config
 * @brief Force TI to send trigger source enabled bits to TI-master or TD
 *
 * @return OK if successful, ERROR otherwise
 *
 */
int
tipusForceSendTriggerSourceEnable()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->trigsrc,
	     (tipusRead(&TIPUSp->trigsrc) & TIPUS_TRIGSRC_SOURCEMASK) |
	     TIPUS_TRIGSRC_FORCE_SEND);
  TIPUSUNLOCK;

  return OK;

}


/**
 * @ingroup Config
 * @brief Disable trigger sources
 *
 * @param fflag
 *   -  0: Disable Triggers
 *   - >0: Disable Triggers and generate enough triggers to fill the current block
 *
 * @return OK if successful, ERROR otherwise
 *
 */
int
tipusDisableTriggerSource(int fflag)
{
  int regset=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;

  if(tipusMaster)
    regset = TIPUS_TRIGSRC_LOOPBACK;

  tipusWrite(&TIPUSp->trigsrc,regset);

  TIPUSUNLOCK;

  /*sergey: do not do that in hallb !!!
  if(fflag && tipusMaster)
  {
    tipusFillToEndBlock();
  }
  */

  return OK;

}

/**
 * @ingroup Config
 * @brief Set the Sync source mask
 *
 * @param sync - MASK indicating the sync source
 *       bit: description
 *       -  0: P0
 *       -  1: HFBR1
 *       -  2: HFBR5
 *       -  3: FP
 *       -  4: LOOPBACK
 *
 * @return OK if successful, ERROR otherwise
 *
 */
int
tipusSetSyncSource(unsigned int sync)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(sync>TIPUS_SYNC_SOURCEMASK)
    {
      printf("%s: ERROR: Invalid Sync Source Mask (%d).\n",
	     __FUNCTION__,sync);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->sync,sync);
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup Config
 * @brief Set the event format
 *
 * @param format - integer number indicating the event format
 *          - 0: 32 bit event number only
 *          - 1: 32 bit event number + 32 bit timestamp
 *          - 2: 32 bit event number + higher 16 bits of timestamp + higher 16 bits of eventnumber
 *          - 3: 32 bit event number + 32 bit timestamp
 *              + higher 16 bits of timestamp + higher 16 bits of eventnumber
 *
 * @return OK if successful, ERROR otherwise
 *
 */
int
tipusSetEventFormat(int format)
{
  unsigned int formatset=0;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if( (format>3) || (format<0) )
    {
      printf("%s: ERROR: Invalid Event Format (%d).  Must be between 0 and 3.\n",
	     __FUNCTION__,format);
      return ERROR;
    }

  TIPUSLOCK;

  formatset = tipusRead(&TIPUSp->dataFormat)
    & ~(TIPUS_DATAFORMAT_TIMING_WORD | TIPUS_DATAFORMAT_HIGHERBITS_WORD);

  switch(format)
    {
    case 0:
      break;

    case 1:
      formatset |= TIPUS_DATAFORMAT_TIMING_WORD;
      break;

    case 2:
      formatset |= TIPUS_DATAFORMAT_HIGHERBITS_WORD;
      break;

    case 3:
      formatset |= (TIPUS_DATAFORMAT_TIMING_WORD | TIPUS_DATAFORMAT_HIGHERBITS_WORD);
      break;

    }

  tipusWrite(&TIPUSp->dataFormat,formatset);

  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup Config
 * @brief Set whether or not the latched pattern of FP Inputs in block readout
 *
 * @param enable
 *    - 0: Disable
 *    - >0: Enable
 *
 * @return OK if successful, otherwise ERROR
 *
 */
int
tipusSetFPInputReadout(int enable)
{
  if(TIPUSp == NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  if(enable)
    tipusWrite(&TIPUSp->dataFormat,
	       tipusRead(&TIPUSp->dataFormat) | TIPUS_DATAFORMAT_FPINPUT_READOUT);
  else
    tipusWrite(&TIPUSp->dataFormat,
	       tipusRead(&TIPUSp->dataFormat) & ~TIPUS_DATAFORMAT_FPINPUT_READOUT);
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup MasterConfig
 * @brief Set and enable the "software" trigger
 *
 *  @param trigger  trigger type 1 or 2 (playback trigger)
 *  @param nevents  integer number of events to trigger
 *  @param period_inc  period multipuslier, depends on range (0-0x7FFF)
 *  @param range
 *     - 0: small period range (min: 120ns, increments of 120ns)
 *     - 1: large period range (min: 120ns, increments of 245.7us)
 *
 * @return OK if successful, ERROR otherwise
 *
 */
int
tipusSoftTrig(int trigger, unsigned int nevents, unsigned int period_inc, int range)
{
  unsigned int periodMax=(TIPUS_FIXEDPULSER1_PERIOD_MASK>>16);
  unsigned int reg=0;
  unsigned int time=0;

  if(TIPUSp==NULL)
    {
      printf("\ntiSoftTrig: ERROR: TI not initialized\n");
      return ERROR;
    }

  if(trigger!=1 && trigger!=2)
    {
      printf("\ntiSoftTrig: ERROR: Invalid trigger type %d\n",trigger);
      return ERROR;
    }

  if(nevents>TIPUS_FIXEDPULSER1_NTRIGGERS_MASK)
    {
      printf("\ntiSoftTrig: ERROR: nevents (%d) must be less than %d\n",nevents,
	     TIPUS_FIXEDPULSER1_NTRIGGERS_MASK);
      return ERROR;
    }
  if(period_inc>periodMax)
    {
      printf("\ntiSoftTrig: ERROR: period_inc (%d) must be less than %d ns\n",
	     period_inc,periodMax);
      return ERROR;
    }
  if( (range!=0) && (range!=1) )
    {
      printf("\ntiSoftTrig: ERROR: range (%d) must be 0 or 1\n",range);
      return ERROR;
    }

  if(range==0)
    time = 32+8*period_inc;
  if(range==1)
    time = 32+8*period_inc*2048;

  printf("\ntiSoftTrig: INFO: Setting software trigger for %d nevents with period of %.1f\n",
	 nevents,((float)time)/(1000.0));

  reg = (range<<31)| (period_inc<<16) | (nevents);
  TIPUSLOCK;
  if(trigger==1)
    {
      tipusWrite(&TIPUSp->fixedPulser1, reg);
    }
  else if(trigger==2)
    {
      tipusWrite(&TIPUSp->fixedPulser2, reg);
    }
  TIPUSUNLOCK;

  return OK;

}


/**
 * @ingroup MasterConfig
 * @brief Set the parameters of the random internal trigger
 *
 * @param trigger  - Trigger Selection
 *       -              1: trig1
 *       -              2: trig2
 * @param setting  - frequency prescale from 500MHz
 *
 * @sa tiDisableRandomTrigger
 * @return OK if successful, ERROR otherwise.
 *
 */
int
tipusSetRandomTrigger(int trigger, int setting)
{
  double rate;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(trigger!=1 && trigger!=2)
    {
      printf("\ntiSetRandomTrigger: ERROR: Invalid trigger type %d\n",trigger);
      return ERROR;
    }

  if(setting>TIPUS_RANDOMPULSER_TRIG1_RATE_MASK)
    {
      printf("%s: ERROR: setting (0x%x) must be less than 0x%x\n",
	     __FUNCTION__,setting,TIPUS_RANDOMPULSER_TRIG1_RATE_MASK);
      return ERROR;
    }

  if(setting>0)
    rate = ((double)500000) / ((double) (2<<(setting-1)));
  else
    rate = ((double)500000);

  printf("%s: Enabling random trigger (trig%d) at rate (kHz) = %.2f\n",
	 __FUNCTION__,trigger,rate);

  TIPUSLOCK;
  if(trigger==1)
    tipusWrite(&TIPUSp->randomPulser,
	       setting | (setting<<4) | TIPUS_RANDOMPULSER_TRIG1_ENABLE);
  else if (trigger==2)
    tipusWrite(&TIPUSp->randomPulser,
	       (setting | (setting<<4))<<8 | TIPUS_RANDOMPULSER_TRIG2_ENABLE );

  TIPUSUNLOCK;

  return OK;
}


/**
 * @ingroup MasterConfig
 * @brief Disable random trigger generation
 * @sa tiSetRandomTrigger
 * @return OK if successful, ERROR otherwise.
 */
int
tipusDisableRandomTrigger()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->randomPulser,0);
  TIPUSUNLOCK;
  return OK;
}

/**
 * @ingroup Readout
 * @brief Read a block of events from the TI FIFO
 *
 * @param   data  - local memory address to place data
 * @param   nwrds - Max number of words to transfer
 * @param   rflag - Readout Flag
 *       -       0 - programmed I/O from the specified board
 *       -       1 - DMA transfer
 *
 * @return Number of words transferred to data if successful, ERROR otherwise
 *
 */
int
readfifo()
{
  int ii;
  int dCnt, wCnt=0;
  volatile unsigned int val;
  static int bump = 0;
  int blocksize = 0x1000;
  unsigned long newaddr=0;
  int tipusTriedAgain=0;

  if(TIPUSp==NULL)
    {
      printf("\n%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  int blocklevel=0;
  int iev=0, idata=0;
  int ntrig = 0, trigwords = 0;
  int trailerFound = 0;
  dCnt = 0;

  for(ii=0; ii<20; ii++)
  {
    val = tipusRead(&TIPUSp->fifo);
    printf("fifo[%2d]=0x%08x\n",ii,val);
  }

  TIPUSUNLOCK;
  return(0);
}

int
tipusFifoReadBlock(volatile unsigned int *data, int nwrds, int rflag)
{
  int ii;
  int dCnt, wCnt=0;
  volatile unsigned int val;
  static int bump = 0;
  int blocksize = 0x1000;
  unsigned long newaddr=0;
  int tipusTriedAgain=0;

  if(TIPUSp==NULL)
  {
    printf("\n%s: ERROR: TI not initialized\n",__FUNCTION__);
    return ERROR;
  }

  if(data==NULL)
  {
    printf("\n%s: ERROR: Invalid Destination address\n",__FUNCTION__);
    return(ERROR);
  }

  TIPUSLOCK;
  int blocklevel=0;
  int iev=0, idata=0;
  int ntrig = 0, trigwords = 0;
  int trailerFound = 0;
  dCnt = 0;


  /*sergey
  for(ii=0; ii<20; ii++)
  {
    val = tipusRead(&TIPUSp->fifo);
    printf("fifo[%2d]=0x%08x\n",ii,val);
  }
  return(0);
  sergey*/




  /* Read Block header - should be first word */
  val = tipusRead(&TIPUSp->fifo);
  //printf("val0=0x%08x\n",val);
  data[dCnt++] = val;

  if((val & TIPUS_DATA_TYPE_DEFINE_MASK) && ((val & TIPUS_WORD_TYPE_MASK) == TIPUS_BLOCK_HEADER_WORD_TYPE))
  {
    ntrig = val & TIPUS_DATA_BLKLEVEL_MASK;

    /* Next word is the CODA 3.0 header */
    val = tipusRead(&TIPUSp->fifo);
    //printf("val1=0x%08x\n",val);
    data[dCnt++] = val;

    if( ((val & 0xFF100000)>>16 == 0xFF10) && ((val & 0xFF00)>>8 == 0x20) )
    {
      blocklevel = val & TIPUS_DATA_BLKLEVEL_MASK;

      if((blocklevel & 0xff) != ntrig)
      {
	printf("\n%s: ERROR: TI Blocklevel %d inconsistent with TI Trigger Bank Header (0x%08x)",
		     __func__, ntrig, val);
	      // return?

      }

      /* Loop through each event in the block */
      for(iev=0; iev<blocklevel; iev++)
      {
	/* Event header */
	TRYAGAIN:
	  val = tipusRead(&TIPUSp->fifo);
          //printf("val2=0x%08x\n",val);
	  data[dCnt++] = val;

	  if((val & 0xFF0000)>>16 == 0x01)
	  {
	    trigwords = val & 0xffff;
	    for(idata=0; idata<trigwords; idata++)
	    {
	      val = tipusRead(&TIPUSp->fifo);
              //printf("val4=0x%08x\n",val);
	      data[dCnt++] = val;
	    }

	  } /* Event Header test */
	  else
	  {
	    tipusTriedAgain++;
            //sleep(1);
	    usleep(1/*1000*/);
	    if(tipusTriedAgain>20)
	    {
	      printf("%s: ERROR: Invalid Event Header Word 0x%08x\n",
			     __FUNCTION__,val);
	      TIPUSUNLOCK;
	      {
	        int jjj;
	        printf("\n   TILIB: dCnt=%d. Data so far:\n",dCnt);
                for(jjj=0; jjj<=dCnt; jjj++) printf("   [%3d] 0x%08x\n",jjj,data[jjj]);
	        printf("   TILIB: End Of Data.\n");fflush(stdout);
	      }
	      return -1;
	    }
	    goto TRYAGAIN;
          }
      } /* Loop through each event in block */

      /* Read Block Trailer */
      val = tipusRead(&TIPUSp->fifo);
      data[dCnt++] = val;

      if((val & TIPUS_DATA_TYPE_DEFINE_MASK) &&
	 (val & TIPUS_WORD_TYPE_MASK)==TIPUS_BLOCK_TRAILER_WORD_TYPE)
      {
	trailerFound = 1;

	if((dCnt%2)!=0)
	{
	  /* Read out an extra word (filler) in the fifo */
	  val = tipusRead(&TIPUSp->fifo);

	  if(((val & TIPUS_DATA_TYPE_DEFINE_MASK) != TIPUS_DATA_TYPE_DEFINE_MASK) ||
	     ((val & TIPUS_WORD_TYPE_MASK) != TIPUS_FILLER_WORD_TYPE))
	  {
	    printf("\n%s: ERROR: Unexpected word after block trailer (0x%08x)\n",
		   __func__, val);
	  }
	}


      } // Block trailer test */
      else
      {
	printf("%s: ERROR: Invalid Block Trailer Word 0x%08x\n",
		     __FUNCTION__,val);
	TIPUSUNLOCK;
	{
	  int jjj;
	  printf("\n   TILIB: dCnt=%d. Data so far:\n",dCnt);
          for(jjj=0; jjj<=dCnt; jjj++) printf("   [%3d] 0x%08x\n",jjj,data[jjj]);
	  printf("   TILIB: End Of Data.\n");fflush(stdout);
	}

	return ERROR;
      }

    } /* CODA 3.0 header test */
    else
    {
      printf("%s: ERROR: Invalid Trigger Bank Header Word 0x%08x\n",
		 __FUNCTION__,val);
      TIPUSUNLOCK;
      return(ERROR);
    }

  } /* Block Header test */
  else
  {
    printf("%s: ERROR: Invalid Block Header Word 0x%08x\n",
	     __FUNCTION__,val);
    TIPUSUNLOCK;
    {
      int jjj;
      printf("\n   TILIB: dCnt=%d. Data so far:\n",dCnt);
      for(jjj=0; jjj<=dCnt; jjj++) printf("   [%3d] 0x%08x\n",jjj,data[jjj]);
      printf("   TILIB: End Of Data.\n");fflush(stdout);
    }

    return(dCnt);
  }


  TIPUSUNLOCK;
  return(dCnt);
}


/**
 * @ingroup Readout
 * @brief Filter data directly read from PCIe into JLab Block Data form
 *
 * @param   data_in   - local memory address to find raw TIpcieUS data
 * @param   in_nwrds  - number of valid 4byte words in data_in
 * @param   data_out  - local memory address to place filtered data
 * @param   out_nwrds - number of valid 4byte words in data_out
 *
 * @return OK if successful, ERROR otherwise
 *
 */
static int32_t
tipusBlockFilter(volatile uint32_t *data_in, int32_t in_nwrds,
		 volatile uint32_t *data_out, int32_t *out_nwrds)
{
  int32_t rval = OK;

  /* data_in is described in the manual as
     Bit(255:192): fixed as hex 0x71E5,DA7A,5948,6921,
     except the Block trailer, Bit#199 = 1 (0x69A1).
     Bit(191:128): TI data(63:0), two TI data words.  It is the repeat of Bit(63:0).
     Bit(127:64):fixed as hex 0x71E5,DA7A,5948, A521,
     except the Block trailer, Bit#71 = 1 (0xA5A1).
     Bit(63:0): TIdata(63:0), two TI data words
  */
  typedef struct
  {
    uint32_t data[2];
    uint32_t ignore[6];
  } INformat;
  INformat *fmt_in = (INformat *) data_in;
  int32_t fmt_nwords = in_nwrds / 8;

  int32_t idata, nout = 0;
  for(idata = 0; idata < fmt_nwords; idata++)
    {
      data_out[nout++] = fmt_in[idata].data[0];
      data_out[nout++] = fmt_in[idata].data[1];
    }
  *out_nwrds = nout;

  return rval;
}

/**
 * @ingroup Readout
 * @brief Read a block of events from the TI
 *
 * @param   data  - local memory address to place data
 * @param   nwrds - Max number of words to transfer
 * @param   rflag - Readout Flag
 *       -       0 - programmed I/O from the specified board
 *       -       1 - DMA transfer
 *
 * @return Number of words transferred to data if successful, ERROR otherwise
 *
 */
#include <unistd.h>
#define DEVICE_NAME_DEFAULT "/dev/xdma0_c2h_0"
#define RW_MAX_SIZE	0x7ffff000
static int tipusDmaFD = 0;

int
tipusReadBlock(volatile unsigned int *data, int nwrds, int rflag)
{
  static int ncall=0;
  int rval = 0;

  if(tipusUseDma==0) /* Programmed I/O */
    {
      rval = tipusFifoReadBlock(data, nwrds, rflag);
      return rval;
    }

  if(TIPUSp==NULL)
    {
      printf("\n%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(tipusDmaFD < 0)
    {
      printf("\n%s: ERROR: DMA not initialized\n",
	     __func__);
      return ERROR;
    }

  if(data==NULL)
    {
      printf("\n%s: ERROR: Invalid Destination address\n",__FUNCTION__);
      return(ERROR);
    }

  /* estimate max amount of data to expect, from the block level */
  int32_t max_nwrds = ((5*tipusBlockLevel) + 4) * 4;

  if(max_nwrds > tipusBlockDataSize)
    {
      printf("%s: ERROR: max_nwrds > tipusBlockDataSize (0x%x > 0x%x)\n",
	     __func__, max_nwrds, tipusBlockDataSize);

      return ERROR;
    }
  uint64_t size = max_nwrds << 2;
  ssize_t rc;
  uint64_t count = 0;

  /* Use pre-allocated memory */
  char *buf = (char *)tipusBlockData;
  off_t offset = 0;

  TIPUSLOCK;
  while (count < size) {
    uint64_t bytes = size - count;

    if (bytes > RW_MAX_SIZE)
      bytes = RW_MAX_SIZE;

    if (offset) {
#ifdef DMA_DEBUG
      printf("%s(%d): lseek(%d, %d, %d)\n",
	     __func__, ++ncall, tipusDmaFD, offset, SEEK_SET);
#endif
      rc = lseek(tipusDmaFD, offset, SEEK_SET);
      if (rc != offset) {
	perror("seek");
	fprintf(stderr, "%s, seek off 0x%lx != 0x%lx.\n",
		__func__, rc, offset);
	TIPUSUNLOCK;
	return -EIO;
      }
    }

    /* read data from file into memory buffer */
#ifdef DMA_DEBUG
    printf("%s(%d): read(%d, ..., %d)\n",
	   __func__, ncall, tipusDmaFD, bytes);
#endif
    rc = read(tipusDmaFD, buf, bytes);

    if (rc < 0) {
      perror("read");
      fprintf(stderr, "%s, R off 0x%lx, 0x%lx != 0x%lx.\n",
	      __func__, count, rc, bytes);
      TIPUSUNLOCK;
      return -EIO;
    } else if (rc < bytes) {
      count += rc;
      buf += rc;
      offset += rc;
      break;
    } else {
      count += bytes;
      buf += bytes;
      offset += bytes;
    }
  }

  int32_t filter_nwords = 0;
  rval = tipusBlockFilter(tipusBlockData, (count >> 2),
			  data, &filter_nwords);

  TIPUSUNLOCK;
  return filter_nwords;

}

/**
 * @ingroup Config
 *
 * @brief Option to generate a fake trigger bank when
 *        @tipusReadTriggerBlock finds an ERROR.
 *        Enabled by library default.
 *
 * @param enable Enable fake trigger bank if enable != 0.
 *
 * @return OK
 *
 */
int
tipusFakeTriggerBankOnError(int enable)
{
  TIPUSLOCK;
  if(enable)
    tipusFakeTriggerBank = 1;
  else
    tipusFakeTriggerBank = 0;
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup Readout
 * @brief Generate a fake trigger bank.  Called by @tipusReadTriggerBlock if ERROR.
 *
 * @param   data  - local memory address to place data
 *
 * @return Number of words generated to data if successful, ERROR otherwise
 *
 */
int
tipusGenerateTriggerBank(volatile unsigned int *data)
{
  int bl = 0;
  int iword, nwords = 2;
  unsigned int error_tag = 0;
  unsigned int word;

  bl = tipusGetCurrentBlockLevel();
  data[0] = nwords - 1;
  data[1] = 0xFF102000 | (error_tag << 16)| bl;

  return nwords;
}

/**
 * @ingroup Readout
 * @brief Read a block from the TI and form it into a CODA Trigger Bank
 *
 * @param   data  - local memory address to place data
 *
 * @return Number of words transferred to data if successful, ERROR otherwise
 *
 */
int
tipusReadTriggerBlock(volatile unsigned int *data)
{
  int rval=0, nwrds=0, rflag=0;
  int iword=0;
  unsigned int word=0;
  int iblkhead=-1, iblktrl=-1;
  unsigned int block_data[128];

  if(data==NULL)
    {
      printf("\n%s: ERROR: Invalid Destination address\n",
	     __FUNCTION__);
      return(ERROR);
    }

  /* estimate max amount of data to expect, from the block level */
  nwrds = ((5*tipusBlockLevel) + 4) * 4;

  /* Obtain the trigger bank by just making a call the tipusReadBlock */
  rval = tipusReadBlock(data, nwrds, 0);

  if(rval < 0)
    {
      /* Error occurred */
      printf("%s: ERROR: tipusReadBlock returned ERROR\n",
	     __FUNCTION__);

      if(tipusFakeTriggerBank)
	return tipusGenerateTriggerBank(data);
      else
	return ERROR;
    }
  else if (rval == 0)
    {
      /* No data returned */
      printf("%s: WARN: No data available\n",
	     __FUNCTION__);

      if(tipusFakeTriggerBank)
	return tipusGenerateTriggerBank(data);
      else
	return 0;
    }

  /* Work down to find index of block header */
  while(iword<rval)
    {

      word = data[iword];

      if(word & TIPUS_DATA_TYPE_DEFINE_MASK)
	{
	  if(((word & TIPUS_WORD_TYPE_MASK)) == TIPUS_BLOCK_HEADER_WORD_TYPE)
	    {
	      iblkhead = iword;
	      break;
	    }
	}
      iword++;
    }

  /* Check if the index is valid */
  if(iblkhead < 0)
    {
      printf("%s: ERROR: Invalid TI Block Header (index = %d) \n",
	     __func__, iblkhead);

      if(tipusFakeTriggerBank)
	return tipusGenerateTriggerBank(data);
      else
	return ERROR;
    }

  /* Work up to find index of block trailer */
  iword=rval-1;
  while(iword>=0)
    {

      word = data[iword];
      if(word & TIPUS_DATA_TYPE_DEFINE_MASK)
	{
	  if(((word & TIPUS_WORD_TYPE_MASK)) == TIPUS_BLOCK_TRAILER_WORD_TYPE)
	    {
#ifdef CDEBUG
	      printf("%s: block trailer? 0x%08x\n",
		     __FUNCTION__,word);
#endif
	      iblktrl = iword;
	      break;
	    }
	}
      iword--;
    }

  /* Check if the index is valid */
  if(iblktrl == -1)
    {
      printf("%s: ERROR: Failed to find TI Block Trailer\n",
	     __FUNCTION__);

      if(tipusFakeTriggerBank)
	return tipusGenerateTriggerBank(data);
      else
	return ERROR;
    }

  /* Get the block trailer, and check the number of words contained in it */
  word = data[iblktrl];
#ifdef CDEBUG
  printf("  iblkhead = %d   iblktrl = %d  \n",
	 iblkhead, iblktrl);
#endif
  tipusBlockSyncFlag = (word & TIPUS_BLOCK_TRAILER_SYNCEVENT_FLAG) ? 1 : 0;

  if((iblktrl - iblkhead + 1) != (word & TIPUS_BLOCK_TRAILER_WORD_COUNT_MASK))
    {
      printf("%s: Number of words inconsistent (index count = %d, block trailer count = %d\n",
	     __FUNCTION__,
	     (iblktrl - iblkhead + 1), word & TIPUS_BLOCK_TRAILER_WORD_COUNT_MASK);
      printf("  iblkhead = %d   iblktrl = %d  \n",
	     iblkhead, iblktrl);

      if(tipusFakeTriggerBank)
	return tipusGenerateTriggerBank(data);
      else
	return ERROR;
    }

  /* Modify the total words returned */
  rval = iblktrl - iblkhead;

  /* Write in the Trigger Bank Length */
  data[iblkhead] = rval-1;

  return rval;

}

/**
 * @ingroup Readout
 * @brief Return the value of the sync event flag from the previous call to
 *        tiReadTriggerBlock
 *
 * @return tiBlockSyncFlag if successful, ERROR otherwise
 *
 */
int
tipusGetBlockSyncFlag()
{
  return tipusBlockSyncFlag;
}


int
tipusCheckTriggerBlock(volatile unsigned int *data)
{
  unsigned int blen=0, blevel=0, evlen=0;
  int iword=0, iev=0, ievword=0;
  int rval=OK;

  printf("--------------------------------------------------------------------------------\n");
  /* First word should be the trigger bank length */
  blen = data[iword];
  printf("%4d: %08X - TRIGGER BANK LENGTH - len = %d\n",iword, data[iword], blen);
  iword++;

  /* Trigger Bank Header */
  if( ((data[iword] & 0xFF100000)>>16 != 0xFF10) ||
      ((data[iword] & 0x0000FF00)>>8 != 0x20) )
    {
      rval = ERROR;
      printf("%4d: %08X - **** INVALID TRIGGER BANK HEADER ****\n",
	     iword,
	     data[iword]);
      iword++;
      while(iword<blen+1)
	{
	  if(iword>blen)
	    {
	      rval = ERROR;
	      printf("----: **** ERROR: Data continues past Trigger Bank Length (%d) ****\n",blen);
	    }
	  printf("%4d: %08X - **** REST OF DATA ****\n",
		 iword,
		 data[iword]);
	  iword++;
	}
    }
  else
    {
      if(iword>blen)
	{
	  rval = ERROR;
	  printf("----: **** ERROR: Data continues past Trigger Bank Length (%d) ****\n",blen);
	}
      blevel = data[iword] & 0xFF;
      printf("%4d: %08X - TRIGGER BANK HEADER - type = %d  blocklevel = %d\n",
	     iword,
	     data[iword],
	     (data[iword] & 0x000F0000)>>16,
	     blevel);
      iword++;

      for(iev=0; iev<blevel; iev++)
	{
	  if(iword>blen)
	    {
	      rval = ERROR;
	      printf("----: **** ERROR: Data continues past Trigger Bank Length (%d) ****\n",blen);
	    }

	  if((data[iword] & 0x00FF0000)>>16!=0x01)
	    {
	      rval = ERROR;
	      printf("%4d: %08x - **** INVALID EVENT HEADER ****\n",
		     iword, data[iword]);
	      iword++;
	      while(iword<blen+1)
		{
		  printf("%4d: %08X - **** REST OF DATA ****\n",
			 iword,
			 data[iword]);
		  iword++;
		}
	      break;
	    }
	  else
	    {
	      if(iword>blen)
		{
		  rval = ERROR;
		  printf("----: **** ERROR: Data continues past Trigger Bank Length (%d) ****\n",blen);
		}

	      evlen = data[iword] & 0x0000FFFF;
	      printf("%4d: %08x - EVENT HEADER - trigtype = %d  len = %d\n",
		     iword,
		     data[iword],
		     (data[iword] & 0xFF000000)>>24,
		     evlen);
	      iword++;

	      if(iword>blen)
		{
		  rval = ERROR;
		  printf("----: **** ERROR: Data continues past Trigger Bank Length (%d) ****\n",blen);
		}

	      printf("%4d: %08x - EVENT NUMBER - evnum = %d\n",
		     iword,
		     data[iword],
		     data[iword]);
	      iword++;
	      for(ievword=1; ievword<evlen; ievword++)
		{
		  if(iword>blen)
		    {
		      rval = ERROR;
		      printf("----: **** ERROR: Data continues past Trigger Bank Length (%d) ****\n",blen);
		    }
		  printf("%4d: %08X - EVENT DATA\n",
			 iword,
			 data[iword]);
		  iword++;
		}
	    }
	}
    }

  printf("--------------------------------------------------------------------------------\n");
  return rval;
}

int
tipusDecodeTriggerType(volatile unsigned int *data, int data_len, int event)
{
  int rval = -1;
  int iword = 0;
  int blocklevel = -1;
  int event_len = -1;
  int ievent = 1;
  int trigger_type = -1;
  unsigned int dataword = 0;

  /* Loop until we find the trigger bank */
  while(iword < data_len)
    {
      dataword = data[iword];

      if( ((dataword & 0xFF100000)>>16 == 0xFF10) &&
	  ((dataword & 0x0000FF00)>>8 == 0x20) )
	{
	  blocklevel =  dataword & 0xFF;
	  iword++;
	  break;
	}
      iword++;
    }

  if(blocklevel == -1)
    {
      printf("%s: ERROR: Failed to find Trigger Bank header\n",
	     __func__);
      return ERROR;
    }

  if(event > blocklevel)
    {
      printf("%s: ERROR: event (%d) greater than blocklevel (%d)\n",
	     __func__, event, blocklevel);
      return ERROR;
    }

  /* Loop until we get to the event requested */
  while((iword < data_len) && (ievent <= blocklevel))
    {
      dataword = data[iword];

      if((dataword & 0x00FF0000)>>16 == 0x01)
	{
	  trigger_type = (dataword & 0xFF000000) >> 24;
	  if(ievent == event)
	    {
	      rval = trigger_type;
	      break;
	    }
	  event_len = dataword & 0xFFFF;
	  ievent++;
	  iword += event_len + 1;
	}
      else
	{
	  /* we're lost... just increment */
	  iword++;
	}
    }

  if(rval == -1)
    {
      printf("%s: ERROR: Failed to find trigger type for event %d\n",
	     __func__, event);
    }

  return rval;
}

/**
 * @ingroup Config
 * @brief Enable Fiber transceiver
 *
 *  Note:  All Fiber are enabled by default
 *         (no harm, except for 1-2W power usage)
 *
 * @sa tiDisableFiber
 * @param   fiber: integer indicative of the transceiver to enable
 *
 *
 * @return OK if successful, ERROR otherwise.
 *
 */
int
tipusEnableFiber(unsigned int fiber)
{
  unsigned int sval;
  unsigned int fiberbit;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

#ifdef OLD_FIBER_REG
  if((fiber<1) | (fiber>8))
    {
      printf("%s: ERROR: Invalid value for fiber (%d)\n",
	     __FUNCTION__,fiber);
      return ERROR;
    }

  fiberbit = (1<<(fiber-1));

  TIPUSLOCK;
  sval = tipusRead(&TIPUSp->fiber);
  tipusWrite(&TIPUSp->fiber,
	     sval | fiberbit );
  TIPUSUNLOCK;
#else
  printf("%s: WARNING: This routine does nothing.\n",__func__);

#endif
  return OK;

}

/**
 * @ingroup Config
 * @brief Disable Fiber transceiver
 *
 * @sa tiEnableFiber
 *
 * @param   fiber: integer indicative of the transceiver to disable
 *
 *
 * @return OK if successful, ERROR otherwise.
 *
 */
int
tipusDisableFiber(unsigned int fiber)
{
  unsigned int rval = OK;
  unsigned int fiberbit;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

#ifdef OLD_FIBER_REG
  if((fiber<1) | (fiber>8))
    {
      printf("%s: ERROR: Invalid value for fiber (%d)\n",
	     __FUNCTION__,fiber);
      return ERROR;
    }

  fiberbit = (1<<(fiber-1));

  TIPUSLOCK;
  rval = tipusRead(&TIPUSp->fiber);
  tipusWrite(&TIPUSp->fiber,
	     rval & ~fiberbit );
  TIPUSUNLOCK;
#else
  printf("%s: WARNING: This routine does nothing.\n",__func__);

#endif

  return rval;

}

/**
 * @ingroup Config
 * @brief Set the busy source with a given sourcemask sourcemask bits:
 *
 * @param sourcemask
 *  - 0: SWA
 *  - 1: SWB
 *  - 2: P2
 *  - 3: FP-FTDC
 *  - 4: FP-FADC
 *  - 5: FP
 *  - 6: Unused
 *  - 7: Loopack
 *  - 8-15: Fiber 1-8
 *
 * @param rFlag - decision to reset the global source flags
 *       -      0: Keep prior busy source settings and set new "sourcemask"
 *       -      1: Reset, using only that specified with "sourcemask"
 * @return OK if successful, ERROR otherwise.
 */
int
tipusSetBusySource(unsigned int sourcemask, int rFlag)
{
  unsigned int busybits=0;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(sourcemask>TIPUS_BUSY_SOURCEMASK)
    {
      printf("%s: ERROR: Invalid value for sourcemask (0x%x)\n",
	     __FUNCTION__, sourcemask);
      return ERROR;
    }

  TIPUSLOCK;
  if(rFlag)
    {
      /* Read in the previous value , resetting previous BUSYs*/
      busybits = tipusRead(&TIPUSp->busy) & ~(TIPUS_BUSY_SOURCEMASK);
    }
  else
    {
      /* Read in the previous value , keeping previous BUSYs*/
      busybits = tipusRead(&TIPUSp->busy);
    }

  busybits |= sourcemask;

  tipusWrite(&TIPUSp->busy, busybits);
  TIPUSUNLOCK;

  return OK;

}


/**
 *  @ingroup MasterConfig
 *  @brief Set the the trigger lock mode.
 *
 *  @param enable Enable flag
 *      0: Disable
 *     !0: Enable
 *
 * @return OK if successful, ERROR otherwise.
 */
int
tipusSetTriggerLock(int enable)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(!tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Master.\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  if(enable)
    tipusWrite(&TIPUSp->busy,
	       tipusRead(&TIPUSp->busy) | TIPUS_BUSY_TRIGGER_LOCK);
  else
    tipusWrite(&TIPUSp->busy,
	       tipusRead(&TIPUSp->busy) & ~TIPUS_BUSY_TRIGGER_LOCK);
  TIPUSUNLOCK;

  return OK;
}

/**
 *  @ingroup MasterStatus
 *  @brief Get the current setting of the trigger lock mode.
 *
 * @return 1 if enabled, 0 if disabled, ERROR otherwise.
 */
int
tipusGetTriggerLock()
{
  int rval=0;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(!tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Master.\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = (tipusRead(&TIPUSp->busy) & TIPUS_BUSY_TRIGGER_LOCK)>>6;
  TIPUSUNLOCK;

  return rval;
}


/**
 *  @ingroup MasterConfig
 *  @brief Set the prescale factor for the external trigger
 *
 *  @param   prescale Factor for prescale.
 *               Max {prescale} available is 65535
 *
 *  @return OK if successful, otherwise ERROR.
 */
int
tipusSetPrescale(int prescale)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(prescale<0 || prescale>0xffff)
    {
      printf("%s: ERROR: Invalid prescale (%d).  Must be between 0 and 65535.",
	     __FUNCTION__,prescale);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->trig1Prescale, prescale);
  TIPUSUNLOCK;

  return OK;
}


/**
 *  @ingroup Status
 *  @brief Get the current prescale factor
 *  @return Current prescale factor, otherwise ERROR.
 */
int
tipusGetPrescale()
{
  int rval;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = tipusRead(&TIPUSp->trig1Prescale);
  TIPUSUNLOCK;

  return rval;
}

/**
 *  @ingroup MasterConfig
 *  @brief Set the prescale factor for the selected input
 *
 *  @param   input Selected trigger input (1-6)
 *  @param   prescale Factor for prescale.
 *               Max {prescale} available is 65535
 *
 *  @return OK if successful, otherwise ERROR.
 */
int
tipusSetInputPrescale(int input, int prescale)
{
  unsigned int oldval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if((prescale<0) || (prescale>0xf))
    {
      printf("%s: ERROR: Invalid prescale (%d).  Must be between 0 and 15.",
	     __FUNCTION__,prescale);
      return ERROR;
    }

  if((input<1) || (input>6))
    {
      {
	printf("%s: ERROR: Invalid input (%d).",
	       __FUNCTION__,input);
	return ERROR;
      }
    }

  TIPUSLOCK;
  oldval = tipusRead(&TIPUSp->inputPrescale) & ~(TIPUS_INPUTPRESCALE_FP_MASK(input));
  tipusWrite(&TIPUSp->inputPrescale, oldval | (prescale<<(4*(input-1) )) );
  TIPUSUNLOCK;

  return OK;
}


/**
 *  @ingroup Status
 *  @brief Get the current prescale factor for the selected input
 *  @param   input Selected trigger input (1-6)
 *  @return Current prescale factor, otherwise ERROR.
 */
int
tipusGetInputPrescale(int input)
{
  int rval;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = tipusRead(&TIPUSp->inputPrescale) & TIPUS_INPUTPRESCALE_FP_MASK(input);
  rval = rval>>(4*(input-1));
  TIPUSUNLOCK;

  return rval;
}

/**
 *  @ingroup Config
 *  @brief Set the characteristics of a specified trigger
 *
 *  @param trigger
 *           - 1: set for trigger 1
 *           - 2: set for trigger 2 (playback trigger)
 *  @param delay    delay in units of delay_step
 *  @param width    pulse width in units of 4ns
 *  @param delay_step step size of the delay
 *         - 0: 16ns
 *          !0: 64ns (with an offset of ~4.1 us)
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusSetTriggerPulse(int trigger, int delay, int width, int delay_step)
{
  unsigned int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(trigger<1 || trigger>2)
    {
      printf("%s: ERROR: Invalid trigger (%d).  Must be 1 or 2.\n",
	     __FUNCTION__,trigger);
      return ERROR;
    }
  if(delay<0 || delay>0x7F)
    {
      printf("%s: ERROR: Invalid delay (%d).  Must be less than %d\n",
	     __FUNCTION__,delay,TIPUS_TRIGDELAY_TRIG1_DELAY_MASK);
      return ERROR;
    }
  if(width<0 || width>TIPUS_TRIGDELAY_TRIG1_WIDTH_MASK)
    {
      printf("%s: ERROR: Invalid width (%d).  Must be less than %d\n",
	     __FUNCTION__,width,TIPUS_TRIGDELAY_TRIG1_WIDTH_MASK);
    }


  TIPUSLOCK;
  if(trigger==1)
    {
      rval = tipusRead(&TIPUSp->trigDelay) &
	~(TIPUS_TRIGDELAY_TRIG1_DELAY_MASK | TIPUS_TRIGDELAY_TRIG1_WIDTH_MASK) ;
      rval |= ( (delay) | (width<<8) );
      if(delay_step)
	rval |= TIPUS_TRIGDELAY_TRIG1_64NS_STEP;

      tipusWrite(&TIPUSp->trigDelay, rval);
    }
  if(trigger==2)
    {
      rval = tipusRead(&TIPUSp->trigDelay) &
	~(TIPUS_TRIGDELAY_TRIG2_DELAY_MASK | TIPUS_TRIGDELAY_TRIG2_WIDTH_MASK) ;
      rval |= ( (delay<<16) | (width<<24) );
      if(delay_step)
	rval |= TIPUS_TRIGDELAY_TRIG2_64NS_STEP;

      tipusWrite(&TIPUSp->trigDelay, rval);
    }
  TIPUSUNLOCK;

  return OK;
}

/**
 *  @ingroup Config
 *  @brief Set the width of the prompt trigger from OT#2
 *
 *  @param width Output width will be set to (width + 2) * 4ns
 *
 *    This routine is only functional for Firmware type=2 (modTI)
 *
 *  @return OK if successful, otherwise ERROR
 */
int
tipusSetPromptTriggerWidth(int width)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if((width<0) || (width>TIPUS_PROMPT_TRIG_WIDTH_MASK))
    {
      printf("%s: ERROR: Invalid prompt trigger width (%d)\n",
	     __FUNCTION__,width);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->eventNumber_hi, width);
  TIPUSUNLOCK;

  return OK;
}

/**
 *  @ingroup Status
 *  @brief Get the width of the prompt trigger from OT#2
 *
 *    This routine is only functional for Firmware type=2 (modTI)
 *
 *  @return Output width set to (return value + 2) * 4ns, if successful. Otherwise ERROR
 */
int
tipusGetPromptTriggerWidth()
{
  unsigned int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = tipusRead(&TIPUSp->eventNumber_hi) & TIPUS_PROMPT_TRIG_WIDTH_MASK;
  TIPUSUNLOCK;

  return OK;
}

/**
 *  @ingroup Config
 *  @brief Set the delay time and width of the Sync signal
 *
 * @param delay  the delay (latency) set in units of 4ns.
 * @param width  the width set in units of 4ns.
 * @param twidth  if this is non-zero, set width in units of 32ns.
 *
 */
void
tipusSetSyncDelayWidth(unsigned int delay, unsigned int width, int widthstep)
{
  int twidth=0, tdelay=0;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TS not initialized\n",__FUNCTION__);
      return;
    }

  if(delay>TIPUS_SYNCDELAY_MASK)
    {
      printf("%s: ERROR: Invalid delay (%d)\n",__FUNCTION__,delay);
      return;
    }

  if(width>TIPUS_SYNCWIDTH_MASK)
    {
      printf("%s: WARN: Invalid width (%d).\n",__FUNCTION__,width);
      return;
    }

  if(widthstep)
    width |= TIPUS_SYNCWIDTH_LONGWIDTH_ENABLE;

  tdelay = delay*4;
  if(widthstep)
    twidth = (width&TIPUS_SYNCWIDTH_MASK)*32;
  else
    twidth = width*4;

  printf("%s: Setting Sync delay = %d (ns)   width = %d (ns)\n",
	 __FUNCTION__,tdelay,twidth);

  TIPUSLOCK;
  tipusWrite(&TIPUSp->syncDelay,delay);
  tipusWrite(&TIPUSp->syncWidth,width);
  TIPUSUNLOCK;

}

/**
 * @ingroup MasterConfig
 * @brief Reset the trigger link.
 */
void
tipusTrigLinkReset()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->syncCommand,TIPUS_SYNCCOMMAND_TRIGGERLINK_DISABLE);
  usleep(10);

  tipusWrite(&TIPUSp->syncCommand,TIPUS_SYNCCOMMAND_TRIGGERLINK_DISABLE);
  usleep(10);

  tipusWrite(&TIPUSp->syncCommand,TIPUS_SYNCCOMMAND_TRIGGERLINK_ENABLE);
  usleep(10);

  TIPUSUNLOCK;

  printf ("%s: Trigger Data Link was reset.\n",__FUNCTION__);
}

/**
 * @ingroup MasterConfig
 * @brief Set type of SyncReset to send to TI Slaves
 *
 * @param type Sync Reset Type
 *    - 0: User programmed width in each TI
 *    - !0: Fixed 4 microsecond width in each TI
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusSetSyncResetType(int type)
{

  if(type)
    tipusSyncResetType=TIPUS_SYNCCOMMAND_SYNCRESET_4US;
  else
    tipusSyncResetType=TIPUS_SYNCCOMMAND_SYNCRESET;

  return OK;
}


/**
 * @ingroup MasterConfig
 * @brief Generate a Sync Reset signal.  This signal is sent to the loopback and
 *    all configured TI Slaves.
 *
 *  @param blflag Option to change block level, after SyncReset issued
 *       -   0: Do not change block level
 *       -  >0: Broadcast block level to all connected slaves (including self)
 *            BlockLevel broadcasted will be set to library value
 *            (Set with tiSetBlockLevel)
 *
 */
void
tipusSyncReset(int blflag)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->syncCommand,tipusSyncResetType);
  usleep(10);
  tipusWrite(&TIPUSp->syncCommand,TIPUS_SYNCCOMMAND_RESET_EVNUM);
  usleep(10);
  TIPUSUNLOCK;

  if(blflag) /* Set the block level from "Next" to Current */
    {
      printf("%s: INFO: Setting Block Level to %d\n",
	     __FUNCTION__,tipusNextBlockLevel);
      tipusBroadcastNextBlockLevel(tipusNextBlockLevel);
      tipusSetBlockBufferLevel(tipusBlockBufferLevel);
    }

}

/**
 * @ingroup MasterConfig
 * @brief Generate a Sync Reset Resync signal.  This signal is sent to the loopback and
 *    all configured TI Slaves.  This type of Sync Reset will NOT reset
 *    event numbers
 *
 */
void
tipusSyncResetResync()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->syncCommand,tipusSyncResetType);
  TIPUSUNLOCK;

}

/**
 * @ingroup MasterConfig
 * @brief Generate a Clock Reset signal.  This signal is sent to the loopback and
 *    all configured TI Slaves.
 *
 */
void
tipusClockReset()
{
  unsigned int old_syncsrc=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return;
    }

  if(tipusMaster!=1)
    {
      printf("%s: ERROR: TI is not the Master.  No Clock Reset.\n", __FUNCTION__);
      return;
    }

  TIPUSLOCK;
  /* Send a clock reset */
  tipusWrite(&TIPUSp->syncCommand,TIPUS_SYNCCOMMAND_CLK250_RESYNC);
  usleep(20000);

  /* Store the old sync source */
  old_syncsrc = tipusRead(&TIPUSp->sync) & TIPUS_SYNC_SOURCEMASK;
  /* Disable sync source */
  tipusWrite(&TIPUSp->sync, 0);
  usleep(20000);

  /* Send another clock reset */
  tipusWrite(&TIPUSp->syncCommand,TIPUS_SYNCCOMMAND_CLK250_RESYNC);
  usleep(20000);

  /* Re-enable the sync source */
  tipusWrite(&TIPUSp->sync, old_syncsrc);
  TIPUSUNLOCK;

}


/**
 * @ingroup Config
 * @brief Reset the L1A counter, as incremented by the TI.
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusResetEventCounter()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->reset, TIPUS_RESET_SCALERS_RESET);
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup Status
 * @brief Returns the event counter (48 bit)
 *
 * @return Number of accepted events if successful, otherwise ERROR
 */
unsigned long long int
tipusGetEventCounter()
{
  unsigned long long int rval=0;
  unsigned int lo=0, hi=0;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  lo = tipusRead(&TIPUSp->eventNumber_lo);
  hi = (tipusRead(&TIPUSp->eventNumber_hi) & TIPUS_EVENTNUMBER_HI_MASK)>>16;

  rval = lo | ((unsigned long long)hi<<32);
  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup MasterConfig
 * @brief Set the block number at which triggers will be disabled automatically
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusSetBlockLimit(unsigned int limit)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->blocklimit,limit);
  TIPUSUNLOCK;

  return OK;
}


/**
 * @ingroup Status
 * @brief Returns the value that is currently programmed as the block limit
 *
 * @return Current Block Limit if successful, otherwise ERROR
 */
unsigned int
tipusGetBlockLimit()
{
  unsigned int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = tipusRead(&TIPUSp->blocklimit);
  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Get the current status of the block limit
 *
 * @return 1 if block limit has been reached, 0 if not, otherwise ERROR;
 *
 */
int
tipusGetBlockLimitStatus()
{
  unsigned int reg=0, rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  reg = tipusRead(&TIPUSp->blockBuffer) & TIPUS_BLOCKBUFFER_BUSY_ON_BLOCKLIMIT;
  if(reg)
    rval = 1;
  else
    rval = 0;
  TIPUSUNLOCK;

  return rval;
}


/**
 * @ingroup Readout
 * @brief Returns the number of Blocks available for readout
 *
 * @return Number of blocks available for readout if successful, otherwise ERROR
 *
 */
unsigned int
tipusBReady()
{
  unsigned int blockBuffer=0, blockReady=0, rval=0;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return 0;
    }

  TIPUSLOCK;
  blockBuffer = tipusRead(&TIPUSp->blockBuffer);
  rval        = (blockBuffer&TIPUS_BLOCKBUFFER_BLOCKS_READY_MASK)>>8;
  blockReady  = ((blockBuffer&TIPUS_BLOCKBUFFER_TRIGGERS_NEEDED_IN_BLOCK)>>16)?0:1;
  tipusSyncEventReceived = (blockBuffer&TIPUS_BLOCKBUFFER_SYNCEVENT)>>31;
  tipusNReadoutEvents = (blockBuffer&TIPUS_BLOCKBUFFER_RO_NEVENTS_MASK)>>24;

  if( (rval==1) && (tipusSyncEventReceived) & (blockReady))
    tipusSyncEventFlag = 1;
  else
    tipusSyncEventFlag = 0;

  TIPUSUNLOCK;

  if(blockBuffer==ERROR)
    return ERROR;

  return rval;
}

/**
 * @ingroup Readout
 * @brief Return the value of the Synchronization flag, obtained from tiBReady.
 *   i.e. Return the value of the SyncFlag for the current readout block.
 *
 * @sa tiBReady
 * @return
 *   -  1: if current readout block contains a Sync Event.
 *   -  0: Otherwise
 *
 */
int
tipusGetSyncEventFlag()
{
  int rval=0;

  TIPUSLOCK;
  rval = tipusSyncEventFlag;
  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup Readout
 * @brief Return the value of whether or not the sync event has been received
 *
 * @return
 *     - 1: if sync event received
 *     - 0: Otherwise
 *
 */
int
tipusGetSyncEventReceived()
{
  int rval=0;

  TIPUSLOCK;
  rval = tipusSyncEventReceived;
  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup Readout
 * @brief Return the value of the number of readout events accepted
 *
 * @return Number of readout events accepted
 */
int
tipusGetReadoutEvents()
{
  int rval=0;

  TIPUSLOCK;
  rval = tipusNReadoutEvents;
  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup MasterConfig
 * @brief Set the block buffer level for the number of blocks in the system
 *     that need to be read out.
 *
 *     If this buffer level is full, the TI will go BUSY.
 *     The BUSY is released as soon as the number of buffers in the system
 *     drops below this level.
 *
 *  @param     level
 *       -        0:  No Buffer Limit  -  Pipeline mode
 *       -        1:  One Block Limit - "ROC LOCK" mode
 *       -  2-65535:  "Buffered" mode.
 *
 * @return OK if successful, otherwise ERROR
 *
 */
int
tipusSetBlockBufferLevel(unsigned int level)
{
  unsigned int trigsrc = 0;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(level>TIPUS_BLOCKBUFFER_BUFFERLEVEL_MASK)
    {
      printf("%s: ERROR: Invalid value for level (%d)\n",
	     __FUNCTION__,level);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->blockBuffer, level);

  tipusBlockBufferLevel = level;

  if(tipusMaster)
    {
      /* Broadcast buffer level to TI-slaves */
      trigsrc = tipusRead(&TIPUSp->trigsrc);

      /* Turn on the VME trigger, if not enabled */
      if(!(trigsrc & TIPUS_TRIGSRC_VME))
	tipusWrite(&TIPUSp->trigsrc, TIPUS_TRIGSRC_VME | trigsrc);

      /* Broadcast using trigger command */
      tipusWrite(&TIPUSp->triggerCommand, TIPUS_TRIGGERCOMMAND_SET_BUFFERLEVEL | level);

      /* Turn off the VME trigger, if it was initially disabled */
      if(!(trigsrc & TIPUS_TRIGSRC_VME))
	tipusWrite(&TIPUSp->trigsrc, trigsrc);
    }

  TIPUSUNLOCK;

  return OK;
}

/**
 *  @ingroup Status
 *  @brief Get the block buffer level, as broadcasted from the TS
 *
 * @return Broadcasted block buffer level if successful, otherwise ERROR
 */
int
tipusGetBroadcastBlockBufferLevel()
{
  int rval = 0;

  if(TIPUSp == NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval =
    (int) ((tipusRead(&TIPUSp->dataFormat) &
	    TIPUS_DATAFORMAT_BCAST_BUFFERLEVEL_MASK) >> 24);
  TIPUSUNLOCK;

  return rval;
}

/**
 *  @ingroup Config
 *  @brief Set the TI to be BUSY if number of stored blocks is equal to
 *         the set block buffer level
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusBusyOnBufferLevel(int enable)
{
  if(TIPUSp == NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->vmeControl,
	     tipusRead(&TIPUSp->vmeControl) | TIPUS_VMECONTROL_BUSY_ON_BUFFERLEVEL);
  TIPUSUNLOCK;

  return OK;
}

/**
 *  @ingroup Config
 *  @brief Enable/Disable the use of the broadcasted buffer level, instead of the
 *         value set locally with @tiSetBlockBufferLevel.
 *
 *  @param enable - 1: Enable, 0: Disable
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusUseBroadcastBufferLevel(int enable)
{
  if(TIPUSp == NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }


  TIPUSLOCK;
  if(enable)
    tipusWrite(&TIPUSp->vmeControl,
	       tipusRead(&TIPUSp->vmeControl) & ~TIPUS_VMECONTROL_USE_LOCAL_BUFFERLEVEL);
  else
    tipusWrite(&TIPUSp->vmeControl,
	       tipusRead(&TIPUSp->vmeControl) | TIPUS_VMECONTROL_USE_LOCAL_BUFFERLEVEL);
  TIPUSUNLOCK;

  return OK;
}


/**
 * @ingroup MasterConfig
 * @brief Enable/Disable trigger inputs labelled TS#1-6 on the Front Panel
 *
 *     These inputs MUST be disabled if not connected.
 *
 *   @param   inpMask
 *       - 0:  TS#1
 *       - 1:  TS#2
 *       - 2:  TS#3
 *       - 3:  TS#4
 *       - 4:  TS#5
 *       - 5:  TS#6
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusEnableTSInput(unsigned int inpMask)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(inpMask>0x3f)
    {
      printf("%s: ERROR: Invalid inpMask (0x%x)\n",__FUNCTION__,inpMask);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->tsInput, inpMask);
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup MasterConfig
 * @brief Disable trigger inputs labelled TS#1-6 on the Front Panel
 *
 *     These inputs MUST be disabled if not connected.
 *
 *   @param   inpMask
 *       - 0:  TS#1
 *       - 1:  TS#2
 *       - 2:  TS#3
 *       - 3:  TS#4
 *       - 4:  TS#5
 *       - 5:  TS#6
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusDisableTSInput(unsigned int inpMask)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(inpMask>0x3f)
    {
      printf("%s: ERROR: Invalid inpMask (0x%x)\n",__FUNCTION__,inpMask);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->tsInput, tipusRead(&TIPUSp->tsInput) & ~inpMask);
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup Config
 * @brief Set (or unset) high level for the output ports on the front panel
 *     labelled as O#1-4
 *
 * @param         set1  O#1
 * @param         set2  O#2
 * @param         set3  O#3
 * @param         set4  O#4
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusSetOutputPort(unsigned int set1, unsigned int set2, unsigned int set3, unsigned int set4)
{
  unsigned int bits=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(set1)
    bits |= (1<<0);
  if(set2)
    bits |= (1<<1);
  if(set3)
    bits |= (1<<2);
  if(set4)
    bits |= (1<<3);

  TIPUSLOCK;
  tipusWrite(&TIPUSp->output, bits);
  TIPUSUNLOCK;

  return OK;
}


/**
 * @ingroup Config
 * @brief Set the clock to the specified source.
 *
 * @param   source
 *         -   0:  Onboard clock
 *         -   1:  External clock (HFBR1 input)
 *         -   5:  External clock (HFBR5 input)
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusSetClockSource(unsigned int source)
{
  int rval=OK;
  unsigned int clkset=0;
  unsigned int clkread=0;
  char sClock[20] = "";
  unsigned int ret=0;
  unsigned int old_syncsrc=0;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  switch(source)
    {
    case 0: /* ONBOARD */
      clkset = TIPUS_CLOCK_INTERNAL;
      sprintf(sClock,"ONBOARD (%d)",source);
      break;
    case 1: /* EXTERNAL (HFBR1) */
      clkset = TIPUS_CLOCK_HFBR1;
      sprintf(sClock,"EXTERNAL-HFBR1 (%d)",source);
      break;
    case 5: /* EXTERNAL (HFBR5) */
      clkset = TIPUS_CLOCK_HFBR5 | TIPUS_CLOCK_BRIDGE; /*sergey: for now - for fiber5 always activate bridge mode*/
      sprintf(sClock,"EXTERNAL-HFBR5 (%d)",source);
      break;
    default:
      printf("%s: ERROR: Invalid Clock Souce (%d)\n",__FUNCTION__,source);
      return ERROR;
    }

  printf("%s: Setting clock source to %s\n",__FUNCTION__,sClock);


  /*sergey*/
  printf("%s: Clock source register 0x2C BEFOR adding output clock setting = 0x%08x\n",__FUNCTION__,clkset);
  clkset |= tipusClockOutputMode;
  printf("%s: Clock source register 0x2C AFTER adding output clock setting = 0x%08x\n",__FUNCTION__,clkset);
  /*sergey*/


  /*sergey on William's advise: always disable SYNC before handling CLK, otherwise FPGA got reloaded !!!*/
  printf("BEFOR tipusSetSyncSource(0)\n");
  tipusSetSyncSource(0);
  printf("AFTER tipusSetSyncSource(0)\n");
  sleep(1);

  TIPUSLOCK;

#if 0
  old_syncsrc = tipusRead(&TIPUSp->sync) & TIPUS_SYNC_SOURCEMASK;
  printf("old_syncsrc=0x%x\n",old_syncsrc);fflush(stdout);
  //tipusWrite(&TIPUSp->sync, 0);
  usleep(20000);
#endif


  printf("#: B8=0x%x\n",tipusRead(&TIPUSp->GTPtriggerBufferLength));fflush(stdout);
  sleep(1);


  printf("0: B8=0x%x\n",tipusRead(&TIPUSp->GTPtriggerBufferLength));fflush(stdout);
  sleep(1);

  tipusWrite(&TIPUSp->clock, clkset);
  sleep(1);
  printf("1: B8=0x%x\n",tipusRead(&TIPUSp->GTPtriggerBufferLength));fflush(stdout);



#if 0
  /*sergey on William's advise: finish the job */
  tipusWrite(&TIPUSp->sync, old_syncsrc);
#endif


  // need MGT reset here ???

  /* Reset DCM (Digital Clock Manager) - 250/200MHz */
  tipusWrite(&TIPUSp->reset, TIPUS_RESET_CLK250); //0x100
  sleep(1);
  printf("2: B8=0x%x\n",tipusRead(&TIPUSp->GTPtriggerBufferLength));fflush(stdout);

  /* Reset DCM (Digital Clock Manager) - 125MHz */
  tipusWrite(&TIPUSp->reset, TIPUS_RESET_CLK125); //0x200
  sleep(1);
  printf("3: B8=0x%x\n",tipusRead(&TIPUSp->GTPtriggerBufferLength));fflush(stdout);

  /* FPGA IOdelay reset */
  tipusWrite(&TIPUSp->reset, TIPUS_RESET_IODELAY); //0x4000
  sleep(1);
  printf("4: B8=0x%x\n",tipusRead(&TIPUSp->GTPtriggerBufferLength));fflush(stdout);


  /*check the DCM lock*/
  ret = (tipusRead(&TIPUSp->GTPtriggerBufferLength) & 0xFC000000) >> 24;
  if(ret != 0xE4)
  {
    printf("%s: ERROR Setting Clock Source (high 8 bits of register 0xB8 = 0x%x)\n",
		 __FUNCTION__,ret);
    sleep(1);
  }




  if(source==1 || source==5) /* Turn on running mode for External Clock verification */
  {
    tipusWrite(&TIPUSp->runningMode, TIPUS_RUNNINGMODE_ENABLE);
    usleep(10);
    clkread = tipusRead(&TIPUSp->clock) & TIPUS_CLOCK_MASK;
    if( clkread != (clkset&TIPUS_CLOCK_MASK) )
    {
      printf("%s: ERROR Setting Clock Source (clkset = 0x%x, clkread = 0x%x)\n",
		 __FUNCTION__,clkset, clkread);
      rval = ERROR;
    }
    tipusWrite(&TIPUSp->runningMode,TIPUS_RUNNINGMODE_DISABLE);
  }

  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Get the current clock source
 * @return Current Clock Source
 */
int
tipusGetClockSource()
{
  int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = tipusRead(&TIPUSp->clock) & 0x3;
  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Set the fiber delay required to align the sync and triggers for all crates.
 * @return Current fiber delay setting
 */
void
tipusSetFiberDelay(unsigned int delay, unsigned int offset)
{
  unsigned int fiberLatency=0, syncDelay=0, syncDelay_write=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return;
    }

  fiberLatency=0;
  TIPUSLOCK;

  if(delay>offset)
    {
      printf("%s: WARN: delay (%d) greater than offset (%d).  Setting difference to zero\n",
	     __FUNCTION__,delay,offset);
      syncDelay = 0;
    }
  else
    {
      syncDelay = (offset-(delay));
    }

  /* set the sync delay according to the fiber latency */
  syncDelay_write =
    ((syncDelay & 0xff) << 8) | ((syncDelay & 0xff) << 16) |
    ((syncDelay & 0xff) << 24);

  tipusWrite(&TIPUSp->fiberSyncDelay,syncDelay_write);

#ifdef STOPTHIS
  if(tipusMaster != 1)  /* Slave only */
    {
      taskDelay(10);
      tipusWrite(&TIPUSp->reset,0x4000);  /* reset the IODELAY */
      taskDelay(10);
      tipusWrite(&TIPUSp->reset,0x800);   /* auto adjust the sync phase for HFBR#1 */
      taskDelay(10);
    }
#endif

  TIPUSUNLOCK;

  printf("%s: Wrote 0x%08x to fiberSyncDelay\n",__FUNCTION__,syncDelay_write);

}

int
tipusResetSlaveConfig()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(!tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Master.\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  tipusSlaveMask = 0;
  tipusWrite(&TIPUSp->busy, (tipusRead(&TIPUSp->busy) & ~TIPUS_BUSY_HFBR_MASK));
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup MasterConfig
 * @brief Add and configurate a TI Slave for the TI Master.
 *
 *      This routine should be used by the TI Master to configure
 *      HFBR porti and BUSY sources.
 *
 * @param    fiber  The fiber port of the TI Master that is connected to the slave
 *
 * @sa tiAddSlaveMask
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusAddSlave(unsigned int fiber)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(!tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Master.\n",__FUNCTION__);
      return ERROR;
    }

  if(fiber!=1)
    {
      printf("%s: ERROR: Invalid value for fiber (%d)\n",
	     __FUNCTION__,fiber);
      return ERROR;
    }

  /* Add this slave to the global slave mask */
  tipusSlaveMask |= (1<<(fiber-1));

  /* Add this fiber as a busy source (use first fiber macro as the base) */
  if(tipusSetBusySource(TIPUS_BUSY_HFBR1<<(fiber-1),0)!=OK)
    return ERROR;

  /* Enable the fiber */
  if(tipusEnableFiber(fiber)!=OK)
    return ERROR;

  return OK;

}

static int tiTriggerRuleClockPrescale[3][4] =
  {
    {4, 4, 8, 16}, // 250 MHz ref
    {16, 32, 64, 128}, // 100.0 MHz ref
    {16, 32, 64, 128} // 100.0 MHz ref prescaled by 32
  };

int
tipusPrintTriggerHoldoff(int dflag)
{
  unsigned long TIBase = 0;
  unsigned int triggerRule = 0, triggerRuleMin = 0, vmeControl = 0;
  int irule = 0, slowclock = 0, clockticks = 0, timestep = 0, minticks = 0;
  float clock[3] = {250, 100.0, 100.0/32.}, stepsize = 0., time = 0., min = 0.;

  if(TIPUSp == NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  triggerRule    = tipusRead(&TIPUSp->triggerRule);
  triggerRuleMin = tipusRead(&TIPUSp->triggerRuleMin);
  vmeControl     = tipusRead(&TIPUSp->vmeControl);
  TIPUSUNLOCK;

  if(dflag)
    {
      printf("  Registers:\n");
      TIBase = (unsigned long)TIPUSp;
      printf("   triggerRule    (0x%04lx) = 0x%08x\t",
	     (unsigned long)(&TIPUSp->triggerRule) - TIBase, triggerRule);
      printf(" triggerRuleMin (0x%04lx) = 0x%08x\n",
	     (unsigned long)(&TIPUSp->triggerRuleMin) - TIBase, triggerRuleMin);
    }

  printf("\n");
  printf("    Rule   Timesteps    + Up to     Minimum  ");
  if(dflag)
    printf("  ticks   clock   prescale\n");
  else
    printf("\n");
  printf("    ----   ---[ns]---  ---[ns]---  ---[ns]---");
  if(dflag)
    printf("  -----  -[MHz]-  --------\n");
  else
    printf("\n");

  slowclock = (vmeControl & (1 << 31)) >> 31;
  for(irule = 0; irule < 4; irule++)
    {
      clockticks = (triggerRule >> (irule*8)) & 0x7F;
      timestep   = ((triggerRule >> (irule*8)) >> 7) & 0x1;
      if((triggerRuleMin >> (irule*8)) & 0x80)
	minticks = (triggerRuleMin >> (irule*8)) & 0x7F;
      else
	minticks = 0;

      if((timestep == 1) && (slowclock == 1))
	{
	  timestep = 2;
	}

      stepsize = ((float) tiTriggerRuleClockPrescale[timestep][irule] /
		  (float) clock[timestep]);

      time = (float)clockticks * stepsize;

      min = (float) minticks * stepsize;

      printf("    %4d     %8.1f    %8.1f    %8.1f ",
	     irule + 1, 1E3 * time, 1E3 * stepsize, min);

      if(dflag)
	printf("   %3d    %5.1f       %3d\n",
	       clockticks, clock[timestep],
	       tiTriggerRuleClockPrescale[timestep][irule]);
      else
	printf("\n");

    }
  printf("\n");

  return OK;
}

/**
 * @ingroup MasterConfig
 * @brief Set the value for a specified trigger rule.
 *
 * @param   rule  the number of triggers within some time period..
 *            e.g. rule=1: No more than ONE trigger within the
 *                         specified time period
 *
 * @param   value  the specified time period (in steps of timestep)
 * @param timestep Timestep that is dependent on the trigger rule selected
 *<pre>
 *                         rule
 *    timestep    1      2       3       4
 *    -------   ----- ------- ------- -------
 *       0       16ns    16ns    32ns    64ns
 *       1      160ns   320ns   640ns  1280ns
 *       2     5120ns 10240ns 20480ns 40960ns
 *</pre>
 *
 * @return OK if successful, otherwise ERROR.
 *
 */
int
tipusSetTriggerHoldoff(int rule, unsigned int value, int timestep)
{
  unsigned int wval=0, rval=0;
  unsigned int maxvalue=0x7f;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if( (rule<1) || (rule>5) )
    {
      printf("%s: ERROR: Invalid value for rule (%d).  Must be 1-4\n",
	     __FUNCTION__,rule);
      return ERROR;
    }
  if(value>maxvalue)
    {
      printf("%s: ERROR: Invalid value (%d). Must be less than %d.\n",
	     __FUNCTION__,value,maxvalue);
      return ERROR;
    }

  if(timestep)
    value |= (1<<7);

  /* Read the previous values */
  TIPUSLOCK;
  rval = tipusRead(&TIPUSp->triggerRule);

  switch(rule)
    {
    case 1:
      wval = value | (rval & ~TIPUS_TRIGGERRULE_RULE1_MASK);
      break;
    case 2:
      wval = (value<<8) | (rval & ~TIPUS_TRIGGERRULE_RULE2_MASK);
      break;
    case 3:
      wval = (value<<16) | (rval & ~TIPUS_TRIGGERRULE_RULE3_MASK);
      break;
    case 4:
      wval = (value<<24) | (rval & ~TIPUS_TRIGGERRULE_RULE4_MASK);
      break;
    }

  tipusWrite(&TIPUSp->triggerRule,wval);

  if(timestep==2)
    tipusWrite(&TIPUSp->vmeControl,
	       tipusRead(&TIPUSp->vmeControl) | TIPUS_VMECONTROL_SLOWER_TRIGGER_RULES);
  else
    tipusWrite(&TIPUSp->vmeControl,
	       tipusRead(&TIPUSp->vmeControl) & ~TIPUS_VMECONTROL_SLOWER_TRIGGER_RULES);

  TIPUSUNLOCK;

  return OK;

}

/**
 * @ingroup Status
 * @brief Get the value for a specified trigger rule.
 *
 * @param   rule   the number of triggers within some time period..
 *            e.g. rule=1: No more than ONE trigger within the
 *                         specified time period
 *
 * @return If successful, returns the value (in steps of 16ns)
 *            for the specified rule. ERROR, otherwise.
 *
 */
int
tipusGetTriggerHoldoff(int rule)
{
  unsigned int rval=0;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(rule<1 || rule>5)
    {
      printf("%s: ERROR: Invalid value for rule (%d).  Must be 1-4.\n",
	     __FUNCTION__,rule);
      return ERROR;
    }

  TIPUSLOCK;
  rval = tipusRead(&TIPUSp->triggerRule);
  TIPUSUNLOCK;

  switch(rule)
    {
    case 1:
      rval = (rval & TIPUS_TRIGGERRULE_RULE1_MASK);
      break;

    case 2:
      rval = (rval & TIPUS_TRIGGERRULE_RULE2_MASK)>>8;
      break;

    case 3:
      rval = (rval & TIPUS_TRIGGERRULE_RULE3_MASK)>>16;
      break;

    case 4:
      rval = (rval & TIPUS_TRIGGERRULE_RULE4_MASK)>>24;
      break;
    }

  return rval;

}

/**
 * @ingroup MasterConfig
 * @brief Set the value for the minimum time of specified trigger rule.
 *
 * @param   rule  the number of triggers within some time period..
 *            e.g. rule=1: No more than ONE trigger within the
 *                         specified time period
 *
 * @param   value  the specified time period (in steps of timestep)
 *<pre>
 *       	 	      rule
 *    		         2      3      4
 *    		       ----- ------ ------
 *    		        16ns  160ns  160ns
 *    	(timestep=2)    16ns 5120ns 5120ns
 *</pre>
 *
 * @return OK if successful, otherwise ERROR.
 *
 */
int
tipusSetTriggerHoldoffMin(int rule, unsigned int value)
{
  unsigned int mask=0, enable=0, shift=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(rule<2 || rule>5)
    {
      printf("%s: ERROR: Invalid rule (%d).  Must be 2-4.\n",
	     __FUNCTION__,rule);
      return ERROR;
    }

  if(value > 0x7f)
    {
      printf("%s: ERROR: Invalid value (%d). Must be less than %d.\n",
	     __FUNCTION__,value,0x7f);
      return ERROR;
    }

  switch(rule)
    {
    case 2:
      mask = ~(TIPUS_TRIGGERRULEMIN_MIN2_MASK | TIPUS_TRIGGERRULEMIN_MIN2_EN);
      enable = TIPUS_TRIGGERRULEMIN_MIN2_EN;
      shift = 8;
      break;
    case 3:
      mask = ~(TIPUS_TRIGGERRULEMIN_MIN3_MASK | TIPUS_TRIGGERRULEMIN_MIN3_EN);
      enable = TIPUS_TRIGGERRULEMIN_MIN3_EN;
      shift = 16;
      break;
    case 4:
      mask = ~(TIPUS_TRIGGERRULEMIN_MIN4_MASK | TIPUS_TRIGGERRULEMIN_MIN4_EN);
      enable = TIPUS_TRIGGERRULEMIN_MIN4_EN;
      shift = 24;
      break;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->triggerRuleMin,
	     (tipusRead(&TIPUSp->triggerRuleMin) & mask) |
	     enable |
	     (value << shift) );
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup Status
 * @brief Get the value for a specified trigger rule minimum busy.
 *
 * @param   rule   the number of triggers within some time period..
 *            e.g. rule=1: No more than ONE trigger within the
 *                         specified time period
 *
 * @param  pflag  if not 0, print the setting to standard out.
 *
 * @return If successful, returns the value
 *          (in steps of 16ns for rule 2, 480ns otherwise)
 *            for the specified rule. ERROR, otherwise.
 *
 */
int
tipusGetTriggerHoldoffMin(int rule, int pflag)
{
  int rval=0;
  unsigned int mask=0, enable=0, shift=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(rule<2 || rule>5)
    {
      printf("%s: ERROR: Invalid rule (%d).  Must be 2-4.\n",
	     __FUNCTION__,rule);
      return ERROR;
    }

  switch(rule)
    {
    case 2:
      mask = TIPUS_TRIGGERRULEMIN_MIN2_MASK;
      enable = TIPUS_TRIGGERRULEMIN_MIN2_EN;
      shift = 8;
      break;
    case 3:
      mask = TIPUS_TRIGGERRULEMIN_MIN3_MASK;
      enable = TIPUS_TRIGGERRULEMIN_MIN3_EN;
      shift = 16;
      break;
    case 4:
      mask = TIPUS_TRIGGERRULEMIN_MIN4_MASK;
      enable = TIPUS_TRIGGERRULEMIN_MIN4_EN;
      shift = 24;
      break;
    }

  TIPUSLOCK;
  rval = (tipusRead(&TIPUSp->triggerRuleMin) & mask)>>shift;
  TIPUSUNLOCK;

  if(pflag)
    {
      printf("%s: Trigger rule %d  minimum busy = %d - %s\n",
	     __FUNCTION__,rule,
	     rval & 0x7f,
	     (rval & (1<<7))?"ENABLED":"DISABLED");
    }

  return rval & ~(1<<8);
}

#ifdef NOTIMPLEMENTED
/**
 *  @ingroup Config
 *  @brief Disable the necessity to readout the TI for every block.
 *
 *      For instances when the TI data is not required for analysis
 *      When a block is "ready", a call to tiResetBlockReadout must be made.
 *
 * @sa tiEnableDataReadout tiResetBlockReadout
 * @return OK if successful, otherwise ERROR
 */
int
tipusDisableDataReadout()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  tipusReadoutEnabled = 0;
  TIPUSLOCK;
  tipusWrite(&TIPUSp->vmeControl,
	     tipusRead(&TIPUSp->vmeControl) | TIPUS_VMECONTROL_BUFFER_DISABLE);
  TIPUSUNLOCK;

  printf("%s: Readout disabled.\n",__FUNCTION__);

  return OK;
}

/**
 *  @ingroup Config
 *  @brief Enable readout the TI for every block.
 *
 * @sa tiDisableDataReadout
 * @return OK if successful, otherwise ERROR
 */
int
tipusEnableDataReadout()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  tipusReadoutEnabled = 1;
  TIPUSLOCK;
  tipusWrite(&TIPUSp->vmeControl,
	     tipusRead(&TIPUSp->vmeControl) & ~TIPUS_VMECONTROL_BUFFER_DISABLE);
  TIPUSUNLOCK;

  printf("%s: Readout enabled.\n",__FUNCTION__);

  return OK;
}
#endif /* NOTIMPLEMENTED */

/**
 *  @ingroup Readout
 *  @brief Decrement the hardware counter for blocks available, effectively
 *      simulating a readout from the data fifo.
 *
 * @sa tiDisableDataReadout
 */
void
tipusResetBlockReadout()
{

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->reset,TIPUS_RESET_BLOCK_READOUT);
  TIPUSUNLOCK;

}


/**
 * @ingroup MasterConfig
 * @brief Configure trigger table to be loaded with a user provided array.
 *
 * @param itable Input Table (Array of 16 4byte words)
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusTriggerTableConfig(unsigned int *itable)
{
  int ielement=0;

  if(itable==NULL)
    {
      printf("%s: ERROR: Invalid input table address\n",
	     __FUNCTION__);
      return ERROR;
    }

  for(ielement=0; ielement<16; ielement++)
    tipusTrigPatternData[ielement] = itable[ielement];

  return OK;
}

/**
 * @ingroup MasterConfig
 * @brief Get the current trigger table stored in local memory (not necessarily on TI).
 *
 * @param otable Output Table (Array of 16 4byte words, user must allocate memory)
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusGetTriggerTable(unsigned int *otable)
{
  int ielement=0;

  if(otable==NULL)
    {
      printf("%s: ERROR: Invalid output table address\n",
	     __FUNCTION__);
      return ERROR;
    }

  for(ielement=0; ielement<16; ielement++)
    otable[ielement] = tipusTrigPatternData[ielement];

  return OK;
}

/**
 * @ingroup MasterConfig
 * @brief Configure trigger tabled to be loaded with a predefined
 * trigger table (mapping TS inputs to trigger types).
 *
 * @param mode
 *  - 0:
 *    - TS#1,2,3,4,5 generates Trigger1 (physics trigger),
 *    - TS#6 generates Trigger2 (playback trigger),
 *    - No SyncEvent;
 *  - 1:
 *    - TS#1,2,3 generates Trigger1 (physics trigger),
 *    - TS#4,5,6 generates Trigger2 (playback trigger).
 *    - If both Trigger1 and Trigger2, they are SyncEvent;
 *  - 2:
 *    - TS#1,2,3,4,5 generates Trigger1 (physics trigger),
 *    - TS#6 generates Trigger2 (playback trigger),
 *    - If both Trigger1 and Trigger2, generates SyncEvent;
 *  - 3:
 *    - TS#1,2,3,4,5,6 generates Trigger1 (physics trigger),
 *    - No Trigger2 (playback trigger),
 *    - No SyncEvent;
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusTriggerTablePredefinedConfig(int mode)
{
  int ielement=0;
  unsigned int trigPattern[4][16] =
    {
      { /* mode 0:
	   TS#1,2,3,4,5 generates Trigger1 (physics trigger),
	   TS#6 generates Trigger2 (playback trigger),
	   No SyncEvent;
	*/
	0x43424100, 0x47464544, 0x4b4a4948, 0x4f4e4d4c,
	0x53525150, 0x57565554, 0x5b5a5958, 0x5f5e5d5c,
	0x636261a0, 0x67666564, 0x6b6a6968, 0x6f6e6d6c,
	0x73727170, 0x77767574, 0x7b7a7978, 0x7f7e7d7c,
      },
      { /* mode 1:
	   TS#1,2,3 generates Trigger1 (physics trigger),
	   TS#4,5,6 generates Trigger2 (playback trigger).
	   If both Trigger1 and Trigger2, they are SyncEvent;
	*/
	0x43424100, 0x47464544, 0xcbcac988, 0xcfcecdcc,
	0xd3d2d190, 0xd7d6d5d4, 0xdbdad998, 0xdfdedddc,
	0xe3e2e1a0, 0xe7e6e5e4, 0xebeae9a8, 0xefeeedec,
	0xf3f2f1b0, 0xf7f6f5f4, 0xfbfaf9b8, 0xfffefdfc,
      },
      { /* mode 2:
	   TS#1,2,3,4,5 generates Trigger1 (physics trigger),
	   TS#6 generates Trigger2 (playback trigger),
	   If both Trigger1 and Trigger2, generates SyncEvent;
	*/
	0x43424100, 0x47464544, 0x4b4a4948, 0x4f4e4d4c,
	0x53525150, 0x57565554, 0x5b5a5958, 0x5f5e5d5c,
	0xe3e2e1a0, 0xe7e6e5e4, 0xebeae9e8, 0xefeeedec,
	0xf3f2f1f0, 0xf7f6f5f4, 0xfbfaf9f8, 0xfffefdfc
      },
      { /* mode 3:
           TS#1,2,3,4,5,6 generates Trigger1 (physics trigger),
           No Trigger2 (playback trigger),
           No SyncEvent;
        */
        0x43424100, 0x47464544, 0x4b4a4948, 0x4f4e4d4c,
        0x53525150, 0x57565554, 0x5b5a5958, 0x5f5e5d5c,
        0x63626160, 0x67666564, 0x6b6a6968, 0x6f6e6d6c,
        0x73727170, 0x77767574, 0x7b7a7978, 0x7f7e7d7c,
      }
    };

  if(mode>3)
    {
      printf("%s: WARN: Invalid mode %d.  Using Trigger Table mode = 0\n",
	     __FUNCTION__,mode);
      mode=0;
    }

  /* Copy predefined choice into static array to be loaded */

  for(ielement=0; ielement<16; ielement++)
    {
      tipusTrigPatternData[ielement] = trigPattern[mode][ielement];
    }

  return OK;
}

/**
 * @ingroup MasterConfig
 * @brief Define a specific trigger pattern as a hardware trigger (trig1/trig2/syncevent)
 * and Event Type
 *
 * @param trigMask Trigger Pattern (must be less than 0x3F)
 *    - TS inputs defining the pattern.  Starting bit: TS#1 = bit0
 * @param hwTrig Hardware trigger type (must be less than 3)
 *      0:  no trigger
 *      1:  Trig1 (event trigger)
 *      2:  Trig2 (playback trigger)
 *      3:  SyncEvent
 * @param evType Event Type (must be less than 255)
 *
 * @return OK if successful, otherwise ERROR
 */

int
tipusDefineEventType(int trigMask, int hwTrig, int evType)
{
  int element=0, byte=0;
  int data=0;
  unsigned int old_pattern=0;

  if(trigMask>0x3f)
    {
      printf("%s: ERROR: Invalid trigMask (0x%x)\n",
	     __FUNCTION__, trigMask);
      return ERROR;
    }

  if(hwTrig>3)
    {
      printf("%s: ERROR: Invalid hwTrig (%d)\n",
	     __FUNCTION__, hwTrig);
      return ERROR;
    }

  if(evType>0x3F)
    {
      printf("%s: ERROR: Invalid evType (%d)\n",
	     __FUNCTION__, evType);
      return ERROR;
    }

  element = trigMask/4;
  byte    = trigMask%4;

  data    = (hwTrig<<6) | evType;

  old_pattern = (tipusTrigPatternData[element] & ~(0xFF<<(byte*8)));
  tipusTrigPatternData[element] = old_pattern | (data<<(byte*8));

  return OK;
}

/**
 * @ingroup MasterConfig
 * @brief Define the event type for the TI Master's fixed and random internal trigger.
 *
 * @param fixed_type Fixed Pulser Event Type
 * @param random_type Pseudo Random Pulser Event Type
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusDefinePulserEventType(int fixed_type, int random_type)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if((fixed_type<0)||(fixed_type>0xFF))
    {
      printf("%s: ERROR: Invalid fixed_type (0x%x)\n",__FUNCTION__,fixed_type);
      return ERROR;
    }

  if((random_type<0)||(random_type>0xFF))
    {
      printf("%s: ERROR: Invalid random_type (0x%x)\n",__FUNCTION__,random_type);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->pulserEvType,
	     (fixed_type)<<16 | (random_type)<<24);
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup MasterConfig
 * @brief Load a predefined trigger table (mapping TS inputs to trigger types).
 *
 * @param mode
 *  - 0:
 *    - TS#1,2,3,4,5 generates Trigger1 (physics trigger),
 *    - TS#6 generates Trigger2 (playback trigger),
 *    - No SyncEvent;
 *  - 1:
 *    - TS#1,2,3 generates Trigger1 (physics trigger),
 *    - TS#4,5,6 generates Trigger2 (playback trigger).
 *    - If both Trigger1 and Trigger2, they are SyncEvent;
 *  - 2:
 *    - TS#1,2,3,4,5 generates Trigger1 (physics trigger),
 *    - TS#6 generates Trigger2 (playback trigger),
 *    - If both Trigger1 and Trigger2, generates SyncEvent;
 *  - 3:
 *    - TS#1,2,3,4,5,6 generates Trigger1 (physics trigger),
 *    - No Trigger2 (playback trigger),
 *    - No SyncEvent;
 *  - 4:
 *    User configured table @sa tiDefineEventType, tiTriggerTablePredefinedConfig
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusLoadTriggerTable(int mode)
{
  int ipat;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(mode>4)
    {
      printf("%s: WARN: Invalid mode %d.  Using Trigger Table mode = 0\n",
	     __FUNCTION__,mode);
      mode=0;
    }

  if(mode!=4)
    tipusTriggerTablePredefinedConfig(mode);

  TIPUSLOCK;
  for(ipat=0; ipat<16; ipat++)
    tipusWrite(&TIPUSp->trigTable[ipat], tipusTrigPatternData[ipat]);

  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup MasterStatus
 * @brief Print trigger table to standard out.
 *
 * @param showbits Show trigger bit pattern, instead of hex
 *
 */
void
tipusPrintTriggerTable(int showbits)
{
  int ielement, ibyte;
  int hwTrig=0, evType=0;

  for(ielement = 0; ielement<16; ielement++)
    {
      if(showbits)
	{
	  printf("--TS INPUT-\n");
	  printf("1 2 3 4 5 6  HW evType\n");
	}
      else
	{
	  printf("TS Pattern  HW evType\n");
	}

      for(ibyte=0; ibyte<4; ibyte++)
	{
	  hwTrig = ((tipusTrigPatternData[ielement]>>(ibyte*8)) & 0xC0)>>6;
	  evType = (tipusTrigPatternData[ielement]>>(ibyte*8)) & 0x3F;

	  if(showbits)
	    {
	      printf("%d %d %d %d %d %d   %d   %2d\n",
		     ((ielement*4+ibyte) & (1<<0))?1:0,
		     ((ielement*4+ibyte) & (1<<1))?1:0,
		     ((ielement*4+ibyte) & (1<<2))?1:0,
		     ((ielement*4+ibyte) & (1<<3))?1:0,
		     ((ielement*4+ibyte) & (1<<4))?1:0,
		     ((ielement*4+ibyte) & (1<<5))?1:0,
		     hwTrig, evType);
	    }
	  else
	    {
	      printf("  0x%02x       %d   %2d\n", ielement*4+ibyte,hwTrig, evType);
	    }
	}
      printf("\n");

    }


}


/**
 *  @ingroup MasterConfig
 *  @brief Set the window of the input trigger coincidence window
 *  @param window_width Width of the input coincidence window (units of 4ns)
 *  @return OK if successful, otherwise ERROR
 */
int
tipusSetTriggerWindow(int window_width)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if((window_width<1) || (window_width>TIPUS_TRIGGERWINDOW_COINC_MASK))
    {
      printf("%s: ERROR: Invalid Trigger Coincidence Window (%d)\n",
	     __FUNCTION__,window_width);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->triggerWindow,
	     (tipusRead(&TIPUSp->triggerWindow) & ~TIPUS_TRIGGERWINDOW_COINC_MASK)
	     | window_width);
  TIPUSUNLOCK;

  return OK;
}

/**
 *  @ingroup MasterStatus
 *  @brief Get the window of the input trigger coincidence window
 *  @return Width of the input coincidence window (units of 4ns), otherwise ERROR
 */
int
tipusGetTriggerWindow()
{
  int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = tipusRead(&TIPUSp->triggerWindow) & TIPUS_TRIGGERWINDOW_COINC_MASK;
  TIPUSUNLOCK;

  return rval;
}

/**
 *  @ingroup MasterConfig
 *  @brief Set the width of the input trigger inhibit window
 *  @param window_width Width of the input inhibit window (units of 4ns)
 *  @return OK if successful, otherwise ERROR
 */
int
tipusSetTriggerInhibitWindow(int window_width)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if((window_width<1) || (window_width>(TIPUS_TRIGGERWINDOW_INHIBIT_MASK>>8)))
    {
      printf("%s: ERROR: Invalid Trigger Inhibit Window (%d)\n",
	     __FUNCTION__,window_width);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->triggerWindow,
	     (tipusRead(&TIPUSp->triggerWindow) & ~TIPUS_TRIGGERWINDOW_INHIBIT_MASK)
	     | (window_width<<8));
  TIPUSUNLOCK;

  return OK;
}

/**
 *  @ingroup MasterStatus
 *  @brief Get the width of the input trigger inhibit window
 *  @return Width of the input inhibit window (units of 4ns), otherwise ERROR
 */
int
tipusGetTriggerInhibitWindow()
{
  int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = (tipusRead(&TIPUSp->triggerWindow) & TIPUS_TRIGGERWINDOW_INHIBIT_MASK)>>8;
  TIPUSUNLOCK;

  return rval;
}

/**
 *  @ingroup MasterConfig
 *  @brief Set the delay of Trig1 relative to Trig2 when trigger source is 11.
 *
 *  @param delay Trig1 delay after Trig2
 *    - Latency in steps of 4 nanoseconds with an offset of ~2.6 microseconds
 *
 *  @return OK if successful, otherwise ERROR
 */

int
tipusSetTrig21Delay(int delay)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(delay>0x1FF)
    {
      printf("%s: ERROR: Invalid delay (%d)\n",
	     __FUNCTION__,delay);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->triggerWindow,
	     (tipusRead(&TIPUSp->triggerWindow) & ~TIPUS_TRIGGERWINDOW_TRIG21_MASK) |
	     (delay<<16));
  TIPUSUNLOCK;
  return OK;
}

/**
 *  @ingroup MasterStatus
 *  @brief Get the delay of Trig1 relative to Trig2 when trigger source is 11.
 *
 *  @return Latency in steps of 4 nanoseconds with an offset of ~2.6 microseconds, otherwise ERROR
 */

int
tipusGetTrig21Delay()
{
  int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = (tipusRead(&TIPUSp->triggerWindow) & TIPUS_TRIGGERWINDOW_TRIG21_MASK)>>16;
  TIPUSUNLOCK;

  return rval;
}

/**
 *  @ingroup MasterConfig
 *  @brief Set the trigger latch pattern readout in the data stream to include
 *          the Level of the input trigger OR the transition to Hi.
 *
 *  @param enable
 *      1 to enable
 *     <1 to disable
 *
 *  @return OK if successful, otherwise ERROR
 */

int
tipusSetTriggerLatchOnLevel(int enable)
{
  if(TIPUSp == NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(enable < 1)
    enable = 0;

  TIPUSLOCK;
  tipusWrite(&TIPUSp->triggerWindow,
	     (tipusRead(&TIPUSp->triggerWindow) & ~TIPUS_TRIGGERWINDOW_LEVEL_LATCH) |
	     (enable<<31));
  TIPUSUNLOCK;
  return OK;
}

/**
 *  @ingroup MasterStatus
 *  @brief Get the trigger latch pattern readout in the data stream to include
 *          the Level of the input trigger OR the transition to Hi.
 *
 *  @return 1 if enabled, 0 if disabled, otherwise ERROR
 */

int
tipusGetTriggerLatchOnLevel()
{
  int rval=0;
  if(TIPUSp == NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = (tipusRead(&TIPUSp->triggerWindow) & TIPUS_TRIGGERWINDOW_LEVEL_LATCH)>>31;
  TIPUSUNLOCK;

  return rval;
}

/**
 *  @ingroup MasterConfig
 *  @brief Latch the Busy and Live Timers.
 *
 *     This routine should be called prior to a call to tiGetLiveTime and tiGetBusyTime
 *
 *  @sa tiGetLiveTime
 *  @sa tiGetBusyTime
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusLatchTimers()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->reset, TIPUS_RESET_SCALERS_LATCH);
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup Status
 * @brief Return the current "live" time of the module
 *
 * @returns The current live time in units of 7.68 us
 *
 */
unsigned int
tipusGetLiveTime()
{
  unsigned int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = tipusRead(&TIPUSp->livetime);
  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Return the current "busy" time of the module
 *
 * @returns The current live time in units of 7.68 us
 *
 */
unsigned int
tipusGetBusyTime()
{
  unsigned int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = tipusRead(&TIPUSp->busytime);
  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Calculate the live time (percentage) from the live and busy time scalers
 *
 * @param sflag if > 0, then returns the integrated live time
 *
 * @return live time as a 3 digit integer % (e.g. 987 = 98.7%)
 *
 */
int
tipusLive(int sflag)
{
  int rval=0;
  float fval=0;
  unsigned int newBusy=0, newLive=0, newTotal=0;
  unsigned int live=0, total=0;
  static unsigned int oldLive=0, oldTotal=0;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->reset,TIPUS_RESET_SCALERS_LATCH);
  newLive = tipusRead(&TIPUSp->livetime);
  newBusy = tipusRead(&TIPUSp->busytime);

  newTotal = newLive+newBusy;

  if((sflag==0) && (oldTotal<newTotal))
    { /* Differential */
      live  = newLive - oldLive;
      total = newTotal - oldTotal;
    }
  else
    { /* Integrated */
      live = newLive;
      total = newTotal;
    }

  oldLive = newLive;
  oldTotal = newTotal;

  if(total>0)
    fval = 1000*(((float) live)/((float) total));

  rval = (int) fval;

  TIPUSUNLOCK;

  return rval;
}


/**
 * @ingroup Status
 * @brief Get the current counter for the specified TS Input
 *
 * @param input
 *   - 1-6 : TS Input (1-6)
 * @param latch:
 *   -  0: Do not latch before readout
 *   -  1: Latch before readout
 *   -  2: Latch and reset before readout
 *
 *
 * @return Specified counter value
 *
 */
unsigned int
tipusGetTSscaler(int input, int latch)
{
  unsigned int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if((input<1)||(input>6))
    {
      printf("%s: ERROR: Invalid input (%d).\n",
	     __FUNCTION__,input);
      return ERROR;
    }

  if((latch<0) || (latch>2))
    {
      printf("%s: ERROR: Invalid latch (%d).\n",
	     __FUNCTION__,latch);
      return ERROR;
    }

  TIPUSLOCK;
  switch(latch)
    {
    case 1:
      tipusWrite(&TIPUSp->reset,TIPUS_RESET_SCALERS_LATCH);
      break;

    case 2:
      tipusWrite(&TIPUSp->reset,TIPUS_RESET_SCALERS_LATCH | TIPUS_RESET_SCALERS_RESET);
      break;
    }

  rval = tipusRead(&TIPUSp->ts_scaler[input-1]);
  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Show block Status of specified fiber
 * @param fiber  Fiber port to show
 * @param pflag  Whether or not to print to standard out
 * @return 0
 */
unsigned int
tipusBlockStatus(int fiber, int pflag)
{
  unsigned int rval=0;
  char name[50];
  unsigned int nblocksReady, nblocksNeedAck;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(fiber>1)
    {
      printf("%s: ERROR: Invalid value (%d) for fiber\n",__FUNCTION__,fiber);
      return ERROR;

    }

  switch(fiber)
    {
    case 0:
      rval = (tipusRead(&TIPUSp->adr24) & 0xFFFF0000)>>16;
      break;

    case 1:
    case 3:
    case 5:
    case 7:
      rval = (tipusRead(&TIPUSp->blockStatus[(fiber-1)/2]) & 0xFFFF);
      break;

    case 2:
    case 4:
    case 6:
    case 8:
      rval = ( tipusRead(&TIPUSp->blockStatus[(fiber/2)-1]) & 0xFFFF0000 )>>16;
      break;
    }

  if(pflag)
    {
      nblocksReady   = rval & TIPUS_BLOCKSTATUS_NBLOCKS_READY0;
      nblocksNeedAck = (rval & TIPUS_BLOCKSTATUS_NBLOCKS_NEEDACK0)>>8;

      if(fiber==0)
	sprintf(name,"Loopback");
      else
	sprintf(name,"Fiber %d",fiber);

      printf("%s: %s : Blocks ready / need acknowledge: %d / %d\n",
	     __FUNCTION__, name,
	     nblocksReady, nblocksNeedAck);

    }

  return rval;
}

static int
FiberMeas()
{
  int clksrc = 0, itry = 0, ntries = 2;
  unsigned int defaultDelay=0x1f1f1f00, fiberLatency=0, syncDelay=0, syncDelay_write=0;
  unsigned int firstMeas = 0;
  int failed = 0;
  int rval = OK;

  clksrc = tipusGetClockSource();
  /* Check to be sure the TI has external HFBR1 clock enabled */
  if(clksrc != TIPUS_CLOCK_HFBR1)
    {
      printf("%s: ERROR: Unable to measure fiber latency without HFBR1 as Clock Source\n",
	     __FUNCTION__);
      printf("\t Using default Fiber Sync Delay = %d (0x%x)",
	     defaultDelay, defaultDelay);

      TIPUSLOCK;
      tipusWrite(&TIPUSp->fiberSyncDelay,defaultDelay);
      TIPUSUNLOCK;

      return ERROR;
    }

  TIPUSLOCK;
  for(itry = 0; itry < ntries; itry++)
    {
      /* Reset the IODELAY */
      tipusWrite(&TIPUSp->reset,TIPUS_RESET_IODELAY);
      usleep(100);

      /* Auto adjust the return signal phase */
      tipusWrite(&TIPUSp->reset,TIPUS_RESET_FIBER_AUTO_ALIGN);
      usleep(100);

      /* Measure the fiber latency */
      tipusWrite(&TIPUSp->reset,TIPUS_RESET_MEASURE_LATENCY);
      usleep(1000);



      /* Get the fiber latency measurement result */
      fiberLatency = tipusRead(&TIPUSp->fiberLatencyMeasurement);
      /*sergey: itLib.c has this
      if(tiSlaveFiberIn==1)
	fiberLatency = vmeRead32(&TIp->fiberLatencyMeasurement);
      else
	fiberLatency = vmeRead32(&TIp->fiberAlignment);
      */



#ifdef DEBUGFIBERMEAS
      printf("Software offset = 0x%08x (%d)\n",
	     tipusFiberLatencyOffset, tipusFiberLatencyOffset);
      printf("Fiber Latency is 0x%08x\n",
	     fiberLatency);
      printf("  Latency data = 0x%08x (%d ns)\n",
	     (fiberLatency>>23), (fiberLatency>>23) * 4);
#endif


      /* Auto adjust the sync phase */
      tipusWrite(&TIPUSp->reset,TIPUS_RESET_AUTOALIGN_HFBR1_SYNC);
      /*sergey: tiLib.c has this
      if(tiSlaveFiberIn==1)
	vmeWrite32(&TIp->reset,TI_RESET_AUTOALIGN_HFBR1_SYNC);
      else
	vmeWrite32(&TIp->reset,TI_RESET_AUTOALIGN_HFBR5_SYNC);
      */
      usleep(100);

      /* Get the fiber latency measurement result */
      fiberLatency = tipusRead(&TIPUSp->fiberLatencyMeasurement);
      /*sergey: tiLib.c has this
      if(tiSlaveFiberIn==1)
	fiberLatency = vmeRead32(&TIp->fiberLatencyMeasurement);
      else
	fiberLatency = vmeRead32(&TIp->fiberAlignment);
      */

      /* Divide by two to get the one way trip */
      tipusFiberLatencyMeasurement =
	((fiberLatency & TIPUS_FIBERLATENCYMEASUREMENT_DATA_MASK)>>23)>>1;

      if(itry == 0)
	firstMeas = tipusFiberLatencyMeasurement;
      else
	{
	  if(firstMeas != tipusFiberLatencyMeasurement)
	    failed = 1;
	}
    }

  syncDelay = (tipusFiberLatencyOffset-(((fiberLatency>>23)&0x1ff)>>1));
  syncDelay_write = (syncDelay&0xFF)<<8 | (syncDelay&0xFF)<<16 | (syncDelay&0xFF)<<24;
  usleep(100);

  tipusWrite(&TIPUSp->fiberSyncDelay,syncDelay_write);
  usleep(10);
  syncDelay = tipusRead(&TIPUSp->fiberSyncDelay);
  TIPUSUNLOCK;

#ifdef DEBUGFIBERMEAS
  printf (" \n The fiber latency of 0xA0 is: 0x%08x\n", fiberLatency);
  printf (" \n The sync latency of 0x50 is: 0x%08x\n",syncDelay);
#endif /* DEBUGFIBERMEAS */

  if(failed == 1)
    {
      printf("\n");
      printf("%s: ERROR: TI Fiber Measurement failed!"
	     "\n\tFirst Measurement != Second Measurement (%d != %d)\n\n",
	     __FUNCTION__,
	     firstMeas, tipusFiberLatencyMeasurement);
      tipusFiberLatencyMeasurement = 0;
      rval = ERROR;
    }
  else if(!((tipusFiberLatencyMeasurement > 0) && (tipusFiberLatencyMeasurement <= 0xFF)))
    {
      printf("\n");
      printf("%s: ERROR: TI Fiber Measurement failed!"
	     "\n\tMeasurement out of bounds (%d)\n\n",
	     __FUNCTION__,
	     tipusFiberLatencyMeasurement);
      tipusFiberLatencyMeasurement = 0;
      rval = ERROR;
    }
  else
    {
      printf("%s: TI Fiber Measurement success!"
	     "  tipusFiberLatencyMeasurement = 0x%x (%d)\n",
	     __FUNCTION__,
	     tipusFiberLatencyMeasurement, tipusFiberLatencyMeasurement);
      rval = OK;
    }

  return rval;
}

/**
 * @ingroup Status
 * @brief Return measured fiber length
 * @return Value of measured fiber length
 */
int
tipusGetFiberLatencyMeasurement()
{
  return tipusFiberLatencyMeasurement;
}

/**
 * @ingroup MasterConfig
 * @brief Enable/Disable operation of User SyncReset
 * @sa tiUserSyncReset
 * @param enable
 *   - >0: Enable
 *   - 0: Disable
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusSetUserSyncResetReceive(int enable)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  if(enable)
    tipusWrite(&TIPUSp->sync, (tipusRead(&TIPUSp->sync) & TIPUS_SYNC_SOURCEMASK) |
	       TIPUS_SYNC_USER_SYNCRESET_ENABLED);
  else
    tipusWrite(&TIPUSp->sync, (tipusRead(&TIPUSp->sync) & TIPUS_SYNC_SOURCEMASK) &
	       ~TIPUS_SYNC_USER_SYNCRESET_ENABLED);
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup Status
 * @brief Return last SyncCommand received
 * @param
 *   - >0: print to standard out
 * @return Last SyncCommand received
 */
int
tipusGetLastSyncCodes(int pflag)
{
  int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }


  TIPUSLOCK;
  if(tipusMaster)
    rval = ((tipusRead(&TIPUSp->sync) & TIPUS_SYNC_LOOPBACK_CODE_MASK)>>16) & 0xF;
  else
    rval = ((tipusRead(&TIPUSp->sync) & TIPUS_SYNC_HFBR1_CODE_MASK)>>8) & 0xF;
  TIPUSUNLOCK;

  if(pflag)
    {
      printf(" Last Sync Code received:  0x%x\n",rval);
    }

  return rval;
}

/**
 * @ingroup Status
 * @brief Get the status of the SyncCommand History Buffer
 *
 * @param pflag
 *   - >0: Print to standard out
 *
 * @return
 *   - 0: Empty
 *   - 1: Half Full
 *   - 2: Full
 */
int
tipusGetSyncHistoryBufferStatus(int pflag)
{
  int hist_status=0, rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  hist_status = tipusRead(&TIPUSp->sync)
    & (TIPUS_SYNC_HISTORY_FIFO_MASK);
  TIPUSUNLOCK;

  switch(hist_status)
    {
    case TIPUS_SYNC_HISTORY_FIFO_EMPTY:
      rval=0;
      if(pflag) printf("%s: Sync history buffer EMPTY\n",__FUNCTION__);
      break;

    case TIPUS_SYNC_HISTORY_FIFO_HALF_FULL:
      rval=1;
      if(pflag) printf("%s: Sync history buffer HALF FULL\n",__FUNCTION__);
      break;

    case TIPUS_SYNC_HISTORY_FIFO_FULL:
    default:
      rval=2;
      if(pflag) printf("%s: Sync history buffer FULL\n",__FUNCTION__);
      break;
    }

  return rval;

}

/**
 * @ingroup Config
 * @brief Reset the SyncCommand history buffer
 */
void
tipusResetSyncHistory()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->reset, TIPUS_RESET_SYNC_HISTORY);
  TIPUSUNLOCK;

}

/**
 * @ingroup Config
 * @brief Control level of the SyncReset signal
 * @sa tiSetUserSyncResetReceive
 * @param enable
 *   - >0: High
 *   -  0: Low
 * @param pflag
 *   - >0: Print status to standard out
 *   -  0: Supress status message
 */
void
tipusUserSyncReset(int enable, int pflag)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return;
    }

  TIPUSLOCK;
  if(enable)
    tipusWrite(&TIPUSp->syncCommand,TIPUS_SYNCCOMMAND_SYNCRESET_HIGH);
  else
    tipusWrite(&TIPUSp->syncCommand,TIPUS_SYNCCOMMAND_SYNCRESET_LOW);

  usleep(20000);
  TIPUSUNLOCK;

  if(pflag)
    {
      printf("%s: User Sync Reset ",__FUNCTION__);
      if(enable)
	printf("HIGH\n");
      else
	printf("LOW\n");
    }

}

/**
 * @ingroup Status
 * @brief Print to standard out the history buffer of Sync Commands received.
 */
void
tipusPrintSyncHistory()
{
  unsigned int syncHistory=0;
  int count=0, code=1, valid=0, timestamp=0, overflow=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return;
    }

  while(code!=0)
    {
      TIPUSLOCK;
      syncHistory = tipusRead(&TIPUSp->syncHistory);
      TIPUSUNLOCK;

      /* printf("     TimeStamp: Code (valid)\n"); */

      if(tipusMaster)
	{
	  code  = (syncHistory & TIPUS_SYNCHISTORY_LOOPBACK_CODE_MASK)>>10;
	  valid = (syncHistory & TIPUS_SYNCHISTORY_LOOPBACK_CODE_VALID)>>14;
	}
      else
	{
	  /*
	  code  = syncHistory & TIPUS_SYNCHISTORY_HFBR1_CODE_MASK;
	  valid = (syncHistory & TIPUS_SYNCHISTORY_HFBR1_CODE_VALID)>>4;
	  */
	  if(tipusSlaveFiberIn == 1)
	    {
	      code  = syncHistory & TIPUS_SYNCHISTORY_HFBR1_CODE_MASK;
	      valid = (syncHistory & TIPUS_SYNCHISTORY_HFBR1_CODE_VALID)>>4;
	    }
	  else if(tipusSlaveFiberIn == 5)
	    {
	      code  = (syncHistory & TIPUS_SYNCHISTORY_HFBR5_CODE_MASK) >> 5;
	      valid = (syncHistory & TIPUS_SYNCHISTORY_HFBR5_CODE_VALID)>>9;
	    }
	  else
	    {
	      printf("%s: Invalid slave fiber port %d\n",
		     __func__, tipusSlaveFiberIn);
	      return;
	    }

	}

      overflow  = (syncHistory & TIPUS_SYNCHISTORY_TIMESTAMP_OVERFLOW)>>15;
      timestamp = (syncHistory & TIPUS_SYNCHISTORY_TIMESTAMP_MASK)>>16;

      /*       if(valid) */
      {
	printf("%4d: 0x%08x   %d %5d :  0x%x (%d)\n",
	       count, syncHistory,
	       overflow, timestamp, code, valid);
      }
      count++;
      if(count>1024)
	{
	  printf("%s: More than expected in the Sync History Buffer... exitting\n",
		 __FUNCTION__);
	  break;
	}
    }
}


/**
 * @ingroup MasterConfig
 * @brief Set the value of the syncronization event interval
 *
 *
 * @param  blk_interval
 *      Sync Event will occur in the last event of the set blk_interval (number of blocks)
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusSetSyncEventInterval(int blk_interval)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(!tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Master.\n",__FUNCTION__);
      return ERROR;
    }

  if(blk_interval>TIPUS_SYNCEVENTCTRL_NBLOCKS_MASK)
    {
      printf("%s: WARN: Value for blk_interval (%d) too large.  Setting to %d\n",
	     __FUNCTION__,blk_interval,TIPUS_SYNCEVENTCTRL_NBLOCKS_MASK);
      blk_interval = TIPUS_SYNCEVENTCTRL_NBLOCKS_MASK;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->syncEventCtrl, blk_interval);
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup MasterStatus
 * @brief Get the SyncEvent Block interval
 * @return Block interval of the SyncEvent
 */
int
tipusGetSyncEventInterval()
{
  int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(!tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Master.\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = tipusRead(&TIPUSp->syncEventCtrl) & TIPUS_SYNCEVENTCTRL_NBLOCKS_MASK;
  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup MasterReadout
 * @brief Force a sync event (type = 0).
 * @return OK if successful, otherwise ERROR
 */
int
tipusForceSyncEvent()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(!tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Master.\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->reset, TIPUS_RESET_FORCE_SYNCEVENT);
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup Readout
 * @brief Sync Reset Request is sent to TI-Master or TS.
 *
 *    This option is available for multicrate systems when the
 *    synchronization is suspect.  It should be exercised only during
 *    "sync events" where the requested sync reset will immediately
 *    follow all ROCs concluding their readout.
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusSyncResetRequest()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  tipusDoSyncResetRequest=1;
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup MasterReadout
 * @brief Determine if a TI has requested a Sync Reset
 *
 * @return 1 if requested received, 0 if not, otherwise ERROR
 */
int
tipusGetSyncResetRequest()
{
  int request=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(!tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Master.\n",__FUNCTION__);
      return ERROR;
    }


  TIPUSLOCK;
  request = (tipusRead(&TIPUSp->blockBuffer) & TIPUS_BLOCKBUFFER_SYNCRESET_REQUESTED)>>30;
  TIPUSUNLOCK;

  return request;
}

/**
 * @ingroup MasterConfig
 * @brief Configure which ports (and self) to enable response of a SyncReset request.
 * @param portMask Mask of ports to enable (port 1 = bit 0)
 * @param self 1 to enable self, 0 to disable
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusEnableSyncResetRequest(unsigned int portMask, int self)
{
  if(TIPUSp == NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(!tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Master.\n",__FUNCTION__);
      return ERROR;
    }

  if(portMask > 0x1)
    {
      printf("%s: ERROR: Invalid portMask (0x%x)\n",
	     __FUNCTION__, portMask);
      return ERROR;
    }

  /* Mask sure self is binary */
  if(self)
    self = 1;
  else
    self = 0;

  TIPUSLOCK;
  tipusWrite(&TIPUSp->rocEnable,
	     (tipusRead(&TIPUSp->rocEnable) & TIPUS_ROCENABLE_MASK) |
	     (portMask << 11) | (self << 10) );
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup MasterStatus
 * @brief Status of SyncReset Request received bits.
 * @param pflag Print to standard out if not 0
 * @return Port mask of SyncReset Request received (port 1 = bit 0, TI-Master = bit 8), otherwise ERROR;
 */
int
tipusSyncResetRequestStatus(int pflag)
{
  int self = 0, rval = 0, ibit = 0;
  if(TIPUSp == NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(!tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Master.\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = (int)(tipusRead(&TIPUSp->rocEnable) & TIPUS_ROCENABLE_SYNCRESET_REQUEST_MONITOR_MASK);
  TIPUSUNLOCK;

  /* Reorganize the bits */
  if(rval)
    {
      self = (rval & 0x1);
      rval = rval >> 1;
      rval = rval | (self<<8);
    }

  if(pflag)
    {
      if(rval)
	{
	  printf("    ***** SyncReset Requested from ");

	  for(ibit = 0; ibit < 8; ibit++)
	    {
	      printf("%d ", ibit + 1);
	    }

	  if(rval & (1 << 8))
	    {
	      printf("SELF ");
	    }

	  printf("*****\n");
	}
      else
	{
	  printf("    No SyncReset Requested\n");
	}
    }

  return rval;
}

/**
 * @ingroup MasterConfig
 * @brief Reset the registers that record the triggers enabled status of TI Slaves.
 *
 */
void
tipusTriggerReadyReset()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return;
    }

  if(!tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Master.\n",__FUNCTION__);
      return;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->syncCommand,TIPUS_SYNCCOMMAND_TRIGGER_READY_RESET);
  TIPUSUNLOCK;


}

/**
 * @ingroup MasterReadout
 * @brief Generate non-physics triggers until the current block is filled.
 *    This feature is useful for "end of run" situations.
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusFillToEndBlock()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(!tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Master.\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->reset, TIPUS_RESET_FILL_TO_END_BLOCK);
  TIPUSUNLOCK;

  printf("tipusFillToEndBlock() is done\n");

  return OK;
}

/**
 * @ingroup MasterConfig
 * @brief Reset the MGT
 * @return OK if successful, otherwise ERROR
 */
int
tipusResetMGT()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  /*sergey
  if(!tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Master.\n",__FUNCTION__);
      return ERROR;
    }
  sergey*/

  /*sergey*/
  printf("MGTMGTMGTMGTMGTMGTMGTMGTMGTMGT ...\n");
  /*sergey*/

  TIPUSLOCK;
  tipusWrite(&TIPUSp->reset, TIPUS_RESET_MGT);
  TIPUSUNLOCK;

  /*sergey usleep(10);*/sleep(1);

  /*sergey*/
  printf("MGTMGTMGTMGTMGTMGTMGTMGTMGTMGT ...\n");
  /*sergey*/

  return OK;
}

/**
 * @ingroup MasterConfig
 * @brief Reset the MGT Receiver
 * @return OK if successful, otherwise ERROR
 */
int
tipusResetMGTReceiver()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(!tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Master.\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->reset, TIPUS_RESET_MGT_RECEIVER);
  TIPUSUNLOCK;
  usleep(10);

  return OK;
}

/**
 * @ingroup Config
 * @brief Set the input delay for the specified front panel TSinput (1-6)
 * @param chan Front Panel TSInput Channel (1-6)
 * @param delay Delay in units of 4ns (0=8ns)
 * @return OK if successful, otherwise ERROR
 */
int
tipusSetTSInputDelay(int chan, int delay)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if((chan<1) || (chan>6))
    {
      printf("%s: ERROR: Invalid chan (%d)\n",__FUNCTION__,
	     chan);
      return ERROR;
    }

  if((delay<0) || (delay>0x1ff))
    {
      printf("%s: ERROR: Invalid delay (%d)\n",__FUNCTION__,
	     delay);
      return ERROR;
    }

  TIPUSLOCK;
  chan--;
  tipusWrite(&TIPUSp->fpDelay[chan/3],
	     (tipusRead(&TIPUSp->fpDelay[chan/3]) & ~TIPUS_FPDELAY_MASK(chan))
	     | delay<<(10*(chan%3)));
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup Status
 * @brief Get the input delay for the specified front panel TSinput (1-6)
 * @param chan Front Panel TSInput Channel (1-6)
 * @return Channel delay (units of 4ns) if successful, otherwise ERROR
 */
int
tipusGetTSInputDelay(int chan)
{
  int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if((chan<1) || (chan>6))
    {
      printf("%s: ERROR: Invalid chan (%d)\n",__FUNCTION__,
	     chan);
      return ERROR;
    }

  TIPUSLOCK;
  chan--;
  rval = (tipusRead(&TIPUSp->fpDelay[chan/3]) & TIPUS_FPDELAY_MASK(chan))>>(10*(chan%3));
  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Print Front Panel TSinput Delays to Standard Out
 * @return OK if successful, otherwise ERROR
 */
int
tipusPrintTSInputDelay()
{
  unsigned int reg[2];
  int ireg=0, ichan=0, delay=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  for(ireg=0; ireg<2; ireg++)
    reg[ireg] = tipusRead(&TIPUSp->fpDelay[ireg]);
  TIPUSUNLOCK;

  printf("%s: Front panel delays:", __FUNCTION__);
  for(ichan=0;ichan<6;ichan++)
    {
      delay = reg[ichan/3] & TIPUS_FPDELAY_MASK(ichan)>>(10*(ichan%3));
      if((ichan%4)==0)
	{
	  printf("\n");
	}
      printf("Chan %2d: %5d   ",ichan+1,delay);
    }
  printf("\n");

  return OK;
}

/**
 * @ingroup Status
 * @brief Return value of buffer length from GTP
 * @return value of buffer length from GTP
 */
unsigned int
tipusGetGTPBufferLength(int pflag)
{
  unsigned int rval=0;

  TIPUSLOCK;
  rval = tipusRead(&TIPUSp->GTPtriggerBufferLength);
  TIPUSUNLOCK;

  if(pflag)
    printf("%s: 0x%08x\n",__FUNCTION__,rval);

  return rval;
}

/**
 * @ingroup MasterStatus
 * @brief Returns the mask of fiber channels that report a "connected"
 *     status from a TI.
 *
 * @return Fiber Connected Mask
 */
int
tipusGetConnectedFiberMask()
{
  int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(!tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Master.\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = (tipusRead(&TIPUSp->fiber) & TIPUS_FIBER_CONNECTED_MASK)>>16;
  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup MasterStatus
 * @brief Returns the mask of fiber channels that report a "connected"
 *     status from a TI has it's trigger source enabled.
 *
 * @return Trigger Source Enabled Mask
 */
int
tipusGetTrigSrcEnabledFiberMask()
{
  int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(!tipusMaster)
    {
      printf("%s: ERROR: TI is not the TI Master.\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = (tipusRead(&TIPUSp->fiber) & TIPUS_FIBER_TRIGSRC_ENABLED_MASK)>>24;
  TIPUSUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Enable the readout fifo in BAR0 for block data readout, instead of DMA.
 * @return OK if successful, otherwise ERROR
 */
int
tipusEnableFifo()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  /* Disable DMA readout */
  tipusWrite(&TIPUSp->vmeControl,
	     tipusRead(&TIPUSp->vmeControl) &~TIPUS_VMECONTROL_DMASETTING_MASK);

  /* Enable FIFO */
  tipusWrite(&TIPUSp->rocEnable, TIPUS_ROCENABLE_FIFO_ENABLE);
  TIPUSUNLOCK;

  return OK;
}

#ifdef OLDDMA
/**
 * @ingroup Config
 * @brief Configure the Direct Memory Access (DMA) for the TI
 *
 * @param packet_size TLP Maximum Packet Size
 *   1 - 128 B
 *   2 - 256 B
 *   4 - 512 B
 *
 * @param adr_mode TLP Address Mode
 *   0 - 32bit/3 header mode
 *   1 - 64bit/4 header mode
 *
 * @param dma_size DMA memory size to allocate
 *   1 - 1 MB
 *   2 - 2 MB
 *   3 - 4 MB
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusDmaConfig(int packet_size, int adr_mode, int dma_size)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if( (packet_size!=1) && (packet_size!=2) && (packet_size!=4) )
    {
      printf("%s: ERROR: Invalid packet_size (%d)\n",
	     __FUNCTION__,packet_size);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->dmaSetting,
	     (tipusRead(&TIPUSp->dmaSetting) & TIPUS_DMASETTING_PHYS_ADDR_HI_MASK) |
	     (packet_size<<24) |
	     (dma_size<<28) |
	     (adr_mode<<31) );

  tipusWrite(&TIPUSp->vmeControl,
	     tipusRead(&TIPUSp->vmeControl) | TIPUS_VMECONTROL_DMA_DATA_ENABLE);
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup Config
 * @brief Set the physical memory address for DMA
 *
 * @param phys_addr_lo Low 32 bits of memory address
 *
 * @param phys_addr_hi High 16 bits of memory address
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusDmaSetAddr(unsigned int phys_addr_lo, unsigned int phys_addr_hi)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  if(phys_addr_hi>0)
    {
      tipusWrite(&TIPUSp->dmaSetting,
		 (tipusRead(&TIPUSp->dmaSetting) & ~TIPUS_DMASETTING_PHYS_ADDR_HI_MASK) |
		 (phys_addr_hi & TIPUS_DMASETTING_PHYS_ADDR_HI_MASK));
    }

  tipusWrite(&TIPUSp->dmaAddr, phys_addr_lo);
  TIPUSUNLOCK;

  return OK;
}

/**
 * @ingroup Status
 * @brief Show the PCIE status
 *
 *  FIXME: Regs not relevant for current hw version?
 *
 * @param pflag Print Flag
 *   !0 - Print out raw registers
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusPCIEStatus(int pflag)
{
  unsigned int dmaSetting, dmaAddr,
    pcieConfigLink, pcieConfigStatus, pcieConfig, pcieDevConfig;
  unsigned long TIBase=0;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  dmaSetting       = tipusRead(&TIPUSp->dmaSetting);
  dmaAddr          = tipusRead(&TIPUSp->dmaAddr);
  pcieConfigLink   = tipusRead(&TIPUSp->pcieConfigLink);
  pcieConfigStatus = tipusRead(&TIPUSp->pcieConfigStatus);
  pcieConfig       = tipusRead(&TIPUSp->pcieConfig);
  pcieDevConfig    = tipusRead(&TIPUSp->pcieDevConfig);
  TIPUSUNLOCK;

  TIBase = (unsigned long)TIPUSp;

  printf("\n");
  printf("PCIE STATUS for Tipcieus\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("\n");

  if(pflag)
    {
      printf(" Registers (offset):\n");

      printf(" dmaSetting       (0x%04lx) = 0x%08x ",
	     (unsigned long)&TIPUSp->dmaSetting - TIBase, dmaSetting);
      printf(" dmaAddr          (0x%04lx) = 0x%08x\n",
	     (unsigned long)&TIPUSp->dmaAddr - TIBase, dmaAddr);

      printf(" pcieConfigLink   (0x%04lx) = 0x%08x ",
	     (unsigned long)&TIPUSp->pcieConfigLink - TIBase, pcieConfigLink);
      printf(" pcieConfigStatus (0x%04lx) = 0x%08x\n",
	     (unsigned long)&TIPUSp->pcieConfigStatus - TIBase, pcieConfigStatus);

      printf(" pcieConfig       (0x%04lx) = 0x%08x ",
	     (unsigned long)&TIPUSp->pcieConfig - TIBase, pcieConfig);
      printf(" pcieDevConfig    (0x%04lx) = 0x%08x\n",
	     (unsigned long)&TIPUSp->pcieDevConfig - TIBase, pcieDevConfig);
      printf("\n");
    }

  printf("  Physical Memory Address = 0x%04x %08x \n",
	 dmaSetting&TIPUS_DMASETTING_PHYS_ADDR_HI_MASK,
	 dmaAddr);
  printf("  DMA Size = %d MB\n",
	 ((dmaSetting&TIPUS_DMASETTING_DMA_SIZE_MASK)>>24)==1?1:
	 ((dmaSetting&TIPUS_DMASETTING_DMA_SIZE_MASK)>>24)==2?2:
	 ((dmaSetting&TIPUS_DMASETTING_DMA_SIZE_MASK)>>24)==3?4:0);
  printf("  TLP:  Address Mode = %s   Packet Size = %d\n",
	 (dmaSetting&TIPUS_DMASETTING_ADDR_MODE_MASK)?
	 "64 bit / 4 header":
	 "32 bit / 3 header",
	 ((dmaSetting&TIPUS_DMASETTING_MAX_PACKET_SIZE_MASK)>>28)==1?128:
	 ((dmaSetting&TIPUS_DMASETTING_MAX_PACKET_SIZE_MASK)>>28)==2?256:
	 ((dmaSetting&TIPUS_DMASETTING_MAX_PACKET_SIZE_MASK)>>28)==4?512:0);

  printf("\n");

  printf("--------------------------------------------------------------------------------\n");
  printf("\n\n");

  return OK;
}
#endif /* OLDDMA */

#ifdef NOTDONEYET

/*************************************************************
 Library Interrupt/Polling routines
*************************************************************/

/*******************************************************************************
 *
 *  tiInt
 *  - Default interrupt handler
 *    Handles the TI interrupt.  Calls a user defined routine,
 *    if it was connected with tiIntConnect()
 *
 */
static void
tiInt(void)
{
  tiIntCount++;

  INTLOCK;

  if (tiIntRoutine != NULL)	/* call user routine */
    (*tiIntRoutine) (tiIntArg);

  /* Acknowledge trigger */
  if(tiDoAck==1)
    {
      tiIntAck();
    }
  INTUNLOCK;

}
#endif /* NOTDONEYET */

/*******************************************************************************
 *
 *  tipusPoll
 *  - Default Polling Server Thread
 *    Handles the polling of latched triggers.  Calls a user
 *    defined routine if was connected with tipusIntConnect.
 *
 */
static void
tipusPoll(void)
{
  int tidata;
  int policy=0;
  struct sched_param sp;
  /* #define DO_CPUAFFINITY */
#ifdef DO_CPUAFFINITY
  int j;
  cpu_set_t testCPU;

  if (pthread_getaffinity_np(pthread_self(), sizeof(testCPU), &testCPU) <0)
    {
      perror("pthread_getaffinity_np");
    }
  printf("tipusPoll: CPUset = ");
  for (j = 0; j < CPU_SETSIZE; j++)
    if (CPU_ISSET(j, &testCPU))
      printf(" %d", j);
  printf("\n");

  CPU_ZERO(&testCPU);
  CPU_SET(1,&testCPU);
  if (pthread_setaffinity_np(pthread_self(),sizeof(testCPU), &testCPU) <0)
    {
      perror("pthread_setaffinity_np");
    }
  if (pthread_getaffinity_np(pthread_self(), sizeof(testCPU), &testCPU) <0)
    {
      perror("pthread_getaffinity_np");
    }

  printf("tipusPoll: CPUset = ");
  for (j = 0; j < CPU_SETSIZE; j++)
    if (CPU_ISSET(j, &testCPU))
      printf(" %d", j);

  printf("\n");


#endif

  /* Set scheduler and priority for this thread */

  //policy=SCHED_OTHER;
  //sp.sched_priority=40;
  policy=SCHED_FIFO;
  sp.sched_priority=80;

  printf("%s: Entering polling loop...\n",__FUNCTION__);
  pthread_setschedparam(pthread_self(),policy,&sp);
  pthread_getschedparam(pthread_self(),&policy,&sp);
  printf ("%s: INFO: Running at %s/%d\n",__FUNCTION__,
	  (policy == SCHED_FIFO ? "FIFO"
	   : (policy == SCHED_RR ? "RR"
	      : (policy == SCHED_OTHER ? "OTHER"
		 : "unknown"))), sp.sched_priority);
  prctl(PR_SET_NAME,"tipusPoll");

  while(1)
    {
      usleep(1);
      pthread_testcancel();

      /* If still need Ack, don't test the Trigger Status */
      if(tipusNeedAck>0)
	{
	  continue;
	}

      tidata = 0;

      tidata = tipusBReady();
      if(tidata == ERROR)
	{
	  printf("%s: ERROR: tiIntPoll returned ERROR.\n",__FUNCTION__);
	  break;
	}

      if(tidata && tipusIntRunning)
	{
	  INTLOCK;
	  tipusDaqCount = tidata;
	  tipusIntCount++;

	  if (tipusIntRoutine != NULL)	/* call user routine */
	    {
	      (*tipusIntRoutine) (tipusIntArg);
	    }

	  /* Write to TI to Acknowledge Interrupt */
	  if(tipusDoAck==1)
	    {
	      tipusIntAck();
	    }


	  INTUNLOCK;
	}

    }
  printf("%s: Read ERROR: Exiting Thread\n",__FUNCTION__);
  pthread_exit(0);

}

/*******************************************************************************
 *
 * tipusDoLibraryPollingThread - Set the decision on whether or not the
 *      TIR library should perform the trigger polling via thread.
 *
 *   choice:   0 - No Thread Polling
 *             1 - Library Thread Polling (started with tirIntEnable)
 *
 *
 * RETURNS: OK, or ERROR .
 */

int
tipusDoLibraryPollingThread(int choice)
{
  if(choice)
    tipusDoIntPolling=1;
  else
    tipusDoIntPolling=0;

  return tipusDoIntPolling;
}

/*******************************************************************************
 *
 *  tipusStartPollingThread
 *  - Routine that launches tipusoll in its own thread
 *
 */
static void
tipusStartPollingThread(void)
{
  int ti_status;

  ti_status =
    pthread_create(&tipuspollthread,
		   NULL,
		   (void*(*)(void *)) tipusPoll,
		   (void *)NULL);
  if(ti_status!=0)
    {
      printf("%s: ERROR: TI Polling Thread could not be started.\n",
	     __FUNCTION__);
      printf("\t pthread_create returned: %d\n",ti_status);
    }

}

/**
 * @ingroup IntPoll
 * @brief Connect a user routine to the TI Interrupt or
 *    latched trigger, if polling.
 *
 * @param vector VME Interrupt Vector
 * @param routine Routine to call if block is available
 * @param arg argument to pass to routine
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusIntConnect(unsigned int vector, VOIDFUNCPTR routine, unsigned int arg)
{
  int status;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return(ERROR);
    }


  tipusIntCount = 0;
  tipusAckCount = 0;
  tipusDoAck = 1;

  /* Set Vector and Level */
  if((vector < 0xFF)&&(vector > 0x40))
    {
      tipusIntVec = vector;
    }
  else
    {
      tipusIntVec = TIPUS_INT_VEC;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->intsetup, (tipusIntLevel<<8) | tipusIntVec );
  TIPUSUNLOCK;

  status=0;

  switch (tipusReadoutMode)
    {
    case TIPUS_READOUT_TS_POLL:
    case TIPUS_READOUT_EXT_POLL:
      break;

    case TIPUS_READOUT_TS_INT:
    case TIPUS_READOUT_EXT_INT:
#ifdef NOTDONEYET
      status = vmeIntConnect (tiIntVec, tiIntLevel,
			      tiInt,arg);
      if (status != OK)
	{
	  printf("%s: vmeIntConnect failed with status = 0x%08x\n",
		 __FUNCTION__,status);
	  return(ERROR);
	}
      break;
#else
      printf("%s: ERROR: Interrupt Mode (%d) not yet supported\n",
	     __FUNCTION__,tipusReadoutMode);
      return ERROR;
#endif
    default:
      printf("%s: ERROR: TI Mode not defined (%d)\n",
	     __FUNCTION__,tipusReadoutMode);
      return ERROR;
    }

  printf("%s: INFO: Interrupt Vector = 0x%x  Level = %d\n",
	 __FUNCTION__,tipusIntVec,tipusIntLevel);

  if(routine)
    {
      tipusIntRoutine = routine;
      tipusIntArg = arg;
    }
  else
    {
      tipusIntRoutine = NULL;
      tipusIntArg = 0;
    }

  return(OK);

}

/**
 * @ingroup IntPoll
 * @brief Disable interrupts or kill the polling service thread
 *
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusIntDisconnect()
{
  int status;
  void *res;

  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(tipusIntRunning)
    {
      printf("%s: ERROR: TI is Enabled - Call tipusIntDisable() first\n",
	     __FUNCTION__);
      return ERROR;
    }

  INTLOCK;

  status=0;

  switch (tipusReadoutMode)
    {
    case TIPUS_READOUT_TS_POLL:
    case TIPUS_READOUT_EXT_POLL:
      {
	if(tipuspollthread)
	  {
	    if(pthread_cancel(tipuspollthread)<0)
	      perror("pthread_cancel");
	    if(pthread_join(tipuspollthread,&res)<0)
	      perror("pthread_join");
	    if (res == PTHREAD_CANCELED)
	      printf("%s: Polling thread canceled\n",__FUNCTION__);
	    else
	      printf("%s: ERROR: Polling thread NOT canceled\n",__FUNCTION__);
	  }
      }
      break;
    case TIPUS_READOUT_TS_INT:
    case TIPUS_READOUT_EXT_INT:
#ifdef NOTDONEYET
      status = vmeIntDisconnect(tiIntLevel);
      if (status != OK)
	{
	  printf("vmeIntDisconnect failed\n");
	}
      break;
#else
      printf("%s: ERROR: Interrupt mode not yet supported\n",
	     __func__);
#endif
    default:
      break;
    }

  INTUNLOCK;

  printf("%s: Disconnected\n",__FUNCTION__);

  return OK;

}

/**
 * @ingroup IntPoll
 * @brief Connect a user routine to be executed instead of the default
 *  TI interrupt/trigger latching acknowledge prescription
 *
 * @param routine Routine to call
 * @param arg argument to pass to routine
 * @return OK if successful, otherwise ERROR
 */
int
tipusAckConnect(VOIDFUNCPTR routine, unsigned int arg)
{
  if(routine)
    {
      tipusAckRoutine = routine;
      tipusAckArg = arg;
    }
  else
    {
      printf("%s: WARN: routine undefined.\n",__FUNCTION__);
      tipusAckRoutine = NULL;
      tipusAckArg = 0;
      return ERROR;
    }
  return OK;
}

/**
 * @ingroup IntPoll
 * @brief Acknowledge an interrupt or latched trigger.  This "should" effectively
 *  release the "Busy" state of the TI.
 *
 *  Execute a user defined routine, if it is defined.  Otherwise, use
 *  a default prescription.
 */
void
tipusIntAck()
{
  int resetbits=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return;
    }

  if (tipusAckRoutine != NULL)
    {
      /* Execute user defined Acknowlege, if it was defined */
      TIPUSLOCK;
      (*tipusAckRoutine) (tipusAckArg);
      TIPUSUNLOCK;
    }
  else
    {
      TIPUSLOCK;
      tipusDoAck = 1;
      tipusAckCount++;
      resetbits = TIPUS_RESET_BUSYACK;

      if(!tipusReadoutEnabled)
	{
	  /* Readout Acknowledge and decrease the number of available blocks by 1 */
	  resetbits |= TIPUS_RESET_BLOCK_READOUT;
	}

      if(tipusDoSyncResetRequest)
	{
	  resetbits |= TIPUS_RESET_SYNCRESET_REQUEST;
	  tipusDoSyncResetRequest=0;
	}

      tipusWrite(&TIPUSp->reset, resetbits);

      tipusNReadoutEvents = 0;
      TIPUSUNLOCK;
    }


}

/**
 * @ingroup IntPoll
 * @brief Enable interrupts or latching triggers (depending on set TI mode)
 *
 * @param iflag if = 1, trigger counter will be reset
 *
 * @return OK if successful, otherwise ERROR
 */
int
tipusIntEnable(int iflag)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return(-1);
    }

  TIPUSLOCK;
  if(iflag == 1)
    {
      tipusIntCount = 0;
      tipusAckCount = 0;
    }

  tipusIntRunning = 1;
  tipusDoAck      = 1;
  tipusNeedAck    = 0;

  switch (tipusReadoutMode)
    {
    case TIPUS_READOUT_TS_POLL:
    case TIPUS_READOUT_EXT_POLL:
      if(tipusDoIntPolling)
	tipusStartPollingThread();
      break;

    case TIPUS_READOUT_TS_INT:
    case TIPUS_READOUT_EXT_INT:
#ifdef NOTDONEYET
      printf("%s: ******* ENABLE INTERRUPTS *******\n",__FUNCTION__);
      tipusWrite(&TIPUSp->intsetup,
		 tipusRead(&TIPUSp->intsetup) | TIPUS_INTSETUP_ENABLE );
      break;
#endif
    default:
      tipusIntRunning = 0;
      printf("%s: ERROR: TI Readout Mode not defined %d\n",
	     __FUNCTION__,tipusReadoutMode);
      TIPUSUNLOCK;
      return(ERROR);

    }

  tipusWrite(&TIPUSp->runningMode,0x71);
  TIPUSUNLOCK; /* Locks performed in tipusEnableTriggerSource() */

  //usleep(300000);
  sleep(1); //sergey

  tipusEnableTriggerSource();

  return(OK);

}

/**
 * @ingroup IntPoll
 * @brief Disable interrupts or latching triggers
 *
 */
void
tipusIntDisable()
{

  if(TIPUSp==NULL)
  {
    printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
    return;
  }

  tipusDisableTriggerSource(1);
  printf("tipusIntDisable(): just called tipusDisableTriggerSource(1)\n");fflush(stdout);

  TIPUSLOCK;
  tipusWrite(&TIPUSp->intsetup,
	     tipusRead(&TIPUSp->intsetup) & ~(TIPUS_INTSETUP_ENABLE));
  tipusWrite(&TIPUSp->runningMode,0x0);
  tipusIntRunning = 0;
  TIPUSUNLOCK;

  printf("tipusIntDisable() finishing ...\n");fflush(stdout);
  tipusStatus(1);
  printf("tipusIntDisable() done\n");fflush(stdout);
}

/**
 * @ingroup Status
 * @brief Return current readout count
 */
unsigned int
tipusGetIntCount()
{
  unsigned int rval=0;

  TIPUSLOCK;
  rval = tipusIntCount;
  TIPUSUNLOCK;

  return(rval);
}

/**
 * @ingroup Status
 * @brief Return current acknowledge count
 */
unsigned int
tipusGetAckCount()
{
  unsigned int rval=0;

  TIPUSLOCK;
  rval = tipusAckCount;
  TIPUSUNLOCK;

  return(rval);
}

/**
 * @ingroup Status
 * @brief Read the fiber fifo from the TI
 *
 * @param   fiber - Fiber fifo to read. 1 and 5 only supported.
 * @param   data  - local memory address to place data
 * @param  maxwords - Maximum number of 32bit words to put into data array.
 *
 * @return Number of words transferred to data if successful, ERROR otherwise
 *
 */
int
tipusReadFiberFifo(int fiber, volatile unsigned int *data, int maxwords)
{
  int nwords = 0;
  unsigned int word = 0;

  if(data==NULL)
    {
      printf("%s: ERROR: Invalid Destination address\n",
	     __func__);
      return(ERROR);
    }

  if((fiber != 1) && (fiber !=5))
    {
      printf("%s: Invalid fiber (%d)\n",
	     __func__, fiber);
      return ERROR;
    }

  TIPUSLOCK;
  while(nwords < maxwords)
    {
      if(fiber == 1)
	word = tipusRead(&TIPUSp->trigTable[12]);
      else
      	word = tipusRead(&TIPUSp->trigTable[13]);

      if(word & (1<<31))
	break;

      data[nwords++] = word;
    }
  TIPUSUNLOCK;

  return nwords;
}


/**
 * @ingroup Status
 * @brief Read the fiber fifo from the TI and print to standard out.
 *
 * @param   fiber - Fiber fifo to read. 1 and 5 only supported.
 *
 * @return OK if successful, ERROR otherwise
 *
 */
int
tipusPrintFiberFifo(int fiber)
{
  volatile unsigned int *data;
  int maxwords = 256, iword, rwords = 0;

  if((fiber != 1) && (fiber !=5))
    {
      printf("%s: Invalid fiber (%d)\n",
	     __func__, fiber);
      return ERROR;
    }

  data = (volatile unsigned int *)malloc(maxwords * sizeof(unsigned int));
  if(!data)
    {
      printf("%s: Unable to acquire memory\n",
	     __func__);
      return ERROR;
    }

  rwords = tipusReadFiberFifo(fiber, data, maxwords);

  if(rwords == 0)
    {
      printf("%s: No data in fifo\n\n",
	     __func__);
      return OK;
    }
  else if(rwords == ERROR)
    {
      printf("%s: tipusReadFiberFifo(..) returned ERROR\n",
	     __func__);
      return ERROR;
    }

  printf(" Fiber %d fifo (%d words)\n",
	 fiber, rwords);
  printf("      Timestamp     Data\n");
  printf("----------------------------\n");
  for(iword = 0; iword < rwords; iword++)
    {
      printf("%3d:    0x%04x     0x%04x\n",
	     iword,
	     (data[iword] & 0xFFFF0000)>>16,
	     (data[iword] & 0xFFFF));
    }
  printf("----------------------------\n");
  printf("\n");

  if(data)
    free((void *)data);

  return OK;
}


#ifdef NOTSUPPORTED
/* Module TI Routines */
int
tipusRocEnable(int roc)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if((roc<1) || (roc>8))
    {
      printf("%s: ERROR: Invalid roc (%d)\n",
	     __FUNCTION__,roc);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->rocEnable, (tipusRead(&TIPUSp->rocEnable) & TIPUS_ROCENABLE_MASK) |
	     TIPUS_ROCENABLE_ROC(roc-1));
  TIPUSUNLOCK;

  return OK;
}

int
tipusRocEnableMask(int rocmask)
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(rocmask>TIPUS_ROCENABLE_MASK)
    {
      printf("%s: ERROR: Invalid rocmask (0x%x)\n",
	     __FUNCTION__,rocmask);
      return ERROR;
    }

  TIPUSLOCK;
  tipusWrite(&TIPUSp->rocEnable, rocmask);
  TIPUSUNLOCK;

  return OK;
}

int
tipusGetRocEnableMask()
{
  int rval=0;
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TIPUSLOCK;
  rval = tipusRead(&TIPUSp->rocEnable) & TIPUS_ROCENABLE_MASK;
  TIPUSUNLOCK;

  return rval;
}
#endif /* NOTSUPPORTED */

unsigned int
tipusRead(volatile unsigned int *reg)
{
  unsigned int value=0;
  value = *reg;

  return value;
}

int
tipusWrite(volatile unsigned int *reg, unsigned int value)
{
  int stat=0;
  *reg = value;

  return stat;
}

unsigned int
tipusJTAGRead(unsigned int reg)
{
  unsigned int value=0;
#ifdef MAPJTAG
  value = *(TIPUSpj+(reg>>2));
#else
  printf("%s: ERROR: JTAG Not supported\n",
	 __func__);
  value = ERROR;
#endif /* MAPJTAG */

  return value;
}

int
tipusJTAGWrite(unsigned int reg, unsigned int value)
{
  int stat=0;

#ifdef MAPJTAG
  *(TIPUSpj+(reg>>2)) = value;
#else
  printf("%s: ERROR: JTAG Not supported\n",
	 __func__);
  stat = ERROR;
#endif /* MAPJTAG */

  return stat;
}

/* Allocate memory for DMA, based on provided blocklevel */

int
tipusAllocDmaMemory(int blocklevel)
{
  int nwords = ((5 * blocklevel) + 4) * 4;

  if((blocklevel < 0) || (blocklevel > 255))
    {
      printf("%s: Invalid blocklevel (%d)\n",
	     __func__, blocklevel);
      return ERROR;
    }

  if(tipusBlockData != NULL)
    {
      /* Free memory that's already allocated */
      free(tipusBlockData);
      tipusBlockData = NULL;
    }

  tipusBlockData = malloc(nwords * sizeof(unsigned int));
  if(tipusBlockData == NULL)
    {
      perror("malloc");
      printf("%s: Unable to allocate local memory for DMA\n",
	     __func__);
      return ERROR;
    }

  return OK;
}

int
tipusOpenDmaDevice()
{
  char *devname = DEVICE_NAME_DEFAULT;
  int fpga_fd = open(devname, O_RDWR | O_NONBLOCK);

  if (fpga_fd < 0)
    {
      perror("open");
      fprintf(stderr, "unable to open device %s, %d.\n",
	      devname, fpga_fd);
      return -EINVAL;
    }

  return fpga_fd;
}

int
tipusCloseDmaDevice(int fd)
{
  char *devname = DEVICE_NAME_DEFAULT;
  int rval = close(fd);
  if(rval < 0)
    {
      perror("close");
      fprintf(stderr, "error closing device %s, %d.\n",
	      devname, fd);
      return rval;
    }

  return OK;
}

int
tipusOpen()
{
  off_t         dev_base;
  unsigned int  bars[3]={0,0,0};

  if(tipusFD>0)
    {
      printf("%s: ERROR: Tipcieus already opened.\n",
	     __FUNCTION__);
      return ERROR;
    }

  tipusFD = open("/dev/xdma0_user",O_RDWR  | O_SYNC);

  if(tipusFD<0)
    {
      perror("tipusOpen: ERROR");
      return ERROR;
    }

#define MAP_SIZE (8*1024UL)
  tipusMappedBase = mmap(0, MAP_SIZE,
			 PROT_READ|PROT_WRITE, MAP_SHARED, tipusFD, 0);
  if (tipusMappedBase == MAP_FAILED)
    {
      perror("mmap");
      return ERROR;
    }

  TIPUSp = (volatile struct TIPCIEUS_RegStruct *)tipusMappedBase;

  /* Open DMA Device */
  int rval = tipusOpenDmaDevice();
  if(rval == ERROR)
    {
      printf("%s: ERROR: Unable to open DMA Device\n",
	     __func__);

      tipusDmaFD = -1;
      return ERROR;
    }
  else
    tipusDmaFD = rval;

  tipusBlockDataSize = 0x1400;
  tipusBlockData = malloc(tipusBlockDataSize * sizeof(unsigned int));
  if(tipusBlockData == NULL)
    {
      perror("malloc");
      printf("%s: Unable to allocate local memory for DMA\n",
	     __func__);
      return ERROR;
    }

  return OK;
}

int
tipusClose()
{
  if(TIPUSp==NULL)
    {
      printf("%s: ERROR: Invalid TIPUS File Descriptor\n",
	     __FUNCTION__);
      return ERROR;
    }

  /* Free Memory for DMA */
  if(tipusBlockData != NULL)
    {
      free(tipusBlockData);
      tipusBlockData = NULL;
    }

  if(tipusDmaFD > 0)
    {
      tipusCloseDmaDevice(tipusDmaFD);
      tipusDmaFD = -1;
    }

#ifdef MAPJTAG
  if(munmap(tipusJTAGMappedBase,0x1000)<0)
    perror("munmap");
#endif /* MAPJTAG */

  if(munmap(tipusMappedBase,MAP_SIZE)<0)
    perror("munmap");


  close(tipusFD);
  return OK;
}



/*sergey: new function*/
int
tipusGetRandomTriggerSetting(int trigger)
{
  int val;
#if 0
  if(TIp==NULL)
  {
    printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
    return ERROR;
  }

  if(trigger!=1 && trigger!=2)
  {
    logMsg("\ntiSetRandomTrigger: ERROR: Invalid trigger type %d\n",trigger,2,3,4,5,6);
    return ERROR;
  }

  TILOCK;
  val = vmeRead32(&TIp->randomPulser);
  TIUNLOCK;

  if(trigger==1)
    return (val>>0) & 0xF;
  else if(trigger==2)
    return (val>>8) & 0xF;
#endif
  return 0;
}

/*sergey-new function*/
int
tipusGetRandomTriggerEnable(int trigger)
{
  int val;
#if 0
  if(TIp==NULL)
  {
    printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
    return ERROR;
  }

  if(trigger!=1 && trigger!=2)
  {
    logMsg("\ntiSetRandomTrigger: ERROR: Invalid trigger type %d\n",trigger,2,3,4,5,6);
    return ERROR;
  }

  TILOCK;
  val = vmeRead32(&TIp->randomPulser);
  TIUNLOCK;

  if((trigger==1) && (val & TI_RANDOMPULSER_TRIG1_ENABLE))
    return 1;
  else if((trigger==2) && (val & TI_RANDOMPULSER_TRIG2_ENABLE))
    return 1;
#endif
  return 0;
}

/*sergey*/
int
tipusGetBlockBufferLevel()
{
  int rval = 0;

#if 0
  if(TIp == NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }

  TILOCK;
  if(vmeRead32(&TIp->vmeControl) & TI_VMECONTROL_USE_LOCAL_BUFFERLEVEL)
    {
      rval = vmeRead32(&TIp->blockBuffer) & TI_BLOCKBUFFER_BUFFERLEVEL_MASK;
    }
  else
    {
      rval = (vmeRead32(&TIp->dataFormat) & TI_DATAFORMAT_BCAST_BUFFERLEVEL_MASK) >> 24;
    }
  TIUNLOCK;
#endif

  return rval;
}

/* sergey: add function */
int
tipusGetTSInputMask()
{
  int ret;

#if 0
  if(TIp == NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return ERROR;
    }
  TILOCK;
  ret = vmeRead32(&TIp->tsInput) & 0x3F; /* sergey: add 0x3f mask */
  printf("tiGetTSInputMask: mask=0x%08x\n",ret);
  TIUNLOCK;
#endif

  return(ret);
}

/* sergey: copied from tiLib and modified */
int
tipusPrintBusyCounters()
{
  //struct TIPCIE_RegStruct ro;
  uint64_t livetime,busytime,totaltime;
  static uint64_t livetime_old,busytime_old,totaltime_old;
  uint64_t livetime_new,busytime_new,totaltime_new;
  /* 
  unsigned int counter[16];
  const char *scounter[16] =
    {
      "SWA    ",
      "SWB    ",
      "P2     ",
      "FP-FTDC",
      "FP-FADC",
      "FP     ",
      "Unused ",
      "Loopack",
      "Fiber 1",
      "Fiber 2",
      "Fiber 3",
      "Fiber 4",
      "Fiber 5",
      "Fiber 6",
      "Fiber 7",
      "Fiber 8"
    };
  int icnt=0;
  */



  TIPUSLOCK;

  /* Latch scalers first */
  tipusWrite(&TIPUSp->reset,TIPUS_RESET_SCALERS_LATCH);

  livetime_new     = tipusRead(&TIPUSp->livetime);
  busytime_new     = tipusRead(&TIPUSp->busytime);

  TIPUSUNLOCK;



  livetime = livetime_new - livetime_old;
  busytime = busytime_new - busytime_old;

  livetime_old = livetime_new;
  busytime_old = busytime_new;


  /* sergey: add total busy and print normalized to livetime */
  totaltime = livetime + busytime;
  printf("\n\n");
  printf(" Livetime           0x%016x (%16lld) [livetime %3d percent]\n",livetime,livetime,(livetime*100)/totaltime);
  printf(" Total busy counter 0x%016x (%16lld) [deadtime %3d percent]\n",busytime,busytime,(busytime*100)/totaltime);
  printf("-------------------------------------------------------------------\n");
  /* not available
  printf(" Busy Counters \n");
  for(icnt=0; icnt<16; icnt++)
  {
    printf("%s             0x%08x (%10d) [deadtime %3d percent]\n",
			 scounter[icnt], counter[icnt], counter[icnt], (counter[icnt]*100)/totaltime);
  }
  printf("-------------------------------------------------------------------\n");
  */
  printf("\n\n");

  return OK;
}

/*sergey: easy to remember, used as often as tiStatus */
int
tipusBusy()
{
  if(tipusMaster) tipusPrintBusyCounters();
}

int
tipusGetNumberOfBlocksInBuffer()
{
  int blockBuffer;
  int nblocks;
  blockBuffer  = tipusRead(&TIPUSp->blockBuffer);
  nblocks = (blockBuffer&TIPUS_BLOCKBUFFER_BLOCKS_READY_MASK)>>8;
  return(nblocks);
}

