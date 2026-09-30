/*----------------------------------------------------------------------------*
 *  Copyright (c) 2010        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Bryan Moffit                                                   *
 *             moffit@jlab.org                   Jefferson Lab, MS-12B3       *
 *             Phone: (757) 269-5660             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *     A front for the stuff that actually does the work.
 *      APIs are switched from the Makefile
 *
 *----------------------------------------------------------------------------*/

#ifdef ARCH_armv71

#include "jvme.h"

#else


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include "jvme.h"
#include "jvmeWindows.h"
#include "jvmeControl.h"



/* global variables */
int32_t vmeQuietFlag = 0;
int32_t vmeDebugMode=0;
FILE *fDebugMode=NULL;
pid_t processID;

/* VXS Payload Port to VME Slot map */
#define MAX_VME_SLOTS 21    /* This is either 20 or 21 */
static int32_t maxVmeSlots=MAX_VME_SLOTS;
uint16_t PayloadPort21[MAX_VME_SLOTS+1] =
  {
    0,     /* Filler for mythical VME slot 0 */
    0,     /* VME Controller */
    17, 15, 13, 11, 9, 7, 5, 3, 1,
    0,     /* Switch Slot A - SD */
    0,     /* Switch Slot B - CTP/GTP */
    2, 4, 6, 8, 10, 12, 14, 16,
    18     /* VME Slot Furthest to the Right - TI */
  };

uint16_t PayloadPort20[MAX_VME_SLOTS+1] =
  {
    0,     /* Filler for mythical VME slot 0 */
    17, 15, 13, 11, 9, 7, 5, 3, 1,
    0,     /* Switch Slot A - SD */
    0,     /* Switch Slot B - CTP/GTP */
    2, 4, 6, 8, 10, 12, 14, 16,
    18,     /* VME Slot Furthest to the Right - TI */
    0
  };

/*!
  Routine to delay a task from executing

  @see nanosleep(3)

  @param ticks number of ticks to delay task (units: 1 tick = 16.7 ms)

  @return 0 if successful, -1 on error
*/

int32_t
taskDelay(int32_t ticks)
{
  int32_t nsec = 0, rval = OK;
  struct timespec waittime, rem;

  if(ticks <= 0)
    return 0;

  nsec = ticks * 16700000;

  if(nsec >= 1E9)
    {
      waittime.tv_sec = (int)(nsec / (1E9));
      waittime.tv_nsec = nsec - ((long)waittime.tv_sec * (1E9));
    }
  else
    {
      waittime.tv_sec = 0;
      waittime.tv_nsec = nsec;
    }

  rval = nanosleep(&waittime, &rem);

  if(rval != OK)
    {
      printf("%s: ERROR: Interrupted. Time Remaining: %ds %dns\n",
	     __func__, (int)rem.tv_sec, (int)rem.tv_nsec);
    }

  return rval;
}

/*!
  Routine to print a formatted message.  Same usage as printf(...)

  @see printf(3)

  @param format Formatted message
  @param ... additional parameters as needed by message
*/
int32_t
logMsg(const char *format, ...)
{
  va_list args;
  int32_t retval;

  va_start(args,format);
  retval = vprintf(format, args);
  va_end(args);

  return retval;
}

/*!
  Routine to return system scaler

  @returns system clock ticks since reset (or rollover)
*/
unsigned long long int rdtsc(void)
{
  /*    unsigned long long int x; */
  unsigned a, d;

  __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));

  return ((unsigned long long)a) | (((unsigned long long)d) << 32);
}


/*!
  Routine to enable (default) or disable verbose messages in VME API

  @param pflag
  - 0 to disable
  - 1 to enable
*/
void
vmeSetQuietFlag(uint32_t pflag)
{
  if(pflag <= 1)
    vmeQuietFlag = pflag;
  else
    printf("%s: ERROR: invalid argument pflag=%d\n",
	   __func__,pflag);
}

/*!
  Routine to enable (default) or disable VME Debug Mode

  @param pflag
  - >=1 to enable
  - Otherwise to disable
*/
void
vmeSetVMEDebugMode(int32_t enable)
{
  if(enable>=1)
    {
      vmeDebugMode=1;
      printf("%s: Enabled\n",__func__);
    }
  else
    {
      vmeDebugMode=0;
      printf("%s: Disabled\n",__func__);
    }

  if(fDebugMode==NULL)
    fDebugMode = stdout;
}

int32_t
vmeSetVMEDebugModeOutputFilename(char *fOutput)
{
  fDebugMode = fopen(fOutput,"w");
  if(fDebugMode==NULL)
    {
      perror("vmeSetVMEDebugModeOutput");
      return ERROR;
    }

  printf("%s: Output set to %s\n",__func__,fOutput);

  return OK;
}

int32_t
vmeSetVMEDebugModeOutput(int32_t *fOutput)
{
  fDebugMode = (FILE *)fOutput;

  if(fOutput == (int32_t *)stdout)
    printf("%s: Output set to STDOUT\n",__func__);
  else if(fOutput == (int32_t *)stderr)
    printf("%s: Output set to STDERR\n",__func__);
  else
    printf("%s: Output set to FILE\n",__func__);

  return OK;
}

int32_t
vmeSetA32BltWindowWidth(uint32_t size)
{
  printf("%s: ERROR: Not supported in this library\n",
	 __func__);
  return ERROR;
}

