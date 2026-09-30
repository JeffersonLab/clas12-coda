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
 *     Header for Routines specific to the tsi148 VME Bridge
 *
 *----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdint.h>

int32_t  jvmeTsi148Init();
int32_t  jvmeTsi148Close();
int32_t  jvmeTsi148UpdateA32SlaveWindow(int32_t window, u_long *base);
int32_t  jvmeTsi148ResetA32SlaveWindow(int32_t window);
int32_t  jvmeTsi148GetSysReset();
int32_t  jvmeTsi148SysReset();
int32_t  jvmeTsi148GetBERRIrq();
int32_t  jvmeTsi148SetBERRIrq(int32_t enable);
int32_t  jvmeTsi148ClearException(int32_t pflag);
int32_t  jvmeTsi148ClearBERR();
int32_t  jvmeTsi148SetA24AM(int32_t addr_mod);
int32_t  jvmeTsi148DmaConfig(uint32_t addrType, uint32_t dataType, uint32_t sstMode);
int32_t  jvmeTsi148DmaSend(u_long locAdrs, uint32_t vmeAdrs, int32_t size);
int32_t  jvmeTsi148DmaSendPhys(u_long physAdrs, uint32_t vmeAdrs, int32_t size);
int32_t  jvmeTsi148DmaDone();
int32_t  jvmeTsi148GetBerrStatus();
int32_t  jvmeTsi148SetupDmaLLBuffer(u_long vaddr, u_long paddr);
int32_t  jvmeTsi148DmaSetupLL(u_long locAddrBase, uint32_t *vmeAddr,
			      uint32_t *dmaSize, uint32_t numt);
int32_t  jvmeTsi148DmaSendLL();
void jvmeTsi148ReadDMARegs();
void jvmeTsi148ReadMasterWindowRegs();
