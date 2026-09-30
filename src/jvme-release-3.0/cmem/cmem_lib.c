/************************************************************************/
/*									*/
/*  This is the CMEM_RCC library 					*/
/*  Its purpose is to provide user applications with contiguous data 	*/
/*  buffers for DMA operations. It is not based on any extensions to 	*/
/*  the Linux kernel like the BigPhysArea patch.			*/
/*									*/
/*  12. Dec. 01  MAJO  created						*/
/*  24. Mar. 13  Moffit - adapted for dam readout                       */
/*									*/
/*******C 2019 - The software with that certain something****************/

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include "cmem_rcc.h"

int cmem_debuglevel = 5;

#define DEBUG_TEXT(level, format, ...)					\
  if(cmem_debuglevel >= level) {					\
    printf("cmem_debug(%d): ", level);					\
    printf(format, ## __VA_ARGS__);					\
    printf("\n");}


enum
  {
    DFDB_ROSFM = 1,          //ROSFragmentManagement (actually ROSModules)
    DFDB_ROSEF,              //ROSEventFragment
    DFDB_ROSSWROBIN,         //ROSSWRobin
    DFDB_ROSFILAR,           //ROSfilar
    DFDB_ROSMEMPOOL, 	     //ROSMemoryPool
    DFDB_ROSEIM,             //ROSEventInputManager
    DFDB_ROSIO,              //ROSIO
    DFDB_ROSTG,              //Trigger generator in ROSIO
    DFDB_SLINK,              //ROSslink
    DFDB_ROSSOLAR,           //ROSsolar
    DFDB_ROSQUEST,           //ROSsolar (QUEST)
    DFDB_QUEUE,              //Queue debugging (ROSSWRobin, ROSIO, ROSCore)
    DFDB_ROSCORE,            //ROSCore
    DFDB_ROSROBIN,           //ROSRobin
    DFDB_DFDB,               //DFDebug
    DFDB_ROBINIRQ,           //RobinInterruptCatcher (in ROSModules)
    DFDB_ROSROBINNP,         //ROSRobinNP
    DFDB_CMEMRCC = 100,      //cmem_rcc
    DFDB_IORCC,              //io_rcc
    DFDB_VMERCC,             //vme_rcc
    DFDB_RCCTS,              //rcc_time_stamp
    DFDB_RCCCORBO,           //rcc_corbo
    DFDB_RCDEXAMPLE = 300,   //RCDExample
    DFDB_RCDBITSTRING,       //RCDBitString
    DFDB_RCDMENU,            //RCDMenu
    DFDB_RCDMODULE,          //RCDModule
    DFDB_RCDTTC,             //RCDTtc
    DFDB_RCDVME,             //RCDVme
    DFDB_RCDLTP,             //RCDLTPModule and RCDLTPConfiguration
    DFDB_RCDRODBUSY,         //rcc_rodbusy and RODBusyModule
    DFDB_RCDTTCVIMODULE,     //RCDTtcviModule
    DFDB_RF2TTC,             //RF2TTC and RFRX modules
    DFDB_TTCVI = 400,        //Ttcvi
    DFDB_FELIXCARD = 500     //FELIX PCIe card
  };

//Globals
static int fd, is_open = 0;

/*******************************/
CMEM_Error_code_t CMEM_Open(void)
/*******************************/
{
  int ret;

  DEBUG_TEXT(15, "CMEM_Open: Called");

  //we need to open the driver only once
  if (is_open)
  {
    is_open++;             //keep track of multiple open calls
    return(RCC_ERROR_RETURN(0, CMEM_RCC_SUCCESS));
  }

  DEBUG_TEXT(15, "CMEM_Open: Opening /dev/cmem_rcc");
  if ((fd = open("/dev/cmem_rcc", O_RDWR)) < 0)
  {
    DEBUG_TEXT(5, "CMEM_Open: Failed to open /dev/cmem_rcc");
    return(RCC_ERROR_RETURN(0, CMEM_RCC_FILE));
  }
  DEBUG_TEXT(15, "CMEM_Open: /dev/cmem_rcc is open");

  is_open = 1;
  return(0);
}


/*************************************/
CMEM_Error_code_t CMEM_OpenNopage(void)
/*************************************/
{
  int ret;
  DEBUG_TEXT(15, "CMEM_OpenNopage: Called");

  //we need to open the driver only once
  if (is_open)
  {
    is_open++;             //keep track of multiple open calls
    return(RCC_ERROR_RETURN(0, CMEM_RCC_SUCCESS));
  }

  DEBUG_TEXT(15, "CMEM_OpenNopage: Opening /dev/cmem_rcc_2");
  if ((fd = open("/dev/cmem_rcc_2", O_RDWR)) < 0)
  {
    DEBUG_TEXT(5, "CMEM_OpenNopage: Failed to open /dev/cmem_rcc_2");
    return(RCC_ERROR_RETURN(0, CMEM_RCC_FILE));
  }
  DEBUG_TEXT(15, "CMEM_OpenNopage: /dev/cmem_rcc_2 is open");

  is_open = 1;
  return(0);
}


/********************************/
CMEM_Error_code_t CMEM_Close(void)
/********************************/
{
  DEBUG_TEXT(15, "CMEM_Close: Called");
  if (!is_open)
    return(CMEM_RCC_NOTOPEN);

  if (is_open > 1)
    is_open--;
  else
  {
    close(fd);
    is_open = 0;
  }

  return(0);
}


/***************************************************************************/
CMEM_Error_code_t CMEM_SegmentGet(int segment_identifier, cmem_rcc_t *params)
/***************************************************************************/
{
  int ret;

  DEBUG_TEXT(15, "CMEM_SegmentGet: Called");
  if (!is_open)
    return(CMEM_RCC_NOTOPEN);

  params->handle = segment_identifier;
  ret = ioctl(fd, CMEM_RCC_GETPARAMS, params);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_SegmentGet: Error from ioctl, errno = 0x%x",errno);
    return(RCC_ERROR_RETURN(errno, CMEM_RCC_IOCTL));
  }
  return(0);
}