/*!
  Routine to initialize the VME API
  - opens default VME Windows and maps them into Userspace
  - maps VME Bridge Registers into Userspace
  - disables interrupts on VME Bus Errors
  - creates a shared mutex for interrupt/trigger locking
  - calls vmeBusCreateLockShm()

  @return 0, if successful.  An API dependent error code, otherwise.
*/

int32_t
vmeOpen()
{
  int32_t status = OK;

  status = jvmeOpenDefaultWindows();
  status = jvmeBridgeInit();
  vmeBusCreateLockShm();

  return status;
}

int32_t vmeOpenDefaultWindows() {return vmeOpen();}

/*!
  Routine to cleanup what was initialized by vmeOpenDefaultWindows()

  @return 0, if successful.  An API dependent error code, otherwise.
*/
int32_t
vmeClose()
{
  int32_t status = OK;

  status = jvmeBridgeClose();
  status = jvmeCloseDefaultWindows();

  if(fDebugMode!=NULL)
    fclose(fDebugMode);

  return status;
}

int32_t vmeCloseDefaultWindows() {return vmeClose();}

/*!
  Routine to open a Slave window to the VME A32 space and map it into Userspace

  @param base The base address of the A32 window
  @param size The size of the A32 window

  @return 0 if successful, otherwise.. an API dependent error code

*/
int32_t
vmeOpenSlaveA32(uint32_t base, uint32_t size)
{
  uint32_t rval = OK;

  printf("%s: ERROR: Not supported in this library\n",
	 __func__);
  rval = ERROR;

  return rval;
}

/*!
  Routine to close and unmap the slave window opened with vmeOpenSlaveA32()

  @return 0 if successful, otherwise.. an API dependent error code
*/
int32_t
vmeCloseA32Slave()
{
  uint32_t rval = OK;

  printf("%s: ERROR: Not supported in this library\n",
	 __func__);
  rval = ERROR;

  return rval;
}

/*!
  Routine to read from a register from the VME Bridge Chip

  @param offset Register offset (from VME Bridge base register) from which to read

  @return 32bit word read from register, if successful.  -1, otherwise.
*/
uint32_t
vmeReadRegister(uint32_t offset)
{
  uint32_t rval = OK;

  printf("%s: ERROR: Not supported in this library\n",
	 __func__);
  rval = ERROR;

  return rval;
}

/*!
  Routine to write to a register from the VME Bridge Chip

  @param offset Register offset (from VME Bridge base register) from which to read
  @param buffer 32bit word to write to requested register offset

  @return 0, if successful.  -1, otherwise.
*/
int32_t
vmeWriteRegister(uint32_t offset, uint32_t buffer)
{
  int32_t status = OK;

  printf("%s: ERROR: Not supported in this library\n",
	 __func__);
  status = ERROR;

  return status;
}

/*!
  Routine to assert SYSRESET on the VME Bus

  @return 0 if successful, otherwise.. an API dependent error code
*/
int32_t
vmeSysReset()
{
  int32_t status = OK;

  status = jvmeSysReset();

  return status;
}

/*!
  Routine to query the status of interrupts on VME Bus Error

  @return 1 if enabled, 0 if disabled, -1 if error.
*/
int32_t
vmeBERRIrqStatus()
{
  int32_t status = OK;

  status = jvmeBERRIrqStatus();

  return status;
}

/*!
  Routine to disable interrupts on VME Bus Error

  @return 0 if successful, otherwise.. an API dependent error code
*/
int32_t
vmeDisableBERRIrq(int32_t pflag)
{
  int32_t status = OK;

  status = jvmeSetBERRIrq(0, pflag);

  return status;
}

/*!
  Routine to enable interrupts on VME Bus Error

  @return 0 if successful, otherwise.. an API dependent error code
*/
int32_t
vmeEnableBERRIrq(int32_t pflag)
{
  int32_t status = OK;

  status = jvmeSetBERRIrq(1, pflag);

  return status;
}

/*!
  Routine to probe a VME Address for a VME Bus Error

  @param *addr address to be probed
  @param size  size (1, 2, or 4) of address to read (in bytes)
  @param *rval where to return value

  @return 0, if successful. -1, otherwise.
*/
int32_t
vmeMemProbe(char *addr, int32_t size, char *rval)
{
  int32_t status = OK;
  status = jvmeMemProbe(addr, size, rval);

  return status;
}

/*!
  Routine to clear any VME Exception that is currently flagged on the VME Bridge Chip

  @param pflag
  - 1 to turn on verbosity
  - 0 to disable verbosity.
*/
int32_t
vmeClearException(int32_t pflag)
{
  int32_t status = OK;

  status = jvmeClearException(pflag);

  return status;
}

/*!
  Routine to connect a routine to a VME Bus Interrupt

  @param vector  interrupt vector to attach to
  @param level   VME Bus interrupt level
  @param routine routine to be called
  @param arg     argument to be passed to the routine

  @return 0, if successful. -1, otherwise.
*/
int32_t
vmeIntConnect(uint32_t vector, uint32_t level, VOIDFUNCPTR routine, uint32_t arg)
{
  int32_t status = OK;

#ifdef INTSUPPORT
  status = (int)jvmeIntConnect(vector, level, routine, arg);
#else
  printf("%s: ERROR: Not supported in this library\n",
	 __func__);
  status = ERROR;
#endif

  return status;
}

/*!
  Routine to release the routine attached with vmeIntConnect()

  @param level
    - VME Bus Interrupt level (Linux)

  @return 0, if successful. -1, otherwise.
*/
int32_t
vmeIntDisconnect(uint32_t level)
{
  int32_t status = OK;

#ifdef INTSUPPORT
  status = jvmeIntDisconnect(level);
#else
  printf("%s: ERROR: Not supported in this library\n",
	 __func__);
  status = ERROR;
#endif

  return status;

}

