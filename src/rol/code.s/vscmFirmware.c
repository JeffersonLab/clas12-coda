/* vscmFirmware.c */

#if defined(VXWORKS) || defined(Linux_vme)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vscmLib.h"

#define DEBUG

#define FLASH_CMD_WRPAGE      0x02
#define FLASH_CMD_RD          0x03
#define FLASH_CMD_GETSTATUS   0x05
#define FLASH_CMD_WREN        0x06
#define FLASH_CMD_GETID       0x9F
#define FLASH_CMD_ERASE64K    0xD8

#define FLASH_BYTE_LENGTH     8*1024*1024
#define FLASH_MFG_WINBOND     0xEF
#define FLASH_DEVID_W25Q64    0x4017

#define VSCM_SPI_MODE_BITBANG   0
#define VSCM_SPI_MODE_PARALLEL  1
#define VSCM_SPI_MODE_REV2      2
#define VSCM_SPI_MODE_REV3      3

extern volatile VSCM_regs *VSCMpr[VSCM_MAX_BOARDS + 1];

int
vscmGetSpiMode(int id)
{
  uint32_t fw_rev = vscmFirmwareRev(id);
  uint32_t fw_major = fw_rev>>8;
  int result = -1;

  if(fw_major == 2)
  {
    volatile unsigned int *p = (volatile unsigned int *)(&VSCMpr[id]->Cfg.SpiRev2);
    vmeWrite32(p, 0x104);
    result = VSCM_SPI_MODE_REV2;
  }
  else if(fw_major == 3)
  {
    vmeWrite32(&VSCMpr[id]->Cfg.SpiCtrl, 0x100);
    result = VSCM_SPI_MODE_REV3;
  }
  else
    printf("Error - unknown VSCM SPI mode to use...\n");

  printf("%s: Rev=0x%04X, Using SPI mode %d\n", __func__, fw_rev, result);

  return result;
}

void
vscmSelectSpi(int id, int sel, int mode)
{
  if(mode == VSCM_SPI_MODE_REV2)
  {
    volatile unsigned int *p = (volatile unsigned int *)(&VSCMpr[id]->Cfg.SpiRev2);
    if (sel)
      vmeWrite32(p, 0x200);
    else
      vmeWrite32(p, 0x100);
  }
  else if(mode == VSCM_SPI_MODE_REV3)
  {
    if (sel)
      vmeWrite32(&VSCMpr[id]->Cfg.SpiCtrl, 0x200);
    else
      vmeWrite32(&VSCMpr[id]->Cfg.SpiCtrl, 0x100);
  }
}

uint8_t
vscmTransferSpi(int id, uint8_t data, int mode)
{
  int i;
  uint32_t rsp = 0;

  if(mode == VSCM_SPI_MODE_REV2)
  {
    volatile unsigned int *p = (volatile unsigned int *)(&VSCMpr[id]->Cfg.SpiRev2);
    vmeWrite32(p, data | 0x400);
    rsp = vmeRead32(p);
  }
  else if(mode == VSCM_SPI_MODE_REV3)
  {
    vmeWrite32(&VSCMpr[id]->Cfg.SpiCtrl, data | 0x400);
    for(i=0;i<8;i++)
    {
      rsp = vmeRead32(&VSCMpr[id]->Cfg.SpiStatus);
      if(rsp & 0x800)
        break;
    }
  }
  return rsp & 0xFF;
}

void
vscmFlashGetID(int id, uint8_t *rsp, int mode)
{
	vscmSelectSpi(id, 1, mode);
	vscmTransferSpi(id, FLASH_CMD_GETID, mode);
	rsp[0] = vscmTransferSpi(id, 0xFF, mode);
	rsp[1] = vscmTransferSpi(id, 0xFF, mode);
	rsp[2] = vscmTransferSpi(id, 0xFF, mode);
	vscmSelectSpi(id, 0, mode);
}

