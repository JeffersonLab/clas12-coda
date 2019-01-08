/* vscmLib.c */

#if defined(VXWORKS) || defined(Linux_vme)

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef DEBUG

#ifdef VXWORKS
/*
#include "dmainit.h"
*/
#include <vxWorks.h>
#include <taskLib.h>

#include "sockLib.h"
#include "inetLib.h"
#include "hostLib.h"
#include "ioLib.h"

#include "wdLib.h"

#define SYNC()    { __asm__ volatile("eieio"); __asm__ volatile("sync"); }
#define VSCMLOCK
#define VSCMUNLOCK

/*defined in all_rocs.c
uint32_t
vmeRead32(volatile uint32_t *addr) {return *addr;}
void
vmeWrite32(volatile uint32_t *addr, uint32_t val) {*addr = val;}
*/

#else

#define SYNC()
#define sysClkRateGet() CLOCKS_PER_SEC

#ifdef CODA3DMA
#include "jvme.h"
#endif

/*
Override jvme.h taskDelay to reduce the delay
Use nanosleep() on Linux when more accuracy is needed
*/
#define taskDelay(ticks) usleep(ticks * 100)

#include <unistd.h>
#include <stddef.h>
#include <time.h>
#include <pthread.h>

pthread_mutex_t vscmMutex = PTHREAD_MUTEX_INITIALIZER;
#define VSCMLOCK    if (pthread_mutex_lock(&vscmMutex) < 0) \
                      perror("pthread_mutex_lock");
#define VSCMUNLOCK  if (pthread_mutex_unlock(&vscmMutex) < 0) \
                      perror("pthread_mutex_unlock");

#ifdef CODA3DMA
DMA_MEM_ID vmeIN, vmeOUT;
extern DMANODE *the_event;
extern unsigned int *dma_dabufp;
#endif

#endif

#include "vscmLib.h"

#include "xxxConfig.h"

#include "ipc.h"

static int active;

/* Define Global Variables */
int nvscm = 0;                                          /* Number of VSCMs in Crate */
volatile VSCM_regs *VSCMpr[VSCM_MAX_BOARDS + 1]; /* pointers to VSCM memory map */
volatile uintptr_t *VSCMpf[VSCM_MAX_BOARDS + 1];        /* pointers to VSCM FIFO memory */
volatile uintptr_t *VSCMpmb;                            /* pointer to Multiblock Window */
int vscmID[VSCM_MAX_BOARDS];                            /* array of slot numbers for VSCMs */

int vscmA32Base = 0x09000000;
int vscmA32Offset = 0x0;                                /* Difference in CPU A32 Base - VME A32 Base */
int vscmA24Offset = 0x0;                                /* Difference in CPU A24 Base - VME A24 Base */

int vscmInited = 0;
int minSlot = 21;
int maxSlot = 1;

int vscmSemLastHeartbeat[VSCM_MAX_BOARDS+1];

typedef struct
{
  int module;
  int region;
} BST_trans_entry;

//                           CRATE,SLOT,HFCB
BST_trans_entry BST_trans_table[2][22][2] = {
    // SVT1
    {
      // HFCB0    HFCB1
      // Mod,Reg  Mod,Reg
      { { 0, 0}, { 0, 0} }, // SLOT  0 
      { { 0, 0}, { 0, 0} }, // SLOT  1 
      { { 0, 0}, { 0, 0} }, // SLOT  2 
      { { 1, 1}, { 2, 1} }, // SLOT  3 
      { { 3, 1}, { 9, 1} }, // SLOT  4 
      { {10, 1}, { 0, 0} }, // SLOT  5 
      { { 0, 0}, { 0, 0} }, // SLOT  6 
      { { 1, 2}, { 2, 2} }, // SLOT  7 
      { { 3, 2}, { 4, 2} }, // SLOT  8 
      { {12, 2}, {13, 2} }, // SLOT  9 
      { {14, 2}, { 0, 0} }, // SLOT 10 
      { { 0, 0}, { 0, 0} }, // SLOT 11 
      { { 0, 0}, { 0, 0} }, // SLOT 12 
      { { 1, 3}, { 2, 3} }, // SLOT 13 
      { { 3, 3}, { 4, 3} }, // SLOT 14 
      { { 5, 3}, {15, 3} }, // SLOT 15 
      { {16, 3}, {17, 3} }, // SLOT 16 
      { {18, 3}, { 0, 0} }, // SLOT 17 
      { { 0, 0}, { 0, 0} }, // SLOT 18 
      { { 0, 0}, { 0, 0} }, // SLOT 19 
      { { 0, 0}, { 0, 0} }, // SLOT 20 
      { { 0, 0}, { 0, 0} }, // SLOT 21 
    },
    // SVT2
    {
      // HFCB0    HFCB1
      // Mod,Reg  Mod,Reg
      { { 0, 0}, { 0, 0} }, // SLOT  0 
      { { 0, 0}, { 0, 0} }, // SLOT  1 
      { { 0, 0}, { 0, 0} }, // SLOT  2 
      { { 4, 1}, { 5, 1} }, // SLOT  3 
      { { 6, 1}, { 7, 1} }, // SLOT  4 
      { { 8, 1}, { 0, 0} }, // SLOT  5 
      { { 0, 0}, { 0, 0} }, // SLOT  6 
      { { 5, 2}, { 6, 2} }, // SLOT  7 
      { { 7, 2}, { 8, 2} }, // SLOT  8 
      { { 9, 2}, {10, 2} }, // SLOT  9 
      { {11, 2}, { 0, 0} }, // SLOT 10 
      { { 0, 0}, { 0, 0} }, // SLOT 11 
      { { 0, 0}, { 0, 0} }, // SLOT 12 
      { { 6, 3}, { 7, 3} }, // SLOT 13 
      { { 8, 3}, { 9, 3} }, // SLOT 14 
      { {10, 3}, {11, 3} }, // SLOT 15 
      { {12, 3}, {13, 3} }, // SLOT 16 
      { {14, 3}, { 0, 0} }, // SLOT 17 
      { { 0, 0}, { 0, 0} }, // SLOT 18 
      { { 0, 0}, { 0, 0} }, // SLOT 19 
      { { 0, 0}, { 0, 0} }, // SLOT 20 
      { { 0, 0}, { 0, 0} }, // SLOT 21 
    }
  };

#define STRLEN 1024

/*read config file */

#define VAL_DECODER \
      for (ii=0; ii<nval; ii++) { \
        if (!strncmp(charval[ii],"0x",2)) \
          sscanf((char *)&charval[ii][2],"%8x",&val[ii]); \
        else \
          sscanf(charval[ii],"%u",&val[ii]); \
      } \
      nval = 0
int
vscmSlot(unsigned int id)
{
  if(id>=nvscm)
  {
    printf("%s: ERROR: Index (%d) >= VSCMs initialized (%d).\n",__FUNCTION__,id,nvscm);
    return(-1);
  }

  return(vscmID[id]);
}

int
vscmId(unsigned int slot)
{
  int id;

  for(id=0; id<nvscm; id++)
  {
    if(vscmID[id]==slot)
  {
      return(id);
  }
  }

  printf("%s: ERROR: VSCM in slot %d does not exist or not initialized.\n",__FUNCTION__,slot);
  return(-1);
}

int
vscmConfigDownload(int id, char *fname)
{
  FILE *fd;
  char *ch, str[STRLEN], keyword[STRLEN], charval[10][STRLEN];
  unsigned int val[10];
  int ii, nval;

  if (vscmIsNotInit(&id, __func__))
    return -1;

  if ((fd = fopen(fname,"r")) == NULL) {
    printf("[%d] VSCM_ConfigDownload: Can't open config file >%s<\n",id,fname);
    return -1;
  }

  nval = 0;
  while ((ch = fgets(str, STRLEN, fd)) != NULL)
  {
  /*printf(">%s< %d\n",str,strlen(ch));*/
    if (ch[0] == '#' || ch[0] == ' ' || ch[0] == '\t' || \
        ch[0] == '\n' || ch[0] == '\r') {
      continue;
    }
    else {
      sscanf(str,"%30s", keyword);
/*0        0        20*/
/*
      if (!strcmp(keyword,"VSCM_MAX_TRIGGER_NUM")) {
        sscanf(str,"%30s %9s", keyword, charval[0]);
        nval = 1;        
        VAL_DECODER;
        vscmSetMaxTriggerLimit(val[0]);
      }
*/
      if (!strcmp(keyword,"VSCM_CLOCK_EXTERNAL")) {
        vscmSetClockSource(id, 1);
      }

      else if (!strcmp(keyword,"VSCM_CLOCK_INTERNAL")) {
        vscmSetClockSource(id, 0);
      }

      else if (!strcmp(keyword,"FSSR_ADDR_REG_DISC_THR")) {
        sscanf(str,"%30s %1s %1s %3s", keyword, \
                charval[0], charval[1], charval[2]);
        nval = 3;        
        VAL_DECODER;
        fssrSetThreshold(id, (int)val[0],val[1],val[2]);
      }
/*0        0x00000000        0x00000000        0x00000000        0x00000000*/
      else if (!strcmp(keyword,"FSSR_ADDR_REG_KILL")) {
        sscanf(str,"%30s %1s %10s %10s %10s %10s", \
                keyword, charval[0], charval[1], charval[2], \
                charval[3], charval[4]);
        nval = 5;        
        VAL_DECODER;
        if (fssrSetMask(id, val[0], FSSR_ADDR_REG_KILL, (uint32_t *)&val[1]))
#ifdef DEBUG
          logMsg("ERROR: %s: %d/%u Mask Reg# %d not set correctly\n", \
                  __func__, id, val[0], FSSR_ADDR_REG_KILL); 
#else
        continue;
#endif
      }
/*0        0x00000000        0x00000000        0x00000000        0x00000000*/
      else if (!strcmp(keyword,"FSSR_ADDR_REG_INJECT")) {
        sscanf(str,"%30s %1s %10s %10s %10s %10s", keyword, \
                charval[0], charval[1], charval[2], charval[3], charval[4]);
        nval = 5;        
        VAL_DECODER;
        if (fssrSetMask(id, val[0], FSSR_ADDR_REG_INJECT, (uint32_t *)&val[1]))
#ifdef DEBUG
          logMsg("ERROR: %s: %d/%u Mask Reg# %d not set correctly\n", \
                  __func__, id, val[0], FSSR_ADDR_REG_INJECT); 
#else
        continue;
#endif
      }
/*0        0x1F*/
      else if (!strcmp(keyword,"FSSR_ADDR_REG_DCR")) {
        sscanf(str,"%30s %1s %4s", keyword, charval[0], charval[1]);
        nval = 2;        
        VAL_DECODER;
        fssrSetControl(id, val[0], val[1]);
      }
/*32*/
      else if(!strcmp(keyword,"VSCM_BCO_FREQ")) {
        sscanf(str,"%30s %3s", keyword, charval[0]);
        nval = 1;        
        VAL_DECODER;
        vscmSetBCOFreq(id, val[0]);
      }
/*256        512        32*/
      else if(!strcmp(keyword,"VSCM_TRIG_WINDOW")) {
        sscanf(str,"%30s %4s %4s %4s", \
                keyword, charval[0], charval[1], charval[2]);
        nval = 3;
        VAL_DECODER;
        vscmSetTriggerWindow(id, val[0],val[1],val[2]);
      }
      else
      {
        ; /* unknown key - do nothing */
    /*
        logMsg("ERROR: %s: unknown keyword >%s< (%u 0x%02x)\n", \
                __func__, keyword, ch[0], ch[0]);
        fclose(fd);
        return -3;
    */
      }
    }
  }

  fclose(fd);

  for (ii = 0; ii < 8; ii++) {
    fssrSetActiveLines(id, ii, FSSR_ALINES_6);
    fssrRejectHits(id, ii, 0);
    fssrSCR(id, ii);
    fssrSendData(id, ii, 1);
  }

  return 0;
}

/*******************/

void
fssrInternalPulserEnable(int id, int chip)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  fssrTransfer(id, chip, 2, FSSR_CMD_DEFAULT, 1, NULL);
}

void
fssrSetInternalPulserAmp(int id, int chip, uint8_t mask)
{
  uint32_t val = mask;

  if (vscmIsNotInit(&id, __func__))
    return;

  fssrTransfer(id, chip, 1, FSSR_CMD_WRITE, 8, &val);
}

uint8_t
fssrGetInternalPulserAmp(int id, int chip)
{
  uint32_t rsp;

  if (vscmIsNotInit(&id, __func__))
    return 0;

  fssrTransfer(id, chip, 1, FSSR_CMD_READ, 9, &rsp);
  return (rsp & 0xff);
}

void
fssrSetControl(int id, int chip, uint8_t mask)
{
  uint32_t val = mask;

  if (vscmIsNotInit(&id, __func__))
    return;

  fssrTransfer(id, chip, FSSR_ADDR_REG_DCR, FSSR_CMD_WRITE, 8, &val);
}

uint8_t
fssrGetControl(int id, int chip)
{
  uint32_t rsp;

  if (vscmIsNotInit(&id, __func__))
    return 0;

  fssrTransfer(id, chip, FSSR_ADDR_REG_DCR, FSSR_CMD_READ, 9, &rsp);
#if DEBUG
  printf("Control = 0x%02X\n", rsp & 0xFF);
#endif

  return (rsp & 0xFF);
}

