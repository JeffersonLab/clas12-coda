
#if defined(Linux_vme) || defined(Linux_armv7l)

#include <sys/mman.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "CrateMsgTypes.h"

#include "ipc.h"

#ifdef Linux_vme
#include "jvme.h"
#include "dsc2Lib.h"
#include "fadcLib.h"
#include "vscmLib.h"
#include "sspLib.h"
#endif

#ifdef Linux_armv7l
#include "vtpLib.h"
#endif


#define SCALER_THREAD


#ifdef SCALER_THREAD



/****************************************************************************
 scalers stuff - allows organize scalers readout by this server with defined
interval and report it to tcp clients such as CODA or EPICS

NOTES ON ADDING NEW FUNCTIONS:

Files to be modified:

main/DiagGuiServer.c
code.s/CrateMsgTypes.h
code.s/CrateMsgServer.c

code.s/cratemsgclient.h
code.s/cratemsgclient.cc
main/cratemsgclienttest.cc

What have to be done:

1. Add low-level function to DiagGuiServer.c, it will access actual hardware;
   3 places have to be modified: for example if function () is implemented,
   corresponding function Vme_GetCrateMap() have to be implemented as well,
   and pointer to that function must be in gCrateServerCBFcn struct:
      gCrateServerCBFcn.GetCrateMap = Vme_GetCrateMap

2. Add corresponding to header file CrateMsgTypes.h, for example:

   #define SCALER_SERVER_GET_CRATE_MAP     0x101

   typedef struct
   {
     int cnt;
   } Cmd_GetCrateMap;

   typedef struct
   {
     int cnt;
     int nslots;
     unsigned int vals[1];
   } Cmd_GetCrateMap_Rsp;

   typedef struct
   {
      .....
      Cmd_GetCrateMap	        m_Cmd_GetCrateMap;
	  Cmd_GetCrateMap_Rsp	    m_Cmd_GetCrateMap_Rsp;
      ...
   } CrateMsgStruct;

   typedef struct
   {
     int (*GetCrateMap)(Cmd_GetCrateMap *pCmd, Cmd_GetCrateMap_Rsp *pCmd_Rsp);
   } ServerCBFunctions;

3. Add 2 pieces into CrateMsgServer.c (function and call for it):

   int
   CrateMsg_GetCrateMap(CrateMsgStruct *msg, int swap)
   {
     if(gServerCBFunctions.GetCrateMap)
     {
	   return (*gServerCBFunctions.GetCrateMap)(&msg->msg.m_Cmd_GetCrateMap, &msg->msg.m_Cmd_GetCrateMap_Rsp);
     }
     return(0);
   }

   ...
   void *
   ConnectionThread(void *parm)
   {
     ...
	 else if(pParm->msg.type == SCALER_SERVER_GET_CRATE_MAP)
	 {
	   result = CrateMsg_GetCrateMap(&pParm->msg, swap);
	 }
     ...
   }

4. Add corresponding pieces into 3 'cratemsgclient' files to provide an examples for users:

  

*****************************************************************************/

#ifdef VXWORKS

#define SCALER_LOCK   semTake(vmescalers_lock, WAIT_FOREVER)
#define SCALER_UNLOCK semGive(vmescalers_lock)

#else

#define SCALER_LOCK   pthread_mutex_lock(&vmescalers_lock)
#define SCALER_UNLOCK pthread_mutex_unlock(&vmescalers_lock)

#endif

#define DIST_ADDR  0xEA00	  /*  base address of FADC signal distribution board  (A16)  */

#define MAXBOARDS  21   /* max number od boards per crate */
#define MAXWORDS  /*256*//*4096*/8192   /* max number of scaler words per board */

#define MIN(a,b) ( (a) < (b) ? (a) : (b) )

static int vmeScalersReadInterval = 1;
#ifdef VXWORKS
static SEM_ID vmescalers_lock;
#else
static pthread_mutex_t vmescalers_lock;
#endif


static int nfadc, ndsc2_tcp, nvscm, nssp, rflag, rmode;
static unsigned int  vmescalersmap[MAXBOARDS+1];  /* crate map */

static unsigned int  vmescalerslen[MAXBOARDS];  /*scalers space (the number of words) */
static unsigned int *vmescalers[MAXBOARDS];     /*scalers memory space address*/

static unsigned int  vmedatalen[MAXBOARDS];  /*data space (the number of words) */
static unsigned int *vmedata[MAXBOARDS];     /*data memory space address*/

static unsigned int i2_from_rol1;

static unsigned int *tdcbuf;
static unsigned int adcbuf[400];
static unsigned int vscmbuf[MAXWORDS];
static unsigned int sspbuf[MAXWORDS];

#ifdef Linux_armv7l
#define MAXVTPWORDS 4096
static unsigned int vtpbuf[MAXVTPWORDS];
#endif

static int init_boards;


/* parameter 'time' in seconds */
static int
vmeSetScalersReadInterval(int time)
{
  if(time < 0)       vmeScalersReadInterval = 0; /* 0 means no reads */
  else if(time > 10) vmeScalersReadInterval = 10;
  else               vmeScalersReadInterval = time;

  printf("vmeSetScalersReadInterval: vmeScalersReadInterval set to %d\n",vmeScalersReadInterval);

  return(vmeScalersReadInterval);
}

static int
vmeGetScalersReadInterval()
{
  return(vmeScalersReadInterval);
}




static int ScalersReadoutStart();
static int ScalersReadoutStop();


