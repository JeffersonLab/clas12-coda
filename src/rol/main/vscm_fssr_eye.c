
/* vscm_mclkphase.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vscmLib.h"



#ifdef Linux_vme

#include "jvme.h"
/*
#define B(to_bit,from_reg,from_bit) (((from_reg>>from_bit) & 0x1)<<to_bit)

typedef struct
{
  int addr;
  int mask;
  int data;
} addr_data_t;

#define FRAC_PRECISION  10
#define FIXED_WIDTH     32

uint32_t
round_frac(uint32_t decimal, uint32_t precision)
{
  uint32_t round_frac = decimal;

  if(decimal & (1<<((FRAC_PRECISION-1)-precision)))
    round_frac+= (1<<((FRAC_PRECISION-1)-precision));

  return round_frac;
}

uint32_t
pll_divider(uint32_t divide, uint32_t duty)
{
  uint32_t duty_cycle_fix = (duty<<FRAC_PRECISION)/100000;
  uint32_t high_time, w_edge, low_time, no_count, temp, result;

  if(divide == 1)
  {
    high_time = 1;
    w_edge    = 0;
    low_time  = 1;
    no_count  = 1;
  }
  else
  {
    temp = round_frac(duty_cycle_fix*divide,1);
    
    high_time = (temp>>FRAC_PRECISION) & 0x7F;
    w_edge    = (temp>>(FRAC_PRECISION-1)) & 0x1;

    if(!high_time)
    {
      high_time = 1;
      w_edge = 0;
    }

    if(high_time == divide)
    {
      high_time = divide-1;
      w_edge = 1;
    }

    low_time = divide - high_time;
    no_count = 0;
  }

  printf("low_time = %2d, high_time = %2d, no_count = %d, w_edge = %d, ", low_time & 0x3f, high_time & 0x3f, no_count, w_edge);

  result = (low_time  & 0x3F)<<0;
  result|= (high_time & 0x3F)<<6;
  result|= (no_count  & 0x01)<<12;
  result|= (w_edge    & 0x01)<<13;

  return result;
}

uint32_t
pll_phase(uint32_t divide, uint32_t phase)
{
  uint32_t phase_fixed = (phase<<FRAC_PRECISION)/1000;
  uint32_t phase_in_cycles = (phase_fixed * divide)/360;
  uint32_t temp = round_frac(phase_in_cycles,3);
  uint32_t phase_mux  = (temp>>(FRAC_PRECISION-2-1)) & 0x07;
  uint32_t delay_time = (temp>>(FRAC_PRECISION+1-1)) & 0x3F;
  uint32_t result;

  result = delay_time;
  result|= phase_mux<<6;

  printf("delay_time = %2d, phase_mux = %2d, ", delay_time, phase_mux);

  return result;
}

uint32_t
compute_phase_value(float phase)
{
  uint32_t result;
  uint32_t iphase = ((phase * 1000) >= 360000) ? (phase * 1000 - 360000) : phase * 1000;
  uint32_t iduty = 50000;
  uint32_t divide = 12;
  uint32_t div_calc = pll_divider(divide, iduty);
  uint32_t phase_calc = pll_phase(divide, iphase);

  result = (div_calc & 0x3FFF) | ((phase_calc & 0x1FF)<<14);

  printf("Input phase: %6.3f, div_calc = %4d (%04X), phase_calc = %4d (%04X)\n", phase, div_calc, div_calc, phase_calc, phase_calc);

  return result;
}

void
vscmWriteMclkSettings(int id, float mclk_phase)
{
  // only updateds CLKOUT0 and CLKOUT1 settings of PLLADV
  uint32_t CLKOUT[2];
  uint32_t val;

  CLKOUT[0] = compute_phase_value(mclk_phase);
  CLKOUT[1] = compute_phase_value(mclk_phase+90.0);

  addr_data_t settings[9] = {
      {0x05, 0x50FF, B(15,CLKOUT[0],19) | B(13,CLKOUT[0],18) | 
                     B(11,CLKOUT[0],16) | B(10,CLKOUT[0],17) | B( 9,CLKOUT[0],15) | B( 8,CLKOUT[0],14) },

      {0x06, 0x010B, B(15,CLKOUT[1],19) | B(14,CLKOUT[1], 5) | B(13,CLKOUT[1], 3) | B(12,CLKOUT[1],12) |
                     B(11,CLKOUT[1], 1) | B(10,CLKOUT[1], 2) | B( 9,CLKOUT[1],19) | B( 7,CLKOUT[1],17) | B( 6,CLKOUT[1],16) |
                     B( 5,CLKOUT[1],14) | B( 4,CLKOUT[1],15) | B( 2,CLKOUT[0],13) },

      {0x07, 0xE02C, B(12,CLKOUT[1],11) | B(11,CLKOUT[1], 9) | B(10,CLKOUT[1],10) |
                     B( 9,CLKOUT[1], 8) | B( 8,CLKOUT[1], 7) | B( 7,CLKOUT[1], 6) | B( 6,CLKOUT[1],20) | B( 4,CLKOUT[1],13) |
                     B( 1,CLKOUT[1],21) | B( 0,CLKOUT[1],22) },

      {0x09, 0xCFFF, B(13,CLKOUT[0],21) | B(12,CLKOUT[0],22) },

      {0x0B, 0x7DFF, B(15,CLKOUT[0], 5) |
                     B( 9,CLKOUT[0], 4) },

      {0x0D, 0xFF8F, B( 6,CLKOUT[0], 3) | 
                     B( 5,CLKOUT[0], 0) | B( 4,CLKOUT[0], 2) },

      {0x0F, 0xFFFB, B( 2,CLKOUT[1], 1) },

      {0x10, 0x8FFF, B(14,CLKOUT[0], 9) | B(13,CLKOUT[0],11) | B(12,CLKOUT[0],10) },

      {0x11, 0xFFA0, B( 6,CLKOUT[1],18) |
                     B( 4,CLKOUT[1], 0) | B( 3,CLKOUT[0], 6) | B( 2,CLKOUT[0],20) | B( 1,CLKOUT[0], 8) | B( 0,CLKOUT[0], 7) },
    };

  vscmMclkReset(id, 1);

  for(int i=0;i<sizeof(settings)/sizeof(addr_data_t);i++)
  {
    val = vscmMclkDrpRead(id, settings[i].addr);
    val&= settings[i].mask;
    val|= settings[i].data;
    vscmMclkDrpWrite(id, settings[i].addr, val);
  }

  vscmMclkReset(id, 0);

  usleep(10000);

  if(vscmMclkLocked(id))
    printf("Successful: PLL locked\n");
  else
    printf("Unsuccessful: PLL NOT locked\n");
}
*/

