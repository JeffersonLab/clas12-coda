/*----------------------------------------------------------------------------*
 *  Copyright (c) 1991, 1992  Southeastern Universities Research Association, *
 *                            Continuous Electron Beam Accelerator Facility   *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 * CEBAF Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606 *
 *       heyes@cebaf.gov   Tel: (804) 249-7030    Fax: (804) 249-7363         *
 *----------------------------------------------------------------------------*
 * Description: follows this header.
 *
 * Author:
 *	David Abbott
 *      Bryan Moffit
 *	CEBAF Data Acquisition Group
 *
 * Revision History:
 *	  Initial revision
 *
 *        June 2011 - Bryan Moffit - Port to Linux, and Library rewrite
 *
 *----------------------------------------------------------------------------*/

/* sfi_triglib.h -- Primitive trigger control for the SFI 340 using
                    available interrupt sources including CEBAF
                    designed AUX Trigger supervisor interface card.
                    intmask -- refers to the bit pattern one wishes to
                    use to enable certain trigger sources (see the VME
                    IRQ Source and Mask Register) NIM1,NIM2,NIM3 ECL1
                    AUX_B42 etc...

   Routines:
   void sfiAuxWrite();       write 32bit word to AUX port
   void sfiAuxRead();        read 32bit word from AUX port
   void sfiTrigLink();       link in trigger isr
   void sfitenable();        enable trigger
   void sfitdisable();       disable trigger
   void sfitack();           Acknowledge/Reset trigger
   char sfittype();          read trigger type
   int  sfittest();          test for trigger (POLL)

   ------------------------------------------------------------------------------*/

#define _GNU_SOURCE

/* Define SFI Modes of operation:    Ext trigger - Interrupt mode   0
                                     TS  trigger - Interrupt mode   1
                                     Ext trigger - polling  mode    2 
                                     TS  trigger - polling  mode    3  */
#ifndef SFI_EXT_INT
#define SFI_EXT_INT    0
#endif
#ifndef SFI_TS_INT
#define SFI_TS_INT     1
#endif
#ifndef SFI_EXT_POLL
#define SFI_EXT_POLL   2
#endif
#ifndef SFI_TS_POLL
#define SFI_TS_POLL    3
#endif

#define SFI_AUX_VERSION_MASK  0x00006000 
#define SFI_AUX_VERSION_1     0x00006000
#define SFI_AUX_VERSION_2     0x00004000

#define SFI_AUX_TRIGGER_DATA_MASK 0x000000FF

/* Version 1 bits */
#define SFI1_AUX_EXT_TRIGGER_MODE   (1<<7)
#define SFI1_AUX_ENABLE_EXT_TRIGGER (1<<8)
/* Version 2 bits */
#define SFI_AUX_EXT_TRIGGER_MODE   (1<<8)
#define SFI_AUX_ENABLE_EXT_TRIGGER (1<<9)

#define SFI_AUX_SAMPLE_MODE        (1<<10)
#define SFI_AUX_RESET              (1<<15)

#define SFI_EXT_DATA_MASK          (0x3F0000)

#define SFI_AUX_ENABLE_OUTPUT_WRITE (1<<23)


#define SFI_INT_LEVEL  5
#define SFI_INT_VECTOR 0xED
#define SFI_INT_ENABLE (1<<11)


pthread_mutex_t sfiISR_mutex=PTHREAD_MUTEX_INITIALIZER;

#ifdef VXWORKS
#define INTLOCK {				\
    if(pthread_mutex_lock(&sfiISR_mutex)<0)	\
      perror("pthread_mutex_lock");		\
}

#define INTUNLOCK {				\
    if(pthread_mutex_unlock(&sfiISR_mutex)<0)	\
      perror("pthread_mutex_unlock");		\
}
#else
#define INTLOCK {				\
    vmeBusLock();				\
}
#define INTUNLOCK {				\
    vmeBusUnlock();				\
}
#endif

#define mem32(a) (*(unsigned int *) (a))

