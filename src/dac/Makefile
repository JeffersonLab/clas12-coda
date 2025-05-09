#
# Makefile for dac/
#

#-----------------------------
# mandatory definitions: MAIN
#-----------------------------
MAIN = dac


#------------------
# generic Makefile
#------------------
include $(CODA)/src/Makefile.include



# uncomment following piece for debugging ###
CFLAGS = -g -fPIC -DLinux -DLINUX
ifeq ("$(OSTYPE_MACHINE)","Linux_i686_vme")
  CFLAGS += -DLinux_vme -I$(CODA)/src/rol/jvme
endif
ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64")
  CFLAGS += -DLinux_x86_64
endif
ifeq ("$(OSTYPE_MACHINE)","Linux_armv7l")
  CFLAGS += -DLinux_armv7l
endif
#############################################





ifeq ("$(OSTYPE_MACHINE)","Linux_armv7l")
  LDCC = $(LDCXX)
endif

ifeq ("$(OSTYPE_MACHINE)","SunOS_i86pc")
    CFLAGS += -DSunOS_i86pc
endif
ifeq ("$(OSTYPE_MACHINE)","SunOS_sun4u")
    CFLAGS += -DSunOS_sun4u
endif


#------------------------
# special building cases
#------------------------

ifeq ("$(OSTYPE_MACHINE)","Linux_i686_vme")

bin: $(BIN_FILES_PROG) ./$(OSTYPE_MACHINE)/bin/coda_roc_gef

  ./$(OSTYPE_MACHINE)/bin/coda_roc_gef : main/coda_roc.c $(LIBNAMES)
	$(MKBINDIR)
	$(CC) $(CCBINOPT)
	$(CCLINKBIN)

###	$(CC) -DNEW_ROC $(CCBINOPT)

endif

ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64_vme")

bin: $(BIN_FILES_PROG) ./$(OSTYPE_MACHINE)/bin/coda_roc_gef

  ./$(OSTYPE_MACHINE)/bin/coda_roc_gef : main/coda_roc.c $(LIBNAMES)
	$(MKBINDIR)
	$(CC) $(CCBINOPT)
	$(CCLINKBIN)

###	$(CC) -DNEW_ROC $(CCBINOPT)

endif





#-----------------------
# customize definitions
#-----------------------

CLON_FLAGS += -I./$(MAIN).s \
	-I$(CODA)/common/include \
	-D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS \
	-DHAVE_GETCWD=1 -DHAVE_OPENDIR=1 -DHAVE_STRERROR=1 \
	-DHAVE_STRSTR=1 -DHAVE_STRTOL=1 -DHAVE_TMPNAM=1 \
	-DHAVE_WAITPID=1 -DHAVE_STRDUP=1 -DHAVE_STRERROR=1 \
	-DHAVE_STRDUP=1 -DHAVE_STRCASECMP=1 -DHAVE_UNISTD_H=1 \
	-DHAVE_ERRNO_H=1 -DHAVE_FCNTL_H=1 -DNO_STREAM_H=1 -DNO_UN_H=1 \
	-DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_TOLOWER=1 \
	-DSTDC_HEADERS=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 \
	-DHAVE_FLOAT_H=1 -DHAVE_MALLOC_H=1 -DHAVE_ERRNO_H=1 \
	-DHAVE_UNISTD_H=1 -DHAVE_MEMORY_H=1 -DHAVE_LIMITS_H=1 \
	-DHAVE_SYS_TIME_H=1 -DHAVE_SYS_WAIT_H=1 \
	-DTIME_WITH_SYS_TIME=1 -DNO_UNION_WAIT=1 -DNEED_MATHERR=1 \
	-DSVIPC_NO_UNION_SEMUN=1 -DTK_FILE_COUNT=_cnt \
		-D_REENTRANT \
		-D__EXTENSIONS__ \
		-DCODA_USER="\"${USER}\"" \
		-DVERSION="\"$(CODA_VERSION)\"" -D_LANGUAGE_C \
		-DDAYTIME="\"`date`\""
CLON_FLAGS += -DNO_X11






####################
#
####################


ifeq ("$(OSTYPE)","VXWORKS")

  LIBNAMES += ./VXWORKS_ppc/lib/libdac.so

else ifeq ("$(OSTYPE_MACHINE)","Linux_armv7l")

  LIBNAMES += ./Linux_armv7l/lib/libdac.a
  LIBNAMES += $(globallib)/libcodautil.a

  LIBNAMES += /usr/lib/libmysqlclient.so

  LIBNAMES += $(CODA)/Linux_armv7l/lib/libet.a
  LIBNAMES += $(CODA)/Linux_armv7l/lib/libevio.a
  LIBNAMES += $(CODA)/Linux_armv7l/lib/libcodautil.a

  LIBNAMES += $(CODA)/src/ipc/$(OSTYPE_MACHINE)/lib/libipc.a
  LIBNAMES += $(IPC_HOME)/lib/libactivemq-cpp.so
  LIBNAMES += -lapr-1
  LIBNAMES += -lrt -lcrypt -lssl -lcrypto -ldl -lz
  LIBNAMES += -L/usr/kerberos/lib -lpthread -pthread -Wl,-rpath -Wl,/usr/local/lib