/**************************************************************************************/
CMEM_Error_code_t CMEM_SegmentAllocate(u_long size, char *name, int *segment_identifier)
/**************************************************************************************/
{
  int order, ret;
  cmem_rcc_t desc;
  u_int system_pagesize;

  DEBUG_TEXT(15, "CMEM_SegmentAllocate: Called");
  if (!is_open)
    return(CMEM_RCC_NOTOPEN);

  if (size == 0)
  {
    DEBUG_TEXT(5, "CMEM_SegmentAllocate: size is zero");
    return(RCC_ERROR_RETURN(0, CMEM_RCC_NOSIZE));
  }

  system_pagesize = (u_int)getpagesize();

  //Calculate the "order"
  if (size <= system_pagesize)            order = 0;
  else if (size <= 2 * system_pagesize)   order = 1;
  else if (size <= 4 * system_pagesize)   order = 2;
  else if (size <= 8 * system_pagesize)   order = 3;
  else if (size <= 16 * system_pagesize)  order = 4;
  else if (size <= 32 * system_pagesize)  order = 5;
  else if (size <= 64 * system_pagesize)  order = 6;
  else if (size <= 128 * system_pagesize) order = 7;
  else if (size <= 256 * system_pagesize) order = 8;
  else if (size <= 512 * system_pagesize) order = 9;
  else
  {
    DEBUG_TEXT(5, "CMEM_SegmentAllocate: size is too big");
    return(RCC_ERROR_RETURN(0, CMEM_RCC_TOOBIG));
  }

  strcpy(desc.name, name);
  desc.order = order;
  desc.type = TYPE_GFP;

  DEBUG_TEXT(20, "CMEM_SegmentAllocate: order = %d", order);
  DEBUG_TEXT(15, "CMEM_SegmentAllocate: calling ioctl");

  ret = ioctl(fd, CMEM_RCC_GET, &desc);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_SegmentAllocate: Error from ioctl, errno = 0x%x", errno);
    return(RCC_ERROR_RETURN(errno, CMEM_RCC_IOCTL));
  }

  DEBUG_TEXT(20, "CMEM_SegmentAllocate: calling mmap");
  DEBUG_TEXT(20, "CMEM_SegmentAllocate: size                    = 0x%lx", (u_long)desc.size);
  DEBUG_TEXT(20, "CMEM_SegmentAllocate: offset ( = PCI address) = 0x%lx", (u_long)desc.paddr);
  desc.uaddr = (u_long)mmap(0, desc.size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (long)desc.paddr);
  if ((long)desc.uaddr == 0 || (long)desc.uaddr == -1)
  {
    DEBUG_TEXT(5, "CMEM_SegmentAllocate: Error from mmap, errno = 0x%x", errno);
    return(RCC_ERROR_RETURN(0, CMEM_RCC_MMAP));
  }
  DEBUG_TEXT(20, "CMEM_SegmentAllocate: Virtual address = 0x%lx", (u_long)desc.uaddr);

  DEBUG_TEXT(15, "CMEM_SegmentAllocate: Tell the driver about the uaddr");
  ret = ioctl(fd, CMEM_RCC_SETUADDR, &desc);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_SegmentAllocate: Error from 2nd ioctl, errno = 0x%x", errno);
    return(RCC_ERROR_RETURN(errno, CMEM_RCC_IOCTL));
  }

  desc.locked = 0;
  *segment_identifier = desc.handle;
  DEBUG_TEXT(20, "CMEM_SegmentAllocate: Done with handle = %d", *segment_identifier);
  return(0);
}


