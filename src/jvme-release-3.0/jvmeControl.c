/*----------------------------------------------------------------------------*
 *  Copyright (c) 2018        Southeastern Universities Research Association, *
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
 *     JLab VME user interface to provide control and get status of the
 *     VME Bridge.
 *
 *----------------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <byteswap.h>
#include "jvme.h"
#include "dmaPList.h"
#include "jvmeControl.h"
#include "jvmeVIVO.h"
#include "jvmeTsi148.h"
#include "tsi148.h"
#include "jvmeUniverseII.h"
#include "ca91c042.h"

extern int vmeQuietFlag;
/* Timers to measure DMA */
unsigned long long dma_timer[10];


typedef struct jvmeControlStruct
{
  int devFile; /** file descriptor for device */
  int bridgeType;  /* Type of bridge */
  void *localPtr;    /** local pointer to vme bridge */
  uint32_t size;    /** Mapped Size of VME bridge */
} jCTRL;

jCTRL jCtrl;

static char jlab_bridge_filenames[24] =
  {
    "/dev/bus/vme/ctl"
  };

#define NBRIDGETYPES 3

enum JVME_BRIDGE_ENUM
  {
    JVME_CA91CX42 = 0,
    JVME_TSI148 = 1,
    JVME_VIVO = 2
  };

static char jBridgeNames[NBRIDGETYPES][24] =
  {
    "ca91cx42",
    "tsi148",
    "vivo"
  };


struct vme_bridge_id
{
  int32_t deviceId;
  uint32_t mapSize;
};

typedef struct
{
  int32_t level;
  int32_t statid;
  struct timeval tv;
  struct task_struct *task;
  int32_t timedout;
  int32_t monitor;
} vme_irq_t;

typedef struct
{
  unsigned long req_route;
  unsigned long xfer_route;
  unsigned long long size;
  unsigned long dwidth;
  unsigned long aspace;
  unsigned long long addr;
  unsigned long cycle;
  unsigned long *buffer;
} dma_t;

#define VME_IOC_MAGIC 0xAE
#define VME_GET_BRIDGE_INFO		_IOR(VME_IOC_MAGIC, 0x06, struct vme_bridge_id)
#define VME_DMA_REQ			_IOWR(VME_IOC_MAGIC, 0x0C, dma_t)
#define VME_DMA_REMOVE			_IO(VME_IOC_MAGIC, 0x0D)
#define VME_DMA_NEW_LIST		_IO(VME_IOC_MAGIC, 0x0E)
#define VME_DMA_FREE_LIST		_IO(VME_IOC_MAGIC, 0x0F)
#define VME_DMA_ATTR_SET		_IOR(VME_IOC_MAGIC, 0x10, dma_t)
#define VME_DMA_FREE_ATTR		_IO(VME_IOC_MAGIC, 0x11)
#define VME_DMA_XFER			_IOWR(VME_IOC_MAGIC, 0x12, dma_t)
#define VME_WAIT_IRQ			_IOWR(VME_IOC_MAGIC, 0x13, vme_irq_t)
#define VME_REM_IRQ			_IOWR(VME_IOC_MAGIC, 0x14, vme_irq_t)
#define VME_GEN_IRQ			_IOWR(VME_IOC_MAGIC, 0x15, vme_irq_t)

/*! Object to hold memory info for DMA Linked-List Descriptors */
static DMA_CMEM_ID cmemPart;

#define JVME_SUPPORTED_IDS {			\
    PCI_DEVICE_ID_TUNDRA_CA91C142,		\
      PCI_DEVICE_ID_TUNDRA_TSI148,		\
      PCI_DEVICE_ID_ABACO_VIVO}
#define JVME_SUPPORTED_MAX_IDS 3

/** Mutex for locking/unlocking VME Bridge driver access */
pthread_mutex_t bridge_mutex = PTHREAD_MUTEX_INITIALIZER;

/*! \name Mutex Defines
  Definitions for mutex locking/unlocking access to Tempe driver
  \{ */
/*! Lock the mutex for access to the Tempe driver
   \hideinitializer
 */
