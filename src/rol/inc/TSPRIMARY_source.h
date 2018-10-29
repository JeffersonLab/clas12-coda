/******************************************************************************
*
* header file for use Linux VME defined rols with CODA
*
*                             DJA   Nov 2000
*
* SVN: $Rev: 396 $
*
*******************************************************************************/
#ifndef __TSPRIMARY_ROL__
#define __TSPRIMARY_ROL__


#define DAQ_READ_CONF_FILE  {daqSetExpid(expid); daqConfig("");  if(strncasecmp(rol->confFile,"none",4)) daqConfig(rol->confFile);}
#define TS_READ_CONF_FILE   {tsSetExpid(expid);  tsConfig("");   if(strncasecmp(rol->confFile,"none",4)) tsConfig(rol->confFile);}


#include <stdio.h>
#include <libdb.h>

#include "../jvme/jlabgef.h"
#include "../jvme/jvme.h"

#include "../code.s/tsLib.h"

extern char *mysql_host; /* defined in coda_component.c */
extern char *expid; /* defined in coda_component.c */

extern char configname[128]; /* coda_component.c (need to add it in rolInt.h/ROLPARAMS !!??) */

/* Define Hardware sources */
#define TIR_SOURCE 1
#define TS_SOURCE  2

#define TS_LEVEL 3 /*do we have it in one of the TS registers ???*/

#undef DEBUG

static int nackcalls;

/*??????????*/
static unsigned int *TSPRIMARYPollAddr = NULL;
static unsigned int TSPRIMARYPollMask;
static unsigned int TSPRIMARYPollValue;
static unsigned long TSPRIMARY_prescale = 1;
static unsigned long TSPRIMARY_count = 0;
/*??????????*/

static unsigned int ttest_ready=0;
static unsigned int ttest_not_ready=0;

void
ttestprint()
{
  printf("ttest_ready=%u ttest_not_ready=%u\n",ttest_ready,ttest_not_ready);
}

void
ttestclean()
{
  ttest_ready=0;
  ttest_not_ready=0;
}

/*----------------------------------------------------------------------------
  tsprimary_trigLib.c -- Dummy trigger routines for GENERAL USER based ROLs

 File : tsprimary_trigLib.h

 Routines:
	   void tsprimarytenable();        enable trigger
	   void tsprimarytdisable();       disable trigger
	   char tsprimaryttype();          return trigger type 
	   int  tsprimaryttest();          test for trigger  (POLL Routine)
------------------------------------------------------------------------------*/

static int TSPRIMARY_handlers;
static int TSPRIMARYflag;
static int TSPRIMARY_isAsync;

static int buffer_level = 1;

/*max tested value is 40*/
static int block_level = /*40*/1;


void
TSPRIMARY_int_handler(int arg)
{
  theIntHandler(TSPRIMARY_handlers);                   /* Call our handler */
}



static unsigned int i2_from_rol1;
static int ntd;

