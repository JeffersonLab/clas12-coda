#!/usr/bin/python3

import dam
import numpy as np

def fee_reg_read(fee, addr):
    d.fee_request[fee].reg_address = addr
    d.fee_request[fee].reg_read = 1
    return d.fee_reply[fee].data_read

def fee_reg_write(fee, addr, data):
    d.fee_request[fee].data_write = data
    d.fee_request[fee].reg_address = addr
    d.fee_request[fee].reg_write = 1

d = dam.dam()

#for reg in [0x0, 0x10, 0x20]:

print("reg status")

for reg in range(0,0x80, 0x10):
    print("\t0x{:04x} = 0x{:x}".format(reg,d.reg_read(reg)));

print()

for reg in range(0x800,0x920, 0x10):
    print("\t0x{:04x} = 0x{:x}".format(reg,d.reg_read(reg)));

print()

for reg in [0xA000, 0xB000, 0xC000, 0xC010, 0xC020, 0xC030]:
    print("\t0x{:04x} = 0x{:x}".format(reg,d.reg_read(reg)));

print()

for i in range(0, 8):
    print(i, hex(fee_reg_read(i, 0x200)));
