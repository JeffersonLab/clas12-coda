CFLAGS_cmem_rcc.o := -I$(src)/../../cmem/ -DDRIVER_ERROR -DDRIVER_DEBUG

obj-m += cmem_rcc.o

KVERSION := $(KERNELRELEASE)
ifeq ($(origin KERNELRELEASE), undefined)
KVERSION := $(shell uname -r)
endif

KDIR := /lib/modules/$(KVERSION)/build
PWD := $(shell pwd)

EXTRA_CFLAGS := -Wno-unused -DDEBUG

all:
	@test -s ../Module.symvers || { echo "Need to build vme source first! Exiting..."; exit 1;}
	@ln -sf ../Module.symvers .
	sh ../write_version.sh
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
