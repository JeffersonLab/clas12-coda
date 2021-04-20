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
 *     Driver library for the Master Oscillator distribution module.
 * </pre>
 *----------------------------------------------------------------------------*/

#if defined(VXWORKS) || defined(Linux_vme)

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>
#ifdef VXWORKS
#include <vxWorks.h>
#include <logLib.h>
#include <taskLib.h>
#include <intLib.h>
#include <iv.h>
#else
#include <stdint.h>
#include <stdlib.h>
#include "jvme.h"
#endif
#include "moLib.h"

#ifdef VXWORKS
IMPORT  STATUS sysBusToLocalAdrs(int, char *, char **);
#endif

/* Mutex to guard MO read/writes */
pthread_mutex_t   moMutex = PTHREAD_MUTEX_INITIALIZER;
#define MLOCK     if(pthread_mutex_lock(&moMutex)<0) perror("pthread_mutex_lock");
#define MUNLOCK   if(pthread_mutex_unlock(&moMutex)<0) perror("pthread_mutex_unlock");

/* Macro to check MOp */
#define CHECKMO {						\
    if(MOp == NULL) {						\
      logMsg("%s: ERROR : MO not initialized \n",		\
	     __FUNCTION__,2,3,4,5,6);			\
      return ERROR;						\
    }								\
  }

typedef struct mo_chan_struct
{
  uint32_t div;
  uint32_t dm;
} MO_CHAN;

/* Global Variables */
volatile struct mo_struct *MOp=NULL;   /* pointer to MO memory map */
unsigned long moA24Offset=0;                     /* Difference in CPU A24 Base and VME A24 Base */
int moDefaultPS_0=16;                  /* Default value set for first level prescale */
MO_CHAN moChan[10] =                   /* Default divider and duty mode to set during initialization: 2,0-presale=32, 4,0-prescale=64 */
  {
	/*
    { 2, 0 }, { 2, 0 }, { 2, 0 }, { 2, 0 }, { 2, 0 },
    { 2, 0 }, { 2, 0 }, { 2, 0 }, { 2, 0 }, { 2, 0 }
	*/
    { 4, 0 }, { 4, 0 }, { 4, 0 }, { 4, 0 }, { 4, 0 },
    { 4, 0 }, { 4, 0 }, { 4, 0 }, { 4, 0 }, { 4, 0 }
	
  };
uint8_t moBoardRev = 0;
uint8_t moFWRev = 0;
uint8_t moMaxChanNumber = 9;

/**
 * @defgroup Config Initialization/Configuration
 * @defgroup Status Status
 * @defgroup Deprec Deprecated - To be removed
 */

/**
 *  @ingroup Config
 *  @brief Initialize the MO register space into local memory,
 *  and perform initial default setup.
 *
 *  @param tAddr  VME A24 Address of MO
 *
 *  @param iFlag Initialization bit mask
 *     - 0   Do not initialize the board, just setup the pointers to the registers
 *     - 1   Ignore firmware check
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
moInit(uint32_t tAddr, uint32_t iFlag)
{
  int skipFW=0, skipInit=0, stat=0, ichan=0;
  uint32_t rval=0, supported = MO_SUPPORTED_VERSION_V1;
  unsigned long laddr=0;

  if(iFlag & MO_INIT_NOINIT)
    skipInit=1;
  if(iFlag & MO_INIT_NOFWCHECK)
    skipFW=1;



#ifdef VXWORKS
  stat = sysBusToLocalAdrs(0x39,(char *)tAddr,(char **)&laddr);
  if (stat != 0)
    {
      printf("flexioInit: ERROR: Error in sysBusToLocalAdrs res=%d \n",stat);
      return ERROR;
    }
#else
  stat = vmeBusToLocalAdrs(0x39,(char *)(unsigned long)tAddr,(char **)&laddr);
  if (stat != 0)
    {
      printf("flexioInit: ERROR: Error in vmeBusToLocalAdrs res=%d \n",stat);
      return ERROR;
    }
#endif
  moA24Offset = laddr - tAddr;

  MOp = (struct mo_struct *)laddr;

  /* Check if MO board is readable */
