/******************************************************************************
*
*  dcLib.h  - Driver library header file for readout of the dc boards
*
*  Author: Ber Raydo
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


/* Include ADC definitions */
#include "dcLib.h"

#undef DEBUG

#ifdef VXWORKS
#define DCLOCK
#define DCUNLOCK
#else
/* Mutex to guard flexio read/writes */
pthread_mutex_t   dcMutex = PTHREAD_MUTEX_INITIALIZER;
#define DCLOCK      if(pthread_mutex_lock(&dcMutex)<0) perror("pthread_mutex_lock");
#define DCUNLOCK    if(pthread_mutex_unlock(&dcMutex)<0) perror("pthread_mutex_unlock");
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
BOOL              dcIntRunning  = FALSE;                    /* running flag */
int               dcIntID       = -1;                       /* id number of ADC generating interrupts */
LOCAL VOIDFUNCPTR dcIntRoutine  = NULL;                     /* user interrupt service routine */
LOCAL int         dcIntArg      = 0;                        /* arg to user routine */
LOCAL UINT32      dcIntLevel    = DC_VME_INT_LEVEL;         /* default VME interrupt level */
LOCAL UINT32      dcIntVec      = DC_VME_INT_VEC;           /* default interrupt Vector */

/* Define global variables */
int ndc = 0;                                       /* Number of DCs in Crate */
int dcA32Base   = 0x09000000;                      /* Minimum VME A32 Address for use by DCs */
int dcA32Offset = 0x08000000;                      /* Difference in CPU A32 Base - VME A32 Base */
int dcA24Offset = 0x0;                             /* Difference in CPU A24 Base - VME A24 Base */
volatile struct dc_struct *DCp[(DC_MAX_BOARDS+1)]; /* pointers to DC memory map */
volatile unsigned int *DCpd[(DC_MAX_BOARDS+1)];      /* pointers to DC FIFO memory */
volatile unsigned int *DCpmb;                        /* pointer to Multblock window */
int dcID[DC_MAX_BOARDS];                           /* array of slot numbers for DCs */
unsigned int dcAddrList[DC_MAX_BOARDS];            /* array of a24 addresses for DCs */
int dcRev[(DC_MAX_BOARDS+1)];                      /* Board Revision Info for each module */
int dcInited=0;                                    /* >0 if Library has been Initialized before */
int dcMaxSlot=0;                                   /* Highest Slot hold an DC */
int dcMinSlot=0;                                   /* Lowest Slot holding an DC */
int dcSource=0;                                    /* Signal source for DC system control*/
int dcBlockLevel=0;                                /* Block Level for DCs */
int dcIntCount = 0;                                /* Count of interrupts from DC */
struct dc_data_struct dc_data;


int
dcSlot(unsigned int id)
{
  if(id>=ndc)
  {
    printf("%s: ERROR: Index (%d) >= DCs initialized (%d).\n",__FUNCTION__,id,ndc);
    return(-1);
  }

  return(dcID[id]);
}

int
dcId(unsigned int slot)
{
  int id;

  for(id=0; id<ndc; id++)
  {
    if(dcID[id]==slot)
	{
      return(id);
	}
  }

  printf("%s: ERROR: DC in slot %d does not exist or not initialized.\n",__FUNCTION__,slot);
  return(-1);
}

/*******************************************************************************
 *
 * dcInit - Initialize JLAB FADC Library. 
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
 *             0 Initialize DC (default behavior)
 *             1 Skip initialization (just setup register map pointers)
 *
 *      bit 17:  Use dcAddrList instead of addr and addr_inc
 *               for VME addresses.
 *             0 Initialize with addr and addr_inc
 *             1 Use dcAddrList 
 *      
 *
 * RETURNS: ndc, or 0 if no boards found, or ERROR
 */

