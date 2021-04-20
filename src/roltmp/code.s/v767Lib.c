/******************************************************************************
 *
 *  v767Lib.c     -  Driver library for readout of C.A.E.N. Model 767 TDC
 *                   using a VxWorks 5.2 or later based Single Board computer. 
 *
 *  Author: David Abbott 
 *          Jefferson Lab Data Acquisition Group
 *          August 2002
 *
 *  Revision  1.0 - Initial Revision
 *                    - Supports up to 20 CAEN Model 767s in a Crate
 *                    - Programmed I/O read
 *                    - Interrupts from a Single 767
 *
 *  Revision 1.1  - Added block reads and event flushing
 *  Revision 1.1saw - Added V767Status2 call to return status 2 register
 *  Revision 1.2  - Port to Linux
 *
 */

#if defined(VXWORKS) || defined(Linux_vme)

#ifdef VXWORKS
#include "vxWorks.h"
#include "logLib.h"
#include "taskLib.h"
#include "intLib.h"
#include "iv.h"
#include "semLib.h"
#include "vxLib.h"
#else
#include "jvme.h"
#endif
#include "stdio.h"
#include "string.h"
#include <pthread.h>
#include "math.h"

/* Include TDC definitions */
#include "v767Lib.h"

#define MAX_TDC_CHIPS 4

#ifdef VXWORKS
/* Include DMA Library definintions */
#include "universeDma.h"

/* Define external Functions */
IMPORT  STATUS sysBusToLocalAdrs(int, char *, char **);
IMPORT  STATUS intDisconnect(int);
IMPORT  STATUS sysIntEnable(int);
IMPORT  STATUS sysIntDisable(int);
#endif

/* Mutex to guard c775 reads/writes */
pthread_mutex_t v767mutex = PTHREAD_MUTEX_INITIALIZER;
#define V767LOCK   if(pthread_mutex_lock(&v767mutex)<0) perror("pthread_mutex_lock");
#define V767UNLOCK if(pthread_mutex_unlock(&v767mutex)<0) perror("pthread_mutex_unlock");

/* Macros */
#define V767_EXEC_MODULE_RESET(ii) {vmeWrite16(&v767p[ii]->SSReset, 1);}
#define V767_EXEC_EVENT_RESET(ii)  {vmeWrite16(&v767p[ii]->clearCount, 1);}
#define V767_EXEC_CLEAR(ii)        {vmeWrite16(&v767p[ii]->clear, 1);}
#define V767_EXEC_TRIGGER(ii)      {vmeWrite16(&v767p[ii]->trigger, 1);}



/* Define Interrupts variables */
BOOL              v767IntRunning  = FALSE;                    /* running flag */
int               v767IntID       = -1;                       /* id number of TDC generating interrupts */
LOCAL VOIDFUNCPTR v767IntRoutine  = NULL;                     /* user interrupt service routine */
LOCAL int         v767IntArg      = 0;                        /* arg to user routine */
LOCAL int         v767IntEvCount  = 0;                        /* Number of Events to generate Interrupt */
LOCAL UINT32      v767IntLevel    = V767_VME_INT_LEVEL;       /* default VME interrupt level */
LOCAL UINT32      v767IntVec      = V767_INT_VEC;             /* default interrupt Vector */


/* Define global variables */
int Nv767 = 0;                                /* Number of TDCs in Crate */
volatile struct v767_struct *v767p[20];       /* pointers to TDC memory map */
int v767IntCount = 0;                         /* Count of interrupts from TDC */
int v767EventCount[20];                       /* Count of Events taken by TDC (Event Count Register value) */
int v767EvtReadCnt[20];                       /* Count of events read from specified TDC */
unsigned int v767MemOffset = 0;               /* CPUs A24 or A32 address space offset */

#ifdef VXWORKS
SEM_ID v767Sem;                               /* Semephore for Task syncronization */
#endif

/*******************************************************************************
 *
 * v767Init - Initialize v767 Library. 
 *
 *
 * RETURNS: OK, or ERROR if the address is invalid or board is not present.
 */

STATUS 
v767Init (UINT32 addr, UINT32 addr_inc, int nadc, UINT16 crateID)
{
  int ii, res, rdata, errFlag = 0;
  UINT32 boardID = 0;
  unsigned long laddr;
  
  /* Check for valid address */
  if(addr==0) 
    {
      printf("v767Init: ERROR: Must specify a Bus (VME-based A32/A24) address for TDC 0\n");
      return(ERROR);
    }
  else if(addr < 0x00ffffff) 
    { /* A24 Addressing */
      if((addr_inc==0)||(nadc==0))
	nadc = 1; /* assume only one TDC to initialize */

      /* get the TDCs address */
#ifdef VXWORKS
      res = sysBusToLocalAdrs(0x39,(char *)addr,(char **)&laddr);
#else
      res = vmeBusToLocalAdrs(0x39,(char *)addr,(char **)&laddr);
#endif
      if (res != 0) 
	{
	  printf("v767Init: ERROR in sysBusToLocalAdrs(0x39,0x%x,&laddr) \n",addr);
	  return(ERROR);
	}
      v767MemOffset = laddr - addr;
    }
  else
    { /* A32 Addressing */
      if((addr_inc==0)||(nadc==0))
	nadc = 1; /* assume only one TDC to initialize */

      /* get the TDC address */
#ifdef VXWORKS
      res = sysBusToLocalAdrs(0x09,(char *)addr,(char **)&laddr);
#else
      res = vmeBusToLocalAdrs(0x09,(char *)addr,(char **)&laddr);
#endif
      if (res != 0) 
	{
	  printf("v767Init: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",addr);
	  return(ERROR);
	}
      v767MemOffset = laddr - addr;
    }

  Nv767 = 0;
  for (ii=0;ii<nadc;ii++) 
    {
      v767p[ii] = (struct v767_struct *)(laddr + ii*addr_inc);
      /* Check if Board exists at that address -
	 Using geoAddr just in case BERR is enabled in the FIFO */
#ifdef VXWORKS
      res = vxMemProbe((char *) &v767p[ii]->geoAddr,0,2,(char *)&rdata);
#else
      res = vmeMemProbe((char *) &v767p[ii]->geoAddr,2,(char *)&rdata);
#endif
      if(res < 0) 
	{
	  printf("v767Init: ERROR: No addressable board at addr=0x%x\n",(UINT32) v767p[ii]);
	  v767p[ii] = NULL;
	  errFlag = 1;
	  break;
	} 
      else 
	{
	  boardID = 
	    ( (vmeRead16(&v767p[ii]->cr.boardID[0])&0xff) << 24) |
	    ( (vmeRead16(&v767p[ii]->cr.boardID[2])&0xff) << 16) |
	    ( (vmeRead16(&v767p[ii]->cr.boardID[4])&0xff) << 8) |
	    (vmeRead16(&v767p[ii]->cr.boardID[6])&0xff);
	  if((boardID != V767_BOARD_ID)) 
	    {
	      printf(" ERROR: v767 Firmware revision does not match: %d \n",boardID);
	      return(ERROR);
	    }
	}
      Nv767++;
#ifdef VXWORKS
      printf("Initialized TDC ID %d at address 0x%08x \n",ii,(UINT32) v767p[ii]);
#else
      printf("Initialized TDC ID %d at VME (USER) address 0x%08x (0x%08x) \n",ii,
	     (UINT32) v767p[ii] - v767MemOffset, (UINT32) v767p[ii]);
#endif
    }

#ifdef VXWORKS
  /* Initialize/Create Semephore */
  if(v767Sem != 0) 
    {
      semFlush(v767Sem);
      semDelete(v767Sem);
    }
  v767Sem = semBCreate(SEM_Q_PRIORITY,SEM_EMPTY);
  if(v767Sem <= 0) 
    {
      printf("v767Init: ERROR: Unable to create Binary Semephore\n");
      return(ERROR);
    }
#endif
  
  /* Set Geographic Address with ID , Reset all moduless */
  for(ii=0;ii<Nv767;ii++) 
    {
      vmeWrite16(&v767p[ii]->geoAddr, ii);
      V767_EXEC_MODULE_RESET(ii);
    }
  taskDelay(120);

  if(errFlag > 0) 
    {
      printf("v767Init: ERROR: Unable to initialize all TDC Modules\n");
      if(Nv767 > 0)
	printf("v767Init: %d TDC(s) successfully initialized\n",Nv767);
      return(ERROR);
    } 
  else 
    {
      return(OK);
    }
}