/*****************************************************************************/
CMEM_Error_code_t CMEM_SegmentSize(int segment_identifier, u_long *actual_size)
/*****************************************************************************/
{
  int ret;
  cmem_rcc_t desc;

  DEBUG_TEXT(15, "CMEM_SegmentSize: Called");
  if (!is_open)
    return(CMEM_RCC_NOTOPEN);

  ret = CMEM_SegmentGet(segment_identifier, &desc);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_SegmentSize: Error from CMEM_SegmentGet");
    return(RCC_ERROR_RETURN(ret, CMEM_RCC_GETP));
  }

  *actual_size = desc.size;
  return(0);
}


/********************************************************/
CMEM_Error_code_t CMEM_SegmentLock(int segment_identifier)
/********************************************************/
{
  int ret;

  DEBUG_TEXT(15, "CMEM_SegmentLock: Called");
  if (!is_open)
    return(CMEM_RCC_NOTOPEN);

  DEBUG_TEXT(15, "CMEM_SegmentLock: calling ioctl for handle %d ", segment_identifier);
  ret = ioctl(fd, CMEM_RCC_LOCK, &segment_identifier);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_SegmentLock: Error from ioctl, errno = 0x%x", errno);
    return(RCC_ERROR_RETURN(errno, CMEM_RCC_IOCTL));
  }

  return(0);
}


/**********************************************************/
CMEM_Error_code_t CMEM_SegmentUnlock(int segment_identifier)
/**********************************************************/
{
  int ret;

  DEBUG_TEXT(15, "CMEM_SegmentUnlock: Called");
  if (!is_open)
    return(CMEM_RCC_NOTOPEN);

  DEBUG_TEXT(15, "CMEM_SegmentUnlock: calling ioctl for handle %d",  segment_identifier);
  ret = ioctl(fd, CMEM_RCC_UNLOCK, &segment_identifier);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_SegmentUnlock: Error from ioctl, errno = 0x%x",  errno);
    return(RCC_ERROR_RETURN(errno, CMEM_RCC_IOCTL));
  }

  return(0);
}


/*********************************************************************************************/
CMEM_Error_code_t CMEM_SegmentPhysicalAddress(int segment_identifier, u_long *physical_address)
/*********************************************************************************************/
{
  int ret;
  cmem_rcc_t desc;

  DEBUG_TEXT(15, "CMEM_SegmentPhysicalAddress: Called");
  if (!is_open)
    return(CMEM_RCC_NOTOPEN);

  ret = CMEM_SegmentGet(segment_identifier, &desc);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_SegmentPhysicalAddress: Error from CMEM_SegmentGet");
    return(RCC_ERROR_RETURN(ret, CMEM_RCC_GETP));
  }

  *physical_address = desc.paddr;
  return(0);
}


/*******************************************************************************************/
CMEM_Error_code_t CMEM_SegmentVirtualAddress(int segment_identifier, u_long *virtual_address)
/*******************************************************************************************/
{
  int ret;
  cmem_rcc_t desc;

  DEBUG_TEXT(15, "CMEM_SegmentVirtualAddress: Called");
  if (!is_open)
    return(CMEM_RCC_NOTOPEN);

  ret = CMEM_SegmentGet(segment_identifier, &desc);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_SegmentPhysicalAddress: Error from CMEM_SegmentGet");
    return(RCC_ERROR_RETURN(ret, CMEM_RCC_GETP));
  }

  *virtual_address = desc.uaddr;
  return(0);
}


