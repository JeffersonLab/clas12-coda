/******************************************************************************
*
* Header file for use General USER defined rols with CODA crl (version 2.0)
* 
*   This file implements use of the JLAB TI (pipeline) Module as a trigger interface
*
*                             Bryan Moffit  December 2012
*
*******************************************************************************/
#ifndef __GEN_ROL__
#define __GEN_ROL__

#if 0
#define DAQ_READ_CONF_FILE  /*{daqConfig("");    if(strncasecmp(rol->confFile,"none",4)) daqConfig(rol->confFile);}*/
#define TI_READ_CONF_FILE   /*{tiConfig("");     if(strncasecmp(rol->confFile,"none",4)) tiConfig(rol->confFile);}*/
#endif
#define DAQ_READ_CONF_FILE  {daqSetExpid(expid);                        daqConfig("");     if(strncasecmp(rol->confFile,"none",4)) daqConfig(rol->confFile);}
#define TIP_READ_CONF_FILE  {tipSetExpid(expid);                        tipConfig("");     if(strncasecmp(rol->confFile,"none",4)) tipConfig(rol->confFile);}

#define vmeBusLock()
#define vmeBusUnlock()

static int GEN_handlers,GENflag;
static int GEN_isAsync;
static unsigned int *GENPollAddr = NULL;
static unsigned int GENPollMask;
static unsigned int GENPollValue;

/*max tested value is 40*/
static int block_level = /*40*/BLOCKLEVEL;
static int next_block_level = BLOCKLEVEL;


extern char *mysql_host; /* defined in coda_component.c */
extern char *expid; /* defined in coda_component.c */

extern char configname[128]; /* coda_component.c (need to add it in rolInt.h/ROLPARAMS !!??) */

/* Put any global user defined variables needed here for GEN readout */
#include "TIpcieLib.h"
extern int tipDoAck;
extern int tipIntCount;
/*
extern void *tsLiveFunc;
extern int tsLiveCalc;
*/
extern int rocClose();

void
GEN_int_handler()
{
  theIntHandler(GEN_handlers);                   /* Call our handler */
  tipDoAck=0; /* Make sure the library doesn't automatically ACK */
}



/*sergey: add init() */
static void
gentinit(int code)
{
  int ii, i1, i2, i3;
  unsigned int slavemask, connectmask;

  tipInit(TRIG_MODE,/*0*/TIP_INIT_SKIP_FIRMWARE_CHECK);
  tipSetBusySource(0,1); /* remove all busy conditions */
  tipIntDisable();
  TIP_READ_CONF_FILE;

#ifdef TI_SLAVE

  tipSetPrescale(0);

  tipDisableTSInput(TIP_TSINPUT_ALL);

  tipSetBusySource(TIP_BUSY_FP ,1);
#else

  tipLoadTriggerTable(0);

  tipSetPrescale(0);

#ifdef PULSER_TRIGGER
    tipSetTriggerSource(TIP_TRIGGER_PULSER);
#else
      tipSetTriggerSource(TIP_TRIGGER_TSINPUTS);

      /* Enable input 1 and 2 */
      tipEnableTSInput(TIP_TSINPUT_ALL);
#endif
  /* Enable self and front panel busy input */

  tipSetBusySource(TIP_BUSY_LOOPBACK  | TIP_BUSY_FP ,1);
  tipSetSyncEventInterval(0);

#endif



//  tipSetBlockLevel(BLOCKLEVEL);

  /* Number of blocks allowed in the system at any given time */
//  tipSetBlockBufferLevel(BUFFERLEVEL);

  /* Sync Event interval
   *  0 : off
   *  n : Sync event every n blocks
   */


}
















/*----------------------------------------------------------------------------
  gen_trigLib.c -- Dummy trigger routines for GENERAL USER based ROLs

 File : gen_trigLib.h

 Routines:
           void gentriglink();       link interrupt with trigger
	   void gentenable();        enable trigger
	   void gentdisable();       disable trigger
	   char genttype();          return trigger type 
	   int  genttest();          test for trigger  (POLL Routine)
------------------------------------------------------------------------------*/