/*******************************************************************************
 *
 * v767Status - Gives Status info on specified TDC
 *
 *
 * RETURNS: None
 */

void
v767Status( int id, int reg, int sflag)
{
  int drdy=0, empty=0, afull=0, bfull=0;
  int berr=0, blk=0, testmode=0, busy=0;
  int acqmode=0, drdymode=0;
  int tdcerror[4];
  UINT16 bitReg, stat1Reg, cntl1Reg, stat2Reg, cntl2Reg;
  UINT16 iLvl, iVec, evCount;

  if((id<0) || (v767p[id] == NULL)) 
    {
      printf("v767Status: ERROR : TDC id %d not initialized \n",id);
      return;
    }

  /* read various registers */

  V767LOCK;
  bitReg = vmeRead16(&v767p[id]->bitSet)&V767_BITSET_MASK;
  stat1Reg = vmeRead16(&v767p[id]->status1)&V767_STATUS1_MASK;
  cntl1Reg = vmeRead16(&v767p[id]->control1)&V767_CONTROL1_MASK;
  stat2Reg = vmeRead16(&v767p[id]->status2)&V767_STATUS2_MASK;
  cntl2Reg = vmeRead16(&v767p[id]->control2)&V767_CONTROL2_MASK;
  
  iLvl = vmeRead16(&v767p[id]->intLevel)&V767_INTLEVEL_MASK;
  iVec = vmeRead16(&v767p[id]->intVector)&V767_INTVECTOR_MASK;
  evCount  = vmeRead16(&v767p[id]->evCount)&V767_EVCOUNT_MASK;
  V767UNLOCK;

  drdy  = stat1Reg&V767_STATUS1_DATA_READY;
  empty = stat2Reg&V767_STATUS2_EMPTY;
  afull = stat2Reg&V767_STATUS2_ALMOST_FULL;
  bfull = stat2Reg&V767_STATUS2_FULL;

  tdcerror[0] = (stat2Reg&V767_STATUS2_ERROR_0)>>12;
  tdcerror[1] = (stat2Reg&V767_STATUS2_ERROR_1)>>13;
  tdcerror[2] = (stat2Reg&V767_STATUS2_ERROR_2)>>14;
  tdcerror[3] = (stat2Reg&V767_STATUS2_ERROR_3)>>15;

  busy  = stat1Reg&V767_STATUS1_BUSY;

  berr = cntl1Reg&V767_CONTROL1_BERR_ENABLE;
  blk  = cntl1Reg&V767_CONTROL1_BLKEND;
  testmode = cntl2Reg&V767_TESTMODE;
  acqmode = cntl2Reg&V767_CONTROL2_ACQMODE_MASK;
  drdymode = (cntl2Reg&V767_CONTROL2_DREADY_MASK)>>2;

  /* print out status info */

#ifdef VXWORKS
  printf("STATUS for TDC id %d at base address 0x%x \n",id,(UINT32) v767p[id]);
#else
  printf("STATUS for TDC id %d at VME base address 0x%.8x \n",id,
	 (UINT32) v767p[id]- v767MemOffset);
#endif
  printf("---------------------------------------------- \n");

  if(iLvl>0) 
    {
      printf(" ** Interrupts Enabled **\n");
      printf(" VME Interrupt Level: %d   Vector: 0x%x \n",iLvl,iVec);
      printf(" Interrupt Count    : %d \n",v767IntCount);
    } 
  else 
    {
      printf(" Interrupts Disabled\n");
      printf(" Last Interrupt Count    : %d \n",v767IntCount);
    }
  printf("\n");

  printf("  Data Status \n");
  printf("               --1--  --2--\n");
  if(bfull) 
    {
      printf("    Status     0x%04x 0x%04x (Buffer Full)\n",stat1Reg,stat2Reg);
    } 
  else if(afull) 
    {
      printf("    Status     0x%04x 0x%04x (Almost Full)\n",stat1Reg,stat2Reg);
    }
  else if(drdy) 
    {
      printf("    Status     0x%04x 0x%04x (Data Ready)\n",stat1Reg,stat2Reg);
    }
  else 
    {
      printf("    Status     0x%04x 0x%04x (No Data)\n",stat1Reg,stat2Reg);
    }

  if(busy)
    printf("    Module Busy   : On\n");
  else
    printf("    Module Busy   : Off\n");
  
  if(evCount == 0xffff)
    printf("    Total Events  = (No Events taken)\n");
  else
    printf("    Total Events  = %d\n",evCount);


  printf("\n");

  printf("  TDC Mode/Status (control1 reg = 0x%04x)\n",cntl1Reg); 
  printf("  TDC Mode/Status (control2 reg = 0x%04x)\n",cntl2Reg); 
  printf("  TDC Mode/Status (bitset reg   = 0x%04x)\n",bitReg);

  if(testmode)
    printf("    Test Mode       : On\n");

  printf("    Aquisition Mode : %d\n",acqmode);
  printf("    Data Ready Mode : %d\n",drdymode);

  if(berr)
    printf("    Bus Errors      : On\n");
  else
    printf("    Bus Errors      : Off\n");

  if(blk)
    printf("    Block End       : On\n");
  else
    printf("    Block End       : Off\n");

  printf("    TDC Errors      : 0:%d   1:%d   2:%d   3:%d\n",
	 tdcerror[0],tdcerror[1],tdcerror[2],tdcerror[3]);

  printf("\n");

}