#ifdef VXWORKS
  stat = vxMemProbe((char *)(&MOp->version),0,4,(char *)&rval);
#else
  stat = vmeMemProbe((char *)(&MOp->version),4,(char *)&rval);
#endif
  if (stat != OK)
    {
      printf("%s: ERROR: MO module not addressable\n",__FUNCTION__);
      MOp=NULL;
      return ERROR;;
    }

  if(((rval & MO_VERSION_ID_MASK)>>16) != MO_ID)
    {
      printf("%s: ERROR: Invalid Board ID: 0x%x (rval = 0x%08x)\n",
	     __FUNCTION__, rval & MO_VERSION_ID_MASK, rval);
      MOp=NULL;
      return ERROR;
    }

  printf("%s: MO VME (Local) address = 0x%08x (0x%lx)\n",
	 __FUNCTION__,
	 tAddr, laddr);

  moFWRev    = rval & MO_VERSION_FWREV_MASK;
  moBoardRev = (rval & MO_VERSION_BOARDREV_MASK) >> 8;

  if(moBoardRev == 0x2)
    {
      moMaxChanNumber = 4;
      supported = MO_SUPPORTED_VERSION_V2;
    }

  /* Check firmware version */
  if(moFWRev!=supported)
    {
      if(skipFW)
	{
	  printf("%s: WARN: Firmware Version (0x%x) not supported by this driver.\n\tSupported Version: 0x%x.\n",
		 __FUNCTION__, moFWRev, supported);
	}
      else
	{
	  printf("%s: ERROR: Firmware Version (0x%x) not supported by this driver.\n\tSupported Version: 0x%x.\n",
		 __FUNCTION__, moFWRev, supported);
	  MOp=NULL;
	  return ERROR;
	}
    }

  if(skipInit)
    return OK;

  /* Do the powerup type initialization here */
  if(moReset()!=OK)
    {
      printf("%s: ERROR resetting MO.\n",
	     __FUNCTION__);
      return ERROR;
    }
  if(moSetupClocks()!=OK)
    {
      printf("%s: ERROR setting up divider clocks.\n",
	     __FUNCTION__);
      return ERROR;
    }

#if 1
  /* Set up default prescale factors */
  if(moConfigPS0(moDefaultPS_0)!=OK)
    {
      printf("%s: ERROR setting first level prescale\n",
	     __FUNCTION__);
      return ERROR;
    }

  for(ichan=0;ichan<(moMaxChanNumber+1);ichan++)
    {
      if(moConfigOutput(ichan, moChan[ichan].div, moChan[ichan].dm)!=OK)
	{
	  printf("%s: ERROR setting channel %2d. divider = %2d, duty_mode = %d.\n",
		 __FUNCTION__,ichan, moChan[ichan].div, moChan[ichan].dm);
	}
    }
#endif
  /* Sync dividers */
  if(moSyncDividers()!=OK)
    {
      printf("%s: ERROR synchronizing dividers.\n",
	     __FUNCTION__);
      return ERROR;
    }


  return OK;
}

