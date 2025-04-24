
/* libdam.c */

#ifdef Linux_vme

void
libdam_dummy()
{
  return;
}

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "dam_ioctl.h"

int dam_open(int *fd, char *fname)
{
    if ( (*fd = open(fname, O_RDWR)) <= 0 ) {
        perror(__func__);
        return -1;
    }

    return 0;
}

int dam_close(int fd) 
{
    return close(fd);
}

uint32_t dam_register_read(int fd, uint32_t addr)
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

int dam_register_write(int fd, uint32_t addr, uint32_t data) 
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

uint64_t dam_memory_map(int fd, uint64_t addr, size_t size)
{
    void *vaddr;
    uint64_t offset;

    int64_t pagesz = sysconf(_SC_PAGE_SIZE);

    if (pagesz == -1)
        pagesz = 0x10000;

    pagesz -= 1;                     // Turn value into its matching bitmask
    offset = addr & pagesz;          // mmap requires pagesize alignment
    addr &= (0xffffffffffffffffL & (~pagesz));

    vaddr = mmap(0, size, (PROT_READ|PROT_WRITE), MAP_SHARED, fd, addr);
    if (vaddr == MAP_FAILED) {
        perror(__func__);
        return (uint64_t)vaddr;
    }

    return (unsigned long)vaddr + offset;
}

#endif

