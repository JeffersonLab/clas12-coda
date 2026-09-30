#
#	Makefile for the vme bridge drivers.
#
# Copyright:
# 	Copyright 2013-2014, 2016 Abaco Systems, Inc.
#
#	This program is free software; you can redistribute  it and/or modify it
#	under  the terms of  the GNU General  Public License as published by the
#	Free Software Foundation;  either version 2 of the  License, or (at your
#	option) any later version

obj-m := vme_vivo.o vme_tsi148.o vme_ca91cx42.o

KVERSION := $(KERNELRELEASE)
ifeq ($(origin KERNELRELEASE), undefined)
KVERSION := $(shell uname -r)
endif


all:
	$(MAKE) -C /lib/modules/$(KVERSION)/build M=$(CURDIR) modules

clean:
	$(MAKE) -C /lib/modules/$(KVERSION)/build M=$(CURDIR) clean

install:
	$(MAKE) -C /lib/modules/$(KVERSION)/build M=$(CURDIR) modules_install
