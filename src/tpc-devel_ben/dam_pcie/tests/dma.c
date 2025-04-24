#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "libdam.h"

extern int usleep(__useconds_t __useconds);

int main(void)
{
    int fd = 0;
    size_t size = (sysconf(_SC_PAGESIZE) * 256 * 3);
    uint32_t *buff = NULL;
    dam_open(&fd, "/dev/dam0");

    int model = dam_register_read(fd, 0x830);
    int map   = dam_register_read(fd, 0x000);
    printf("FLX Model: %i, Reg Map: %i\n", model, map);

    if (ioctl(fd, DEBUG_DMA, NULL) == -1) {
        perror(__func__);
        return -1;
    }

    dam_register_write(fd, 0x8c0, 0xffff);
    usleep(10000);

    printf("DMA_TEST: %x\n", dam_register_read(fd, 0x8c0));

    if (ioctl(fd, DEBUG_DMA, NULL) == -1) {
        perror(__func__);
        return -1;
    }

    dam_close(fd);
    return 0;
}