int
fssrParseControl(int id, int chip, char *s)
{
  uint8_t val;
  char str[20];
  char numstr[4];

/*  if (strlen(s) < strlen(str))
 *    return -1;*/

  val = fssrGetControl(id, chip);

  switch (val & 3) {
    case 0:
      strcpy(str, "65, ");
      break;
    case 1:
      strcpy(str, "85, ");
      break;
    case 2:
      strcpy(str, "100, ");
      break;
    case 3:
      strcpy(str, "125, ");
      break;
  }

  switch ((val >> 2) & 1) {
    case 0:
      strcat(str, "High, ");
      break;
    case 1:
      strcat(str, "Low, ");
      break;
  }

  switch ((val >> 3) & 1) {
    case 0:
      strcat(str, "On, ");
      break;
    case 1:
      strcat(str, "Off, ");
      break;
  }

#ifdef VXWORKS
  /* VxWorks is oddly missing snprint() */
  sprintf(numstr, "%u", vmeRead32(&VSCMpr[id]->FssrCtrl.ClkCtrl) * 8);
#else
  snprintf(numstr, sizeof(numstr), "%u", \
            vmeRead32(&VSCMpr[id]->FssrCtrl.ClkCtrl) * 8);
#endif
  strcat(str, numstr);

  strcpy(s, str);
  return 0;
}

void
fssrSetThreshold(int id, int chip, int idx, uint8_t thr)
{
  uint32_t val;
  uint8_t reg;

  if (vscmIsNotInit(&id, __func__))
    return;

  if (idx > 7 || idx < 0)
    return;

  val = thr;
  reg = (FSSR_ADDR_REG_DISC_THR0 + idx);
  fssrTransfer(id, chip, reg, FSSR_CMD_WRITE, 8, &val);
}

uint8_t
fssrGetThreshold(int id, int chip, uint8_t idx)
{
  uint32_t rsp = 0;
  uint8_t reg;

  if (vscmIsNotInit(&id, __func__))
    return 0;

  if (idx > 7)
    return rsp;

  reg = (FSSR_ADDR_REG_DISC_THR0 + idx);
  fssrTransfer(id, chip, reg, FSSR_CMD_READ, 9, &rsp);
#if DEBUG
  logMsg("Threshold %u = %u\n", idx, rsp & 0xFF);
#endif

  return (rsp & 0xFF);
}

void
fssrSetVtn(int id, int chip, uint8_t thr)
{
  uint32_t val;

  if (vscmIsNotInit(&id, __func__))
    return;

  val = thr;
  fssrTransfer(id, chip, FSSR_ADDR_REG_DISC_VTN, FSSR_CMD_WRITE, 8, &val);
}

uint8_t
fssrGetVtn(int id, int chip)
{
  uint32_t rsp = 0;

  if (vscmIsNotInit(&id, __func__))
    return 0;

  fssrTransfer(id, chip, FSSR_ADDR_REG_DISC_VTN, FSSR_CMD_READ, 9, &rsp);
#if DEBUG
  logMsg("Vtn = %u\n", rsp & 0xFF);
#endif

  return (rsp & 0xFF);
}

int
fssrWaitReady(int id) {
  int i;
  for (i = 0; i < 10; i++) {
    if(vmeRead32(&VSCMpr[id]->FssrCtrl.Status) & (1<<14))
      return 1;
    //usleep(1000);
    taskDelay(1);
  }
#ifdef DEBUG
  logMsg("ERROR: %s: interface timeout\n", __func__);
#endif
  return 0;
}

void
fssrTransfer(int id, uint8_t chip, uint8_t reg, uint8_t cmd, \
              uint8_t nBits, uint32_t *pData)
{
  uint32_t SerCfgReg = 0;
  
  SerCfgReg |= (chip & 0xF) << 24;
  SerCfgReg |= (reg & 0x1F) << 0;
  SerCfgReg |= (cmd & 0x7) << 8;
  SerCfgReg |= (nBits & 0xFF) << 16;
  SerCfgReg |= (1 << 15);

  if (pData && nBits > 0)  vmeWrite32(&VSCMpr[id]->FssrCtrl.SerData[0], pData[0]);
  if (pData && nBits > 32) vmeWrite32(&VSCMpr[id]->FssrCtrl.SerData[1], pData[1]);
  if (pData && nBits > 64) vmeWrite32(&VSCMpr[id]->FssrCtrl.SerData[2], pData[2]);
  if (pData && nBits > 96) vmeWrite32(&VSCMpr[id]->FssrCtrl.SerData[3], pData[3]);

  if (!fssrWaitReady(id))
    logMsg("ERROR: %s(%d,%d,%d,%d,%d,...) not ready to start\n", __func__, id, chip, reg, cmd, nBits);
 
  vmeWrite32(&VSCMpr[id]->FssrCtrl.SerCtrl, SerCfgReg); 
  
  if (!fssrWaitReady(id))
    logMsg("ERROR: %s(%d,%d,%d,%d,%d,...) did not end\n", __func__, id, chip, reg, cmd, nBits);

  if (pData && (cmd == FSSR_CMD_READ)) {
    int i;
    uint32_t rsp[4];
    rsp[0] = vmeRead32(&VSCMpr[id]->FssrCtrl.SerData[0]);
    rsp[1] = vmeRead32(&VSCMpr[id]->FssrCtrl.SerData[1]);
    rsp[2] = vmeRead32(&VSCMpr[id]->FssrCtrl.SerData[2]);
    rsp[3] = vmeRead32(&VSCMpr[id]->FssrCtrl.SerData[3]);
    for (i = 0; i < nBits; i++) {
      if (i >= 96) {
        if (i == 96) pData[3] = 0;
        if (rsp[0] & (1 << (127 - i)))
          pData[3] |= 1 << (i - 96);
      }
      else if (i >= 64) {
        if (i == 64) pData[2] = 0;
        if (rsp[1] & (1 << (95 - i)))
          pData[2] |= 1 << (i - 64);
      }
      else if (i >= 32) {
        if (i == 32) pData[1] = 0;
        if (rsp[2] & (1 << (63 - i)))
          pData[1] |= 1 << (i - 32);
      }
      else {
        if (i == 0) pData[0] = 0;
        if (rsp[3] & (1 << (31 - i)))
          pData[0] |= 1 << i;
      }
    }
  }
  
#if DEBUG
  if (cmd == FSSR_CMD_READ) {
    logMsg("Data response: 0x%08X 0x%08X 0x%08X 0x%08X\n", \
            vmeRead32(&VSCMpr[id]->FssrCtrl.SerData[3]), \
            vmeRead32(&VSCMpr[id]->FssrCtrl.SerData[2]), \
            vmeRead32(&VSCMpr[id]->FssrCtrl.SerData[1]), \
            vmeRead32(&VSCMpr[id]->FssrCtrl.SerData[0]));
  }
#endif
}

void
fssrMasterReset(int id)
{
  vmeWrite32(&VSCMpr[id]->FssrCtrl.SerCtrl, (0xF << 28));
  taskDelay(1);
  vmeWrite32(&VSCMpr[id]->FssrCtrl.SerCtrl, 0);
  taskDelay(1);
}

int
fssrMaskCompare(uint32_t *mask, uint32_t *readmask)
{
  int i, status = 0;

  for (i = 0; i < 4; i++) {
    uint32_t v = readmask[i];
    /* Reverse Bit Sequence
    http://graphics.stanford.edu/~seander/bithacks.html#ReverseParallel*/
    v = ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
    v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
    v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
    v = ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
    v = ( v >> 16             ) | ( v               << 16);

    if (mask[3 - i] != v)
      status |= 1;
  }

  return status;
}

int
fssrSetMask(int id, int chip, int reg, uint32_t *mask)
{
  uint32_t readmask[4] = {0, 0, 0, 0};
  /* Per FSSR procedures disable/enable core
   * when doing a kill/inject operation*/
  fssrRejectHits(id, chip, 1);
  fssrTransfer(id, chip, reg, FSSR_CMD_WRITE, 128, mask);
  fssrRejectHits(id, chip, 0);

  fssrGetMask(id, chip, reg, readmask);
  if (fssrMaskCompare(mask, readmask))
    return 1;

  return 0;
}

/* Disable all channels on a chip */
void
fssrKillMaskDisableAll(int id, int chip)
{
  uint32_t mask[4];

  if (vscmIsNotInit(&id, __func__))
    return;

  mask[0] = 0xFFFFFFFF;
  mask[1] = 0xFFFFFFFF;
  mask[2] = 0xFFFFFFFF;
  mask[3] = 0xFFFFFFFF;
  if (fssrSetMask(id, chip, FSSR_ADDR_REG_KILL, mask))
    logMsg("ERROR: %s: Mask Reg# %d not set correctly\n", \
            __func__, FSSR_ADDR_REG_KILL);
}

/* Enable all channels on a chip */
void
fssrKillMaskEnableAll(int id, int chip)
{
  uint32_t mask[4] = {0, 0, 0, 0};

  if (vscmIsNotInit(&id, __func__))
    return;

  if (fssrSetMask(id, chip, FSSR_ADDR_REG_KILL, mask))
    logMsg("ERROR: %s: Mask Reg# %d not set correctly\n", \
            __func__, FSSR_ADDR_REG_KILL);
}

/* Toggle a single channel (disabled) on a chip */
void
fssrKillMaskDisableSingle(int id, int chip, int chan)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  fssrMaskSingle(id, chip, FSSR_ADDR_REG_KILL, chan, 1);
}

/* Toggle a single channel (enable) on a chip */
void
fssrKillMaskEnableSingle(int id, int chip, int chan)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  fssrMaskSingle(id, chip, FSSR_ADDR_REG_KILL, chan, 0);
}

/* Toggle a single channel inject mask on a chip */
void
fssrInjectMaskEnableSingle(int id, int chip, int chan)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  fssrMaskSingle(id, chip, FSSR_ADDR_REG_INJECT, chan, 1);
}

/*
 * Toggle a single mask channel on a chip
 * boolean = what to set the mask value to
 */
void
fssrMaskSingle(int id, int chip, int reg, int chan, int boolean)
{
  uint32_t mask[4], readmask[4];

  if (chan >=0 && chan <= 127) {
    int i;
    chan = 127 - chan;
    fssrGetMask(id, chip, reg, readmask);

    for (i = 0; i < 4; i++) {
      uint32_t v = readmask[i];
      /* Reverse Bit Sequence
      http://graphics.stanford.edu/~seander/bithacks.html#ReverseParallel*/
      v = ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
      v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
      v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
      v = ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
      v = ( v >> 16             ) | ( v               << 16);

      mask[3 - i] = v;
    }

    if (boolean == 1)
      mask[chan >> 5] |= (1 << (chan & 0x1F));
    else if (boolean == 0)
      mask[chan >> 5] &= ~(1 << (chan & 0x1F));

    if (fssrSetMask(id, chip, reg, mask))
      logMsg("ERROR: %s: Mask Reg# %d not set correctly\n", __func__, reg);
  }
  else
    logMsg("ERROR: %s: Reg %d bad channel #: %d\n", __func__, reg, chan);
}

/* Disable Inject mask on all channels on a chip */
void
fssrInjectMaskDisableAll(int id, int chip)
{
  uint32_t mask[4] = {0, 0, 0, 0};

  if (vscmIsNotInit(&id, __func__))
    return;

  if (fssrSetMask(id, chip, FSSR_ADDR_REG_INJECT, mask))
    logMsg("ERROR: %s: %d/%d Mask Reg# %d not set correctly\n", \
            __func__, id, chip, FSSR_ADDR_REG_INJECT);
}

void
fssrKillMaskDisableAllChips(int id)
{
  int i;

  if (vscmIsNotInit(&id, __func__))
    return;

  for (i = 0; i < 8; i++)
    fssrKillMaskDisableAll(id, i);
}

void
fssrInjectMaskDisableAllChips(int id)
{
  int i;

  if (vscmIsNotInit(&id, __func__))
    return;

  for (i = 0; i < 8; i++)
    fssrInjectMaskDisableAll(id, i);
}

void
fssrGetMask(int id, int chip, int reg, uint32_t *mask)
{
/*
  mask[0] = 0;
  mask[1] = 0;
  mask[2] = 0;
  mask[3] = 0;
*/

  fssrTransfer(id, chip, reg, FSSR_CMD_READ, 129, mask);
}

void
fssrGetKillMask(int id, int chip, uint32_t *mask)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  fssrGetMask(id, chip, FSSR_ADDR_REG_KILL, mask);

#if DEBUG
  logMsg("Kill [ch127->0] = 0x%08X 0x%08X 0x%08X 0x%08X\n", \
          mask[3], mask[2], mask[1], mask[0]);
#endif
}

void
fssrGetInjectMask(int id, int chip, uint32_t *mask)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  fssrGetMask(id, chip, FSSR_ADDR_REG_INJECT, mask);

#if DEBUG
  logMsg("Inject [ch127->0] = 0x%08X 0x%08X 0x%08X 0x%08X\n", \
          mask[3], mask[2], mask[1], mask[0]);
#endif
}

uint8_t
fssrGetBCONum(int id, int chip)
{
  return fssrGetBCONumOffset(id, chip, FSSR_SCR_BCONUM_START);
}

uint8_t
fssrGetBCONumOffset(int id, int chip, uint8_t offset) {
  uint32_t rsp;
  
  vmeWrite32(&VSCMpr[id]->FssrCtrl.SerClk, 0x100 | ((offset + 1) & 0xFF));
  fssrTransfer(id, chip, FSSR_ADDR_REG_AQBCO, FSSR_CMD_SET, 1, NULL);
  vmeWrite32(&VSCMpr[id]->FssrCtrl.SerClk, 0);
  fssrTransfer(id, chip, FSSR_ADDR_REG_AQBCO, FSSR_CMD_READ, 9, &rsp);

#if DEBUG
  logMsg("BCO [sync @ %u] = %u\n", offset, rsp & 0xFF);
#endif

  return (rsp & 0xFF);
}

