
/* hps1

net_thread:  waiting=    280    sending=      0 microsec per event (nev=728)
setHeartError: 0 >sys 0, mask 21<
WARN: HeartBeat[0]: heartbeat=1426780246(1426780246) heartmask=21
UDP_cancel: cancel >inf:hps1 sys 0, mask 21<
UDP_cancel: cancel >inf:hps1 sys 0, mask 21<
wait: request in progress
*/

/* hps1test.c */
#if defined(Linux_vme)

/* HPS firmware:
Board Firmware Rev/ID = 0x02c1 : ADC Processing Rev = 0x0a03

TI init:
tcpClient adcecal1 'tiInit(0xa80000,3,0)'
coda_roc_gef -s clasprod -o "adcecal1 ROC" -i
*/

/* hps1test.c - first readout list for VXS crates with MVT and new TI */

#define USE_MVT
#undef DEBUG_MVT
#undef DEBUG

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "circbuf.h"

/* from fputil.h */
#define SYNC_FLAG 0x20000000

/* readout list name */
#define ROL_NAME__ "HPS1MVT"

/* polling mode if needed */
#define POLLING_MODE

/* main TI board */
#define TI_ADDR   (21<<19)  /* if 0 - default will be used, assuming slot 21*/


/* name used by loader */

#ifdef TI_MASTER
#define INIT_NAME hps1mvt_master__init
#define TI_READOUT TI_READOUT_EXT_POLL /* Poll for available data, front panel triggers */
#else
#ifdef TI_SLAVE
#define INIT_NAME hps1mvt_slave__init
#define TI_READOUT TI_READOUT_TS_POLL /* Poll for available data, triggers from master over fiber */
#else
#define INIT_NAME hps1mvt__init
#define TI_READOUT TI_READOUT_EXT_POLL /* Poll for available data, front panel triggers */
#endif
#endif

#include "rol.h"

void usrtrig(unsigned int EVTYPE, unsigned int EVSOURCE);
void usrtrig_done();

#include "TIPRIMARY_source.h"

/* user code */
#ifdef USE_MVT
#include "mvtLib.h"
static int nmvt;   /* Number of MVT SSPs found with mvtInit(..) */
static int mvt_to_cntr;
static struct timeval mvt_to;
static struct timeval mvt_max_wait;
static int mvt_max_to_iter = 0;
FILE *mvt_fptr_err_1 = (FILE *)NULL;
#endif

static char rcname[5];

//#define NBOARDS 22    /* maximum number of VME boards: we have 21 boards, but numbering starts from 1 */
//#define MY_MAX_EVENT_LENGTH 3000/*3200*/ /* max words per board */
static unsigned int *tdcbuf;

extern int rocMask;  /* defined in roc_component.c */
extern int tiMaster; /* defined in tiLib.c */


/*temporary here: for time profiling */
#define ABS(x)      ((x) < 0 ? -(x) : (x))

#define TIMERL_VAR \
  static hrtime_t startTim, stopTim, dTim; \
  static int nTim; \
  static hrtime_t Tim, rmsTim, minTim=10000000, maxTim, normTim=1

#define TIMERL_START \
{ \
  startTim = gethrtime(); \
}

#define TIMERL_STOP(whentoprint_macros,histid_macros) \
{ \
  stopTim = gethrtime(); \
  if(stopTim > startTim) \
  { \
    nTim ++; \
    dTim = stopTim - startTim; \
    /*if(histid_macros >= 0)   \
    { \
      uthfill(histi, histid_macros, (int)(dTim/normTim), 0, 1); \
    }*/														\
    Tim += dTim; \
    rmsTim += dTim*dTim; \
    minTim = minTim < dTim ? minTim : dTim; \
    maxTim = maxTim > dTim ? maxTim : dTim; \
    /*logMsg("good: %d %ud %ud -> %d\n",nTim,startTim,stopTim,Tim,5,6);*/ \
    if(nTim == whentoprint_macros) \
    { \
      logMsg("timer: %7llu microsec (min=%7llu max=%7llu rms**2=%7llu)\n", \
                Tim/nTim/normTim,minTim/normTim,maxTim/normTim, \
                ABS(rmsTim/nTim-Tim*Tim/nTim/nTim)/normTim/normTim,5,6); \
      nTim = Tim = 0; \
    } \
  } \
  else \
  { \
    /*logMsg("bad:  %d %ud %ud -> %d\n",nTim,startTim,stopTim,Tim,5,6);*/ \
  } \
}

void
tsleep(int n)
{
}

extern struct TI_A24RegStruct *TIp;
static int ti_slave_fiber_port = 1;

