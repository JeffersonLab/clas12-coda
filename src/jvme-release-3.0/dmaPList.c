/*----------------------------------------------------------------------------*
 *  Copyright (c) 2009        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Authors: David Abbott                                                   *
 *             abbottd@jlab.org                  Jefferson Lab, MS-12B3       *
 *             Phone: (757) 269-7190             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *             Bryan Moffit                                                   *
 *             moffit@jlab.org                   Jefferson Lab, MS-12B3       *
 *             Phone: (757) 269-5660             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *     Library for a memory allocation system
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cmem/cmem_rcc.h"

#include "dmaPList.h"

/* Maximum size that can be allocated for one DMA partition in Linux = 4 MB */
#define LINUX_MAX_PARTSIZE 0x400000

pthread_mutex_t   partMutex = PTHREAD_MUTEX_INITIALIZER;
#define PARTLOCK     if(pthread_mutex_lock(&partMutex)<0) perror("pthread_mutex_lock");
#define PARTUNLOCK   if(pthread_mutex_unlock(&partMutex)<0) perror("pthread_mutex_unlock");


#define maximum(a,b) (a<b ? b : a)

/* global data */
static DMALIST  dmaPList;     /* global part list */
static int useSlaveWindow=0;  /* decision to use (1) a Slave VME window (useful for SFI) */
extern void *a32slave_window; /* global variable from jlabgef.c */
extern int a32slave_physmembase;
/** Flag for turning off or on driver verbosity
    \see vmeSetQuietFlag()
*/
extern uint32_t vmeQuietFlag;

/*! Buffer node pointer */
DMANODE *the_event;
/*! Data pointer */
uint32_t *dma_dabufp;

/*!
  Structure to hold information on Physical Memory allocated regions
*/
typedef struct
{
  u_long base;
  u_long width;
} DMA_PHYSMEM_INFO;

/*! Pointer to array of info for allocated regions */
static DMA_PHYSMEM_INFO *partPhysMem = NULL;
/*! Size of @partPhysMem */
static int npartPhysMem = 0;

/*!
  Routine to allow for using a Slave VME window (opened with vmeOpenSlaveA32)
  for the DMA buffer.
  This routine MUST be called before initializing DMA buffers, if the Slave
  VME Window physical memory is to be used.

  @param iFlag
  - 0 to use regular physical memory
  - 1 to use physical memory already mapped to a VME Slave Window

  @return OK if successful, ERROR on error
*/
int
dmaPUseSlaveWindow (int iFlag)
{
  /* Check argument */
  if((iFlag < 0) || (iFlag > 1))
    {
      printf("%s: ERROR: Invalid iFlag (%d).  Must be 0 or 1.\n",
	     __func__,iFlag);
      return -1;
    }

  if(iFlag == 0) /* Don't use Slave Window.. */
    {
      useSlaveWindow=0;
      return 0;
    }

  if(iFlag == 1) /* Use Slave Window */
    {
      /* Check if window was already opened and initialized */
      if(a32slave_window==NULL)
	{
	  useSlaveWindow=0;
	  printf("%s: ERROR: Slave Window has not been initialized.\n",
		 __func__);
	  return -1;
	}

      useSlaveWindow=1;
      return 0;
    }

  return -1; /* Shouldn't get here, anyway */
}


void
dmaPartInit()
{
  dmalistInit(&dmaPList);

}