uint8_t
fssrGetBCONumNoSync(int id, int chip)
{
  uint32_t rsp;

  fssrTransfer(id, chip, FSSR_ADDR_REG_AQBCO, FSSR_CMD_SET, 1, NULL);
  fssrTransfer(id, chip, FSSR_ADDR_REG_AQBCO, FSSR_CMD_READ, 9, &rsp);
  
#if DEBUG
  logMsg("BCO [no sync] = %u\n", rsp & 0xFF);
#endif

  return (rsp & 0xFF);
}

void
fssrRejectHits(int id, int chip, int reject)
{
  if(reject)
    fssrTransfer(id, chip, FSSR_ADDR_REG_REJECTHITS, FSSR_CMD_SET, 1, NULL);
  else
    fssrTransfer(id, chip, FSSR_ADDR_REG_REJECTHITS, FSSR_CMD_RESET, 1, NULL);
}

void
fssrSendData(int id, int chip, int send)
{
  if (send)
    fssrTransfer(id, chip, FSSR_ADDR_REG_SENDDATA, FSSR_CMD_SET, 1, NULL);
  else
    fssrTransfer(id, chip, FSSR_ADDR_REG_SENDDATA, FSSR_CMD_RESET, 1, NULL);
}

void
fssrSetActiveLines_Asic(int id, int chip, unsigned int lines)
{
  uint32_t val = (lines & 0x3);
  int mode, i;

  if (vscmIsNotInit(&id, __func__))
    return;

  fssrTransfer(id, chip, FSSR_ADDR_REG_ALINES, FSSR_CMD_WRITE, 2, &val);
}

void
fssrSetActiveLines_Fpga(int id, int chip, unsigned int lines)
{
  uint32_t val = (lines & 0x3);
  int mode;

  if (vscmIsNotInit(&id, __func__))
    return;

  switch (lines) {
  case FSSR_ALINES_4:
    mode = 1;
    break;
  case FSSR_ALINES_2:
    mode = 2;
    break;
  case FSSR_ALINES_1:
    mode = 3;
    break;
  default:
    mode = 0;
    break;
  }

  vmeWrite32(&VSCMpr[id]->Fssr[chip].Ctrl, mode | 0x10000);
  vmeWrite32(&VSCMpr[id]->Fssr[chip].Ctrl, mode);
}

void
fssrSetActiveLines(int id, int chip, unsigned int lines)
{
  uint32_t val = (lines & 0x3);
  int mode;

  if (vscmIsNotInit(&id, __func__))
    return;

  fssrTransfer(id, chip, FSSR_ADDR_REG_ALINES, FSSR_CMD_WRITE, 2, &val);

  switch (lines) {
  case FSSR_ALINES_4:
    mode = 1;
    break;
  case FSSR_ALINES_2:
    mode = 2;
    break;
  case FSSR_ALINES_1:
    mode = 3;
    break;
  default:
    mode = 0;
    break;
  }

  vmeWrite32(&VSCMpr[id]->Fssr[chip].Ctrl, mode | 0x10000);
  vmeWrite32(&VSCMpr[id]->Fssr[chip].Ctrl, mode);

  // Allow time for state machines to reset
  taskDelay(1);
}

/*
 * Set the Chip ID for the chips based on first number passed
 *
 * For each ID 8 (0b01XXX) is added to the passed chip ID
 * This is due to the fact that the chip ID is really 5 bits, 
 * but only 3 are user settable via wire bonds
 * 0 = set both connectors to use the same chip IDs
 * 1 = only set for the top connector
 * 2 = only set for the bottom connector
 */
void
fssrSetChipID(int id, \
              unsigned int hfcb, \
              unsigned int u1, \
              unsigned int u2, \
              unsigned int u3, \
              unsigned int u4)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  if (hfcb > 2) {
    logMsg("ERROR: %s: Invalid HFCB #\n", __func__);
    return;
  }
  if (u1 > 7 || u2 > 7 || u3 > 7 || u4 > 7) {
    logMsg("ERROR: %s: Invalid Chip ID\n", __func__);
    return;
  }

  if (hfcb == 0 || hfcb == 1) {
    vmeWrite32(&VSCMpr[id]->FssrCtrl.AddrH1, \
                ((8 + u4) << 24) | ((8 + u3) << 16) | \
                ((8 + u2) << 8) | ((8 + u1) << 0));
  }
  if (hfcb == 0 || hfcb == 2) {
    vmeWrite32(&VSCMpr[id]->FssrCtrl.AddrH2, \
                ((8 + u4) << 24) | ((8 + u3) << 16) | \
                ((8 + u2) << 8) | ((8 + u1) << 0));
  }
}


uint32_t
fssrGetChipID(int id, int chip)
{
  uint32_t ret;

  /* Only use the lower 3 bits i.e. Chip ID wire bonds */
  if (chip >=0 && chip <= 3)
  {
    ret = vmeRead32(&VSCMpr[id]->FssrCtrl.AddrH1) >> (chip * 8) & 7;
  } 
  else if (chip >= 4 && chip <= 7)
  {
    ret = vmeRead32(&VSCMpr[id]->FssrCtrl.AddrH2) >> ((chip - 4) * 8) & 7;
  }
  else
  {
    logMsg("ERROR: %s: chip must be in range 0-7\n", __func__);
  }

  return(ret);
}

void
fssrSCR(int id, int chip)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  vmeWrite32(&VSCMpr[id]->FssrCtrl.SerClk, 0x100 | FSSR_SCR_BCONUM_START);
  fssrTransfer(id, chip, FSSR_ADDR_REG_SCR, FSSR_CMD_SET, 1, NULL);
  vmeWrite32(&VSCMpr[id]->FssrCtrl.SerClk, 0);
}

char *
readNormalizedScaler(char *buf, char *prefix, \
                          uint32_t ref, uint32_t scaler)
{
  double normalized = VSCM_SYS_CLK * (double)scaler / (double)ref;
#ifdef VXWORKS
  /* VxWorks is oddly missing snprint() */
  sprintf(buf, "%s = %08u, %.1fHz\n", prefix, scaler, normalized);
#else
  snprintf(buf, 80, "%s = %08u, %.1fHz\n", \
            prefix, scaler, normalized);
#endif
  return buf;
}

void
fssrStatusAll()
{
  int i, j;
  for (i = 0; i < nvscm; i++) {
    vscmDisableScaler(vscmID[i]);

    for (j = 0; j < 8; j++)
      fssrStatus(vscmID[i], j);

    vscmEnableScaler(vscmID[i]);
  }
}

void 
fssrStatus(int id, int chip)
{
  uint32_t ref;
  uint32_t mask[4];
  char buf[80];

  if (vscmIsNotInit(&id, __func__))
    return;

  if (chip <0 || chip > 7) {
    logMsg("ERROR: %s: Chip must be in range 0-7\n", __func__);
    return;
  }

  ref = vmeRead32(&VSCMpr[id]->Sd.Scalers[VSCM_SCALER_GCLK125]);

  printf("SLOT: %d ", id);
  
  switch (chip) {
    case 0: printf("HFCB 1 U1:\n"); break;
    case 1: printf("HFCB 1 U2:\n"); break;
    case 2: printf("HFCB 1 U3:\n"); break;
    case 3: printf("HFCB 1 U4:\n"); break;
    case 4: printf("HFCB 2 U1:\n"); break;
    case 5: printf("HFCB 2 U2:\n"); break;
    case 6: printf("HFCB 2 U3:\n"); break;
    case 7: printf("HFCB 2 U4:\n"); break;
  }

  printf("----------- Status ------------\n");
  printf("Last Status Word   = 0x%08X\n", \
          vmeRead32(&VSCMpr[id]->Fssr[chip].LastStatusWord));
  printf(readNormalizedScaler(buf, "StatusWordCount   ", ref, \
          vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerStatusWord)));
  printf(readNormalizedScaler(buf, "EventWordCount    ", ref, \
          vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerEvent)));
  printf(readNormalizedScaler(buf, "TotalWordCount    ", ref, \
          vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerWords)));
  printf(readNormalizedScaler(buf, "IdleWordCount     ", ref, \
          vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerIdle)));
  printf(readNormalizedScaler(buf, "AcqBcoCount       ", ref, \
          vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerAqBco)));
  printf(readNormalizedScaler(buf, "MarkErrors        ", ref, \
          vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerMarkErr)));
  printf(readNormalizedScaler(buf, "StripEncodeErrors ", ref, \
          vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerEncErr)));
  printf(readNormalizedScaler(buf, "ChipIdErrors      ", ref, \
          vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerChipIdErr)));
  printf(readNormalizedScaler(buf, "GotHit            ", ref, \
          vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerGotHit)));
  printf(readNormalizedScaler(buf, "Coretalking       ", ref, \
          vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerCoreTalking)));
/*  printf("FSSR Max Latency   = %u (BCO ticks)\n", \
            vmeRead32(&VSCMpr[id]->Fssr[chip].LatencyMax));
*/
  printf("----------- Config ------------\n");  
  printf("FSSR BCO Clock Period: %uns\n", \
          vmeRead32(&VSCMpr[id]->FssrCtrl.ClkCtrl) * 8);
  printf("FSSR Control: 0x%02X\n", fssrGetControl(id, chip));
  printf("FSSR Thresholds: %u %u %u %u %u %u %u %u\n", \
          fssrGetThreshold(id, chip, 0), fssrGetThreshold(id, chip, 1), \
          fssrGetThreshold(id, chip, 2), fssrGetThreshold(id, chip, 3), \
          fssrGetThreshold(id, chip, 4), fssrGetThreshold(id, chip, 5), \
          fssrGetThreshold(id, chip, 6), fssrGetThreshold(id, chip, 7));
    
  fssrGetKillMask(id, chip, mask);
  printf("FSSR Kill[ch127->0]: 0x%08X 0x%08X 0x%08X 0x%08X\n", mask[3], mask[2], mask[1], mask[0]);
  
  fssrGetInjectMask(id, chip, mask);
  printf("FSSR Inject[ch127->0]: 0x%08X 0x%08X 0x%08X 0x%08X\n", mask[3], mask[2], mask[1], mask[0]);

  printf("FSSR BCO[@0 @128, @255]: %u %u %u\n", \
          fssrGetBCONumOffset(id, chip, FSSR_SCR_BCONUM_START), \
          fssrGetBCONumOffset(id, chip, FSSR_SCR_BCONUM_START-128), \
          fssrGetBCONumOffset(id, chip, FSSR_SCR_BCONUM_START-1));
  printf("\n"); 
}


/* following functions reads individual FSSR parameters */

uint32_t
fssrReadLastStatusWord(int id, int chip)
{
  if (vscmIsNotInit(&id, __func__)) return(-1);
  return(vmeRead32(&VSCMpr[id]->Fssr[chip].LastStatusWord));
}

uint32_t
fssrReadScalerStatusWord(int id, int chip)
{
  if (vscmIsNotInit(&id, __func__)) return(-1);
  return(vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerStatusWord));
}

uint32_t
fssrReadScalerEvent(int id, int chip)
{
  if (vscmIsNotInit(&id, __func__)) return(-1);
  return(vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerEvent));
}

uint32_t
fssrReadScalerWords(int id, int chip)
{
  if (vscmIsNotInit(&id, __func__)) return(-1);
  return(vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerWords));
}

uint32_t
fssrReadScalerIdle(int id, int chip)
{
  if (vscmIsNotInit(&id, __func__)) return(-1);
  return(vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerIdle));
}

uint32_t
fssrReadScalerAqBco(int id, int chip)
{
  if (vscmIsNotInit(&id, __func__)) return(-1);
  return(vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerAqBco));
}

uint32_t
fssrReadScalerMarkErr(int id, int chip)
{
  if (vscmIsNotInit(&id, __func__)) return(-1);
  return(vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerMarkErr));
}

uint32_t
fssrReadScalerEncErr(int id, int chip)
{
  if (vscmIsNotInit(&id, __func__)) return(-1);
  return(vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerEncErr));
}

uint32_t
fssrReadScalerChipIdErr(int id, int chip)
{
  if (vscmIsNotInit(&id, __func__)) return(-1);
  return(vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerChipIdErr));
}

uint32_t
fssrReadScalerGotHit(int id, int chip)
{
  if (vscmIsNotInit(&id, __func__)) return(-1);
  return(vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerGotHit));
}

uint32_t
fssrReadScalerStrip(int id, int chip)
{
  if (vscmIsNotInit(&id, __func__)) return(-1);
  return(vmeRead32(&VSCMpr[id]->Fssr[chip].HistCnt));
}

uint32_t
fssrReadScalerCoreTalking(int id, int chip)
{
  if (vscmIsNotInit(&id, __func__)) return(-1);
  return(vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerCoreTalking));
}

uint32_t
fssrReadLastDataWord(int id, int chip)
{
  if (vscmIsNotInit(&id, __func__)) return(-1);
  return(vmeRead32(&VSCMpr[id]->Fssr[chip].LastDataWord));
}

uint32_t
fssrReadScalerRef(int id)
{
  if (vscmIsNotInit(&id, __func__)) return(-1);
  return(vmeRead32(&VSCMpr[id]->Sd.Scalers[VSCM_SCALER_GCLK125]));
}

/******************/
/* VSCM functions */
/******************/

