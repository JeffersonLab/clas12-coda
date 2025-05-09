#
# File:
#    Makefile
#
# Description:
#    Makefile for the JLAB extensions to the VME Controller API (GEFANUC and others)
#
# SVN: $Rev: 393 $
#
#
#DEBUG=1

ifndef LINUXVME_LIB
	LINUXVME_LIB	= $CODA/extensions/linuxvme/libs
endif
ifndef LINUXVME_INC
	LINUXVME_INC	= $CODA/extensions/linuxvme/include
endif

CROSS_COMPILE		=
CC			= $(CROSS_COMPILE)gcc
AR                      = ar
RANLIB                  = ranlib
#
#CFLAGS			= -O2 -I. -I/usr/include
CFLAGS			= -g -I. -I/usr/include
#
CFLAGS                 += -DGEFANUC
ifdef DEBUG
CFLAGS			+= -Wall -g
endif

SRC			= jvme.c jlabgef.c jlabgefDMA.c dmaPList.c

OBJS			= $(SRC:%.c=%.o)


LIBS			= 
SHLIBS			= 
OSTYPE_MACHINE = $(OSTYPE)_$(MACHINE)
ifeq ("$(OSTYPE_MACHINE)","Linux_i686_vme")
  LIBS			= libjvme.a
  SHLIBS		= libjvme.so
endif

###all: $(LIBS) links
all: $(LIBS) $(SHLIBS)

libjvme.so: $(SRC) /usr/lib/gef/libvme.a Makefile
	$(AR) x /usr/lib/gef/libvme.a
	$(CC) -fpic -shared $(CFLAGS) -lrt -lpthread -o $@ $(SRC) gefcmn_*.o
	@rm gefcmn_*.o

libjvme.a: $(OBJS) /usr/lib/gef/libvme.a Makefile
	$(AR) x /usr/lib/gef/libvme.a
	$(AR) rv $@ $(OBJS)
	$(AR) rv $@ gefcmn_*.o
	@rm gefcmn_*.o

clean distclean:
	@rm -f $(OBJS) $(LIBS) $(SHLIBS) *~

links: libjvme.a libjvme.so
	ln -sf $(PWD)/libjvme.so $(LINUXVME_LIB)/libjvme.so
	ln -sf $(PWD)/libjvme.a $(LINUXVME_LIB)/libjvme.a
	ln -sf $(PWD)/dmaPList.h $(LINUXVME_INC)/dmaPList.h
	ln -sf $(PWD)/jvme.h $(LINUXVME_INC)/jvme.h

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

install:
	cp $(LIBS) $(CODA)/$(OSTYPE_MACHINE)/lib
	cp $(SHLIBS) $(CODA)/$(OSTYPE_MACHINE)/lib

.PHONY: all clean distclean
