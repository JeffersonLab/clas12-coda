#
# Makefile for rc/runControl/rcServer
#

#-----------------------------
# mandatory definitions: MAIN
#-----------------------------
MAIN = rcServer


#------------------
# generic Makefile
#------------------
include $(CODA)/src/Makefile.include



#-----------------------
# customize definitions
#-----------------------

#for debugging
ifeq ("$(OSTYPE)","SunOS")
  CFLAGS = -g -KPIC -mt -DSunOS -DOSTYPE="SunOS"
endif

##CLON_FLAGS += -D_TRACE_OBJECTS

###CLON_FLAGS += -DNEW_STUFF

CLON_FLAGS += -I./src.s -I../Variables/src.s -I../Common/src.s  \
	-I./Components.s -I./Constants.s -I./Network.s -I./Transitioners.s \
	-I./pixmaps -I$(CODA)/common/include -I../../cg/cg.s -I../../motif/include \
	-I../../sace3.1/include $(X11INCDIR)

LIBNAMES += ../Common/$(OSTYPE_MACHINE)/lib/libCommon.a
LIBNAMES += ../Variables/$(OSTYPE_MACHINE)/lib/libVariables.a

LIBNAMES += ../../sace3.1/$(OSTYPE_MACHINE)/lib/libsace3.1.a

LIBNAMES += $(globallib)/libcodautil.a

ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64")
  LIBNAMES += /usr/lib64/mysql/libmysqlclient.so
endif
ifeq ("$(OSTYPE_MACHINE)","Linux_i686_vme")
  LIBNAMES += /usr/lib/mysql/libmysqlclient.so
endif
ifeq ("$(OSTYPE_MACHINE)","Linux_i686")
  LIBNAMES += /usr/lib/mysql/libmysqlclient.so
endif

LIBNAMES += $(X11LIBS)
LIBNAMES += $(SYSLIBS)



ifeq ("$(OSTYPE)","SunOS")
  CLON_FLAGS += -DSYSV -DSVR4 -D_CODA_2_0_T \
	-D_CODA_USE_THREADS -DDEBUG -Dsolaris  \
	-DHAVE_UNISTD_H=1 -DHAVE_LIMITS_H=1 -DHAVE_MACROS_H=1 -DSTDC_HEADERS=1 \
	-DTK_FILE_COUNT=_cnt -Dstrtod=fixstrtod -DNEED_MATHERR=1
endif

ifeq ("$(OSTYPE)","Linux")
  CLON_FLAGS += -D_CODA_2_0_T -D_CODA_DALOG
endif

ifeq ("$(OSTYPE)","Darwin")
  CLON_FLAGS += -D_CODA_2_0_T -D_CODA_DALOG
endif

#sergey: remove remanents of tcl/tk
#############CLON_FLAGS += -DUSE_TK

#-------------------
# customize targets
#-------------------
install:
	cp $(localbin)/rcServer $(CODA)/$(OSTYPE_MACHINE)/bin/





























