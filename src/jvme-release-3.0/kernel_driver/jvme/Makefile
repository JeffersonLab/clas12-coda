#
#	Makefile for the jvme vme kernel driver.
#
#   This program is free software; you can redistribute it and/or modify it
#	under the terms and conditions of the GNU General Public License,
#	version 2, as published by the Free Software Foundation.
#




obj-m 	:= jvme.o

KVERSION := $(KERNELRELEASE)
ifeq ($(origin KERNELRELEASE), undefined)
KVERSION := $(shell uname -r)
endif

KDIR := /lib/modules/$(KVERSION)/build
PWD := $(shell pwd)



#ifneq ($(KERNELRELEASE),)
#obj-m	:= jvme.o
#else
#KDIR	:= /lib/modules/$(shell uname -r)/build
#PWD	:= $(shell pwd)

default:
	@test -s ../Module.symvers || { echo "Need to build vme source first! Exiting..."; exit 1;}
	@ln -sf ../vme.h .
	@ln -sf ../vme_bridge.h .
	@ln -sf ../Module.symvers .
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

#endif

clean:
	rm -f jvme.o jvme.ko jvme.mod.* .jvme.* *.order *.symvers

install:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules_install
