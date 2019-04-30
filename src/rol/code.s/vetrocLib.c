/******************************************************************************
*
*  vetrocLib.h  - Driver library header file for readout of the vetroc boards
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


/* Include VETROC definitions */
#include "vetrocLib.h"

#ifdef VXWORKS
#define VETROCLOCK
#define VETROCUNLOCK
#else
/* Mutex to guard flexio read/writes */
pthread_mutex_t   vetrocMutex = PTHREAD_MUTEX_INITIALIZER;
#define VETROCLOCK      if(pthread_mutex_lock(&vetrocMutex)<0) perror("pthread_mutex_lock");
#define VETROCUNLOCK    if(pthread_mutex_unlock(&vetrocMutex)<0) perror("pthread_mutex_unlock");
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
BOOL              vetrocIntRunning  = FALSE;                    /* running flag */
int               vetrocIntID       = -1;                       /* id number of VETROC generating interrupts */
LOCAL VOIDFUNCPTR vetrocIntRoutine  = NULL;                     /* user interrupt service routine */
LOCAL int         vetrocIntArg      = 0;                        /* arg to user routine */
LOCAL UINT32      vetrocIntLevel    = VETROC_VME_INT_LEVEL;         /* default VME interrupt level */
LOCAL UINT32      vetrocIntVec      = VETROC_VME_INT_VEC;           /* default interrupt Vector */

/* Define global variables */
int nvetroc = 0;                                       /* Number of VETROCs in Crate */
int vetrocA32Base   = 0x09000000;                      /* Minimum VME A32 Address for use by VETROCs */
int vetrocA32Offset = 0x08000000;                      /* Difference in CPU A32 Base - VME A32 Base */
int vetrocA24Offset = 0x0;                             /* Difference in CPU A24 Base - VME A24 Base */
volatile vetroc_regs *VETROCp[(VETROC_MAX_BOARDS+1)]; /* pointers to VETROC memory map */
volatile unsigned int *VETROCpd[(VETROC_MAX_BOARDS+1)];      /* pointers to VETROC FIFO memory */
volatile unsigned int *VETROCpmb;                        /* pointer to Multblock window */
int vetrocID[VETROC_MAX_BOARDS];                           /* array of slot numbers for VETROCs */
unsigned int vetrocAddrList[VETROC_MAX_BOARDS];            /* array of a24 addresses for VETROCs */
int vetrocRev[(VETROC_MAX_BOARDS+1)];                      /* Board Revision Info for each module */
int vetrocInited=0;                                    /* >0 if Library has been Initialized before */
int vetrocMaxSlot=0;                                   /* Highest Slot hold an VETROC */
int vetrocMinSlot=0;                                   /* Lowest Slot holding an VETROC */
int vetrocSource=0;                                    /* Signal source for VETROC system control*/
int vetrocBlockLevel=0;                                /* Block Level for VETROCs */
int vetrocIntCount = 0;                                /* Count of interrupts from VETROC */

int
vetrocSlot(unsigned int id)
{
  if(id>=nvetroc)
  {
    printf("%s: ERROR: Index (%d) >= VETROCs initialized (%d).\n",__FUNCTION__,id,nvetroc);
    return(-1);
  }

  return(vetrocID[id]);
}

int
vetrocId(unsigned int slot)
{
  int id;

  for(id=0; id<nvetroc; id++)
  {
    if(vetrocID[id]==slot)
	{
      return(id);
	}
  }

  printf("%s: ERROR: VETROC in slot %d does not exist or not initialized.\n",__FUNCTION__,slot);
  return(-1);
}

/*******************************************************************************
 *
 * vetrocInit - Initialize JLAB VETROC Library. 
 *
 *
 *   iFlag: 16 bit integer
 *       Low 6 bits - Specifies the default Signal distribution (clock,trigger) 
 *                    sources for the board (INTernal, FrontPanel, VXS, VME(Soft))
 *       bit    0-3:  defines Sync Reset source
 *                     0  VME (Software Sync-Reset)
 *                     1  VXS (P0)
 *       bit    4-7:  defines Trigger source
 *                     0  VME (Software Triggers)
 *                     1  VXS (P0) 
 *                     2  FP Input 1
 *       bits   8:  defines Clock Source
 *                     0  Internal 250MHz Clock
 *                     1  VXS (P0)
 *
  *      bit 16:  Exit before board initialization
 *             0 Initialize VETROC (default behavior)
 *             1 Skip initialization (just setup register map pointers)
 *
 *      bit 17:  Use vetrocAddrList instead of addr and addr_inc
 *               for VME addresses.
 *             0 Initialize with addr and addr_inc
 *             1 Use vetrocAddrList 
 *      
 *
 * RETURNS: nvetroc, or 0 if no boards found, or ERROR
 */