uint8_t
vscmFlashGetStatus(int id, int mode)
{
  uint8_t rsp;

  vscmSelectSpi(id, 1, mode);
  vscmTransferSpi(id, FLASH_CMD_GETSTATUS, mode);
  rsp = vscmTransferSpi(id, 0xFF, mode);
  vscmSelectSpi(id, 0, mode);

  return rsp;
}

void
vscmReloadFirmware(int id)
{
#if 0
  int i;
  uint16_t reloadSequence[] = {
    0xFFFF, 0xAA99, 0x5566, 0x3261,
    0x0000, 0x3281, 0x0B00, 0x32A1,
    0x0000, 0x32C1, 0x0B00, 0x30A1,
    0x000E, 0x2000
  };

  if (vscmIsNotInit(&id, __func__))
    return;

  vmeWrite32(&VSCMpr[id]->ICap, 0x40000 | 0x00000);
  vmeWrite32(&VSCMpr[id]->ICap, 0x40000 | 0x20000);
  for (i = 0; i < (sizeof(reloadSequence) / sizeof(reloadSequence[0])); i++) {
    vmeWrite32(&VSCMpr[id]->ICap, 0x00000 | reloadSequence[i]);
    vmeWrite32(&VSCMpr[id]->ICap, 0x20000 | reloadSequence[i]);
  }
  for (i = 0; i < 10; i++) {
    vmeWrite32(&VSCMpr[id]->ICap, 0x40000 | 0x00000);
    vmeWrite32(&VSCMpr[id]->ICap, 0x40000 | 0x20000);
  }
  /* Delay to allow firmware to finish loading */
  printf("Reloading Firmware ... ");fflush(stdout);
#ifdef VXWORKS
  taskDelay(2*sysClkRateGet());
#else
  sleep(2);
#endif
  printf("Finished\n");
#endif
}

int
vscmIsFirmwareFile(FILE *f, const char *func)
{
  const uint8_t head[] = {170, 153, 85, 102, 48, 161, 0, 7};
  uint8_t buf[8];
  /* Skip first 16 bytes as they're always 0xFF */
  fseek(f, 16, SEEK_SET);
  fread(buf, 1, 8, f);

  if (memcmp(head, buf, 8) != 0) {
    printf("ERROR: %s: Not Xilinx .bin file for VSCM\n", func);
    return 0;
  }
  /* Set stream back to beginning */
  rewind(f);
  return 1; 
}