/* Global variables */
static BOOL         sfiIntRunning  = FALSE;   /* running flag */
static VOIDFUNCPTR  sfiIntRoutine  = NULL;	      /* user interrupt service routine */
static int          sfiIntArg      = 0;	              /* arg to user routine */
static unsigned int sfiIntLevel    = 0;               /* VME Interrupt Level 1-7 */
static unsigned int sfiIntVec      = SFI_INT_VECTOR;  /* default interrupt Vector */

static VOIDFUNCPTR  sfiAckRoutine  = NULL;            /* user trigger acknowledge routine */
static int          sfiAckArg      = 0;               /* arg to user trigger ack routine */

static unsigned int sfiAuxVersion  = 0; /* Default is an invalid Version ID */

unsigned int sfiIntMode;    /* SFI TI mode of operation */
unsigned int sfiIntCount;   /* Interrupt/trigger count */
unsigned int sfiSyncFlag;   /* syncFlag for current trigger */
unsigned int sfiLateFail;   /* lateFail for current trigger */
unsigned int sfiDoAck;      /* whether to perform a ACK after next trigger service */
unsigned int sfiNeedAck;    /* whether an ACK needs to be performed */

/* polling thread pthread and pthread_attr */
pthread_attr_t sfipollthread_attr;
pthread_t      sfipollthread;


/* Aux Port Read/Write prototypes */
void sfiAuxWrite(unsigned int val32);
unsigned int sfiAuxRead();
/* Aux Port Read/Write prototypes - no lock version */
static void sfiAuxWrite_nl(unsigned int val32);
static unsigned int sfiAuxRead_nl();

/* Other static method prototypes */
static void startSFIPollThread();
static void sfiInt (void);
static void sfiPoll();


/*******************************************************************************
*
* sfiTrigInit - Initialize the sfi Trigger Library and check if AUX card
*               is available.
*
*        mode - 0 (SFI_EXT_INT)  External triggers interrupts
*               1 (SFI_TS_INT)   TS triggers interrupts
*               2 (SFI_EXT_POLL) Polling for External triggers 
*               3 (SFI_EXT_POLL) Polling for TS triggers
*
* RETURNS: OK if successful, otherwise ERROR.
*
*/

int
sfiTrigInit(int mode)
{
  unsigned int rval=0;
  int version;

  sfiIntCount = 0;
  sfiDoAck = 1;
  
  /* Check if SFI has been initialized */
  if(sfi.VMESlaveAddress<0)
    {
      printf("%s: ERROR: SFI not initialized.\n",
	     __FUNCTION__);
      return ERROR;
    }

  /* Reset all interrupts */
  vmeWrite32(sfi.VmeIrqMaskReg, 0xFF00);

  
  /* Check if SFI AUX board is readable */
  rval = sfiAuxRead_nl();
  if (rval == 0xffffffff) 
    {
      printf("%s: ERROR: AUX card not addressable\n",
	     __FUNCTION__);
      return ERROR;
    } 
  else 
    {
      sfiAuxWrite(SFI_AUX_RESET); /* Reset the board */
      sfiAuxVersion = (SFI_AUX_VERSION_MASK) & rval;
    }
  
  if(sfiAuxVersion == SFI_AUX_VERSION_1)
    version = 1;
  if(sfiAuxVersion == SFI_AUX_VERSION_2)
    version = 2;

  sfiIntMode = mode;
  switch(mode)
    {
    case SFI_EXT_INT:
      printf("SFI (version %d) setup for External Interrupts\n", version);
      break;
    case SFI_TS_INT:
      printf("SFI (version %d) setup for TS Interrupts\n", version);
      break;
    case SFI_EXT_POLL:
      printf("SFI (version %d) setup for External Polling\n", version);
      break;
    case SFI_TS_POLL:
      printf("SFI (version %d) setup for TS Polling\n", version);
      break;
    default:
      printf("%s: WARN:  Invalid mode (%d).  SFI (version %d) default to external interrupts\n",
	     __FUNCTION__,mode,version);
      sfiIntMode = SFI_EXT_INT;
    }

  

  return OK;

}