/*!
  Create and initialize a memory partition

  @param *name Name of the new partition
  @param size  Size of a single item
  @param c     Initial number if items
  @param incr  Number of items to add when enlarging

  @return Created memory partition
*/
DMA_MEM_ID
dmaPCreate(char *name, int size, int c, int incr)
{
  DMA_MEM_ID pPart;
  /*   int c_alloc = 0; */
  int size_incr = 0;

  CMEM_Error_code_t cmem_ret;

  /* Open access to cmem device, multiple opens is ok */
  cmem_ret = CMEM_Open();
  if(cmem_ret != 0)
    {
      printf("%s: ERROR: CMEM_Open returned %d\n",
	     __func__, cmem_ret);
      return NULL;
    }

  /* Allocate memory to hold partition info */
  pPart = (DMA_MEM_PART *) malloc(sizeof(DMA_MEM_PART));
  if(pPart == NULL)
    {
      perror("malloc");
      return NULL;
    }

  memset ((char *)pPart,0,sizeof(DMA_MEM_PART));

  /* Initialize holder of physical memory regions */
  if(c > 0)
    {
      partPhysMem  = (DMA_PHYSMEM_INFO *)malloc(c*sizeof(DMA_PHYSMEM_INFO));
      npartPhysMem = 0;
    }

  if (pPart != NULL)
    {
      dmalistInit (&(pPart->list));

      /* Check if the size needs to be increased to put the partition on
	 an 8 byte boundary */
      if((size + sizeof(DMANODE))%8 != 0)
	size_incr = 8-(size + sizeof(DMANODE))%8;

      pPart->size = size + sizeof(DMANODE) + size_incr;
      pPart->incr = 0;
      pPart->total = 0;

      strcpy(pPart->name, name);
      if (name && strlen(name) == 0)
	pPart->name[0] = 0;
      dmalistAdd (&dmaPList, (DMANODE *)pPart);
      if((dmaPIncr (pPart, c)) != c)
	return (0);
    }
  return pPart;
}

/*!
  Routine to find a memory partition based on its name

  @param *name Name of the partition to find

  @return Pointer to found memory partition, if successful.
*/
DMA_MEM_ID
dmaPFindByName(char *name)
{
  DMA_MEM_ID	pPart;

  pPart = (DMA_MEM_ID) dmalistFirst (&dmaPList);

  while (pPart != NULL)
    {
      if (pPart->name && strcmp(pPart->name, name) == 0)break;
      pPart = (DMA_MEM_ID) dmalistNext ((DMANODE *)pPart);
    }
  return (pPart);
}


/*!
  Frees all nodes for a given memory part and removes part from global
  part list.

  @param pPart Memory parition
*/
void
dmaPFree(DMA_MEM_ID pPart)
{
  DMANODE *the_node;
  CMEM_Error_code_t cmem_ret;

  if(pPart == NULL) return;

  if(!vmeQuietFlag)
    printf("%s: free list %s\n",
	   __func__, pPart->name);

  if (pPart->incr == 1)
    {
      /* Free all buffers in the partition individually */
      while (pPart->list.c)
	{
	  dmalistGet(&(pPart->list),the_node);
	  cmem_ret = CMEM_SegmentFree(the_node->cmem_segment);
	  if(cmem_ret != 0)
	    {
	      printf("%s: ERROR: returned from CMEM_SegmentFree(segment=%d)\n",
		     __func__, the_node->cmem_segment);
	    }

	  the_node = (DMANODE *)0;
	}
      dmalistSnip (&dmaPList, (DMANODE *)pPart);
    }
  else
    {
      /* Just need to Free the one contiguous block of data */
      dmalistSnip (&dmaPList, (DMANODE *)pPart);
      dmalistGet(&(pPart->list),the_node);
      if(the_node)
	{
	  if(!useSlaveWindow)
	    {
	      cmem_ret = CMEM_SegmentFree(the_node->cmem_segment);
	      if(cmem_ret != 0)
		{
		  printf("%s: ERROR: returned from CMEM_SegmentFree(segment=%d)\n",
			 __func__, the_node->cmem_segment);
		}
	    }
	}

    }

  free(pPart);
  free(partPhysMem);
  partPhysMem = NULL;
  npartPhysMem = 0;

  pPart = 0;
}


/*!
  Frees all memory parts in global part list and frees all nodes for a
  given list.
*/
void
dmaPFreeAll()
{
  DMA_MEM_ID	pPart = (DMA_MEM_ID) 0;

  if (dmalistCount(&dmaPList))
    {
      pPart = (DMA_MEM_ID) dmalistFirst (&dmaPList);
      while (pPart != NULL)
	{
	  dmaPFree(pPart);
	  pPart = (DMA_MEM_ID) dmalistFirst (&dmaPList);
	}
    }

  CMEM_Error_code_t cmem_ret;

  /* Open access to cmem device */
  cmem_ret = CMEM_Close();
  if((cmem_ret != 0) && (cmem_ret != CMEM_RCC_NOTOPEN))
    {
      printf("%s: ERROR: CMEM_Close returned %d\n",
	     __func__, cmem_ret);
      return;
    }
}



