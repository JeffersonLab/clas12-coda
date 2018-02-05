#ifdef Linux_vme

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
 *     Primitive trigger control for VME CPUs using the TJNAF Trigger
 *     Supervisor (TI) card
 *
 * </pre>
 *----------------------------------------------------------------------------*/

#define _GNU_SOURCE

#define DEVEL
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/prctl.h>
#include <pthread.h>

#include "jvme.h"
#include "tiLib.h"

/* Mutex to guard TI read/writes */
extern pthread_mutex_t   tiMutex;
#define TILOCK     if(pthread_mutex_lock(&tiMutex)<0) perror("pthread_mutex_lock");
#define TIUNLOCK   if(pthread_mutex_unlock(&tiMutex)<0) perror("pthread_mutex_unlock");

/* Global Variables */
extern volatile struct TI_A24RegStruct  *TIp;    /* pointer to TI memory map */
extern volatile        unsigned int     *TIpd;  /* pointer to TI data FIFO */
extern unsigned int        tiIntCount;
extern unsigned int        tiAckCount;
static int          tiReadoutEnabled = 1;    /* Readout enabled, by default */

extern unsigned long tiA24Offset;      /* Difference in CPU A24 Base and VME A24 Base */
extern unsigned long tiA32Offset;      /* Difference in CPU A32 Base and VME A32 Base */
extern           int tiMaster;         /* Whether or not this TI is the Master */
extern           int tiBlockLevel;     /* Current Block level for TI */
extern           int tiNextBlockLevel; /* Next Block level for TI */
extern unsigned  int tiSlaveMask;      /* TI Slaves (mask) to be used with TI Master */
extern unsigned  int tiTriggerSource;  /* Set with tiSetTriggerSource(...) */

void my_tiIntAck()
{
	int resetbits=0;
	unsigned int request = 0;

	if(TIp == NULL)
	{
		logMsg("tiIntAck: ERROR: TI not initialized\n",0,0,0,0,0,0);
		return;
	}

	TILOCK;
		tiAckCount++;
		resetbits = TI_RESET_BUSYACK;
		if(!tiReadoutEnabled)
		{
			// Readout Acknowledge and decrease the number of available blocks by 1
			resetbits |= TI_RESET_BLOCK_READOUT;
		}

		request = (vmeRead32(&TIp->blockBuffer) & TI_BLOCKBUFFER_SYNCRESET_REQUESTED)>>30;
	  	if( request )  
		{
			printf("%s: ERROR : TI_BLOCKBUFFER %x \n",__FUNCTION__, request);
		}

		vmeWrite32(&TIp->reset, resetbits);
	TIUNLOCK;

	if( request )
		tiSyncResetResync();
}


void my_tiSyncReset()
{
	int resetbits=0;
	unsigned int request = 0;

	if(TIp == NULL)
	{
		logMsg("tiIntAck: ERROR: TI not initialized\n",0,0,0,0,0,0);
		return;
	}

	TILOCK;
		request = (vmeRead32(&TIp->blockBuffer) & TI_BLOCKBUFFER_SYNCRESET_REQUESTED)>>30;
		if( request )  
		{
			printf("%s: ERROR : TI_BLOCKBUFFER  %x \n",__FUNCTION__, request);
			// resetbits |= TI_RESET_BUSYACK;
		}
		vmeWrite32(&TIp->reset, resetbits);
	TIUNLOCK;

	if( request )
		tiSyncResetResync();
}




int my_tiIntEnable(int iflag)
{
	if(TIp == NULL)
	{
		logMsg("tiIntAck: ERROR: TI not initialized\n",0,0,0,0,0,0);
		return ERROR;
	}

	if(iflag == 1)
	{
		tiIntCount = 0;
		tiAckCount = 0;
	}
	return(0);
}