#ifdef Linux_vme /* VME section */

static int
vmeScalersRead()
{
  int itype, id, ii, nw, nw_len, slot, fiber;
  unsigned int chmask = 0xFFFF;

  SCALER_LOCK;

  for(itype=0; itype<SCALER_TYPE_MAX; itype++)
  {

    if(itype == SCALER_TYPE_DSC2)    /* dsc2 scalers */
	{
      for(id=0; id<ndsc2_tcp; id++)
      {
        slot = dsc2Slot_tcp(id);
vmeBusLock();
        nw = dsc2ReadScalers(slot, tdcbuf, MAXWORDS, rflag, 1/*rmode*/);
/*vmeBusUnlock(); move below trying to debug problem 'in FADC data: trailer #words 58 != actual #words 54'*/
        vmescalerslen[slot] = nw;
        for(ii=0; ii<nw; ii++) vmescalers[slot][ii] = tdcbuf[ii];
vmeBusUnlock();
      }
	}
    else if(itype == SCALER_TYPE_FADC250)    /* fadc250 scalers */
	{
      for(id=0; id<nfadc; id++)
      {
        slot = faSlot(id);
vmeBusLock();
        nw = faReadScalers(slot, adcbuf, chmask, 0x3/*rflag*/);
vmeBusUnlock();
        vmescalerslen[slot] = nw;
        for(ii=0; ii<nw; ii++) vmescalers[slot][ii] = adcbuf[ii];
      }
    }
    else if(itype == SCALER_TYPE_VSCM)    /* vscm scalers */
	{
      for(id=0; id<nvscm; id++)
      {
        slot = vscmSlot(id);
vmeBusLock();
        nw = vscmReadScalers(slot, vscmbuf, MAXWORDS, 0xff/*rflag*/, 0);
vmeBusUnlock();
        vmescalerslen[slot] = nw;
        for(ii=0; ii<nw; ii++) vmescalers[slot][ii] = vscmbuf[ii];
		/*printf("vmeScalersRead: nw=%d, vmescalers[slot][nw-2]=%d, vmescalers[slot][nw-1]=%d\n",nw,vmescalers[slot][nw-2],vmescalers[slot][nw-1]);*/
      }
    }
    else if(itype == SCALER_TYPE_SSP)    /* ssp scalers */
	{
      for(id=0; id<nssp; id++)
      {
        unsigned int fibermask;
        slot = sspSlot(id);
		/*printf("slot=%d\n",slot);*/

        if(sspGetFirmwareType(slot) == SSP_CFG_SSPTYPE_HALLBRICH)
		{
vmeBusLock();
		sspRich_ScanFibers_NoInit(slot);
vmeBusUnlock();

          sspRich_GetConnectedFibers(slot, &fibermask);
          printf("fibermask=0x%08x\n",fibermask);
          /* loop over fibers */
          nw = 0;
          for(fiber=0; fiber<32; fiber++)
	      {
            unsigned int ref;
		    unsigned int maroc[RICH_CHAN_NUM];

		    if(fibermask & (1<<fiber))
		    {
/*printf("befor lock\n");fflush(stdout);*/
vmeBusLock();
/*printf("in lock\n");fflush(stdout);*/
              sspRich_ReadScalers(slot, fiber, &ref, maroc);
vmeBusUnlock();
/*printf("after lock\n");fflush(stdout);*/
			  /*printf("got fiber %d, nw=%d (%d)\n",fiber,nw,nw/(RICH_CHAN_NUM+3));*/
              nw_len = nw;
              sspbuf[nw++] = 0; /*reserve space for length*/
              sspbuf[nw++] = fiber;
              sspbuf[nw++] = ref;
			  
              memcpy(&sspbuf[nw], maroc, RICH_CHAN_NUM*sizeof(int)); nw += RICH_CHAN_NUM;
              sspbuf[nw_len] = nw - nw_len; /*inclusive length in words*/
		    }
	      }
		}

        vmescalerslen[slot] = nw;
        for(ii=0; ii<nw; ii++) vmescalers[slot][ii] = sspbuf[ii];
		/*printf("vmeScalersRead: nw=%d, vmescalers[slot][nw-2]=%d, vmescalers[slot][nw-1]=%d\n",nw,vmescalers[slot][nw-2],vmescalers[slot][nw-1]);*/
      }
    }
  }

  SCALER_UNLOCK;

  return(0);
}