/*******************************************************************************
*
* sfiTrigIntConnect - connect a user routine to the TIR interrupt or
*                     latched trigger, if polling
*
* RETURNS: OK if successful, otherwise ERROR .
*/


int
sfiTrigIntConnect(unsigned int vector, VOIDFUNCPTR routine, unsigned int arg)
{
#ifndef VXWORKS
  int status;
#endif

  sfiIntCount = 0;
  sfiDoAck    = 1;

  /* Set Vector and Level */
  SFILOCK;
  if((vector < 255)&&(vector > 64)) {
    sfiIntVec = vector;
  }else{
    sfiIntVec = SFI_INT_VECTOR;
  }

  sfiIntLevel = SFI_INT_LEVEL;

  /* Enable level 5 interrupts on SFI*/  
  vmeWrite32(sfi.VmeIrqLevelAndVectorReg, 
	  ((sfiIntLevel<<8) | sfiIntVec | SFI_INT_ENABLE) );

  printf("%s: INFO: Int Vector = 0x%x  Level = %d\n",
	 __FUNCTION__,sfiIntVec,sfiIntLevel);

#ifdef VXWORKS
  if((intDisconnect(INUM_TO_IVEC(SFI_INT_VECTOR)) !=0))
     printf("Error disconnecting Interrupt\n");
#endif

  switch (sfiIntMode) 
    {
    case SFI_TS_POLL:
    case SFI_EXT_POLL:
      /* Stuff will be done at sfiTrigEnable */
      break;
    case SFI_TS_INT:
    case SFI_EXT_INT:
#ifdef VXWORKS
      intConnect(INUM_TO_IVEC(sfiIntVec),sfiInt,arg);
#else
      status = vmeIntConnect (sfiIntVec, sfiIntLevel,sfiInt,arg);
      if (status != OK) 
	{
	  printf("vmeIntConnect failed\n");
	  SFIUNLOCK;
	  return(ERROR);
	}
#endif  
      
      break;
    default:
      printf("%s: ERROR: SFI Mode not defined %d\n",
	     __FUNCTION__,sfiIntMode);
      SFIUNLOCK;
      return(ERROR);
    }
  
  if(routine) 
    {
      sfiIntRoutine = routine;
      sfiIntArg = arg;
    }
  else
    {
      sfiIntRoutine = NULL;
      sfiIntArg = 0;
    }

  SFIUNLOCK;

  return OK;

}


void 
sfiTrigIntDisconnect()
{
#ifndef VXWORKS
  int status;
#endif
  void *res;


  if(sfiIntRunning) {
    printf("%s: ERROR: SFI TI is Enabled - Call sfiTrigDisable() first\n",
	   __FUNCTION__);
    return;
  }

  INTLOCK;

  switch (sfiIntMode) 
    {
    case SFI_TS_INT:
    case SFI_EXT_INT:
      
#ifdef VXWORKS
      /* Disconnect any current interrupts */
      sysIntDisable(sfiIntLevel);
      if((intDisconnect(sfiIntVec) !=0))
	printf("%s: Error disconnecting Interrupt\n",__FUNCTION__);
#else
      status = vmeIntDisconnect(sfiIntLevel);
      if (status != OK) {
	printf("vmeIntDisconnect failed\n");
      }
#endif
      break;
    case SFI_TS_POLL:
    case SFI_EXT_POLL:
      if(sfipollthread) 
	{
	  if(pthread_cancel(sfipollthread)<0) 
	    perror("pthread_cancel");
	  if(pthread_join(sfipollthread,&res)<0)
	    perror("pthread_join");
	  if (res == PTHREAD_CANCELED)
	    printf("%s: Polling thread canceled\n",__FUNCTION__);
	  else
	    printf("%s: ERROR: Polling thread NOT canceled\n",__FUNCTION__);
	}
      break;

    default:
      break;
    }

  /* Reset SFI TI */
  sfiAuxWrite(SFI_AUX_RESET);
  INTUNLOCK;

  printf("%s: Disconnected\n",__FUNCTION__);

  return;
}

