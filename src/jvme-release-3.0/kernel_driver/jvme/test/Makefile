#
# File:
#    Makefile
#
# Description:
#    Makefile for the testing jvme kernel driver
#
#
# $Date$
# $Rev$
#

CROSS_COMPILE		=
CC			= $(CROSS_COMPILE)gcc
CFLAGS			= -Wall -g -I.. -I.

PROGS			= mmap_example user_example mmap_bridge

all: $(PROGS)

clean distclean:
	@rm -f $(PROGS) *~ *.so

%: %.c
	echo "Making $@"
	$(CC) $(CFLAGS) -o $@ $(@:%=%.c)

.PHONY: all clean distclean