static int
vmeDataRead()
{
  int itype, id, ii, nw, nw_len, slot, fiber;

  SCALER_LOCK;

  for(itype=0; itype<SCALER_TYPE_MAX; itype++)
  {
    if(itype == SCALER_TYPE_SSP)    /* ssp board */
	{
      for(id=0; id<nssp; id++)
      {
        unsigned int fibermask;
        slot = sspSlot(id);
        if(sspGetFirmwareType(slot) == SSP_CFG_SSPTYPE_HALLBRICH)
		{
          sspRich_GetConnectedFibers(slot, &fibermask);
		  /*printf("data fibermask=0x%08x\n",fibermask);*/
          /* loop over fibers */
          nw = 0;
          for(fiber=0; fiber<32; fiber++)
	      {
            sspRich_Monitor mon;
		    if(fibermask & (1<<fiber))
		    {
vmeBusLock();
              sspRich_ReadMonitor(slot, fiber, &mon);
vmeBusUnlock();
              /*printf("fiber=%d nw=%d (%d)\n",fiber,nw,nw/11);*/
              nw_len = nw;
              sspbuf[nw++] = 0; /*reserve space for length*/
              sspbuf[nw++] = fiber;
              sspbuf[nw++] = mon.temps.fpga;
              sspbuf[nw++] = mon.temps.regulator[0];
              sspbuf[nw++] = mon.temps.regulator[1];
              sspbuf[nw++] = mon.voltages.pcb_5v;
              sspbuf[nw++] = mon.voltages.pcb_3_3v;
              sspbuf[nw++] = mon.voltages.fpga_vccint_1v;
              sspbuf[nw++] = mon.voltages.fpga_vccaux_1_8v;
              sspbuf[nw++] = mon.voltages.fpga_mgt_1v;
              sspbuf[nw++] = mon.voltages.fpga_mgt_1_2v;
              sspbuf[nw_len] = nw - nw_len; /*inclusive length in words*/

		    }
	      }
		}
        vmedatalen[slot] = nw;
        for(ii=0; ii<nw; ii++) vmedata[slot][ii] = sspbuf[ii];
      }
    }
  }

  SCALER_UNLOCK;

  return(0);
}
























/* return in the buf the threshold values of the type defined by 'type'
(low or high) of each slot's channel; returns -1 if slot is empty
or unknown */
int
vmeGetBoardParams(int slot, int partype, int *buf, int *len)
{
  int chan;

  *len = 0;

  if(vmescalersmap[slot] == SCALER_TYPE_DSC2)
  {
    if(partype==SCALER_PARTYPE_THRESHOLD)
	{
vmeBusLock();
      *len = 16;
      for(chan=0; chan<16; chan++) buf[chan] = dsc2GetThreshold(slot, chan, 1);
vmeBusUnlock();
	}
    else if(partype==SCALER_PARTYPE_THRESHOLD2)
	{
vmeBusLock();
      *len = 16;
      for(chan=0; chan<16; chan++) buf[chan] = dsc2GetThreshold(slot, chan, 2);
vmeBusUnlock();
    }
    else if(partype==SCALER_PARTYPE_NCHANNELS)
	{
      *len = 1;
      buf[0] = 16;
    }
  }
  else if(vmescalersmap[slot] == SCALER_TYPE_FADC250)
  {
    if(partype==SCALER_PARTYPE_THRESHOLD)
	{
vmeBusLock();
      *len = 16;
      for(chan=0; chan<16; chan++) buf[chan] = faGetChThreshold(slot, chan);
vmeBusUnlock();
	}
    else if(partype==SCALER_PARTYPE_NCHANNELS)
	{
      *len = 1;
      buf[0] = 16;
    }
  }

  if(*len == 0) return(-1);
  return(0);
}


/* same for channel */
int
vmeGetChannelParams(int slot, int channel, int partype, int *buf, int *len)
{

  *len = 0;

  if(vmescalersmap[slot] == SCALER_TYPE_DSC2)
  {
    if(partype==SCALER_PARTYPE_THRESHOLD)
	{
vmeBusLock();
      *len = 1;
      buf[0] = dsc2GetThreshold(slot, channel, 1);
vmeBusUnlock();
	}
    else if(partype==SCALER_PARTYPE_THRESHOLD2)
	{
vmeBusLock();
      *len = 1;
      buf[0] = dsc2GetThreshold(slot, channel, 2);
vmeBusUnlock();
    }
  }
  else if(vmescalersmap[slot] == SCALER_TYPE_FADC250)
  {
    if(partype==SCALER_PARTYPE_THRESHOLD)
	{
vmeBusLock();
      *len = 1;
      buf[0] = faGetChThreshold(slot, channel);
vmeBusUnlock();
	}
  }
  /*
  printf("--> vmeGetChannelParams: len=%d buf[0]=%d\n",*len,buf[0]);
  */
  if(*len == 0) return(-1);
  return(0);
}


/* set threshold values of each type(low and high)
of each slot's channel. The mask defines which channels should be set */
int
vmeSetChannelParams(int slot, int channel, int partype, int *buf, int len)
{
  int err = 1;

  if(vmescalersmap[slot] == SCALER_TYPE_DSC2)
  {
    if(partype==SCALER_PARTYPE_THRESHOLD)
	{
vmeBusLock();
      dsc2SetThreshold(slot, channel, buf[0], 1);
vmeBusUnlock();
      err = 0;
	}
    else if(partype==SCALER_PARTYPE_THRESHOLD2)
	{
vmeBusLock();
      dsc2SetThreshold(slot, channel, buf[0], 2);
vmeBusUnlock();
      err = 0;
    }
  }
  else if(vmescalersmap[slot] == SCALER_TYPE_FADC250)
  {
    if(partype==SCALER_PARTYPE_THRESHOLD)
	{
vmeBusLock();
      faSetChThreshold(slot, channel, buf[0]);
vmeBusUnlock();
      err = 0;
	}
  }

  if(err) return(-1);
  return(0);
}


/* function for forcing synch. event every 10 seconds; use command
      taskSpawn "FORCE_SYNC",119,0,6000,ts2syncTask
   in the boot script */