/**
 *  @ingroup Config
 *  @brief Configures all output channels with common divider value 'divider' and
 *         duty cycle parameter 'duty_mode'
 *
 *  @param divider Divider Value
 *
 *  @param duty_mode Duty cycle mode
 *       - 0: duty cycle closest to 50% achievable for given divider
 *       - 1: minimum duty cycle achievable for given divider
 *       - 2: maximum duty cycle achievable for given divider
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
moConfigCommon(int divider, int duty_mode)
{

  int ichan;

  CHECKMO;

  if( (divider < 1) || (divider > 32) )
    {
      printf("%s: Invalid divider value (%d)\n",
	     __FUNCTION__,divider);
      return ERROR;
    }
  if( (duty_mode < 0) || (duty_mode > 2) )
    {
      printf("%s: Invalid duty_mode value (%d)\n",
	     __FUNCTION__,duty_mode);
      return ERROR;
    }

  if(moReset()!=OK)
    {
      printf("%s: ERROR resetting MO.\n",
	     __FUNCTION__);
      return ERROR;
    }
  if(moSetupClocks()!=OK)
    {
      printf("%s: ERROR setting up divider clocks.\n",
	     __FUNCTION__);
      return ERROR;
    }

  for(ichan=0;ichan<(moMaxChanNumber+1);ichan++)
    {
      if(moConfigOutput(ichan, divider, duty_mode)!=OK)
	{
	  printf("%s: ERROR setting channel %2d. divider = %2d, duty_mode = %d.\n",
		 __FUNCTION__,ichan, divider, duty_mode);
	}
    }

  if(moSyncDividers()!=OK)
    {
      printf("%s: ERROR synchronizing dividers.\n",
	     __FUNCTION__);
      return ERROR;
    }

  return OK;

}

/**
 *  @ingroup Config
 *  @brief Synchronize all dividers
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
moSyncDividers()
{
  uint32_t divsel = 0;

  CHECKMO;

  divsel = MO_DIV_CTRL_DIV0_SELECT | MO_DIV_CTRL_DIV1_SELECT;

  MLOCK;
  vmeWrite32(&MOp->div_ctrl, divsel | 0x5804);	// sync all dividers (must toggle bit)
  vmeWrite32(&MOp->div_ctrl, divsel | 0x5A01);	// update register
  vmeWrite32(&MOp->div_ctrl, divsel | 0x5800);
  vmeWrite32(&MOp->div_ctrl, divsel | 0x5A01);	// update register
  MUNLOCK;
  return OK;
}

/**
 *  @ingroup Config
 *
 *  @brief Reset Module to power-up state.
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
moReset()
{
  uint32_t divsel=0;

  CHECKMO;

  MLOCK;
  vmeWrite32(&MOp->csr, MO_CSR_HARD_RESET);		// hard reset module

  divsel = MO_DIV_CTRL_DIV0_SELECT | MO_DIV_CTRL_DIV1_SELECT;
  vmeWrite32(&MOp->div_ctrl, divsel | 0x0030);	// soft reset both divider chips (must toggle bit)
  vmeWrite32(&MOp->div_ctrl, divsel | 0x0010);	// (DON'T need to update register bits for addr=00!)
  MUNLOCK;
  return OK;
}

/**
 *  @ingroup Config
 *
 *  @brief Reset divider chips to power-up state
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
moResetDividers()
{
  uint32_t divsel=0;

  CHECKMO;

  MLOCK;
  divsel = MO_DIV_CTRL_DIV0_SELECT | MO_DIV_CTRL_DIV1_SELECT;
  vmeWrite32(&MOp->div_ctrl, divsel | 0x0030);	// soft reset both divider chips (must toggle bit)
  vmeWrite32(&MOp->div_ctrl, divsel | 0x0010);	// (DON'T need to update register bits for addr=00!)
  MUNLOCK;
  return OK;
}

/**
 *  @ingroup Config
 *
 *  @brief Setup standard clock configuration
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
moSetupClocks()
{
  uint32_t value_0, value_1;

  CHECKMO;

  MLOCK;
  vmeWrite32(&MOp->div_ctrl, MO_DIV_CTRL_DIV0_SELECT | 0x4502);	// use CLK2 input for DIV 0 divider, power down CLK1
  vmeWrite32(&MOp->div_ctrl, MO_DIV_CTRL_DIV1_SELECT | 0x4505);	// use CLK1 input for DIV 1 divider, power down CLK2
  vmeWrite32(&MOp->div_ctrl, MO_DIV_CTRL_DIV0_SELECT | MO_DIV_CTRL_DIV1_SELECT |
	     MO_DIV_CTRL_READ_SER_DATA | 0x4500);	// read both divider chips
  vmeWrite32(&MOp->div_ctrl, MO_DIV_CTRL_DIV0_SELECT | MO_DIV_CTRL_DIV1_SELECT | 0x5A01);	// update registers
  value_0 = vmeRead32(&MOp->div_read[0]);
  value_1 = vmeRead32(&MOp->div_read[1]);
  MUNLOCK;
  //     	printf("div addr = %X   reg 0 = %X   reg 1 = %X\n", 0x45, value_0, value_1);

  return OK;
}

/**
 *  @ingroup Config
 *
 *  @brief Configures 'output' (0-9) with divider value 'divider' (1-32)
 *         and duty cycle mode 'duty_mode' (0-2)
 *
 *  @param output    Output channel
 *  @param divider   Divider value
 *  @param duty_mode Duty cycle mode
 *       - 0: duty cycle closest to 50% achievable for given divider
 *       - 1: minimum duty cycle achievable for given divider
 *       - 2: maximum duty cycle achievable for given divider
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
moConfigOutput(int output, int divider, int duty_mode)
{
  uint32_t addr[10] = { 0x4E, 0x4C, 0x52, 0x50, 0x4A,	// chip address map for outputs (0-9)
			0x4E, 0x4C, 0x52, 0x50, 0x4A };
  uint32_t offset, high, low, div_value;
  CHECKMO;

  if( (output < 0) || (output > moMaxChanNumber))	// check if inputs are within bounds
    {
      printf("%s: Invalid output value (%d)\n",
	     __FUNCTION__,output);
      return ERROR;
    }

  if( (divider < 1) || (divider > 32))
    {
      printf("%s: Invalid divider value (%d)\n",
	     __FUNCTION__,divider);
      return ERROR;
    }

  if( (duty_mode < 0) || (duty_mode > 2))
    {
      printf("%s: Invalid duty_mode value (%d)\n",
	     __FUNCTION__,duty_mode);
      return ERROR;
    }

  if( output <= 4 )
    offset = MO_DIV_CTRL_DIV0_SELECT;		// divider 0 chip
  else
    offset = MO_DIV_CTRL_DIV1_SELECT;		// divider 1 chip

  MLOCK;
  if( divider == 1 )			// must bypass divider (write 0x80 to next address)
    {
      vmeWrite32(&MOp->div_ctrl, offset | ((addr[output] + 1) << 8) | 0x80);
    }
  else					// for all other divider values
    {
      switch( duty_mode )
	{
	case 0:				// 50% (or closest achievable) duty cycle
	  high = (int)(divider/2);
	  low = divider - high;
	  break;

	case 1:
	  if( divider < 18 )		// minimum duty cycle achievable
	    high = 1;
	  else
	    high = divider - 16;
	  low = divider - high;
	  break;

	case 2:				// maximum duty cycle achievable
	  if( divider < 18 )
	    low = 1;
	  else
	    low = divider - 16;
	  high = divider - low;
	  break;
	}

      div_value = ((low - 1) << 4) | (high - 1);
      //	    printf("output = %d   LO = %X   HI = %X   div_value = %X\n", output, (low - 1), (high - 1), div_value);

      vmeWrite32(&MOp->div_ctrl, offset | ((addr[output] + 1) << 8) | 0x0); // clear bypass divider
      vmeWrite32(&MOp->div_ctrl, offset | (addr[output] << 8) | div_value);

    }

  vmeWrite32(&MOp->div_ctrl, offset | 0x5A01);		// update registers
  MUNLOCK;

  return OK;

}

/**
 *  @ingroup Status
 *  @brief Print configuration of all dividers
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
moConfigPrint()
{
  uint32_t ps0[4] = { 2, 4, 8, 16 };

  uint32_t addr[10] = { 0x4E, 0x4C, 0x52, 0x50, 0x4A,	// chip address map for outputs (0-9)
		   0x4E, 0x4C, 0x52, 0x50, 0x4A };

  uint32_t ps0_value, output, divider, high, low, chip, offset, reg_value;
  uint32_t total_divide;
  int ii;
  float duty_cycle;

  CHECKMO;

  MLOCK;
  ps0_value = 0x3 & vmeRead32(&MOp->ps0_ctrl);
  printf("\n\n%s:\ninitial prescale factor = %d\n",
	 __FUNCTION__, ps0[ps0_value]);

  printf("\noutput   divider  total divide  duty cycle\n");
  printf("------------------------------------------\n");
  for(ii=0;ii<(moMaxChanNumber + 1);ii++)
    {
      output = ii;
      if( (output >= 0) && (output <= 4) )
	{
	  offset = MO_DIV_CTRL_DIV0_SELECT;		// divider 0 chip
	  chip = 0;
	}
      else
	{
	  offset = MO_DIV_CTRL_DIV1_SELECT;		// divider 1 chip
	  chip = 1;
	}

      vmeWrite32(&MOp->div_ctrl, offset | MO_DIV_CTRL_READ_SER_DATA |
		 ((addr[output] + 1) << 8));	// check if divider is bypassed
      reg_value = 0xFF & vmeRead32(&MOp->div_read[chip]);

      if( reg_value & 0x80 )
	{
	  divider = 1;
	  duty_cycle = 0.50;
	}
      else
	{
	  vmeWrite32(&MOp->div_ctrl, offset | MO_DIV_CTRL_READ_SER_DATA |
		     (addr[output] << 8));		// find divider, duty cycle
	  reg_value = 0xFF & vmeRead32(&MOp->div_read[chip]);
	  high = (0xF & reg_value) + 1;
	  low  = (0xF & (reg_value >> 4)) + 1;
	  divider = high + low;
	  duty_cycle = ((float)(high))/((float)(divider));
	}
      total_divide = ps0[ps0_value] * divider;
      printf("%4d     %4d      %6d        %6.3f \n",
	     output, divider, total_divide, duty_cycle);
    }
  printf("------------------------------------------\n\n");
  MUNLOCK;

  return OK;
}

/**
 *  @ingroup Config
 *  @brief Set up Initial Prescale factor (2,4,8,16)
 *
 *  @param ps0 Prescale factor
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
moConfigPS0(int ps0)
{
  uint32_t value;

  CHECKMO;

  switch(ps0)
    {
    case 2:
      value = 0;
      break;

    case 4:
      value = 1;
      break;

    case 8:
      value = 2;
      break;

    case 16:
      value = 3;
      break;

    default:
      printf("%s: ERROR: Invalid ps0 value (%d). Setting to 2.\n",
	     __FUNCTION__,ps0);
      value = 0;
    }

  MLOCK;
  vmeWrite32(&MOp->ps0_ctrl, value);
  MUNLOCK;

  return OK;
}

/**
 *  @ingroup Config
 *  @brief Set second stage prescale factor for selected channel
 *
 *  @param channel Selected Channel
 *  @param prescale Second stage prescale factor
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
moSetPrescale(uint32_t channel, uint32_t prescale)
{
  uint32_t duty_mode=0;
  CHECKMO;

  if((channel<0) || (channel>moMaxChanNumber))
    {
      printf("%s: ERROR: Invalid channel (%d)\n",
	     __FUNCTION__,channel);
      return ERROR;
    }

  if((prescale<1) || (prescale>32))
    {
      printf("%s: ERROR: Invalid prescale (%d)\n",
	     __FUNCTION__,prescale);
      return ERROR;
    }

  if(moGetDutyMode(channel, &duty_mode)!=OK)
    {
      printf("%s: ERROR: Failled to get current duty_mode for channel %d\n",
	     __FUNCTION__,channel);
      return ERROR;
    }

  if(moConfigOutput(channel, prescale, duty_mode)!=OK)
    {
      printf("%s(%d,%d): ERROR: Unable to set prescale.\n",
	     __FUNCTION__,channel, prescale);
      return ERROR;
    }

  if(moSyncDividers()!=OK)
    {
      printf("%s: ERROR synchronizing dividers.\n",
	     __FUNCTION__);
      return ERROR;
    }

  return OK;
}

/**
 *  @ingroup Status
 *  @brief Get second stage prescale factor for selected channel
 *
 *  @param channel Selected Channel
 *  @param *prescale Address to store second stage prescale factor
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
moGetPrescale(uint32_t channel, uint32_t *prescale)
{
  uint32_t addr[10] = { 0x4E, 0x4C, 0x52, 0x50, 0x4A,	// chip address map for outputs (0-9)
			0x4E, 0x4C, 0x52, 0x50, 0x4A };
  uint32_t div_bypass=0, div=0, divsel=0;
  int chip=0;
  CHECKMO;

  if((channel>=0) && (channel<=4))
    {
      divsel = MO_DIV_CTRL_DIV0_SELECT;
      chip=0;
    }
  else if( (channel>=5) && (channel<=9) && (moMaxChanNumber == 9) )
    {
      divsel = MO_DIV_CTRL_DIV1_SELECT;
      chip=1;
    }
  else
    {
      printf("%s: ERROR: Invalid channel (%d)\n",
	     __FUNCTION__,channel);
      return ERROR;
    }

  if(!prescale)
    {
      printf("%s: ERROR: Invalid prescale pointer\n",
	     __FUNCTION__);
      return ERROR;
    }

  MLOCK;
  vmeWrite32(&MOp->div_ctrl, divsel | MO_DIV_CTRL_READ_SER_DATA | (addr[channel]+1)<<8);
  div_bypass = vmeRead32(&MOp->div_read[chip]) & MO_DIV_READ_DATA_MASK;

  /* Serial data math */
  if(div_bypass & 0x80)
    div = 1;
  else
    {
      vmeWrite32(&MOp->div_ctrl, divsel | MO_DIV_CTRL_READ_SER_DATA | (addr[channel])<<8);
      div = vmeRead32(&MOp->div_read[chip]) & MO_DIV_READ_DATA_MASK;
      div = ((div&0xF)+1) + (((div&0xF0)>>4)+1);
    }

  *prescale = div;
  MUNLOCK;

  return OK;
}

