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
  int pcal_cluster_emax;
  int pcal_cluster_width;
  
  int ecal_cluster_emin_en;
  int ecal_cluster_emin;
  int ecal_cluster_emax;
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

  int dc_en;
  int dc_road_required;
  int dc_road_inbend_required;
  int dc_road_outbend_required;
  int dc_mult_min;
  int dc_width;

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

  int ftofpcu_en;
  int ftofpcu_width;
  int ftofpcu_match_mask;
} strigger;

typedef struct
{
  int esum_delay;
  int cluster_delay;
  int esum_intwidth;
  int cosmic_delay;
  int pcu_delay;
} ss_ecal;

typedef struct
{
  int seg_delay;
} ss_dc;

typedef struct
{
  int ftof_width;
  int pcu_width;
  int match_table;
} ss_ftofpcu;

typedef struct
{
  int en;

  int ft_cluster_en;
  int ft_cluster_emin;
  int ft_cluster_emax;
  int ft_cluster_hodo_nmin;
  int ft_cluster_nmin;
  int ft_cluster_width;

  int ft_cluster_mult_en;
  int ft_cluster_mult_min;
  int ft_cluster_mult_width;

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
  union
  {
    unsigned int val;
    struct
    {
      unsigned int cmd_fsu        : 1;  //bit 0
      unsigned int cmd_ss         : 1;  //bit 1
      unsigned int cmd_fsb        : 1;  //bit 2
      unsigned int swb_buf_250f   : 1;  //bit 3
      unsigned int swb_buf_500f   : 1;  //bit 4
      unsigned int swb_buf_1p     : 1;  //bit 5
      unsigned int swb_buf_2p     : 1;  //bit 6
      unsigned int ONOFF_ss       : 1;  //bit 7
      unsigned int sw_ss_300f     : 1;  //bit 8
      unsigned int sw_ss_600f     : 1;  //bit 9
      unsigned int sw_ss_1200f    : 1;  //bit 10
      unsigned int EN_ADC         : 1;  //bit 11
      unsigned int H1H2_choice    : 1;  //bit 12
      unsigned int sw_fsu_20f     : 1;  //bit 13
      unsigned int sw_fsu_40f     : 1;  //bit 14
      unsigned int sw_fsu_25k     : 1;  //bit 15
      unsigned int sw_fsu_50k     : 1;  //bit 16
      unsigned int sw_fsu_100k    : 1;  //bit 17
      unsigned int sw_fsb1_50k    : 1;  //bit 18
      unsigned int sw_fsb1_100k   : 1;  //bit 19
      unsigned int sw_fsb1_100f   : 1;  //bit 20
      unsigned int sw_fsb1_50f    : 1;  //bit 21
      unsigned int cmd_fsb_fsu    : 1;  //bit 22
      unsigned int valid_dc_fs    : 1;  //bit 23
      unsigned int sw_fsb2_50k    : 1;  //bit 24
      unsigned int sw_fsb2_100k   : 1;  //bit 25
      unsigned int sw_fsb2_100f   : 1;  //bit 26
      unsigned int sw_fsb2_50f    : 1;  //bit 27
      unsigned int valid_dc_fsb2  : 1;  //bit 28
      unsigned int ENb_tristate   : 1;  //bit 29
      unsigned int polar_discri   : 1;  //bit 30
      unsigned int inv_discriADC  : 1;  //bit 31
    } bits;
  } Global0;

  union
  {
    unsigned int val;
    struct
    {
      unsigned int d1_d2              : 1;  //bit 0
      unsigned int cmd_CK_mux         : 1;  //bit 1
      unsigned int ONOFF_otabg        : 1;  //bit 2
      unsigned int ONOFF_dac          : 1;  //bit 3
      unsigned int small_dac          : 1;  //bit 4
      unsigned int enb_outADC         : 1;  //bit 5
      unsigned int inv_startCmptGray  : 1;  //bit 6
      unsigned int ramp_8bit          : 1;  //bit 7
      unsigned int ramp_10bit         : 1;  //bit 8
      unsigned int Reserved0          : 23;  //bit 9-31
    } bits;
  } Global1;

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
    strigger    strg[8];
    ss_ecal     ecal;
    ss_ecal     pcal;
    ss_htcc     htcc;
    ss_ftof     ftof;
    ss_dc       dc;
    ss_ctof     ctof;
    ss_cnd      cnd;
    ss_ftofpcu  ftofpcu;
    int         gtpif_latency;
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
    int disable_evtbuild;
    int disable_fiber;
  } rich;
  
} SSP_CONF;

/* functions */
void sspInitGlobals();
int sspReadConfigFile(char *filename);
int sspDownloadAll();
int sspConfig(char *fname);
void sspMon(int slot);

#endif
