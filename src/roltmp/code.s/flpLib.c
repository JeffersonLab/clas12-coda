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
 *     Software driver library for the JLab Fast LED Pulser.
 *
 * </pre>
 *----------------------------------------------------------------------------*/

#if defined(VXWORKS) || defined(Linux_vme)

#define _GNU_SOURCE

#ifdef VXWORKS
#include <vxWorks.h>
#include <sysLib.h>
#include <logLib.h>
#include <vxLib.h>
#include "jvme.h"
#else 
#include <unistd.h>
#include "jvme.h"
#endif
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "flpLib.h"

/* Mutex to guard FLP read/writes */
pthread_mutex_t   flpMutex = PTHREAD_MUTEX_INITIALIZER;
#define FLOCK     if(pthread_mutex_lock(&flpMutex)<0) perror("pthread_mutex_lock");
#define FUNLOCK   if(pthread_mutex_unlock(&flpMutex)<0) perror("pthread_mutex_unlock");

/* Global Variables */
volatile struct FLP_regs  *FLPp=NULL;    /* pointer to FLP memory map */
unsigned long flpA24Offset=0;                    /* Difference in CPU A24 Base and VME A24 Base */
unsigned short flpVersion=0;
static int flpID[FLP_MAX_BOARDS+1];                       /* array of slot numbers for FLPs */

static int Nflp = 0;                        /* Number of FLPs in Crate */

#ifdef VXWORKS
extern  int sysBusToLocalAdrs(int, char *, char **);
#endif

/* Map Index is V1=0, V2=1, VLED=2, Value is register index */
int Jmap[2][3] = 
  {
    {1, 0, 2}, // OUT1
    {0, 2, 1}  // OUT2
  };

/* sergey: returns some globals */
int
flpGetNflp()
{
  return(Nflp);
}

int
flpSlot(unsigned int id)
{
  if(id>=Nflp)
  {
    printf("%s: ERROR: Index (%d) >= FLPs initialized (%d).\n",__FUNCTION__,id,Nflp);
    return(ERROR);
  }

  return(flpID[0/*id*/]);
}

/* Calculate R3 for regulator voltage */
static float flpCalcR(float voltage)
{
  if(voltage<3.126) return 0;
  
  return ((voltage-3.126)/0.00089333);
}

static float flpCalcV(int type, float voltage)
{
  float rval=0;
  switch(type)
    {
    case FLP_V1:
    case FLP_V2:
      {
	rval = flpCalcR(voltage)/19.53125;
	break;
      }
    case FLP_VLED:
      {
	rval = flpCalcR(voltage)/39.0625;
	break;
      }
    default:
      rval=0;
    }

  return rval;
}

static float flpCalcVfromR(int type, int R)
{
  float rval;

  if(type==FLP_VLED)
    R *= 39.0625;
  else
    R *= 19.53125;

  rval=1.25*(1+((2.1e+3) + R)/(1.5e+3)) + (60e-6)*((2.1e+3) + R);
  return rval;
}

static int flpStatusWait(int output, int type, int valid)
{
  int iwait=0;
  unsigned short mask = FLP_VOLT_READY;

  if(FLPp == NULL) 
    {
      printf("%s: ERROR: FLP not initialized\n",__FUNCTION__);
      return ERROR;
    }

  if(valid)
    mask |= FLP_VOLT_DATA_VALID;

  while(iwait<100)
    {
      if(output==FLP_OUT1)
	{
	  if((vmeRead16(&FLPp->read.OUT1[Jmap[output][type]]) & mask)==mask)
	    break;
	}
      else /* output==FLP_OUT2 */
	{
	  if((vmeRead16(&FLPp->read.OUT2[Jmap[output][type]]) & mask)==mask)
	    break;
	}
      iwait++;
    }

  if(iwait==100)
    {
      printf("%s: ERROR: Timeout.\n",
	     __FUNCTION__);
      return ERROR;
    }
  
  return OK;
}

static int flpWriteMCP(int output, int type, unsigned short value)
{
  if(FLPp == NULL) 
    {
      printf("%s: ERROR: FLP not initialized\n",__FUNCTION__);
      return ERROR;
    }
  
  if(flpStatusWait(output, type, 0)==OK)
    {
      if(output==FLP_OUT1)
	{
	  vmeWrite16(&FLPp->write.OUT1[Jmap[output][type]], value | FLP_VME_MODE_ENABLE);
	}
      else /* output==FLP_OUT2 */
	{
	  vmeWrite16(&FLPp->write.OUT2[Jmap[output][type]], value);
	}
    }
  else
    {
      printf("%s: ERROR: Unable to write to %d - %d.\n",
	     __FUNCTION__,output, type);
      return ERROR;
    }

  return OK;
}

