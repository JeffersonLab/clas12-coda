#
# Makefile for rol/
#

#-----------------------------
# mandatory definitions: MAIN
#-----------------------------
MAIN = rol


#------------------
# generic Makefile
#------------------
include $(CODA)/src/Makefile.include

LIBCONFIG = /usr/local/src/libconfig-1.8.1/lib



#----------------------------------------
# special building cases - executables
#----------------------------------------


bin: $(BIN_FILES_PROG) \
	./$(OSTYPE_MACHINE)/bin/vtpserver_regular \
	./$(OSTYPE_MACHINE)/bin/vtpserver_streaming \
	./$(OSTYPE_MACHINE)/bin/vtpserver_mpd \
	./$(OSTYPE_MACHINE)/bin/DiagGuiServer_regular \
	./$(OSTYPE_MACHINE)/bin/DiagGuiServer_streaming \
	./$(OSTYPE_MACHINE)/bin/DiagGuiServer_mpd \
	./$(OSTYPE_MACHINE)/bin/vtpMpdFiberStatus \
	./$(OSTYPE_MACHINE)/bin/vtpMpdHistoMode \
	./$(OSTYPE_MACHINE)/bin/vtpMpdSampleMode

#	./$(OSTYPE_MACHINE)/bin/vtpMpdStatus \
#	./$(OSTYPE_MACHINE)/bin/vtpMpdTcpConnect \
#	./$(OSTYPE_MACHINE)/bin/vtpMpdInit



#----------------------------------------
# special building cases - readout lists
#----------------------------------------


$(LIB_FILES_ROL): $(SOURCE_FILES_ROL) ./$(OSTYPE_MACHINE)/rol/rol1_master.so \
	./$(OSTYPE_MACHINE)/rol/rol1_slave.so \
	./$(OSTYPE_MACHINE)/rol/rol1_slavemaster.so \
	./$(OSTYPE_MACHINE)/rol/scaler1_master.so \
	./$(OSTYPE_MACHINE)/rol/scaler1_slave.so \
	./$(OSTYPE_MACHINE)/rol/tipci1_master.so \
	./$(OSTYPE_MACHINE)/rol/tipci1_slave.so \
	./$(OSTYPE_MACHINE)/rol/sampa1_master.so \
	./$(OSTYPE_MACHINE)/rol/sampa1_slave.so \
	./$(OSTYPE_MACHINE)/rol/maroc1_master.so \
	./$(OSTYPE_MACHINE)/rol/maroc1_slave.so \
	./$(OSTYPE_MACHINE)/rol/petiroc1_master.so \
	./$(OSTYPE_MACHINE)/rol/petiroc1_slave.so \
	./$(OSTYPE_MACHINE)/rol/nalu1_master.so \
	./$(OSTYPE_MACHINE)/rol/nalu1_slave.so \
	./$(OSTYPE_MACHINE)/rol/rol1mvt_master.so \
	./$(OSTYPE_MACHINE)/rol/rol1mvt_slave.so \
	./$(OSTYPE_MACHINE)/rol/srs1_master.so \
	./$(OSTYPE_MACHINE)/rol/srs1_slave.so \
	./$(OSTYPE_MACHINE)/rol/urwell1_master.so \
	./$(OSTYPE_MACHINE)/rol/urwell1_slave.so \
	./$(OSTYPE_MACHINE)/rol/rol1_sro_master.so \
	./$(OSTYPE_MACHINE)/rol/rol1_sro_slave.so \
	./$(OSTYPE_MACHINE)/rol/rol1_ti_master.so \
	./$(OSTYPE_MACHINE)/rol/rol1_ti_slave.so

#	./$(OSTYPE_MACHINE)/rol/fbrol1_master.so \
#	./$(OSTYPE_MACHINE)/rol/fbrol1_slave.so \

#	./$(OSTYPE_MACHINE)/rol/vmm1_master.so \
#	./$(OSTYPE_MACHINE)/rol/vmm1_slave.so \

# <RogueCoda.h> must be moved from clonfarm1:/data to $CODA, or to $CLAS !!!
#	./$(OSTYPE_MACHINE)/rol/hpssvt1_master.so \
#	./$(OSTYPE_MACHINE)/rol/hpssvt1_slave.so



