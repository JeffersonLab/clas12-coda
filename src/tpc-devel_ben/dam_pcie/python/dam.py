#!/usr/bin/env python3
import mmap
import struct
import time

BLOCK_SIZE = (1 << 16)-1
BAR2_OFFSET = 0

reg_field_dma_busy_status = {
  "TOHOST_BUSY" : (0x1,  0),
  "TOHOST_BUSY_LATCHED" : (0xe,  1),
  "ENABLE" : (0x10,  4),
}

reg_field_int_test = {
  "IRQ" : (0xf,  0),
}

reg_field_generic_constants = {
  "DESCRIPTORS" : (0xff,  0),
  "INTERRUPTS" : (0xff00,  8),
}

reg_field_dma_test = {
  "LENGTH" : (0xffff,  0),
}

reg_field_si5345_pll = {
  "i2c_busy" : (0x1,  0),
  "nLOL" : (0x2,  1),
  "program" : (0x4,  2),
}

reg_field_fee_request = {
  "data_write" : (0xffffffff,  0),
  "reg_address" : (0xffff00000000, 32),
  "reg_write" : (0x1000000000000, 48),
  "reg_read" : (0x2000000000000, 49),
}

reg_field_fee_reply = {
  "data_read" : (0xffffffff,  0),
  "reg_address" : (0xffff00000000, 32),
  "reg_fail" : (0x1000000000000, 48),
  "reg_ack" : (0x2000000000000, 49),
  "rx_ready" : (0x4000000000000, 50),
}

reg_field_fee_link_block_cnt = {
  "rx_eob" : (0xffffffff,  0),
  "rx_sob" : (0xffffffff00000000, 32),
}

reg_field_fee_link_crc_errors = {
  "control_data" : (0xffff,  0),
  "stream_data" : (0xffff0000, 16),
  "fifo_full" : (0xffff00000000, 32),
}


regs = {
  "reg_map_version" : (0x0000, 1, 16, None),
  "board_id_timestamp" : (0x0010, 1, 16, None),
  "git_commit_time" : (0x0020, 1, 16, None),
  "git_tag" : (0x0030, 1, 16, None),
  "git_commit_number" : (0x0040, 1, 16, None),
  "git_hash" : (0x0050, 1, 16, None),
  "status_leds" : (0x0060, 1, 16, None),
  "dma_busy_status" : (0x0070, 1, 16, reg_field_dma_busy_status),
  "int_test" : (0x0800, 1, 16, reg_field_int_test),
  "generic_constants" : (0x0810, 1, 16, reg_field_generic_constants),
  "num_of_channels" : (0x0820, 1, 16, None),
  "card_type" : (0x0830, 1, 16, None),
  "opto_trx_num" : (0x0840, 1, 16, None),
  "cr_internal_loopback_mode" : (0x0850, 1, 16, None),
  "debug_mode" : (0x0860, 1, 16, None),
  "firmware_mode" : (0x0870, 1, 16, None),
  "gtrefclk_source" : (0x0880, 1, 16, None),
  "blocksize" : (0x0890, 1, 16, None),
  "pcie_endpoint" : (0x08A0, 1, 16, None),
  "scratch_pad" : (0x08B0, 1, 16, None),
  "dma_test" : (0x08C0, 1, 16, reg_field_dma_test),
  "lmk_locked" : (0x08D0, 1, 16, None),
  "phy_reset" : (0x08E0, 1, 16, None),
  "si5345_pll" : (0x08F0, 1, 16, reg_field_si5345_pll),
  "fee_stream_enable" : (0x0900, 1, 16, None),
  "dma_packet_chunk_size" : (0x0910, 1, 16, None),
  "dma_fifo_full_count" : (0x0920, 1, 16, None),
  "fee_request" : (0xA000, 8, 128, reg_field_fee_request),
  "fee_reply" : (0xB000, 8, 128, reg_field_fee_reply),
  "tx_locked" : (0xC000, 1, 16, None),
  "rx_locked" : (0xC010, 1, 16, None),
  "gtm_recv_clock_freq" : (0xC020, 1, 16, None),
  "gtm_bco" : (0xC030, 1, 16, None),
#  "fee_link_counters" : (0xD000, 8, 256, reg_field_fee_link_counters),
#  "evt_stat" : (0xE000, 8, 384, reg_field_evt_stat),
#  "evt_ctrl" : (0xF000, 8, 256, reg_field_evt_ctrl),
}