static int flpReadMCP(int output, int type, unsigned short *value)
{
  if(FLPp == NULL) 
    {
      printf("%s: ERROR: FLP not initialized\n",__FUNCTION__);
      return ERROR;
    }
  
  if(flpStatusWait(output, type, 1)==OK)
    {
      if(output==FLP_OUT1)
	{
	  vmeWrite16(&FLPp->write.OUT1[Jmap[output][type]], FLP_VOLT_READ | FLP_VME_MODE_ENABLE);
	  if(flpStatusWait(output, type, 1)==OK)
	    {
	      *value = vmeRead16(&FLPp->read.OUT1[Jmap[output][type]]);
	    }
	}
      else /* output==FLP_OUT2 */
	{
	  vmeWrite16(&FLPp->write.OUT2[Jmap[output][type]], FLP_VOLT_READ);
	  if(flpStatusWait(output, type, 1)==OK)
	    {
	      *value = vmeRead16(&FLPp->read.OUT2[Jmap[output][type]]);
	    }
	}
    }
  else
    {
      printf("%s: ERROR: Unable to Read from %d - %d.\n",
	     __FUNCTION__,output, type);
      return ERROR;
    }

  return OK;

}

/**
 * @defgroup Config Initialization/Configuration
 * @defgroup Status Status
 * @defgroup Deprec Deprecated - To be removed
 */

/**
 *  @ingroup Config
 *  @brief Initialize the FLP register space into local memory,
 *  and initialize registers
 *
 *  @param tAddr  Address
 *     - A24 VME Address of the TI
 *
 *  @param iFlag Initialization bit mask
 *     - 0   Do not initialize the board, just setup the pointers to the registers
 *     - 1   Ignore firmware check
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
flpInit(unsigned int tAddr, int iFlag)
{
  unsigned long laddr;
  unsigned short rval;
  int stat, ireg;
  int noBoardInit=0, noFirmwareCheck=0;
  int supportedVersion=0;  

  /* Check VME address */
  if(tAddr<0 || tAddr>0xffffff)
    {
      printf("%s: ERROR: Invalid VME Address (%d)\n",__FUNCTION__,
	     tAddr);
    }

  if(iFlag&FLP_INIT_NO_INIT)
    {
      noBoardInit = 1;
    }
  if(iFlag&FLP_INIT_SKIP_FIRMWARE_CHECK)
    {
      noFirmwareCheck=1;
    }


#ifdef VXWORKS
  stat = sysBusToLocalAdrs(0x39,(char *)tAddr,(char **)&laddr);
  if (stat != 0) 
    {
      printf("%s: ERROR: Error in sysBusToLocalAdrs res=%d \n",__FUNCTION__,stat);
      return ERROR;
    } 
  else 
    {
      printf("FLP address = 0x%lx\n",laddr);
    }
#else
  stat = vmeBusToLocalAdrs(0x39,(char *)(unsigned long)tAddr,(char **)&laddr);
  if (stat != 0) 
    {
      printf("%s: ERROR: Error in vmeBusToLocalAdrs res=%d \n",__FUNCTION__,stat);
      return ERROR;
    } 
  else 
    {
      if(!noBoardInit)
	printf("FLP VME (Local) address = 0x%.6x (0x%.8lx)\n",tAddr,laddr);
    }
#endif
  flpID[0/*id*/] = tAddr / 0x80000; /* sergey: temporary assuming 'standard' address convension */

  flpA24Offset = laddr-tAddr;

  /* Set Up pointer */
  FLPp = (struct FLP_regs *)laddr;

  /* Check if FLP board is readable */
#ifdef VXWORKS
  stat = vxMemProbe((char *)(&FLPp->fw_version),0,2,(char *)&rval);
#else
  stat = vmeMemProbe((char *)(&FLPp->fw_version),2,(char *)&rval);