else ifeq ("$(OSTYPE_MACHINE)","Linux_nios2")

  LIBNAMES += ./Linux_nios2/lib/libdac.a
  LIBNAMES += $(CODA)/Linux_nios2/lib/libevio.a
  LIBNAMES += $(CODA)/Linux_nios2/lib/libcodautil.a

else

  LIBNAMES += $(CODA)/src/creg/$(OSTYPE_MACHINE)/lib/libcreg.a
  LIBNAMES += $(CODA)/src/codautil/$(OSTYPE_MACHINE)/lib/libcodautil.a

  ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64")
    LIBNAMES += /usr/lib64/mysql/libmysqlclient.so
  endif
  ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64_RHEL9")
    #LIBNAMES += /usr/lib64/libmariadb.so
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




  ifeq ("$(OSTYPE_MACHINE)","Linux_i686_vme")
    LIBNAMES += $(CODA)/src/rol/jvme/libjvme.so
    LIBNAMES += /usr/lib/gef/libvme.a
  endif
  ifeq ("$(OSTYPE_MACHINE)","Linux_x86_64_vme")
    LIBNAMES += $(CODA)/src/jvme/$(OSTYPE_MACHINE)/lib/libjvme.so
    LIBNAMES += /usr/lib64/gef/libvme.a
  endif


  LIBNAMES += $(CODA)/src/evio-4.3/$(OSTYPE_MACHINE)/lib/libevio.a

  LIBNAMES += $(CODA)/src/et-14.0/$(OSTYPE_MACHINE)/lib/libet.a



#
# IPC library
#
  LIBNAMES += -L/usr/lib64 -lstdc++
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

endif



####################
#
####################







#ifeq ("$(OSTYPE_MACHINE)","SunOS_sun4u")
#  LIBNAMES += /usr/sfw/lib/libgcc_s.so
#endif

ifeq ("$(OSTYPE)","SunOS")
  CLON_FLAGS += -DSOLARIS=1 -DOSTYPE="SunOS"
  LIBNAMES += -lpthread
  LIBNAMES += -lresolv
  LIBNAMES += $(X11LIBS)
  LIBNAMES += $(SYSLIBS)
endif

ifeq ("$(OSTYPE)","Linux")
  CLON_FLAGS += -DLINUX=1
  CLON_FLAGS += -D_GNU_SOURCE # added 2-may-2011 to satisfy 'tee'
ifeq ("$(OSTYPE_MACHINE)","Linux_nios2")
  LIBNAMES += $(SYSLIBS)
  LIBNAMES += -lpthread
  LDFLAGS += -rdynamic
else
##  LIBNAMES += /usr/lib/libssl.so # for RHEL5 ..
  LIBNAMES += -lssl
  LIBNAMES += -lresolv
  LIBNAMES += $(X11LIBS)
  LIBNAMES += $(SYSLIBS)
  LIBNAMES += -lpthread
# Sergey: -rdynamic needed by Linux, it allows to search for global
#   symbols in main program by dlopen/dlsym
  LDFLAGS += -rdynamic
endif
endif

ifeq ("$(OSTYPE)","VXWORKS")
#  CLON_FLAGS = -I./$(MAIN).s -I$(CODA)/common/include \
#		-DNO_X11 -DNEED_ACCESS -DNO_GETTOD -DNO_VALUES_H \
#		-DNO_SYS_ERRLIST -D__PROTOTYPE_5_0 -DTCL_VW
  CLON_FLAGS = -I./$(MAIN).s \
	-I$(CODA)/src/rol/inc \
	-I$(CODA)/common/include \
	-DNO_X11 -D_GNU_TOOL -DTCL_VW -DCODA_USER="\"boiarino\"" \
	-DOSTYPE="VXWORKSPPC" -DVERSION="\"2.5\"" -DNO_GETWD=1 -DNO_WAIT3=1 \
	-DHAVE_UNISTD_H=1  -D_LANGUAGE_C -DDAYTIME="\"`date`\""
endif

ifeq ("$(OSTYPE_MACHINE)","Linux_armv7l")
  CLON_FLAGS += -I$(CODA)/Linux_armv7l/include
  LIBNAMES += $(SYSLIBS)
# Sergey: -rdynamic needed by Linux, it allows to search for global
#   symbols in main program by dlopen/dlsym
  LDFLAGS += -rdynamic
endif


#-------------------
# customize targets
#-------------------
install: install_lib install_bin
	rm -rf tmp
	mkdir tmp
	cp $(MAIN).s/rc.h tmp/
	cp $(MAIN).s/da.h tmp/
	cp $(MAIN).s/obj.h tmp/
	cp $(MAIN).s/rolInt.h tmp/
	cp $(MAIN).s/circbuf.h tmp/
	cp $(MAIN).s/helicity.h tmp/
	chmod 664 tmp/*
	-mkdir -p $(CODA)/common/include
	cp tmp/* $(CODA)/common/include
	rm -rf tmp