/*!
  Routine to convert a a VME Bus address to a Userspace Address

  @param vmeAdrsSpace Bus address space in whihc vmeBusAdrs resides
  @param *vmeBusAdrs  Bus address to convert
  @param **pLocalAdrs   Where to return Userspace address

  @return 0, if successful. -1, otherwise.
*/
int32_t
vmeBusToLocalAdrs(int32_t vmeAdrsSpace, char *vmeBusAdrs, char **pLocalAdrs)
{
  int32_t status = OK;

  status = (int)jvmeBusToLocalAdrs(vmeAdrsSpace, vmeBusAdrs, pLocalAdrs);

  return status;

}

/*!
  Routine to convert a Userspace Address to a VME Bus address

  @param localAdrs  Local (userspace) address to convert
  @param *vmeAdrs   Where to return VME address
  @param *amCode    Where to return address modifier

  @return 0, if successful. -1, otherwise.
*/
int32_t
vmeLocalToVmeAdrs(u_long localAdrs, uint32_t *vmeAdrs, uint16_t *amCode)
{
  int32_t status = OK;

  status = (int)jvmeLocalToVmeAdrs(localAdrs,vmeAdrs,amCode);

  return status;
}
/*!
  Routine to enable/disable debug flags set in the VME Bridge Kernel Driver

  @param flags API dependent flags to toggle specific debug levels and messages

  @return 0, if successful. -1, otherwise.
*/
int32_t
vmeSetDebugFlags(int32_t flags)
{
  int32_t status=OK;

  printf("%s: ERROR: Not supported in this library\n",
	 __func__);
  status = ERROR;

  return status;
}



/*!
  Routine to change the address modifier of the A24 Outbound VME Window.
  The A24 Window must be opened, prior to calling this routine.

  @param addr_mod Address modifier to be used.  If 0, the default (0x39) will be used.

  @return 0, if successful. -1, otherwise
*/

int32_t
vmeSetA24AM(int32_t addr_mod)
{
  int32_t status = OK;

  status = jvmeSetA24AM(addr_mod);

  return status;
}

/* DMA SUBROUTINES */

/*!
  Routine to initialize the Tempe DMA Interface

  @param addrType
  Address Type of the data source
  - 0: A16
  - 1: A24
  - 2: A32
  @param dataType
  Data type of the data source
  - 0: D16
  - 1: D32
  - 2: BLT
  - 3: MBLT
  - 4: 2eVME
  - 5: 2eSST
  @param sstMode
  2eSST transfer rate.  If 2eSST is set for dataType (otherwise, ignored):
  - 0: 160 MB/s
  - 1: 267 MB/s
  - 2: 320 MB/s

  @return 0, if successful. -1, otherwise.
*/
int32_t
vmeDmaConfig(uint32_t addrType, uint32_t dataType, uint32_t sstMode)
{
  int32_t retVal = OK;

  retVal = jvmeDmaConfig(addrType,dataType,sstMode);

  return retVal;
}

/*!
  Routine to initiate a DMA

  @param locAdrs Destination Userspace address
  @param vmeAdrs VME Bus source address
  @param size    Maximum size of the DMA in bytes

  @return 0, if successful. -1, otherwise.
*/
int32_t
vmeDmaSend(u_long locAdrs, uint32_t vmeAdrs, int32_t size)
{
  int32_t status = OK;

  status = jvmeDmaSend(locAdrs,vmeAdrs,size);

  return status;

}

/*!
  Routine to initiate a DMA using physical memory address

  @param physAdrs Destination Physical Memory address
  @param vmeAdrs VME Bus source address
  @param size    Maximum size of the DMA in bytes

  @return 0, if successful. -1, otherwise.
*/
int32_t
vmeDmaSendPhys(u_long physAdrs, uint32_t vmeAdrs, int32_t size)
{
  int32_t status = OK;

  status = jvmeDmaSendPhys(physAdrs, vmeAdrs, size);

  return status;

}

/*!
  Routine to return the Bus Error status from most recent DMA

  @return 1 if Bus Error ended DMA or 0 if not, if successful. Otherwise ERROR.
*/
int32_t
vmeDmaBerrStatus()
{
  int32_t retVal = OK;

  retVal = jvmeDmaBerrStatus();

  return retVal;
}

/*!
  Routine to poll for a DMA Completion or timeout.

  @return Number of bytes transferred, if successful, -1, otherwise.
*/
int32_t
vmeDmaDone()
{
  int32_t retVal = OK;

  retVal = jvmeDmaDone();

  return retVal;
}

/*!
  Routine to DMA rest of transaction to trash can
  @param vmeAddr VME address to continue DMA
  @returns number of bytes transferred.
*/
int32_t
vmeDmaFlush(uint32_t vmeaddr)
{
  int32_t retVal;

  logMsg("vmeDmaFlush: ERROR: Routine not supported.\n",
	 1, 2, 3, 4, 5, 6);
  retVal = ERROR;

  return retVal;
}

/*!
  Routine to allocate memory for a linked list buffer.

  @return 0 if successful, otherwise.. an API dependent error code
*/
int32_t
vmeDmaAllocLLBuffer()
{
  int32_t retVal = OK;

  retVal = jvmeDmaAllocLLBuffer(0x1000);

  return retVal;
}