#endif
  
  if (stat != 0) 
    {
      printf("%s: ERROR: FLP module not addressable\n",__FUNCTION__);
      FLPp=NULL;
      return(-1);
    }
  else
    {
      /* Check that it is a FLP */
      if((rval&FLP_FW_VERSION_ID_MASK) != FLP_ID) 
	{
	  printf("%s: ERROR: Invalid Board ID: 0x%x (rval = 0x%08x)\n",
		 __FUNCTION__,
		 (rval&FLP_FW_VERSION_ID_MASK),rval);
	  FLPp=NULL;
	  return(ERROR);
	}

      /* Get the Firmware Information and print out some details */
      supportedVersion = FLP_SUPPORTED_FIRMWARE;
      
      flpVersion = rval&FLP_FW_VERSION_MASK;
      printf("FLP ID: 0x%x \tFirmware version: 0x%X \n",
	     (rval&FLP_FW_VERSION_ID_MASK), flpVersion);
      
      if(flpVersion < supportedVersion)
	{
	  if(noFirmwareCheck)
	    {
	      printf("%s: WARN: Firmware version (0x%x) not supported by this driver.\n  Supported version = 0x%x  (IGNORED)\n",
		     __FUNCTION__,flpVersion,supportedVersion);
	    }
	  else
	    {
	      printf("%s: ERROR: Firmware version (0x%x) not supported by this driver.\n  Supported version = 0x%x\n",
		     __FUNCTION__,flpVersion,supportedVersion);
	      FLPp=NULL;
	      return ERROR;
	    }
	}
    }

  Nflp = 1; /* sergey: for now - one board */
  
  /* Check if we should exit here, or initialize some board defaults */
  if(noBoardInit)
    {
      return OK;
    }

  for(ireg=0; ireg<3; ireg++)
    {
      vmeWrite16(&FLPp->write.OUT1[ireg], FLP_VOLT_READ | FLP_VME_MODE_ENABLE);
      vmeWrite16(&FLPp->write.OUT2[ireg], FLP_VOLT_READ);
    }

  return OK;
}

/**
 *  @ingroup Status
 *  @brief Print status of the FLP to standard out
 *  @return OK if successful, otherwise ERROR.
 */

