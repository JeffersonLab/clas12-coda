/*----------------------------------------------------------------------------*
 *  Copyright (c) 2020        Southeastern Universities Research Association, *
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
 *     Routines specific to the Universe II VME Bridge
 *
 *----------------------------------------------------------------------------*/

#include <pthread.h>
#include <stdio.h>
#include "jvme.h"
#include "jvmeControl.h"
#include "ca91c042.h"
#include "jvmeUniverseII.h"
#include "dmaPList.h"

#ifndef PCI_CSR
#define PCI_CSR 0x0004
#endif

extern unsigned long long dma_timer[10];

#define LOCK_UNIV {				\
    if(pthread_mutex_lock(&bridge_mutex)<0)	\
      perror("pthread_mutex_lock");		\
  }
/*! Unlock the mutex for access to the Tempe driver
   \hideinitializer
 */
#define UNLOCK_UNIV {				\
    if(pthread_mutex_unlock(&bridge_mutex)<0)	\
      perror("pthread_mutex_unlock");		\
  }

/** Mutex for locking/unlocking Tempe driver access */
extern pthread_mutex_t bridge_mutex;

extern uint32_t vmeQuietFlag;

/*! Userspace base address of Tempe registers */
volatile uint32_t *pUniv = NULL;

/*! Maximum allowed entries in DMA Linked List */
#define CA91CX42_DMA_MAX_LL 21
/*! Sample descriptor - contains preset attributes */
static volatile ca91cx42DmaDescriptor_t dmaDescSample;
/*! Pointer to Linked-List descriptors */
static volatile ca91cx42DmaDescriptor_t *dmaDescList;

// FIXME: These need allocated from cmem
static volatile uint32_t *dmaListMap;
static u_long dmaListAdr;

/*! Total number of requested words in the DMA Linked List */
static uint32_t dmaLL_totalwords=0;
static int32_t dmaLLInvalid=0;  /* Set in jlabgefDmaSetupLL */

/*  Variable structure which can be used for Call Back funtion if an
    error occurred.  These values will be set to the final DMA Reg.
    if an error occurs in the sysVmeDmaInt ISR  */
struct stat_struct
{
  uint32_t finalStatus;  /* Final Status of DMA */
  uint32_t finalLadr;   /* Final Local Address if error */
  uint32_t finalVadr;   /* Final VMEbus Address if error */
  uint32_t finalCount;   /* Final Byte Count if error */
};
struct stat_struct univDmaStat;      /*  Error Status Structure */

/*! Total number of requested words in the DMA Linked List */
/* static uint32_t dmaLL_totalwords=0; */
/* u_long dmaLL_physaddr = 0; */

static int32_t dmaBerrStatus=0;
/* static int32_t dmaLLInvalid=0;  /\* Set in jlabgefDmaSetupLL *\/ */
/*! Buffer node pointer */
extern DMANODE *the_event;
/*! Data pointer */
extern uint32_t *dma_dabufp;


void
univWrite32(uint32_t offset, uint32_t wval)
{
  if(pUniv == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return;
    }

  if(offset > 0x1000)
    {
      printf("%s: Invalid register offset (%d)\n",
	     __func__, offset);
      return;
    }

  pUniv[ offset >> 2 ] = wval;
}

