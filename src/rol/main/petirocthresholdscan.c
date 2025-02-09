/* petirocthresholdscan.c */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "petirocLib.h"
#include "petirocConfig.h"

static int npetiroc;

int main(int argc, char *argv[])
{
  PETIROC_Regs chip[2];
  printf("\npetirocthresholdscan started ..\n\n");fflush(stdout);

  npetiroc = petirocInit(0, PETIROC_MAX_NUM, PETIROC_INIT_REGSOCKET);

  printf("\npetirocinit: npetiroc=%d\n\n",npetiroc);fflush(stdout);
  if(npetiroc<=0) exit(0);

  petirocInitGlobals();
  petirocConfig("");
  printf("\npetiroc initialized\n\n");fflush(stdout);


  for(int thr=0; thr<1024; thr++)
  {
    for(int j=0; j<npetiroc; j++)
    {
      int slot = petirocSlot(j);
 
      for(int i=0;i<2;i++)
      {
        chip[i].SlowControl.mask_discri_charge        = 0; 
        chip[i].SlowControl.inputDAC_ch0              = 0; 
        chip[i].SlowControl.inputDAC_en_ch0           = 1; 
        chip[i].SlowControl.inputDAC_ch1              = 0; 
        chip[i].SlowControl.inputDAC_en_ch1           = 1; 
        chip[i].SlowControl.inputDAC_ch2              = 0; 
        chip[i].SlowControl.inputDAC_en_ch2           = 1; 
        chip[i].SlowControl.inputDAC_ch3              = 0; 
        chip[i].SlowControl.inputDAC_en_ch3           = 1; 
        chip[i].SlowControl.inputDAC_ch4              = 0; 
        chip[i].SlowControl.inputDAC_en_ch4           = 1; 
        chip[i].SlowControl.inputDAC_ch5              = 0; 
        chip[i].SlowControl.inputDAC_en_ch5           = 1; 
        chip[i].SlowControl.inputDAC_ch6              = 0; 
        chip[i].SlowControl.inputDAC_en_ch6           = 1; 
        chip[i].SlowControl.inputDAC_ch7              = 0; 
        chip[i].SlowControl.inputDAC_en_ch7           = 1; 
        chip[i].SlowControl.inputDAC_ch8              = 0; 
        chip[i].SlowControl.inputDAC_en_ch8           = 1; 
        chip[i].SlowControl.inputDAC_ch9              = 0; 
        chip[i].SlowControl.inputDAC_en_ch9           = 1; 
        chip[i].SlowControl.inputDAC_ch10             = 0; 
        chip[i].SlowControl.inputDAC_en_ch10          = 1; 
        chip[i].SlowControl.inputDAC_ch11             = 0; 
        chip[i].SlowControl.inputDAC_en_ch11          = 1; 
        chip[i].SlowControl.inputDAC_ch12             = 0; 
        chip[i].SlowControl.inputDAC_en_ch12          = 1; 
        chip[i].SlowControl.inputDAC_ch13             = 0; 
        chip[i].SlowControl.inputDAC_en_ch13          = 1; 
        chip[i].SlowControl.inputDAC_ch14             = 0; 
        chip[i].SlowControl.inputDAC_en_ch14          = 1; 
        chip[i].SlowControl.inputDAC_ch15             = 0; 
        chip[i].SlowControl.inputDAC_en_ch15          = 1; 
        chip[i].SlowControl.inputDAC_ch16             = 0; 
        chip[i].SlowControl.inputDAC_en_ch16          = 1; 
        chip[i].SlowControl.inputDAC_ch17             = 0; 
        chip[i].SlowControl.inputDAC_en_ch17          = 1; 
        chip[i].SlowControl.inputDAC_ch18             = 0; 
        chip[i].SlowControl.inputDAC_en_ch18          = 1; 
        chip[i].SlowControl.inputDAC_ch19             = 0; 
        chip[i].SlowControl.inputDAC_en_ch19          = 1; 
        chip[i].SlowControl.inputDAC_ch20             = 0; 
        chip[i].SlowControl.inputDAC_en_ch20          = 1; 
        chip[i].SlowControl.inputDAC_ch21             = 0; 
        chip[i].SlowControl.inputDAC_en_ch21          = 1; 
        chip[i].SlowControl.inputDAC_ch22             = 0; 
        chip[i].SlowControl.inputDAC_en_ch22          = 1; 
        chip[i].SlowControl.inputDAC_ch23             = 0; 
        chip[i].SlowControl.inputDAC_en_ch23          = 1; 
        chip[i].SlowControl.inputDAC_ch24             = 0; 
        chip[i].SlowControl.inputDAC_en_ch24          = 1; 
        chip[i].SlowControl.inputDAC_ch25             = 0; 
        chip[i].SlowControl.inputDAC_en_ch25          = 1; 
        chip[i].SlowControl.inputDAC_ch26             = 0; 
        chip[i].SlowControl.inputDAC_en_ch26          = 1; 
        chip[i].SlowControl.inputDAC_ch27             = 0; 
        chip[i].SlowControl.inputDAC_en_ch27          = 1; 
        chip[i].SlowControl.inputDAC_ch28             = 0; 
        chip[i].SlowControl.inputDAC_en_ch28          = 1; 
        chip[i].SlowControl.inputDAC_ch29             = 0; 
        chip[i].SlowControl.inputDAC_en_ch29          = 1; 
        chip[i].SlowControl.inputDAC_ch30             = 0; 
        chip[i].SlowControl.inputDAC_en_ch30          = 1; 
        chip[i].SlowControl.inputDAC_ch31             = 0; 
        chip[i].SlowControl.inputDAC_en_ch31          = 1; 
        chip[i].SlowControl.inputDACdummy             = 0; 
        chip[i].SlowControl.mask_discri_time          = 0; 
        chip[i].SlowControl.DAC6b_ch0                 = 0; 
        chip[i].SlowControl.DAC6b_ch1                 = 0; 
        chip[i].SlowControl.DAC6b_ch2                 = 0; 
        chip[i].SlowControl.DAC6b_ch3                 = 0; 
        chip[i].SlowControl.DAC6b_ch4                 = 0; 
        chip[i].SlowControl.DAC6b_ch5                 = 0; 
        chip[i].SlowControl.DAC6b_ch6                 = 0; 
        chip[i].SlowControl.DAC6b_ch7                 = 0; 
        chip[i].SlowControl.DAC6b_ch8                 = 0; 
        chip[i].SlowControl.DAC6b_ch9                 = 0; 
        chip[i].SlowControl.DAC6b_ch10                = 0; 
        chip[i].SlowControl.DAC6b_ch11                = 0; 
        chip[i].SlowControl.DAC6b_ch12                = 0; 
        chip[i].SlowControl.DAC6b_ch13                = 0; 
        chip[i].SlowControl.DAC6b_ch14                = 0; 
        chip[i].SlowControl.DAC6b_ch15                = 0; 
        chip[i].SlowControl.DAC6b_ch16                = 0; 
        chip[i].SlowControl.DAC6b_ch17                = 0; 
        chip[i].SlowControl.DAC6b_ch18                = 0; 
        chip[i].SlowControl.DAC6b_ch19                = 0; 
        chip[i].SlowControl.DAC6b_ch20                = 0; 
        chip[i].SlowControl.DAC6b_ch21                = 0; 
        chip[i].SlowControl.DAC6b_ch22                = 0; 
        chip[i].SlowControl.DAC6b_ch23                = 0; 
        chip[i].SlowControl.DAC6b_ch24                = 0; 
        chip[i].SlowControl.DAC6b_ch25                = 0; 
        chip[i].SlowControl.DAC6b_ch26                = 0; 
        chip[i].SlowControl.DAC6b_ch27                = 0; 
        chip[i].SlowControl.DAC6b_ch28                = 0; 
        chip[i].SlowControl.DAC6b_ch29                = 0; 
        chip[i].SlowControl.DAC6b_ch30                = 0; 
        chip[i].SlowControl.DAC6b_ch31                = 0; 
        chip[i].SlowControl.EN_10b_DAC                = 1; 
        chip[i].SlowControl.PP_10b_DAC                = 0; 
        chip[i].SlowControl.vth_discri_charge         = 1023;
        chip[i].SlowControl.vth_time                  = thr;
        chip[i].SlowControl.EN_ADC                    = 0; 
        chip[i].SlowControl.PP_ADC                    = 0; 
        chip[i].SlowControl.sel_startb_ramp_ADC_ext   = 0; 
        chip[i].SlowControl.usebcompensation          = 0; 
        chip[i].SlowControl.ENbiasDAC_delay           = 0; 
        chip[i].SlowControl.PPbiasDAC_delay           = 0; 
        chip[i].SlowControl.ENbiasramp_delay          = 0; 
        chip[i].SlowControl.PPbiasramp_delay          = 0; 
        chip[i].SlowControl.DACdelay                  = 100;
        chip[i].SlowControl.EN_discri_delay           = 0; 
        chip[i].SlowControl.PP_discri_delay           = 0; 
        chip[i].SlowControl.EN_temp_sensor            = 0; 
        chip[i].SlowControl.PP_temp_sensor            = 0; 
        chip[i].SlowControl.EN_bias_pa                = 1; 
        chip[i].SlowControl.PP_bias_pa                = 0; 
        chip[i].SlowControl.EN_bias_discri            = 1; 
        chip[i].SlowControl.PP_bias_discri            = 0; 
        chip[i].SlowControl.cmd_polarity              = 0; 
        chip[i].SlowControl.LatchDiscri               = 0; 
        chip[i].SlowControl.EN_bias_6b_DAC            = 1; 
        chip[i].SlowControl.PP_bias_6b_DAC            = 0; 
        chip[i].SlowControl.EN_bias_tdc               = 1; 
        chip[i].SlowControl.PP_bias_tdc               = 0; 
        chip[i].SlowControl.ON_input_DAC              = 0; 
        chip[i].SlowControl.EN_bias_charge            = 1; 
        chip[i].SlowControl.PP_bias_charge            = 0; 
        chip[i].SlowControl.Cf                        = 0;
        chip[i].SlowControl.EN_bias_sca               = 1; 
        chip[i].SlowControl.PP_bias_sca               = 0; 
        chip[i].SlowControl.EN_bias_discri_charge     = 0; 
        chip[i].SlowControl.PP_bias_discri_charge     = 0; 
        chip[i].SlowControl.EN_bias_discri_ADC_time   = 0; 
        chip[i].SlowControl.PP_bias_discri_ADC_time   = 0; 
        chip[i].SlowControl.EN_bias_discri_ADC_charge = 0; 
        chip[i].SlowControl.PP_bias_discri_ADC_charge = 0; 
        chip[i].SlowControl.DIS_razchn_int            = 0; 
        chip[i].SlowControl.DIS_razchn_ext            = 0; 
        chip[i].SlowControl.SEL_80M                   = 0; 
        chip[i].SlowControl.EN_80M                    = 0; 
        chip[i].SlowControl.EN_slow_lvds_rec          = 1; 
        chip[i].SlowControl.PP_slow_lvds_rec          = 0; 
        chip[i].SlowControl.EN_fast_lvds_rec          = 1; 
        chip[i].SlowControl.PP_fast_lvds_rec          = 0; 
        chip[i].SlowControl.EN_transmitter            = 0; 
        chip[i].SlowControl.PP_transmitter            = 0; 
        chip[i].SlowControl.ON_1mA                    = 1; 
        chip[i].SlowControl.ON_2mA                    = 1; 
        chip[i].SlowControl.NC                        = 0; 
        chip[i].SlowControl.ON_ota_mux                = 0; 
        chip[i].SlowControl.ON_ota_probe              = 0; 
        chip[i].SlowControl.DIS_trig_mux              = 0; 
        chip[i].SlowControl.EN_NOR32_time             = 0; 
        chip[i].SlowControl.EN_NOR32_charge           = 0; 
        chip[i].SlowControl.DIS_triggers              = 0; 
        chip[i].SlowControl.EN_dout_oc                = 0; 
        chip[i].SlowControl.EN_transmit               = 0; 
      }

      petiroc_cfg_rst(slot);
      petiroc_slow_control(slot, chip);
//      petiroc_slow_control(slot, chip);
    }
    printf("thr=%d\n", thr);
    usleep(20000);
    petiroc_clear_scalers();
    usleep(100000);
    petiroc_status_all();
  }

  petirocEnd();

  exit(0);
}