void my_tiDeadtime()
{
	unsigned int livetime, busytime;

	if(TIp == NULL)
	{
		logMsg("tiIntAck: ERROR: TI not initialized\n",0,0,0,0,0,0);
		return;
	}

	/* latch live and busytime scalers */
	tiLatchTimers();

	TILOCK;
		/* Latch scalers first */
		vmeWrite32(&TIp->reset,TI_RESET_SCALERS_LATCH);
		livetime     = vmeRead32(&TIp->livetime);
		busytime     = vmeRead32(&TIp->busytime);
	TIUNLOCK;

	if ( busytime || livetime)
	{ 
		fprintf( stdout, "  deadtime %7.3f %% ",  (double) busytime  / ( (double)busytime + (double) livetime ) *100.); 
	}
}

int my_tiLive()
{
	int reg_bl = 0, bl=0, rval=0;
	float fval=0;
	unsigned int newBusy=0, newLive=0, newTotal=0;
	unsigned int live=0, total=0;
	static unsigned int oldLive=0, oldTotal=0;

	if(TIp == NULL)
	{
		logMsg("tiIntAck: ERROR: TI not initialized\n",0,0,0,0,0,0);
		return ERROR;
	}

	TILOCK;
		vmeWrite32(&TIp->reset,TI_RESET_SCALERS_LATCH);
		newLive = vmeRead32(&TIp->livetime);
		newBusy = vmeRead32(&TIp->busytime);
		//reg_bl = vmeRead32(&TIp->blocklevel);
	TIUNLOCK;
  
	bl = (reg_bl & TI_BLOCKLEVEL_CURRENT_MASK)>>16;
  
	newTotal = newLive+newBusy;
	if((oldTotal<newTotal))
	{
		/* Differential */
		live  = newLive - oldLive;
		total = newTotal - oldTotal;
    	}
	else
	{
		/* Integrated */
		live = newLive;
		total = newTotal;
	}
	oldLive = newLive;
	oldTotal = newTotal;

	if(total>0)
		fval =  (float) total;
	rval = total;

	return rval;
}

/**
 * @ingroup Status
 * @brief Save some status information of the TI to file pointed by fptr
 *
 * @param pflag if pflag>0, print out raw registers
 *
 * @param fptr pointer to FILE structure
 *
 */