int 
vetrocInit(UINT32 addr, UINT32 addr_inc, int ndc, int iFlag)
{
  int ii, res, errFlag = 0;
  int boardID = 0;
  int maxSlot = 1;
  int minSlot = 21;
  int syncSrc=0, trigSrc=0, clkSrc=0, srSrc=0;
  unsigned int rdata, laddr, laddr_inc, a32addr;
  volatile vetroc_regs *vetroc;
  unsigned short sdata;
  int noBoardInit=0;
  int useList=0;

  /* Check if we have already Initialized boards before */
  if((vetrocInited>0) && (vetrocID[0] != 0)) 
  {
    /* Hard Reset of all VETROC boards in the Crate
    for(ii=0;ii<nvetroc;ii++) 
	{
	  vetrocHardReset(vetrocID[ii]);
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
      printf("vetrocInit: ERROR: Must specify a Bus (VME-based A24) address for VETROC 0\n");
      return(ERROR);
    }
  else if(addr > 0x00ffffff) 
    { /* A24 Addressing */
      printf("vetrocInit: ERROR: A32 Addressing not allowed for VETROC configuration space\n");
      return(ERROR);
    }
  else
    { /* A24 Addressing */
      if( ((addr_inc==0)||(ndc==0)) && (useList==0) )
	ndc = 1; /* assume only one VETROC to initialize */

      /* get the VETROC address */
#ifdef VXWORKS
      res = sysBusToLocalAdrs(0x39,(char *)addr,(char **)&laddr);
#else
      res = vmeBusToLocalAdrs(0x39,(char *)addr,(char **)&laddr);
#endif
      if (res != 0) 
	{
#ifdef VXWORKS
	  printf("vetrocInit: ERROR in sysBusToLocalAdrs(0x39,0x%x,&laddr) \n",addr);
#else
	  printf("vetrocInit: ERROR in vmeBusToLocalAdrs(0x39,0x%x,&laddr) \n",addr);
#endif
	  return(ERROR);
	}
      vetrocA24Offset = laddr - addr;
    }

  /* Init Some Global variables */
  vetrocSource = iFlag&VETROC_SOURCE_MASK;
  vetrocInited = nvetroc = 0;
  bzero((char *)vetrocID,sizeof(vetrocID));

  printf("ndc=%d\n",ndc);fflush(stdout);
  for (ii=0; ii<ndc; ii++) 
  {
      if(useList==1)
	{
	  laddr_inc = vetrocAddrList[ii] + vetrocA24Offset;
	}
      else
	{
	  laddr_inc = laddr +ii*addr_inc;
	}
      vetroc = (vetroc_regs *)laddr_inc;
      /* Check if Board exists at that address */
#ifdef VXWORKS
      res = vxMemProbe((char *) &(vetroc->Clk.BoardId),VX_READ,4,(char *)&rdata);
#else
      res = vmeMemProbe((char *) &(vetroc->Clk.BoardId),4,(char *)&rdata);
#endif
      if(res < 0) 
	{
      printf("NO ADDRESSABLE BOARD !!!\n");fflush(stdout);
#ifdef VXWORKS
	  printf("vetrocInit: ERROR: No addressable board at addr=0x%x\n",(UINT32) vetroc);
#else
	  printf("vetrocInit: ERROR: No addressable board at VME (Local) addr=0x%x (0x%x)\n",
		 (UINT32) laddr_inc-vetrocA24Offset, (UINT32) vetroc);
#endif
	  /*errFlag = 1;*/
/* 	  break; */
	}
      else 
	{
	  /* Check that it is an VETROC board */
	  if((rdata&VETROC_BOARD_MASK) != VETROC_BOARD_ID) 
	  {
	    printf(" ERROR: For board at 0x%x, Invalid Board ID: 0x%x\n",
		     (UINT32) vetroc, rdata);
        continue;
	  }
	  /* Check if this is board has a valid slot number */
	  boardID =  ((vmeRead32(&(vetroc->Clk.FirmwareRev)))&VETROC_SLOT_ID_MASK)>>VETROC_SLOT_ID_SHIFT;
	  if((boardID <= 0)||(boardID >21)) 
	  {
	    printf(" ERROR: Board Slot ID is not in range: %d\n",boardID);
	    continue;
	  }
	  VETROCp[boardID] = (vetroc_regs *)(laddr_inc);
	  vetrocRev[boardID] = vmeRead32(&(vetroc->Clk.FirmwareRev)) & VETROC_VERSION_MASK;
/* 	} */
	  vetrocID[nvetroc] = boardID;
	  if(boardID >= maxSlot) maxSlot = boardID;
	  if(boardID <= minSlot) minSlot = boardID;
	  
	  printf("Initialized VETROC %2d  Slot # %2d at address 0x%08x (0x%08x) \n",
		 nvetroc,vetrocID[nvetroc],(UINT32) VETROCp[(vetrocID[nvetroc])],
		 (UINT32) VETROCp[(vetrocID[nvetroc])]-vetrocA24Offset);
	  nvetroc++;
/* 	  printf("Initialized VETROC %2d  Slot # %2d at address 0x%08x \n", */
/* 		 ii,vetrocID[ii],(UINT32) VETROCp[(vetrocID[ii])]); */
	}
  }
  printf("Finished boards initialization, nvetroc=%d\n",nvetroc);fflush(stdout);


  /* Check if we are to exit when pointers are setup */
  noBoardInit=iFlag&(1<<16);

  /* Hard Reset of all VETROC boards in the Crate */
/*
  for(ii=0;ii<nvetroc;ii++) 
  {
    if(!noBoardInit)
	{
      printf("HardReseting slot %d ..\n",vetrocID[ii]);fflush(stdout);
      vetrocHardReset(vetrocID[ii]);
      printf(".. done.\n");fflush(stdout);
	}
  }
  printf("Finished hard reset\n");fflush(stdout);
  taskDelay(120); 
*/

  /* Initialize Interrupt variables */
  vetrocIntID = -1;
  vetrocIntRunning = FALSE;
  vetrocIntLevel = VETROC_VME_INT_LEVEL;
  vetrocIntVec = VETROC_VME_INT_VEC;
  vetrocIntRoutine = NULL;
  vetrocIntArg = 0;

  /* Calculate the A32 Offset for use in Block Transfers */
#ifdef VXWORKS
  res = sysBusToLocalAdrs(0x09,(char *)vetrocA32Base,(char **)&laddr);
  if (res != 0) 
    {
      printf("vetrocInit: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",vetrocA32Base);
      return(ERROR);
    } 
  else 
    {
      vetrocA32Offset = laddr - vetrocA32Base;
    }
#else
  res = vmeBusToLocalAdrs(0x09,(char *)vetrocA32Base,(char **)&laddr);
  if (res != 0) 
    {
      printf("vetrocInit: ERROR in vmeBusToLocalAdrs(0x09,0x%x,&laddr) \n",vetrocA32Base);fflush(stdout);
      return(ERROR);
    } 
  else 
    {
      vetrocA32Offset = laddr - vetrocA32Base;
    }
#endif

    printf("vetrocInit: ");
	 
    if(!noBoardInit)
    {
      /* Setup Sync Source */
      switch(iFlag & VETROC_SYNC_SOURCE_MASK)
		{
        case VETROC_SYNC_SOURCE_SW:
          printf("syncSrc=SW ");
          syncSrc = VETROC_SD_MUX_0;
          break;
		  case VETROC_SYNC_SOURCE_VXS:
          printf("syncSrc=VXS ");
          syncSrc = VETROC_SD_MUX_SYNCFI;
          break;
		}
		
      /* Setup Trig Source */
      switch(iFlag & VETROC_TRIG_SOURCE_MASK)
		{
        case VETROC_TRIG_SOURCE_SW:
          printf("trigSrc=SW ");
          trigSrc = VETROC_SD_MUX_0;
          break;
		  case VETROC_TRIG_SOURCE_VXS:
          printf("trigSrc=VXS ");
          trigSrc = VETROC_SD_MUX_TRIG1F;
          break;
		  case VETROC_TRIG_SOURCE_FP1:
          printf("trigSrc=FP1 ");
          trigSrc = VETROC_SD_MUX_FPIN1;
          break;
		}
		
      /* Setup Clock Source */
      switch(iFlag & VETROC_CLK_SOURCE_MASK)
		{
        case VETROC_CLK_SOURCE_INT:
          printf("clkSrc=LOCAL ");
          clkSrc = (VETROC_CLK_SRC_LOCAL<<VETROC_CLK_CTRL_REFA_SHIFT) |
                   (VETROC_CLK_SRC_LOCAL<<VETROC_CLK_CTRL_REFB_SHIFT) |
                   (VETROC_CLK_SRC_LOCAL<<VETROC_CLK_CTRL_FPGA_SHIFT) |
                   (VETROC_CLK_SRC_LOCAL<<VETROC_CLK_CTRL_TD_SHIFT);
          break;
		  case VETROC_CLK_SOURCE_VXS:
          printf("clkSrc=VXS ");
          clkSrc = (VETROC_CLK_SRC_VXS<<VETROC_CLK_CTRL_REFA_SHIFT) |
                   (VETROC_CLK_SRC_VXS<<VETROC_CLK_CTRL_REFB_SHIFT) |
                   (VETROC_CLK_SRC_VXS<<VETROC_CLK_CTRL_FPGA_SHIFT) |
                   (VETROC_CLK_SRC_VXS<<VETROC_CLK_CTRL_TD_SHIFT);
          break;
		}
    }

  /* Enable Clock source - Internal Clk enabled by default */ 
  for(ii=0; ii<nvetroc; ii++) 
  {
    vmeWrite32(&(VETROCp[vetrocID[ii]]->Clk.Ctrl),clkSrc | VETROC_REF_CLK_RESET);
    vmeWrite32(&(VETROCp[vetrocID[ii]]->Clk.Ctrl),clkSrc);
  }
  taskDelay(1);

  /* Hard Reset FPGAs and FIFOs */
  for(ii=0;ii<nvetroc;ii++)
  {
    vmeWrite32(&(VETROCp[vetrocID[ii]]->Clk.Reset),VETROC_CLK_SOFT_RESET);
	 taskDelay(1);
    vmeWrite32(&(VETROCp[vetrocID[ii]]->Clk.Reset),0);
  }
  taskDelay(1);

  /* Write configuration registers with default/defined Sources */
  for(ii=0;ii<nvetroc;ii++) 
  {
    
      /* Program an A32 access address for this VETROC's FIFO */
      a32addr = vetrocA32Base + ii*VETROC_MAX_A32_MEM;
#ifdef VXWORKS
      res = sysBusToLocalAdrs(0x09,(char *)a32addr,(char **)&laddr);
      if (res != 0) 
	{
	  printf("vetrocInit: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",a32addr);
	  return(ERROR);
	}
#else
      res = vmeBusToLocalAdrs(0x09,(char *)a32addr,(char **)&laddr);
      if (res != 0) 
	{
	  printf("vetrocInit: ERROR in vmeBusToLocalAdrs(0x09,0x%x,&laddr) \n",a32addr);
	  return(ERROR);
	}
#endif
      VETROCpd[vetrocID[ii]] = (unsigned int *)(laddr);  /* Set a pointer to the FIFO */
      if(!noBoardInit)
	{
	  vmeWrite32(&(VETROCp[vetrocID[ii]]->EB.AD32),(a32addr>>16) + VETROC_A32_ENABLE);  /* Write the register and enable */
	
	  /* Set Default Block Level to 1 */
	  vmeWrite32(&(VETROCp[vetrocID[ii]]->EB.BlockCfg),1);

      /* berr for every board; TODO: multiblock needs it on last board only !!!*/
      vmeWrite32(&(VETROCp[vetrocID[ii]]->EB.ReadoutCfg), VETROC_ENABLE_BERR);

	}
      vetrocBlockLevel=1;

    /* Setup Trigger and Sync Reset sources */
    if(!noBoardInit)
    {
      vmeWrite32(&(VETROCp[vetrocID[ii]]->Sd.SrcSel[VETROC_OUTPUT_TRIG]),trigSrc);
      vmeWrite32(&(VETROCp[vetrocID[ii]]->Sd.SrcSel[VETROC_OUTPUT_SYNC]),syncSrc);
    }
  }

  /* If there are more than 1 VETROC in the crate then setup the Muliblock Address
     window. This must be the same on each board in the crate */
  if(nvetroc > 1) 
  {
      a32addr = vetrocA32Base + (nvetroc+1)*VETROC_MAX_A32_MEM; /* set MB base above individual board base */
#ifdef VXWORKS
      res = sysBusToLocalAdrs(0x09,(char *)a32addr,(char **)&laddr);
      if (res != 0) 
	{
	  printf("vetrocInit: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",a32addr);
	  return(ERROR);
	}
#else
      res = vmeBusToLocalAdrs(0x09,(char *)a32addr,(char **)&laddr);
      if (res != 0) 
	{
	  printf("vetrocInit: ERROR in vmeBusToLocalAdrs(0x09,0x%x,&laddr) \n",a32addr);
	  return(ERROR);
	}
#endif
      VETROCpmb = (unsigned int *)(laddr);  /* Set a pointer to the FIFO */


goto skipmultiblock;
    if(!noBoardInit)
	{
	  for (ii=0;ii<nvetroc;ii++) 
	    {
	      /* Write the register and enable */
	      vmeWrite32(&(VETROCp[vetrocID[ii]]->EB.Adr32M),
			((a32addr+VETROC_MAX_A32MB_SIZE)>>8) + (a32addr>>23) + VETROC_AMB_ENABLE);
	    }
	}    
      /* Set First Board and Last Board */
      vetrocMaxSlot = maxSlot;
      vetrocMinSlot = minSlot;
    if(!noBoardInit)
	{
	  vmeWrite32(&(VETROCp[minSlot]->EB.Adr32M),
		    vmeRead32(&(VETROCp[minSlot]->EB.Adr32M)) | VETROC_FIRST_BOARD);
	  vmeWrite32(&(VETROCp[maxSlot]->EB.Adr32M),
		    vmeRead32(&(VETROCp[maxSlot]->EB.Adr32M)) | VETROC_LAST_BOARD);
	}
skipmultiblock:
	;    
  }

  vetrocInited = nvetroc;
  if(errFlag > 0) 
  {
    printf("vetrocInit: ERROR: Unable to initialize all VETROC Modules\n");
    return(ERROR);
  } 
  else 
  {
    if(nvetroc > 0) printf("vetrocInit: %d VETROC(s) successfully initialized\n",nvetroc );
    return(nvetroc);
  }
}

