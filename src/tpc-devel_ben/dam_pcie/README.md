# sPHENIX Data Aggregation Module PCIe Linux Kernel Driver

## Build Prerequisites

On Debian based systems:

```
$ sudo apt-get install build-essential kmod linux-headers-`uname -r`
```

On Arch Linux:

`$ sudo pacman -S gcc kmod linux-headers`

## What's included

- udev: udev rule and install files for `/dev/damX` 
- utils: development tools
- common: shared header files for ioctl and libdam
- src: Linux Kenerel module source
- tests: unit test for kernel module and ioctl
- libdam: userspace library for ioctl for `dam_pcie` driver
- damreg: userspace example applcation for reading and writing registers

## Building

With any luck, one can build the whole codebase by running the top level `Makefile` by invoking `make`.

## Building and Testing Kernel Module

```
$ cd src
$ make
$ sudo insmod dam_drv.ko
$ sudo dmesg | tail -n20
$ cd ../tests
$ make && ./ioctl
```
