/******************************************************************************
*
* header file for use Linux VME defined rols with CODA
*
*                             DJA   Nov 2000
*
* SVN: $Rev: 396 $
*
*******************************************************************************/
#ifndef __TIPRIMARY_ROL__
#define __TIPRIMARY_ROL__


#define DAQ_READ_CONF_FILE  {daqSetExpid(expid);                        daqConfig("");     if(strncasecmp(rol->confFile,"none",4)) daqConfig(rol->confFile);}
#define TI_READ_CONF_FILE   {tiSetExpid(expid);                         tiConfig("");      if(strncasecmp(rol->confFile,"none",4)) tiConfig(rol->confFile);}
#define DSC2_READ_CONF_FILE {dsc2SetExpid(expid);                       dsc2Config("");    if(strncasecmp(rol->confFile,"none",4)) dsc2Config(rol->confFile);}
#define DCRB_READ_CONF_FILE {dcrbSetExpid(expid);                       dcrbConfig("");    if(strncasecmp(rol->confFile,"none",4)) dcrbConfig(rol->confFile);}
#define VSCM_READ_CONF_FILE {vscmSetExpid(expid);                       vscmConfig("");    if(strncasecmp(rol->confFile,"none",4)) vscmConfig(rol->confFile);}
#define FADC_READ_CONF_FILE {fadc250SetExpid(expid);                    fadc250Config(""); if(strncasecmp(rol->confFile,"none",4)) fadc250Config(rol->confFile);}
#define SSP_READ_CONF_FILE  {sspSetExpid(expid);     sspInitGlobals();  sspConfig("");     if(strncasecmp(rol->confFile,"none",4)) sspConfig(rol->confFile);}
#define GTP_READ_CONF_FILE  {gtpSetExpid(expid);                        gtpConfig("");     if(strncasecmp(rol->confFile,"none",4)) gtpConfig(rol->confFile);}
#define TDC_READ_CONF_FILE  {tdc1290SetExpid(expid);                    tdc1190Config(""); if(strncasecmp(rol->confFile,"none",4)) tdc1190Config(rol->confFile);}
#define MVT_READ_CONF_FILE  {mvtSetExpid(expid);                        mvtConfig("");     if(strncasecmp(rol->confFile,"none",4)) mvtConfig(rol->confFile);}
#define FTT_READ_CONF_FILE  {fttConfig("");                                                if(strncasecmp(rol->confFile,"none",4)) fttConfig(rol->confFile);}
#define FLP_READ_CONF_FILE  {flpSetExpid(expid);                        flpConfig("");     if(strncasecmp(rol->confFile,"none",4)) flpConfig(rol->confFile);}


#include <stdio.h>
#include <libdb.h>

#ifndef VXWORKS
#include "../jvme/jlabgef.h"
#include "../jvme/jvme.h"
#endif

#include "../code.s/tiLib.h"

static unsigned int dmaMemSize; /* DMA memory size, see below */

#ifdef VXWORKS
extern unsigned int tsIntCount;
extern struct TI_A24RegStruct *TIp;
extern int tiDoAck;
#endif

extern char *mysql_host; /* defined in coda_component.c */
extern char *expid; /* defined in coda_component.c */

extern char configname[128]; /* coda_component.c (need to add it in rolInt.h/ROLPARAMS !!??) */

/* Define Hardware sources */
#define TIR_SOURCE 1
#define TS_SOURCE  2

#define TS_LEVEL 3 /*do we have it in one of the TS registers ???*/


static int nackcalls;

/*??????????*/
static unsigned int *TIPRIMARYPollAddr = NULL;
static unsigned int TIPRIMARYPollMask;
static unsigned int TIPRIMARYPollValue;
static unsigned long TIPRIMARY_prescale = 1;
static unsigned long TIPRIMARY_count = 0;
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
  tiprimary_trigLib.c -- Dummy trigger routines for GENERAL USER based ROLs

 File : tiprimary_trigLib.h

 Routines:
	   void tiprimarytenable();        enable trigger
	   void tiprimarytdisable();       disable trigger
	   char tiprimaryttype();          return trigger type 
	   int  tiprimaryttest();          test for trigger  (POLL Routine)