/*!
  Routine to increase a partition size.

  @param pPart Memory parition to increase
  @param c     Minimum number of items to add.

  @return Number of items added, if successful, -1, otherwise.
*/
int
dmaPIncr(DMA_MEM_ID pPart, int c)
{

  register char *node;
  register u_long *block;
  /*   unsigned bytes; */
  int total_bytes;
  int actual = c;		/* actual # of items added */
  CMEM_Error_code_t cmem_ret;
  u_long cmem_paddr = 0;
  u_long cmem_vaddr = 0;
  int cmem_segment = 0;

  pPart->total += c;

  if ((pPart == NULL)||(c == 0)) return (0);

  total_bytes =  c * pPart->size;

  if(LINUX_MAX_PARTSIZE <= total_bytes)
    {
      if(useSlaveWindow)
	{
	  printf("%s: ERROR:  Unable to create memory partition for Slave Window.\n",
		 __func__);
	  printf("  Requested partition size (%d) is larger than max allowed (%d)\n",
		 total_bytes,LINUX_MAX_PARTSIZE);
	  return -1;
	}

      if(!vmeQuietFlag)
	printf("%s: Creating a fragmented memory partition.\n",__func__);

      if(LINUX_MAX_PARTSIZE < pPart->size)
	{
	  printf("%s: ERROR: Requested partition size (%d) is larger than max allowed (%d)\n",
		 __func__,pPart->size,LINUX_MAX_PARTSIZE);
	  return (-1);
	}
      else
	{
	  pPart->incr = 1; /* Create a Fragmented memory Partition */

	  while (actual--)
	    {
	      /* Allocate memory for individual buffer */
	      cmem_ret =
		CMEM_GFPBPASegmentAllocate(pPart->size, (char *)"jvme_dma",
					   &cmem_segment);

	      if(cmem_ret != 0)
		{
		  printf("%s: CMEM_GFPBPASegmentAllocate returned %d. size = 0x%x\n",
			 __func__, cmem_ret, pPart->size);
		  return -1;
		}

	      cmem_ret = CMEM_SegmentPhysicalAddress(cmem_segment, &cmem_paddr);

	      if (!cmem_ret)
		{
		  cmem_ret = CMEM_SegmentVirtualAddress(cmem_segment, &cmem_vaddr);
		}
	      else
		{
		  return -1;
		}


	      block = (u_long *) cmem_vaddr;

	      if (block == NULL)
		{
		  return (-1);
		}

	      memset((char *) block, 0, pPart->size);

	      ((DMANODE *)block)->part = pPart; /* remember where we came from... */
	      ((DMANODE *)block)->cmem_segment = cmem_segment;
	      ((DMANODE *)block)->partBaseAdr = cmem_vaddr;
	      ((DMANODE *)block)->physMemBase = cmem_paddr;

	      dmalistAdd (&pPart->list,(DMANODE *)block);
	    }
	  return (c);
	}
    }
  else /* Single memory block for data */
    {
      if(useSlaveWindow)
	{
	  block = (u_long *)a32slave_window;
	  cmem_paddr = a32slave_physmembase;
	}
      else
	{
	  /* Allocate memory for all buffers */
	  cmem_ret =
	    CMEM_GFPBPASegmentAllocate(total_bytes, (char *)"jvme_dma",
				       &cmem_segment);

	  if(cmem_ret != 0)
	    {
	      printf("%s: CMEM_GFPBPASegmentAllocate returned %d. size = 0x%x\n",
		     __func__, cmem_ret, total_bytes);
	      return -1;
	    }

	  cmem_ret = CMEM_SegmentPhysicalAddress(cmem_segment, &cmem_paddr);

	  if (!cmem_ret)
	    {
	      cmem_ret = CMEM_SegmentVirtualAddress(cmem_segment, &cmem_vaddr);
	    }
	  else
	    {
	      return -1;
	    }

	  block = (u_long *) cmem_vaddr;
	}

      if (block == NULL)
	{
	  printf("%s: ERROR: Memory Allocator returned NULL\n",
		 __func__);
	  return (-1);
	}

      pPart->incr = 0;
      memset((char *) block, 0, c * pPart->size);

      node = (char *) block;
      *((char **) &pPart->part[0]) = node;

      /* Split large allocation into the individual buffers */
      while (actual--)
	{
	  ((DMANODE *)node)->part = pPart; /* remember where we came from... */
	  ((DMANODE *)node)->cmem_segment = cmem_segment;
	  ((DMANODE *)node)->partBaseAdr = (pPart->part[0]);
	  ((DMANODE *)node)->physMemBase = cmem_paddr;

	  dmalistAdd (&pPart->list,(DMANODE *)node);
	  node += pPart->size;
	}

      return (c);
    }

}