/* FIXME: Only "works" in vxWorks, at the moment */
#ifdef VXWORKS
/*******************************************************************************
 *
 * v767Int - default interrupt handler
 *
 * This rountine handles the v767 TDC interrupt.  A user routine is
 * called, if one was connected by v767IntConnect().
 *
 * RETURNS: N/A
 *
 */

LOCAL void 
v767Int (void)
{
  UINT32 nevt=0;
  
  /* Disable interrupts */
  sysIntDisable(v767IntLevel);

  v767IntCount++;
 
  if (v767IntRoutine != NULL)  
    {     /* call user routine */
      (*v767IntRoutine) (v767IntArg);
    }
  else
    {
      if((v767IntID<0) || (v767p[v767IntID] == NULL)) 
	{
	  logMsg("v767Int: ERROR : TDC id %d not initialized \n",v767IntID,0,0,0,0,0);
	  return;
	}

      logMsg("v767Int: Processed %d events\n",nevt,0,0,0,0,0);

    }

  /* Enable interrupts */
  sysIntEnable(v767IntLevel);

}


/*******************************************************************************
 *
 * v767IntConnect - connect a user routine to the v767 TDC interrupt
 *
 * This routine specifies the user interrupt routine to be called at each
 * interrupt. 
 *
 * RETURNS: OK, or ERROR if Interrupts are enabled
 */

STATUS 
v767IntConnect (VOIDFUNCPTR routine, int arg, UINT16 level, UINT16 vector)
{

  if(v767IntRunning) 
    {
      printf("v767IntConnect: ERROR : Interrupts already Initialized for TDC id %d\n",
	     v767IntID);
      return(ERROR);
    }
  
  v767IntRoutine = routine;
  v767IntArg = arg;

  /* Check for user defined VME interrupt level and vector */
  if(level == 0) 
    {
      v767IntLevel = V767_VME_INT_LEVEL; /* use default */
    }
  else if (level > 7) 
    {
      printf("v767IntConnect: ERROR: Invalid VME interrupt level (%d). Must be (1-7)\n",level);
      return(ERROR);
    } 
  else 
    {
      v767IntLevel = level;
    }

  if(vector == 0) 
    {
      v767IntVec = V767_INT_VEC;  /* use default */
    }
  else if ((vector < 32)||(vector>255)) 
    {
      printf("v767IntConnect: ERROR: Invalid interrupt vector (%d). Must be (32<vector<255)\n",vector);
      return(ERROR);
    }
  else
    {
      v767IntVec = vector;
    }
      
  /* Connect the ISR */
#ifdef VXWORKSPPC
  if((intDisconnect((int)INUM_TO_IVEC(v767IntVec)) != 0)) 
    {
      printf("v767IntConnect: ERROR disconnecting Interrupt\n");
      return(ERROR);
    }
#endif
  if((intConnect(INUM_TO_IVEC(v767IntVec),v767Int,0)) != 0) 
    {
      printf("v767IntConnect: ERROR in intConnect()\n");
      return(ERROR);
    }

  return (OK);
}


/*******************************************************************************
 *
 * v767IntEnable - Enable interrupts from specified TDC
 *
 * Enables interrupts for a specified TDC.
 * 
 * RETURNS OK or ERROR if TDC is not available or parameter is out of range
 */

STATUS 
v767IntEnable (int id, UINT16 evCnt)
{

  if(v767IntRunning) 
    {
      printf("v767IntEnable: ERROR : Interrupts already initialized for TDC id %d\n",
	     v767IntID);
      return(ERROR);
    }

  if((id<0) || (v767p[id] == NULL)) 
    {
      printf("v767IntEnable: ERROR : TDC id %d not initialized \n",id);
      return(ERROR);
    }
  else
    {
      v767IntID = id;
    }
  
  
  sysIntEnable(v767IntLevel);   /* Enable VME interrupts */
  
  /* Zero Counter and set Running Flag */
  v767IntEvCount = evCnt;
  v767IntCount = 0;
  v767IntRunning = TRUE;
  /* Enable interrupts on TDC */
  
  return(OK);
}


/*******************************************************************************
 *
 * v767IntDisable - disable the TDC interrupts
 *
 * RETURNS: OK, or ERROR if not initialized
 */

STATUS 
v767IntDisable (int iflag)
{

  if((v767IntID<0) || (v767p[v767IntID] == NULL)) 
    {
      logMsg("v767IntDisable: ERROR : TDC id %d not initialized \n",v767IntID,0,0,0,0,0);
      return(ERROR);
    }

  sysIntDisable(v767IntLevel);   /* Disable VME interrupts */


  /* Tell tasks that Interrupts have been disabled */
  if(iflag > 0) 
    {
      v767IntRunning = FALSE;
    }
  else
    {
      semGive(v767Sem);
    }
  
  return (OK);
}
#endif /* VXWORKS */

/*******************************************************************************
 *
 *  v767ReadMicro - Read Microcontroller Register
 *
 *
 *   returns  OK or ERROR
 */
STATUS
v767ReadMicro(int id, UINT16 *data, int nwords)
{
  int ii,jj,kk=0;
  UINT16 mstatus;

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767ReadMicro: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }

  /* default to read one dataword */
  if(nwords==0) nwords=1;

  V767LOCK;

retry:

  mstatus = vmeRead16(&v767p[id]->microHandshake)&V767_MICRO_READOK;

  if(mstatus) 
  {
    for(ii=0; ii<nwords; ii++) 
	{
	  jj=0;
	  while(!(vmeRead16(&v767p[id]->microHandshake)&V767_MICRO_READOK)) 
	  {
	    jj++;
	    if(jj>1000) 
		{
		  logMsg("v767ReadMicro: ERROR: Read Status not OK (read %d)\n",ii,0,0,0,0,0);
		  V767UNLOCK;
		  return(ERROR);
		}
	  }
	  taskDelay(1);
	  data[ii] = vmeRead16(&v767p[id]->microReg);
	}
  }
  else
  {
    kk++;
    if(kk>=1000) 
	{
	  logMsg("v767ReadMicro: ERROR: Read Status not OK\n",0,0,0,0,0,0);
	  V767UNLOCK;
	  return(ERROR);
	}
    else
	{
	  taskDelay(1); /*sergey*/
	  goto retry;
	}
  }

  V767UNLOCK;

  return(OK);
}

/*******************************************************************************
 *
 *  v767WriteMicro - Write to Microcontroller Register
 *
 *
 *   returns  OK or ERROR
 */
