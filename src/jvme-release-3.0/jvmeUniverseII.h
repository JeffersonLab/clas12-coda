#pragma once
/*----------------------------------------------------------------------------*
 *  Copyright (c) 2020        Southeastern Universities Research Association, *
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
 *     Header for Routines specific to the Unverse II VME Bridge
 *
 *----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdint.h>

int32_t  jvmeUnivInit();
int32_t  jvmeUnivClose();
int32_t  jvmeUnivUpdateA32SlaveWindow(int32_t window, u_long *base);
int32_t  jvmeUnivGetSysReset();
int32_t  jvmeUnivSysReset();
int32_t  jvmeUnivGetBERRIrq();
int32_t  jvmeUnivSetBERRIrq(int32_t enable);
int32_t  jvmeUnivClearException(int32_t pflag);
int32_t  jvmeUnivDmaReset(int32_t pflag);

int32_t  jvmeUnivSetA24AM(int32_t addr_mod);
int32_t  jvmeUnivDmaConfig(uint32_t addrType, uint32_t dataType);
int32_t  jvmeUnivDmaSend(u_long locAdrs, uint32_t vmeAdrs, int32_t size);
int32_t  jvmeUnivDmaSendPhys(u_long physAdrs, uint32_t vmeAdrs, int32_t size);
int32_t  jvmeUnivDmaDone(int32_t pcnt);
int32_t  jvmeUnivGetBerrStatus();
int32_t  jvmeUnivSetupDmaLLBuffer(u_long vaddr, u_long paddr);
int32_t  jvmeUnivDmaSetupLL(u_long locAddrBase, uint32_t *vmeAddr,
			  uint32_t *dmaSize, uint32_t numt);
int32_t  jvmeUnivDmaSendLL();
void jvmeUnivReadDMARegs();
