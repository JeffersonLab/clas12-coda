/******************************************************************************
*
*  dcrbLib.h  - Driver library header file for readout of the dcrb boards
*
*  Author: Ben Raydo
*          Jefferson Lab Data Acquisition Group
*
*/

#if defined(VXWORKS) || defined(Linux_vme)

#ifdef VXWORKS
#include <vxWorks.h>
/*#include "vxCompat.h"*/
#else
#include <stddef.h>
#include <pthread.h>
#include "jvme.h"
#endif
#include <stdio.h>
#include <string.h>
#ifdef VXWORKS
#include <logLib.h>
#include <taskLib.h>
#include <intLib.h>
#include <iv.h>
#include <semLib.h>
#include <vxLib.h>
#else
#include <unistd.h>
#endif


/* Include DCRB definitions */
#include "dcrbLib.h"

#include "xxxConfig.h"
static int active;

#ifdef VXWORKS
#define DCRBLOCK
#define DCRBUNLOCK
#else
/* Mutex to guard flexio read/writes */
pthread_mutex_t   dcrbMutex = PTHREAD_MUTEX_INITIALIZER;
#define DCRBLOCK      if(pthread_mutex_lock(&dcrbMutex)<0) perror("pthread_mutex_lock");
#define DCRBUNLOCK    if(pthread_mutex_unlock(&dcrbMutex)<0) perror("pthread_mutex_unlock");
#endif

#define LSWAP(x)        ((((x) & 0x000000ff) << 24) | \
                         (((x) & 0x0000ff00) <<  8) | \
                         (((x) & 0x00ff0000) >>  8) | \
                         (((x) & 0xff000000) >> 24))

/* Define external Functions */
#ifdef VXWORKS
IMPORT  STATUS sysBusToLocalAdrs(int, char *, char **);
IMPORT  STATUS intDisconnect(int);
IMPORT  STATUS sysIntEnable(int);
IMPORT  STATUS sysIntDisable(int);
/*
IMPORT  STATUS sysVmeDmaDone(int, int);
IMPORT  STATUS sysVmeDmaSend(UINT32, UINT32, int, BOOL);
*/

#define EIEIO    __asm__ volatile ("eieio")
#define SYNC     __asm__ volatile ("sync")
#endif

/* Define Interrupts variables */
BOOL              dcrbIntRunning  = FALSE;                    /* running flag */
int               dcrbIntID       = -1;                       /* id number of DCRB generating interrupts */
LOCAL VOIDFUNCPTR dcrbIntRoutine  = NULL;                     /* user interrupt service routine */
LOCAL int         dcrbIntArg      = 0;                        /* arg to user routine */
LOCAL UINT32      dcrbIntLevel    = DCRB_VME_INT_LEVEL;         /* default VME interrupt level */
LOCAL UINT32      dcrbIntVec      = DCRB_VME_INT_VEC;           /* default interrupt Vector */

/* Define global variables */
int ndcrb = 0;                                       /* Number of DCRBs in Crate */
int dcrbA32Base   = 0x09000000;                      /* Minimum VME A32 Address for use by DCRBs */
int dcrbA32Offset = 0x08000000;                      /* Difference in CPU A32 Base - VME A32 Base */
int dcrbA24Offset = 0x0;                             /* Difference in CPU A24 Base - VME A24 Base */
volatile dcrb_regs *DCRBp[(DCRB_MAX_BOARDS+1)];      /* pointers to DCRB memory map */
volatile unsigned int *DCRBpd[(DCRB_MAX_BOARDS+1)];  /* pointers to DCRB FIFO memory */
volatile unsigned int *DCRBpmb;                      /* pointer to Multblock window */
int dcrbID[DCRB_MAX_BOARDS+1];                       /* array of slot numbers for DCRBs */
unsigned int dcrbAddrList[DCRB_MAX_BOARDS+1];        /* array of a24 addresses for DCRBs */
int dcrbRev[(DCRB_MAX_BOARDS+1)];                    /* Board Revision Info for each module */
int dcrbInited=0;                                    /* >0 if Library has been Initialized before */
int dcrbMaxSlot=0;                                   /* Highest Slot hold an DCRB */
int dcrbMinSlot=0;                                   /* Lowest Slot holding an DCRB */
int dcrbSource=0;                                    /* Signal source for DCRB system control*/
int dcrbBlockLevel=0;                                /* Block Level for DCRBs */
int dcrbIntCount = 0;                                /* Count of interrupts from DCRB */

int
dcrbSlot(unsigned int id)
{
  if(id>=ndcrb)
  {
    printf("%s: ERROR: Index (%d) >= DCRBs initialized (%d).\n",__FUNCTION__,id,ndcrb);
    return(-1);
  }

  return(dcrbID[id]);
}

int
dcrbId(unsigned int slot)
{
  int id;

  for(id=0; id<ndcrb; id++)
  {
    if(dcrbID[id]==slot)
	{
      return(id);
	}
  }

  printf("%s: ERROR: DCRB in slot %d does not exist or not initialized.\n",__FUNCTION__,slot);
  return(-1);
}

/*******************************************************************************
 *
 * dcrbInit - Initialize JLAB DCRB Library. 
 *
 *
 *   iFlag: 16 bit integer
 *       Low 6 bits - Specifies the default Signal distribution (clock,trigger) 
 *                    sources for the board (INTernal, FrontPanel, VXS, VME(Soft))
 *       bit    0:  defines Sync Reset source
 *                     0  VME (Software Sync-Reset)
 *                     1  VXS (P0)
 *       bit    1:  defines Trigger source
 *                     0  VME (Software Triggers)
 *                     1  VXS (P0) 
 *       bits   2:  defines Clock Source
 *                     0  Internal 250MHz Clock
 *                     1  VXS (P0)
 *
 *       Common Modes of Operation:
 *           Value = 0  CLK (Int)  TRIG (Soft)        SYNC (Soft)    (Debug/Test Mode)
 *                   1  CLK (Int)  TRIG (VXS-HighRes) SYNC (Soft)    (Single Board Modes)
 *                   2  CLK (Int)  TRIG (VXS)         SYNC (Soft)    
 *                   3  CLK (Int)  TRIG (VXS-HighRes) SYNC (VXS)
 *                   4  CLK (VXS)  TRIG (Soft)        SYNC (Soft)
 *                   5  CLK (VXS)  TRIG (VXS-HighRes) SYNC (VXS)     (VXS SD Mode)
 *                   7  CLK (VXS)  TRIG (VXS)         SYNC (VXS)     (VXS SD Mode)
 *
 *      bit 16:  Exit before board initialization
 *             0 Initialize DCRB (default behavior)
 *             1 Skip initialization (just setup register map pointers)
 *
 *      bit 17:  Use dcrbAddrList instead of addr and addr_inc
 *               for VME addresses.
 *             0 Initialize with addr and addr_inc
 *             1 Use dcrbAddrList 
 *      
 *
 * RETURNS: ndcrb, or 0 if no boards found, or ERROR
 */