void 
sfiTrigEnable(int iflag)
{
#ifdef VXWORKS
  int lock_key;
#endif
  int intMask = 0x10; /* Enable SFI interrupts from AUX port */
  int sfiWrite=0;

  SFILOCK;
  if(iflag == 1)
    sfiIntCount = 0;

  sfiIntRunning = 1;
  sfiDoAck      = 1;
  sfiNeedAck    = 0;

  switch (sfiIntMode)
    {
    case SFI_TS_POLL:
      {
	startSFIPollThread();
	vmeWrite32(sfi.VmeIrqLevelAndVectorReg, 
		((SFI_INT_LEVEL)<<8 | SFI_INT_VECTOR));     /* Enable Internal Interrupts */
	vmeWrite32(sfi.VmeIrqMaskReg, (intMask<<8));    /* Reset any Pending Trigger */
	vmeWrite32(sfi.VmeIrqMaskReg, intMask);         /* Enable source */

	sfiAuxWrite_nl(0); /*Set AUX Port for Trigger Supervisor triggers*/
      }
    case SFI_EXT_POLL:
      {
	startSFIPollThread();
	vmeWrite32(sfi.VmeIrqLevelAndVectorReg, 
		((SFI_INT_LEVEL)<<8 | SFI_INT_VECTOR));     /* Enable Internal Interrupts */
	vmeWrite32(sfi.VmeIrqMaskReg, (intMask<<8));    /* Reset any Pending Trigger */
	vmeWrite32(sfi.VmeIrqMaskReg, intMask);         /* Enable source */

	/*Set AUX Port for External triggers*/
	if(sfiAuxVersion == SFI_AUX_VERSION_2)
	  sfiWrite = (SFI_AUX_EXT_TRIGGER_MODE | SFI_AUX_ENABLE_EXT_TRIGGER);
	else
	  sfiWrite = (SFI1_AUX_EXT_TRIGGER_MODE | SFI1_AUX_ENABLE_EXT_TRIGGER);

	sfiAuxWrite_nl(sfiWrite);

	break;
      }
    case SFI_TS_INT:
      {
#ifdef VXWORKS
	lock_key = intLock();
#endif
	vmeWrite32(sfi.VmeIrqMaskReg, (intMask<<8));    /* Reset any Pending Trigger */
	vmeWrite32(sfi.VmeIrqMaskReg, intMask);         /* Enable source */

#ifdef VXWORKSPPC
	sysIntEnable(SFI_INT_LEVEL);                    /* Enable VME Level 5 interrupts */
	intEnable(11);       /* Enable VME Interrupts on IBC chip */
#endif
	sfiAuxWrite_nl(0);      /*Set AUX Port for Trigger Supervisor triggers*/


#ifdef VXWORKS
	intUnlock(lock_key);
#endif
	break;
      }
    case SFI_EXT_INT:
      {
#ifdef VXWORKS
	lock_key = intLock();
#endif
	vmeWrite32(sfi.VmeIrqMaskReg, (intMask<<8));    /* Reset any Pending Trigger */
	vmeWrite32(sfi.VmeIrqMaskReg, intMask);         /* Enable source */

#ifdef VXWORKSPPC
	sysIntEnable(sfiIntLevel);                    /* Enable VME Level 5 interrupts */
	intEnable(11);       /* Enable VME Interrupts on IBC chip */
#endif
	if(sfiAuxVersion == SFI_AUX_VERSION_2)
	  sfiWrite = (SFI_AUX_EXT_TRIGGER_MODE | SFI_AUX_ENABLE_EXT_TRIGGER);
	else
	  sfiWrite = (SFI1_AUX_EXT_TRIGGER_MODE | SFI1_AUX_ENABLE_EXT_TRIGGER);
      
	sfiAuxWrite_nl(sfiWrite);

#ifdef VXWORKS
	intUnlock(lock_key);
#endif
	break;
      }
    }
  SFIUNLOCK;

}

