/*
 * File:
 *    jvmeDmaTest.c
 *
 * Description:
 *    Test DMA with JLab Vme Driver
 *
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <byteswap.h>
#include "jvme.h"
#include "jvmeControl.h"

typedef struct jvmeControlStruct
{
  int devFile; /** file descriptor for device */
  int bridgeType;		/* Type of bridge */
  void *localPtr;    /** local pointer to vme bridge */
  uint32_t size;    /** Mapped Size of VME bridge */
} jCTRL;

extern jCTRL jCtrl;

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

/* Magic number for use in ioctls */
#define VME_IOC_MAGIC 0xAE

#define VME_DMA_REQ			_IOWR(VME_IOC_MAGIC, 0x0C, dma_t)
#define VME_DMA_REMOVE			_IO(VME_IOC_MAGIC, 0x0D)
#define VME_DMA_NEW_LIST		_IO(VME_IOC_MAGIC, 0x0E)
#define VME_DMA_FREE_LIST		_IO(VME_IOC_MAGIC, 0x0F)
#define VME_DMA_ATTR_SET		_IOR(VME_IOC_MAGIC, 0x10, dma_t)
#define VME_DMA_FREE_ATTR		_IO(VME_IOC_MAGIC, 0x11)
#define VME_DMA_XFER			_IOWR(VME_IOC_MAGIC, 0x12, dma_t)

#define VME_A16		0x1
#define VME_A24		0x2
#define	VME_A32		0x4
#define VME_A64		0x8
#define VME_CRCSR	0x10
#define VME_USER1	0x20
#define VME_USER2	0x40
#define VME_USER3	0x80
#define VME_USER4	0x100

#define VME_A16_MAX	0x10000ULL
#define VME_A24_MAX	0x1000000ULL
#define VME_A32_MAX	0x100000000ULL
#define VME_A64_MAX	0x10000000000000000ULL
#define VME_CRCSR_MAX	0x1000000ULL


/* VME Cycle Types */
#define VME_SCT		0x1
#define VME_BLT		0x2
#define VME_MBLT	0x4
#define VME_2eVME	0x8
#define VME_2eSST	0x10
#define VME_2eSSTB	0x20

#define VME_2eSST160	0x100
#define VME_2eSST267	0x200
#define VME_2eSST320	0x400

#define	VME_SUPER	0x1000
#define	VME_USER	0x2000
#define	VME_PROG	0x4000
#define	VME_DATA	0x8000

/* VME Data Widths */
#define VME_D8		0x1
#define VME_D16		0x2
#define VME_D32		0x4
#define VME_D64		0x8

#define VME_DMA_VME_TO_MEM		(1<<0)
#define VME_DMA_MEM_TO_VME		(1<<1)

int
main(int argc, char *argv[])
{
  int rval = 0;
  dma_t testDma;

  rval = jvmeMapControl();
  if(rval) printf(" jvmeMapControl returned  %d\n", rval);

  testDma.req_route = VME_DMA_VME_TO_MEM;

  rval = ioctl(jCtrl.devFile, VME_DMA_REQ, &testDma);
  if (rval != 0)
    {
      perror("ERROR: Failed to request dma");
    }

  rval = ioctl(jCtrl.devFile, VME_DMA_NEW_LIST, 0);
  if (rval != 0)
    {
      perror("ERROR: Failed to create new dma list");
    }

  printf("Press <Enter> to continue\n");
  getchar();

  testDma.addr = 0x08000000;
  testDma.aspace = VME_A32;
  testDma.cycle = VME_2eSST267 | VME_2eSST;
  testDma.dwidth = 0x8; // not used in VIVO kernel driver?
  testDma.size = 0x80;


  rval = ioctl(jCtrl.devFile, VME_DMA_ATTR_SET, &testDma);
  if (rval != 0)
    {
      perror("ERROR: Failed to create new dma list");
    }

  printf("Press <Enter> to continue\n");
  getchar();

  rval = ioctl(jCtrl.devFile, VME_DMA_XFER, &testDma);
  if (rval != 0)
    {
      perror("ERROR: Failed to DMA Transfer");
    }

  printf("Press <Enter> to continue\n");
  getchar();

  rval = jvmeUnmapControl();
  if(rval) printf(" jvmeUnmapControl returned  %d\n", rval);

  exit(0);
}

/*
  Local Variables:
  compile-command: "make -k -B jvmeDmaTest"
  End:
 */
