/*----------------------------------------------------------------------------*/
/**
 * @mainpage
 * <pre>
 *  Copyright (c) 2012        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Authors: Bryan Moffit                                                   *
 *             moffit@jlab.org                   Jefferson Lab, MS-12B3       *
 *             Phone: (757) 269-5660             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *     Software driver for the JLab Trigger Distribution Module
 *
 * </pre>
 *----------------------------------------------------------------------------*/
#if defined(VXWORKS) || defined(Linux_vme) /*sergey*/

#define _GNU_SOURCE

#ifdef VXWORKS
#include <vxWorks.h>
#include <sysLib.h>
#include <logLib.h>
#include <taskLib.h>
#include <intLib.h>
#include <iv.h>
#include <semLib.h>
#include <vxLib.h>
#include "vxCompat.h"
#else 
#include <sys/prctl.h>
#include <unistd.h>
#include "jvme.h"
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "tdLib.h"

/** Mutex to guard TD read/writes */
pthread_mutex_t   tdMutex = PTHREAD_MUTEX_INITIALIZER;
/** Mutex Lock */
#define TDLOCK     if(pthread_mutex_lock(&tdMutex)<0) perror("pthread_mutex_lock");
/** Mutex Unlock */
#define TDUNLOCK   if(pthread_mutex_unlock(&tdMutex)<0) perror("pthread_mutex_unlock");

/* Global Variables */
int nTD = 0;                                  /**< Number of TDs found with tdInit() */
volatile struct TD_A24RegStruct *TDp[MAX_VME_SLOTS+1];    /**< pointer to TD memory map */
int tdID[MAX_VME_SLOTS+1];                    /**< array of slot numbers for TDs */
unsigned int tdAddrList[MAX_VME_SLOTS+1];     /**< array of a24 addresses for TDs */
unsigned long tdA24Offset=0;                            /**< Difference in CPU A24 Base and VME A24 Base */
int tdCrateID=0x59;                           /**< Crate ID */
int tdBlockLevel=0;                           /**< Block level for TD */
int tdFiberLatencyOffset = 0xbf;              /**< Default offset for fiber latency */
unsigned int tdSlaveMask[MAX_VME_SLOTS+1];    /**< TI Slaves (mask) to be used */

#ifdef VXWORKS
extern  int sysBusToLocalAdrs(int, char *, char **);
extern  int intDisconnect(int);
extern  int sysIntEnable(int);
IMPORT  STATUS sysIntDisable(int);
#endif

/**
 * @defgroup Config Initialization/Configuration
 * @defgroup Status Status
 * @defgroup Deprec Deprecated - To be removed
 */

/**
 *  @ingroup Config
 *  @brief Initialize JLAB TD Library
 *  
 * @param addr
 *  - A24 VME Address of the TD
 * @param addr_inc
 *  - Amount to increment addr to find the next TD
 * @param nfind
 *  - Number of times to increment
 * @param iFlag - Initialization mask
 *   -   0:  Exit before board initialization (just map structure pointer)
 *   -   1:  RFU
 *   -   2:  Ignore firmware check
 *   -   3:  Use tdAddrList instead of addr and addr_inc for VME addresses
 *
 *  @return OK if successful, otherwise ERROR.
 */
STATUS
tdInit(UINT32 addr, UINT32 addr_inc, int nfind, int iFlag)
{
  int useList=0, noBoardInit=0, noFirmwareCheck=0;
  int islot, itd, TD_SLOT;
  int res;
  unsigned int rdata, boardID;
  unsigned long laddr, laddr_inc;
  unsigned int firmwareInfo=0, tdVersion=0, tdType=0;
  volatile struct TD_A24RegStruct *td;


  /* Check if we're skipping initialization, and just mapping the structure pointer */
  if(iFlag&TD_INIT_NO_INIT)
    {
      noBoardInit = 1;
    }
  /* Check if we're skipping the firmware check */
  if(iFlag&TD_INIT_SKIP_FIRMWARE_CHECK)
    {
      noFirmwareCheck=1;
    }
  /* Check if we're initializing using a list */
  if(iFlag&TD_INIT_USE_ADDR_LIST)
    {
      useList=1;
    }

  /* Check for valid address */
  if( (addr==0) && (useList==0) ) 
    { /* Scan through valid slot -> A24 address */
      useList=1;
      nfind=16;

      /* Loop through JLab Standard GEOADDR to VME addresses to make a list */
      for(islot=3; islot<11; islot++) /* First 8 */
	tdAddrList[islot-3] = (islot<<19);
      
      /* Skip Switch Slots */
      
      for(islot=13; islot<21; islot++) /* Last 8 */
	tdAddrList[islot-5] = (islot<<19);

    }
  else if(addr > 0x00ffffff) 
    { /* A32 Addressing */
      printf("%s: ERROR: A32 Addressing not allowed for TD configuration space\n",
	     __FUNCTION__);
      return(ERROR);
    }
  else
    { /* A24 Addressing */
      if(addr < 22)
	{ /* First argument is a slot number, instead of VME address */
	  printf("%s: Initializing using slot number %d (VME address 0x%x)\n",
		 __FUNCTION__,addr,addr<<19);
	  addr = addr<<19; // Shift to VME A24 address;
	  
	  /* If addr_inc is also in slot number form, shift it */
	  if((addr_inc<22) && (addr_inc>0))
	    addr_inc = addr_inc<<19;

	  /* Check and shift the address list, if it's used */
	  if(useList==1)
	    {
	      for(itd=0; itd<nfind; itd++)
		{
		  if(tdAddrList[itd] < 22)
		    {
		      tdAddrList[itd] = tdAddrList[itd]<<19;
		    }
		}
	    }
	}

      if( ((addr_inc==0)||(nfind==0)) && (useList==0) )
	{ /* assume only one TD to initialize */
	  nfind = 1; 
	}
    }

  /* get the TD address */
#ifdef VXWORKS
  res = sysBusToLocalAdrs(0x39,(char *)(unsigned long)addr,(char **)&laddr);
#else
  res = vmeBusToLocalAdrs(0x39,(char *)(unsigned long)addr,(char **)&laddr);
#endif

#ifndef SHOWERROR
#ifndef VXWORKS
  vmeSetQuietFlag(1);
#endif
#endif

  if (res != 0) 
    {
#ifdef VXWORKS
      printf("%s: ERROR in sysBusToLocalAdrs(0x39,0x%x,&laddr) \n",
	     __FUNCTION__,addr);
#else
      printf("%s: ERROR in vmeBusToLocalAdrs(0x39,0x%x,&laddr) \n",
	     __FUNCTION__,addr);
#endif
      return(ERROR);
    }
  tdA24Offset = laddr - addr;

  nTD = 0; /*sergey*/
  for (itd=0;itd<nfind;itd++) 
    {
      if(useList==1)
	{
	  laddr_inc = tdAddrList[itd] + tdA24Offset;
	}
      else
	{
	  laddr_inc = laddr +itd*addr_inc;
	}

      td = (struct TD_A24RegStruct *)laddr_inc;
      /* Check if Board exists at that address */
#ifdef VXWORKS
      res = vxMemProbe((char *) &(td->boardID),VX_READ,4,(char *)&rdata);
#else
      res = vmeMemProbe((char *) &(td->boardID),4,(char *)&rdata);
#endif

      if(res < 0) 
	{
#ifdef SHOWERRORS
#ifdef VXWORKS
	  printf("%s: ERROR: No addressable board at addr=0x%x\n",
		 __FUNCTION__,(UINT32) td);
#else
	  printf("%s: ERROR: No addressable board at VME (Local) addr=0x%x (0x%x)\n",
		 __FUNCTION__,
		 (UINT32) laddr_inc-tdA24Offset, (UINT32) td);
#endif
#endif /* SUPPRESSERRORS */
	}
      else 
	{
	  /* Check that it is a TD */
	  if(((rdata&TD_BOARDID_TYPE_MASK)>>16) != TD_BOARDID_TYPE_TD)
	    {
 	      printf(" WARN: For board at VME addr=0x%x, Invalid Board ID: 0x%x\n",
 		     (UINT32)(laddr_inc - tdA24Offset), rdata);
	      continue;
	    }
	  else 
	    {
	      /* Check if this is board has a valid slot number */
	      boardID =  (rdata&TD_BOARDID_GEOADR_MASK)>>8;
	      if((boardID <= 0)||(boardID >21)) 
		{
		  printf(" WARN: Board Slot ID is not in range: %d (this module ignored)\n"
			 ,boardID);
		  continue;
		}
	      else
		{
		  TDp[boardID] = (struct TD_A24RegStruct *)(laddr_inc);
		  tdID[nTD] = boardID;

		  /* Get the Firmware Information and print out some details */
		  firmwareInfo = tdGetFirmwareVersion(tdID[nTD]);

		  if(firmwareInfo>0)
		    {
		      printf("  User ID: 0x%x \tFirmware (type - revision): %X - %x.%x\n",
			     (firmwareInfo&TD_FIRMWARE_ID_MASK)>>16, 
			     (firmwareInfo&TD_FIRMWARE_TYPE_MASK)>>12, 
			     (firmwareInfo&TD_FIRMWARE_MAJOR_VERSION_MASK)>>4, 
			     firmwareInfo&TD_FIRWMARE_MINOR_VERSION_MASK);

		      tdVersion = firmwareInfo&0xFFF;
		      tdType    = (firmwareInfo&TD_FIRMWARE_TYPE_MASK)>>12;
		      if((tdVersion < TD_SUPPORTED_FIRMWARE) || (tdType!=TD_SUPPORTED_TYPE))
			{
			  if(noFirmwareCheck)
			    {
			      printf("%s: WARN: Type %x Firmware version (0x%x) not supported by this driver.\n  Supported: Type %x version 0x%x (IGNORED)\n",
				     __FUNCTION__,
				     tdType,tdVersion,TD_SUPPORTED_TYPE,TD_SUPPORTED_FIRMWARE);
			    }
			  else
			    {
			      printf("%s: ERROR: Type %x Firmware version (0x%x) not supported by this driver.\n  Supported Type %x version 0x%x\n",
				     __FUNCTION__,
				     tdType,tdVersion,TD_SUPPORTED_TYPE,TD_SUPPORTED_FIRMWARE);
			      TDp[boardID]=NULL;
			      continue;
			    }
			}
		    }
		  else
		    {
		      printf("%s:  ERROR: Invalid firmware 0x%08x\n",
			     __FUNCTION__,firmwareInfo);
		      return ERROR;
		    }

		  printf("Initialized TD %2d  Slot # %2d at address 0x%08lx (0x%08x) \n",
			 nTD, tdID[nTD],(unsigned long) TDp[(tdID[nTD])],
			 (UINT32)((unsigned long)TDp[(tdID[nTD])]-tdA24Offset));
		}
	    }
	  nTD++;
	}
    }
 
#ifndef SHOWERROR
#ifndef VXWORKS
  vmeSetQuietFlag(0);
#endif
#endif

  tdInitPortNames();

  if(noBoardInit)
    {
      if(nTD>0)
	{
	  printf("%s: %d TD(s) successfully mapped (not initialized)\n",
		 __FUNCTION__,nTD);
	  return OK;
	}
    }

  if(nTD==0)
    {
      printf("%s: ERROR: Unable to initialize any TD modules\n",
	     __FUNCTION__);
      return ERROR;
    }

  /* Do some module initialization here */
  for(itd=0; itd<nTD; itd++)
    {
      TD_SLOT = tdID[itd];

      tdSetBlockLevel(TD_SLOT,1);
      tdSetBlockBufferLevel(TD_SLOT,1);
      tdSetTriggerSource(TD_SLOT, TD_TRIGSRC_P0 | TD_TRIGSRC_LOOPBACK);
      tdSetSyncSource(TD_SLOT, TD_SYNC_P0);

      tdSetBusySource(TD_SLOT,0,1);
      tdSetFiberMask(TD_SLOT,0xff);

      tdAutoAlignSync(TD_SLOT);

      tdTriggerReadyReset(TD_SLOT);

      /* MGT reset */
      tdResetMGT(TD_SLOT);

      /* Reset the Slave Mask */
      tdSlaveMask[TD_SLOT] = 0;
    }

  printf("%s: Found and configured %d TD modules\n",
	 __FUNCTION__,nTD);

  return OK;
}

/**
 *  @ingroup Status
 *  @brief Convert an index into a slot number, where the index is
 *          the element of an array of TDs in the order in which they were
 *          initialized.
 * @param i Integer referring to the order of which TDs were initialized
 *
 * @return Slot number if Successfull, otherwise ERROR.
 */
int
tdSlot(unsigned int i)
{
  if(i>=nTD)
    {
      printf("%s: ERROR: Index (%d) >= TDs initialized (%d).\n",
	     __FUNCTION__,i,nTD);
      return ERROR;
    }

  return tdID[i];
}

/**
 *  @ingroup Status
 *  @brief Return the VME Slot mask of initialized TDs
 *
 * @return Slot Mask if successful
 *
 */
unsigned int
tdSlotMask()
{
  int itd=0;
  unsigned int dmask=0;

  for(itd=0; itd<nTD; itd++)
    {
      dmask |= (1<<(tdID[itd]));
    }

  return dmask;
}

/**
 *  @ingroup Status
 *  @brief Test of register map 
 *
 *  @return OK
 *
 */