void
vscmSWSync(int id)
{
  unsigned int reg = vmeRead32(&VSCMpr[id]->Sd.SyncCtrl);

  vmeWrite32(&VSCMpr[id]->Sd.SyncCtrl, (reg & 0xFFFFFFE0) | IO_MUX_0); 
  vmeWrite32(&VSCMpr[id]->Sd.SyncCtrl, (reg & 0xFFFFFFE0) | IO_MUX_1); 
  vmeWrite32(&VSCMpr[id]->Sd.SyncCtrl, reg); 
}

void
vscmSetTriggerWindow(int id, \
                      uint32_t windowSize, \
                      uint32_t windowLookback, \
                      uint32_t bcoFreq)
{
  uint32_t pulser_period;

  if (vscmIsNotInit(&id, __func__))
    return;

  vmeWrite32(&VSCMpr[id]->Eb.Lookback, windowLookback*8);
  vmeWrite32(&VSCMpr[id]->Eb.WindowWidth, windowSize*8);

#if 0
  /* Check the maximum pulser rate only if its already set */
  if ((pulser_period = vmeRead32(&VSCMpr[id]->PulserPeriod))) {
    /* Formula from: https://clasweb.jlab.org/elog-svt/daq/5 */
    uint32_t trig_rate_limit = 50000000 / (16 * (1 + windowSize / bcoFreq));
    /* Convert pulser period to frequency in Hz before comparing to limit */
    if ((int)(1.0 / (pulser_period * 8.0e-9)) > trig_rate_limit) {
      /* Add 0.5 to naively force rounding up */
      vmeWrite32(&VSCMpr[id]->PulserPeriod, \
                ((1.0 / trig_rate_limit) / 8.0e-9) + 0.5);
      logMsg("INFO: %s: Raised Pulser Period from %u ns to %u ns\n", \
              __func__, pulser_period, vmeRead32(&VSCMpr[id]->PulserPeriod));
    }
  }
#endif
}

int
vscmGetTriggerWindowWidth(int id)
{
  int width;
  
  if (vscmIsNotInit(&id, __func__))
    return;

  width = vmeRead32(&VSCMpr[id]->Eb.WindowWidth) / 8;

  return width;
}

int
vscmGetTriggerWindowOffset(int id)
{
  int offset;
  
  if (vscmIsNotInit(&id, __func__))
    return;

  offset = vmeRead32(&VSCMpr[id]->Eb.Lookback) / 8;
  
  return offset;
}

void
vscmPrintFifo(unsigned int *buf, int n)
{
  int i;
  unsigned int word;
  for(i = 0; i < n; i++)
  {
#ifdef VXWORKS
    word = buf[i];
#else
    word = LSWAP(buf[i]);
#endif
    printf("0x%08X", word);
      
    if(word & 0x80000000)
    {
      int type = (word>>27)&0xF;
      switch(type)
      {
        case DATA_TYPE_BLKHDR:
          printf(" {BLKHDR} SLOTID: %d", (word>>22)&0x1f);
    printf(" NEVENTS: %d", (word>>11)&0x7ff);
    printf(" BLOCK: %d\n", (word>>0)&0x7ff);
    break;
        case DATA_TYPE_BLKTLR:
          printf(" {BLKTLR} SLOTID: %d", (word>>22)&0x1f);
    printf(" NWORDS: %d\n", (word>>0)&0x3fffff);
    break;
        case DATA_TYPE_EVTHDR:
    printf(" {EVTHDR} EVENT: %d\n", (word>>0)&0x7ffffff);
    break;
        case DATA_TYPE_TRGTIME:
    printf(" {TRGTIME}\n");
    break;
        case DATA_TYPE_BCOTIME:
    printf(" {BCOTIME}\n");
    break;
        case DATA_TYPE_FSSREVT:
    printf(" {FSSREVT}");
    printf(" HFCBID: %1u", (word>>20)&0x1);
    printf(" CHIPID: %1u", (word>>19)&0x7);
    printf(" CH: %3u", (word>>12)&0x7F);
    printf(" BCO: %3u", (word>>4)&0xFF);
    printf(" ADC: %1u\n", (word>>0)&0x7);
    break;
        case DATA_TYPE_DNV:
    printf(" {***DNV***}\n");
          return;
    break;
        case DATA_TYPE_FILLER:
    printf(" {FILLER}\n");
    break;
        default:
    printf(" {***DATATYPE ERROR***}\n");
          return;
    break;
      }
    }
    else
    {
      printf("\n");
    }
  }
  return;
}

/*******************************************************************************
 *
 * vscmReadBlock - General Data readout routine
 *
 *    id    - VSCM to read from
 *    data  - local memory address to place data
 *    nwrds - Max number of words to transfer
 *    rflag - Readout Flag
 *              0 - programmed I/O from the specified board
 *              1 - DMA transfer using Universe/Tempe DMA Engine 
 *                    (DMA VME transfer Mode must be setup prior)
 *              2 - Multiblock DMA transfer (Multiblock must be enabled
 *                     and daisychain in place or SD being used)
 *
 * RETURNS: Number of words transferred to data if successful, ERROR otherwise
 *
 */
int
vscmReadBlock(int id, volatile uintptr_t *data, int nwrds, int rflag)
{
  int retVal;
  volatile uintptr_t *laddr;
#ifndef CODA3DMA
  unsigned int vmeAdr;
#endif

  if (vscmIsNotInit(&id, __func__))
    return -1;

  if (VSCMpf[id] == NULL) {
    logMsg("ERROR: %s: VSCM A32 not initialized\n", __func__);
    return -1;
  }

  if (data == NULL) {
    logMsg("ERROR: %s: Invalid Destination address\n", __func__);
    return -1;
  }

  VSCMLOCK;

  /* Block transfer */
  if (rflag >= 1) {
    /* Assume that the DMA programming is already setup. 
    Don't Bother checking if there is valid data - that should be done prior
    to calling the read routine */

    laddr = data;

#ifdef CODA3DMA
#ifdef VXWORKS
    retVal = sysVmeDmaSend(laddr, vscmA32Base, (nwrds << 2), 0);
#else
    retVal = vmeDmaSend((unsigned long)laddr, vscmA32Base, (nwrds << 2));
#endif
#else
    vmeAdr = (unsigned int)(VSCMpf[id]) - vscmA32Offset;
    retVal = usrVme2MemDmaStart(vmeAdr, (unsigned int *)laddr, (nwrds << 2));
#endif
    if (retVal |= 0) {
      logMsg("ERROR: %s: DMA transfer Init @ 0x%x Failed\n", __func__, retVal);
      VSCMUNLOCK;
      return retVal;
    }

    /* Wait until Done or Error */
#ifdef CODA3DMA
#ifdef VXWORKS
    retVal = sysVmeDmaDone(10000,1);
#else
    retVal = vmeDmaDone();
#endif
#else
    retVal = usrVme2MemDmaDone();
#endif

    if (retVal > 0) {
#ifdef CODA3DMA
#ifdef VXWORKS
      int xferCount = (nwrds - (retVal >> 2));
#else
      int xferCount = (retVal >> 2);
#endif
#else
      int xferCount = (retVal >> 2);
#endif
      VSCMUNLOCK;
      return xferCount;
    }
    else if (retVal == 0) {
      logMsg("WARN: %s: DMA transfer terminated by word count 0x%x\n", \
              __func__, nwrds);
      VSCMUNLOCK;
      return 0;
    }
    /* Error in DMA */
    else {
      logMsg("ERROR: %s: DmaDone returned an Error\n", __func__);
      VSCMUNLOCK;
      return 0;
    }
  }
  /* Programmed IO */
  else {
    int dCnt = 0;
    int ii = 0;

    while (ii < nwrds) {
      uint32_t val = *VSCMpf[id];
#ifndef VXWORKS
      val = LSWAP(val);
#endif
/*
      if (val == TI_EMPTY_FIFO)
        break;
#ifndef VXWORKS
      val = LSWAP(val);
#endif
*/
      data[ii] = val;
      ii++;
    }
    ii++;
    dCnt += ii;

    VSCMUNLOCK;
    return dCnt;
  }

  VSCMUNLOCK;
  return 0;
}

void
vscmDisableScaler(int id)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  vmeWrite32(&VSCMpr[id]->Sd.ScalerLatch, 1);
}

void
vscmEnableScaler(int id)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  vmeWrite32(&VSCMpr[id]->Sd.ScalerLatch, 0);
}

void
vscmPrintScalers(int id)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  vscmDisableScaler(id);
  printf("FP_OUTPUT0 = %d\n", vmeRead32(&VSCMpr[id]->Sd.Scalers[VSCM_SCALER_FP_OUTPUT(0)]));
  printf("FP_OUTPUT1 = %d\n", vmeRead32(&VSCMpr[id]->Sd.Scalers[VSCM_SCALER_FP_OUTPUT(1)]));
  printf("FP_OUTPUT2 = %d\n", vmeRead32(&VSCMpr[id]->Sd.Scalers[VSCM_SCALER_FP_OUTPUT(2)]));
  printf("FP_OUTPUT3 = %d\n", vmeRead32(&VSCMpr[id]->Sd.Scalers[VSCM_SCALER_FP_OUTPUT(3)]));
  vscmEnableScaler(id);
}

void
vcsmSetFPOutputSrc(int id, int port, int src)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  vmeWrite32(&VSCMpr[id]->Sd.FpOutputCtrl[port], src);
}

void
vscmSetClockSource(int id, int clock_int_ext)
{
  if (vscmIsNotInit(&id, __func__))
    return;
 

  if(clock_int_ext)
  {
    printf("%s(%d,%d) - Set External\n", __func__, id, clock_int_ext);
    /* External Clock */
    vmeWrite32(&VSCMpr[id]->Clk.Ctrl, 0xC0000000); /* sets clock */
    vmeWrite32(&VSCMpr[id]->Clk.Ctrl, 0x40000000); /* release reset */
  }
  else
  {
    printf("%s(%d,%d) - Set Internal\n", __func__, id, clock_int_ext);
    /* Internal Clock */
    vmeWrite32(&VSCMpr[id]->Clk.Ctrl, 0x80000000); /* sets clock */
    vmeWrite32(&VSCMpr[id]->Clk.Ctrl, 0x00000000); /* release reset */
  }
 
  vscmFifoClear(id);
  fssrMasterReset(id);
  taskDelay(10);
}

int
vscmGetClockSource(int id)
{
  if (vscmIsNotInit(&id, __func__))
    return;
 
  return (vmeRead32(&VSCMpr[id]->Clk.Ctrl)>>30) & 0x1; 
}

void
vscmClearStripScalers(int id, int chip)
{
  int i;

  if (vscmIsNotInit(&id, __func__))
    return;

  for (i = 0; i < 128; i++)
    vmeRead32(&VSCMpr[id]->Fssr[chip].HistCnt);
}

int
vscmReadStripScalers(int id, int chip, uint32_t *arr)
{
  int i;

  if (vscmIsNotInit(&id, __func__))
    return -1;

  for (i = 0; i < 128; i++)
    arr[i] = vmeRead32(&VSCMpr[id]->Fssr[chip].HistCnt);

  return 0;
}

/*******************************************************************
 *   Function : vscmReadScalers
 *                      
 *   Function : Reads & latches values from scalers.
 *                                                    
 *   Parameters :  UINT32 id    - Module slot number
 *                 UINT32 *data - local memory address to place data
 *                 UINT32 nwrds - Max number of words to transfer
 *                 UINT32 rflag - Readout flag
 *                                 bit 0 - FSSR Chip 0 scalers
 *                                 bit 1 - FSSR Chip 1 scalers
 *                                 bit 2 - FSSR Chip 2 scalers
 *                                 bit 3 - FSSR Chip 3 scalers
 *                                 bit 4 - FSSR Chip 4 scalers
 *                                 bit 5 - FSSR Chip 5 scalers
 *                                 bit 6 - FSSR Chip 6 scalers
 *                                 bit 7 - FSSR Chip 7 scalers
 *                 UINT32 rmode - Readout mode
 *                                 0 - programmed I/O
 *
 *   * 32bit words in *data will be written as they are received from
 *     the OS, with no attention paid to "endian-ness".  
 *     E.g. in vxWorks (PPC) - big endian
 *          in Linux (Intel) - little endian
 * 
 *   Data format:
 *      First word contains readout flag
 * 
 *      Followed by N scaler chunks [135 words each], where N is the number of bits set in the rflag
 *        0:     Reference (scaler integration time in 8ns ticks for strips)
 *        1-128: FSSR strips
 *        129:   Reference (scaler integration time in 8ns ticks for others)
 *        130:   Mark Error
 *        131:   Encoding Error
 *        132:   Chip ID Error
 *        133:   Got Hit
 *        134:   Core Talking
 *                                                    
 *   Returns -1 if Error, Number of words transferred if OK.
 *                                                    
 *******************************************************************/