/*******************************************************************************
 *
 * vetrocSetClockSource - Set the clock source
 *
 *   This routine should be used in the case that the source clock
 *   is NOT set in vetrocInit (and defaults to Internal).  Such is the case
 *   when clocks are synchronized in a many crate system.  The clock source
 *   of the VETROC should ONLY be set AFTER those clocks have been set and
 *   synchronized.
 *
 *   clkSrc: 1 bit integer
 *       bit 0:  defines Clock Source
 *           0  Internal 250MHz Clock
 *           1  VXS (P0)
 *
 */

int
vetrocSetClockSource(int id, int clkSrc)
{
  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
  {
    printf("vetrocStatus: ERROR : VETROC in slot %d is not initialized \n",id);
    return(-1);
  }

  if(clkSrc>0x1)
  {
    printf("%s: ERROR: Invalid Clock Source specified (0x%x)\n", __FUNCTION__,clkSrc);
    return ERROR;
  }

  if(clkSrc)
  {
    clkSrc = (VETROC_CLK_SRC_LOCAL<<VETROC_CLK_CTRL_REFA_SHIFT) |
      (VETROC_CLK_SRC_LOCAL<<VETROC_CLK_CTRL_REFB_SHIFT) |
      (VETROC_CLK_SRC_LOCAL<<VETROC_CLK_CTRL_FPGA_SHIFT) |
      (VETROC_CLK_SRC_LOCAL<<VETROC_CLK_CTRL_TD_SHIFT);

    printf("%s: VETROC id %d clock source set to INTERNAL\n",__FUNCTION__,id);
  }
  else
  {
    clkSrc = (VETROC_CLK_SRC_VXS<<VETROC_CLK_CTRL_REFA_SHIFT) |
      (VETROC_CLK_SRC_VXS<<VETROC_CLK_CTRL_REFB_SHIFT) |
      (VETROC_CLK_SRC_VXS<<VETROC_CLK_CTRL_FPGA_SHIFT) |
      (VETROC_CLK_SRC_VXS<<VETROC_CLK_CTRL_TD_SHIFT);
		
    printf("%s: VETROC id %d clock source set to VXS (P0)\n",__FUNCTION__,id);
  }

  VETROCLOCK;
  vmeWrite32(&(VETROCp[id]->Clk.Ctrl),clkSrc | VETROC_REF_CLK_RESET);
  vmeWrite32(&(VETROCp[id]->Clk.Ctrl),clkSrc);
  VETROCUNLOCK;
  
  taskDelay(20);
	 
  return OK;
}

void
vetrocStatus(int id, int sflag)
{ 
  int ii;
  unsigned int vers, bid;
  unsigned int adr32m, adr32;
  unsigned int lookBack, windowWidth, blockConfig;
  unsigned int clkConfig, trigBusyThreshold, trigSrc, syncSrc;
  unsigned int intr, geo;
  unsigned int fifoWordCnt, fifoEventCnt, fifoBlockCnt;
  unsigned int readoutCfg;
  unsigned int chDisable[8];

  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
    {
      printf("vetrocStatus: ERROR : VETROC in slot %d is not initialized \n",id);
      return;
    }

  VETROCLOCK;
  vers   = vmeRead32(&VETROCp[id]->Clk.FirmwareRev) & VETROC_VERSION_MASK;
  bid    = vmeRead32(&VETROCp[id]->Clk.BoardId);
  adr32m = vmeRead32(&VETROCp[id]->EB.Adr32M);
  lookBack = vmeRead32(&VETROCp[id]->EB.Lookback) * 8;
  windowWidth = vmeRead32(&VETROCp[id]->EB.WindowWidth) * 8;
  blockConfig = vmeRead32(&VETROCp[id]->EB.BlockCfg);
  clkConfig = (vmeRead32(&VETROCp[id]->Clk.Ctrl) >> VETROC_CLK_CTRL_FPGA_SHIFT) & 0x3;
  trigSrc = vmeRead32(&VETROCp[id]->Sd.SrcSel[VETROC_OUTPUT_TRIG]);
  syncSrc = vmeRead32(&VETROCp[id]->Sd.SrcSel[VETROC_OUTPUT_SYNC]);
  adr32 = vmeRead32(&VETROCp[id]->EB.AD32);
  intr = vmeRead32(&VETROCp[id]->EB.Interrupt);
  geo = (vmeRead32(&VETROCp[id]->Clk.FirmwareRev) & VETROC_SLOT_ID_MASK) >> VETROC_SLOT_ID_SHIFT;
  fifoWordCnt = vmeRead32(&VETROCp[id]->EB.FifoWordCnt);
  fifoEventCnt = vmeRead32(&VETROCp[id]->EB.FifoEventCnt);
  fifoBlockCnt = vmeRead32(&VETROCp[id]->EB.FifoBlockCnt);
  readoutCfg = vmeRead32(&VETROCp[id]->EB.ReadoutCfg);
  for(ii = 0; ii < 8; ii++)
    chDisable[ii] = vmeRead32(&VETROCp[id]->Tdc[ii].EnableN);
  VETROCUNLOCK;

#ifdef VXWORKS
  printf("\nSTATUS for VETROC in slot %d at base address 0x%x \n",
	 id, (UINT32) VETROCp[id]);
#else
  printf("\nSTATUS for VETROC in slot %d at VME (Local) base address 0x%x (0x%x)\n",
	 id, (UINT32) VETROCp[id] - vetrocA24Offset, (UINT32) VETROCp[id]);
#endif
  printf("---------------------------------------------------------------------- \n");

  printf(" Board Firmware Rev/ID = 0x%04x / 0x%08x\n", (vers)&0xffff, bid);
  if(adr32m&VETROC_AMB_ENABLE) 
    {
      printf(" Alternate VME Addressing: Multiblock Enabled\n");
      if(adr32&VETROC_A32_ENABLE)
	printf("   A32 Enabled at VME (Local) base 0x%08x (0x%08x)\n",(adr32&0xFF80)<<16,(UINT32) VETROCpd[id]);
      else
	printf("   A32 Disabled\n");
    
      printf("   Multiblock VME Address Range 0x%08x - 0x%08x\n",(adr32m&0x1FF)<<23,(adr32m&0x1FF0000)<<7);
    }
  else
    {
      printf(" Alternate VME Addressing: Multiblock Disabled\n");
      if(adr32&VETROC_A32_ENABLE)
	printf("   A32 Enabled at VME (Local) base 0x%08x (0x%08x)\n",(adr32&0xFF80)<<16,(UINT32) VETROCpd[id]);
      else
	printf("   A32 Disabled\n");
    }

  printf("\n Signal Sources: \n");
  if(clkConfig==VETROC_CLK_SRC_LOCAL)
    printf("   Ref Clock : Internal\n");
  else if(clkConfig==VETROC_CLK_SRC_VXS)
    printf("   Ref Clock : VXS\n");
  else
    printf("   Ref Clock : %d (Undefined)\n",clkConfig);

  switch(trigSrc)
  {
    case VETROC_SD_MUX_0:
    case VETROC_SD_MUX_1:      printf("   Trig Src  : VME (Software)\n"); break;
    case VETROC_SD_MUX_TRIG1F: printf("   Trig Src  : VXS\n"); break;
	 case VETROC_SD_MUX_FPIN1:  printf("   Trig Src  : FP1\n"); break;
    default:                   printf("   Trig Src  : unknown\n"); break;
  }  

  switch(syncSrc) 
  {
    case VETROC_SD_MUX_0:
    case VETROC_SD_MUX_1:      printf("   Sync Src  : VME (Software)\n"); break;
    case VETROC_SD_MUX_SYNCFI: printf("   Sync Src  : VXS\n"); break;
    default:                   printf("   Sync Src  : unknown\n"); break;
  }  

  printf("\n Configuration: \n");
  
  if(readoutCfg & VETROC_ENABLE_BERR)
    printf("   Bus Error ENABLED\n");
  else
    printf("   Bus Error DISABLED\n");


  if(adr32m & VETROC_ENABLE_MULTIBLOCK) 
  {
    if(adr32m & VETROC_FIRST_BOARD)
      printf("   MultiBlock transfer ENABLED (First Board - token via VXS)\n");
    else if(adr32m & VETROC_LAST_BOARD)
      printf("   MultiBlock transfer ENABLED (Last Board  - token via VXS)\n");
    else
      printf("   MultiBlock transfer ENABLED (Token via VXS)\n");
  } 
  else 
    printf("   MultiBlock transfer DISABLED\n");

  printf("\n TDC Processing Configuration:\n");
  printf("   Channel Disable Mask[ 15-  0] = 0x%04x\n", chDisable[0]);
  printf("   Channel Disable Mask[ 31- 16] = 0x%04x\n", chDisable[1]);
  printf("   Channel Disable Mask[ 47- 32] = 0x%04x\n", chDisable[2]);
  printf("   Channel Disable Mask[ 63- 48] = 0x%04x\n", chDisable[3]);
  printf("   Channel Disable Mask[ 79- 64] = 0x%04x\n", chDisable[4]);
  printf("   Channel Disable Mask[ 95- 80] = 0x%04x\n", chDisable[5]);
  printf("   Channel Disable Mask[111- 96] = 0x%04x\n", chDisable[6]);
  printf("   Channel Disable Mask[127-112] = 0x%04x\n", chDisable[7]);
  printf("   Lookback (PL)    = %d ns   Time Window (PTW) = %d ns\n", lookBack, windowWidth);

  printf("   Blocks in FIFO  = %u  (Block level = %d)\n", fifoBlockCnt, blockConfig & 0x7ff);
  printf("   Events in FIFO  = %u\n", fifoEventCnt);
  printf("   Words in FIFO   = %u = %u\n", fifoWordCnt);
  
  printf("\n");
}