./$(OSTYPE_MACHINE)/rol/rol1_sro_master.o : rols/rol1_sro.c
	$(MKROLDIR)
	$(CC) -DTI_MASTER $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/rol1_sro_slave.o : rols/rol1_sro.c
	$(MKROLDIR)
	$(CC) -DTI_SLAVE $(CCROLOPT)


./$(OSTYPE_MACHINE)/rol/rol1_ti_master.o : rols/rol1.c
	$(MKROLDIR)
	$(CC) -DTI_MASTER -DTI_ONLY $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/rol1_ti_slave.o : rols/rol1.c
	$(MKROLDIR)
	$(CC) -DTI_SLAVE -DTI_ONLY $(CCROLOPT)







# for CLAS12
./$(OSTYPE_MACHINE)/rol/vtp1.o : rols/vtp1.c
	$(MKROLDIR)
	$(CC) -I$(CODA)/src/vtp_regular $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/vtp1.so : ./$(OSTYPE_MACHINE)/rol/vtp1.o
	$(SHLD) $(LDFLAGS) ./$< $(LIBS_DIRS) $(LIBVTP2) $(LIBNAMES) -o ./$@



# for PRAD
#./$(OSTYPE_MACHINE)/rol/vtp1.o : rols/vtp1.c
#	$(MKROLDIR)
#	$(CC) -I$(CODA)/src/vtp_mpd $(CCROLOPT)
#
#./$(OSTYPE_MACHINE)/rol/vtp1.so : ./$(OSTYPE_MACHINE)/rol/vtp1.o
#	$(SHLD) $(LDFLAGS) ./$< $(LIBS_DIRS) $(LIBVTP1) $(LIBNAMES) -o ./$@













./$(OSTYPE_MACHINE)/rol/vtp1mpd.o : rols/vtp1mpd.c
	$(MKROLDIR)
	$(CC) -I$(CODA)/src/vtp_mpd $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/vtp1mpd.so : ./$(OSTYPE_MACHINE)/rol/vtp1mpd.o
	$(SHLD) $(LDFLAGS) ./$< $(LIBS_DIRS) $(LIBVTP1) $(LIBNAMES) -o ./$@




./$(OSTYPE_MACHINE)/rol/vtp1_sro_new.o : rols/vtp1_sro_new.c
	$(MKROLDIR)
	$(CC) -I$(CODA)/src/vtp_streaming $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/vtp1_sro_new.so : ./$(OSTYPE_MACHINE)/rol/vtp1_sro_new.o
	$(SHLD) $(LDFLAGS) ./$< $(LIBS_DIRS) $(LIBVTP3) $(LIBNAMES) -o ./$@








./$(OSTYPE_MACHINE)/rol/rol1_master.o : rols/rol1.c
	$(MKROLDIR)
	$(CC) -DTI_MASTER $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/rol1_slave.o : rols/rol1.c
	$(MKROLDIR)
	$(CC) -DTI_SLAVE $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/rol1_slavemaster.o : rols/rol1.c
	$(MKROLDIR)
	$(CC) -DTI_SLAVEMASTER $(CCROLOPT)







./$(OSTYPE_MACHINE)/rol/rol1mvt_master.o : rols/rol1mvt.c
	$(MKROLDIR)
	$(CC) -DTI_MASTER $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/rol1mvt_slave.o : rols/rol1mvt.c
	$(MKROLDIR)
	$(CC) -DTI_SLAVE $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/scaler1_master.o : rols/scaler1.c
	$(MKROLDIR)
	$(CC) -DTI_MASTER $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/scaler1_slave.o : rols/scaler1.c
	$(MKROLDIR)
	$(CC) -DTI_SLAVE $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/fbrol1_master.o : rols/fbrol1.c
	$(MKROLDIR)
	$(CC) -DTI_MASTER $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/fbrol1_slave.o : rols/fbrol1.c
	$(MKROLDIR)
	$(CC) -DTI_SLAVE $(CCROLOPT)