int 
dcrbInit(unsigned int addr, unsigned int addr_inc, int ndc, int iFlag)
{
  int ii, res, errFlag = 0, vers = 0;
  int boardID = 0;
  int maxSlot = 1;
  int minSlot = 21;
  int syncSrc=0, trigSrc=0, clkSrc=0, srSrc=0;
  unsigned int rdata, laddr, laddr_inc, a32addr;
  volatile dcrb_regs *dcrb;
  unsigned short sdata;
  int noBoardInit=0;
  int useList=0;

  /* Check if we have already Initialized boards before */
  if((dcrbInited>0) && (dcrbID[0] != 0)) 
  {
    /* Hard Reset of all DCRB boards in the Crate
    for(ii=0; ii<ndcrb; ii++) 
	{
	  dcrbHardReset(dcrbID[ii]);
	}
    taskDelay(120);
    */
    ;
  }

  /* Check if we're initializing using a list */
  useList=(iFlag&(1<<17))>>17;

  /* Check for valid address */
  if(addr==0) 
    {
      printf("dcrbInit: ERROR: Must specify a Bus (VME-based A24) address for DCRB 0\n");
      return(ERROR);
    }
  else if(addr > 0x00ffffff) 
    { /* A24 Addressing */
      printf("dcrbInit: ERROR: A32 Addressing not allowed for DCRB configuration space\n");
      return(ERROR);
    }
  else
    { /* A24 Addressing */
      if( ((addr_inc==0)||(ndc==0)) && (useList==0) )
	ndc = 1; /* assume only one DCRB to initialize */

      /* get the DCRB address */
#ifdef VXWORKS
      res = sysBusToLocalAdrs(0x39,(char *)addr,(char **)&laddr);
#else
      res = vmeBusToLocalAdrs(0x39,(char *)addr,(char **)&laddr);
#endif
      if (res != 0) 
	{
#ifdef VXWORKS
	  printf("dcrbInit: ERROR in sysBusToLocalAdrs(0x39,0x%x,&laddr) \n",addr);
#else
	  printf("dcrbInit: ERROR in vmeBusToLocalAdrs(0x39,0x%x,&laddr) \n",addr);
#endif
	  return(ERROR);
	}
      dcrbA24Offset = laddr - addr;
    }

  /* Init Some Global variables */
  dcrbSource = iFlag&DCRB_SOURCE_MASK;
  dcrbInited = ndcrb = 0;
  bzero((char *)dcrbID,sizeof(dcrbID));

  printf("ndc=%d\n",ndc);fflush(stdout);
  for (ii=0; ii<ndc; ii++) 
  {
      if(useList==1)
	{
	  laddr_inc = dcrbAddrList[ii] + dcrbA24Offset;
	}
      else
	{
	  laddr_inc = laddr +ii*addr_inc;
	}
      dcrb = (dcrb_regs *)laddr_inc;
      /* Check if Board exists at that address */
#ifdef VXWORKS
      res = vxMemProbe((char *) &(dcrb->Cfg.BoardId),VX_READ,4,(char *)&rdata);
#else
      res = vmeMemProbe((char *) &(dcrb->Cfg.BoardId),4,(char *)&rdata);
#endif
      if(res < 0) 
	{
      printf("NO ADDRESSABLE BOARD !!!\n");fflush(stdout);
#ifdef VXWORKS
	  printf("dcrbInit: ERROR: No addressable board at addr=0x%x\n",(UINT32) dcrb);
#else
	  printf("dcrbInit: ERROR: No addressable board at VME (Local) addr=0x%x (0x%x)\n",
		 (UINT32) laddr_inc-dcrbA24Offset, (UINT32) dcrb);
#endif
	  /*errFlag = 1;*/
/* 	  break; */
	}
      else 
	{
	  /* Check that it is an DCRB board */
	  if((rdata&DCRB_BOARD_MASK) != DCRB_BOARD_ID) 
	  {
	    printf(" ERROR: For board at 0x%x, Invalid Board ID: 0x%x\n",
		     (UINT32) dcrb, rdata);
        continue;
	  }
	  /* Check if this is board has a valid slot number */
	  boardID =  ((vmeRead32(&(dcrb->Cfg.FirmwareRev)))&DCRB_SLOT_ID_MASK)>>DCRB_SLOT_ID_SHIFT;
	  if((boardID <= 0)||(boardID >21)) 
	  {
	    printf(" ERROR: Board Slot ID is not in range: %d\n",boardID);
	    continue;
	  }
	  DCRBp[boardID] = (dcrb_regs *)(laddr_inc);
	  dcrbRev[boardID] = vmeRead32(&(dcrb->Cfg.FirmwareRev)) & DCRB_VERSION_MASK;
/* 	} */
	  dcrbID[ndcrb] = boardID;
	  if(boardID >= maxSlot) maxSlot = boardID;
	  if(boardID <= minSlot) minSlot = boardID;
	  
    vers   = vmeRead32(&DCRBp[(dcrbID[ndcrb])]->Cfg.FirmwareRev) & DCRB_VERSION_MASK;
	  printf("Initialized DCRB %2d  Slot # %2d at address 0x%08x (0x%08x), versoin 0x%08X \n",
		 ndcrb,dcrbID[ndcrb],(UINT32) DCRBp[(dcrbID[ndcrb])],
		 (UINT32) DCRBp[(dcrbID[ndcrb])]-dcrbA24Offset, vers);
	  ndcrb++;
/* 	  printf("Initialized DCRB %2d  Slot # %2d at address 0x%08x \n", */
/* 		 ii,dcrbID[ii],(UINT32) DCRBp[(dcrbID[ii])]); */
	}
  }
  printf("Finished boards initialization, ndcrb=%d\n",ndcrb);fflush(stdout);


  /* Check if we are to exit when pointers are setup */
  noBoardInit=iFlag&(1<<16);

  /* Hard Reset of all DCRB boards in the Crate */
  for(ii=0; ii<ndcrb; ii++) 
  {
    if(!noBoardInit)
	{
      printf("HardReseting slot %d ..\n",dcrbID[ii]);fflush(stdout);
      /*dcrbHardReset(dcrbID[ii]);*/
      printf(".. done.\n");fflush(stdout);
	}
  }
  printf("Finished hard reset\n");fflush(stdout);
  taskDelay(120); 

  /* Initialize Interrupt variables */
  dcrbIntID = -1;
  dcrbIntRunning = FALSE;
  dcrbIntLevel = DCRB_VME_INT_LEVEL;
  dcrbIntVec = DCRB_VME_INT_VEC;
  dcrbIntRoutine = NULL;
  dcrbIntArg = 0;

  /* Calculate the A32 Offset for use in Block Transfers */
#ifdef VXWORKS
  res = sysBusToLocalAdrs(0x09,(char *)dcrbA32Base,(char **)&laddr);
  if (res != 0) 
    {
      printf("dcrbInit: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",dcrbA32Base);
      return(ERROR);
    } 
  else 
    {
      dcrbA32Offset = laddr - dcrbA32Base;
    }
#else
  res = vmeBusToLocalAdrs(0x09,(char *)dcrbA32Base,(char **)&laddr);
  if (res != 0) 
    {
      printf("dcrbInit: ERROR in vmeBusToLocalAdrs(0x09,0x%x,&laddr) \n",dcrbA32Base);fflush(stdout);
      return(ERROR);
    } 
  else 
    {
      dcrbA32Offset = laddr - dcrbA32Base;
    }
#endif

  printf("111\n");fflush(stdout);

    if(!noBoardInit)
    {
	  switch(iFlag&0x7)
	    {
		case 0:
			printf("dcrbInit: Enabling DCRB for Internal Clock ");
			printf("and Software Triggers (Soft Sync Reset)\n");
			clkSrc  = DCRB_REF_CLK_INTERNAL;
			trigSrc = 0;
			syncSrc = 0;
			break;
		case 1: case 2:
			printf("dcrbInit: Enabling DCRB for Internal Clock ");
			printf("and VXS Triggers (Soft Sync Reset)\n");
			clkSrc  = DCRB_REF_CLK_INTERNAL;
			trigSrc = DCRB_SD_MUX_TRIG1;
			syncSrc = 0;
			break;
		case 3:
			printf("dcrbInit: Enabling DCRB for Internal Clock ");
			printf("and VXS-HighRes Triggers (Soft Sync Reset)\n");
			clkSrc  = DCRB_REF_CLK_INTERNAL;
			trigSrc = DCRB_SD_MUX_TRIG1;
			syncSrc = DCRB_SD_MUX_SYNC;
			break;
		case 4:
			printf("dcrbInit: Enabling DCRB for VXS Clock ");
			printf("and Software Triggers (Soft Sync Reset)\n");
			clkSrc  = DCRB_REF_CLK_P0;
			trigSrc = 0;
			syncSrc = 0;
			break;
		case 5:
			printf("dcrbInit: Enabling DCRB for VXS Clock ");
			printf("and VXS Triggers (Soft Sync Reset)\n");
			clkSrc  = DCRB_REF_CLK_P0;
			trigSrc = DCRB_SD_MUX_TRIG1;
			syncSrc = 0;
			break;
		case 6: case 7:
			printf("dcrbInit: Enabling DCRB for VXS Clock ");
			printf("and VXS Triggers (VXS Sync Reset)\n");
			clkSrc  = DCRB_REF_CLK_P0;
			trigSrc = DCRB_SD_MUX_TRIG1;
			syncSrc = DCRB_SD_MUX_SYNC;
			break;
	    }
    }

  /* Enable Clock source - Internal Clk enabled by default */ 
  for(ii=0; ii<ndcrb; ii++) 
  {
    vmeWrite32(&(DCRBp[dcrbID[ii]]->Clk.Ctrl),clkSrc | DCRB_REF_CLK_RESET);
    vmeWrite32(&(DCRBp[dcrbID[ii]]->Clk.Ctrl),clkSrc);
  }
  taskDelay(1);

  /* Hard Reset FPGAs and FIFOs */
  for(ii=0; ii<ndcrb; ii++)
  {
    vmeWrite32(&(DCRBp[dcrbID[ii]]->Cfg.Reset),DCRB_CFG_RESET);
	 taskDelay(1);
    vmeWrite32(&(DCRBp[dcrbID[ii]]->Cfg.Reset),0);
  }
  taskDelay(1);

  /* Default 100mV threshold */
  dcrbSetDAC(dcrbID[ii], 100);
  /* Default no calibration signals enabled */
  dcrbSetDAC_Pulser(dcrbID[ii], 0x00, 0.0, 2000, 1000, 1010, 2);

  /* Write configuration registers with default/defined Sources */
  for(ii=0; ii<ndcrb; ii++) 
  {
    
      /* Program an A32 access address for this DCRB's FIFO */
      a32addr = dcrbA32Base + ii*DCRB_MAX_A32_MEM;
#ifdef VXWORKS
      res = sysBusToLocalAdrs(0x09,(char *)a32addr,(char **)&laddr);
      if (res != 0) 
	{
	  printf("dcrbInit: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",a32addr);
	  return(ERROR);
	}
#else
      res = vmeBusToLocalAdrs(0x09,(char *)a32addr,(char **)&laddr);
      if (res != 0) 
	{
	  printf("dcrbInit: ERROR in vmeBusToLocalAdrs(0x09,0x%x,&laddr) \n",a32addr);
	  return(ERROR);
	}
#endif
      DCRBpd[dcrbID[ii]] = (unsigned int *)(laddr);  /* Set a pointer to the FIFO */
      if(!noBoardInit)
	{
	  vmeWrite32(&(DCRBp[dcrbID[ii]]->EB.AD32),(a32addr>>16) + DCRB_A32_ENABLE);  /* Write the register and enable */
	
	  /* Set Default Block Level to 1 */
	  vmeWrite32(&(DCRBp[dcrbID[ii]]->EB.BlockCfg),1);

      /* berr for every board; TODO: multiblock needs it on last board only !!!*/
      vmeWrite32(&(DCRBp[dcrbID[ii]]->EB.ReadoutCfg), DCRB_ENABLE_BERR);

	}
      dcrbBlockLevel=1;

    /* Setup Trigger and Sync Reset sources */
    if(!noBoardInit)
    {
      vmeWrite32(&(DCRBp[dcrbID[ii]]->Sd.SrcSel[DCRB_OUTPUT_TRIG]),trigSrc);
      vmeWrite32(&(DCRBp[dcrbID[ii]]->Sd.SrcSel[DCRB_OUTPUT_SYNC]),syncSrc);
    }
  }

  /* If there are more than 1 DCRB in the crate then setup the Muliblock Address
     window. This must be the same on each board in the crate */
  if(ndcrb > 1) 
  {
      a32addr = dcrbA32Base + (ndcrb+1)*DCRB_MAX_A32_MEM; /* set MB base above individual board base */
#ifdef VXWORKS
      res = sysBusToLocalAdrs(0x09,(char *)a32addr,(char **)&laddr);
      if (res != 0) 
	{
	  printf("dcrbInit: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",a32addr);
	  return(ERROR);
	}
#else
      res = vmeBusToLocalAdrs(0x09,(char *)a32addr,(char **)&laddr);
      if (res != 0) 
	{
	  printf("dcrbInit: ERROR in vmeBusToLocalAdrs(0x09,0x%x,&laddr) \n",a32addr);
	  return(ERROR);
	}
#endif
      DCRBpmb = (unsigned int *)(laddr);  /* Set a pointer to the FIFO */


goto skipmultiblock;
    if(!noBoardInit)
	{
	  for (ii=0;ii<ndcrb;ii++) 
	    {
	      /* Write the register and enable */
	      vmeWrite32(&(DCRBp[dcrbID[ii]]->EB.Adr32M),
			((a32addr+DCRB_MAX_A32MB_SIZE)>>8) + (a32addr>>23) + DCRB_AMB_ENABLE);
	    }
	}    
      /* Set First Board and Last Board */
      dcrbMaxSlot = maxSlot;
      dcrbMinSlot = minSlot;
    if(!noBoardInit)
	{
	  vmeWrite32(&(DCRBp[minSlot]->EB.Adr32M),
		    vmeRead32(&(DCRBp[minSlot]->EB.Adr32M)) | DCRB_FIRST_BOARD);
	  vmeWrite32(&(DCRBp[maxSlot]->EB.Adr32M),
		    vmeRead32(&(DCRBp[maxSlot]->EB.Adr32M)) | DCRB_LAST_BOARD);
	}
skipmultiblock:
	;    
  }

  dcrbInited = ndcrb;
  if(errFlag > 0) 
  {
    printf("dcrbInit: ERROR: Unable to initialize all DCRB Modules\n");
    return(ERROR);
  } 
  else 
  {
    if(ndcrb > 0) printf("dcrbInit: %d DCRB(s) successfully initialized\n",ndcrb );
    return(ndcrb);
  }
}






int
dcrbHardReset(int id)
{
	int i;
	unsigned short reloadSequence[] = {
		0xFFFF, 0xAA99, 0x5566, 0x3261,
		0x0000, 0x3281, 0x0B00, 0x32A1,
		0x0000, 0x32C1, 0x0B00, 0x30A1,
		0x000E, 0x2000
	};

	vmeWrite32(&(DCRBp[id]->Cfg.ICapCtrl), DCRB_ICAP_CE);
	vmeWrite32(&(DCRBp[id]->Cfg.ICapCtrl), DCRB_ICAP_CE | DCRB_ICAP_CLK);
	for(i = 0; i < sizeof(reloadSequence)/sizeof(reloadSequence[0]); i++)
	{
		vmeWrite32(&(DCRBp[id]->Cfg.ICapDataWr), reloadSequence[i]);
		vmeWrite32(&(DCRBp[id]->Cfg.ICapCtrl), DCRB_ICAP_CLK);
	}
	for(i = 0; i < 10; i++)
		vmeWrite32(&(DCRBp[id]->Cfg.ICapCtrl), DCRB_ICAP_CE | DCRB_ICAP_CLK);
	
	return(OK);
}

