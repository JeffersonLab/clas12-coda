#
# Makefile for coda/ipc/
#

#-----------------------------
# mandatory definitions: MAIN
#-----------------------------
MAIN = ipc


#------------------
# generic Makefile
#------------------
include $(CODA)/src/Makefile.include



#-----------------------
# customize definitions
#-----------------------

#RCDB
CLON_FLAGS += -std=c++11 -DRCDB_MYSQL -I/usr/include/mysql -I$(RCDB_HOME)/cpp/include
CLON_FLAGS += -I./$(MAIN).s -I$(CODA)/common/include -I$(CODA)/common/include
CLON_FLAGS += -I$(IPC_INC)

#JAVA
JFLAGS = -classpath ./:$(CLON)/common/jar/clonutil.jar:$(IMPORT_JAR)/activemq-all.jar:$(IMPORT_JAR)/qpid-jms-client-0.35.0.jar

ifeq ("$(OSTYPE_MACHINE)","Linux_i686")
  CLON_FLAGS += -I/usr/local/include/apr-1
else ifeq ("$(OSTYPE_MACHINE)","Linux_i686_vme")
  CLON_FLAGS += -I/usr/local/include/apr-1
else ifeq ("$(OSTYPE_MACHINE)","Linux_i686_RHEL5_vme")
  CLON_FLAGS += -I/usr/local/include/apr-1
else
  CLON_FLAGS += -I/usr/include/apr-1
endif

LIBNAMES += $(CODA)/src/et-14.0/$(OSTYPE_MACHINE)/lib/libet.a
LIBNAMES += $(CODA)/src/evio-4.3/$(OSTYPE_MACHINE)/lib/libevio.a
LIBNAMES += $(IPC_LIB)

ifeq ("$(OSTYPE_MACHINE)","Linux_i686")
  LIBNAMES += /usr/local/lib/libapr-1.so
  LIBNAMES += -luuid -lrt -lcrypt -lssl -lcrypto -ldl -lz
  LIBNAMES += -L/usr/kerberos/lib -lpthread -pthread -Wl,-rpath -Wl,/usr/local/lib
else
  LIBNAMES += -lapr-1
  LIBNAMES += -lrt -lcrypt -lssl -lcrypto -ldl -lz
  LIBNAMES += -L/usr/kerberos/lib -lpthread -pthread -Wl,-rpath -Wl,/usr/local/lib
endif


#for 'inet_aton'
LIBNAMES += -lresolv

ifeq ("$(OSTYPE)","SunOS")
  LIBNAMES += -lCrun
endif

ifeq ("$(OSTYPE)","Linux")
  LIBNAMES += -lpthread
endif


LIBNAMES += $(X11LIBS)
LIBNAMES += $(SYSLIBS)



#-------------------
# customize targets
#-------------------
#install: install_lib install_bin install_java
install: install_lib install_bin
	rm -rf tmp
	mkdir tmp
	cp $(MAIN).s/*.h tmp
	chmod 664 tmp/*
	-mkdir -p $(CODA)/common/include
	cp tmp/* $(CODA)/common/include
	rm -rf tmp





