------------------------------------------------------------------------------*/

static int TIPRIMARY_handlers;
static int TIPRIMARYflag;
static int TIPRIMARY_isAsync;

/*max tested value is 40*/
static int block_level = /*40*/1;
static int next_block_level = 1;

#ifdef VXWORKS

static unsigned int tdcbuftmp[131072];

void
TIPRIMARY_int_handler()
{
  /*logMsg("TIPRIMARY_int_handler reached\n",1,2,3,4,5,6);*/
  theIntHandler(TIPRIMARY_handlers);                   /* Call our handler */
  tiDoAck=0; /* Make sure the library doesn't automatically ACK */
}

#else

void
TIPRIMARY_int_handler(int arg)
{
  theIntHandler(TIPRIMARY_handlers);                   /* Call our handler */
}

#endif


static unsigned int i2_from_rol1;

static void
tiprimarytinit(int code)
{
  int ii, i1, i2, i3, ret;
  unsigned int slavemask, connectmask;

  /*int overall_offset=0x80;*/




/* TEMPORARY!!! this will kill mutex so DiagGuiServer must be restarted after that !!!!!!!!!!!!!!!!!!!!! */
vmeCheckMutexHealth(1); /*- use 'mutexclean' command if needed !!!!!!!!!!!!!!!!!!!*/



  /* DMA setup */

/* increase DMA memory size here if necessary */
usrVmeDmaSetMemSize(0x200000);

/* usrVmeDmaSetMemSize(0x800000); produces error:
 usrVmeDmaSetMemSize: set memSize to 0x00800000 (8 MB)
 usrVmeDmaGetMemSize: memSize = 0x00800000 (8 MB)
 usrVmeDmaInit ERROR: gefVmeAllocDmaBuf returned 0x30100002
*/

 dmaMemSize = usrVmeDmaGetMemSize();
 usrVmeDmaInit();
#ifdef VXWORKS
  i2 = &tdcbuftmp[0];
#else
  usrVmeDmaMemory(&i1, &i2, &i3);
#endif
  i2_from_rol1 = i2;
  printf("tiprimarytinit: i2_from_rol1 = 0x%08x\n",i2_from_rol1);

  i2_from_rol1 = (i2_from_rol1 & 0xFFFFFFF0);
  printf("tiprimarytinit: i2_from_rol1 = 0x%08x\n",i2_from_rol1);

  i2_from_rol1 = i2_from_rol1 + 0x10;
  printf("tiprimarytinit: i2_from_rol1 = 0x%08x\n",i2_from_rol1);



  /*!!! MUST CALL following somewhere ???
  vmeCloseDefaultWindows();
  */

  /* Disable IRQ for VME Bus Errors
  vmeDisableBERRIrq();
  */

  /* Initialize VME Interrupt interface - use defaults */
vmeBusLock();
 /*tiSetFiberLatencyOffset_preInit(0xbf);*/ /*default is 0xbf in 4ns ticks*/
  ret = tiInit(TI_ADDR,TI_READOUT,0); /*tiInit((21<<19),2,0)*/
  if(ret<0) ret = tiInit(0,TI_READOUT,0);
  if(ret<0)
  {
    printf("ERROR: tiInit() returns %d \n",ret);
    UDP_user_request(MSGERR, "rol1", "tiInit() failure. Do 'Reset' and  'Download' again !");
    /*exit(0);*/
  }
#ifdef DEBUG
  tiStatus(1);
#endif
  tiSetBusySource(0,1); /* remove all busy conditions */
  tiIntDisable();
  tiDisableVXSSignals();
  TI_READ_CONF_FILE;
#ifdef DEBUG
  printf("after tiConfig()\n");
  tiStatus(1);
#endif
  block_level = tiConfigGetBlockLevel();
  printf("TIPRIMARY: new block_level (config) to %d\n",block_level);
vmeBusUnlock();



#ifdef TI_SLAVE

vmeBusLock();
  tiDisableTSInput(TI_TSINPUT_ALL);
vmeBusUnlock();

#else

  /* only 1 trigger type for physics trigger */
vmeBusLock();
/*tiSetTriggerSource(TI_TRIGGER_TSINPUTS); move to config */

/* moved to config
  tiDisableTSInput(TI_TSINPUT_ALL);
  tiEnableTSInput( TI_TSINPUT_1 | TI_TSINPUT_2 | TI_TSINPUT_3 | TI_TSINPUT_4 | TI_TSINPUT_5 | TI_TSINPUT_6);
*/
  tiLoadTriggerTable(3);

vmeBusUnlock();

#endif






#ifndef TI_SLAVE /* master and standalone crates, NOT slave */

vmeBusLock();
#ifdef DEBUG
  printf("befor tiSyncReset\n");
  tiStatus(1);
#endif
  tiSyncReset(0);
#ifdef DEBUG
  printf("after tiSyncReset\n");
  tiStatus(1);
#endif
vmeBusUnlock();
  /* fine */

#ifdef DEBUG
  printf("tiClockReset/tiTrigLinkReset\n");
#endif
  taskDelay(200);



  /* on William's advise */
vmeBusLock();
  tiTrigDisable();
vmeBusUnlock();
  taskDelay(200);



vmeBusLock();
  tiSetSyncSource(0); /* we do not want to issue 'tiClockReset' to the master, so we set sync source to 0, and restore it after */
  tiClockReset();
  tiSetSyncSource(TI_SYNC_LOOPBACK);
#ifdef DEBUG
  printf("tiClockReset done\n");
  tiStatus(1);
#endif
vmeBusUnlock();



try_again1:
  /* check if any of fiber connections were lost; if so, resync */
vmeBusLock();
  connectmask = tiGetConnectedFiberMask();
  printf("FIBER CONNECT MASK: 0x%08x\n",connectmask);
  slavemask = tiGetSlaveMask(); /* ERROR: we do not have it yet, addSlave() will be called in Prestart !!! */
  printf("FIBER SLAVE MASK: 0x%08x\n",slavemask);
vmeBusUnlock();
  for(ii=0; ii<8; ii++)
  {
    i1 = slavemask&(1<<ii);
    if(i1)
	{
      i2 = (i1 & connectmask) >> ii;
      printf("======> ii=%d i1=%d i2=%d\n",ii,i1,i2);
      if(i2==0)
	  {
        printf("Fiber %d lost connection - trying to recover\n");
vmeBusLock();
		tiResetMGT();
vmeBusUnlock();
        taskDelay(10);
        goto try_again1;
	  }
	}
  }

  taskDelay(200);
vmeBusLock();
  tiTrigLinkReset();
#ifdef DEBUG
  printf("tiTrigLinkReset done\n");
#endif
  tiStatus(1);
vmeBusUnlock();
  taskDelay(200);

#else

vmeBusLock();
  tiStatus(1);
vmeBusUnlock();

#endif






}



