#
# Makefile for cterm/
#

#-----------------------------
# mandatory definitions: MAIN
#-----------------------------
MAIN = cterm


#------------------
# generic Makefile
#------------------
include $(CODA)/src/Makefile.include

#debugging
##CFLAGS = -g -fPIC -DLinux -DLINUX

#LDCC = $(FC)
#LDFC = $(FC)
LDCC = $(LDCXX)
LDFC = $(LDCXX)

# if want debugging mode, uncomment following lines
ifeq ("$(OSTYPE)","SunOS")
    CFLAGS = -g -KPIC -mt -DSunOS -DOSTYPE="SunOS"
endif
ifeq ("$(OSTYPE_MACHINE)","SunOS_i86pc")
    CFLAGS += -DSunOS_i86pc
endif
ifeq ("$(OSTYPE_MACHINE)","SunOS_sun4u")
    CFLAGS += -DSunOS_sun4u
endif


#-----------------------
# customize definitions
#-----------------------
CLON_FLAGS +=  $(X11INCDIR) -I./src.s -DHAVE_CONFIG_H -I/usr/include/freetype2 -I/usr/include/freetype2/config -Dlinux \
				-D_POSIX_C_SOURCE=199309L -D_POSIX_SOURCE -D_XOPEN_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DFUNCPROTO=15 \
				-DNARROWPROTO -DPROJECTROOT=/usr -DUTMP  -DOSMAJORVERSION=2 -DOSMINORVERSION=6 -D_GNU_SOURCE \
				-I$(CODA)/common/include


CLON_FLAGS += -DDO_EXPECT
CLON_FLAGS += -DDO_CREG

## sergey: if following defined, embedding stops working ...
##CLON_FLAGS += -DOPT_TOOLBAR
##CLON_FLAGS += -DDO_LABEL

LIBNAMES += $(CODA)/src/creg/$(OSTYPE_MACHINE)/lib/libcreg.a

LIBNAMES += $(TERMCAPLIBS)

LIBNAMES += $(X11LIBS)
#LIBNAMES += $(SYSLIBS)

# cost fails in examples
LIBNAMES += -lXaw



#-------------------
# customize targets
#-------------------
install: install_lib install_bin

#install: install_lib install_bin
#	rm -rf tmp
#	mkdir tmp
#	cp $(MAIN).s/cterm.h tmp/cterm.h
#	chmod 664 tmp/*
#	-mkdir -p $(CODA)/common/include
#	cp tmp/* $(CODA)/common/include
#	rm -rf tmp



























