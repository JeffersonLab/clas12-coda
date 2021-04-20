#
# Makefile for roltmp/
#

#-----------------------------
# mandatory definitions: MAIN
#-----------------------------
MAIN = roltmp


#------------------
# generic Makefile
#------------------
include $(CODA)/src/Makefile.include

localrol = ./$(OSTYPE_MACHINE)/roltmp
globalrol = $(CODA)/$(OSTYPE_MACHINE)/roltmp

#------------------------
# special building cases
#------------------------

#ifeq ("$(OSTYPE_MACHINE)","Linux_i686_vme")

$(LIB_FILES_ROL): $(SOURCE_FILES_ROL) ./$(OSTYPE_MACHINE)/rol/rol1_master.so \
	./$(OSTYPE_MACHINE)/rol/rol1_slave.so


./$(OSTYPE_MACHINE)/rol/rol1_master.o : rols/rol1.c
	$(MKROLDIR)
	$(CC) -DTI_MASTER $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/rol1_slave.o : rols/rol1.c
	$(MKROLDIR)
	$(CC) -DTI_SLAVE $(CCROLOPT)


#endif


#-----------------------
# customize definitions
#-----------------------

#LDCC = $(FC)
#LDFC = $(FC)
LDCC = $(LDCXX)
LDFC = $(LDCXX)

ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64_vme")
  CLON_FLAGS +=  -DLinux_x86_64_vme -I$(CODA)/src/jvme/src.s
endif
CLON_FLAGS += -I./inc -I./code.s -I./mvt.s -I./sfi.s -I../vtp -I$(CODA)/common/include

CLON_FLAGS += -DIPC

# hps run
##############CLON_FLAGS += -I$(CLAS)/slac_svt/heavy-photon-daq/software/rogue_coda/include/hps/builder/client/

ifeq ("$(OSTYPE)","Linux")
  CLON_FLAGS += -I/usr/local/src/tirpci/tirpci_installation/include
  CLON_FLAGS += -I/usr/local/src/tirpci


# evio
  CLON_FLAGS += -I$(CODA)/src/evio-4.3/src.s

endif

ifeq ("$(OSTYPE)","VXWORKS")

  CLON_FLAGS += -I$(CODA)/VXWORKS_ppc/include
  LIBNAMES +=

else ifeq ("$(OSTYPE_MACHINE)","Linux_armv7l")

  CLON_FLAGS += -I$(CODA)/Linux_armv7l/include
  LIBNAMES += $(CODA)/src/codautil/$(OSTYPE_MACHINE)/lib/libcodautil.a
  LIBNAMES += /usr/lib/libmysqlclient.so
  LIBNAMES += -lvtp




  LIBNAMES += $(CODA)/src/codautil/$(OSTYPE_MACHINE)/lib/libcodautil.a

  LIBNAMES += $(CODA)/src/ipc/$(OSTYPE_MACHINE)/lib/libipc.a
  LIBNAMES += $(IPC_HOME)/lib/libactivemq-cpp.so
  LIBNAMES += -lapr-1
  LIBNAMES += -lrt -lcrypt -lssl -lcrypto -ldl -lz
  LIBNAMES += -L/usr/kerberos/lib -lpthread -pthread -Wl,-rpath -Wl,/usr/local/lib




  LIBNAMES += /usr/local/lib/libi2c.a
  LIBNAMES += -lpthread
  LIBNAMES += -lrt
else ifeq ("$(OSTYPE_MACHINE)","Linux_nios2")
  CLON_FLAGS += -I$(CODA)/Linux_nios2/include
  LIBNAMES +=
else


### for hps run
  ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64")
#    LIBNAMES += $(CLAS)/slac_svt/heavy-photon-daq/software/rogue_coda/install/lib/libhps-builder-client-real.so
########    LIBNAMES += $(CODA)/$(OSTYPE_MACHINE)/lib/libhps-builder-client-real.so
  endif

### 
#################  LIBNAMES += $(CODA)/src/gem/$(OSTYPE_MACHINE)/lib/libgem.a