int
main(int argc, char *argv[])
{
  int res, nvscm;
  char myname[256];
  unsigned int addr, laddr;
  int i, val, slot = 0, chip;

  /* Open the default VME windows */
  vmeOpenDefaultWindows();
  printf("\n");

  /* update firmware */

  nvscm = vscmInit(0x100000,0x80000,20,0x80000000);
//  vscmConfig ("");

  printf("NVSCM=%d\n",nvscm);

//  fssrEyeSetup(3, 0);

  for(i=0;i<nvscm;i++)
  {
    for(chip=0;chip<8;chip++)
    {
      printf("%s(slot=%d,chip=%d)\n", __func__, vscmSlot(i), chip);
      fssrEyeStatus(vscmSlot(i), chip);
    }
  }
/*
  for(int addr=0;addr<32;addr++)
  {
    val = vscmMclkDrpRead(3, addr);
    printf("Address: 0x%02X = 0x%04X\n", addr, val);
  }

  float phase = 0.0;
  while(phase < 360.0)
  {
    vscmWriteMclkSettings(3, phase);

    phase+= 3.0;
    vscmDisableScaler(3);
    vscmEnableScaler(3);
    usleep(1000000);
    vscmDisableScaler(3);
    for(i=0;i<8;i++)
      printf("fssrReadScalerStatusWord(3,%d) = %d, fssrReadScalerMarkErr(3,%d) = %d\n", i, fssrReadScalerStatusWord(3,i), i, fssrReadScalerMarkErr(3,i));
  }
*/
  exit(0);
}

#else

int
main()
{
  return(0);
}

#endif
