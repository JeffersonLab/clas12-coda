
/* petirocConfig.h */

#ifndef PETIROCCONFIG_H
#define PETIROCCONFIG_H

#define FNLEN     128       /* length of config. file name */
#define STRLEN    250       /* length of str_tmp */
#define ROCLEN     80       /* length of ROC_name */

#include "petirocLib.h"

typedef struct
{
  PETIROC_Regs chip[2];
  int pulser_freq;
  int pulser_amp[4];
  int pulser_en;
  int tdc_auto_cal_en;
  int tdc_auto_cal_min_entries;
  int trig_busy_thr;
  int trig_delay;
  int tdc_enable_mask[2];
  int window_width;
  int window_offset;
  int clk_ext;
  int gain_sel;
  int fw_rev;
  int fw_timestamp;
} PETIROC_CONF;

void petirocSetExpid(char *string);
int petirocConfig(char *fname);
void petirocInitGlobals();
int petirocReadConfigFile(char *filename_in);
int petirocDownloadAll();
int petirocUploadAll(char *string, int length);
int petirocUploadAllPrint();

#endif