int
flpStatus(int rflag)
{
  struct FLP_regs fr;
  float freq_set[2], freq_meas[2];
  int freq_set_mult[2]={0,0};
  int freq_meas_mult[2]={0,0};
  int ireg=0;
  if(FLPp == NULL) 
    {
      printf("%s: ERROR: FLP not initialized\n",__FUNCTION__);
      return ERROR;
    }

  /* Latch the written values */
  FLOCK;
  for(ireg=0; ireg<3; ireg++)
    {
      flpReadMCP(FLP_OUT1, ireg, (unsigned short*)&fr.read.OUT1[ireg]);
      flpReadMCP(FLP_OUT2, ireg, (unsigned short*)&fr.read.OUT2[ireg]);
    }

  for(ireg=0; ireg<2; ireg++)
    {
      fr.pulser1_ctrl[ireg] = vmeRead16(&FLPp->pulser1_ctrl[ireg]);
      fr.pulser2_ctrl[ireg] = vmeRead16(&FLPp->pulser2_ctrl[ireg]);
    }
  freq_set[0] = 1.0/(20.0e-9*((float)((unsigned int)fr.pulser1_ctrl[0] | 
			       ((unsigned int)fr.pulser1_ctrl[1]&FLP_PULSER_CTRL_HIBITS_MASK)<<5)));
  freq_set[1] = 1.0/(20.0e-9*((float)((unsigned int)fr.pulser2_ctrl[0] | 
			       ((unsigned int)fr.pulser2_ctrl[1]&FLP_PULSER_CTRL_HIBITS_MASK)<<5)));

  fr.ctrl = vmeRead16(&FLPp->ctrl);
  for(ireg=0; ireg<3; ireg++)
    {
      fr.V_ADC[ireg] = vmeRead16(&FLPp->V_ADC[ireg]);
    }
  fr.fw_version = vmeRead16(&FLPp->fw_version);
  fr.pulser1_freq = vmeRead16(&FLPp->pulser1_freq);
  fr.pulser2_freq = vmeRead16(&FLPp->pulser2_freq);

  freq_meas[0] = 10.0*((float)fr.pulser1_freq);
  freq_meas[1] = 10.0*((float)fr.pulser2_freq);

  if(freq_set[0]>=1000.0)
    {
      freq_set_mult[0] = 1;
      freq_set[0] /= 1000.0;
      if(freq_set[0]>=1000.0)
	{
	  freq_set_mult[0] = 2;
	  freq_set[0] /= 1000.0;
	}
    }
  if(freq_set[1]>=1000.0)
    {
      freq_set_mult[1] = 1;
      freq_set[1] /= 1000.0;
      if(freq_set[1]>=1000.0)
	{
	  freq_set_mult[1] = 2;
	  freq_set[1] /= 1000.0;
	}
    }

  if(freq_meas[0]>=1000.0)
    {
      freq_meas_mult[0] = 1;
      freq_meas[0] /= 1000.0;
      if(freq_meas[0]>=1000.0)
	{
	  freq_meas_mult[0] = 2;
	  freq_meas[0] /= 1000.0;
	}
    }
  if(freq_meas[1]>=1000.0)
    {
      freq_meas_mult[1] = 1;
      freq_meas[1] /= 1000.0;
      if(freq_meas[1]>=1000.0)
	{
	  freq_meas_mult[1] = 2;
	  freq_meas[1] /= 1000.0;
	}
    }
  FUNLOCK;

  printf("\n");
#ifdef VXWORKS
  printf("STATUS for FLP at base address 0x%08x \n",
	 (unsigned int) FLPp);
#else
  printf("STATUS for FLP at VME (Local) base address 0x%06lx (0x%lx) \n",
	 (unsigned long) FLPp - flpA24Offset, (unsigned long) FLPp);
#endif

  printf("--------------------------------------------------------------------------------\n");
  printf(" Firmware Version = 0x%x\n",fr.fw_version & FLP_FW_VERSION_MASK);
  printf("\n");
  printf("                      OUT1                            OUT2\n");
  printf("              V1       V2      VLED           V1       V2      VLED \n");
  printf("--------------------------------------------------------------------------------\n");

  printf("Set          %.2f     %.2f     %.2f          %.2f     %.2f     %.2f\n",
	 (float)flpCalcVfromR(FLP_V1,fr.read.OUT1[FLP_V1]&FLP_VOLT_MASK),
	 (float)flpCalcVfromR(FLP_V2,fr.read.OUT1[FLP_V2]&FLP_VOLT_MASK),
	 (float)flpCalcVfromR(FLP_VLED,fr.read.OUT1[FLP_VLED]&FLP_VOLT_MASK),
	 (float)flpCalcVfromR(FLP_V1,fr.read.OUT2[FLP_V1]&FLP_VOLT_MASK),
	 (float)flpCalcVfromR(FLP_V2,fr.read.OUT2[FLP_V2]&FLP_VOLT_MASK),
	 (float)flpCalcVfromR(FLP_VLED,fr.read.OUT2[FLP_VLED]&FLP_VOLT_MASK)
	 );

  printf("Readback     %.2f     %.2f     %.2f          %.2f     %.2f     %.2f\n",
	 (float)((fr.V_ADC[0]&0xFF))*(5.5/256.0),
	 (float)((fr.V_ADC[0]&0xFF00)>>8)*(5.5/256.0),
	 (float)((fr.V_ADC[1]&0xFF00)>>8)*(8.0/256.0),
	 (float)((fr.V_ADC[1]&0xFF))*(5.5/256.0),
	 (float)((fr.V_ADC[2]&0xFF))*(5.5/256.0),
	 (float)((fr.V_ADC[2]&0xFF00)>>8)*(8.0/256.0)
	 );

  printf("\n\n");
  printf(" PULSER1    Internal    External         PULSER2    Internal    External\n");
  printf(" Selected   Frequency   Frequency        Selected   Frequency   Frequency\n");
  printf("--------------------------------------------------------------------------------\n");
  printf(" %s  %6.1f %s  %6.1f %s        %s  %6.1f %s  %6.1f %s\n",
	 (fr.pulser1_ctrl[1]&FLP_PULSER_CTRL_EXT_ENABLE) &&
	 (fr.pulser1_ctrl[1]&FLP_PULSER_CTRL_ENABLE)?"Both    ":
	 (fr.pulser1_ctrl[1]&FLP_PULSER_CTRL_EXT_ENABLE)?"External":
	 (fr.pulser1_ctrl[1]&FLP_PULSER_CTRL_ENABLE)?"Internal":"None    ",
	 (freq_set[0]),
	 (freq_set_mult[0]==2)?"MHz":
	 (freq_set_mult[0]==1)?"kHz":"Hz ",
	 (freq_meas[0]),
	 (freq_meas_mult[0])?"kHz":"Hz ",
	 (fr.pulser2_ctrl[1]&FLP_PULSER_CTRL_EXT_ENABLE) &&
	 (fr.pulser2_ctrl[1]&FLP_PULSER_CTRL_ENABLE)?"Both    ":
	 (fr.pulser2_ctrl[1]&FLP_PULSER_CTRL_EXT_ENABLE)?"External":
	 (fr.pulser2_ctrl[1]&FLP_PULSER_CTRL_ENABLE)?"Internal":"None    ",
	 (freq_set[1]),
	 (freq_set_mult[1]==2)?"MHz":
	 (freq_set_mult[1]==1)?"kHz":
	 "Hz ",
	 (freq_meas[1]),
	 (freq_meas_mult[1])?"kHz":"Hz "
	 );

  printf("\n\n");
  printf("--------------------------------------------------------------------------------\n\n");

  return OK;
}