void
titest1()
{
  if(TIp==NULL) {printf("NULL\n");return;}
  printf("0x%08x(%d) 0x%08x(%d)\n",
		 vmeRead32(&TIp->fiberLatencyMeasurement),vmeRead32(&TIp->fiberLatencyMeasurement),
		 vmeRead32(&TIp->fiberAlignment),vmeRead32(&TIp->fiberAlignment));
}

#ifdef USE_MVT
static unsigned int mvtSlotMask = 0; /* bit=slot (starting from 0) */
static int MVT_SLOT;
#endif

static void
__download()
{
	int ii, i1, i2, i3;
	int id;
	int ret;

	// Log file variables
	char logfilename[128];
	char logfilename_backup[128];
	char log_file_perms[16];
	char log_message[256];
	struct stat log_stat;   
	// time variables
	time_t      cur_time;
	struct tm  *time_struct;
	char *ch, tmp[64];

	rol->poll = 1;
	printf("\n>>>>>>>>>>>>>>> ROCID=%d, CLASSID=%d <<<<<<<<<<<<<<<<\n",rol->pid,rol->classid);
	printf("CONFFILE >%s<\n\n",rol->confFile);
	printf("LAST COMPILED: %s %s\n", __DATE__, __TIME__);

	printf("USRSTRING >%s<\n\n",rol->usrString);

	/* if slave, get fiber port number from user string */
#ifdef TI_SLAVE
	ti_slave_fiber_port = 1; /* default */

	ch = strstr(rol->usrString,"fp=");
	if(ch != NULL)
	{
		strcpy(tmp,ch+strlen("fp="));
		printf("FP >>>>>>>>>>>>>>>>>>>>>%s<<<<<<<<<<<<<<<<<<<<<\n",tmp);
		ti_slave_fiber_port = atoi(tmp);
		printf("ti_slave_fiber_port =%d\n",ti_slave_fiber_port);
		tiSetFiberIn_preInit(ti_slave_fiber_port);
	}
#endif

	/**/
	CTRIGINIT;

	/* initialize OS windows and TI board */
	CDOINIT(TIPRIMARY,TIR_SOURCE);

	/************/
	/* init daq */
	daqInit();
	DAQ_READ_CONF_FILE;

  /*************************************/
  /* redefine TI settings if neseccary */
  tiSetUserSyncResetReceive(1);

#ifndef TI_SLAVE
  /* TS 1-6 create physics trigger, no sync event pin, no trigger 2 */
	vmeBusLock();
	{
		tiLoadTriggerTable(3);
		tiSetTriggerWindow(7);	// (7+1)*4ns trigger it coincidence time to form trigger type
	}
	vmeBusUnlock();
#endif

  /*********************************************************/
  /*********************************************************/
  /* set wide pulse */
//	vmeBusLock();
//	{
//		tiSetSyncDelayWidth(1,127,1);
//	}
//	vmeBusUnlock();

//	usrVmeDmaSetConfig(2,3,0); /*A32,MBLT,  80MB/s*/
//	usrVmeDmaSetConfig(2,4,0); /*A32,2eVME,160MB/s*/
//	usrVmeDmaSetConfig(2,5,0); /*A32,2eSST,160MB/s*/
	usrVmeDmaSetConfig(2,5,1); /*A32,2eSST,267MB/s*/
/*
	usrVmeDmaSetChannel(1);
	printf("===== Use DMA Channel %d\n\n\n",usrVmeDmaGetChannel());
*/
	tdcbuf = (unsigned int *)i2_from_rol1;

  /******************/
  /* USER code here */
#ifdef USE_MVT
	// Get current time
	cur_time = time(NULL);
	time_struct = localtime(&cur_time);
	if( mvt_fptr_err_1 == (FILE *)NULL )
	{
		sprintf(logfilename, "mvt_roc_%d_rol_1.log", rol->pid);
		sprintf(log_file_perms, "a+");
		if( stat( logfilename, &log_stat ) == 0 )
		{
			// file exists, check its size
			if( log_stat.st_size > 100000 )
			{
				// Too big, rename it
				// form backup log file name
				sprintf
				(
					logfilename_backup,
					"mvt_roc_%d_%02d%02d%02d_%02dH%02d.log",
					rol->pid,
					time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday,
					time_struct->tm_hour, time_struct->tm_min
				);
  				if( rename(logfilename, logfilename_backup) ) 
				{
					fprintf(stderr, "%s: rename failed from log file %s to %s with %d %s\n",
						__FUNCTION__, logfilename, logfilename_backup, errno, strerror( errno ));
				}
				sprintf(log_file_perms, "w");
			}
		}

		// Open file
		if( (mvt_fptr_err_1 = fopen(logfilename, log_file_perms)) == (FILE *)NULL )
		{
			fprintf(stderr, "%s: fopen failed to open log file %s in %s mode with %d %s\n",
				__FUNCTION__, logfilename, log_file_perms, errno, strerror( errno ));
		}
		mvtSetLogFilePointer( mvt_fptr_err_1 );
	}
	if( mvt_fptr_err_1 != (FILE *)NULL )
	{
		fprintf( mvt_fptr_err_1, "**************************************************\n" );
		fprintf( mvt_fptr_err_1, "%s at %02d%02d%02d %02dH%02d\n", __FUNCTION__,
			time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday, time_struct->tm_hour, time_struct->tm_min );
		fflush( mvt_fptr_err_1 );
	}

	printf("\nMVT: start\n\n");
	printf("\nMVT: !!!!!!!!!!!!!!!!!!!!! confFile=%s, rolnum=%d, rolpid=%d\n\n", rol->confFile,rol->runNumber, rol->pid );

	// Do Config here
	vmeBusLock();
		nmvt = mvtConfig("", rol->runNumber, rol->pid );
	vmeBusUnlock();

	if( ( nmvt <= 0 ) || (3 <= nmvt) )
	{
		sprintf( log_message, "%s: wrong number of BEUs %d in %s crate %d; must be in [1;3] range",
			__FUNCTION__, nmvt, mvtRocId2SysName( rol->pid ), rol->pid );
		fprintf(stderr, "%s\n", log_message );
		if( mvt_fptr_err_1 != (FILE *)NULL )
		{
			fprintf(mvt_fptr_err_1, "%s\n", log_message );
			fflush( mvt_fptr_err_1 );
		}
		UDP_user_request(MSGERR, "rol1", "!!!!!!!!!!!!!!!!!!!!!");
		UDP_user_request(MSGERR, "rol1", log_message);
		UDP_user_request(MSGERR, "rol1", "Try to download again");
		UDP_user_request(MSGERR, "rol1", "!!!!!!!!!!!!!!!!!!!!!");
	}
	else
	{
		sprintf( log_message, "%s: found number of BEUs %d in %s crate %d",
			__FUNCTION__, nmvt, mvtRocId2SysName( rol->pid ), rol->pid );
		fprintf( stdout, "%s\n", log_message );
		UDP_user_request(MSGINF, "rol1", "!!!!!!!!!!!!!!!!!!!!!");
		UDP_user_request(MSGINF, "rol1", log_message);
		UDP_user_request(MSGINF, "rol1", "!!!!!!!!!!!!!!!!!!!!!");
	}

	// Set SD active slots
	mvtSlotMask=0;
	for(id=0; id<nmvt; id++)
	{
		MVT_SLOT = mvtSlot(id);		// no VME access here 
		mvtSlotMask |= (1<<MVT_SLOT);
		printf("=======================> mvtSlotMask=0x%08x\n",mvtSlotMask);
	}

	vmeBusLock();
		// To be understood during the merging
		// For now sdInit(1) is done in mvtLib
//		ret = sdInit(1);   /* Initialize the SD library */
		ret = 1;
		if( ret >= 0 )
		{
			sdSetActiveVmeSlots(mvtSlotMask);
			sdStatus(1);
		}
		else
		{
			sprintf( log_message, "%s: sdInit(1) failed with %d in %s crate %d",
				__FUNCTION__, ret, mvtRocId2SysName( rol->pid ), rol->pid );
			fprintf( stdout, "%s\n", log_message );
			if( mvt_fptr_err_1 != (FILE *)NULL )
			{
				fprintf(mvt_fptr_err_1, "%s\n", log_message );
				fflush( mvt_fptr_err_1 );
			}
			UDP_user_request(MSGERR, "rol1", "!!!!!!!!!!!!!!!!!!!!!");
			UDP_user_request(MSGERR, "rol1", log_message);
			UDP_user_request(MSGERR, "rol1", "!!!!!!!!!!!!!!!!!!!!!");
		}
	vmeBusUnlock();

#endif // #ifdef USE_MVT

	sprintf(rcname,"RC%02d",rol->pid);
	printf("rcname >%4.4s<\n",rcname);
	
	{
//		int ii, i1, i2, i3;
		usrVmeDmaMemory(&i1, &i2, &i3);
		printf("%s: phyBase = 0x%08x usrBase=0x%08x size=%d %dk\n",__FUNCTION__, i1, i2, i3, i3/1024);
	}

	logMsg("INFO: User Download Executed\n",1,2,3,4,5,6);
}