STATUS
v767WriteMicro(int id, UINT16 data)
{
  int kk=0;
  volatile UINT16 mstatus=0;

  if((id<0) || (v767p[id] == NULL)) 
  {
      logMsg("v767WriteMicro: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return(ERROR);
  }

  V767LOCK;

retry:

  mstatus = vmeRead16(&v767p[id]->microHandshake)&V767_MICRO_WRITEOK;

  if(mstatus) 
  {
    taskDelay(1);
    vmeWrite16(&v767p[id]->microReg, data);
  }
  else
  {
    kk++;
    mstatus=0;
    if(kk>=1000) 
	{
	  logMsg("v767WriteMicro: ERROR: Write Status not OK\n",0,0,0,0,0,0);
	  V767UNLOCK;
	  return(ERROR);
	}
    else
	{
	  taskDelay(1); /* sergey*/
	  goto retry;
	}
  }

  V767UNLOCK;
  return(OK);

}

/*******************************************************************************
 *
 * v767TriggerConfig  - Program or Read Back TDC Trigger Matching Configuration
 * 
 * 
 * 
 * 
 *
 * RETURNS: OK or ERROR.
 */

STATUS
v767TriggerConfig(int id, INT16 mw, INT16 to, INT16 tl)
{

  UINT16 t1, t2, t3, config;
  UINT16 tdata;

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767TriggerConfig: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }

  if((mw==0)&&(to==0)&&(tl==0)) 
    {
      /* Read and print Trigger Configuration */
      v767WriteMicro(id,GET_WINDOW_WIDTH);
      v767ReadMicro(id,&t1,1);

      v767WriteMicro(id,GET_WINDOW_OFFSET);
      v767ReadMicro(id,&t2,1);

      v767WriteMicro(id,GET_TRIGGER_LATENCY);
      v767ReadMicro(id,&t3,1);

      v767WriteMicro(id,READ_TRIGGER_CONFIG);
      v767ReadMicro(id,&config,1);

      printf("  Match Window       = %d ns\n",(int) (t1*V767_CLOCK_PERIOD));
      printf("  Window  Offset     = %d ns\n",(int) (t2*V767_CLOCK_PERIOD));
      printf("  Trigger Latency    = %d ns\n",(int) (t3*V767_CLOCK_PERIOD));

      if(config&0x1)
	printf("  Trigger time subtraction ENABLED\n");
      else
	printf("  Trigger time subtraction DISABLED\n");
      if(config&0x2)
	printf("  Overlapping triggers ENABLED\n");
      else
	printf("  Overlapping triggers DISABLED\n");


  }  
  else
  {

    v767WriteMicro(id,SET_WINDOW_WIDTH);
    taskDelay(15); 

    tdata = (UINT16)(mw/V767_CLOCK_PERIOD);
    v767WriteMicro(id,tdata);

    taskDelay(15);

    v767WriteMicro(id,SET_WINDOW_OFFSET);
    taskDelay(15); 

    tdata = (UINT16)(to/V767_CLOCK_PERIOD);
    v767WriteMicro(id,tdata);

    taskDelay(15);

    v767WriteMicro(id,SET_TRIGGER_LATENCY);
    taskDelay(15); 

    tdata = (UINT16)(tl/V767_CLOCK_PERIOD);
    v767WriteMicro(id,tdata);

    taskDelay(15); 

  }


  return(OK);

}


/*******************************************************************************
 *
 * v767PrintEvent - Print an event from TDC to standard out. 
 *
 *
 * RETURNS: Number of Data words read from the TDC (including Header/Trailer).
 */
// FIXME: mutex locks/unlocks
int
v767PrintEvent(int id, int pflag)
{

  int nWords, evCount, trigMatch;
  UINT32 gheader, gtrailer, tmpData, dCnt;
  int chanID, dataVal, tEdge;

  if((id<0) || (v767p[id] == NULL)) 
    {
      printf("v767PrintEvent: ERROR : TDC id %d not initialized \n",id);
      return(ERROR);
    }

  /* Check if there is a valid event */
  V767LOCK;
  if(vmeRead16(&v767p[id]->status1)&V767_STATUS1_DATA_READY) 
    {
      dCnt = 0;
      trigMatch = vmeRead16(&v767p[id]->control2)&V767_CONTROL2_ACQMODE_MASK;
    
      if(trigMatch != V767_ACQMODE_CS) 
	{  /* If trigger match mode then print individual event */
      
	  /* Read Header - Get event count */
	  gheader = vmeRead32(&v767p[id]->fifo);
	  if((gheader&V767_DATA_TYPE_MASK) != V767_HEADER_DATA) 
	    {
	      printf("v767PrintEvent: ERROR: Invalid Header Word 0x%08x\n",gheader);
	      V767UNLOCK;
	      return(ERROR);
	    }
	  else
	    {
	      printf("  TDC DATA for Module %d\n",id);
	      evCount = (gheader&V767_HEADER_EVCOUNT_MASK);
	      dCnt++;
	      printf("    Header: 0x%08x   Event Count = %d \n",gheader,evCount);
	    }

	  /* Loop and get data for each */
	  tmpData = vmeRead32(&v767p[id]->fifo);

	  while ((tmpData&V767_DATA_TYPE_MASK) == V767_DATA) 
	    {
	      dCnt++;
	      if ((dCnt % 5) == 0) printf("\n    ");
	      printf("     0x%08x",tmpData);

	      tmpData = vmeRead32(&v767p[id]->fifo);
	    }
	  printf("\n");

	  /* Out of the Loop so last  data word should be Global EOB */
	  gtrailer = tmpData;
	  if((gtrailer&V767_DATA_TYPE_MASK) != V767_EOB_DATA) 
	    {
	      printf("v767PrintEvent: ERROR: Invalid EOB Word 0x%08x\n",gtrailer);
	      V767UNLOCK;
	      return(ERROR);
	    }
	  else
	    {
	      nWords = (gtrailer&V767_EOB_WORDCOUNT_MASK);
	      dCnt++;
	      printf("    EOB   : 0x%08x   Total Word Count = %d \n",gtrailer,nWords);
	    }

	} 
      else
	{ /* Continuous Storage mode */

	  tmpData = vmeRead32(&v767p[id]->data[dCnt]);
	  printf("  TDC Continuous Storage DATA\n");
	  while ((tmpData&V767_DATA_TYPE_MASK) != V767_INVALID_DATA) 
	    {

	      chanID = (tmpData&V767_CHANNEL_MASK)>>24;
	      tEdge = (tmpData&V767_EDGE_MASK)>>20;
	      dataVal = (tmpData&V767_DATA_MASK);

	      printf("    %d   %d   %d\n",chanID, tEdge, dataVal);
	      dCnt++;
	      tmpData = vmeRead32(&v767p[id]->data[dCnt]);
	    }
	  printf("\n");

	}
      V767UNLOCK;
      return (dCnt);

    }
  else
    {
      printf("v767PrintEvent: No data available for readout!\n");
      V767UNLOCK;
      return(0);
    }
}

/*******************************************************************************
 *
 * v767ReadEvent - Read an event from TDC to a specified address. 
 *
 *
 * RETURNS: Number of Data words read from the TDC (including Header/Trailer).
 */

