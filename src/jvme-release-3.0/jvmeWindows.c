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
 *     JLab VME user interface to provide access to VME windows.
 *
 *----------------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <byteswap.h>
#include "jvme.h"
#include "jvmeWindows.h"

/**
   Structure to hold VME window info
 */

typedef struct jvmeWindowStruct
{
  int devFile; /** file descriptor for device */
  void *localPtr;    /** local pointer to vme windows */
  uint32_t vmeBase;  /** Base VME address */
  uint32_t size;    /** Size of VME window */
  uint32_t vmeMask;  /** Mask of valid bits in VME address */
  uint8_t addrSpace; /** VME address space */
  uint8_t transferMode;	/** VME transfer mode */
  uint8_t transferMaxWidth; /** Maxmimum VME transfer width */
  uint64_t physMemBase;	/** Base address of physical memory (Slave windows only) */
} jWIN;

#define NMASTERWINDOWS 4
#define NSLAVEWINDOWS  4
jWIN jMasterWindow[NMASTERWINDOWS];
jWIN jSlaveWindow[NSLAVEWINDOWS];

// Slave Window information used (extern) by dmaPList
u_long a32slave_physmembase = 0;
u_long a32slave_window = 0;


static char jlab_master_names[NMASTERWINDOWS][24] = {
  "/dev/bus/vme/m_a16",
  "/dev/bus/vme/m_a24",
  "/dev/bus/vme/m_a32",
  "/dev/bus/vme/m_crcsr",
};

static char jlab_slave_names[NSLAVEWINDOWS][24] = {
  "/dev/bus/vme/s_rsvd1",
  "/dev/bus/vme/s_rsvd2",
  "/dev/bus/vme/s_a32",
  "/dev/bus/vme/s_rsvd3"
};

static char jMasterWindowNames[NMASTERWINDOWS][24] = {
  "A16",
  "A24",
  "A32",
  "CRCSR"
};

static char jSlaveWindowNames[NSLAVEWINDOWS][24] = {
  "RSVD1"
  "RSVD2"
  "A32"
  "RSVD3"
};

/** Flag for turning off or on driver verbosity
    \see vmeSetQuietFlag()
*/
extern int vmeQuietFlag;

/** Mutex for locking/unlocking Tempe driver access */
/* pthread_mutex_t tsi_mutex = PTHREAD_MUTEX_INITIALIZER; */


/* VMEbus Slave Window Configuration Structure */
struct vme_slave
{
  int enable;			/* State of Window */
  uint64_t vme_addr;		/* Starting Address on the VMEbus */
  uint64_t pci_addr;		/* Starting Address on the VMEbus */
  uint64_t size;		/* Window Size */
  uint32_t aspace;		/* Address Space */
  uint32_t cycle;		/* Cycle properties */
};

#define VME_IOC_MAGIC 0xAE
#define VME_GET_SLAVE _IOR(VME_IOC_MAGIC, 1, struct vme_slave)
#define VME_SET_SLAVE _IOW(VME_IOC_MAGIC, 2, struct vme_slave)
#define VME_GET_MASTER _IOR(VME_IOC_MAGIC, 3, struct vme_master)
#define VME_SET_MASTER _IOW(VME_IOC_MAGIC, 4, struct vme_master)

#define AM_2_JVME(x) jvmeAMtoIndex(x)
#define AM_2_JVMEKERN(x) jvmeAMtoKernIndex(x)
#define JVME_2_AM(x) jvmeIndextoAM(x)
#define JVME_2_JVMEKERN(x) jvmeIndextoKernelIndex(x)
#define JVMEKERN_2_AM(x) jvmeKernelIndextoAM(x)
#define JVMEKERN_2_JVME(x) jvmeKernelIndextoIndex(x)


static int
jvmeAMtoIndex(int amcode)
{
  int rval = ERROR;

  switch (amcode)
    {
    case VME_ADDR_MOD_A16U:
      rval = JVME_A16;
      break;

    case VME_ADDR_MOD_A24UD:
      rval = JVME_A24;
      break;

    case VME_ADDR_MOD_A32UD:
      rval = JVME_A32;
      break;

    case VME_ADDR_MOD_CR_CSR:
      rval = JVME_CRCSR;
      break;

    default:
      rval = ERROR;
      fprintf(stderr, "%s: ERROR: Invalid address modifier (0x%x)\n", __func__, amcode);
    }

  return rval;
}