void
my_tiStatusDump(int pflag, FILE* fptr)
{
  struct TI_A24RegStruct *ro;
  int iinp, iblock, ifiber;
  unsigned int blockStatus[5], nblocksReady, nblocksNeedAck;
  unsigned int fibermask;
  unsigned long TIBase;
  unsigned long long int l1a_count=0;

  /*sergey*/
  unsigned int ii;
  unsigned int trigger_rule_times[4];
  /*sergey*/

  if(TIp==NULL)
    {
      printf("%s: ERROR: TI not initialized\n",__FUNCTION__);
      return;
    }

  if(fptr==(FILE *)NULL)
    {
      printf("%s: ERROR: fptr=NULL\n",__FUNCTION__);
      return;
    }

  ro = (struct TI_A24RegStruct *) malloc(sizeof(struct TI_A24RegStruct));
  if(ro == NULL)
    {
      printf("%s: ERROR allocating memory for TI register structure\n",
	     __FUNCTION__);
      return;
    }

  /* latch live and busytime scalers */
  tiLatchTimers();
  l1a_count    = tiGetEventCounter();
  tiGetCurrentBlockLevel();

  TILOCK;
  ro->boardID      = vmeRead32(&TIp->boardID);
  ro->fiber        = vmeRead32(&TIp->fiber);
  ro->intsetup     = vmeRead32(&TIp->intsetup);
  ro->trigDelay    = vmeRead32(&TIp->trigDelay);
  ro->adr32        = vmeRead32(&TIp->adr32);
  ro->blocklevel   = vmeRead32(&TIp->blocklevel);
  ro->dataFormat   = vmeRead32(&TIp->dataFormat);
  ro->vmeControl   = vmeRead32(&TIp->vmeControl);
  ro->trigsrc      = vmeRead32(&TIp->trigsrc);
  ro->sync         = vmeRead32(&TIp->sync);
  ro->busy         = vmeRead32(&TIp->busy);
  ro->clock        = vmeRead32(&TIp->clock);
  ro->trig1Prescale = vmeRead32(&TIp->trig1Prescale);
  ro->blockBuffer  = vmeRead32(&TIp->blockBuffer);

  /*sergey*/
  ro->syncDelay    = vmeRead32(&TIp->syncDelay);
  ro->triggerRule  = vmeRead32(&TIp->triggerRule);
  ro->triggerRuleMin  = vmeRead32(&TIp->triggerRuleMin);
  /*sergey*/

  ro->tsInput      = vmeRead32(&TIp->tsInput);

  ro->output       = vmeRead32(&TIp->output);
  ro->syncEventCtrl= vmeRead32(&TIp->syncEventCtrl);
  ro->blocklimit   = vmeRead32(&TIp->blocklimit);
  ro->fiberSyncDelay = vmeRead32(&TIp->fiberSyncDelay);

  ro->GTPStatusA   = vmeRead32(&TIp->GTPStatusA);
  ro->GTPStatusB   = vmeRead32(&TIp->GTPStatusB);

  /* Latch scalers first */
  vmeWrite32(&TIp->reset,TI_RESET_SCALERS_LATCH);
  ro->livetime     = vmeRead32(&TIp->livetime);
  ro->busytime     = vmeRead32(&TIp->busytime);

  ro->inputCounter = vmeRead32(&TIp->inputCounter);

  for(iblock=0;iblock<4;iblock++)
    blockStatus[iblock] = vmeRead32(&TIp->blockStatus[iblock]);

  blockStatus[4] = vmeRead32(&TIp->adr24);

  ro->nblocks      = vmeRead32(&TIp->nblocks);

  ro->GTPtriggerBufferLength = vmeRead32(&TIp->GTPtriggerBufferLength);

  ro->rocEnable    = vmeRead32(&TIp->rocEnable);
  TIUNLOCK;

  TIBase = (unsigned long)TIp;

  fprintf(fptr,"\n");
#ifdef VXWORKS
  fprintf(fptr,"STATUS for TI at base address 0x%08x \n",
	 (unsigned int) TIp);
#else
  fprintf(fptr,"STATUS for TI at VME (Local) base address 0x%08lx (0x%lx) \n",
	 (unsigned long) TIp - tiA24Offset, (unsigned long) TIp);
#endif
  fprintf(fptr,"--------------------------------------------------------------------------------\n");

  fprintf(fptr," A32 Data buffer ");
  if((ro->vmeControl&TI_VMECONTROL_A32) == TI_VMECONTROL_A32)
    {
      fprintf(fptr,"ENABLED at ");
#ifdef VXWORKS
      fprintf(fptr,"base address 0x%.8lx\n",
	     (unsigned long)TIpd);
#else
      fprintf(fptr,"VME (Local) base address 0x%08lx (0x%lx)\n",
	     (unsigned long)TIpd - tiA32Offset, (unsigned long)TIpd);
#endif
    }
  else
    fprintf(fptr,"DISABLED\n");

  if(tiMaster)
    fprintf(fptr," Configured as a TI Master\n");
  else
    fprintf(fptr," Configured as a TI Slave\n");

  fprintf(fptr," Readout Count: %d\n",tiIntCount);
  fprintf(fptr,"     Ack Count: %d\n",tiAckCount);
  fprintf(fptr,"     L1A Count: %llu\n",l1a_count);
  fprintf(fptr,"   Block Limit: %d   %s\n",ro->blocklimit,
	 (ro->blockBuffer & TI_BLOCKBUFFER_BUSY_ON_BLOCKLIMIT)?"* Finished *":"");
  fprintf(fptr,"   Block Count: %d\n",ro->nblocks & TI_NBLOCKS_COUNT_MASK);

  if(pflag>0)
    {
      fprintf(fptr,"\n");
      fprintf(fptr," Registers (offset):\n");
      fprintf(fptr,"  boardID        (0x%04lx) = 0x%08x\t", (unsigned long)&TIp->boardID - TIBase, ro->boardID);
      fprintf(fptr,"  fiber          (0x%04lx) = 0x%08x\n", (unsigned long)(&TIp->fiber) - TIBase, ro->fiber);
      fprintf(fptr,"  intsetup       (0x%04lx) = 0x%08x\t", (unsigned long)(&TIp->intsetup) - TIBase, ro->intsetup);
      fprintf(fptr,"  trigDelay      (0x%04lx) = 0x%08x\n", (unsigned long)(&TIp->trigDelay) - TIBase, ro->trigDelay);
      fprintf(fptr,"  adr32          (0x%04lx) = 0x%08x\t", (unsigned long)(&TIp->adr32) - TIBase, ro->adr32);
      fprintf(fptr,"  blocklevel     (0x%04lx) = 0x%08x\n", (unsigned long)(&TIp->blocklevel) - TIBase, ro->blocklevel);
      fprintf(fptr,"  dataFormat     (0x%04lx) = 0x%08x\t", (unsigned long)(&TIp->dataFormat) - TIBase, ro->dataFormat);
      fprintf(fptr,"  vmeControl     (0x%04lx) = 0x%08x\n", (unsigned long)(&TIp->vmeControl) - TIBase, ro->vmeControl);
      fprintf(fptr,"  trigger        (0x%04lx) = 0x%08x\t", (unsigned long)(&TIp->trigsrc) - TIBase, ro->trigsrc);
      fprintf(fptr,"  sync           (0x%04lx) = 0x%08x\n", (unsigned long)(&TIp->sync) - TIBase, ro->sync);
      fprintf(fptr,"  busy           (0x%04lx) = 0x%08x\t", (unsigned long)(&TIp->busy) - TIBase, ro->busy);
      fprintf(fptr,"  clock          (0x%04lx) = 0x%08x\n", (unsigned long)(&TIp->clock) - TIBase, ro->clock);
      fprintf(fptr,"  blockBuffer    (0x%04lx) = 0x%08x\t", (unsigned long)(&TIp->blockBuffer) - TIBase, ro->blockBuffer);

      fprintf(fptr,"  output         (0x%04lx) = 0x%08x\n", (unsigned long)(&TIp->output) - TIBase, ro->output);
      fprintf(fptr,"  fiberSyncDelay (0x%04lx) = 0x%08x\t", (unsigned long)(&TIp->fiberSyncDelay) - TIBase, ro->fiberSyncDelay);
      fprintf(fptr,"  nblocks        (0x%04lx) = 0x%08x\n", (unsigned long)(&TIp->nblocks) - TIBase, ro->nblocks);

      fprintf(fptr,"  GTPStatusA     (0x%04lx) = 0x%08x\t", (unsigned long)(&TIp->GTPStatusA) - TIBase, ro->GTPStatusA);
      fprintf(fptr,"  GTPStatusB     (0x%04lx) = 0x%08x\n", (unsigned long)(&TIp->GTPStatusB) - TIBase, ro->GTPStatusB);

      fprintf(fptr,"  livetime       (0x%04lx) = 0x%08x\t", (unsigned long)(&TIp->livetime) - TIBase, ro->livetime);
      fprintf(fptr,"  busytime       (0x%04lx) = 0x%08x\n", (unsigned long)(&TIp->busytime) - TIBase, ro->busytime);
      fprintf(fptr,"  GTPTrgBufLen   (0x%04lx) = 0x%08x\t", (unsigned long)(&TIp->GTPtriggerBufferLength) - TIBase, ro->GTPtriggerBufferLength);

      /*sergey*/
      fprintf(fptr,"  syncDelay      (0x%04lx) = 0x%08x\n", (unsigned long)(&TIp->syncDelay) - TIBase, ro->syncDelay);
      fprintf(fptr,"  triggerRule    (0x%04lx) = 0x%08x\t", (unsigned long)(&TIp->triggerRule) - TIBase, ro->triggerRule);
      fprintf(fptr,"  triggerRuleMin (0x%04lx) = 0x%08x\n", (unsigned long)(&TIp->triggerRuleMin) - TIBase, ro->triggerRuleMin);
	  /*sergey*/

      fprintf(fptr,"\n");
    }
  fprintf(fptr,"\n");

  if((!tiMaster) && (tiBlockLevel==0))
    {
      fprintf(fptr," Block Level not yet received\n");
    }
  else
    {
      fprintf(fptr," Block Level        = %d ", tiBlockLevel);
      if(tiBlockLevel != tiNextBlockLevel)
	fprintf(fptr,"(To be set = %d)\n", tiNextBlockLevel);
      else
	fprintf(fptr,"\n");
    }

  fprintf(fptr," Block Buffer Level = ");
  if(ro->vmeControl & TI_VMECONTROL_USE_LOCAL_BUFFERLEVEL)
    {
      fprintf(fptr,"%d -Local- ",
	     ro->blockBuffer & TI_BLOCKBUFFER_BUFFERLEVEL_MASK);
    }
  else
    {
      fprintf(fptr,"%d -Broadcast- ",
	     (ro->dataFormat & TI_DATAFORMAT_BCAST_BUFFERLEVEL_MASK) >> 24);
    }

  fprintf(fptr,"(%s)\n",(ro->vmeControl & TI_VMECONTROL_BUSY_ON_BUFFERLEVEL)?
	 "Busy Enabled":"Busy not enabled");

  if(tiMaster)
    {
      if((ro->syncEventCtrl & TI_SYNCEVENTCTRL_NBLOCKS_MASK) == 0)
	fprintf(fptr," Sync Events DISABLED\n");
      else
	fprintf(fptr," Sync Event period  = %d blocks\n",
	       ro->syncEventCtrl & TI_SYNCEVENTCTRL_NBLOCKS_MASK);
    }

  fprintf(fptr,"\n");
  fprintf(fptr," Fiber Status         1     2     3     4     5     6     7     8\n");
  fprintf(fptr,"                    ----- ----- ----- ----- ----- ----- ----- -----\n");
  fprintf(fptr,"  Connected          ");
  for(ifiber=0; ifiber<8; ifiber++)
    {
      fprintf(fptr,"%s   ",
	     (ro->fiber & TI_FIBER_CONNECTED_TI(ifiber+1))?"YES":"   ");
    }
  fprintf(fptr,"\n");
  if(tiMaster)
    {
      fprintf(fptr,"  Trig Src Enabled   ");
      for(ifiber=0; ifiber<8; ifiber++)
	{
	  fprintf(fptr,"%s   ",
		 (ro->fiber & TI_FIBER_TRIGSRC_ENABLED_TI(ifiber+1))?"YES":"   ");
	}
    }
  fprintf(fptr,"\n\n");

  if(tiMaster)
    {
      if(tiSlaveMask)
	{
	  fprintf(fptr," TI Slaves Configured on HFBR (0x%x) = ",tiSlaveMask);
	  fibermask = tiSlaveMask;
	  for(ifiber=0; ifiber<8; ifiber++)
	    {
	      if( fibermask & (1<<ifiber))
		fprintf(fptr," %d",ifiber+1);
	    }
	  fprintf(fptr,"\n");
	}
      else
	fprintf(fptr," No TI Slaves Configured on HFBR\n");

    }

  fprintf(fptr," Clock Source (%d) = \n",ro->clock & TI_CLOCK_MASK);
  switch(ro->clock & TI_CLOCK_MASK)
    {
    case TI_CLOCK_INTERNAL:
      fprintf(fptr,"   Internal\n");
      break;

    case TI_CLOCK_HFBR5:
      fprintf(fptr,"   HFBR #5 Input\n");
      break;

    case TI_CLOCK_HFBR1:
      fprintf(fptr,"   HFBR #1 Input\n");
      break;

    case TI_CLOCK_FP:
      fprintf(fptr,"   Front Panel\n");
      break;

    default:
      fprintf(fptr,"   UNDEFINED!\n");
    }

  if(tiTriggerSource&TI_TRIGSRC_SOURCEMASK)
    {
      if(ro->trigsrc)
	fprintf(fptr," Trigger input source (%s) =\n",
	       (ro->blockBuffer & TI_BLOCKBUFFER_BUSY_ON_BLOCKLIMIT)?"DISABLED on Block Limit":
	       "ENABLED");
      else
	fprintf(fptr," Trigger input source (DISABLED) =\n");
      if(tiTriggerSource & TI_TRIGSRC_P0)
	fprintf(fptr,"   P0 Input\n");
      if(tiTriggerSource & TI_TRIGSRC_HFBR1)
	fprintf(fptr,"   HFBR #1 Input\n");
      if(tiTriggerSource & TI_TRIGSRC_HFBR5)
	fprintf(fptr,"   HFBR #5 Input\n");
      if(tiTriggerSource & TI_TRIGSRC_LOOPBACK)
	fprintf(fptr,"   Loopback\n");
      if(tiTriggerSource & TI_TRIGSRC_FPTRG)
	fprintf(fptr,"   Front Panel TRG\n");
      if(tiTriggerSource & TI_TRIGSRC_VME)
	fprintf(fptr,"   VME Command\n");
      if(tiTriggerSource & TI_TRIGSRC_TSINPUTS)
	fprintf(fptr,"   Front Panel TS Inputs\n");
      if(tiTriggerSource & TI_TRIGSRC_TSREV2)
	fprintf(fptr,"   Trigger Supervisor (rev2)\n");
      if(tiTriggerSource & TI_TRIGSRC_PULSER)
	fprintf(fptr,"   Internal Pulser\n");
      if(tiTriggerSource & TI_TRIGSRC_PART_1)
	fprintf(fptr,"   TS Partition 1 (HFBR #1)\n");
      if(tiTriggerSource & TI_TRIGSRC_PART_2)
	fprintf(fptr,"   TS Partition 2 (HFBR #1)\n");
      if(tiTriggerSource & TI_TRIGSRC_PART_3)
	fprintf(fptr,"   TS Partition 3 (HFBR #1)\n");
      if(tiTriggerSource & TI_TRIGSRC_PART_4)
	fprintf(fptr,"   TS Partition 4 (HFBR #1)\n");
    }
  else
    {
      fprintf(fptr," No Trigger input sources\n");
    }

  if(ro->sync&TI_SYNC_SOURCEMASK)
    {
      fprintf(fptr," Sync source = \n");
      if(ro->sync & TI_SYNC_P0)
	fprintf(fptr,"   P0 Input\n");
      if(ro->sync & TI_SYNC_HFBR1)
	fprintf(fptr,"   HFBR #1 Input\n");
      if(ro->sync & TI_SYNC_HFBR5)
	fprintf(fptr,"   HFBR #5 Input\n");
      if(ro->sync & TI_SYNC_FP)
	fprintf(fptr,"   Front Panel Input\n");
      if(ro->sync & TI_SYNC_LOOPBACK)
	fprintf(fptr,"   Loopback\n");
      if(ro->sync & TI_SYNC_USER_SYNCRESET_ENABLED)
	fprintf(fptr,"   User SYNCRESET Receieve Enabled\n");
    }
  else
    {
      fprintf(fptr," No SYNC input source configured\n");
    }

  if(ro->busy&TI_BUSY_SOURCEMASK)
    {
      fprintf(fptr," BUSY input source = \n");
      if(ro->busy & TI_BUSY_SWA)
	fprintf(fptr,"   Switch Slot A    %s\n",(ro->busy&TI_BUSY_MONITOR_SWA)?"** BUSY **":"");
      if(ro->busy & TI_BUSY_SWB)
	fprintf(fptr,"   Switch Slot B    %s\n",(ro->busy&TI_BUSY_MONITOR_SWB)?"** BUSY **":"");
      if(ro->busy & TI_BUSY_P2)
	fprintf(fptr,"   P2 Input         %s\n",(ro->busy&TI_BUSY_MONITOR_P2)?"** BUSY **":"");
      if(ro->busy & TI_BUSY_TRIGGER_LOCK)
	fprintf(fptr,"   Trigger Lock     \n");
      if(ro->busy & TI_BUSY_FP_FTDC)
	fprintf(fptr,"   Front Panel TDC  %s\n",(ro->busy&TI_BUSY_MONITOR_FP_FTDC)?"** BUSY **":"");
      if(ro->busy & TI_BUSY_FP_FADC)
	fprintf(fptr,"   Front Panel ADC  %s\n",(ro->busy&TI_BUSY_MONITOR_FP_FADC)?"** BUSY **":"");
      if(ro->busy & TI_BUSY_FP)
	fprintf(fptr,"   Front Panel      %s\n",(ro->busy&TI_BUSY_MONITOR_FP)?"** BUSY **":"");
      if(ro->busy & TI_BUSY_LOOPBACK)
	fprintf(fptr,"   Loopback         %s\n",(ro->busy&TI_BUSY_MONITOR_LOOPBACK)?"** BUSY **":"");
      if(ro->busy & TI_BUSY_HFBR1)
	fprintf(fptr,"   HFBR #1          %s\n",(ro->busy&TI_BUSY_MONITOR_HFBR1)?"** BUSY **":"");
      if(ro->busy & TI_BUSY_HFBR2)
	fprintf(fptr,"   HFBR #2          %s\n",(ro->busy&TI_BUSY_MONITOR_HFBR2)?"** BUSY **":"");
      if(ro->busy & TI_BUSY_HFBR3)
	fprintf(fptr,"   HFBR #3          %s\n",(ro->busy&TI_BUSY_MONITOR_HFBR3)?"** BUSY **":"");
      if(ro->busy & TI_BUSY_HFBR4)
	fprintf(fptr,"   HFBR #4          %s\n",(ro->busy&TI_BUSY_MONITOR_HFBR4)?"** BUSY **":"");
      if(ro->busy & TI_BUSY_HFBR5)
	fprintf(fptr,"   HFBR #5          %s\n",(ro->busy&TI_BUSY_MONITOR_HFBR5)?"** BUSY **":"");
      if(ro->busy & TI_BUSY_HFBR6)
	fprintf(fptr,"   HFBR #6          %s\n",(ro->busy&TI_BUSY_MONITOR_HFBR6)?"** BUSY **":"");
      if(ro->busy & TI_BUSY_HFBR7)
	fprintf(fptr,"   HFBR #7          %s\n",(ro->busy&TI_BUSY_MONITOR_HFBR7)?"** BUSY **":"");
      if(ro->busy & TI_BUSY_HFBR8)
	fprintf(fptr,"   HFBR #8          %s\n",(ro->busy&TI_BUSY_MONITOR_HFBR8)?"** BUSY **":"");
    }
  else
    {
      fprintf(fptr," No BUSY input source configured\n");
    }
  if(tiMaster)
    {
      if(ro->tsInput & TI_TSINPUT_MASK)
	{
	  fprintf(fptr," Front Panel TS Inputs Enabled: ");
	  for(iinp=0; iinp<6; iinp++)
	    {
	      if( (ro->tsInput & TI_TSINPUT_MASK) & (1<<iinp))
		fprintf(fptr," %d",iinp+1);
	    }
	  fprintf(fptr,"\n");
	}
      else
	{
	  fprintf(fptr," All Front Panel TS Inputs Disabled\n");
	}
    }

  if(tiMaster)
    {
      fprintf(fptr,"\n");
      fprintf(fptr," Trigger Rules:\n");
      tiPrintTriggerHoldoff(pflag);
    }

  if(tiMaster)
    {
      if(ro->rocEnable & TI_ROCENABLE_SYNCRESET_REQUEST_ENABLE_MASK)
	{
	  fprintf(fptr," SyncReset Request ENABLED from ");

	  if(ro->rocEnable & (1 << 10))
	    {
	      fprintf(fptr,"SELF ");
	    }

	  for(ifiber=0; ifiber<8; ifiber++)
	    {
	      if(ro->rocEnable & (1 << (ifiber + 1 + 10)))
		{
		  fprintf(fptr,"%d ", ifiber + 1);
		}
	    }

	  fprintf(fptr,"\n");
	}
      else
	{
	  fprintf(fptr," SyncReset Requests DISABLED\n");
	}

      fprintf(fptr,"\n");
      tiSyncResetRequestStatus(1);
    }
  fprintf(fptr,"\n");

  if(ro->intsetup&TI_INTSETUP_ENABLE)
    fprintf(fptr," Interrupts ENABLED\n");
  else
    fprintf(fptr," Interrupts DISABLED\n");
  fprintf(fptr,"   Level = %d   Vector = 0x%02x\n",
	 (ro->intsetup&TI_INTSETUP_LEVEL_MASK)>>8, (ro->intsetup&TI_INTSETUP_VECTOR_MASK));

  if(ro->vmeControl&TI_VMECONTROL_BERR)
    fprintf(fptr," Bus Errors Enabled\n");
  else
    fprintf(fptr," Bus Errors Disabled\n");

  fprintf(fptr,"\n");
  fprintf(fptr," Blocks ready for readout: %d\n",(ro->blockBuffer&TI_BLOCKBUFFER_BLOCKS_READY_MASK)>>8);
  if(tiMaster)
    {
      fprintf(fptr," Slave Block Status:   %s\n",
	     (ro->busy&TI_BUSY_MONITOR_TRIG_LOST)?"** Waiting for Trigger Ack **":"");
      /* TI slave block status */
      fibermask = tiSlaveMask;
      for(ifiber=0; ifiber<8; ifiber++)
	{
	  if( fibermask & (1<<ifiber) )
	    {
	      if( (ifiber % 2) == 0)
		{
		  nblocksReady   = blockStatus[ifiber/2] & TI_BLOCKSTATUS_NBLOCKS_READY0;
		  nblocksNeedAck = (blockStatus[ifiber/2] & TI_BLOCKSTATUS_NBLOCKS_NEEDACK0)>>8;
		}
	      else
		{
		  nblocksReady   = (blockStatus[(ifiber-1)/2] & TI_BLOCKSTATUS_NBLOCKS_READY1)>>16;
		  nblocksNeedAck = (blockStatus[(ifiber-1)/2] & TI_BLOCKSTATUS_NBLOCKS_NEEDACK1)>>24;
		}
	      fprintf(fptr,"  Fiber %d  :  Blocks ready / need acknowledge: %d / %d\n",
		     ifiber+1,nblocksReady, nblocksNeedAck);
	    }
	}

      /* TI master block status */
      nblocksReady   = (blockStatus[4] & TI_BLOCKSTATUS_NBLOCKS_READY1)>>16;
      nblocksNeedAck = (blockStatus[4] & TI_BLOCKSTATUS_NBLOCKS_NEEDACK1)>>24;
      fprintf(fptr,"  Loopback :  Blocks ready / need acknowledge: %d / %d\n",
	     nblocksReady, nblocksNeedAck);

    }

  fprintf(fptr,"\n");
  fprintf(fptr," Input counter %d\n",ro->inputCounter);

  fprintf(fptr,"--------------------------------------------------------------------------------\n");
  fprintf(fptr,"\n\n");

  if(ro)
    free(ro);
}



#else

void tiUtils_dummy() {}

#endif
