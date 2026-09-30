#!/bin/bash

rmmod cmem_rcc
rmmod jvme
rmmod vme_vivo
rmmod vme_ca91cx42
rmmod vme_tsi148
rmmod vme

insmod vme.ko
insmod bridges/vme_tsi148.ko
insmod bridges/vme_ca91cx42.ko
insmod bridges/vme_vivo.ko
insmod jvme/jvme.ko
insmod cmem/cmem_rcc.ko gfpbpa_size=512 gfpbpa_zone=1 gfpbpa_quantum=4