static int
jvmeAMtoKernelIndex(int amcode)
{
  int rval = ERROR;

  switch (amcode)
    {
    case VME_ADDR_MOD_A16U:
      rval = 0x1;
      break;

    case VME_ADDR_MOD_A24UD:
      rval = 0x2;
      break;

    case VME_ADDR_MOD_A32UD:
      rval = 0x4;
      break;

    case VME_ADDR_MOD_CR_CSR:
      rval = 0x10;
      break;

    default:
      rval = ERROR;
      fprintf(stderr, "%s: ERROR: Invalid address modifier (0x%x)\n", __func__, amcode);
    }

  return rval;
}

static int
jvmeIndextoAM(int jIndex)
{
  int rval = ERROR;

  switch (jIndex)
    {
    case JVME_A16:
      rval = VME_ADDR_MOD_A16U;
      break;

    case JVME_A24:
      rval = VME_ADDR_MOD_A24UD;
      break;

    case JVME_A32:
      rval = VME_ADDR_MOD_A32UD;
      break;

    case JVME_CRCSR:
      rval = VME_ADDR_MOD_CR_CSR;
      break;

    default:
      rval = ERROR;
      fprintf(stderr, "%s: ERROR: Invalid index (%d)\n", __func__, jIndex);
    }

  return rval;
}

static int
jvmeIndextoKernelIndex(int jIndex)
{
  int rval = ERROR;

  switch (jIndex)
    {
    case JVME_A16:
      rval = 0x1;
      break;

    case JVME_A24:
      rval = 0x2;
      break;

    case JVME_A32:
      rval = 0x4;
      break;

    case JVME_CRCSR:
      rval = 0x10;
      break;

    default:
      fprintf(stderr, "%s: ERROR: Invalid index (%d)\n", __func__, jIndex);
      rval = ERROR;
    }

  return rval;
}

static int
jvmeKernelIndextoIndex(int jKernIndex)
{
  int rval = ERROR;

  switch (jKernIndex)
    {
    case 0x1:
      rval = JVME_A16;
      break;

    case 0x2:
      rval = JVME_A24;
      break;

    case 0x4:
      rval = JVME_A32;
      break;

    case 0x10:
      rval = JVME_CRCSR;
      break;

    default:
      fprintf(stderr, "%s: ERROR: Invalid index (%d)\n", __func__, jKernIndex);
      rval = ERROR;
    }

  return rval;
}

static int
jvmeKernelIndextoAM(int jKernIndex)
{
  int rval = ERROR;

  switch (jKernIndex)
    {
    case 0x1:
      rval = VME_ADDR_MOD_A16U;
      break;

    case 0x2:
      rval = VME_ADDR_MOD_A24UD;
      break;

    case 0x4:
      rval = VME_ADDR_MOD_A32UD;
      break;

    case 0x10:
      rval = VME_ADDR_MOD_CR_CSR;
      break;

    default:
      fprintf(stderr, "%s: ERROR: Invalid index (%d)\n", __func__, jKernIndex);
      rval = ERROR;
    }

  return rval;
}

/*!
  Open a VME Master window for the specified address space, and mmap
  it into userspace

  \see jvmeCloseMasterWindow

  @param am Address Modifier of window to open
      0  A16
      1  A24
      2  A32
      3  CRCSR

  @param *localPtr Where to return base address of userspace map

  @return OK, if successful.  ERROR, otherwise.
*/