int 
dcInit(UINT32 addr, UINT32 addr_inc, int ndcin, int iFlag)
{
  int ii, res, errFlag = 0;
  int boardID = 0;
  int maxSlot = 1;
  int minSlot = 21;
  int trigSrc=0, clkSrc=0, srSrc=0;
  unsigned int rdata, laddr, laddr_inc, a32addr;
  volatile struct dc_struct *dc;
  unsigned short sdata;
  int noBoardInit=0;
  int useList=0;

  /* Check if we have already Initialized boards before */
  if((dcInited>0) && (dcID[0] != 0)) 
  {
    /* Hard Reset of all DC boards in the Crate */
    for(ii=0;ii<ndc;ii++) 
	{
	  dcHardReset(dcID[ii]);
	}
    taskDelay(120);
  }
  
  /* Check if we're initializing using a list */
  useList=(iFlag&(1<<17))>>17;

  /* Check for valid address */
  if(addr==0) 
    {
      printf("dcInit: ERROR: Must specify a Bus (VME-based A24) address for DC 0\n");
      return(ERROR);
    }
  else if(addr > 0x00ffffff) 
    { /* A24 Addressing */
      printf("dcInit: ERROR: A32 Addressing not allowed for DC configuration space\n");
      return(ERROR);
    }
  else
    { /* A24 Addressing */
      if( ((addr_inc==0)||(ndcin==0)) && (useList==0) )
	ndcin = 1; /* assume only one DC to initialize */

      /* get the DC address */
#ifdef VXWORKS
      res = sysBusToLocalAdrs(0x39,(char *)addr,(char **)&laddr);
#else
      res = vmeBusToLocalAdrs(0x39,(char *)addr,(char **)&laddr);
#endif
      if (res != 0) 
	{
#ifdef VXWORKS
	  printf("dcInit: ERROR in sysBusToLocalAdrs(0x39,0x%x,&laddr) \n",addr);
#else
	  printf("dcInit: ERROR in vmeBusToLocalAdrs(0x39,0x%x,&laddr) \n",addr);
#endif
	  return(ERROR);
	}
      dcA24Offset = laddr - addr;
    }

  /* Init Some Global variables */
  dcSource = iFlag & DC_SOURCE_MASK;
  dcInited = ndc = 0;
  bzero((char *)dcID,sizeof(dcID));

  printf("ndcin=%d\n",ndcin);fflush(stdout);
  for (ii=0; ii<ndcin; ii++) 
  {
      if(useList==1)
	{
	  laddr_inc = dcAddrList[ii] + dcA24Offset;
	}
      else
	{
	  laddr_inc = laddr +ii*addr_inc;
	}
      dc = (struct dc_struct *)laddr_inc;
      /* Check if Board exists at that address */
#ifdef VXWORKS
      res = vxMemProbe((char *) &(dc->BoardID),VX_READ,4,(char *)&rdata);
#else
      res = vmeMemProbe((char *) &(dc->BoardID),4,(char *)&rdata);
#endif
      if(res < 0) 
	{
#ifdef DEBUG
      printf("NO ADDRESSABLE BOARD !!!\n");fflush(stdout);
#ifdef VXWORKS
	  printf("dcInit: ERROR: No addressable board at addr=0x%x\n",(UINT32) dc);
#else
	  printf("dcInit: ERROR: No addressable board at VME (Local) addr=0x%x (0x%x)\n",
		 (UINT32) laddr_inc-dcA24Offset, (UINT32) dc);
#endif
#endif
	  /*errFlag = 1;*/
/* 	  break; */
	}
      else 
	{
	  /* Check that it is an DC board */
	  if((rdata&DC_BOARD_MASK) != DC_BOARD_ID) 
	  {
#ifdef DEBUG
	    printf(" ERROR: For board at 0x%x, Invalid Board ID: 0x%x\n",
		     (UINT32) dc, rdata);
#endif
        continue;
	  }
	  /* Check if this is board has a valid slot number */
	  boardID =  ((vmeRead32(&(dc->BlockConfig)))&DC_SLOT_ID_MASK)>>11;
	  if((boardID <= 0)||(boardID >21)) 
	  {
	    printf(" ERROR: Board Slot ID is not in range: %d\n",boardID);
	    continue;
	  }
	  DCp[boardID] = (struct dc_struct *)(laddr_inc);
	  dcRev[boardID] = vmeRead32(&(dc->FirmwareRev));
/* 	} */
	  dcID[ndc] = boardID;
	  if(boardID >= maxSlot) maxSlot = boardID;
	  if(boardID <= minSlot) minSlot = boardID;
	  
	  printf("Initialized DC %2d  Slot # %2d at address 0x%08x (0x%08x) \n",
		 ndc,dcID[ndc],(UINT32) DCp[(dcID[ndc])],
		 (UINT32) DCp[(dcID[ndc])]-dcA24Offset);
	  ndc++;
/* 	  printf("Initialized DC %2d  Slot # %2d at address 0x%08x \n", */
/* 		 ii,dcID[ii],(UINT32) DCp[(dcID[ii])]); */
	}
  }
  printf("Finished boards initialization, ndc=%d\n",ndc);fflush(stdout);


  /* Check if we are to exit when pointers are setup */
  noBoardInit=iFlag&(1<<16);

  /* Hard Reset of all DC boards in the Crate */
  for(ii=0;ii<ndc;ii++) 
  {
    if(!noBoardInit)
	{
      printf("HardReseting slot %d ..\n",dcID[ii]);fflush(stdout);
      /*dcHardReset(dcID[ii]);*/
      printf(".. done.\n");fflush(stdout);
	}
  }
  printf("Finished hard reset\n");fflush(stdout);
  taskDelay(120); 

  /* Initialize Interrupt variables */
  dcIntID = -1;
  dcIntRunning = FALSE;
  dcIntLevel = DC_VME_INT_LEVEL;
  dcIntVec = DC_VME_INT_VEC;
  dcIntRoutine = NULL;
  dcIntArg = 0;

  /* Calculate the A32 Offset for use in Block Transfers */
#ifdef VXWORKS
  res = sysBusToLocalAdrs(0x09,(char *)dcA32Base,(char **)&laddr);
  if (res != 0) 
    {
      printf("dcInit: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",dcA32Base);
      return(ERROR);
    } 
  else 
    {
      dcA32Offset = laddr - dcA32Base;
    }
#else
  res = vmeBusToLocalAdrs(0x09,(char *)dcA32Base,(char **)&laddr);
  if (res != 0) 
    {
      printf("dcInit: ERROR in vmeBusToLocalAdrs(0x09,0x%x,&laddr) \n",dcA32Base);fflush(stdout);
      return(ERROR);
    } 
  else 
    {
      dcA32Offset = laddr - dcA32Base;
    }
#endif

    if((ndc>0) && !noBoardInit)
    {
	  switch(iFlag&0x7)
	    {
		case 0:
			printf("dcInit: Enabling DC for Internal Clock ");
			printf("and Software Triggers (Soft Sync Reset)\n");
			clkSrc  = DC_REF_CLK_INTERNAL;
			trigSrc = 0;
			break;
		case 1: case 2:
			printf("dcInit: Enabling DC for Internal Clock ");
			printf("and VXS Triggers (Soft Sync Reset)\n");
			clkSrc  = DC_REF_CLK_INTERNAL;
			trigSrc = DC_TRIG_P0;
			break;
		case 3:
			printf("dcInit: Enabling DC for Internal Clock ");
			printf("and VXS-HighRes Triggers (Soft Sync Reset)\n");
			clkSrc  = DC_REF_CLK_INTERNAL;
			trigSrc = DC_TRIG_P0 | DC_SYNC_P0;
			break;
		case 4:
			printf("dcInit: Enabling DC for VXS Clock ");
			printf("and Software Triggers (Soft Sync Reset)\n");
			clkSrc  = DC_REF_CLK_P0;
			trigSrc = 0;
			break;
		case 5:
			printf("dcInit: Enabling DC for VXS Clock ");
			printf("and VXS Triggers (Soft Sync Reset)\n");
			clkSrc  = DC_REF_CLK_P0;
			trigSrc = DC_TRIG_P0;
			break;
		case 6: case 7:
			printf("dcInit: Enabling DC for VXS Clock ");
			printf("and VXS Triggers (VXS Sync Reset)\n");
			clkSrc  = DC_REF_CLK_P0;
			trigSrc = DC_TRIG_P0 | DC_SYNC_P0;
			break;
	    }
    }

  /* Enable Clock source - Internal Clk enabled by default */ 
  for(ii=0; ii<ndc; ii++) 
  {
    vmeWrite32(&(DCp[dcID[ii]]->ClockConfig),clkSrc | DC_REF_CLK_RESET);
	vmeWrite32(&(DCp[dcID[ii]]->ClockConfig),clkSrc);
    printf("Board %d (slot %d) ClockConfig=0x%08x\n",ii,dcID[ii],vmeRead32(&(DCp[dcID[ii]]->ClockConfig)));
  }
  taskDelay(1);

  /* Hard Reset FPGAs and FIFOs */
  for(ii=0;ii<ndc;ii++) 
    {
      vmeWrite32(&(DCp[dcID[ii]]->Reset),0);
    }
  taskDelay(1);

  /* Write configuration registers with default/defined Sources */
  for(ii=0;ii<ndc;ii++) 
  {
    
      /* Program an A32 access address for this DC's FIFO */
      a32addr = dcA32Base + ii*DC_MAX_A32_MEM;
#ifdef VXWORKS
      res = sysBusToLocalAdrs(0x09,(char *)a32addr,(char **)&laddr);
      if (res != 0) 
	{
	  printf("dcInit: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",a32addr);
	  return(ERROR);
	}
#else
      res = vmeBusToLocalAdrs(0x09,(char *)a32addr,(char **)&laddr);
      if (res != 0) 
	{
	  printf("dcInit: ERROR in vmeBusToLocalAdrs(0x09,0x%x,&laddr) \n",a32addr);
	  return(ERROR);
	}
#endif
      DCpd[dcID[ii]] = (unsigned int *)(laddr);  /* Set a pointer to the FIFO */
      if(!noBoardInit)
	{
	  vmeWrite32(&(DCp[dcID[ii]]->ADR32),(a32addr>>16) + DC_A32_ENABLE);  /* Write the register and enable */
	
	  /* Set Default Block Level to 1 */
	  vmeWrite32(&(DCp[dcID[ii]]->BlockConfig),1);

      /* berr for every board; TODO: multiblock needs it on last board only !!!*/
      vmeWrite32(&(DCp[dcID[ii]]->ReadoutConfig), DC_ENABLE_BERR);

	}
      dcBlockLevel=1;

      /* Setup Trigger and Sync Reset sources */
      if(!noBoardInit)
	{
	  vmeWrite32(&(DCp[dcID[ii]]->TriggerSource),trigSrc);
	}
  }

  /* If there are more than 1 DC in the crate then setup the Muliblock Address
     window. This must be the same on each board in the crate */
  if(ndc > 1) 
  {
      a32addr = dcA32Base + (ndc+1)*DC_MAX_A32_MEM; /* set MB base above individual board base */
#ifdef VXWORKS
      res = sysBusToLocalAdrs(0x09,(char *)a32addr,(char **)&laddr);
      if (res != 0) 
	{
	  printf("dcInit: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",a32addr);
	  return(ERROR);
	}
#else
      res = vmeBusToLocalAdrs(0x09,(char *)a32addr,(char **)&laddr);
      if (res != 0) 
	{
	  printf("dcInit: ERROR in vmeBusToLocalAdrs(0x09,0x%x,&laddr) \n",a32addr);
	  return(ERROR);
	}
#endif
      DCpmb = (unsigned int *)(laddr);  /* Set a pointer to the FIFO */


goto skipmultiblock;
    if(!noBoardInit)
	{
	  for (ii=0;ii<ndc;ii++) 
	    {
	      /* Write the register and enable */
	      vmeWrite32(&(DCp[dcID[ii]]->ADR32M),
			((a32addr+DC_MAX_A32MB_SIZE)>>8) + (a32addr>>23) + DC_AMB_ENABLE);
	    }
	}    
      /* Set First Board and Last Board */
      dcMaxSlot = maxSlot;
      dcMinSlot = minSlot;
    if(!noBoardInit)
	{
	  vmeWrite32(&(DCp[minSlot]->ADR32M),
		    vmeRead32(&(DCp[minSlot]->ADR32M)) | DC_FIRST_BOARD);
	  vmeWrite32(&(DCp[maxSlot]->ADR32M),
		    vmeRead32(&(DCp[maxSlot]->ADR32M)) | DC_LAST_BOARD);
	}
skipmultiblock:
	;    
  }

  dcInited = ndc;
  if(errFlag > 0) 
  {
    printf("dcInit: ERROR: Unable to initialize all DC Modules\n");
    return(ERROR);
  } 
  else 
  {
    if(ndc > 0) printf("dcInit: %d DC(s) successfully initialized\n",ndc );
    return(ndc);
  }
}






int
dcHardReset(int id)
{
	int i;
	unsigned short reloadSequence[] = {
		0xFFFF, 0xAA99, 0x5566, 0x3261,
		0x0000, 0x3281, 0x0B00, 0x32A1,
		0x0000, 0x32C1, 0x0B00, 0x30A1,
		0x000E, 0x2000
	};

	vmeWrite32(&(DCp[id]->ICap), DC_ICAP_CE);
	vmeWrite32(&(DCp[id]->ICap), DC_ICAP_CE | DC_ICAP_CLK);
	for(i = 0; i < sizeof(reloadSequence)/sizeof(reloadSequence[0]); i++)
	{
		vmeWrite32(&(DCp[id]->ICap), reloadSequence[i]);
		vmeWrite32(&(DCp[id]->ICap), DC_ICAP_CLK | reloadSequence[i]);
	}
	for(i = 0; i < 10; i++)
	{
		vmeWrite32(&(DCp[id]->ICap), DC_ICAP_CE);
		vmeWrite32(&(DCp[id]->ICap), DC_ICAP_CE | DC_ICAP_CLK);
	}
	return(OK);
}

/*******************************************************************************
 *
 * dcSetClockSource - Set the clock source
 *
 *   This routine should be used in the case that the source clock
 *   is NOT set in dcInit (and defaults to Internal).  Such is the case
 *   when clocks are synchronized in a many crate system.  The clock source
 *   of the DC should ONLY be set AFTER those clocks have been set and
 *   synchronized.
 *
 *   clkSrc: 1 bit integer
 *       bit 0:  defines Clock Source
 *           0  Internal 250MHz Clock
 *           1  VXS (P0)
 *
 */

int
dcSetClockSource(int id, int clkSrc)
{
  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      printf("dcStatus: ERROR : DC in slot %d is not initialized \n",id);
      return(-1);
    }

  if(clkSrc>0x1)
    {
      printf("%s: ERROR: Invalid Clock Source specified (0x%x)\n",
	     __FUNCTION__,clkSrc);
      return ERROR;
    }

  /* Enable Clock source - Internal Clk enabled by default */ 
  vmeWrite32(&(DCp[dcID[id]]->ClockConfig),clkSrc | DC_REF_CLK_RESET);
  vmeWrite32(&(DCp[dcID[id]]->ClockConfig),clkSrc);
  taskDelay(20);

  switch(clkSrc)
    {
    case DC_REF_CLK_INTERNAL:
      printf("%s: DC id %d clock source set to INTERNAL\n",
	     __FUNCTION__,id);
      break;

    case DC_REF_CLK_P0:
      printf("%s: DC id %d clock source set to VXS (P0)\n",
	     __FUNCTION__,id);
      break;
    }

  return OK;
}

