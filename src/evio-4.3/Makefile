#
# Makefile for evio/
#

#-----------------------------
# mandatory definitions: MAIN
#-----------------------------
MAIN = evio

#------------------
# generic Makefile
#------------------
include $(CODA)/src/Makefile.include

#-----------------------
# customize definitions
#-----------------------
CLON_FLAGS += -I./src.s -I./fadc.s -I$(CLON)/src/trigger/ec.s -I$(CLON)/src/trigger/pc.s

LIBNAMES += -lpthread -lexpat -lz

install: install_lib
	-mkdir -p $(globalbin)
	cp $(localbin)/evio2xml $(globalbin)
	cp $(localbin)/evio2xml4.0 $(globalbin)
	cp $(localbin)/evioUtil* $(globalbin)
#
	rm -rf tmp
	mkdir tmp
	cp src.s/evio.h tmp/
	cp src.s/evioBankUtil.h tmp/
	cp fadc.s/fadc_decompress.h tmp/
	chmod 664 tmp/*
	-mkdir -p $(CODA)/common/include
	cp tmp/* $(CODA)/common/include
	rm -rf tmp