./$(OSTYPE_MACHINE)/rol/srs1_master.o : rols/srs1.c
	$(MKROLDIR)
	$(CC) -DTI_MASTER $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/srs1_slave.o : rols/srs1.c
	$(MKROLDIR)
	$(CC) -DTI_SLAVE $(CCROLOPT)



./$(OSTYPE_MACHINE)/rol/tipci1_master.o : rols/tipci1.c
	$(MKROLDIR)
	$(CC) -DTI_MASTER $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/tipci1_slave.o : rols/tipci1.c
	$(MKROLDIR)
	$(CC) -DTI_SLAVE $(CCROLOPT)


#./$(OSTYPE_MACHINE)/rol/vmm1_master.o : rols/tipci1.c
#	$(MKROLDIR)
#	$(CC) -DTI_MASTER -DUSE_VMM $(CCROLOPT)

#./$(OSTYPE_MACHINE)/rol/vmm1_slave.o : rols/tipci1.c
#	$(MKROLDIR)
#	$(CC) -DTI_SLAVE -DUSE_VMM $(CCROLOPT)


./$(OSTYPE_MACHINE)/rol/sampa1_master.o : rols/tipci1.c
	$(MKROLDIR)
	$(CC) -DTI_MASTER -DUSE_SAMPA $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/sampa1_slave.o : rols/tipci1.c
	$(MKROLDIR)
	$(CC) -DTI_SLAVE -DUSE_SAMPA $(CCROLOPT)


./$(OSTYPE_MACHINE)/rol/maroc1_master.o : rols/tipci1.c
	$(MKROLDIR)
	$(CC) -DTI_MASTER -DUSE_MAROC $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/maroc1_slave.o : rols/tipci1.c
	$(MKROLDIR)
	$(CC) -DTI_SLAVE -DUSE_MAROC $(CCROLOPT)


./$(OSTYPE_MACHINE)/rol/petiroc1_master.o : rols/tipci1.c
	$(MKROLDIR)
	$(CC) -DTI_MASTER -DUSE_PETIROC $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/petiroc1_slave.o : rols/tipci1.c
	$(MKROLDIR)
	$(CC) -DTI_SLAVE -DUSE_PETIROC $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/nalu1_master.o : rols/tipci1.c
	$(MKROLDIR)
	$(CC) -DTI_MASTER -DUSE_NALU $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/nalu1_slave.o : rols/tipci1.c
	$(MKROLDIR)
	$(CC) -DTI_SLAVE -DUSE_NALU $(CCROLOPT)


./$(OSTYPE_MACHINE)/rol/urwell1_master.o : rols/tipci1.c
	$(MKROLDIR)
	$(CC) -DTI_MASTER -DUSE_URWELL $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/urwell1_slave.o : rols/tipci1.c
	$(MKROLDIR)
	$(CC) -DTI_SLAVE -DUSE_URWELL $(CCROLOPT)


#./$(OSTYPE_MACHINE)/rol/srs1_master.o : rols/tipci1.c
#	$(MKROLDIR)
#	$(CC) -DTI_MASTER -DUSE_SRS $(CCROLOPT)
#
#./$(OSTYPE_MACHINE)/rol/srs1_slave.o : rols/tipci1.c
#	$(MKROLDIR)
#	$(CC) -DTI_SLAVE -DUSE_SRS $(CCROLOPT)


./$(OSTYPE_MACHINE)/rol/hpssvt1_master.o : rols/tipci1.c
	$(MKROLDIR)
	$(CC) -DTI_MASTER -DUSE_HPS -I/data/hps/slac_svt/server/heavy-photon-daq/software/event_builder/include/hps/builder/client/ $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/hpssvt1_slave.o : rols/tipci1.c
	$(MKROLDIR)
	$(CC) -DTI_SLAVE -DUSE_HPS -I/data/hps/slac_svt/server/heavy-photon-daq/software/event_builder/include/hps/builder/client/ $(CCROLOPT)

./$(OSTYPE_MACHINE)/rol/hpssvt1_master.so : ./$(OSTYPE_MACHINE)/rol/hpssvt1_master.o
	$(SHLD) $(LDFLAGS) ./$< $(LIBS_DIRS) $(HPSLIBNAMES) $(LIBNAMES) -o ./$@

