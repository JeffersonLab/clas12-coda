#ifndef __SIS3801H__
#define __SIS3801H__
/***************************************************************************
                          sis3801.h   Scaler 3801 GmbH
                             -------------------
    begin                : Mon Sep 9 09:15:00 UTC 2002
    email                : tanest@jlab.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define STR7201_BOARD_ID       0x0202
#define SIS3801_BOARD_ID       0x3801
#define SIS3820_BOARD_ID       0x3820
#define SIS_MAX_SLOTS              21

/* Level 2 defs */
#define NL2CH 30
#define L2SCALER 0x200000

typedef struct sis3801_struct
{
  /* 0x0000 */ volatile unsigned int csr;
  /* 0x0004 */ volatile unsigned int irq;
  /* 0x0008          */ unsigned int _blank0;
  /* 0x000C */ volatile unsigned int copy_disable;
  /* 0x0010 */ volatile unsigned int write_to_fifo;
  /* 0x0014          */ unsigned int _blank1[(0x20 - 0x14) >> 2];
  /* 0x0020 */ volatile unsigned int clear;
  /* 0x0024 */ volatile unsigned int next;
  /* 0x0028 */ volatile unsigned int enable;
  /* 0x002C */ volatile unsigned int disable;
  /* 0x0030          */ unsigned int _blank2[(0x50 - 0x30) >> 2];
  /* 0x0050 */ volatile unsigned int enable_ref1;
  /* 0x0054 */ volatile unsigned int disable_ref1;
  /* 0x0058          */ unsigned int _blank3[(0x60 - 0x58) >> 2];
  /* 0x0060 */ volatile unsigned int reset;
  /* 0x0064          */ unsigned int _blank4;
  /* 0x0068 */ volatile unsigned int test;
  /* 0x006C          */ unsigned int _blank5[(0x80 - 0x6C) >> 2];
  /* 0x0080 */ volatile unsigned int prescale;
  /* 0x0084          */ unsigned int _blank6[(0x100 - 0x84) >> 2];
  /* 0x0100 */ volatile unsigned int fifo[(0x200 - 0x100) >> 2];
} SIS3801;

#define ENABLE_EXT_NEXT   0x00010000
#define DISABLE_EXT_NEXT  0x01000000
#define ENABLE_EXT_CLEAR  0x00020000
#define DISABLE_EXT_CLEAR 0x02000000
#define ENABLE_EXT_DIS    0x00040000
#define DISABLE_EXT_DIS   0x04000000

#define ENABLE_MODE0      0x00000000
#define ENABLE_MODE1      0x00000004
#define ENABLE_MODE2      0x00000008
#define ENABLE_MODE3      0x0000000c
#define DISABLE_MODES     0x00000c00

#define ENABLE_TEST       0x00000020
#define DISABLE_TEST      0x00002000

#define ENABLE_10MHZ_TO_LNE_PRESCALER   (1<<6)
#define DISABLE_10MHZ_TO_LNE_PRESCALER  (1<<14)

#define ENABLE_PRESCALER  (1<<7)
#define DISABLE_PRESCALER (1<<15)

#define ENABLE_25MHZ      0x00000010
#define DISABLE_25MHZ     0x00001000

#define ENABLE_IRQ        0x00f00000
#define ENABLE_IRQ0       0x00100000
#define ENABLE_IRQ1       0x00200000
#define ENABLE_IRQ2       0x00400000
#define ENABLE_IRQ3       0x00800000

#define DISABLE_IRQ       0xf0000000
#define DISABLE_IRQ0      0x10000000
#define DISABLE_IRQ1      0x20000000
#define DISABLE_IRQ2      0x40000000
#define DISABLE_IRQ3      0x80000000

/* interupt vector */
#define SCAL_VME_INT_LEVEL   5
#define SCAL_INT_VEC         0xec
/* masks for status register */

#define FIFO_FULL         0x00001000
#define FIFO_ALMOST_FULL  0x00000800
#define FIFO_HALF_FULL    0x00000400
#define FIFO_ALMOST_EMPTY 0x00000200
#define FIFO_EMPTY        0x00000100

/* Initialization flags */
#define S3801_INIT_INPUT_MODE_FLAG 0x000F
#define S3801_INIT_NO_INIT (1<<15)

struct flex_struct
{
  /* 0x00 */ volatile unsigned short csr1;
  /* 0x02 */ volatile unsigned short port1_data;
  /* 0x04 */ volatile unsigned short csr2;
  /* 0x06 */ volatile unsigned short port2_data;
  /* 0x08 */ volatile unsigned short vector;
};


/* Function prototypes */
int  sis3801Init(unsigned int addr, unsigned int addr_inc, int nsis, int iFlag);
int  sis3801CheckAddresses();
int  sis3801setinputmode(int id, int mode);
int  sis3801readfifo(int id);
void sis3801writefifo(int id, int value);
int  sis3801status(int id);
void sis3801control(int id, int value);
void sis3801mask(int id, int value);
void sis3801clear(int id);
void sis3801nextclock(int id);
void sis3801enablenextlogic(int id);
void sis3801disablenextlogic(int id);
void sis3801reset(int id);
void sis3801testclock(int id);
void sis3801setlneprescaler(int id, unsigned int prescale);
void sis3801Uledon(int id);
void sis3801Uledoff(int id);
int  sis3801almostread(int id, unsigned int *value);
int  sis3801read(int id, unsigned int *value);
unsigned int sis3801GetAddress(int id);

STATUS scalIntInit(int id, UINT32 vector);
STATUS scalIntConnect(VOIDFUNCPTR routine, int arg);
STATUS scalIntEnable(UINT32 mask);
STATUS scalIntDisable(void);
int store(int id, int mask);

/* level 2 scaler functions */
int l2_init(int id);
int l2_readscaler(unsigned int counts[NL2CH]);
int l2_status();
int l2_status_dead();
int l2_status_hot();
int l2_flex_init();
int l2_ADB_reset(char *ADB_name);
int l2_ADB_reset_VMEpulse(char *ADB_name);
int l2_ADB_name2id(char *ADB_name, int prn);
void l2_status_dead_reset_STOP();

int l2_trigger_ERROR_set();
int l2_trigger_ERROR_clear();

int l2_reset();
int l2_status_dead_reset();

int readout1();
void scalIntUser(int arg);
void scalPrint();
void scalPulse(int count);
int l2_input(unsigned int counts[NL2CH]);
int pp();
int l2_delay();


int junk(unsigned int counts[NL2CH]);
int hist();

#endif /* __SIS3801H__ */
