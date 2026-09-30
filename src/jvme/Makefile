#
# Makefile for jvme/
#

#-----------------------------
# mandatory definitions: MAIN
#-----------------------------
MAIN = jvme


#------------------
# generic Makefile
#------------------
PROJECT = jvme
include $(CODA)/src/Makefile.include


# uncomment following piece for debugging ###
#CFLAGS = -g -fPIC -DLinux -DLINUX
#ifeq ("$(OSTYPE_MACHINE)","Linux_i686_vme")
#  CFLAGS += -DLinux_vme -I$(CODA)/src/rol/jvme
#endif
#ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64")
#  CFLAGS += -DLinux_x86_64
#endif
#############################################

#-----------------------
# customize definitions
#-----------------------

CLON_FLAGS += -I./$(MAIN).s -I$(CODA)/common/include
ifeq ("$(VMETYPE)","fanuc")
  CLON_FLAGS += -DGEFANUC
  CLON_FLAGS += -D_GNU_SOURCE
endif
ifeq ("$(VMETYPE)","newdrv")
  CLON_FLAGS += -DNEWDRV -DARCH_x86_64
endif


#MYFILE=src.s/jvme.h
#ifneq ($(findstring ${MYFILE},$(wildcard ./src.s/*)),)
#	ALL = all
#else
#	ALL = createlinks all
#endif
#
#aal: $(ALL)
#
#clean: cleanlinks


createlinks:
	cd src.s; ln -s ../../jvme-release-3.0/ca91c042.h ca91c042.h
	- cd src.s; ln -s ../../jvme-release-3.0/dmaPList.h dmaPList.h
	- cd src.s; ln -s ../../jvme-release-3.0/jvmeControl.h jvmeControl.h
	- cd src.s; ln -s ../../jvme-release-3.0/jvme.h jvme.h
	- cd src.s; ln -s ../../jvme-release-3.0/jvmeTsi148.h jvmeTsi148.h
	- cd src.s; ln -s ../../jvme-release-3.0/jvmeUniverseII.h jvmeUniverseII.h
	- cd src.s; ln -s ../../jvme-release-3.0/jvmeVIVO.h jvmeVIVO.h
	- cd src.s; ln -s ../../jvme-release-3.0/jvmeWindows.h jvmeWindows.h
	- cd src.s; ln -s ../../jvme-release-3.0/tsi148.h tsi148.h
	- cd src.s; ln -s ../../jvme-release-3.0/dmaPList.c dmaPList.c
	- cd src.s; ln -s ../../jvme-release-3.0/jvme.c jvme.c
	- cd src.s; ln -s ../../jvme-release-3.0/jvmeControl.c jvmeControl.c
	- cd src.s; ln -s ../../jvme-release-3.0/jvmeTsi148.c jvmeTsi148.c
	- cd src.s; ln -s ../../jvme-release-3.0/jvmeUniverseII.c jvmeUniverseII.c
	- cd src.s; ln -s ../../jvme-release-3.0/jvmeVIVO.c jvmeVIVO.c
	- cd src.s; ln -s ../../jvme-release-3.0/jvmeWindows.c jvmeWindows.c
	- cd src.s; ln -s ../../jvme-release-3.0/cmem cmem
	- cd src.s; ln -s ../../jvme-release-3.0/cmem/cmem_lib.c cmem_lib.c
	- cd src.s; ln -s ../../jvme-release-3.0/cmem/cmem_rcc.h cmem_rcc.h
	- cd src.s; ln -s ../../jvme-release-3.0/cmem/cmem_rcc_common.h cmem_rcc_common.h

cleanlinks:
	cd src.s; rm -f ca91c042.h
	- cd src.s; rm  -f dmaPList.h
	- cd src.s; rm  -f jvmeControl.h
	- cd src.s; rm  -f jvme.h
	- cd src.s; rm  -f jvmeTsi148.h
	- cd src.s; rm  -f jvmeUniverseII.h
	- cd src.s; rm  -f jvmeVIVO.h
	- cd src.s; rm  -f jvmeWindows.h
	- cd src.s; rm  -f tsi148.h
	- cd src.s; rm  -f dmaPList.c
	- cd src.s; rm  -f jvme.c
	- cd src.s; rm  -f jvmeControl.c
	- cd src.s; rm  -f jvmeTsi148.c
	- cd src.s; rm  -f jvmeUniverseII.c
	- cd src.s; rm  -f jvmeVIVO.c
	- cd src.s; rm  -f jvmeWindows.c
	- cd src.s; rm  -f cmem
	- cd src.s; rm  -f cmem_lib.c
	- cd src.s; rm  -f cmem_rcc.h
	- cd src.s; rm  -f cmem_rcc_common.h


#print:
#	echo "ALL >$(ALL)<"



####################
#
####################


#  ifeq ("$(OSTYPE_MACHINE)","Linux_i686_vme")
#    LIBNAMES += $(CODA)/src/rol/jvme/libjvme.so
#    LIBNAMES += /usr/lib/gef/libvme.a
#  endif
#  ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64_vme")
#    LIBNAMES += $(CODA)/src/jvme/$(OSTYPE_MACHINE)/lib/libjvme.so
#    LIBNAMES += /usr/lib64/gef/libvme.a
#  endif




####################
#
####################

#-------------------
# customize targets
#-------------------
install: install_lib
	rm -rf tmp
	mkdir tmp
	cp src.s/jvme.h tmp/
	cp src.s/usrvme.h tmp/
	cp src.s/dmaPList.h tmp/
	chmod 664 tmp/*
	-mkdir -p $(CODA)/common/include
	cp tmp/* $(CODA)/common/include
	rm -rf tmp