/*!
  Routine to free the memory allocated with vmeDmaAllocLLBuffer()

  @return 0 if successful, otherwise.. an API dependent error code
*/
int32_t
vmeDmaFreeLLBuffer()
{
  int32_t retVal = OK;

  retVal = jvmeDmaFreeLLBuffer();

  return retVal;
}

/*!
  Routine to setup a DMA Linked List

  @param locAddrBase Userspace Destination address
  @param *vmeAddr    Array of VME Source addresses
  @param *dmaSize    Array of sizes of each DMA, in bytes
  @param numt        Nuumber of DMA (also size of the arrays)

  @return 0, if successful.  -1, otherwise.
*/
int32_t
vmeDmaSetupLL(u_long locAddrBase,uint32_t *vmeAddr,
	      uint32_t *dmaSize,uint32_t numt)
{
  int32_t rval = 0;

  rval = jvmeDmaSetupLL(locAddrBase,vmeAddr,dmaSize,numt);

  return rval;
}

/*!
  Routine to initiate a linked list DMA that was setup with vmeDmaSetupLL()
*/
int32_t
vmeDmaSendLL()
{
  int32_t rval = 0;

  rval = jvmeDmaSendLL();

  return rval;
}

/*!
  Routine to convert the current data buffer pointer position in userspace
  to Physical Memory space

  @param locAdrs
  Pointer to current position in data buffer in userspace.

  @return Physical Memory address of the current position in the data buffer.
*/
u_long
vmeDmaLocalToPhysAdrs(u_long locAdrs)
{
  u_long retVal=OK;

  printf("%s: ERROR: Not supported in this library\n",
	 __func__);
  retVal = ERROR;

  return retVal;
}

/*!
  Routine to convert the current data buffer pointer position in userspace
  to a VME Address.

  The VME Slave window must be initialized, prior to a call to this routine.

  @param locAdrs
  Pointer to current position in data buffer in userspace.

  @return VME Slave address of the current position in the data buffer.
*/
uint32_t
vmeDmaLocalToVmeAdrs(u_long locAdrs)
{
  uint32_t retVal=OK;

  printf("%s: ERROR: Not supported in this library\n",
	 __func__);
  retVal = ERROR;

  return retVal;
}

/*!
  Routine to print the Tempe DMA Registers
*/
void
vmeReadDMARegs()
{

}

/* API independent code here */

/* Shared Robust Mutex for vme bus access */
char* shm_name_vmeBus = "/vmeBus";
struct dma_config
{
  uint32_t dsal;
  uint32_t dsau;
  uint32_t ddal;
  uint32_t ddau;
  uint32_t dsat;
  uint32_t ddat;
  uint32_t dcnt;
  uint32_t ddbs;
  uint32_t dctl; /* without "Go" TSI148_LCSR_DCTL_DGO */
};
/* Keep this as a structure, in case we want to add to it in the future */
struct shared_memory_mutex
{
  pthread_mutex_t mutex;
  pthread_mutexattr_t m_attr;
  pid_t lockPID;
  struct dma_config dma[2];
  int32_t last_dma_channel;
  int32_t current_dma_channel;
};
struct shared_memory_mutex *p_sync=NULL;
/* mmap'd address of shared memory mutex */
void *addr_shm = NULL;

static int32_t
vmeBusMutexInit()
{
  if(!vmeQuietFlag)
    printf("%s: Initializing vmeBus mutex\n",__func__);

  p_sync->lockPID = 0;
  if(pthread_mutexattr_init(&p_sync->m_attr)<0)
    {
      perror("pthread_mutexattr_init");
      printf("%s: ERROR:  Unable to initialized mutex attribute\n",__func__);
      return ERROR;
    }
  if(pthread_mutexattr_setpshared(&p_sync->m_attr, PTHREAD_PROCESS_SHARED)<0)
    {
      perror("pthread_mutexattr_setpshared");
      printf("%s: ERROR:  Unable to set shared attribute\n",__func__);
      return ERROR;
    }
  if(pthread_mutexattr_setrobust_np(&p_sync->m_attr, PTHREAD_MUTEX_ROBUST_NP)<0)
    //if(pthread_mutexattr_setrobust(&p_sync->m_attr, PTHREAD_MUTEX_ROBUST)<0)
    {
      perror("pthread_mutexattr_setrobust_np");
      //perror("pthread_mutexattr_setrobust");
      printf("%s: ERROR:  Unable to set robust attribute\n",__func__);
      return ERROR;
    }
  if(pthread_mutex_init(&(p_sync->mutex), &p_sync->m_attr)<0)
    {
      perror("pthread_mutex_init");
      printf("%s: ERROR:  Unable to initialize shared mutex\n",__func__);
      return ERROR;
    }

  return OK;
}