./$(OSTYPE_MACHINE)/rol/hpssvt1_slave.so : ./$(OSTYPE_MACHINE)/rol/hpssvt1_slave.o
	$(SHLD) $(LDFLAGS) ./$< $(LIBS_DIRS) $(HPSLIBNAMES) $(LIBNAMES) -o ./$@










./$(OSTYPE_MACHINE)/bin/vtpserver_regular : main/vtpserver.c $(LIBNAMES)
	$(MKBINDIR)
	$(CC) -I$(CODA)/src/vtp_regular $(CCBINOPT)
	$(LDCC) $(LDFLAGS) ./$@.o $(LIBS_DIRS) $(CODA)/src/vtp_regular/libvtp.a $(LIBNAMES) -o ./$@

./$(OSTYPE_MACHINE)/bin/vtpserver_streaming : main/vtpserver.c $(LIBNAMES)
	$(MKBINDIR)
	$(CC) -I$(CODA)/src/vtp_streaming $(CCBINOPT)
	$(LDCC) $(LDFLAGS) ./$@.o $(LIBS_DIRS) $(CODA)/src/vtp_streaming/libvtp.a $(LIBNAMES) -o ./$@

./$(OSTYPE_MACHINE)/bin/vtpserver_mpd : main/vtpserver.c $(LIBNAMES)
	$(MKBINDIR)
	$(CC) -I$(CODA)/src/vtp_mpd $(CCBINOPT)
	$(LDCC) $(LDFLAGS) ./$@.o $(LIBS_DIRS) $(CODA)/src/vtp_mpd/libvtp.a $(LIBNAMES) -o ./$@


# 'vtpserver' is the same as 'vtpserver_mpd'
#./$(OSTYPE_MACHINE)/bin/vtpserver : main/vtpserver.c $(LIBNAMES)
#	$(MKBINDIR)
#	$(CC) -I$(CODA)/src/vtp_mpd $(CCBINOPT)
#	$(LDCC) $(LDFLAGS) ./$@.o $(LIBS_DIRS) $(CODA)/src/vtp_mpd/libvtp.a $(LIBNAMES) -o ./$@

# 'vtpserver' is the same as 'vtpserver_regular'
./$(OSTYPE_MACHINE)/bin/vtpserver : main/vtpserver.c $(LIBNAMES)
	$(MKBINDIR)
	$(CC) -I$(CODA)/src/vtp_regular $(CCBINOPT)
	$(LDCC) $(LDFLAGS) ./$@.o $(LIBS_DIRS) $(CODA)/src/vtp_regular/libvtp.a $(LIBNAMES) -o ./$@




./$(OSTYPE_MACHINE)/bin/DiagGuiServer_regular : main/DiagGuiServer.c $(LIBNAMES)
	$(MKBINDIR)
	$(CC) -I$(CODA)/src/vtp_regular $(CCBINOPT)
	$(LDCC) $(LDFLAGS) ./$@.o $(LIBS_DIRS) $(CODA)/src/vtp_regular/libvtp.a $(LIBNAMES) -o ./$@

./$(OSTYPE_MACHINE)/bin/DiagGuiServer_streaming : main/DiagGuiServer.c $(LIBNAMES)
	$(MKBINDIR)
	$(CC) -I$(CODA)/src/vtp_streaming $(CCBINOPT)
	$(LDCC) $(LDFLAGS) ./$@.o $(LIBS_DIRS) $(CODA)/src/vtp_streaming/libvtp.a $(LIBNAMES) -o ./$@

./$(OSTYPE_MACHINE)/bin/DiagGuiServer_mpd : main/DiagGuiServer.c $(LIBNAMES)
	$(MKBINDIR)
	$(CC) -I$(CODA)/src/vtp_mpd $(CCBINOPT)
	$(LDCC) $(LDFLAGS) ./$@.o $(LIBS_DIRS) $(CODA)/src/vtp_mpd/libvtp.a $(LIBNAMES) -o ./$@