/*******************************************************************************
 *
 * dcrbSetClockSource - Set the clock source
 *
 *   This routine should be used in the case that the source clock
 *   is NOT set in dcrbInit (and defaults to Internal).  Such is the case
 *   when clocks are synchronized in a many crate system.  The clock source
 *   of the DCRB should ONLY be set AFTER those clocks have been set and
 *   synchronized.
 *
 *   clkSrc: 1 bit integer
 *       bit 0:  defines Clock Source
 *           0  Internal 250MHz Clock
 *           1  VXS (P0)
 *
 */

int
dcrbSetClockSource(int id, int clkSrc)
{
  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
  {
    printf("dcrbStatus: ERROR : DCRB in slot %d is not initialized \n",id);
    return;
  }

  if(clkSrc>0x1)
  {
    printf("%s: ERROR: Invalid Clock Source specified (0x%x)\n", __FUNCTION__,clkSrc);
    return ERROR;
  }

  if(clkSrc) clkSrc = DCRB_REF_CLK_P0;

  /* Enable Clock source - Internal Clk enabled by default */ 
  vmeWrite32(&(DCRBp[id]->Clk.Ctrl),clkSrc | DCRB_REF_CLK_RESET);
  vmeWrite32(&(DCRBp[id]->Clk.Ctrl),clkSrc);
  taskDelay(20);

  switch(clkSrc)
    {
    case DCRB_REF_CLK_INTERNAL:
      printf("%s: DCRB id %d clock source set to INTERNAL\n",
	     __FUNCTION__,id);
      break;

    case DCRB_REF_CLK_P0:
      printf("%s: DCRB id %d clock source set to VXS (P0)\n",
	     __FUNCTION__,id);
      break;
    }

  return OK;
}

void
dcrbStatus(int id, int sflag)
{ 
  int ii;
  unsigned int vers, bid;
  unsigned int adr32m, adr32;
  unsigned int lookBack, windowWidth, blockConfig;
  unsigned int testPulseConfigDCE[3];
  unsigned int testPulseConfigFCE[3];
  unsigned int clkConfig, dacConfig, trigBusyThreshold, trigSrc, syncSrc;
  unsigned int intr, geo;
  unsigned int fifoWordCnt, fifoEventCnt, fifoBlockCnt;
  unsigned int readoutCfg;
  unsigned int chDisable[6], tdcConfig[6];

  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
    {
      printf("dcrbStatus: ERROR : DCRB in slot %d is not initialized \n",id);
      return;
    }

  DCRBLOCK;
  vers   = vmeRead32(&DCRBp[id]->Cfg.FirmwareRev) & DCRB_VERSION_MASK;
  bid    = vmeRead32(&DCRBp[id]->Cfg.BoardId);
  adr32m = vmeRead32(&DCRBp[id]->EB.Adr32M);
  lookBack = vmeRead32(&DCRBp[id]->EB.Lookback);
  windowWidth = vmeRead32(&DCRBp[id]->EB.WindowWidth);
  blockConfig = vmeRead32(&DCRBp[id]->EB.BlockCfg);
  clkConfig = vmeRead32(&DCRBp[id]->Clk.Ctrl);
  testPulseConfigDCE[0] = vmeRead32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_DCE0]);
  testPulseConfigDCE[1] = vmeRead32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_DCE1]);
  testPulseConfigDCE[2] = vmeRead32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_DCE2]);
  testPulseConfigFCE[0] = vmeRead32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_FCE0]);
  testPulseConfigFCE[1] = vmeRead32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_FCE1]);
  testPulseConfigFCE[2] = vmeRead32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_FCE2]);
  dacConfig = vmeRead32(&DCRBp[id]->Cfg.DacThreshold);
  trigSrc = vmeRead32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_TRIG]);
  syncSrc = vmeRead32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_SYNC]);
  adr32 = vmeRead32(&DCRBp[id]->EB.AD32);
  intr = vmeRead32(&DCRBp[id]->EB.Interrupt);
  geo = (vmeRead32(&DCRBp[id]->Cfg.FirmwareRev) & DCRB_SLOT_ID_MASK) >> DCRB_SLOT_ID_SHIFT;
  fifoWordCnt = vmeRead32(&DCRBp[id]->EB.FifoWordCnt);
  fifoEventCnt = vmeRead32(&DCRBp[id]->EB.FifoEventCnt);
  fifoBlockCnt = vmeRead32(&DCRBp[id]->EB.FifoBlockCnt);
  readoutCfg = vmeRead32(&DCRBp[id]->EB.ReadoutCfg);
  tdcConfig[0] = vmeRead32(&DCRBp[id]->Tdc[0].DeadCycles);
  tdcConfig[1] = vmeRead32(&DCRBp[id]->Tdc[1].DeadCycles);
  tdcConfig[2] = vmeRead32(&DCRBp[id]->Tdc[2].DeadCycles);
  tdcConfig[3] = vmeRead32(&DCRBp[id]->Tdc[3].DeadCycles);
  tdcConfig[4] = vmeRead32(&DCRBp[id]->Tdc[4].DeadCycles);
  tdcConfig[5] = vmeRead32(&DCRBp[id]->Tdc[5].DeadCycles);
  chDisable[0] = ~(vmeRead32(&DCRBp[id]->Tdc[0].EnableN));
  chDisable[1] = ~(vmeRead32(&DCRBp[id]->Tdc[1].EnableN));
  chDisable[2] = ~(vmeRead32(&DCRBp[id]->Tdc[2].EnableN));
  chDisable[3] = ~(vmeRead32(&DCRBp[id]->Tdc[3].EnableN));
  chDisable[4] = ~(vmeRead32(&DCRBp[id]->Tdc[4].EnableN));
  chDisable[5] = ~(vmeRead32(&DCRBp[id]->Tdc[5].EnableN));
  DCRBUNLOCK;

#ifdef VXWORKS
  printf("\nSTATUS for DCRB in slot %d at base address 0x%x \n",
	 id, (UINT32) DCRBp[id]);
#else
  printf("\nSTATUS for DCRB in slot %d at VME (Local) base address 0x%x (0x%x)\n",
	 id, (UINT32) DCRBp[id] - dcrbA24Offset, (UINT32) DCRBp[id]);
#endif
  printf("---------------------------------------------------------------------- \n");

  printf(" Board Firmware Rev/ID = 0x%04x / 0x%08x\n", (vers)&0xffff, bid);
  if(adr32m&DCRB_AMB_ENABLE) 
    {
      printf(" Alternate VME Addressing: Multiblock Enabled\n");
      if(adr32&DCRB_A32_ENABLE)
	printf("   A32 Enabled at VME (Local) base 0x%08x (0x%08x)\n",(adr32&0xFF80)<<16,(UINT32) DCRBpd[id]);
      else
	printf("   A32 Disabled\n");
    
      printf("   Multiblock VME Address Range 0x%08x - 0x%08x\n",(adr32m&0x1FF)<<23,(adr32m&0x1FF0000)<<7);
    }
  else
    {
      printf(" Alternate VME Addressing: Multiblock Disabled\n");
      if(adr32&DCRB_A32_ENABLE)
	printf("   A32 Enabled at VME (Local) base 0x%08x (0x%08x)\n",(adr32&0xFF80)<<16,(UINT32) DCRBpd[id]);
      else
	printf("   A32 Disabled\n");
    }

  printf("\n Signal Sources: \n");
  if((clkConfig&DCRB_REF_CLK_MASK)==DCRB_REF_CLK_INTERNAL)
    printf("   Ref Clock : Internal\n");
  else if((clkConfig&DCRB_REF_CLK_MASK)==DCRB_REF_CLK_P0)
    printf("   Ref Clock : VXS\n");
  else
    printf("   Ref Clock : %d (Undefined)\n",(clkConfig&DCRB_REF_CLK_MASK));

  switch(trigSrc)
  {
    case DCRB_SD_MUX_0:
    case DCRB_SD_MUX_1:			printf("   Trig Src  : VME (Software)\n"); break;
    case DCRB_SD_MUX_TRIG1:	printf("   Trig Src  : VXS\n"); break;
    default:                  printf("   Trig Src  : unknown\n"); break;
  }  

  switch(syncSrc) 
  {
    case DCRB_SD_MUX_0:
    case DCRB_SD_MUX_1:			printf("   Sync Src  : VME (Software)\n"); break;
    case DCRB_SD_MUX_SYNC:		printf("   Sync Src  : VXS\n"); break;
    default:                  printf("   Sync Src  : unknown\n"); break;
  }  

  printf("\n Configuration: \n");
  
  if(readoutCfg & DCRB_ENABLE_BERR)
    printf("   Bus Error ENABLED\n");
  else
    printf("   Bus Error DISABLED\n");


  if(adr32m & DCRB_ENABLE_MULTIBLOCK) 
  {
    if(adr32m & DCRB_FIRST_BOARD)
      printf("   MultiBlock transfer ENABLED (First Board - token via VXS)\n");
    else if(adr32m & DCRB_LAST_BOARD)
      printf("   MultiBlock transfer ENABLED (Last Board  - token via VXS)\n");
    else
      printf("   MultiBlock transfer ENABLED (Token via VXS)\n");
  } 
  else 
    printf("   MultiBlock transfer DISABLED\n");

  printf("\n TDC Processing Configuration:\n");
  printf("   Channel Disable Mask[15- 0] = 0x%04x, Deadtime = %d\n", chDisable[0], tdcConfig[0]*8);
  printf("   Channel Disable Mask[31-16] = 0x%04x, Deadtime = %d\n", chDisable[1], tdcConfig[1]*8);
  printf("   Channel Disable Mask[47-32] = 0x%04x, Deadtime = %d\n", chDisable[2], tdcConfig[2]*8);
  printf("   Channel Disable Mask[63-48] = 0x%04x, Deadtime = %d\n", chDisable[3], tdcConfig[3]*8);
  printf("   Channel Disable Mask[79-64] = 0x%04x, Deadtime = %d\n", chDisable[4], tdcConfig[4]*8);
  printf("   Channel Disable Mask[95-80] = 0x%04x, Deadtime = %d\n", chDisable[5], tdcConfig[5]*8);
  printf("   Lookback (PL)    = %d ns   Time Window (PTW) = %d ns\n", lookBack, windowWidth);

  printf("   Blocks in FIFO  = %u  (Block level = %d)\n", fifoBlockCnt, blockConfig & 0x7ff);
  printf("   Events in FIFO  = %u\n", fifoEventCnt);
  printf("   Words in FIFO   = %u\n", fifoWordCnt);
  
  printf("\n DAC Threshold: %dmV", dcrbGetDAC(id));
  printf("\n");
}

void 
dcrbGStatus(int sflag)
{
  int ii;

  for (ii=0;ii<ndcrb;ii++) 
    dcrbStatus(dcrbID[ii],sflag);
}



/***********************
 *
 *  dcrbSetProcMode - Setup DCRB processing modes.
 *
 */