static void
tsprimarytinit(int code)
{
  int ii, i1, i2, i3, ret;
  unsigned int slavemask=0, connectmask=0;

  /*int overall_offset=0x80;*/

/* TEMPORARY!!! this will kill mutex so DiagGuiServer must be restarted after that !!!!!!!!!!!!!!!!!!!!! */
vmeCheckMutexHealth(1); /*- use 'mutexclean' command if needed !!!!!!!!!!!!!!!!!!!*/

  /* DMA setup */
  /*usrVmeDmaSetMemSize(0x200000);*/
  usrVmeDmaInit();
  usrVmeDmaMemory(&i1, &i2, &i3);

  i2_from_rol1 = i2;
  printf("tsprimarytinit: i2_from_rol1 = 0x%08x\n",i2_from_rol1);

  i2_from_rol1 = (i2_from_rol1 & 0xFFFFFFF0);
  printf("tsprimarytinit: i2_from_rol1 = 0x%08x\n",i2_from_rol1);

  i2_from_rol1 = i2_from_rol1 + 0x10;
  printf("tsprimarytinit: i2_from_rol1 = 0x%08x\n",i2_from_rol1);


  /* Disable IRQ for VME Bus Errors
  vmeDisableBERRIrq();
  


*/



  /* Initialize TS */

vmeBusLock();
 /*tiSetFiberLatencyOffset_preInit(0xbf);*/ /*default is 0xbf in 4ns ticks*/
  ret = tsInit(TS_ADDR,TS_READOUT,0); /*tsInit((21<<19),2,0)*/
  if(ret<0) ret = tsInit(0,TS_READOUT,0);
  if(ret<0) {printf("cannot find TS, ret=%d - exit\n",ret);exit(0);}
#ifdef DEBUG
  tsStatus(1);
#endif
  tsSetBusySource(0,1); /* remove all busy conditions */
  tsIntDisable();
  /*tsDisableVXSSignals();TS*/
  TS_READ_CONF_FILE;

#ifdef DEBUG
  printf("after tsConfig()\n");
  tsStatus(1);
#endif

  block_level = tsConfigGetBlockLevel();
  buffer_level = tsConfigGetBufferLevel();
  printf("TSPRIMARY: new block_level (config) to %d\n",block_level);
  printf("TSPRIMARY: new buffer_level (config) to %d\n",buffer_level);
vmeBusUnlock();




  /* only 1 trigger type for physics trigger */
vmeBusLock();


  /* enable 1st bitpattern word */
  tsSetGTPInputReadout(1);

  /* enable 2nd bitpattern word */
  tsSetFPInputReadout(1);

  /* enable 3rd bitpattern word */
  tsSetBeforePrescaleReadout(1);

  /*tsSetTrigCoinWindow();*/

  tsLoadTriggerTable();

  /* Set the sync delay width to 0x40*32 = 2.048us */
  tsSetSyncDelayWidth(0x30, 0x40, 1);
vmeBusUnlock();




vmeBusLock();
#ifdef DEBUG
  printf("befor tiSyncReset\n");
  tsStatus(1);
#endif

  tsSyncReset(1); /* '1' will push 'next_block_level' to 'block_level' in slave TI's (not TD's !) */

#ifdef DEBUG
  printf("after tiSyncReset\n");
  tsStatus(1);
#endif
vmeBusUnlock();
  /* fine */

#ifdef DEBUG
  printf("tiClockReset/tiTrigLinkReset\n");
#endif
  taskDelay(200);



  /* on William's advise */
vmeBusLock();
/*tsTrigDisable();TS*/
vmeBusUnlock();
  taskDelay(200);



vmeBusLock();
  tsSetSyncSource(0); /* we do not want to issue 'tiClockReset' to the master, so we set sync source to 0, and restore it after */
  tsClockReset();
  tsSetSyncSource(TS_SYNC_LOOPBACK);
#ifdef DEBUG
  printf("tiClockReset done\n");
  tsStatus(1);
#endif
vmeBusUnlock();



try_again1:
  /* check if any of fiber connections were lost; if so, resync */
vmeBusLock();
  /*connectmask = tsGetConnectedFiberMask();TS*/
  printf("FIBER CONNECT MASK: 0x%08x\n",connectmask);
  /*slavemask = tiGetSlaveMask();*/
vmeBusUnlock();
  for(ii=0; ii<8; ii++)
  {
    i1 = slavemask&(1<<ii);
    if(i1)
	{
      i2 = (i1 & connectmask) >> ii;
      printf("======> ii=%d i2=%d\n",ii,i2);
      if(i2==0)
	  {
        printf("Fiber %d lost connection - trying to recover\n");
vmeBusLock();
		tsResetMGT();
vmeBusUnlock();
        taskDelay(10);
        goto try_again1;
	  }
	}
  }

  taskDelay(200);
vmeBusLock();
  tsTrigLinkReset();
#ifdef DEBUG
  printf("tiTrigLinkReset done\n");
#endif
  tsStatus(1);
vmeBusUnlock();
  taskDelay(200);

}