/*****************************************************************
 *
 *  Wrapper routines for DMA Memory Partition list manipulation
 *
 *   dmaPFreeItem:  Free a buffer(node) back to its owner partition
 *   dmaPEmpty   :  Check if a Partition has available nodes
 *   dmaPGetItem :  Get (reserve) the first available  node from a partition
 *   dmaAddItem  :  Add node to a specified partition's list.
 *
 */

/*!
  Free a buffer(node) back to its owner partition

  @param *pItem Buffer(node) to free
*/
void
dmaPFreeItem(DMANODE *pItem)
{
  CMEM_Error_code_t cmem_ret;


  PARTLOCK;
  /* if the node does not have an owner then delete it - otherwise add it back to
     the owner list - lock out interrupts to be safe */
  if ((pItem)->part == 0)
    {
      if(useSlaveWindow)
	{
	  printf("%s: I dont think I should be here... useSlaveWindow==%d",
		 __func__,useSlaveWindow);
	}

      cmem_ret = CMEM_SegmentFree(pItem->cmem_segment);
      if(cmem_ret != 0)
	{
	  printf("%s: ERROR: returned from CMEM_SegmentFree(segment=%d)\n",
		 __func__, pItem->cmem_segment);
	}

      pItem = 0;
    }
  else
    {
      pItem->length=0;
      dmalistAdd (&pItem->part->list, pItem);
    }

  /* execute any command accociated with freeing the buffer */
  if(pItem->part->free_cmd != NULL)
    (*(pItem->part->free_cmd)) (pItem->part->clientData);

  PARTUNLOCK;
}

/*!
  Check if a Partition has available nodes

  @param pPart Partition to check

  @return 1 if empty, otherwise 0.
*/
int
dmaPEmpty(DMA_MEM_ID pPart)
{
  int rval;
  PARTLOCK;
  rval = (pPart->list.c == 0);
  PARTUNLOCK;
  return rval;
}

/*!
  Return the number of available nodes in specified Partition

  @param pPart Partition to check

  @return Number of nodes available.
*/
int
dmaPNodeCount(DMA_MEM_ID pPart)
{
  int rval;
  PARTLOCK;
  rval = pPart->list.c;
  PARTUNLOCK;
  return rval;
}


/*!
  Get (reserve) the first available node from a parititon

  @param pPart Partition to obtain a node

  @return First available node, if successful. 0, otherwise.
*/
DMANODE *
dmaPGetItem(DMA_MEM_ID pPart)
{
  DMANODE *theNode;

  PARTLOCK;
  dmalistGet(&(pPart->list),theNode);
  if(!theNode)
    {
      PARTUNLOCK;
      return 0;
    }

  if(theNode->length > theNode->part->size)
    {
      printf("%s: ERROR:", __func__);
      printf("  Event length (%d) is larger than the Event buffer size (%d).  (Event %d)\n",
	     (int)theNode->length,theNode->part->size,
	     (int)theNode->nevent);
    }
  PARTUNLOCK;
  return(theNode);
}

/*!
  Add node to a specified partition's list

  @param pPart  Partition to add to
  @param *pItem Item to add to partition's list
*/
void
dmaPAddItem(DMA_MEM_ID pPart, DMANODE *pItem)
{

  PARTLOCK;
  dmalistAdd(&(pPart->list),pItem);
  if(pItem->length > pItem->part->size)
    {
      printf("%s: ERROR:", __func__);
      printf("  Event length (%d) is larger than the Event buffer size (%d).  (Event %d)\n",
	     (int)pItem->length,pItem->part->size,
	     (int)pItem->nevent);
    }
  PARTUNLOCK;
}