int
dcrbSetProcMode(int id, unsigned int lookBack, unsigned int windowWidth, unsigned int deadTime)
{  
  int err=0;
  unsigned int ptw_last_adr, ptw_max_buf;

  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
  {
    logMsg("dcrbSetProcMode: ERROR : DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
    return(ERROR);
  }

  /*Defaults */
  if((lookBack==0)||(lookBack>DCRB_MAX_LOOKBACK))  lookBack  = DCRB_DEFAULT_LOOKBACK;
  if((windowWidth==0)||(windowWidth>DCRB_MAX_WINDOW)) windowWidth = DCRB_DEFAULT_WINDOW;
  if((deadTime==0)||(deadTime>DCRB_MAX_DEADTIME)) deadTime = DCRB_MAX_DEADTIME;

  /* Consistancy check */
  if(windowWidth > lookBack) 
  {
    err++;
    printf("dcrbSetProcMode: ERROR: Window must be <= Latency\n"); 
  }

  /* convert inputs from ns to ticks */
  lookBack = lookBack;
  windowWidth = windowWidth;
  deadTime = deadTime / DCRB_NS_PER_CLOCK;

  DCRBLOCK;
  vmeWrite32(&DCRBp[id]->EB.Lookback, lookBack);
  vmeWrite32(&DCRBp[id]->EB.WindowWidth, windowWidth);
  vmeWrite32(&DCRBp[id]->Tdc[0].DeadCycles, deadTime);
  vmeWrite32(&DCRBp[id]->Tdc[1].DeadCycles, deadTime);
  vmeWrite32(&DCRBp[id]->Tdc[2].DeadCycles, deadTime);
  vmeWrite32(&DCRBp[id]->Tdc[3].DeadCycles, deadTime);
  vmeWrite32(&DCRBp[id]->Tdc[4].DeadCycles, deadTime);
  vmeWrite32(&DCRBp[id]->Tdc[5].DeadCycles, deadTime);
  DCRBUNLOCK;

  return(OK);
}

int
dcrbGetWindowOffset(int id)
{  
  int lookBack;

  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
  {
    logMsg("dcrbGetWindowOffset: ERROR : DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
    return(ERROR);
  }

  DCRBLOCK;
  lookBack = vmeRead32(&DCRBp[id]->EB.Lookback);
  DCRBUNLOCK;

  return(lookBack);
}

int
dcrbGetWindowWidth(int id)
{  
  int windowWidth;

  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
  {
    logMsg(": ERROR dcrbGetWindowWidth: DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
    return(ERROR);
  }

  DCRBLOCK;
  windowWidth = vmeRead32(&DCRBp[id]->EB.WindowWidth);
  DCRBUNLOCK;

  return(windowWidth);
}

int
dcrbGetDeadtime(int id)
{  
  int deadTime;

  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
  {
    logMsg("dcrbGetDeadtime: ERROR : DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
    return(ERROR);
  }

  DCRBLOCK;
  deadTime = vmeRead32(&DCRBp[id]->Tdc[0].DeadCycles);
  DCRBUNLOCK;

  /* convert inputs from ticks to ns */
  deadTime = deadTime * DCRB_NS_PER_CLOCK;

  return(deadTime);
}

void
dcrbGSetProcMode(unsigned int lookBack, unsigned int windowWidth, unsigned int deadTime)
{
  int ii, res;

  for (ii=0;ii<ndcrb;ii++) {
    res = dcrbSetProcMode(dcrbID[ii],lookBack,windowWidth, deadTime);
    if(res<0) printf("ERROR: slot %d, in dcrbSetProcMode()\n",dcrbID[ii]);
  }
}

/**************************************************************************************
 *
 *  dcrbReadBlock - General Data readout routine
 *
 *    id    - Slot number of module to read
 *    data  - local memory address to place data
 *    nwrds - Max number of words to transfer
 *    rflag - Readout Flag
 *              0 - programmed I/O from the specified board
 *              1 - DMA transfer using Universe/Tempe DMA Engine 
 *                    (DMA VME transfer Mode must be setup prior)
 *              2 - Multiblock DMA transfer (Multiblock must be enabled
 *                     and daisychain in place or SD being used)
 */
int
dcrbReadBlock(int id, volatile unsigned int *data, int nwrds, int rflag)
{
  int ii, blknum, evnum1;
  int stat, retVal, xferCount, rmode, async;
  int dCnt, berr=0;
  int dummy=0;
  volatile unsigned int *laddr;
  unsigned int bhead, ehead, val;
  unsigned int vmeAdr, csr;

  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
    {
      logMsg("dcrbReadBlock: ERROR : DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }

  if(data==NULL) 
    {
      logMsg("dcrbReadBlock: ERROR: Invalid Destination address\n",0,0,0,0,0,0);
      return(ERROR);
    }

  if(nwrds <= 0) nwrds= (DCRB_MAX_TDC_CHANNELS*DCRB_MAX_DATA_PER_CHANNEL) + 8;
  rmode = rflag&0x0f;
  async = rflag&0x80;
  
  if(rmode >= 1) 
    { /* Block Transfers */
    
      /*Assume that the DMA programming is already setup. */
      /* Don't Bother checking if there is valid data - that should be done prior
	 to calling the read routine */

      /* Check for 8 byte boundary for address - insert dummy word (Slot 0 DCRB Dummy DATA)*/
      if((unsigned long) (data)&0x7) 
	{
#ifdef VXWORKS
	  *data = DCRB_DUMMY_DATA;
#else
	  *data = LSWAP(DCRB_DUMMY_DATA);
#endif
	  dummy = 1;
	  laddr = (data + 1);
	} 
      else 
	{
	  dummy = 0;
	  laddr = data;
	}

      DCRBLOCK;
      if(rmode == 2) 
	{ /* Multiblock Mode */
	  if((vmeRead32(&(DCRBp[id]->EB.Adr32M))&DCRB_FIRST_BOARD)==0) 
	    {
	      logMsg("dcrbReadBlock: ERROR: DCRB in slot %d is not First Board\n",id,0,0,0,0,0);
	      DCRBUNLOCK;
	      return(ERROR);
	    }
	  vmeAdr = (unsigned int)(DCRBpmb) - dcrbA32Offset;
	}
      else
	{
	  vmeAdr = (unsigned int)(DCRBpd[id]) - dcrbA32Offset;
	}
	  /*
#ifdef VXWORKS
      retVal = sysVmeDmaSend((UINT32)laddr, vmeAdr, (nwrds<<2), 0);
#else
      retVal = vmeDmaSend((UINT32)laddr, vmeAdr, (nwrds<<2));
#endif
	  */
    retVal = usrVme2MemDmaStart(vmeAdr, (UINT32)laddr, (nwrds<<2));
      if(retVal |= 0) 
	{
	  logMsg("dcrbReadBlock: ERROR in DMA transfer Initialization 0x%x\n",retVal,0,0,0,0,0);
	  DCRBUNLOCK;
	  return(retVal);
	}

      if(async) 
	{ /* Asynchonous mode - return immediately - don't wait for done!! */
	  DCRBUNLOCK;
	  return(OK);
	}
      else
	{
	  /* Wait until Done or Error */
	  /*
#ifdef VXWORKS
	  retVal = sysVmeDmaDone(10000,1);
#else
	  retVal = vmeDmaDone();
#endif
	  */
      retVal = usrVme2MemDmaDone();
	}

      if(retVal > 0) 
	{
	/*
	  / * Check to see that Bus error was generated by FADC * /
	  if(rmode == 2) 
	    {
	      csr = vmeRead32(&(FAp[fadcMaxSlot]->csr));  / * from Last FADC * /
	      stat = (csr)&FA_CSR_BERR_STATUS;  / * from Last FADC * /
	    }
	  else
	    {
	      csr = vmeRead32(&(FAp[id]->csr));  / * from Last FADC * /
	      stat = (csr)&FA_CSR_BERR_STATUS;  / * from Last FADC * /
	    }
		*/
	  if((retVal>0))/* && (stat))*/ 
	    {
#ifdef VXWORKS
	      xferCount = (/*nwrds -*/ (retVal>>2) + dummy);  /* Number of Longwords transfered */
#else
	      xferCount = ((retVal>>2) + dummy);  /* Number of Longwords transfered */
	      /* 	xferCount = (retVal + dummy);  /\* Number of Longwords transfered *\/ */
#endif
	      DCRBUNLOCK;
	      return(xferCount); /* Return number of data words transfered */
	    }
	  else
	    {
#ifdef VXWORKS
	      xferCount = (/*nwrds -*/ (retVal>>2) + dummy);  /* Number of Longwords transfered */
#else
	      xferCount = ((retVal>>2) + dummy);  /* Number of Longwords transfered */
#endif
	      logMsg("dcrbReadBlock: DMA transfer terminated by unknown BUS Error (csr=0x%x xferCount=%d)\n",csr,xferCount,0,0,0,0);
	      DCRBUNLOCK;
	      return(xferCount);
	      /* 	return(ERROR); */
	    }
	} 
      else if (retVal == 0)
	{ /* Block Error finished without Bus Error */
#ifdef VXWORKS
	  logMsg("dcrbReadBlock: WARN: DMA transfer terminated by word count 0x%x\n",nwrds,0,0,0,0,0);
#else
	  logMsg("dcrbReadBlock: WARN: DMA transfer returned zero word count 0x%x\n",nwrds,0,0,0,0,0);
#endif
	  DCRBUNLOCK;
	  return(/*nwrds*/retVal>>2);
	} 
      else 
	{  /* Error in DMA */
#ifdef VXWORKS
	  logMsg("dcrbReadBlock: ERROR: sysVmeDmaDone returned an Error\n",0,0,0,0,0,0);
#else
	  logMsg("dcrbReadBlock: ERROR: vmeDmaDone returned an Error\n",0,0,0,0,0,0);
#endif
	  DCRBUNLOCK;
	  return(retVal>>2);
	}

    } 
  else 
    {  /*Programmed IO */

      /* Check if Bus Errors are enabled. If so then disable for Prog I/O reading */
      DCRBLOCK;
      berr = vmeRead32(&(DCRBp[id]->EB.ReadoutCfg));
      if(berr & DCRB_ENABLE_BERR)
	    vmeWrite32(&(DCRBp[id]->EB.ReadoutCfg), berr & ~DCRB_ENABLE_BERR);

      dCnt = 0;
      /* Read Block Header - should be first word */
      bhead = (unsigned int) *DCRBpd[id]; 
#ifndef VXWORKS
      bhead = LSWAP(bhead);
#endif
      if((bhead&DCRB_DATA_TYPE_DEFINE)&&((bhead&DCRB_DATA_TYPE_MASK) == DCRB_DATA_BLOCK_HEADER)) {
	blknum = bhead&DCRB_DATA_BLKNUM_MASK;
	ehead = (unsigned int) *DCRBpd[id];
#ifndef VXWORKS
	ehead = LSWAP(ehead);
#endif
	evnum1 = ehead&DCRB_DATA_TRIGNUM_MASK;
#ifdef VXWORKS
	data[dCnt] = bhead;
#else
	data[dCnt] = LSWAP(bhead); /* Swap back to little-endian */
#endif
	dCnt++;
#ifdef VXWORKS
	data[dCnt] = ehead;
#else
	data[dCnt] = LSWAP(ehead); /* Swap back to little-endian */
#endif
	dCnt++;
      }
      else
	{
	  /* We got bad data - Check if there is any data at all */
	  if( vmeRead32(&(DCRBp[id]->EB.FifoWordCnt)) == 0) 
	    {
	      logMsg("dcrbReadBlock: FIFO Empty (0x%08x)\n",bhead,0,0,0,0,0);
	      DCRBUNLOCK;
	      return(0);
	    } 
	  else 
	    {
	      logMsg("dcrbReadBlock: ERROR: Invalid Header Word 0x%08x\n",bhead,0,0,0,0,0);
	      DCRBUNLOCK;
	      return(ERROR);
	    }
	}

      ii=0;
      while(ii<nwrds) 
	{
	  val = (unsigned int) *DCRBpd[id];
	  data[ii+2] = val;
#ifndef VXWORKS
	  val = LSWAP(val);
#endif
	  if( (val&DCRB_DATA_TYPE_DEFINE) 
	      && ((val&DCRB_DATA_TYPE_MASK) == DCRB_DATA_BLOCK_TRAILER) )
	    break;
	  ii++;
	}
      ii++;
      dCnt += ii;


      if(berr & DCRB_ENABLE_BERR)
        vmeWrite32(&(DCRBp[id]->EB.ReadoutCfg), berr | DCRB_ENABLE_BERR);

      DCRBUNLOCK;
      return(dCnt);
    }

  DCRBUNLOCK;
  return(OK);
}

int
dcrbReadBlockStatus(int id, volatile unsigned int *data, int nwrds, int rflag)
{

  int stat, retVal, xferCount, rmode, async;
  int dummy=0;
  unsigned int csr=0;
  
  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
    {
      logMsg("dcrbReadBlockStatus: ERROR : DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }

  if(nwrds <= 0) nwrds= (DCRB_MAX_TDC_CHANNELS*DCRB_MAX_DATA_PER_CHANNEL) + 8;
  rmode = rflag&0x0f;
  async = rflag&0x80;

  /* Check for 8 byte boundary for address - insert dummy word (Slot 0 DCRB Dummy DATA)*/
  if((unsigned long) (data)&0x7) 
    {
      dummy = 1;
    } 
  else 
    {
      dummy = 0;
    }
  /*
#ifdef VXWORKS
  retVal = sysVmeDmaDone(10000,1);
#else
  retVal = vmeDmaDone();
#endif
  */
      retVal = usrVme2MemDmaDone();
  DCRBLOCK;
  if(retVal > 0) 
    {
	/*
      / * Check to see that Bus error was generated by FADC * /
      if(rmode == 2) 
	{
	  csr = vmeRead32(&(FAp[fadcMaxSlot]->csr));  / * from Last FADC * /
	  stat = (csr)&FA_CSR_BERR_STATUS;  / * from Last FADC * /
	}
      else
	{
	  stat = vmeRead32(&(FAp[id]->csr))&FA_CSR_BERR_STATUS;  / * from FADC id * /
	}
	*/
      if((retVal>0) && (stat)) 
	{
	  xferCount = (/*nwrds -*/ (retVal>>2) + dummy);  /* Number of Longwords transfered */
	  DCRBUNLOCK;
	  return(xferCount); /* Return number of data words transfered */
	}
      else
	{
	  xferCount = (/*nwrds -*/ (retVal>>2) + dummy);  /* Number of Longwords transfered */
	  logMsg("dcrbReadBlockStatus: DMA transfer terminated by unknown BUS Error (csr=0x%x nwrds=%d)\n",csr,xferCount,0,0,0,0);
	  DCRBUNLOCK;
	  return(ERROR);
	}
    } 
  else if (retVal == 0)
    { /* Block Error finished without Bus Error */
      logMsg("dcrbReadBlockStatus: WARN: DMA transfer terminated by word count 0x%x\n",nwrds,0,0,0,0,0);
      DCRBUNLOCK;
      return(/*nwrds*/retVal>>2);
    } 
  else 
    {  /* Error in DMA */
      logMsg("dcrbReadBlockStatus: ERROR: sysVmeDmaDone returned an Error\n",0,0,0,0,0,0);
      DCRBUNLOCK;
      return(retVal);
    }
  
}

int
dcrbPrintBlock(int id, int rflag)
{

  int ii, blknum, evnum1;
  int nwrds=32768, dCnt, berr=0;
  unsigned int data, bhead, ehead;

  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
    {
      printf("dcrbPrintEvent: ERROR : DCRB in slot %d is not initialized \n",id);
      return(ERROR);
    }

  /* Check if data available */
  DCRBLOCK;
  if(vmeRead32(&(DCRBp[id]->EB.FifoWordCnt))==0) 
    {
      printf("dcrbPrintEvent: ERROR: FIFO Empty\n");
      DCRBUNLOCK;
      return(0);
    }

  /* Check if Bus Errors are enabled. If so then disable for reading */
  berr = vmeRead32(&(DCRBp[id]->EB.ReadoutCfg));
  if(berr & DCRB_ENABLE_BERR)
    vmeWrite32(&(DCRBp[id]->EB.ReadoutCfg), berr & ~DCRB_ENABLE_BERR);

  dCnt = 0;
  /* Read Block Header - should be first word */
  bhead = (unsigned int) *DCRBpd[id];
#ifndef VXWORKS
  bhead = LSWAP(bhead);
#endif
  if( (bhead&DCRB_DATA_TYPE_DEFINE)&&((bhead&DCRB_DATA_TYPE_MASK) == DCRB_DATA_BLOCK_HEADER)) 
    {
      blknum = bhead&DCRB_DATA_BLKNUM_MASK;
      ehead = (unsigned int) *DCRBpd[id];
#ifndef VXWORKS
      ehead = LSWAP(ehead);
#endif
      evnum1 = ehead&DCRB_DATA_TRIGNUM_MASK;
      printf("%4d: ",dCnt+1); 
      dcrbDataDecode(bhead);
      dCnt++;
      printf("%4d: ",dCnt+1); 
      dcrbDataDecode(ehead);
      dCnt++;
    }
  else
    {
      /* We got bad data - Check if there is any data at all */
      if(vmeRead32(&(DCRBp[id]->EB.FifoWordCnt))==0) 
	{
	  logMsg("dcrbPrintBlock: FIFO Empty (0x%08x)\n",bhead,0,0,0,0,0);
	  DCRBUNLOCK;
	  return(0);
	} 
      else 
	{
	  logMsg("dcrbPrintBlock: ERROR: Invalid Header Word 0x%08x\n",bhead,0,0,0,0,0);
	  DCRBUNLOCK;
	  return(ERROR);
	}
    }
  
  ii=0;
  while(ii<nwrds) 
    {
      data = (unsigned int) *DCRBpd[id];
#ifndef VXWORKS
      data = LSWAP(data);
#endif
      printf("%4d: ",dCnt+1+ii); 
      dcrbDataDecode(data);
      if((data&DCRB_DATA_TYPE_DEFINE)&&((data&DCRB_DATA_TYPE_MASK) == DCRB_DATA_BLOCK_TRAILER))
	break;
      if((data&DCRB_DATA_TYPE_DEFINE)&&((data&DCRB_DATA_TYPE_MASK) == DCRB_DATA_INVALID))
	break;
      ii++;
    }
  ii++;
  dCnt += ii;


  if(berr & DCRB_ENABLE_BERR)
    vmeWrite32(&(DCRBp[id]->EB.ReadoutCfg), berr | DCRB_ENABLE_BERR);
  
  DCRBUNLOCK;
  return(dCnt);
  
}




/*****************************************************************************/


/* Reset Event Builder (FIFOs etc) */
void
dcrbClear(int id)
{
  if(id==0) id = dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
    {
      logMsg("dcrbClear: ERROR : DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
      return;
    }

  DCRBLOCK;
  vmeWrite32(&(DCRBp[id]->Cfg.Reset),1);
  taskDelay(2);
  vmeWrite32(&(DCRBp[id]->Cfg.Reset),0);
  DCRBUNLOCK;
}


void
dcrbReset(int id, int iFlag)
{
  unsigned int a32addr, addrMB;

  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
  {
    logMsg("dcrbReset: ERROR : DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
    return;
  }

  DCRBLOCK;
  if(iFlag==0)
  {
    a32addr = vmeRead32(&(DCRBp[id]->EB.AD32));
    addrMB  = vmeRead32(&(DCRBp[id]->EB.Adr32M));
  }

  dcrbHardReset(id);
  taskDelay(120);

  if(iFlag==0)
  {
    vmeWrite32(&(DCRBp[id]->EB.AD32),a32addr);
    vmeWrite32(&(DCRBp[id]->EB.Adr32M),addrMB);
  }
  DCRBUNLOCK;
}

void
dcrbChanDisable(int id, unsigned int cmask0, unsigned int cmask1, unsigned int cmask2)
{

  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
  {
    logMsg("dcrbChanDisable: ERROR : DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
    return;
  }

  DCRBLOCK;
  /* Write New Disable Mask */
  vmeWrite32(&(DCRBp[id]->Tdc[0].EnableN), (cmask0>>0)&0xFFFF);
  vmeWrite32(&(DCRBp[id]->Tdc[1].EnableN), (cmask0>>16)&0xFFFF);
  vmeWrite32(&(DCRBp[id]->Tdc[2].EnableN), (cmask1>>0)&0xFFFF);
  vmeWrite32(&(DCRBp[id]->Tdc[3].EnableN), (cmask1>>16)&0xFFFF);
  vmeWrite32(&(DCRBp[id]->Tdc[4].EnableN), (cmask2>>0)&0xFFFF);
  vmeWrite32(&(DCRBp[id]->Tdc[5].EnableN), (cmask2>>16)&0xFFFF);
  DCRBUNLOCK;

}

/* Return Event/Block count for DCRB in slot id */
int
dcrbDready(int id, int dflag)
{
  unsigned int dcnt=0;

  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
    {
      logMsg("dcrbDready: ERROR : DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }
  DCRBLOCK;
  if(dflag)
    dcnt = vmeRead32(&(DCRBp[id]->EB.FifoBlockCnt));
  else
    dcnt = vmeRead32(&(DCRBp[id]->EB.FifoEventCnt));
  DCRBUNLOCK;
  
  return(dcnt);
}

/* Return a Block Ready status for DCRB. If Block Level is =1 then return Event Ready status */
int
dcrbBready(int id)
{
  int stat=0;

  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
    {
      logMsg("dcrbBready: ERROR : DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }
  
  DCRBLOCK;
  stat = vmeRead32(&(DCRBp[id]->EB.FifoBlockCnt));
  DCRBUNLOCK;

  if(stat)
    return(1);
  else
    return(0);
}

unsigned int
dcrbGBready()
{
  int ii, id, stat=0;
  unsigned int dmask=0;
  
  DCRBLOCK;
  for(ii=0;ii<ndcrb;ii++) 
    {
      id = dcrbID[ii];
      stat = vmeRead32(&(DCRBp[id]->EB.FifoBlockCnt));
 
      if(stat)
	dmask |= (1<<id);
    }
  DCRBUNLOCK;
  
  return(dmask);
}

/* return Scan mask for all initialized DCRBs */
unsigned int
dcrbScanMask()
{
  int idcrb, id, dmask=0;

  for(idcrb=0; idcrb<ndcrb; idcrb++)
    {
      id = dcrbID[idcrb];
      dmask |= (1<<id);
    }

  return(dmask);
}

void
dcrbEnableBusError(int id)
{

  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
    {
      logMsg("dcrbEnableBusError: ERROR : DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
      return;
    }
  
  DCRBLOCK;
  vmeWrite32(&(DCRBp[id]->EB.ReadoutCfg),
    vmeRead32(&(DCRBp[id]->EB.ReadoutCfg)) | DCRB_ENABLE_BERR );
  DCRBUNLOCK;
}

void
dcrbGEnableBusError()
{
  int ii;

  DCRBLOCK;
  for(ii=0;ii<ndcrb;ii++) 
  {
    vmeWrite32(&(DCRBp[dcrbID[ii]]->EB.ReadoutCfg),
      vmeRead32(&(DCRBp[dcrbID[ii]]->EB.ReadoutCfg)) | DCRB_ENABLE_BERR );
  }
  DCRBUNLOCK;
}


void
dcrbDisableBusError(int id)
{

  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
    {
      logMsg("dcrbDisableBusError: ERROR : DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
      return;
    }
  
  DCRBLOCK;
  vmeWrite32(&(DCRBp[id]->EB.ReadoutCfg),
	    vmeRead32(&(DCRBp[id]->EB.ReadoutCfg)) & ~DCRB_ENABLE_BERR );
  DCRBUNLOCK;

}


void
dcrbEnableMultiBlock()
{
  int ii, id;

  if((ndcrb <= 1) || (DCRBp[dcrbID[0]] == NULL)) 
    {
      logMsg("dcrbEnableMultiBlock: ERROR : Cannot Enable MultiBlock mode \n",0,0,0,0,0,0);
      return;
    }
  
  for(ii=0;ii<ndcrb;ii++) 
    {
      id = dcrbID[ii];
      DCRBLOCK;
      vmeWrite32(&(DCRBp[id]->EB.Adr32M),
		vmeRead32(&(DCRBp[id]->EB.Adr32M)) | DCRB_ENABLE_MULTIBLOCK );
      DCRBUNLOCK;
      dcrbDisableBusError(id);
      if(id == dcrbMinSlot) 
	{
	  DCRBLOCK;
	  vmeWrite32(&(DCRBp[id]->EB.Adr32M),
		    vmeRead32(&(DCRBp[id]->EB.Adr32M)) | DCRB_FIRST_BOARD );
	  DCRBUNLOCK;
	}
      if(id == dcrbMaxSlot) 
	{
	  DCRBLOCK;
	  vmeWrite32(&(DCRBp[id]->EB.Adr32M),
		    vmeRead32(&(DCRBp[id]->EB.Adr32M)) | DCRB_LAST_BOARD );
	  DCRBUNLOCK;
	  dcrbEnableBusError(id);   /* Enable Bus Error only on Last Board */
	}
    }

}

void
dcrbDisableMultiBlock()
{
  int ii;

  if((ndcrb <= 1) || (DCRBp[dcrbID[0]] == NULL)) 
    {
      logMsg("dcrbDisableMultiBlock: ERROR : Cannot Disable MultiBlock Mode\n",0,0,0,0,0,0);
      return;
    }
  
  DCRBLOCK;
  for(ii=0;ii<ndcrb;ii++)
    vmeWrite32(&(DCRBp[dcrbID[ii]]->EB.Adr32M),
	      vmeRead32(&(DCRBp[dcrbID[ii]]->EB.Adr32M)) & ~DCRB_ENABLE_MULTIBLOCK );
  DCRBUNLOCK;
}



int
dcrbSetBlockLevel(int id, int level)
{
  int rval;

  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
    {
      logMsg("dcrbSetBlockLevel: ERROR : DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }
  
  if(level<=0) level = 1;
  DCRBLOCK;
  vmeWrite32(&(DCRBp[id]->EB.BlockCfg), level);
  dcrbBlockLevel = level;
  rval = vmeRead32(&(DCRBp[id]->EB.BlockCfg)) & DCRB_BLOCK_LEVEL_MASK;
  DCRBUNLOCK;

  return(rval);

}
void
dcrbGSetBlockLevel(int level)
{
  int ii;

  if(level<=0) level = 1;
  DCRBLOCK;
  for(ii=0;ii<ndcrb;ii++)
    vmeWrite32(&(DCRBp[dcrbID[ii]]->EB.BlockCfg), level);
  DCRBUNLOCK;

  dcrbBlockLevel = level;
}

int  
dcrbPulserSetup(int id, float freq, float duty, unsigned int npulses)
{ 
  unsigned int per, low; 
	 
  if(id==0) id=dcrbID[0];
  
  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
  {
    printf("%s: ERROR : DCRB in slot %d is not initialized \n", __FUNCTION__, id);
    return(ERROR);
  }

  if(freq < DCRB_PULSER_FREQ_MIN)
  { 
    printf("%s: ERROR: Frequency input (%f) too low. Setting to minimum...\n",  
	     __FUNCTION__,freq); 
    freq = DCRB_PULSER_FREQ_MIN;
  } 
	 
  if(freq > DCRB_PULSER_FREQ_MAX) 
  { 
    printf("%s: ERROR: Frequency input (%f) too high. Setting to maximum...\n",  
	     __FUNCTION__,freq); 
    freq = DCRB_PULSER_FREQ_MAX; 
  } 
	 
  if((duty < 0.0) || (duty > 1.0)) 
  { 
    printf("%s: ERROR: Invalid duty cycle %f. Setting to 0.5\n",  
	     __FUNCTION__,duty); 
    duty = 0.5; 
  } 
 
  DCRBLOCK;	 
  // Setup period register... 
  per = 125000000.0f / freq; 
  if(!per) 
    per = 1; 
  vmeWrite32(&DCRBp[id]->Sd.PulserPeriod, per); 
	 
  // Setup duty cycle register...	 
  low = per * duty; 
  if(!low) 
    low = 1; 
  vmeWrite32(&DCRBp[id]->Sd.PulserLowCycles, low); 
  vmeWrite32(&DCRBp[id]->Sd.PulserNPulses, npulses);
	 
  printf("%s: Actual frequency = %f, duty = %f\n", __FUNCTION__, 
	 125000000.0f/(float)per, (float)low/(float)per); 
  DCRBUNLOCK; 

/*	if(npulses<0xFFFFFFFF) dcrbPulserStart(id); */
  return OK;
} 

int
dcrbSetDAC_Pulser(int id, int grp_mask, float freq, int offset_mV, int low_mV, int high_mV, int width)
{
  unsigned int period;

  if(id==0) id=dcrbID[0];
  
  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
  {
    logMsg("dcrbSetDAC: ERROR : DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
    return(ERROR);
  }

  DCRBLOCK;
  vmeWrite32(&DCRBp[id]->Cfg.DacLow, low_mV+2048);
  vmeWrite32(&DCRBp[id]->Cfg.DacHigh, high_mV+2048);
  vmeWrite32(&DCRBp[id]->Cfg.DacOffset, offset_mV+2048);

  if(grp_mask & 0x1)   vmeWrite32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_DCE0], 18);
  else                 vmeWrite32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_DCE0], 0);
  if(grp_mask & 0x2)   vmeWrite32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_DCE1], 18);
  else                 vmeWrite32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_DCE1], 0);
  if(grp_mask & 0x4)   vmeWrite32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_DCE2], 18);
  else                 vmeWrite32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_DCE2], 0);
  if(grp_mask & 0x8)   vmeWrite32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_FCE0], 18);
  else                 vmeWrite32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_FCE0], 0);
  if(grp_mask & 0x10)  vmeWrite32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_FCE1], 18);
  else                 vmeWrite32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_FCE1], 0);
  if(grp_mask & 0x20)  vmeWrite32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_FCE2], 18);
  else                 vmeWrite32(&DCRBp[id]->Sd.SrcSel[DCRB_OUTPUT_FCE2], 0);
  DCRBUNLOCK;

  dcrbPulserSetup(id, freq, 2.0f/125000000.0f, 0xFFFFFFFF);

  return(OK);
}

