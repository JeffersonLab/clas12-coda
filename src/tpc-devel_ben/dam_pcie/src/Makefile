CFLAGS_dam_drv.o := -DDEBUG

dam_pcie-objs := dam_drv.o dam_mod.o
obj-m += dam_pcie.o

KVERSION := $(KERNELRELEASE)
ifeq ($(origin KERNELRELEASE), undefined)
KVERSION := $(shell uname -r)
endif

KDIR := /lib/modules/$(KVERSION)/build
PWD := $(shell pwd)

EXTRA_CFLAGS := -I$(src)/../common/

all:
	sh ../utils/write_version.sh
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