int
jvmeOpenMasterWindow(int am, void **localPtr)
{
  /* VMEbus Master Window Configuration Structure */
  struct vme_master
  {
    int enable;			/* State of Window */
    uint64_t vme_addr;		/* Starting Address on the VMEbus */
    uint64_t size;		/* Window Size */
    uint32_t aspace;		/* Address Space */
    uint32_t cycle;		/* Cycle properties */
    uint32_t dwidth;		/* Maximum Data Width */
  } master;
  int fd, retval;
  void *map;
  uint32_t vmeMask = 0;

  if ((am < 0) || (am > NMASTERWINDOWS))
    {
      fprintf(stderr, "%s: ERROR: Invalid address modifier (0x%x)\n", __func__, am);
      return ERROR;
    }

  /* jlab_master_names[4]":
    "/dev/bus/vme/m_a16",
    "/dev/bus/vme/m_a24",
    "/dev/bus/vme/m_a32",
    "/dev/bus/vme/m_crcsr",
  */

  //printf("jvmeOpenMasterWindow: am=%d, >%s<\n",am,jlab_master_names[am]);

  fd = open(jlab_master_names[am], O_RDWR);
  if (fd == -1)
    {
      perror("ERROR: Opening master window device file");
      return ERROR;
    }

  master.aspace = 0;
  //printf("jvmeOpenMasterWindow: befor ioctl: master.aspace=%d\n",master.aspace),
  retval = ioctl(fd, VME_GET_MASTER, &master);
  if (retval != 0)
    {
      perror("ERROR: Failed to get master window");
      return ERROR;
    }

  //printf("jvmeOpenMasterWindow: after ioctl: master.aspace=%d\n",master.aspace),

  map = mmap(0, master.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (map == MAP_FAILED)
    {
      close(fd);
      perror("Error mmapping the file");
      return ERROR;
    }

  //printf("jvmeOpenMasterWindow: after mmap: master.aspace=%d\n",master.aspace),
  //printf("jvmeOpenMasterWindow: master.vme_addr=0x%lx\n",master.vme_addr),
  //printf("jvmeOpenMasterWindow: master.size=0x%lx\n",master.size),

  /* Fill in the library struct */
  jMasterWindow[am].devFile = fd;
  jMasterWindow[am].localPtr = map;
  jMasterWindow[am].vmeBase = master.vme_addr;
  jMasterWindow[am].size = master.size;
  jMasterWindow[am].transferMode = master.cycle;
  jMasterWindow[am].transferMaxWidth = master.dwidth;
  jMasterWindow[am].physMemBase = 0;

  /* Convert kernel driver aspace bits into the VME Address Modifier */
  jMasterWindow[am].addrSpace = JVMEKERN_2_AM(master.aspace);
  //printf("a: 0x%lx -> 0x%lx\n",master.aspace,JVMEKERN_2_AM(master.aspace));

  switch (jMasterWindow[am].addrSpace)
    {
    case VME_ADDR_MOD_A16U:
      vmeMask = 0x0000FFFF;
      break;

    case VME_ADDR_MOD_A24UD:
      vmeMask = 0x00FFFFFF;
      break;

    case VME_ADDR_MOD_A32UD:
      vmeMask = 0xFFFFFFFF;
      break;

    case VME_ADDR_MOD_CR_CSR:
      vmeMask = 0x00FFFFFF;
      break;

    default:
      vmeMask = 0;
    }
  jMasterWindow[am].vmeMask = vmeMask;
  //printf("am=%d -> mask=0x%lx\n",am,jMasterWindow[am].vmeMask);

  /* Assign the map address to the user's pointer */
  if (localPtr != NULL)
    *localPtr = map;

  if (!vmeQuietFlag)
    jvmePrintMasterWindow(am);

  return OK;
}

int
jvmeOpenSlaveWindow(int am, void **localPtr)
{
  /* VMEbus Master Window Configuration Structure */
  struct vme_slave slave;
  int fd, retval;
  void *map;
  uint32_t vmeMask = 0;

  /* Only supporting the A32 Slave Window for now */
  if (am != JVME_A32)
    {
      fprintf(stderr, "%s: ERROR: Address modifier (%d) not supported.\n", __func__, am);
      return ERROR;
    }

  fd = open(jlab_slave_names[am], O_RDWR);
  if (fd == -1)
    {
      perror("ERROR: Opening slave window device file");
      fprintf(stderr, " am= %d filename = %s\n",
	     am, jlab_slave_names[am]);
      return ERROR;
    }

  retval = ioctl(fd, VME_GET_SLAVE, &slave);
  if (retval != 0)
    {
      perror("ERROR: Failed to get slave window");
      return ERROR;
    }

  map = mmap(0, slave.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, slave.pci_addr);
  if (map == MAP_FAILED)
    {
      close(fd);
      perror("Error mmapping the file");
      return ERROR;
    }

  /* Fill in the library struct */
  jSlaveWindow[am].devFile = fd;
  jSlaveWindow[am].localPtr = map;
  jSlaveWindow[am].vmeBase = slave.vme_addr;
  jSlaveWindow[am].size = slave.size;
  jSlaveWindow[am].transferMode = slave.cycle;
  jSlaveWindow[am].physMemBase = slave.pci_addr;

  /* Convert kernel driver aspace bits into the VME Address Modifier */
  jSlaveWindow[am].addrSpace = JVMEKERN_2_AM(slave.aspace);

  switch (jSlaveWindow[am].addrSpace)
    {
    case VME_ADDR_MOD_A16U:
      vmeMask = 0x0000FFFF;
      break;

    case VME_ADDR_MOD_A24UD:
      vmeMask = 0x00FFFFFF;
      break;

    case VME_ADDR_MOD_A32UD:
      vmeMask = 0xFFFFFFFF;
      break;

    case VME_ADDR_MOD_CR_CSR:
      vmeMask = 0x00FFFFFF;
      break;

    default:
      vmeMask = 0;
    }
  jSlaveWindow[am].vmeMask = vmeMask;

  /* Fill the variables used by dmaPList for event buffers */
  if (am == JVME_A32)
    {
      a32slave_window = jSlaveWindow[am].vmeBase;
      a32slave_physmembase = jSlaveWindow[am].physMemBase;
    }

  /* Assign the map address to the user's pointer */
  if (localPtr != NULL)
    *localPtr = map;

  if (!vmeQuietFlag)
    jvmePrintSlaveWindow(am);

  return OK;
}

int
jvmeCloseMasterWindow(int am)
{
  int rval = OK;

  if ((am < 0) || (am > NMASTERWINDOWS))
    {
      fprintf(stderr, "%s: ERROR: Invalid address modifier (0x%x)\n", __func__, am);
      return ERROR;
    }

  if (am == JVME_A32)
    {
      a32slave_window = 0;
      a32slave_physmembase = 0;
    }

  rval = munmap(jMasterWindow[am].localPtr, jMasterWindow[am].size);
  if (rval == -1)
    {
      perror("ERROR unmapping master window. munmap");
      return ERROR;
    }

  rval = close(jMasterWindow[am].devFile);
  if (rval < 0)
    {
      perror("Error closing master window dev file");
      return ERROR;
    }

  jMasterWindow[am].devFile = -1;

  return OK;
}

int
jvmeCloseSlaveWindow(int am)
{
  int rval = OK;

  if ((am < 0) || (am > NMASTERWINDOWS))
    {
      fprintf(stderr, "%s: ERROR: Invalid address modifier (0x%x)\n", __func__, am);
      return ERROR;
    }

  rval = munmap(jSlaveWindow[am].localPtr, jSlaveWindow[am].size);
  if (rval == -1)
    {
      perror("ERROR unmapping slave window. munmap");
      return ERROR;
    }

  rval = close(jSlaveWindow[am].devFile);
  if (rval < 0)
    {
      perror("Error closing slave window dev file");
      return ERROR;
    }

  jSlaveWindow[am].devFile = -1;

  /* Clear variables used by dmaPList for event buffers */
  a32slave_window = 0;
  a32slave_physmembase = 0;

  return OK;
}

int
jvmeOpenMasterWindows(int windowMask)
{
  int iam, stat = OK, rval = OK;

  /* Open all of the master windows */
  for (iam = 0; iam < NMASTERWINDOWS; iam++)
    {
      if ((1 << iam) & windowMask)
	{
	  stat = jvmeOpenMasterWindow(iam, NULL);
	  if (stat != OK)
	    {
	      fprintf(stderr, "%s: ERROR opening master window %d\n", __func__, iam);
	      rval = ERROR;
	    }
	}
    }

  return rval;
}

int
jvmeOpenSlaveWindows(int windowMask)
{
  int iam, stat = OK, rval = OK;

  /* Open all of the slave windows */
  for (iam = 0; iam < NSLAVEWINDOWS; iam++)
    {
      if ((1 << iam) & windowMask)
	{
	  stat = jvmeOpenSlaveWindow(iam, NULL);
	  if (stat != OK)
	    {
	      fprintf(stderr, "%s: ERROR opening slave window %d\n", __func__, iam);
	      rval = ERROR;
	    }
	}
    }

  return rval;
}

int
jvmeOpenDefaultWindows()
{
  int rval = OK;
  int winMask =
    (1 << JVME_A16) | (1 << JVME_A24) | (1 << JVME_A32) | (1 << JVME_CRCSR);

  rval |= jvmeOpenMasterWindows(winMask);
  /* rval |= jvmeOpenSlaveWindows(JVME_A32); */

  return rval;
}


int
jvmeCloseMasterWindows(int windowMask)
{
  int iam, stat = OK, rval = OK;

  /* Open all of the master windows */
  for (iam = 0; iam < NMASTERWINDOWS; iam++)
    {
      if ((1 << iam) & windowMask)
	{
	  stat = jvmeCloseMasterWindow(iam);
	  if (stat != OK)
	    {
	      fprintf(stderr, "%s: ERROR closing master window %d\n", __func__, iam);
	      rval = ERROR;
	    }
	}
    }

  return rval;
}

int
jvmeCloseSlaveWindows(int windowMask)
{
  int iam, stat = OK, rval = OK;

  /* Open all of the slave windows */
  for (iam = 0; iam < NSLAVEWINDOWS; iam++)
    {
      if ((1 << iam) & windowMask)
	{
	  stat = jvmeCloseSlaveWindow(iam);
	  if (stat != OK)
	    {
	      fprintf(stderr, "%s: ERROR closing slave window %d\n", __func__, iam);
	      rval = ERROR;
	    }
	}
    }

  return rval;
}
int
jvmeCloseDefaultWindows()
{
  int rval = OK;
  int winMask =
    (1 << JVME_A16) | (1 << JVME_A24) | (1 << JVME_A32) | (1 << JVME_CRCSR);

  rval |= jvmeCloseMasterWindows(winMask);

  return rval;
}


int
jvmePrintMasterWindow(int am)
{
  int rval = OK;
  printf("%5s (0x%02x) :", jMasterWindowNames[am],
	 jMasterWindow[am].addrSpace);

  if (jMasterWindow[am].devFile > 0)
    {
      printf("  VME (LOCAL) 0x%08x (0x%lx), width 0x%08x\n",
	     jMasterWindow[am].vmeBase,
	     (unsigned long) jMasterWindow[am].localPtr,
	     jMasterWindow[am].size);
    }
  else
    {
      printf("  Closed\n");
    }

  return rval;
}

int
jvmePrintSlaveWindow(int am)
{
  int rval = OK;
  printf("%5s (0x%02x) :", jSlaveWindowNames[am],
	 jSlaveWindow[am].addrSpace);

  if (jSlaveWindow[am].devFile > 0)
    {
      printf("  VME (LOCAL) 0x%08x (0x%lx), width 0x%08x\n",
	     jSlaveWindow[am].vmeBase,
	     (unsigned long) jSlaveWindow[am].localPtr,
	     jSlaveWindow[am].size);
    }
  else
    {
      printf("  Closed\n");
    }

  return rval;
}

/*!
  Routine to convert a a VME Bus address to a Userspace Address

  @param vmeAdrsSpace Bus address space in whihc vmeBusAdrs resides
  @param *vmeBusAdrs  Bus address to convert
  @param **pPciAdrs   Where to return Userspace address

  @return 0, if successful. -1, otherwise.
*/
int
jvmeBusToLocalAdrs(int vmeAdrsSpace, char *vmeBusAdrs, char **pPciAdrs)
{
  int index = 0;
  uint32_t vmeAdrToConvert = 0;
  uint32_t base = 0, limit = 0;
  uint32_t trans = 0, busAdrs = 0;
  char *pciBusAdrs = 0;


  index = AM_2_JVME(vmeAdrsSpace);

  if (index == ERROR)
    {
      fprintf(stderr, "%s: ERROR: Invalid address space (0x%02x)\n",
	     __func__, vmeAdrsSpace);
      return ERROR;
    }

  /* See if the window is enabled */
  if (jMasterWindow[index].devFile > 0)
    {
      vmeAdrToConvert = (uint32_t)
	(((unsigned long) vmeBusAdrs) & jMasterWindow[index].vmeMask);
      base = jMasterWindow[index].vmeBase;
      limit = jMasterWindow[index].size + base;

      if (((unsigned long)vmeBusAdrs <= jMasterWindow[index].vmeMask) &&
	  ((base <= vmeAdrToConvert) && (limit > vmeAdrToConvert)))
	{
	  busAdrs = vmeAdrToConvert - base;
	  pciBusAdrs = (char *)
	    ((unsigned long) busAdrs +
	     (unsigned long) jMasterWindow[index].localPtr);
	}
      else
	{
          printf("0: index=%d\n",index);
	  printf("1: 0x%lx 0x%lx\n",(unsigned long)vmeBusAdrs, jMasterWindow[index].vmeMask);
	  printf("2: 0x%x 0x%x\n",base, vmeAdrToConvert);
          printf("3: 0x%x 0x%x\n",limit, vmeAdrToConvert); 
	  fprintf(stderr, "%s: ERROR: %s VME Address (0x%lx) outside of Window [0x%x, 0x%x]\n",
		  __func__, jMasterWindowNames[index], (unsigned long) vmeBusAdrs,
		  base, limit-1);
	  return ERROR;
	}
    }
  else
    {
      fprintf(stderr, "%s: ERROR: %s window is not available\n",
	     __func__, jMasterWindowNames[index]);
      return ERROR;
    }

  *pPciAdrs = pciBusAdrs;

  return OK;
}

/*!
  Routine to convert a Userspace Address to a VME Bus address

  @param localAdrs  Local (userspace) address to convert
  @param *vmeAdrs   Where to return VME address
  @param *amCode    Where to return address modifier

  @return 0, if successful. -1, otherwise.
*/
int
jvmeLocalToVmeAdrs(unsigned long localAdrs, unsigned int *vmeAdrs,
		   unsigned short *amCode)
{
  int iam;
  unsigned long localBase = 0;
  uint32_t base = 0, width = 0;

  /* Go through each window to see where the localAdrs falls */
  for (iam = 0; iam < NMASTERWINDOWS; iam++)
    {
      localBase = (unsigned long) jMasterWindow[iam].localPtr;
      base = jMasterWindow[iam].vmeBase;
      width = jMasterWindow[iam].size;

      if ((localAdrs >= localBase) && (localAdrs < (localBase + width)))
	{
	  *vmeAdrs = localAdrs - localBase + base;
	  *amCode = jMasterWindow[iam].addrSpace;
	  return OK;
	}

    }

  fprintf(stderr, "%s: ERROR: VME address not found from 0x%lx\n",
	 __func__, localAdrs);

  *amCode = 0xFFFF;
  *vmeAdrs = 0xFFFFFFFF;
  return ERROR;
}

int
jvmeBusBlockRead32(int amcode, uint32_t addr, int nwords, uint32_t *buf)
{
  int iword = 0;
  off_t offset = 0, ret = 0;
  int jidx = AM_2_JVME(amcode);
  ssize_t sret = 0;

  /* Check if window is open */
  if (jMasterWindow[jidx].devFile <= 0)
    {
      fprintf(stderr, "%s: ERROR: %s window file descriptor invalid.\n",
	     __func__, jMasterWindowNames[jidx]);
      return ERROR;
    }

  /* lseek to specified address */
  offset = (off_t)(addr - jMasterWindow[jidx].vmeBase);

  ret = lseek(jMasterWindow[jidx].devFile, offset, SEEK_SET);
  if(ret < 0)
    {
      perror("lseek");
      return ERROR;
    }

  /* read nwords*(4 bytes) */
  sret = read(jMasterWindow[jidx].devFile, buf, nwords*sizeof(uint32_t));
  if(sret < 0)
    {
      perror("read");
      return ERROR;
    }
  else if(sret != nwords*sizeof(uint32_t))
    {
      fprintf(stderr, "%s: ERROR: read return size (%d)\n",
	     __func__, (int)sret);
      return ERROR;
    }

  /* byte swap */
  for(iword = 0; iword < nwords; iword++)
    buf[iword] = bswap_32(buf[iword]);

  return OK;
}

uint32_t
jvmeBusRead32(int amcode, uint32_t addr)
{
  uint32_t rval = 0;
  int stat = OK;

  stat = jvmeBusBlockRead32(amcode, addr, 1, (uint32_t *)&rval);

  if(stat != OK)
    return ERROR;

  return rval;
}

int
jvmeBusBlockWrite32(int amcode, uint32_t addr, int nwords, uint32_t *buf)
{
  int iword = 0;
  off_t offset = 0, ret = 0;
  int jidx = AM_2_JVME(amcode);
  ssize_t sret = 0;

  /* Check if window is open */
  if (jMasterWindow[jidx].devFile <= 0)
    {
      fprintf(stderr, "%s: ERROR: %s window file descriptor invalid.\n",
	     __func__, jMasterWindowNames[jidx]);
      return ERROR;
    }

  /* lseek to specified address */
  offset = (off_t)(addr - jMasterWindow[jidx].vmeBase);

  ret = lseek(jMasterWindow[jidx].devFile, offset, SEEK_SET);
  if(ret < 0)
    {
      perror("lseek");
      return ERROR;
    }

  /* byte swap */
  for(iword = 0; iword < nwords; iword++)
    buf[iword] = bswap_32(buf[iword]);

  /* write nwords*(4 bytes) */
  sret = write(jMasterWindow[jidx].devFile, buf, nwords*sizeof(uint32_t));
  if(sret < 0)
    {
      perror("write");
      return ERROR;
    }
  else if(sret != nwords*sizeof(uint32_t))
    {
      fprintf(stderr, "%s: ERROR: write return size (%d)\n",
	     __func__, (int)sret);
      return ERROR;
    }

  return OK;
}

void
jvmeBusWrite32(int amcode, uint32_t addr, uint32_t wval)
{
  jvmeBusBlockWrite32(amcode, addr, 1, (uint32_t *)&wval);
}


int
jvmeBusBlockRead16(int amcode, uint32_t addr, int nwords, uint16_t *buf)
{
  int iword = 0;
  off_t offset = 0, ret = 0;
  int jidx = AM_2_JVME(amcode);
  ssize_t sret = 0;

  /* Check if window is open */
  if (jMasterWindow[jidx].devFile <= 0)
    {
      fprintf(stderr, "%s: ERROR: %s window file descriptor invalid.\n",
	     __func__, jMasterWindowNames[jidx]);
      return ERROR;
    }

  /* lseek to specified address */
  offset = (off_t)(addr - jMasterWindow[jidx].vmeBase);

  ret = lseek(jMasterWindow[jidx].devFile, offset, SEEK_SET);
  if(ret < 0)
    {
      perror("lseek");
      return ERROR;
    }

  /* read nwords*(2 bytes) */
  sret = read(jMasterWindow[jidx].devFile, buf, nwords*sizeof(uint16_t));
  if(sret < 0)
    {
      perror("read");
      return ERROR;
    }
  else if(sret != nwords*sizeof(uint16_t))
    {
      fprintf(stderr, "%s: ERROR: read return size (%d)\n",
	     __func__, (int)sret);
      return ERROR;
    }

  /* byte swap */
  for(iword = 0; iword < nwords; iword++)
    buf[iword] = bswap_16(buf[iword]);

  return OK;
}

uint16_t
jvmeBusRead16(int amcode, uint32_t addr)
{
  uint16_t rval = 0;
  int stat = OK;

  stat = jvmeBusBlockRead16(amcode, addr, 1, (uint16_t *)&rval);

  if(stat != OK)
    return ERROR;

  return rval;
}

int
jvmeBusBlockWrite16(int amcode, uint32_t addr, int nwords, uint16_t *buf)
{
  int iword = 0;
  off_t offset = 0, ret = 0;
  int jidx = AM_2_JVME(amcode);
  ssize_t sret = 0;

  /* Check if window is open */
  if (jMasterWindow[jidx].devFile <= 0)
    {
      fprintf(stderr, "%s: ERROR: %s window file descriptor invalid.\n",
	     __func__, jMasterWindowNames[jidx]);
      return ERROR;
    }

  /* lseek to specified address */
  offset = (off_t)(addr - jMasterWindow[jidx].vmeBase);

  ret = lseek(jMasterWindow[jidx].devFile, offset, SEEK_SET);
  if(ret < 0)
    {
      perror("lseek");
      return ERROR;
    }

  /* byte swap */
  for(iword = 0; iword < nwords; iword++)
    buf[iword] = bswap_16(buf[iword]);

  /* write nwords*(4 bytes) */
  sret = write(jMasterWindow[jidx].devFile, buf, nwords*sizeof(uint16_t));
  if(sret < 0)
    {
      perror("write");
      return ERROR;
    }
  else if(sret != nwords*sizeof(uint16_t))
    {
      fprintf(stderr, "%s: ERROR: write return size (%d)\n",
	     __func__, (int)sret);
      return ERROR;
    }

  return OK;
}

void
jvmeBusWrite16(int amcode, uint32_t addr, uint16_t wval)
{
  jvmeBusBlockWrite16(amcode, addr, 1, (uint16_t *)&wval);
}


int
jvmeBusBlockRead8(int amcode, uint32_t addr, int nwords, uint8_t *buf)
{
  off_t offset = 0, ret = 0;
  int jidx = AM_2_JVME(amcode);
  ssize_t sret = 0;

  /* Check if window is open */
  if (jMasterWindow[jidx].devFile <= 0)
    {
      fprintf(stderr, "%s: ERROR: %s window file descriptor invalid.\n",
	     __func__, jMasterWindowNames[jidx]);
      return ERROR;
    }

  /* lseek to specified address */
  offset = (off_t)(addr - jMasterWindow[jidx].vmeBase);

  ret = lseek(jMasterWindow[jidx].devFile, offset, SEEK_SET);
  if(ret < 0)
    {
      perror("lseek");
      return ERROR;
    }

  /* read nwords*(2 bytes) */
  sret = read(jMasterWindow[jidx].devFile, buf, nwords*sizeof(uint8_t));
  if(sret < 0)
    {
      perror("read");
      return ERROR;
    }
  else if(sret != nwords*sizeof(uint8_t))
    {
      fprintf(stderr, "%s: ERROR: read return size (%d)\n",
	     __func__, (int)sret);
      return ERROR;
    }

  return OK;
}

uint8_t
jvmeBusRead8(int amcode, uint32_t addr)
{
  uint8_t rval = 0;
  int stat = OK;

  stat = jvmeBusBlockRead8(amcode, addr, 1, (uint8_t *)&rval);

  if(stat != OK)
    return ERROR;

  return rval;
}

int
jvmeBusBlockWrite8(int amcode, uint32_t addr, int nwords, uint8_t *buf)
{
  off_t offset = 0, ret = 0;
  int jidx = AM_2_JVME(amcode);
  ssize_t sret = 0;

  /* Check if window is open */
  if (jMasterWindow[jidx].devFile <= 0)
    {
      fprintf(stderr, "%s: ERROR: %s window file descriptor invalid.\n",
	     __func__, jMasterWindowNames[jidx]);
      return ERROR;
    }

  /* lseek to specified address */
  offset = (off_t)(addr - jMasterWindow[jidx].vmeBase);

  ret = lseek(jMasterWindow[jidx].devFile, offset, SEEK_SET);
  if(ret < 0)
    {
      perror("lseek");
      return ERROR;
    }

  /* write nwords*(1 byte) */
  sret = write(jMasterWindow[jidx].devFile, buf, nwords*sizeof(uint8_t));
  if(sret < 0)
    {
      perror("write");
      return ERROR;
    }
  else if(sret != nwords*sizeof(uint8_t))
    {
      fprintf(stderr, "%s: ERROR: write return size (%d)\n",
	     __func__, (int)sret);
      return ERROR;
    }

  return OK;
}

void
jvmeBusWrite8(int amcode, uint32_t addr, uint8_t wval)
{
  jvmeBusBlockWrite8(amcode, addr, 1, (uint8_t *)&wval);
}