int
vscmFirmwareUpdate(int id, const char *filename)
{
  uint8_t rspID[3];
  int mode;

  if (vscmIsNotInit(&id, __func__))
    return EXIT_FAILURE;

  mode = vscmGetSpiMode(id);

  vscmSelectSpi(id, 0, mode);
  vscmFlashGetID(id, rspID, mode);
#ifdef DEBUG
  printf("Flash: Mfg=0x%02X, Type=0x%02X, Capacity=0x%02X\n", \
          rspID[0], rspID[1], rspID[2]);
#endif

  if ((rspID[0] == FLASH_MFG_WINBOND) && \
      (rspID[1] == (FLASH_DEVID_W25Q64 >> 8)) && \
      (rspID[2] == (FLASH_DEVID_W25Q64 & 0xFF))) {

    FILE *f;
    int i;
    unsigned int addr = 0;
    uint8_t buf[256];

    f = fopen(filename, "rb");
    if (!f) {
      printf("ERROR: %s: invalid file %s\n", __func__, filename);
      return EXIT_FAILURE;
    }
    if (!vscmIsFirmwareFile(f, __func__))
      return EXIT_FAILURE;

    memset(buf, 0xff, 256);
    while (fread(buf, 1, 256, f) > 0) {
      /* sector erase */
      if (!(addr % 65536)) {
        vscmSelectSpi(id, 1, mode);
        vscmTransferSpi(id, FLASH_CMD_WREN, mode); /* write enable */
        vscmSelectSpi(id, 0, mode);

        vscmSelectSpi(id, 1, mode);
        vscmTransferSpi(id, FLASH_CMD_ERASE64K, mode); /* 64k sector erase */
        vscmTransferSpi(id, ((addr >> 16) & 0xFF), mode);
        vscmTransferSpi(id, ((addr >> 8) & 0xFF), mode);
        vscmTransferSpi(id, (addr & 0xFF), mode);
        vscmSelectSpi(id, 0, mode);

        printf(".");
        fflush(stdout);
        i = 0;
        while (1) {
          if (!(vscmFlashGetStatus(id, mode) & 0x1))
            break;

          taskDelay(1);

          /* 1000ms maximum sector erase time */
          if (i == (60 + 6)) {
            fclose(f);
            printf("ERROR: %s: Failed to erase flash\n", __func__);
            return EXIT_FAILURE;
          }
          i++;
        }
      }

      vscmSelectSpi(id, 1, mode);
      vscmTransferSpi(id, FLASH_CMD_WREN, mode); /* write enable */
      vscmSelectSpi(id, 0, mode);

      vscmSelectSpi(id, 1, mode);
      vscmTransferSpi(id, FLASH_CMD_WRPAGE, mode); /* write page */
      vscmTransferSpi(id, ((addr >> 16) & 0xFF), mode);
      vscmTransferSpi(id, ((addr >> 8) & 0xFF), mode);
      vscmTransferSpi(id, (addr & 0xFF), mode);

      for (i = 0; i < 256; i++)
        vscmTransferSpi(id, buf[i], mode);
      vscmSelectSpi(id, 0, mode);

      i = 0;
      while (1) {
        if (!(vscmFlashGetStatus(id, mode) & 0x1)) /* no faster than 1us/call */
          break;
        /* 3ms maximum page program time */
        if (i == 3000) {
          fclose(f);
          printf("ERROR: %s: Failed to program flash\n", __func__);
          return EXIT_FAILURE;
        }
        i++;
      }
      memset(buf, 0xff, 256);
      addr += 256;
    }
    fclose(f);
  }
  else {
    printf("ERROR: %s: Failed to identify flash\n", __func__);
    return EXIT_FAILURE;
  }
  printf("done\n");
  return EXIT_SUCCESS;
}

