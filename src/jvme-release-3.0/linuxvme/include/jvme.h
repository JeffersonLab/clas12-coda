#pragma once
/*----------------------------------------------------------------------------*
 *  Copyright (c) 2010        Southeastern Universities Research Association, *
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
 *     A front for the stuff that actually does the work.
 *      APIs are switched from the Makefile
 *
 *----------------------------------------------------------------------------*/


#ifndef ARCH_armv71

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <byteswap.h>
#include "dmaPList.h"

/** \name Defines and typedefs
   Several definitions and typedefs that provide some compatibility
   with those used in vxWorks
*/
/* \{ */
#define INT16  short
#define UINT16 unsigned short
#define INT32  int
#define UINT32 unsigned int
#define UINT64 unsigned long
#define STATUS int

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef OK
#define OK    0
#endif

#ifndef ERROR
#define ERROR (-1)
#endif

#define LOCAL
#ifndef _ROLDEFINED
typedef void            (*VOIDFUNCPTR) ();
typedef int             (*FUNCPTR) ();
#endif
typedef char            BOOL;


/* Routine prototypes */
int32_t taskDelay(int32_t ticks);
int32_t logMsg(const char *format, ...);
unsigned long long int rdtsc(void);

/**
    \hideinitializer
    Macro to perform a byte swapping for a 32 bit integer
*/
#ifndef LSWAP
#define LSWAP(x) bswap_32(x)
#endif

/**
    \hideinitializer
    Macro to perform a byte swapping for a 16 bit integer
*/
#ifndef SSWAP
#define SSWAP(x) bswap_16(x)
#endif
/* \} */

#define VME_ADDR_MOD_A16U		0x29	/* A16 nonprivileged access */
#define VME_ADDR_MOD_A24UD		0x39	/* A24 nonpriv data access */
#define VME_ADDR_MOD_A32UD		0x09	/* A32 nonpriv data access */
#define VME_ADDR_MOD_CR_CSR             0x2F	/* Conf ROM/Ctl&stat reg acc */


void vmeSetQuietFlag(uint32_t pflag);
void vmeSetVMEDebugMode(int32_t enable);
int32_t  vmeSetVMEDebugModeOutputFilename(char *fOutput);
int32_t  vmeSetVMEDebugModeOutput(int32_t *fOutput);
int32_t  vmeSetA32BltWindowWidth(uint32_t size);
int32_t  vmeOpen();
int32_t  vmeOpenDefaultWindows();
int32_t  vmeClose();
int32_t  vmeCloseDefaultWindows();
int32_t  vmeOpenSlaveA32(uint32_t base, uint32_t size);
int32_t  vmeCloseA32Slave();
uint32_t vmeReadRegister(uint32_t offset);
int32_t  vmeWriteRegister(uint32_t offset, uint32_t buffer);
int32_t  vmeSysReset();
int32_t  vmeBERRIrqStatus();
int32_t  vmeDisableBERRIrq(int32_t pflag);
int32_t  vmeEnableBERRIrq(int32_t pflag);
int32_t  vmeMemProbe(char *addr, int32_t size, char *rval);
int32_t  vmeClearException(int32_t pflag);
int32_t  vmeIntConnect(uint32_t vector, uint32_t level, VOIDFUNCPTR routine, uint32_t arg);
int32_t  vmeIntDisconnect(uint32_t level);
int32_t  vmeBusToLocalAdrs(int32_t vmeAdrsSpace, char *vmeBusAdrs, char **pLocalAdrs);
int32_t  vmeLocalToVmeAdrs(u_long localAdrs, uint32_t *vmeAdrs, uint16_t *amCode);
int32_t  vmeSetDebugFlags(int32_t flags);
int32_t  vmeSetA24AM(int32_t addr_mod);
int32_t  vmeDmaConfig(uint32_t addrType, uint32_t dataType, uint32_t sstMode);
int32_t  vmeDmaSend(u_long locAdrs, uint32_t vmeAdrs, int32_t size);
int32_t  vmeDmaSendPhys(u_long physAdrs, uint32_t vmeAdrs, int32_t size);
int32_t  vmeDmaDone();
int32_t  vmeDmaFlush(uint32_t vmeaddr);
int32_t  vmeDmaAllocLLBuffer();
int32_t  vmeDmaFreeLLBuffer();
int32_t  vmeDmaSetupLL(u_long locAddrBase,uint32_t *vmeAddr,
		       uint32_t *dmaSize,uint32_t numt);
int32_t  vmeDmaSendLL();
u_long vmeDmaLocalToPhysAdrs(u_long locAdrs);
uint32_t  vmeDmaLocalToVmeAdrs(u_long locAdrs);
void vmeReadDMARegs();

int32_t  vmeBusCreateLockShm();
int32_t  vmeBusKillLockShm(int32_t kflag);
int32_t  vmeBusLock();
int32_t  vmeBusTryLock();
int32_t  vmeBusTimedLock(int32_t time_seconds);
int32_t  vmeBusUnlock();
int32_t  vmeCheckMutexHealth(int32_t time_seconds);

int32_t  vmeSetMaximumVMESlots(int32_t slots);
int32_t  vxsPayloadPort2vmeSlot(int32_t payloadport);
uint32_t vxsPayloadPortMask2vmeSlotMask(uint32_t ppmask);
int32_t  vmeSlot2vxsPayloadPort(int32_t vmeslot);
uint32_t vmeSlotMask2vxsPayloadPortMask(uint32_t vmemask);

uint8_t vmeRead8(volatile uint8_t *addr);
uint16_t vmeRead16(volatile uint16_t *addr);
uint32_t vmeRead32(volatile uint32_t *addr);
void vmeWrite8(volatile uint8_t *addr, uint8_t val);
void vmeWrite16(volatile uint16_t *addr, uint16_t val);
void vmeWrite32(volatile uint32_t *addr, uint32_t val);

uint8_t  vmeBusRead8(int32_t amcode, uint32_t vmeaddr);
uint16_t vmeBusRead16(int32_t amcode, uint32_t vmeaddr);
uint32_t vmeBusRead32(int32_t amcode, uint32_t vmeaddr);
void vmeBusWrite8(int32_t amcode, uint32_t vmeaddr, uint8_t val);
void vmeBusWrite16(int32_t amcode, uint32_t vmeaddr, uint16_t val);
void vmeBusWrite32(int32_t amcode, uint32_t vmeaddr, uint32_t val);

#else /* ARCH_armv71 */
int32_t  vmeSetDebugFlags(int32_t flags) {return 0;};
int32_t  vmeOpenDefaultWindows() {return 0;};
int32_t  vmeCloseDefaultWindows() {return 0;};
#endif /* ARCH_armv71 */