/* called at prestart (CTRIGRSA) */
static void
tsprimarytriglink(int code, VOIDFUNCPTR isr)
{
  int itd, numRows, ix, port, roc_id_db, roc_id_fiber[21][9];
  char tmp[1000];
  MYSQL *dbsocket;
  MYSQL_RES *result;
  MYSQL_ROW row;

  tsSetCrateID(rol->pid); /* set TS boardID equal to rocID, will be used to identify slaves */



  /********************************************/
  /* found and add all 'inuse' rocs as slaves */

  ntd = tdGetNtds(); /* number of TD boards found  */
  for(itd=0; itd<ntd; itd++)
  {
    for(port=1; port<=8; port++)
    {
	  /*SERGEY: IGNORE CRATES WHICH ARE NOT IN CONFIG - will NOT help: sometimes TD can return some junk number
        for fibers connected to roc NOT in config, and it can coincide with rocid for roc IN config, it 
        confuses everything; should return 0 or real ID for rocs NOT in config */
vmeBusLock();
      roc_id_fiber[itd][port] = tdGetCrateID(tdSlot(itd),port);
vmeBusUnlock();
 printf("TSPRIMARY: td=%d(slot=%d), port=%d, roc_id_fiber=%d\n",itd,tdSlot(itd),port,roc_id_fiber[itd][port]);
    }
  }

  dbsocket = dbConnect(mysql_host, expid);
  if(dbsocket==NULL)
  {
    printf("TSPRIMARY: ERROR: cannot connect to the database - exit\n");
    return;
  }

  sprintf(tmp,"SELECT name,outputs,inuse FROM %s",configname);
  if(mysql_query(dbsocket, tmp) != 0)
  {
    printf("TSPRIMARY: ERROR: cannot select\n");
    return;
  }
  else
  {
    printf("selected\n");
  }

  if( !(result = mysql_store_result(dbsocket)) )
  {
    printf("TSPRIMARY: ERROR in mysql_store_result()\n");
    return;
  }
  else
  {
    numRows = mysql_num_rows(result);
    printf("TSPRIMARY: nrow=%d, my rocid=%d\n",numRows,rol->pid);

	for(ix=0; ix<numRows; ix++)
    {
      row = mysql_fetch_row(result);
      printf("TSPRIMARY: [%1d] received from DB >%s< >%s< >%s<\n",ix,row[0],row[1],row[2]);

      if( strncmp(row[2],"no",2) != 0 ) /* 'inuse' != 'no' */
      {
        roc_id_db = atoi(row[2]);
        printf("TSPRIMARY: roc_id_db = %d\n",roc_id_db);

        printf("TSPRIMARY: looping over ports ..\n");
        for(itd=0; itd<ntd; itd++)
        {
          for(port=1; port<=8; port++)
          {
            if(roc_id_db == roc_id_fiber[itd][port])
		    {
              if(roc_id_db == rol->pid) /* never here ? */
		      {
                printf("TSPRIMARY: rocid=%d - do nothing (cannot be myself's slave)\n",roc_id_db);
		      }
		      else
		      {
                printf("TSPRIMARY: added slave connected to fiber %d of the TD slot %d, rocid=%d\n",
                  port,tdSlot(itd),roc_id_db);
vmeBusLock();
                tdAddSlave(tdSlot(itd),port);
vmeBusUnlock();
		      }
              break;
            }
          }
		}

      }
    }

    mysql_free_result(result);
  }

  /* disconnect from database */
  dbDisconnect(dbsocket);

  switch(code)
  {
    case TIR_SOURCE:
      /* Initialize VME Interrupt variables
      tirClearIntCount();
      */
      /* Connect User Trigger Routine */


      break;
	  /*
    case TS_SOURCE:
      if(ts)
      {
        ts->intVec = TIR_INT_VEC;
      }
      else
      {
        printf(" tsprimarytriglink: ERROR: ts uninitialized\n");
        return;
      }


      break;
	  */

    default:
      printf(" tsprimarytriglink: ERROR: source type %d undefined\n",code);
      return;
  }

  printf("tsprimarytriglink: register int handler 0x%08x\n",isr);fflush(stdout);
}

