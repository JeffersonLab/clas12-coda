#ifndef _LIBDAM_H
#define _LIBDAM_H
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "dam_ioctl.h"

int dam_open(int *fd, char *fname);
int dam_close(int fd);
uint32_t dam_register_read(int fd, uint32_t addr);
int dam_register_write(int fd, uint32_t addr, uint32_t data);
uint64_t dam_memory_map(int fd, uint64_t addr, size_t size);

#endif