static void
vmeReadTask()
{
  int id, iFlag;
  int ii, jj, slot;
  unsigned int maxA32Address;
  unsigned int fadcA32Address = 0x09000000;

#ifdef VXWORKS
  extern unsigned long sysClkRateGet();
#endif

  for(ii=0; ii<MAXBOARDS; ii++)
  {
    if( (vmescalers[ii] = (unsigned int *) calloc(MAXWORDS,4)) <= 0)
    {
      printf("ERROR in ScalerThread: cannot allocate memory for vmescalers[]\n");
      return;
    }
    printf("ScalerThread: Allocated vmescalers[]: 0x%08x words for slot %d at address 0x%08x\n",MAXWORDS,ii,vmescalers[ii]);

    if( (vmedata[ii] = (unsigned int *) calloc(MAXWORDS,4)) <= 0)
    {
      printf("ERROR in ScalerThread: cannot allocate memory for vmedata[]\n");
      return;
    }
    printf("ScalerThread: Allocated vmedata[]: 0x%08x words for slot %d at address 0x%08x\n",MAXWORDS,ii,vmedata[ii]);
  }

  for(ii=0; ii<(MAXBOARDS+1); ii++)
  {
    vmescalersmap[ii] = -1;
  }


  /* dma */
  usrVmeDmaSetConfig(2,3,0); /*A32,MBLT*/
  printf("!!!!!!!!! set DMA as A32,MBLT\n");

  tdcbuf = (unsigned int *)i2_from_rol1;



  /*************/
  /* DCS2 INIT */

  iFlag = 0;
  if(init_boards==0)
  {
    /* skip initialization* */
    iFlag |= (1<<16);
  }
  /*iFlag |= (1<<19);*/ /* ignore slot numbers, enumerate boards from 0 */

  dsc2Init(0x100000,0x80000,16,iFlag);
  dsc2Config("");
maxA32Address = dsc2GetA32MaxAddress();
fadcA32Address = maxA32Address + FA_MAX_A32_MEM;
  ndsc2_tcp = dsc2GetNdsc_tcp();
  printf("vmeReadTask: found %d dsc2 boards\n",ndsc2_tcp);

  /* fill map array with DSC2's found */
  for(ii=0; ii<ndsc2_tcp; ii++) if( (slot=dsc2Slot_tcp(ii)) > 0) vmescalersmap[slot] = SCALER_TYPE_DSC2;


  /*************/
  /* FADC INIT */

  rflag = 0xFF; /* latch and read everything */
  rmode = 0; /* not-dma readout */

  iFlag = 0;  /* base address */
  iFlag = (DIST_ADDR)<<10;
  iFlag |= (1<<0);    /* Sync Source: VXS */
  iFlag |= (1<<2);    /* Trigger Source: VXS */
  /*iFlag |= (1<<5);*/    /* Clock Source: VXS */
  iFlag |= (0<<5);  /* Internal Clock Source */

  if(init_boards==0)
  {
    /* skip initialization* */
    iFlag |= (1<<16);
  }

faSetA32BaseAddress(fadcA32Address);
  faInit((unsigned int)(3<<19),(1<<19),18,iFlag);
  nfadc = faGetNfadc();

  /* fill map array with FADC's found */
  for(ii=0; ii<nfadc; ii++) if( (slot=faSlot(ii)) > 0) vmescalersmap[slot] = SCALER_TYPE_FADC250;



  /*************/
  /* VSCM INIT */

  nvscm = vscmInit(0x100000,0x80000,20,0);
  vscmConfig ("");

  /* fill map array with FADC's found */
  for(ii=0; ii<nvscm; ii++) if( (slot=vscmSlot(ii)) > 0) vmescalersmap[slot] = SCALER_TYPE_VSCM;



  /************/
  /* SSP INIT */

  printf("Start SSP initialization\n");

  iFlag  = SSP_INIT_MODE_DISABLED; /* Disabled, initially */
  iFlag |= SSP_INIT_SKIP_FIRMWARE_CHECK;
  iFlag |= SSP_INIT_MODE_VXS;
  nssp=0;

  nssp = sspInit(0,0,0,iFlag);
  sspConfig ("");

  /* fill map array with FADC's found */
  for(ii=0; ii<nssp; ii++) if( (slot=sspSlot(ii)) > 0) vmescalersmap[slot] = SCALER_TYPE_SSP;

  printf("Finished SSP initialization, nssp=%d\n",nssp);



  /* always clean up init flag ! */
  init_boards = 0;



  printf("Starting readout loop, vmeScalersReadInterval=%d\n",vmeScalersReadInterval);

  while(1)
  {
    if(vmeScalersReadInterval==0) /* if interval==0, wait 1 sec and check again */
    {
#ifdef VXWORKS
      taskDelay(sysClkRateGet());
#else
      sleep(1);
#endif
    }
    else
    {
#ifdef VXWORKS
      taskDelay(sysClkRateGet()*vmeScalersReadInterval);
#else
      sleep(vmeScalersReadInterval);
#endif


#if 1
/* send trigger scalers from SSPs */
if(nssp>0) sspGSendScalers();
/* send trigger scalers from SSPs */
#endif


	  //printf("vmeReadTask: reading scalers ...\n");
      vmeScalersRead();
	  //printf("vmeReadTask: ... scalers read\n");

	  //printf("vmeReadTask: reading data ...\n");
      vmeDataRead();
	  //printf("vmeReadTask: ... data read\n");
    }
  }

  ScalersReadoutStop();

  return;
}


#endif /* VME section */







#ifdef Linux_armv7l /* VTP section */

