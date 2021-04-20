#
# Makefile for codaedit/
#

#-----------------------------
# mandatory definitions: MAIN
#-----------------------------
MAIN = codaedit


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
CLON_FLAGS += -I./$(MAIN).s -I$(CODA)/common/include $(X11INCDIR)
CLON_FLAGS += -I/usr/include/mysql

CLON_FLAGS += -DUSE_CREG
LIBNAMES += $(globallib)/libcreg.a
LIBNAMES += $(globallib)/libcodautil.a

ifeq ("$(OSTYPE_MACHINE)","Linux_armv7l")
  LIBNAMES += /usr/lib/libmysqlclient.so
else ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64")
  LIBNAMES += /usr/lib64/mysql/libmysqlclient.so
else ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64_vme")
  LIBNAMES += /usr/lib64/mysql/libmysqlclient.so
else ifeq ("$(OSTYPE_MACHINE)","Linux_i686_vme")
    LIBNAMES += /usr/lib/mysql/libmysqlclient.so
else ifeq ("$(OSTYPE_MACHINE)","Linux_i686")
    LIBNAMES += /usr/lib/mysql/libmysqlclient.so
endif

LIBNAMES += -lresolv
LIBNAMES += $(X11LIBS)
LIBNAMES += $(SYSLIBS)


#-------------------
# customize targets
#-------------------
install: install_lib install_bin
	rm -rf tmp
	mkdir tmp
	cp $(MAIN).s/codaedit.h tmp/cedit.h
	chmod 664 tmp/*
	-mkdir -p $(CODA)/common/include
	cp tmp/* $(CODA)/common/include
	rm -rf tmp



