/*******************************/
CMEM_Error_code_t CMEM_Dump(void)
/*******************************/
{
  int ret;
  char mytext[TEXT_SIZE];

  DEBUG_TEXT(15, "CMEM_Dump: Called");
  if (!is_open)
    return(CMEM_RCC_NOTOPEN);

  ret = ioctl(fd, CMEM_RCC_DUMP, mytext);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_Dump: Error from ioctl, errno = 0x%d",  (errno));
    return(RCC_ERROR_RETURN(errno, CMEM_RCC_IOCTL));
  }
  printf("%s", mytext);

  return(0);
}


//For compatibility with old code
/***********************************************************/
CMEM_Error_code_t CMEM_BPASegmentFree(int segment_identifier)
/***********************************************************/
{
  CMEM_Error_code_t ret;

  DEBUG_TEXT(15, "CMEM_BPASegmentFree: Called");
  ret = CMEM_SegmentFree(segment_identifier);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_BPASegmentFree: Error from CMEM_SegmentFree");
    return(RCC_ERROR_RETURN(ret, CMEM_RCC_LL));
  }
  return(0);
}


/**************************************************************/
CMEM_Error_code_t CMEM_GFPBPASegmentFree(int segment_identifier)
/**************************************************************/
{
  int ret;

  DEBUG_TEXT(15, "CMEM_GFPBPASegmentFree: Called");

  ret = CMEM_SegmentFree(segment_identifier);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_GFPBPASegmentFree: Error from CMEM_SegmentFree");
    return(RCC_ERROR_RETURN(ret, CMEM_RCC_LL));
  }
  return(0);
}


/*****************************************************/
CMEM_Error_code_t CMEM_SegmentUnlockAndFree(char* name)
/*****************************************************/
{
  int ret;
  cmem_rcc_t desc;

  DEBUG_TEXT(15, "CMEM_SegmentUnlockAndFree: Called");

  if (!is_open)
    return(CMEM_RCC_NOTOPEN);

  strcpy(desc.name, name);
  ret = ioctl(fd, CMEM_RCC_GETNAME, &desc);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_SegmentUnlockAndFree: Error from ioctl, errno = 0x%d",  (errno));
    if (errno == CMEM_RCC_ILLNAME)
    {
      DEBUG_TEXT(5, "CMEM_SegmentUnlockAndFree: ILLNAME");
      return(RCC_ERROR_RETURN(errno, CMEM_RCC_ILLNAME));
    }
    else
    {
      DEBUG_TEXT(5, "CMEM_SegmentUnlockAndFree: other error");
      return(RCC_ERROR_RETURN(errno, CMEM_RCC_IOCTL));
    }
  }

  DEBUG_TEXT(15, "CMEM_SegmentUnlockAndFree: Matching buffer found in handle %d",  desc.handle);
  DEBUG_TEXT(15, "CMEM_SegmentUnlockAndFree: desc.paddr  = 0x%lx",  (desc.paddr));
  DEBUG_TEXT(15, "CMEM_SegmentUnlockAndFree: desc.size   = 0x%lx",  (desc.size));
  DEBUG_TEXT(15, "CMEM_SegmentUnlockAndFree: desc.locked = %d",  desc.locked);
  DEBUG_TEXT(15, "CMEM_SegmentUnlockAndFree: desc.name   = %s",  desc.name);

  if (desc.locked)
  {
    DEBUG_TEXT(15, "CMEM_SegmentUnlockAndFree: unlocking buffer");
    ret = CMEM_SegmentUnlock(desc.handle);
    if (ret)
    {
      DEBUG_TEXT(5, "CMEM_SegmentUnlockAndFree: Error from CMEM_SegmentUnlock");
      return(RCC_ERROR_RETURN(ret, CMEM_RCC_LL));
    }
  }

  ret = CMEM_SegmentFree(desc.handle);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_SegmentUnlockAndFree: Error from CMEM_SegmentFree");
    return(RCC_ERROR_RETURN(ret, CMEM_RCC_LL));
  }

  return(0);
}


//For compatibility with old code
/*****************************************************************************************/
CMEM_Error_code_t CMEM_BPASegmentAllocate(u_long size, char *name, int *segment_identifier)
/*****************************************************************************************/
{
  CMEM_Error_code_t ret;

  DEBUG_TEXT(15, "CMEM_BPASegmentAllocate: Called");
  ret = CMEM_GFPBPASegmentAllocate(size, name, segment_identifier);
  DEBUG_TEXT(15, "CMEM_BPASegmentAllocate: Done");
  return(ret);
}