# 'DiagGuiServer' is the same as 'DiagGuiServer_mpd'
#./$(OSTYPE_MACHINE)/bin/DiagGuiServer : main/DiagGuiServer.c $(LIBNAMES)
#	$(MKBINDIR)
#	$(CC) -I$(CODA)/src/vtp_mpd $(CCBINOPT)
#	$(LDCC) $(LDFLAGS) ./$@.o $(LIBS_DIRS) $(CODA)/src/vtp_mpd/libvtp.a $(LIBNAMES) -o ./$@

# 'DiagGuiServer' is the same as 'DiagGuiServer_regular'
./$(OSTYPE_MACHINE)/bin/DiagGuiServer : main/DiagGuiServer.c $(LIBNAMES)
	$(MKBINDIR)
	$(CC) -I$(CODA)/src/vtp_regular $(CCBINOPT)
	$(LDCC) $(LDFLAGS) ./$@.o $(LIBS_DIRS) $(CODA)/src/vtp_regular/libvtp.a $(LIBNAMES) -o ./$@




./$(OSTYPE_MACHINE)/bin/vtpMpdFiberStatus : main/vtpMpdFiberStatus.c $(LIBNAMES)
	$(MKBINDIR)
	$(CC) -I$(CODA)/src/vtp_mpd $(CCBINOPT)
	$(LDCC) $(LDFLAGS) ./$@.o $(LIBS_DIRS) $(CODA)/src/vtp_mpd/libvtp.a $(LIBNAMES) -o ./$@

./$(OSTYPE_MACHINE)/bin/vtpMpdHistoMode : main/vtpMpdHistoMode.c $(LIBNAMES)
	$(MKBINDIR)
	$(CC) -I$(CODA)/src/vtp_mpd $(CCBINOPT)
	$(LDCC) $(LDFLAGS) ./$@.o $(LIBS_DIRS) $(CODA)/src/vtp_mpd/libvtp.a $(LIBNAMES) -o ./$@

./$(OSTYPE_MACHINE)/bin/vtpMpdInit : main/vtpMpdInit.c $(LIBNAMES)
	$(MKBINDIR)
	$(CC) -I$(CODA)/src/vtp_mpd $(CCBINOPT)
	$(LDCC) $(LDFLAGS) ./$@.o $(LIBS_DIRS) $(CODA)/src/vtp_mpd/libvtp.a $(LIBNAMES) -o ./$@

./$(OSTYPE_MACHINE)/bin/vtpMpdSampleMode : main/vtpMpdSampleMode.c $(LIBNAMES)
	$(MKBINDIR)
	$(CC) -I$(CODA)/src/vtp_mpd $(CCBINOPT)
	$(LDCC) $(LDFLAGS) ./$@.o $(LIBS_DIRS) $(CODA)/src/vtp_mpd/libvtp.a $(LIBNAMES) -o ./$@

./$(OSTYPE_MACHINE)/bin/vtpMpdStatus : main/vtpMpdStatus.c $(LIBNAMES)
	$(MKBINDIR)
	$(CC) -I$(CODA)/src/vtp_mpd $(CCBINOPT)
	$(LDCC) $(LDFLAGS) ./$@.o $(LIBS_DIRS) $(CODA)/src/vtp_mpd/libvtp.a $(LIBNAMES) -o ./$@

./$(OSTYPE_MACHINE)/bin/vtpMpdTcpConnect : main/vtpMpdTcpConnect.c $(LIBNAMES)
	$(MKBINDIR)
	$(CC) -I$(CODA)/src/vtp_mpd $(CCBINOPT)
	$(LDCC) $(LDFLAGS) ./$@.o $(LIBS_DIRS) $(CODA)/src/vtp_mpd/libvtp.a $(LIBNAMES) -o ./$@








#-----------------------
# customize definitions
#-----------------------

#LDCC = $(FC)
#LDFC = $(FC)
LDCC = $(LDCXX)
LDFC = $(LDCXX)

#for debugging
#CFLAGS = -g -fPIC -DLinux -DLINUX -DLinux_vme -I$(CODA)/src/jvme/src.s

#ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64_vme")
#  CLON_FLAGS +=  -DLinux_x86_64_vme
#endif
CLON_FLAGS +=  -D$(OSTYPE_MACHINE)

CLON_FLAGS += -I./inc -I./code.s -I./mvt.s
## -I./sfi.s
CLON_FLAGS += -I../vtp
CLON_FLAGS += -I$(CODA)/common/include
CLON_FLAGS += -I$(LIBCONFIG)

CLON_FLAGS += -DIPC


ifeq ("$(OSTYPE)","Linux")
  CLON_FLAGS += -I/usr/local/src/tirpci/tirpci_installation/include
  CLON_FLAGS += -I/usr/local/src/tirpci


# evio
  CLON_FLAGS += -I$(CODA)/src/evio-4.3/src.s

endif


LIBVTP1 =
LIBVTP2 =
LIBVTP3 =

ifeq ("$(OSTYPE)","VXWORKS")

  CLON_FLAGS += -I$(CODA)/VXWORKS_ppc/include
  LIBNAMES +=

else ifeq ("$(OSTYPE_MACHINE)","Linux_armv7l_Arch")

  LIBNAMES += $(CODA)/src/evio-4.3/$(OSTYPE_MACHINE)/lib/libevio.a
  LIBNAMES += $(CODA)/src/codautil/$(OSTYPE_MACHINE)/lib/libcodautil.a
  LIBNAMES += /usr/lib/libmysqlclient.so

  LIBVTP1 = $(CODA)/src/vtp_mpd/$(OSTYPE_MACHINE)/lib/libvtp.so
  LIBVTP2 = $(CODA)/src/vtp_regular/$(OSTYPE_MACHINE)/lib/libvtp.so
  LIBVTP3 = $(CODA)/src/vtp_streaming/$(OSTYPE_MACHINE)/lib/libvtp.so

  LIBNAMES += $(CODA)/src/codautil/$(OSTYPE_MACHINE)/lib/libcodautil.a

  LIBNAMES += $(CODA)/src/ipc/$(OSTYPE_MACHINE)/lib/libipc.a
  LIBNAMES += $(IPC_LIB)
  LIBNAMES += -lapr-1
  LIBNAMES += -lrt -lcrypt -lssl -lcrypto -ldl -lz
  LIBNAMES += -L/usr/kerberos/lib -lpthread -pthread -Wl,-rpath -Wl,/usr/local/lib

  LIBNAMES += $(LIBCONFIG)/.libs/libconfig.a

  LIBNAMES += /usr/local/lib/libi2c.so
  LIBNAMES += -lpthread
  LIBNAMES += -lrt

else ifeq ("$(OSTYPE_MACHINE)","Linux_armv7l_RHEL7")

  LIBNAMES += $(CODA)/src/evio-4.3/$(OSTYPE_MACHINE)/lib/libevio.a
  LIBNAMES += $(CODA)/src/codautil/$(OSTYPE_MACHINE)/lib/libcodautil.a
  LIBNAMES += /usr/lib/mysql/libmysqlclient.so

  LIBVTP1 = $(CODA)/src/vtp_mpd/$(OSTYPE_MACHINE)/lib/libvtp.so
  LIBVTP2 = $(CODA)/src/vtp_regular/$(OSTYPE_MACHINE)/lib/libvtp.so
  LIBVTP3 = $(CODA)/src/vtp_streaming/$(OSTYPE_MACHINE)/lib/libvtp.so

  LIBNAMES += $(CODA)/src/codautil/$(OSTYPE_MACHINE)/lib/libcodautil.a

  LIBNAMES += $(CODA)/src/ipc/$(OSTYPE_MACHINE)/lib/libipc.so
  LIBNAMES += $(IPC_LIB)

  LIBNAMES += $(LIBCONFIG)/.libs/libconfig.a
  #LIBNAMES += $(LIBCONFIG)/.libs/libconfig.so

  LIBNAMES += -lapr-1
  LIBNAMES += -lrt -lcrypt -lssl -lcrypto -ldl -lz
  LIBNAMES += -L/usr/kerberos/lib -lpthread -pthread -Wl,-rpath -Wl,/usr/local/lib

  LIBNAMES += /usr/local/lib/libi2c.so
  LIBNAMES += -lpthread
  LIBNAMES += -lrt

