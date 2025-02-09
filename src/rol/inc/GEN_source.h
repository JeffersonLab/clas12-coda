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

#define DAQ_READ_CONF_FILE  {daqSetExpid(expid);    daqConfig("");     if(strncasecmp(rol->confFile,"none",4)) daqConfig(rol->confFile);}
#define TIP_READ_CONF_FILE  {tipusSetExpid(expid);  tipusConfig("");   if(strncasecmp(rol->confFile,"none",4)) tipusConfig(rol->confFile);}

#define vmeBusLock()
#define vmeBusUnlock()

static int GEN_handlers,GENflag;
static int GEN_isAsync;
static unsigned int *GENPollAddr = NULL;
static unsigned int GENPollMask;
static unsigned int GENPollValue;


#define BLOCKLEVEL 1
/*max tested value is 40*/
static int block_level = BLOCKLEVEL;
static int next_block_level = BLOCKLEVEL;


extern char *mysql_host; /* defined in coda_component.c */
extern char *expid; /* defined in coda_component.c */

extern char configname[128]; /* coda_component.c (need to add it in rolInt.h/ROLPARAMS !!??) */

/* Put any global user defined variables needed here for GEN readout */

#include "TIpcieUSLib.h"
#include "tipusConfig.h"
extern int tipusDoAck;
extern int tipusIntCount;

/*
extern void *tsLiveFunc;
extern int tsLiveCalc;
*/
extern int rocClose();

void
GEN_int_handler()
{
  theIntHandler(GEN_handlers);                   /* Call our handler */

  tipusDoAck=0; /* Make sure the library doesn't automatically ACK */
}



/*sergey: add init() */
static void
gentinit(int code)
{
  int ii, i1, i2, i3;
  unsigned int slavemask, connectmask;

  //tipusInit(TRIG_MODE,TIPUS_INIT_SKIP_FIRMWARE_CHECK | TIPUS_INIT_USE_DMA);
  tipusInit(TRIG_MODE,TIPUS_INIT_SKIP_FIRMWARE_CHECK);
  tipusSetBusySource(0,1); /* remove all busy conditions */
  tipusIntDisable();



#ifndef TI_SLAVE
  TIP_READ_CONF_FILE;
#endif



#ifdef TI_SLAVE /*slave*/

  tipusSetPrescale(0);
  tipusDisableTSInput(TIPUS_TSINPUT_ALL);
  //tipusSetBusySource(TIPUS_BUSY_FP, 1); ???

#else /*master*/

  tipusLoadTriggerTable(0);
  tipusSetPrescale(0);

  /* Enable self and front panel busy input */
  tipusSetBusySource(TIPUS_BUSY_LOOPBACK  | TIPUS_BUSY_FP ,1);
  tipusSetSyncEventInterval(0);

#endif /*slave/master*/

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

  printf("===> setting our roc_id = %d\n",rol->pid);
  tipusSetCrateID(rol->pid); /* set TI boardID equal to rocID, will be used to identify slaves */
  printf("==> reading back roc_id's: self=%d, fiber=%d\n",tipusGetCrateID(0),tipusGetCrateID(1));

  tipusIntConnect(0,isr,0);
  tipusTrigLinkReset();
  /* Fix from Bryan 17may16 */
#ifndef TI_SLAVE
  usleep(10000);
  tipusSyncReset(1);
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
  tipusStatus(1);
  tipusPrintTempVolt();
  if(GEN_isAsync==0)
  {
    GENflag = 1;
    tipusDoLibraryPollingThread(0); /* Turn off library polling */	
  }
  tipusIntEnable(1); 

}

static void 
gentdisable(int code, int card)
{
  if(GEN_isAsync==0)
  {
    GENflag = 0;
    tipusDoLibraryPollingThread(0); /* Turn off library polling */	
  }
  tipusIntDisable();
  tipusIntDisconnect();
  /*
  tsLiveCalc=0;
  tsLiveFunc = NULL;
  */
  tipusStatus(1);

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
  ret = tipusBReady();
  if(ret==-1)
  {
    printf("%s: ERROR: tipBReady returned ERROR\n",__FUNCTION__);
  }
  if(ret)
  {
    syncFlag = tipusGetSyncEventFlag();
    tipusIntCount++;
  }

  return(ret);
}

static inline void 
gentack(int code, unsigned int intMask)
{
  tipusIntAck();
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
      tipusClose();
      ended=1;
    }

}

__attribute__((constructor)) void start (void)
{
  static int started=0;

  if(started==0)
  {
      printf("\n!!!!!!!!!!!!!!! ROC Load !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");
      
      tipusOpen();
      started=1;

  }

}

#endif