/*!
  Initialize an existing partition

  @param pPart Parition to initialize

  @return 0, if successful.  -1, otherwise.
*/
int
dmaPReInit(DMA_MEM_ID pPart)
{
  register char *node;
  register DMANODE *theNode;
  int actual;
  u_long oldPhysMemBase = 0;
  u_long oldPartBaseAdr = 0;
  int old_cmem_segment = 0;

  if (pPart == NULL) return -1;

  if (pPart->incr == 1)
    {   /* Does this partition have a Fragmented buffer list */
      /* Check if partition has buffers that do not belong to it
	 and return them to their rightful owners */
      if ((pPart->total == 0) && (dmalistCount(&pPart->list) > 0))
	{
	  while (dmalistCount(&pPart->list) > 0)
	    {
	      dmalistGet(&pPart->list,theNode);
	      dmaPFreeItem(theNode);
	    }
	}

    }
  else
    {
      /* Cheat to initialize memory partition assuming buffers in one
	 contiguous memory bloack */

      /* Get the dma handles (if they exist) before they're erased */
      dmalistGet(&pPart->list,theNode);
      if(theNode)
	{
	  old_cmem_segment = theNode->cmem_segment;
	  oldPhysMemBase = theNode->physMemBase;
	  oldPartBaseAdr = theNode->partBaseAdr;
	}
      else
	{
	  old_cmem_segment = 0;
	}

      memset(*((char **) &pPart->part[0]), 0, pPart->total * pPart->size);

      node = *((char **) &pPart->part[0]);

      actual = pPart->total;

      pPart->list.f = pPart->list.l = (DMANODE *) (pPart->list.c = 0);

      while (actual--)
	{
	  ((DMANODE *)node)->part = pPart; /* remember where we came from... */
	  ((DMANODE *)node)->cmem_segment = old_cmem_segment;
	  ((DMANODE *)node)->physMemBase = oldPhysMemBase;
	  ((DMANODE *)node)->partBaseAdr = oldPartBaseAdr;
	  dmalistAdd (&pPart->list,(DMANODE *)node);
	  node += pPart->size;
	}
    }
  return 0;
}

/*!
  Initialize all existing memory partitions

  @return 0
*/
int
dmaPReInitAll()
{
  DMA_MEM_ID	pPart = (DMA_MEM_ID) 0;

  if (dmalistCount(&dmaPList))
    {
      pPart = (DMA_MEM_ID) dmalistFirst (&dmaPList);
      while (pPart != NULL)
	{
	  dmaPReInit(pPart);
	  pPart = (DMA_MEM_ID) dmalistNext ((DMANODE *) pPart);
	}
    }
  return 0;
}

/***************************************************************
 * dmaPHdr - Print headings for part statitistics printout
 */

static void
dmaPHdr()
{
  printf("Address    ");
#ifdef ARCH_x86_64
  printf("    ");
#endif
  printf("total   free   busy     size  incr  (KBytes)  Name\n");

  printf("----------");
#ifdef ARCH_x86_64
  printf("----");
#endif
  printf(" -----  -----  -----  -------  ----  --------  ----------\n");
}


/***************************************************************
 * dmaPPrint - Print statitistics for a single part
 */

static void
dmaPPrint(DMA_MEM_ID pPart)
{
  int freen;

#ifdef ARCH_x86_64
  printf("0x%012lx ",(u_long)pPart);
#else
  printf("0x%08lx ",(u_long)pPart);
#endif

  if (pPart != NULL)
    {
      freen = dmalistCount (&pPart->list);
      printf("%5d  %5d  %5d  %7d     %1d  (%6d)  %s\n",
	     pPart->total,
	     freen,
	     pPart->total - freen,
	     pPart->size,
	     pPart->incr,
	     (((pPart->total * pPart->size) + 1023) / 1024),
	     pPart->name
	     );
    }
}


/*!
  Print statistics on a memory part

  @param pPart Memory paritition

  @return 0
*/
int
dmaPStats(DMA_MEM_ID pPart)
{
  dmaPHdr ();
  dmaPPrint (pPart);
  return (0);
}


