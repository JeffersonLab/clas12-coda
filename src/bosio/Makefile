#
# Makefile for bosio/
#

#-----------------------------
# mandatory definitions: MAIN
#-----------------------------
MAIN = bosio


#------------------
# generic Makefile
#------------------
include $(CODA)/src/Makefile.include



#-----------------------
# customize definitions
#-----------------------

# using fortran's  cernlib
LDCC = $(FC)

# if debugging
#FFLAGS = -g -KPIC -mt -DSunOS -e -ftrap=no%inexact,no%underflow
#CFLAGS = -g -KPIC -mt -DSunOS -DOSTYPE="SunOS"

CLON_FLAGS += -I./$(MAIN).s -I$(CODA)/common/include \
			-D_POSIX_PTHREAD_SEMANTICS

ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64_RHEL9")
CLON_FLAGS += -I/usr/include/tirpc
endif



LIBNAMES += $(CODA)/$(OSTYPE_MACHINE)/lib/libevio.a

LIBNAMES += $(CODA)/$(OSTYPE_MACHINE)/lib/libet.a
LIBNAMES += -lexpat
LIBNAMES += -lpthread

ifneq ("$(OSTYPE)","Darwin")
  LIBNAMES += -lrt
endif
LIBNAMES += -lresolv

##ifneq ("$(MACHINE)","x86_64")
  LIBNAMES += $(CERNLIBS)
##endif

LIBNAMES += $(F77LIBS)
LIBNAMES += $(SYSLIBS)
ifeq ("$(OSTYPE)","Linux")
  LIBNAMES += -lcrypt
endif

ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64_RHEL9")
  LIBNAMES += -ltirpc
endif


#-------------------
# customize targets
#-------------------
install: install_lib install_bin
	rm -rf tmp
	mkdir tmp
	cp $(MAIN).s/*.h tmp
	chmod 664 tmp/*
	-mkdir -p $(CODA)/common/include
	cp tmp/* $(CODA)/common/include
	rm -rf tmp






