/**
 *  @ingroup Config
 *  @brief Set second stage duty_mode for selected channel
 *
 *  @param channel Selected Channel
 *  @param duty_mode Second stage duty_mode
 *       - 0: duty cycle closest to 50% achievable for given divider
 *       - 1: minimum duty cycle achievable for given divider
 *       - 2: maximum duty cycle achievable for given divider
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
moSetDutyMode(uint32_t channel, int duty_mode)
{
  uint32_t prescale=0;
  CHECKMO;

  if((channel<0) || (channel>moMaxChanNumber))
    {
      printf("%s: ERROR: Invalid channel (%d)\n",
	     __FUNCTION__,channel);
      return ERROR;
    }

  if((duty_mode<0) || (duty_mode>2))
    {
      printf("%s: ERROR: Invalid duty_mode (%d)\n",
	     __FUNCTION__,duty_mode);
      return ERROR;
    }

  if(moGetPrescale(channel,&prescale)!=OK)
    {
      printf("%s: ERROR: Failed to obtain current prescale for channel %d\n",
	     __FUNCTION__,channel);
      return ERROR;
    }

  if(moConfigOutput(channel, prescale, duty_mode)!=OK)
    {
      printf("%s(%d,%d): ERROR: Unable to set prescale.\n",
	     __FUNCTION__,channel, prescale);
      return ERROR;
    }

  if(moSyncDividers()!=OK)
    {
      printf("%s: ERROR synchronizing dividers.\n",
	     __FUNCTION__);
      return ERROR;
    }

  return OK;

}

/**
 *  @ingroup Status
 *  @brief Get second stage duty_mode for selected channel
 *
 *  @param channel Selected Channel
 *  @param *duty_mode Address to store Second stage duty_mode
 *       - 0: duty cycle closest to 50% achievable for given divider
 *       - 1: minimum duty cycle achievable for given divider
 *       - 2: maximum duty cycle achievable for given divider
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
moGetDutyMode(uint32_t channel, uint32_t *duty_mode)
{
  uint32_t addr[10] = { 0x4E, 0x4C, 0x52, 0x50, 0x4A,	// chip address map for outputs (0-9)
			0x4E, 0x4C, 0x52, 0x50, 0x4A };

  uint32_t divider, high, low, chip, offset, reg_value;

  CHECKMO;

  if((channel<0) || (channel>moMaxChanNumber))
    {
      printf("%s: ERROR: Invalid channel (%d)\n",
	     __FUNCTION__,channel);
      return ERROR;
    }

  if(!duty_mode)
    {
      printf("%s: ERROR: Invalid duty_mode pointer\n",
	     __FUNCTION__);
      return ERROR;
    }

  if( (channel >= 0) && (channel <= 4) )
    {
      offset = MO_DIV_CTRL_DIV0_SELECT;		// divider 0 chip
      chip = 0;
    }
  else
    {
      offset = MO_DIV_CTRL_DIV1_SELECT;		// divider 1 chip
      chip = 1;
    }

  MLOCK;
  vmeWrite32(&MOp->div_ctrl, offset | MO_DIV_CTRL_READ_SER_DATA |
	     ((addr[channel] + 1) << 8));	// check if divider is bypassed
  reg_value = 0xFF & vmeRead32(&MOp->div_read[chip]);

  if( reg_value & 0x80 )
    {
      *duty_mode = 0;
    }
  else
    {
      vmeWrite32(&MOp->div_ctrl, offset | MO_DIV_CTRL_READ_SER_DATA |
		 (addr[channel] << 8));		// find divider, duty cycle
      reg_value = 0xFF & vmeRead32(&MOp->div_read[chip]);
      high = (0xF & reg_value) + 1;
      low  = (0xF & (reg_value >> 4)) + 1;
      divider = high + low;

      if(high == ((int)(divider/2)))
	{
	  *duty_mode = 0;
	}
      else
	{
	  if(divider<18)
	    {
	      if(high==1)
		*duty_mode = 1;
	      else
		*duty_mode = 2;
	    }
	  else
	    {
	      if(high == (divider-16))
		*duty_mode = 1;
	      else
		*duty_mode = 2;
	    }
	}
    }
  MUNLOCK;

  return OK;
}

/**
 *  @ingroup Status
 *  @brief Get second stage duty cycle for selected channel
 *
 *  @param channel Selected Channel
 *  @param *duty_cycle Address to store Second stage duty cycle (in percent)
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
moGetDutyCycle(uint32_t channel, float *duty_cycle)
{
  uint32_t addr[10] = { 0x4E, 0x4C, 0x52, 0x50, 0x4A,	// chip address map for outputs (0-9)
			0x4E, 0x4C, 0x52, 0x50, 0x4A };

  uint32_t  divider, high, low, chip, offset, reg_value;

  CHECKMO;

  if((channel<0) || (channel>moMaxChanNumber))
    {
      printf("%s: ERROR: Invalid channel (%d)\n",
	     __FUNCTION__,channel);
      return ERROR;
    }

  if(!duty_cycle)
    {
      printf("%s: ERROR: Invalid duty_cycle pointer\n",
	     __FUNCTION__);
      return ERROR;
    }

  if( (channel >= 0) && (channel <= 4) )
    {
      offset = MO_DIV_CTRL_DIV0_SELECT;		// divider 0 chip
      chip = 0;
    }
  else
    {
      offset = MO_DIV_CTRL_DIV1_SELECT;		// divider 1 chip
      chip = 1;
    }

  MLOCK;
  vmeWrite32(&MOp->div_ctrl, offset | MO_DIV_CTRL_READ_SER_DATA |
	     ((addr[channel] + 1) << 8));	// check if divider is bypassed
  reg_value = 0xFF & vmeRead32(&MOp->div_read[chip]);

  if( reg_value & 0x80 )
    {
      *duty_cycle = 0.500;
    }
  else
    {
      vmeWrite32(&MOp->div_ctrl, offset | MO_DIV_CTRL_READ_SER_DATA |
		 (addr[channel] << 8));		// find divider, duty cycle
      reg_value = 0xFF & vmeRead32(&MOp->div_read[chip]);
      high = (0xF & reg_value) + 1;
      low  = (0xF & (reg_value >> 4)) + 1;
      divider = high + low;

      *duty_cycle = ((float)(high))/((float)(divider));
    }
  MUNLOCK;

  return OK;
}


/**
 *  @ingroup Config
 *  @brief Set initial stage prescale for all channels
 *
 *  @param prescale Initial Stage prescale value
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
moSetInitialPrescale(uint32_t prescale)
{
  return moConfigPS0(prescale);
}

/**
 *  @ingroup Status
 *  @brief Get Initial State Prescale value
 *
 *  @param *prescale Address to store initial state prescale value
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
moGetInitialPrescale(uint32_t *prescale)
{
  int err = OK;
  uint32_t ps0[4] = { 2, 4, 8, 16 };
  uint32_t ps0_value = 0;
  CHECKMO;

  MLOCK;
  ps0_value = 0x3 & vmeRead32(&MOp->ps0_ctrl);
  if ( 0 > ps0_value || ps0_value > 3 )
    {
      *prescale = 0;
      err = ERROR;
    }
  else
    {
      *prescale = ps0[ps0_value];
    }
  MUNLOCK;

  return err;
}


/**
 *  @ingroup Status
 *  @brief Test data integrity by writing and reading test register
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int
moTestAccess()
{
  uint32_t data_value=0, read_value=0;
  int error=OK, ii;

  CHECKMO;
  printf("\n\n%s: Begin data integrity test\n",__FUNCTION__);

  MLOCK;
  data_value = 0x0;
  vmeWrite32(&MOp->test, data_value);
  read_value = vmeRead32(&MOp->test);
  if( read_value != data_value )
    {
      error = ERROR;
      printf("***** data error: data write = %X   data read = %X\n", data_value, read_value);
    }

  data_value = 0xFFFFFFFF;
  vmeWrite32(&MOp->test, data_value);
  read_value = vmeRead32(&MOp->test);
  if( read_value != data_value )
    {
      error = ERROR;
      printf("***** data error: data write = %X   data read = %X\n", data_value, read_value);
    }

  data_value = 0xAAAAAAAA;
  vmeWrite32(&MOp->test, data_value);
  read_value = vmeRead32(&MOp->test);
  if( read_value != data_value )
    {
      error = ERROR;
      printf("***** data error: data write = %X   data read = %X\n", data_value, read_value);
    }

  data_value = 0x55555555;
  vmeWrite32(&MOp->test, data_value);
  read_value = vmeRead32(&MOp->test);
  if( read_value != data_value )
    {
      error = ERROR;
      printf("***** data error: data write = %X   data read = %X\n", data_value, read_value);
    }

  data_value = 1;
  for(ii=0;ii<32;ii++)
    {
      vmeWrite32(&MOp->test, data_value);
      read_value = vmeRead32(&MOp->test);
      if( read_value != data_value )
	{
	  error = ERROR;
	  printf("***** data error: data write = %X   data read = %X\n", data_value, read_value);
	}
      data_value = data_value * 2;
    }

  if( error == 0 )
    printf("\n----- data transfers O.K. -----\n\n");

  MUNLOCK;

  return error;
}

#else /* dummy version*/

void
moLib_dummy()
{
  return;
}

#endif