int
vscmFirmwareVerify(int id, const char *filename)
{
  uint8_t rspID[3];
  int mode;

  if (vscmIsNotInit(&id, __func__))
    return EXIT_FAILURE;

  mode = vscmGetSpiMode(id);

  vscmSelectSpi(id, 0, mode);
  vscmFlashGetID(id, rspID, mode);
#ifdef DEBUG
  printf("Flash: Mfg=0x%02X, Type=0x%02X, Capacity=0x%02X\n", \
          rspID[0], rspID[1], rspID[2]);
#endif

  if ((rspID[0] == FLASH_MFG_WINBOND) && \
      (rspID[1] == (FLASH_DEVID_W25Q64 >> 8)) && \
      (rspID[2] == (FLASH_DEVID_W25Q64 & 0xFF))) {

    FILE *f;
    int i, len;
    unsigned int addr = 0;
    uint8_t buf[256], val;

    f = fopen(filename, "rb");
    if (!f) {
      printf("ERROR: %s: Invalid file %s\n", __func__, filename);
      return EXIT_FAILURE;
    }
    if (!vscmIsFirmwareFile(f, __func__))
      return EXIT_FAILURE;

    vscmSelectSpi(id, 1, mode);
    vscmTransferSpi(id, FLASH_CMD_RD, mode); /* continuous array read */
    vscmTransferSpi(id, ((addr >> 16) & 0xFF), mode);
    vscmTransferSpi(id, ((addr >> 8) & 0xFF), mode);
    vscmTransferSpi(id, (addr & 0xFF), mode);

    while ((len = fread(buf, 1, 256, f)) > 0) {
      for (i = 0; i < len; i++) {
        val = vscmTransferSpi(id, 0xFF, mode);
        if (buf[i] != val) {
          vscmSelectSpi(id, 0, mode);
          fclose(f);
          printf("ERROR: %s: Failed verify at addr 0x%08X[%02X,%02X]\n", \
                  __func__, addr+i, buf[i], val);
          return EXIT_FAILURE;
        }
      }
      addr += 256;
      if ((addr % 65536) == 0) {
        printf(".");
        fflush(stdout);

        taskDelay(1);
      }
    }
    vscmSelectSpi(id, 0, mode);
    fclose(f);
  }
  else {
    printf("ERROR: %s: Failed to identify flash\n", __func__);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int
vscmFirmwareUpdateVerify(int id, const char *filename)
{
  int result;

  if (vscmIsNotInit(&id, __func__))
    return EXIT_FAILURE;

  printf("Updating firmware...\n");
  result = vscmFirmwareUpdate(id, filename);
  if (result != 0) {
    printf("failed.\n");
    return result;
  }
  else
    printf("succeeded.\n");

  printf("Verifying...\n");
  result = vscmFirmwareVerify(id, filename);
  if (result != 0) {
    printf("failed.\n");
    return result;
  }
  else
    printf("ok.\n");

  vscmReloadFirmware(id);
		
  return EXIT_SUCCESS;
}

int
vscmFirmwareRead(int id, const char *filename)
{
  uint8_t rspID[3];
  int mode;

  if (vscmIsNotInit(&id, __func__))
    return EXIT_FAILURE;

  mode = vscmGetSpiMode(id);

  vscmSelectSpi(id, 0, mode);
  vscmFlashGetID(id, rspID, mode);
#ifdef DEBUG
  printf("Flash: Mfg=0x%02X, Type=0x%02X, Capacity=0x%02X\n", \
          rspID[0], rspID[1], rspID[2]);
#endif
  if ((rspID[0] == FLASH_MFG_WINBOND) && \
      (rspID[1] == (FLASH_DEVID_W25Q64 >> 8)) && \
      (rspID[2] == (FLASH_DEVID_W25Q64 & 0xFF))) {

    FILE *f;
    int i;
    unsigned int addr = 0;

    f = fopen(filename, "wb");
    if (!f) {
      printf("ERROR: %s: Invalid file %s\n", __func__, filename);
      return EXIT_FAILURE;
    } 

    vscmSelectSpi(id, 1, mode);
    vscmTransferSpi(id, FLASH_CMD_RD, mode); /* continuous array read */
    vscmTransferSpi(id, ((addr >> 16) & 0xFF), mode);
    vscmTransferSpi(id, ((addr >> 8) & 0xFF), mode);
    vscmTransferSpi(id, (addr & 0xFF), mode);
		
    for (i = 0; i < FLASH_BYTE_LENGTH; i++) {
      fputc(vscmTransferSpi(id, 0xFF, mode), f);
      if (!(i % 65536)) {
        printf(".");

        taskDelay(1);
      }
    }
    vscmSelectSpi(id, 0, mode);
    fclose(f);
  }
  else {
    printf("ERROR: %s: Failed to identify flash\n", __func__);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

void
vscmFirmware(char *filename, int slot)
{
  int nvscm;
  int ii, id;

  nvscm = vscmInit((unsigned int)(3<<19),(1<<19),20,0x80000000);

  if(slot<0) /* do nothing */
  {
    ;
  }
  else if(slot==0) /* do all boards */
  {
    for(ii=0; ii<nvscm; ii++)
    {
      id = vscmSlot(ii);
      vscmFirmwareUpdateVerify(id, filename);
    }
  }
  else /* do one board */
  {
    vscmFirmwareUpdateVerify(slot, filename);
  }

  return;
}

#else /* dummy version*/

void
vscmFirmware_dummy()
{
  return;
}

#endif

