/*----------------------------------------------------------------------------*
 *  Copyright (c) 2024        Southeastern Universities Research Association, *
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
 *     Routines to handle control of VIVO
 *
 *----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include "jvme.h"
#include "jvmeVIVO.h"
#include "jvmeControl.h"
#include "dmaPList.h"


//#define DEBUG
/*sergey: add 'volatile' in all places where &pVIVO[] is pointed to*/

extern unsigned long long dma_timer[10];


/*! \name Mutex Defines
  Definitions for mutex locking/unlocking access to Tempe driver
  \{ */
/*! Lock the mutex for access to the Tempe driver
   \hideinitializer
 */
#define LOCK_VIVO {				\
    if(pthread_mutex_lock(&bridge_mutex)<0)	\
      perror("pthread_mutex_lock");		\
  }
/*! Unlock the mutex for access to the Tempe driver
   \hideinitializer
 */
#define UNLOCK_VIVO {				\
    if(pthread_mutex_unlock(&bridge_mutex)<0)	\
      perror("pthread_mutex_unlock");		\
  }
/* \} */


/** Mutex for locking/unlocking Tempe driver access */
extern pthread_mutex_t bridge_mutex;

extern uint32_t vmeQuietFlag;

volatile uint32_t *pVIVO = NULL;



/*! Maximum allowed entries in DMA Linked List */
#define VIVO_DMA_MAX_LL 21
/*! Sample descriptor - contains preset attributes */
static volatile vivo_dma_descriptor_t dmaDescSample;
/*! Pointer to Linked-List descriptors */
static volatile vivo_dma_descriptor_t *dmaDescList;

// FIXME: These need allocated from cmem
static volatile uint32_t *dmaListMap;
static unsigned long dmaListAdr;

/*! Total number of requested words in the DMA Linked List */
static uint32_t dmaLL_totalwords=0;
static int32_t dmaLLInvalid=0;  /* Set in jlabgefDmaSetupLL */

static int32_t dmaBerrStatus=0;

/*! Buffer node pointer */
extern DMANODE *the_event;
/*! Data pointer */
extern uint32_t *dma_dabufp;

/*sergey: print all registers*/
#define PRINT_DMA_REGS \
  printf("sa     = 0x%08x (%6u) (Source Address)\n",                        dmaDescSample.sa,     dmaDescSample.sa); \
  printf("sau    = 0x%08x (%6u) (Source Address Upper)\n",                  dmaDescSample.sau,    dmaDescSample.sau); \
  printf("da     = 0x%08x (%6u) (Destination Address)\n",                   dmaDescSample.da,     dmaDescSample.da); \
  printf("dau    = 0x%08x (%6u) (Destination Address Upper)\n",             dmaDescSample.dau,    dmaDescSample.dau); \
  printf("satt   = 0x%08x (%6u) (Source Attributes)\n",	                    dmaDescSample.satt,   dmaDescSample.satt); \
  printf("datt   = 0x%08x (%6u) (Destination Attributes)\n",                dmaDescSample.datt,   dmaDescSample.datt); \
  printf("ndesc  = 0x%08x (%6u) (Next Descriptor Address)\n",	            dmaDescSample.ndesc,  dmaDescSample.ndesc); \
  printf("ndescu = 0x%08x (%6u) (Next Descriptor Address Upper)\n",         dmaDescSample.ndescu, dmaDescSample.ndescu); \
  printf("bss    = 0x%08x (%6u) (Destination Broadcast Slave Select)\n",    dmaDescSample.bss,    dmaDescSample.bss); \
  printf("tl     = 0x%08x (%6u) (Transaction Length)\n",                    dmaDescSample.tl,     dmaDescSample.tl); \
  printf("pad44  = 0x%08x (%6u) (Descriptors must be on 64-byte bounds)\n", dmaDescSample.pad44,  dmaDescSample.pad44); \
  printf("pad48  = 0x%08x (%6u)\n",                                         dmaDescSample.pad48,  dmaDescSample.pad48); \
  printf("pad52  = 0x%08x (%6u)\n",                                         dmaDescSample.pad52,  dmaDescSample.pad52); \
  printf("pad56  = 0x%08x (%6u)\n",                                         dmaDescSample.pad56,  dmaDescSample.pad56); \
  printf("pad60  = 0x%08x (%6u)\n",                                         dmaDescSample.pad60,  dmaDescSample.pad60); \
  printf("pad64  = 0x%08x (%6u)\n",                                         dmaDescSample.pad64,  dmaDescSample.pad64); \
  printf("axi_vivo->axis_ecr   = 0x%08x (%6u)\n",axi_vivo->axis_ecr,axi_vivo->axis_ecr); \
  printf("axi_vivo->axis_ecr_a = 0x%08x (%6u)\n",axi_vivo->axis_ecr_a,axi_vivo->axis_ecr_a); \
  printf("axi_vivo->axim_ecr   = 0x%08x (%6u)\n",axi_vivo->axim_ecr,axi_vivo->axim_ecr);	\
  printf("axi_vivo->axim_ecr_a = 0x%08x (%6u)\n",axi_vivo->axim_ecr_a,axi_vivo->axim_ecr_a);