/*!
  Routine to create (if needed) a shared mutex for VME Bus locking

  @return 0, if successful. -1, otherwise.
*/
int32_t
vmeBusCreateLockShm()
{
  int32_t fd_shm;
  int32_t needMutexInit=0, stat=0;
  mode_t prev_mode;

  /* Save the process ID of the current process */
  processID = getpid();

  /* First check to see if the file already exists */
  fd_shm = shm_open(shm_name_vmeBus, O_RDWR,
		    S_IRUSR | S_IWUSR |
		    S_IRGRP | S_IWGRP |
		    S_IROTH | S_IWOTH );
  if(fd_shm<0)
    {
      /* Bad file handler.. */
      if(errno == ENOENT)
	{
	  needMutexInit=1;
	}
      else
	{
	  perror("shm_open");
	  printf(" %s: ERROR: Unable to open shared memory\n",__func__);
	  return ERROR;
	}
    }

  if(needMutexInit)
    {
      if(!vmeQuietFlag)
	printf("%s: Creating vmeBus shared memory file\n",__func__);

      prev_mode = umask(0); /* need to override the current umask, if necessary */

      /* Create and map 'mutex' shared memory */
      fd_shm = shm_open(shm_name_vmeBus, O_CREAT|O_RDWR,
			S_IRUSR | S_IWUSR |
			S_IRGRP | S_IWGRP |
			S_IROTH | S_IWOTH );
      umask(prev_mode);
      if(fd_shm<0)
	{
	  perror("shm_open");
	  printf(" %s: ERROR: Unable to open shared memory\n",__func__);
	  return ERROR;
	}
      ftruncate(fd_shm, sizeof(struct shared_memory_mutex));
    }

  addr_shm = mmap(0, sizeof(struct shared_memory_mutex), PROT_READ|PROT_WRITE, MAP_SHARED, fd_shm, 0);
  if(addr_shm<0)
    {
      perror("mmap");
      printf("%s: ERROR: Unable to mmap shared memory\n",__func__);
      return ERROR;
    }
  p_sync = addr_shm;

  if(needMutexInit)
    {
      stat = vmeBusMutexInit();
      if(stat==ERROR)
	{
	  printf("%s: ERROR Initializing vmeBus Mutex\n",
		 __func__);
	  return ERROR;
	}
    }

  if(!vmeQuietFlag)
    printf("%s: vmeBus shared memory mutex initialized\n",__func__);
  return OK;
}

/*!
  Routine to destroy the shared mutex created by vmeBusCreateLockShm()

  @return 0, if successful. -1, otherwise.
*/
int32_t
vmeBusKillLockShm(int32_t kflag)
{
  int32_t rval = OK;

  if(munmap(addr_shm, sizeof(struct shared_memory_mutex))<0)
    perror("munmap");

  if(kflag==1)
    {
      if(pthread_mutexattr_destroy(&p_sync->m_attr)<0)
	perror("pthread_mutexattr_destroy");

      if(pthread_mutex_destroy(&p_sync->mutex)<0)
	perror("pthread_mutex_destroy");

      if(shm_unlink(shm_name_vmeBus)<0)
	perror("shm_unlink");

      if(!vmeQuietFlag)
	printf("%s: vmeBus shared memory mutex destroyed\n",__func__);
    }
  return rval;
}

/*!
  Routine to lock the shared mutex created by vmeBusCreateLockShm()

  @return 0, if successful. -1 or other error code otherwise.
*/
int32_t
vmeBusLock()
{
  int32_t rval;

  if(p_sync!=NULL)
    {
      rval = pthread_mutex_lock(&(p_sync->mutex));
      if(rval<0)
	{
	  perror("pthread_mutex_lock");
	  printf("%s: ERROR locking vmeBus\n",__func__);
	}
      else if (rval>0)
	{
	  printf("%s: ERROR: %s\n",__func__,
		 (rval==EINVAL)?"EINVAL":
		 (rval==EBUSY)?"EBUSY":
		 (rval==EAGAIN)?"EAGAIN":
		 (rval==EPERM)?"EPERM":
		 (rval==EOWNERDEAD)?"EOWNERDEAD":
		 (rval==ENOTRECOVERABLE)?"ENOTRECOVERABLE":
		 "Undefined");
	  if(rval==EOWNERDEAD)
	    {
	      printf("%s: WARN: Previous owner of vmeBus (mutex) died unexpectedly\n",
		     __func__);
	      printf("  Attempting to recover..\n");
	      if(pthread_mutex_consistent_np(&(p_sync->mutex))<0)
		//if(pthread_mutex_consistent(&(p_sync->mutex))<0)
		{
		  perror("pthread_mutex_consistent_np");
		  //perror("pthread_mutex_consistent");
		}
	      else
		{
		  printf("  Successful!\n");
		  rval=OK;
		}
	    }
	  if(rval==ENOTRECOVERABLE)
	    {
	      printf("%s: ERROR: vmeBus mutex in an unrecoverable state!\n",
		     __func__);
	    }
	}
      else
	{
	  p_sync->lockPID = processID;
	}
    }
  else
    {
      printf("%s: ERROR: vmeBusLock not initialized.\n",__func__);
      return ERROR;
    }
  return rval;
}

/*!
  Routine to try to lock the shared mutex created by vmeBusCreateLockShm()

  @return 0, if successful. -1 or other error code otherwise.
*/
int32_t
vmeBusTryLock()
{
  int32_t rval=ERROR;

  if(p_sync!=NULL)
    {
      rval = pthread_mutex_trylock(&(p_sync->mutex));
      if(rval<0)
	{
	  perror("pthread_mutex_trylock");
	}
      else if(rval>0)
	{
	  printf("%s: ERROR: %s\n",__func__,
		 (rval==EINVAL)?"EINVAL":
		 (rval==EBUSY)?"EBUSY":
		 (rval==EAGAIN)?"EAGAIN":
		 (rval==EPERM)?"EPERM":
		 (rval==EOWNERDEAD)?"EOWNERDEAD":
		 (rval==ENOTRECOVERABLE)?"ENOTRECOVERABLE":
		 "Undefined");
	  if(rval==EBUSY)
	    {
	      printf("%s: Locked vmeBus (mutex) owned by PID = %d\n",
		     __func__, p_sync->lockPID);
	    }
	  if(rval==EOWNERDEAD)
	    {
	      printf("%s: WARN: Previous owner of vmeBus (mutex) died unexpectedly\n",
		     __func__);
	      printf("  Attempting to recover..\n");
	      if(pthread_mutex_consistent_np(&(p_sync->mutex))<0)
		//if(pthread_mutex_consistent(&(p_sync->mutex))<0)
		{
		  perror("pthread_mutex_consistent_np");
		  //perror("pthread_mutex_consistent");
		}
	      else
		{
		  printf("  Successful!\n");
		  rval=OK;
		}
	    }
	  if(rval==ENOTRECOVERABLE)
	    {
	      printf("%s: ERROR: vmeBus mutex in an unrecoverable state!\n",
		     __func__);
	    }
	}
      else
	{
	  p_sync->lockPID = processID;
	}
    }
  else
    {
      printf("%s: ERROR: vmeBus mutex not initialized\n",__func__);
      return ERROR;
    }

  return rval;

}