static void
__prestart()
{
	int ret;
	int block_level;

	// Log file variables
	char logfilename[128];
	char logfilename_backup[128];
	char log_file_perms[16];
	struct stat log_stat;   
	char log_message[256];
	// time variables
	time_t      cur_time;
	struct tm  *time_struct;

	/* Clear some global variables etc for a clean start */
	*(rol->nevents) = 0;
	event_number = 0;

	tiEnableVXSSignals();

	CTRIGRSS(TIPRIMARY, TIR_SOURCE, usrtrig, usrtrig_done);

	printf(">>>>>>>>>> next_block_level = %d, block_level = %d, use next_block_level\n",next_block_level,block_level);
	block_level = next_block_level;

	/**************************************************************************/
	/* setting TI busy conditions, based on boards found in Download          */
	/* tiInit() does nothing for busy, tiConfig() sets fiber, we set the rest */
	/* NOTE: if ti is busy, it will not send trigger enable over fiber, since */
	/*       it is the same fiber and busy has higher priority                */
#ifndef TI_SLAVE
	vmeBusLock();
		tiSetBusySource(TI_BUSY_LOOPBACK,0);
		/*tiSetBusySource(TI_BUSY_FP,0);*/
	vmeBusUnlock();
#endif

	/*****************************************************************/
	/*****************************************************************/
	/* USER code here */
	/******************/

	vmeBusLock();
		tiIntDisable();
	vmeBusUnlock();

	/* master and standalone crates, NOT slave */
#ifndef TI_SLAVE
	sleep(1);

	vmeBusLock();
		tiSyncReset(1);
	vmeBusUnlock();

	sleep(1);

	vmeBusLock();
		tiSyncReset(1);
	vmeBusUnlock();

	sleep(1);

	vmeBusLock();
		ret = tiGetSyncResetRequest();
	vmeBusUnlock();
	if(ret)
	{
		printf("ERROR: syncrequest still ON after tiSyncReset(); trying again\n");
		sleep(1);
		vmeBusLock();
			tiSyncReset(1);
		vmeBusUnlock();
		sleep(1);
	}

	vmeBusLock();
		ret = tiGetSyncResetRequest();
	vmeBusUnlock();
	if(ret)
	{
		printf("ERROR: syncrequest still ON after tiSyncReset(); try 'tcpClient <rocname> tiSyncReset'\n");
	}
  	else
	{
		printf("INFO: syncrequest is OFF now\n");
	}

	printf("holdoff rule 1 set to %d\n",tiGetTriggerHoldoff(1));
	printf("holdoff rule 2 set to %d\n",tiGetTriggerHoldoff(2));
#endif // #ifndef TI_SLAVE

/*
	set block level in all boards where it is needed;
	it will overwrite any previous block level settings
*/



	tiSetBusySource(TI_BUSY_SWB, 0);

	// Get current time
	cur_time = time(NULL);
	time_struct = localtime(&cur_time);
#ifdef USE_MVT
	if( mvt_fptr_err_1 == (FILE *)NULL )
	{
		sprintf(logfilename, "mvt_roc_%d_rol_1.log", rol->pid);
		sprintf(log_file_perms, "a+");
		if( stat( logfilename, &log_stat ) == 0 )
		{
			// file exists, check its size
			if( log_stat.st_size > 100000 )
			{
				// Too big, rename it
				// form backup log file name
				sprintf
				(
					logfilename_backup,
					"mvt_roc_%d_%02d%02d%02d_%02dH%02d.log",
					rol->pid,
					time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday,
					time_struct->tm_hour, time_struct->tm_min
				);
  				if( rename(logfilename, logfilename_backup) ) 
				{
					fprintf(stderr, "%s: rename failed from log file %s to %s with %d\n",
						__FUNCTION__, logfilename, logfilename_backup, errno);
				 	perror("rename failed");
				}
				sprintf(log_file_perms, "w");
			}
		}
		// Open file
		if( (mvt_fptr_err_1 = fopen(logfilename, log_file_perms)) == (FILE *)NULL )
		{
			fprintf(stderr, "%s: fopen failed to open log file %s in %s mode with %d %s\n",
				__FUNCTION__, logfilename, log_file_perms, errno, strerror( errno ));
		}
		mvtSetLogFilePointer( mvt_fptr_err_1 );
	}
	if( mvt_fptr_err_1 != (FILE *)NULL )
	{
		fprintf( mvt_fptr_err_1,"**************************************************\n" );
		fprintf( mvt_fptr_err_1,"%s at %02d%02d%02d %02dH%02d\n", __FUNCTION__,
			time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday,
			time_struct->tm_hour, time_struct->tm_min );
		fprintf( mvt_fptr_err_1,"%s : Information below concerns run %d tiMaster=%d\n", __FUNCTION__, rol->runNumber, tiMaster );
		fflush(  mvt_fptr_err_1 );
	}

	if(nmvt>0)
	{
		vmeBusLock();
			ret = mvtPrestart();
		vmeBusUnlock();
		if(ret<=0)
		{
			sprintf( log_message, "%s: mvtPrestart failed with %d in %s crate %d",
				__FUNCTION__, ret, mvtRocId2SysName( rol->pid ), rol->pid );
			fprintf(stderr, "%s\n", log_message );
			if( mvt_fptr_err_1 != (FILE *)NULL )
			{
				fprintf(mvt_fptr_err_1, "%s\n", log_message );
				fflush( mvt_fptr_err_1 );
			}
			UDP_user_request(MSGERR, "rol1", "!!!!!!!!!!!!!!!!!!!!!");
			UDP_user_request(MSGERR, "rol1", log_message);
			UDP_user_request(MSGERR, "rol1", "!!!!!!!!!!!!!!!!!!!!!");
		}
		else
		{
			sprintf( log_message, "%s: found number of FEUs %d in %s crate %d with %d BEUs",
				__FUNCTION__, ret, mvtRocId2SysName( rol->pid ), rol->pid, nmvt );
			fprintf( stdout, "%s\n", log_message );
			UDP_user_request(MSGINF, "rol1", "!!!!!!!!!!!!!!!!!!!!!");
			UDP_user_request(MSGINF, "rol1", log_message);
			UDP_user_request(MSGINF, "rol1", "!!!!!!!!!!!!!!!!!!!!!");
		}
		block_level = tiGetCurrentBlockLevel();
		mvtSetCurrentBlockLevel( block_level );
		if( mvt_fptr_err_1 != (FILE *)NULL )
		{
			fprintf(mvt_fptr_err_1, "%s; mvt block level set to %d \n", __FUNCTION__, block_level);
			fflush( mvt_fptr_err_1 );
		}
/*
		vmeBusLock();
			mvtPrestart();
		vmeBusUnlock();
*/
		mvt_to_cntr = 0;
		mvt_to.tv_sec  = 0;
		mvt_to.tv_usec = 90000;
		mvt_max_wait.tv_sec  = 0;
		mvt_max_wait.tv_usec  = 0;
		mvt_max_to_iter = 0;
	}
#endif

//ADDED BY YM September 25 2015
// because the tisyncReset is distributed to BEUSSPs only after MvtPrestart is executed and sync commandes are enabled. 
#ifndef TI_SLAVE
	sleep(1);
	vmeBusLock();
		tiSyncReset(1);
	vmeBusUnlock();
	sleep(1);
	vmeBusLock();
		tiSyncReset(1);
	vmeBusUnlock();
	sleep(1);

	vmeBusLock();
		ret = tiGetSyncResetRequest();
	vmeBusUnlock();
	if(ret)
	{
		printf("ERROR: syncrequest still ON after tiSyncReset()\n");
	}
#endif

	vmeBusLock();
		tiStatus(1);
		sdStatus(1);
	vmeBusUnlock();
	printf("INFO: Prestart1 Executed\n");fflush(stdout);

#ifdef TI_MASTER
printf("INFO: Prestart1 as TI_MASRER\n");fflush(stdout);
#else
#ifdef TI_SLAVE
printf("INFO: Prestart1 as TI_SLAVE\n");fflush(stdout);
#else
printf("INFO: Prestart1 as TI_NONE\n");fflush(stdout);
#endif
#endif

	*(rol->nevents) = 0;
	rol->recNb = 0;
	return;
}  



