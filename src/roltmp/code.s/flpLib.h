/*----------------------------------------------------------------------------*
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
 *     Software driver library for the JLab Fast LED Pulser.
 *
 *----------------------------------------------------------------------------*/
#ifndef FLPLIB_H
#define FLPLIB_H

#define FLP_SUPPORTED_FIRMWARE 0x03
#define FLP_MAX_BOARDS         20


struct FLP_A24RegStruct
{
  /** 0x00 */ volatile unsigned short config[11];
  /** 0x16 */ volatile unsigned short status[13];
};

struct FLP_V_Regs
{
  /** 0x00 */ volatile unsigned short OUT1[3];
  /** 0x06 */ volatile unsigned short OUT2[3];
};

struct FLP_regs
{
  /** 0x00 */ struct FLP_V_Regs write;
  /** 0x0C */ volatile unsigned short pulser1_ctrl[2];
  /** 0x10 */ volatile unsigned short pulser2_ctrl[2];
  /** 0x14 */ volatile unsigned short ctrl;
  /** 0x16 */ struct FLP_V_Regs read;
  /** 0x22 */ volatile unsigned short V_ADC[3];
  /** 0x28 */ volatile unsigned short fw_version;
  /** 0x2A */ volatile unsigned short pulser1_freq;
  /** 0x2C */ volatile unsigned short pulser2_freq;
  /** 0x2E */ volatile unsigned short ethernet;
};

enum FLP_VOLTAGE_TYPES
  {
    FLP_V1,
    FLP_V2,
    FLP_VLED
  };

enum FLP_SOURCE_TYPES
  {
    FLP_EXT,
    FLP_INT
  };

enum FLP_OUT_TYPES
  {
    FLP_OUT1,
    FLP_OUT2
  };

enum FLP_IN_TYPES
  {
    FLP_IN1,
    FLP_IN2
  };

/* (0x0) write.OUT1_V1 bits */
#define FLP_VME_MODE_ENABLE   (1<<15)

/* Voltage regs masks */
#define FLP_VOLT_MASK  0x1FF
#define FLP_VOLT_READ  (1<<9)

/* (0x14) ctrl bits */
#define FLP_CTRL_OUT1_ENABLE         (1<<0)
#define FLP_CTRL_OUT2_ENABLE         (1<<1)
#define FLP_CTRL_PULSER1_LED_ENABLE  (1<<2)

/* (0x16) read.J{4,3}[] bits */
#define FLP_VOLT_MASK       0x1FF
#define FLP_VOLT_DATA_VALID (1<<14)
#define FLP_VOLT_READY      (1<<15)

/* pulser#_ctrl Masks and bits */
#define FLP_PULSER_CTRL_ENABLE      (1<<14)
#define FLP_PULSER_CTRL_EXT_ENABLE  (1<<15)
#define FLP_PULSER_CTRL_HIBITS_MASK  0x3800

/* (0x28) fw_version bits and masks */
#define FLP_FW_VERSION_MASK       0x00FF
#define FLP_FW_VERSION_ID_MASK    0xFF00
#define FLP_ID                    0x4000

#define FLP_INIT_NO_INIT              (1<<0)
#define FLP_INIT_SKIP_FIRMWARE_CHECK  (1<<1)

/* Function Prototypes */
int  flpInit(unsigned int tAddr, int iFlag);
int  flpStatus(int rflag);
int  flpSetOutput(int output, int v1, int v2, int vled);
int  flpEnableOutput(int output);
int  flpDisableOutput(int output);
int  flpGetOutputStatus(int output);
int  flpSetOutputVoltages(int output, float V1, float V2, float VLED);
int  flpGetOutputVoltages(int output, float *V1, float *V2, float *VLED);
int  flpSetPulserPeriod(int output, int period);
int  flpGetPulserPeriod(int output, unsigned int *period);
int  flpGetExternalFrequency(int input, float *freq);
int  flpSetPulserSource(int output, int src);
int  flpEnableIntPulser(int output);
int  flpDisableIntPulser(int output);

/* sergey */
int  flpGetNflp();
int  flpSlot(unsigned int id);

#endif /* FLPLIB_H */
