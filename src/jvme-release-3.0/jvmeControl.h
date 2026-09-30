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
 *     Header for JLab VME user interface to provide control and get
 *     status of the VME Bridge.
 *
 *----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdint.h>

int32_t jvmeMapControl();
int32_t jvmeUnmapControl();
int32_t jvmeGetBridgePointer(void **localPtr);
int32_t jvmeSysReset();
int32_t jvmeBERRIrqStatus();
int32_t jvmeSetBERRIrq(int32_t pflag, int32_t enable);
int32_t jvmeSetA24AM(int32_t addr_mod);
int32_t jvmeMemProbe(char *addr, int32_t size, char *rval);
int32_t jvmeClearException(int32_t pflag);
int32_t jvmeBridgeInit();
int32_t jvmeBridgeClose();
int32_t jvmeDmaConfig(uint32_t addrType, uint32_t dataType, uint32_t sstMode);
int32_t jvmeDmaSend(u_long locAdrs, uint32_t vmeAdrs, int32_t size);
int32_t jvmeDmaSendPhys(u_long physAdrs, uint32_t vmeAdrs, int32_t size);
int32_t jvmeDmaDone();
int32_t jvmeDmaBerrStatus();
int32_t jvmeDmaSetupLL(u_long locAddrBase,uint32_t *vmeAddr,
		       uint32_t *dmaSize,uint32_t numt);
int32_t jvmeDmaSendLL();

int32_t jvmeDmaAllocLLBuffer();
int32_t jvmeDmaFreeLLBuffer();

int32_t jvmeIntConnect(uint32_t vector, uint32_t level, VOIDFUNCPTR routine, uint32_t arg);
int32_t jvmeIntDisconnect(uint32_t level);