int
tdCheckAddresses()
{
  unsigned long offset=0, expected=0, base=0;
  struct TD_A24RegStruct *TDr;
  
  printf("%s:\n\t ---------- Checking TD address space ---------- \n",__FUNCTION__);

  base = (unsigned long) &TDr->boardID;

  offset = ((unsigned long) &TDr->trigsrc) - base;
  expected = 0x20;
  if(offset != expected)
    printf("%s: ERROR TDp->triggerSource not at offset = 0x%lx (@ 0x%lx)\n",
	   __FUNCTION__,expected,offset);

  offset = ((unsigned long) &TDr->syncWidth) - base;
  expected = 0x80;
  if(offset != expected)
    printf("%s: ERROR TDp->syncWidth not at offset = 0x%lx (@ 0x%lx)\n",
	   __FUNCTION__,expected,offset);
    
  offset = ((unsigned long) &TDr->adr24) - base;
  expected = 0xD0;
  if(offset != expected)
    printf("%s: ERROR TDp->adr24 not at offset = 0x%lx (@ 0x%lx)\n",
	   __FUNCTION__,expected,offset);
    
  offset = ((unsigned long) &TDr->reset) - base;
  expected = 0x100;
  if(offset != expected)
    printf("%s: ERROR TDp->reset not at offset = 0x%lx (@ 0x%lx)\n",
	   __FUNCTION__,expected,offset);
    
  offset = ((unsigned long) &TDr->fpBusy) - base;
  expected = 0x128;
  if(offset != expected)
    printf("%s: ERROR TDp->fpBusy not at offset = 0x%lx (@ 0x%lx)\n",
	   __FUNCTION__,expected,offset);
    
  offset = ((unsigned long) &TDr->trigTable[0]) - base;
  expected = 0x140;
  if(offset != expected)
    printf("%s: ERROR TDp->trigTable[0] not at offset = 0x%lx (@ 0x%lx)\n",
	   __FUNCTION__,expected,offset);
    
  offset = ((unsigned long) &TDr->master_tiID) - base;
  expected = 0x1F0;
  if(offset != expected)
    printf("%s: ERROR TDp->master_tiID not at offset = 0x%lx (@ 0x%lx)\n",
	   __FUNCTION__,expected,offset);
    
  offset = ((unsigned long) &TDr->JTAGPROMBase[0]) - base;
  expected = 0x10000;
  if(offset != expected)
    printf("%s: ERROR TDp->JTAGPROMBase[0] not at offset = 0x%lx (@ 0x%lx)\n",
	   __FUNCTION__,expected,offset);
    
  offset = ((unsigned long) &TDr->JTAGFPGABase[0]) - base;
  expected = 0x20000;
  if(offset != expected)
    printf("%s: ERROR TDp->JTAGFPGABase[0] not at offset = 0x%lx (@ 0x%lx)\n",
	   __FUNCTION__,expected,offset);
    
  return OK;
}

/**
 *  @ingroup Status
 *  @brief Print some status information of the TD to standard out
 *
 *  @param    pflag 
 *   - >0: print out raw registers
 *
 *   @return OK if successful, ERROR otherwise
 */
void
tdStatus(int id, int pflag)
{
  unsigned int boardID, fiber, intsetup, trigDelay;
  unsigned int adr32, blocklevel, vmeControl, trigsrc, sync;
  unsigned int busy, fiber_busy, blockBuffer;
  unsigned int tsInput;
  unsigned int output;
  unsigned int livetime, busytime;
  unsigned int inputCounter;
  unsigned int blockStatus[5], iblock, nblocksReady, nblocksNeedAck;
  unsigned int ifiber, fibermask;
  unsigned long TDBase;

  if(id==0) id=tdID[0];

  if((id<=0) || (id>21) || (TDp[id]==NULL)) /* sergey: check id range */
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return;
    }

  /* Latch live and busy timers */
  tdLatchTimers(id);

  TDLOCK;
  boardID      = vmeRead32(&TDp[id]->boardID);
  fiber        = vmeRead32(&TDp[id]->fiber);
  intsetup     = vmeRead32(&TDp[id]->intsetup);
  trigDelay    = vmeRead32(&TDp[id]->trigDelay);
  adr32        = vmeRead32(&TDp[id]->adr32);
  blocklevel   = vmeRead32(&TDp[id]->blocklevel);
  vmeControl   = vmeRead32(&TDp[id]->vmeControl);
  trigsrc      = vmeRead32(&TDp[id]->trigsrc);
  sync         = vmeRead32(&TDp[id]->sync);
  busy         = vmeRead32(&TDp[id]->busy);
  fiber_busy   = vmeRead32(&TDp[id]->fiber_busy);
  blockBuffer  = vmeRead32(&TDp[id]->blockBuffer);

  tsInput      = vmeRead32(&TDp[id]->tsInput);

  output       = vmeRead32(&TDp[id]->output);

  livetime     = vmeRead32(&TDp[id]->livetime);
  busytime     = vmeRead32(&TDp[id]->busytime);

  inputCounter = vmeRead32(&TDp[id]->trigCount);

  for(iblock=0;iblock<4;iblock++)
    blockStatus[iblock] = vmeRead32(&TDp[id]->blockStatus[iblock]);

  blockStatus[4] = vmeRead32(&TDp[id]->adr24);

  TDUNLOCK;

  TDBase = (unsigned long)TDp[id];

  printf("\n");
#ifdef VXWORKS
  printf("STATUS for TD %2d at base address 0x%08x \n",
	 id, (unsigned int) TDp[id]);
#else
  printf("STATUS for TD %2d at VME (USER) base address 0x%08x (0x%08lx) \n",
	 id, (unsigned int)((unsigned long) TDp[id] - tdA24Offset), (unsigned long) TDp[id]);
#endif
  printf("--------------------------------------------------------------------------------\n");

  if(pflag>0)
    {
      printf(" Registers (offset):\n");
      printf("  boardID        (0x%04lx) = 0x%08x\t", (unsigned long)(&TDp[id]->boardID) - TDBase, boardID);
      printf("  fiber          (0x%04lx) = 0x%08x\n", (unsigned long)(&TDp[id]->fiber) - TDBase, fiber);
      printf("  intsetup       (0x%04lx) = 0x%08x\t", (unsigned long)(&TDp[id]->intsetup) - TDBase, intsetup);
      printf("  trigDelay      (0x%04lx) = 0x%08x\n", (unsigned long)(&TDp[id]->trigDelay) - TDBase, trigDelay);
      printf("  adr32          (0x%04lx) = 0x%08x\t", (unsigned long)(&TDp[id]->adr32) - TDBase, adr32);
      printf("  vmeControl     (0x%04lx) = 0x%08x\n", (unsigned long)(&TDp[id]->vmeControl) - TDBase, vmeControl);
      printf("  trigsrc        (0x%04lx) = 0x%08x\t", (unsigned long)(&TDp[id]->trigsrc) - TDBase, trigsrc);
      printf("  sync           (0x%04lx) = 0x%08x\n", (unsigned long)(&TDp[id]->sync) - TDBase, sync);
      printf("  busy           (0x%04lx) = 0x%08x\t", (unsigned long)(&TDp[id]->busy) - TDBase, busy);
      printf("  blockBuffer    (0x%04lx) = 0x%08x\t", (unsigned long)(&TDp[id]->blockBuffer) - TDBase, blockBuffer);

      printf("  output         (0x%04lx) = 0x%08x\n", (unsigned long)(&TDp[id]->output) - TDBase, output);

      printf("  livetime       (0x%04lx) = 0x%08x\t", (unsigned long)(&TDp[id]->livetime) - TDBase, livetime);
      printf("  busytime       (0x%04lx) = 0x%08x\n", (unsigned long)(&TDp[id]->busytime) - TDBase, busytime);
    }
  printf("\n");

  printf(" Crate ID = 0x%02x\n",boardID&TD_BOARDID_CRATEID_MASK);
  printf(" Block size = %d\n",blocklevel & TD_BLOCKLEVEL_MASK);

  fibermask = fiber & TD_FIBER_MASK;
  if(fibermask)
    {
      printf(" HFBR enabled (0x%x)= \n",fibermask);
      for(ifiber=0; ifiber<8; ifiber++)
	{
	  if( fibermask & (1<<ifiber) ) 
	    printf("   %d: -%s-   -%s-\n",ifiber+1,
		   (fiber & TD_FIBER_CONNECTED_TI(ifiber+1))?"    CONNECTED":"NOT CONNECTED",
		   (fiber & TD_FIBER_TRIGSRC_ENABLED_TI(ifiber+1))?"TRIGSRC ENABLED":"TRIGSRC DISABLED");
	}
      printf("\n");
    }
  else
    printf(" All HFBR Disabled\n");

  if(tdSlaveMask[id])
    {
      printf(" TI Slaves Configured on HFBR (0x%x) = ",tdSlaveMask[id]);
      fibermask = tdSlaveMask[id];
      for(ifiber=0; ifiber<8; ifiber++)
	{
	  if( fibermask & (1<<ifiber)) 
	    printf(" %d",ifiber+1);
	}
      printf("\n");	
    }
  else
    printf(" No TD Slaves Configured on HFBR\n");
      
  if(trigsrc&TD_TRIGSRC_SOURCEMASK)
    {
      if(trigsrc)
	printf(" Trigger input source (ENABLED) =\n");
      else
	printf(" Trigger input source (DISABLED) =\n");
      if(trigsrc & TD_TRIGSRC_P0)
	printf("   P0 Input\n");
      if(trigsrc & TD_TRIGSRC_HFBR1)
	printf("   HFBR #1 Input\n");
      if(trigsrc & TD_TRIGSRC_LOOPBACK)
	printf("   Loopback\n");
      if(trigsrc & TD_TRIGSRC_FPTRG)
	printf("   Front Panel TRG\n");
      if(trigsrc & TD_TRIGSRC_VME)
	printf("   VME Command\n");
      if(trigsrc & TD_TRIGSRC_TSINPUTS)
	printf("   Front Panel TS Inputs\n");
      if(trigsrc & TD_TRIGSRC_TSREV2)
	printf("   Trigger Supervisor (rev2)\n");
      if(trigsrc & TD_TRIGSRC_PULSER)
	printf("   Internal Pulser\n");
      if(trigsrc & TD_TRIGSRC_ENABLE)
	printf("   FP/Ext/GTP\n");
    }
  else 
    {
      printf(" No Trigger input sources\n");
    }

  if(sync&TD_SYNC_SOURCEMASK)
    {
      printf(" Sync source = \n");
      if(sync & TD_SYNC_P0)
	printf("   P0 Input\n");
      if(sync & TD_SYNC_HFBR1)
	printf("   HFBR #1 Input\n");
      if(sync & TD_SYNC_HFBR5)
	printf("   HFBR #5 Input\n");
      if(sync & TD_SYNC_FP)
	printf("   Front Panel Input\n");
      if(sync & TD_SYNC_LOOPBACK)
	printf("   Loopback\n");
    }
  else
    {
      printf(" No SYNC input source configured\n");
    }

  if(busy&TD_BUSY_SOURCEMASK)
    {
      printf(" BUSY input source = \n");
      if(busy & TD_BUSY_P2)
	printf("   P2 Input         %s\n",(busy&TD_BUSY_MONITOR_P2)?"** BUSY **":"");
      if(busy & TD_BUSY_TRIGGER_LOCK)
	printf("   Trigger Lock     \n");
      if(busy & TD_BUSY_LOOPBACK)
	printf("   Loopback         %s\n",(busy&TD_BUSY_MONITOR_LOOPBACK)?"** BUSY **":"");
      if(busy & TD_BUSY_HFBR1)
	printf("   HFBR #1          %s %s\n",
	       (busy&TD_BUSY_MONITOR_HFBR1)?"** BUSY **":"",
	       (fiber_busy & TD_FIBER_BUSY_HFBR1)?"(Fiber)":"(Block Ack)");

      if(busy & TD_BUSY_HFBR2)
	printf("   HFBR #2          %s %s\n",
	       (busy&TD_BUSY_MONITOR_HFBR2)?"** BUSY **":"",
	       (fiber_busy & TD_FIBER_BUSY_HFBR2)?"(Fiber)":"(Block Ack)");

      if(busy & TD_BUSY_HFBR3)
	printf("   HFBR #3          %s %s\n",
	       (busy&TD_BUSY_MONITOR_HFBR3)?"** BUSY **":"",
	       (fiber_busy & TD_FIBER_BUSY_HFBR3)?"(Fiber)":"(Block Ack)");

      if(busy & TD_BUSY_HFBR4)
	printf("   HFBR #4          %s %s\n",
	       (busy&TD_BUSY_MONITOR_HFBR4)?"** BUSY **":"",
	       (fiber_busy & TD_FIBER_BUSY_HFBR4)?"(Fiber)":"(Block Ack)");

      if(busy & TD_BUSY_HFBR5)
	printf("   HFBR #5          %s %s\n",
	       (busy&TD_BUSY_MONITOR_HFBR5)?"** BUSY **":"",
	       (fiber_busy & TD_FIBER_BUSY_HFBR5)?"(Fiber)":"(Block Ack)");

      if(busy & TD_BUSY_HFBR6)
	printf("   HFBR #6          %s %s\n",
	       (busy&TD_BUSY_MONITOR_HFBR6)?"** BUSY **":"",
	       (fiber_busy & TD_FIBER_BUSY_HFBR6)?"(Fiber)":"(Block Ack)");

      if(busy & TD_BUSY_HFBR7)
	printf("   HFBR #7          %s %s\n",
	       (busy&TD_BUSY_MONITOR_HFBR7)?"** BUSY **":"",
	       (fiber_busy & TD_FIBER_BUSY_HFBR7)?"(Fiber)":"(Block Ack)");

      if(busy & TD_BUSY_HFBR8)
	printf("   HFBR #8          %s %s\n",
	       (busy&TD_BUSY_MONITOR_HFBR8)?"** BUSY **":"",
	       (fiber_busy & TD_FIBER_BUSY_HFBR8)?"(Fiber)":"(Block Ack)");

    }
  else
    {
      printf(" No BUSY input source configured\n");
    }

  printf(" Slave Block Status:   %s\n",(busy&TD_BUSY_MONITOR_TRIG_LOST)?"** Waiting for Trigger Ack **":"");

  /* TD slave block status */
  fibermask = tdSlaveMask[id];
  for(ifiber=0; ifiber<8; ifiber++)
    {
      if( fibermask & (1<<ifiber) )
	{
	  if( (ifiber % 2) == 0)
	    {
	      nblocksReady   = blockStatus[ifiber/2] & TD_BLOCKSTATUS_NBLOCKS_READY0;
	      nblocksNeedAck = (blockStatus[ifiber/2] & TD_BLOCKSTATUS_NBLOCKS_NEEDACK0)>>8;
	    }
	  else
	    {
	      nblocksReady   = (blockStatus[(ifiber-1)/2] & TD_BLOCKSTATUS_NBLOCKS_READY1)>>16;
	      nblocksNeedAck = (blockStatus[(ifiber-1)/2] & TD_BLOCKSTATUS_NBLOCKS_NEEDACK1)>>24;
	    }
	  printf("  Fiber %d  :  Blocks ready / need acknowledge: %d / %d\n",
		 ifiber+1,nblocksReady, nblocksNeedAck);
	}
    }

  printf(" Trigger counter =  %d\n",inputCounter);

  printf("--------------------------------------------------------------------------------\n");
  printf("\n\n");

}

