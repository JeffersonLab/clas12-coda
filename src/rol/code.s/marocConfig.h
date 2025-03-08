
/* marocConfig.h */

#ifndef MAROCCONFIG_H
#define MAROCCONFIG_H

#define FNLEN     128       /* length of config. file name */
#define STRLEN    250       /* length of str_tmp */
#define ROCLEN     80       /* length of ROC_name */

typedef struct
{
  union
  {
    int val;
    struct
    {
      int cmd_fsu        : 1;  //bit 0
      int cmd_ss         : 1;  //bit 1
      int cmd_fsb        : 1;  //bit 2
      int swb_buf_250f   : 1;  //bit 3
      int swb_buf_500f   : 1;  //bit 4
      int swb_buf_1p     : 1;  //bit 5
      int swb_buf_2p     : 1;  //bit 6
      int ONOFF_ss       : 1;  //bit 7
      int sw_ss_300f     : 1;  //bit 8
      int sw_ss_600f     : 1;  //bit 9
      int sw_ss_1200f    : 1;  //bit 10
      int EN_ADC         : 1;  //bit 11
      int H1H2_choice    : 1;  //bit 12
      int sw_fsu_20f     : 1;  //bit 13
      int sw_fsu_40f     : 1;  //bit 14
      int sw_fsu_25k     : 1;  //bit 15
      int sw_fsu_50k     : 1;  //bit 16
      int sw_fsu_100k    : 1;  //bit 17
      int sw_fsb1_50k    : 1;  //bit 18
      int sw_fsb1_100k   : 1;  //bit 19
      int sw_fsb1_100f   : 1;  //bit 20
      int sw_fsb1_50f    : 1;  //bit 21
      int cmd_fsb_fsu    : 1;  //bit 22
      int valid_dc_fs    : 1;  //bit 23
      int sw_fsb2_50k    : 1;  //bit 24
      int sw_fsb2_100k   : 1;  //bit 25
      int sw_fsb2_100f   : 1;  //bit 26
      int sw_fsb2_50f    : 1;  //bit 27
      int valid_dc_fsb2  : 1;  //bit 28
      int ENb_tristate   : 1;  //bit 29
      int polar_discri   : 1;  //bit 30
      int inv_discriADC  : 1;  //bit 31
    } bits;
  } Global0;

  union
  {
    int val;
    struct
    {
      int d1_d2              : 1;  //bit 0
      int cmd_CK_mux         : 1;  //bit 1
      int ONOFF_otabg        : 1;  //bit 2
      int ONOFF_dac          : 1;  //bit 3
      int small_dac          : 1;  //bit 4
      int enb_outADC         : 1;  //bit 5
      int inv_startCmptGray  : 1;  //bit 6
      int ramp_8bit          : 1;  //bit 7
      int ramp_10bit         : 1;  //bit 8
      int Reserved0          : 23;  //bit 9-31
    } bits;
  } Global1;

  int DAC0;
  int DAC1;
  int Gain[64];
  int Sum[2];
  int CTest[2];
  int MaskOr[2];
  int TriggerOr0[2];
  int TriggerOr1[2];
} maroc_config_t;

typedef struct
{
  maroc_config_t chip[3];
  int ctest_dac;
  int ctest_enable;
  int tdc_enable_mask[6];
  int window_width;
  int window_offset;
  int asic_num;
} MAROC_CONF;

void marocSetExpid(char *string);
int marocConfig(char *fname);
void marocInitGlobals();
int marocReadConfigFile(char *filename_in);
int marocDownloadAll();
int marocUploadAll(char *string, int length);
int marocUploadAllPrint();



#endif

