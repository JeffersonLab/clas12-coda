#
# Makefile for rc/runControl/Xui
#

#-----------------------------
# mandatory definitions: MAIN
#-----------------------------
MAIN = Xui


#------------------
# generic Makefile
#------------------
include $(CODA)/src/Makefile.include


#-----------------------
# customize definitions
#-----------------------

#for debugging
CLON_FLAGS += -D_TRACE_OBJECTS

CLON_FLAGS += -I./include -I../Variables/src.s -I../Common/src.s -I../rcClient/src.s \
	-I./pixmaps -I$(CODA)/common/include -I../../cg/cg.s -I../../motif/include \
	-I../../sace3.1/include $(X11INCDIR)


# rocs
CLON_FLAGS += -I$(CODA)/src/cterm/src.s -I$(CODA)/src/codaedit/codaedit.s


LIBNAMES += ../Variables/$(OSTYPE_MACHINE)/lib/libVariables.a
LIBNAMES += ../rcClient/$(OSTYPE_MACHINE)/lib/librcClient.a
LIBNAMES += ../rcServer/$(OSTYPE_MACHINE)/lib/librcServer.a
LIBNAMES += ../Common/$(OSTYPE_MACHINE)/lib/libCommon.a


LIBNAMES += ../../sace3.1/$(OSTYPE_MACHINE)/lib/libsace3.1.a
LIBNAMES += ../../motif/$(OSTYPE_MACHINE)/lib/libmotif.a
LIBNAMES += ../../cg/$(OSTYPE_MACHINE)/lib/libcg.a

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


CLON_FLAGS += -DUSE_CREG
LIBNAMES += $(globallib)/libcreg.a

LIBNAMES += $(globallib)/libcodaedit.a

#specific for cterm
LIBNAMES += $(globallib)/libcterm.a

LIBNAMES += $(TERMCAPLIBS)

LIBNAMES += -pthread

##LIBNAMES += -lXpm -lXm -lm
LIBNAMES += $(X11LIBS)


# specific for cterm: must be after X11LIBS, seems interfere with our custom motif ?????????????
###LIBNAMES += -lXaw
###LIBNAMES += $(SYSLIBS)
LIBNAMES += -lXaw





ifeq ("$(OSTYPE)","SunOS")
  CLON_FLAGS += -DSYSV -DSVR4 -D_CODA_2_0_T \
	-D_CODA_USE_THREADS -DDEBUG -Dsolaris  \
	-DHAVE_UNISTD_H=1 -DHAVE_LIMITS_H=1 -DHAVE_MACROS_H=1 -DSTDC_HEADERS=1 \
	-DTK_FILE_COUNT=_cnt -Dstrtod=fixstrtod -DNEED_MATHERR=1
endif

ifeq ("$(OSTYPE)","Linux")
#  CLON_FLAGS += -D_CODA_2_0_T -DHAVE_UNISTD_H=1 -DHAVE_LIMITS_H=1 -DHAVE_MACROS_H=1 -DSTDC_HEADERS=1 \
#	-DTK_FILE_COUNT=_cnt -Dstrtod=fixstrtod -DNEED_MATHERR=1
  CLON_FLAGS += -D_CODA_2_0_T -DHAVE_UNISTD_H=1 -DHAVE_LIMITS_H=1 -DHAVE_MACROS_H=1 -DSTDC_HEADERS=1 \
	-DTK_FILE_COUNT=_cnt -DNEED_MATHERR=1
endif

ifeq ("$(OSTYPE)","Darwin")
  CLON_FLAGS += -D_CODA_2_0_T -DHAVE_UNISTD_H=1 -DHAVE_LIMITS_H=1 -DHAVE_MACROS_H=1 -DSTDC_HEADERS=1 \
        -DTK_FILE_COUNT=_cnt -DNEED_MATHERR=1
endif

#-------------------
# customize targets
#-------------------
install:
	cp $(localbin)/runcontrol $(CODA)/$(OSTYPE_MACHINE)/bin/
	cp $(localbin)/rocs $(CODA)/$(OSTYPE_MACHINE)/bin/ 

#	@for i in $(CODA)/common/html $(CODA)/common/html/rc ; \
#	do \
#	if [ ! -d $$i ] ; then \
#		echo "Making directory $$i"; \
#		mkdir $$i; \
#		chmod 755 $$i; \
#		else true; \
#	fi; \
#	done;
#	rm -rf tmp
#	mkdir tmp
#	cp ./html/*.html tmp/
#	cp ./html/*.gif tmp/
#	chmod 664 tmp/*
#	cp tmp/* $(CODA)/common/html/rc/
#	rm -rf tmp
#





