int32_t
jvmeVIVOInit()
{
  int32_t rval = OK;

  rval = jvmeGetBridgePointer((void **)&pVIVO);

  if(rval != OK)
    {
      printf("%s: Failed to get bridge pointer\n", __func__);
      return ERROR;
    }

  return rval;
}

int32_t
jvmeVIVOClose()
{
  int32_t rval = OK;

  return rval;
}

/*!
  Routine to assert SYSRESET on the VME Bus

  @param enable 1 to enable, otherwise disable

  @return 1 if successful, otherwise ERROR
*/
int
jvmeVIVOSysReset()
{
  if(pVIVO == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  int32_t _base = VIVO_VME_MSTR & 0xFFFF;
  volatile master_regs_t *master_vivo = (master_regs_t *)&pVIVO[_base>>2];

  LOCK_VIVO;
  master_vivo->sys_ctrl |= VIVO_SYS_CTRL_SRESET;
  master_vivo->sys_ctrl &= ~VIVO_SYS_CTRL_SRESET;
  UNLOCK_VIVO;

  return OK;
}

/*!
  Get the status of the IRQ response setting to DMA Completion.

  @return 1 if enabled, 0 if disabled, -1 if error.
*/
int
jvmeVIVOGetDMAIrq()
{
  uint32_t int_ebl = 0;
  int32_t rval = 0;

  if(pVIVO == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  int32_t _base = VIVO_CRCSR_GRP_BASE & 0xFFFF;
  volatile crcsr_grp_regs_t *crcsr_vivo = (crcsr_grp_regs_t *)&pVIVO[_base >> 2];

  LOCK_VIVO;
  int_ebl = crcsr_vivo->int_ebl;
  UNLOCK_VIVO;
  if(int_ebl == -1)
    {
      printf("%s: ERROR INT EBL read failed.", __func__);
      return ERROR;
    }

  /* Check if DMA IRQ is enabled */
  rval = (int_ebl & VIVO_INT_EBL_DMA0) ? 1 : 0;

  return rval;
}

/*!
  Set the status of the IRQ response setting to DMA Completion.

  @param enable 1 to enable, otherwise disable

  @return 1 if successful, otherwise ERROR
*/
int
jvmeVIVOSetDMAIrq(int32_t enable)
{
  uint32_t int_ebl = 0;

  if(pVIVO == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  int32_t _base = VIVO_CRCSR_GRP_BASE & 0xFFFF;
  volatile crcsr_grp_regs_t *crcsr_vivo = (crcsr_grp_regs_t *)&pVIVO[_base >> 2];

  LOCK_VIVO;
  if(enable)
    crcsr_vivo->int_ebl = crcsr_vivo->int_ebl | VIVO_INT_EBL_DMA0;
  else
    crcsr_vivo->int_ebl = crcsr_vivo->int_ebl & ~VIVO_INT_EBL_DMA0;
  UNLOCK_VIVO;

  return OK;
}

/*!
  Get the status of the IRQ response setting to BERR.

  @return 1 if enabled, 0 if disabled, -1 if error.
*/
int
jvmeVIVOGetBERRIrq()
{
  uint32_t int_ebl = 0;
  int32_t rval = 0;

  if(pVIVO == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  int32_t _base = VIVO_CRCSR_GRP_BASE & 0xFFFF;
  volatile crcsr_grp_regs_t *crcsr_vivo = (crcsr_grp_regs_t *)&pVIVO[_base >> 2];

  LOCK_VIVO;
  int_ebl = crcsr_vivo->int_ebl;
  UNLOCK_VIVO;
  if(int_ebl == -1)
    {
      printf("%s: ERROR TEMPE_INTEN read failed.", __func__);
      return ERROR;
    }

  /* Check if BERR IRQ is enabled */
  rval = (int_ebl & VIVO_INT_EBL_VBERR) ? 1 : 0;

  return rval;
}

/*!
  Set the status of the IRQ response setting to BERR.

  @param enable 1 to enable, otherwise disable

  @return 1 if successful, otherwise ERROR
*/
int
jvmeVIVOSetBERRIrq(int32_t enable)
{
  uint32_t int_ebl = 0;

  if(pVIVO == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  int32_t _base = VIVO_CRCSR_GRP_BASE & 0xFFFF;
  volatile crcsr_grp_regs_t *crcsr_vivo = (crcsr_grp_regs_t *)&pVIVO[_base >> 2];

  LOCK_VIVO;
  if(enable)
    crcsr_vivo->int_ebl = crcsr_vivo->int_ebl | VIVO_INT_EBL_VBERR;
  else
    crcsr_vivo->int_ebl = crcsr_vivo->int_ebl & ~VIVO_INT_EBL_VBERR;
  UNLOCK_VIVO;

  return OK;
}


/*!
  Routine to clear any VME Exception that is currently flagged on the VME Bridge Chip

  @param pflag
  - 1 to turn on verbosity
  - 0 to disable verbosity.
*/
int
jvmeVIVOClearException(int32_t pflag, uint32_t *vaddr)
{
  /* check the VME Exception Attribute... clear it, and put out a warning */
  volatile uint32_t ecr=0;
  int32_t rval = 0;

  if(pVIVO == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  int32_t _base = VIVO_AXIS_ECR & 0xFFFF;
  volatile axi_err_regs_t *axi_vivo = (axi_err_regs_t *)&pVIVO[_base >> 2];
  _base = VIVO_CSR_ADER_HH1 & 0xFFFF;
  volatile top_regs_t *top_vivo = (top_regs_t *)&pVIVO[_base >> 2];

  LOCK_VIVO;
  ecr = axi_vivo->axis_ecr;
  *vaddr = (axi_vivo->axis_ecr_a & VIVO_AXIS_ECR_A_ADDR);

  if(ecr & VIVO_AXIS_ECR_TYPE)
    {
      if(pflag==1)
	{
	  printf("%s: Clearing VME Exception (0x%x) at axis ecr address 0x%x\n",
		 __func__,
		 ecr, *vaddr);
	}
      axi_vivo->axis_ecr = 0;
      top_vivo->bit_clear |= VIVO_BIT_CLEAR_BERRSC;
      rval = 1;
    }
  UNLOCK_VIVO;

  return rval;
}

/*!
  Clear any Bus Error exceptions, if they exist
*/
int
jvmeVIVOClearBERR()
{
  volatile uint32_t ecr=0;
  uint32_t vme_adr = 0;
  int32_t rval = 0;

  if(pVIVO == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  int32_t _base = VIVO_AXIS_ECR & 0xFFFF;
  volatile axi_err_regs_t *axi_vivo = (axi_err_regs_t *)&pVIVO[_base >> 2];
  _base = VIVO_CSR_ADER_HH1 & 0xFFFF;
  volatile top_regs_t *top_vivo = (top_regs_t *)&pVIVO[_base >> 2];

  /* check the VME Exception Attribute... clear it, and put out a warning */
  LOCK_VIVO;
  ecr = axi_vivo->axis_ecr;

  if( (ecr & VIVO_AXIS_ECR_TYPE_RETRY) ||
      (ecr & VIVO_AXIS_ECR_TYPE_BERR) )
    {
      vme_adr = axi_vivo->axis_ecr_a;
      if(!vmeQuietFlag)
	{
	  printf("%s: Clearing VME BERR/2eST (0x%x) at VME address 0x%x\n",
		 __func__,
		 ecr,
		 vme_adr);
	}
      axi_vivo->axis_ecr = 0;
      top_vivo->bit_clear |= VIVO_BIT_CLEAR_BERRSC;
      rval = 1;
    }
  UNLOCK_VIVO;

  return rval;
}

/*!
  Routine to change the address modifier of the A24 Outbound VME Window.
  The A24 Window must be opened, prior to calling this routine.

  @param addr_mod Address modifier to be used.  If 0, the default (0x39) will be used.

  @return 0, if successful. -1, otherwise
*/

int
jvmeVIVOSetA24AM(int32_t addr_mod)
{
  int32_t iwin, restoreA24 = 0;
  uint32_t amode, otat, AM_OTAT, enabled;

  if(pVIVO == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  if( (addr_mod<0x10 || addr_mod>0x1F) && (addr_mod !=0) )
    {
      printf("%s: ERROR: Invalid AM code (0x%x).  Must be 0x10 - 0x1F).",
	     __func__,addr_mod);
    }

  if(addr_mod == 0)
    {
      restoreA24 = 1;
    }

  int32_t _base = VIVO_VME_MSTR & 0xFFFF;
  volatile master_regs_t *master_vivo = (master_regs_t *)&pVIVO[_base>>2];

  if(addr_mod > 0)
    {
      /* If needed, update the AM for USER1 */
      amode = ((master_vivo->mstr & VIVO_VME_MSTR_USERAM0) >> 16) | 0x10;
      if(amode != addr_mod)
	{
	  printf("%s: updating USER1 from 0x%x to 0x%x\n",
		 __func__, amode, addr_mod);
	  master_vivo->mstr =  (master_vivo->mstr & ~VIVO_VME_MSTR_USERAM0) |
	    ((addr_mod & 0xf) << 16);

	}
    }

  /* Update the a24 window to user USER1 */
  volatile master_window_regs_t *mw_vivo;
  int32_t am_as = 0, done = 0;
  for(iwin = 0; iwin < 8; iwin++)
    {
      _base = (VIVO_MW_CTRL0 & 0xFFFF) + (iwin * 0x20);
      mw_vivo = (volatile master_window_regs_t *)&pVIVO[_base>>2];

      am_as = mw_vivo->ctrlx & VIVO_MW_CTRLX_AM_AS;

      if( (am_as == VIVO_MW_CTRLX_AM_AS_USER1) && restoreA24)
	{
	  mw_vivo->ctrlx =
	    (mw_vivo->ctrlx & ~VIVO_MW_CTRLX_AM_AS) | VIVO_MW_CTRLX_AM_AS_A24;
	  break;

	}
      else if( (am_as == VIVO_MW_CTRLX_AM_AS_A24) && !restoreA24)
	{
	  mw_vivo->ctrlx =
	    (mw_vivo->ctrlx & ~VIVO_MW_CTRLX_AM_AS) | VIVO_MW_CTRLX_AM_AS_USER1;
	  break;
	}

    }


  return OK;
}




/*!
  Routine to initialize the local VIVO DMA Descriptor

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
int
jvmeVIVODmaConfig(uint32_t addrType, uint32_t dataType, uint32_t sstMode)
{
  printf("\n\njvmeVIVODmaConfig: %u %u %u\n\n\n",addrType,dataType,sstMode);

  /* Some default attributes */
  dmaDescSample.satt  = VIVO_DMASATTX_AM_NPA_SUPER; /* Supervisory Mode */
  dmaDescSample.satt |= VIVO_DMASATTX_DPRT_VMESRC; /* VME Source */

  switch(addrType)
    {
    case 0: /* A16 */
      dmaDescSample.satt |= VIVO_DMASATTX_AM_AS_A16;
      break;
    case 1: /* A24 */
      dmaDescSample.satt |= VIVO_DMASATTX_AM_AS_A24;
      break;
    case 2: /* A32 */
      dmaDescSample.satt |= VIVO_DMASATTX_AM_AS_A32;
      break;
    default:
      printf("%s: ERROR: Address mode addrType=%d is not supported\n",
	     __func__, addrType);
      return ERROR;
    }

  switch(dataType)
    {
    case 0: /* D16 - SCT */
    case 1: /* D32 - SCT */
      dmaDescSample.satt |= VIVO_DMASATTX_BT_SCT;
      break;
    case 2: /* BLK32 */
      dmaDescSample.satt |= VIVO_DMASATTX_BT_BLT;
      break;
    case 3: /* MBLK */
      dmaDescSample.satt |= VIVO_DMASATTX_BT_MBLT;
      break;
    case 5: /* 2eSST */
      dmaDescSample.satt |= VIVO_DMASATTX_BT_2eSST;
      switch(sstMode)
	{
	case 0: /* SST160 */
	  dmaDescSample.satt |= VIVO_DMASATTX_SST_160;
	  break;
	case 1: /* SST267 */
	  dmaDescSample.satt |= VIVO_DMASATTX_SST_267;
	  break;
	case 2: /* SST320 */
	  dmaDescSample.satt |= VIVO_DMASATTX_SST_320;
	  break;
	default: /* SST160 */
	  dmaDescSample.satt |= VIVO_DMASATTX_SST_320;
	}
      break;
    default:
      printf("%s: ERROR: Data type dataType=%d is not supported\n",
	     __func__, dataType);
      return ERROR;
    }

  printf("%s: INFO: dmaDescSample.satt=0x%08x\n\n",__func__,dmaDescSample.satt);

  return OK;

}

/*!
  Routine to initiate a DMA

  @param locAdrs Destination Userspace address
  @param vmeAdrs VME Bus source address
  @param size    Maximum size of the DMA in bytes

  @return 0, if successful. -1, otherwise.
*/
int
jvmeVIVODmaSend(u_long locAdrs, uint32_t vmeAdrs, int32_t size)
{
  int32_t rval = OK;
  u_long offset = 0, physAdrs = 0;
  int32_t nbytes=0;
  
  if(!the_event)
    {
      printf("%s: ERROR: the_event pointer is invalid!\n",__func__);
      return ERROR;
    }

  if(!the_event->physMemBase)
    {
      printf("%s: ERROR: DMA Physical Memory has an invalid base address (0x%08x)",
	     __func__,
	     (uint32_t)the_event->physMemBase);
      return ERROR;
    }

  /* Local addresses (in Userspace) need to be translated to the physical memory */
  /* Here's the offset between current buffer position and the event head */
  offset = locAdrs - the_event->partBaseAdr;
  physAdrs = the_event->physMemBase + offset;

#ifdef DEBUG
  printf("locAdrs     = 0x%lx   partBaseAdr = 0x%lx\n",
	 locAdrs, the_event->partBaseAdr);
  printf("physMemBase = 0x%lx   offset      = 0x%lx\n\n",
	 the_event->physMemBase,offset);
#endif

  /* Calculate the bytes left in this buffer */
  nbytes = the_event->part->size - sizeof(DMANODE);
  nbytes = (nbytes - ((u_long)dma_dabufp - (u_long)&(the_event->length)));

  /* Make sure nbytes is realistic */
  if(nbytes<0)
    {
      printf("%s: ERROR: Space left in buffer is less than zero (%d). Quitting\n",
	     __func__,nbytes);
      return ERROR;
    }

  /* Check the specified "size" vs. size left in buffer */
  /* ... if size==0, just use the space left in the buffer */
  if(size>nbytes)
    {
      printf("%s: WARN: Specified number of DMA bytes (%d) is greater than \n",
	     __func__,
	     size);
      printf("\tthe space left in the buffer (%d).  Using %d\n",nbytes,nbytes);
    }
  else if( (size !=0) && (size<=nbytes) )
    {
      nbytes = size;
    }

  rval = jvmeVIVODmaSendPhys(physAdrs, vmeAdrs, nbytes);

  return rval;
}

/*!
  Routine to initiate a DMA using physical memory address (instead of userspace address)

  @param physAdrs Destination Physical Memory Address
  @param vmeAdrs VME Bus source address
  @param size    Maximum size of the DMA in bytes

  @return 0, if successful. -1, otherwise.
*/
int
jvmeVIVODmaSendPhys(u_long physAdrs, uint32_t vmeAdrs, int32_t size)
{
  int32_t tmp_ctrlx=0;

  //printf("\n\njvmeVIVODmaSendPhys reached ==============================\n"); fflush(stdout);

#ifdef TIMERS
  dma_timer[0] = rdtsc();
#endif /* TIMERS */

  if(pVIVO == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }


#if 0
  /*for PRINT_DMA_REGS */
  int32_t _basetmp = VIVO_AXIS_ECR & 0xFFFF;
  volatile axi_err_regs_t *axi_vivo = (axi_err_regs_t *)&pVIVO[_basetmp >> 2];
  /*for PRINT_DMA_REGS */
#endif


  
  int32_t idma = 0;
  int32_t _base = (VIVO_DMA_GRP_BASE & 0xFFFF) + (idma * 0x1000);
  volatile dmaregs_t *dma_vivo = (dmaregs_t *)&pVIVO[_base >> 2]; //sergey: volatile

  /* Clear any previous exception */
  vmeClearException(1);

  //printf("%s: INFO: physAdrs=0x%lx, vmeAdrs=0x%lx, size=%d\n\n",__func__,physAdrs,vmeAdrs,size);

#ifdef ARCH_x86_64
  dmaDescSample.dau = (physAdrs & 0xFFFFFFFF00000000UL) >> 32;
#else
  dmaDescSample.dau = 0;
#endif
  dmaDescSample.da = physAdrs & 0xFFFFFFFF;

  /* Source (VME) address */
  dmaDescSample.sa = vmeAdrs;
  dmaDescSample.tl = size;

  /* Some defaults hardcoded for the DMA Control Register */

  /* Set Direct mode */
  tmp_ctrlx = VIVO_DMACTRLX_MODE_DIRECT;

  /* 2048 VME/PCI Block Size */
  tmp_ctrlx |= (VIVO_DMACTRLX_VTS_2048 | VIVO_DMACTRLX_AHTS_2048);
  /* 0us VME/PCI Back-off */
  tmp_ctrlx |= (VIVO_DMACTRLX_VBOT_0us | VIVO_DMACTRLX_AHBOT_0us );

#ifdef TIMERS
  dma_timer[1] = rdtsc();
#endif /* TIMERS */

  LOCK_VIVO;
  /* Source Address - Won't support 64bit addressing */
  dma_vivo->sa = dmaDescSample.sa;

  /* Destination Address */
  dma_vivo->da = dmaDescSample.da;
  if(dmaDescSample.dau>0)
    dma_vivo->dau = dmaDescSample.dau;

  /* Source attributes */
  //printf("%s: INFO: dmaDescSample.satt=0x%08x\n\n",__func__,dmaDescSample.satt);
  dma_vivo->satt = dmaDescSample.satt;
  //printf("%s: INFO: dma_vivo->satt=0x%08x\n\n",__func__,dma_vivo->satt);

  /* Data count */
  dma_vivo->tl = dmaDescSample.tl;
  //printf("%s: INFO: data count: dma_vivo->tl=%d\n\n",__func__,dma_vivo->tl);

#if 0
  printf("\n\n\njvmeVIVODmaSendPhys: staring DMA =============================\n");
  axi_vivo->axis_ecr_a = 0;
  PRINT_DMA_REGS;
#endif
  
  dma_vivo->ctrlx = tmp_ctrlx;

  /* START! */
  tmp_ctrlx |=  VIVO_DMACTRLX_START;
  dma_vivo->ctrlx = tmp_ctrlx;

  UNLOCK_VIVO;

#ifdef DEBUG
  jvmeVIVOPrintDmaRegs(0);
#endif

#ifdef TIMERS
  dma_timer[2] = rdtsc();
#endif /* TIMERS */


  return OK;
}

/*!
  Routine to poll for a DMA Completion or timeout.

  @return Number of bytes transferred, if successful, -1, otherwise.
*/
int
jvmeVIVODmaDone(int32_t timeout)
{
  uint32_t val=0;
  int32_t ii=0;
  int32_t channel=0;
  uint32_t vmeExceptionAttribute=0;
  uint32_t veal;
  uint32_t dcnt;
  int32_t status=OK;

  //printf("\n\njvmeVIVODmaDone reached ==============================\n"); fflush(stdout);

  if(timeout <= 0) timeout=10000000;
//timeout=3;
 
  dmaBerrStatus=0;

#ifdef TIMERS
  dma_timer[3] = rdtsc();
#endif /* TIMERS */

#ifdef LL
  if(dmaLLInvalid) /* Set in jlabgefDmaSetupLL */
  {
    return ERROR;
  }
#endif // LL

  if(pVIVO == NULL)
  {
    printf("%s: ERROR: No MAP to VME bridge\n", __func__);
    return ERROR;
  }

  int32_t idma = 0;
  int32_t _base = (VIVO_DMA_GRP_BASE & 0xFFFF) + (idma * 0x1000);
  volatile dmaregs_t *dma_vivo = (dmaregs_t *)&pVIVO[_base >> 2];

  _base = VIVO_AXIS_ECR & 0xFFFF;
  volatile axi_err_regs_t *axi_vivo = (axi_err_regs_t *)&pVIVO[_base >> 2];

  _base = VIVO_CSR_ADER_HH1 & 0xFFFF;
  volatile top_regs_t *top_vivo = (top_regs_t *)&pVIVO[_base >> 2];

  LOCK_VIVO;

  val = dma_vivo->stat;

  const int32_t dmadone = VIVO_DMASTATX_DONE | VIVO_DMASTATX_PAUSE | VIVO_DMASTATX_ABORT | VIVO_DMASTATX_ERROR;

  while( ((val & dmadone) == 0) && (ii < timeout) )
  {
    val = dma_vivo->stat;
//printf("dma_vivo->stat = 0x%08x\n",dma_vivo->stat);
    ii++;
//sleep(1);
    //taskDelay(1);
  }
//printf("dma_vivo->stat = 0x%08x, ii=%d\n",dma_vivo->stat,ii);

  //printf("ii=%d\n",ii);
  if(ii>=timeout)
  {
    printf("%s: DMA timed-out. DMA Status Register = 0x%08x (tried %d loops)\n", __func__,val,ii);
    UNLOCK_VIVO;
    jvmeVIVOPrintDmaRegs(0);
    LOCK_VIVO;
    status = ERROR;
  }

#ifdef TIMERS
  dma_timer[4] = rdtsc();
#endif /* TIMERS */

  /* check the VME Exception Attribute...
     clear it if the DMA ended on BERR or 2eST (2e Slave Termination) */
  vmeExceptionAttribute = axi_vivo->axis_ecr;

#if 0
    printf("\njvmeVIVO: DmaDone: registers:\n");
    PRINT_DMA_REGS;
#endif

  if( (vmeExceptionAttribute & VIVO_AXIS_ECR_TYPE_RETRY) ||
      (vmeExceptionAttribute & VIVO_AXIS_ECR_TYPE_BERR) )
  {
    dmaBerrStatus=1;
    /* Clear the error */
    axi_vivo->axis_ecr = 0;
    top_vivo->bit_clear |= VIVO_BIT_CLEAR_BERRSC;

    if(status != ERROR)
    {
      /* Read where the BERR occurred */
      veal = axi_vivo->axis_ecr_a;
      /* Return value is the difference between VEAL and the Starting Address */
      status = veal - dmaDescSample.sa;
#if 0
      printf("\njvmeVIVO: status=%d\n\n",status);
#endif
      if(status<0)
      {
	printf("%s: ERROR: VME Exception Address < DMA Source Address (0x%08x < 0x%08x)\n", __func__,veal,dmaDescSample.da);
	status = ERROR;
      }
    }
  }
  else
  {
    dmaBerrStatus=0;
    /* DMA ended on DMA Count (No BERR), return original byte count */
    if(status != ERROR)
    {
      status = dmaDescSample.tl;
      /* check and make sure 0 bytes are left in the DMA Count register */
      dcnt = dma_vivo->tl;

      
      //sergey: see following print for tdc1190 fifo-based dma, data seems fine, comment it out for now, have to investigate
#if 0
      if(dcnt != 0)
      {
	printf("%s: ERROR: DMA terminated on master byte count,", __func__);
	printf(" however (dcnt=%d) != 0 (the number of loops ii=%d, timeout=%d) (vmeAdrs=0x%08x, size=%d)\n",
                     dcnt, ii, timeout, dmaDescSample.sa, dmaDescSample.tl); //sergey: print 2 more values
        PRINT_DMA_REGS;
      }
#endif
      
    }
  }

#ifdef LL
  /* If we started a linked-list transaction, dmaLL_totalwords should be non-zero */
  if(dmaLL_totalwords>0)
  {
    status = dmaLL_totalwords<<2;
    dmaLL_totalwords=0;
  }
#endif // LL

  UNLOCK_VIVO;
#ifdef TIMERS
  dma_timer[5] = rdtsc();
#endif /* TIMERS */

  return status;

}

int
jvmeVIVOGetBerrStatus()
{
  return dmaBerrStatus;
}

int32_t
jvmeVIVOSetupDmaLLBuffer(u_long vaddr, u_long paddr)
{
  dmaListMap = (uint32_t *)vaddr;
  dmaListAdr = paddr;

  return 0;
}



#define PREG(_reg)							\
  printf("  %10.18s (0x%04lx) = 0x%08x%s",				\
	 #_reg, (u_long)&rv->_reg + _base - (u_long)rv, rv->_reg, \
	 (((u_long)&rv->_reg + _base) & 0x4) ? "\n" : "\t");

void
jvmeVIVOPrintMasterWindowRegs(int32_t pflag)
{
  volatile master_window_regs_t *rv;
  int32_t iwin = 0;
  int32_t _base = 0;


  if(pVIVO == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return;
    }

  if(pflag)
    {
      printf("%s: Master Window registers\n", __func__);
      for(iwin = 0; iwin < 8; iwin++)
	{
	  _base = (VIVO_MW_CTRL0 & 0xFFFF) + (iwin * 0x20);
	  rv = (volatile master_window_regs_t *)&pVIVO[_base>>2];
	  PREG(ctrlx);
	  PREG(addr);
	  PREG(mask);
	  PREG(offset);
	  PREG(sstbs);
	  printf("\n\n");
	}
    }

  printf("\n");

  printf("VIVO Master Windows\n\n");
  printf("   ..... AM ..... \n");
  printf("   DA NPA   AS    BT     SST SCTPWR  PCI        VME        Width      2eSST\n");
  printf("--------------------------------------------------------------------------------\n");
  /*       0 Da Super A16   SCT    160 Posted  0xc0000000 0xffff0000 0x00000000 0x00000000 */
  for(iwin = 0; iwin < 8; iwin++)
  {
    _base = (VIVO_MW_CTRL0 & 0xFFFF) + (iwin * 0x20);
    rv = (volatile master_window_regs_t *)&pVIVO[_base>>2];

    if(rv->ctrlx & VIVO_MW_CTRLX_EBL)
      printf(" %d ", iwin);
    else
      printf(" - ");

    printf("%-2.2s ",
	   (rv->ctrlx & VIVO_MW_CTRLX_AM_DA) ? "Pg" : "Da");

    printf("%-5.5s ",
	   (rv->ctrlx & VIVO_MW_CTRLX_AM_NPA) ? "User" : "Super");

    printf("%-5.5s ",
	   (rv->ctrlx & VIVO_MW_CTRLX_AM_AS) == VIVO_MW_CTRLX_AM_AS_A16 ? "A16" :
	   (rv->ctrlx & VIVO_MW_CTRLX_AM_AS) == VIVO_MW_CTRLX_AM_AS_A24 ? "A24" :
	   (rv->ctrlx & VIVO_MW_CTRLX_AM_AS) == VIVO_MW_CTRLX_AM_AS_A32 ? "A32" :
	   (rv->ctrlx & VIVO_MW_CTRLX_AM_AS) == VIVO_MW_CTRLX_AM_AS_CRCSR ? "CRCSR" :
	   (rv->ctrlx & VIVO_MW_CTRLX_AM_AS) == VIVO_MW_CTRLX_AM_AS_USER1 ? "USER1" :
	   (rv->ctrlx & VIVO_MW_CTRLX_AM_AS) == VIVO_MW_CTRLX_AM_AS_USER2 ? "USER2" :
	   "-----");

    printf("%-6.6s ",
	   (rv->ctrlx & VIVO_MW_CTRLX_BT) == VIVO_MW_CTRLX_BT_SCT ? "SCT" :
	   (rv->ctrlx & VIVO_MW_CTRLX_BT) == VIVO_MW_CTRLX_BT_BLT ? "BLT" :
	   (rv->ctrlx & VIVO_MW_CTRLX_BT) == VIVO_MW_CTRLX_BT_MBLT ? "MBLT" :
	   (rv->ctrlx & VIVO_MW_CTRLX_BT) == VIVO_MW_CTRLX_BT_2eSST ? "2eSST" :
	   (rv->ctrlx & VIVO_MW_CTRLX_BT) == VIVO_MW_CTRLX_BT_2eSSTB ? "2eSSTB" :
	   "------");

    printf("%3d ",
	   (rv->ctrlx & VIVO_MW_CTRLX_SST) == VIVO_MW_CTRLX_SST_160 ? 160 :
	   (rv->ctrlx & VIVO_MW_CTRLX_SST) == VIVO_MW_CTRLX_SST_267 ? 267 :
	   (rv->ctrlx & VIVO_MW_CTRLX_SST) == VIVO_MW_CTRLX_SST_320 ? 320 : 0);

    printf("%-7.7s ",
	   (rv->ctrlx & VIVO_MW_CTRLX_SCTPWR) ? "Coupled" : "Posted");

    printf("0x%08x ", rv->addr & VIVO_MW_ADDRX_ADDR);

    printf("0x%08x ", rv->offset & VIVO_MW_OFFSETX_OFFSET);

    printf("0x%08x ", ~(rv->mask & VIVO_MW_MASKX_MASK) + 1);

    printf("0x%08x ", rv->sstbs & VIVO_MW_2ESSTX_SSTBBS);


    printf("\n");

  }
  printf("\n");
}

void
jvmeVIVOPrintSlaveWindowRegs(int32_t pflag)
{
  volatile slave_window_regs_t *rv;
  int32_t iwin = 0;
  int32_t _base = 0;

  if(pVIVO == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return;
    }

  if(pflag)
    {
      printf("%s: Slave Window registers\n", __func__);
      for(iwin = 0; iwin < 8; iwin++)
	{
	  _base = (VIVO_SLVW_CTRL0 & 0xFFFF) + (iwin * 0x10);
	  rv = (volatile slave_window_regs_t *)&pVIVO[_base>>2];
	  PREG(ctrlx);
	  PREG(mask);
	  PREG(offset);
	  PREG(offsetu);
	  printf("\n");
	}
    }

  printf("\n");

  printf("VIVO Slave Windows\n\n");
  printf("   \n");
  printf("   ..... AM ..... \n");
  printf("   FAF DFS   NPRIV SUP DAT PRG   BLT MBLT   SBRA BE AREH ATO   Rate  BLEN \n");
  printf("--------------------------------------------------------------------------------\n");

  printf("   NPRIV SUP DAT PRG   BLT MBLT   Rate  BLEN PCI   VME  Width  \n");
  printf("   Address Modes   BLT MBLT   Rate  BLEN PCI   VME  Width  \n");
  printf("--------------------------------------------------------------------------------\n");
  /*       0 NPRIV   1   1   1     1    1        */
  for(iwin = 0; iwin < 8; iwin++)
    {
      _base = (VIVO_SLVW_CTRL0 & 0xFFFF) + (iwin * 0x10);
      rv = (volatile slave_window_regs_t *)&pVIVO[_base>>2];


      printf("\n");
    }


}

void
jvmeVIVOPrintMasterRegs()
{
  if(pVIVO == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return;
    }

  int32_t _base = VIVO_VME_MSTR & 0xFFFF;
  volatile master_regs_t *rv = (volatile master_regs_t *)&pVIVO[_base>>2];

  printf("%s: Master registers\n", __func__);
  PREG(mstr);
  PREG(mstr_stat);
  printf("\n");
  PREG(sys_ctrl);
  printf("\n\n");
  PREG(dev_ctrl);
  PREG(dev_ver);
  PREG(user_ver);
  printf("\n\n");

}

void
jvmeVIVOPrintAXIErrorCaptureRegs()
{
  if(pVIVO == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return;
    }

  int32_t _base = VIVO_AXIS_ECR & 0xFFFF;
  volatile axi_err_regs_t *rv = (volatile axi_err_regs_t *)&pVIVO[_base >> 2];

  printf("%s: AXI Error Capture registers\n", __func__);
  PREG(axis_ecr);
  PREG(axis_ecr_a);
  PREG(axim_ecr);
  PREG(axim_ecr_a);
  printf("\n");

}

void
jvmeVIVOPrintDmaRegs(int32_t dma_id)
{
  if(pVIVO == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return;
    }

  volatile dmaregs_t *rv;
  int32_t _base = 0;

  printf("%s: DMA registers\n", __func__);
  int32_t idma = 0;
  for(idma = 0; idma < 2; idma ++)
    {
      _base = (VIVO_DMA_GRP_BASE & 0xFFFF) + (idma * 0x1000);
      rv = (volatile dmaregs_t *)&pVIVO[_base >> 2];
      PREG(ctrlx);
      PREG(stat);
      PREG(is);
      PREG(ie);
      PREG(descl);
      PREG(descu);
      PREG(csa);
      PREG(csau);
      PREG(cda);
      PREG(cdau);
      printf("\n");
      PREG(sa);
      PREG(sau);
      PREG(da);
      PREG(dau);
      PREG(satt);
      PREG(datt);
      PREG(ndesc);
      PREG(ndescu);
      PREG(bss);
      PREG(tl);
      printf("\n");
    }
}