/********************************************************************************************/
CMEM_Error_code_t CMEM_GFPBPASegmentAllocate(u_long size, char *name, int *segment_identifier)
/********************************************************************************************/
{
  int ret;
  u_int handle;
  cmem_rcc_t desc;

  DEBUG_TEXT(15, "CMEM_GFPBPASegmentAllocate: Called");

  strcpy(desc.name, name);
  desc.size = size;
  desc.order = 0; //not required
  desc.numa_id = 0;
  desc.type = TYPE_GFPBPA;

  ret = CMEM_Allocate(desc, &handle);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_GFPBPASegmentAllocate: Error from CMEM_Allocate");
    return(RCC_ERROR_RETURN(ret, CMEM_RCC_LL));
  }

  *segment_identifier = handle;
  DEBUG_TEXT(20, "CMEM_GFPBPASegmentAllocate: Done with handle = %d",  *segment_identifier);
  return(0);
}


/*************************************************************/
CMEM_Error_code_t CMEM_Allocate(cmem_rcc_t desc, u_int *handle)
/*************************************************************/
{
  int ret;

  DEBUG_TEXT(15, "CMEM_Allocate: Called");
  if (!is_open)
    return(CMEM_RCC_NOTOPEN);

  DEBUG_TEXT(20, "CMEM_Allocate: calling ioctl with size = 0x%lx",  (desc.size));
  ret = ioctl(fd, CMEM_RCC_GET, &desc);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_Allocate: Error from ioctl, errno = 0x%d",  (errno));
    return(RCC_ERROR_RETURN(errno, CMEM_RCC_IOCTL));
  }

  if (!desc.paddr)
  {
    DEBUG_TEXT(5, "CMEM_Allocate: Error from ioctl (PCI address = 0)");
    return(RCC_ERROR_RETURN(errno, CMEM_RCC_IOCTL));
  }

  DEBUG_TEXT(20, "CMEM_Allocate: calling mmap");
  DEBUG_TEXT(20, "CMEM_Allocate: size               = 0x%lx",  (desc.size));
  DEBUG_TEXT(20, "CMEM_Allocate: offset (PCI addr.) = 0x%lx",  (desc.paddr));
  desc.uaddr = (u_long)mmap(0, desc.size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (long)desc.paddr);
  if ((long)desc.uaddr == 0 || (long)desc.uaddr == -1)
  {
    DEBUG_TEXT(5, "CMEM_Allocate: Error from mmap, errno = 0x%x",  (errno));
    return(RCC_ERROR_RETURN(0, CMEM_RCC_MMAP));
  }
  DEBUG_TEXT(20, "CMEM_Allocate: Virtual address = 0x%lx",  ((u_long)desc.uaddr));

  DEBUG_TEXT(20, "CMEM_Allocate: Tell the driver about the uaddr");
  ret = ioctl(fd, CMEM_RCC_SETUADDR, &desc);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_Allocate: Error from 2nd ioctl, errno = 0x%x",  (errno));
    return(RCC_ERROR_RETURN(errno, CMEM_RCC_IOCTL));
  }

  *handle = desc.handle;
  DEBUG_TEXT(15, "CMEM_Allocate: Done");
  return(0);
}


