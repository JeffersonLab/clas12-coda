#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>  
#include <errno.h>
#include <termio.h>
#include <termios.h>

#include "libdam.h"

#define DEVNAME "/dev/dam0"

// We have two operations: read and write
static const char *read_op  = "read";
static const char *write_op = "write";

int usage()
{
    printf(  "damreg read|write [<arguments>...]\n" \
             "  Numeric arguments may be in hex or decimal\n" \
             "Examples of usage:\n" \
             "  rri read 0x80ffff        -- read one register\n" \
             "  rri read 0x800000 5      -- read 5 registers starting at 0x800000\n" \
             "  rri write 0x800000 0     -- write 0 to register 0x800000\n" \
             "  rri write 0x800000 1 2 3 0xdecafbad\n" \
             "                             -- write 4 registers\n" \
             "\n");
    return -1;
}

int main(int argc, char* const argv[])
{
    if (argc < 2) return usage();

    char     *operation  = argv[1];
    uint32_t  address = strtoul(argv[2], NULL, 0);
    int       fd = 0;

    dam_open(&fd, DEVNAME);

    if (strncmp(operation, read_op, strlen(read_op)) &&
            strncmp(operation, write_op, strlen(write_op))) {
        return usage();
    }

    if (0 == strncmp(operation, read_op, strlen(read_op))) {
        // Syntax:
        // damreg foo read addr
        // damreg foo read addr count

        unsigned count = 1;

        if (argc > 3) count = strtoul(argv[3], NULL, 0);

        for (unsigned i = 0; i < count*0x4; i += 0x4) {
            uint32_t a = address + i;

            uint32_t dat = dam_register_read(fd, a);
            printf("  Register 0x%x (%d): 0x%08x (%i)\n", a, a, dat, dat);
        }
    } else if (0 == strncmp(operation, write_op, strlen(write_op))) {
        // Syntax:
        // damreg foo write addr value
        // damreg foo write addr value value ...

        for (int i = 0; i < (argc - 3); i += 0x4) {
            uint32_t a = address+(i*0x4);
            uint32_t val = strtoul(argv[i+3], NULL, 0);
            dam_register_write(fd, a, val);
        }
    }

    dam_close(fd);
    return 0;
}