static
char portName[TD_MAX_VME_SLOTS][TD_MAX_FIBER_PORTS+1][TD_MAX_PORTNAME_CHARS];

/**
 *  @ingroup Config
 *  @brief Initialize portName array with default names in the form:
 *     Slot%2d - Port%d
 *   - This routine is called in tdInit(...)
 */
void
tdInitPortNames()
{
  int islot=0, iport=0;

  for(islot=0; islot<TD_MAX_VME_SLOTS; islot++)
    {
      switch(islot)
	{
	case 0: case 1: case 11: case 12:
	  for(iport=0; iport<TD_MAX_FIBER_PORTS+1; iport++)
	    {
	      snprintf(portName[islot][iport],sizeof(portName[islot][iport]),
		       "none");
	    }
	  break;

	default:
	  for(iport=1; iport<TD_MAX_FIBER_PORTS+1; iport++)
	    {
	      snprintf(portName[islot][iport],sizeof(portName[islot][iport]),
		       "Slot%d - Port%d",islot,iport);
	    }
	  snprintf(portName[islot][0],sizeof(portName[islot][iport]),
		   "none");
	}
    }

}

/**
 *  @ingroup Config
 *  @brief Rename a specified port to a specified 20 character string
 *  @param id Slot number
 *  @param iport Port Number
 *  @param name String to assign to port
 *
 *  @return OK if successful, otherwise ERROR.
 */
int
tdSetPortName(int id, int iport, char *name)
{

  if(id==0) id=tdID[0];

  if(id>TD_MAX_VME_SLOTS)
    {
      printf("%s: ERROR: Invalid Slot Number (%d)\n",
	     __FUNCTION__,id);
      return ERROR;
    }

  if(iport>TD_MAX_FIBER_PORTS)
    {
      printf("%s: ERROR: Invalid Port Number (%d)\n",
	     __FUNCTION__,iport);
      return ERROR;
    }

  if(name!=NULL)
    {
      if(strlen(name)>TD_MAX_PORTNAME_CHARS)
	{
	  printf("%s: WARN: Truncating name (size = %d)\n",__FUNCTION__,strlen(name));
	}
      strncpy(portName[id][iport],name,TD_MAX_PORTNAME_CHARS);

    }
  else
    {
      printf("%s(%d): ERROR: Invalid input name\n",__FUNCTION__,id);
      return ERROR;
    }

  return OK;
}

/**
 *  @ingroup Status
 *  @brief Get the current specified port name
 *  @param id Slot Number
 *  @param iport Port Number
 *  @param name Where to return port name
 *
 *  @return OK if successful, otherwise ERROR.
 */
int
tdGetPortName(int id, int iport, char **name)
{
  if(id==0) id=tdID[0];

  if(iport>TD_MAX_FIBER_PORTS)
    {
      printf("%s: ERROR: Invalid port number (%d)\n",
	     __FUNCTION__,iport);
      return ERROR;
    }

  if(name!=NULL)
    {
      strncpy((char *)name, portName[id][iport], TD_MAX_PORTNAME_CHARS);
    }
  else
    {
      printf("%s: Invalid pointer to return name\n",
	     __FUNCTION__);
      return ERROR;
    }

  return OK;
}

/**
 *  @ingroup Status
 *  @brief Save the current port names to a file (with file name: filename)
 *
 *     Format is the same which is expected by 
 *     tdLoadPortNames(...)
 *  @param filename Name of output file
 *
 *  @return OK if successful, otherwise ERROR.
 */
int
tdSavePortNames(char *filename)
{
  FILE *outFile;
  int islot=0, iport=0;
  char name[TD_MAX_PORTNAME_CHARS+1];

  outFile = fopen(filename,"w");
  if(!outFile)
    {
      printf("%s: ERROR: Unable to open %s for writting\n",
	     __FUNCTION__,filename);
      return ERROR;
    }

  for(islot=0; islot<TD_MAX_VME_SLOTS; islot++)
    {
      switch(islot)
	{
	case 0: case 1: case 11: case 12:
	  break;

	default:
	  for(iport=1; iport<TD_MAX_FIBER_PORTS+1; iport++)
	    {
	      strncpy(name,portName[islot][iport],TD_MAX_PORTNAME_CHARS);
	      if(strcmp(name,"none")!=0)
		fprintf(outFile,"%2d %d %s\n",islot,iport,name);
	    }
	}
    }
  fclose(outFile);
  
  return OK;
}

/**
 *  @ingroup Config
 *  @brief Load the Port Names as specified in a file.
 *
 *     Format must be:
 *     Slot Port Name 
 *     e.g.
 *      3 2 ROC5
 *      3 3 ROC10
 *
 *  @param filename Name of input file
 *  @return OK if successful, otherwise ERROR.
 */
int
tdLoadPortNames(char *filename)
{
  FILE *inFile;
  int islot=0, iport=0;
  char name[TD_MAX_PORTNAME_CHARS+1];
  char line[200];

  inFile = fopen(filename,"r");
  if(!inFile)
    {
      printf("%s: ERROR: Unable to open %s for reading\n",
	     __FUNCTION__,filename);
      return ERROR;
    }

  while(!feof(inFile))
    {
      /* Get the current line */
      if(!fgets(line, sizeof(line), inFile))
	break;
      
      if(sscanf(line,"%2d %d %[^\n]",&islot,&iport,(char *)&name)==3)
	{
	  switch(islot)
	    {
	    default:
	      tdSetPortName(islot,iport,name);
	    }
	}
      
    }

  fclose(inFile);

  return OK;
}

/**
 *  @ingroup Status
 *  @brief Print the current port names to standard out.
 *
 */
void
tdPrintPortNames()
{
  int islot, iport;
  char name[TD_MAX_PORTNAME_CHARS];

  printf("Slot - Port : Name\n");
  for(islot=0; islot<TD_MAX_VME_SLOTS; islot++)
    {
      switch(islot)
	{
	default:
	  for(iport=0; iport<TD_MAX_FIBER_PORTS+1; iport++)
	    {
	      strncpy(name,portName[islot][iport],TD_MAX_PORTNAME_CHARS);
	      printf(" %2d - %d   : %s\n",islot,iport,name);
	    }
	}
    }
}


/**
 *  @ingroup Status
 *  @brief Print a summary of all configured TD modules and ports to standard out
 *
 *  @param    pflag
 *   - 0:  Print module and port info
 *   - 1:  Module info only
 *   - 2:  Port Info only
 *
 */
/*sergey: change type to 'int'; if pflag >0, it will use it as desired block_level;
 if at least one ROC has different one it will return (-1), otherwise return (0) */