#define LOCK_BRIDGE {				\
    if(pthread_mutex_lock(&bridge_mutex)<0)	\
      perror("pthread_mutex_lock");		\
  }
/*! Unlock the mutex for access to the Tempe driver
   \hideinitializer
 */
#define UNLOCK_BRIDGE {				\
    if(pthread_mutex_unlock(&bridge_mutex)<0)	\
      perror("pthread_mutex_unlock");		\
  }
/* \} */

int32_t
jvmeBridgeInit()
{
  int32_t status = 0;

  status = jvmeMapControl();
  if(status < 0)
    return -1;

  switch(jCtrl.bridgeType)
    {
    case JVME_CA91CX42:
      status = jvmeUnivInit();
      break;

    case JVME_TSI148:
      status = jvmeTsi148Init();
      break;

    case JVME_VIVO:
      status = jvmeVIVOInit();
      break;

    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
      status = ERROR;
    }
  return status;

}

int32_t
jvmeBridgeClose()
{
  int32_t status = 0;

  switch(jCtrl.bridgeType)
    {
    case JVME_CA91CX42:
      status = jvmeUnivClose();
      break;

    case JVME_TSI148:
      status = jvmeTsi148Close();
      break;

    case JVME_VIVO:
      status = jvmeVIVOClose();
      break;

    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
      status = ERROR;
    }


  status = jvmeUnmapControl();

  return status;

}

int
jvmeMapControl()
{
  int rval = OK, fd = -1, id = 0, supported = 0;
  void *map;
  int supported_ids[JVME_SUPPORTED_MAX_IDS] = JVME_SUPPORTED_IDS;
  struct vme_bridge_id bridge_info;

  fd = open(jlab_bridge_filenames, O_RDWR);
  if (fd == -1)
    {
      perror("ERROR: Opening VME bridge control device file");
      return ERROR;
    }

  jCtrl.devFile = fd;

  rval = ioctl(jCtrl.devFile, VME_GET_BRIDGE_INFO, &bridge_info);
  if (rval != 0)
    {
      perror("ERROR: Failed to get bridge info");
      close(jCtrl.devFile);
      return ERROR;
    }

  /* Check device ID versus supported IDs */
  for(id=0; id < JVME_SUPPORTED_MAX_IDS; id++)
    {
      if(bridge_info.deviceId == supported_ids[id])
	{
	  supported=1;
	  break;
	}
    }
  if(!supported)
    {
      printf("%s: ERROR: Invalid device ID (0x%04x)\n",
	     __func__,
	     bridge_info.deviceId);
      close(jCtrl.devFile);
      return ERROR;
    }

  jCtrl.bridgeType = id;

  map = mmap(0, bridge_info.mapSize, PROT_READ | PROT_WRITE, MAP_SHARED,
	     jCtrl.devFile, 0);
  if (map == MAP_FAILED)
    {
      perror("Error mmapping the file");
      close(jCtrl.devFile);
      return ERROR;
    }

  jCtrl.localPtr = map;
  jCtrl.size = bridge_info.mapSize;

  return rval;
}

int
jvmeUnmapControl()
{
  int status = OK, rval = OK;

  if(jCtrl.localPtr == NULL)
    {
      printf("%s: WARN: Control pointer not initialized\n",
	     __func__);
      rval = ERROR;
    }
  else
    {
      status = munmap(jCtrl.localPtr, jCtrl.size);

      if (status == -1)
	{
	  perror("ERROR unmapping control pointer");
	  rval = ERROR;
	}
      else
	jCtrl.localPtr = NULL;
    }

  status = close(jCtrl.devFile);

  if (rval < 0)
    {
      perror("Error closing control dev file");
      rval = ERROR;
    }
  else
    jCtrl.devFile  = -1;

  return rval;
}

int
jvmeGetBridgePointer(void **localPtr)
{
  int rval= OK;
  uint32_t *ptr = NULL;

  if(jCtrl.localPtr == NULL)
    {
      printf("%s: WARN: Control pointer not initialized\n",
	     __func__);
      *localPtr = NULL;
      rval = ERROR;
    }

  *localPtr = jCtrl.localPtr;

  return rval;
}