void 
vetrocGStatus(int sflag)
{
  int ii;

  for (ii=0;ii<nvetroc;ii++) 
    vetrocStatus(vetrocID[ii],sflag);
}

void
vetrocTrig(int id)
{
  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL))
  {
    logMsg("vetrocReset: ERROR : VETROC in slot %d is not initialized \n",id,0,0,0,0,0);
    return;
  }

  VETROCLOCK;
  vmeWrite32(&(VETROCp[vetrocID[id]]->Sd.SrcSel[VETROC_OUTPUT_TRIG]),VETROC_SD_MUX_1);
  vmeWrite32(&(VETROCp[vetrocID[id]]->Sd.SrcSel[VETROC_OUTPUT_TRIG]),VETROC_SD_MUX_0);
  VETROCUNLOCK;
}

void
vetrocGTrig(int id)
{
  int ii;
  for (ii=0;ii<nvetroc;ii++)
    vetrocTrig(vetrocID[ii]);
}

void
vetrocSync(int id)
{
  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL))
  {
    logMsg("vetrocReset: ERROR : VETROC in slot %d is not initialized \n",id,0,0,0,0,0);
    return;
  }

  VETROCLOCK;
  vmeWrite32(&(VETROCp[vetrocID[id]]->Sd.SrcSel[VETROC_OUTPUT_SYNC]),VETROC_SD_MUX_1);
  vmeWrite32(&(VETROCp[vetrocID[id]]->Sd.SrcSel[VETROC_OUTPUT_SYNC]),VETROC_SD_MUX_0);
  VETROCUNLOCK;
}

void
vetrocGSync(int id)
{
  int ii;
  for (ii=0;ii<nvetroc;ii++)
    vetrocSync(vetrocID[ii]);
}

/***********************
 *
 *  vetrocSetProcMode - Setup VETROC processing modes.
 *
 */
int
vetrocSetProcMode(int id, unsigned int lookBack, unsigned int windowWidth)
{  
  int err=0;
  unsigned int ptw_last_adr, ptw_max_buf;

  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
  {
    logMsg("vetrocSetProcMode: ERROR : VETROC in slot %d is not initialized \n",id,0,0,0,0,0);
    return(ERROR);
  }

  /*Defaults */
  if((lookBack==0)||(lookBack>VETROC_MAX_LOOKBACK))  lookBack  = VETROC_DEFAULT_LOOKBACK;
  if((windowWidth==0)||(windowWidth>VETROC_MAX_WINDOW)) windowWidth = VETROC_DEFAULT_WINDOW;

  /* Consistancy check */
  if(windowWidth > lookBack) 
  {
    err++;
    printf("vetrocSetProcMode: ERROR: Window must be <= Latency\n"); 
  }

  /* convert inputs from ns to ticks */
  lookBack = lookBack / VETROC_NS_PER_CLOCK;
  windowWidth = windowWidth / VETROC_NS_PER_CLOCK;

  VETROCLOCK;
  vmeWrite32(&VETROCp[id]->EB.Lookback, lookBack);
  vmeWrite32(&VETROCp[id]->EB.WindowWidth, windowWidth);
  VETROCUNLOCK;

  return(OK);
}

void
vetrocGSetProcMode(unsigned int lookBack, unsigned int windowWidth)
{
  int ii, res;

  for (ii=0;ii<nvetroc;ii++) {
    res = vetrocSetProcMode(vetrocID[ii],lookBack,windowWidth);
    if(res<0) printf("ERROR: slot %d, in vetrocSetProcMode()\n",vetrocID[ii]);
  }
}