/********************************************************/
CMEM_Error_code_t CMEM_SegmentFree(int segment_identifier)
/********************************************************/
{
  int ret;
  cmem_rcc_t desc;

  DEBUG_TEXT(15, "CMEM_SegmentFree: Called");
  if (!is_open)
    return(CMEM_RCC_NOTOPEN);

  ret = CMEM_SegmentGet(segment_identifier, &desc);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_SegmentFree: Error from CMEM_SegmentGet");
    return(RCC_ERROR_RETURN(ret, CMEM_RCC_GETP));
  }

  DEBUG_TEXT(15, "CMEM_SegmentFree: calling munmap");
  DEBUG_TEXT(20, "CMEM_SegmentFree: desc.uaddr   = 0x%lx",  (desc.uaddr));
  DEBUG_TEXT(20, "CMEM_SegmentFree: desc.size    = 0x%lx",  (desc.size));
  //DEBUG_TEXT(20, "CMEM_SegmentFree: desc.numa_id = %d",  (desc.numa_id));
  ret = munmap((void *)desc.uaddr, desc.size);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_SegmentFree: Error from munmap, errno = 0x%x",  (errno));
    return(RCC_ERROR_RETURN(0, CMEM_RCC_MUNMAP));
  }

  DEBUG_TEXT(20, "CMEM_SegmentFree: desc.locked = %d",  desc.locked);
  if (desc.locked != 1)
  {
    DEBUG_TEXT(15, "CMEM_SegmentFree: calling ioctl = 0x%lx",  CMEM_RCC_FREE);
    ret = ioctl(fd, CMEM_RCC_FREE, &segment_identifier);
    DEBUG_TEXT(15, "CMEM_SegmentFree: back from calling ioctl = 0x%lx",  CMEM_RCC_FREE);
    if (ret)
    {
      DEBUG_TEXT(5, "CMEM_SegmentFree: Error from ioctl, errno = 0x%x",  (errno));
      return(RCC_ERROR_RETURN(errno, CMEM_RCC_IOCTL));
    }
  }
  return(0);
}


/*********************************************************************/
CMEM_Error_code_t CMEM_err_get(err_pack err, err_str pid, err_str code)
/*********************************************************************/
{
  strcpy(pid, P_ID_CMEM_RCC_STR);

  switch (RCC_ERROR_MINOR(err))
  {
    case CMEM_RCC_SUCCESS:  strcpy(code, CMEM_RCC_SUCCESS_STR); break;
    case CMEM_RCC_FILE:     strcpy(code, CMEM_RCC_FILE_STR); break;
    case CMEM_RCC_NOTOPEN:  strcpy(code, CMEM_RCC_NOTOPEN_STR); break;
    case CMEM_RCC_IOCTL:    strcpy(code, CMEM_RCC_IOCTL_STR); break;
    case CMEM_RCC_MMAP:     strcpy(code, CMEM_RCC_MMAP_STR); break;
    case CMEM_RCC_MUNMAP:   strcpy(code, CMEM_RCC_MUNMAP_STR); break;
    case CMEM_RCC_OVERFLOW: strcpy(code, CMEM_RCC_OVERFLOW_STR); break;
    case CMEM_RCC_TOOBIG:   strcpy(code, CMEM_RCC_TOOBIG_STR); break;
    case CMEM_RCC_ILLHAND:  strcpy(code, CMEM_RCC_ILLHAND_STR); break;
    case CMEM_RCC_ILLNAME:  strcpy(code, CMEM_RCC_ILLNAME_STR); break;
    case CMEM_RCC_GETP:     strcpy(code, CMEM_RCC_GETP_STR); break;
    case CMEM_RCC_NOSIZE:   strcpy(code, CMEM_RCC_NOSIZE_STR); break;
    case CMEM_RCC_CFU:      strcpy(code, CMEM_RCC_CFU_STR); break;
    case CMEM_RCC_GFP:      strcpy(code, CMEM_RCC_GFP_STR); break;
    case CMEM_RCC_BPA:      strcpy(code, CMEM_RCC_BPA_STR); break;
    case CMEM_RCC_CTU:      strcpy(code, CMEM_RCC_CTU_STR); break;
    case CMEM_RCC_KMALLOC:  strcpy(code, CMEM_RCC_KMALLOC_STR); break;
    case CMEM_RCC_LL:       strcpy(code, CMEM_RCC_LL_STR); break;
    case CMEM_RCC_ABOVE4G:  strcpy(code, CMEM_RCC_ABOVE4G_STR); break;
    case CMEM_RCC_NOSUP:    strcpy(code, CMEM_RCC_NOSUP_STR); break;
    default:                strcpy(code, CMEM_RCC_NO_CODE_STR); return(RCC_ERROR_RETURN(0,CMEM_RCC_NO_CODE)); break;
  }
  return(RCC_ERROR_RETURN(0, CMEM_RCC_SUCCESS));
}