int
vscmReadScalers(int id, volatile unsigned int *data, int nwrds, int rflag, int rmode)
{
  int chip, i, dCnt = 0;
  unsigned int val, ref;

  if (vscmIsNotInit(&id, __func__))
    return(0);

  if(!data) 
  {
    logMsg("%s: ERROR: Invalid Destination address\n",__FUNCTION__,0,0,0,0,0);
    return(0);
  }

  if(rmode==0)
  { /* Programmed I/O */

    /* Hold scalers */
    vmeWrite32(&VSCMpr[id]->Sd.ScalerLatch, 1);

    if(dCnt < nwrds) data[dCnt++] = rflag;

    ref = vmeRead32(&VSCMpr[id]->Sd.Scalers[VSCM_SCALER_GCLK125]);
                                              
    for(chip = 0; chip < 8; chip++)
    {
      if(!(rflag & (1<<chip))) continue;

      if(dCnt < nwrds) data[dCnt++] = ref;
      for(i = 0; i < 128; i++)
      {
        val = vmeRead32(&VSCMpr[id]->Fssr[chip].HistCnt);
        /*printf("id=%2d chip=%3d chan=%3d count=%7d\n",id,chip,i,val);*/
        if(dCnt < nwrds) data[dCnt++] = val;
      }
      
      if(dCnt < nwrds) data[dCnt++] = ref;
      if(dCnt < nwrds) data[dCnt++] = vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerMarkErr);
      if(dCnt < nwrds) data[dCnt++] = vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerEncErr);
      if(dCnt < nwrds) data[dCnt++] = vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerChipIdErr);
      if(dCnt < nwrds) data[dCnt++] = vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerGotHit);
      if(dCnt < nwrds) data[dCnt++] = vmeRead32(&VSCMpr[id]->Fssr[chip].ScalerCoreTalking);
    }

    /* Release/reset scalers */
    vmeWrite32(&VSCMpr[id]->Sd.ScalerLatch, 0);

    return(dCnt);
  }
  else
  {
    logMsg("%s: ERROR: Unsupported mode (%d)\n",__FUNCTION__,rmode,3,4,5,6);
    return(0);
  }

}

int
vscmPrintStripScalers(int id, int chip)
{
  uint32_t arr[128];
  int i;

  if (vscmIsNotInit(&id, __func__))
    return -1;

  vscmDisableScaler(id);
  for (i = 0; i < 128; i++)
  {
    arr[i] = vmeRead32(&VSCMpr[id]->Fssr[chip].HistCnt);
    printf("id=%2d chip=%3d chan=%3d count=%7d\n",id,chip,i,arr[i]);
  }
  vscmEnableScaler(id);

  return 0;
}

uint32_t
vscmReadVmeClk(int id)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  return(vmeRead32(&VSCMpr[id]->Sd.Scalers[VSCM_SCALER_SYSCLK50]));
}

void
vscmResetToken(int id)
{
  uint32_t val;
  if (vscmIsNotInit(&id, __func__))
    return;
  
  VSCMLOCK;
  val = vmeRead32(&VSCMpr[id]->Eb.Adr32M) | (1<<28);
  vmeWrite32(&VSCMpr[id]->Eb.Adr32M, val);
  VSCMUNLOCK;
}

void
vscmFifoClear(int id)
{
  uint32_t val;
  if (vscmIsNotInit(&id, __func__))
    return;

  val = vmeRead32(&VSCMpr[id]->Clk.Ctrl);

  /* set soft reset */
  val |= 0x20000000;
  vmeWrite32(&VSCMpr[id]->Clk.Ctrl, val);

  /* release soft reset */
  val &= ~0x20000000;
  vmeWrite32(&VSCMpr[id]->Clk.Ctrl, val);
}

void
vscmGStat(int id)
{
  int i;

  for (i = 0; i < nvscm; i++)
    vscmStat(vscmID[i]);
}

void
vscmStat(int id)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  logMsg("VME slot: %u\n",         (vmeRead32(&VSCMpr[id]->Cfg.FirmwareRev)>>24) & 0x1f);
  logMsg("FIFO Word Count: %u\n",   vmeRead32(&VSCMpr[id]->Eb.FifoWordCnt));
  logMsg("FIFO Event Count: %u\n",  vmeRead32(&VSCMpr[id]->Eb.FifoEventCnt));
  logMsg("FIFO Block Count: %u\n",  vmeRead32(&VSCMpr[id]->Eb.FifoBlockCnt));
  logMsg("Input Triggers: %u\n",    vscmGetInputTriggers(id));
  logMsg("Accepted Triggers: %u\n", vscmGetAcceptedTriggers(id));
  logMsg("Sync Scaler: %u\n", vmeRead32(&VSCMpr[id]->Sd.Scalers[VSCM_SCALER_SYNC]));
  logMsg("Trig1 Scaler: %u\n", vmeRead32(&VSCMpr[id]->Sd.Scalers[VSCM_SCALER_TRIG1]));
  logMsg("Sync Cfg: 0x%08X\n", vmeRead32(&VSCMpr[id]->Sd.SyncCtrl));
  logMsg("Trig1 Cfg: 0x%08X\n", vmeRead32(&VSCMpr[id]->Sd.TrigCtrl));
  logMsg("Clock Cfg: 0x%08X\n", vmeRead32(&VSCMpr[id]->Clk.Ctrl));
}

uint32_t
vscmFirmwareRev(int id)
{
  if (vscmIsNotInit(&id, __func__))
    return -1;

  return vmeRead32(&VSCMpr[id]->Cfg.FirmwareRev) & 0xFFFF;
}

uint32_t
vscmGetInputTriggers(int id)
{
  if (vscmIsNotInit(&id, __func__))
    return 0;

/*  return vmeRead32(&VSCMpr[id]->ScalerTrigger); */
  return 0; /* not implemented */
}

uint32_t
vscmGetAcceptedTriggers(int id)
{
  if (vscmIsNotInit(&id, __func__))
    return 0;

/*  return vmeRead32(&VSCMpr[id]->ScalerTriggerAccepted); */
  return 0; /* not implemented */
}

/* Return the number of Events in FIFO */
uint32_t
vscmDReady(int id)
{
  uint32_t rval;

  if (vscmIsNotInit(&id, __func__))
    return -1;

  VSCMLOCK;
  rval = vmeRead32(&VSCMpr[id]->Eb.FifoEventCnt);
  VSCMUNLOCK;

  return rval;
}

int
vscmBReady(int id)
{
  uint32_t rval;

  if (vscmIsNotInit(&id, __func__))
    return -1;

  VSCMLOCK;
  rval = vmeRead32(&VSCMpr[id]->Eb.FifoBlockCnt);
  VSCMUNLOCK;

  return (rval > 0) ? 1 : 0;
}

uint32_t
vscmGBReady()
{
  uint32_t mask = 0;
  int i, stat;

  /*VSCMLOCK;*/
  for (i = 0; i < nvscm; i++) {
    stat = vscmBReady(vscmID[i]);
    if (stat)
      mask |= (1 << vscmID[i]);
  }
  /*VSCMUNLOCK;*/

  return mask;
}

int
vscmGetSerial(int id)
{
  unsigned char buf[3];
  int i, mode;

  if (vscmIsNotInit(&id, __func__))
    return -1;

  mode = vscmGetSpiMode(id);

  vscmSelectSpi(id, 0, mode);
  vscmSelectSpi(id, 1, mode);

  vscmTransferSpi(id, 0x03, mode); /* Read Continuous */
  vscmTransferSpi(id, 0x7F, mode);
  vscmTransferSpi(id, 0xF0, mode);
  vscmTransferSpi(id, 0x00, mode);

  memset(buf, 0, sizeof(buf));
  for (i = 0; i < sizeof(buf); i++) {
    buf[i] = vscmTransferSpi(id, 0xFF, mode);
    if (buf[i] == 0x0)
      break;
    if (buf[i] == 0xFF) {
      buf[0] = 0x0;
      break;
    }
  }
  vscmSelectSpi(id, 0, mode);
  return atoi(buf);
}

void
vscmSetBCOFreq(int id, uint32_t freq)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  vmeWrite32(&VSCMpr[id]->FssrCtrl.ClkCtrl, freq);
}

int
vscmGetBCOFreq(int id)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  return vmeRead32(&VSCMpr[id]->FssrCtrl.ClkCtrl);
}

uint8_t
vscmSetDacCalibration(int id)
{
  uint8_t result;
  int mode;

  mode = vscmGetSpiMode(id);

  vscmSelectSpi(id, 0, mode);
  vscmSelectSpi(id, 1, mode);

  vscmTransferSpi(id, 0x03, mode); /* Read Continuous */
  vscmTransferSpi(id, 0x7F, mode);
  vscmTransferSpi(id, 0xF0, mode);
  vscmTransferSpi(id, 0x80, mode);
  result = vscmTransferSpi(id, 0xFF, mode);
  vscmSelectSpi(id, 0, mode);

  /* Do this to load calibration value into DAC */
  vmeWrite32(&VSCMpr[id]->Dac.Ctrl, (8192 << 16) | 0x80000D00 | (result & 0x3F));
  taskDelay(1);

#ifdef DEBUG
  logMsg("INFO: %s: DAC Calibration = %u\n", __func__, result);
#endif

  return result;
}

/* freq = rate in Hz for calibration pulser */
void
vscmSetPulserRate(int id, uint32_t freq)
{
  uint32_t periodCycles;
  uint32_t dutyCycles;
  uint32_t window;
  uint32_t trig_rate_limit;
  uint32_t bcoFreq;

  if (vscmIsNotInit(&id, __func__))
    return;

  if (!freq) {
    periodCycles = VSCM_SYS_CLK;
    dutyCycles = VSCM_SYS_CLK + 1;
  }
  else {
    /* subtract 1 since index is from 0 */
    periodCycles = (VSCM_SYS_CLK / freq) - 1;
    /* Always run at 50% duty cycle */
    dutyCycles = periodCycles >> 1;

    if (!dutyCycles) dutyCycles = 1;
    if (!periodCycles) periodCycles = 2;
  }

  /* Check to see if need to limit rate only if window is already set */
/*
  window = vmeRead32(&VSCMpr[id]->Eb.WindowWidth) / 8;
  if (window) {
    bcoFreq = vmeRead32(&VSCMpr[id]->FssrCtrl.ClkCtrl);
    window = (((window >> 24 & 0xFF) - (window >> 8 & 0xFF) & 0xFF) - 1);
    window *= bcoFreq;
    trig_rate_limit = 50000000 / (16 * (1 + window / bcoFreq));
    if (freq > trig_rate_limit) {
      printf("%s: WARNING: changing period cycles from %d to ",
        __func__, periodCycles);
      periodCycles = (int)((1.0 / trig_rate_limit) / 8.0e-9) + 0.5;
      printf("%s\n", periodCycles);
    }
  }
*/

  vmeWrite32(&VSCMpr[id]->Sd.PulserLowCycles, dutyCycles);
  vmeWrite32(&VSCMpr[id]->Sd.PulserPeriod, periodCycles);

#ifdef DEBUG
  logMsg("%s: Pulser setup (%u, %u)\n", __func__, periodCycles, dutyCycles);
#endif
}

/* Get the rate (in Hz) for the calibration pulser */
uint32_t
vscmGetPulserRate(int id)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  return (VSCM_SYS_CLK / (vmeRead32(&VSCMpr[id]->Sd.PulserPeriod) + 1));
}

/*
 * ch = which pulser connector to manipulate: 0 = both, 1 = top, 2 = bottom
 * amp = amplitude in millivolts (mV)
 * num_pulses = number of pulses to deliver - default to freerunning
 */
void
vscmPulser(int id, int ch, uint32_t amp, uint32_t num_pulses)
{
  const int length = 128;
  uint32_t i, val, unum;

  if (vscmIsNotInit(&id, __func__))
    return;

  if (ch < 0 || ch > 2) {
    logMsg("ERROR: %s Invalid channel, must be 0, 1 or 2\n", __func__);
    return;
  }

  /* Convert amplitude to DAC units
   * 1e3 factor is to convert into mV
   * Factor of 2 is from 50ohm termination */
  amp /= ((1.0 / (8192 * 2)) * 1e3);

  if (!num_pulses)
    unum = 0xFFFFFFFF;
  else
    unum = num_pulses;

  vmeWrite32(&VSCMpr[id]->Dac.Ctrl, (8192 << 16));
  vmeWrite32(&VSCMpr[id]->Sd.PulserNPulses, unum);

  for (i = 0; i < length; i++) {
    /* set first and last entries to "0" */
    if (i == 0 || i == (length - 1)) {
      val = 8192;
    }
    else {
      val = 8192 + amp;
    }

    if (ch == 0 || ch == 1) {
      vmeWrite32(&VSCMpr[id]->Dac.Ch0, (i << 23) | ((length - 1) << 14) | val);
    }
    if (ch == 0 || ch == 2) { 
      vmeWrite32(&VSCMpr[id]->Dac.Ch1, (i << 23) | ((length - 1) << 14) | val);
    }
  }
}

void
vscmPulserStart(int id)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  vmeWrite32(&VSCMpr[id]->Sd.PulserStart, 1);
}

void
vscmPulserStop(int id)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  vmeWrite32(&VSCMpr[id]->Sd.PulserNPulses, 0);
}

void
vscmPulserDelay(int id, uint8_t delay)
{
  uint32_t val;
  if (vscmIsNotInit(&id, __func__))
    return;

  val = (vmeRead32(&VSCMpr[id]->Dac.TrigCtrl) & 0xFF00FFFF) | (delay << 16);
  vmeWrite32(&VSCMpr[id]->Dac.TrigCtrl, val);
}

void
vscmPulserBCOSync(int id, uint8_t bco, int sync)
{
  uint32_t val;
  if (vscmIsNotInit(&id, __func__))
    return;

  val = vmeRead32(&VSCMpr[id]->Dac.TrigCtrl);

  if (sync == 1) {
    val &= ~(0x8000FF00); /* Clear bit 31 and BCO field */
    vmeWrite32(&VSCMpr[id]->Dac.TrigCtrl, val | (1 << 30) | (bco << 8));
  }
  else {
    val &= ~(0x4000FF00); /* Clear bit 30 and BCO field */
    vmeWrite32(&VSCMpr[id]->Dac.TrigCtrl, val | (1 << 31));
  }
}

void
vscmSetHitMask(int id, uint8_t mask, uint8_t trig_width)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  vmeWrite32(&VSCMpr[id]->Sd.TrgHitCtrl, mask);
  vmeWrite32(&VSCMpr[id]->Tdc.TrgHitWidth, trig_width);
}