void 
sfiTrigDisable()
{
  unsigned int intMask = 0x10;

  sfiDoAck=0;

  SFILOCK;
  vmeWrite32(sfi.VmeIrqMaskReg, (intMask<<8));      /* Clear Source  */

#ifdef VXWORKS
  if( (sfiIntMode == SFI_TS_INT) || (sfiIntMode == SFI_EXT_INT) )
    sysIntDisable(SFI_INT_LEVEL);        /* Disable VME Level 5 interrupts */
#endif

  /*Reset AUX Port */
  sfiAuxWrite_nl(SFI_AUX_RESET);

  sfiIntRunning = 0;

  SFIUNLOCK;
}

int
sfiAckConnect(VOIDFUNCPTR routine, unsigned int arg)
{
  if(routine)
    {
      sfiAckRoutine = routine;
      sfiAckArg = arg;
    }
  else
    {
      printf("%s: WARN: routine undefined.\n",__FUNCTION__);
      sfiAckRoutine = NULL;
      sfiAckArg = 0;
      return ERROR;
    }
  return OK;
}


void 
sfiTrigAck()
{
  unsigned int intMask = 0x10;

  if (sfiAckRoutine != NULL)
    {
      /* Execute user defined Acknowlege, if it was defined */
      SFILOCK;
      (*sfiAckRoutine) (sfiAckArg);
      SFIUNLOCK;
    }
  else
    {
      SFILOCK;
      sfiNeedAck = 0;
      sfiDoAck   = 1;

      switch(sfiIntMode)
	{
	case SFI_TS_POLL:
	case SFI_EXT_POLL:
	  vmeWrite32(sfi.VmeIrqMaskReg, intMask<<8);          /* Clear Source  */
	  break;

	case SFI_TS_INT:
	case SFI_EXT_INT:
	  break;
	}
      vmeWrite32(sfi.writeVmeOutSignalReg, 0x1000);       /* Set A10 Ack       */
      vmeWrite32(sfi.VmeIrqMaskReg, intMask);             /* Enable Source  */
      vmeWrite32(sfi.writeVmeOutSignalReg, 0x10000000);   /* Clear A10 Ack       */

      SFIUNLOCK;
    }


}

unsigned int
sfiTrigType()
{
  unsigned int tt;
  unsigned int reg;

  /* Read Trigger type from TS AUX port */
  SFILOCK;
  reg = sfiAuxRead_nl();                               /* read data from AUX port */
  SFIUNLOCK;
  if(reg == 0xffffffff)
    logMsg("%s: ERROR: Error reading Aux port\n",__FUNCTION__,0,0,0,0,0);

  if ( (sfiIntMode == SFI_TS_INT) || (sfiIntMode == SFI_TS_POLL) )
    {
      if(sfiAuxVersion == SFI_AUX_VERSION_2)
	tt       = ((reg&0xfc)>>2);
      else
	tt       = ((reg&0x3c)>>2);
      
      sfiLateFail = ((reg&0x02)>>1);
      sfiSyncFlag = ((reg&0x01));
    }
  else  /* sfiIntMode == SFI_EXT_INT || sfiIntMode == SFI_EXT_POLL */
    {
      tt=reg&0x3f;
    }

  return(tt);
}

int
sfiTrigData(unsigned int *itype, unsigned int *isyncFlag, unsigned int *ilateFail)
{
  unsigned int reg;

  /* Read Trigger type from TS AUX port */
  SFILOCK;
  reg = sfiAuxRead_nl();                               /* read data from AUX port */
  SFIUNLOCK;
  if(reg == 0xffffffff)
    {
      logMsg("%s: ERROR: Error reading Aux port\n",__FUNCTION__,0,0,0,0,0);
      return ERROR;
    }

  if ( (sfiIntMode == SFI_TS_INT) || (sfiIntMode == SFI_TS_POLL) )
    {
      if(sfiAuxVersion == SFI_AUX_VERSION_2)
	*itype       = ((reg&0xfc)>>2);
      else
	*itype       = ((reg&0x3c)>>2);

      sfiLateFail = ((reg&0x02)>>1);
      sfiSyncFlag = ((reg&0x01));

      *ilateFail   = ((reg&0x02)>>1);
      *isyncFlag   = ((reg&0x01));
    }
  else
    {
      *itype=reg&0x3f;
      *ilateFail = 0;
      *isyncFlag = 0;
    }
  return OK;
}

