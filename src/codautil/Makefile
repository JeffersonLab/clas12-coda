#
# Makefile for coda/codautil/
#

#-----------------------------
# mandatory definitions: MAIN
#-----------------------------
MAIN = codautil


#------------------
# generic Makefile
#------------------
include $(CODA)/src/Makefile.include



#-----------------------
# customize definitions
#-----------------------

CLON_FLAGS += -I./$(MAIN).s \
				-I$(CODA)/common/include -I$(CODA)/common/include \
				-I$(IPC_INC) \
				-I$(CODA)/src/ipc/ipc.s

CLON_FLAGS += -I/usr/include/mysql

ifeq ("$(OSTYPE_MACHINE)","Linux_i686")
  CLON_FLAGS += -I/usr/local/include/apr-1
else ifeq ("$(OSTYPE_MACHINE)","Linux_i686_vme")
  CLON_FLAGS += -I/usr/local/include/apr-1
else ifeq ("$(OSTYPE_MACHINE)","Linux_i686_RHEL5_vme")
  CLON_FLAGS += -I/usr/local/include/apr-1
else
  CLON_FLAGS += -I/usr/include/apr-1
endif


LIBNAMES += $(CODA)/src/ipc/$(OSTYPE_MACHINE)/lib/libipc.a

ifeq ("$(OSTYPE_MACHINE)","Linux_armv7l_Arch")
  LIBNAMES += /usr/lib/libmysqlclient.so
else ifeq ("$(OSTYPE_MACHINE)","Linux_armv7l_RHEL7")
  LIBNAMES += /usr/lib/mysql/libmysqlclient.so
else ifeq ("$(OSTYPE_PLATFORM)","Linux_x86_64")
  LIBNAMES += /usr/lib64/mysql/libmysqlclient.so
else ifeq ("$(OSTYPE_PLATFORM)","Linux_i686")
    LIBNAMES += /usr/lib/mysql/libmysqlclient.so
endif

LIBNAMES += $(CODA)/src/et-14.0/$(OSTYPE_MACHINE)/lib/libet.a
LIBNAMES += $(CODA)/src/evio-4.3/$(OSTYPE_MACHINE)/lib/libevio.a
LIBNAMES += $(IPC_LIB)

ifeq ("$(OSTYPE_PLATFORM)","Linux_i686")
  LIBNAMES += /usr/local/lib/libapr-1.so
  LIBNAMES += -luuid -lrt -lcrypt -lssl -lcrypto -ldl -lz
  LIBNAMES += -L/usr/kerberos/lib -lpthread -pthread -Wl,-rpath -Wl,/usr/local/lib
else
  LIBNAMES += -lapr-1
  LIBNAMES += -lrt -lcrypt -lssl -lcrypto -ldl -lz
  LIBNAMES += -L/usr/kerberos/lib -lpthread -pthread -Wl,-rpath -Wl,/usr/local/lib
endif

#for old solaris only ???
LIBNAMES += -lresolv

LIBNAMES += $(X11LIBS)
LIBNAMES += $(SYSLIBS)



#-------------------
# customize targets
#-------------------
install: install_lib install_bin
	rm -rf tmp
	mkdir tmp
	cp $(MAIN).s/*.h tmp
	chmod 664 tmp/*
	cp tmp/* $(CODA)/common/include
	rm -rf tmp





