/**************************************************************************************
 *
 *  vetrocReadBlock - General Data readout routine
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
vetrocReadBlock(int id, volatile UINT32 *data, int nwrds, int rflag)
{
  int ii, blknum, evnum1;
  int stat, retVal, xferCount, rmode, async;
  int dCnt, berr=0;
  int dummy=0;
  volatile unsigned int *laddr;
  unsigned int bhead, ehead, val;
  unsigned int vmeAdr, csr;

  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
    {
      logMsg("vetrocReadBlock: ERROR : VETROC in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }

  if(data==NULL) 
    {
      logMsg("vetrocReadBlock: ERROR: Invalid Destination address\n",0,0,0,0,0,0);
      return(ERROR);
    }

  if(nwrds <= 0) nwrds= (VETROC_MAX_TDC_CHANNELS*VETROC_MAX_DATA_PER_CHANNEL) + 8;
  rmode = rflag&0x0f;
  async = rflag&0x80;
  
  if(rmode >= 1) 
    { /* Block Transfers */
    
      /*Assume that the DMA programming is already setup. */
      /* Don't Bother checking if there is valid data - that should be done prior
	 to calling the read routine */

      /* Check for 8 byte boundary for address - insert dummy word (Slot 0 VETROC Dummy DATA)*/
      if((unsigned long) (data)&0x7) 
	{
#ifdef VXWORKS
	  *data = VETROC_DUMMY_DATA;
#else
	  *data = LSWAP(VETROC_DUMMY_DATA);
#endif
	  dummy = 1;
	  laddr = (data + 1);
	} 
      else 
	{
	  dummy = 0;
	  laddr = data;
	}

      VETROCLOCK;
      if(rmode == 2) 
	{ /* Multiblock Mode */
	  if((vmeRead32(&(VETROCp[id]->EB.Adr32M))&VETROC_FIRST_BOARD)==0) 
	    {
	      logMsg("vetrocReadBlock: ERROR: VETROC in slot %d is not First Board\n",id,0,0,0,0,0);
	      VETROCUNLOCK;
	      return(ERROR);
	    }
	  vmeAdr = (unsigned int)(VETROCpmb) - vetrocA32Offset;
	}
      else
	{
	  vmeAdr = (unsigned int)(VETROCpd[id]) - vetrocA32Offset;
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
	  logMsg("vetrocReadBlock: ERROR in DMA transfer Initialization 0x%x\n",retVal,0,0,0,0,0);
	  VETROCUNLOCK;
	  return(retVal);
	}

      if(async) 
	{ /* Asynchonous mode - return immediately - don't wait for done!! */
	  VETROCUNLOCK;
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
	      VETROCUNLOCK;
	      return(xferCount); /* Return number of data words transfered */
	    }
	  else
	    {
#ifdef VXWORKS
	      xferCount = (/*nwrds -*/ (retVal>>2) + dummy);  /* Number of Longwords transfered */
#else
	      xferCount = ((retVal>>2) + dummy);  /* Number of Longwords transfered */
#endif
	      logMsg("vetrocReadBlock: DMA transfer terminated by unknown BUS Error (csr=0x%x xferCount=%d)\n",csr,xferCount,0,0,0,0);
	      VETROCUNLOCK;
	      return(xferCount);
	      /* 	return(ERROR); */
	    }
	} 
      else if (retVal == 0)
	{ /* Block Error finished without Bus Error */
#ifdef VXWORKS
	  logMsg("vetrocReadBlock: WARN: DMA transfer terminated by word count 0x%x\n",nwrds,0,0,0,0,0);
#else
	  logMsg("vetrocReadBlock: WARN: DMA transfer returned zero word count 0x%x\n",nwrds,0,0,0,0,0);
#endif
	  VETROCUNLOCK;
	  return(/*nwrds*/retVal>>2);
	} 
      else 
	{  /* Error in DMA */
#ifdef VXWORKS
	  logMsg("vetrocReadBlock: ERROR: sysVmeDmaDone returned an Error\n",0,0,0,0,0,0);
#else
	  logMsg("vetrocReadBlock: ERROR: vmeDmaDone returned an Error\n",0,0,0,0,0,0);
#endif
	  VETROCUNLOCK;
	  return(retVal>>2);
	}

    } 
  else 
    {  /*Programmed IO */

      /* Check if Bus Errors are enabled. If so then disable for Prog I/O reading */
      VETROCLOCK;
      berr = vmeRead32(&(VETROCp[id]->EB.ReadoutCfg));
      if(berr & VETROC_ENABLE_BERR)
	    vmeWrite32(&(VETROCp[id]->EB.ReadoutCfg), berr & ~VETROC_ENABLE_BERR);

      dCnt = 0;
      /* Read Block Header - should be first word */
      bhead = (unsigned int) *VETROCpd[id]; 
#ifndef VXWORKS
      bhead = LSWAP(bhead);
#endif
      if((bhead&VETROC_DATA_TYPE_DEFINE)&&((bhead&VETROC_DATA_TYPE_MASK) == VETROC_DATA_BLOCK_HEADER)) {
	blknum = bhead&VETROC_DATA_BLKNUM_MASK;
	ehead = (unsigned int) *VETROCpd[id];
#ifndef VXWORKS
	ehead = LSWAP(ehead);
#endif
	evnum1 = ehead&VETROC_DATA_TRIGNUM_MASK;
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
	  if( vmeRead32(&(VETROCp[id]->EB.FifoWordCnt)) == 0) 
	    {
	      logMsg("vetrocReadBlock: FIFO Empty (0x%08x)\n",bhead,0,0,0,0,0);
	      VETROCUNLOCK;
	      return(0);
	    } 
	  else 
	    {
	      logMsg("vetrocReadBlock: ERROR: Invalid Header Word 0x%08x\n",bhead,0,0,0,0,0);
	      VETROCUNLOCK;
	      return(ERROR);
	    }
	}

      ii=0;
      while(ii<nwrds) 
	{
	  val = (unsigned int) *VETROCpd[id];
	  data[ii+2] = val;
#ifndef VXWORKS
	  val = LSWAP(val);
#endif
	  if( (val&VETROC_DATA_TYPE_DEFINE) 
	      && ((val&VETROC_DATA_TYPE_MASK) == VETROC_DATA_BLOCK_TRAILER) )
	    break;
	  ii++;
	}
      ii++;
      dCnt += ii;


      if(berr & VETROC_ENABLE_BERR)
        vmeWrite32(&(VETROCp[id]->EB.ReadoutCfg), berr | VETROC_ENABLE_BERR);

      VETROCUNLOCK;
      return(dCnt);
    }

  VETROCUNLOCK;
  return(OK);
}

int
vetrocReadBlockStatus(int id, volatile UINT32 *data, int nwrds, int rflag)
{

  int stat, retVal, xferCount, rmode, async;
  int dummy=0;
  unsigned int csr=0;
  
  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
    {
      logMsg("vetrocReadBlockStatus: ERROR : VETROC in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }

  if(nwrds <= 0) nwrds= (VETROC_MAX_TDC_CHANNELS*VETROC_MAX_DATA_PER_CHANNEL) + 8;
  rmode = rflag&0x0f;
  async = rflag&0x80;

  /* Check for 8 byte boundary for address - insert dummy word (Slot 0 VETROC Dummy DATA)*/
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
  VETROCLOCK;
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
	  VETROCUNLOCK;
	  return(xferCount); /* Return number of data words transfered */
	}
      else
	{
	  xferCount = (/*nwrds -*/ (retVal>>2) + dummy);  /* Number of Longwords transfered */
	  logMsg("vetrocReadBlockStatus: DMA transfer terminated by unknown BUS Error (csr=0x%x nwrds=%d)\n",csr,xferCount,0,0,0,0);
	  VETROCUNLOCK;
	  return(ERROR);
	}
    } 
  else if (retVal == 0)
    { /* Block Error finished without Bus Error */
      logMsg("vetrocReadBlockStatus: WARN: DMA transfer terminated by word count 0x%x\n",nwrds,0,0,0,0,0);
      VETROCUNLOCK;
      return(/*nwrds*/retVal>>2);
    } 
  else 
    {  /* Error in DMA */
      logMsg("vetrocReadBlockStatus: ERROR: sysVmeDmaDone returned an Error\n",0,0,0,0,0,0);
      VETROCUNLOCK;
      return(retVal);
    }
  
}

