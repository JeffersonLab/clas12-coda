/* Simple VME User module test.
 *
 * This test opens the first VME master window, configures it as follows:
 * - A24 Address space
 * - Address: 0x30000
 * - Size: 0x10000
 * - Access Mode: User/Data
 * - Mode: 32-bit transfers
 *
 * It then attempts to read and print out the first 512 bytes of data found
 * at this address.
 */
#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <byteswap.h>
typedef uint32_t u32;
#include "jvme.h"
int
main(int argc, char *argv[])
{
  int fd;
  int i;
  int retval;
  void *map;
  struct ti_struct {
    volatile uint32_t blank[(0x300000)/4];
    volatile uint32_t data[512/4];
  };
  
  struct vme_master master;

  printf("Simple VME User Module Test\n");

  fd = open("/dev/bus/vme/m_a24", O_RDWR);
  if (fd == -1)
    {
      perror("ERROR: Opening window device file");
      return 1;
    }

  retval = ioctl(fd, VME_GET_MASTER, &master);
  if (retval != 0)
    {
      printf("retval=%d\n", retval);
      perror("ERROR: Failed to configure window");
      return 1;
    }

  printf(" enable   = %d\n", master.enable);
  printf(" vme_addr = 0x%0llx\n", master.vme_addr);
  printf(" size     = 0x%0llx\n", master.size);
  printf(" aspace   = 0x%08x\n", master.aspace);
  printf(" cycle    = 0x%08x\n", master.cycle);
  printf(" dwidth   = 0x%08x\n", master.dwidth);
  
  map = mmap(0, master.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (map == MAP_FAILED)
    {
      close(fd);
      perror("Error mmapping the file");
      exit(EXIT_FAILURE);
    }

  printf("  map    = 0x%0lx\n",
	 (uint64_t)map);
  struct ti_struct *ti_map = (struct ti_struct *)map;
  printf("  ti_map = 0x%0lx\n",
	 (uint64_t)&ti_map->data[0]);
  
  for (i = 0; i < (512/4); i++)
    {
      if (i % 4 == 0)
	{
	  printf("\n" "%4.4x: ", i*4);
	}
      printf("%8.8x ", bswap_32(ti_map->data[i]));
    }
  printf("\n");

  close(fd);

  return 0;
}
