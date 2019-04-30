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
include $(CODA)/src/Makefile.arch

#-----------------------
# customize definitions
#-----------------------
ifeq ("$(OSTYPE_MACHINE)","SunOS_i86pc")
#  CLON_FLAGS += -m64
  SHLD += -m64
#  LDCXX += -m64
endif


CLON_FLAGS += -I./src.s -I$(CODA)/common/include

ifneq ("$(OSTYPE)","VXWORKS")
  LIBNAMES += $(CODA)/$(OSTYPE_MACHINE)/lib/librol.a
#####  LIBNAMES += $(CODA)/src/rol/jvme/libjvme.so

  LIBNAMES += $(ROOTGLIBS)
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






