uint8_t
vscmGetHitMask(int id)
{
 uint32_t val;
  if (vscmIsNotInit(&id, __func__))
    return;

  val = vmeRead32(&VSCMpr[id]->Sd.TrgHitCtrl);

  return val & 0xFF;
}

uint8_t
vscmGetHitMaskWidth(int id)
{
 uint32_t val;
  if (vscmIsNotInit(&id, __func__))
    return;

  val = vmeRead32(&VSCMpr[id]->Tdc.TrgHitWidth);

  return (val>>0) & 0xFFF;
}

/*
 * If id = 0 change id to first VSCM slot
 * Returns 1 if VSCM in slot id is not initalized
 * Returns 0 if VSCM is initalized
 */
int
vscmIsNotInit(int *id, const char *func)
{
  if(*id == 0) *id = vscmID[0];

  if((*id <= 0) || (*id > 21) || (VSCMpr[*id] == NULL))
  {
    logMsg("ERROR: %s: VSCM in slot %d is not initialized\n", func, *id);
    return(1);
  }

  return(0);
}

void
vscmSetBlockLevel(int id, int block_level)
{
  vmeWrite32(&VSCMpr[id]->Eb.BlockCfg, block_level);
}



void
vscmRebootFpga(int id)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  vmeWrite32(&VSCMpr[id]->Cfg.Reboot, 0x1);
}

void
vscmGRebootFpga()
{
  int i;

  /*VSCMLOCK;*/
  for (i = 0; i < nvscm; i++) {
    vscmRebootFpga(vscmID[i]);
  }
  /*VSCMUNLOCK;*/
}

void
vscmPrestart(char *fname)
{
  int i;
  char *env, filename[256];

  /*from old version - allows to end run after N events
  vscmSetMaxTriggerLimit(0);
  vscmClearTriggerCount();
  */

  for (i = 0; i < nvscm; i++) {
    vscmFifoClear(vscmID[i]);
    fssrMasterReset(vscmID[i]);
  }

  taskDelay(10);

  /* form config filename */
  if(fname[0]=='/') /* use 'as is' */
  {
    strcpy(filename,fname);
  }
  else /* add $CLON_PARMS before fname */
  {
    env = getenv("CLON_PARMS");
    if(env==NULL)
  {
      printf("ERROR in vscmPrestart: evn var CLON_PARMS does not exist\n");
      return;
  }
    strcpy(filename,env);
    strcat(filename,"/vscm/");
    strcat(filename,fname);
  }
  printf("vscmPrestart: use config file >%s<\n",filename);

  /* Initialize FSSR2 chips */
  for (i = 0; i < nvscm; i++)
  {
  fssrSetChipID(vscmID[i], 0, 1, 2, 3, 4);
    vscmConfigDownload(vscmID[i], filename);
  }
}

void
vscmSemPrintStats(int id)
{
  if (vscmIsNotInit(&id, __func__))
    return;

  printf("%s(%d):\n", __func__, id);
  printf("   heartbeat = %d\n", vmeRead32(&VSCMpr[id]->Cfg.SemHeartbeatCnt));
  printf("   errors    = %d\n", vmeRead32(&VSCMpr[id]->Cfg.SemErrorCnt));
}

void
vscmSemPrintStatsAll()
{
  int i;

  printf("%s:                 ", __func__);
  for(i=0;i<nvscm;i++) printf("%2d ", vscmID[i]);
  printf("\n");

  printf("   heartbeat = ");
  for(i=0;i<nvscm;i++) printf("%2d ",  vmeRead32(&VSCMpr[vscmID[i]]->Cfg.SemHeartbeatCnt));

  printf("   errors    = ");
  for(i=0;i<nvscm;i++) printf("%2d ", vmeRead32(&VSCMpr[vscmID[i]]->Cfg.SemErrorCnt));
  printf("\n");
}

int
vscmGSendScalers()
{
  char name[100];
  int i, id, hfcb, strip, chip, crate;
  float ref[4], data[512];
  unsigned int val, cnt;
  char host[100];

  gethostname(host,sizeof(host));
  for(i=0; i<strlen(host); i++)
  {
    if(host[i] == '.')
    {
      host[i] = '\0';
      break;
    }
  }

  if(!strcmp(host, "svt1"))
    crate = 0;
  else if(!strcmp(host, "svt2"))
    crate = 1;
  else
    return;

  for(i=0; i<nvscm; i++)
  {
    id = vscmID[i];
    if(vscmIsNotInit(&id, __func__))
      return 0;

    // SEM State
    cnt = vmeRead32(&VSCMpr[id]->Cfg.SemHeartbeatCnt);
    val = (cnt != vscmSemLastHeartbeat[id]) ? 1 : 0;
    vscmSemLastHeartbeat[id] = cnt;
    sprintf(name, "SVT_DAQ_SVT%dSLOT%d:SEMSTATE", crate+1, id);
    epics_json_msg_send(name, "int", 1, &val);

    // SEM State
    val = vmeRead32(&VSCMpr[id]->Cfg.SemErrorCnt);;
    sprintf(name, "SVT_DAQ_SVT%dSLOT%d:SEMERROR", crate+1, id);
    epics_json_msg_send(name, "int", 1, &val);

    // get&send VSCM related scalers
    vmeWrite32(&VSCMpr[id]->Sd.ScalerLatch, 1);

    for(hfcb=0; hfcb<2; hfcb++)
    {
      BST_trans_entry entry = BST_trans_table[crate][vscmID[i]][hfcb];
      if(!entry.module || !entry.region)
        continue;

      // Get reference scalers
      val = vmeRead32(&VSCMpr[id]->Sd.Scalers[VSCM_SCALER_GCLK125]);
      for(chip=0; chip<4; chip++)
      {
        if(!val) val = 1;
        ref[chip] = 125000000.0 / (float)val;
      }

      // Event word scaler
      for(chip=0; chip<4; chip++)
        data[chip] = ref[chip] * ((float)vmeRead32(&VSCMpr[id]->Fssr[hfcb*4+chip].ScalerEvent));
      sprintf(name, "SVT_DAQ_R%dS%d:EVTRATE", entry.region, entry.module);
      epics_json_msg_send(name, "float", 4, data);

      // Gothit scaler
      for(chip=0; chip<4; chip++)
        data[chip] = ref[chip] * ((float)vmeRead32(&VSCMpr[id]->Fssr[hfcb*4+chip].ScalerGotHit));
      sprintf(name, "SVT_DAQ_R%dS%d:GOTHIT", entry.region, entry.module);
      epics_json_msg_send(name, "float", 4, data);

      // Error scaler
      for(chip=0; chip<4; chip++)
      {
        data[chip] = ref[chip] * ((float)vmeRead32(&VSCMpr[id]->Fssr[hfcb*4+chip].ScalerMarkErr));
        data[chip]+= ref[chip] * ((float)vmeRead32(&VSCMpr[id]->Fssr[hfcb*4+chip].ScalerEncErr));
        data[chip]+= ref[chip] * ((float)vmeRead32(&VSCMpr[id]->Fssr[hfcb*4+chip].ScalerChipIdErr));

        if(!vmeRead32(&VSCMpr[id]->Fssr[hfcb*4+chip].ScalerStatusWord)) data[chip]++;
        if(!vmeRead32(&VSCMpr[id]->Fssr[hfcb*4+chip].ScalerEvent)) data[chip]++;
        if(!vmeRead32(&VSCMpr[id]->Fssr[hfcb*4+chip].ScalerWords)) data[chip]++;
      }
      sprintf(name, "SVT_DAQ_R%dS%d:FSSRERR", entry.region, entry.module);
      epics_json_msg_send(name, "float", 4, data);

      // Strip scalers
      for(chip=0; chip<4; chip++)
      {
        for(strip=0; strip<128; strip++)
          data[chip*128+strip] = ref[chip] * ((float)vmeRead32(&VSCMpr[id]->Fssr[hfcb*4+chip].HistCnt));
      }
      sprintf(name, "SVT_DAQ_R%dS%d:STRIPRATE", entry.region, entry.module);
      epics_json_msg_send(name, "float", 512, data);
    }

    vmeWrite32(&VSCMpr[id]->Sd.ScalerLatch, 0);
  }
  return 0;
}

/*
 * Returns the number of VSCMs initalized
 */