static void
gentriglink(int code, VOIDFUNCPTR isr)
{
  int stat=0;
  printf("TIpcie: Setting Crate ID to %d\n",rol->pid);
  tipSetCrateID(rol->pid); /* set TI boardID equal to rocID, will be used to identify slaves */

  tipIntConnect(0,isr,0);

  tipTrigLinkReset();
  /* Fix from Bryan 17may16 */
#ifndef TI_SLAVE
  usleep(10000);
  tipSyncReset(1);
#endif	
}

static void 
gentenable(int code, int card)
{
  int iflag = 1; /* Clear Interrupt scalers */
  int lockkey;
  /*
  tsLiveCalc=1;
  tsLiveFunc = tipLive;
  */
  tipStatus(1);
  tipPrintTempVolt();

  if(GEN_isAsync==0)
  {
    GENflag = 1;
    tipDoLibraryPollingThread(0); /* Turn off library polling */	
  }
  
  tipIntEnable(1); 
  
  /* Fix from Bryan 17may16 */
#ifndef TI_SLAVE
#ifdef PULSER_TRIGGER
    /* Enable TI pulser */
    if(PULSER_TYPE==0)
	{
	  tipSoftTrig(1,PULSER_FIXED_NUMBER,PULSER_FIXED_PERIOD,PULSER_FIXED_LONG);
	}
    else
	{
	  tipSetRandomTrigger(1,PULSER_RANDOM_FREQ);
	}
#endif
#endif

}

static void 
gentdisable(int code, int card)
{
  /* Fix from Bryan 17may16 */
#ifndef TI_SLAVE
#ifdef PULSER_TRIGGER
    /* Disable TI pulser */
    if(PULSER_TYPE==0)
	{
	  tipSoftTrig(1,0,0,0);
	}
    else
	{
	  tipDisableRandomTrigger();
	}
#endif
#endif

  if(GEN_isAsync==0)
  {
    GENflag = 0;
    tipDoLibraryPollingThread(0); /* Turn off library polling */	
  }
  tipIntDisable();
  tipIntDisconnect();
  /*
  tsLiveCalc=0;
  tsLiveFunc = NULL;
  */
  tipStatus(1);

}

static unsigned int
genttype(int code)
{
  unsigned int tt=0;

  tt = 1;

  return(tt);
}

static int 
genttest(int code)
{
  unsigned int ret=0;


  /*printf("genttest\n");*/
  usleep(1);
  ret = tipBReady();
  if(ret==-1)
  {
    printf("%s: ERROR: tipBReady returned ERROR\n",__FUNCTION__);
  }
  if(ret)
  {
    syncFlag = tipGetSyncEventFlag();
    tipIntCount++;
  }
  
  return(ret);
}

static inline void 
gentack(int code, unsigned int intMask)
{
  {
    tipIntAck();
  }
}


/* Define CODA readout list specific Macro routines/definitions */

#define GEN_TEST  genttest

#define GEN_INIT {GEN_handlers =0; GEN_isAsync = 0; GENflag = 0; gentinit(0);}

#define GEN_ASYNC(code,id)  {GEN_handlers = (id); GEN_isAsync = 1; gentriglink(code,GEN_int_handler);}

#define GEN_SYNC(code/*,id*/)   {GEN_handlers = 1/*(id)*/; GEN_isAsync = 0; gentriglink(code,GEN_int_handler);}

#define GEN_SETA(code) GENflag = code;

#define GEN_SETS(code) GENflag = code;

#define GEN_ENA(code,val) gentenable(code, val);

#define GEN_DIS(code,val) gentdisable(code, val);

#define GEN_CLRS(code) GENflag = 0;

#define GEN_GETID(code) GEN_handlers

#define GEN_TTYPE genttype

#define GEN_START(val)	 {;}

#define GEN_STOP(val)	 {;}

#define GEN_ENCODE(code) (code)

#define GEN_ACK(code,val)   gentack(code,val);

__attribute__((destructor)) void end (void)
{
  static int ended=0;

  if(ended==0)
    {
      printf("ROC Cleanup\n");

      rocClose();

      /*
      tsLiveCalc=0;
      tsLiveFunc = NULL;
      */

      tipClose();
      ended=1;
    }

}

__attribute__((constructor)) void start (void)
{
  static int started=0;

  if(started==0)
    {
      printf("ROC Load\n");
      
      tipOpen();
      started=1;

    }

}

#endif

