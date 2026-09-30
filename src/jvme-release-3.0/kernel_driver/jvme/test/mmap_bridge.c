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
  uint32_t *map;

  printf("Simple VME User Module Test\n");

  fd = open("/dev/bus/vme/ctl", O_RDWR);
  if (fd == -1)
    {
      perror("ERROR: Opening window device file");
      return 1;
    }

  map = mmap(0, 512, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (map == MAP_FAILED)
    {
      close(fd);
      perror("Error mmapping the file");
      exit(EXIT_FAILURE);
    }

  for (i = 0; i < (512/4); i++)
    {
      if (i % 4 == 0)
	{
	  printf("\n" "%4.4x: ", i*4);
	}
      printf("%8.8x ", (map[i]));
    }
  printf("\n");

  close(fd);

  return 0;
}