/* called at prestart (CTRIGRSA) */
static void
tiprimarytriglink(int code, VOIDFUNCPTR isr)
{
  int numRows, ix, port, roc_id_db, roc_id_fiber[9], have_sync_vtp;
  char tmp[1000];
  char ourhostname[128], ourvtphostname[128], *p;
  MYSQL *dbsocket;
  MYSQL_RES *result;
  MYSQL_ROW row;

  tiSetCrateID(rol->pid); /* set TI boardID equal to rocID, will be used to identify slaves */
vmeBusLock();
  next_block_level = tiGetNextBlockLevel();
vmeBusUnlock();
  printf("TIPRIMARY: next_block_level = %d\n",next_block_level);



#ifdef USE_HPS
vmeBusLock();
  if(rol->pid==37||rol->pid==39) tiRemoveRocSWA(); /*temporary: remove GTPs by default*/
vmeBusUnlock();
#endif

/*TEST
if(rol->pid==1||rol->pid==38) tiAddRocSWA();
else                          tiRemoveRocSWA();
TEST*/



  /********************************************/
  /* found and add all 'inuse' rocs as slaves */

  for(port=1; port<=8; port++)
  {
vmeBusLock();
    roc_id_fiber[port] = tiGetCrateID(port);
vmeBusUnlock();
	printf("TIPRIMARY: port=%d, roc_id_fiber=%d\n",port,roc_id_fiber[port]);
  }

  dbsocket = dbConnect(mysql_host, expid);
  if(dbsocket==NULL)
  {
    printf("TIPRIMARY: ERROR: cannot connect to the database - exit\n");
    return;
  }

  sprintf(tmp,"SELECT name,outputs,inuse FROM %s",configname);
  if(mysql_query(dbsocket, tmp) != 0)
  {
    printf("TIPRIMARY: ERROR: cannot select\n");
    return;
  }
  else
  {
    printf("selected\n");
  }

  if( !(result = mysql_store_result(dbsocket)) )
  {
    printf("TIPRIMARY: ERROR in mysql_store_result()\n");
    return;
  }
  else
  {
    numRows = mysql_num_rows(result);
    printf("TIPRIMARY: nrow=%d, my rocid=%d\n",numRows,rol->pid);


	/* get our hostname and our vtp hostname if it exist */
    strcpy(tmp,getenv("HOST"));
    if( (p=strchr(tmp,'.'))!=NULL )
	{
      printf("Will use everything before first '.' appearance in HOST=>%s<\n",tmp);
      *p = '\0';
	}
    sprintf(ourhostname,"%s",tmp);
    sprintf(ourvtphostname,"%svtp",tmp);
    printf("TIPRIMARY: our hostname is >%s<, our vtp hostname is >%s<\n",ourhostname,ourvtphostname);
    have_sync_vtp = 0;




    /***************************************/
    /* loop over all rows in configuration */

	for(ix=0; ix<numRows; ix++)
    {
      row = mysql_fetch_row(result);
      printf("TIPRIMARY: [%1d] received from DB >%s< >%s< >%s<\n",ix,row[0],row[1],row[2]);


      /* check if we have vtp as sync roc in our crate; assuming that vtp
		 hostname will be our name with 'vtp' in the end, and 'outputs' for vtp roc is not empty */
      if( strncmp(row[0],ourvtphostname,strlen(ourvtphostname)) == 0 ) /* we have vtp */
	  {
        if( strlen(row[1]) > 2)
		{
          have_sync_vtp = 1; /* vtp have output */
          printf("TIPRIMARY: VTP installed in our crate, and it SYNC'ed\n");
		}
      }




/*SERGEY: DO tiAddSlave() FOR MASTER OR STANDALONE ONLY; HAVE TO RESOLVE HPS PART WHERE SOMETHING ELSE IS DONE AS WELL ... */
#ifndef TI_SLAVE

      /* 'inuse' field has roc id */
      if( strncmp(row[2],"no",2) != 0 ) /* 'inuse' != 'no' */
      {
        roc_id_db = atoi(row[2]);
        printf("   TIPRIMARY: roc_id_db = %d\n",roc_id_db);

#ifdef USE_HPS

        if(roc_id_db==38 && rol->pid==37) /*hps1/hps1gtp, temporary until resolved in hardware*/
		{
          printf("   TIPRIMARY: set busy for hps1gtp\n");
vmeBusLock();
          tiSetBusySource(TI_BUSY_SWA,0);
          tiAddRocSWA();
vmeBusUnlock();
		}
        else if(roc_id_db==40 && rol->pid==39) /*hps2/hps2gtp, temporary until resolved in hardware*/
		{
          printf("   TIPRIMARY: set busy for hps2gtp\n");
vmeBusLock();
          tiSetBusySource(TI_BUSY_SWA,0);
          tiAddRocSWA();
vmeBusUnlock();
		}
        else if(roc_id_db==52|| /*ignore all dpm's except 2, temporary until resolved in hardware*/
                roc_id_db==53||
                roc_id_db==54||
                roc_id_db==55||
                roc_id_db==56||
                roc_id_db==57||
                roc_id_db==66||
                roc_id_db==60||
                roc_id_db==61||
                roc_id_db==62||
                roc_id_db==63||
                roc_id_db==64||
                roc_id_db==65)
		{
          printf("   TIPRIMARY: do nothing for 'secondary' DPMs\n");
		}
        else if(roc_id_db==51) /*temporary until resolved in hardware*/
		{
          printf("   TIPRIMARY: add slave connected to fiber 4 (DPM0)\n");
vmeBusLock();
/* hps10: tiAddSlave(2);*/ /* temporary !!!! TI buster, so moved 4->2 */
          tiAddSlave(4);
vmeBusUnlock();
		}
        else if(roc_id_db==59) /*temporary until resolved in hardware*/
		{
          printf("   TIPRIMARY: add slave connected to fiber 5 (DPM8)\n");
vmeBusLock();
/* hps10: tiAddSlave(3);*/ /* temporary !!!! TI buster, so moved 5->3 */
          tiAddSlave(5);
vmeBusUnlock();
		}
        else

#endif


		{
          printf("   TIPRIMARY: looping over ports ..\n");
          for(port=1; port<=8; port++)
          {
            if(roc_id_db == roc_id_fiber[port])
		    {
              if(roc_id_db == rol->pid) /* never here ? */
		      {
                printf("      TIPRIMARY: rocid=%d - do nothing (cannot be myself's slave\n",roc_id_db);
		      }
		      else
		      {
                printf("      TIPRIMARY: added slave connected to fiber %d, rocid=%d\n",port,roc_id_db);
vmeBusLock();
                tiAddSlave(port);
vmeBusUnlock();
		      }
              break;
            }
          }
		}


      } /* 'inuse' != 'no' */

#endif /* #ifndef TI_SLAVE */



    }

    /* loop over all rows in configuration */
    /***************************************/



    mysql_free_result(result);
  }

  /* disconnect from database */
  dbDisconnect(dbsocket);


  /* if we have sync vtp */
  if(have_sync_vtp)
  {
    printf("TIPRIMARY: add SWA ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
vmeBusLock();
    tiSetBusySource(TI_BUSY_SWA,0);
    tiAddRocSWA();
vmeBusUnlock();
  }
  else
  {
    printf("TIPRIMARY: remove SWA +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
vmeBusLock();
    tiRemoveRocSWA();
vmeBusUnlock();
  }


  switch(code)
  {
    case TIR_SOURCE:
      /* Initialize VME Interrupt variables
      tirClearIntCount();
      */
      /* Connect User Trigger Routine */

/*
#ifdef VXWORKS	  
      tiIntConnect(TI_INT_VEC, isr, 0);
#endif
*/

      break;
	  /*
    case TS_SOURCE:
      if(ts)
      {
        ts->intVec = TIR_INT_VEC;
      }
      else
      {
        printf(" tiprimarytriglink: ERROR: ts uninitialized\n");
        return;
      }

#ifdef VXWORKSPPC
      if((intDisconnect(INUM_TO_IVEC(TI_INT_VEC)) !=0))
      {
        printf("Error disconnecting Interrupt\n");
      }
      intConnect(INUM_TO_IVEC(TI_INT_VEC),isr,0);
#endif

      break;
	  */

    default:
      printf(" tiprimarytriglink: ERROR: source type %d undefined\n",code);
      return;
  }

  printf("tiprimarytriglink: register int handler 0x%08x\n",isr);fflush(stdout);
}

static void 
tiprimarytenable(int val, unsigned int intMask)
{
  TIPRIMARYflag = 1;

  ttest_ready = 0;
  ttest_not_ready = 0;
  nackcalls = 0;

/*sergeytiIntEnable(val);*/

vmeBusLock();
  tiEnableTriggerSource();
vmeBusUnlock();

  /*
  tiIntEnable(0);
  */

}

static void 
tiprimarytdisable(int val, unsigned int intMask)
{
  int len;
  unsigned int tmpbuf[4000];

#ifndef TI_SLAVE
vmeBusLock();
  tiDisableTriggerSource(1);
  tiBlockStatus(0,1);
vmeBusUnlock();
#endif

vmeBusLock();
  tiStatus(1);
vmeBusUnlock();


  /* clear all buffers here !!! */
/*crashes EB
vmeBusLock();
  val = tiBReady();
  printf("reading remaining data from TI, val0=%d\n",val);
  while(val)
  {
    len = tiReadBlock(tmpbuf,900>>2,1);
    printf("reading remaining data from TI, len=%d\n",len);
    val = tiBReady();
    printf("reading remaining data from TI, val=%d\n",val);
  }
vmeBusUnlock();
*/


vmeBusLock();
  tiIntDisable();
vmeBusUnlock();
  /*
#ifdef VXWORKS
  tiIntDisconnect();
#endif
  */
  TIPRIMARYflag = 0;
}

static void 
tiprimarytack(int code, unsigned int intMask)
{
  /*if(code == TIR_SOURCE)*/
  {
    /*printf("TI_PRIMARY: call tiIntAck()\n");*/
vmeBusLock();
    tiIntAck();
	/*
    nackcalls ++;
    printf("TI_PRIMARY: call tiIntAck() %d times\n",nackcalls);
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
tiprimaryttype(unsigned int code)
{
  return(1); /* not used any more: event type reported in every event in TI data, and recodred into fragment header in ROL2 */
}

/* for polling mode only */
static int 
tiprimaryttest(unsigned int code)
{
  int val=0;

  if(code == TIR_SOURCE)
  {
vmeBusLock();
    val = tiBReady();
vmeBusUnlock();
    if(val)
    {
      ttest_ready ++;
vmeBusLock();
	  syncFlag = tiGetSyncEventFlag();/*tiGetSyncEventReceived();*/
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

#define TIPRIMARY_TEST  tiprimaryttest

#ifdef VXWORKS
#define TIPRIMARY_INIT       {TIPRIMARY_handlers=0;TIPRIMARY_isAsync=0; TIPRIMARYflag=0; tiprimarytinit(0);}
#else
#define TIPRIMARY_INIT(code) {TIPRIMARY_handlers=0;TIPRIMARY_isAsync=0; TIPRIMARYflag=0; tiprimarytinit(code);}
#endif

#define TIPRIMARY_ASYNC(code)  {TIPRIMARY_handlers = 1; TIPRIMARY_isAsync = 1; tiprimarytriglink(code,TIPRIMARY_int_handler);}

#define TIPRIMARY_SYNC(code)   {TIPRIMARY_handlers = 1; TIPRIMARY_isAsync = 0; tiprimarytriglink(code,TIPRIMARY_int_handler);}

#define TIPRIMARY_SETA(code) TIPRIMARYflag = code;

#define TIPRIMARY_SETS(code) TIPRIMARYflag = code;

#define TIPRIMARY_ENA(code,val) tiprimarytenable(code,val);

#define TIPRIMARY_DIS(code,val) tiprimarytdisable(code,val);

#define TIPRIMARY_ACK(code,val) tiprimarytack(code,val);

#define TIPRIMARY_CLRS(code) TIPRIMARYflag = 0;

#define TIPRIMARY_GETID(code) TIPRIMARY_handlers

#define TIPRIMARY_TTYPE tiprimaryttype

#define TIPRIMARY_START(val)	 {;}

#define TIPRIMARY_STOP(val)	 {tiprimarytdisable(val);}

#define TIPRIMARY_ENCODE(code) (code)


#endif

