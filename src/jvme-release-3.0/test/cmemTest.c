/*
 * File:
 *    cmemTest.c
 *
 * Description:
 *    Test cmem driver
 *
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "../cmem/cmem_rcc.h"

typedef struct {
  int32_t segment;
  int32_t size;
  u_long paddr;
  u_long vaddr;
} cmem_t;

cmem_t dma_cmem =
  {
    .segment = -1,
    .size = -1,
    .paddr = 0x0UL,
    .vaddr = 0x0UL
  };



int32_t
cmem_init(int32_t size)
{
  CMEM_Error_code_t cmem_ret = CMEM_Open();

  printf("%s: %s returned %d\n",
	 __func__,
	 "CMEM_Open",
	 cmem_ret);

  cmem_ret = CMEM_Open();

  printf("%s: %s returned %d\n",
	 __func__,
	 "CMEM_Open",
	 cmem_ret);

  if(size <= 0)
    size = 1*1024*1024;

  dma_cmem.size = size;

  printf("%s: allocating cmem size=0x%x (%d)\n",
	 __func__, dma_cmem.size, dma_cmem.size);

  cmem_ret = CMEM_GFPBPASegmentAllocate(dma_cmem.size, (char *)"dam_dma", &dma_cmem.segment);

  if (!cmem_ret)
    {
      cmem_ret = CMEM_SegmentPhysicalAddress(dma_cmem.segment, &dma_cmem.paddr);

      if (!cmem_ret)
	cmem_ret = CMEM_SegmentVirtualAddress(dma_cmem.segment, &dma_cmem.vaddr);
      else
	{
	  return -1;
	}
    }
  else
    {
      dma_cmem.segment = -1;
      dma_cmem.size = -1;
      return -1;
    }

  printf("%s: segment %d: paddr = 0x%lx,   vaddr = 0x%lx\n",
	 __func__, dma_cmem.segment, dma_cmem.paddr, dma_cmem.vaddr);

  /* uint32_t *data = (uint32_t *) dma_cmem.vaddr; */
  /* int32_t idata, ndata = 512+128; */
  /* for(idata = 0; idata < ndata; idata++) */
  /*   printf("%2d: 0x%08x\n", idata, data[idata]); */

  return cmem_ret;
}

int32_t
cmem_close()
{
  int32_t cmem_ret;

  cmem_ret = CMEM_SegmentFree(dma_cmem.segment);
  cmem_ret = CMEM_Close();

  dma_cmem.segment = -1;
  dma_cmem.size = -1;

  return cmem_ret;
}

int
main(int argc, char *argv[])
{
  int stat = 0;

  cmem_init(0);
  printf("pause\n");
  getchar();
  cmem_close();

  exit(0);
}
/*
  Local Variables:
  compile-command: "make -k cmemTest "
  End:
*/