int
jvmeSysReset()
{
  int status = OK;

  switch(jCtrl.bridgeType)
    {
    case JVME_CA91CX42:
      status = jvmeUnivSysReset();
      break;

    case JVME_TSI148:
      status = jvmeTsi148SysReset();
      break;

    case JVME_VIVO:
      status = jvmeVIVOSysReset();
      break;

    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
      status = ERROR;
    }

  return status;
}

int32_t
jvmeBERRIrqStatus()
{
  int status = OK;

  switch(jCtrl.bridgeType)
    {
    case JVME_CA91CX42:
      status = jvmeUnivGetBERRIrq();
      break;

    case JVME_TSI148:
      status = jvmeTsi148GetBERRIrq();
      break;

    case JVME_VIVO:
      status = jvmeVIVOGetBERRIrq();
      break;

    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
      status = ERROR;
    }

  return status;
}

int
jvmeSetBERRIrq(int pflag, int enable)
{
  int status = OK;

  switch(jCtrl.bridgeType)
    {
    case JVME_CA91CX42:
      status = jvmeUnivSetBERRIrq(enable);
      break;

    case JVME_TSI148:
      status = jvmeTsi148SetBERRIrq(enable);
      break;

    case JVME_VIVO:
      status = jvmeVIVOSetBERRIrq(enable);
      break;

    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
      status = ERROR;
    }

  return status;

}

int
jvmeSetA24AM(int addr_mod)
{
  int status = OK;

  switch(jCtrl.bridgeType)
    {
    case JVME_CA91CX42:
      status = jvmeUnivSetA24AM(addr_mod);
      break;

    case JVME_TSI148:
      status = jvmeTsi148SetA24AM(addr_mod);
      break;

    case JVME_VIVO:
      status = jvmeVIVOSetA24AM(addr_mod);
      break;

    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
      status = ERROR;
    }

  return status;
}


/*!
  Routine to probe a VME Address for a VME Bus Error

  @param *addr address to be probed
  @param size  size (1, 2, or 4) of address to read (in bytes)
  @param *rval where to return value

  @return 0, if successful. -1, otherwise.
*/
int
jvmeMemProbe(char *addr, int size, char *rval)
{
  int status = OK;
  uint8_t *val8;
  uint16_t *val16;
  uint32_t *val32;


  status = jvmeClearException(!vmeQuietFlag);
  switch(size)
    {
    case 1:
      val8 = (uint8_t *)rval;
      *val8 = vmeRead8((volatile uint8_t *)addr) & 0xff;
      break;
    case 2:
      val16 = (uint16_t *)rval;
      *val16 = vmeRead16((volatile uint16_t *)addr) & 0xffff;
      break;
    case 4:
      val32 = (uint32_t *)rval;
      *val32 = vmeRead32((volatile uint32_t *)addr) & 0xffffffff;
      break;

    default:
      printf("%s: ERROR: Invalid size (%d)\n",
	     __func__, size);
      status = ERROR;
    }

  status = jvmeClearException(!vmeQuietFlag);
  if(status == 1)
    status = ERROR;

  return status;
}

int32_t
jvmeClearException(int pflag)
{
  uint32_t vaddr = 0;
  int32_t status = 0;

  switch(jCtrl.bridgeType)
    {
    case JVME_TSI148:
      status = jvmeTsi148ClearException(pflag);
      break;

    case JVME_VIVO:
      status = jvmeVIVOClearException(pflag, &vaddr);
      break;

    case JVME_CA91CX42:
      status = jvmeUnivClearException(pflag);
      break;

    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
      status = ERROR;
    }
  return status;
}

int
jvmeDmaConfig(uint32_t addrType, uint32_t dataType, uint32_t sstMode)
{
  int32_t status = 0;
  switch(jCtrl.bridgeType)
    {
    case JVME_CA91CX42:
      status = jvmeUnivDmaConfig(addrType, dataType);
      break;

    case JVME_TSI148:
      status = jvmeTsi148DmaConfig(addrType, dataType, sstMode);
      break;

    case JVME_VIVO:
      status = jvmeVIVODmaConfig(addrType, dataType, sstMode);
      break;

    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
      status = ERROR;
    }
  return status;

}

