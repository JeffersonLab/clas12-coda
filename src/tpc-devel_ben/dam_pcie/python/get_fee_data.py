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
print(fee_reg_read(0, 0x100))