else ifeq ("$(OSTYPE_MACHINE)","Linux_nios2")

  CLON_FLAGS += -I$(CODA)/Linux_nios2/include
  LIBNAMES +=

else

  HPSLIBNAMES = $(CODA)/$(OSTYPE_MACHINE)/lib/libhps-builder-client-real.so

  LIBNAMES += $(CODA)/src/evio-4.3/$(OSTYPE_MACHINE)/lib/libevio.a
  LIBNAMES += $(CODA)/src/codautil/$(OSTYPE_MACHINE)/lib/libcodautil.a

  ifeq ("$(OSTYPE_PLATFORM)","Linux_x86_64")
    LIBNAMES += /usr/lib64/mysql/libmysqlclient.so
  else ifeq ("$(OSTYPE_PLATFORM)","Linux_i686")
    LIBNAMES += /usr/lib/mysql/libmysqlclient.so
  endif


  ifneq ("$(VMETYPE)","")
    ifeq ("$(VMETYPE)","fanuc")
      LIBNAMES += $(CODA)/src/jvme_fanuc/libjvme.so
    else
      LIBNAMES += $(CODA)/src/jvme/$(OSTYPE_MACHINE)/lib/libjvme.so
    endif
    LIBNAMES += $(LIBCONFIG)/.libs/libconfig.a
    #LIBNAMES += $(LIBCONFIG)/.libs/libconfig.so
  endif


  ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64_vme")
    LIBNAMES += $(CODA)/src/ipc/$(OSTYPE_MACHINE)/lib/libipc.a
    LIBNAMES += $(IPC_LIB)
    LIBNAMES += -lapr-1
    LIBNAMES += -lrt -lcrypt -lssl -lcrypto -ldl -lz
    LIBNAMES += -L/usr/kerberos/lib -lpthread -pthread -Wl,-rpath -Wl,/usr/local/lib
  endif


  ifeq ("$(OSTYPE_PLATFORM)","Linux_x86_64")
    LIBNAMES += $(CODA)/src/ipc/$(OSTYPE_MACHINE)/lib/libipc.a
    LIBNAMES += $(IPC_LIB)
    LIBNAMES += -lapr-1
    LIBNAMES += -lrt -lcrypt -lssl -lcrypto -ldl -lz
    LIBNAMES += -L/usr/kerberos/lib -lpthread -pthread -Wl,-rpath -Wl,/usr/local/lib
  endif


  ifeq ("$(OSTYPE_PLATFORM)","Linux_i686")
    LIBNAMES += $(CODA)/src/ipc/$(OSTYPE_MACHINE)/lib/libipc.a
    LIBNAMES += $(IPC_LIB)
    LIBNAMES += -lapr-1
    LIBNAMES += -lrt -lcrypt -lssl -lcrypto -ldl -lz
    LIBNAMES += -L/usr/kerberos/lib -lpthread -pthread -Wl,-rpath -Wl,/usr/local/lib
  endif




#cern root

# root's uses -std=c++17, we do not want that
#  ROOTCFLAGS   := $(shell root-config --cflags)
#  ROOTLIBS     := $(shell root-config --libs) 
#  ROOTGLIBS    := $(shell root-config --glibs)


  LIBNAMES += -lpthread
  LIBNAMES += -lexpat

ifeq ("$(OSTYPE_MACHINE)","Linux_i686_vme")

  LIBNAMES += $(CODA)/src/codautil/$(OSTYPE_MACHINE)/lib/libcodautil.a

  LIBNAMES += $(CODA)/src/ipc/$(OSTYPE_MACHINE)/lib/libipc.a
  LIBNAMES += $(IPC_LIB)
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

#
# must install librol.so for VXWORKS only !!!!!!!!!!
#install_rol

install: install_obj install_lib install_bin install_rol
	rm -rf tmp
	mkdir tmp
	cp code.s/*.h tmp
#	cp sfi.s/*.h tmp
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