void
dcStatus(int id, int sflag)
{ 
  int ii;
  unsigned int vers, bid;
  unsigned int busyFifo, busyTrig, errorFifo;
  unsigned int adr32m, adr32;
  unsigned int lookBack, windowWidth, blockConfig, tdcConfig, testPulseConfig;
  unsigned int clkConfig, dacConfig, trigBusyThreshold, trigSrc;
  unsigned int intr, geo;
  unsigned int fifoWordCnt, fifoEventCnt, fifoBlockCnt;
  unsigned int intWordCnt, intEventCnt, intBlockCnt;
  unsigned int readoutCfg;
  unsigned int chDisable[3];

  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      printf("dcStatus: ERROR : DC in slot %d is not initialized \n",id);
      return;
    }

  DCLOCK;
  vers   = vmeRead32(&DCp[id]->FirmwareRev);
  bid    = vmeRead32(&DCp[id]->BoardID);
  busyFifo = vmeRead32(&DCp[id]->GrpBusyFifo);
  busyTrig = vmeRead32(&DCp[id]->GrpBusyTrig);
  errorFifo = vmeRead32(&DCp[id]->GrpErrorFifo);
  adr32m = vmeRead32(&DCp[id]->ADR32M);
  lookBack = vmeRead32(&DCp[id]->LookBack);
  windowWidth = vmeRead32(&DCp[id]->WindowWidth);
  blockConfig = vmeRead32(&DCp[id]->BlockConfig);
  tdcConfig = vmeRead32(&DCp[id]->TDCConfig);
  clkConfig = vmeRead32(&DCp[id]->ClockConfig);
  testPulseConfig = vmeRead32(&DCp[id]->TestPulseConfig);
  dacConfig = vmeRead32(&DCp[id]->DACConfig);
  trigBusyThreshold = vmeRead32(&DCp[id]->TriggerBusyThreshold);
  trigSrc = vmeRead32(&DCp[id]->TriggerSource);
  adr32 = vmeRead32(&DCp[id]->ADR32);
  intr = vmeRead32(&DCp[id]->Interrupt);
  geo = vmeRead32(&DCp[id]->Geo);
  fifoWordCnt = vmeRead32(&DCp[id]->FifoWordCnt);
  fifoEventCnt = vmeRead32(&DCp[id]->FifoEventCnt);
  fifoBlockCnt = vmeRead32(&DCp[id]->FifoBlockCnt);
  readoutCfg = vmeRead32(&DCp[id]->ReadoutConfig);
  intWordCnt = vmeRead32(&DCp[id]->IntWordCnt);
  intEventCnt = vmeRead32(&DCp[id]->IntEventCnt);
  intBlockCnt = vmeRead32(&DCp[id]->IntBlockCnt);
  
  chDisable[0] = vmeRead32(&DCp[id]->ChDisable[0]);
  chDisable[1] = vmeRead32(&DCp[id]->ChDisable[1]);
  chDisable[2] = vmeRead32(&DCp[id]->ChDisable[2]);
  DCUNLOCK;

#ifdef VXWORKS
  printf("\nSTATUS for DC in slot %d at base address 0x%x \n",
	 id, (UINT32) DCp[id]);
#else
  printf("\nSTATUS for DC in slot %d at VME (Local) base address 0x%x (0x%x)\n",
	 id, (UINT32) DCp[id] - dcA24Offset, (UINT32) DCp[id]);
#endif
  printf("---------------------------------------------------------------------- \n");

  printf(" Board Firmware Rev/ID = 0x%04x / 0x%08x\n", (vers)&0xffff, bid);
  if(adr32m&DC_AMB_ENABLE) 
    {
      printf(" Alternate VME Addressing: Multiblock Enabled\n");
      if(adr32&DC_A32_ENABLE)
	printf("   A32 Enabled at VME (Local) base 0x%08x (0x%08x)\n",(adr32&0xFF80)<<16,(UINT32) DCpd[id]);
      else
	printf("   A32 Disabled\n");
    
      printf("   Multiblock VME Address Range 0x%08x - 0x%08x\n",(adr32m&0x1FF)<<23,(adr32m&0x1FF0000)<<7);
    }
  else
    {
      printf(" Alternate VME Addressing: Multiblock Disabled\n");
      if(adr32&DC_A32_ENABLE)
	printf("   A32 Enabled at VME (Local) base 0x%08x (0x%08x)\n",(adr32&0xFF80)<<16,(UINT32) DCpd[id]);
      else
	printf("   A32 Disabled\n");
    }

  if(intr&DC_INT_ENABLE_MASK) 
    {
      printf("\n  Interrupts ENABLED: ");
      printf(" on Block Count(%d), Event Count(%d), Word Count(%d)", intBlockCnt, intEventCnt, intWordCnt);
      printf("\n");
      printf("  Interrupt Reg: 0x%08x\n",intr);
      printf("  VME INT Vector = 0x%x  Level = %d\n",(intr&DC_INT_VEC_MASK),((intr&DC_INT_LEVEL_MASK)>>8));
    }

  printf("\n Signal Sources: \n");
  if((clkConfig&DC_REF_CLK_MASK)==DC_REF_CLK_INTERNAL)
    {
      printf("   Ref Clock : Internal\n");
    }
  else if((clkConfig&DC_REF_CLK_MASK)==DC_REF_CLK_P0)
    {
      printf("   Ref Clock : VXS\n");
    }
  else
    {
      printf("   Ref Clock : %d (Undefined)\n",(clkConfig&DC_REF_CLK_MASK));
    }

  switch(trigSrc&DC_TRIG_MASK) 
    {
    case 0:
      printf("   Trig Src  : VME (Software)\n");
      break;
    case DC_TRIG_P0_SYNC:
      printf("   Trig Src  : VXS\n");
      break;
    case DC_TRIG_P0:
      printf("   Trig Src  : VXS-HighRes\n");
      break;
    }  

  switch(trigSrc&DC_SYNC_MASK) 
    {
    case 0:
      printf("   Sync Reset: VME (Software)\n");
      break;
    case DC_SYNC_P0:
      printf("   Sync Reset: VXS (Sync)\n");
      break;
    }  

  printf("\n Configuration: \n");

  if(readoutCfg&DC_ENABLE_BERR)
    printf("   Bus Error ENABLED\n");
  else
    printf("   Bus Error DISABLED\n");


  if(adr32m&DC_ENABLE_MULTIBLOCK) 
    {
	  if(adr32m&DC_FIRST_BOARD)
	    printf("   MultiBlock transfer ENABLED (First Board - token via VXS)\n");
	  else if(adr32m&DC_LAST_BOARD)
	    printf("   MultiBlock transfer ENABLED (Last Board  - token via VXS)\n");
	  else
	    printf("   MultiBlock transfer ENABLED (Token via VXS)\n");
	  /* #ifdef VERSION1 */
    } 
  else 
    {
      printf("   MultiBlock transfer DISABLED\n");
    }

  printf("\n TDC Processing Configuration: \n");
  printf("   Channel Disable Mask[31- 0] = 0x%08x\n",chDisable[0]);
  printf("   Channel Disable Mask[63-32] = 0x%08x\n",chDisable[1]);
  printf("   Channel Disable Mask[95-64] = 0x%08x\n",chDisable[2]);
  printf("   Lookback (PL)    = %d ns   Time Window (PTW) = %d ns\n",lookBack,windowWidth);
  printf("   Channel deadtime = %d ns\n", tdcConfig*8);

  printf("\n");
  printf("   Busy Fifo Mask  = 0x%06x\n", busyFifo);
  printf("   Busy Trig Mask  = 0x%06x\n", busyTrig);
  printf("   Error Fifo Mask = 0x%06x\n", errorFifo);

  printf("   Blocks in FIFO  = %u  (Block level = %d)\n", fifoBlockCnt, blockConfig & 0x7ff);
  printf("   Events in FIFO  = %u\n", fifoEventCnt);
  printf("   Words in FIFO   = %u = %u\n", fifoWordCnt);
  printf("\n");
}

void 
dcGStatus(int sflag)
{
  int ii;

  for (ii=0;ii<ndc;ii++) 
    {
      dcStatus(dcID[ii],sflag);
    }

}



/***********************
 *
 *  dcSetProcMode - Setup DC processing modes.
 *
 */
int
dcSetProcMode(int id, unsigned int lookBack, unsigned int windowWidth, unsigned int deadTime)
{
  
  int err=0;
  unsigned int ptw_last_adr, ptw_max_buf;


  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcSetProcMode: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }

	
	#define DC_MAX_LOOKBACK    65535
#define DC_MAX_WINDOW      65535

  /*Defaults */
  if((lookBack==0)||(lookBack>DC_MAX_LOOKBACK))  lookBack  = DC_DEFAULT_LOOKBACK;
  if((windowWidth==0)||(windowWidth>DC_MAX_WINDOW)) windowWidth = DC_DEFAULT_WINDOW;
  if((deadTime==0)||(deadTime>DC_MAX_DEADTIME)) deadTime = DC_MAX_DEADTIME;

  /* Consistancy check */
  if(windowWidth > lookBack) 
    {
      err++;
      printf("dcSetProcMode: ERROR: Window must be <= Latency\n"); 
    }

  DCLOCK;
  vmeWrite32(&DCp[id]->LookBack, lookBack);
  vmeWrite32(&DCp[id]->WindowWidth, windowWidth);
  vmeWrite32(&DCp[id]->TDCConfig, deadTime);
  DCUNLOCK;

  return(OK);
}

void
dcGSetProcMode(unsigned int lookBack, unsigned int windowWidth, unsigned int deadTime)
{
  int ii, res;

  for (ii=0;ii<ndc;ii++) {
    res = dcSetProcMode(dcID[ii],lookBack,windowWidth, deadTime);
    if(res<0) printf("ERROR: slot %d, in dcSetProcMode()\n",dcID[ii]);
  }
}