int
tdGStatus(int pflag)
{
  int itd=0, id=0, iport=0, ifiber=0, iblock=0;
  unsigned int fiber[TD_MAX_VME_SLOTS];
  unsigned int trigsrc[TD_MAX_VME_SLOTS], sync[TD_MAX_VME_SLOTS];
  unsigned int busy[TD_MAX_VME_SLOTS], fiber_busy[TD_MAX_VME_SLOTS], blockBuffer[TD_MAX_VME_SLOTS];
  unsigned int livetime[TD_MAX_VME_SLOTS], busytime[TD_MAX_VME_SLOTS];
  unsigned int inputCounter[TD_MAX_VME_SLOTS];
  unsigned int blockStatus[TD_MAX_VME_SLOTS][5], nblocksReady, nblocksNeedAck;
  unsigned int hfbr_tiID[TD_MAX_VME_SLOTS][8];
  unsigned int fibermask;
  int slaveCount=0;
  int ret = 0;
  int block_level;

  /*
  if((pflag<0)|(pflag>2))
    {
      printf("%s: ERROR: Invalid pflag (%d)\n",__FUNCTION__,pflag);
      return;
    pflag = 0;
    }
  */

  /* Grab all of the register info we need */
  TDLOCK;
  for(itd=0; itd<nTD; itd++)
    {
      id = tdSlot(itd);

      fiber[id]    = vmeRead32(&TDp[id]->fiber);
      trigsrc[id]  = vmeRead32(&TDp[id]->trigsrc);
      sync[id]         = vmeRead32(&TDp[id]->sync);
      busy[id]         = vmeRead32(&TDp[id]->busy);
      fiber_busy[id]   = vmeRead32(&TDp[id]->fiber_busy);
      blockBuffer[id]  = vmeRead32(&TDp[id]->blockBuffer);

      livetime[id]     = vmeRead32(&TDp[id]->livetime);
      busytime[id]     = vmeRead32(&TDp[id]->busytime);

      inputCounter[id] = vmeRead32(&TDp[id]->trigCount);

      for(iblock=0;iblock<4;iblock++)
	blockStatus[id][iblock] = vmeRead32(&TDp[id]->blockStatus[iblock]);

      blockStatus[id][4] = vmeRead32(&TDp[id]->adr24);
      for(iport=0; iport<8; iport++)
	{
	  hfbr_tiID[id][iport] = vmeRead32(&TDp[id]->hfbr_tiID[iport]);
	}
    }
  TDUNLOCK;

  printf("\n");

  /*if((pflag==0) || (pflag==1))*/
    {
      printf("TD Module Status Summary\n");

/*       printf("              Ports          SyncSrc    BusySrc    Busy Status  Waiting For  \n"); */
/*       printf("Slot  (Connected, TrigSrcEn) (V15FL) (PLT12345678) (PL12345678) Trigger Ack\n"); */
      printf("              Ports          SyncSrc    BusySrc    Busy Status   Triggers TrgAck\n");
      printf("Slot  (Connected, TrigSrcEn) (V15FL) (PLT12345678) (PL12345678)  Received  Wait\n");
      printf("--------------------------------------------------------------------------------\n");
      for(itd=0; itd<nTD; itd++)
	{
	  id = tdSlot(itd);
	  fibermask = fiber[id] & TD_FIBER_MASK;
      
	  /* Slot */
	  printf("%2d    ",id);

	  /* Ports Connected */
	  printf(" %s%s%s%s%s%s%s%s    ",
		 (fiber[id] & TD_FIBER_CONNECTED_TI(1))?"1":"-",
		 (fiber[id] & TD_FIBER_CONNECTED_TI(2))?"2":"-",
		 (fiber[id] & TD_FIBER_CONNECTED_TI(3))?"3":"-",
		 (fiber[id] & TD_FIBER_CONNECTED_TI(4))?"4":"-",
		 (fiber[id] & TD_FIBER_CONNECTED_TI(5))?"5":"-",
		 (fiber[id] & TD_FIBER_CONNECTED_TI(6))?"6":"-",
		 (fiber[id] & TD_FIBER_CONNECTED_TI(7))?"7":"-",
		 (fiber[id] & TD_FIBER_CONNECTED_TI(8))?"8":"-");

	  /* TrigSrc Enabled */
	  printf("%s%s%s%s%s%s%s%s   ",
		 (fiber[id] & TD_FIBER_TRIGSRC_ENABLED_TI(1))?"1":"-",
		 (fiber[id] & TD_FIBER_TRIGSRC_ENABLED_TI(2))?"2":"-",
		 (fiber[id] & TD_FIBER_TRIGSRC_ENABLED_TI(3))?"3":"-",
		 (fiber[id] & TD_FIBER_TRIGSRC_ENABLED_TI(4))?"4":"-",
		 (fiber[id] & TD_FIBER_TRIGSRC_ENABLED_TI(5))?"5":"-",
		 (fiber[id] & TD_FIBER_TRIGSRC_ENABLED_TI(6))?"6":"-",
		 (fiber[id] & TD_FIBER_TRIGSRC_ENABLED_TI(7))?"7":"-",
		 (fiber[id] & TD_FIBER_TRIGSRC_ENABLED_TI(8))?"8":"-");

	  /* Sync Source */
	  printf("%s%s%s%s%s   ",
		 (sync[id] & TD_SYNC_P0)?"V":"-",
		 (sync[id] & TD_SYNC_HFBR1)?"1":"-",
		 (sync[id] & TD_SYNC_HFBR5)?"5":"-",
		 (sync[id] & TD_SYNC_FP)?"F":"-",
		 (sync[id] & TD_SYNC_LOOPBACK)?"L":"-");

	  /* Busy Source */
	  printf("%s%s%s%s%s%s%s%s%s%s%s   ",
		 (busy[id] & TD_BUSY_P2)?"P":"-",
		 (busy[id] & TD_BUSY_LOOPBACK)?"L":"-",
		 (busy[id] & TD_BUSY_TRIGGER_LOCK)?"T":"-",
		 (busy[id] & TD_BUSY_HFBR1)?"1":"-",
		 (busy[id] & TD_BUSY_HFBR2)?"2":"-",
		 (busy[id] & TD_BUSY_HFBR3)?"3":"-",
		 (busy[id] & TD_BUSY_HFBR4)?"4":"-",
		 (busy[id] & TD_BUSY_HFBR5)?"5":"-",
		 (busy[id] & TD_BUSY_HFBR6)?"6":"-",
		 (busy[id] & TD_BUSY_HFBR7)?"7":"-",
		 (busy[id] & TD_BUSY_HFBR8)?"8":"-");

	  /* Busy Status */
	  printf("%s%s%s%s%s%s%s%s%s%s  ",
		 (busy[id] & (TD_BUSY_P2 | TD_BUSY_MONITOR_P2))==(TD_BUSY_P2 | TD_BUSY_MONITOR_P2)?"P":"-",
		 (busy[id] & (TD_BUSY_LOOPBACK | TD_BUSY_MONITOR_LOOPBACK))==(TD_BUSY_LOOPBACK | TD_BUSY_MONITOR_LOOPBACK)?"L":"-",
		 (busy[id] & (TD_BUSY_HFBR1 | TD_BUSY_MONITOR_HFBR1))==(TD_BUSY_HFBR1 | TD_BUSY_MONITOR_HFBR1)?"1":"-",
		 (busy[id] & (TD_BUSY_HFBR2 | TD_BUSY_MONITOR_HFBR2))==(TD_BUSY_HFBR2 | TD_BUSY_MONITOR_HFBR2)?"2":"-",
		 (busy[id] & (TD_BUSY_HFBR3 | TD_BUSY_MONITOR_HFBR3))==(TD_BUSY_HFBR3 | TD_BUSY_MONITOR_HFBR3)?"3":"-",
		 (busy[id] & (TD_BUSY_HFBR4 | TD_BUSY_MONITOR_HFBR4))==(TD_BUSY_HFBR4 | TD_BUSY_MONITOR_HFBR4)?"4":"-",
		 (busy[id] & (TD_BUSY_HFBR5 | TD_BUSY_MONITOR_HFBR5))==(TD_BUSY_HFBR5 | TD_BUSY_MONITOR_HFBR5)?"5":"-",
		 (busy[id] & (TD_BUSY_HFBR6 | TD_BUSY_MONITOR_HFBR6))==(TD_BUSY_HFBR6 | TD_BUSY_MONITOR_HFBR6)?"6":"-",
		 (busy[id] & (TD_BUSY_HFBR7 | TD_BUSY_MONITOR_HFBR7))==(TD_BUSY_HFBR7 | TD_BUSY_MONITOR_HFBR7)?"7":"-",
		 (busy[id] & (TD_BUSY_HFBR8 | TD_BUSY_MONITOR_HFBR8))==(TD_BUSY_HFBR8 | TD_BUSY_MONITOR_HFBR8)?"8":"-");

	  printf("0x%08x   ",
		 inputCounter[id]);

	  printf("%s",
		 (busy[id] & (TD_BUSY_MONITOR_TRIG_LOST))?"Y":"N");

	  printf("\n");
	}


      printf("\n");
    }

	/*if((pflag==0) || (pflag==2))*/
    {
      printf("TD Port STATUS Summary\n");
      printf("                                                      Block Status      Block  Buffer\n");
      printf("Sl-P  RocName  COMM  ROCID  TrigSrcEn   Busy Status  Ready / NeedAck    Level   Level\n");
      printf("--------------------------------------------------------------------------------\n");
      for(itd=0; itd<nTD; itd++)
	{
	  id = tdSlot(itd);
	  for(iport=1; iport<TD_MAX_FIBER_PORTS+1; iport++)
	    {
	      /* Only continue if this port has been configured as a slave */
	      if((tdSlaveMask[id] & (1<<(iport-1)))==0) continue;

	      /* Slot and Port number */
	      printf("%2d-%d ", id, iport);

		  /*sergey*/
          printf("%9.9s ",portName[id][iport]);
          /*sergey*/

	      /* Connection Status */
	      printf("%s   ",
		     (fiber[id] & TD_FIBER_CONNECTED_TI(iport))?" UP ":"DOWN");

	      if(!(fiber[id] & TD_FIBER_CONNECTED_TI(iport)))
		{
		  printf("\n");
		  continue;
		}
	      
	      /* ROCID */
	      printf("%3d    ",
		     (hfbr_tiID[id][iport-1]&TD_ID_CRATEID_MASK)>>8);
	      
	      printf("%s   ",
		     (fiber[id] & TD_FIBER_TRIGSRC_ENABLED_TI(iport))?" ENABLED":"DISABLED");

	      /* Busy Status */
	      printf("%s ",
		     (busy[id] & TD_BUSY_MONITOR_FIBER_BUSY(iport))?
		     ((fiber_busy[id] & TD_FIBER_BUSY(iport))?"BUSY-Fiber":"BUSY-BlkAck")
		     :"           ");

	      /* Block Status */
	      ifiber=iport-1;
	      if( (ifiber % 2) == 0)
		{
		  nblocksReady   = blockStatus[id][ifiber/2] & TD_BLOCKSTATUS_NBLOCKS_READY0;
		  nblocksNeedAck = (blockStatus[id][ifiber/2] & TD_BLOCKSTATUS_NBLOCKS_NEEDACK0)>>8;
		}
	      else
		{
		  nblocksReady   = (blockStatus[id][(ifiber-1)/2] & TD_BLOCKSTATUS_NBLOCKS_READY1)>>16;
		  nblocksNeedAck = (blockStatus[id][(ifiber-1)/2] & TD_BLOCKSTATUS_NBLOCKS_NEEDACK1)>>24;
		}
	      printf("   %3d / %3d",nblocksReady, nblocksNeedAck);
	  
          block_level = (hfbr_tiID[id][iport-1]&TD_ID_BLOCKLEVEL_MASK)>>16;
	      printf("         %3d",
		     block_level);

          if((pflag>0)&&(pflag!=block_level))
		  {
            ret = -1;
            printf("<-");
		  }
          else
		  {
            printf("  ");
		  }

	      printf("   %3d",
		     (hfbr_tiID[id][iport-1]&TD_ID_BLOCK_BUFFERLEVEL_MASK)>>24);

	      printf("\n");
	      slaveCount++;
	    }
	}
      printf("\n");
      printf("Total Slaves Added = %d\n",slaveCount);

    }

  printf("--------------------------------------------------------------------------------\n");
  printf("\n");

  return(ret);
}


/**
 *  @ingroup Config
 *  @brief Set the number of events per block
 *  @param id Slot number
 *  @param blockLevel Block Level
 *  @return OK if successful, ERROR otherwise
 */
int
tdSetBlockLevel(int id, unsigned int blockLevel)
{
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  if( (blockLevel>TD_BLOCKLEVEL_MASK) || (blockLevel==0) )
    {
      printf("%s: ERROR: Invalid Block Level (%d)\n",__FUNCTION__,blockLevel);
      return ERROR;
    }

  TDLOCK;
  vmeWrite32(&TDp[id]->blocklevel, blockLevel);
  TDUNLOCK;

  return OK;

}

/**
 *  @ingroup Config
 *  @brief Set the block level for all initialized TDs
 *  @param blockLevel Block Level
 *  @return OK if successful, ERROR otherwise
 */
int
tdGSetBlockLevel(unsigned int blockLevel)
{
  int res=OK, itd;
  
  for(itd=0; itd<nTD; itd++)
    res |= tdSetBlockLevel(tdID[itd], blockLevel);

  return res;
}

/**
 *  @ingroup Config
 *  @brief Set the block buffer level for the number of blocks in the system
 *     that need to be read out.
 *
 *     If this buffer level is full, the TD will go BUSY.
 *     The BUSY is released as soon as the number of buffers in the system
 *     drops below this level.
 *
 *  @param id Slot number
 *  @param level
 *    - 0:  No Buffer Limit  -  Pipeline mode
 *    - 1:  One Block Limit - "ROC LOCK" mode
 *    - 2-65535:  "Buffered" mode.
 *
 * @return OK if successful, otherwise ERROR
 */
int
tdSetBlockBufferLevel(int id, unsigned int level)
{
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  if(level>TD_BLOCKBUFFER_BUFFERLEVEL_MASK)
    {
      printf("%s: ERROR: Invalid value for level (%d)\n",
	     __FUNCTION__,level);
      return ERROR;
    }

  TDLOCK;
  vmeWrite32(&TDp[id]->blockBuffer, level);
  TDUNLOCK;

  return OK;
}

/**
 *  @ingroup Config
 *  @brief Set the block buffer level for the number of blocks in the system
 *     that need to be read out for all initialized TDs.
 *
 *     If this buffer level is full, the TD will go BUSY.
 *     The BUSY is released as soon as the number of buffers in the system
 *     drops below this level.
 *
 *  @param level
 *    - 0:  No Buffer Limit  -  Pipeline mode
 *    - 1:  One Block Limit - "ROC LOCK" mode
 *    - 2-65535:  "Buffered" mode.
 *
 * @return OK if successful, otherwise ERROR
 */
int
tdGSetBlockBufferLevel(unsigned int level)
{
  int res=OK, itd;
  
  for(itd=0; itd<nTD; itd++)
    res |= tdSetBlockBufferLevel(tdID[itd], level);

  return res;
}


/**
 *  @ingroup Status
 *  @brief Get the Firmware Version
 *  @param id Slot number
 *
 *  @return Firmware Version if successful, ERROR otherwise
 */
int
tdGetFirmwareVersion(int id)
{
  unsigned int rval=0;

  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  /* reset the VME_to_JTAG engine logic */
  vmeWrite32(&TDp[id]->reset,TD_RESET_JTAG);

  /* Reset FPGA JTAG to "reset_idle" state */
  vmeWrite32(&TDp[id]->JTAGFPGABase[(0x003C)>>2],0);

  /* enable the user_code readback */
  vmeWrite32(&TDp[id]->JTAGFPGABase[(0x092C)>>2],0x3c8);

  /* shift in 32-bit to FPGA JTAG */
  vmeWrite32(&TDp[id]->JTAGFPGABase[(0x1F1C)>>2],0);
  
  /* Readback the firmware version */
  rval = vmeRead32(&TDp[id]->JTAGFPGABase[(0x1F1C)>>2]);
  TDUNLOCK;

  return rval;
}

/**
 *  @ingroup Config
 *  @brief Enable Fiber transceiver
 *
 *  Note:  All Fiber are enabled by default 
 *         (no harm, except for 1-2W power usage)
 *
 *
 *  @param id Slot number
 *  @param  fiber integer indicative of the transceiver to enable
 *
 * @return OK if successful, ERROR otherwise.
 *
 */
int
tdEnableFiber(int id, unsigned int fiber)
{
  unsigned int sval;
  unsigned int fiberbit;

  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  if((fiber<1) | (fiber>8))
    {
      printf("%s: ERROR: Invalid value for fiber (%d)\n",
	     __FUNCTION__,fiber);
      return ERROR;
    }

  fiberbit = (1<<(fiber-1));

  TDLOCK;
  sval = vmeRead32(&TDp[id]->fiber);
  vmeWrite32(&TDp[id]->fiber,
	     sval | fiberbit );
  TDUNLOCK;

  return OK;
  
}


/**
 *  @ingroup Config
 *  @brief Disable Fiber transceiver
 *
 *  @param id Slot number
 *  @param  fiber integer indicative of the transceiver to disable
 *
 * @return OK if successful, ERROR otherwise.
 *
 */
int
tdDisableFiber(int id, unsigned int fiber)
{
  unsigned int rval;
  unsigned int fiberbit;

  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  if((fiber<1) | (fiber>8))
    {
      printf("%s: ERROR: Invalid value for fiber (%d)\n",
	     __FUNCTION__,fiber);
      return ERROR;
    }

  fiberbit = (1<<(fiber-1));

  TDLOCK;
  rval = vmeRead32(&TDp[id]->fiber);
  vmeWrite32(&TDp[id]->fiber,
	   rval & ~fiberbit );
  TDUNLOCK;

  return rval;
  
}

/**
 *  @ingroup Config
 *  @brief Enable/Disable fiber ports according to specified mask
 *  @param id Slot number
 *  @param fibermask mask indicative of the transceivers to enable/disable
 *
 * @return OK if successful, ERROR otherwise.
 */