/**
 *  @ingroup Config
 *  @brief Enable the specified output
 *  @param output The choosen output to enable
 *     FLP_OUT1: Enable the top DB9 output
 *     FLP_OUT2: Enable the bottom DB9 output
 *  @return OK if successful, otherwise ERROR.
 */

int
flpEnableOutput(int output)
{
  if(FLPp == NULL) 
    {
      printf("%s: ERROR: FLP not initialized\n",__FUNCTION__);
      return ERROR;
    }
  
  if((output!=FLP_OUT1) && (output!=FLP_OUT2))
    {
      printf("%s: ERROR: Invalid output (%d)\n",
	     __FUNCTION__,output);
      return ERROR;
    }

  FLOCK;
  if(output==FLP_OUT1)
    {
      vmeWrite16(&FLPp->ctrl,
		 vmeRead16(&FLPp->ctrl) | FLP_CTRL_OUT1_ENABLE);
    }
  else
    {
      vmeWrite16(&FLPp->ctrl,
		 vmeRead16(&FLPp->ctrl) | FLP_CTRL_OUT2_ENABLE);
    }
  FUNLOCK;

  return OK;
}

/**
 *  @ingroup Config
 *  @brief Disable the specified output
 *  @param output The choosen output to disable
 *     FLP_OUT1: Disable the top DB9 output
 *     FLP_OUT2: Disable the bottom DB9 output
 *  @return OK if successful, otherwise ERROR.
 */

int
flpDisableOutput(int output)
{
  if(FLPp == NULL) 
    {
      printf("%s: ERROR: FLP not initialized\n",__FUNCTION__);
      return ERROR;
    }
  
  if((output!=FLP_OUT1) && (output!=FLP_OUT2))
    {
      printf("%s: ERROR: Invalid output (%d)\n",
	     __FUNCTION__,output);
      return ERROR;
    }

  FLOCK;
  if(output==FLP_OUT1)
    {
      vmeWrite16(&FLPp->ctrl,
		 vmeRead16(&FLPp->ctrl) & ~FLP_CTRL_OUT1_ENABLE);
    }
  else
    {
      vmeWrite16(&FLPp->ctrl,
		 vmeRead16(&FLPp->ctrl) & ~FLP_CTRL_OUT2_ENABLE);
    }
  FUNLOCK;

  return OK;
}

/**
 *  @ingroup Status
 *  @brief Get output enable/disable status of the specified output
 *  @param output The choosen output to disable
 *     FLP_OUT1: Disable the top DB9 output
 *     FLP_OUT2: Disable the bottom DB9 output
 *  @return 1 if enabled, 0 if disabled, otherwise ERROR.
 */
int
flpGetOutputStatus(int output)
{
  int rval=0;
  if(FLPp == NULL) 
    {
      printf("%s: ERROR: FLP not initialized\n",__FUNCTION__);
      return ERROR;
    }
  
  if((output!=FLP_OUT1) && (output!=FLP_OUT2))
    {
      printf("%s: ERROR: Invalid output (%d)\n",
	     __FUNCTION__,output);
      return ERROR;
    }

  FLOCK;
  if(output==FLP_OUT1)
    {
      rval = (vmeRead16(&FLPp->ctrl) & FLP_CTRL_OUT1_ENABLE)?1:0;
    }
  else
    {
      rval = (vmeRead16(&FLPp->ctrl) & FLP_CTRL_OUT2_ENABLE)?1:0;
    }
  FUNLOCK;

  return rval;
}



/**
 *  @ingroup Config
 *  @brief Set the output voltages for the specified DB9 output
 *  @param output Output DB9 connector
 *     FLP_OUT1: the top DB9 output
 *     FLP_OUT2: the bottom DB9 output
 *  @param V1 Voltage for V1 (3.2-5.5V)
 *  @param V2 Voltage for V2 (3.2-5.5V)
 *  @param VLED Voltage for VLED (3.2-8.0V)
 *  @return OK if successful, otherwise ERROR.
 */