/*!
  Routine to lock the shared mutex created by vmeBusCreateLockShm()

  @return 0, if successful. -1 or other error code otherwise.
*/

int32_t
vmeBusTimedLock(int32_t time_seconds)
{
  int32_t rval=ERROR;
  struct timespec timeout;

  if(p_sync!=NULL)
    {
      clock_gettime(CLOCK_REALTIME, &timeout);
      timeout.tv_nsec = 0;
      timeout.tv_sec += time_seconds;

      rval = pthread_mutex_timedlock(&p_sync->mutex,&timeout);
      if(rval<0)
	{
	  perror("pthread_mutex_timedlock");
	}
      else if(rval>0)
	{
	  printf("%s: ERROR: %s\n",__func__,
		 (rval==EINVAL)?"EINVAL":
		 (rval==EBUSY)?"EBUSY":
		 (rval==EAGAIN)?"EAGAIN":
		 (rval==ETIMEDOUT)?"ETIMEDOUT":
		 (rval==EPERM)?"EPERM":
		 (rval==EOWNERDEAD)?"EOWNERDEAD":
		 (rval==ENOTRECOVERABLE)?"ENOTRECOVERABLE":
		 "Undefined");
	  if(rval==ETIMEDOUT)
	    {
	      printf("%s: Timeout: Locked vmeBus (mutex) owned by PID = %d\n",
		     __func__, p_sync->lockPID);
	    }
	  if(rval==EOWNERDEAD)
	    {
	      printf("%s: WARN: Previous owner of vmeBus (mutex) died unexpectedly\n",
		     __func__);
	      printf("  Attempting to recover..\n");
	      if(pthread_mutex_consistent_np(&(p_sync->mutex))<0)
		//if(pthread_mutex_consistent(&(p_sync->mutex))<0)
		{
		  perror("pthread_mutex_consistent_np");
		  //perror("pthread_mutex_consistent");
		}
	      else
		{
		  printf("  Successful!\n");
		  rval=OK;
		}
	    }
	}
      else
	{
	  p_sync->lockPID = processID;
	}
    }
  else
    {
      printf("%s: ERROR: vmeBus mutex not initialized\n",__func__);
      return ERROR;
    }

  return rval;

}

/*!
  Routine to unlock the shared mutex created by vmeBusCreateLockShm()

  @return 0, if successful. -1 or other error code otherwise.
*/
int32_t
vmeBusUnlock()
{
  int32_t rval=0;
  if(p_sync!=NULL)
    {
      p_sync->lockPID = 0;
      rval = pthread_mutex_unlock(&p_sync->mutex);
      if(rval<0)
	{
	  perror("pthread_mutex_unlock");
	}
      else if(rval>0)
	{
	  printf("%s: ERROR: %s \n",__func__,
		   (rval==EINVAL)?"EINVAL":
		   (rval==EBUSY)?"EBUSY":
		   (rval==EAGAIN)?"EAGAIN":
		   (rval==EPERM)?"EPERM":
		   "Undefined");
	}
    }
  else
    {
      printf("%s: ERROR: vmeBus mutex not initialized.\n",__func__);
      return ERROR;
    }
  return rval;
}

