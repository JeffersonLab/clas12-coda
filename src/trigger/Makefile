#
# Makefile for trigger/
#

#-----------------------------
# mandatory definitions: MAIN
#-----------------------------
MAIN = trigger


#------------------
# generic Makefile
#------------------
include $(CODA)/src/Makefile.include
###include $(CODA)/src/Makefile.arch

# ROOT definitions
ROOTCFLAGS   := $(shell root-config --cflags)
ROOTLIBS     := $(shell root-config --libs)
ROOTGLIBS    := $(shell root-config --glibs)

#-----------------------
# customize definitions
#-----------------------

CLON_FLAGS += -std=c++11

CLON_FLAGS += $(ROOTCFLAGS)


CLON_FLAGS += -I./src.s -I$(CODA)/common/include

ifneq ("$(OSTYPE)","VXWORKS")

# for vme read/write
  LIBNAMES += $(CODA)/src/rol/$(OSTYPE_MACHINE)/lib/librol.a

# for database access
  LIBNAMES += $(CODA)/src/codautil/$(OSTYPE_MACHINE)/lib/libcodautil.a



ifeq ("$(OSTYPE_MACHINE)","Linux_armv7l")
  LIBNAMES += /usr/lib/libmysqlclient.so
else ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64")
  LIBNAMES += /usr/lib64/mysql/libmysqlclient.so
else ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64_RHEL9")
  LIBNAMES += /usr/lib64/mysql/libmysqlclient.so
  #LIBNAMES += /usr/lib64/libmariadb.so
else ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64_vme")
  LIBNAMES += /usr/lib64/mysql/libmysqlclient.so
else ifeq ("$(OSTYPE_MACHINE)","Linux_i686_vme")
    LIBNAMES += /usr/lib/mysql/libmysqlclient.so
else ifeq ("$(OSTYPE_MACHINE)","Linux_i686")
    LIBNAMES += /usr/lib/mysql/libmysqlclient.so
endif

  LIBNAMES += $(ROOTLIBS) $(ROOTGLIBS)

#LIBNAMES += -lresolv
#LIBNAMES += $(X11LIBS)
#LIBNAMES += $(SYSLIBS)


#  LIBNAMES += $(SYSLIBS)
endif




#-------------------
# customize targets
#-------------------
install: install_lib install_bin
#	rm -rf tmp
#	mkdir tmp
###	cp src.s/tt.h tmp
#	chmod 664 tmp/*
#	-mkdir -p $(CODA)/common/include
#	cp tmp/* $(CODA)/common/include
#	rm -rf tmp






