int32_t
jvmeDmaSend(u_long locAdrs, uint32_t vmeAdrs, int32_t size)
{
  int32_t status = 0;

  switch(jCtrl.bridgeType)
    {
    case JVME_CA91CX42:
      status = jvmeUnivDmaSend(locAdrs,vmeAdrs,size);
      break;

    case JVME_TSI148:
      status = jvmeTsi148DmaSend(locAdrs,vmeAdrs,size);
      break;

    case JVME_VIVO:
      status = jvmeVIVODmaSend(locAdrs,vmeAdrs,size);
      break;

    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
      status = ERROR;
    }
  return status;
}

int32_t
jvmeDmaSendPhys(u_long physAdrs, uint32_t vmeAdrs, int32_t size)
{
  int32_t status = 0;

  switch(jCtrl.bridgeType)
    {
    case JVME_CA91CX42:
      status = jvmeUnivDmaSendPhys(physAdrs, vmeAdrs, size);
      break;

    case JVME_TSI148:
      status = jvmeTsi148DmaSendPhys(physAdrs, vmeAdrs, size);
      break;

    case JVME_VIVO:
      status = jvmeVIVODmaSendPhys(physAdrs, vmeAdrs, size);
      break;

    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
      status = ERROR;
    }
  return status;
}


int32_t
jvmeDmaDone()
{
  int32_t status = 0;

  switch(jCtrl.bridgeType)
    {
    case JVME_CA91CX42:
      status = jvmeUnivDmaDone(1000000);
      break;

    case JVME_TSI148:
      status = jvmeTsi148DmaDone();
      break;

    case JVME_VIVO:
      status = jvmeVIVODmaDone(-1);
      break;

    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
      status = ERROR;
    }
  return status;
}

int32_t
jvmeDmaBerrStatus()
{
  int32_t status = 0;

  switch(jCtrl.bridgeType)
    {
    case JVME_CA91CX42:
      status = jvmeUnivGetBerrStatus();
      break;

    case JVME_TSI148:
      status = jvmeTsi148GetBerrStatus();
      break;

    case JVME_VIVO:
      status = jvmeVIVOGetBerrStatus();
      break;

    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
      status = ERROR;
    }
  return status;

}


int32_t
jvmeDmaSetupLL(u_long locAddrBase,uint32_t *vmeAddr,
	      uint32_t *dmaSize,uint32_t numt)
{
  int32_t status = 0;

  switch(jCtrl.bridgeType)
    {
    case JVME_CA91CX42:
      status = jvmeUnivDmaSetupLL(locAddrBase, vmeAddr, dmaSize, numt);
      break;

    case JVME_TSI148:
      status = jvmeTsi148DmaSetupLL(locAddrBase, vmeAddr, dmaSize, numt);
      break;

    case JVME_VIVO:
    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
      status = ERROR;
    }
  return status;
}

int32_t
jvmeDmaSendLL()
{
  int32_t status = 0;

  switch(jCtrl.bridgeType)
    {
    case JVME_CA91CX42:
      jvmeUnivDmaSendLL();
      break;

    case JVME_TSI148:
      jvmeTsi148DmaSendLL();
      break;

    case JVME_VIVO:
    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
      status = ERROR;
    }
  return status;
}

int32_t
jvmeDmaAllocLLBuffer(int32_t size)
{
  int32_t status = 0;
  if(size <= 0)
    size = 0x1000;

  cmemPart = dmaCMemAlloc("dma_list", size);
  if(cmemPart == NULL)
    {
      printf("%s: ERROR: Allocating Memory for Linked List DMA\n",
	     __func__);
      return -1;
    }

  switch(jCtrl.bridgeType)
    {
    case JVME_CA91CX42:
      status = jvmeUnivSetupDmaLLBuffer(cmemPart->vaddr, cmemPart->paddr);
      break;

    case JVME_TSI148:
      status = jvmeTsi148SetupDmaLLBuffer(cmemPart->vaddr, cmemPart->paddr);
      break;

    case JVME_VIVO:
      status = jvmeVIVOSetupDmaLLBuffer(cmemPart->vaddr, cmemPart->paddr);
      break;

    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
      status = ERROR;
    }

  return status;
}

