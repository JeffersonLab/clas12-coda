#
# Makefile for rc/runControl/rcClient
#

#-----------------------------
# mandatory definitions: MAIN
#-----------------------------
MAIN = rcClient


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

CLON_FLAGS += -I./src.s -I./include -I../Variables/src.s -I../Common/src.s \
	-I../rcServer/Components.s \
	-I./pixmaps -I$(CODA)/common/include -I../../cg/cg.s -I../../motif/include \
	-I../../sace3.1/include -I../../XmHTML-1.1.4/include $(X11INCDIR)

##CLON_FLAGS += -D_TRACE_OBJECTS

LIBNAMES += ../Variables/$(OSTYPE_MACHINE)/lib/libVariables.a
LIBNAMES += ../rcServer/$(OSTYPE_MACHINE)/lib/librcServer.a
LIBNAMES += ../Common/$(OSTYPE_MACHINE)/lib/libCommon.a

LIBNAMES += ../../sace3.1/$(OSTYPE_MACHINE)/lib/libsace3.1.a

LIBNAMES += $(globallib)/libcodautil.a

ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64")
  LIBNAMES += /usr/lib64/mysql/libmysqlclient.so
endif
ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64_vme")
  LIBNAMES += /usr/lib64/mysql/libmysqlclient.so
endif
ifeq ("$(OSTYPE_MACHINE)","Linux_i686_vme")
  LIBNAMES += /usr/lib/mysql/libmysqlclient.so
endif
ifeq ("$(OSTYPE_MACHINE)","Linux_i686")
  LIBNAMES += /usr/lib/mysql/libmysqlclient.so
endif


CLON_FLAGS += -DUSE_CREG
LIBNAMES += $(globallib)/libcreg.a


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
	cp $(localbin)/rcClient $(CODA)/$(OSTYPE_MACHINE)/bin/





