static void
vtpReadTask()
{
  int id, iFlag;
  int ii, jj, nw, slot;
  unsigned int max_scalers;

  for(ii=0; ii<MAXBOARDS; ii++)
  {
    if( (vmescalers[ii] = (unsigned int *) calloc(MAXWORDS,4)) <= 0)
    {
      printf("ERROR in ScalerThread: cannot allocate memory\n");
      return;
    }
    printf("ScalerThread: Allocated 0x%08x words for slot %d at address 0x%08x\n",MAXWORDS,ii,vmescalers[ii]);
  }

  for(ii=0; ii<(MAXBOARDS+1); ii++)
  {
    vmescalersmap[ii] = -1;
  }


  /*************/
  /* VTP INIT */

  /*
  vtpInit();
  vtpConfig ("");
  */

  /* fill map array with VTP found */
  vmescalersmap[slot] = SCALER_TYPE_VTP;


  /* always clean up init flag ! */
  init_boards = 0;

  while(1)
  {
    if(vmeScalersReadInterval==0) /* if interval==0, wait 1 sec and check again */
    {
#ifdef VXWORKS
      taskDelay(sysClkRateGet());
#else
      sleep(1);
#endif
    }
    else
    {
#ifdef VXWORKS
      taskDelay(sysClkRateGet()*vmeScalersReadInterval);
#else
      sleep(vmeScalersReadInterval);
#endif
printf("vtpSendScalers()\n");      
      SCALER_LOCK;
	  /* actual scales readout */
      slot = 11; /* VTP always in slot 11 */

/*nw = vtpReadScalers(vtpbuf, MAXVTPWORDS);*/
nw = 0;
vtpSendScalers();

      vmescalerslen[slot] = nw;
      for(ii=0; ii<nw; ii++) vmescalers[slot][ii] = vtpbuf[ii];

      SCALER_UNLOCK;

    }
  }

  ScalersReadoutStop();

  return;
}

#endif /* VTP section */











/* return a number of slots (len) in the crate
and the array of size len that is an array of board types.
(type -1 : slot is empty, type 0 : is Discr2, type 1 : is FADC250)
 */
int
GetCrateMap(int *buf, int *len)
{
  int slot, ii;

  *len = MAXBOARDS + 1;
  for(slot=0; slot<(*len); slot++) buf[slot] = vmescalersmap[slot];
  /*
  printf("\nGetCrateMap: *len=%d\n",*len);fflush(stdout);
  for(ii=0; ii<(*len); ii++) printf("  slot %2d, boardID 0x%08x\n",ii,buf[ii]);
  printf("\n");
  */
  return(0);
}



/* following function called from boot script */
static int
ScalersReadoutStart()
{
#ifndef VXWORKS
  pthread_t id;
#endif  

#ifdef VXWORKS
  vmescalers_lock = semBCreate(SEM_Q_FIFO, SEM_FULL);
  if(vmescalers_lock == NULL)
  {
    printf("ScalersReadoutStart ERROR: could not allocate a semaphore\n");
    return(-1);
  }
#else
  pthread_mutex_init(&vmescalers_lock, NULL);
#endif

#ifdef VXWORKS
  if(taskSpawn("vmeREAD", 250, 0, 100000,(FUNCPTR)vmeReadTask,0,0,0,0,0,0,0,0,0,0) == ERROR)
  {
    printf("ERROR ScalersReadoutStart: cannot start thread\n");
    perror("taskSpawn"); 
    return(-2);
  }
  else
  {
    printf("ScalersReadoutStart: 'vmeREAD' task started\n");
  }
#else
  {
    pthread_attr_t attr;

    pthread_attr_init(&attr); /* initialize attr with default attributes */
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

#ifdef Linux_vme
    pthread_create(&id, &attr, vmeReadTask, NULL);
#endif
#ifdef Linux_armv7l
    pthread_create(&id, &attr, vtpReadTask, NULL);
#endif
  }
#endif

  return(0);
}


static int
ScalersReadoutStop()
{
  /* TODO: END TASK HERE !!!!!!!!!!!!!!!!!!!!!!!! */

  free(vmescalers);
  free(vmedata);

#ifdef VXWORKS
  semGive(vmescalers_lock);
  semDelete(vmescalers_lock);
#else
  pthread_mutex_unlock(&vmescalers_lock);
  pthread_mutex_destroy(&vmescalers_lock);
#endif

  return(0);
}






/********************************************************************************************************
 end of scalers stuff
********************************************************************************************************/

#endif








void sig_handler(int signo);

ServerCBFunctions gCrateServerCBFcn;

int swap32(int val)
{
  return DW_SWAP(val);
}

short swap16(short val)
{
  return HW_SWAP(val);
}


#ifdef Linux_vme /* vme section */

int
Vme_Read16(Cmd_Read16 *pCmd_Read16, Cmd_Read16_Rsp *pCmd_Read16_Rsp)
{
	short *pRd;
	short *pWr = (short *)((long)pCmd_Read16_Rsp->vals);
	int c = pCmd_Read16->cnt;
	int size = 4+2*c;

	vmeBusToLocalAdrs(0x39, (char *)((long)pCmd_Read16->addr),(char **)&pRd);

	pCmd_Read16_Rsp->cnt = c;	
	
vmeBusLock();
	if(pCmd_Read16->flags & CRATE_MSG_FLAGS_ADRINC)
		while(c--) *pWr++ = swap16(*pRd++);
	else
		while(c--) *pWr++ = swap16(*pRd);
vmeBusUnlock();

	return size;
}