int
tdSetFiberMask(int id, unsigned int fibermask)
{
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  if(fibermask>0xFF)
    {
      printf("%s: ERROR: Invalid value for fibermask (%d)\n",
	     __FUNCTION__,fibermask);
      return ERROR;
    }

  TDLOCK;
  vmeWrite32(&TDp[id]->fiber, fibermask);
  TDUNLOCK;

  return OK;
}


/**
 *  @ingroup Config
 *  @brief Set the busy source with a specified sourcemask
 *
 *  @param id Slot number
 *  @param  sourcemask bits: 
 *   - N: FIXME: FILL THIS IN
 *
 *  @param  rFlag - decision to reset the global source flags
 *    - 0: Keep prior busy source settings and set new "sourcemask"
 *    - 1: Reset, using only that specified with "sourcemask"
 * @return OK if successful, ERROR otherwise.
*/
int
tdSetBusySource(int id, unsigned int sourcemask, int rFlag)
{
  unsigned int busybits=0;

  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }
  
  if(sourcemask>TD_BUSY_SOURCEMASK)
    {
      printf("%s: ERROR: Invalid value for sourcemask (0x%x)\n",
	     __FUNCTION__, sourcemask);
      return ERROR;
    }

  TDLOCK;
  if(rFlag)
    {
      /* Read in the previous value , resetting previous BUSYs*/
      busybits = vmeRead32(&TDp[id]->busy) & ~(TD_BUSY_SOURCEMASK);
    }
  else
    {
      /* Read in the previous value , keeping previous BUSYs*/
      busybits = vmeRead32(&TDp[id]->busy);
    }

  busybits |= sourcemask;

  vmeWrite32(&TDp[id]->busy, busybits);
  TDUNLOCK;

  return OK;

}

/**
 *  @ingroup Config
 *  @brief Set the the trigger lock mode for the specified TD
 *
 *  @param id Slot number
 *  @param enable Enable flag
 *      0: Disable
 *     !0: Enable
 *
 * @return OK if successful, ERROR otherwise.
 */
int
tdSetTriggerLock(int id, int enable)
{
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  if(enable)
    vmeWrite32(&TDp[id]->busy,
	       vmeRead32(&TDp[id]->busy) | TD_BUSY_TRIGGER_LOCK);
  else
    vmeWrite32(&TDp[id]->busy,
	       vmeRead32(&TDp[id]->busy) & ~TD_BUSY_TRIGGER_LOCK);
  TDUNLOCK;

  return OK;
}

/**
 *  @ingroup Config
 *  @brief Set the the trigger lock mode for all initialized TDs
 *
 *  @param enable Enable flag
 *      0: Disable
 *     !0: Enable
 *
 * @return OK if successful, ERROR otherwise.
 */
int
tdGSetTriggerLock(int enable)
{
  int itd=0, id=0;

  TDLOCK;
  for(itd=0; itd<nTD; itd++)
    {
      id = tdSlot(itd);
      if(enable)
	vmeWrite32(&TDp[id]->busy,
		   vmeRead32(&TDp[id]->busy) | TD_BUSY_TRIGGER_LOCK);
      else
	vmeWrite32(&TDp[id]->busy,
		   vmeRead32(&TDp[id]->busy) & ~TD_BUSY_TRIGGER_LOCK);
    }
  TDUNLOCK;

  return OK;
}

/**
 *  @ingroup Config
 *  @brief Get the current setting of the trigger lock mode for the specified TD
 *
 *  @param id Slot number
 *
 * @return 1 if enabled, 0 if disabled, ERROR otherwise.
*/
int
tdGetTriggerLock(int id)
{
  int rval=0;
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  rval = (vmeRead32(&TDp[id]->busy) & TD_BUSY_TRIGGER_LOCK)>>6;
  TDUNLOCK;

  return rval;
}


/**
 *  @ingroup Status
 *  @brief Set the Sync source mask
 *
 *  @param id Slot number
 *  @param sync - MASK indicating the sync source
 *    - 0: P0
 *    - 1: HFBR1
 *    - 2: HFBR5
 *    - 3: FP
 *    - 4: LOOPBACK
 *
 * @return OK if successful, ERROR otherwise
 *
 */
int
tdSetSyncSource(int id, unsigned int sync)
{
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  if(sync>TD_SYNC_SOURCEMASK)
    {
      printf("%s: ERROR: Invalid Sync Source Mask (%d).\n",
	     __FUNCTION__,sync);
      return ERROR;
    }

  TDLOCK;
  vmeWrite32(&TDp[id]->sync,sync);
  TDUNLOCK;

  return OK;
}


/**
 *  @ingroup Config
 *  @brief Set trigger sources with specified trigmask
 *
 *  @param id Slot number
 *  @param trigmask bits:  
 *    - 0:  P0
 *    - 1:  HFBR #1 
 *    - 2:  TI Master Loopback
 *    - 3:  Front Panel (TRG) Input
 *    - 4:  VME Trigger
 *    - 5:  Front Panel TS Inputs
 *    - 6:  TS (rev 2) Input
 *    - 7:  Random Trigger
 *    - 8:  FP/Ext/GTP 
 *    - 9:  P2 Busy 
 *
 *  @return OK if successful, ERROR otherwise
 */
int
tdSetTriggerSource(int id, int trigmask)
{
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  /* Check input mask */
  if(trigmask>TD_TRIGSRC_SOURCEMASK)
    {
      printf("%s: ERROR: Invalid trigger source mask (0x%x).\n",
	     __FUNCTION__,trigmask);
      return ERROR;
    }

  TDLOCK;
  vmeWrite32(&TDp[id]->trigsrc, trigmask);
  TDUNLOCK;

  return OK;
}

/**
 *  @ingroup Config
 *  @brief Reset the configuration of TI Slaves on the TD.
 *
 *      This routine removes all slaves and resets the fiber port busys.
 *
 *  @param id Slot number
 *
 *  @return OK if successful, ERROR otherwise
 *
 */
int
tdResetSlaveConfig(int id)
{
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  tdSlaveMask[id] = 0;
  vmeWrite32(&TDp[id]->busy, (vmeRead32(&TDp[id]->busy) & ~TD_BUSY_HFBR_MASK));
  TDUNLOCK;

  return OK;
}

/**
 *  @ingroup Config
 *  @brief Add and configure a TI Slave for the TD.
 *
 *      This routine should be used by the TD to configure
 *      HFBR port and BUSY sources.
 *
 *  @param id Slot number
 *  @param fiber  The fiber port of the TD that is connected to the slave
 *
 *  @return OK if successful, ERROR otherwise
 */
int
tdAddSlave(int id, unsigned int fiber)
{
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  if((fiber<1) || (fiber>8) )
    {
      printf("%s: ERROR: Invalid value for fiber (%d)\n",
	     __FUNCTION__,fiber);
      return ERROR;
    }

  /* Add this slave to the global slave mask */
  tdSlaveMask[id] |= (1<<(fiber-1));
  
  /* Add this fiber as a busy source (use first fiber macro as the base) */
  if(tdSetBusySource(id, TD_BUSY_HFBR1<<(fiber-1),0)!=OK)
    return ERROR;

  /* Enable the fiber */
  if(tdEnableFiber(id, fiber)!=OK)
    return ERROR;

  return OK;

}

/**
 *  @ingroup Config
 *  @brief Remove a TI Slave for the TD.
 *
 *  @param id Slot number
 *  @param fiber  The fiber port of the TD to remove.
 *
 *  @return OK if successful, ERROR otherwise
 */
int
tdRemoveSlave(int id, unsigned int fiber)
{
  unsigned int busybits;
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  if((fiber<1) || (fiber>8) )
    {
      printf("%s: ERROR: Invalid value for fiber (%d)\n",
	     __FUNCTION__,fiber);
      return ERROR;
    }

  /* Remove this slave to the global slave mask */
  tdSlaveMask[id] &= ~(1<<(fiber-1));
  
  /* Remove this fiber as a busy source (use first fiber macro as the base) */
  TDLOCK;
  /* Read in previous values, keeping current busy's */
  busybits = vmeRead32(&TDp[id]->busy);

  /* Turn off busy to the fiber in question */
  busybits &= ~(1<<(TD_BUSY_HFBR1-1+fiber));

  /* Write the new mask */
  vmeWrite32(&TDp[id]->busy, busybits);
  TDUNLOCK;

  /* Keep the fiber enabled: No call to tdEnableFiber(..) */

  return OK;
}

/**
 *  @ingroup Config
 *  @brief Add and configure  TI Slaves by using a mask for the TD.
 *
 *      This routine should be used by the TD to configure
 *      HFBR ports and BUSY sources.
 *
 *  @param id Slot number
 *  @param  fibermask The fiber port mask of the TDs that are connected to
 *     the slaves. bit 0 - port 1... bit 7 - port 8
 *
 *  @return OK if successful, ERROR otherwise
 */
int
tdAddSlaveMask(int id, unsigned int fibermask)
{
  int ibit=0;

  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  if((fibermask==0) || (fibermask>0x100))
    {
      printf("%s: ERROR: Invalid value for fibermask (0x%x)\n",
	     __FUNCTION__,fibermask);
      return ERROR;
    }

  for(ibit=0; ibit<8; ibit++)
    {
      if(fibermask & (1<<ibit))
	tdAddSlave(id, ibit+1);
    }

  return OK;
 
}

/**
 *  @ingroup Config
 *  @brief Auto Align Sync Delay
 *
 *    This routine is called in tiInit
 *  @param id Slot number
 *  @return OK if successful, ERROR otherwise
 */
int
tdAutoAlignSync(int id)
{
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  /* IOdelay reset */
  vmeWrite32(&TDp[id]->reset, TD_RESET_IODELAY);
  taskDelay(1);

  /* Auto Align */
  vmeWrite32(&TDp[id]->reset, TD_RESET_AUTOALIGN_P0_SYNC);
  taskDelay(1);
  TDUNLOCK;

  return OK;
}

/**
 * @ingroup Status
 * @brief Get the Module Serial Number
 *
 * @param id Slot number
 * @param rSN  Pointer to string to pass Serial Number
 *
 * @return SerialNumber if successful, ERROR otherwise
 *
 */
unsigned int
tdGetSerialNumber(int id, char **rSN)
{
  unsigned int rval=0;
  char retSN[10];

  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD (id=%2d) not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  vmeWrite32(&TDp[id]->reset,TD_RESET_JTAG);           /* reset */
  vmeWrite32(&TDp[id]->JTAGPROMBase[(0x3c)>>2],0);     /* Reset_idle */
  vmeWrite32(&TDp[id]->JTAGPROMBase[(0xf2c)>>2],0xFD); /* load the UserCode Enable */
  vmeWrite32(&TDp[id]->JTAGPROMBase[(0x1f1c)>>2],0);   /* shift in 32-bit of data */
  rval = vmeRead32(&TDp[id]->JTAGPROMBase[(0x1f1c)>>2]);
  TDUNLOCK;

  sprintf(retSN,"TD-%d",rval&0xfff);
  if(rSN!=NULL)
  {
    strcpy((char *)rSN,retSN);
  }


  printf("%s: TD in slot %2d Serial Number is %s (0x%08x)\n", 
	 __FUNCTION__,id,retSN,rval);

  return rval;
  

}

/**
 * @ingroup Config
 * @brief Latch the Busy and Live Timers.
 *
 *     This routine should be called prior to a call to tdGetLiveTime and tdGetBusyTime
 *
 * @param id Slot number
 * @sa tdGetLiveTime
 * @sa tdGetBusyTime
 *
 *  @return OK if successful, ERROR otherwise
 */
int
tdLatchTimers(int id)
{
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  vmeWrite32(&TDp[id]->reset, TD_RESET_LATCH_TIMERS);
  TDUNLOCK;

  return OK;
}

/**
 * @ingroup Status
 * @brief Return the current "live" time of the module
 * @param id Slot number
 *
 * @returns The current live time in units of 7.68 us
 *
 */