class reg_field(object):
    def __init__(self, dam, base_addr, count, size, field):
        self.__dam__ = dam
        self.__field__ = field
        self.base_addr = base_addr
        self.count = count
        self.size = size

    def __repr__(self):
        return f'base_addr:{self.base_addr}, {self.__field__.keys()} , {self.count}'

    def __len__(self):
        return self.count

    def __iter__(self):
        self.offset = 0
        return self

    def __next__(self):
        if (self.offset <= self.count):
            self.offset += 1
            return reg_field(self.__dam__, self.base_addr+(self.offset-1)*(self.size//self.count), 1, 16, self.__field__)
        else:
            raise StopIteration

    def __setitem__(self, key, value):
        if (self.count > 1):
            return reg_field(self.__dam__, self.base_addr+int(key)*(self.size//self.count), 1, 16, self.__field__)
        else:
            mask, shift = self.__field__[key]
            val = self.__dam__.reg_read(self.base_addr)
            val = (val & ((1 << shift)-1)) | (value << shift)
            self.__dam__.reg_write(self.base_addr, val)

    def __getitem__(self, key):
        if (self.count > 1):
            return reg_field(self.__dam__, self.base_addr+int(key)*(self.size//self.count), 1, 16, self.__field__)
        else:
            mask, shift = self.__field__[key]
            return (self.__dam__.reg_read(self.base_addr) & mask) >> shift

    def __setattr__(self, key, value):
        if key.startswith('_'):
            super().__setattr__(key, value)
            return

        if (key in self.__field__):
            self.__setitem__(key, value)
        else:
            super().__setattr__(key, value)

    def __getattr__(self, key):
        if key.startswith('_'):
            super().__getattr__(key)
            return

        if (key in self.__field__):
            return self.__getitem__(key)
        else:
            return object.__getattribute__(self, key)

    def __dir__(self):
        return self.__field__.keys()


class dam(object):
    def __init__(self, dev_name="/dev/dam0"):
        self.__fp__ = open(dev_name, 'r+b', buffering=0)
        self.mem = mmap.mmap(self.__fp__.fileno(), BLOCK_SIZE, offset=BAR2_OFFSET)

    def read(self, length):
        return self.__fp__.read(length)

    def reg_read(self, addr):
        ''' Read a 64-bit word (8 bytes) '''
        return struct.unpack("<Q", self.mem[addr:addr+8])[0]

    def reg_write(self, addr, data):
        ''' Write a 64-bit word into addr '''
        self.mem[addr:addr+8] = struct.pack("<Q", data)

    def keys(self):
        return regs.keys()

    def __len__(self):
        return len(regs)

    def __setitem__(self, key, value):
        addr, count, size, fields = regs[key]
        if (fields == None):
            self.reg_write(addr, value)
        else:
            return reg_field(self, addr, count, size, fields)

    def __getitem__(self, key):
        addr, count, size, fields = regs[key]
        if (fields == None):
            return self.reg_read(addr)
        else:
            return reg_field(self, addr, count, size, fields)

    def __setattr__(self, key, value):
        if (key in regs):
            self.__setitem__(key, value)
        else:
            super().__setattr__(key, value)

    def __getattr__(self, key):
        if (key in regs):
            return self.__getitem__(key)
        else:
            return object.__getattribute__(self, key)

    def __dir__(self):
        return sorted(list(regs.keys()) + ["keys", "reg_read", "reg_write"])