/*!
  Routine to check the "health" of the mutex created with vmeBusCreateLockShm()

  If the mutex is found to be stale (Owner of the lock has died), it will
  be recovered.

  @param time_seconds     How many seconds to wait for mutex to unlock when testing

  @return 0, if successful. -1, otherwise.
*/
int32_t
vmeCheckMutexHealth(int32_t time_seconds)
{
  int32_t rval=0, busy_rval=0;

  if(p_sync!=NULL)
    {
      if(!vmeQuietFlag)
	printf("%s: Checking health of vmeBus shared mutex...\n",
	       __func__);

      /* Try the Mutex to see if it's state (locked/unlocked) */
      printf(" * ");
      rval = vmeBusTryLock();
      switch (rval)
	{
	case -1: /* Error */
	  printf("%s: rval = %d: Not sure what to do here\n",
		 __func__,rval);
	  break;
	case 0:  /* Success - Got the lock */
	  if(!vmeQuietFlag)
	    printf(" * ");

	  rval = vmeBusUnlock();
	  break;

	case EAGAIN: /* Bad mutex attribute initialization */
	case EINVAL: /* Bad mutex attribute initialization */
	  /* Re-Init here */
	  if(!vmeQuietFlag)
	    printf(" * ");

	  rval = vmeBusMutexInit();
	  break;

	case EBUSY: /* It's Locked */
	  {
	    /* Check to see if we can unlock it */
	    if(!vmeQuietFlag)
	      printf(" * ");

	    busy_rval = vmeBusUnlock();
	    switch(busy_rval)
	      {
	      case OK:     /* Got the unlock */
		rval=busy_rval;
		break;

	      case EAGAIN: /* Bad mutex attribute initialization */
	      case EINVAL: /* Bad mutex attribute initialization */
		/* Re-Init here */
		if(!vmeQuietFlag)
		  printf(" * ");

		rval = vmeBusMutexInit();
		break;

	      case EPERM: /* Mutex owned by another thread */
		{
		  /* Check to see if we can get the lock within 5 seconds */
		  if(!vmeQuietFlag)
		    printf(" * ");

		  busy_rval = vmeBusTimedLock(time_seconds);
		  switch(busy_rval)
		    {
		    case -1: /* Error */
		      printf("%s: rval = %d: Not sure what to do here\n",
			     __func__,busy_rval);
		      break;

		    case 0:  /* Success - Got the lock */
		      printf(" * ");
		      rval = vmeBusUnlock();
		      break;

		    case EAGAIN: /* Bad mutex attribute initialization */
		    case EINVAL: /* Bad mutex attribute initialization */
		      /* Re-Init here */
		      if(!vmeQuietFlag)
			printf(" * ");

		      rval = vmeBusMutexInit();
		      break;

		    case ETIMEDOUT: /* Timeout getting the lock */
		      /* Re-Init here */
		      if(!vmeQuietFlag)
			printf(" * ");

		      rval = vmeBusMutexInit();
		      break;

		    default:
		      printf("%s: Undefined return from pthread_mutex_timedlock (%d)\n",
			     __func__,busy_rval);
		      rval=busy_rval;

		    }

		}
		break;

	      default:
		printf("%s: Undefined return from vmeBusUnlock (%d)\n",
		       __func__,busy_rval);
		      rval=busy_rval;

	      }

	  }
	  break;

	default:
	  printf("%s: Undefined return from vmeBusTryLock (%d)\n",
		 __func__,rval);

	}

      if(rval==OK)
	{
	  if(!vmeQuietFlag)
	    printf("%s: Mutex Clean and Unlocked\n",__func__);
	}
      else
	{
	  printf("%s: Mutex is NOT usable\n",__func__);
	}

    }
  else
    {
      printf("%s: INFO: vmeBus Mutex not initialized\n",
	     __func__);
      return ERROR;
    }

  return rval;
}

int32_t
vmeSetMaximumVMESlots(int32_t slots)
{
  if((slots<1)||(slots>MAX_VME_SLOTS))
    {
      printf("%s: ERROR: Invalid slots (%d)\n",
	     __func__,slots);
      return ERROR;
    }
  maxVmeSlots = slots;

  return OK;
}

/*!
 Routine to return the VME slot, provided the VXS payload port.

 @return VME Slot number.
*/
int32_t
vxsPayloadPort2vmeSlot(int32_t payloadport)
{
  int32_t rval=0;
  int32_t islot;
  uint16_t *PayloadPort;

  if(payloadport<1 || payloadport>18)
    {
      printf("%s: ERROR: Invalid payloadport %d\n",
	     __func__,payloadport);
      return ERROR;
    }

  if(maxVmeSlots==20)
    PayloadPort = PayloadPort20;
  else if(maxVmeSlots==21)
    PayloadPort = PayloadPort21;
  else
    {
      printf("%s: ERROR: No lookup table for maxVmeSlots = %d\n",
	     __func__,maxVmeSlots);
      return ERROR;
    }

  for(islot=1;islot<MAX_VME_SLOTS+1;islot++)
    {
      if(payloadport == PayloadPort[islot])
	{
	  rval = islot;
	  break;
	}
    }

  if(rval==0)
    {
      printf("%s: ERROR: Unable to find VME Slot from Payload Port %d\n",
	     __func__,payloadport);
      rval=ERROR;
    }

  return rval;
}

/*!
 Routine to return the VME slot mask, provided the VXS payload port mask.

 @return VME Slot mask.
*/
uint32_t
vxsPayloadPortMask2vmeSlotMask(uint32_t ppmask)
{
  int32_t ipp=0;
  uint32_t vmemask=0;

  for(ipp=0; ipp<18; ipp++)
    {
      if(ppmask & (1<<ipp))
	vmemask |= (1<<vxsPayloadPort2vmeSlot(ipp+1));
    }

  return vmemask;
}

/*!
  Routine to return the VXS Payload Port provided the VME slot

  @return VXS Payload Port number.
*/
int32_t
vmeSlot2vxsPayloadPort(int32_t vmeslot)
{
  int32_t rval=0;
  uint16_t *PayloadPort;

  if(vmeslot<1 || vmeslot>maxVmeSlots)
    {
      printf("%s: ERROR: Invalid VME slot %d\n",
	     __func__,vmeslot);
      return ERROR;
    }

  if(maxVmeSlots==20)
    PayloadPort = PayloadPort20;
  else if(maxVmeSlots==21)
    PayloadPort = PayloadPort21;
  else
    {
      printf("%s: ERROR: No lookup table for maxVmeSlots = %d\n",
	     __func__,maxVmeSlots);
      return ERROR;
    }

  rval = (int)PayloadPort[vmeslot];

  if(rval==0)
    {
      printf("%s: ERROR: Unable to find Payload Port from VME Slot %d\n",
	     __func__,vmeslot);
      rval=ERROR;
    }

  return rval;
}