static void 
tsprimarytenable(int val, unsigned int intMask)
{
  TSPRIMARYflag = 1;

  ttest_ready = 0;
  ttest_not_ready = 0;
  nackcalls = 0;

/*sergeytiIntEnable(val);*/

vmeBusLock();
  tsEnableTriggerSource();
vmeBusUnlock();

  /*
  tsIntEnable(0);
  */

}

static void 
tsprimarytdisable(int val, unsigned int intMask)
{
  int len;
  unsigned int tmpbuf[4000];

vmeBusLock();
  tsDisableTriggerSource(1);
  tsBlockStatus(0,1);
vmeBusUnlock();

vmeBusLock();
  tsStatus(1);
vmeBusUnlock();


  /* clear all buffers here !!! */
/*crashes EB
vmeBusLock();
  val = tsBReady();
  printf("reading remaining data from TS, val0=%d\n",val);
  while(val)
  {
    len = tsReadBlock(tmpbuf,900>>2,1);
    printf("reading remaining data from TS, len=%d\n",len);
    val = tiBReady();
    printf("reading remaining data from TS, val=%d\n",val);
  }
vmeBusUnlock();
*/


vmeBusLock();
  tsIntDisable();
vmeBusUnlock();

  TSPRIMARYflag = 0;
}

static void 
tsprimarytack(int code, unsigned int intMask)
{
  /*if(code == TIR_SOURCE)*/
  {
    /*printf("TS_PRIMARY: call tiIntAck()\n");*/
vmeBusLock();
    tsIntAck();
	/*
    nackcalls ++;
    printf("TS_PRIMARY: call tiIntAck() %d times\n",nackcalls);
	*/
vmeBusUnlock();
  }
  /*
  if(code == TS_SOURCE)
  {
    ts->lrocBufStatus = 0x100;
  }
  */
}

static unsigned int
tsprimaryttype(unsigned int code)
{
  return(1); /* not used any more: event type reported in every event in TI data, and recodred into fragment header in ROL2 */
}

/* for polling mode only */
static int 
tsprimaryttest(unsigned int code)
{
  int val=0;

  if(code == TIR_SOURCE)
  {
vmeBusLock();
    val = tsBReady();
vmeBusUnlock();
    if(val)
    {
      ttest_ready ++;
vmeBusLock();
	  syncFlag = tsGetSyncEventFlag();/*tiGetSyncEventReceived();*/
vmeBusUnlock();
      return(1);
    }
    else
    {
      ttest_not_ready ++;
      syncFlag = 0;
      return(0);
    }
  }

}




/* Define CODA readout list specific Macro routines/definitions */

#define TSPRIMARY_TEST  tsprimaryttest

#define TSPRIMARY_INIT(code) {TSPRIMARY_handlers=0;TSPRIMARY_isAsync=0; TSPRIMARYflag=0; tsprimarytinit(code);}

#define TSPRIMARY_ASYNC(code)  {TSPRIMARY_handlers = 1; TSPRIMARY_isAsync = 1; tsprimarytriglink(code,TSPRIMARY_int_handler);}

#define TSPRIMARY_SYNC(code)   {TSPRIMARY_handlers = 1; TSPRIMARY_isAsync = 0; tsprimarytriglink(code,TSPRIMARY_int_handler);}

#define TSPRIMARY_SETA(code) TSPRIMARYflag = code;

#define TSPRIMARY_SETS(code) TSPRIMARYflag = code;

#define TSPRIMARY_ENA(code,val) tsprimarytenable(code,val);

#define TSPRIMARY_DIS(code,val) tsprimarytdisable(code,val);

#define TSPRIMARY_ACK(code,val) tsprimarytack(code,val);

#define TSPRIMARY_CLRS(code) TSPRIMARYflag = 0;

#define TSPRIMARY_GETID(code) TSPRIMARY_handlers

#define TSPRIMARY_TTYPE tsprimaryttype

#define TSPRIMARY_START(val)	 {;}

#define TSPRIMARY_STOP(val)	 {tsprimarytdisable(val);}

#define TSPRIMARY_ENCODE(code) (code)


#endif

