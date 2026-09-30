#pragma once
/*----------------------------------------------------------------------------*
 *  Copyright (c) 2018        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Bryan Moffit                                                   *
 *             moffit@jlab.org                   Jefferson Lab, MS-12B3       *
 *             Phone: (757) 269-5660             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *     Header for JLab VME user interface to provide access to VME windows.
 *
 *----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdint.h>

enum JVME_WINDOW_ENUM
  {
    JVME_A16 = 0,
    JVME_A24 = 1,
    JVME_A32 = 2,
    JVME_CRCSR = 3,
  };

int32_t  jvmeOpenMasterWindow(int32_t am, void **localPtr);
int32_t  jvmeOpenSlaveWindow(int32_t am, void **localPtr);
int32_t  jvmeCloseMasterWindow(int32_t am);
int32_t  jvmeCloseSlaveWindow(int32_t am);

int32_t  jvmeOpenMasterWindows(int32_t windowMask);
int32_t  jvmeOpenSlaveWindows(int32_t windowMask);
int32_t  jvmeOpenDefaultWindows();

int32_t  jvmeCloseMasterWindows(int32_t windowMask);
int32_t  jvmeCloseSlaveWindows(int32_t windowMask);
int32_t  jvmeCloseDefaultWindows();

int32_t  jvmePrintMasterWindow(int32_t am);
int32_t  jvmePrintSlaveWindow(int32_t am);

int32_t  jvmeBusToLocalAdrs(int32_t vmeAdrsSpace,
			    char *vmeBusAdrs,
			    char **pPciAdrs);
int32_t  jvmeLocalToVmeAdrs(u_long localAdrs, uint32_t *vmeAdrs,
			    uint16_t *amCode);

int32_t  jvmeBusBlockRead32(int32_t amcode, uint32_t addr, int32_t nwords, uint32_t *buf);
uint32_t jvmeBusRead32(int32_t amcode, uint32_t addr);
int32_t  jvmeBusBlockWrite32(int32_t amcode, uint32_t addr, int32_t nwords, uint32_t *buf);
void     jvmeBusWrite32(int32_t amcode, uint32_t addr, uint32_t wval);

int32_t  jvmeBusBlockRead16(int32_t amcode, uint32_t addr, int32_t nwords, uint16_t *buf);
uint16_t jvmeBusRead16(int32_t amcode, uint32_t addr);
int32_t  jvmeBusBlockWrite16(int32_t amcode, uint32_t addr, int32_t nwords, uint16_t *buf);
void     jvmeBusWrite16(int32_t amcode, uint32_t addr, uint16_t wval);

int32_t  jvmeBusBlockRead8(int32_t amcode, uint32_t addr, int32_t nwords, uint8_t *buf);
uint8_t  jvmeBusRead8(int32_t amcode, uint32_t addr);
int32_t  jvmeBusBlockWrite8(int32_t amcode, uint32_t addr, int32_t nwords, uint8_t *buf);
void     jvmeBusWrite8(int32_t amcode, uint32_t addr, uint8_t wval);