/*!
  Print statistics on all partitions
*/
int
dmaPStatsAll()
{
  DMA_MEM_ID  pPart;

  dmaPHdr ();
  pPart = (DMA_MEM_ID) dmalistFirst (&dmaPList);
  while (pPart != NULL)
    {
      dmaPPrint (pPart);
      pPart = (DMA_MEM_ID) dmalistNext ((DMANODE *)pPart);
    }
  return (0);
}


/*!
  Prints statistics for a given list structure

  @param *admalist List of nodes

  @return 0
*/
int
dmaPPrintList(DMALIST *admalist)
{
  DMANODE *theNode;

  printf("dalist->f         %lx\n",(u_long)admalist->f);
  printf("dalist->l         %lx\n",(u_long)admalist->l);
  printf("dalist->c         %ld\n",(u_long)admalist->c);

  theNode = dmalistFirst(admalist);
  while (theNode)
    {
      printf ("part %lx prev %lx self %lx next %lx left %d fd %d\n",
	      (u_long)theNode->part,
	      (u_long)theNode->p,
	      (u_long)theNode,
	      (u_long)theNode->n,
	      (int)theNode->left,
	      theNode->fd);
      theNode = dmalistNext(theNode);
    }
  return(0);
}

/**

 * @brief Allocate memory accessible to VME bridge to hold dma
 *        descriptors for linked-list DMA
 * @param[in] name The name of the memory parition
 * @param[in] size Size of the memory partition
 * @return Pointer to DMA_CMEM_ID structure containing allocated
 *         physical and virtual addresses
 */
DMA_CMEM_ID
dmaCMemAlloc(char *name, int32_t size)
{
  CMEM_Error_code_t cmem_ret;
  int segment = 0;
  DMA_CMEM_ID cPart;

  /* Open access to cmem device, multiple opens is ok */
  cmem_ret = CMEM_Open();
  if(cmem_ret != 0)
    {
      printf("%s: ERROR: CMEM_Open returned %d\n",
	     __func__, cmem_ret);
      return NULL;
    }

  /* Allocate memory to hold partition info */
  cPart = (DMA_CMEM_PART *) malloc(sizeof(DMA_CMEM_PART));
  if(cPart == NULL)
    {
      perror("malloc");
      return NULL;
    }

  memset((char *)cPart, 0, sizeof(DMA_CMEM_PART));

  cPart->size = size;

  strncpy(cPart->name, name, 40);
  if (name && strlen(name) == 0)
    cPart->name[0] = 0;

  /* Allocate memory for individual buffer */
  cmem_ret =
    CMEM_GFPBPASegmentAllocate(cPart->size, cPart->name, &cPart->segment);
  if(cmem_ret != 0)
    {
      printf("%s: CMEM_GFPBPASegmentAllocate returned %d. size = 0x%x\n",
	     __func__, cmem_ret, size);
      return NULL;
    }

  cmem_ret = CMEM_SegmentPhysicalAddress(cPart->segment, &cPart->paddr);
  if(cmem_ret != 0)
    {
      printf("%s: CMEM_SegmentPhysicalAddress returned %d\n",
	     __func__, cmem_ret);
      return NULL;
    }

  cmem_ret = CMEM_SegmentVirtualAddress(cPart->segment, &cPart->vaddr);
  if(cmem_ret != 0)
    {
      printf("%s: CMEM_SegmentVirtualAddress returned %d\n",
	     __func__, cmem_ret);
      return NULL;
    }

  return cPart;
}


/**
 * @brief Free memory allocated to hold dma descriptors for linked-list DMA
 * @param[in] cPart Pointer to memory structure allocated from @dmaCMemAlloc
 * @return OK if successful, otherwise -1;
 */
int32_t
dmaCMemFree(DMA_CMEM_ID cPart)
{
  if(cPart == NULL)
    return -1;

  if(!vmeQuietFlag)
    printf("%s: free list %s\n",
	   __func__, cPart->name);

  CMEM_Error_code_t cmem_ret = CMEM_SegmentFree(cPart->segment);

  if(cmem_ret != 0)
    {
      printf("%s: ERROR: returned from CMEM_SegmentFree(segment=%d)\n",
	     __func__, cPart->segment);
      return -1;
    }

  free(cPart);
  cPart = 0;

  return 0;
}