###  LIBNAMES += $(CODA)/src/dac/$(OSTYPE_MACHINE)/lib/libdac.a

  LIBNAMES += $(CODA)/src/evio-4.3/$(OSTYPE_MACHINE)/lib/libevio.a
  LIBNAMES += $(CODA)/src/codautil/$(OSTYPE_MACHINE)/lib/libcodautil.a

  ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64")
    LIBNAMES += /usr/lib64/mysql/libmysqlclient.so
  endif

  ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64_vme")
    LIBNAMES += /usr/lib64/mysql/libmysqlclient.so
    LIBNAMES += $(CODA)/src/jvme/$(OSTYPE_MACHINE)/lib/libjvme.a
    LIBNAMES += /usr/lib64/gef/libvme.a
    LIBNAMES += $(CODA)/src/ipc/$(OSTYPE_MACHINE)/lib/libipc.a
    LIBNAMES += $(IPC_HOME)/lib/libactivemq-cpp.so
    LIBNAMES += -lapr-1
    LIBNAMES += -lrt -lcrypt -lssl -lcrypto -ldl -lz
    LIBNAMES += -L/usr/kerberos/lib -lpthread -pthread -Wl,-rpath -Wl,/usr/local/lib
  endif

  ifeq ("$(OSTYPE_MACHINE)","Linux_i686_vme")
    LIBNAMES += /usr/lib/mysql/libmysqlclient.so
  endif

  ifeq ("$(OSTYPE_MACHINE)","Linux_i686")
    LIBNAMES += /usr/lib/mysql/libmysqlclient.so
  endif

#cern root
  ROOTCFLAGS   := $(shell root-config --cflags)
  ROOTLIBS     := $(shell root-config --libs) 
  ROOTGLIBS    := $(shell root-config --glibs)
  CLON_FLAGS += $(ROOTCFLAGS)
  LIBNAMES += $(ROOTLIBS) $(ROOTGLIBS)

  LIBNAMES += -lpthread
  LIBNAMES += -lexpat

ifeq ("$(OSTYPE_MACHINE)","Linux_i686_vme")

  LIBNAMES += $(CODA)/src/roltmp/jvme/libjvme.so

  LIBNAMES += $(CODA)/src/codautil/$(OSTYPE_MACHINE)/lib/libcodautil.a

  LIBNAMES += $(CODA)/src/ipc/$(OSTYPE_MACHINE)/lib/libipc.a
  LIBNAMES += $(IPC_HOME)/lib/libactivemq-cpp.so
  LIBNAMES += -lapr-1
  LIBNAMES += -lrt -lcrypt -lssl -lcrypto -ldl -lz
  LIBNAMES += -L/usr/kerberos/lib -lpthread -pthread -Wl,-rpath -Wl,/usr/local/lib

## smartsocket stuff
  CLON_FLAGS +=	-I$(CLON)/common/include
  CLON_FLAGS += -I$(RTHOME)/include
#  LIBNAMES += $(CLON)/Linux_i686/lib/libepicsutil.a
#  LIBNAMES += $(CLON)/Linux_i686/lib/libipc.a
#  LIBNAMES += -L$(RTHOME)/lib/$(OSTYPE_MACHINE) -lrtipc -lrtipcl -lrtutil -lrtcxxipc50
## smartsocket stuff

endif

  LIBNAMES += $(SYSLIBS)
endif


#-------------------
# customize targets
#-------------------

jvmes:
	cd jvme; make clean; make

#
# must install librol.so for VXWORKS only !!!!!!!!!!
#install_rol

install: install_obj install_lib install_bin install_rol
	rm -rf tmp
	mkdir tmp
	cp code.s/*.h tmp
	cp sfi.s/*.h tmp
	chmod 664 tmp/*
	-mkdir -p $(CODA)/common/include
	cp tmp/* $(CODA)/common/include
	rm -rf tmp

	rm -rf tmp
	mkdir tmp
	cp ./inc/*.h tmp
	chmod 664 tmp/*
	-mkdir -p $(CODA)/common/include
	cp tmp/* $(CODA)/common/include
	rm -rf tmp

	rm -rf tmp
	mkdir tmp
	cp ./bootscripts/boot_* tmp
	cp ./bootscripts/*.boot tmp
	chmod 664 tmp/*
	-mkdir -p $(CODA)/VXWORKS_ppc/bootscripts/
	cp tmp/* $(CODA)/VXWORKS_ppc/bootscripts/
	rm -rf tmp
