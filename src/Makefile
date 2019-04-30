#
# Makefile for coda/src/ directory
# 

MAKE = gmake

all:
	cd scripts;                          $(MAKE) install
	- cd rcscripts;                      $(MAKE) install
	- cd evio-4.3;                       $(MAKE)
	- cd evio-4.3;                       $(MAKE) install
	- cd et-14.0;                        $(MAKE)
	- cd et-14.0;                        $(MAKE) ulib
	- cd et-14.0;                        $(MAKE) install
	- cd creg;                           $(MAKE)
	- cd creg;                           $(MAKE) install
	- cd codatcl1.0;                     $(MAKE)
	- cd codatcl1.0;                     $(MAKE) install
	- cd ipc;                            $(MAKE)
	- cd ipc;                            $(MAKE) install
	- cd codautil;                       $(MAKE)
	- cd codautil;                       $(MAKE) install
	- cd codaedit;                       $(MAKE)
	- cd codaedit;                       $(MAKE) install
	- cd cterm;                          $(MAKE)
	- cd cterm;                          $(MAKE) install
#	- cd rol;                            $(MAKE) jvmes
	- cd rol/jvme;                       $(MAKE)
	- cd dac;                            $(MAKE)
	- cd dac;                            $(MAKE) install
	- cd rol;                            $(MAKE)
	- cd rol;                            $(MAKE) install
	- cd rol;                            $(MAKE) rols
	- cd rol;                            $(MAKE) install
	- cd rc;                             $(MAKE)
	- cd rc;                             $(MAKE) install
	- cd diaggui;                        $(MAKE)
	- cd diaggui;                        $(MAKE) install

clean:
	cd scripts;                          $(MAKE) clean
	- cd rcscripts;                      $(MAKE) clean
	- cd evio-4.3;                       $(MAKE) clean
	- cd et-14.0;                        $(MAKE) clean
	- cd codatcl1.0;                     $(MAKE) clean
	- cd creg;                           $(MAKE) clean
	- cd codaedit;                       $(MAKE) clean
	- cd codaterm;                       $(MAKE) clean
	- cd dac;                            $(MAKE) clean
	- cd rol;                            $(MAKE) clean
	- cd rol/jvme;                       $(MAKE) clean
	- cd rc;                             $(MAKE) clean
	- cd diaggui;                        $(MAKE) clean


distclean:
	rm -rf $(CODA)/common/*
	rm -rf $(CODA)/$(OSTYPE_MACHINE)/*


#with cross-compiling
#arm:
#	cd mysql4.1.20;                      make all     ARM
#	- cd mysql4.1.20;                    make install ARM
#	- cd codatt;                         make all     ARM
#	- cd codatt;                         make install ARM
#	- cd dac;                            make all     ARM
#	- cd dac;                            make install ARM
#	- cd rol;                            make all     ARM
#	- cd rol;                            make rols    ARM
#	- cd rol;                            make install ARM
#
#cleanarm:
#	cd mysql4.1.20;                      make clean   ARM
#	- cd codatt;                         make clean   ARM
#	- cd dac;                            make clean   ARM
#	- cd rol;                            make clean   ARM

arm:
	cd evio-4.3;                         make
	- cd evio-4.3;                       make install
	- cd et-14.0;                        make
	- cd et-14.0;                        make ulib
	- cd et-14.0;                        make install
	- cd ipc;                            make
	- cd ipc;                            make install
	- cd codautil;                       make
	- cd codautil;                       make install
	- cd dac;                            make     
	- cd dac;                            make install 
	- cd vtp;                            make     
	- cd vtp;                            make install 
	- cd rol;                            make     
	- cd rol;                            make install 
	- cd rol;                            make rols    
	- cd rol;                            make install 

cleanarm:
	cd ipc;                              make clean
	- cd codautil;                       make clean
	- cd evio-4.3;                       make clean
	- cd et-14.0;                        make clean 
	- cd dac;                            make clean   
	- cd vtp;                            make clean   
	- cd rol;                            make clean   


###########################################



