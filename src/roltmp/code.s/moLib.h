#ifndef __MOLIBH__
#define __MOLIBH__
/*----------------------------------------------------------------------------*/
/**
 * @mainpage
 * <pre>
 *  Copyright (c) 2015        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Authors: Bryan Moffit                                                   *
 *             moffit@jlab.org                   Jefferson Lab, MS-12B3       *
 *             Phone: (757) 269-5660             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *     Header for the Driver library for the Master Oscillator
 *     distribution module.
 * </pre>
 *----------------------------------------------------------------------------*/

#define MO_SUPPORTED_VERSION_V1    0x04
#define MO_SUPPORTED_VERSION_V2    0x01
#define MO_ID                    0x1000


struct mo_struct
{
  /* 0x00 */ volatile uint32_t version;     /**< Version */
  /* 0x04 */ volatile uint32_t csr;         /**< Control/Status (CSR) */
  /* 0x08 */ volatile uint32_t ps0_ctrl;    /**< Prescale Control */
  /* 0x0C */ volatile uint32_t div_ctrl;    /**< Divier Control */
  /* 0x10 */ volatile uint32_t div_read[2]; /**< Divider Read (2) */
  /* 0x18 */ volatile uint32_t test;        /**< Test */
};

/* 0x00 version masks */
#define MO_VERSION_ID_MASK       0xFFFF0000
#define MO_VERSION_BOARDREV_MASK 0x0000FF00
#define MO_VERSION_FWREV_MASK    0x000000FF

/* 0x04 csr bits and masks */
#define MO_CSR_HARD_RESET                  (1<<31)
#define MO_CSR_SOFT_RESET                  (1<<30)
#define MO_CSR_RESET_INITIAL_PRESCALE_CHIP (1<<29)

/* 0x08 ps0_ctrl bits and masks */
#define MO_PS0_CTRL_INITIAL_PRESCALE_MASK  0x00000003

/* 0x0C div_ctrl bits and masks */
#define MO_DIV_CTRL_DIV1_SELECT  (1<<31)
#define MO_DIV_CTRL_DIV0_SELECT  (1<<30)
#define MO_DIV_CTRL_READ_SER_DATA (1<<23)
#define MO_DIV_CTRL_DIV_ADDR_MASK 0x00007F00
#define MO_DIV_CTRL_DATA_MASK     0x000000FF

/* 0x10 div_read[2] bits and masks */
#define MO_DIV_READ_DATA_MASK  0x000000FF

/* Initialization Flags */
#define MO_INIT_NOINIT      (1<<0)
#define MO_INIT_NOFWCHECK   (1<<1)


/* Function prototypes */

#ifndef INT16
#define INT16  short
#endif

#ifndef UINT16
#define UINT16 unsigned short
#endif

#ifndef INT32
#define INT32  int
#endif

#ifndef UINT32
#define UINT32 unsigned int
#endif

#ifndef STATUS
#define STATUS int
#endif

int  moInit(uint32_t tAddr, uint32_t iFlag);
int  moConfigCommon(int divider, int duty_mode);
int  moSyncDividers();
int  moReset();
int  moResetDividers();
int  moSetupClocks();
int  moConfigOutput(int output, int divider, int duty_mode);
int  moConfigPrint();
int  moConfigPS0(int ps0);
int  moSetPrescale(uint32_t channel, uint32_t prescale);
int  moGetPrescale(uint32_t channel, uint32_t *prescale);
int  moSetDutyMode(uint32_t channel, int duty_mode);
int  moGetDutyMode(uint32_t channel, uint32_t *duty_mode);
int  moGetDutyCycle(uint32_t channel, float *duty_cycle);
int  moSetInitialPrescale(uint32_t prescale);
int  moGetInitialPrescale(uint32_t *prescale);
int  moTestAccess();
#endif /* __MOLIBH__ */