/*!
  Routine to return the VXS Payload Port mask provided the VME slot mask

  @return VXS Payload Port mask.
*/
uint32_t
vmeSlotMask2vxsPayloadPortMask(uint32_t vmemask)
{
  int32_t islot=0;
  uint32_t ppmask=0;

  for(islot=0; islot<22; islot++)
    {
      if(vmemask & (1<<islot))
	ppmask |= (1<<(vmeSlot2vxsPayloadPort(islot)-1));
    }

  return ppmask;
}

int32_t
vmeSlot2A24(int32_t vmeslot)
{
  int32_t rval = 0;

  if((vmeslot < 1) || (vmeslot > maxVmeSlots))
    {
      rval = ERROR;
    }
  else
    {
      rval = (vmeslot) << 19;
    }
  return rval;
}

/* Local Memory Register Read/Write routines */
uint8_t
vmeRead8(volatile uint8_t *addr)
{
  uint8_t rval;
  uint32_t vmeAdrs=0;
  uint16_t amcode=0;
  u_long local=(u_long)addr;

  rval = *addr;

  if(vmeDebugMode)
    {
      vmeLocalToVmeAdrs(local,&vmeAdrs,&amcode);
      fprintf(fDebugMode,"VDM:  0x%02x  D8  READ: 0x%08X    0x%02X\n",
	     amcode,vmeAdrs,rval);
    }

  return rval;
}

uint16_t
vmeRead16(volatile uint16_t *addr)
{
  uint16_t rval;
  uint32_t vmeAdrs=0;
  uint16_t amcode=0;
  u_long local=(u_long)addr;

  rval = *addr;
  rval = SSWAP(rval);

  if(vmeDebugMode)
    {
      vmeLocalToVmeAdrs(local,&vmeAdrs,&amcode);
      fprintf(fDebugMode,"VDM:  0x%02x D16  READ: 0x%08X    0x%04X\n",
	     amcode,vmeAdrs,rval);
    }

  return rval;
}

uint32_t
vmeRead32(volatile uint32_t *addr)
{
  uint32_t rval;
  uint32_t vmeAdrs=0;
  uint16_t amcode=0;
  u_long local=(u_long)addr;

  rval = *addr;
  rval = LSWAP(rval);

  if(vmeDebugMode)
    {
      vmeLocalToVmeAdrs(local,&vmeAdrs,&amcode);
      fprintf(fDebugMode,"VDM:  0x%02x D32  READ: 0x%08X    0x%08X\n",
	     amcode,vmeAdrs,rval);
    }

  return rval;
}

void
vmeWrite8(volatile uint8_t *addr, uint8_t val)
{
  uint32_t vmeAdrs=0;
  uint16_t amcode=0;
  u_long local=(u_long)addr;

  *addr = val;
  if(vmeDebugMode)
    {
      vmeLocalToVmeAdrs(local,&vmeAdrs,&amcode);
      fprintf(fDebugMode,"VDM:  0x%02x  D8 WRITE: 0x%08X    0x%02X\n",
	     amcode,vmeAdrs,val);
    }

  return;
}

void
vmeWrite16(volatile uint16_t *addr, uint16_t val)
{
  uint32_t vmeAdrs=0;
  uint16_t amcode=0;
  u_long local=(u_long)addr;

  val = SSWAP(val);
  *addr = val;

  if(vmeDebugMode)
    {
      vmeLocalToVmeAdrs(local,&vmeAdrs,&amcode);
      fprintf(fDebugMode,"VDM:  0x%02x D16 WRITE: 0x%08X    0x%04X\n",
	     amcode,vmeAdrs,val);
    }

  return;
}

void
vmeWrite32(volatile uint32_t *addr, uint32_t val)
{
  uint32_t vmeAdrs=0;
  uint16_t amcode=0;
  u_long local=(u_long)addr;

  val = LSWAP(val);
  *addr = val;

  if(vmeDebugMode)
    {
      vmeLocalToVmeAdrs(local,&vmeAdrs,&amcode);
      fprintf(fDebugMode,"VDM:  0x%02x D32 WRITE: 0x%08X    0x%08X\n",
	     amcode,vmeAdrs,val);
    }

  return;
}

/* VME Bus Register Read/Write routines */
uint8_t
vmeBusRead8(int32_t amcode, uint32_t vmeaddr)
{
  uint8_t rval=0;

  rval = jvmeBusRead8(amcode, vmeaddr);

  return rval;
}

uint16_t
vmeBusRead16(int32_t amcode, uint32_t vmeaddr)
{
  uint16_t rval=0;

  rval = jvmeBusRead16(amcode, vmeaddr);

  return rval;
}

uint32_t
vmeBusRead32(int32_t amcode, uint32_t vmeaddr)
{
  uint32_t rval=0;

  rval = jvmeBusRead32(amcode, vmeaddr);

  return rval;
}

void
vmeBusWrite8(int32_t amcode, uint32_t vmeaddr, uint8_t val)
{
  jvmeBusWrite8(amcode, vmeaddr, val);
}

void
vmeBusWrite16(int32_t amcode, uint32_t vmeaddr, uint16_t val)
{
  jvmeBusWrite16(amcode, vmeaddr, val);
}

void
vmeBusWrite32(int32_t amcode, uint32_t vmeaddr, uint32_t val)
{
  jvmeBusWrite32(amcode, vmeaddr, val);
}

#endif /* ARCH_armv71 */