unsigned int
tdGetLiveTime(int id)
{
  unsigned int rval=0;
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  rval = vmeRead32(&TDp[id]->livetime);
  TDUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Return the current "busy" time of the module
 * @param id Slot number
 *
 * @returns The current live time in units of 7.68 us
 *
 */
unsigned int
tdGetBusyTime(int id)
{
  unsigned int rval=0;
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  rval = vmeRead32(&TDp[id]->busytime);
  TDUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Configure which ports (and self) to enable response of a SyncReset request.
 * @param id Slot number
 * @param portMask Mask of ports to enable (port 1 = bit 0)
 * @param self 1 to enable self, 0 to disable
 *
 * @return OK if successful, otherwise ERROR
 */
int
tdEnableSyncResetRequest(int id, unsigned int portMask)
{
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  if(portMask > 0xFF)
    {
      printf("%s: ERROR: Invalid portMask (0x%x)\n",
	     __FUNCTION__, portMask);
      return ERROR;
    }

  TDLOCK;
  vmeWrite32(&TDp[id]->rocEnable,
	   (vmeRead32(&TDp[id]->rocEnable) & TD_ROCENABLE_MASK) |
	   (portMask << 11));
  TDUNLOCK;
  
  return OK;
}

/**
 * @ingroup Status
 * @brief Status of SyncReset Request received bits.
 * @param id Slot number
 * @param pflag Print to standard out if not 0
 * @return Port mask of SyncReset Request received (port 1 = bit 0), otherwise ERROR;
 */
int
tdSyncResetRequestStatus(int id, int pflag)
{
  int rval = 0, ibit = 0;
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  rval = (int)(vmeRead32(&TDp[id]->rocEnable) & TD_ROCENABLE_SYNCRESET_REQUEST_MONITOR_MASK);
  TDUNLOCK;

  /* Reorganize the bits */
  if(rval)
    {
      rval = rval >> 1;
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
 * @ingroup Status
 * @brief Returns the mask of fiber channels that report a "connected" status from a TI.
 * @param id Slot number
 *
 * @return mask of fiber channels that report "connected"
 */
int
tdGetConnectedFiberMask(int id)
{
  int rval=0;
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  rval = (vmeRead32(&TDp[id]->fiber) & TD_FIBER_CONNECTED_MASK)>>16;
  TDUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Returns the mask of fiber channels that report a "connected"
 *     status from a TI has it's trigger source enabled.
 *
 * @param id Slot number
 *
 * @return mask of fiber channels that report "enabled" trigger source
 */
int
tdGetTrigSrcEnabledFiberMask(int id)
{
  int rval=0;
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  rval = (vmeRead32(&TDp[id]->fiber) & TD_FIBER_TRIGSRC_ENABLED_MASK)>>24;
  TDUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Reset the triggers enabled status bits of TI Slaves.
 *
 * @param id Slot number
 *
 * @return OK if successful, otherwise ERROR
 */
int
tdTriggerReadyReset(int id)
{
  unsigned int syncsource=0;
  if(id==0) id=tdID[0];

  if(TDp[id]==NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  /* Reset Receiver */
  vmeWrite32(&TDp[id]->reset, TD_RESET_MGT_RX_RESET);
  taskDelay(1);

  /* Get the current SyncReset Source */
  syncsource = vmeRead32(&TDp[id]->sync) & TD_SYNC_SOURCEMASK;

  /* Set Loopback as Source */
  vmeWrite32(&TDp[id]->sync, TD_SYNC_LOOPBACK);

  /* Send the Trigger Source Enabled Reset */
  vmeWrite32(&TDp[id]->syncCommand,TD_SYNCCOMMAND_TRIGGER_READY_RESET); 

  /* Restore original SyncReset Source */
  vmeWrite32(&TDp[id]->sync, syncsource);
  TDUNLOCK;

  return OK;
}

/**
 * @ingroup Status
 * @brief Get the crate ID of the selected port
 * @param id Slot number
 * @param port
 *   - 1-8: Fiber port 1-8
 *
 * @return port Crate ID if successful, ERROR otherwise
 *
 */
int
tdGetCrateID(int id, int port)
{
  int rval=0;
  if(id==0) id=tdID[0];

  if(TDp[id] == NULL) 
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  if((port<1) || (port>8))
    {
      printf("%s: ERROR: Invalid port (%d)\n",
	     __FUNCTION__,port);
    }

  TDLOCK;
  rval = (vmeRead32(&TDp[id]->hfbr_tiID[port-1]) & TD_ID_CRATEID_MASK)>>8;
  TDUNLOCK;

  return rval;
}

/*sergey*/
int
tdPrintCrateID(int id, int port)
{
  int crate_id;
  crate_id = tdGetCrateID(id, port);
  printf("TD slot %d, port %d -> crate ID = %d\n",id,port,crate_id);
}
/*sergey*/

/**
 * @ingroup Config
 * @brief Set the port names for the specified TD from the received Crate IDs from the TI Slaves.
 * @param id Slot number
 * @return OK if successful, otherwise ERROR;
 */
int
tdSetPortNamesFromCrateID(int id)
{
  int iport=0, crateID=0;
  char portname[20];
  if(id==0) id=tdID[0];

  if(TDp[id] == NULL) 
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  for(iport=1; iport<9; iport++)
    {
      crateID = tdGetCrateID(id, iport);
      sprintf((char *)&portname,"ROC%d",crateID);
      tdSetPortName(id,iport,portname);
    }
  
  return OK;
}

/**
 * @ingroup Config
 * @brief Set the port names for all initialized TD from the received Crate IDs from the TI Slaves.
 * @return OK if successful, otherwise ERROR;
 */
void
tdGSetPortNamesFromCrateID()
{
  int itd=0;

  for(itd=0; itd<nTD; itd++)
    tdSetPortNamesFromCrateID(tdSlot(itd));
}

/**
 * @ingroup Status
 * @brief Get the trigger sources enabled bits of the selected port
 * @param id Slot number
 * @param port
 *   - 1-8: Fiber port 1-8
 *
 * @return bitmask
 *   - 0 - P0 
 *   - 1 - Fiber 1
 *   - 2 - Loopback
 *   - 3 - TRG (FP)
 *   - 4 - VME
 *   - 5 - TS Inputs (FP)
 *   - 6 - TS (rev 2)
 *   - 7 - Internal Pulser
 */
int
tdGetPortTrigSrcEnabled(int id, int port)
{
  int rval=0;
  if(id==0) id=tdID[0];

  if(TDp[id] == NULL) 
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  if((port<1) || (port>8))
    {
      printf("%s: ERROR: Invalid port (%d)\n",
	     __FUNCTION__,port);
    }

  TDLOCK;
  rval = (vmeRead32(&TDp[id]->hfbr_tiID[port-1]) & TD_ID_TRIGSRC_ENABLE_MASK);
  TDUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Get the blocklevel of the TI Slave on the selected port
 * @param id Slot number
 * @param port
 *   - 1-8: Fiber port 1-8
 *
 * @return port blocklevel if successful, ERROR otherwise
 */
int
tdGetSlaveBlocklevel(int id, int port)
{
  int rval=0;
  if(id==0) id=tdID[0];

  if(TDp[id] == NULL) 
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  if((port<1) || (port>8))
    {
      printf("%s: ERROR: Invalid port (%d)\n",
	     __FUNCTION__,port);
    }

  TDLOCK;
  rval = (vmeRead32(&TDp[id]->hfbr_tiID[port-1]) & TD_ID_BLOCKLEVEL_MASK)>>16;
  TDUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Reset the MGT
 * @param id Slot number
 */
int
tdResetMGT(int id)
{
  if(id==0) id=tdID[0];

  if(TDp[id] == NULL) 
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  vmeWrite32(&TDp[id]->reset, TD_RESET_MGT);
  TDUNLOCK;
  taskDelay(1);

  return OK;
}

/**
 * @ingroup Config
 * @brief Reset the MGT Rx CDR
 * @param id Slot number
 */
int
tdResetMGTRx(int id)
{
  if(id==0) id=tdID[0];

  if(TDp[id] == NULL) 
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  vmeWrite32(&TDp[id]->reset, TD_RESET_MGT_RX_RESET);
  TDUNLOCK;
  taskDelay(1);

  return OK;
}

/**
 * @ingroup Config
 * @brief Reset the Fiber Tranceivers
 * @param id Slot number
 */
int
tdResetFiber(int id)
{
  if(id==0) id=tdID[0];

  if(TDp[id] == NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  vmeWrite32(&TDp[id]->reset, TD_RESET_FIBER);
  TDUNLOCK;
  taskDelay(1);

  return OK;
}

/**
 * @ingroup Status
 * @brief Print a summary of all fiber port connections to potential TI Slaves
 *
 * @param id Slot number
 * @param pflag
 *  - 0: Default output
 *  - 1: Print Raw Registers
 */
void
tdSlaveStatus(int id, int pflag)
{
  int iport=0, ibs=0, ifiber=0;
  unsigned long TDBase;
  unsigned int hfbr_tiID[8] = {1,2,3,4,5,6,7,8};
  unsigned int blockStatus[5];
  unsigned int fiber=0, busy=0, trigsrc=0;
  int nblocksReady=0, nblocksNeedAck=0, slaveCount=0;

  if(id==0) id=tdID[0];

  if(TDp[id] == NULL) 
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return;
    }

  TDLOCK;
  for(iport=0; iport<8; iport++)
    {
      hfbr_tiID[iport] = vmeRead32(&TDp[id]->hfbr_tiID[iport]);
    }
  fiber       = vmeRead32(&TDp[id]->fiber);
  busy        = vmeRead32(&TDp[id]->busy);
  trigsrc     = vmeRead32(&TDp[id]->trigsrc);
  for(ibs=0; ibs<4; ibs++)
    {
      blockStatus[ibs] = vmeRead32(&TDp[id]->blockStatus[ibs]);
    }

  TDUNLOCK;

  TDBase = (unsigned long)TDp[id];


  printf("TD - Slot %2d - Port STATUS Summary\n",id);
  printf("                                                      Block Status\n");
  if(pflag>0)
    {
      printf(" Registers (offset):\n");
      printf("  TDBase     (0x%08lx)\n",TDBase-tdA24Offset);
      printf("  busy           (0x%04lx) = 0x%08x\t", (unsigned long)(&TDp[id]->busy) - TDBase, busy);
      printf("  fiber          (0x%04lx) = 0x%08x\n", (unsigned long)(&TDp[id]->fiber) - TDBase, fiber);
      printf("  hfbr_tiID[0]   (0x%04lx) = 0x%08x\t", (unsigned long)(&TDp[id]->hfbr_tiID[0]) - TDBase, hfbr_tiID[0]);
      printf("  hfbr_tiID[1]   (0x%04lx) = 0x%08x\n", (unsigned long)(&TDp[id]->hfbr_tiID[1]) - TDBase, hfbr_tiID[1]);
      printf("  hfbr_tiID[2]   (0x%04lx) = 0x%08x\t", (unsigned long)(&TDp[id]->hfbr_tiID[2]) - TDBase, hfbr_tiID[2]);
      printf("  hfbr_tiID[3]   (0x%04lx) = 0x%08x\n", (unsigned long)(&TDp[id]->hfbr_tiID[3]) - TDBase, hfbr_tiID[3]);
      printf("  hfbr_tiID[4]   (0x%04lx) = 0x%08x\t", (unsigned long)(&TDp[id]->hfbr_tiID[4]) - TDBase, hfbr_tiID[4]);
      printf("  hfbr_tiID[5]   (0x%04lx) = 0x%08x\n", (unsigned long)(&TDp[id]->hfbr_tiID[5]) - TDBase, hfbr_tiID[5]);
      printf("  hfbr_tiID[6]   (0x%04lx) = 0x%08x\t", (unsigned long)(&TDp[id]->hfbr_tiID[6]) - TDBase, hfbr_tiID[6]);
      printf("  hfbr_tiID[7]   (0x%04lx) = 0x%08x\n", (unsigned long)(&TDp[id]->hfbr_tiID[7]) - TDBase, hfbr_tiID[7]);

      printf("\n\n");
    }

  printf("Port  ROCID   Connected   TrigSrcEn   Busy Status    Ready / NeedAck  Blocklevel\n");
  printf("--------------------------------------------------------------------------------\n");
  /* Slaves last */
  for(iport=1; iport<9; iport++)
    {
      /* Only continue of this port has been configured as a slave */
      if((tdSlaveMask[id] & (1<<(iport-1)))==0) continue;
      
      /* Slot and Port number */
      printf("%d     ", iport);

      /* ROCID */
      printf("%5d      ",
	     (hfbr_tiID[iport-1]&TD_ID_CRATEID_MASK)>>8);
	  
      /* Connection Status */
      printf("%s      %s       ",
	     (fiber & TD_FIBER_CONNECTED_TI(iport))?"YES":"NO ",
	     (fiber & TD_FIBER_TRIGSRC_ENABLED_TI(iport))?"ENABLED ":"DISABLED");

      /* Busy Status */
      printf("%s       ",
	     (busy & TD_BUSY_MONITOR_FIBER_BUSY(iport))?"BUSY":"    ");

      /* Block Status */
      ifiber=iport-1;
      if( (ifiber % 2) == 0)
	{
	  nblocksReady   = blockStatus[ifiber/2] & TD_BLOCKSTATUS_NBLOCKS_READY0;
	  nblocksNeedAck = (blockStatus[ifiber/2] & TD_BLOCKSTATUS_NBLOCKS_NEEDACK0)>>8;
	}
      else
	{
	  nblocksReady   = (blockStatus[(ifiber-1)/2] & TD_BLOCKSTATUS_NBLOCKS_READY1)>>16;
	  nblocksNeedAck = (blockStatus[(ifiber-1)/2] & TD_BLOCKSTATUS_NBLOCKS_NEEDACK1)>>24;
	}
      printf("   %3d / %3d",nblocksReady, nblocksNeedAck);
      printf("         %3d",(hfbr_tiID[iport-1]&TD_ID_BLOCKLEVEL_MASK)>>16);
	  
      printf("\n");
      slaveCount++;
    }
  printf("\n");
  printf("Total Slaves Added = %d\n",slaveCount);

}

/**
 * @ingroup Status
 * @brief Obtain the status of blocks sent readout acknowledges received from the TI Slaves.
 *
 * @param id Slot number
 * @param port Port number (1-8)
 * @param pflag
 *  - !0: Print to standard out
 *
 * @return 16 bit integer with the following:
 *  - bits 0-7:  The number of blocks sent to the TI Slave that have not been acknowedged.
 *  - bits 8-15: The number of blocks that have been acknowedged, but still require a readout acknowledge.
 */
unsigned int
tdGetBlockStatus(int id, int port, int pflag)
{
  unsigned int rval=0;
  int fiber=0, nblocksReady=0, nblocksNeedAck=0;
  if(id==0) id=tdID[0];

  if(TDp[id] == NULL) 
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }
  if((port<1) || (port>8))
    {
      printf("%s: ERROR: Invalid port number (%d)\n",
	     __FUNCTION__,port);
      return ERROR;
    }


  fiber=port-1;
  TDLOCK;
  if( (fiber % 2) == 0)
    {
      rval = vmeRead32(&TDp[id]->blockStatus[fiber/2]);
      nblocksReady   = rval & TD_BLOCKSTATUS_NBLOCKS_READY0;
      nblocksNeedAck = (rval & TD_BLOCKSTATUS_NBLOCKS_NEEDACK0)>>8;
      rval = rval & 0xFFFF;
    }
  else
    {
      rval = vmeRead32(&TDp[id]->blockStatus[(fiber-1)/2]);
      nblocksReady   = (rval & TD_BLOCKSTATUS_NBLOCKS_READY1)>>16;
      nblocksNeedAck = (rval & TD_BLOCKSTATUS_NBLOCKS_NEEDACK1)>>24;
      rval = (rval & 0xFFFF0000)>>16;
    }
  TDUNLOCK;


  if(pflag)
    {
      printf("   %3d / %3d",nblocksReady, nblocksNeedAck);
    }

  return rval;

}

int
tdGetBusyStatus(int id, int port, int pflag)
{
  unsigned int rval=0;
  unsigned int busy=0;
  unsigned int fiber_busy=0;
  if(id==0) id=tdID[0];

  if(TDp[id] == NULL) 
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }
  if((port<1) || (port>8))
    {
      printf("%s: ERROR: Invalid port number (%d)\n",
	     __FUNCTION__,port);
      return ERROR;
    }

  TDLOCK;
  busy        = vmeRead32(&TDp[id]->busy);
  fiber_busy  = vmeRead32(&TDp[id]->fiber_busy);
  TDUNLOCK;

  if(busy & TD_BUSY_MONITOR_FIBER_BUSY(port))
    {
      rval = 1;
      if(pflag)
	printf("%s(%d): Port %d is BUSY (%s)\n",
	       __FUNCTION__,id,port,
	       (fiber_busy & TD_FIBER_BUSY(port))?"Fiber":"Block Count");
    }
  else
    {
      rval = 0;
      if(pflag)
	printf("%s(%d): Port %d is NOT BUSY\n",
	       __FUNCTION__,id,port);
    }

  return rval;
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
tdSetOutputPort(int id, unsigned int set1, unsigned int set2, unsigned int set3, unsigned int set4)
{
  unsigned int bits=0;
  if(id==0) id=tdID[0];

  if(TDp[id] == NULL) 
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
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

  TDLOCK;
  vmeWrite32(&TDp[id]->output, bits);
  TDUNLOCK;

  return OK;
}

/**
 * @ingroup Status
 * @brief Return the number of triggers received from the trigger supervisor.
 *
 * @param id Slot number
 *
 * @return trigger count if successful, otherwise ERROR
 */
unsigned int
tdGetTrigCount(int id)
{
  unsigned int rval=0;
  if(id==0) id=tdID[0];

  if(TDp[id] == NULL) 
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }


  TDLOCK;
  rval = vmeRead32(&TDp[id]->trigCount);
  TDUNLOCK;
  
  return rval;
}

/**
 * @ingroup Status
 * @brief Return BUSY counter for specified Busy Source
 * @param id Slot number
 * @param busysrc
 *  - 0: SWA
 *  - 1: SWB
 *  - 2: P2
 *  - 3: FP-FTDC
 *  - 4: FP-FADC
 *  - 5: FP
 *  - 6: Unused
 *  - 7: Loopack
 *  - 8-15: Fiber 1-8
 * @return
 *   - Busy counter for specified busy source
 */
unsigned int
tdGetBusyCounter(int id, int busysrc)
{
  unsigned int rval=0;
  if(id==0) id=tdID[0];

  if(TDp[id] == NULL) 
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }
  
  TDLOCK;
  if(busysrc<7)
    rval = vmeRead32(&TDp[id]->busy_scaler1[busysrc]);
  else
    rval = vmeRead32(&TDp[id]->busy_scaler2[busysrc-7]);
  TDUNLOCK;
  
  return rval;
}

/**
 * @ingroup Status
 * @brief Print the BUSY counters for all busy sources
 * @param id Slot number
 * @return
 *   - OK if successful, otherwise ERROR;
 */
int
tdPrintBusyCounters(int id)
{
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
  if(id==0) id=tdID[0];

  if(TDp[id] == NULL) 
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  for(icnt=0; icnt<16; icnt++)
    {
      if(icnt<7)
	counter[icnt] = vmeRead32(&TDp[id]->busy_scaler1[icnt]);
      else
	counter[icnt] = vmeRead32(&TDp[id]->busy_scaler2[icnt-7]);
    }
  TDUNLOCK;

  printf("\n\n");
  printf(" Busy Counters \n");
  printf("--------------------------------------------------------------------------------\n");
  for(icnt=0; icnt<16; icnt++)
    {
      printf("%s   0x%08x (%10d)\n",
	     scounter[icnt], counter[icnt], counter[icnt]);
    }
  printf("--------------------------------------------------------------------------------\n");
  printf("\n\n");

  return OK;
}

int
tdGFiberBusyStatus(int nsec)
{
  unsigned int counter[TD_MAX_VME_SLOTS][8];
  int enabled[TD_MAX_VME_SLOTS][8];
  int itd, id, ifiber, icounter;

  memset((char *)counter, 0, sizeof(counter));

  TDLOCK;
  for(itd=0; itd<nTD; itd++)
    {
      id = tdSlot(itd);
      for(ifiber = 0; ifiber < 8; ifiber++)
	{
	  icounter = ifiber + 1;

	  counter[id][ifiber] = vmeRead32(&TDp[id]->busy_scaler2[icounter]);

	  enabled[id][ifiber] =
	    (vmeRead32(&TDp[id]->busy) & (1 << (8 + ifiber))) ? 1 : 0;
	}
    }

  if(nsec > 0)
    {
      sleep(nsec);
      for(itd=0; itd<nTD; itd++)
	{
	  id = tdSlot(itd);
	  for(ifiber = 0; ifiber < 8; ifiber++)
	    {
	      icounter = ifiber + 1;
	      counter[id][ifiber] =
		vmeRead32(&TDp[id]->busy_scaler2[icounter]) - counter[id][ifiber];
	    }
	}
    }
  TDUNLOCK;

  printf("\n\n");
  printf("                        -----  TD Fiber Busy Counters -----\n");
  if(nsec > 0)
    printf("                             (counted over %d seconds)", nsec);

  printf("\n\n");

  printf("Slot     1        2        3        4        5        6        7        8\n");
  printf("--------------------------------------------------------------------------------\n");

  for(itd=0; itd<nTD; itd++)
    {
      id = tdSlot(itd);

      /* Slot */
      printf(" %2d  ",id);

      for(ifiber = 0; ifiber < 8; ifiber++)
	{
	  if(enabled[id][ifiber])
	    {
	      printf("%8X ", counter[id][ifiber]);
	    }
	  else
	    {
	      printf("-------- ");
	    }
	}
      printf("\n");

      printf("--------------------------------------------------------------------------------\n");
    }
  printf("\n");

  return OK;
}

/**
 * @ingroup Status
 * @brief Read the fiber fifo from the TD 
 *
 * @param   fiber - Fiber fifo to read. 1 and 5 only supported.
 * @param   data  - local memory address to place data
 * @param  maxwords - Maximum number of 32bit words to put into data array.
 * @param  rflag - Readout Flag
 *      0 - Stop only on fifo empty, or maxwords
 *      1 - Stop on Block Received, fifo empty, or maxwords
 *      2 - Empty fifo.  @data not used. Maxwords ignored.
 *
 * @return Number of words transferred to data if successful, ERROR otherwise
 *
 */
int
tdReadFiberFifo(int id, int fiber, volatile unsigned int *data, int maxwords,
		int rflag)
{
  int nwords = 0, nodata = 0;
  unsigned int word = 0;
  unsigned int end_mask = 0;
  
  if(id==0) id=tdID[0];

  if(TDp[id] == NULL) 
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  if((data==NULL) && (rflag != 2))
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

  // Check readout flag
  switch(rflag)
    {
    case 1: // Stop on block received, fifo empty
      end_mask = ((1 << 31) || (1 << 4));
      break;

    case 2: // Empty fifo
      nodata = 1;
      end_mask = (1 << 31);
      break;
	
    case 0:
    default: // Stop on fifo empty.
      end_mask = (1 << 31);
    }

  TDLOCK;
  while(nwords < maxwords)
    {
      if(fiber == 1)
	word = vmeRead32(&TDp[id]->trigTable[12]);
      else
      	word = vmeRead32(&TDp[id]->trigTable[13]);

      if(word & end_mask)
	break;

#ifndef VXWORKS
      word = LSWAP(word);
#endif
      
      if(!nodata)
	data[nwords++] = word;
      else
	nwords++;
    }
  TDUNLOCK;
  
  return nwords;
}


/**
 * @ingroup Status
 * @brief Read the fiber fifo from the TD and print to standard out.
 *
 * @param   fiber - Fiber fifo to read. 1 and 5 only supported.
 *
 * @return OK if successful, ERROR otherwise
 *
 */
int
tdPrintFiberFifo(int id, int fiber)
{
  volatile unsigned int *data;
  int maxwords = 256, iword, rwords = 0;
  struct td_fiber_word
  {
    unsigned int undef:3;              // bits(2:0)
    unsigned int readout_ack:1;        // 3
    unsigned int block_received:1;     // 4
    unsigned int trig2_ack:1;          // 5
    unsigned int trig1_ack:1;          // 6
    unsigned int busy2:1;              // 7
    unsigned int not_sync_reset_req:1; // 8
    unsigned int sync_reset_req:1;     // 9
    unsigned int trg_ack:1;            // 10
    unsigned int busy:1;               // 11
    unsigned int header:4;             // bits(15:12)
    unsigned int timestamp:16;         // bits(31:16)
  };

  union td_fiber_data_word
  {
    struct td_fiber_word bf1;
    unsigned int raw;
  } uni;

  if(id==0) id=tdID[0];

  if(TDp[id] == NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

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

  rwords = tdReadFiberFifo(id, fiber, data, maxwords, 0);

  if(rwords == 0)
    {
      printf("%s: No data in fifo\n\n",
	     __func__);
      return OK;
    }
  else if(rwords == ERROR)
    {
      printf("%s: tdReadFiberFifo(..) returned ERROR\n",
	     __func__);
      return ERROR;
    }

  printf(" TD %2d Fiber %d fifo (%d words)\n",
	 id,
	 fiber, rwords);
  printf("                              Blk  2   1  Bsy !Sy Syn Trg\n");
  printf("      Timestamp     Data  RO  Rec Ack Ack  2  Req Req Ack Bsy Header\n");
  printf("---------------------------------------------------------------------\n");
  for(iword = 0; iword < rwords; iword++)
    {
      uni.raw = data[iword];
      printf("%3d:    0x%04x     0x%04x  %d   %d   %d   %d   %d   %d   %d   %d   %d   %0x01x\n",
	     iword,
	     uni.bf1.timestamp,
	     (uni.raw & 0xFFFF),
	     uni.bf1.readout_ack,    // 1
	     uni.bf1.block_received,
	     uni.bf1.trig2_ack,
	     uni.bf1.trig1_ack,
	     uni.bf1.busy2,          // 5
	     uni.bf1.not_sync_reset_req,
	     uni.bf1.sync_reset_req,
	     uni.bf1.trg_ack,
	     uni.bf1.busy,           // 9
	     uni.bf1.header
	     );
    }
  printf("----------------------------\n");
  printf("\n");

  if(data)
    free((void *)data);

  return OK;
}

/**
 * @ingroup Status
 * @brief Clear the selected fiber fifo from selected TD
 *
 * @param id Slot number
 * @param fiber - Fiber fifo to read. 1 and 5 only supported. 0 for both
 *
 * @return Number of words cleared from fifo if successful, ERROR otherwise
 *
 */
int
tdClearFiberFifo(int id, int fiber)
{
  int rval = 0, rwords = 0, maxwords = 256;
  if(id==0) id=tdID[0];

  if(TDp[id] == NULL) 
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  if(fiber == 0)
    {
      rval = tdReadFiberFifo(id, 1, NULL, maxwords, 0);
      if(rval == ERROR)
	{
	  return ERROR;
	}

      rwords = rval;

      rval = tdReadFiberFifo(id, 5, NULL, maxwords, 0);

      if(rval == ERROR)
	{
	  return ERROR;
	}

      rwords += rval;

    }
  else
    {
      rval = tdReadFiberFifo(id, 5, NULL, maxwords, 0);

      if(rval == ERROR)
	{
	  return ERROR;
	}

      rwords = rval;
    }

  return rwords;
}

/**
 * @ingroup Status
 * @brief Get the error status bits for the trigger links from specified TDs
 *
 * @param id Slot number
 * @param pflag
 *  - !0: Print to standard out
 *
 * @return Trigger Link bits if successful, ERROR otherwise
 */
unsigned int
tdGetTriggerLinkStatus(int id, int pflag)
{
  unsigned int GTPStatusB = 0, bitflags = 0;
  int ibit = 0;

  if(id==0) id=tdID[0];
  if(TDp[id] == NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  TDLOCK;
  GTPStatusB = vmeRead32(&TDp[id]->GTPStatusB);
  TDUNLOCK;

  if(pflag)
    {
      printf("TD Slot %2d STATUS for Trigger Links\n", id);

      printf("      Connected    RX Data Error      Disparity    NON 8b/10b Data\n");
      printf("     (12345678)      (12345678)      (12345678)      (12345678)\n");
      printf("--------------------------------------------------------------------------------\n");

      printf("      ");
      bitflags = GTPStatusB & TD_GTPSTATUSB_CHANNEL_BONDING_MASK;
      for(ibit = 0; ibit < 8; ibit++)
	{
	  if( (1<<ibit) & bitflags )
	    printf("%d", ibit+1);
	  else
	    printf("-");
	}

      printf("        ");
      bitflags = (GTPStatusB & TD_GTPSTATUSB_DATA_ERROR_MASK) >> 8;
      for(ibit = 0; ibit < 8; ibit++)
	{
	  if( (1<<ibit) & bitflags )
	    printf("%d", ibit+1);
	  else
	    printf("-");
	}

      printf("        ");
      bitflags = (GTPStatusB & TD_GTPSTATUSB_DISPARITY_ERROR_MASK) >> 16;
      for(ibit = 0; ibit < 8; ibit++)
	{
	  if( (1<<ibit) & bitflags )
	    printf("%d", ibit+1);
	  else
	    printf("-");
	}
      printf("        ");

      bitflags = (GTPStatusB & TD_GTPSTATUSB_DATA_NOT_IN_TABLE_ERROR_MASK) >> 24;
      for(ibit = 0; ibit < 8; ibit++)
	{
	  if( (1<<ibit) & bitflags )
	    printf("%d", ibit+1);
	  else
	    printf("-");
	}

      printf("\n");
    }

  return GTPStatusB;

}

/**
 * @ingroup Status
 * @brief Print the error status bits for the trigger links for all initialized TDs
 *
 * @return OK if successful, ERROR otherwise
 *
 */
int
tdGPrintTriggerLinkStatus()
{
  unsigned int GTPStatusB[TD_MAX_VME_SLOTS+1], bitflags = 0;
  int id;
  int itd = 0, ibit = 0;

  TDLOCK;
  for(itd = 0; itd < nTD; itd++)
  {
    id = tdSlot(itd);
    GTPStatusB[id] = vmeRead32(&TDp[id/*itd*/]->GTPStatusB); /* sergey: itd->id */
  }
  TDUNLOCK;

  printf("STATUS for Trigger Links\n");

  printf("          Connected    RX Data Error      Disparity    NON 8b/10b Data\n");
  printf("Slot     (12345678)      (12345678)      (12345678)      (12345678)\n");
  printf("--------------------------------------------------------------------------------\n");


  for(itd = 0; itd < nTD; itd++)
    {
      printf(" %2d   ", tdSlot(itd));
      bitflags = GTPStatusB[tdSlot(itd)]
	& TD_GTPSTATUSB_CHANNEL_BONDING_MASK;
      for(ibit = 0; ibit < 8; ibit++)
	{
	  if( (1<<ibit) & bitflags )
	    printf("%d", ibit+1);
	  else
	    printf("-");
	}

      printf("        ");
      bitflags = (GTPStatusB[tdSlot(itd)]
		  & TD_GTPSTATUSB_DATA_ERROR_MASK) >> 8;
      for(ibit = 0; ibit < 8; ibit++)
	{
	  if( (1<<ibit) & bitflags )
	    printf("%d", ibit+1);
	  else
	    printf("-");
	}

      printf("        ");
      bitflags = (GTPStatusB[tdSlot(itd)]
		  & TD_GTPSTATUSB_DISPARITY_ERROR_MASK) >> 16;
      for(ibit = 0; ibit < 8; ibit++)
	{
	  if( (1<<ibit) & bitflags )
	    printf("%d", ibit+1);
	  else
	    printf("-");
	}
      printf("        ");

      bitflags = (GTPStatusB[tdSlot(itd)]
		  & TD_GTPSTATUSB_DATA_NOT_IN_TABLE_ERROR_MASK) >> 24;
      for(ibit = 0; ibit < 8; ibit++)
	{
	  if( (1<<ibit) & bitflags )
	    printf("%d", ibit+1);
	  else
	    printf("-");
	}

      printf("\n");
    }

  return OK;
}


int
tdGetTranceiverStatus(int id, unsigned int *data, int maxwords)
{
  int ibyte, itr;
  int nreadbytes = 21;
  unsigned short readbytes[21] =
    {
      22, 23, /* Temp */
      26, 27, /* Voltage */
      34, 35, 36, 37, 38, 39, 40, 41, /* rxPower */
      42, 43, 44, 45, 46, 47, 48, 49, /* txBias */
      86 /* Tx? Disable */
    };
  unsigned int ReadVal;
  int nwords = 0, maxtr = 8;
  unsigned int *i2cOptp;
  unsigned int vmeControl_old = 0;

  if(id==0) id=tdID[0];

  if(TDp[id] == NULL)
    {
      printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
      return ERROR;
    }

  i2cOptp = (unsigned int *)((unsigned long)TDp[id] + 0x50000);

  TDLOCK;
  /* set the i2c device address to 0xA0# */
  vmeControl_old = vmeRead32(&TDp[id]->vmeControl);
  vmeWrite32(&TDp[id]->vmeControl, 0x00000111);

  for (itr = 0; itr < maxtr; itr++) // loop over the eight transceivers
    {
      vmeWrite32(&TDp[id]->fiber, 0x1ff - ((1 << itr) & 0xff));

      for (ibyte = 0; ibyte < nreadbytes; ibyte++) // loop over bytes
	{

	  ReadVal = vmeRead32(&i2cOptp[(0xC00 + readbytes[ibyte]*4)>>2]);
	  taskDelay(1);

	  data[nwords++] = (itr << 24) | (readbytes[ibyte] << 16) | (ReadVal & 0xFFFF);

	  if(nwords == maxwords)
	    goto DONE;
	}

    }

 DONE:
  vmeWrite32(&TDp[id]->vmeControl, vmeControl_old);

  TDUNLOCK;

  return nwords;
}

int
tdPrintTranceiverStatus(int id)
{
  int rval = OK, idata, ndata = 21 * 8, maxtr = 8;
  unsigned int data[21 * 8];
  unsigned short reg = 0, value = 0;
  int itr, Tempt[8], Volt[8];
  int rxPower[8][4], txBias[8][4];
  int txDisable[8];

  /* Readout the regs */
  rval = tdGetTranceiverStatus(id, (unsigned int *)data, ndata);
  if(rval == ERROR)
    return ERROR;

  /* Initialize the decoded arrays */
  memset((char *)Tempt, 0, sizeof(Tempt));
  memset((char *)Volt, 0, sizeof(Volt));

  memset((char *)rxPower, 0, sizeof(rxPower));
  memset((char *)txBias, 0, sizeof(txBias));

  memset((char *)txDisable, 0, sizeof(txDisable));

  /* Decode the data into the arrays */
  for(idata = 0; idata < ndata; idata++)
    {
      itr = (data[idata] & 0xFF000000) >> 24;
      reg = (data[idata] & 0x00FF0000) >> 16;
      value = (data[idata] & 0x0000FFFF) >> 0;

      if(itr > 7)
	{
	  printf("%s: ERROR: Invalid Fiber value (%d). data[%d] = 0x%08x\n",
		 __func__, itr, idata, data[idata]);
	  continue;
	}

      if (reg == 22) Tempt[itr] = (value&0xff) << 8;
      if (reg == 23) Tempt[itr] |= (value&0xff);

      if (reg == 26) Volt[itr] = (value & 0xff) << 8;
      if (reg == 27) Volt[itr] |= value&0xff;

      if (reg == 34) rxPower[itr][0] = (value & 0xff) << 8;
      if (reg == 35) rxPower[itr][0] |= (value & 0xff);

      if (reg == 36) rxPower[itr][1] = (value & 0xff) << 8;
      if (reg == 37) rxPower[itr][1] |= (value & 0xff);

      if (reg == 38) rxPower[itr][2] = (value & 0xff) << 8;
      if (reg == 39) rxPower[itr][2] |= (value & 0xff);

      if (reg == 40) rxPower[itr][3] = (value & 0xff) << 8;
      if (reg == 41) rxPower[itr][3] |= (value & 0xff);

      if (reg == 42) txBias[itr][0] = (value & 0xff) << 8;
      if (reg == 43) txBias[itr][0] |= (value & 0xff);

      if (reg == 44) txBias[itr][1] = (value & 0xff) << 8;
      if (reg == 45) txBias[itr][1] |= (value & 0xff);

      if (reg == 46) txBias[itr][2] = (value & 0xff) << 8;
      if (reg == 47) txBias[itr][2] |= (value & 0xff);

      if (reg == 48) txBias[itr][3] = (value & 0xff) << 8;
      if (reg == 49) txBias[itr][3] |= (value & 0xff);

      if (reg == 86) txDisable[itr] = (value & 0xff);

    }

  for (itr = 0; itr < maxtr; itr++) // loop over the eight transceivers
    {

      printf("\n Optic Transceiver #%d ", itr+1);
      if(Volt[itr] == rxPower[itr][0]) /* Reads back 0xffffffff */
	{
	  printf(" - N/A\n");
	  continue;
	}


      /* Convert register values to physical units */
      Volt[itr] /= 10; /* mV */
      Tempt[itr] /= 256;

      int i;
      for(i = 0; i < 4; i++)
	{
	  rxPower[itr][i] /= 10;
	  txBias[itr][i] *= 2;
	}

      printf("\n   Module Temp : %7d C    Supply Volt : %6d mV \n",
	     Tempt[itr], Volt[itr]);
      printf("   tx0 Disable : %7d      tx1 Disable : %6d\n",
	     txDisable[itr] & (1<<0),
	     txDisable[itr] & (1<<1));
      printf("   tx2 Disable : %7d      tx3 Disable : %6d\n",
	     txDisable[itr] & (1<<2),
	     txDisable[itr] & (1<<3));

      for(i = 0; i < 4; i++)
	printf("   rxPower[%d]  : %7d uW   txBias[%d]   : %6d uA\n",
	       i, rxPower[itr][i], i, txBias[itr][i]);

      fflush(stdout);
    }


  return OK;
}





/*sergey*/
int
tdGetNtds()
{
  return(nTD);
}






/***************************************************/
/* ---------------- FROM ALEX SOMOV -------------- */

/**
  * @ingroup Status
  * @brief Return BUSY counter for specified Busy Source
  * @param id Slot number
  * @return busy counters:
      data[0]  -  data[7] -  Fiber,
      data[8]  -  Loopback
      data[9]  -  Livetime
      data[10] -  Busytime
  */
#define DEFAULT_BUSY 0
unsigned int
tdReadScalers(int id, volatile unsigned int *data)
{
   int busy_stat = 0;
   int nwrds  =  0;

   if(id==0) id=tdID[0];

   if(TDp[id] == NULL) {
     printf("%s: ERROR: TD in slot %d not initialized\n",__FUNCTION__,id);
     return ERROR;
   }



   TDLOCK;

   /* Latch Timers */
   vmeWrite32(&TDp[id]->reset, TD_RESET_LATCH_TIMERS);

   busy_stat = vmeRead32(&TDp[id]->busy);


   if(busy_stat & TD_BUSY_HFBR1)
     data[nwrds]  =  vmeRead32(&TDp[id]->busy_scaler2[1]);
   else data[nwrds] = DEFAULT_BUSY;
   nwrds++;

   if(busy_stat & TD_BUSY_HFBR2)
     data[nwrds]  =  vmeRead32(&TDp[id]->busy_scaler2[2]);
   else data[nwrds] = DEFAULT_BUSY;
   nwrds++;

   if(busy_stat & TD_BUSY_HFBR3)
     data[nwrds]  =  vmeRead32(&TDp[id]->busy_scaler2[3]);
   else data[nwrds] = DEFAULT_BUSY;
   nwrds++;

   if(busy_stat & TD_BUSY_HFBR4)
     data[nwrds]  =  vmeRead32(&TDp[id]->busy_scaler2[4]);
   else data[nwrds] = DEFAULT_BUSY;
   nwrds++;

   if(busy_stat & TD_BUSY_HFBR5)
     data[nwrds]  =  vmeRead32(&TDp[id]->busy_scaler2[5]);
   else data[nwrds] = DEFAULT_BUSY;
   nwrds++;

   if(busy_stat & TD_BUSY_HFBR6)
     data[nwrds]  =  vmeRead32(&TDp[id]->busy_scaler2[6]);
   else data[nwrds] = DEFAULT_BUSY;
   nwrds++;

   if(busy_stat & TD_BUSY_HFBR7)
     data[nwrds]  =  vmeRead32(&TDp[id]->busy_scaler2[7]);
   else data[nwrds] = DEFAULT_BUSY;
   nwrds++;

   if(busy_stat & TD_BUSY_HFBR8)
     data[nwrds]  =  vmeRead32(&TDp[id]->busy_scaler2[8]);
   else data[nwrds] = DEFAULT_BUSY;
   nwrds++;



   // Loopback
   data[nwrds]  =  vmeRead32(&TDp[id]->busy_scaler2[0]);
   nwrds++;


   data[nwrds] = vmeRead32(&TDp[id]->livetime);
   nwrds++;

   data[nwrds] = vmeRead32(&TDp[id]->busytime);
   nwrds++;

   TDUNLOCK;

   return nwrds;
}


/* ---------------- FROM ALEX SOMOV -------------- */
/***************************************************/





#else /* dummy version*/

void
tdLib_dummy()
{
  return;
}

#endif