int
flpSetOutputVoltages(int output, float V1, float V2, float VLED)
{
  unsigned short wval=0;
  if(FLPp == NULL) 
    {
      printf("%s: ERROR: FLP not initialized\n",__FUNCTION__);
      return ERROR;
    }
  
  if((output!=FLP_OUT1) && (output!=FLP_OUT2))
    {
      printf("%s: ERROR: Invalid output (%d)\n",
	     __FUNCTION__,output);
      return ERROR;
    }

  if((V1<3.2) || (V1>5.5))
    {
      printf("%s: ERROR: Invalid V1 (%.1f)\n",__FUNCTION__,V1);
      return ERROR;
    }

  if((V2<3.2) || (V2>5.5))
    {
      printf("%s: ERROR: Invalid V2 (%.1f)\n",__FUNCTION__,V2);
      return ERROR;
    }

  if((VLED<3.2) || (VLED>8.0))
    {
      printf("%s: ERROR: Invalid VLED (%.1f)\n",__FUNCTION__,VLED);
      return ERROR;
    }


  FLOCK;
  wval = (unsigned short)flpCalcV(FLP_V1, V1);
  flpWriteMCP(output, FLP_V1, wval);
  wval = (unsigned short)flpCalcV(FLP_V2, V2);
  flpWriteMCP(output, FLP_V2, wval);
  wval = (unsigned short)flpCalcV(FLP_VLED, VLED);
  flpWriteMCP(output, FLP_VLED, wval);
  FUNLOCK;

  return OK;
}

/**
 *  @ingroup Status
 *  @brief Get the output voltages for the specified DB9 output
 *  @param output Output DB9 connector
 *     FLP_OUT1: the top DB9 output
 *     FLP_OUT2: the bottom DB9 output
 *  @param V1 Voltage for V1 (3.2-5.5V)
 *  @param V2 Voltage for V2 (3.2-5.5V)
 *  @param VLED Voltage for VLED (3.2-8.0V)
 *  @return OK if successful, otherwise ERROR.
 */

int
flpGetOutputVoltages(int output, float *V1, float *V2, float *VLED)
{
  int ireg=0;
  struct FLP_V_Regs fr;

  if(FLPp == NULL) 
    {
      printf("%s: ERROR: FLP not initialized\n",__FUNCTION__);
      return ERROR;
    }
  
  if((output!=FLP_OUT1) && (output!=FLP_OUT2))
    {
      printf("%s: ERROR: Invalid output (%d)\n",
	     __FUNCTION__,output);
      return ERROR;
    }

  FLOCK;




#if 0 /*sergey*/
  for(ireg=0; ireg<3; ireg++)
  {
    if(output==FLP_OUT1)      flpReadMCP(FLP_OUT1, ireg, (unsigned short*)&fr.OUT1[ireg]);
    else if(output==FLP_OUT2) flpReadMCP(FLP_OUT2, ireg, (unsigned short*)&fr.OUT2[ireg]);
  }
  *V1   = flpCalcVfromR(FLP_V1,fr.OUT1[FLP_V1]&FLP_VOLT_MASK);
  *V2   = flpCalcVfromR(FLP_V2,fr.OUT1[FLP_V2]&FLP_VOLT_MASK);
  *VLED = flpCalcVfromR(FLP_VLED,fr.OUT1[FLP_VLED]&FLP_VOLT_MASK);
#endif



  /* sergey: above does not work, do following */
  {
    struct FLP_regs fr;

    for(ireg=0; ireg<3; ireg++)
    {
      fr.V_ADC[ireg] = vmeRead16(&FLPp->V_ADC[ireg]);
    }
    if(output==FLP_OUT1)
	{
	  *V1   = (float)((fr.V_ADC[0]&0xFF))*(5.5/256.0);
	  *V2   = (float)((fr.V_ADC[0]&0xFF00)>>8)*(5.5/256.0);
	  *VLED = (float)((fr.V_ADC[1]&0xFF00)>>8)*(8.0/256.0);
	}
    else if(output==FLP_OUT2)
	{
	  *V1   = (float)((fr.V_ADC[1]&0xFF))*(5.5/256.0);
	  *V2   = (float)((fr.V_ADC[2]&0xFF))*(5.5/256.0);
	  *VLED = (float)((fr.V_ADC[2]&0xFF00)>>8)*(8.0/256.0);
    }
  }




  FUNLOCK;
  
  return OK;
}

/**
 *  @ingroup Config
 *  @brief Set the Period of the specified output DB9 connector
 *  @param output Output DB9 connector
 *     FLP_OUT1: the top DB9 output
 *     FLP_OUT2: the bottom DB9 output
 *  @param period Period in units of 20ns. (2500-524287)
 *  @return OK if successful, otherwise ERROR.
 */