int
v767ReadEvent(int id, UINT32 *tdata, int maxWords)
{

  int nWords=0, evCount, trigMatch;
  UINT32 gheader, gtrailer, tmpData, dCnt;

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767ReadEvent: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return(ERROR);
    }

  if(maxWords==0) maxWords = 1024;

  V767LOCK;
  /* Check if there is a valid event */
  if(vmeRead16(&v767p[id]->status1)&V767_STATUS1_DATA_READY) 
    {
      dCnt = 0;
      trigMatch = vmeRead16(&v767p[id]->control2)&V767_CONTROL2_ACQMODE_MASK;

      if(trigMatch != V767_ACQMODE_CS)
	{  /* If trigger match mode then read individual event */

	  /* Read Header - Get event count */
	  gheader = vmeRead32(&v767p[id]->fifo);
	  if((gheader&V767_DATA_TYPE_MASK) != V767_HEADER_DATA) 
	    {
	      logMsg("v767ReadEvent: ERROR: Invalid Header Word 0x%08x\n",gheader,0,0,0,0,0);
	      V767UNLOCK;
	      return(ERROR);
	    }
	  else
	    {
#ifdef VXWORKS
	      tdata[dCnt] = gheader;
#else
	      tdata[dCnt] = LSWAP(gheader);
#endif
	      evCount = (gheader&V767_HEADER_EVCOUNT_MASK);
	      dCnt++;
	    }

	  /* Loop and get data for each */
	  tmpData = vmeRead32(&v767p[id]->fifo);
	  while (((tmpData&V767_DATA_TYPE_MASK) == V767_DATA)&&(dCnt<maxWords)) 
	    {
#ifdef VXWORKS
	      tdata[dCnt]=tmpData;
#else
	      tdata[dCnt]=LSWAP(tmpData);
#endif
	      dCnt++;
	      tmpData = vmeRead32(&v767p[id]->fifo);
	    }

	  /* Out of the Loop so last  data word should be Global EOB */
	  gtrailer = tmpData;
	  if((gtrailer&V767_DATA_TYPE_MASK) != V767_EOB_DATA) 
	    {
	      logMsg("v767ReadEvent: ERROR: Invalid EOB Word 0x%08x\n",gtrailer,0,0,0,0,0);
	      V767UNLOCK;
	      return(ERROR);
	    }
	  else
	    {
#ifdef VXWORKS
	      tdata[dCnt] = gtrailer;
#else
	      tdata[dCnt] = LSWAP(gtrailer);
#endif
	      nWords = (gtrailer&V767_EOB_WORDCOUNT_MASK) + 2;
	      dCnt++;
	    }

	} 
      else
	{ /* Continuous Storage mode */

	  tmpData = vmeRead32(&v767p[id]->fifo);
	  while (((tmpData&V767_DATA_TYPE_MASK) != V767_INVALID_DATA)&&(dCnt<maxWords)) 
	    {
#ifdef VXWORKS
	      tdata[dCnt]=tmpData;
#else
	      tdata[dCnt]=LSWAP(tmpData);
#endif
	      dCnt++;
	      tmpData = vmeRead32(&v767p[id]->fifo);
	    }

	}

      if(nWords != dCnt) 
	{
	  logMsg("v767ReadEvent: ERROR: Word Count inconsistant: nWords = %d  Loop Count = %d\n",
		 nWords,dCnt,0,0,0,0);
	  V767UNLOCK;
	  return(ERROR);
	}
      else
	{
	  V767UNLOCK;
	  return (dCnt);
	}

    }
  else
    {
      V767UNLOCK;
      return(0);
    }
}

/*******************************************************************************
 *
 * v767FlushEvent - Flush event/data from TDC. 
 *
 *
 * RETURNS: Number of Data words read from the TDC.
 */

int
v767FlushEvent(int id, int fflag)
{

  int done = 0;
  UINT32 tmpData, dCnt;

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767FlushEvent: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return(-1);
    }

  V767LOCK;
  /* Check if there is a valid event */
  if(vmeRead16(&v767p[id]->status2)&V767_STATUS2_EMPTY) 
    {
      if(fflag > 0) logMsg("v767FlushEvent: Data Buffer is EMPTY!\n",0,0,0,0,0,0);
      V767UNLOCK;
      return(0);
    }

  /* Check if Data Ready Flag is on */
  if(vmeRead16(&v767p[id]->status1)&V767_STATUS1_DATA_READY) 
    {
      dCnt = 0;
    
      while (!done) 
	{
	  tmpData = vmeRead32(&v767p[id]->fifo);

	  switch (tmpData&V767_DATA_TYPE_MASK) 
	    {
	    case V767_HEADER_DATA:
	      if(fflag > 0) logMsg("v767FlushEvent: Found Header 0x%08x\n",tmpData,0,0,0,0,0);
	      break;
	    case V767_DATA:
	      break;
	    case V767_EOB_DATA:
	      if(fflag > 0) logMsg(" v767FlushEvent: Found Trailer 0x%08x\n",tmpData,0,0,0,0,0);
	      done = 1;
	      break;
	    case V767_INVALID_DATA:
	      if(fflag > 0) logMsg(" v767FlushEvent: Buffer Empty 0x%08x\n",tmpData,0,0,0,0,0);
	      done = 1;
	      break;
	    default:
	      if(fflag > 0) logMsg(" v767FlushEvent: Invalid Data 0x%08x\n",tmpData,0,0,0,0,0);
	    }

	  /* Print out Data */
	  if(fflag > 1) 
	    {
	      if ((dCnt % 5) == 0) printf("\n    ");
	      printf("  0x%08x ",tmpData);
	    }
	  dCnt++;
	}
      if(fflag > 1) printf("\n");

      V767UNLOCK;
      return (dCnt);

    }
  else
    {
      if(fflag > 0) logMsg("v767FlushEvent: Data Not ready for readout!\n",0,0,0,0,0,0);
      V767UNLOCK;
      return(0);
    }
}


/*******************************************************************************
 *
 * v767ReadBlock - Read Block of events from TDC to specified address. 
 *
 * INPUTS:    id     - module id of TDC to access
 *            data   - address of data destination
 *            nwrds  - number of data words to transfer
 *
 * RETURNS: OK or ERROR on success of transfer.
 *
 */

int
v767ReadBlock(int id, volatile UINT32 *data, int nwrds)
{
  int retVal, xferCount;
  UINT16 stat = 0;
  UINT32 trailer=0;

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767ReadBlock: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return(-1);
    }

  V767LOCK;
#ifdef VXWORKSPPC
  /* Don't bother checking if there is a valid event. Just blast data out of the 
     FIFO Valid or Invalid 
     Also assume that the Universe DMA programming is setup. */

  retVal = sysVmeDmaSend((UINT32)data, (UINT32)(v767p[id]->data), (nwrds<<2), 0);
  if(retVal < 0) 
    {
      logMsg("v767ReadBlock: ERROR in DMA transfer Initialization 0x%x\n",retVal,0,0,0,0,0);
      V767UNLOCK;
      return(retVal);
    }
  /* Wait until Done or Error */
  retVal = sysVmeDmaDone(1000,1);