static void
__end()
{
	int iwait=0;
	int blocksLeft=0;

	printf("\n\nINFO: End1 Reached\n");fflush(stdout);

	/* Before disconnecting... wait for blocks to be emptied */
	vmeBusLock();
		tiStatus(1);
		sdStatus(1);
	vmeBusUnlock();

	CDODISABLE(TIPRIMARY,TIR_SOURCE,0);

	/* Before disconnecting... wait for blocks to be emptied */
	vmeBusLock();
//		tiStatus(1);
//		sdStatus(1);
		blocksLeft = tiBReady();
	vmeBusUnlock();
	printf(">>>>>>>>>>>>>>>>>>>>>>> %d blocks left on the TI\n",blocksLeft);fflush(stdout);
	if(blocksLeft)
	{
		printf(">>>>>>>>>>>>>>>>>>>>>>> before while ... %d blocks left on the TI\n",blocksLeft);fflush(stdout);
		while(iwait < 10)
		{
			taskDelay(10);
			if(blocksLeft <= 0) break;
			vmeBusLock();
	  			blocksLeft = tiBReady();
      			printf(">>>>>>>>>>>>>>>>>>>>>>> inside while ... %d blocks left on the TI\n",blocksLeft);fflush(stdout);
			vmeBusUnlock();
			iwait++;
		}
		printf(">>>>>>>>>>>>>>>>>>>>>>> after while ... %d blocks left on the TI\n",blocksLeft);fflush(stdout);
	}

#ifdef USE_MVT
	if(nmvt>0)
	{
		vmeBusLock();
			mvtEnd();
		vmeBusUnlock();		
		if( mvt_fptr_err_1 != (FILE *)NULL )
		{
			mvtClrLogFilePointer();
			fprintf(mvt_fptr_err_1,"%s: mvt had to wait %d sec & %d us and iterate %d times\n", __FUNCTION__, mvt_max_wait.tv_sec, mvt_max_wait.tv_usec, mvt_max_to_iter );
			fflush( mvt_fptr_err_1 );
			fclose( mvt_fptr_err_1 );
			mvt_fptr_err_1 = (FILE *)NULL;
		}
	}
#endif


	vmeBusLock();
		tiStatus(1);
		sdStatus(1);
	vmeBusUnlock();

	printf("INFO: End1 Executed\n\n\n");fflush(stdout);
	return;
}


