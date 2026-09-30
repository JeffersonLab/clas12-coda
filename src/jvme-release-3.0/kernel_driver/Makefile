#
#	Makefile for the vme driver.
#
# Copyright:
# 	Copyright 2013-2014 Abaco Systems, Inc.
#
#	This program is free software; you can redistribute  it and/or modify it
#	under  the terms of  the GNU General  Public License as published by the
#	Free Software Foundation;  either version 2 of the  License, or (at your
#	option) any later version

obj-m := vme.o

KVERSION := $(KERNELRELEASE)
ifeq ($(origin KERNELRELEASE), undefined)
KVERSION := $(shell uname -r)
endif

all:
	$(MAKE) -C /lib/modules/$(KVERSION)/build M=$(CURDIR) modules
	cat Module.symvers >> bridges/Module.symvers
	$(MAKE) -C bridges
	$(MAKE) -C jvme
	$(MAKE) -C cmem

clean:
	$(MAKE) -C /lib/modules/$(KVERSION)/build M=$(CURDIR) clean
	$(MAKE) -C bridges clean
	$(MAKE) -C jvme clean
	$(MAKE) -C cmem clean

install:
	$(MAKE) -C /lib/modules/$(KVERSION)/build M=$(CURDIR) modules_install
	$(MAKE) -C bridges install
	$(MAKE) -C jvme install
	$(MAKE) -C cmem install