#else
  /*sergey
  retVal = vmeDmaSend((UINT32)data,(UINT32)(v767p[id]->data) - v767MemOffset,(nwrds<<2));
  */
  retVal = usrVme2MemDmaStart((UINT32)(v767p[id]->data) - v767MemOffset, (UINT32)data, (nwrds<<2));

  if(retVal < 0) 
    {
      logMsg("v767ReadBlock: ERROR in DMA transfer Initialization 0x%x\n",retVal,0,0,0,0,0);
      V767UNLOCK;
      return(retVal);
    }
  /* Wait until Done or Error */
  /*sergey
  retVal = vmeDmaDone(1000,1);
  */
  retVal = usrVme2MemDmaDone();
#endif
  
  if(retVal != 0) 
    { /* Transfer Error */
      /* Check to see if error was generated by TDC */
      stat = vmeRead16(&v767p[id]->bitSet)&V767_VME_BUS_ERROR;
      if((retVal>0) && (stat)) 
	{
	  vmeWrite16(&v767p[id]->bitClear, V767_VME_BUS_ERROR);
	  /*       logMsg("v767ReadBlock: INFO: DMA terminated by TDC(BUS Error) - Transfer OK\n",0,0,0,0,0,0); */
#ifdef VXWORKS
	  xferCount = (nwrds - (retVal>>2));  /* Number of Longwords transfered */
	  trailer = data[xferCount-1];
#else
	  xferCount = (retVal>>2);  /* Number of Longwords transfered */
	  trailer = LSWAP(data[xferCount-1]);
#endif
	  if ((trailer&V767_DATA_TYPE_MASK) == V767_EOB_DATA) 
	    {
	      V767UNLOCK;
	      return(xferCount); /* Return number of data words transfered */
	    } 
	  else 
	    {
	      /* Check next to last word for trailer */
#ifdef VXWORKS
	      trailer = data[xferCount-2];
#else
	      trailer = LSWAP(data[xferCount-2]);
#endif
	      if ((trailer&V767_DATA_TYPE_MASK) == V767_EOB_DATA) 
		{
		  V767UNLOCK;
		  return(xferCount-1); /* Return number of data words transfered */
		} 
	      else 
		{
		  logMsg("v767ReadBlock: ERROR: Invalid Trailer data 0x%x\n",trailer,0,0,0,0,0);
		  V767UNLOCK;
		  return(xferCount);
		}
	    }
	} 
      else 
	{
	  logMsg("v767ReadBlock: ERROR in DMA transfer 0x%x\n",retVal,0,0,0,0,0);
	  V767UNLOCK;
	  return(retVal);
	}
    } 
  else 
    { /* Transfer OK */
#ifdef VXWORKS
      trailer = data[nwrds-1];
#else
      trailer = LSWAP(data[nwrds-1]);
#endif
      if ( ((nwrds%2)==0) && ((trailer&V767_DATA_TYPE_MASK) != V767_EOB_DATA) )
	logMsg("v767ReadBlock: ERROR: Invalid Trailer data 0x%x\n",trailer,0,0,0,0,0);
      V767UNLOCK;
      return(nwrds);
    }

}


/*******************************************************************************
 *
 * v767Dready - Return status of Data Ready bit in TDC
 *
 *
 * RETURNS: 0(No Data) or 1(Data Ready) or ERROR.
 */

int
v767Dready(int id)
{
  UINT16 stat=0;

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767Dready: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (ERROR);
    }

  V767LOCK;
  stat = vmeRead16(&v767p[id]->status1)&V767_STATUS1_DATA_READY;
  V767UNLOCK;
  if(stat)
    return(1);
  else
    return(0);
}


/*******************************************************************************
 *
 * v767Trig         - Issue Software trigger to TDC
 * v767Clear        - Clear TDC
 * v767Reset        - Clear/Reset TDC
 *
 *
 * RETURNS: None.
 */

void
v767Trig(int id)
{
  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767Trig: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return;
    }
  V767LOCK;
  V767_EXEC_TRIGGER(id);
  V767UNLOCK;
}

void
v767Clear(int id)
{
  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767Clear: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return;
    }
  V767LOCK;
  V767_EXEC_CLEAR(id);
  V767UNLOCK;
}

void
v767Reset(int id)
{
  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767Reset: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return;
    }
  V767LOCK;
  V767_EXEC_MODULE_RESET(id);
  V767UNLOCK;

}



/*******************************************************************************
 *
 * v767Overflow - Return status of Gerror bit in TDC
 *
 *
 * RETURNS: 0(No Data) or 1(Data Ready) or ERROR.
 */

int
v767Overflow(int id)
{

  UINT16 stat=0;

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767Overflow: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (ERROR);
    }
  
  V767LOCK;
  stat = vmeRead16(&v767p[id]->status2)&V767_STATUS2_GERROR;
  V767UNLOCK;

  if(stat)
    return(1);
  else
    return(0);
}


/*******************************************************************************
 *
 * v767Status2 - Return the Status 2 register
 *
 *
 * RETURNS: 0(No Data) or 1(Data Ready) or ERROR.
 */

int
v767Status2(int id)
{
  UINT16 stat=0;

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767Overflow: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (ERROR);
    }

  V767LOCK;
  stat = vmeRead16(&v767p[id]->status2)&0xffff;
  V767UNLOCK;
  return(stat);
}

/*******************************************************************************
 *
 * v767SetACQMode
 *
 * INPUTS:    id     - module id of TDC to access
 *            mode   - 0: stop_tm, 1: start_tm, 2: start_gm, 3: cs
 *
 * RETURNS: OK, or ERROR if not initialized
 */

STATUS
v767SetAcqMode(int id, int mode)
{
  STATUS retval=OK; 

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767SetAcqMode: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (ERROR);
    }

  switch (mode) 
    {
    case V767_ACQMODE_STOP_TM:
      v767WriteMicro(id,SET_ACQMODE_STOP_TM);
      taskDelay(25);
      break;
    case V767_ACQMODE_START_TM:
      v767WriteMicro(id,SET_ACQMODE_START_TM);
      taskDelay(25);
      break;
    case V767_ACQMODE_START_GM:
      v767WriteMicro(id,SET_ACQMODE_START_GM);
      taskDelay(25);
      break;
    case V767_ACQMODE_CS:
      v767WriteMicro(id,SET_ACQMODE_CS);
      taskDelay(25);
      break;
    default:
      retval = ERROR;
      break;
    }

  return (retval);
}


/*******************************************************************************
 *
 * v767SetDReadyMode
 *
 * INPUTS:    id     - module id of TDC to access
 *            mode   - 0: one event, 1: almost full, 2: not empty
 *
 * RETURNS: OK, or ERROR if not initialized
 */