int
Vme_Write16(Cmd_Write16 *pCmd_Write16)
{
	short *pRd = pCmd_Write16->vals;
	short *pWr;
	int c = pCmd_Write16->cnt;
	
	vmeBusToLocalAdrs(0x39, (char *)((long)pCmd_Write16->addr),(char **)&pWr);

	vmeBusLock();
	if(pCmd_Write16->flags & CRATE_MSG_FLAGS_ADRINC)
		while(c--) *pWr++ = swap16(*pRd++);
	else
		while(c--) *pWr = swap16(*pRd++);
	vmeBusUnlock();

	return 0;
}

int
Vme_Read32(Cmd_Read32 *pCmd_Read32, Cmd_Read32_Rsp *pCmd_Read32_Rsp)
{
	int *pRd;
	int *pWr = pCmd_Read32_Rsp->vals;
	int c = pCmd_Read32->cnt;
	int size = 4+4*c;

	vmeBusToLocalAdrs(0x39, (char *)((long)pCmd_Read32->addr),(char **)&pRd);

	pCmd_Read32_Rsp->cnt = c;	

	/*printf("Vme_Read32: addr=0x%08x, laddr=0x%08x, cnt=%d, flag=%d\n",pCmd_Read32->addr,pRd,c,pCmd_Read32->flags);*/

	vmeBusLock();
	if(pCmd_Read32->flags & CRATE_MSG_FLAGS_ADRINC)
		while(c--) *pWr++ = swap32(*pRd++);
	else
		while(c--) *pWr++ = swap32(*pRd);
	vmeBusUnlock();

	return size;
}

int
Vme_Write32(Cmd_Write32 *pCmd_Write32)
{
	int *pRd = pCmd_Write32->vals;
	int *pWr;
	int c = pCmd_Write32->cnt;
	
	vmeBusToLocalAdrs(0x39, (char *)((long)pCmd_Write32->addr),(char **)&pWr);

	vmeBusLock();
	if(pCmd_Write32->flags & CRATE_MSG_FLAGS_ADRINC)
		while(c--) *pWr++ = swap32(*pRd++);
	else
		while(c--) *pWr = swap32(*pRd++);
	vmeBusUnlock();

	return 0;
}


/**************************/
/* start scaler functions */


int
Vme_ReadScalers(Cmd_ReadScalers *pCmd_ReadScalers, Cmd_ReadScalers_Rsp *pCmd_ReadScalers_Rsp)
{
  int *pWr = pCmd_ReadScalers_Rsp->vals;
  int c = pCmd_ReadScalers->cnt; /* count from input message - not used */
  int size/* = 4+4*c*/;
  int nw, slot, ii;

  /*printf("Vme_ReadScalers reached\n");fflush(stdout);*/
  nw = pCmd_ReadScalers->cnt;
  slot = pCmd_ReadScalers->slot;
  /*printf("Vme_ReadScalers: input: cnt=%d slot=%d\n",nw,slot);*/

  SCALER_LOCK;

  pCmd_ReadScalers_Rsp->cnt = vmescalerslen[slot];

  for(ii=0; ii<pCmd_ReadScalers_Rsp->cnt; ii++) pWr[ii] = vmescalers[slot][ii];

  size = pCmd_ReadScalers_Rsp->cnt*4 + 4; /* +4 because have to count 'cnt' in outgoing message */
  /*printf("Vme_ReadScalers: respond: cnt=%d (size=%d) 100=%d 120=%d\n",pCmd_ReadScalers_Rsp->cnt,size,pWr[100],pWr[120]);*/

  SCALER_UNLOCK;

  return(size); /* size of outgoing message in bytes ! */
}


int
Vme_ReadData(Cmd_ReadScalers *pCmd_ReadScalers, Cmd_ReadScalers_Rsp *pCmd_ReadScalers_Rsp)
{
  int *pWr = pCmd_ReadScalers_Rsp->vals;
  int c = pCmd_ReadScalers->cnt; /* count from input message - not used */
  int size/* = 4+4*c*/;
  int nw, slot, ii;

  /*printf("Vme_ReadData reached\n");fflush(stdout);*/
  nw = pCmd_ReadScalers->cnt;
  slot = pCmd_ReadScalers->slot;
  /*printf("Vme_ReadData: input: cnt=%d slot=%d\n",nw,slot);*/

  SCALER_LOCK;

  pCmd_ReadScalers_Rsp->cnt = vmedatalen[slot];

  for(ii=0; ii<pCmd_ReadScalers_Rsp->cnt; ii++) pWr[ii] = vmedata[slot][ii];
  /*printf("Vme_ReadData: respond: cnt=%d (size=%d) 100=%d 120=%d\n",pCmd_ReadScalers_Rsp->cnt,size,pWr[100],pWr[120]);*/

  size = pCmd_ReadScalers_Rsp->cnt*4 + 4; /* +4 because have to count 'cnt' in outgoing message */

  SCALER_UNLOCK;

  return(size); /* size of outgoing message in bytes ! */
}



int
Vme_GetBoardParams(Cmd_GetBoardParams *pCmd, Cmd_GetBoardParams_Rsp *pCmd_Rsp)
{
  int *pWr = pCmd_Rsp->vals;
  int size = 4;
  int slot = pCmd->slot;
  int partype = pCmd->partype;
  int len;
  /*
  printf("Vme_GetBoardParams reached, slot=%d, partype=%d\n",slot,partype);fflush(stdout);
  */
  vmeGetBoardParams(slot, partype, pWr, &len);

  pCmd_Rsp->cnt = len;
  size = len*4 + 4;

  return(size); /* output message size in bytes */
}