int
sfiTrigDecodeData(unsigned int idata, unsigned int *itype,
		  unsigned int *isyncFlag, unsigned int *ilateFail)
{

  if ( (sfiIntMode == SFI_TS_INT) || (sfiIntMode == SFI_TS_POLL) )
    {
      if(sfiAuxVersion == SFI_AUX_VERSION_2)
	*itype       = ((idata&0xfc)>>2);
      else
	*itype       = ((idata&0x3c)>>2);

      sfiLateFail = ((idata&0x02)>>1);
      sfiSyncFlag = ((idata&0x01));

      *ilateFail   = ((idata&0x02)>>1);
      *isyncFlag   = ((idata&0x01));
    }
  else
    {

      *itype=idata&0x3f;
      *ilateFail = 0;
      *isyncFlag = 0;

    }
  return OK;
}

int 
sfiTrigTest()
{
  int ii=0;
  /* NOTE: See that internal VME IRQ is set on the SFI*/
#ifdef VXWORKSPPC
  cacheInvalidate(1,sfi.VmeIrqLevelAndVectorReg,4);
#endif
  SFILOCK;
/*   ii = (((vmeRead32(sfi.readSeqFifoFlags)) & (1<<15)) == 0); */
  ii = (((vmeRead32(sfi.VmeIrqLevelAndVectorReg)) & 0x4000) != 0);

  if(ii)
    sfiIntCount++;

  SFIUNLOCK;
  return(ii);
}

void
sfiTrigClearIntCount()
{
  sfiIntCount=0;

}

unsigned int
sfiTrigGetIntCount()
{
  return sfiIntCount;
}

static void
startSFIPollThread()
{
  int psfi_status;							
  
  psfi_status = pthread_create(&sfipollthread,
			       NULL,					
			       (void*(*)(void *)) sfiPoll,		
			       (void *)NULL);				
  if(psfi_status!=0) {						
    printf("Error: SFI Polling Thread could not be started.\n");	
    printf("\t ... returned: %d\n",psfi_status);			
  }									
}

/*******************************************************************************
*
* sfiInt - default interrupt handler
*
* This rountine handles the SFI interrupt.  A user routine is
* called, if one was connected by sfiTrigIntConnect().
*
* RETURNS: N/A
*
* SEE ALSO: sfiTrigIntConnect()
*/

static void 
sfiInt(void)
{
  int intMask=0x10;
  
  INTLOCK;

  vmeWrite32(sfi.VmeIrqMaskReg, intMask<<8);          /* Clear Source  */

  sfiIntCount++;

  if (sfiIntRoutine != NULL)	/* call user routine */
    (*sfiIntRoutine) (sfiIntArg);

  /* Acknowledge trigger */
  if(sfiDoAck==1) 
    {
      sfiTrigAck();
    }

  INTUNLOCK;
}


/*******************************************************************************
*
* sfiPoll - Polling Service Thread
*
* This thread handles the polling of the SFI interrupt.  A user routine is
* called, if one was connected by sfiTrigIntConnect().
*
* RETURNS: N/A
*
* SEE ALSO: sfiTrigIntConnect()
*/