STATUS
v767SetDReadyMode(int id, int mode)
{
  STATUS retval=OK; 

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767SetDReadyMode: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (ERROR);
    }

  switch (mode) 
    {
    case V767_DREADY_MODE_1EVENT:
      v767WriteMicro(id,SET_DREADY_MODE_1EVENT);
      taskDelay(25);
      break;
    case V767_DREADY_MODE_ALMOST_FULL:
      v767WriteMicro(id,SET_DREADY_MODE_ALMOST_FULL);
      taskDelay(25);
      break;  
    case V767_DREADY_MODE_NOT_EMPTY:
      v767WriteMicro(id,SET_DREADY_MODE_NOT_EMPTY);
      taskDelay(25);
      break;
    default:
      retval = ERROR;
      break;
    }

  return (retval);
}

/*******************************************************************************
 *
 * v767BusErrEnable
 *
 * INPUTS:    id     - module id of TDC to access
 *
 * RETURNS: OK, or ERROR if not initialized
 */

STATUS
v767BusErrEnable(int id)
{

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767BusErrEnable: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (ERROR);
    }

  V767LOCK;
  vmeWrite16(&v767p[id]->control1,
	     vmeRead16(&v767p[id]->control1) | V767_CONTROL1_BERR_ENABLE);
  V767UNLOCK;

  return OK;
}


/*******************************************************************************
 *
 * v767BusErrDisable
 *
 * INPUTS:    id     - module id of TDC to access
 *
 * RETURNS: OK, or ERROR if not initialized
 */

STATUS
v767BusErrDisable(int id)
{

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767BusErrDisable: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (ERROR);
    }

  V767LOCK;
  vmeWrite16(&v767p[id]->control1,
	     vmeRead16(&v767p[id]->control1) & ~V767_CONTROL1_BERR_ENABLE);
  V767UNLOCK;

  return OK;
}


/*******************************************************************************
 *
 * v767AddrRotary
 *
 * INPUTS:    id     - module id of TDC to access
 *
 * RETURNS: OK, or ERROR if not initialized
 */

STATUS
v767AddrRotary(int id)
{

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767AddrRotary: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (ERROR);
    }

  V767LOCK;
  vmeWrite16(&v767p[id]->bitClear, V767_SELADDR_MASK);
  V767UNLOCK;

  return OK;
}


/*******************************************************************************
 *
 * v767SetBLKEndMode
 *
 * INPUTS:    id     - module id of TDC to access
 *            mode   - 0: Block ends when buffer is completly empty 1: Block ends after first EOB
 *
 * RETURNS: OK, or ERROR if not initialized
 */

STATUS
v767SetBLKEndMode(int id, int mode)
{
  STATUS retval=OK; 

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767SetBLKEndMode : ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (ERROR);
    }
  V767LOCK;

  switch (mode) 
    {
    case 0:
      vmeWrite16(&v767p[id]->control1,
		 vmeRead16(&v767p[id]->control1) & ~V767_CONTROL1_BLKEND);
      break;
    case 1:
      vmeWrite16(&v767p[id]->control1,
		 vmeRead16(&v767p[id]->control1) | V767_CONTROL1_BLKEND);
      break;
    default:
      retval = ERROR;
      break;
    }
  V767UNLOCK;
  return (retval);
}

/*******************************************************************************
 *
 * v767AutoloadEnable
 *
 * INPUTS:    id     - module id of TDC to access
 *
 * RETURNS: OK, or ERROR if not initialized
 */

STATUS
v767AutoloadEnable(int id)
{

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767AutoloadEnable: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (ERROR);
    }

  v767WriteMicro(id,ENABLE_AUTO_LOAD);
  taskDelay(15);

  return OK;
}

/*******************************************************************************
 *
 * v767AutoloadDisable
 *
 * INPUTS:    id     - module id of TDC to access
 *
 * RETURNS: OK, or ERROR if not initialized
 */

STATUS
v767AutoloadDisable(int id)
{

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767AutoloadDisable: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (ERROR);
    }

  v767WriteMicro(id,DISABLE_AUTO_LOAD);
  taskDelay(15);

  return OK;
}

/*******************************************************************************
 *
 * v767SaveConfig
 *
 * INPUTS:    id     - module id of TDC to access
 *
 * RETURNS: OK, or ERROR if not initialized
 */

STATUS
v767SaveConfig(int id)
{

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767SaveConfig: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (ERROR);
    }

  v767WriteMicro(id,SAVE_CONFIG);
  taskDelay(100);

  return OK;
}

/*******************************************************************************
 *
 * v767SetEdgeDetectionMode
 *
 * INPUTS:    id     - module id of TDC to access
 *            mode   - 0: rising edge, 1: falling edge, 6: both edges on all channels
 *            The TDC supports more modes, which are not yet implemented
 *
 * RETURNS: OK, or ERROR if not initialized
 */

STATUS
v767SetEdgeDetectionMode(int id, int mode)
{
  STATUS retval=OK; 

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767SetedgeDetectionMode: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (ERROR);
    }

  switch (mode) 
    {
    case 0:
      v767WriteMicro(id,EDGEDET_RISE_ALL);
      taskDelay(15);
      break;
    case 1:
      v767WriteMicro(id,EDGEDET_FALL_ALL);
      taskDelay(15);
      break;
    case 6:
      v767WriteMicro(id,EDGEDET_BOTH_ALL);
      taskDelay(15);
      break;
    default:
      retval = ERROR;
      break;
    }
  return (retval);
}

/*******************************************************************************
 *
 * v767GetEdgeDetectionMode
 *
 * INPUTS:    id     - module id of TDC to access
 *
 * RETURNS: -1 if not initialized, 100 if non standard, or mode as in SetEdgeDetection
 */

int
v767GetEdgeDetectionMode(int id)
{
  UINT16 t1,t2,t3;
  int retval=-1; 

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767GetedgeDetectionMode: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (retval);
    }

  v767WriteMicro(id,EDGEDET_READ);
  v767ReadMicro(id,&t1,1);
  v767ReadMicro(id,&t2,1);
  v767ReadMicro(id,&t3,1);

  printf(" Even  Channels : %d \n", (int) (t1&3) );
  printf(" Odd   Channels : %d \n", (int) (t2&3) );
  printf(" Start Channel  : %d \n", (int) (t3&3) );

  taskDelay(15);

  if (((t1&3)==1)&&((t2&3)==1)) 
    {
      retval=0;
    } 
  else if (((t1&3)==2)&&((t2&3)==2)) 
    {
      retval=1;
    } 
  else if (((t1&3)==3)&&((t2&3)==3)) 
    {
      retval=6;
    } 
  else 
    {
      retval=100;
    }

  return (retval);
}


/*******************************************************************************
 *
 * v767OverlapTrigEnable
 *
 * INPUTS:    id     - module id of TDC to access
 *
 * RETURNS: OK, or ERROR if not initialized
 */

STATUS
v767OverlapTrigEnable(int id)
{

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767OverlapTrigEnable: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (ERROR);
    }

  v767WriteMicro(id,ENABLE_OVL_TRG);
  taskDelay(15);

  return OK;
}