int
Vme_GetChannelParams(Cmd_GetChannelParams *pCmd, Cmd_GetChannelParams_Rsp *pCmd_Rsp)
{
  int *pWr = pCmd_Rsp->vals;
  int size = 4;
  int slot = pCmd->slot;
  int channel = pCmd->channel;
  int partype = pCmd->partype;
  int len;
  /*
  printf("Vme_GetChannelParams reached, slot=%d, channel=%d, partype=%d\n",slot,channel,partype);fflush(stdout);
  */
  vmeGetChannelParams(slot, channel, partype, pWr, &len);

  pCmd_Rsp->cnt = len;
  size = len*4 + 4;

  return(size); /* output message size in bytes */
}


int
Vme_SetChannelParams(Cmd_SetChannelParams *pCmd)
{
  int *pRd = pCmd->vals;
  int size = 4;
  int slot = pCmd->slot;
  int channel = pCmd->channel;
  int partype = pCmd->partype;
  int len = pCmd->cnt;
  /*
  printf("Vme_SetChannelParams reached, slot=%d, channel=%d, partype=%d, len=%d, buf[0]=%d\n",
    slot,channel,partype,len,*pRd);
  fflush(stdout);
  */
  vmeSetChannelParams(slot, channel, partype, pRd, len);

  return(0); /* output message size in bytes */
}



/* end scaler functions */
/************************/

#endif /* vme section */



#ifdef Linux_armv7l /* vtp section */

int
Vtp_Read32(Cmd_Read32 *pCmd_Read32, Cmd_Read32_Rsp *pCmd_Read32_Rsp)
{
	unsigned int pRd = pCmd_Read32->addr; /*vtpRead32(volatile unsigned int *addr)*/
	unsigned int *pWr = pCmd_Read32_Rsp->vals;
	int c = pCmd_Read32->cnt;
	int size = 4+4*c;

	pCmd_Read32_Rsp->cnt = c;
	if(pCmd_Read32->flags & CRATE_MSG_FLAGS_ADRINC)
	  while(c--) {*pWr++ = vtpRead32((volatile unsigned int *)pRd); pRd++;}
	else
	  while(c--) {*pWr++ = vtpRead32((volatile unsigned int *)pRd);}

	return size;
}

int
Vtp_Write32(Cmd_Write32 *pCmd_Write32)
{
	unsigned int *pRd = pCmd_Write32->vals;
	unsigned int pWr = pCmd_Write32->addr; /*vtpWrite32(volatile unsigned int *addr, unsigned int val)*/
	int c = pCmd_Write32->cnt;
	unsigned int val;

	if(pCmd_Write32->flags & CRATE_MSG_FLAGS_ADRINC)
	  while(c--) {val = *pRd++; vtpWrite32((volatile unsigned int *)pWr, val); pWr++;}
	else
	  while(c--) {val = *pRd++; vtpWrite32((volatile unsigned int *)pWr, val);}

	return 0;
}

int
Vtp_ReadScalers(Cmd_ReadScalers *pCmd_ReadScalers, Cmd_ReadScalers_Rsp *pCmd_ReadScalers_Rsp)
{
  unsigned int *pWr = pCmd_ReadScalers_Rsp->vals;
  int c = pCmd_ReadScalers->cnt; /* count from input message - not used */
  int size/* = 4+4*c*/;
  int nw, slot, ii;

  /*printf("Vtp_ReadScalers reached\n");fflush(stdout);*/
  nw = pCmd_ReadScalers->cnt;
  slot = pCmd_ReadScalers->slot;
  /*printf("Vtp_ReadScalers: input: cnt=%d slot=%d\n",nw,slot);*/

  pCmd_ReadScalers_Rsp->cnt = vmescalerslen[slot];

  for(ii=0; ii<pCmd_ReadScalers_Rsp->cnt; ii++) pWr[ii] = vmescalers[slot][ii];
  /*printf("Vtp_ReadScalers: respond: cnt=%d (size=%d) 100=%d 120=%d\n",pCmd_ReadScalers_Rsp->cnt,size,pWr[100],pWr[120]);*/

  size = pCmd_ReadScalers_Rsp->cnt*4 + 4; /* +4 because have to count 'cnt' in outgoing message */

  return(size); /* size of outgoing message in bytes ! */
}

#endif /* vtp section */






int
Vme_GetCrateMap(Cmd_GetCrateMap *pCmd, Cmd_GetCrateMap_Rsp *pCmd_Rsp)
{
  int *pWr = pCmd_Rsp->vals;
  int c = pCmd->cnt; /* count from input message - not used */
  int size/* = 4+4*c*/;
  int nw, slot, ii;
  
  printf("Vme_GetCrateMap reached\n");fflush(stdout);
  
  GetCrateMap(pWr, &pCmd_Rsp->nslots);
  pCmd_Rsp->cnt = pCmd_Rsp->nslots;
  
  for(ii=0; ii<pCmd_Rsp->nslots; ii++) printf("Vme_GetCrateMap: [%2d] 0x%08x\n",ii,pWr[ii]);
  
  size = pCmd_Rsp->nslots*4 + 8; /* +8 because have to count 'cnt' and 'nslots' in outgoing message */

  return(size); /* output message size in bytes */
}





int
Vme_Delay(Cmd_Delay *pCmd_Delay)
{
  usleep(1000*pCmd_Delay->ms);
	
  return(0);
}