static void
sfiPoll()
{
  int intMask=0x10;
  int sfidata;
  int policy=0;
  struct sched_param sp;

  policy=SCHED_FIFO;
  sp.sched_priority=40;
  printf("%s: Entering polling loop...\n",__FUNCTION__);
  pthread_setschedparam(pthread_self(),policy,&sp);
  pthread_getschedparam(pthread_self(),&policy,&sp);
  printf ("%s: INFO: Running at %s/%d\n",
	  __FUNCTION__,
	  (policy == SCHED_FIFO ? "FIFO"
	   : (policy == SCHED_RR ? "RR"
	      : (policy == SCHED_OTHER ? "OTHER"
		 : "unknown"))), sp.sched_priority);  

  while(1) 
    {

      /* If still need Ack, don't test the Trigger Status */
      if(sfiNeedAck) continue;

      sfidata = 0;
	  
      sfidata = sfiTrigTest();
      if(sfidata == ERROR) 
	{
	  printf("%s: ERROR: sfiTrigIntPoll returned ERROR.\n",__FUNCTION__);
	  break;
	}


      if(sfidata) 
	{

	  INTLOCK;

	  vmeWrite32(sfi.VmeIrqMaskReg, intMask<<8);          /* Clear Source  */

	  if (sfiIntRoutine != NULL)	/* call user routine */
	    (*sfiIntRoutine) (sfiIntArg);

	  /* Write to SFI to Acknowledge Interrupt */
	  if(sfiDoAck==1) 
	    {
	      sfiTrigAck();
	    }
	  else
	    {
	      printf("sfiDoAck=0... sfIntCount = %d\n",sfiIntCount);
	    }
	  INTUNLOCK;
	}

      pthread_testcancel();

  }
  printf("%s: Read Error: Exiting Thread\n",__FUNCTION__);
  pthread_exit(0);
}


int
sfiTrigStatus()
{
  unsigned int reg, sfireg;
  int data, mode, extTrig, sampleMode, versionID, extData;
  int intLevel, intVec, intEnabled;
  int version, latched;
  
  SFILOCK;
  reg     = sfiAuxRead_nl();
  latched = (((vmeRead32(sfi.VmeIrqLevelAndVectorReg)) & 0x4000) != 0);
  sfireg  = vmeRead32(sfi.VmeIrqLevelAndVectorReg); 
  SFIUNLOCK;

  versionID = (reg & SFI_AUX_VERSION_MASK);
  if(versionID == SFI_AUX_VERSION_1)
    version = 1;
  if(versionID == SFI_AUX_VERSION_2)
    version = 2;

  data = reg & 0xF;
  if(version == 1)
    {
      mode    = reg & SFI1_AUX_EXT_TRIGGER_MODE;
      extTrig = reg & SFI1_AUX_ENABLE_EXT_TRIGGER;
    }
  else if (version == 2)
    {
      mode    = reg & SFI_AUX_EXT_TRIGGER_MODE;
      extTrig = reg & SFI_AUX_ENABLE_EXT_TRIGGER;
    }

  sampleMode = reg & SFI_AUX_SAMPLE_MODE;

  extData = (reg & SFI_EXT_DATA_MASK)>>16;

  intLevel   = (sfireg & 0x700)>>8;
  intVec     = sfireg & 0xFF;
  intEnabled = sfireg & SFI_INT_ENABLE;

  printf("STATUS for SFI TI \n");
  printf("----------------------------------------- \n");
  printf("Trigger Count: %d\n",sfiIntCount);
  printf("Register: 0x%08x\n",reg);
  printf("VME Interrupt Level = %d  Vector = 0x%x\n",intLevel,intVec);
  printf("  version = %d\n",version);
  printf("  data    = 0x%x\n",data);
  if(mode==0)
    {
      printf("Source     : TS\n");
      if (sampleMode)
	{
	  printf("sampleMode : Enabled\n");
	  printf("           : External Data = 0x%x\n",extData);
	}
    }
  else
    {
      printf("Source     : External\n");
      if(extTrig)
	{
	  printf("State      : Enabled");
	  if (latched)
	    {
	      printf(" (trigger latched!)");
	    }
	  printf("\n");
	}
      else
	printf("State      : Disabled\n");
    }
  if(intEnabled)
    printf("Interrupts : Enabled\n");
  
  return OK;
}

/* Functions for Asyncronous Operations */
int
sfiSeqStatus()
{
  unsigned int reg32=0;
  int res;

  SFILOCK;
  reg32 = vmeRead32(sfi.sequencerStatusReg);
  SFIUNLOCK;

  switch (reg32&0x8001) 
    {
    case 0x00000000: /* Disabled */
      res = 0;
      break;
    case 0x00000001: /* Busy */
      res = 1;
      break;
    case 0x00008000: /* Error */
      res = -1;
      break;
    case 0x00008001: /* OK - Done */
      res = 2;
    }

  return(res);
}

