#
# Makefile for rc/runControl/Variables
#

#-----------------------------
# mandatory definitions: MAIN
#-----------------------------
MAIN = Variables


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

CLON_FLAGS += -I./src.s -I./include -I.. -I../Common/src.s \
	-I./pixmaps -I$(CODA)/common/include -I../../cg/cg.s -I../../motif/include \
	-I../../sace3.1/include -I../../XmHTML-1.1.4/include $(X11INCDIR)

##CLON_FLAGS += -D_TRACE_OBJECTS

LIBNAMES += ../../sace3.1/$(OSTYPE_MACHINE)/lib/libsace3.1.a

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