int
vscmInit(uintptr_t addr, uint32_t addr_inc, int numvscm, int flag)
{
  uintptr_t rdata;
  uintptr_t laddr, laddr2, vmeaddr, a32addr;
  volatile VSCM_regs *vreg;
  int i, res;
  int boardID = 0;
  uint32_t fw;

  /* Check for valid address */
  if (addr == 0) {
    logMsg("ERROR: %s: Must specify VME-based A24 address for VSCM 0\n", \
            __func__);
    return 0;
  }
  else if (addr > 0xFFFFFF) {
    logMsg("ERROR: %s: A32 addressing not allowed\n", __func__);
    return 0;
  }
  else
  {
    /* Make sure to try and init at least 1 board */
    if ((addr_inc == 0) || (numvscm == 0))
      numvscm = 1;
    /* There's only 21 slots in a VXS crate */
    if (numvscm > 21)
      numvscm = 21;

#ifdef VXWORKS
    res = sysBusToLocalAdrs(0x39, (char *)addr, (char **)&laddr);
#else
    res = vmeBusToLocalAdrs(0x39, (char *)addr, (char **)&laddr);
#endif
    if (res != 0)
    {
#ifdef VXWORKS
      logMsg("ERROR: %s: sysBusToLocalAdrs(0x39, 0x%x, &laddr)\n", \
              __func__, addr);
#else
      logMsg("ERROR: %s: vmeBusToLocalAdrs(0x39, %p, &laddr)\n", \
              __func__, (void *)addr);
#endif
      return 0;
    }
    vscmA24Offset = laddr - addr;

    vscmInited = nvscm = 0;
    bzero((char *)vscmID, sizeof(vscmID));

    for (i = 0; i < numvscm; i++)
    {
      vmeaddr = (addr + (i * addr_inc));
      /* skip slots that can't be a VSCM */
      /* 1=SBC 11,12=VXS switch slots */
      /* 2=Reserved for VXS SBC */
      switch(vmeaddr)
      {
        case (1 << 19):
        case (2 << 19):
        case (11 << 19):
        case (12 << 19):
          continue;
      }
      /* 21 is last slot (and is a TI) break out of loop if past it */
      /* EXCEPT when only initalizing 1 VSCM via switches */
      if (vmeaddr >= (21 << 19) && numvscm != 1)
        break;

      vreg = (VSCM_regs *)(laddr + (i * addr_inc));
#ifdef VXWORKS
      res = vxMemProbe((char *)&(vreg->Cfg.BoardID), VX_READ, 4, (char *)&rdata);
#else
      res = vmeMemProbe((char *)&(vreg->Cfg.BoardID), 4, (char *)&rdata);
#endif
      if (res < 0)
      {
#ifdef DEBUG
#ifdef VXWORKS
        logMsg("ERROR: %s: No addressable board at addr=0x%x\n", \
                __func__, (uint32_t)vreg);
#else
        logMsg("ERROR: %s: No addressable board at VME addr=%p (%p)\n", \
                __func__, vmeaddr, (void *)vreg);
#endif
#endif
      }
      else
      {
        if (rdata != VSCM_BOARD_ID)
        {
#ifdef DEBUG
          logMsg("ERROR: %s: For board at %p, Invalid Board ID: %p\n",
                  __func__, (void *)vreg, (void *)rdata);
#endif
          break;
        }
        fw = vmeRead32(&vreg->Cfg.FirmwareRev) & 0xFFFF;

        if( (fw>>8) == 3)
          boardID = (vmeRead32(&vreg->Cfg.FirmwareRev)>>24) & 0x1F;
        else if( (fw>>8) == 2)
          boardID = vmeRead32(&vreg->Cfg.FirmwareRev+0x50/4) & 0x1F;
        else
          boardID = 0;

        if ((boardID <= 0) || (boardID > 21))
        {
          logMsg("ERROR: %s: Board Slot ID %d is not in range.\n", \
                  __func__, boardID);
          return 0;
        }

        VSCMpr[boardID] = (VSCM_regs *)(laddr + (i * addr_inc));
        vscmID[nvscm] = boardID; /* Slot Number */
        if (boardID >= maxSlot) maxSlot = boardID;
        if (boardID <= minSlot) minSlot = boardID;

        nvscm++;
        logMsg("INFO: found VSCM board at slot %2d (FW: %1u.%-2u SN: %2u)\n", \
                boardID, (fw >> 8), (fw & 0xFF), vscmGetSerial(boardID));
      }
    }
    /* Do not configure board, just setup register pointers and return */
    if(flag & 0x80000000)
      return nvscm;

    /* Setup FIFO pointers */
    for (i = 0; i < nvscm; i++)
    {
      a32addr = vscmA32Base + (i * VSCM_MAX_FIFO);

      /* Event readout setup */
      vmeWrite32(&VSCMpr[vscmID[i]]->Eb.AD32, \
                  ((a32addr >> 16) & 0xFF80) | 0x0001);

#ifdef VXWORKS
      res = sysBusToLocalAdrs(0x09, (char *)a32addr, (char **)&laddr2);
      if (res != 0)
      {
        logMsg("ERROR: %s: sysBusToLocalAdrs(0x09, 0x%x, &laddr2)\n", \
                __func__, a32addr);
        return 0;
      }
#else
      res = vmeBusToLocalAdrs(0x09, (char *)a32addr, (char **)&laddr2);
      if (res != 0)
      {
        logMsg("ERROR: %s: vmeBusToLocalAdrs(0x09, %p, &laddr2)\n",
                __func__, (void *)a32addr);
        return 0;
      }
#endif
      VSCMpf[vscmID[i]] = (uintptr_t *)laddr2;
      vscmA32Offset = laddr2 - a32addr;
    }

     /*
      * If more than 1 VSCM in crate then setup the Muliblock Address
      * window. This must be the same on each board in the crate
      */
    if (nvscm > 1)
    {
      /* set MB base above individual board base */
      a32addr = vscmA32Base + (nvscm * VSCM_MAX_FIFO);
#ifdef VXWORKS
      res = sysBusToLocalAdrs(0x09, (char *)a32addr, (char **)&laddr);
      if (res != 0)
      {
        printf("ERROR: %s: in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",
                __func__, a32addr);
        return EXIT_FAILURE;
      }
#else
      res = vmeBusToLocalAdrs(0x09, (char *)a32addr, (char **)&laddr);
      if (res != 0)
      {
        printf("ERROR: %s: in vmeBusToLocalAdrs(0x09,0x%x,&laddr) \n",
                __func__, a32addr);
        return EXIT_FAILURE;
      }
#endif
      VSCMpmb = (uintptr_t *)laddr;  /* Set a pointer to the FIFO */
    for (i = 0; i < nvscm; i++)
      {
        /* Write the register and enable */
        vmeWrite32((volatile unsigned int *)&(VSCMpr[vscmID[i]]->Eb.Adr32M),
                    ((a32addr + VSCM_MAX_A32MB_SIZE) >> 7) |
                    (a32addr >> 23) | (1 << 25));
    }
      vmeWrite32((volatile unsigned int *)&(VSCMpr[minSlot]->Eb.Adr32M),
                vmeRead32((volatile unsigned int *)&(VSCMpr[minSlot]->Eb.Adr32M))
                            | (1 << 26));
      vmeWrite32((volatile unsigned int *)&(VSCMpr[maxSlot]->Eb.Adr32M),
                vmeRead32((volatile unsigned int *)&(VSCMpr[maxSlot]->Eb.Adr32M))
                            | (1 << 27));
    }

    /* Setup VSCM */
    for (i = 0; i < nvscm; i++)
    {
      boardID = vscmID[i]; /* slot number */


#ifdef HBI
      vmeWrite32(&VSCMpr[boardID]->Clk.Ctrl, 0);
      vmeWrite32(&VSCMpr[boardID]->Sd.TrigCtrl, IO_MUX_FPINPUT0);
      /* Set sync input to reset state */
      vmeWrite32(&VSCMpr[boardID]->Sd.SyncCtrl, IO_MUX_0);
#else

      if((flag&0x1)==0)
        vscmSetClockSource(boardID, 1);
      else
        vscmSetClockSource(boardID, 0);

      /* get trigger from switch slot B */
      vmeWrite32(&VSCMpr[boardID]->Sd.TrigCtrl, IO_MUX_SWB_TRIG1);

      /* if there is no TI, cannot use IO_MUX_SWB_SYNC !!!
     to avoid problems, set it to IO_MUX_0 if internal */
      if((flag&0x1)==0)
    {
        /* get sync from switch slot B */
        vmeWrite32(&VSCMpr[boardID]->Sd.SyncCtrl, IO_MUX_SWB_SYNC);
    }
    else
    {
        /* set SYNC line to IO_MUX_0 */
        vmeWrite32(&VSCMpr[boardID]->Sd.SyncCtrl, IO_MUX_0);
    }

#endif


      /* Setup Front Panel and Trigger */
      vmeWrite32(&VSCMpr[boardID]->Sd.FpOutputCtrl[0], IO_MUX_BCOCLK);
      vmeWrite32(&VSCMpr[boardID]->Sd.FpOutputCtrl[1], IO_MUX_FPINPUT1 | (384 << 16));
      vmeWrite32(&VSCMpr[boardID]->Sd.FpOutputCtrl[2], IO_MUX_DACTRIGGERED);
      vmeWrite32(&VSCMpr[boardID]->Sd.FpOutputCtrl[3], IO_MUX_DACTRIGGERED_DLY);
      vmeWrite32(&VSCMpr[boardID]->Sd.DacTrigCtrl, IO_MUX_PULSER);
      vmeWrite32(&VSCMpr[boardID]->Dac.TrigCtrl, 0x80000000 | (0 << 16));

      /* the number of events per block */
      vmeWrite32(&VSCMpr[boardID]->Eb.BlockCfg, 1);
    
    /* maximum number of unprocessed triggers in buffer before busy is asserted */
    vmeWrite32(&VSCMpr[boardID]->Eb.TrigCntBusyThr, 100);


      /* delay for trigger processing in a board - must be more then 4us for 70MHz readout clock;
      if clock changed, it must be changes as well; ex. 35MHz -> 1024 etc */
      /* delay before start data processing in vscm board after recieving trigger */
/*** NOT SUPPORTED - delay trigger instead ***/
/*      vmeWrite32(&VSCMpr[boardID]->TrigLatency, 512);*/ /* multiply by 8ns */

      /* Enable Bus Error */
      vmeWrite32(&VSCMpr[boardID]->Eb.ReadoutCfg, 1);


      /* FSSR Clock & Triggering setup */
      vscmSetBCOFreq(boardID, 16);
      vscmSetTriggerWindow(boardID, 256, 256, 16);


      /* Setup VSCM Pulser */
      vscmSetDacCalibration(boardID);
      vscmSetPulserRate(boardID, 200000);

    /* Send FSSR gothit OR to SWB Trigout */
    /* Delay trigger output to match CLAS12 typical trigger latency */
/*    vmeWrite32(&VSCMpr[boardID]->TrigOutCfg, IO_MUX_FSSRHIT_TRIG | (755<<16));*/

    /* Send FSSR gothit OR to SWB Trigout: this one requires coincidence between top/bottom silicon layers on each HFCB */
    /* Delay trigger output to match CLAS12 typical trigger latency */
    vmeWrite32(&VSCMpr[boardID]->Sd.TrigoutCtrl, IO_MUX_FSSRHIT_TBAND_TRIG | (755<<16));
    
    /* Enable all gothit signals, stretch by 64*8ns for triggering */
    vscmSetHitMask(boardID, 0xFF, 64);

      /* Clear event buffers */
      vscmFifoClear(boardID);
  
      vscmSWSync(boardID);
    /*fssrMasterReset(boardID);*/
    }

#ifdef CODA3DMA
    /* VME DMA setup */
#ifdef VXWORKS
    VME_DMAInit();
    VME_DMAMode(DMA_MODE_BLK32);
#else
    vmeDmaConfig(2, 5, 1);
    dmaPFreeAll();
    vmeIN = dmaPCreate("vmeIN", 2048, 10, 0);
    vmeOUT = dmaPCreate("vmeOUT", 0, 0, 0);
#ifdef DEBUG
    dmaPStatsAll();
#endif
    dmaPReInitAll();
#endif

#endif
  }

  logMsg("INFO: %s: Found %2d VSCM board(s)\n", __func__, nvscm);
  if (nvscm > 16) {
    logMsg("WARN: There are only 16 payload slots in a VXS Crate\n");
  }

  return nvscm;
}









/**** CONFIG ************************************/
/**** CONFIG ************************************/
/**** CONFIG ************************************/

typedef struct
{
  int clock_int_ext; /* clock source: 0-internal, 1-external */
  int bco_freq;      /* BCO frequency */

  int window_width;  /* trigger window width */
  int window_offset; /* trigger window offset */
  int window_bco;    /* ? */

  int fssr_addr_reg_disc_threshold[8][8];
  int fssr_addr_reg_dcr[8];
  int fssr_addr_reg_kill_mask[8][4];
  int fssr_addr_reg_inject_mask[8][4];

  int fssr_gothit_en_mask;
  int fssr_gothit_trig_width;
} VSCM_CONFIG_STRUCT;

static VSCM_CONFIG_STRUCT conf[VSCM_MAX_BOARDS+1]; /* index is slot number */

static char *expid = NULL;

void
vscmSetExpid(char *string)
{
  expid = strdup(string);
}


void
vscmInitGlobals()
{
  int ii, jj, kk, slot;

  for(kk=0; kk<nvscm; kk++)
  {
    slot = vscmID[kk];
    vscmFifoClear(slot);
    fssrMasterReset(slot);
  fssrSetChipID(slot, 0, 1, 2, 3, 4);
  }

  taskDelay(10);

  for(kk=0; kk<nvscm; kk++)
  {
    slot = vscmID[kk];

    conf[slot].fssr_gothit_en_mask = 0xFF;
    conf[slot].fssr_gothit_trig_width = 64;

    conf[slot].clock_int_ext = 0;

    conf[slot].bco_freq = 16;

    conf[slot].window_width = 256;
    conf[slot].window_offset = 256;
    conf[slot].window_bco = 16;  /* use conf[slot].bco_freq instead */

    for(ii=0;ii<8;ii++) for(jj=0;jj<8;jj++) conf[slot].fssr_addr_reg_disc_threshold[ii][jj] = 140;

    for(ii=0;ii<8;ii++) conf[slot].fssr_addr_reg_dcr[ii] = 0x18;

    for(ii=0;ii<8;ii++)
  {
      conf[slot].fssr_addr_reg_kill_mask[ii][0] = 0;
      conf[slot].fssr_addr_reg_kill_mask[ii][1] = 0;
      conf[slot].fssr_addr_reg_kill_mask[ii][2] = 0;
      conf[slot].fssr_addr_reg_kill_mask[ii][3] = 0;
  }

    for(ii=0;ii<8;ii++)
  {
      conf[slot].fssr_addr_reg_inject_mask[ii][0] = 0;
      conf[slot].fssr_addr_reg_inject_mask[ii][1] = 0;
      conf[slot].fssr_addr_reg_inject_mask[ii][2] = 0;
      conf[slot].fssr_addr_reg_inject_mask[ii][3] = 0;
  }

  }
 
  return;
}



