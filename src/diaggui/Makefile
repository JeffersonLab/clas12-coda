#
# Makefile for diaggui/
#

#-----------------------------
# mandatory definitions: MAIN
#-----------------------------
MAIN = diaggui


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

ifeq ("$(OSTYPE_MACHINE)","SunOS_i86pc")
#  CLON_FLAGS += -m64
  SHLD += -m64
#  LDCXX += -m64
endif

CLON_FLAGS = -std=c++11

CLON_FLAGS += $(ROOTCFLAGS)

CLON_FLAGS += -I./src.s \
				-I./inc/ALERTFEB \
				-I./inc/CTP \
				-I./inc/DCRB \
				-I./inc/DCRBScalers \
				-I./inc/DSC \
				-I./inc/FADC \
				-I./inc/FADCScalers_HPS \
				-I./inc/GTP \
				-I./inc/GTP_HPS \
				-I./inc/VTP_HPS \
				-I./inc/VTP_FADCSTREAM \
				-I./inc/SD \
				-I./inc/SSP \
				-I./inc/SSP_HPS \
				-I./inc/TID \
				-I./inc/V1495Pulser \
				-I./inc/VSCM \
				-I./inc/vetroc \
				-I$(CODA)/common/include

CLON_FLAGS += -I$(IPC_HOME)/include/activemq-cpp-$(IPC_VER) \
		   	  -I$(CODA)/src/ipc/ipc.s -w

ifeq ("$(OSTYPE_MACHINE)","Linux_i686")
  CLON_FLAGS += -I/usr/local/include/apr-1
else ifeq ("$(OSTYPE_MACHINE)","Linux_i686_vme")
  CLON_FLAGS += -I/usr/local/include/apr-1
else
  CLON_FLAGS += -I/usr/include/apr-1
endif


LIBNAMES += $(CODA)/src/ipc/$(OSTYPE_MACHINE)/lib/libipc.a
ifeq ("$(OSTYPE_MACHINE)","Linux_i686")
  LIBNAMES += $(IPC_HOME)/lib/libactivemq-cpp.so
  LIBNAMES += /usr/local/lib/libapr-1.so
  LIBNAMES += -luuid -lrt -lcrypt -lssl -lcrypto -ldl -lz
  LIBNAMES += -L/usr/kerberos/lib -lpthread -pthread -Wl,-rpath -Wl,/usr/local/lib
else
  LIBNAMES += $(IPC_HOME)/lib/libactivemq-cpp.so
  LIBNAMES += -lapr-1
  LIBNAMES += -lrt -lcrypt -lssl -lcrypto -ldl -lz
  LIBNAMES += -L/usr/kerberos/lib -lpthread -pthread -Wl,-rpath -Wl,/usr/local/lib
endif


ifneq ("$(OSTYPE)","VXWORKS")
#  LIBNAMES += $(CODA)/$(OSTYPE_MACHINE)/lib/librol.a
#  LIBNAMES += $(CODA)/$(OSTYPE_MACHINE)/lib/libmysql4.1.20.a
#  LIBNAMES += $(CODA)/src/rol/jvme/libjvme.so

LIBNAMES += $(ROOTLIBS) $(ROOTGLIBS)

##  LIBNAMES += $(SYSLIBS)
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






























