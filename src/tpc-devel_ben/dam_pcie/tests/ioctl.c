#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "dam_ioctl.h"

#define DEVNAME "/dev/dam0"

static int my_dam_open(int *fd)
{
    if ( (*fd = open(DEVNAME, O_RDWR)) <= 0 ) {
        perror(__func__);
        return -1;
    }

    return 0;
}

static int my_dam_close(int fd) 
{
    return close(fd);
}

static uint32_t my_dam_register_read(int fd, uint32_t addr)
{
    dam_drv_io_t p;
    p.address = addr;// / sizeof(uint32_t);
    p.data = 0xbeef;

    if (ioctl(fd, READ_REG, &p) == -1) {
        perror(__func__);
        return -1;
    }

    return p.data;
}

static int my_dam_register_write(int fd, uint32_t addr, uint32_t data) 
{
    dam_drv_io_t p;
    p.address = addr;// / sizeof(uint32_t);
    p.data = data;

    if (ioctl(fd, WRITE_REG, &p) == -1) {
        perror(__func__);
        return -1;
    }

    return 0;
}



int
main(void)
{
    int fd = 0;

    printf("Opening FLX device, fd= ");fflush(stdout);
    my_dam_open(&fd);
    printf("%d\n",fd);fflush(stdout);
    sleep(1);
    if(fd<0) exit(0);

    printf("FLX Model is ");fflush(stdout);
    int model = my_dam_register_read(fd, 0x830);
    printf("%i\n",model);fflush(stdout);
    sleep(1);

    printf("FLX Reg Map is ");
    int map   = my_dam_register_read(fd, 0x000);
    printf("%i\n",map);fflush(stdout);
    sleep(1);



#if 0
    // Test read/writes
    my_dam_register_write(fd, 0x8b0, 0x0);
    int scratch = my_dam_register_read(fd, 0x8b0);
    for (int i = 0; i < 0xfffff; i++) {
        my_dam_register_write(fd, 0x8b0, i);
        scratch = my_dam_register_read(fd, 0x8b0);
        if (scratch != i) {
            fprintf(stderr, "ERROR: Unexpected readback, got:%i expected:%i\n", scratch, i);
            break;
        }
    }
#endif

    printf("Closing FLX device fd=%d\n",fd);fflush(stdout);
    my_dam_close(fd);

    return 0;
}