int
main(int argc, char *argv[])
{
  int stat;
#ifdef SCALER_THREAD
  pthread_t gScalerThread;
#endif



  /*check if 'init' flag specified; if so, will init all boards found*/
  init_boards = 0;
  if(argc==2)
  {
    if(!strncmp(argv[1],"init",4))
	{
      init_boards = 1;
      printf("NOTE: boards initialization will be performed and mutex cleared, can be a problem if DAQ is running !!!\n");
	}
    else
	{
      printf("NOTE: boards initialization will NOT be performed\n");
	}
  }

  /*always init !!!
  init_boards = 1;
  printf("NOTE: boards initialization will be performed\n");
  */


  if(signal(SIGINT, sig_handler) == SIG_ERR)
  {
	perror("signal");
	exit(0);
  }

#ifdef Linux_vme
  /*vmeSetQuietFlag(1);*/
  stat = vmeOpenDefaultWindows();
  if(stat != 0) goto CLOSE;

  if(init_boards) vmeCheckMutexHealth(1); /* kiils mutex if exist */

#endif

#ifdef Linux_armv7l
  stat = vtpOpen(VTP_FPGA_OPEN);
  if(stat != VTP_FPGA_OPEN) goto CLOSE;
  else printf("vtpOpen'ed\n");

  if(init_boards) vtpCheckMutexHealth(1); 
#endif


#ifdef SCALER_THREAD

#ifdef Linux_vme
  {
    int i1, i2, i3;

    usrVmeDmaInit();

    usrVmeDmaMemory(&i1, &i2, &i3);
    i2_from_rol1 = i2;
    printf("tiprimarytinit: i2_from_rol1 = 0x%08x\n",i2_from_rol1);
    i2_from_rol1 = (i2_from_rol1 & 0xFFFFFFF0);
    printf("tiprimarytinit: i2_from_rol1 = 0x%08x\n",i2_from_rol1);
    i2_from_rol1 = i2_from_rol1 + 0x10;
    printf("tiprimarytinit: i2_from_rol1 = 0x%08x\n",i2_from_rol1);
  }
#endif

  /* connect to IPC server */
  printf("Connect to IPC server...\n");
  /*epics_json_msg_sender_init(getenv("EXPID"), getenv("SESSION"), "daq", "HallB_DAQ");*/
  epics_json_msg_sender_init("clasrun", "clasprod", "daq", "HallB_DAQ");
  printf("done.\n");


  printf("Start scaler readout thread...\n");
  ScalersReadoutStart(); /* pthread_create inside */
  printf("done.\n");

  sleep(2);
#endif


//	stat = crateShmCreate();
//	if(stat != 0)
//		goto CLOSE;

// Run socket server

#ifdef Linux_vme
  gCrateServerCBFcn.Read16 = Vme_Read16;
  gCrateServerCBFcn.Write16 = Vme_Write16;
  gCrateServerCBFcn.Read32 = Vme_Read32;
  gCrateServerCBFcn.Write32 = Vme_Write32;
  gCrateServerCBFcn.Delay = Vme_Delay;
  gCrateServerCBFcn.ReadScalers = Vme_ReadScalers;
  gCrateServerCBFcn.ReadData = Vme_ReadData;
  gCrateServerCBFcn.GetCrateMap = Vme_GetCrateMap;
  gCrateServerCBFcn.GetBoardParams = Vme_GetBoardParams;
  gCrateServerCBFcn.GetChannelParams = Vme_GetChannelParams;
  gCrateServerCBFcn.SetChannelParams = Vme_SetChannelParams;
#endif

#ifdef Linux_armv7l
  gCrateServerCBFcn.Read16 = NULL;
  gCrateServerCBFcn.Write16 = NULL;
  gCrateServerCBFcn.Read32 = Vtp_Read32;
  gCrateServerCBFcn.Write32 = Vtp_Write32;
  gCrateServerCBFcn.Delay = NULL;
  gCrateServerCBFcn.ReadScalers = Vtp_ReadScalers;
  gCrateServerCBFcn.ReadData = NULL;
  gCrateServerCBFcn.GetCrateMap = Vme_GetCrateMap;
  gCrateServerCBFcn.GetBoardParams = NULL;
  gCrateServerCBFcn.GetChannelParams = NULL;
  gCrateServerCBFcn.SetChannelParams = NULL;
#endif

  printf("Starting CrateMsgServer...");fflush(stdout);
  CrateMsgServerStart(&gCrateServerCBFcn, CRATEMSG_LISTEN_PORT);
  printf("Done.\n");fflush(stdout);


  while(1) sleep(1);

	
CLOSE:

  /* disconnect from IPC server */
  epics_json_msg_close();

#ifdef Linux_vme
  vmeCloseDefaultWindows();
#endif
#ifdef Linux_armv7l
  vtpClose(VTP_FPGA_OPEN);
  printf("vtpClose'd\n");
#endif
	
  return 0;
}

void
closeup()
{
#ifdef Linux_vme
  vmeCloseDefaultWindows();
#endif
#ifdef Linux_armv7l
  vtpClose(VTP_FPGA_OPEN);
#endif

  printf("DiagGUI server closed...\n");
}

void
sig_handler(int signo)
{
	printf("%s: signo = %d\n",__FUNCTION__,signo);
	switch(signo)
	{
		case SIGINT:
			closeup();
			exit(1);  /* exit if CRTL/C is issued */
	}
}

#else

main()
{
  return;
}

#endif