/*********************************************************************************************************/
CMEM_Error_code_t CMEM_SegmentAllocateNuma(u_long size, u_int numa_id, char *name, int *segment_identifier)
/*********************************************************************************************************/
{
  int order, ret;
  cmem_rcc_t desc;
  u_int system_pagesize;

  DEBUG_TEXT(15, "CMEM_SegmentAllocateNuma: Called");
  if (!is_open)
    return(CMEM_RCC_NOTOPEN);

  if (size == 0)
  {
    DEBUG_TEXT(5, "CMEM_SegmentAllocateNuma: size is zero");
    return(RCC_ERROR_RETURN(0, CMEM_RCC_NOSIZE));
  }

  system_pagesize = (u_int)getpagesize();

  //Calculate the "order"
  if (size <= system_pagesize)            order = 0;
  else if (size <= 2 * system_pagesize)   order = 1;
  else if (size <= 4 * system_pagesize)   order = 2;
  else if (size <= 8 * system_pagesize)   order = 3;
  else if (size <= 16 * system_pagesize)  order = 4;
  else if (size <= 32 * system_pagesize)  order = 5;
  else if (size <= 64 * system_pagesize)  order = 6;
  else if (size <= 128 * system_pagesize) order = 7;
  else if (size <= 256 * system_pagesize) order = 8;
  else if (size <= 512 * system_pagesize) order = 9;
  else
  {
    DEBUG_TEXT(5, "CMEM_SegmentAllocateNuma: size is too big");
    return(RCC_ERROR_RETURN(0, CMEM_RCC_TOOBIG));
  }

  strcpy(desc.name, name);
  desc.order = order;
  desc.type = TYPE_NUMA;
  desc.numa_id = numa_id;

  DEBUG_TEXT(20, "CMEM_SegmentAllocateNuma: order = %d",  order);
  DEBUG_TEXT(15, "CMEM_SegmentAllocateNuma: calling ioctl");

  ret = ioctl(fd, CMEM_RCC_GET, &desc);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_SegmentAllocateNuma: Error from ioctl, errno = 0x%x",  (errno));
    return(RCC_ERROR_RETURN(errno, CMEM_RCC_IOCTL));
  }

  DEBUG_TEXT(20, "CMEM_SegmentAllocateNuma: calling mmap");
  DEBUG_TEXT(20, "CMEM_SegmentAllocateNuma: size                    = 0x%lx",  (desc.size));
  DEBUG_TEXT(20, "CMEM_SegmentAllocateNuma: offset ( = PCI address) = 0x%lx",  (desc.paddr));
  desc.uaddr = (u_long)mmap(0, desc.size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (long)desc.paddr);
  if ((long)desc.uaddr == 0 || (long)desc.uaddr == -1)
  {
    DEBUG_TEXT(5, "CMEM_SegmentAllocateNuma: Error from mmap, errno = 0x%x",  (errno));
    return(RCC_ERROR_RETURN(0, CMEM_RCC_MMAP));
  }
  DEBUG_TEXT(20, "CMEM_SegmentAllocateNuma: Virtual address = 0x%lx",  ((u_long)desc.uaddr));

  DEBUG_TEXT(15, "CMEM_SegmentAllocateNuma: Tell the driver about the uaddr");
  ret = ioctl(fd, CMEM_RCC_SETUADDR, &desc);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_SegmentAllocateNuma: Error from 2nd ioctl, errno = 0x%x",  (errno));
    return(RCC_ERROR_RETURN(errno, CMEM_RCC_IOCTL));
  }

  desc.locked = 0;
  *segment_identifier = desc.handle;
  DEBUG_TEXT(20, "CMEM_SegmentAllocateNuma: Done with handle = %d",  *segment_identifier);
  return(0);
}


/*********************************************************************************************************/
CMEM_Error_code_t CMEM_NumaSegmentAllocate(u_long size, u_int numa_id, char *name, int *segment_identifier)
/*********************************************************************************************************/
{
  int ret;
  u_int handle;
  cmem_rcc_t desc;

  DEBUG_TEXT(15, "CMEM_NumaSegmentAllocate: Called");

  strcpy(desc.name, name);
  desc.size = size;
  desc.numa_id = numa_id;
  desc.order = 0; //not required
  desc.type = TYPE_NUMABPA;

  ret = CMEM_Allocate(desc, &handle);
  if (ret)
  {
    DEBUG_TEXT(5, "CMEM_NumaSegmentAllocate: Error from CMEM_Allocate");
    return(RCC_ERROR_RETURN(ret, CMEM_RCC_LL));
  }

  *segment_identifier = handle;
  DEBUG_TEXT(20, "CMEM_NumaSegmentAllocate: Done with handle = %d",  *segment_identifier);
  return(0);
}