/**************************************************************************************
 *
 *  dcReadBlock - General Data readout routine
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
dcReadBlock(int id, volatile UINT32 *data, int nwrds, int rflag)
{
  int ii, blknum, evnum1;
  int stat, retVal, xferCount, rmode, async;
  int dCnt, berr=0;
  int dummy=0;
  volatile unsigned int *laddr;
  unsigned int bhead, ehead, val;
  unsigned int vmeAdr, csr;

  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcReadBlock: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }

  if(data==NULL) 
    {
      logMsg("dcReadBlock: ERROR: Invalid Destination address\n",0,0,0,0,0,0);
      return(ERROR);
    }

  if(nwrds <= 0) nwrds= (DC_MAX_TDC_CHANNELS*DC_MAX_DATA_PER_CHANNEL) + 8;
  rmode = rflag&0x0f;
  async = rflag&0x80;
  
  if(rmode >= 1) 
    { /* Block Transfers */
    
      /*Assume that the DMA programming is already setup. */
      /* Don't Bother checking if there is valid data - that should be done prior
	 to calling the read routine */

      /* Check for 8 byte boundary for address - insert dummy word (Slot 0 DC Dummy DATA)*/
      if((unsigned long) (data)&0x7) 
	{
#ifdef VXWORKS
	  *data = DC_DUMMY_DATA;
#else
	  *data = LSWAP(DC_DUMMY_DATA);
#endif
	  dummy = 1;
	  laddr = (data + 1);
	} 
      else 
	{
	  dummy = 0;
	  laddr = data;
	}

      DCLOCK;
      if(rmode == 2) 
	{ /* Multiblock Mode */
	  if((vmeRead32(&(DCp[id]->ADR32M))&DC_FIRST_BOARD)==0) 
	    {
	      logMsg("dcReadBlock: ERROR: DC in slot %d is not First Board\n",id,0,0,0,0,0);
	      DCUNLOCK;
	      return(ERROR);
	    }
	  vmeAdr = (unsigned int)(DCpmb) - dcA32Offset;
	}
      else
	{
	  vmeAdr = (unsigned int)(DCpd[id]) - dcA32Offset;
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
	  logMsg("dcReadBlock: ERROR in DMA transfer Initialization 0x%x\n",retVal,0,0,0,0,0);
	  DCUNLOCK;
	  return(retVal);
	}

      if(async) 
	{ /* Asynchonous mode - return immediately - don't wait for done!! */
	  DCUNLOCK;
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
	      DCUNLOCK;
	      return(xferCount); /* Return number of data words transfered */
	    }
	  else
	    {
#ifdef VXWORKS
	      xferCount = (/*nwrds -*/ (retVal>>2) + dummy);  /* Number of Longwords transfered */
#else
	      xferCount = ((retVal>>2) + dummy);  /* Number of Longwords transfered */
#endif
	      logMsg("dcReadBlock: DMA transfer terminated by unknown BUS Error (csr=0x%x xferCount=%d)\n",csr,xferCount,0,0,0,0);
	      DCUNLOCK;
	      return(xferCount);
	      /* 	return(ERROR); */
	    }
	} 
      else if (retVal == 0)
	{ /* Block Error finished without Bus Error */
#ifdef VXWORKS
	  logMsg("dcReadBlock: WARN: DMA transfer terminated by word count 0x%x\n",nwrds,0,0,0,0,0);
#else
	  logMsg("dcReadBlock: WARN: DMA transfer returned zero word count 0x%x\n",nwrds,0,0,0,0,0);
#endif
	  DCUNLOCK;
	  return(/*nwrds*/retVal>>2);
	} 
      else 
	{  /* Error in DMA */
#ifdef VXWORKS
	  logMsg("dcReadBlock: ERROR: sysVmeDmaDone returned an Error\n",0,0,0,0,0,0);
#else
	  logMsg("dcReadBlock: ERROR: vmeDmaDone returned an Error\n",0,0,0,0,0,0);
#endif
	  DCUNLOCK;
	  return(retVal>>2);
	}

    } 
  else 
    {  /*Programmed IO */

      /* Check if Bus Errors are enabled. If so then disable for Prog I/O reading */
      DCLOCK;
      berr = vmeRead32(&(DCp[id]->ReadoutConfig))&DC_ENABLE_BERR;
      if(berr)
	    vmeWrite32(&(DCp[id]->ReadoutConfig),vmeRead32(&(DCp[id]->ReadoutConfig)) & ~DC_ENABLE_BERR);

      dCnt = 0;
      /* Read Block Header - should be first word */
      bhead = (unsigned int) *DCpd[id]; 
#ifndef VXWORKS
      bhead = LSWAP(bhead);
#endif
      if((bhead&DC_DATA_TYPE_DEFINE)&&((bhead&DC_DATA_TYPE_MASK) == DC_DATA_BLOCK_HEADER)) {
	blknum = bhead&DC_DATA_BLKNUM_MASK;
	ehead = (unsigned int) *DCpd[id];
#ifndef VXWORKS
	ehead = LSWAP(ehead);
#endif
	evnum1 = ehead&DC_DATA_TRIGNUM_MASK;
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
	  if( vmeRead32(&(DCp[id]->FifoWordCnt)) == 0) 
	    {
	      logMsg("dcReadBlock: FIFO Empty (0x%08x)\n",bhead,0,0,0,0,0);
	      DCUNLOCK;
	      return(0);
	    } 
	  else 
	    {
	      logMsg("dcReadBlock: ERROR: Invalid Header Word 0x%08x\n",bhead,0,0,0,0,0);
	      DCUNLOCK;
	      return(ERROR);
	    }
	}

      ii=0;
      while(ii<nwrds) 
	{
	  val = (unsigned int) *DCpd[id];
	  data[ii+2] = val;
#ifndef VXWORKS
	  val = LSWAP(val);
#endif
	  if( (val&DC_DATA_TYPE_DEFINE) 
	      && ((val&DC_DATA_TYPE_MASK) == DC_DATA_BLOCK_TRAILER) )
	    break;
	  ii++;
	}
      ii++;
      dCnt += ii;


      if(berr)
	vmeWrite32(&(DCp[id]->ReadoutConfig),
		  vmeRead32(&(DCp[id]->ReadoutConfig)) | DC_ENABLE_BERR);

      DCUNLOCK;
      return(dCnt);
    }

  DCUNLOCK;
  return(OK);
}

int
dcReadBlockStatus(int id, volatile UINT32 *data, int nwrds, int rflag)
{

  int stat, retVal, xferCount, rmode, async;
  int dummy=0;
  unsigned int csr=0;
  
  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcReadBlockStatus: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }

  if(nwrds <= 0) nwrds= (DC_MAX_TDC_CHANNELS*DC_MAX_DATA_PER_CHANNEL) + 8;
  rmode = rflag&0x0f;
  async = rflag&0x80;

  /* Check for 8 byte boundary for address - insert dummy word (Slot 0 DC Dummy DATA)*/
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
  DCLOCK;
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
	  DCUNLOCK;
	  return(xferCount); /* Return number of data words transfered */
	}
      else
	{
	  xferCount = (/*nwrds -*/ (retVal>>2) + dummy);  /* Number of Longwords transfered */
	  logMsg("dcReadBlockStatus: DMA transfer terminated by unknown BUS Error (csr=0x%x nwrds=%d)\n",csr,xferCount,0,0,0,0);
	  DCUNLOCK;
	  return(ERROR);
	}
    } 
  else if (retVal == 0)
    { /* Block Error finished without Bus Error */
      logMsg("dcReadBlockStatus: WARN: DMA transfer terminated by word count 0x%x\n",nwrds,0,0,0,0,0);
      DCUNLOCK;
      return(/*nwrds*/retVal>>2);
    } 
  else 
    {  /* Error in DMA */
      logMsg("dcReadBlockStatus: ERROR: sysVmeDmaDone returned an Error\n",0,0,0,0,0,0);
      DCUNLOCK;
      return(retVal);
    }
  
}

int
dcPrintBlock(int id, int rflag)
{

  int ii, blknum, evnum1;
  int nwrds=32768, dCnt, berr=0;
  unsigned int data, bhead, ehead;

  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      printf("dcPrintEvent: ERROR : DC in slot %d is not initialized \n",id);
      return(ERROR);
    }

  /* Check if data available */
  DCLOCK;
  if(vmeRead32(&(DCp[id]->FifoWordCnt))==0) 
    {
      printf("dcPrintEvent: ERROR: FIFO Empty\n");
      DCUNLOCK;
      return(0);
    }

  /* Check if Bus Errors are enabled. If so then disable for reading */
  berr = vmeRead32(&(DCp[id]->ReadoutConfig))&DC_ENABLE_BERR;
  if(berr)
    vmeWrite32(&(DCp[id]->ReadoutConfig),vmeRead32(&(DCp[id]->ReadoutConfig)) & ~DC_ENABLE_BERR);

  dCnt = 0;
  /* Read Block Header - should be first word */
  bhead = (unsigned int) *DCpd[id];
#ifndef VXWORKS
  bhead = LSWAP(bhead);
#endif
  if( (bhead&DC_DATA_TYPE_DEFINE)&&((bhead&DC_DATA_TYPE_MASK) == DC_DATA_BLOCK_HEADER)) 
    {
      blknum = bhead&DC_DATA_BLKNUM_MASK;
      ehead = (unsigned int) *DCpd[id];
#ifndef VXWORKS
      ehead = LSWAP(ehead);
#endif
      evnum1 = ehead&DC_DATA_TRIGNUM_MASK;
      printf("%4d: ",dCnt+1); 
      dcDataDecode(bhead);
      dCnt++;
      printf("%4d: ",dCnt+1); 
      dcDataDecode(ehead);
      dCnt++;
    }
  else
    {
      /* We got bad data - Check if there is any data at all */
      if(vmeRead32(&(DCp[id]->FifoWordCnt))==0) 
	{
	  logMsg("dcPrintBlock: FIFO Empty (0x%08x)\n",bhead,0,0,0,0,0);
	  DCUNLOCK;
	  return(0);
	} 
      else 
	{
	  logMsg("dcPrintBlock: ERROR: Invalid Header Word 0x%08x\n",bhead,0,0,0,0,0);
	  DCUNLOCK;
	  return(ERROR);
	}
    }
  
  ii=0;
  while(ii<nwrds) 
    {
      data = (unsigned int) *DCpd[id];
#ifndef VXWORKS
      data = LSWAP(data);
#endif
      printf("%4d: ",dCnt+1+ii); 
      dcDataDecode(data);
      if((data&DC_DATA_TYPE_DEFINE)&&((data&DC_DATA_TYPE_MASK) == DC_DATA_BLOCK_TRAILER))
	break;
      if((data&DC_DATA_TYPE_DEFINE)&&((data&DC_DATA_TYPE_MASK) == DC_DATA_INVALID))
	break;
      ii++;
    }
  ii++;
  dCnt += ii;


  if(berr)
    vmeWrite32(&(DCp[id]->ReadoutConfig),
	      vmeRead32( &(DCp[id]->ReadoutConfig)) | DC_ENABLE_BERR );
  
  DCUNLOCK;
  return(dCnt);
  
}




/*****************************************************************************/


/* Reset Event Builder (FIFOs etc) */
void
dcClear(int id)
{
  if(id==0) id = dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcClear: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return;
    }

  DCLOCK;
  vmeWrite32(&(DCp[id]->Reset),0);
  DCUNLOCK;
}