int
vetrocPrintBlock(int id, int rflag)
{

  int ii, blknum, evnum1;
  int nwrds=32768, dCnt, berr=0;
  unsigned int data, bhead, ehead;

  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
    {
      printf("vetrocPrintEvent: ERROR : VETROC in slot %d is not initialized \n",id);
      return(ERROR);
    }

  /* Check if data available */
  VETROCLOCK;
  if(vmeRead32(&(VETROCp[id]->EB.FifoWordCnt))==0) 
    {
      printf("vetrocPrintEvent: ERROR: FIFO Empty\n");
      VETROCUNLOCK;
      return(0);
    }

  /* Check if Bus Errors are enabled. If so then disable for reading */
  berr = vmeRead32(&(VETROCp[id]->EB.ReadoutCfg));
  if(berr & VETROC_ENABLE_BERR)
    vmeWrite32(&(VETROCp[id]->EB.ReadoutCfg), berr & ~VETROC_ENABLE_BERR);

  dCnt = 0;
  /* Read Block Header - should be first word */
  bhead = (unsigned int) *VETROCpd[id];
#ifndef VXWORKS
  bhead = LSWAP(bhead);
#endif
  if( (bhead&VETROC_DATA_TYPE_DEFINE)&&((bhead&VETROC_DATA_TYPE_MASK) == VETROC_DATA_BLOCK_HEADER)) 
    {
      blknum = bhead&VETROC_DATA_BLKNUM_MASK;
      ehead = (unsigned int) *VETROCpd[id];
#ifndef VXWORKS
      ehead = LSWAP(ehead);
#endif
      evnum1 = ehead&VETROC_DATA_TRIGNUM_MASK;
      printf("%4d: ",dCnt+1); 
      vetrocDataDecode(bhead);
      dCnt++;
      printf("%4d: ",dCnt+1); 
      vetrocDataDecode(ehead);
      dCnt++;
    }
  else
    {
      /* We got bad data - Check if there is any data at all */
      if(vmeRead32(&(VETROCp[id]->EB.FifoWordCnt))==0) 
	{
	  logMsg("vetrocPrintBlock: FIFO Empty (0x%08x)\n",bhead,0,0,0,0,0);
	  VETROCUNLOCK;
	  return(0);
	} 
      else 
	{
	  logMsg("vetrocPrintBlock: ERROR: Invalid Header Word 0x%08x\n",bhead,0,0,0,0,0);
	  VETROCUNLOCK;
	  return(ERROR);
	}
    }
  
  ii=0;
  while(ii<nwrds) 
    {
      data = (unsigned int) *VETROCpd[id];
#ifndef VXWORKS
      data = LSWAP(data);
#endif
      printf("%4d: ",dCnt+1+ii); 
      vetrocDataDecode(data);
      if((data&VETROC_DATA_TYPE_DEFINE)&&((data&VETROC_DATA_TYPE_MASK) == VETROC_DATA_BLOCK_TRAILER))
	break;
      if((data&VETROC_DATA_TYPE_DEFINE)&&((data&VETROC_DATA_TYPE_MASK) == VETROC_DATA_INVALID))
	break;
      ii++;
    }
  ii++;
  dCnt += ii;


  if(berr & VETROC_ENABLE_BERR)
    vmeWrite32(&(VETROCp[id]->EB.ReadoutCfg), berr | VETROC_ENABLE_BERR);
  
  VETROCUNLOCK;
  return(dCnt);
  
}




/*****************************************************************************/


/* Reset Event Builder (FIFOs etc) */
void
vetrocClear(int id)
{
  if(id==0) id = vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
    {
      logMsg("vetrocClear: ERROR : VETROC in slot %d is not initialized \n",id,0,0,0,0,0);
      return;
    }

  VETROCLOCK;
  vmeWrite32(&(VETROCp[id]->Clk.Reset),1);
  taskDelay(2);
  vmeWrite32(&(VETROCp[id]->Clk.Reset),0);
  VETROCUNLOCK;
}


void
vetrocReset(int id, int iFlag)
{
  unsigned int a32addr, addrMB;

  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
  {
    logMsg("vetrocReset: ERROR : VETROC in slot %d is not initialized \n",id,0,0,0,0,0);
    return;
  }

  VETROCLOCK;
  if(iFlag==0)
  {
    a32addr = vmeRead32(&(VETROCp[id]->EB.AD32));
    addrMB  = vmeRead32(&(VETROCp[id]->EB.Adr32M));
  }

  vmeWrite32(&(VETROCp[id]->Clk.Reset), 1);
  vmeWrite32(&(VETROCp[id]->Clk.Reset), 0);
//  vetrocHardReset(id);
  taskDelay(10);

  if(iFlag==0)
  {
    vmeWrite32(&(VETROCp[id]->EB.AD32),a32addr);
    vmeWrite32(&(VETROCp[id]->EB.Adr32M),addrMB);
  }
  VETROCUNLOCK;
}

void
vetrocChanDisable(int id, int group, int mask)
{

  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
  {
    logMsg("vetrocChanDisable: ERROR : VETROC in slot %d is not initialized \n",id,0,0,0,0,0);
    return;
  }
  
  if(group < 0 || group > 7)
  {
    printf("%s: ERROR: Invalid TDC channel group %d\n", __FUNCTION__, group);
    return;
  }

  VETROCLOCK;
  /* Write New Disable Mask */
  vmeWrite32(&(VETROCp[id]->Tdc[group].EnableN), mask);
  VETROCUNLOCK;
}