int
dcrbSetDAC(int id, int dvalue)
{
  int dac = (int)(((float)dvalue) * 8.06f + 2048.0f);

  if(id==0) id=dcrbID[0];
  
  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
  {
    logMsg("dcrbSetDAC: ERROR : DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
    return(ERROR);
  }

  if(dac < 0)
  {
    dac = 0;
    dvalue = ((float)(dac - 2048) / 8.06f);
    printf("Error in %s: threshold too low, saturating at: %dmV\n", __FUNCTION__, dvalue);
  }
  else if (dac > 4095)
  {
    dac = 4095;
    dvalue = ((float)(dac - 2048) / 8.06f);
    printf("Error in %s: threshold too high, saturating at: %dmV\n", __FUNCTION__, dvalue);
  }
  
  DCRBLOCK;
  vmeWrite32(&DCRBp[id]->Cfg.DacThreshold, dac);
  DCRBUNLOCK;

  return(OK);
}

int
dcrbGetDAC(int id)
{
  int thr;
  if(id==0) id=dcrbID[0];
  
  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
  {
    logMsg("dcrbSetDAC: ERROR : DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
    return(ERROR);
  }
  thr = vmeRead32(&DCRBp[id]->Cfg.DacThreshold);
  thr = (int)( ((float)(thr - 2048)) / 8.06f);

  return(thr);
}

void
dcrbGSetDAC(unsigned int dvalue)
{
  int ii;
  for(ii=0;ii<ndcrb;ii++)
    dcrbSetDAC(dcrbID[ii], dvalue);
}

void dcrbSelectSpi(int id,int sel)
{
	if(sel)
		vmeWrite32(&DCRBp[id]->Cfg.SpiCtrl, DCRB_SPI_NCS_CLEAR);
	else
		vmeWrite32(&DCRBp[id]->Cfg.SpiCtrl, DCRB_SPI_NCS_SET);
}

unsigned char dcrbTransferSpi(int id,unsigned char data)
{
	unsigned int i, rsp = 0;
	
	vmeWrite32(&DCRBp[id]->Cfg.SpiCtrl, data | DCRB_SPI_TRANSFER);

	for(i = 0; i < 8; i++)
	{
		rsp = vmeRead32(&DCRBp[id]->Cfg.SpiStatus);
		if(rsp & DCRB_SPI_READY)
			break;
	}

	return (unsigned char)(rsp & 0xFF);
}

unsigned int dcrbFlashGetId(int id)
{
	unsigned int rsp;

	dcrbSelectSpi(id,1);
	dcrbTransferSpi(id,FLASH_CMD_GETID);
	rsp = dcrbTransferSpi(id,0xFF);
	rsp |= dcrbTransferSpi(id,0xFF)<<8;
	rsp |= dcrbTransferSpi(id,0xFF)<<16;
	rsp |= dcrbTransferSpi(id,0xFF)<<24;
	dcrbSelectSpi(id,0);

	return rsp;
}

unsigned char dcrbFlashGetStatus(int id, unsigned char cmd)
{
	unsigned char rsp;
	
	dcrbSelectSpi(id,1);
	dcrbTransferSpi(id,cmd);
	rsp = dcrbTransferSpi(id,0xFF);
	dcrbSelectSpi(id,0);
	
	return rsp;
}

int dcrbFirmwareUpdateVerify(int id, const char *filename)
{
	int i, result;

  if(id==0) id=dcrbID[0];
  
  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
    {
      printf("dcrbFirmwareUpdateVerify: ERROR : DCRB in slot %d is not initialized \n",id);
      return(ERROR);
    }

	printf("Slot %d, Updating firmware...", id);
	result = dcrbFirmwareUpdate(id, filename);
	if(result != OK)
	{
		printf("failed.\n");
		return result;
	}
	else
		printf("succeeded.\n");
	
	printf("\nVerifying...");
	result = dcrbFirmwareVerify(id, filename);
	if(result != OK)
	{
		printf("failed.\n");
		return result;
	}
	else
		printf("ok.\n");

	dcrbHardReset(id);
	taskDelay(120);
		
	return OK;
}

void dcrbGFirmwareUpdateVerify(const char *filename)
{
	int ii;
	for(ii = 0; ii < ndcrb; ii++)
		dcrbFirmwareUpdateVerify(dcrbID[ii], filename);
}

int dcrbFirmwareUpdate(int id, const char *filename)
{
	FILE *f;
	int i, flashId = 0;
	unsigned int addr = 0, page = 0, rspId;
	unsigned char buf[1056];

  if(id==0) id=dcrbID[0];
  
  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
    {
      printf("dcrbFirmwareUpdate: ERROR : DCRB in slot %d is not initialized \n",id);
      return(ERROR);
    }
	
	dcrbSelectSpi(id,0);
	rspId = dcrbFlashGetId(id);
	
//	printf("Flash: Mfg=0x%02X, Type=0x%02X, Capacity=0x%02X\n", (rspId>>0)&0xFF, (rspId>>8)&0xFF, (rspId>>16)&0xFF);

	if(rspId == FLASH_DEV_M25P64)
	{
		f = fopen(filename, "rb");
		if(!f)
		{
			printf("%s: ERROR: dcrbFirmwareUpdate invalid file %s\n", __FUNCTION__, filename);
			return ERROR;
		}
	
		memset(buf, 0xff, 256);
		while(fread(buf, 1, 256, f) > 0)
		{
		  if(!(addr % 65536))		/* sector erase*/
			{
				dcrbSelectSpi(id,1);
				dcrbTransferSpi(id,FLASH_CMD_WREN);	/* write enable*/
				dcrbSelectSpi(id,0);

				dcrbSelectSpi(id,1);
				dcrbTransferSpi(id,FLASH_CMD_ERASE64K);	/* 64k sector erase*/
				dcrbTransferSpi(id,(addr>>16)&0xFF);
				dcrbTransferSpi(id,(addr>>8)&0xFF);
				dcrbTransferSpi(id,(addr)&0xFF);
				dcrbSelectSpi(id,0);

				printf(".");
				i = 0;
				while(1)
				{
					if(!(dcrbFlashGetStatus(id, FLASH_CMD_GETSTATUS) & 0x1))
						break;
					taskDelay(1);
					if(i == 60+6)	/* 1000ms maximum sector erase time*/
					{
						fclose(f);
						printf("%s: ERROR: dcrbFirmwareUpdate failed to erase flash\n", __FUNCTION__);
						return ERROR;
					}
					i++;
				}
			}

			dcrbSelectSpi(id,1);
			dcrbTransferSpi(id,FLASH_CMD_WREN);	/* write enable*/
			dcrbSelectSpi(id,0);

			dcrbSelectSpi(id,1);
			dcrbTransferSpi(id,FLASH_CMD_WRPAGE);	/* write page*/
			dcrbTransferSpi(id,(addr>>16)&0xFF);
			dcrbTransferSpi(id,(addr>>8)&0xFF);
			dcrbTransferSpi(id,(addr)&0xFF);
			for(i = 0; i < 256; i++)
				dcrbTransferSpi(id,buf[i]);
			dcrbSelectSpi(id,0);

			i = 0;
			while(1)
			{
			  if(!(dcrbFlashGetStatus(id, FLASH_CMD_GETSTATUS) & 0x1))	/* no faster than 1us per call*/
					break;
			  if(i == 3000)	/* 3ms maximum page program time*/
				{
					fclose(f);
					printf("%s: ERROR: dcrbFirmwareUpdate failed to program flash\n", __FUNCTION__);
					return ERROR;
				}
				i++;
			}
			memset(buf, 0xff, 256);
			addr+= 256;
		}
		fclose(f);
	}
	else
	{
		printf("%s: ERROR: dcrbFirmwareUpdate failed to identify flash id (or device not supported)\n", __FUNCTION__);
		return ERROR;
	}
	return OK;
}

int dcrbFirmwareRead(int id, const char *filename)
{
	FILE *f;
	int i,len, flashId = 0;
	unsigned int addr = 0, rspId;
	unsigned char buf[256];

  if(id==0) id=dcrbID[0];
  
  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
    {
      printf("dcrbFirmwareRead: ERROR : DCRB in slot %d is not initialized \n",id);
      return(ERROR);
    }

	dcrbSelectSpi(id,0);
	rspId = dcrbFlashGetId(id);
	
	printf("Flash: Mfg=0x%02X, Type=0x%02X, Capacity=0x%02X\n", (rspId>>0)&0xFF, (rspId>>8)&0xFF, (rspId>>16)&0xFF);

	if(rspId == FLASH_DEV_M25P64)
	{
		f = fopen(filename, "wb");
		if(!f)
		{
			printf("%s: ERROR: dcrbFirmwareRead invalid file %s\n", __FUNCTION__, filename);
			return ERROR;
		}
		
		dcrbSelectSpi(id,1);
		dcrbTransferSpi(id,FLASH_CMD_RD);	/* continuous array read */
		dcrbTransferSpi(id,(addr>>16)&0xFF);
		dcrbTransferSpi(id,(addr>>8)&0xFF);
		dcrbTransferSpi(id,(addr)&0xFF);
		
		for(i = 0; i < FLASH_BYTE_LENGTH; i++)
		{
			fputc(dcrbTransferSpi(id,0xFF), f);
			if(!(i% 65536))
			{
				printf(".");
				taskDelay(1);
			}
		}
			
		dcrbSelectSpi(id,0);
		fclose(f);
	}
	else
	{
		printf("%s: ERROR: dcrbFirmwareRead failed to identify flash id 0x%02X\n", __FUNCTION__, flashId);
		return ERROR;
	}
	return OK;
}

int dcrbFirmwareVerify(int id, const char *filename)
{
	FILE *f;
	int i,len, flashId = 0;
	unsigned int addr = 0, rspId;
	unsigned char buf[256];
	unsigned char val;
	
  if(id==0) id=dcrbID[0];
  
  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
    {
      logMsg("dcrbFirmwareVerify: ERROR : DCRB in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }
	
	dcrbSelectSpi(id,0);
	rspId = dcrbFlashGetId(id);
	
//	printf("Flash: Mfg=0x%02X, Type=0x%02X, Capacity=0x%02X\n", (rspId>>0)&0xFF, (rspId>>8)&0xFF, (rspId>>16)&0xFF);

	if(rspId == FLASH_DEV_M25P64)
	{
		f = fopen(filename, "rb");
		if(!f)
		{
			printf("%s: ERROR: dcrbFirmwareVerify invalid file %s\n", __FUNCTION__, filename);
			return ERROR;
		}
		
		dcrbSelectSpi(id,1);
		dcrbTransferSpi(id,FLASH_CMD_RD);	/* continuous array read */
		dcrbTransferSpi(id,(addr>>16)&0xFF);
		dcrbTransferSpi(id,(addr>>8)&0xFF);
		dcrbTransferSpi(id,(addr)&0xFF);

		while((len = fread(buf, 1, 256, f)) > 0)
		{
			for(i = 0; i < len; i++)
			{
				val = dcrbTransferSpi(id,0xFF);
				if(buf[i] != val)
				{
					dcrbSelectSpi(id,0);
					fclose(f);					
					printf("%s: ERROR: dcrbFirmwareVerify failed verify at addess 0x%08X[%02X,%02X]\n", __FUNCTION__, addr+i, buf[i], val);
					return ERROR;
				}
			}
			addr+=256;
			if(!(addr & 0xFFFF))
				printf(".");
		}
		dcrbSelectSpi(id,0);
		fclose(f);
	}
	else
	{
		printf("%s: ERROR: dcrbFirmwareVerify failed to identify flash id 0x%02X\n", __FUNCTION__, flashId);
		return ERROR;
	}
	return OK;
}

void 
dcrbDataDecode(unsigned int data)
{
	printf("0x%08X", data);

	if(data & 0x80000000)
	{
		int type = data & 0x78000000;
		switch(type)
		{
			case DCRB_DATA_BLOCK_HEADER:
				printf(" {BLKHDR} SLOTID: %d", (data>>22)&0x1f);
				printf(" NEVENTS: %d", (data>>11)&0x7ff);
				printf(" BLOCK: %d\n", (data>>0)&0x7ff);				
				break;
			case DCRB_DATA_BLOCK_TRAILER:
				printf(" {BLKTLR} SLOTID: %d", (data>>22)&0x1f);
				printf(" NWORDS: %d\n", (data>>0)&0x3fffff);
				break;
			case DCRB_DATA_EVENT_HEADER:
				printf(" {EVTHDR} EVENT: %d\n", (data>>0)&0x7ffffff);
				break;
			case DCRB_DATA_TRIGGER_TIME:
				printf(" {TRGTIME}\n");
				break;
			case DCRB_DATA_TDCEVT:
				printf(" {TDCEVT} CH: %d", (data>>16)&0x7f);
				printf(" TIME: %dns\n", (data>>0)&0xFFFF);
				break;
			case DCRB_DATA_INVALID:
				printf(" {***DNV***}\n");
				break;
			case DCRB_DATA_FILLER:
				printf(" {FILLER}\n");
				break;
			default:
				printf(" {***DATATYPE ERROR***}\n");
				break;
		}
	}
	else
		printf("\n");

  return;
}


/* width in ns */
int
dcrbSetTriggerPulseWidth(int id, unsigned int width)
{
  int i;
  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
  {
    printf("dcrbSoftTrig: ERROR : DCRB in slot %d is not initialized \n",id);
    return;
  }

  width /= DCRB_NS_PER_CLOCK;

  for(i=0;i<6;i++)
    vmeWrite32(&(DCRBp[id]->Tdc[i].TriggerWidth),width);

  return(width);
}

int
dcrbGetTriggerPulseWidth(int id)
{
  unsigned int width;
  
  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
  {
    printf("dcrbSoftTrig: ERROR : DCRB in slot %d is not initialized \n",id);
    return;
  }

  width = vmeRead32(&(DCRBp[id]->Tdc[0].TriggerWidth));
  //vmeWrite32(&(DCRBp[id]->Tdc[1].TriggerWidth),width);
  //vmeWrite32(&(DCRBp[id]->Tdc[2].TriggerWidth),width);
  //vmeWrite32(&(DCRBp[id]->Tdc[3].TriggerWidth),width);
  //vmeWrite32(&(DCRBp[id]->Tdc[4].TriggerWidth),width);
  //vmeWrite32(&(DCRBp[id]->Tdc[5].TriggerWidth),width);
  
  return(width*DCRB_NS_PER_CLOCK);
}

/* returns 1 if link is UP, 0, otherwise */
int
dcrbLinkStatus(int id)
{
  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
  {
    printf("dcrbSoftTrig: ERROR : DCRB in slot %d is not initialized \n",id);
    return;
  }

  return( ( (vmeRead32(&(DCRBp[id]->Ser[1].Status)))&0x1000 )>>12 );
}

int
dcrbLinkReset(int id)
{
  if(id==0) id=dcrbID[0];

  if((id<=0) || (id>21) || (DCRBp[id] == NULL)) 
  {
    printf("dcrbSoftTrig: ERROR : DCRB in slot %d is not initialized \n",id);
    return;
  }

  vmeWrite32(&(DCRBp[id]->Ser[1].Ctrl), 0x201);
//  vmeWrite32(&(DCRBp[id]->Ser[0].Ctrl), 0x201);
  taskDelay(1);
  vmeWrite32(&(DCRBp[id]->Ser[1].Ctrl), 0x200);
//  vmeWrite32(&(DCRBp[id]->Ser[0].Ctrl), 0x200);
  taskDelay(10);
  vmeWrite32(&(DCRBp[id]->Ser[1].Ctrl), 0x800);
//  vmeWrite32(&(DCRBp[id]->Ser[0].Ctrl), 0x800);
  taskDelay(1);


  return(0);
}










/**** CONFIG ************************************/
typedef struct
{
  int window_offset[DCRB_MAX_BOARDS];
  int window_width[DCRB_MAX_BOARDS];
  int deadtime[DCRB_MAX_BOARDS];
  int threshold[DCRB_MAX_BOARDS];
  int trig_hit_width[DCRB_MAX_BOARDS];
} DCRB_CONFIG_STRUCT;

DCRB_CONFIG_STRUCT conf;


static char *expid = NULL;

void
dcrbSetExpid(char *string)
{
  expid = strdup(string);
}

void
dcrbInitGlobals()
{
  int ii;

  for(ii=0; ii<DCRB_MAX_BOARDS; ii++)
  {
    conf.window_offset[ii] = 0;
    conf.window_width[ii] = 0;
    conf.deadtime[ii] = 0;
    conf.threshold[ii] = 20;
    conf.trig_hit_width[ii] = 2000;
  }
 
  return;
}



#define NCHAN 8
#define SCAN_MSK \
	args = sscanf (str_tmp, "%*s %d %d %d %d %d %d %d %d   \
                                     %d %d %d %d %d %d %d %d", \
		       &msk[ 0], &msk[ 1], &msk[ 2], &msk[ 3], \
		       &msk[ 4], &msk[ 5], &msk[ 6], &msk[ 7], \
		       &msk[ 8], &msk[ 9], &msk[10], &msk[11], \
		       &msk[12], &msk[13], &msk[14], &msk[15])


int
dcrbReadConfigFile(char *filename)
{
  FILE   *fd;
  char   fname[FNLEN] = { "" };  /* config file name */
  int    ii, jj, ch;
  char   str_tmp[STRLEN], keyword[ROCLEN];
  char   host[ROCLEN], ROC_name[ROCLEN];
  char   str2[2];
  int    args, i1, i2, i3, i4, msk[NCHAN];
  float  f1;
  int    slot1, slot2, slot, chan;
  unsigned int  ui1, ui2;
  char *getenv();
  char *clonparms;

  gethostname(host,ROCLEN);  /* obtain our hostname */
  clonparms = getenv("CLON_PARMS");

  if(expid==NULL)
  {
    expid = getenv("EXPID");
    printf("\nNOTE: use EXPID=>%s< from environment\n",expid);
  }
  else
  {
    printf("\nNOTE: use EXPID=>%s< from CODA\n",expid);
  }

  if(strlen(filename)!=0) /* filename specified */
  {
    if ( filename[0]=='/' || (filename[0]=='.' && filename[1]=='/') )
	{
      sprintf(fname, "%s", filename);
	}
    else
	{
      sprintf(fname, "%s/dcrb/%s", clonparms, filename);
	}

    if((fd=fopen(fname,"r")) == NULL)
    {
      printf("\ndcrbReadConfigFile: Can't open config file >%s<\n",fname);
      return(-1);
    }
  }
  else /* filename does not specified */
  {
    sprintf(fname, "%s/dcrb/%s.cnf", clonparms, host);
    if((fd=fopen(fname,"r")) == NULL)
    {
      sprintf(fname, "%s/dcrb/%s.cnf", clonparms, expid);
      if((fd=fopen(fname,"r")) == NULL)
      {
        printf("\ndcrbReadConfigFile: Can't open config file >%s<\n",fname);
        return(-2);
	  }
	}

  }
  printf("\ndcrbReadConfigFile: Using configuration file >%s<\n",fname);

  /* Parsing of config file */
  active = 0;
  while ((ch = getc(fd)) != EOF)
  {
    if ( ch == '#' || ch == ' ' || ch == '\t' )
    {
      while (getc(fd) != '\n') {}
    }
    else if( ch == '\n' ) {}
    else
    {
      ungetc(ch,fd);
      fgets(str_tmp, STRLEN, fd);
      sscanf (str_tmp, "%s %s", keyword, ROC_name);

      /* Start parsing real config inputs */
      if(strcmp(keyword,"DCRB_CRATE") == 0)
      {
        if(strcmp(ROC_name,host) == 0)
        {
          printf("\nReadConfigFile: crate = %s  host = %s - activated\n",ROC_name,host);
          active = 1;
        }
        else if(strcmp(ROC_name,"all") == 0)
        {
          printf("\nReadConfigFile: crate = %s  host = %s - activated\n",ROC_name,host);
          active = 1;
        }
        else
        {
          printf("\nReadConfigFile: crate = %s  host = %s - disactivated\n",ROC_name,host);
          active = 0;
        }
      }
      else if(active && (strcmp(keyword,"DCRB_SLOT")==0))
      {
        sscanf (str_tmp, "%*s %s", str2);
        if(isdigit(str2[0]))
        {
          slot1 = atoi(str2);
          slot2 = slot1 + 1;
          if(slot1<2 && slot1>21)
          {
            printf("\nReadConfigFile: Wrong slot number %d\n\n",slot1);
            return(-1);
          }
        }
        else if(!strcmp(str2,"all"))
        {
          slot1 = 0;
          slot2 = DCRB_MAX_BOARDS;
        }
        else
        {
          printf("\nReadConfigFile: Wrong slot >%s<, must be 'all' or actual slot number\n\n",str2);
          return(-1);
        }
      }
      else if(active && (strcmp(keyword,"DCRB_W_OFFSET")==0))
      {
        sscanf (str_tmp, "%*s %d", &i1);
        for(slot=slot1; slot<slot2; slot++) conf.window_offset[slot] = i1;
      }
      else if(active && (strcmp(keyword,"DCRB_W_WIDTH")==0))
      {
        sscanf (str_tmp, "%*s %d", &i1);
        for(slot=slot1; slot<slot2; slot++) conf.window_width[slot] = i1;
      }
      else if(active && (strcmp(keyword,"DCRB_DEADTIME")==0))
      {
        sscanf (str_tmp, "%*s %d", &i1);
        for(slot=slot1; slot<slot2; slot++) conf.deadtime[slot] = i1;
      }
      else if(active && (strcmp(keyword,"DCRB_THRESHOLD")==0))
      {
        sscanf (str_tmp, "%*s %d", &i1);
        for(slot=slot1; slot<slot2; slot++) conf.threshold[slot] = i1;
      }
      else if(active && (strcmp(keyword,"DCRB_HIT_TRIG_WIDTH")==0))
      {
        sscanf (str_tmp, "%*s %d", &i1);
        for(slot=slot1; slot<slot2; slot++) conf.trig_hit_width[slot] = i1;
      }
      else
      {
        ; /* unknown key - do nothing */
        /*
        printf("dcrbReadConfigFile: Unknown Field or Missed Field in\n");
        printf("   %s \n", fname);
        printf("   str_tmp=%s", str_tmp);
        printf("   keyword=%s \n\n", keyword);
        return(-10);
        */
      }

    }
  } /* end of while */

  fclose(fd);

  return(0);
}

int
dcrbDownloadAll()
{
  int id, slot;

  for(id=0; id<ndcrb; id++)
  {
    slot = dcrbSlot(id);

    dcrbSetProcMode(slot, conf.window_offset[slot], conf.window_width[slot], conf.deadtime[slot]);
    dcrbSetDAC(slot, conf.threshold[slot]);
    dcrbSetTriggerPulseWidth(slot, conf.trig_hit_width[slot]);

    /* reset all TDC serdes */
    dcrbChanDisable(slot, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);

    /* release reset to all TDC serdes */
    dcrbChanDisable(slot, 0x00000000, 0x00000000, 0x00000000);
  }

  return(0);
}

/* dcrbInit() have to be called before this function */
int  
dcrbConfig(char *fname)
{
  int res;
  char *string; /*dummy, will not be used*/

  if(strlen(fname) > 0) /* filename specified  - upload initial settings from the hardware */
  {
    dcrbUploadAll(string, 0);
  }
  else /* filename not specified  - set defaults */
  {
    dcrbInitGlobals();
  }

  /* read config file */
  if( (res = dcrbReadConfigFile(fname)) < 0 ) return(res);

  /* download to all boards */
  dcrbDownloadAll();

  return(0);
}

void
dcrbMon(int slot)
{
  int id;

}



/* upload setting from all found DSC2s */
int
dcrbUploadAll(char *string, int length)
{
  int id, slot, i, ii, jj, kk, ifiber, len1, len2;
  char *str, sss[1024];
  unsigned int tmp, connectedfibers;
  unsigned short sval;
  unsigned short bypMask;
  unsigned short channels[8];

  for(id=0; id<ndcrb; id++)
  {
    slot = dcrbSlot(id);
    conf.window_offset[slot] = dcrbGetWindowOffset(slot);
    conf.window_width[slot]  = dcrbGetWindowWidth(slot);
    conf.deadtime[slot]      = dcrbGetDeadtime(slot);
    conf.threshold[slot]     = dcrbGetDAC(slot);
    conf.trig_hit_width[slot] = dcrbGetTriggerPulseWidth(slot);
    
    printf("DCRB slot=%d: w_offset=%d, w_width=%d, deadtime=%d, threshold=%d\n",
		   slot,conf.window_offset[slot],conf.window_width[slot],conf.deadtime[slot],conf.threshold[slot]);
  }

  if(length)
  {
    str = string;
    str[0] = '\0';

    for(id=0; id<ndcrb; id++)
    {
      slot = dcrbSlot(id);
      sprintf(sss,"DCRB_SLOT %d\n",slot);ADD_TO_STRING;

      sprintf(sss,"DCRB_W_OFFSET %d\n", conf.window_offset[slot]); ADD_TO_STRING;
      sprintf(sss,"DCRB_W_WIDTH %d\n", conf.window_width[slot]); ADD_TO_STRING;
      sprintf(sss,"DCRB_DEADTIME %d\n", conf.deadtime[slot]); ADD_TO_STRING;
      sprintf(sss,"DCRB_THRESHOLD %d\n", conf.threshold[slot]); ADD_TO_STRING;
      sprintf(sss,"DCRB_HIT_TRIG_WIDTH %d\n", conf.trig_hit_width[slot]); ADD_TO_STRING;
    }

    CLOSE_STRING;
  }

}

int
dcrbUploadAllPrint()
{
  char str[8192];
  dcrbUploadAll(str, 8191);
  printf("%s",str);
}













#else /* dummy version*/

void
dcrbLib_dummy()
{
  return;
}

#endif