unsigned int
sfiReadSeqFifo()
{
  register unsigned int reg32;
  volatile unsigned int fval;

  SFILOCK;
  reg32 = vmeRead32(sfi.readSeqFifoFlags);

  if((reg32 & 0x00000010) == 0x00000010)  /* Empty */
    {
      fval = vmeRead32(sfi.readSeq2VmeFifoBase); /* Dummy read */
      reg32 = vmeRead32(sfi.readSeqFifoFlags);
      if((reg32&0x00000010) == 0x00000010) 
	{
	  /* STILL EMPTY - Error !!! */
	  SFIUNLOCK;
	  return(0);
	}
    }
  
  /* read fifo */
  fval = vmeRead32(sfi.readSeq2VmeFifoBase);
  SFIUNLOCK;

  return(fval);
}

int 
sfiSeqDone()
{
  register unsigned int reg32 = 0;
  register int Return = 0;
  register int Exit   = 0;


/* wait for sequencer done */
  do 
    {
      SFILOCK;
      reg32 = vmeRead32(sfi.sequencerStatusReg);
      SFIUNLOCK;

      switch(reg32 & 0x00008001) 
	{
	case 0x00008001:
	  /* OK */
	  Return = 0;
	  Exit = 1;
	  break;
	case 0x00000001:
	  /* Not Finished */
	  break;
	case 0x00000000:
	  /* Not Initialized */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00020000;
	  Exit = 1;
	  break;
	case 0x00008000:
	  /* Bad Status is set, we will see */
	  Return = vmeRead32(sfi.sequencerStatusReg) & 0x0000ffff;
	  Return |= 0x00010000;
	  Exit = 1;
	  break;
	}
    } while(!Exit);

  return(Return);
}

void
sfiRamStart(unsigned int ramAddr, unsigned int *data)
{
  unsigned int *dmaptr;

  dmaptr = (unsigned int *)vmeDmaLocalToVmeAdrs((unsigned int)data);

  SFILOCK;
/*   vmeWrite32(fastLoadDmaAddressPointer, ((unsigned int) (data) + sfiCpuMemOffset) ); */
  vmeWrite32(fastLoadDmaAddressPointer, (unsigned int) dmaptr);

  vmeWrite32((fastEnableRamSequencer + (ramAddr>>2)), 1);       /* start Sequencer at RAM Address */
  SFIUNLOCK;

}

void
sfiEnableSequencer()
{
  SFILOCK;
  vmeWrite32(sfi.sequencerEnable, 0);
  SFIUNLOCK;
}

void
sfiAuxWrite(unsigned int val32)
{
  SFILOCK;
  sfiAuxWrite_nl(val32);
  SFIUNLOCK;
}

static void
sfiAuxWrite_nl(unsigned int val32)
{
  vmeWrite32(sfi.sequencerDisable, 0);
  vmeWrite32(sfi.writeAuxReg, val32);
  vmeWrite32(sfi.writeVmeOutSignalReg, 0x4000);
  vmeWrite32(sfi.generateAuxB40Pulse, 0);
  vmeWrite32(sfi.writeVmeOutSignalReg, 0x40000000);
}

unsigned int
sfiAuxRead()
{
  unsigned int rval=0;
  SFILOCK;
  rval = sfiAuxRead_nl();
  SFIUNLOCK;
  
  return rval;
}

static unsigned int
sfiAuxRead_nl()
{
  int xx=0;
  unsigned int val32 = 0xffffffff;

/*   vmeWrite32(sfi.sequencerDisable, 1); */
  vmeWrite32(sfi.sequencerDisable, 0);
  vmeWrite32(sfi.writeVmeOutSignalReg, 0x2000);
  while ( (xx<10) && (val32 = 0xffffffff) )
    {
      val32 = vmeRead32(sfi.readLocalFbAdBus);
      xx++;
    }
  vmeWrite32(sfi.writeVmeOutSignalReg, 0x20000000);

  return (val32);
}