/* Return Event/Block count for VETROC in slot id */
int
vetrocDready(int id, int dflag)
{
  unsigned int dcnt=0;

  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
    {
      logMsg("vetrocDready: ERROR : VETROC in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }
  VETROCLOCK;
  if(dflag)
    dcnt = vmeRead32(&(VETROCp[id]->EB.FifoBlockCnt));
  else
    dcnt = vmeRead32(&(VETROCp[id]->EB.FifoEventCnt));
  VETROCUNLOCK;
  
  return(dcnt);
}

/* Return a Block Ready status for VETROC. If Block Level is =1 then return Event Ready status */
int
vetrocBready(int id)
{
  int stat=0;

  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
    {
      logMsg("vetrocBready: ERROR : VETROC in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }
  
  VETROCLOCK;
  stat = vmeRead32(&(VETROCp[id]->EB.FifoBlockCnt));
  VETROCUNLOCK;

  if(stat)
    return(1);
  else
    return(0);
}

unsigned int
vetrocGBready()
{
  int ii, id, stat=0;
  unsigned int dmask=0;
  
  VETROCLOCK;
  for(ii=0;ii<nvetroc;ii++) 
    {
      id = vetrocID[ii];
      stat = vmeRead32(&(VETROCp[id]->EB.FifoBlockCnt));
 
      if(stat)
	dmask |= (1<<id);
    }
  VETROCUNLOCK;
  
  return(dmask);
}

/* return Scan mask for all initialized VETROCs */
unsigned int
vetrocScanMask()
{
  int ivetroc, id, dmask=0;

  for(ivetroc=0; ivetroc<nvetroc; ivetroc++)
    {
      id = vetrocID[ivetroc];
      dmask |= (1<<id);
    }

  return(dmask);
}

void
vetrocEnableBusError(int id)
{

  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
    {
      logMsg("vetrocEnableBusError: ERROR : VETROC in slot %d is not initialized \n",id,0,0,0,0,0);
      return;
    }
  
  VETROCLOCK;
  vmeWrite32(&(VETROCp[id]->EB.ReadoutCfg),
    vmeRead32(&(VETROCp[id]->EB.ReadoutCfg)) | VETROC_ENABLE_BERR );
  VETROCUNLOCK;
}

void
vetrocGEnableBusError()
{
  int ii;

  VETROCLOCK;
  for(ii=0;ii<nvetroc;ii++) 
  {
    vmeWrite32(&(VETROCp[vetrocID[ii]]->EB.ReadoutCfg),
      vmeRead32(&(VETROCp[vetrocID[ii]]->EB.ReadoutCfg)) | VETROC_ENABLE_BERR );
  }
  VETROCUNLOCK;
}


void
vetrocDisableBusError(int id)
{

  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
    {
      logMsg("vetrocDisableBusError: ERROR : VETROC in slot %d is not initialized \n",id,0,0,0,0,0);
      return;
    }
  
  VETROCLOCK;
  vmeWrite32(&(VETROCp[id]->EB.ReadoutCfg),
	    vmeRead32(&(VETROCp[id]->EB.ReadoutCfg)) & ~VETROC_ENABLE_BERR );
  VETROCUNLOCK;

}


void
vetrocEnableMultiBlock()
{
  int ii, id;

  if((nvetroc <= 1) || (VETROCp[vetrocID[0]] == NULL)) 
    {
      logMsg("vetrocEnableMultiBlock: ERROR : Cannot Enable MultiBlock mode \n",0,0,0,0,0,0);
      return;
    }
  
  for(ii=0;ii<nvetroc;ii++) 
    {
      id = vetrocID[ii];
      VETROCLOCK;
      vmeWrite32(&(VETROCp[id]->EB.Adr32M),
		vmeRead32(&(VETROCp[id]->EB.Adr32M)) | VETROC_ENABLE_MULTIBLOCK );
      VETROCUNLOCK;
      vetrocDisableBusError(id);
      if(id == vetrocMinSlot) 
	{
	  VETROCLOCK;
	  vmeWrite32(&(VETROCp[id]->EB.Adr32M),
		    vmeRead32(&(VETROCp[id]->EB.Adr32M)) | VETROC_FIRST_BOARD );
	  VETROCUNLOCK;
	}
      if(id == vetrocMaxSlot) 
	{
	  VETROCLOCK;
	  vmeWrite32(&(VETROCp[id]->EB.Adr32M),
		    vmeRead32(&(VETROCp[id]->EB.Adr32M)) | VETROC_LAST_BOARD );
	  VETROCUNLOCK;
	  vetrocEnableBusError(id);   /* Enable Bus Error only on Last Board */
	}
    }

}

void
vetrocDisableMultiBlock()
{
  int ii;

  if((nvetroc <= 1) || (VETROCp[vetrocID[0]] == NULL)) 
    {
      logMsg("vetrocDisableMultiBlock: ERROR : Cannot Disable MultiBlock Mode\n",0,0,0,0,0,0);
      return;
    }
  
  VETROCLOCK;
  for(ii=0;ii<nvetroc;ii++)
    vmeWrite32(&(VETROCp[vetrocID[ii]]->EB.Adr32M),
	      vmeRead32(&(VETROCp[vetrocID[ii]]->EB.Adr32M)) & ~VETROC_ENABLE_MULTIBLOCK );
  VETROCUNLOCK;
}



int
vetrocSetBlockLevel(int id, int level)
{
  int rval;

  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
    {
      logMsg("vetrocSetBlockLevel: ERROR : VETROC in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }
  
  if(level<=0) level = 1;
  VETROCLOCK;
  vmeWrite32(&(VETROCp[id]->EB.BlockCfg), level);
  vetrocBlockLevel = level;
  rval = vmeRead32(&(VETROCp[id]->EB.BlockCfg)) & VETROC_BLOCK_LEVEL_MASK;
  VETROCUNLOCK;

  return(rval);

}
void
vetrocGSetBlockLevel(int level)
{
  int ii;

  if(level<=0) level = 1;
  VETROCLOCK;
  for(ii=0;ii<nvetroc;ii++)
    vmeWrite32(&(VETROCp[vetrocID[ii]]->EB.BlockCfg), level);
  VETROCUNLOCK;

  vetrocBlockLevel = level;
}

int  
vetrocPulserSetup(int id, float freq, float duty, unsigned int npulses)
{ 
  unsigned int per, low; 
	 
  if(id==0) id=vetrocID[0];
  
  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
  {
    printf("%s: ERROR : VETROC in slot %d is not initialized \n", __FUNCTION__, id);
    return(ERROR);
  }

  if(freq < VETROC_PULSER_FREQ_MIN)
  { 
    printf("%s: ERROR: Frequency input (%f) too low. Setting to minimum...\n",  
	     __FUNCTION__,freq); 
    freq = VETROC_PULSER_FREQ_MIN;
  } 
	 
  if(freq > VETROC_PULSER_FREQ_MAX) 
  { 
    printf("%s: ERROR: Frequency input (%f) too high. Setting to maximum...\n",  
	     __FUNCTION__,freq); 
    freq = VETROC_PULSER_FREQ_MAX; 
  } 
	 
  if((duty < 0.0) || (duty > 1.0)) 
  { 
    printf("%s: ERROR: Invalid duty cycle %f. Setting to 0.5\n",  
	     __FUNCTION__,duty); 
    duty = 0.5; 
  } 
 
  VETROCLOCK;	 
  // Setup period register... 
  per = 125000000.0f / freq; 
  if(!per) 
    per = 1; 
  vmeWrite32(&VETROCp[id]->Sd.PulserPeriod, per); 
	 
  // Setup duty cycle register...	 
  low = per * duty; 
  if(!low) 
    low = 1; 
  vmeWrite32(&VETROCp[id]->Sd.PulserLowCycles, low); 
  vmeWrite32(&VETROCp[id]->Sd.PulserNPulses, npulses);
	 
  printf("%s: Actual frequency = %f, duty = %f\n", __FUNCTION__, 
	 125000000.0f/(float)per, (float)low/(float)per); 
  VETROCUNLOCK; 

/*	if(npulses<0xFFFFFFFF) vetrocPulserStart(id); */
  return OK;
} 

void vetrocSelectSpi(int id,int sel)
{
	if(sel)
		vmeWrite32(&VETROCp[id]->Clk.SpiCtrl, VETROC_SPI_NCS_CLEAR);
	else
		vmeWrite32(&VETROCp[id]->Clk.SpiCtrl, VETROC_SPI_NCS_SET);
}

unsigned char vetrocTransferSpi(int id,unsigned char data)
{
	unsigned int i, rsp = 0;
	
	vmeWrite32(&VETROCp[id]->Clk.SpiCtrl, data | VETROC_SPI_TRANSFER);

	for(i = 0; i < 8; i++)
	{
		rsp = vmeRead32(&VETROCp[id]->Clk.SpiStatus);
		if(rsp & VETROC_SPI_READY)
			break;
	}

	return (unsigned char)(rsp & 0xFF);
}

unsigned int vetrocFlashGetId(int id)
{
	unsigned int rsp;

	vetrocSelectSpi(id,1);
	vetrocTransferSpi(id,VETROC_FLASH_CMD_GETID);
	rsp = vetrocTransferSpi(id,0xFF);
	rsp |= vetrocTransferSpi(id,0xFF)<<8;
	rsp |= vetrocTransferSpi(id,0xFF)<<16;
	rsp |= vetrocTransferSpi(id,0xFF)<<24;
	vetrocSelectSpi(id,0);

	return rsp;
}

unsigned char vetrocFlashGetStatus(int id, unsigned char cmd)
{
	unsigned char rsp;
	
	vetrocSelectSpi(id,1);
	vetrocTransferSpi(id,cmd);
	rsp = vetrocTransferSpi(id,0xFF);
	vetrocSelectSpi(id,0);
	
	return rsp;
}

int vetrocFirmwareUpdateVerify(int id, const char *filename)
{
	int i, result;

  if(id==0) id=vetrocID[0];
  
  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
    {
      printf("vetrocFirmwareUpdateVerify: ERROR : VETROC in slot %d is not initialized \n",id);
      return(ERROR);
    }

	printf("Updating firmware...");
	result = vetrocFirmwareUpdate(id, filename);
	if(result != OK)
	{
		printf("failed.\n");
		return result;
	}
	else
		printf("succeeded.\n");
	
	printf("\nVerifying...");
	result = vetrocFirmwareVerify(id, filename);
	if(result != OK)
	{
		printf("failed.\n");
		return result;
	}
	else
		printf("ok.\n");

//	vetrocHardReset(id);
//	taskDelay(120);
		
	return OK;
}

void vetrocGFirmwareUpdateVerify(const char *filename)
{
	int ii;
	for(ii = 0; ii < nvetroc; ii++)
		vetrocFirmwareUpdateVerify(vetrocID[ii], filename);
}

int vetrocFirmwareUpdate(int id, const char *filename)
{
	FILE *f;
	int i, flashId = 0;
	unsigned int addr = 0, page = 0, rspId;
	unsigned char buf[1056];

  if(id==0) id=vetrocID[0];
  
  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
    {
      printf("vetrocFirmwareUpdate: ERROR : VETROC in slot %d is not initialized \n",id);
      return(ERROR);
    }
	
	vetrocSelectSpi(id,0);
	rspId = vetrocFlashGetId(id);
	
	printf("Flash[0x%08X]: Mfg=0x%02X, Type=0x%02X, Capacity=0x%02X\n", rspId, (rspId>>0)&0xFF, (rspId>>8)&0xFF, (rspId>>16)&0xFF);

	if(rspId == VETROC_FLASH_DEVID_N25Q256A)
	{
		f = fopen(filename, "rb");
		if(!f)
		{
			printf("%s: ERROR: vetrocFirmwareUpdate invalid file %s\n", __FUNCTION__, filename);
			return ERROR;
		}
	
		memset(buf, 0xff, 256);
		while(fread(buf, 1, 256, f) > 0)
		{
		  if(!(addr % 65536))		/* sector erase*/
			{
				vetrocSelectSpi(id,1);
				vetrocTransferSpi(id,VETROC_FLASH_CMD_WREN);	/* write enable*/
				vetrocSelectSpi(id,0);

				vetrocSelectSpi(id,1);
				vetrocTransferSpi(id,VETROC_FLASH_CMD_ERASE64K);	/* 64k sector erase*/
				vetrocTransferSpi(id,(addr>>16)&0xFF);
				vetrocTransferSpi(id,(addr>>8)&0xFF);
				vetrocTransferSpi(id,(addr)&0xFF);
				vetrocSelectSpi(id,0);

				printf(".");
				i = 0;
				while(1)
				{
					if(!(vetrocFlashGetStatus(id, VETROC_FLASH_CMD_GETSTATUS) & 0x1))
						break;
					taskDelay(1);
					if(i == 60+6)	/* 1000ms maximum sector erase time*/
					{
						fclose(f);
						printf("%s: ERROR: vetrocFirmwareUpdate failed to erase flash\n", __FUNCTION__);
						return ERROR;
					}
					i++;
				}
			}

			vetrocSelectSpi(id,1);
			vetrocTransferSpi(id,VETROC_FLASH_CMD_WREN);	/* write enable*/
			vetrocSelectSpi(id,0);

			vetrocSelectSpi(id,1);
			vetrocTransferSpi(id,VETROC_FLASH_CMD_WRPAGE);	/* write page*/
			vetrocTransferSpi(id,(addr>>16)&0xFF);
			vetrocTransferSpi(id,(addr>>8)&0xFF);
			vetrocTransferSpi(id,(addr)&0xFF);
			for(i = 0; i < 256; i++)
				vetrocTransferSpi(id,buf[i]);
			vetrocSelectSpi(id,0);

			i = 0;
			while(1)
			{
			  if(!(vetrocFlashGetStatus(id, VETROC_FLASH_CMD_GETSTATUS) & 0x1))	/* no faster than 1us per call*/
					break;
			  if(i == 3000)	/* 3ms maximum page program time*/
				{
					fclose(f);
					printf("%s: ERROR: vetrocFirmwareUpdate failed to program flash\n", __FUNCTION__);
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
		printf("%s: ERROR: vetrocFirmwareUpdate failed to identify flash id (or device not supported)\n", __FUNCTION__);
		return ERROR;
	}
	return OK;
}

int vetrocFirmwareRead(int id, const char *filename)
{
	FILE *f;
	int i,len, flashId = 0;
	unsigned int addr = 0, rspId;
	unsigned char buf[256];

  if(id==0) id=vetrocID[0];
  
  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
    {
      printf("vetrocFirmwareRead: ERROR : VETROC in slot %d is not initialized \n",id);
      return(ERROR);
    }

	vetrocSelectSpi(id,0);
	rspId = vetrocFlashGetId(id);
	
	printf("Flash[0x%08X]: Mfg=0x%02X, Type=0x%02X, Capacity=0x%02X\n", rspId, (rspId>>0)&0xFF, (rspId>>8)&0xFF, (rspId>>16)&0xFF);

	if(rspId == VETROC_FLASH_DEVID_N25Q256A)
	{
		f = fopen(filename, "wb");
		if(!f)
		{
			printf("%s: ERROR: vetrocFirmwareRead invalid file %s\n", __FUNCTION__, filename);
			return ERROR;
		}
		
		vetrocSelectSpi(id,1);
		vetrocTransferSpi(id,VETROC_FLASH_CMD_RD);	/* continuous array read */
		vetrocTransferSpi(id,(addr>>16)&0xFF);
		vetrocTransferSpi(id,(addr>>8)&0xFF);
		vetrocTransferSpi(id,(addr)&0xFF);
		
		for(i = 0; i < VETROC_FLASH_BYTE_LENGTH; i++)
		{
			fputc(vetrocTransferSpi(id,0xFF), f);
			if(!(i% 65536))
			{
				printf(".");
				taskDelay(1);
			}
		}
			
		vetrocSelectSpi(id,0);
		fclose(f);
	}
	else
	{
		printf("%s: ERROR: vetrocFirmwareRead failed to identify flash id 0x%02X\n", __FUNCTION__, flashId);
		return ERROR;
	}
	return OK;
}

int vetrocFirmwareVerify(int id, const char *filename)
{
	FILE *f;
	int i,len, flashId = 0;
	unsigned int addr = 0, rspId;
	unsigned char buf[256];
	unsigned char val;
	
  if(id==0) id=vetrocID[0];
  
  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
    {
      logMsg("vetrocFirmwareVerify: ERROR : VETROC in slot %d is not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }
	
	vetrocSelectSpi(id,0);
	rspId = vetrocFlashGetId(id);
	
	printf("Flash[0x%08X]: Mfg=0x%02X, Type=0x%02X, Capacity=0x%02X\n", rspId, (rspId>>0)&0xFF, (rspId>>8)&0xFF, (rspId>>16)&0xFF);

	if(rspId == VETROC_FLASH_DEVID_N25Q256A)
	{
		f = fopen(filename, "rb");
		if(!f)
		{
			printf("%s: ERROR: vetrocFirmwareVerify invalid file %s\n", __FUNCTION__, filename);
			return ERROR;
		}
		
		vetrocSelectSpi(id,1);
		vetrocTransferSpi(id,VETROC_FLASH_CMD_RD);	/* continuous array read */
		vetrocTransferSpi(id,(addr>>16)&0xFF);
		vetrocTransferSpi(id,(addr>>8)&0xFF);
		vetrocTransferSpi(id,(addr)&0xFF);

		while((len = fread(buf, 1, 256, f)) > 0)
		{
			for(i = 0; i < len; i++)
			{
				val = vetrocTransferSpi(id,0xFF);
				if(buf[i] != val)
				{
					vetrocSelectSpi(id,0);
					fclose(f);					
					printf("%s: ERROR: vetrocFirmwareVerify failed verify at addess 0x%08X[%02X,%02X]\n", __FUNCTION__, addr+i, buf[i], val);
					return ERROR;
				}
			}
			addr+=256;
			if(!(addr & 0xFFFF))
				printf(".");
		}
		vetrocSelectSpi(id,0);
		fclose(f);
	}
	else
	{
		printf("%s: ERROR: vetrocFirmwareVerify failed to identify flash id 0x%02X\n", __FUNCTION__, flashId);
		return ERROR;
	}
	return OK;
}

void 
vetrocDataDecode(unsigned int data)
{
	static int type = 0;
	static int idx = 0;
	printf("0x%08X", data);

	if(data & 0x80000000)
	{
		type = data & 0x78000000;
		idx = 0;
	}
	
	switch(type)
	{
		case VETROC_DATA_BLOCK_HEADER:
			printf(" {BLKHDR} SLOTID: %d", (data>>22)&0x1f);
			printf(" NEVENTS: %d", (data>>11)&0x7ff);
			printf(" BLOCK: %d\n", (data>>0)&0x7ff);				
			break;
		case VETROC_DATA_BLOCK_TRAILER:
			printf(" {BLKTLR} SLOTID: %d", (data>>22)&0x1f);
			printf(" NWORDS: %d\n", (data>>0)&0x3fffff);
			break;
		case VETROC_DATA_EVENT_HEADER:
			printf(" {EVTHDR} EVENT: %d\n", (data>>0)&0x7ffffff);
			break;
		case VETROC_DATA_TRIGGER_TIME:
			if(idx == 0) printf(" {TRGTIME} 0: %08X\n", data & 0xffffff);
			if(idx == 1) printf(" {TRGTIME} 1: %08X\n", data & 0xffffff);
			break;
		case VETROC_DATA_TDCEVT:
			printf(" {TDCEVT} CH: %d", (data>>16)&0x7f);
			printf(" TIME: %dns\n", (data>>0)&0xFFFF);
			break;
		case VETROC_DATA_INVALID:
			printf(" {***DNV***}\n");
			break;
		case VETROC_DATA_FILLER:
			printf(" {FILLER}\n");
			break;
		default:
			printf(" {***DATATYPE ERROR***}\n");
			break;
	}
	idx++;
	return;
}


/* width in ns */
int
vetrocTriggerPulseWidth(int id, unsigned int width)
{
  int ii;
  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
  {
    printf("vetrocSoftTrig: ERROR : VETROC in slot %d is not initialized \n",id);
    return(-1);
  }

  width /= VETROC_NS_PER_CLOCK;

  for(ii = 0; ii < 8; ii++)
    vmeWrite32(&(VETROCp[id]->Tdc[ii].TriggerWidth),width);

  return(width);
}


/* returns 1 if link is UP, 0, otherwise */
int
vetrocLinkStatus(int id)
{
  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
  {
    printf("vetrocSoftTrig: ERROR : VETROC in slot %d is not initialized \n",id);
    return(-1);
  }

  return (vmeRead32(&(VETROCp[id]->Ser.Status))>>4) & 0x1;
}

int
vetrocLinkReset(int id)
{
  if(id==0) id=vetrocID[0];

  if((id<=0) || (id>21) || (VETROCp[id] == NULL)) 
  {
    printf("vetrocSoftTrig: ERROR : VETROC in slot %d is not initialized \n",id);
    return(-1);
  }

  vmeWrite32(&(VETROCp[id]->Ser.Ctrl), 0x203);
  taskDelay(1);
  vmeWrite32(&(VETROCp[id]->Ser.Ctrl), 0x202);
  taskDelay(1);
  vmeWrite32(&(VETROCp[id]->Ser.Ctrl), 0x200);
  taskDelay(1);
  vmeWrite32(&(VETROCp[id]->Ser.Ctrl), 0x000);
  taskDelay(1);

  return(0);
}

#else /* dummy version*/

void
vetrocLib_dummy()
{
  return;
}

#endif