static void
__pause()
{
	CDODISABLE(TIPRIMARY,TIR_SOURCE,0);
	logMsg("INFO: Pause Executed\n",1,2,3,4,5,6);
} /*end pause */


static void
__go()
{
	logMsg("INFO: Entering Go 1\n",1,2,3,4,5,6);

#ifndef TI_SLAVE
	/* set sync event interval (in blocks) */
	vmeBusLock();
//		tiSetSyncEventInterval(10000/*block_level*/);
		tiSetSyncEventInterval(0/*block_level*/);		
	vmeBusUnlock();
#endif

#ifdef USE_MVT
	if(nmvt>0)
	{
	vmeBusLock();
		mvtGo();
	vmeBusUnlock();
	}
#endif

	/* always clear exceptions */
	jlabgefClearException(1);

	CDOENABLE(TIPRIMARY,TIR_SOURCE,0); /* bryan has (,1,1) ... */

	logMsg("INFO: Go 1 Executed\n",1,2,3,4,5,6);
}

//static int constant_set = 0;
void
usrtrig(unsigned int EVTYPE, unsigned int EVSOURCE)
{
	int *jw, ind, ind2, i, ii, jj, jjj, blen, len, rlen, itdcbuf, nbytes;
//	unsigned int *tdcbuf_save, *tdc;
//	unsigned int *dabufp1, *dabufp2;
	int njjloops, slot;
	int nwords;
	TIMERL_VAR;

#ifdef USE_MVT
	unsigned int datascan, mask;
//	unsigned short *dabufp16, *dabufp16_save;
	int id;
	int dCnt, idata;
	int mvtgbr;
	int mvt_to_iter;
#endif

	char *chptr, *chptr0;



	// Software timeout for the MVT readout0
	struct timeval mvt_t0;
	struct timeval mvt_t1;
	struct timeval mvt_dt;
/*
if( constant_set == 0 )
{
if(tiGetAckCount() > 0)
{
tiDisableRandomTrigger();
tiSoftTrig(1, 0xFFFF, 270, 1);
constant_set = 1;
#ifdef USE_MVT
		if( mvt_fptr_err_1 != (FILE *)NULL )
		{
			fprintf(mvt_fptr_err_1, "%s: Random trigger changed to constantd\n", __FUNCTION__);
			fflush( mvt_fptr_err_1);
		}
#endif
}
}
*/
	/*printf("EVTYPE=%d syncFlag=%d\n",EVTYPE,syncFlag);*/
	if(syncFlag)
		printf("EVTYPE=%d syncFlag=%d\n",EVTYPE,syncFlag);

	rol->dabufp = (int *) 0;

	CEOPEN(EVTYPE, BT_BANKS); /* reformatted on CODA_format.c !!! */

	if((syncFlag<0)||(syncFlag>1))         /* illegal */
	{
		printf("Illegal1: syncFlag=%d EVTYPE=%d\n",syncFlag,EVTYPE);
#ifdef USE_MVT
		if( mvt_fptr_err_1 != (FILE *)NULL )
		{
			fprintf(mvt_fptr_err_1, "Illegal1: syncFlag=%d EVTYPE=%d\n",syncFlag,EVTYPE);
			fflush( mvt_fptr_err_1);
		}
#endif
	}
	else if((syncFlag==0)&&(EVTYPE==0))    /* illegal */
	{
		printf("Illegal2: syncFlag=%d EVTYPE=%d\n",syncFlag,EVTYPE);
#ifdef USE_MVT
		if( mvt_fptr_err_1 != (FILE *)NULL )
		{
			fprintf(mvt_fptr_err_1, "Illegal2: syncFlag=%d EVTYPE=%d\n",syncFlag,EVTYPE);
			fflush( mvt_fptr_err_1);
		}
#endif
	}
	else if((syncFlag==1)&&(EVTYPE==0))    /* force_sync (scaler) events */
	{
#ifdef USE_MVT
		if( mvt_fptr_err_1 != (FILE *)NULL )
		{
			fprintf(mvt_fptr_err_1, "force_sync (scaler) events ?! we are geting here on End transition: syncFlag=1 EVTYPE=0 !!!\n");
			fflush( mvt_fptr_err_1);
		}
#endif
//		;
/*
!!! 	we are geting here on End transition: syncFlag=1 EVTYPE=0 !!!
*/
	}
	else if((syncFlag==0)&&(EVTYPE==15)) /* helicity strob events */
	{
#ifdef USE_MVT
		if( mvt_fptr_err_1 != (FILE *)NULL )
		{
			fprintf(mvt_fptr_err_1, "helicity strob events ?!\n");
			fflush( mvt_fptr_err_1);
		}
#endif
//		;
	}
	else           /* physics and physics_sync events */
	{

		/* for EVIO format, will dump raw data */
//		tdcbuf_save = tdcbuf;

		/*************/
		/* TI stuff */

		/* Set high, the first output port 
		tiSetOutputPort(1,0,0,0);
		*/

		/* Grab the data from the TI */
		vmeBusLock();
			len = tiReadBlock(tdcbuf,900>>2,1);
		vmeBusUnlock();
		if(len<=0)
		{
			printf("ERROR in tiReadBlock : No data or error, len = %d\n",len);
#ifdef USE_MVT
			if( mvt_fptr_err_1 != (FILE *)NULL )
			{
				fprintf(mvt_fptr_err_1, "ERROR in tiReadBlock : No data or error, len = %d\n",len);
				fflush( mvt_fptr_err_1);
			}
#endif
			sleep(1);
		}
 		else
		{
/*
			if( mvt_fptr_err_1 != (FILE *)NULL )
			{
				fprintf(mvt_fptr_err_1, "%s: TI block len=%d\n",__FUNCTION__, len);
				fflush( mvt_fptr_err_1);
			}
*/
			/*
				printf("ti: len=%d\n",len);
				for(jj=0; jj<len; jj++) printf("ti[%2d] 0x%08x\n",jj,LSWAP(tdcbuf[jj]));
			*/
			BANKOPEN(0xe10A,1,rol->pid);
			for(jj=0; jj<len; jj++)
				*rol->dabufp++ = tdcbuf[jj];
			BANKCLOSE;
		}

		/* Turn off all output ports 
		tiSetOutputPort(0,0,0,0);
		*/
		/* TI stuff */
		/*************/
#ifdef DEBUG
		printf("fadc1: start fadc processing\n");fflush(stdout);
#endif

		TIMERL_START;							//??????????????????????????????????

#ifdef USE_MVT
		if(nmvt>0)
		{
			gettimeofday(&mvt_t0, 0);
			mvt_to_iter = 0;
			do
			{
				vmeBusLock();
					mvtgbr = mvtGBReady(rol->pid);
				vmeBusUnlock();
				if( mvtgbr == mvtSlotMask ) 
				{
					break;
				}
				mvt_to_iter++;
				gettimeofday(&mvt_t1, 0);
				timersub(&mvt_t1,&mvt_t0,&mvt_dt);
			} while( timercmp(&mvt_dt,&mvt_to,<) );
			if( mvtgbr != mvtSlotMask )
			{
				mvt_to_cntr++;
				printf("MVT NOT READY: gbready=0x%08x, expect 0x%08x, to_cntr=%d EVENT_NUMBER=%d\n", mvtgbr, mvtSlotMask, mvt_to_cntr, EVENT_NUMBER);
				if( mvt_fptr_err_1 != (FILE *)NULL )
				{
					fprintf(mvt_fptr_err_1, "MVT NOT READY: gbready=0x%08x, expect 0x%08x, to_cntr=%d EVENT_NUMBER=%d\n", mvtgbr, mvtSlotMask, mvt_to_cntr, EVENT_NUMBER);
					if( mvt_to_cntr <= 5 )
						mvtStatusDump(0, mvt_fptr_err_1);
					if( mvt_to_cntr = 5 )
						fprintf(mvt_fptr_err_1, "Too many timeouts; mvtStatusDump will not be called any more\n");
					fflush( mvt_fptr_err_1 );
				}
			}
			else
			{
				if( mvt_to_iter )
				{
					if( mvt_max_to_iter < mvt_to_iter )
						mvt_max_to_iter = mvt_to_iter;
					if( timercmp(&mvt_max_wait, &mvt_dt,<) )
					{
						mvt_max_wait.tv_sec = mvt_dt.tv_sec;
						mvt_max_wait.tv_usec = mvt_dt.tv_usec;
					}
				}

				vmeBusLock();
					len = mvtReadBlock(rol->pid,tdcbuf,1000000,1);
					//printf("ROL1: mvt len = %d words\n",len);
				vmeBusUnlock();
				if(len>0)
				{
/*
		                        if( mvt_fptr_err_1 != (FILE *)NULL )
               			        {
                     		          	fprintf(mvt_fptr_err_1, "%s: MVT block len=%d\n",__FUNCTION__, len);
                        		        fflush( mvt_fptr_err_1);
                      			 }
*/
					BANKOPEN(0xe118,1,rol->pid);
#ifdef DEBUG_MVT
					if( mvt_fptr_err_1 != (FILE *)NULL )
					{
						fprintf(mvt_fptr_err_1,"%s : mvt data len=%d to be copied to rol->dabufp=0x%08x first data=0x%08x\n",
							__FUNCTION__, len, rol->dabufp, tdcbuf[00] );
						fflush( mvt_fptr_err_1 );
					}
#endif
					for(jj=0; jj<len; jj++) *rol->dabufp++ = tdcbuf[jj];
					BANKCLOSE;
				}
			}
   		} // if(nmvt>0)
#endif // #ifdef USE_MVT

		TIMERL_STOP(100000/block_level,1000+rol->pid);		//??????????????????????????????????

		/* read boards configurations */
		if(syncFlag==1 || EVENT_NUMBER==1)
		{
			BANKOPEN(0xe10E,3,rol->pid);
			chptr = chptr0 =(char *)rol->dabufp;
			nbytes = 0;

			/* add one 'return' to make evio2xml output nicer */
			*chptr++ = '\n';
			nbytes ++;

			vmeBusLock();
				len = tiUploadAll(chptr, 10000);
			vmeBusUnlock();
			/*printf("\nTI len=%d\n",len);
			printf(">%s<\n",chptr);*/
			chptr += len;
			nbytes += len;

#ifdef USE_MVT
			if(nmvt>0)
			{
	//// For the moment no VME access
////vmeBusLock(); 
				len = mvtUploadAll(chptr, 256*1024);
////vmeBusUnlock();
				chptr += len;
				nbytes += len;
			}
#endif

			/* 'nbytes' does not includes end_of_string ! */
			chptr[0] = '\n';
			chptr[1] = '\n';
			chptr[2] = '\n';
			chptr[3] = '\n';
			nbytes = (((nbytes+1)+3)/4)*4;
			chptr0[nbytes-1] = '\0';

			nwords = nbytes/4;
			rol->dabufp += nwords;

			BANKCLOSE;
		} // if(syncFlag==1 || EVENT_NUMBER==1)

#ifndef TI_SLAVE
		/* print livetite */
		if(syncFlag==1)
		{
			int livetime, live_percent;
			vmeBusLock();
				tiLatchTimers();
				livetime = tiLive(0);
			vmeBusUnlock();
			live_percent = livetime/10;
			printf("============= Livetime=%3d percent\n",live_percent);
		}
#endif
	}
	/* close event */
	CECLOSE;
	return;
}

void
usrtrig_done()
{
	return;
}

void
__done()
{
	/* from parser */
	poolEmpty = 0; /* global Done, Buffers have been freed */

	/* Acknowledge tir register */
	CDOACK(TIPRIMARY,TIR_SOURCE,0);

	return;
}

static void
__status()
{
	return;
}  

void
fadc1_dummy()
{
	return;
}

#endif