void
dcReset(int id, int iFlag)
{
  unsigned int a32addr, addrMB;

  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcReset: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return;
    }

  DCLOCK;
  if(iFlag==0)
    {
      a32addr = vmeRead32(&(DCp[id]->ADR32));
      addrMB  = vmeRead32(&(DCp[id]->ADR32M));
    }

  dcHardReset(id);
  taskDelay(120);

  if(iFlag==0)
    {
      vmeWrite32(&(DCp[id]->ADR32),a32addr);
      vmeWrite32(&(DCp[id]->ADR32M),addrMB);
    }
  DCUNLOCK;
}

void
dcChanDisable(int id, unsigned int cmask0, unsigned int cmask1, unsigned int cmask2)
{

  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcChanDisable: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return;
    }

  DCLOCK;
  /* Write New Disable Mask */
  vmeWrite32(&(DCp[id]->ChDisable[0]), cmask0);
  vmeWrite32(&(DCp[id]->ChDisable[1]), cmask1);
  vmeWrite32(&(DCp[id]->ChDisable[2]), cmask2);
  DCUNLOCK;

}

void
dcTrig(int id)
{
  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcTrig: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return;
    }

  DCLOCK;
  vmeWrite32(&(DCp[id]->TriggerSource), vmeRead32(&(DCp[id]->TriggerSource)) | DC_TRIG_VME);
  DCUNLOCK;
}

void
dcGTrig()
{
  int ii;

  for(ii=0;ii<ndc;ii++)
    dcTrig(dcID[ii]);
}



void
dcSync(int id)
{
  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcSync: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return;
    }

  DCLOCK;
  vmeWrite32(&(DCp[id]->TriggerSource), vmeRead32(&(DCp[id]->TriggerSource)) | DC_SYNC_VME);
  DCUNLOCK;
}



/* Return Event/Block count for DC in slot id */
int
dcDready(int id, int dflag)
{
  unsigned int dcnt=0;

  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcDready: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }
  DCLOCK;
  if(dflag)
    dcnt = vmeRead32(&(DCp[id]->FifoBlockCnt));
  else
    dcnt = vmeRead32(&(DCp[id]->FifoEventCnt));
  DCUNLOCK;

  
  return(dcnt);
}

/* Return a Block Ready status for DC. If Block Level is =1 then return Event Ready status */
int
dcBready(int id)
{
  int stat=0;

  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcBready: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }
  
  DCLOCK;
  stat = vmeRead32(&(DCp[id]->FifoBlockCnt));
  DCUNLOCK;

  if(stat)
    return(1);
  else
    return(0);
}

unsigned int
dcGBready()
{
  int ii, id, stat=0;
  unsigned int dmask=0;
  
  DCLOCK;
  for(ii=0;ii<ndc;ii++) 
    {
      id = dcID[ii];
      stat = vmeRead32(&(DCp[id]->FifoBlockCnt));
 
      if(stat)
	dmask |= (1<<id);
    }
  DCUNLOCK;
  
  return(dmask);
}

/* return Scan mask for all initialized DCs */
unsigned int
dcScanMask()
{
  int idc, id, dmask=0;

  for(idc=0; idc<ndc; idc++)
    {
      id = dcID[idc];
      dmask |= (1<<id);
    }

  return(dmask);
}

/* if val>0 then set the busy level, if val=0 then read it back.
   if bflag>0 then force the module Busy */
int
dcBusyLevel(int id, unsigned int val, int bflag)
{
  unsigned int blreg=0;

  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcBusyLevel: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }
  
  /* if Val > 0 then set the Level else leave it alone*/
  DCLOCK;

  vmeWrite32(&(DCp[id]->BusyEventCnt), 0xFFFFFFFF);
  if(bflag)
    vmeWrite32(&(DCp[id]->BusyBlockCnt), 0xFFFFFFFF);
  else
    vmeWrite32(&(DCp[id]->BusyBlockCnt), 0);

  if(val) 
	vmeWrite32(&(DCp[id]->BusyWordCnt),val);
  else
    blreg = vmeRead32(&(DCp[id]->BusyWordCnt));

  DCUNLOCK;

  return(blreg);
}

int
dcBusy(int id)
{
  unsigned int blreg=0;
  unsigned int dreg=0;

  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcBusy: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }
  
  DCLOCK;
  blreg = vmeRead32(&(DCp[id]->BusyWordCnt));
  dreg  = vmeRead32(&(DCp[id]->FifoWordCnt));
  DCUNLOCK;

  if(dreg>=blreg)
    return(1);
  else
    return(0);
}


void
dcEnableSoftTrig(int id)
{

  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcEnableSoftTrig: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return;
    }
  
  /* Clear the source */
  DCLOCK;
  vmeWrite32(&(DCp[id]->TriggerSource),
	    vmeRead32(&(DCp[id]->TriggerSource)) & ~DC_TRIG_MASK );
  /* Set Source and Enable*/
  vmeWrite32(&(DCp[id]->TriggerSource),
	    vmeRead32(&(DCp[id]->TriggerSource)) | DC_TRIG_VME );
  DCUNLOCK;
}
void
dcGEnableSoftTrig()
{
  int ii, id;

  for(ii=0;ii<ndc;ii++) 
    {
      id = dcID[ii];
      dcEnableSoftTrig(id);
    }
}

void
dcEnableSoftSync(int id)
{

  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcEnableSoftSync: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return;
    }
  
  /* Clear the source */
  DCLOCK;
  vmeWrite32(&(DCp[id]->TriggerSource),
	    vmeRead32(&(DCp[id]->TriggerSource)) & ~DC_SYNC_MASK);
  /* Set Source and Enable*/
  vmeWrite32(&(DCp[id]->TriggerSource),
	    vmeRead32(&(DCp[id]->TriggerSource)) | DC_SYNC_VME);
  DCUNLOCK;
}

void
dcEnableBusError(int id)
{

  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcEnableBusError: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return;
    }
  
  DCLOCK;
  vmeWrite32(&(DCp[id]->ReadoutConfig),
	    vmeRead32(&(DCp[id]->ReadoutConfig)) | DC_ENABLE_BERR );
  DCUNLOCK;
}

void
dcGEnableBusError()
{
  int ii;

  DCLOCK;
  for(ii=0;ii<ndc;ii++) 
    {
      vmeWrite32(&(DCp[dcID[ii]]->ReadoutConfig),
		vmeRead32(&(DCp[dcID[ii]]->ReadoutConfig)) | DC_ENABLE_BERR );
    }
  DCUNLOCK;
}


void
dcDisableBusError(int id)
{

  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcDisableBusError: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return;
    }
  
  DCLOCK;
  vmeWrite32(&(DCp[id]->ReadoutConfig),
	    vmeRead32(&(DCp[id]->ReadoutConfig)) & ~DC_ENABLE_BERR );
  DCUNLOCK;

}


void
dcEnableMultiBlock()
{
  int ii, id;

  if((ndc <= 1) || (DCp[dcID[0]] == NULL)) 
    {
      logMsg("dcEnableMultiBlock: ERROR : Cannot Enable MultiBlock mode \n",0,0,0,0,0,0);
      return;
    }
  
  for(ii=0;ii<ndc;ii++) 
    {
      id = dcID[ii];
      DCLOCK;
      vmeWrite32(&(DCp[id]->ADR32M),
		vmeRead32(&(DCp[id]->ADR32M)) | DC_ENABLE_MULTIBLOCK );
      DCUNLOCK;
      dcDisableBusError(id);
      if(id == dcMinSlot) 
	{
	  DCLOCK;
	  vmeWrite32(&(DCp[id]->ADR32M),
		    vmeRead32(&(DCp[id]->ADR32M)) | DC_FIRST_BOARD );
	  DCUNLOCK;
	}
      if(id == dcMaxSlot) 
	{
	  DCLOCK;
	  vmeWrite32(&(DCp[id]->ADR32M),
		    vmeRead32(&(DCp[id]->ADR32M)) | DC_LAST_BOARD );
	  DCUNLOCK;
	  dcEnableBusError(id);   /* Enable Bus Error only on Last Board */
	}
    }

}

void
dcDisableMultiBlock()
{
  int ii;

  if((ndc <= 1) || (DCp[dcID[0]] == NULL)) 
    {
      logMsg("dcDisableMultiBlock: ERROR : Cannot Disable MultiBlock Mode\n",0,0,0,0,0,0);
      return;
    }
  
  DCLOCK;
  for(ii=0;ii<ndc;ii++)
    vmeWrite32(&(DCp[dcID[ii]]->ADR32M),
	      vmeRead32(&(DCp[dcID[ii]]->ADR32M)) & ~DC_ENABLE_MULTIBLOCK );
  DCUNLOCK;
}



int
dcSetBlockLevel(int id, int level)
{
  int rval;

  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcSetBlockLevel: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }
  
  if(level<=0) level = 1;
  DCLOCK;
  vmeWrite32(&(DCp[id]->BlockConfig), level);
  dcBlockLevel = level;
  rval = vmeRead32(&(DCp[id]->BlockConfig)) & DC_BLOCK_LEVEL_MASK;
  DCUNLOCK;

  return(rval);

}
void
dcGSetBlockLevel(int level)
{
  int ii;

  if(level<=0) level = 1;
  DCLOCK;
  for(ii=0;ii<ndc;ii++)
    vmeWrite32(&(DCp[dcID[ii]]->BlockConfig), level);
  DCUNLOCK;

  dcBlockLevel = level;
}

int
dcSetDAC(int id, unsigned int dvalue)
{
  if(id==0) id=dcID[0];
  
  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcSetDAC: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }
  
  if(dvalue>DC_MAX_DAC_VAL) 
    {
      logMsg("dcSetDAC: ERROR : DC value (%d) out of range (0-%d) \n",
	     dvalue,DC_MAX_DAC_VAL,0,0,0,0);
      return(ERROR);
    }
  
  DCLOCK;
  vmeWrite32(&DCp[id]->DACConfig, (dvalue * 93600) / 1000);
  DCUNLOCK;

  return(OK);
}

void
dcGSetDAC(unsigned int dvalue)
{
  int ii;
  for(ii=0;ii<ndc;ii++)
    dcSetDAC(dcID[ii], dvalue);
}

