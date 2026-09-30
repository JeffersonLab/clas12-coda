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
  uint32_t data[512/4];

  struct vme_master master;

  printf("Simple VME User Module Test\n");

  fd = open("/dev/bus/vme/m0", O_RDONLY);
  if (fd == -1)
    {
      perror("ERROR: Opening window device file");
      return 1;
    }

  master.enable = 1;
  master.vme_addr = (6<<19);
  master.size = 0x10000;
  master.aspace = 2;		// VME_A24
  master.cycle = 0x2000 | 0x8000;	// user/data access
  master.dwidth = 4;		// 32 bit word access

  retval = ioctl(fd, VME_SET_MASTER, &master);
  if (retval != 0)
    {
      printf("retval=%d\n", retval);
      perror("ERROR: Failed to configure window");
      return 1;
    }

  /*
   * Reading first 512 bytes
   */
  for (i = 0; i < 512/4; i++)
    {
      data[i] = 0;
    }

  retval = pread(fd, data, 512, 0);
  if (retval < 512)
    {
      printf("WARNING: Only read %d bytes", retval);
    }

  for (i = 0; i < (retval/4); i++)
    {
      if (i % 4 == 0)
	{
	  printf("\n" "%4.4x: ", i*4);
	}
      printf("%8.8x ", bswap_32(data[i]));
    }
  printf("\n");

  close(fd);

  return 0;
}