int
flpSetPulserPeriod(int output, int period)
{
  if(FLPp == NULL) 
    {
      printf("%s: ERROR: FLP not initialized\n",__FUNCTION__);
      return ERROR;
    }
  
  if((output!=FLP_OUT1) && (output!=FLP_OUT2))
    {
      printf("%s: ERROR: Invalid output (%d)\n",
	     __FUNCTION__,output);
      return ERROR;
    }

  if((period<0)||(period>0x0007ffff))
    {
      printf("%s: ERROR: Invalid period (%d)\n",
	     __FUNCTION__,period);
      return ERROR;
    }

  if((period<2500)&(period>=0))
    {
      printf("%s: ERROR: Invalid period (%d).  Must be greater than 2500.\n",
	     __FUNCTION__,period);
      return ERROR;
    }

  FLOCK;
  switch(output)
    {
    case FLP_OUT1:
      {
	vmeWrite16(&FLPp->pulser1_ctrl[0],period&0xFFFF);
	vmeWrite16(&FLPp->pulser1_ctrl[1],
		   (vmeRead16(&FLPp->pulser1_ctrl[1]) &~ FLP_PULSER_CTRL_HIBITS_MASK) |
		   (period>>16)<<11);
	break;
      }
    case FLP_OUT2:
      {
	vmeWrite16(&FLPp->pulser2_ctrl[0],period&0xFFFF);
	vmeWrite16(&FLPp->pulser2_ctrl[1],
		   (vmeRead16(&FLPp->pulser2_ctrl[1]) &~ FLP_PULSER_CTRL_HIBITS_MASK) |
		   (period>>16)<<11);
	break;
      }
    }
      
  FUNLOCK;

  return OK;
}

/**
 *  @ingroup Status
 *  @brief Get the Period of the specified output DB9 connector
 *  @param output Output DB9 connector
 *     FLP_OUT1: the top DB9 output
 *     FLP_OUT2: the bottom DB9 output
 *  @param period Period in units of 20ns.
 *  @return OK if successful, otherwise ERROR.
 */

int
flpGetPulserPeriod(int output, unsigned int *period)
{
  if(FLPp == NULL) 
    {
      printf("%s: ERROR: FLP not initialized\n",__FUNCTION__);
      return ERROR;
    }
  
  if((output!=FLP_OUT1) && (output!=FLP_OUT2))
    {
      printf("%s: ERROR: Invalid output (%d)\n",
	     __FUNCTION__,output);
      return ERROR;
    }

  FLOCK;
  switch(output)
    {
    case FLP_OUT1:
      {
	*period = (unsigned int)vmeRead16(&FLPp->pulser1_ctrl[0]) | 
	  (((unsigned int)vmeRead16(&FLPp->pulser1_ctrl[1]) &~ FLP_PULSER_CTRL_HIBITS_MASK)<<5);
	break;
      }
    case FLP_OUT2:
      {
	*period = (unsigned int)vmeRead16(&FLPp->pulser2_ctrl[0]) | 
	  (((unsigned int)vmeRead16(&FLPp->pulser2_ctrl[1]) &~ FLP_PULSER_CTRL_HIBITS_MASK)<<5);
	break;
      }
    }
  FUNLOCK;

  *period = (*period) & 0xFFFF; /* sergey */

  return OK;
}

/**
 *  @ingroup Status
 *  @brief Get the Frequency of the specified external input
 *  @param input External Input
 *     FLP_IN1: Left external input
 *     FLP_IN2: Right external input
 *  @param freq Frequency of specified input in hertz
 *  @return OK if successful, otherwise ERROR.
 */

int
flpGetExternalFrequency(int input, float *freq)
{
  if(FLPp == NULL) 
    {
      printf("%s: ERROR: FLP not initialized\n",__FUNCTION__);
      return ERROR;
    }
  
  if((input!=FLP_OUT1) && (input!=FLP_OUT2))
    {
      printf("%s: ERROR: Invalid input (%d)\n",
	     __FUNCTION__,input);
      return ERROR;
    }

  FLOCK;
  switch(input)
    {
    case FLP_OUT1:
      {
	*freq = 10.0*((float)vmeRead16(&FLPp->pulser1_freq));
	break;
      }
    case FLP_OUT2:
      {
	*freq = 10.0*((float)vmeRead16(&FLPp->pulser2_freq));
	break;
      }
    }
  FUNLOCK;

  return OK;
}