int
dcSetCalMask(int id, unsigned int dce_mask, unsigned fce_mask)
{
  if(id==0) id=dcID[0];
  
  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcSetCalMask: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }
  
  if((dce_mask>0x3f) || (fce_mask>0x3f))
    {
      logMsg("dcSetDAC: ERROR : DC dce_mask/fce_mask (%d/%d) out of range\n",
	     dce_mask,fce_mask,0,0,0,0);
      return(ERROR);
    }
  
  DCLOCK;
  vmeWrite32(&DCp[id]->TestPulseConfig, fce_mask | (dce_mask<<6));
  DCUNLOCK;

  return(OK);
}

void
dcGSetCalMask(unsigned int dce_mask, unsigned fce_mask)
{
  int ii;
  for(ii=0;ii<ndc;ii++)
    dcSetCalMask(dcID[ii], dce_mask,fce_mask);
}

void dcSelectSpi(int id,int sel)
{
	if(sel)
		vmeWrite32(&DCp[id]->SpiFlash, 0x0);
	else
		vmeWrite32(&DCp[id]->SpiFlash, 0x4);
}

unsigned char dcTransferSpi(int id,unsigned char data)
{
	int i;
	unsigned char rsp = 0;
	for(i = 0; i < 8; i++)
	{
		vmeWrite32(&DCp[id]->SpiFlash, ((data>>7)&0x1));	
		rsp = (rsp<<1) | vmeRead32(&DCp[id]->SpiFlash) & 0x1;
		vmeWrite32(&DCp[id]->SpiFlash, 0x2|((data>>7)&0x1));
		data<<=1;
	}
	return rsp;
}

void dcFlashGetId(int id,unsigned char *rsp)
{
	dcSelectSpi(id,1);
	dcTransferSpi(id,FLASH_CMD_GETID);
	rsp[0] = dcTransferSpi(id,0xFF);
	rsp[1] = dcTransferSpi(id,0xFF);
	rsp[2] = dcTransferSpi(id,0xFF);
	dcSelectSpi(id,0);
}

unsigned char dcFlashGetStatus(int id, unsigned char cmd)
{
	unsigned char rsp;
	
	dcSelectSpi(id,1);
	dcTransferSpi(id,cmd);
	rsp = dcTransferSpi(id,0xFF);
	dcSelectSpi(id,0);
	
	return rsp;
}

int dcFirmwareUpdateVerify(int id, const char *filename)
{
	int i, result;

  if(id==0) id=dcID[0];
  
  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      printf("dcFirmwareUpdateVerify: ERROR : DC in slot %d is not initialized \n",id);
      return(ERROR);
    }

	printf("Updating firmware...");
	result = dcFirmwareUpdate(id, filename);
	if(result != OK)
	{
		printf("failed.\n");
		return result;
	}
	else
		printf("succeeded.\n");
	
	printf("\nVerifying...");
	result = dcFirmwareVerify(id, filename);
	if(result != OK)
	{
		printf("failed.\n");
		return result;
	}
	else
		printf("ok.\n");

	dcHardReset(id);
	taskDelay(120);
		
	return OK;
}

void dcGFirmwareUpdateVerify(const char *filename)
{
	int ii;
	for(ii = 0; ii < ndc; ii++)
		dcFirmwareUpdateVerify(dcID[ii], filename);
}

int dcFirmwareUpdate(int id, const char *filename)
{
	FILE *f;
	int i, flashId = 0;
	unsigned int addr = 0, page = 0;
	unsigned char buf[1056], rspId[3];

  if(id==0) id=dcID[0];
  
  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      printf("dcFirmwareUpdate: ERROR : DC in slot %d is not initialized \n",id);
      return(ERROR);
    }
	
	dcSelectSpi(id,0);
	dcFlashGetId(id,rspId);
	
	printf("Flash: Mfg=0x%02X, Type=0x%02X, Capacity=0x%02X\n", rspId[0], rspId[1], rspId[2]);

	if( (rspId[0] == FLASH_MFG_WINBOND) &&
	    (rspId[1] == (FLASH_DEVID_W25Q64>>8)) &&
		(rspId[2] == (FLASH_DEVID_W25Q64&0xFF)) )
	{
		f = fopen(filename, "rb");
		if(!f)
		{
			printf("%s: ERROR: dcFirmwareUpdate invalid file %s\n", __FUNCTION__, filename);
			return ERROR;
		}
	
		memset(buf, 0xff, 256);
		while(fread(buf, 1, 256, f) > 0)
		{
		  if(!(addr % 65536))		/* sector erase*/
			{
				dcSelectSpi(id,1);
				dcTransferSpi(id,FLASH_CMD_WREN);	/* write enable*/
				dcSelectSpi(id,0);

				dcSelectSpi(id,1);
				dcTransferSpi(id,FLASH_CMD_ERASE64K);	/* 64k sector erase*/
				dcTransferSpi(id,(addr>>16)&0xFF);
				dcTransferSpi(id,(addr>>8)&0xFF);
				dcTransferSpi(id,(addr)&0xFF);
				dcSelectSpi(id,0);

				printf(".");
				i = 0;
				while(1)
				{
					if(!(dcFlashGetStatus(id, FLASH_CMD_GETSTATUS) & 0x1))
						break;
					taskDelay(1);
					if(i == 60+6)	/* 1000ms maximum sector erase time*/
					{
						fclose(f);
						printf("%s: ERROR: dcFirmwareUpdate failed to erase flash\n", __FUNCTION__);
						return ERROR;
					}
					i++;
				}
			}

			dcSelectSpi(id,1);
			dcTransferSpi(id,FLASH_CMD_WREN);	/* write enable*/
			dcSelectSpi(id,0);

			dcSelectSpi(id,1);
			dcTransferSpi(id,FLASH_CMD_WRPAGE);	/* write page*/
			dcTransferSpi(id,(addr>>16)&0xFF);
			dcTransferSpi(id,(addr>>8)&0xFF);
			dcTransferSpi(id,(addr)&0xFF);
			for(i = 0; i < 256; i++)
				dcTransferSpi(id,buf[i]);
			dcSelectSpi(id,0);

			i = 0;
			while(1)
			{
			  if(!(dcFlashGetStatus(id, FLASH_CMD_GETSTATUS) & 0x1))	/* no faster than 1us per call*/
					break;
			  if(i == 3000)	/* 3ms maximum page program time*/
				{
					fclose(f);
					printf("%s: ERROR: dcFirmwareUpdate failed to program flash\n", __FUNCTION__);
					return ERROR;
				}
				i++;
			}
			memset(buf, 0xff, 256);
			addr+= 256;
		}
		fclose(f);
	}
	else if( (rspId[0] == FLASH_MFG_ATMEL) &&
	         (rspId[1] == (FLASH_DEVID_AT45DB642D>>8)) &&
		     (rspId[2] == (FLASH_DEVID_AT45DB642D&0xFF)) )
	{
		f = fopen(filename, "rb");
		if(!f)
		{
			printf("%s: ERROR: dcFirmwareUpdate invalid file %s\n", __FUNCTION__, filename);
			return ERROR;
		}
	
		memset(buf, 0xff, 1056);
		while(fread(buf, 1, 1056, f) > 0)
		{
		  dcSelectSpi(id,1);	/* write buffer 1 */
			dcTransferSpi(id,FLASH_CMD_WRBUF1);
			dcTransferSpi(id,0x00);
			dcTransferSpi(id,0x00);
			dcTransferSpi(id,0x00);
			for(i = 0; i < 1056; i++)
				dcTransferSpi(id,buf[i]);
			dcSelectSpi(id,0);

			dcSelectSpi(id,1);	/* buffer 1 to flash w/page erase */
			dcTransferSpi(id,FLASH_CMD_PGBUF1ERASE);
			dcTransferSpi(id, (page>>5) & 0xFF);
			dcTransferSpi(id, (page<<3) & 0xFF);
			dcTransferSpi(id, 0x00);
			dcSelectSpi(id,0);
			
			i = 0;
			while(1)
			{
				if(dcFlashGetStatus(id, FLASH_CMD_GETSTATUS2) & 0x80)
					break;
				if(i == 40000)	/* 40ms maximum page program time */
				{
					fclose(f);
					printf("%s: ERROR: dcFirmwareUpdate failed to program flash\n", __FUNCTION__);
					return ERROR;
				}
				i++;
			}			
			memset(buf, 0xff, 1056);
			page++;
		}
		fclose(f);
	}
	else
	{
		printf("%s: ERROR: dcFirmwareUpdate failed to identify flash id (or device not supported)\n", __FUNCTION__);
		return ERROR;
	}
	return OK;
}

int dcFirmwareRead(int id, const char *filename)
{
	FILE *f;
	int i,len, flashId = 0;
	unsigned int addr = 0;
	unsigned char buf[256];
	unsigned char rspId[3];

  if(id==0) id=dcID[0];
  
  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      printf("dcFirmwareRead: ERROR : DC in slot %d is not initialized \n",id);
      return(ERROR);
    }
	
	dcSelectSpi(id,0);
	dcFlashGetId(id,rspId);
	
	printf("Flash: Mfg=0x%02X, Type=0x%02X, Capacity=0x%02X\n", rspId[0], rspId[1], rspId[2]);

	if( (rspId[0] == FLASH_MFG_WINBOND) &&
	    (rspId[1] == (FLASH_DEVID_W25Q64>>8)) &&
		(rspId[2] == (FLASH_DEVID_W25Q64&0xFF)) )
	{
		f = fopen(filename, "wb");
		if(!f)
		{
			printf("%s: ERROR: dcFirmwareRead invalid file %s\n", __FUNCTION__, filename);
			return ERROR;
		}
		
		dcSelectSpi(id,1);
		dcTransferSpi(id,FLASH_CMD_RD);	/* continuous array read */
		dcTransferSpi(id,(addr>>16)&0xFF);
		dcTransferSpi(id,(addr>>8)&0xFF);
		dcTransferSpi(id,(addr)&0xFF);
		
		for(i = 0; i < FLASH_BYTE_LENGTH; i++)
		{
			fputc(dcTransferSpi(id,0xFF), f);
			if(!(i% 65536))
			{
				printf(".");
				taskDelay(1);
			}
		}
			
		dcSelectSpi(id,0);
		fclose(f);
	}
	else if( (rspId[0] == FLASH_MFG_ATMEL) &&
	         (rspId[1] == (FLASH_DEVID_AT45DB642D>>8)) &&
		     (rspId[2] == (FLASH_DEVID_AT45DB642D&0xFF)) )
	{
		f = fopen(filename, "wb");
		if(!f)
		{
			printf("%s: ERROR: dcFirmwareRead invalid file %s\n", __FUNCTION__, filename);
			return ERROR;
		}
		
		dcSelectSpi(id,1);
		dcTransferSpi(id,FLASH_CMD_RD);	/* continuous array read */
		dcTransferSpi(id,(addr>>16)&0xFF);
		dcTransferSpi(id,(addr>>8)&0xFF);
		dcTransferSpi(id,(addr)&0xFF);
		
		for(i = 0; i < FLASH_BYTE_LENGTH; i++)
		{
			fputc(dcTransferSpi(id,0xFF), f);
			if(!(i% 65536))
			{
				printf(".");
				taskDelay(1);
			}
		}
			
		dcSelectSpi(id,0);
		fclose(f);
	}
	else
	{
		printf("%s: ERROR: dcFirmwareRead failed to identify flash id 0x%02X\n", __FUNCTION__, flashId);
		return ERROR;
	}
	return OK;
}