int32_t
jvmeDmaFreeLLBuffer()
{
  int32_t status = 0;

  switch(jCtrl.bridgeType)
    {
    case JVME_CA91CX42:
      status = jvmeUnivSetupDmaLLBuffer(0, 0);
      break;

    case JVME_TSI148:
      status = jvmeTsi148SetupDmaLLBuffer(0, 0);
      break;

    case JVME_VIVO:
      status = jvmeVIVOSetupDmaLLBuffer(0, 0);
      break;

    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
      status = ERROR;
    }

  return dmaCMemFree(cmemPart);
}

int32_t
jvmePrintDmaRegs()
{
  int32_t status = 0;

  switch(jCtrl.bridgeType)
    {
    case JVME_CA91CX42:
      jvmeUnivReadDMARegs();
      break;

    case JVME_TSI148:
      jvmeTsi148ReadDMARegs();
      break;

    case JVME_VIVO:
      jvmeVIVOPrintDmaRegs(0);
      break;

    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
      status = ERROR;
    }
  return status;


}


/*sergey*/
void
jvmePrintMasterWindowRegs()
{
  switch(jCtrl.bridgeType)
    {
    case JVME_CA91CX42:
      ;
      break;

    case JVME_TSI148:
      jvmeTsi148ReadMasterWindowRegs();
      break;

    case JVME_VIVO:
      jvmeVIVOPrintMasterWindowRegs(1);
      break;

    default:
      printf("%s: ERROR: Not supported for %s\n",
	     __func__,
	     jBridgeNames[jCtrl.bridgeType]);
    }
}




typedef struct jvme_level_callback
{
  int32_t enabled;
  uint32_t vector;
  VOIDFUNCPTR routine;
  uint32_t arg;
} jvme_level_callback_t;

#define JVME_MAX_LEVEL 7
jvme_level_callback_t jvmeIntCallbacks[JVME_MAX_LEVEL+1];

/* polling thread pthread and pthread_attr */
static pthread_t jvmeIntThread[JVME_MAX_LEVEL+1];

static
void jvmeInt(void *v_level)
{
  int32_t ready = 0;
  uint32_t *plevel = v_level;
  uint32_t level = plevel[0];
  vme_irq_t vme_irq;

  if((level == 0) || (level > JVME_MAX_LEVEL))
    {
      printf("%s: Invalid Level (%d)\n",
	     __func__, level);
      pthread_exit(0);
    }

  vme_irq.level = level;
  vme_irq.statid = jvmeIntCallbacks[level].vector;
  vme_irq.task = 0;
  vme_irq.tv.tv_sec = 1;
  vme_irq.tv.tv_usec = 0;
  vme_irq.timedout = 0;

  prctl(PR_SET_NAME, __func__);
  printf("%s: Starting Thread (level = %d)\n", __func__, level);

  while(1)
    {

      pthread_testcancel();

      jvmeIntCallbacks[level].enabled = 1;
      if (ioctl(jCtrl.devFile, VME_WAIT_IRQ, &vme_irq) < 0)
	{
	  perror("ioctl");
	  printf("%s: Failed to perform the VME_WAIT_IRQ command\n", __func__);
	  break;
	}

      if (vme_irq.timedout)
	printf("%s: Interrupt: Timed Out\n", __func__);
      else
	{
	  printf("%s: Interrupt: Level 0x%x, Vector 0x%x\n",
		 __func__,
		 vme_irq.level, vme_irq.statid);

	  if (jvmeIntCallbacks[level].routine != NULL)	/* call user routine */
	    (jvmeIntCallbacks[level].routine) (jvmeIntCallbacks[level].arg);

	}

      vme_irq.timedout = 0;


    }

  jvmeIntCallbacks[level].enabled = 0;
  printf("%s: Exiting Thread (level = %d)\n", __func__, level);
  pthread_exit(0);

}