uint32_t
univRead32(uint32_t offset)
{
  if(pUniv == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  if(offset > 0x1000)
    {
      printf("%s: Invalid register offset (%d)\n",
	     __func__, offset);
      return ERROR;
    }

  return pUniv[ offset >> 2 ];
}

int32_t
jvmeUnivInit()
{
  int32_t rval = OK;

  rval = jvmeGetBridgePointer((void **)&pUniv);

  if(rval != OK)
    {
      printf("%s: Failed to get bridge pointer\n", __func__);
      return ERROR;
    }

  return rval;
}

int32_t
jvmeUnivClose()
{
  return 0;
}


/*!
  Update A32 Slave window to allow all types of block transfers.

  \see jlabgefOpenA32Slave()

  @param window Slave Window Number
  @param base Address to return A32 Slave Window physical memory base address.
  @return OK, if successful.  Otherwise ERROR.
*/
int
jvmeUnivUpdateA32SlaveWindow(int32_t window, u_long *base)
{
  int32_t rval = OK;
  uint32_t ctl, bs, bd, to;
  uint32_t misc_stat;
  u_long phys_addr;

  if(pUniv == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  if((window < 0) || (window > 4))
    {
      printf("%s: ERROR: Invalid A32 Slave window number (%d)\n",
	     __func__, window);
      return ERROR;
    }

  LOCK_UNIV;
  misc_stat = univRead32(MISC_STAT);
  ctl = univRead32(CA91CX42_VSI_CTL[window]);
  bs  = univRead32(CA91CX42_VSI_BS[window]);
  bd  = univRead32(CA91CX42_VSI_BD[window]);
  to  = univRead32(CA91CX42_VSI_TO[window]);

  phys_addr = bs + to;
  *base = phys_addr;

#ifdef DEBUGSLAVE
  printf("%s:\n  MISC_STAT = 0x%08x\n  CTL = 0x%08x\n  BS = 0x%08x\n  BD = 0x%08x\n  TO = 0x%08x\n",
	 __func__, misc_stat, ctl, bs, bd, to);
  printf("  phys_addr = 0x%0lx\n",phys_addr);
#endif

  UNLOCK_UNIV;

  return rval;
}



/*!
  Get the status of the SysReset bit
  This should always return 0

  \see jlabgefSysReset()

  @return If successful, 0 if SysReset is clear, 1 if high.  Otherwise ERROR.
*/
int
jvmeUnivGetSysReset()
{
  int32_t rval = 0;
  uint32_t reg;

  if(pUniv == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  LOCK_UNIV;
  reg = univRead32(MISC_CTL);

  UNLOCK_UNIV;

#ifdef DEBUG_SYSRESET
  printf("VCTRL = 0x%08x SRESET = 0x%x\n",
	 reg,
	 reg & CA91CX42_MISC_CTL_SW_SRST);
#endif /* DEBUG_SYSRESET */

  rval = (reg & CA91CX42_MISC_CTL_SW_SRST) ? 1 : 0;

  return rval;
}

/*!
  Set SYSRESET on VME Bus

  @return OK, if successful.  Otherwise ERROR.
*/
int
jvmeUnivSysReset()
{
  int32_t rval = 0;
  uint32_t reg;

  if(pUniv == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  LOCK_UNIV;
  reg = univRead32(MISC_CTL);
  univWrite32(MISC_CTL, reg | CA91CX42_MISC_CTL_SW_SRST);
  UNLOCK_UNIV;

  return rval;
}

/*!
  Get the status of the IRQ response setting to BERR and DMA Done

  \see jlabgefBERRIrqStatus()

  @return 1 if enabled, 0 if disabled, -1 if error.
*/
int
jvmeUnivGetBERRIrq()
{
  int32_t rval = 0;
  uint32_t tmpCtl;

  if(pUniv == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  LOCK_UNIV;
  tmpCtl = univRead32(LINT_EN);
  UNLOCK_UNIV;

  if(tmpCtl == -1)
    {
      printf("%s: ERROR TEMPE_INTEN read failed.", __func__);
      return ERROR;
    }

  if(tmpCtl & (CA91CX42_LINT_VERR | CA91CX42_LINT_DMA))
    rval = 1;
  else
    rval = 0;

  return rval;
}


/*!
  Set the status of the IRQ response setting to BERR and DMA done

  \see jlabgefDisableBERRIrq(int32_t pflag)
  \see jlabgefEnableBERRIrq(int32_t pflag)

  @param enable 1 to enable, otherwise disable

  @return 1 if successful, otherwise ERROR
*/
int
jvmeUnivSetBERRIrq(int32_t enable)
{
  uint32_t tmpCtl;

  if(pUniv == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  LOCK_UNIV;
  tmpCtl = univRead32(LINT_EN);

  if(enable)
    tmpCtl |= (CA91CX42_LINT_VERR | CA91CX42_LINT_DMA);
  else
    tmpCtl &= ~(CA91CX42_LINT_VERR | CA91CX42_LINT_DMA);

  univWrite32(LINT_EN, tmpCtl);

  UNLOCK_UNIV;

  return OK;
}

/*!
  Routine to clear any Exception that is currently flagged on the VME Bridge Chip

  This is for coupled read/writes (single cycle reads/writes)

  @param pflag
  - 1 to turn on verbosity
  - 0 to disable verbosity.

  @returns 1 if an exception was cleared, 0 if not, otherwise ERROR;
*/
int
jvmeUnivClearException(int32_t pflag)
{
  int32_t rval = OK;;
  uint32_t pci_csr = 0;
  const uint32_t
    S_TA = (1 << 27),  /* Signalled target-abort */
    BM = (1 << 2),     /* Master enable */
    MS = (1 << 1),     /* Target memory enable */
    IOS = (1 << 0);    /* Target IO enable */

  /* check the PCI Config Space Control/Status Reg...
     clear it, and (pflag==1) put out a warning */

  if(pUniv == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  LOCK_UNIV;
  pci_csr = univRead32(PCI_CSR);

  if (pci_csr & (S_TA))
    {
      if(pflag==1)
	{
	  printf("%s: INFO: Clearing VME Exception.\n",
		 __func__);

	}
      univWrite32(PCI_CSR, S_TA |        /* reset */
		  BM | MS | IOS);        /* Make sure the chip stays up */
      rval = 1;
    }
  UNLOCK_UNIV;

  return rval;
}

/*!
  Routine to reset the DMA Engine

  @param pflag
  - 1 to turn on verbosity
  - 0 to disable verbosity.

  @returns 1 if an error bit was cleared, 0 if not, otherwise ERROR;
*/
int
jvmeUnivDmaReset(int32_t pflag)
{
  int32_t rval = OK;;
  uint32_t dgcs = 0;
  uint32_t resetMask =
    CA91CX42_DGCS_STOP | CA91CX42_DGCS_HALT | CA91CX42_DGCS_DONE |
    CA91CX42_DGCS_LERR | CA91CX42_DGCS_VERR | CA91CX42_DGCS_PERR;
  uint32_t errMask =
    CA91CX42_DGCS_LERR | CA91CX42_DGCS_VERR | CA91CX42_DGCS_PERR;


  if(pUniv == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  LOCK_UNIV;
  dgcs = univRead32(DGCS);

  if (dgcs & errMask)
    {
      if(pflag==1)
	{
	  printf("%s: INFO: Clearing DMA Errors (0x%x).\n",
		 __func__, dgcs & errMask);

	}
      rval = 1;
    }

  univWrite32(DGCS, resetMask); /* reset */
  UNLOCK_UNIV;

  return rval;
}

/*!
  Routine to change the address modifier of the A24 Outbound VME Window.
  The A24 Window must be opened, prior to calling this routine.

  @param addr_mod Address modifier to be used.  If 0, the default (0x39) will be used.

  @return 0, if successful. -1, otherwise
*/
int
jvmeUnivSetA24AM(int32_t addr_mod)
{
  uint32_t reg = 0, vas = 0, en = 0;
  int32_t iwin = 0;
  static int32_t a24_window_number = -1;
  int32_t user1_window_number = -1;

  LOCK_UNIV;

  /* First time this routine is called, it should not know the a24 window number */
  if(a24_window_number == -1)
    {
      /* Loop through LSIs to determine which window we are changing */
      for(iwin = 0; iwin < 8; iwin++)
	{
	  /* Check if window is enabled */
	  en = univRead32(CA91CX42_LSI_CTL[iwin]) & CA91CX42_LSI_CTL_EN;

	  if(en)
	    {
	      vas = univRead32(CA91CX42_LSI_CTL[iwin]) & CA91CX42_LSI_CTL_VAS_M;

	      if(vas == CA91CX42_LSI_CTL_VAS_A24)
		a24_window_number = iwin;

	      /* Get this info, just in case */
	      if(vas == CA91CX42_LSI_CTL_VAS_USER1)
		user1_window_number = iwin;
	    }
	}

      if(a24_window_number == -1)
	{
	  /* Missed it */
	  /* Check if we got a user1 window instead */
	  if(user1_window_number != -1)
	    {
	      printf("%s: WARNING: Did not determine A24 window. Using found USER1 window (%d).\n",
		     __func__, user1_window_number);
	      a24_window_number = user1_window_number;
	    }
	  else
	    {
	      printf("%s: ERROR: Could not determine A24 window\n",
		     __func__);
	      UNLOCK_UNIV;
	      return ERROR;
	    }
	}
    }

  if(addr_mod > 0)
    {
      /* Set the User AM code register for User1 */
      reg = univRead32(USER_AM) & 0x00FF0000;
      univWrite32(USER_AM, reg | ((addr_mod & 0x3F) << 26));

      /* Set the VAS USER1 bits for the LSI window id */
      reg = univRead32(CA91CX42_LSI_CTL[a24_window_number]) & ~CA91CX42_LSI_CTL_VAS_M;

      univWrite32(CA91CX42_LSI_CTL[a24_window_number],
		  reg | CA91CX42_LSI_CTL_VAS_USER1);

    }
  else
    {
      /* Switch the USER1 window back to A24 */
      /* Set the VAS A24 bits for the LSI window id */
      reg = univRead32(CA91CX42_LSI_CTL[a24_window_number]) & ~CA91CX42_LSI_CTL_VAS_M;

      univWrite32(CA91CX42_LSI_CTL[a24_window_number],
		  reg | CA91CX42_LSI_CTL_VAS_A24);

    }

  UNLOCK_UNIV;

  return OK;
}


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

  @return 0, if successful. -1, otherwise.
*/
int
jvmeUnivDmaConfig(uint32_t addrType, uint32_t dataType)
{
  int32_t status = OK;
  /* Initialize the descriptor DCTL
     - Set for 64bit PCI transfers */
  dmaDescSample.dctl = univRead32(DCTL) | CA91CX42_DCTL_LD64EN;

  switch(addrType)
    {
    case 0: /* A16 */
      dmaDescSample.dctl =
	(dmaDescSample.dctl & ~CA91CX42_DCTL_VAS_M) | CA91CX42_DCTL_VAS_A16;
      break;

    case 1: /* A24 */
      dmaDescSample.dctl =
	(dmaDescSample.dctl & ~CA91CX42_DCTL_VAS_M) | CA91CX42_DCTL_VAS_A24;
      break;

    case 2: /* A32 */
      dmaDescSample.dctl =
	(dmaDescSample.dctl & ~CA91CX42_DCTL_VAS_M) | CA91CX42_DCTL_VAS_A32;
      break;

    default:
      printf("%s: Invalid addrType (%d)\n",
	     __func__, addrType);
      status = ERROR;
    }

  switch(dataType)
    {
    case 0: /* D16 */
      dmaDescSample.dctl =
	(dmaDescSample.dctl & ~(CA91CX42_DCTL_VDW_M | CA91CX42_DCTL_VCT_M))
	| CA91CX42_DCTL_VDW_D16;
      break;

    case 1: /* D32 */
      dmaDescSample.dctl =
	(dmaDescSample.dctl & ~(CA91CX42_DCTL_VDW_M | CA91CX42_DCTL_VCT_M))
	| CA91CX42_DCTL_VDW_D32;
      break;

    case 2: /* BLT */
      dmaDescSample.dctl =
	(dmaDescSample.dctl & ~(CA91CX42_DCTL_VDW_M | CA91CX42_DCTL_VCT_M))
	| (CA91CX42_DCTL_VDW_D32 | CA91CX42_DCTL_VCT_BLT);
      break;

    case 3: /* MBLT */
      dmaDescSample.dctl =
	(dmaDescSample.dctl & ~(CA91CX42_DCTL_VDW_M | CA91CX42_DCTL_VCT_M))
	| (CA91CX42_DCTL_VDW_D64 | CA91CX42_DCTL_VCT_BLT);
      break;

    default:
      printf("%s: Invalid dataType (%d).  Setting to MBLT.\n",
	     __func__, dataType);
      dmaDescSample.dctl =
	(dmaDescSample.dctl & ~(CA91CX42_DCTL_VDW_M | CA91CX42_DCTL_VCT_M))
	| (CA91CX42_DCTL_VDW_D64 | CA91CX42_DCTL_VCT_BLT);

      status = ERROR;
    }

  return status;
}

/*!
  Routine to initiate a DMA

  @param locAdrs Destination Userspace address
  @param vmeAdrs VME Bus source address
  @param size    Maximum size of the DMA in bytes

  @return 0, if successful. -1, otherwise.
*/
int
jvmeUnivDmaSend(u_long locAdrs, uint32_t vmeAdrs, int32_t size)
{
  long offset = 0;
  int32_t tmp_dgcs=0;
  int32_t nbytes=0;

  dma_timer[0] = rdtsc();

  if(pUniv == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

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

  /* Clear any previous exception */
  jvmeUnivDmaReset(1);

  /* Local addresses (in Userspace) need to be translated to the physical memory */
  /* Here's the offset between current buffer position and the event head */
  offset = locAdrs - the_event->partBaseAdr;
  dmaDescSample.dla = (the_event->physMemBase + offset) & 0xFFFFFFFF;

#ifdef ARCH_x86_64
  if((the_event->physMemBase + offset)>>32 > 0)
    {
      printf("%s: ERROR: Universe II cannot handle memory address (0x%lx)\n",
	     __func__, (the_event->physMemBase + offset));
      return ERROR;
    }
#endif

#ifdef DEBUG_DMA
  printf("%s:\n",__func__);
  printf("locAdrs     = 0x%lx   partBaseAdr = 0x%lx\n",
	 locAdrs, the_event->partBaseAdr);
  printf("physMemBase = 0x%lx   offset      = 0x%lx\n\n",
	 the_event->physMemBase,offset);
#endif

  /* Source (VME) address */
  dmaDescSample.dva = vmeAdrs;

  /* Calculate the bytes left in this buffer, leaving space for 2KB (0x800) flush */
  nbytes = the_event->part->size - sizeof(DMANODE);
  nbytes = (nbytes - ((long)dma_dabufp - (long)&(the_event->length))) - 0x800;

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
      printf("%s: WARNING: Specified number of DMA bytes (%d) is greater than \n",
	     __func__,
	     size);
      printf("\tthe space left in the buffer (%d).  Using %d\n",nbytes,nbytes);
    }
  else if( (size !=0) && (size<=nbytes) )
    {
      nbytes = size;
    }

  dmaDescSample.dtbc = nbytes;

  dma_timer[1] = rdtsc();

  LOCK_UNIV;
  /* VME Address */
  univWrite32(DVA, dmaDescSample.dva);

  /* PCI Address */
  univWrite32(DLA, dmaDescSample.dla);

  /* VME attributes (filled in with jvmeUnivDmaConfig)*/
  univWrite32(DCTL, dmaDescSample.dctl);

  /* Data count */
  univWrite32(DTBC, dmaDescSample.dtbc);

  /* Some defaults hardcoded for the DMA Control Register
     VON = 2048 bytes, VOFF = 0 microseconds
     No interrupts enabled
  */
  tmp_dgcs  = (0x4 << 20);

#ifdef DEBUG_DMA
  printf("%s ...\n", __func__);
  printf("  vmeAdrs = 0x%08x  dmaDescSample.dva = 0x%08x\n",
	 vmeAdrs, dmaDescSample.dva);
  UNLOCK_UNIV;
  jvmeUnivReadDMARegs();
  LOCK_UNIV;
#endif

  /* GO! */
  tmp_dgcs |=  CA91CX42_DGCS_GO;
  univWrite32(DGCS, tmp_dgcs);

  UNLOCK_UNIV;

#ifdef DEBUG_DMA
  printf("%s ...\n", __func__);
  printf("  vmeAdrs = 0x%08x  dmaDescSample.dva = 0x%08x\n",
	 vmeAdrs, dmaDescSample.dva);
  jvmeUnivReadDMARegs();
#endif

  dma_timer[2] = rdtsc();

  return OK;
}

/*!
  Routine to initiate a DMA using physical memory address (instead of userspace address)

  @param physAdrs Destination Physical Memory Address
  @param vmeAdrs VME Bus source address
  @param size    Maximum size of the DMA in bytes

  @return 0, if successful. -1, otherwise.
*/
int
jvmeUnivDmaSendPhys(u_long physAdrs, uint32_t vmeAdrs, int32_t size)
{
  int32_t tmp_dgcs=0;
  int32_t nbytes=0;

  dma_timer[0] = rdtsc();

  if(pUniv == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  if(!physAdrs)
    {
      printf("%s: ERROR: DMA Physical Memory has an invalid base address (0x%lx)",
	     __func__,
	     (u_long)physAdrs);
      return ERROR;
    }

  /* Correction : they must be on the same byte boundary  DJA */
  if((physAdrs & 0x7) != (vmeAdrs & 0x7))
    {
      printf("%s: ERROR: physAdrs(0x%lx) and vmeAdrs(0x%08x) not on same byte boundary\n",
	     __func__, physAdrs, vmeAdrs);
      return ERROR;
    }

  /* Clear any previous exception */
  jvmeUnivDmaReset(1);

  dmaDescSample.dla = (physAdrs) & 0xFFFFFFFF;

#ifdef ARCH_x86_64
  if((physAdrs)>>32 > 0)
    {
      printf("%s: ERROR: Universe II cannot handle memory address (0x%lx)\n",
	     __func__, physAdrs);
      return ERROR;
    }
#endif

  /* Source (VME) address */
  dmaDescSample.dva = vmeAdrs;

  nbytes = size;

  /* Make sure nbytes is realistic */
  if((nbytes<0) || (nbytes > 0x1000000))
    {
      printf("%s: ERROR: Invalid DMA Size (%d).\n",
	     __func__,nbytes);
      return ERROR;
    }

  dmaDescSample.dtbc = nbytes;

  dma_timer[1] = rdtsc();

  LOCK_UNIV;
  /* VME Address */
  univWrite32(DVA, dmaDescSample.dva);

  /* PCI Address */
  univWrite32(DLA, dmaDescSample.dla);

  /* VME attributes (filled in with jvmeUnivDmaConfig)*/
  univWrite32(DCTL, dmaDescSample.dctl);

  /* Data count */
  univWrite32(DTBC, dmaDescSample.dtbc);

  /* Some defaults hardcoded for the DMA Control Register
     VON = 2048 bytes, VOFF = 0 microseconds
     No interrupts enabled
  */
  tmp_dgcs  = (0x4 << 20);

  /* GO! */
  tmp_dgcs |=  CA91CX42_DGCS_GO;

  univWrite32(DGCS, tmp_dgcs);

  UNLOCK_UNIV;

#ifdef DEBUG_DMA
  printf("%s ...\n", __func__);
  jvmeUnivReadDMARegs();
#endif

  dma_timer[2] = rdtsc();

  return OK;
}

/*!
  Routine to poll for a DMA Completion or timeout.

  @return Number of bytes transferred, if successful, -1, otherwise.
*/
int
jvmeUnivDmaDone(int32_t pcnt)
{
  int32_t icnt = 0;
  uint32_t univTemp, dMask, errMask;

  dmaBerrStatus=0;

  dma_timer[3] = rdtsc();

  /*  Wait for the results of the DMA  Done or Error */
  dMask = CA91CX42_DGCS_DONE | CA91CX42_DGCS_LERR |
    CA91CX42_DGCS_VERR | CA91CX42_DGCS_PERR;

  /* Mask of DMA error condition bits */
  errMask = CA91CX42_DGCS_LERR | CA91CX42_DGCS_VERR | CA91CX42_DGCS_DONE
    | CA91CX42_DGCS_HALT | CA91CX42_DGCS_STOP;

  LOCK_UNIV;

  /* Wait forever */
  if(pcnt <= 0)
    icnt = pcnt - 1;

  while((icnt < pcnt))
    {
      univTemp = univRead32(DGCS);
      if(univTemp & dMask)
	{
	  if((univTemp & CA91CX42_DGCS_DONE) == CA91CX42_DGCS_DONE)
	    {
	      univDmaStat.finalStatus = 0;
	    }
	  else
	    {
	      univDmaStat.finalStatus = univTemp & errMask;
	    }
	  break;
	}
      else
	{
	  /* Check if the DMA engine is even active */
	  if(univTemp & CA91CX42_DGCS_ACT)
	    {
	      if(icnt <= 0) /* wait forever */
		continue;
	      else
		icnt++;
	    }
	  else
	    {
	      /* Ignore missing ACT bit, if Linked List bit is set */
	      if((univTemp & CA91CX42_DGCS_CHAIN) == 0)
		{
		  printf("%s: ERROR: DMA is not Active (0x%08x)\n",
			 __func__, univTemp);
		  UNLOCK_UNIV;
		  return ERROR;
		}
	    }
	}
    }

  UNLOCK_UNIV;
  /* If we timed out then something is wrong - reset - return an error */
  if((pcnt > 0) && (icnt >= pcnt))
    {
      printf("%s: ERROR: DMA timed out. Universe Status = 0x%08x\n",
	     __func__, univTemp);
      jvmeUnivDmaReset(0);
      return ERROR;
    }

  dma_timer[4] = rdtsc();

#ifdef DEBUG_DMA
  printf("%s ...\n", __func__);
  jvmeUnivReadDMARegs();
#endif


  /*  If the DMA terminated with an error, save the addresses and byte count */
  univDmaStat.finalLadr = univRead32(DLA);
  univDmaStat.finalVadr = univRead32(DVA);
  univDmaStat.finalCount = univRead32(DTBC);

  dma_timer[5] = rdtsc();

  switch (univDmaStat.finalStatus)
    {
    case 0:
#ifdef DEBUG_DMA
      printf("%s ...\n", __func__);
      printf("%s: INFO: DMA Complete: poll count %d\n",__func__,icnt);
#endif
      dmaBerrStatus = 0;
      return (dmaDescSample.dtbc);

    case CA91CX42_DGCS_VERR:
      dmaBerrStatus = 1;
#ifdef DEBUG_DMA
      printf("%s ...\n", __func__);
      printf("%s: INFO: DMA Complete: poll count %d  finalCount = 0x%x (%d)\n",
	     __func__,icnt, univDmaStat.finalCount, univDmaStat.finalCount);
#endif
      jvmeUnivDmaReset(0);
      return (dmaDescSample.dtbc - univDmaStat.finalCount);

    default:
      printf("%s: ERROR: DMA Status=0x%08x, Local Addr=0x%08x, "
	     "VME Addr=0x%08x, Bytes Remaining=0x%08x\n",
	     __func__,
	     univDmaStat.finalStatus, univDmaStat.finalLadr,
	     univDmaStat.finalVadr, univDmaStat.finalCount);
      dmaBerrStatus = 1;
      return ERROR;
    }

  return OK;
}


int
jvmeUnivGetBerrStatus()
{
  return dmaBerrStatus;
}


int32_t
jvmeUnivSetupDmaLLBuffer(u_long vaddr, u_long paddr)
{
  dmaListMap = (uint32_t *)vaddr;
  dmaListAdr = paddr;

  return 0;
}


/*!
  Routine to setup a DMA Linked List

  @param locAddrBase Userspace Destination address
  @param *vmeAddr    Array of VME Source addresses
  @param *dmaSize    Array of sizes of each DMA, in bytes
  @param numt        Nuumber of DMA (also size of the arrays)

  @return 0, if successful.  -1, otherwise.
*/

int
jvmeUnivDmaSetupLL(u_long locAddrBase, uint32_t *vmeAddr,
		   uint32_t *dmaSize, uint32_t numt)
{
  u_long localAddr[CA91CX42_DMA_MAX_LL];
  u_long offset=0;
  uint32_t total_words=0;
  uint32_t size_incr=0; /* "current" size of the entire DMA */
  int32_t idma;

  /* check to see if the linked list structure array has been init'd */
  if (dmaListMap == NULL)
    {
      printf("%s: ERROR: DMA Linked-List Memory not initialized\n",
	     __func__);
      return ERROR;
    }

  if(!the_event)
    {
      printf("%s: ERROR: the_event pointer is invalid!\n",
	     __func__);
      return ERROR;
    }

  dmaLL_totalwords=0;
  dmaLLInvalid = 0;

  dmaDescList = (ca91cx42DmaDescriptor_t *) dmaListMap;

  /* Calculate localAddr array values based on buffer partition base, current location
     of dma_dabufp and the dma size for each linked list member */
  offset = locAddrBase - the_event->partBaseAdr;
  localAddr[0] = the_event->physMemBase + offset;

  total_words = dmaSize[0]>>2;

  for (idma=0;idma<numt;idma++)
    {
      if(idma!=0)
	{
	  localAddr[idma] = localAddr[idma-1] + dmaSize[idma-1];
	  total_words += (dmaSize[idma]>>2);
	}

      /* Set the vme (source) address */
      dmaDescList[idma].dva = vmeAddr[idma];

      /* Set the local (destination) address */
#ifdef ARCH_x86_64
      if((localAddr[idma] >> 32) > 0)
	{
	  printf("%s: ERROR: Universe II cannot handle memory address (0x%lx)\n",
		 __func__, (localAddr[idma] >> 32));
	  return ERROR;
	}
#endif

      dmaDescList[idma].dla = localAddr[idma] & 0xFFFFFFFF;

      /* Set the byte count */
      dmaDescList[idma].dtbc = dmaSize[idma];
      size_incr += dmaSize[idma];

      /* Set the address of the next descriptor (DMA command packet pointer)*/
      if( idma < (numt-1) )
	{
	  dmaDescList[idma].dcpp =
	    LSWAP(dmaListAdr + (idma+1)*sizeof(ca91cx42DmaDescriptor_t));
	}
      else
	{
	  /* Last Command Packet */
	  dmaDescList[idma].dcpp = (1 << 0);
	}

      /* Get the DCTL from how was made from jvmeUnivDmaConfig */
      dmaDescList[idma].dctl = dmaDescSample.dctl;

/* #define DEBUGLL */
#ifdef DEBUGLL
      printf("dmaDescList[%d]:\n",idma);
      printf("locAddrBase = 0x%lx\n",(u_long)locAddrBase);
      printf("  DCTL = 0x%08x  DTBC = 0x%08x  DLA = 0x%08x\n",
	     dmaDescList[idma].dctl,
	     dmaDescList[idma].dtbc,
	     dmaDescList[idma].dla);
     printf(" DVA = 0x%08x  DCPP = 0x%08x\n",
	    dmaDescList[idma].dva,
	    dmaDescList[idma].dcpp);
#endif

    }
  dmaLL_totalwords=total_words;



  return OK;
}

/*!
  Routine to initiate a linked list DMA that was setup with vmeDmaSetupLL()
  @return 0, if successful.  -1, otherwise.
*/

int
jvmeUnivDmaSendLL()
{
  uint32_t dgcs = 0x0;

  if(dmaLLInvalid)  /* Set in jlabgefDmaSetupLL */
    {
      return ERROR;
    }

  if(pUniv == NULL)
    {
      printf("%s: ERROR: No MAP to VME bridge\n", __func__);
      return ERROR;
    }

  LOCK_UNIV;
  /* Write the address of the first entry in the linked list to DCPP */
  univWrite32(DCPP, dmaListAdr);

  /* Some defaults hardcoded for the DMA Control Register
     VON = 2048 bytes, VOFF = 0 microseconds
     No interrupts enabled
     Linked-List (DMA Chaining)
  */
  dgcs  = (0x4 << 20) | CA91CX42_DGCS_CHAIN;

  /* GO! */
  dgcs |=  CA91CX42_DGCS_GO;

  univWrite32(DGCS, dgcs);

  UNLOCK_UNIV;

  return OK;
}


void
jvmeUnivReadDMARegs()
{
  uint32_t dctl, dtbc, dla, dva, dcpp, dgcs, d_llue,
    lint_en, lint_stat, lint_map0, lint_map1;

  LOCK_UNIV;
  dctl = univRead32(DCTL);
  dtbc = univRead32(DTBC);
  dla = univRead32(DLA);
  dva = univRead32(DVA);
  dcpp = univRead32(DCPP);
  dgcs = univRead32(DGCS);
  d_llue = univRead32(D_LLUE);
  lint_en = univRead32(LINT_EN);
  lint_stat = univRead32(LINT_STAT);
  lint_map0 = univRead32(LINT_MAP0);
  lint_map1 = univRead32(LINT_MAP1);
  UNLOCK_UNIV;

    printf ("\nCURRENT STATE OF UNIVERSE DMA REGISTERS\n");
    printf ("   DCTL   => 0x%08x\n", dctl);
    printf ("   DTBC   => 0x%08x\n", dtbc);
    printf ("   DLA    => 0x%08x\n", dla);
    printf ("   DVA    => 0x%08x\n", dva);
    printf ("   DCPP   => 0x%08x\n", dcpp);
    printf ("   DGCS   => 0x%08x\n", dgcs);
    printf (" D_LLUE   => 0x%08x\n", d_llue);
    printf ("LINT_EN   => 0x%08x\n", lint_en);
    printf ("LINT_STAT => 0x%08x\n", lint_stat);
    printf ("LINT_MAP0 => 0x%08x\n", lint_map0);
    printf ("LINT_MAP1 => 0x%08x\n", lint_map1);

    printf("DMA Status=0x%08x, Local Addr=0x%08x, VME Addr=0x%08x, Bytes Remaining=0x%08x\n",
	   univDmaStat.finalStatus,
	   univDmaStat.finalLadr,
	   univDmaStat.finalVadr,
	   univDmaStat.finalCount);

  return;
}