int dcFirmwareVerify(int id, const char *filename)
{
	FILE *f;
	int i,len, flashId = 0;
	unsigned int addr = 0;
	unsigned char buf[256];
	unsigned char rspId[3], val;
	
  if(id==0) id=dcID[0];
  
  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
    {
      logMsg("dcFirmwareVerify: ERROR : DC in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }
	
	dcSelectSpi(id,0);
	dcFlashGetId(id,rspId);
	
	printf("Flash: Mfg=0x%02X, Type=0x%02X, Capacity=0x%02X\n", rspId[0], rspId[1], rspId[2]);

	if( (rspId[0] == FLASH_MFG_WINBOND) &&
	    (rspId[1] == (FLASH_DEVID_W25Q64>>8)) &&
		(rspId[2] == (FLASH_DEVID_W25Q64&0xFF)) )
	{
		f = fopen(filename, "rb");
		if(!f)
		{
			printf("%s: ERROR: dcFirmwareVerify invalid file %s\n", __FUNCTION__, filename);
			return ERROR;
		}
		
		dcSelectSpi(id,1);
		dcTransferSpi(id,FLASH_CMD_RD);	/* continuous array read */
		dcTransferSpi(id,(addr>>16)&0xFF);
		dcTransferSpi(id,(addr>>8)&0xFF);
		dcTransferSpi(id,(addr)&0xFF);

		while((len = fread(buf, 1, 256, f)) > 0)
		{
			for(i = 0; i < len; i++)
			{
				val = dcTransferSpi(id,0xFF);
				if(buf[i] != val)
				{
					dcSelectSpi(id,0);
					fclose(f);					
					printf("%s: ERROR: dcFirmwareVerify failed verify at addess 0x%08X[%02X,%02X]\n", __FUNCTION__, addr+i, buf[i], val);
					return ERROR;
				}
			}
			addr+=256;
			if(!(addr & 0xFFFF))
				printf(".");
		}
		dcSelectSpi(id,0);
		fclose(f);
	}
	else if( (rspId[0] == FLASH_MFG_ATMEL) &&
	         (rspId[1] == (FLASH_DEVID_AT45DB642D>>8)) &&
		     (rspId[2] == (FLASH_DEVID_AT45DB642D&0xFF)) )
	{
		f = fopen(filename, "rb");
		if(!f)
		{
			printf("%s: ERROR: dcFirmwareVerify invalid file %s\n", __FUNCTION__, filename);
			return ERROR;
		}
		
		dcSelectSpi(id,1);
		dcTransferSpi(id,FLASH_CMD_RD);	/* continuous array read */
		dcTransferSpi(id,(addr>>16)&0xFF);
		dcTransferSpi(id,(addr>>8)&0xFF);
		dcTransferSpi(id,(addr)&0xFF);

		while((len = fread(buf, 1, 256, f)) > 0)
		{
			for(i = 0; i < len; i++)
			{
				val = dcTransferSpi(id,0xFF);
				if(buf[i] != val)
				{
					dcSelectSpi(id,0);
					fclose(f);					
					printf("%s: ERROR: dcFirmwareVerify failed verify at addess 0x%08X[%02X,%02X]\n", __FUNCTION__, addr+i, buf[i], val);
					return ERROR;
				}
			}
			addr+=256;
			if(!(addr & 0xFFFF))
				printf(".");
		}
		dcSelectSpi(id,0);
		fclose(f);
	}
	else
	{
		printf("%s: ERROR: dcFirmwareVerify failed to identify flash id 0x%02X\n", __FUNCTION__, flashId);
		return ERROR;
	}
	return OK;
}

void 
dcDataDecode(unsigned int data)
{
	printf("0x%08X", data);

	if(data & 0x80000000)
	{
		int type = data & 0x78000000;
		switch(type)
		{
			case DC_DATA_BLOCK_HEADER:
				printf(" {BLKHDR} SLOTID: %d", (data>>22)&0x1f);
				printf(" NEVENTS: %d", (data>>11)&0x7ff);
				printf(" BLOCK: %d\n", (data>>0)&0x7ff);				
				break;
			case DC_DATA_BLOCK_TRAILER:
				printf(" {BLKTLR} SLOTID: %d", (data>>22)&0x1f);
				printf(" NWORDS: %d\n", (data>>0)&0x3fffff);
				break;
			case DC_DATA_EVENT_HEADER:
				printf(" {EVTHDR} EVENT: %d\n", (data>>0)&0x7ffffff);
				break;
			case DC_DATA_TRIGGER_TIME:
				printf(" {TRGTIME}\n");
				break;
			case DC_DATA_TDCEVT:
				printf(" {TDCEVT} CH: %d", (data>>16)&0x7f);
				printf(" TIME: %dns\n", (data>>0)&0xFFFF);
				break;
			case DC_DATA_INVALID:
				printf(" {***DNV***}\n");
				break;
			case DC_DATA_FILLER:
				printf(" {FILLER}\n");
				break;
			default:
				printf(" {***DATATYPE ERROR***}\n");
				break;
		}
	}
	else
		printf("\n");


/*
  int i_print = 1;
  static unsigned int type_last = 15;
  static unsigned int time_last = 0;

  if( data & 0x80000000 )
    {
      fadc_data.new_type = 1;
      fadc_data.type = (data & 0x78000000) >> 27;
    }
  else
    {
      fadc_data.new_type = 0;
      fadc_data.type = type_last;
    }
        
  switch( fadc_data.type )
    {
    case 0:
      fadc_data.slot_id_hd = (data & 0x7C00000) >> 22;
      fadc_data.n_evts = (data & 0x3FF800) >> 11;
      fadc_data.blk_num = (data & 0x7FF);
      if( i_print ) 
	printf("%8X - BLOCK HEADER - slot = %d   n_evts = %d   n_blk = %d\n",
	       data, fadc_data.slot_id_hd, fadc_data.n_evts, fadc_data.blk_num);
      break;
    case 1:
      fadc_data.slot_id_tr = (data & 0x7C00000) >> 22;
      fadc_data.n_words = (data & 0x3FFFFF);
      if( i_print ) 
	printf("%8X - BLOCK TRAILER - slot = %d   n_words = %d\n",
	       data, fadc_data.slot_id_tr, fadc_data.n_words);
      break;
    case 2:
      if( fadc_data.new_type )
	{
	  fadc_data.evt_num_1 = (data & 0x7FFFFFF);
	  if( i_print ) 
	    printf("%8X - EVENT HEADER 1 - evt_num = %d\n", data, fadc_data.evt_num_1);
	}    
      else
	{
	  fadc_data.evt_num_2 = (data & 0x7FFFFFF);
	  if( i_print ) 
	    printf("%8X - EVENT HEADER 2 - evt_num = %d\n", data, fadc_data.evt_num_2);
	}
      break;
    case 3:
      if( fadc_data.new_type )
	{
	  fadc_data.time_1 = (data & 0xFFFFFF);
	  if( i_print ) 
	    printf("%8X - TRIGGER TIME 1 - time = %08x\n", data, fadc_data.time_1);
	  fadc_data.time_now = 1;
	  time_last = 1;
	}    
      else
	{
	  if( time_last == 1 )
	    {
	      fadc_data.time_2 = (data & 0xFFFFFF);
	      if( i_print ) 
		printf("%8X - TRIGGER TIME 2 - time = %08x\n", data, fadc_data.time_2);
	      fadc_data.time_now = 2;
	    }    
	  else if( time_last == 2 )
	    {
	      fadc_data.time_3 = (data & 0xFFFFFF);
	      if( i_print ) 
		printf("%8X - TRIGGER TIME 3 - time = %08x\n", data, fadc_data.time_3);
	      fadc_data.time_now = 3;
	    }    
	  else if( time_last == 3 )
	    {
	      fadc_data.time_4 = (data & 0xFFFFFF);
	      if( i_print ) 
		printf("%8X - TRIGGER TIME 4 - time = %08x\n", data, fadc_data.time_4);
	      fadc_data.time_now = 4;
	    }    
	  else
	    if( i_print ) 
	      printf("%8X - TRIGGER TIME - (ERROR)\n", data);
	                
	  time_last = fadc_data.time_now;
	}    
      break;
    case 4:
      if( fadc_data.new_type )
	{
	  fadc_data.chan = (data & 0x7800000) >> 23;
	  fadc_data.width = (data & 0xFFF);
	  if( i_print ) 
	    printf("%8X - WINDOW RAW DATA - chan = %d   nsamples = %d\n", 
		   data, fadc_data.chan, fadc_data.width);
	}    
      else
	{
	  fadc_data.valid_1 = 1;
	  fadc_data.valid_2 = 1;
	  fadc_data.adc_1 = (data & 0x1FFF0000) >> 16;
	  if( data & 0x20000000 )
	    fadc_data.valid_1 = 0;
	  fadc_data.adc_2 = (data & 0x1FFF);
	  if( data & 0x2000 )
	    fadc_data.valid_2 = 0;
	  if( i_print ) 
	    printf("%8X - RAW SAMPLES - valid = %d  adc = %4d   valid = %d  adc = %4d\n", 
		   data, fadc_data.valid_1, fadc_data.adc_1, 
		   fadc_data.valid_2, fadc_data.adc_2);
	}    
      break;
    case 5:
      fadc_data.over = 0; 
      fadc_data.chan = (data & 0x7800000) >> 23;
      fadc_data.adc_sum = (data & 0x3FFFFF);
      if( data & 0x400000 )
	fadc_data.over = 1;
      if( i_print ) 
	printf("%8X - WINDOW SUM - chan = %d   over = %d   adc_sum = %08x\n",
	       data, fadc_data.chan, fadc_data.over, fadc_data.adc_sum);
      break;
    case 6:
      if( fadc_data.new_type )
	{
	  fadc_data.chan = (data & 0x7800000) >> 23;
	  fadc_data.pulse_num = (data & 0x600000) >> 21;
	  fadc_data.thres_bin = (data & 0x3FF);
	  if( i_print ) 
	    printf("%8X - PULSE RAW DATA - chan = %d   pulse # = %d   threshold bin = %d\n", 
		   data, fadc_data.chan, fadc_data.pulse_num, fadc_data.thres_bin);
	}    
      else
	{
	  fadc_data.valid_1 = 1;
	  fadc_data.valid_2 = 1;
	  fadc_data.adc_1 = (data & 0x1FFF0000) >> 16;
	  if( data & 0x20000000 )
	    fadc_data.valid_1 = 0;
	  fadc_data.adc_2 = (data & 0x1FFF);
	  if( data & 0x2000 )
	    fadc_data.valid_2 = 0;
	  if( i_print ) 
	    printf("%8X - PULSE RAW SAMPLES - valid = %d  adc = %d   valid = %d  adc = %d\n", 
		   data, fadc_data.valid_1, fadc_data.adc_1, 
		   fadc_data.valid_2, fadc_data.adc_2);
	}    
      break;
    case 7:
      fadc_data.chan = (data & 0x7800000) >> 23;
      fadc_data.pulse_num = (data & 0x600000) >> 21;
      fadc_data.quality = (data & 0x180000) >> 19;
      fadc_data.integral = (data & 0x7FFFF);
      if( i_print ) 
	printf("%8X - PULSE INTEGRAL - chan = %d   pulse # = %d   quality = %d   integral = %d\n", 
	       data, fadc_data.chan, fadc_data.pulse_num, 
	       fadc_data.quality, fadc_data.integral);
      break;
    case 8:
      fadc_data.chan = (data & 0x7800000) >> 23;
      fadc_data.pulse_num = (data & 0x600000) >> 21;
      fadc_data.quality = (data & 0x180000) >> 19;
      fadc_data.time = (data & 0xFFFF);
      if( i_print ) 
	printf("%8X - PULSE TIME - chan = %d   pulse # = %d   quality = %d   time = %d\n", 
	       data, fadc_data.chan, fadc_data.pulse_num, 
	       fadc_data.quality, fadc_data.time);
      break;
    case 9:
      if( fadc_data.new_type )
	{
	  fadc_data.chan_a = (data & 0x3C00000) >> 22;
	  fadc_data.source_a = (data & 0x4000000) >> 26;
	  fadc_data.chan_b = (data & 0x1E0000) >> 17;
	  fadc_data.source_b = (data & 0x200000) >> 21;
	  if( i_print ) 
	    printf("%8X - STREAMING RAW DATA - ena A = %d  chan A = %d   ena B = %d  chan B = %d\n", 
		   data, fadc_data.source_a, fadc_data.chan_a, 
		   fadc_data.source_b, fadc_data.chan_b);
	}    
      else
	{
	  fadc_data.valid_1 = 1;
	  fadc_data.valid_2 = 1;
	  fadc_data.adc_1 = (data & 0x1FFF0000) >> 16;
	  if( data & 0x20000000 )
	    fadc_data.valid_1 = 0;
	  fadc_data.adc_2 = (data & 0x1FFF);
	  if( data & 0x2000 )
	    fadc_data.valid_2 = 0;
	  fadc_data.group = (data & 0x40000000) >> 30;
	  if( fadc_data.group )
	    {
	      if( i_print ) 
		printf("%8X - RAW SAMPLES B - valid = %d  adc = %d   valid = %d  adc = %d\n", 
		       data, fadc_data.valid_1, fadc_data.adc_1, 
		       fadc_data.valid_2, fadc_data.adc_2);
	    }		 
	  else
	    if( i_print ) 
	      printf("%8X - RAW SAMPLES A - valid = %d  adc = %d   valid = %d  adc = %d\n", 
		     data, fadc_data.valid_1, fadc_data.adc_1, 
		     fadc_data.valid_2, fadc_data.adc_2);	            
	}    
      break;
    case 10:
      fadc_data.chan = (data & 0x7800000) >> 23;
      fadc_data.pulse_num = (data & 0x600000) >> 21;
      fadc_data.vmin = (data & 0x1FF000) >> 12;
      fadc_data.vpeak = (data & 0xFFF);
      if( i_print ) 
	printf("%8X - PULSE V - chan = %d   pulse # = %d   vmin = %d   vpeak = %d\n", 
	       data, fadc_data.chan, fadc_data.pulse_num, 
	       fadc_data.vmin, fadc_data.vpeak);
      break;

    case 11:
      fadc_data.trig_type_int = data & 0x7;
      fadc_data.trig_state_int = (data & 0x8) >> 3;
      fadc_data.evt_num_int = (data & 0xFFF0) >> 4;
      fadc_data.err_status_int = (data & 0x10000) >> 16;
      if( i_print ) 
	printf("%8X - INTERNAL TRIGGER - type = %d   state = %d   num = %d   error = %d\n",
	       data, fadc_data.trig_type_int, fadc_data.trig_state_int, fadc_data.evt_num_int,
	       fadc_data.err_status_int);
    case 12:
      if( i_print ) 
	printf("%8X - UNDEFINED TYPE = %d\n", data, fadc_data.type);
      break;
    case 13:
      if( i_print ) 
	printf("%8X - END OF EVENT = %d\n", data, fadc_data.type);
      break;
    case 14:
      if( i_print ) 
	printf("%8X - DATA NOT VALID = %d\n", data, fadc_data.type);
      break;
    case 15:
      if( i_print ) 
	printf("%8X - FILLER WORD = %d\n", data, fadc_data.type);
      break;
    }
	
  type_last = fadc_data.type;
*/

  return;
}


void
dcSoftTrig(int id)
{
  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
  {
    printf("dcSoftTrig: ERROR : DC in slot %d is not initialized \n",id);
    return;
  }

  vmeWrite32(&(DCp[id]->TriggerSource),DC_TRIG_VME);

}


void
dcPrintScalers(int id)
{
  unsigned int scaler[6];

  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
  {
    printf("dcPrintScalers: ERROR : DC in slot %d is not initialized \n",id);
    return;
  }

  vmeWrite32(&(DCp[id]->ScalerLatch), 0xFFFFFFFF);

  scaler[0] = vmeRead32(&(DCp[id]->ScalerBusy));
  scaler[1] = vmeRead32(&(DCp[id]->ScalerBusyCycles));
  scaler[2] = vmeRead32(&(DCp[id]->ScalerVmeClk));
  scaler[3] = vmeRead32(&(DCp[id]->ScalerSync));
  scaler[4] = vmeRead32(&(DCp[id]->ScalerTrig1));
  scaler[5] = vmeRead32(&(DCp[id]->ScalerTrig2));

  printf("\n Slot %d scalers\n",id);

  printf("ScalerBusy       %10u\n",scaler[0]);
  printf("ScalerBusyCycles %10u\n",scaler[1]);
  printf("ScalerVmeClk     %10u\n",scaler[2]);
  printf("ScalerSync       %10u\n",scaler[3]);
  printf("ScalerTrig1      %10u\n",scaler[4]);
  printf("ScalerTrig2      %10u\n",scaler[5]);

  printf("ScalerTDCs      %10u %10u %10u\n",
		 vmeRead32(&(DCp[id]->ScalerTDC[0])),
         vmeRead32(&(DCp[id]->ScalerTDC[1])),
         vmeRead32(&(DCp[id]->ScalerTDC[2])) );

  printf("\n Slot %d FIFO: nevents=%u nwords=%u\n",id,vmeRead32(&(DCp[id]->FifoEventCnt)),vmeRead32(&(DCp[id]->FifoWordCnt)));
}


/* width in ns */
int
dcTriggerPulseWidth(int id, unsigned int width)
{
  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
  {
    printf("dcSoftTrig: ERROR : DC in slot %d is not initialized \n",id);
    return(-1);
  }

  width /= 8;

  vmeWrite32(&(DCp[id]->TriggerCtrl),width);

  return(width);
}


/* returns 1 if link is UP, 0, otherwise */
int
dcLinkStatus(int id)
{
  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
  {
    printf("dcSoftTrig: ERROR : DC in slot %d is not initialized \n",id);
    return(-1);
  }

  return( ( (vmeRead32(&(DCp[id]->GtpStatus)))&0x1000 )>>12 );
}

int
dcLinkReset(int id)
{
  if(id==0) id=dcID[0];

  if((id<=0) || (id>21) || (DCp[id] == NULL)) 
  {
    printf("dcSoftTrig: ERROR : DC in slot %d is not initialized \n",id);
    return(-1);
  }

  vmeWrite32(&(DCp[id]->GtpCtrl), 0x203);
  taskDelay(1);
  vmeWrite32(&(DCp[id]->GtpCtrl), 0x202);
  taskDelay(1);
  vmeWrite32(&(DCp[id]->GtpCtrl), 0x200);
  taskDelay(1);
  vmeWrite32(&(DCp[id]->GtpCtrl), 0);
  taskDelay(10);


  return(0);
}




#else /* dummy version*/

void
dcLib_dummy()
{
  return;
}

#endif