int32_t
jvmeIntWait(uint32_t vector, uint32_t level, VOIDFUNCPTR routine, uint32_t arg)
{
  if((level > JVME_MAX_LEVEL) || (level == 0))
    {
      printf("%s: ERROR: Invalid level (%d)\n",
	     __func__, level);
      return ERROR;
    }

  jvmeIntCallbacks[level].vector = vector;
  jvmeIntCallbacks[level].routine = routine;
  jvmeIntCallbacks[level].arg = arg;

  int32_t ready = 0;
  vme_irq_t vme_irq;

  vme_irq.level = level;
  vme_irq.statid = jvmeIntCallbacks[level].vector;
  vme_irq.task = 0;
  vme_irq.tv.tv_sec = 1;
  vme_irq.tv.tv_usec = 0;
  vme_irq.timedout = 0;

  printf("%s: Starting Thread (level = %d)\n", __func__, level);

  jvmeIntCallbacks[level].enabled = 1;
  if (ioctl(jCtrl.devFile, VME_WAIT_IRQ, &vme_irq) < 0)
    {
      perror("ioctl");
      printf("%s: Failed to perform the VME_WAIT_IRQ command\n", __func__);
      return -1;
    }

  if (vme_irq.timedout)
    printf("%s: Interrupt: Timed Out\n", __func__);
  else
    {
      printf("%s: Interrupt: Level 0x%x, Vector 0x%x\n",
	     __func__,
	     vme_irq.level, vme_irq.statid);

      if (jvmeIntCallbacks[level].routine != NULL)	/* call user routine */
	(jvmeIntCallbacks[level].routine) (jvmeIntCallbacks[level].arg);

    }

  vme_irq.timedout = 0;

  jvmeIntCallbacks[level].enabled = 0;
  printf("%s: Exiting Thread (level = %d)\n", __func__, level);

  return 0;
}

int32_t
jvmeIntConnect(uint32_t vector, uint32_t level, VOIDFUNCPTR routine, uint32_t arg)
{
  int32_t status = 0;
  uint32_t *plevel = malloc(4);
  plevel[0] = level;

  if((level > JVME_MAX_LEVEL) || (level == 0))
    {
      printf("%s: ERROR: Invalid level (%d)\n",
	     __func__, level);
      return ERROR;
    }

  jvmeIntCallbacks[level].vector = vector;
  jvmeIntCallbacks[level].routine = routine;
  jvmeIntCallbacks[level].arg = arg;

  status = pthread_create(&jvmeIntThread[level], NULL,
			  (void*(*)(void *)) jvmeInt,
			  (void *)plevel);
  if(status != 0)
    {
      printf("%s: ERROR: Interrupt Callback Thread could not be started.\n",
	     __func__);
      printf("\t pthread_create returned: %d\n", status);
    }


  return status;
}

int32_t
jvmeIntDisconnect(uint32_t level)
{
  int32_t status = 0;
  void *res;

  if((level > JVME_MAX_LEVEL) || (level == 0))
    {
      printf("%s: ERROR: Invalid level (%d)\n",
	     __func__, level);
      return ERROR;
    }

  if(jvmeIntCallbacks[level].enabled == 0)
    {
      printf("%s: ERROR: Callback thread for level %d not connected\n",
	     __func__, level);
      return ERROR;
    }

  if(pthread_cancel(jvmeIntThread[level]) < 0)
    perror("pthread_cancel");

  if(pthread_join(jvmeIntThread[level], &res) < 0)
    perror("pthread_join");

  if (res == PTHREAD_CANCELED)
    printf("%s: Interrupt Callback Thread canceled (level = %d)\n",
	   __func__, level);
  else
    printf("%s: ERROR: Interrupt Callback Thread NOT canceled (level = %d)\n",
	   __func__, level);

  return status;
}