/*******************************************************************************
 *
 * v767OverlapTrigDisable
 *
 * INPUTS:    id     - module id of TDC to access
 *
 * RETURNS: OK, or ERROR if not initialized
 */

STATUS
v767OverlapTrigDisable(int id)
{

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767OverlapTrigDisable: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (ERROR);
    }

  v767WriteMicro(id,DISABLE_OVL_TRG);
  taskDelay(15);

  return OK;
}

/*******************************************************************************
 *
 * v767ChannelMask
 *
 * INPUTS:    id     - module id of TDC to access
 *            tdata  - array of 8 UINT16 words,
 *                     bits set to 0 turn of the corresponding TDC channel
 *
 * RETURNS: OK, or ERROR if not initialized
 */

STATUS
v767ChannelMask(int id,UINT16 *tdata)
{
  int i;

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767ChannelMask: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (ERROR);
    }

  v767WriteMicro(id,WRITE_ENABLE_PATTERN);

  for (i=0; i<8; i++) 
    {
      taskDelay(15);
      v767WriteMicro(id,tdata[i]);
    }

  taskDelay(15);

  return OK;
}


/*******************************************************************************
 *
 * v767Rates
 *
 * INPUTS:    id     - module id of TDC to access
 *
 * RETURNS: OK, or ERROR if not initialized
 * 
 * Prints a map of rates per channel
 * Caution: this routine does change the programming of the TDC
 *          the TDC needs to be reprogrammed afterwards
 */

STATUS
v767Rates(int id)
{

  int i,nwire;
  int nchip;
  int noverflows, ntries;
  int nhits[128], ntrig[128], flag[128];
  unsigned short channelmask[8];

  if((id<0) || (v767p[id] == NULL)) 
    {
      logMsg("v767Rates: ERROR : TDC id %d not initialized \n",id,0,0,0,0,0);
      return (ERROR);
    }

  printf("v767Rates \n");
  printf("========= \n\n");
  printf("Warning: the TDC will be reprogrammed, previous settings will be lost. \n");
  printf("Warning: before running CODA again, \n ");
  printf("         you have to reset and download the configuration again.\n");

  printf("\nResetting TDC\n");

  v767Reset(id); 
  for (i=0;i<8;i++) 
    {
      channelmask[i] = 0xffff;
    }

  for (i=0;i<128;i++) 
    { 
      nhits[i]=0; ntrig[i]=0; flag[i]=1; 
    }

  noverflows=0;
  ntries=0;
  taskDelay(100);
  v767SetAcqMode(id,0);
  v767SetDReadyMode(id,0); 
  v767SetEdgeDetectionMode(id,0);
  v767OverlapTrigDisable(id);
  v767TriggerConfig(id,10000,-10100,0);
  v767ChannelMask(id,channelmask); 
  v767Clear(id);

  printf("Trying standard procedure on all channels\n");

  if(v767RateLoop(id, &ntries, 100000, &noverflows, 10, nhits)!=OK) 
    return ERROR;

  for (i=0;i<128;i++) 
    {
      ntrig[i]+=ntries;
    }

  if (ntries<100000) 
    {

      for (nchip=0;nchip<4;nchip++) 
	{
	  for (i=0;i<8;i++) 
	    {
	      channelmask[i] = 0x0;
	    }
	  channelmask[2*nchip]=0xffff;
	  channelmask[2*nchip+1]=0xffff;
	  v767ChannelMask(id,channelmask); 

	  printf("Some Channels are noisy: Trying Chip %d only \n",nchip);

	  if (v767RateLoop(id, &ntries, 20000, &noverflows, 10, nhits)!=OK) 
	    return ERROR;
      
	  for (i=32*nchip;i<32*nchip+32;i++) 
	    {
	      ntrig[i]+=ntries;
	    }

	  if (ntries<20000) 
	    {
	      printf("Chip %d has noisy channels, going wire by wire \n",nchip);

	      for (nwire=32*nchip;nwire<32*nchip+32;nwire++) 
		{
		  if (nwire<32*nchip+16) 
		    {
		      channelmask[2*nchip]= 1 << (nwire % 16) ;
		      channelmask[2*nchip+1]=0x0;
		    } 
		  else 
		    {
		      channelmask[2*nchip]=0x0;
		      channelmask[2*nchip+1]= 1 << (nwire % 16) ;
		    }

		  v767ChannelMask(id,channelmask); 
		  printf("Some Channels are noisy: Trying Channel %d only \n",nwire);

		  if (v767RateLoop(id, &ntries, 2000, &noverflows, 10, nhits)!=OK) 
		    return ERROR;

		  ntrig[nwire]+=ntries;
		  if (ntries<2000) 
		    {
		      flag[nwire]=0;
		    }
		}
	    }

	}

    }

  printf("V767Rate: done\n\n");
  for (i=0;i<128;i++) 
    { 
      if (flag[i]==1) 
	{
	  printf(" Ch %3d : %8f +- %8f ;",i,(1.e5*nhits[i]/ntrig[i]),(sqrt(nhits[i])/ntrig[i]*1.e5) );
	} 
      else 
	{
	  printf(" Ch %3d : HIGHHIGH ++ HIGHHIGH ;",i);
	}
      if ((i % 2)==1) printf("\n") ;
    }


  return OK;
}


STATUS
v767RateLoop(int id, int* ntries, int maxtries, int* noverflows, int maxoverflows, int* nhits)
{
  int i,chan;
  int nDataWords;
  UINT32 databuffer[1024];

  *ntries=0;
  *noverflows=0;

  while ((*ntries<maxtries)&&(*noverflows<maxoverflows)) 
    {
      V767_EXEC_TRIGGER(id);
      i=0;
      while ((!(v767Dready(id)))&&(i<1000)) 
	{
	  i++;
	}

      if (v767Overflow(id)!=0) 
	{
	  (*noverflows)++;
	  v767Clear(id);
	} 
      else 
	{    
	  if (v767Dready(id)) 
	    {
	      (*ntries)++;
	      nDataWords = v767ReadEvent(id,databuffer,1024);

	      if ( (databuffer[0]&0x600000) != 0x400000 ) 
		{
		  printf("No Header Word: Exiting\n");
		  return ERROR;
		}

	      if ( (nDataWords<1024) && ((databuffer[nDataWords-1]&0x600000) != 0x200000 )) 
		{
		  printf("No EOB Word: Exiting\n");
		  return ERROR;
		}

	      for (i=1; i<nDataWords-1; i++) 
		{
		  if (  (databuffer[i]&0x600000)==0 ) 
		    {
		      chan = (databuffer[i]&0x7f000000)>>24  ; 
		      nhits[chan]++;
		    } 
		  else 
		    {
		      printf("Data Word Invalid: Exiting");
		      return ERROR;
		    }
		}
	    } 
	  else 
	    {
	      printf("No data: Exiting\n");
	      return ERROR;
	    }
	}
    }
  return OK;
}


#else /* dummy version*/

void
v767Lib_dummy()
{
  return;
}

#endif