int
vscmReadConfigFile(char *filename_in)
{
  FILE   *fd;
  char   filename[FNLEN];
  char   fname[FNLEN] = { "" };  /* config file name */
  int    ii, jj, ch;
  char   str_tmp[STRLEN], keyword[ROCLEN];
  char   host[ROCLEN], ROC_name[ROCLEN];
  char   str2[2];
  int    args, i1, i2, i3, i4;
  float  f1;
  int    slot1, slot2, slot, chan;
  unsigned int  ui1, ui2;
  char *getenv();
  char *clonparms;
  int do_parsing;

  int nval;
  char charval[10][STRLEN];
  unsigned int val[10];

  gethostname(host,ROCLEN);  /* obtain our hostname */
  clonparms = getenv("CLON_PARMS");

  if(expid==NULL)
  {
    expid = getenv("EXPID");
    printf("\nNOTE: use EXPID=>%s< from environment\n",expid);
  }
  else
  {
    printf("\nNOTE: use EXPID=>%s< from CODA\n",expid);
  }

  strcpy(filename,filename_in); /* copy filename from parameter list to local string */
  do_parsing = 1;

  while(do_parsing)
  {
    if(strlen(filename)!=0) /* filename specified */
    {
      if ( filename[0]=='/' || (filename[0]=='.' && filename[1]=='/') )
      {
        sprintf(fname, "%s", filename);
      }
      else
      {
        sprintf(fname, "%s/vscm/%s", clonparms, filename);
      }

      if((fd=fopen(fname,"r")) == NULL)
      {
        printf("\nvscmReadConfigFile: Can't open config file >%s<\n",fname);
        return(-1);
      }
    }
    else if(do_parsing<2) /* filename does not specified */
    {
      sprintf(fname, "%s/vscm/%s.cnf", clonparms, host);
      if((fd=fopen(fname,"r")) == NULL)
      {
        sprintf(fname, "%s/vscm/%s.cnf", clonparms, expid);
        if((fd=fopen(fname,"r")) == NULL)
        {
          printf("\nvscmReadConfigFile: Can't open config file >%s<\n",fname);
          return(-2);
        }
      }
    }
    else
    {
      printf("\nReadConfigFile: ERROR: since do_parsing=%d (>1), filename must be specified\n",do_parsing);
      return(-1);
    }

    printf("\nvscmReadConfigFile: Using configuration file >%s<\n",fname);

    /* Parsing of config file */
    active = 0; /* by default disable crate */
    do_parsing = 0; /* will parse only one file specified above, unless it changed during parsing */
    while ((ch = getc(fd)) != EOF)
    {
      if ( ch == '#' || ch == ' ' || ch == '\t' )
      {
        while (getc(fd) != '\n') {}
      }
      else if( ch == '\n' ) {}
      else
      {
        ungetc(ch,fd);
        fgets(str_tmp, STRLEN, fd);
        sscanf (str_tmp, "%s %s", keyword, ROC_name);


        /* Start parsing real config inputs */
        if(strcmp(keyword,"VSCM_CRATE") == 0)
        {
          if(strcmp(ROC_name,host) == 0)
          {
            printf("\nReadConfigFile: crate = %s  host = %s - activated\n",ROC_name,host);
            active = 1;
          }
          else if(strcmp(ROC_name,"all") == 0)
          {
            printf("\nReadConfigFile: crate = %s  host = %s - activated\n",ROC_name,host);
            active = 1;
          }
          else
          {
            printf("\nReadConfigFile: crate = %s  host = %s - disactivated\n",ROC_name,host);
            active = 0;
          }
        }

        else if(active && (strcmp(keyword,"VSCM_CONF_FILE")==0))
        {
          sscanf (str_tmp, "%*s %s", str2);
          /*printf("str2=%s\n",str2);*/
          strcpy(filename,str2);
          do_parsing = 2;
        }

        else if(active && (strcmp(keyword,"VSCM_SLOT")==0))
        {
          sscanf (str_tmp, "%*s %s", str2);
          if(isdigit(str2[0]))
          {
            slot1 = atoi(str2);
            slot2 = slot1 + 1;
            if(slot1<2 && slot1>21)
            {
              printf("\nReadConfigFile: Wrong slot number %d\n\n",slot1);
              return(-1);
            }
          }
          else if(!strcmp(str2,"all"))
          {
            slot1 = 0;
            slot2 = VSCM_MAX_BOARDS+1;
          }
          else
          {
            printf("\nReadConfigFile: Wrong slot >%s<, must be 'all' or actual slot number\n\n",str2);
            return(-1);
          }
        }

        else if(active && (!strcmp(keyword,"VSCM_CLOCK_INTERNAL")))
        {
          for(slot=slot1; slot<slot2; slot++) conf[slot].clock_int_ext = 0;
        }

        else if(active && (!strcmp(keyword,"VSCM_CLOCK_EXTERNAL")))
        {
          for(slot=slot1; slot<slot2; slot++) conf[slot].clock_int_ext = 1;
        }

        else if(!strcmp(keyword,"VSCM_BCO_FREQ"))
        {
          sscanf(str_tmp,"%*s %3s",charval[0]);
          nval = 1;        
          VAL_DECODER;
          for(slot=slot1; slot<slot2; slot++) conf[slot].bco_freq = val[0];
        }

        else if(active && (!strcmp(keyword,"VSCM_TRIG_WINDOW")))
        {
          sscanf(str_tmp,"%*s %4s %4s %4s",charval[0],charval[1],charval[2]);
          nval = 3;
          VAL_DECODER;
          for(slot=slot1; slot<slot2; slot++) 
          {
            conf[slot].window_width = val[0];
            conf[slot].window_offset = val[1];
            conf[slot].window_bco = val[2];
          }
        }

        else if(active && (!strcmp(keyword,"VCSM_FSSR_GOTHIT_CFG")))
        {
          sscanf(str_tmp,"%*s %4s %4s",charval[0],charval[1]);
          nval = 2;
          VAL_DECODER;
          for(slot=slot1; slot<slot2; slot++)
          {
            conf[slot].fssr_gothit_en_mask = val[0];
            conf[slot].fssr_gothit_trig_width = val[1];
          }
        }

        else if(active && (!strcmp(keyword,"FSSR_ADDR_REG_DISC_THR")))
        {
          sscanf(str_tmp,"%*s %1s %1s %3s",charval[0], charval[1], charval[2]);
          nval = 3;        
          VAL_DECODER;
          for(slot=slot1; slot<slot2; slot++) conf[slot].fssr_addr_reg_disc_threshold[val[0]][val[1]] = val[2];
        }

        else if(active && (!strcmp(keyword,"FSSR_ADDR_REG_DCR")))
        {
          sscanf(str_tmp,"%*s %1s %4s",charval[0],charval[1]);
          nval = 2;        
          VAL_DECODER;
          for(slot=slot1; slot<slot2; slot++) conf[slot].fssr_addr_reg_dcr[val[0]] = val[1];
        }

        else if(active && (!strcmp(keyword,"FSSR_ADDR_REG_KILL_STRIP")))
        {
          sscanf(str_tmp,"%*s %d %d",&i1,&i2);
          if( (i1 < 0) || (i1 > 7) || (i2 < 0) || (i2 > 127) )
          {
            printf("%s: %s error %d %d\n", __func__, keyword, i1, i2);
            exit(1);
          }
          for(slot=slot1; slot<slot2; slot++) 
          {
            int reg, bit;
            i2 = 127-i2;
            reg = i2/32;
            bit = i2%32;
            printf("FSSR_ADDR_REG_KILL_STRIP reg=%d,bit=%d\n", reg, bit);
            conf[slot].fssr_addr_reg_kill_mask[i1][reg] |= 1<<bit;
          }
        }

        else if(active && (!strcmp(keyword,"FSSR_ADDR_REG_KILL")))
        {
          sscanf(str_tmp,"%*s %1s %10s %10s %10s %10s",charval[0], charval[1], charval[2],charval[3], charval[4]);
          nval = 5;        
          VAL_DECODER;
          for(slot=slot1; slot<slot2; slot++) 
          {
            conf[slot].fssr_addr_reg_kill_mask[val[0]][0] = val[1];
            conf[slot].fssr_addr_reg_kill_mask[val[0]][1] = val[2];
            conf[slot].fssr_addr_reg_kill_mask[val[0]][2] = val[3];
            conf[slot].fssr_addr_reg_kill_mask[val[0]][3] = val[4];
          }
        }

        else if(active && (!strcmp(keyword,"FSSR_ADDR_REG_INJECT")))
        {
          sscanf(str_tmp,"%*s %1s %10s %10s %10s %10s", keyword,charval[0], charval[1], charval[2], charval[3], charval[4]);
          nval = 5;        
          VAL_DECODER;
          for(slot=slot1; slot<slot2; slot++) 
          {
            conf[slot].fssr_addr_reg_inject_mask[val[0]][0] = val[1];
            conf[slot].fssr_addr_reg_inject_mask[val[0]][1] = val[2];
            conf[slot].fssr_addr_reg_inject_mask[val[0]][2] = val[3];
            conf[slot].fssr_addr_reg_inject_mask[val[0]][3] = val[4];
          }
        }

        else
        {
          ; /* unknown key - do nothing */
    /*
          printf("vscmReadConfigFile: Unknown Field or Missed Field in\n");
          printf("   %s \n", fname);
          printf("   str_tmp=%s", str_tmp);
          printf("   keyword=%s \n\n", keyword);
          return(-10);
    */
        }
      }
    } /* end of while */

    fclose(fd);
  }

  return(0);
}

int
vscmDownloadAll()
{
  int ii, jj, id, slot, ret;

  for(id=0; id<nvscm; id++)
  {
    //Ignore SWB Sync during register configuration
    slot = vscmSlot(id);

    vmeWrite32(&VSCMpr[slot]->Sd.SyncCtrl, IO_MUX_0);

    vscmSetClockSource(slot, conf[slot].clock_int_ext);

    vscmSetBCOFreq(slot, conf[slot].bco_freq);

    vscmSetTriggerWindow(slot, conf[slot].window_width, conf[slot].window_offset, conf[slot].window_bco);

    vscmSetHitMask(slot, conf[slot].fssr_gothit_en_mask, conf[slot].fssr_gothit_trig_width);

    for(ii=0;ii<8;ii++) for(jj=0;jj<8;jj++) fssrSetThreshold(slot,ii,jj,conf[slot].fssr_addr_reg_disc_threshold[ii][jj]);

    for(ii=0;ii<8;ii++) fssrSetControl(slot,ii,conf[slot].fssr_addr_reg_dcr[ii]);

    for(ii=0;ii<8;ii++)
  {
      ret = fssrSetMask(slot,ii,FSSR_ADDR_REG_KILL,(uint32_t *)&conf[slot].fssr_addr_reg_kill_mask[ii][0]);
#ifdef DEBUG
      if(ret) printf("ERROR: %s: %d/%u Mask Reg# %d not set correctly\n",__func__,slot,ii,FSSR_ADDR_REG_KILL); 
#endif
  }

    for(ii=0;ii<8;ii++)
  {
      ret = fssrSetMask(slot,ii,FSSR_ADDR_REG_INJECT,(uint32_t *)&conf[slot].fssr_addr_reg_inject_mask[ii][0]);
#ifdef DEBUG
      if(ret) printf("ERROR: %s: %d/%u Mask Reg# %d not set correctly\n",__func__,slot,ii,FSSR_ADDR_REG_INJECT); 
#endif
  }

    for(ii=0; ii<8; ii++)
    {
      fssrSetActiveLines(slot, ii, FSSR_ALINES_6);
      fssrRejectHits(slot, ii, 0);
      fssrSCR(slot, ii);
      fssrSendData(slot, ii, 1);
    }

    //Enable SWB Sync after register configuration
    vmeWrite32(&VSCMpr[slot]->Sd.SyncCtrl, IO_MUX_SWB_SYNC);
  }

  return(0);
}



/* vscmInit() have to be called before this function */
int  
vscmConfig(char *fname)
{
  int res;
  char *string; /*dummy, will not be used*/

  if(strlen(fname) > 0) /* filename specified  - upload initial settings from the hardware */
  {
    vscmUploadAll(string, 0);
  }
  else /* filename not specified  - set defaults */
  {
    vscmInitGlobals();
  }

  /* read config file */
  if( (res = vscmReadConfigFile(fname)) < 0 ) return(res);

  /* download to all boards */
  vscmDownloadAll();

  return(0);
}

void
vscmMon(int slot)
{
  ;
}



/* upload setting from all found DSC2s */
int
vscmUploadAll(char *string, int length)
{
  int id, slot, i, ii, jj, kk, ifiber, len1, len2;
  char *str, sss[1024];
  unsigned int tmp, connectedfibers;
  unsigned short sval;
  unsigned short bypMask;
  unsigned short channels[8];

  for(id=0; id<nvscm; id++)
  {
    slot = vscmSlot(id);

    conf[slot].clock_int_ext = vscmGetClockSource(slot);

    conf[slot].bco_freq = vscmGetBCOFreq(slot);

    conf[slot].fssr_gothit_en_mask = vscmGetHitMask(slot);

    conf[slot].fssr_gothit_trig_width = vscmGetHitMaskWidth(slot);

    conf[slot].window_width = vscmGetTriggerWindowWidth(slot);
    conf[slot].window_offset = vscmGetTriggerWindowOffset(slot);
/*    conf[slot].window_bco = remove this - not needed */

    for(ii=0;ii<8;ii++) for(jj=0;jj<8;jj++) conf[slot].fssr_addr_reg_disc_threshold[ii][jj] = fssrGetThreshold(slot, ii, jj);

    for(ii=0;ii<8;ii++) conf[slot].fssr_addr_reg_dcr[ii] = fssrGetControl(slot, ii);

    for(ii=0;ii<8;ii++)
      fssrGetKillMask(slot, ii, (uint32_t *)&conf[slot].fssr_addr_reg_kill_mask[ii][0]);

    for(ii=0;ii<8;ii++)
      fssrGetInjectMask(slot, ii, (uint32_t *)&conf[slot].fssr_addr_reg_inject_mask[ii][0]);
  }

  if(length)
  {
    str = string;
    str[0] = '\0';

    for(id=0; id<nvscm; id++)
    {
      slot = vscmSlot(id);

      sprintf(sss,"VSCM_SLOT %d\n",slot);
      ADD_TO_STRING;

      if(conf[slot].clock_int_ext==0)      {sprintf(sss,"VSCM_CLOCK_INTERNAL\n");ADD_TO_STRING;}
    else if(conf[slot].clock_int_ext==1) {sprintf(sss,"VSCM_CLOCK_EXTERNAL\n");ADD_TO_STRING;}

      sprintf(sss,"VSCM_BCO_FREQ %d\n",conf[slot].bco_freq); ADD_TO_STRING;

      sprintf(sss,"VSCM_TRIG_WINDOW %d %d %d\n",conf[slot].window_width,conf[slot].window_offset,conf[slot].window_bco); ADD_TO_STRING;

      sprintf(sss,"VCSM_FSSR_GOTHIT_CFG 0x%02X %d\n",conf[slot].fssr_gothit_en_mask,conf[slot].fssr_gothit_trig_width); ADD_TO_STRING;

      for(jj=0;jj<8;jj++)
    {
        for(ii=0;ii<8;ii++)
      {
      sprintf(sss,"FSSR_ADDR_REG_DISC_THR %d %d %d\n",ii,jj,conf[slot].fssr_addr_reg_disc_threshold[ii][jj]); ADD_TO_STRING;
      }
    }

      for(ii=0;ii<8;ii++)
    {
        sprintf(sss,"FSSR_ADDR_REG_DCR %d %d\n",ii,conf[slot].fssr_addr_reg_dcr[ii]); ADD_TO_STRING;
    }

      for(ii=0;ii<8;ii++)
    {
        sprintf(sss,"FSSR_ADDR_REG_KILL %d 0x%08x 0x%08x 0x%08x 0x%08x\n",ii,
                conf[slot].fssr_addr_reg_kill_mask[ii][0],
          conf[slot].fssr_addr_reg_kill_mask[ii][1],
          conf[slot].fssr_addr_reg_kill_mask[ii][2],
          conf[slot].fssr_addr_reg_kill_mask[ii][3]);
        ADD_TO_STRING;
    }

      for(ii=0;ii<8;ii++)
    {
        sprintf(sss,"FSSR_ADDR_REG_INJECT %d 0x%08x 0x%08x 0x%08x 0x%08x\n",ii,
                conf[slot].fssr_addr_reg_inject_mask[ii][0],
          conf[slot].fssr_addr_reg_inject_mask[ii][1],
          conf[slot].fssr_addr_reg_inject_mask[ii][2],
          conf[slot].fssr_addr_reg_inject_mask[ii][3]);
        ADD_TO_STRING;
    }
    }

    CLOSE_STRING;
  }

}

int
vscmUploadAllPrint()
{
  char str[65537];
  vscmUploadAll(str, 65536);
  printf("%s",str);
}








#else /* dummy version*/

void
vscmLib_dummy()
{
  return;
}

#endif