/**
 *  @ingroup Config
 *  @brief Select the pulser source of the specified output
 *  @param output LEMO output
 *     FLP_OUT1: Left output
 *     FLP_OUT2: Right output
 *  @param src Pulser Source
 *     FLP_EXT: External source
 *     FLP_INT: Internal pulser
 *  @return OK if successful, otherwise ERROR.
 */

int
flpSetPulserSource(int output, int src)
{
  int extenable=0;
  if(FLPp == NULL) 
    {
      printf("%s: ERROR: FLP not initialized\n",__FUNCTION__);
      return ERROR;
    }
  
  if((output!=FLP_OUT1) && (output!=FLP_OUT2))
    {
      printf("%s: ERROR: Invalid output (%d)\n",
	     __FUNCTION__,output);
      return ERROR;
    }

  if((src!=FLP_EXT) && (src!=FLP_INT))
    {
      printf("%s: ERROR: Invalid src (%d)\n",
	     __FUNCTION__,src);
      return ERROR;
    }

  if(src==FLP_EXT)
    extenable=FLP_PULSER_CTRL_EXT_ENABLE;

  FLOCK;
  switch(output)
    {
    case FLP_OUT1:
      {
	vmeWrite16(&FLPp->pulser1_ctrl[1],
		   (vmeRead16(&FLPp->pulser1_ctrl[1]) &~ FLP_PULSER_CTRL_EXT_ENABLE) |
		    extenable);
	break;
      }
    case FLP_OUT2:
      {
	vmeWrite16(&FLPp->pulser2_ctrl[1],
		   (vmeRead16(&FLPp->pulser2_ctrl[1]) &~ FLP_PULSER_CTRL_EXT_ENABLE) |
		    extenable);
	break;
      }
    }
  FUNLOCK;
  
  return OK;
}

/**
 *  @ingroup Config
 *  @brief Enable the internal pulser source of the specified output
 *  @param output LEMO output
 *     FLP_OUT1: Left output
 *     FLP_OUT2: Right output
 *  @return OK if successful, otherwise ERROR.
 */

int
flpEnableIntPulser(int output)
{
  if(FLPp == NULL) 
    {
      printf("%s: ERROR: FLP not initialized\n",__FUNCTION__);
      return ERROR;
    }
  
  if((output!=FLP_OUT1) && (output!=FLP_OUT2))
    {
      printf("%s: ERROR: Invalid output (%d)\n",
	     __FUNCTION__,output);
      return ERROR;
    }

  FLOCK;
  switch(output)
    {
    case FLP_OUT1:
      {
	vmeWrite16(&FLPp->pulser1_ctrl[1],
		   (vmeRead16(&FLPp->pulser1_ctrl[1]) | FLP_PULSER_CTRL_ENABLE));
	break;
      }
    case FLP_OUT2:
      {
	vmeWrite16(&FLPp->pulser2_ctrl[1],
		   (vmeRead16(&FLPp->pulser2_ctrl[1]) | FLP_PULSER_CTRL_ENABLE));
	break;
      }
    }
  FUNLOCK;
  
  return OK;
}

/**
 *  @ingroup Config
 *  @brief Disable the internal pulser source of the specified output
 *  @param output LEMO output
 *     FLP_OUT1: Left output
 *     FLP_OUT2: Right output
 *  @return OK if successful, otherwise ERROR.
 */

int
flpDisableIntPulser(int output)
{
  if(FLPp == NULL) 
    {
      printf("%s: ERROR: FLP not initialized\n",__FUNCTION__);
      return ERROR;
    }
  
  if((output!=FLP_OUT1) && (output!=FLP_OUT2))
    {
      printf("%s: ERROR: Invalid output (%d)\n",
	     __FUNCTION__,output);
      return ERROR;
    }

  FLOCK;
  switch(output)
    {
    case FLP_OUT1:
      {
	vmeWrite16(&FLPp->pulser1_ctrl[1],
		   (vmeRead16(&FLPp->pulser1_ctrl[1]) & ~FLP_PULSER_CTRL_ENABLE));
	break;
      }
    case FLP_OUT2:
      {
	vmeWrite16(&FLPp->pulser2_ctrl[1],
		   (vmeRead16(&FLPp->pulser2_ctrl[1]) & ~FLP_PULSER_CTRL_ENABLE));
	break;
      }
    }
  FUNLOCK;
  
  return OK;
}

#else /* dummy version*/

void
flpLib_dummy()
{
  return;
}

#endif
