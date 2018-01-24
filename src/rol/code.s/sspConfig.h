#ifndef SSPCONFIG_H
#define SSPCONFIG_H

#include "sspLib.h"
#include "sspLib_rich.h"

/****************************************************************************
 *
 *  sspConfig.h  -  configuration library header file for SSP board 
 *
 */


#define FNLEN     128       /* length of config. file name */
#define STRLEN    250       /* length of str_tmp */
#define ROCLEN     80       /* length of ROC_name */
#define NBOARD     22

typedef struct
{
  int emin;
  int emax;
  int nmin;
  int emin_en;
  int emax_en;
  int nmin_en;
  
  int prescale_xmin[7];
  int prescale_xmax[7];
  int prescale[7];
} singles_trig;

typedef struct
{
  int timecoincidence;
  int emin;
  int emax;
  int nmin;
  int summax;
  int summin;
  int summax_en;
  int diffmax;
  int diffmax_en;
  int coplanartolerance;
  int coplanartolerance_en;
  float edfactor;
  int edmin;
  int ed_en;
} pairs_trig;

typedef struct
{
  int en;
  
  int pcal_cluster_emin_en;
  int pcal_cluster_emin;
  int pcal_cluster_width;
  
  int ecal_cluster_emin_en;
  int ecal_cluster_emin;
  int ecal_cluster_width;

  int ecalpcal_cluster_emin_en;
  int ecalpcal_cluster_emin;
  int ecalpcal_cluster_width;
  
  int pcal_esum_en;
  int pcal_esum_emin;
  int pcal_esum_width;
  
  int ecal_esum_en;
  int ecal_esum_emin;
  int ecal_esum_width;

  int dc_mult_en;
  int dc_mult_min;
  int dc_mult_width;

  int htcc_en;
  long long htcc_mask;
  int htcc_width;

  int ftof_en;
  long long ftof_mask;
  int ftof_width;

  int ctof_en;
  int ctof_mask;
  int ctof_width;

  int cnd_en;
  int cnd_mask;
  int cnd_width;

  int ecalin_cosmic_en;
  int ecalout_cosmic_en;
  int pcal_cosmic_en;
  int cosmic_width;
} strigger;

typedef struct
{
  int esum_delay;
  int cluster_delay;
  int esum_intwidth;
  int cosmic_delay;
} ss_ecal;

typedef struct
{
  int seg_delay;
} ss_dc;

typedef struct
{
  int en;

  int ft_cluster_en;
  int ft_cluster_emin;
  int ft_cluster_emax;
  int ft_cluster_hodo_nmin;
  int ft_cluster_nmin;
  int ft_cluster_width;

  int ft_esum_en;
  int ft_esum_emin;
  int ft_esum_width;
} ctrigger;

typedef struct
{
  int esum_delay;
  int cluster_delay;
  int esum_intwidth;
} ss_ft;

typedef struct
{
  int htcc_delay;
} ss_htcc;

typedef struct
{
  int ftof_delay;
} ss_ftof;

typedef struct
{
  int ctof_delay;
} ss_ctof;

typedef struct
{
  int cnd_delay;
} ss_cnd;

typedef struct
{
  int cmd_fsu;
  int cmd_ss;
  int cmd_fsb;
  int swb_buf_250f;
  int swb_buf_500f;
  int swb_buf_1p;
  int swb_buf_2p;
  int ONOFF_ss;
  int sw_ss_300f;
  int sw_ss_600f;
  int sw_ss_1200f;
  int EN_ADC;
  int H1H2_choice;
  int sw_fsu_20f;
  int sw_fsu_40f;
  int sw_fsu_25k;
  int sw_fsu_50k;
  int sw_fsu_100k;
  int sw_fsb1_50k;
  int sw_fsb1_100k;
  int sw_fsb1_100f;
  int sw_fsb1_50f;
  int cmd_fsb_fsu;
  int valid_dc_fs;
  int sw_fsb2_50k;
  int sw_fsb2_100k;
  int sw_fsb2_100f;
  int sw_fsb2_50f;
  int valid_dc_fsb2;
  int ENb_tristate;
  int polar_discri;
  int inv_discriADC;
  int d1_d2;
  int cmd_CK_mux;
  int ONOFF_otabg;
  int ONOFF_dac;
  int small_dac;
  int enb_outADC;
  int inv_startCmptGray;
  int ramp_8bit;
  int ramp_10bit;
  int DAC0;
  int DAC1;
  int Gain[64];
  unsigned int Sum[2];
  unsigned int CTest[2];
  unsigned int MaskOr[2];
} rich_maroc_reg;

typedef struct
{
  rich_maroc_reg chip[3];
  int ctest_dac;
  int ctest_enable;
  int tdc_enable_mask[6];
  int window_width;
  int window_offset;
} rich_fiber;

/** SSP configuration parameters **/
typedef struct {
  int fw_rev;
  int fw_type;
  
  int window_width;
  int window_offset;

  int pulser_freq;
  int ssp_io_mux[SD_SRC_NUM];
  
  struct
  {
    int trigger_latency;
    singles_trig s[2];
    pairs_trig p[2];
    int cosmic_timecoincidence;
    int cosmic_pattern;
  } hps;
  
  struct
  {
    strigger strg[8];
    ss_ecal  ecal;
    ss_ecal  pcal;
    ss_htcc  htcc;
    ss_ftof  ftof;
    ss_dc    dc;
    ss_ctof  ctof;
    ss_cnd   cnd;
    int      gtpif_latency;
  } gt;

  struct
  {
    ctrigger ctrg[4];
    ss_ft    ft;
    int      fanout_en_ctofhtcc;
    int      fanout_en_cnd;
    int      gtpif_latency;
  } gtc; 
 
  struct
  {
    rich_fiber fiber[RICH_FIBER_NUM];
  } rich;
  
} SSP_CONF;

/* functions */
void sspInitGlobals();
int sspReadConfigFile(char *filename);
int sspDownloadAll();
int sspConfig(char *fname);
void sspMon(int slot);

#endif
