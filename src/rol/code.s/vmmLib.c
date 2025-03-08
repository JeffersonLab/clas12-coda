
/* vmmLib.c */


/*
after power recycle:

--- read all registers ---
vmmPrintRegisters: Version = 0xA0000001
vmmPrintRegisters: Ctrl = 0x00000000
vmmPrintRegisters: Ctrl2 = 0x00000000
vmmPrintRegisters: Latency = 0x00000000
vmmPrintRegisters: Delay = 0x00000000
vmmPrintRegisters: Busy = 0x00000000
******** LATCH STATUS VALUES ********
vmmPrintRegisters: Status0 = 0xF8000000
vmmPrintRegisters: Phase = 0x00000000
vmmPrintRegisters: Phase status = 0x00000021
vmmPrintRegisters: Phase = 0x00000000
vmmPrintRegisters: Phase status = 0x00000021
vmmPrintRegisters: Cfg memory ctrl = 0x00000000
vmmPrintRegisters: Cfg memory read = 0x00000000
vmmPrintRegisters: Cfg memory write = 0xFFFFFFFF
vmmPrintRegisters: Cfg memory status = 0x00000000
vmmPrintRegisters: Cfg test1 = 0x00000000
vmmPrintRegisters: Cfg test2 = 0x00000000
vmmPrintRegisters: Cfg test3 = 0x00000000
vmmPrintRegisters: Pulser ctrl1 = 0x00013880
vmmPrintRegisters: Pulser ctrl1 = 0x00013880
vmmPrintRegisters: Pulser ctrl2 = 0x00004E20
vmmPrintRegisters: Pulser ctrl3 = 0x00000000
vmmPrintRegisters: Pulser delay = 0x00000000
vmmPrintRegisters: Enable = 0x00000000
vmmPrintRegisters: Time error ctrl = 0x00000000
vmmPrintRegisters: Time error 1 = 0x00000000
vmmPrintRegisters: Time error 2 = 0x00000000
vmmPrintRegisters: Time error 3 = 0x00000000
vmmPrintRegisters: Time error 4 = 0x00000000
vmmPrintRegisters: Time error 5 = 0x00000000
vmmPrintRegisters: Scaler ctrl1 = 0x00000000
vmmPrintRegisters: Scaler ctrl2 = 0x00000000
******** LATCH SCALER VALUES ********
read_scalers: Pulser scaler = 0x00000000
read_scalers: Scaler trigger = 0x00000000
read_scalers: Scaler trailer1 = 0x00000000
read_scalers: Scaler trailer2 = 0x00000000


after prestart:

--- read all registers ---
vmmPrintRegisters: Version = 0xA0000001
vmmPrintRegisters: Ctrl = 0x00000000
vmmPrintRegisters: Ctrl2 = 0x00000104
vmmPrintRegisters: Latency = 0x00000000
vmmPrintRegisters: Delay = 0x0004004E
vmmPrintRegisters: Busy = 0x00010064
******** LATCH STATUS VALUES ********
vmmPrintRegisters: Status0 = 0x38F42EF4
vmmPrintRegisters: Phase = 0x00000003
vmmPrintRegisters: Phase status = 0x00000003
vmmPrintRegisters: Phase = 0x00000003
vmmPrintRegisters: Phase status = 0x00000003
vmmPrintRegisters: Cfg memory ctrl = 0x00000093
vmmPrintRegisters: Cfg memory read = 0x0604F183
vmmPrintRegisters: Cfg memory write = 0xFFFFFFFF
vmmPrintRegisters: Cfg memory status = 0x00000312
vmmPrintRegisters: Cfg test1 = 0x00000000
vmmPrintRegisters: Cfg test2 = 0x00000000
vmmPrintRegisters: Cfg test3 = 0x00000000
vmmPrintRegisters: Pulser ctrl1 = 0x00013880
vmmPrintRegisters: Pulser ctrl1 = 0x00013880
vmmPrintRegisters: Pulser ctrl2 = 0x00004E20
vmmPrintRegisters: Pulser ctrl3 = 0x00000000
vmmPrintRegisters: Pulser delay = 0x00000000
vmmPrintRegisters: Enable = 0x00100600
vmmPrintRegisters: Time error ctrl = 0x00000000
vmmPrintRegisters: Time error 1 = 0x00000000
vmmPrintRegisters: Time error 2 = 0x00000000
vmmPrintRegisters: Time error 3 = 0x00000000
vmmPrintRegisters: Time error 4 = 0x00000000
vmmPrintRegisters: Time error 5 = 0x00000000
vmmPrintRegisters: Scaler ctrl1 = 0x00000000
vmmPrintRegisters: Scaler ctrl2 = 0x00000000
******** LATCH SCALER VALUES ********
read_scalers: Pulser scaler = 0x00000000
read_scalers: Scaler trigger = 0x00000000
read_scalers: Scaler trailer1 = 0x00000000
read_scalers: Scaler trailer2 = 0x00000000


after unplugging/plugging back clock:

--- read all registers ---
vmmPrintRegisters: Version = 0xA0000001
vmmPrintRegisters: Ctrl = 0x00000000
vmmPrintRegisters: Ctrl2 = 0x00000104
vmmPrintRegisters: Latency = 0x00000000
vmmPrintRegisters: Delay = 0x0004004E
vmmPrintRegisters: Busy = 0x00010064
******** LATCH STATUS VALUES ********
vmmPrintRegisters: Status0 = 0xF88367E8
vmmPrintRegisters: Phase = 0x00000003
vmmPrintRegisters: Phase status = 0x00000001
vmmPrintRegisters: Phase = 0x00000003
vmmPrintRegisters: Phase status = 0x00000001
vmmPrintRegisters: Cfg memory ctrl = 0x00000093
vmmPrintRegisters: Cfg memory read = 0x0604F183
vmmPrintRegisters: Cfg memory write = 0xFFFFFFFF
vmmPrintRegisters: Cfg memory status = 0x00000312
vmmPrintRegisters: Cfg test1 = 0x00000000
vmmPrintRegisters: Cfg test2 = 0x00000000
vmmPrintRegisters: Cfg test3 = 0x00000000
vmmPrintRegisters: Pulser ctrl1 = 0x00013880
vmmPrintRegisters: Pulser ctrl1 = 0x00013880
vmmPrintRegisters: Pulser ctrl2 = 0x00004E20
vmmPrintRegisters: Pulser ctrl3 = 0x00000000
vmmPrintRegisters: Pulser delay = 0x00000000
vmmPrintRegisters: Enable = 0x00100600
vmmPrintRegisters: Time error ctrl = 0x00000000
vmmPrintRegisters: Time error 1 = 0x00000000
vmmPrintRegisters: Time error 2 = 0x00000000
vmmPrintRegisters: Time error 3 = 0x00000000
vmmPrintRegisters: Time error 4 = 0x00000000
vmmPrintRegisters: Time error 5 = 0x00000000
vmmPrintRegisters: Scaler ctrl1 = 0x00000000
vmmPrintRegisters: Scaler ctrl2 = 0x00000000
******** LATCH SCALER VALUES ********
read_scalers: Pulser scaler = 0x00000000
read_scalers: Scaler trigger = 0x00000000
read_scalers: Scaler trailer1 = 0x00000000
read_scalers: Scaler trailer2 = 0x00000000


*/



#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "vmmLib.h"
#include "fpga_io.h"

static void read_scalers(int mode);
static int read_status(void);
static void configure_VMMs(int reset_long, int reset_2x);
static void make_mem_array(int chip, int *mem_word);
static void set_clock_phases(int phase_stat1_save);            


#define EVENT_DATA_PRINT          0
#define EVENT_STAT_PRINT          0


static unsigned long long nwords_current = 0;
static unsigned long long nwords_total = 0;
static unsigned long long nevents_current = 0;
static unsigned long long nevents_total = 0;


struct timeval t_last, t_cur;
static int scaler_group;
static int use_pulser_hits = 0;

static int ENA1 = 0;
static int ENA2 = 0;
static int ENA_CKBC = 0;
static int ENA_CKDT = 0;
static int ENA_CK6B = 0;
static int ENA_TRIG = 0;
static int BCR_mode = 0;
static int ENABLE_TP = 1;



/* trigger_source:
        0 = external
        1 = external delayed (if sending triggers from AUX_OUT to TI's TS#, and receiving it back to TRIG_IN) 
        2 = internal pulser delayed (if running alone)
 */
static int trigger_source = 0/*1*/;

/* cktp_source - pulse source:
        0 = NONE
        1 = external trigger
        2 = internal pulser
*/
static int cktp_source = 0/*1*/;

// for cktp_source = 2, trigger_source=2, have data between 72 and 79 -> use 76
// for cktp_source = 2, trigger_source=1, have data between 65 and 72 -> use 69
// for cktp_source = 1, trigger_source=1, have data between 74 and 81 -> use 78
static int trig_delay_value = 78; // latency !!! trigger delay count ('BC_period_ns'=24ns per count) (0 - 4095)


static int trig_width_value = 4;
static int scaler_mode = 0;	   // direct output mode used for scalers (0 = 6-bit ADC, 1 = time pulse)
static int sync_source = 0;	   // source of SYNC_RESET_signal (0 = external, 1 = internal)
static int lost_lock_occurred = 0; // if zero, adjustment of the data capture clock phase will be performed; IT HAVE TO BE DONE ONLY ONCE - IMPORTANT !!!








/****************************/
/*FROM vmm_crate_cfg_file_L0*/

#define ENABLED  1
#define DISABLED 0
#define NEGATIVE 0

//VMMs reset control: double long hard reset
static int reset_long = 1;
static int reset_2x = 1;

//static int mem_word[54] = {54*0};       // work cfg memory words
static int mem_word_VMM1[54] = {54*0};  // cfg memory words for chip 1
static int mem_word_VMM2[54] = {54*0};  // cfg memory words for chip 2

/********************/
/* global_registers */

static int sdp_dac = 250;		// sdp9-sdp0 [0:0 through 1:1] test pulse DAC (10 bits)	<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
static int sdt = 250;			// sdt9-sdt0 [0:0 through 1:1] coarse threshold DAC (10 bits) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
//shaping time:
static int st = 3;			// st1,st0 [00 01 10 11] peaktime (2 bits) [00 01 10 11] = [ 200, 100, 50, 25 ns ] <<<<<<<<<<
static int sm5 = 0;			// monitor multiplexing: Common monitor: scmx, sm5-sm0 [0 000001 to 000100]
							// 	pulser DAC (after pulser switch), threshold DAC, bandgap reference, temperature sensor)
static int scmx = ENABLED;		// channel monitor: scmx, sm5-sm0 [1 000000 to 111111], channels 0 to 63
static int sbmx = DISABLED;		// routes analog monitor to PDO output
static int slg = ENABLED;		// leakage current disable ([0] enabled)
static int sp = NEGATIVE;		// input charge polarity ([0] negative, [1] positive)
static int sng = DISABLED;		// neighbor (channel and chip) triggering enable
static int sdrv = DISABLED;		// tristates analog outputs with token, used in analog mode
static int sg = 5;			// sg2,sg1,sg0 [000:111]  gain (0.5, 1, 3, 4.5, 6, 9, 12, 16 mV/fC) <<<<<<<<<<<<<<<<<<<<<<<<<
static int stc = 0;			// stc1,stc0 [00 01 10 11]  TAC slope adjustment (60, 100, 350, 650 ns ) <<<<<<<<<<<<<<<<<<<<
static int sfm = ENABLED;		// sfm [0 1] enables full-mirror (AC) and high-leakage operation (enables SLH) <<<<<<<<<<<<<<
static int ssh = DISABLED;		// ssh [0 1] enables sub-hysteresis discrimination
static int sdp = DISABLED;		// disable-at-peak
static int sbft = DISABLED;		// sbft [0 1], sbfp [0 1], sbfm [0 1] analog output buffers, [1] enable (TDO, PDO, MO)
static int sbfp = DISABLED;		//
static int sbfm = ENABLED;		// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
static int s6b = ENABLED;		// enables 6-bit ADC (requires sttt enabled) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
static int s8b = ENABLED;		// enables 8-bit ADC <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
static int s10b = ENABLED;		// enables high resolution ADCs (10/8-bit ADC enable)  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
static int sttt = ENABLED;		// sttt [0 1] enables direct-output logic (both timing and s6b) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
static int stpp = 0;			// timing outputs control 1 (s6b must be disabled)
static int stot = 1; 			// stpp,stot[00,01,10,11]: TtP,ToT,PtP,PtT
							// TtP: threshold-to-peak
							// ToT: time-over-threshold
							// PtP: pulse-at-peak (10ns) (not available with s10b)
							// PtT: peak-to-threshold (not available with s10b)

static int sfa = DISABLED;		// ART enable: sfa [0 1], sfam [0 1] (sfa [1]) and mode (sfam [0] timing at threshold,[1] timing at peak)
static int sfam = 0;			//
static int sc10b = 3;			// sc010b,sc110b 10-bit ADC conv. time (increase subtracts 60 ns) <<<<<<<<<<<<<<<<<<<<<<<<<<<<
static int sc8b = 3;			// sc08b,sc18b 8-bit ADC conv. time (increase subtracts 60 ns) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
static int sc6b = 2;			// sc06b, sc16b, sc26b 6-bit ADC conversion time <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
static int sdcks = DISABLED;		// dual clock edge serialized data enable
static int sdcka = DISABLED;		// dual clock edge serialized ART enable
static int sdck6b = DISABLED;		// dual clock edge serialized 6-bit enable
static int slvs = DISABLED;		// enables direct output IOs <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
static int stlc = DISABLED;		// enables mild tail cancellation (when enabled, overrides sbip)
static int stcr = DISABLED;		// enables auto-reset (at the end of the ramp, if no stop occurs) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
static int ssart = DISABLED;		// enables ART flag synchronization (trail to next trail)
static int srec = DISABLED;		// enables fast recovery from high charge
static int sfrst = DISABLED;		// enables fast reset at 6-b completion <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
static int s32 = DISABLED;		// skips channels 16-47 and makes 15 and 48 neighbors
static int sbip = ENABLED;		// enables bipolar shape (!!!!!!!! ALWAYS ENABLE THIS !!!!!!!!)
static int srat = DISABLED;		// enables timing ramp at threshold <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
static int slvsbc = DISABLED;		// enable slvs 100Ω termination on ckbc
static int slvsart = DISABLED;		// enable slvs 100Ω termination on ckart
static int slvstp = DISABLED;		// enable slvs 100Ω termination on cktp
static int slvstki = DISABLED;		// enable slvs 100Ω termination on cktki
static int slvstk = DISABLED;		// enable slvs 100Ω termination on cktk
static int slvsena = DISABLED;		// enable slvs 100Ω termination on ckena
static int slvsdt = DISABLED;		// enable slvs 100Ω termination on ckdt
static int slvs6b = DISABLED;		// enable slvs 100Ω termination on ck6b
		
static int sL0ena = ENABLED;		// enable L0 core / reset core & gate clk if 0 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
static int sL0enaV = ENABLED;		// disable mixed signal functions when L0 enabled <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
static int l0offset = 4015;		// l0offset i0:11 L0 BC offset - TRIGGER LATENCY - 25 ns/count <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< 
static int offset = 0;			// offset i0:11 Channel tagging BC offset
static int rollover = 4095;		// rollover i0:11 Channel tagging BC rollover
static int window = 7;			// window i0:2 Size of trigger window - TRIGGER WINDOW - 25 ns * (count+1) <<<<<<<<<<<<<<<<<<<<
static int truncated = 0;		// truncated i0:5 Max hits per L0 (0 = NO truncation) ***************************** !!!!!!!!!!!!
static int nskip = 0;			// nskip i0:6 Number of L0 triggers to skip on overflow
static int sL0cktest = DISABLED;	// enable clocks when L0 core disabled (test)
static int sL0ckinv = DISABLED;	// invert BCCLK  *****************************************************************
static int sL0dckinv = DISABLED;	// invert DCK
static int nskipm = DISABLED;		// magic number on BCID - 0xFE8 (4072)
static int res00 = 0;			// ????? reserved?
static int res0 = 0;
static int res1 = 0;
static int res2 = 0;
static int res3 = 0;
static int reset = 0;
static int slh = 0;
static int slxh = 0;
static int stgc = 0;
		

/*********************/				
/* channel_registers */

		// channel_register_mask  ?????
static int sc_ch = 0;		// sc [0 1] large sensor capacitance mode ([0] <∼200 pF , [1] >∼200 pF ) 
static int sl = 0;  		// sl [0 1] leakage current disable [0=enabled]
static int sth = 0; 		// sth [0 1] multiplies test capacitor by 10
static int smx = 0; 		// smx [0 1] channel monitor mode ( [0] analog output, [1] trimmed threshold))
static int sz010b = 0; 	// ????? sz010b, sz110b, sz210b, sz310b, sz410b 10-bit ADC offset subtraction
static int sz08b = 0;  	// ????? sz08b, sz18b, sz28b, sz38b 8-bit ADC offset subtraction
static int sz06b = 0;  	// ????? sz06b, sz16b, sz26b 6-bit ADC offset subtraction

// --------------------------------------------------------------------------------------------------------
// sm_ch - Channel mask enable (1 = channel OFF)                                  
//
//                             ch 0-7            8-15              16-23             24-31        
static int sm_ch[64] = { 0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  
//                             ch 32-39           40-47             48-55             56-63         
			 0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0   };	// NONE OFF

// --------------------------------------------------------------------------------------------------------
// st_ch - Test pulse enable = 1 
                                  
//                             ch 0-7            8-15              16-23             24-31        
static int st_ch[64] = { 0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  	//
//                             ch 32-39           40-47             48-55             56-63         
			 0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0   };	// pulse NONE
                                  
// --------------------------------------------------------------------------------------------------------
// sd0-sd4 [0:0 through 1:1] trim threshold DAC, 1mV step ([0:0] trim 0V , [1:1] trim -29mV ) 
                                  
//                             ch 0-7            8-15              16-23             24-31         
static int sd_ch[64] = { 0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  
//                             ch 32-39           40-47             48-55             56-63        
			 0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0   };


static int sd_ch_1[64] = {0};
static int sd_ch_2[64] = {0};





/*FROM vmm_crate_cfg_file_L0*/
/****************************/















/* internal pulser frequency in Hz */
static int pulser_freq = 10000;

int num_triggers = 1000000000;	// number of triggers in run - entered interactively

static char *clonparms;

static void
event_stat_print(double diff)
{
  printf("Average bytes/sec     = %lf\n", (double)(nwords_current*4L)/diff);
  printf("Total bytes received  = %lf\n", (double)(nwords_total*4L)/diff);
  printf("Average events/sec    = %lf\n", (double)nevents_current/diff);
  printf("Total events received = %lf\n", (double)nevents_total/diff);
  printf("\n");

  nwords_total+= nwords_current;
  nevents_total+= nevents_current;

  nwords_current = 0;
  nevents_current = 0;
}

	
static int
process_buf(int *buf, int nwords)
{
  static int tag;
  int ii, nn, word0, word1, nw, ev, P, R, T, N, relBCID, channel, adc, tdc, BCID, V, orb, BIT0;
  int nevents = 0;

  printf("\n================= process_buf reached, nwords = %d\n\n",nwords);
  nn = 0;
  while(nn<nwords)
  {
    word0 = buf[nn++];
    printf("DATA[%3d] = 0x%08x\n",nn,word0);

    tag = (word0>>28) & 0xF;
    //printf("  TAG = %d\n",tag);

    switch(tag)
    {
    case 0:
      ev = word0&0xFFFFFFF;
      printf("  Event Header: event number = %d\n",ev);
      break;

    case 1:
      printf("  Trigger Time\n");
      word1 = buf[nn++];
      if( ((word1>>28)&0xF)!=2 ) printf("ERROR in Trigger Time\n");
      printf("    Timestamp = 0x%06x%06x, phase count = %d\n",word1&0xFFFFFF,word0&0xFFFFFF,(word0>>26)&0x3);
      break;

    case 3:
      nw = (word0>>16)&0xFF;

      BCID = word0&0xFFF;
      orb = (word0>>12)&0x3;
      P = (word0>>14)&0x1;
      V = (word0>>15)&0x1;

      printf("  VMM1 Chip Header: %d hits, chip trigger header = 0x%04x (BCID=%d, orb=%d, P=%d, V=%d)\n",nw,word0&0xFFFF,BCID,orb,P,V);
      for(ii=0; ii<nw; ii++)
      {
        word1 = buf[nn++];

        relBCID = word1&0x7;
        N = (word1>>3)&0x1;
        tdc = (word1>>4)&0xFF;
        adc = (word1>>12)&0x3FF;
        channel = (word1>>22)&0x3F;
        T = (word1>>28)&0x1;
        R = (word1>>29)&0x1;
        P = (word1>>30)&0x1;
        BIT0 = (word1>>31)&0x1;

	printf("    Hit[%2d] = 0x%08x (channel = %3d, adc = %3d, tdc = %3d) (relBCID=%1d T=%1d R=%1d P=%1d N=%1d) (bit0=%1d)\n",ii,word1,channel,adc,tdc,relBCID,T,R,P,N,BIT0);
      }
      break;

    case 4:
      printf("  VMM1 Chip Trailer: chip trigger number = %d\n",word0&0xFFFFFFF);
      break;

    case 5:
      nw = (word0>>16)&0xFF;

      BCID = word0&0xFFF;
      orb = (word0>>12)&0x3;
      P = (word0>>14)&0x1;
      V = (word0>>15)&0x1;

      printf("  VMM2 Chip Header: %d hits, chip trigger header = 0x%04x (BCID=%d, orb=%d, P=%d, V=%d)\n",nw,word0&0xFFFF,BCID,orb,P,V);
      for(ii=0; ii<nw; ii++)
      {
        word1 = buf[nn++];

        relBCID = word1&0x7;
        N = (word1>>3)&0x1;
        tdc = (word1>>4)&0xFF;
        adc = (word1>>12)&0x3FF;
        channel = (word1>>22)&0x3F;
        T = (word1>>28)&0x1;
        R = (word1>>29)&0x1;
        P = (word1>>30)&0x1;
        BIT0 = (word1>>31)&0x1;

	printf("    Hit[%2d] = 0x%08x (channel = %3d, adc = %3d, tdc = %3d) (relBCID=%1d T=%1d R=%1d P=%1d N=%1d) (bit0=%1d)\n",ii,word1,channel,adc,tdc,relBCID,T,R,P,N,BIT0);
      }
      break;

    case 6:
      printf("  VMM2 Chip Trailer: chip trigger number = %d\n",word0&0xFFFFFFF);
      break;

    default:
      printf("ERROR: unknown data\n");
    }
  }

  printf("\n================= process_buf done\n\n");

  return(nevents);
}


void ppp(int ii)
{
  int val;
  val = fpga_read32(&pFPGA_regs->Clk.Phase_stat1);
  printf("\n --[%2d]--> ***** Clk.Phase_stat1 = 0x%08X) *****\n\n",ii,val); 		
}


int
vmmInit()
{
  int busy_period = 0;
  int busy_time = 0;
  int delay_reg_value;
  int period_count, period_low_count, period_high_count;
  float period, period_low;
  int pulser_delay = 0;	   // CKTP delay 
  int val;
  int pulser_start = 0;
  int vmm_select = 0;
  float BC_frequency, BC_period, BC_period_ns;
  int value = 0;
  int phase_stat1_save; 

  val = open_register_socket();
  if(val<=0)
  {
    printf("ERROR in open_register_socket() - return\n");
    return(-1);
  }

ppp(1);

  /* read board ID */
  val = fpga_read32(&pFPGA_regs->Clk.BoardId);
  printf("%s: Version = 0x%08X\n", __func__, val);

  ENA1 = 0;
  ENA2 = 0;
  ENA_CKBC = 0;
  ENA_CKDT = 0;
  ENA_CK6B = 0;
  ENA_TRIG = 0;
  val = (ENA_CK6B << 20) | (ENA_TRIG << 16) | (ENA_CKDT << 10) | (ENA_CKBC << 9) | (ENA2 << 1) | ENA1; 
ppp(2);
  fpga_write32(&pFPGA_regs->Clk.Enable, val);
ppp(3);

  printf("\n +++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  printf("\n +++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  printf("\n ++++++ BE SURE CLOCK 41.66667 MHz IS APPLIED ++++++\n");
  printf("\n +++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  printf("\n +++++++++++++++++++++++++++++++++++++++++++++++++++\n");



  /* read and remember 'phase_stat1' register - it will be reset by writing 1 to Ctrl(1) (following command) */
  phase_stat1_save = fpga_read32(&pFPGA_regs->Clk.Phase_stat1);
  printf("\n ----> ***** Clk.Phase_stat1 = 0x%08X) *****\n\n",phase_stat1_save); 		

  printf("\n******** RESET CLOCK GENERATOR ********\n");
  fpga_write32(&pFPGA_regs->Clk.Ctrl, 0x00000002);	  // RESET CLOCK GENERATOR - write Ctrl(1) = 1
  sleep(1);
ppp(4);
  fpga_write32(&pFPGA_regs->Clk.Ctrl, 0x00000000);	  // CLEAR RESET - write Ctrl(1) = 0
  sleep(1);
ppp(5);
  fpga_write32(&pFPGA_regs->Clk.Ctrl, 0x02000000);	  // CLEAR latched lost lock bit - write Ctrl(25) = 1
  sleep(1);
ppp(6);
   	    
  printf("\n******** CHECK STATUS OF GENERATED CLOCKS ********\n");
  lost_lock_occurred = read_status();			  // check status of lock
  if(lost_lock_occurred == 0)
  {
    printf("\n******** CLOCKS LOCKED - continue ********\n");
    set_clock_phases(phase_stat1_save); 	// Set clock phases if NOT doing phase a shift study
  }
  else
  {
    printf("\n\n!!!!!!!! EXIT - CLOCKS NOT LOCKED 1 !!!!!!!!\n\n");
    return(-1);
  }

  printf("----------------------------------\n");
  printf("--- do reset ---\n");
  fpga_write32(&pFPGA_regs->Clk.Ctrl, 0x80000000);	  // reset - Ctrl(31) = 1
  sleep(1);
  fpga_write32(&pFPGA_regs->Clk.Ctrl, 0x40000000);	  // soft bus reset - Ctrl(31) = 1 (necessary?)
  printf("----------------------------------\n");
  sleep(1);

  printf("\n***** Program START - ENABLE REG = 0 *****\n");
  val = fpga_read32(&pFPGA_regs->Clk.Enable);

  printf("\n--- VMM main data (10-bit ADC) + L0 trigger ---\n");

  vmm_select = 0;	// currently firmware reads out both VMM #1 and VMM #2

  scaler_mode = 1;		// for DIRECT timing mode
  printf("\n(DIRECT OUTPUTS may be enabled in VMM cfg file for scalers.)\n");
	
  scaler_mode = 0;		// DIRECT ADC mode is assumed
	    
	   
  printf("\n--------------------------------------------------------------------\n");
  printf("   IF the INTERNAL pulse generator is used the pulse will also be \n"); 
  printf("   driven from the AUX output.  This MAY be connected to the EXTERNAL \n");
  printf("   trigger input via cable or jumpers.\n");
  printf("----------------------------------------------------------------------\n\n");

  if( (trigger_source < 0) || (trigger_source > 2) )
  {
    trigger_source = 2;
    printf("!!!!!!!!!! Incorrect trigger source entered - will use INTERNAL pulser delayed !!!!!!!!!!\n");
  }
  if( trigger_source == 0 ) 
  {
    printf("TRIGGER source is EXTERNAL\n");
  }
  if( trigger_source == 1 ) 
  {
    printf("TRIGGER source is EXTERNAL delayed\n");
  }
  if( trigger_source == 2 ) 
  {
    printf("TRIGGER source is INTERNAL pulser delayed\n");
    fpga_write32(&pFPGA_regs->Clk.Double_pulse, 0);  	// NO double pulse
  }


  if( BC_CLOCK_FREQUENCY_MODE == 1 )
  {
    BC_frequency = 40.00e6;
  }
  else if( BC_CLOCK_FREQUENCY_MODE == 2 )
  {
    BC_frequency = 41.66667e6;
  }
  else
  {
    printf("ERROR: wrong BC clock frequency mode, can be 1 or 2\n");
    return(-1);
  }
  BC_period = 1.0/(BC_frequency);
  BC_period_ns = BC_period * 1e9;

  printf("\n===> BC Frequency = %.5f MHz   BC Period = %.3f ns\n", (BC_frequency/1.0e6), BC_period_ns );
  printf("===> Trigger delay = %d (%8.2f ns)\n\n", trig_delay_value, (trig_delay_value * BC_period_ns));


  /*pulser setting*/
  if( (cktp_source < 0) || (cktp_source > 2) )
  {
    cktp_source = 2;
    printf("!!!!!!!!!! Incorrect CKTP source entered - use internal pulser !!!!!!!!!!\n");
  }
  pulser_start = 0;
  if( cktp_source == 0 ) 
  {
    printf("CKTP source is NONE\n");
  }
  if( cktp_source == 1 ) 
  {
    printf("CKTP source is EXTERNAL trigger\n");
  }
  if( cktp_source == 2 ) 
  {
    printf("CKTP source is INTERNAL pulser\n");
  }

  sync_source = 1; // SYNC_RESET source (0 = external, 1 = internal)
  if( (sync_source < 0) || (sync_source > 1) )
  {
    sync_source = 1;
    printf("!!!!!!!!!! Incorrect source of SYNC_RESET entered - use internal source !!!!!!!!!!\n");
  }
	    	
  BCR_mode = 1;	  // BCR only generated at trailing edge of SYNC_RESET - NO OCR
	    	
  value = (BCR_mode << 8) | (pulser_start << 7) | (vmm_select << 6) | (cktp_source << 4) | (sync_source << 2) | trigger_source;
  fpga_write32(&pFPGA_regs->Clk.Ctrl2, value);	    

  if( (trigger_source == 2) || (cktp_source == 2) )
  {
    ENABLE_TP = 1;
    printf("Pulser frequency = %d\n", pulser_freq);

// program pulse generator
    period = 1.0/((float)pulser_freq);			// period in sec
    period_count = (int)(period/BC_period);			// period in BC_period units (BC clock)
    period_low = 0.75*period;					// 25% duty factor trigger signal
    period_low_count = (int)(period_low/BC_period);		// period_low in BC_period units (BC clock)
    period_high_count = period_count - period_low_count;	// period_low in BC_period units (BC clock)

//      printf("Period count =  %d   period high count = %d   period low count = %d\n", 
//	      period_count, period_high_count, period_low_count );
	        			
    fpga_write32(&pFPGA_regs->Clk.Test_pulse3, 0);			// zero register
    fpga_write32(&pFPGA_regs->Clk.Test_pulse1, period_high_count);	// period high
    fpga_write32(&pFPGA_regs->Clk.Test_pulse2, period_low_count);	// period low
    fpga_write32(&pFPGA_regs->Clk.Test_pulse4, pulser_delay);	// pulser delay  		
  }
     	 
//----------------------------------------------------------------------------------------------------------------------------------
// ---------------- Busy option for trigger - all sources --------------------------------------------------------------------------
  busy_time = 0; 	
  busy_period = 100; // busy time in counts ( 'BC_period_ns/4.0' ns each, max = 65535)
  busy_time = 0x10000 | (0xFFFF & busy_period);
   
  fpga_write32(&pFPGA_regs->Clk.Busy, busy_time);  

// do not enable clocks to VMMs yet
  ENA1 = 0;
  ENA2 = 0;
  ENA_CKBC = 0;
  ENA_CKDT = 0;
  ENA_CK6B = 0;
  ENA_TRIG = 0;
  val = (ENA_CK6B << 20) | (ENA_TRIG << 16) | (ENA_CKDT << 10) | (ENA_CKBC << 9) | (ENA2 << 1) | ENA1; 
  fpga_write32(&pFPGA_regs->Clk.Enable, val);   	

// +++++++++++++++++++++++++++++++++++++++++++++++++++++
  printf("\n***** Trigger mode selected, clocks to VMMs NOT enabled yet *****\n");  
  val = fpga_read32(&pFPGA_regs->Clk.Enable);
//  printf("%s: Enable = 0x%08X\n", __func__, val);
  

// write trigger delay and width parameters - not used for external trigger source
  delay_reg_value = ((0xFF & trig_width_value) << 16) | (0xFFF & trig_delay_value); 
  fpga_write32(&pFPGA_regs->Clk.Delay, delay_reg_value); 

//  printf("--- load latency, trig delay value into memory ---\n");
  fpga_write32(&pFPGA_regs->Clk.Ctrl, 0x20000000);	 // load latency, trig delay
 
  sleep(3);                                              // wait 

// ------------------------------------------------------
// ------------------------------------------------------
// Configure VMMs
  configure_VMMs(reset_long, reset_2x);
// ------------------------------------------------------
// ------------------------------------------------------

  sleep(3);                                             // wait 
  

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++
  // enable clocks to VMMs
  ENA1 = 0;
  ENA2 = 0;
  ENA_CKBC = 1;
  ENA_CKDT = 1;
  ENA_CK6B = 1;
  ENA_TRIG = 0;
  val = (ENA_CK6B << 20) | (ENA_TRIG << 16) | (ENA_CKDT << 10) | (ENA_CKBC << 9) | (ENA2 << 1) | ENA1; 
  fpga_write32(&pFPGA_regs->Clk.Enable, val);   	

  // +++++++++++++++++++++++++++++++++++++++++++++++++++++
  printf("\n***** clocks enabled to VMMs *****\n");  
  val = fpga_read32(&pFPGA_regs->Clk.Enable);
  //  printf("%s: Enable = 0x%08X\n", __func__, val);
  
  // reset latched timing errors
  fpga_write32(&pFPGA_regs->Clk.Time_ctrl, 0x80000000);	 // writing bit 31 resets latched errors 

  printf("\n***** Latched timing errors reseted *****\n");  

  return(0);
}


int
vmmEnable()
{
  int val;
  int value; 
  int pulser_start = 0;
  //int no_ck6b = 0;
  //int ENABLE_TRIG = 1;
  //int ENABLE_BC = 1;    // enable CKBC now since triggers may be live
  //int TKI = 1;		// initial token in	    
  //int ENABLE_TK = 1;	// enable CKTK




  sleep(1);
  

  val = open_event_socket();
  if(val<=0) return(-1);


// ---------------------------------------------------------------------------------
// enable VMMs for acquisition
  ENA1 = 1;
  ENA2 = 1;
  ENA_CKBC = 1;
  ENA_CKDT = 1;
  ENA_CK6B = 1;
  ENA_TRIG = 0;
  val = (ENA_CK6B << 20) | (ENA_TRIG << 16) | (ENA_CKDT << 10) | (ENA_CKBC << 9) | (ENA2 << 1) | ENA1; 
  fpga_write32(&pFPGA_regs->Clk.Enable, val);   	

  printf("\n***** enable VMMs for acquisition *****\n");  

  val = fpga_read32(&pFPGA_regs->Clk.Enable);
//  printf("%s: Enable = 0x%08X\n", __func__, val);

  gettimeofday(&t_last, NULL);

  printf("\n******** CHECK STATUS OF GENERATED CLOCKS ********\n");

  lost_lock_occurred = read_status();			  // check status of lock
  if(lost_lock_occurred == 0)
  {
    printf("\n******** CLOCKS LOCKED - continue ********\n");
  }
  else
  {
    printf("\n\n!!!!!!!! EXIT - CLOCKS NOT LOCKED 2 !!!!!!!!\n\n");
   // re-initialize before EXIT
    ENA1 = 0;
    ENA2 = 0;
    ENA_CKBC = 0;
    ENA_CKDT = 0;
    ENA_CK6B = 0;
    ENA_TRIG = 0;
    val = (ENA_CK6B << 20) | (ENA_TRIG << 16) | (ENA_CKDT << 10) | (ENA_CKBC << 9) | (ENA2 << 1) | ENA1; 
    fpga_write32(&pFPGA_regs->Clk.Enable, val);
        	            
    return(-1);
  }
  

// ------------------------------------------------------------------------------------------------
  printf("\n--- GO internal data transfers ---\n");
  fpga_write32(&pFPGA_regs->Clk.Ctrl, 1);	  	  // GO 
  val = fpga_read32(&pFPGA_regs->Clk.Ctrl);
//  printf("%s: Ctrl = 0x%08X\n", __func__, val);
      
// ------------------------------------------------------------------------------------------------     
  printf("\n--- reset scalers - enable counting ---\n");
  fpga_write32(&pFPGA_regs->Clk.Scaler_ctrl2, 0x1);   	// reset scalers
  val = 0x10000 | (scaler_mode << 15) ;			// enable counting for run
  fpga_write32(&pFPGA_regs->Clk.Scaler_ctrl1, val );          

// ------------------------------------------------------------------------------------------------     
  if( (trigger_source == 2) || (cktp_source == 2) )
  {
    val = num_triggers;  
    fpga_write32(&pFPGA_regs->Clk.Test_pulse3, val);  // write number of pulses to generate
      
    pulser_start = 1; //Pulser start mode: '0' starts on register write, '1' starts on SYNC_RESET
    if( pulser_start ) pulser_start = 1;
           
    val = fpga_read32(&pFPGA_regs->Clk.Ctrl2);	// write bit in CTR2 register 
    value = val | (pulser_start << 7);
    fpga_write32(&pFPGA_regs->Clk.Ctrl2, value);
  }	    
      
// -----------------------------------------------------------------------------------------------        
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// enable triggers
  ENA1 = 1;
  ENA2 = 1;
  ENA_CKBC = 1;
  ENA_CKDT = 1;
  ENA_CK6B = 1;
  ENA_TRIG = 1;
  val = (ENA_CK6B << 20) | (ENA_TRIG << 16) | (ENA_CKDT << 10) | (ENA_CKBC << 9) | (ENA2 << 1) | ENA1; 
  fpga_write32(&pFPGA_regs->Clk.Enable, val);
         	
  printf("\n***** enable triggers *****\n");  
  val = fpga_read32(&pFPGA_regs->Clk.Enable);
  printf("%s: Enable = 0x%08X\n", __func__, val);

// -----------------------------------------------------------------------------------------------        
// for internal SYNC_RESET
  if( sync_source )	
  {
    if( pulser_start )				// pulser enabled to start on SYNC_RESET
    {							          
      fpga_write32(&pFPGA_regs->Clk.Ctrl, 0x04000001);        // pulse SYNC_RESET with GO asserted
      printf("\n***** pulser started on issuance of SYNC_RESET *****\n\n");  
    }
    else						// pulser started by CTRL register write  
    {							          // pulser started by CTRL register write
      fpga_write32(&pFPGA_regs->Clk.Ctrl, 0x04000001);        // pulse SYNC_RESET with GO asserted
      fpga_write32(&pFPGA_regs->Clk.Ctrl, 0x08000001);        // pulse START_PULSER with GO asserted
      printf("\n***** SYNC_RESET issued and pulser started *****\n\n");  
    }
  }

  return(0);
}




int
vmmReadBlock(uint32_t *data, int maxwords)
{
  int len, nw, status, j, word;
  double diff;

  len = read_event_socket(data, maxwords);
  if(len > 0)
  {
    nwords_current += len;
#if EVENT_DATA_PRINT
    nevents_current += process_buf(data, len);
#endif
  }
  else
  {
    /*usleep(1000)*/;
  }

  gettimeofday(&t_cur, NULL);
  diff = t_cur.tv_sec + 1.0E-6 * t_cur.tv_usec - t_last.tv_sec - 1.0E-6 * t_last.tv_usec;
  if(diff > 1.0)
  {
#if EVENT_STAT_PRINT
    event_stat_print(diff);
#endif

    gettimeofday(&t_last, NULL);
  }

  return(len);
}


int
vmmDisable()
{
  int j, val, val1, val2, val3, val4, val5;
  float scaler_interval;

  printf("\n\n=== vmmDisable() reached\n");fflush(stdout);

  fpga_write32(&pFPGA_regs->Clk.Scaler_ctrl2, 0x2);     // latch all scalers into registers
 printf("vmmDisable 1\n");fflush(stdout);
  val = fpga_read32(&pFPGA_regs->Clk.Scaler_trigger);

  printf("\nTrigger count = %d\n", val);fflush(stdout);
  printf("\n******** DISABLE TRIGGERS - requested number reached ********\n\n");
  ENA1 = 1;		// leave VMMs in acquisition mode 
  ENA2 = 1;
  ENA_CKBC = 1;		// all clocks running
  ENA_CKDT = 1;

  ENA_CK6B = 1;
  ENA_TRIG = 0;
  val = (ENA_CK6B << 20) | (ENA_TRIG << 16) | (ENA_CKDT << 10) | (ENA_CKBC << 9) | (ENA2 << 1) | ENA1; 
  fpga_write32(&pFPGA_regs->Clk.Enable, val);   	

  val = fpga_read32(&pFPGA_regs->Clk.Test_scaler);	// read test pulse scaler
  printf("Test pulse count = %d\n\n", val);

  val = fpga_read32(&pFPGA_regs->Clk.Scaler_trailer1);	// read trailer 1 scaler
  printf("VMM 1 trailer count = %d\n\n", val);
      
  val = fpga_read32(&pFPGA_regs->Clk.Scaler_trailer2);	// read trailer 2 scaler
  printf("VMM 2 trailer count = %d\n\n", val);

  val1 = fpga_read32(&pFPGA_regs->Clk.Time_err1);
  val2 = fpga_read32(&pFPGA_regs->Clk.Time_err2);
  val3 = fpga_read32(&pFPGA_regs->Clk.Time_err3);
  val4 = fpga_read32(&pFPGA_regs->Clk.Time_err4);
  val5 = fpga_read32(&pFPGA_regs->Clk.Time_err5);
  printf("Time error (D1_1,D0_1,D1_2,D0_2, ch 127-0): %08X    %08X  %08X  %08X  %08X\n\n", val5, val4, val3, val2, val1);
	
  fpga_write32(&pFPGA_regs->Clk.Scaler_ctrl1, 4 );   // disable all counting
  fpga_write32(&pFPGA_regs->Clk.Scaler_ctrl2, 0x2 ); // latch all scalers into registers

  for(j=0;j<128;j++)		// read 128 direct output scalers
  {
    val = j;
    fpga_write32(&pFPGA_regs->Clk.Scaler_ctrl1, val ); 	// select channel
    val = fpga_read32(&pFPGA_regs->Clk.Scaler_data);
    //sergey printf("Channel %d : count = %d\n", j, val);
  }
  val = fpga_read32(&pFPGA_regs->Clk.Scaler_time);
  scaler_interval = (float)((val * 3.2E-6));
  printf("\nTime count = %d  (%.6f sec) \n", val, scaler_interval);

  printf("\n******** re-initialize before EXIT ********\n\n");
  ENA1 = 0;		
  ENA2 = 0;
  ENA_CKBC = 0;	  // remove clocks from VMM
  ENA_CKDT = 0;
  ENA_CK6B = 0;
  ENA_TRIG = 0;
  val = (ENA_CK6B << 20) | (ENA_TRIG << 16) | (ENA_CKDT << 10) | (ENA_CKBC << 9) | (ENA2 << 1) | ENA1; 
  fpga_write32(&pFPGA_regs->Clk.Enable, val);   	

  sleep(1);

  close_event_socket();
  close_register_socket();

  return(0);
}






/**********************************************************************************************/
/* following routines allows to change some settings; they have to be called BEFORE vmmInit() */
/**********************************************************************************************/


int
vmmSetWindowParameters(int latency, int width)
{
  int delay_reg_value;

  trig_delay_value = latency; // 24ns per count (0 - 4095)
  trig_width_value = width;
  printf("vmmSetWindowParameters: set latency to %d (%d ns), width to %d\n",latency,latency*24,width);

  return(0);
}


int
vmmSetGain(int igain)
{
  float gain[8] = {0.5, 1.0, 3.0, 4.5, 6.0, 9.0, 12.0, 16.0};

  if(igain<0 || igain>7)
  {
    printf("vmmSetGain: ERROR: igain %d out of range, have to be from 0 to 7\n",igain);
    return(-1);
  }

  sg = igain;
  printf("vmmSetGain: gain set to %4.1f fC/mV\n",gain[igain]);

  return(0);
}

int
vmmSetThreshold(int thres)
{
  if(thres<0 || thres>0x3FF)
  {
    printf("vmmSetThreshold: ERROR: threshold %d out of range, have to be from 0 to 0x3FF\n",thres);
    return(-1);
  }
  
  sdt = thres;
  printf("vmmSetThreshold: threshold set to %d\n",sdt);

  return(0);
}

int
vmmSetTrimThreshold(int chan, int trimthres)
{
  if(trimthres<0 || trimthres>0x1F)
  {
    printf("vmmSetTrimThreshold: ERROR: trim threshold %d out of range, have to be from 0 to 0x1F\n",trimthres);
    return(-1);
  }

  if(chan<0 || chan>127)
  {
    printf("vmmSetTrimThreshold: ERROR: channel number %d out of range, have to be from 0 to 127\n",chan);
    return(-1);
  }

  if(chan<64) sd_ch_1[chan] = trimthres;
  else        sd_ch_2[chan] = trimthres;
  printf("vmmSetTrimThreshold: trim threshold for channel %d set to %d\n",chan,sd_ch[chan]);

  return(0);
}

int
vmmPrintMemArray()
{
  int ii;
  int val;
  int cfg_data;

#if 0 //reads correctly first time, after that reads wrong peace - need some other initialization before read ???
  fpga_write32(&pFPGA_regs->Clk.Cfg_mem_ctrl, 0x81);     // initialize read mode 

  for(ii=0; ii<54; ii++)					 // read data for VMM 1
  {
    val = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_status);  // status before read
    cfg_data = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_rd);
    mem_word_VMM1[ii] = cfg_data;
  }

  for(ii=0; ii<54; ii++)					 // read data for VMM 2
  {
    val = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_status);  // status before read
    cfg_data = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_rd);
    mem_word_VMM2[ii] = cfg_data;
  }
#endif


  printf("\nVMM1 memory array ==================================================================\n");
  for(ii=0; ii<54; ii++) printf("[%2d] 0x%08x\n",ii,mem_word_VMM1[ii]);
  printf("\nVMM2 memory array ==================================================================\n");
  for(ii=0; ii<54; ii++) printf("[%2d] 0x%08x\n",ii,mem_word_VMM2[ii]);
  printf("\n");

  return(0);
}




int
vmmPrintRegisters()
{
  int val;
  
  printf("\n--- read all registers ---\n");
  val = fpga_read32(&pFPGA_regs->Clk.BoardId);
  printf("%s: Version = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Ctrl);
  printf("%s: Ctrl = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Ctrl2);
  printf("%s: Ctrl2 = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Double_pulse);
  printf("%s: Latency = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Delay);
  printf("%s: Delay = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Busy);
  printf("%s: Busy = 0x%08X\n", __func__, val);
//
  printf("******** LATCH STATUS VALUES ********\n");
  fpga_write32(&pFPGA_regs->Clk.Ctrl, 0x10000000);	  // LATCH status values - write Ctrl(28) = 1

  val = fpga_read32(&pFPGA_regs->Clk.Status0);
  printf("%s: Status0 = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Phase1);
  printf("%s: Phase = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Phase_stat1);
  printf("%s: Phase status = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Phase2);
  printf("%s: Phase2 = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Phase_stat2);
  printf("%s: Phase2 status = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_ctrl);
  printf("%s: Cfg memory ctrl = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_rd);
  printf("%s: Cfg memory read = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_wrt);
  printf("%s: Cfg memory write = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_status);
  printf("%s: Cfg memory status = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Cfg_test1);
  printf("%s: Cfg test1 = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Cfg_test2);
  printf("%s: Cfg test2 = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Cfg_test3);
  printf("%s: Cfg test3 = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Test_pulse1);
  printf("%s: Pulser ctrl1 = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Test_pulse1);
  printf("%s: Pulser ctrl1 = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Test_pulse2);
  printf("%s: Pulser ctrl2 = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Test_pulse3);
  printf("%s: Pulser ctrl3 = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Test_pulse4);
  printf("%s: Pulser delay = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Enable);
  printf("%s: Enable = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Time_ctrl);
  printf("%s: Time error ctrl = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Time_err1);
  printf("%s: Time error 1 = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Time_err2);
  printf("%s: Time error 2 = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Time_err3);
  printf("%s: Time error 3 = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Time_err4);
  printf("%s: Time error 4 = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Time_err5);
  printf("%s: Time error 5 = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Scaler_ctrl1);
  printf("%s: Scaler ctrl1 = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Scaler_ctrl2);
  printf("%s: Scaler ctrl2 = 0x%08X\n", __func__, val);
  
  read_scalers(0); 

  return(0);
}




/***********************************************************/
/********************* local functions *********************/
/***********************************************************/

static void
read_scalers(int mode)
{
  int val;
  int j = 0;
  float scaler_interval;

  printf("******** LATCH SCALER VALUES ********\n");
  fpga_write32(&pFPGA_regs->Clk.Scaler_ctrl2, 0x2 );     // latch all scalers into registers

  val = fpga_read32(&pFPGA_regs->Clk.Test_scaler);
  printf("%s: Pulser scaler = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Scaler_trigger);
  printf("%s: Scaler trigger = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Scaler_trailer1);
  printf("%s: Scaler trailer1 = 0x%08X\n", __func__, val);

  val = fpga_read32(&pFPGA_regs->Clk.Scaler_trailer2);
  printf("%s: Scaler trailer2 = 0x%08X\n", __func__, val);
  
  if(mode == 0) return;

// for mode any other value also read channel scalers
  for(j=0;j<128;j++)		// read 128 direct output scalers
  {
    val = j;
    fpga_write32(&pFPGA_regs->Clk.Scaler_ctrl1, val ); 	// select channel
    val = fpga_read32(&pFPGA_regs->Clk.Scaler_data);
    printf("Channel %d : count = %d\n", j, val);
  }
  val = fpga_read32(&pFPGA_regs->Clk.Scaler_time);
  scaler_interval = (float)((val * 3.2E-6));
  printf("\nTime count = %d  (%.6f sec) \n\n", val, scaler_interval);

}


static int
read_status(void)
{
  int val;
  int lost_lock, lost_lock_main, lost_lock_direct;
  int locked_main, locked_direct, locked_ref;
  int delay_read_addr, delay_write_addr, addr_diff;

  /*sergey
  static int first = 1;
  if(first)
  {
    printf("read_status: on the first call, returns 1 to trigger phases settings\n");
    first = 0;
    return(1);
  }
  printf("read_status: on the non-first call, check status\n");
  */

  printf("read_status: ******** LATCH STATUS VALUES ********\n");
  fpga_write32(&pFPGA_regs->Clk.Ctrl, 0x10000000);	  // LATCH status values - write Ctrl(28) = 1
  printf("read_status: --- read STATUS register ---\n");
  val = fpga_read32(&pFPGA_regs->Clk.Status0);
  printf("read_status: Status0 = 0x%08X\n",val);
  delay_read_addr = val & 0xFFF;
  delay_write_addr = (val >> 12) & 0xFFF;
  addr_diff = delay_write_addr - delay_read_addr;
  if ( addr_diff < 0 ) addr_diff = 4096 - addr_diff;
                
  lost_lock_main = (val >> 31) & 0x1;
  lost_lock_direct = (val >> 30) & 0x1;
  locked_direct = (val >> 29) & 0x1;
  locked_main = (val >> 28) & 0x1;
  locked_ref = (val >> 27) & 0x1;
  printf("read_status: lost lock main = %d  lost lock direct = %d\n", lost_lock_main, lost_lock_direct);
  printf("read_status: lock direct = %d  lock main  = %d  lock ref = %d  Delay rd addr = %X  Delay wrt addr = %X  diff = %d\n",
          locked_direct, locked_main, locked_ref, delay_read_addr, delay_write_addr, addr_diff);
          
  lost_lock = lost_lock_main || lost_lock_direct;
  printf("read_status: returns lost_lock=%d\n",lost_lock);

  return(lost_lock);
}


static void
configure_VMMs(int reset_long, int reset_2x)            
{
  char cfg_filename[200];
  int file_value;
  int val;
  int i;	






#if 0 /*use config file*/
  // get VMM1 cfg memory data words from file
  /*VMM #1 cfg file name*/		
  clonparms = getenv("CLON_PARMS");
  //sprintf(cfg_filename, "%s/vmm/cfg_L0_P25_L80_r4095.txt", clonparms);
  sprintf(cfg_filename, "%s/vmm/vmm.txt", clonparms);
  FILE *f_cfg1 = fopen(cfg_filename, "r");
  if(f_cfg1 == NULL)
  {
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("ERROR opening cfg input file (ZEROs written) %s\n", cfg_filename);  // all ZEROs will be written
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  }
  else
  {
    // read data words of file
    printf("VMM1 cfg file: %s\n",cfg_filename);
    for(i = 0; i < 54; i++)
    {
      fscanf(f_cfg1, "%X\n", &file_value);
      mem_word_VMM1[i] = file_value;
      mem_word_VMM2[i] = file_value;
    } 
    fclose(f_cfg1);
  }
#else
/*use local settings*/
  make_mem_array(1, mem_word_VMM1);
  make_mem_array(2, mem_word_VMM2);
#endif





//  	printf("\n------------------------------------------------------------------\n");
//  	printf("\n******** WRITE cfg memory ********\n");
  fpga_write32(&pFPGA_regs->Clk.Cfg_mem_ctrl, 0x80);     // initialize write mode 

  int count = 1;
  int cfg_data;
  for(i=0; i<54; i++)				       // write data to VMM 1
  {
    val = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_status);  // status before write
    cfg_data = mem_word_VMM1[i];
    fpga_write32(&pFPGA_regs->Clk.Cfg_mem_wrt, cfg_data);
    //printf("count = %d  cfg_data = %08X  status = %08X \n", count, cfg_data, val);
    count++;	
  }

  count = 1;
  for(i=0; i<54; i++)				       // write data to VMM 2
  {
    val = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_status);  // status before write
    cfg_data = mem_word_VMM2[i];
    fpga_write32(&pFPGA_regs->Clk.Cfg_mem_wrt, cfg_data);
    //printf("count = %d  cfg_data = %08X  status = %08X \n", count, cfg_data, val);
    count++;	
  }


// read VMM cfg memory and compare to data written
//  	printf("\n******** READ back cfg memory and compare ********\n");
  fpga_write32(&pFPGA_regs->Clk.Cfg_mem_ctrl, 0x81);     // initialize read mode 

  count = 1;
  for(i=0; i<54; i++)					 // read data for VMM 1
  {
    val = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_status);  // status before read
    cfg_data = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_rd);
    if( cfg_data != mem_word_VMM1[i] )
      printf("\n******** CFD DATA ERROR (VMM1) :  count = %d   Read = %08X   Write = %08X \n\n", count, cfg_data, mem_word_VMM1[i]);
      //printf("count = %d  cfg_data = %08X  status = %08X \n", count, cfg_data, val);
    count++;	
  }

  count = 1;
  for(i=0; i<54; i++)					 // read data for VMM 2
  {
    val = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_status);  // status before read
    cfg_data = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_rd);
    if( cfg_data != mem_word_VMM2[i] )
      printf("\n******** CFD DATA ERROR (VMM2):  count = %d   Read = %08X   Write = %08X \n\n", count, cfg_data, mem_word_VMM2[i]);
      //printf("count = %d  cfg_data = %08X  status = %08X \n", count, cfg_data, val);
    count++;	
  }

// ----------------------------------------------------------------------------------
// hard reset VMM1

  printf("\n******** hard reset VMM1 ********\n");
  val = 0x84 | (reset_long << 4);
  fpga_write32(&pFPGA_regs->Clk.Cfg_mem_ctrl, val);   // hard reset VMM1 
  sleep(1);                                            // wait to finish reset
  if(reset_2x)					     // double HARD RESET
  {
    printf("\n******** hard reset VMM1 ********\n");
    fpga_write32(&pFPGA_regs->Clk.Cfg_mem_ctrl, val);   // hard reset VMM1 
    sleep(1);                                           // wait to finish reset
  }

// read Cfg test registers - contain last 96 bits of cfg data sent to VMM1
//  val = fpga_read32(&pFPGA_regs->Clk.Cfg_test1);  	
//  	printf("cfg test register 1 = %08X \n", val);	
//  val = fpga_read32(&pFPGA_regs->Clk.Cfg_test2);  	
//  	printf("cfg test register 2 = %08X \n", val);	
//  val = fpga_read32(&pFPGA_regs->Clk.Cfg_test3);  	
//  	printf("cfg test register 3 = %08X \n", val);	
// ----------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------
// configure VMM1
//  	  printf("\n------------------------------------------------------------------\n");
  printf("\n******** Configure VMM1 ********\n");
  val = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_status);  // status before configuration
//   	  printf("status before config VMM1 = %08X \n", val);

  fpga_write32(&pFPGA_regs->Clk.Cfg_mem_ctrl, 0x82);   // start configuration of VMM1 
 							// serial data for VMM1 routed to Cfg test regs

  for(i=1; i<=100; i++)					// read status during configuration
  {
    val = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_status);
    //  printf("count = %d  status = %08X \n", i, val);	
    usleep(4); 					// cfg should take ~400 us
  }

  sleep(1);                                            // wait to finish configuation

  val = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_status);  // status after configuration
// 	  printf("status after config VMM1 = %08X \n\n", val);

// read Cfg test registers - contain last 96 bits of cfg data sent to VMM1
  val = fpga_read32(&pFPGA_regs->Clk.Cfg_test1);  	
//  	  printf("cfg test register 1 = %08X \n", val);	
  val = fpga_read32(&pFPGA_regs->Clk.Cfg_test2);  	
//  	  printf("cfg test register 2 = %08X \n", val);	
  val = fpga_read32(&pFPGA_regs->Clk.Cfg_test3);  	
//  	  printf("cfg test register 3 = %08X \n", val);	

//  	  printf("\n------------------------------------------------------------------\n");
  printf("********** VMM1 configured **********\n");
//  	  printf("\n------------------------------------------------------------------\n");
// ----------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------
// hard reset VMM2

  printf("\n******** hard reset VMM2 ********\n");
  val = 0x85 | (reset_long << 4); 
  fpga_write32(&pFPGA_regs->Clk.Cfg_mem_ctrl, val);    // hard reset VMM2 
  sleep(1);                                            // wait to finish reset
  if(reset_2x)					     // double HARD RESET
  {
    printf("\n******** hard reset VMM2 ********\n");
    fpga_write32(&pFPGA_regs->Clk.Cfg_mem_ctrl, val);   // hard reset VMM2 
    sleep(1);                                           // wait to finish reset
  }

// read Cfg test registers - contain last 96 bits of cfg data sent to VMM2
//  val = fpga_read32(&pFPGA_regs->Clk.Cfg_test1);  	
//  	printf("cfg test register 1 = %08X \n", val);	
//  val = fpga_read32(&pFPGA_regs->Clk.Cfg_test2);  	
//  	printf("cfg test register 2 = %08X \n", val);	
//  val = fpga_read32(&pFPGA_regs->Clk.Cfg_test3);  	
//  	printf("cfg test register 3 = %08X \n", val);	
// ----------------------------------------------------------------------------------

// configure VMM2
//  	  printf("\n------------------------------------------------------------------\n");
  printf("\n******** Configure VMM2 ********\n");
  val = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_status);  // status before configuration
//   	  printf("status before config VMM2 = %08X \n", val);

  fpga_write32(&pFPGA_regs->Clk.Cfg_mem_ctrl, 0x93);   // start configuration of VMM2
							// serial data for VMM2 routed to Cfg test regs 
 
  for(i=1; i<=100; i++)					// read status during configuration
  {
    val = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_status);
    //printf("count = %d  status = %08X \n", i, val);	
    usleep(4); 					// cfg should take ~400 us
  }

  sleep(1);                                            // wait to finish configuation

  val = fpga_read32(&pFPGA_regs->Clk.Cfg_mem_status);  // status after configuration
  //printf("status after config VMM2 = %08X \n", val);

  // read Cfg test registers - contain last 96 bits of cfg data sent to VMM2
  val = fpga_read32(&pFPGA_regs->Clk.Cfg_test1);  	
  //printf("cfg test register 1 = %08X \n", val);	
  val = fpga_read32(&pFPGA_regs->Clk.Cfg_test2);  	
  //printf("cfg test register 2 = %08X \n", val);	
  val = fpga_read32(&pFPGA_regs->Clk.Cfg_test3);  	
  //printf("cfg test register 3 = %08X \n", val);	
  //printf("\n------------------------------------------------------------------\n");

  //printf("\n-------------------------------------\n");
  printf("********** VMM2 configured **********\n");
  //printf("\n-------------------------------------\n\n");

  printf("\n******************** DONE configure VMMs ********************\n\n");

  return;	  
}




/********************************** Set main data capture clock phase ***************************/

static void
set_clock_phases(int phase_stat1_save)            
{
  int val;
  int phase_adjust_status1 = 0;
  int phase_adjust_status2 = 0;
  int i, n_steps;
  int phase_data1 = 0;
  int phase_data2 = 0;





  /*
  printf("\n***>>> calling set_clock_phases\n\n");
  if( ((phase_stat1_save>>1)&0x1) == 1 )
  {
    printf("\n set_clock_phases: Main data capture clock phase adjustment has been DONE (0x%08X) - do nothing\n\n",phase_stat1_save);
    return;
  }
  */



  /* sergey: use trig_delay register to find out if we are in the very first vmmInit(0 call
  int trig_delay_save;
  trig_delay_save = fpga_read32(&pFPGA_regs->Clk.Delay);
  if(trig_delay_save > 0)
  {
    printf("\n set_clock_phases: clock phase adjustment has been DONE (trig_delay = 0x%08X) - do nothing\n\n",trig_delay_save);
    return;
  }
*/







  /********************/
  /* ALWAYS DO IT !!! */
  /********************/



  // registers that the phase adjustment has taken place - check phase status register
  val = fpga_read32(&pFPGA_regs->Clk.Phase_stat1);
  printf("\n set_clock_phases: ***** Clk.Phase_stat1 = 0x%08X) *****\n\n", val); 		
  phase_adjust_status1 = (0x2 & val) >> 1;
  if ( phase_adjust_status1 == 1 )
  {
    printf("\n set_clock_phases: ***** Main data capture clock phase adjustment has been DONE (0x%08X) *****\n\n", val); 		
  }
  else
  {
    printf("\n set_clock_phases: ***** Setting main data capture clock phase... *****\n");
    phase_data1 = 0x80000000 | (0xFFF & 3);	// 3 phase increments per step = 55.5 ps
    val = fpga_read32(&pFPGA_regs->Clk.Phase_stat1);
    printf("%s: Phase status (start) = 0x%08X\n", __func__, val);
    n_steps = MAIN_CLOCK_PHASE_SHIFT;			// from phase scan test
    for(i=1;i<=n_steps;i++)				// change phase in steps
    {
      fpga_write32(&pFPGA_regs->Clk.Phase1, phase_data1);	// bits 11-0 are number of steps, bit 31 starts state machine 
      usleep(1000); // wait for phase step adjustment to finish
    }
    val = fpga_read32(&pFPGA_regs->Clk.Phase1);
    printf("%s: Phase data = 0x%08X\n", __func__, val);
    val = fpga_read32(&pFPGA_regs->Clk.Phase_stat1);
    printf("%s: Phase status (end) = 0x%08X\n", __func__, val);
    phase_adjust_status1 = (0x2 & val) >> 1;
    if ( phase_adjust_status1 == 1 )
      printf("\n set_clock_phases: ***** Main data capture clock phase adjustment DONE (0x%08X) *****\n", val);
    else 		
      printf("\n set_clock_phases: ***** Main data capture clock phase adjustment NOT DETECTED (0x%08X) *****\n\n", val);
  }

  val = fpga_read32(&pFPGA_regs->Clk.Phase_stat2);
  phase_adjust_status2 = (0x2 & val) >> 1;
  if ( phase_adjust_status2 == 1 )
  {
    printf("\n set_clock_phases: ***** Direct data capture clock phase2 adjustment ALREADY DONE (0x%08X) *****\n\n", val); 		
  }
  else
  {
    printf("\n set_clock_phases: ***** Setting direct data capture clock phase2... *****\n");
    phase_data2 = 0x80000000 | (0xFFF & 3);		// 3 phase increments per step = 55.5 ps
    val = fpga_read32(&pFPGA_regs->Clk.Phase_stat2);
    printf("%s: Phase2 status (start) = 0x%08X\n", __func__, val);
    n_steps = DIRECT_CLOCK_PHASE_SHIFT;		// from phase scan test
    for(i=1;i<=n_steps;i++)				// change phase in steps
    {
      fpga_write32(&pFPGA_regs->Clk.Phase2, phase_data2);	// bits 11-0 are number of steps, bit 31 starts state machine 
      usleep(1000); // wait for phase step adjustment to finish
    }
    val = fpga_read32(&pFPGA_regs->Clk.Phase2);
    printf("%s: Phase2 data = 0x%08X\n", __func__, val);
    val = fpga_read32(&pFPGA_regs->Clk.Phase_stat2);
    printf("%s: Phase2 status (end) = 0x%08X\n", __func__, val);
    phase_adjust_status2 = (0x2 & val) >> 1;
    if ( phase_adjust_status2 == 1 )
      printf("\n set_clock_phases: ***** Direct data capture clock phase2 adjustment DONE (0x%08X) *****\n\n", val); 		
    else 		
      printf("\n set_clock_phases: ***** Direct data capture clock phase2 adjustment NOT DETECTED (0x%08X) *****\n\n", val);
  }

  printf("%s: ********************************** DONE setting clock phases ***********************************\n", __func__);

  return;
}




static unsigned int
reverse_bits(unsigned int value, int size)
{
  // Reverses bit order of a bit field of size 'size'.
  unsigned int b[32/*size*/];
  unsigned int value_rev;
  int i;
  int y;
    
  for(i = 0; i < size; i++) b[size] = 0;

  y = 1;
  for(i = 0; i < size; i++)		// build bit array for 'value'
  {
    b[i] = (value & y) >> i;
    y = y*2;
    //printf("AAA[%2d]=%d\n",i,b[i]);
  }
    
  value_rev = 0;
  for(i = 0; i < size; i++)		// build bit reversed 'value_rev'
  {
    value_rev = value_rev | ( b[i] << (size - i - 1));
  }
    
  return(value_rev);
}



static void
make_mem_array(int chip, int *mem_word)
{
// assemble current VMM configuration parameters into memory array suitable for file write 

  unsigned int l0offset_rev, offset_rev, rollover_rev, window_rev, truncate_rev, nskip_rev;	// reversed bit quantities
  int chan;


  /*chip-dependant settings*/
  if(chip==1)
  {
    for(chan=0; chan<64; chan++) sd_ch[chan] = sd_ch_1[chan];
  }
  else if(chip==2)
  {
    for(chan=0; chan<64; chan++) sd_ch[chan] = sd_ch_2[chan];
  }
  else
  {
    printf("make_mem_array: ERROR: chip=%d out of range, must be 1 or 2\n",chip);
  }


// *********************************************************************************************************
// ************************* CAREFUL - don't modify memory word definitions below **************************
// *********************************************************************************************************

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Global registers - first bank	
	mem_word[0] = 	((sp & 0x1) << 31) |  ((sdp & 0x1) << 30) | ((sbmx & 0x1) << 29) | ((sbft & 0x1) << 28) | ((sbfp & 0x1) << 27) |
			((sbfm & 0x1) << 26) | ((slg & 0x1) << 25) | ((sm5 & 0x3F) << 19) | ((scmx & 0x1) << 18) | ((sfa & 0x1) << 17) |
			((sfam & 0x1) << 16) | ((st & 0x3) << 14) | ((sfm & 0x1) << 13) | ((sg & 0x7) << 10) | ((sng & 0x1) << 9) |
			((stot & 0x1) << 8) |  ((sttt & 0x1) << 7) | ((ssh & 0x1) << 6) | ((stc & 0x3) << 4) | ((sdt & 0x3C0) >> 6) ;		// 32 BITS
				  
	mem_word[1] = 	((sdt & 0x3F) << 26) | ((sdp_dac & 0x3FF) << 16) | ((sc10b & 0x3) << 14) | ((sc8b & 0x3) << 12) | ((sc6b & 0x7) << 9) |
			((s8b & 0x1) << 8) |  ((s6b & 0x1) << 7) | ((s10b & 0x1) << 6) | ((sdcks & 0x1) << 5) | ((sdcka & 0x1) << 4) |
			((sdck6b & 0x1) << 3) | ((sdrv & 0x1) << 2) | ((stpp & 0x1) << 1) | (res00 & 0x1) | 0x00000000 ; 		// 32 BITS
				  
	mem_word[2] = 	((res0 & 0x1) << 31) | ((res1 & 0x1) << 30) | ((res2 & 0x1) << 29) | ((res3 & 0x1) << 28) | ((slvs & 0x1) << 27) | 
			((s32 & 0x1) << 26) | ((stcr & 0x1) << 25) | ((ssart & 0x1) << 24) | ((srec & 0x1) << 23) | ((stlc & 0x1) << 22) | 
			((sbip & 0x1) << 21) | ((srat & 0x1) << 20) | ((sfrst & 0x1) << 19) | ((slvsbc & 0x1) << 18) | ((slvstp & 0x1) << 17) | 
			((slvstk & 0x1) << 16) | ((slvsdt & 0x1) << 15) | ((slvsart & 0x1) << 14) | ((slvstki & 0x1) << 13) | ((slvsena & 0x1) << 12) | 
			((slvs6b & 0x1) << 11) | ((sL0enaV & 0x1) << 10) | ((slh & 0x1) << 9) | ((slxh & 0x1) << 8) | ((stgc & 0x1) << 7) | 
			((reset & 0x1) << 1) | (reset & 0x1) ; 		// 32 BITS
				  
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Channel registers - (24-bits x 64)  4 channels stored in 3 32-bit words  				  
//------------------------------------------------------------------------------

	mem_word[3] = 	((sc_ch & 0x1) << 31) |  ((sl & 0x1) << 30) | ((st_ch[0] & 0x1) << 29) | ((sth & 0x1) << 28) | ((sm_ch[0] & 0x1) << 27) |		// channel A (24 bits)
			((smx & 0x1) << 26) | ((sd_ch[0] & 0x1F) << 21) | ((sz010b & 0x1F) << 16) | ((sz08b & 0xF) << 12) | ((sz06b & 0x7) << 9) | 0x00000000 |		
				  
			((sc_ch & 0x1) << 7) |  ((sl & 0x1) << 6) | ((st_ch[1] & 0x1) << 5) | ((sth & 0x1) << 4) | ((sm_ch[1] & 0x1) << 3) |		// channel B (8 bits)
			((smx & 0x1) << 2) | ((sd_ch[1] & 0x18) >> 3) ;
				  
	mem_word[4] = 	((sd_ch[1] & 0x7) << 29) | ((sz010b & 0x1F) << 24) | ((sz08b & 0xF) << 20) | ((sz06b & 0x7) << 17) | 0x00000000 |		// channel B (16 bits)				  
				  
			((sc_ch & 0x1) << 15) |  ((sl & 0x1) << 14) | ((st_ch[2] & 0x1) << 13) | ((sth & 0x1) << 12) | ((sm_ch[2] & 0x1) << 11) |		// channel C (16 bits)
			((smx & 0x1) << 10) | ((sd_ch[2] & 0x1F) << 5) | ((sz010b & 0x1F)) ; 
				  
	mem_word[5] = 	((sz08b & 0xF) << 28) | ((sz06b & 0x7) << 25) | 0x00000000 |									// channel C (8 bits)				  
				  
			((sc_ch & 0x1) << 23) |  ((sl & 0x1) << 22) | ((st_ch[3] & 0x1) << 21) | ((sth & 0x1) << 20) | ((sm_ch[3] & 0x1) << 19) |		// channel D (24 bits)
			((smx & 0x1) << 18) | ((sd_ch[3] & 0x1F) << 13) | ((sz010b & 0x1F) << 8) | ((sz08b & 0xF) << 4) | ((sz06b & 0x7) << 1) | 0x00000000 ;		

//------------------------------------------------------------------------------

	mem_word[6] = 	((sc_ch & 0x1) << 31) |  ((sl & 0x1) << 30) | ((st_ch[4] & 0x1) << 29) | ((sth & 0x1) << 28) | ((sm_ch[4] & 0x1) << 27) |		// channel A (24 bits)
			((smx & 0x1) << 26) | ((sd_ch[4] & 0x1F) << 21) | ((sz010b & 0x1F) << 16) | ((sz08b & 0xF) << 12) | ((sz06b & 0x7) << 9) | 0x00000000 |		
				  
			((sc_ch & 0x1) << 7) |  ((sl & 0x1) << 6) | ((st_ch[5] & 0x1) << 5) | ((sth & 0x1) << 4) | ((sm_ch[5] & 0x1) << 3) |			// channel B (8 bits)
			((smx & 0x1) << 2) | ((sd_ch[5] & 0x18) >> 3) ;
				  
	mem_word[7] = 	((sd_ch[5] & 0x7) << 29) | ((sz010b & 0x1F) << 24) | ((sz08b & 0xF) << 20) | ((sz06b & 0x7) << 17) | 0x00000000 |		// channel B (16 bits)				  
				  
			((sc_ch & 0x1) << 15) |  ((sl & 0x1) << 14) | ((st_ch[6] & 0x1) << 13) | ((sth & 0x1) << 12) | ((sm_ch[6] & 0x1) << 11) |		// channel C (16 bits)
			((smx & 0x1) << 10) | ((sd_ch[6] & 0x1F) << 5) | ((sz010b & 0x1F)) ; 
				  
	mem_word[8] = 	((sz08b & 0xF) << 28) | ((sz06b & 0x7) << 25) | 0x00000000 |									// channel C (8 bits)				  
				  
			((sc_ch & 0x1) << 23) |  ((sl & 0x1) << 22) | ((st_ch[7] & 0x1) << 21) | ((sth & 0x1) << 20) | ((sm_ch[7] & 0x1) << 19) |		// channel D (24 bits)
			((smx & 0x1) << 18) | ((sd_ch[7] & 0x1F) << 13) | ((sz010b & 0x1F) << 8) | ((sz08b & 0xF) << 4) | ((sz06b & 0x7) << 1) | 0x00000000 ;		

//------------------------------------------------------------------------------

	mem_word[9] = 	((sc_ch & 0x1) << 31) |  ((sl & 0x1) << 30) | ((st_ch[8] & 0x1) << 29) | ((sth & 0x1) << 28) | ((sm_ch[8] & 0x1) << 27) |		// channel A (24 bits)
			((smx & 0x1) << 26) | ((sd_ch[8] & 0x1F) << 21) | ((sz010b & 0x1F) << 16) | ((sz08b & 0xF) << 12) | ((sz06b & 0x7) << 9) | 0x00000000 |		
				  
			((sc_ch & 0x1) << 7) |  ((sl & 0x1) << 6) | ((st_ch[9] & 0x1) << 5) | ((sth & 0x1) << 4) | ((sm_ch[9] & 0x1) << 3) |			// channel B (8 bits)
			((smx & 0x1) << 2) | ((sd_ch[9] & 0x18) >> 3) ;
				  
	mem_word[10] = 	((sd_ch[9] & 0x7) << 29) | ((sz010b & 0x1F) << 24) | ((sz08b & 0xF) << 20) | ((sz06b & 0x7) << 17) | 0x00000000 |		// channel B (16 bits)				  
				  
			((sc_ch & 0x1) << 15) |  ((sl & 0x1) << 14) | ((st_ch[10] & 0x1) << 13) | ((sth & 0x1) << 12) | ((sm_ch[10] & 0x1) << 11) |		// channel C (16 bits)
			((smx & 0x1) << 10) | ((sd_ch[10] & 0x1F) << 5) | ((sz010b & 0x1F)) ; 
				  
	mem_word[11] = 	((sz08b & 0xF) << 28) | ((sz06b & 0x7) << 25) | 0x00000000 |									// channel C (8 bits)				  
				  
			((sc_ch & 0x1) << 23) |  ((sl & 0x1) << 22) | ((st_ch[11] & 0x1) << 21) | ((sth & 0x1) << 20) | ((sm_ch[11] & 0x1) << 19) |		// channel D (24 bits)
			((smx & 0x1) << 18) | ((sd_ch[11] & 0x1F) << 13) | ((sz010b & 0x1F) << 8) | ((sz08b & 0xF) << 4) | ((sz06b & 0x7) << 1) | 0x00000000 ;		

//------------------------------------------------------------------------------

	mem_word[12] = 	((sc_ch & 0x1) << 31) |  ((sl & 0x1) << 30) | ((st_ch[12] & 0x1) << 29) | ((sth & 0x1) << 28) | ((sm_ch[12] & 0x1) << 27) |		// channel A (24 bits)
			((smx & 0x1) << 26) | ((sd_ch[12] & 0x1F) << 21) | ((sz010b & 0x1F) << 16) | ((sz08b & 0xF) << 12) | ((sz06b & 0x7) << 9) | 0x00000000 |		
				  
			((sc_ch & 0x1) << 7) |  ((sl & 0x1) << 6) | ((st_ch[13] & 0x1) << 5) | ((sth & 0x1) << 4) | ((sm_ch[13] & 0x1) << 3) |			// channel B (8 bits)
			((smx & 0x1) << 2) | ((sd_ch[13] & 0x18) >> 3) ;
				  
	mem_word[13] = 	((sd_ch[13] & 0x7) << 29) | ((sz010b & 0x1F) << 24) | ((sz08b & 0xF) << 20) | ((sz06b & 0x7) << 17) | 0x00000000 |		// channel B (16 bits)				  
				  
			((sc_ch & 0x1) << 15) |  ((sl & 0x1) << 14) | ((st_ch[14] & 0x1) << 13) | ((sth & 0x1) << 12) | ((sm_ch[14] & 0x1) << 11) |		// channel C (16 bits)
			((smx & 0x1) << 10) | ((sd_ch[14] & 0x1F) << 5) | ((sz010b & 0x1F)) ; 
				  
	mem_word[14] = 	((sz08b & 0xF) << 28) | ((sz06b & 0x7) << 25) | 0x00000000 |									// channel C (8 bits)				  
				  
			((sc_ch & 0x1) << 23) |  ((sl & 0x1) << 22) | ((st_ch[15] & 0x1) << 21) | ((sth & 0x1) << 20) | ((sm_ch[15] & 0x1) << 19) |		// channel D (24 bits)
			((smx & 0x1) << 18) | ((sd_ch[15] & 0x1F) << 13) | ((sz010b & 0x1F) << 8) | ((sz08b & 0xF) << 4) | ((sz06b & 0x7) << 1) | 0x00000000 ;		

//------------------------------------------------------------------------------

	mem_word[15] = 	((sc_ch & 0x1) << 31) |  ((sl & 0x1) << 30) | ((st_ch[16] & 0x1) << 29) | ((sth & 0x1) << 28) | ((sm_ch[16] & 0x1) << 27) |		// channel A (24 bits)
			((smx & 0x1) << 26) | ((sd_ch[16] & 0x1F) << 21) | ((sz010b & 0x1F) << 16) | ((sz08b & 0xF) << 12) | ((sz06b & 0x7) << 9) | 0x00000000 |		
				  
			((sc_ch & 0x1) << 7) |  ((sl & 0x1) << 6) | ((st_ch[17] & 0x1) << 5) | ((sth & 0x1) << 4) | ((sm_ch[17] & 0x1) << 3) |			// channel B (8 bits)
			((smx & 0x1) << 2) | ((sd_ch[17] & 0x18) >> 3) ;
				  
	mem_word[16] = 	((sd_ch[17] & 0x7) << 29) | ((sz010b & 0x1F) << 24) | ((sz08b & 0xF) << 20) | ((sz06b & 0x7) << 17) | 0x00000000 |		// channel B (16 bits)				  
				  
			((sc_ch & 0x1) << 15) |  ((sl & 0x1) << 14) | ((st_ch[18] & 0x1) << 13) | ((sth & 0x1) << 12) | ((sm_ch[18] & 0x1) << 11) |		// channel C (16 bits)
			((smx & 0x1) << 10) | ((sd_ch[18] & 0x1F) << 5) | ((sz010b & 0x1F)) ; 
				  
	mem_word[17] = 	((sz08b & 0xF) << 28) | ((sz06b & 0x7) << 25) | 0x00000000 |									// channel C (8 bits)				  
				  
			((sc_ch & 0x1) << 23) |  ((sl & 0x1) << 22) | ((st_ch[19] & 0x1) << 21) | ((sth & 0x1) << 20) | ((sm_ch[19] & 0x1) << 19) |		// channel D (24 bits)
			((smx & 0x1) << 18) | ((sd_ch[19] & 0x1F) << 13) | ((sz010b & 0x1F) << 8) | ((sz08b & 0xF) << 4) | ((sz06b & 0x7) << 1) | 0x00000000 ;		

//------------------------------------------------------------------------------

	mem_word[18] = 	((sc_ch & 0x1) << 31) |  ((sl & 0x1) << 30) | ((st_ch[20] & 0x1) << 29) | ((sth & 0x1) << 28) | ((sm_ch[20] & 0x1) << 27) |		// channel A (24 bits)
			((smx & 0x1) << 26) | ((sd_ch[20] & 0x1F) << 21) | ((sz010b & 0x1F) << 16) | ((sz08b & 0xF) << 12) | ((sz06b & 0x7) << 9) | 0x00000000 |		
				  
			((sc_ch & 0x1) << 7) |  ((sl & 0x1) << 6) | ((st_ch[21] & 0x1) << 5) | ((sth & 0x1) << 4) | ((sm_ch[21] & 0x1) << 3) |			// channel B (8 bits)
			((smx & 0x1) << 2) | ((sd_ch[21] & 0x18) >> 3) ;
				  
	mem_word[19] = 	((sd_ch[21] & 0x7) << 29) | ((sz010b & 0x1F) << 24) | ((sz08b & 0xF) << 20) | ((sz06b & 0x7) << 17) | 0x00000000 |		// channel B (16 bits)				  
				  
			((sc_ch & 0x1) << 15) |  ((sl & 0x1) << 14) | ((st_ch[22] & 0x1) << 13) | ((sth & 0x1) << 12) | ((sm_ch[22] & 0x1) << 11) |		// channel C (16 bits)
			((smx & 0x1) << 10) | ((sd_ch[22] & 0x1F) << 5) | ((sz010b & 0x1F)) ; 
				  
	mem_word[20] = 	((sz08b & 0xF) << 28) | ((sz06b & 0x7) << 25) | 0x00000000 |									// channel C (8 bits)				  
				  
			((sc_ch & 0x1) << 23) |  ((sl & 0x1) << 22) | ((st_ch[23] & 0x1) << 21) | ((sth & 0x1) << 20) | ((sm_ch[23] & 0x1) << 19) |		// channel D (24 bits)
			((smx & 0x1) << 18) | ((sd_ch[23] & 0x1F) << 13) | ((sz010b & 0x1F) << 8) | ((sz08b & 0xF) << 4) | ((sz06b & 0x7) << 1) | 0x00000000 ;		

//------------------------------------------------------------------------------

	mem_word[21] = 	((sc_ch & 0x1) << 31) |  ((sl & 0x1) << 30) | ((st_ch[24] & 0x1) << 29) | ((sth & 0x1) << 28) | ((sm_ch[24] & 0x1) << 27) |		// channel A (24 bits)
			((smx & 0x1) << 26) | ((sd_ch[24] & 0x1F) << 21) | ((sz010b & 0x1F) << 16) | ((sz08b & 0xF) << 12) | ((sz06b & 0x7) << 9) | 0x00000000 |		
				  
			((sc_ch & 0x1) << 7) |  ((sl & 0x1) << 6) | ((st_ch[25] & 0x1) << 5) | ((sth & 0x1) << 4) | ((sm_ch[25] & 0x1) << 3) |			// channel B (8 bits)
			((smx & 0x1) << 2) | ((sd_ch[25] & 0x18) >> 3) ;
				  
	mem_word[22] = 	((sd_ch[25] & 0x7) << 29) | ((sz010b & 0x1F) << 24) | ((sz08b & 0xF) << 20) | ((sz06b & 0x7) << 17) | 0x00000000 |		// channel B (16 bits)				  
				  
			((sc_ch & 0x1) << 15) |  ((sl & 0x1) << 14) | ((st_ch[26] & 0x1) << 13) | ((sth & 0x1) << 12) | ((sm_ch[26] & 0x1) << 11) |		// channel C (16 bits)
			((smx & 0x1) << 10) | ((sd_ch[26] & 0x1F) << 5) | ((sz010b & 0x1F)) ; 
				  
	mem_word[23] = 	((sz08b & 0xF) << 28) | ((sz06b & 0x7) << 25) | 0x00000000 |									// channel C (8 bits)				  
				  
			((sc_ch & 0x1) << 23) |  ((sl & 0x1) << 22) | ((st_ch[27] & 0x1) << 21) | ((sth & 0x1) << 20) | ((sm_ch[27] & 0x1) << 19) |		// channel D (24 bits)
			((smx & 0x1) << 18) | ((sd_ch[27] & 0x1F) << 13) | ((sz010b & 0x1F) << 8) | ((sz08b & 0xF) << 4) | ((sz06b & 0x7) << 1) | 0x00000000 ;		

//------------------------------------------------------------------------------

	mem_word[24] = 	((sc_ch & 0x1) << 31) |  ((sl & 0x1) << 30) | ((st_ch[28] & 0x1) << 29) | ((sth & 0x1) << 28) | ((sm_ch[28] & 0x1) << 27) |		// channel A (24 bits)
			((smx & 0x1) << 26) | ((sd_ch[28] & 0x1F) << 21) | ((sz010b & 0x1F) << 16) | ((sz08b & 0xF) << 12) | ((sz06b & 0x7) << 9) | 0x00000000 |		
				  
			((sc_ch & 0x1) << 7) |  ((sl & 0x1) << 6) | ((st_ch[29] & 0x1) << 5) | ((sth & 0x1) << 4) | ((sm_ch[29] & 0x1) << 3) |			// channel B (8 bits)
			((smx & 0x1) << 2) | ((sd_ch[29] & 0x18) >> 3) ;
				  
	mem_word[25] = 	((sd_ch[29] & 0x7) << 29) | ((sz010b & 0x1F) << 24) | ((sz08b & 0xF) << 20) | ((sz06b & 0x7) << 17) | 0x00000000 |		// channel B (16 bits)				  
				  
			((sc_ch & 0x1) << 15) |  ((sl & 0x1) << 14) | ((st_ch[30] & 0x1) << 13) | ((sth & 0x1) << 12) | ((sm_ch[30] & 0x1) << 11) |		// channel C (16 bits)
			((smx & 0x1) << 10) | ((sd_ch[30] & 0x1F) << 5) | ((sz010b & 0x1F)) ; 
				  
	mem_word[26] = 	((sz08b & 0xF) << 28) | ((sz06b & 0x7) << 25) | 0x00000000 |									// channel C (8 bits)				  
				  
			((sc_ch & 0x1) << 23) |  ((sl & 0x1) << 22) | ((st_ch[31] & 0x1) << 21) | ((sth & 0x1) << 20) | ((sm_ch[31] & 0x1) << 19) |		// channel D (24 bits)
			((smx & 0x1) << 18) | ((sd_ch[31] & 0x1F) << 13) | ((sz010b & 0x1F) << 8) | ((sz08b & 0xF) << 4) | ((sz06b & 0x7) << 1) | 0x00000000 ;		

//------------------------------------------------------------------------------

	mem_word[27] = 	((sc_ch & 0x1) << 31) |  ((sl & 0x1) << 30) | ((st_ch[32] & 0x1) << 29) | ((sth & 0x1) << 28) | ((sm_ch[32] & 0x1) << 27) |		// channel A (24 bits)
			((smx & 0x1) << 26) | ((sd_ch[32] & 0x1F) << 21) | ((sz010b & 0x1F) << 16) | ((sz08b & 0xF) << 12) | ((sz06b & 0x7) << 9) | 0x00000000 |		
				  
			((sc_ch & 0x1) << 7) |  ((sl & 0x1) << 6) | ((st_ch[33] & 0x1) << 5) | ((sth & 0x1) << 4) | ((sm_ch[33] & 0x1) << 3) |			// channel B (8 bits)
			((smx & 0x1) << 2) | ((sd_ch[33] & 0x18) >> 3) ;
				  
	mem_word[28] = 	((sd_ch[33] & 0x7) << 29) | ((sz010b & 0x1F) << 24) | ((sz08b & 0xF) << 20) | ((sz06b & 0x7) << 17) | 0x00000000 |		// channel B (16 bits)				  
				  
			((sc_ch & 0x1) << 15) |  ((sl & 0x1) << 14) | ((st_ch[34] & 0x1) << 13) | ((sth & 0x1) << 12) | ((sm_ch[34] & 0x1) << 11) |		// channel C (16 bits)
			((smx & 0x1) << 10) | ((sd_ch[34] & 0x1F) << 5) | ((sz010b & 0x1F)) ; 
				  
	mem_word[29] = 	((sz08b & 0xF) << 28) | ((sz06b & 0x7) << 25) | 0x00000000 |									// channel C (8 bits)				  
				  
			((sc_ch & 0x1) << 23) |  ((sl & 0x1) << 22) | ((st_ch[35] & 0x1) << 21) | ((sth & 0x1) << 20) | ((sm_ch[35] & 0x1) << 19) |		// channel D (24 bits)
			((smx & 0x1) << 18) | ((sd_ch[35] & 0x1F) << 13) | ((sz010b & 0x1F) << 8) | ((sz08b & 0xF) << 4) | ((sz06b & 0x7) << 1) | 0x00000000 ;		

//------------------------------------------------------------------------------

	mem_word[30] = 	((sc_ch & 0x1) << 31) |  ((sl & 0x1) << 30) | ((st_ch[36] & 0x1) << 29) | ((sth & 0x1) << 28) | ((sm_ch[36] & 0x1) << 27) |		// channel A (24 bits)
			((smx & 0x1) << 26) | ((sd_ch[36] & 0x1F) << 21) | ((sz010b & 0x1F) << 16) | ((sz08b & 0xF) << 12) | ((sz06b & 0x7) << 9) | 0x00000000 |		
				  
			((sc_ch & 0x1) << 7) |  ((sl & 0x1) << 6) | ((st_ch[37] & 0x1) << 5) | ((sth & 0x1) << 4) | ((sm_ch[37] & 0x1) << 3) |			// channel B (8 bits)
			((smx & 0x1) << 2) | ((sd_ch[37] & 0x18) >> 3) ;
				  
	mem_word[31] = 	((sd_ch[37] & 0x7) << 29) | ((sz010b & 0x1F) << 24) | ((sz08b & 0xF) << 20) | ((sz06b & 0x7) << 17) | 0x00000000 |		// channel B (16 bits)				  
				  
			((sc_ch & 0x1) << 15) |  ((sl & 0x1) << 14) | ((st_ch[38] & 0x1) << 13) | ((sth & 0x1) << 12) | ((sm_ch[38] & 0x1) << 11) |		// channel C (16 bits)
			((smx & 0x1) << 10) | ((sd_ch[38] & 0x1F) << 5) | ((sz010b & 0x1F)) ; 
				  
	mem_word[32] = 	((sz08b & 0xF) << 28) | ((sz06b & 0x7) << 25) | 0x00000000 |									// channel C (8 bits)				  
				  
			((sc_ch & 0x1) << 23) |  ((sl & 0x1) << 22) | ((st_ch[39] & 0x1) << 21) | ((sth & 0x1) << 20) | ((sm_ch[39] & 0x1) << 19) |		// channel D (24 bits)
			((smx & 0x1) << 18) | ((sd_ch[39] & 0x1F) << 13) | ((sz010b & 0x1F) << 8) | ((sz08b & 0xF) << 4) | ((sz06b & 0x7) << 1) | 0x00000000 ;		

//------------------------------------------------------------------------------

	mem_word[33] = 	((sc_ch & 0x1) << 31) |  ((sl & 0x1) << 30) | ((st_ch[40] & 0x1) << 29) | ((sth & 0x1) << 28) | ((sm_ch[40] & 0x1) << 27) |		// channel A (24 bits)
			((smx & 0x1) << 26) | ((sd_ch[40] & 0x1F) << 21) | ((sz010b & 0x1F) << 16) | ((sz08b & 0xF) << 12) | ((sz06b & 0x7) << 9) | 0x00000000 |		
				  
			((sc_ch & 0x1) << 7) |  ((sl & 0x1) << 6) | ((st_ch[41] & 0x1) << 5) | ((sth & 0x1) << 4) | ((sm_ch[41] & 0x1) << 3) |			// channel B (8 bits)
			((smx & 0x1) << 2) | ((sd_ch[41] & 0x18) >> 3) ;
				  
	mem_word[34] = 	((sd_ch[41] & 0x7) << 29) | ((sz010b & 0x1F) << 24) | ((sz08b & 0xF) << 20) | ((sz06b & 0x7) << 17) | 0x00000000 |		// channel B (16 bits)				  
				  
			((sc_ch & 0x1) << 15) |  ((sl & 0x1) << 14) | ((st_ch[42] & 0x1) << 13) | ((sth & 0x1) << 12) | ((sm_ch[42] & 0x1) << 11) |		// channel C (16 bits)
			((smx & 0x1) << 10) | ((sd_ch[42] & 0x1F) << 5) | ((sz010b & 0x1F)) ; 
				  
	mem_word[35] = 	((sz08b & 0xF) << 28) | ((sz06b & 0x7) << 25) | 0x00000000 |									// channel C (8 bits)				  
				  
			((sc_ch & 0x1) << 23) |  ((sl & 0x1) << 22) | ((st_ch[43] & 0x1) << 21) | ((sth & 0x1) << 20) | ((sm_ch[43] & 0x1) << 19) |		// channel D (24 bits)
			((smx & 0x1) << 18) | ((sd_ch[43] & 0x1F) << 13) | ((sz010b & 0x1F) << 8) | ((sz08b & 0xF) << 4) | ((sz06b & 0x7) << 1) | 0x00000000 ;		

//------------------------------------------------------------------------------

	mem_word[36] = 	((sc_ch & 0x1) << 31) |  ((sl & 0x1) << 30) | ((st_ch[44] & 0x1) << 29) | ((sth & 0x1) << 28) | ((sm_ch[44] & 0x1) << 27) |		// channel A (24 bits)
			((smx & 0x1) << 26) | ((sd_ch[44] & 0x1F) << 21) | ((sz010b & 0x1F) << 16) | ((sz08b & 0xF) << 12) | ((sz06b & 0x7) << 9) | 0x00000000 |		
				  
			((sc_ch & 0x1) << 7) |  ((sl & 0x1) << 6) | ((st_ch[45] & 0x1) << 5) | ((sth & 0x1) << 4) | ((sm_ch[45] & 0x1) << 3) |			// channel B (8 bits)
			((smx & 0x1) << 2) | ((sd_ch[45] & 0x18) >> 3) ;
				  
	mem_word[37] = 	((sd_ch[45] & 0x7) << 29) | ((sz010b & 0x1F) << 24) | ((sz08b & 0xF) << 20) | ((sz06b & 0x7) << 17) | 0x00000000 |		// channel B (16 bits)				  
				  
			((sc_ch & 0x1) << 15) |  ((sl & 0x1) << 14) | ((st_ch[46] & 0x1) << 13) | ((sth & 0x1) << 12) | ((sm_ch[46] & 0x1) << 11) |		// channel C (16 bits)
			((smx & 0x1) << 10) | ((sd_ch[46] & 0x1F) << 5) | ((sz010b & 0x1F)) ; 
				  
	mem_word[38] = 	((sz08b & 0xF) << 28) | ((sz06b & 0x7) << 25) | 0x00000000 |									// channel C (8 bits)				  
				  
			((sc_ch & 0x1) << 23) |  ((sl & 0x1) << 22) | ((st_ch[47] & 0x1) << 21) | ((sth & 0x1) << 20) | ((sm_ch[47] & 0x1) << 19) |		// channel D (24 bits)
			((smx & 0x1) << 18) | ((sd_ch[47] & 0x1F) << 13) | ((sz010b & 0x1F) << 8) | ((sz08b & 0xF) << 4) | ((sz06b & 0x7) << 1) | 0x00000000 ;		

//------------------------------------------------------------------------------

	mem_word[39] = 	((sc_ch & 0x1) << 31) |  ((sl & 0x1) << 30) | ((st_ch[48] & 0x1) << 29) | ((sth & 0x1) << 28) | ((sm_ch[48] & 0x1) << 27) |		// channel A (24 bits)
			((smx & 0x1) << 26) | ((sd_ch[48] & 0x1F) << 21) | ((sz010b & 0x1F) << 16) | ((sz08b & 0xF) << 12) | ((sz06b & 0x7) << 9) | 0x00000000 |		
				  
			((sc_ch & 0x1) << 7) |  ((sl & 0x1) << 6) | ((st_ch[49] & 0x1) << 5) | ((sth & 0x1) << 4) | ((sm_ch[49] & 0x1) << 3) |			// channel B (8 bits)
			((smx & 0x1) << 2) | ((sd_ch[49] & 0x18) >> 3) ;
				  
	mem_word[40] = 	((sd_ch[49] & 0x7) << 29) | ((sz010b & 0x1F) << 24) | ((sz08b & 0xF) << 20) | ((sz06b & 0x7) << 17) | 0x00000000 |		// channel B (16 bits)				  
				  
			((sc_ch & 0x1) << 15) |  ((sl & 0x1) << 14) | ((st_ch[50] & 0x1) << 13) | ((sth & 0x1) << 12) | ((sm_ch[50] & 0x1) << 11) |		// channel C (16 bits)
			((smx & 0x1) << 10) | ((sd_ch[50] & 0x1F) << 5) | ((sz010b & 0x1F)) ; 
				  
	mem_word[41] = 	((sz08b & 0xF) << 28) | ((sz06b & 0x7) << 25) | 0x00000000 |									// channel C (8 bits)				  
				  
			((sc_ch & 0x1) << 23) |  ((sl & 0x1) << 22) | ((st_ch[51] & 0x1) << 21) | ((sth & 0x1) << 20) | ((sm_ch[51] & 0x1) << 19) |		// channel D (24 bits)
			((smx & 0x1) << 18) | ((sd_ch[51] & 0x1F) << 13) | ((sz010b & 0x1F) << 8) | ((sz08b & 0xF) << 4) | ((sz06b & 0x7) << 1) | 0x00000000 ;		

//------------------------------------------------------------------------------

	mem_word[42] = 	((sc_ch & 0x1) << 31) |  ((sl & 0x1) << 30) | ((st_ch[52] & 0x1) << 29) | ((sth & 0x1) << 28) | ((sm_ch[52] & 0x1) << 27) |		// channel A (24 bits)
			((smx & 0x1) << 26) | ((sd_ch[52] & 0x1F) << 21) | ((sz010b & 0x1F) << 16) | ((sz08b & 0xF) << 12) | ((sz06b & 0x7) << 9) | 0x00000000 |		
				  
			((sc_ch & 0x1) << 7) |  ((sl & 0x1) << 6) | ((st_ch[53] & 0x1) << 5) | ((sth & 0x1) << 4) | ((sm_ch[53] & 0x1) << 3) |			// channel B (8 bits)
			((smx & 0x1) << 2) | ((sd_ch[53] & 0x18) >> 3) ;
				  
	mem_word[43] = 	((sd_ch[53] & 0x7) << 29) | ((sz010b & 0x1F) << 24) | ((sz08b & 0xF) << 20) | ((sz06b & 0x7) << 17) | 0x00000000 |		// channel B (16 bits)				  
				  
			((sc_ch & 0x1) << 15) |  ((sl & 0x1) << 14) | ((st_ch[54] & 0x1) << 13) | ((sth & 0x1) << 12) | ((sm_ch[54] & 0x1) << 11) |		// channel C (16 bits)
			((smx & 0x1) << 10) | ((sd_ch[54] & 0x1F) << 5) | ((sz010b & 0x1F)) ; 
				  
	mem_word[44] = 	((sz08b & 0xF) << 28) | ((sz06b & 0x7) << 25) | 0x00000000 |									// channel C (8 bits)				  
				  
			((sc_ch & 0x1) << 23) |  ((sl & 0x1) << 22) | ((st_ch[55] & 0x1) << 21) | ((sth & 0x1) << 20) | ((sm_ch[55] & 0x1) << 19) |		// channel D (24 bits)
			((smx & 0x1) << 18) | ((sd_ch[55] & 0x1F) << 13) | ((sz010b & 0x1F) << 8) | ((sz08b & 0xF) << 4) | ((sz06b & 0x7) << 1) | 0x00000000 ;		

//------------------------------------------------------------------------------

	mem_word[45] = 	((sc_ch & 0x1) << 31) |  ((sl & 0x1) << 30) | ((st_ch[56] & 0x1) << 29) | ((sth & 0x1) << 28) | ((sm_ch[56] & 0x1) << 27) |		// channel A (24 bits)
			((smx & 0x1) << 26) | ((sd_ch[56] & 0x1F) << 21) | ((sz010b & 0x1F) << 16) | ((sz08b & 0xF) << 12) | ((sz06b & 0x7) << 9) | 0x00000000 |		
				  
			((sc_ch & 0x1) << 7) |  ((sl & 0x1) << 6) | ((st_ch[57] & 0x1) << 5) | ((sth & 0x1) << 4) | ((sm_ch[57] & 0x1) << 3) |			// channel B (8 bits)
			((smx & 0x1) << 2) | ((sd_ch[57] & 0x18) >> 3) ;
				  
	mem_word[46] = 	((sd_ch[57] & 0x7) << 29) | ((sz010b & 0x1F) << 24) | ((sz08b & 0xF) << 20) | ((sz06b & 0x7) << 17) | 0x00000000 |		// channel B (16 bits)				  
				  
			((sc_ch & 0x1) << 15) |  ((sl & 0x1) << 14) | ((st_ch[58] & 0x1) << 13) | ((sth & 0x1) << 12) | ((sm_ch[58] & 0x1) << 11) |		// channel C (16 bits)
			((smx & 0x1) << 10) | ((sd_ch[58] & 0x1F) << 5) | ((sz010b & 0x1F)) ; 
				  
	mem_word[47] = 	((sz08b & 0xF) << 28) | ((sz06b & 0x7) << 25) | 0x00000000 |									// channel C (8 bits)				  
				  
			((sc_ch & 0x1) << 23) |  ((sl & 0x1) << 22) | ((st_ch[59] & 0x1) << 21) | ((sth & 0x1) << 20) | ((sm_ch[59] & 0x1) << 19) |		// channel D (24 bits)
			((smx & 0x1) << 18) | ((sd_ch[59] & 0x1F) << 13) | ((sz010b & 0x1F) << 8) | ((sz08b & 0xF) << 4) | ((sz06b & 0x7) << 1) | 0x00000000 ;		

//------------------------------------------------------------------------------

	mem_word[48] = 	((sc_ch & 0x1) << 31) |  ((sl & 0x1) << 30) | ((st_ch[60] & 0x1) << 29) | ((sth & 0x1) << 28) | ((sm_ch[60] & 0x1) << 27) |		// channel A (24 bits)
			((smx & 0x1) << 26) | ((sd_ch[60] & 0x1F) << 21) | ((sz010b & 0x1F) << 16) | ((sz08b & 0xF) << 12) | ((sz06b & 0x7) << 9) | 0x00000000 |		
				  
			((sc_ch & 0x1) << 7) |  ((sl & 0x1) << 6) | ((st_ch[61] & 0x1) << 5) | ((sth & 0x1) << 4) | ((sm_ch[61] & 0x1) << 3) |			// channel B (8 bits)
			((smx & 0x1) << 2) | ((sd_ch[61] & 0x18) >> 3) ;
				  
	mem_word[49] = 	((sd_ch[61] & 0x7) << 29) | ((sz010b & 0x1F) << 24) | ((sz08b & 0xF) << 20) | ((sz06b & 0x7) << 17) | 0x00000000 |		// channel B (16 bits)				  
				  
			((sc_ch & 0x1) << 15) |  ((sl & 0x1) << 14) | ((st_ch[62] & 0x1) << 13) | ((sth & 0x1) << 12) | ((sm_ch[62] & 0x1) << 11) |		// channel C (16 bits)
			((smx & 0x1) << 10) | ((sd_ch[62] & 0x1F) << 5) | ((sz010b & 0x1F)) ; 
				  
	mem_word[50] = 	((sz08b & 0xF) << 28) | ((sz06b & 0x7) << 25) | 0x00000000 |									// channel C (8 bits)				  
				  
			((sc_ch & 0x1) << 23) |  ((sl & 0x1) << 22) | ((st_ch[63] & 0x1) << 21) | ((sth & 0x1) << 20) | ((sm_ch[63] & 0x1) << 19) |		// channel D (24 bits)
			((smx & 0x1) << 18) | ((sd_ch[63] & 0x1F) << 13) | ((sz010b & 0x1F) << 8) | ((sz08b & 0xF) << 4) | ((sz06b & 0x7) << 1) | 0x00000000 ;		

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

// reverse bit order for second bank parameters
	l0offset_rev = reverse_bits((unsigned int)l0offset, 12);
	offset_rev   = reverse_bits((unsigned int)offset, 12);
	rollover_rev = reverse_bits((unsigned int)rollover, 12);
	window_rev   = reverse_bits((unsigned int)window, 3);
	truncate_rev = reverse_bits((unsigned int)truncated, 6);
	nskip_rev    = reverse_bits((unsigned int)nskip, 7);			
		
// Global registers - second bank	
	mem_word[51] = 	0x00000000 | (nskipm & 0x1) ; 		// 32 BITS
					  
	mem_word[52] = 	((sL0cktest & 0x1) << 31) | ((sL0dckinv & 0x1) << 30) | ((sL0ckinv & 0x1) << 29) | ((sL0ena & 0x1) << 28) | 
			((truncate_rev & 0x3F) << 22) | ((nskip_rev & 0x7F) << 15) | ((window_rev & 0x7) << 12) | (rollover_rev & 0xFFF) ; // 32 BITS
				  
	mem_word[53] = 	((l0offset_rev & 0xFFF) << 20) | ((offset_rev & 0xFFF) << 8) | 0x00000000 ;		// 32 BITS


	printf("\n===> mem_word[53]=0x%08x\n",mem_word[53]);
        printf("===> l0offset=0x%x, offset=0x%x\n",l0offset,offset);
        printf("===> l0offset_rev=0x%x, offset_rev=0x%x\n",l0offset_rev,offset_rev);
	printf("\n");

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
}



/*NOT USED HERE*/

#if 0

static int
phase_study_main(void)            
{
  int key_value = 0;
  int phase_data2 = 0;
  float step_size, phase;
  int n_steps;
  int phase_adjust_status1 = 0;
  int phase_adjust_status2 = 0;
  char t_err_filename[200];
  int i;
  int val, val5;

// ---------------------------------------------------------------------------------
// ********************* MAIN Data Clock phase adjust study **********************************
// ---------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// ******** DO NOT need to generate test pulses for main data in L0 mode ********
//             8b/10b IDLE characters provide data transitions! 
// -----------------------------------------------------------------------------------------
  printf("\nEnter number of phase shift increments in each step (0-4095) (use 3) (18.5 ps/increment) - ");
  scanf("%d", &key_value);
  phase_data2 = 0x80000000 | (0xFFF & key_value);
  step_size = key_value * 18.5;
  printf("phase data = %08X (step size = %8.1f)\n\n", phase_data2, step_size );

  printf("\nEnter number of steps (use 300) -");
  scanf("%d", &key_value);
  n_steps = key_value;

  printf("\n\n------------------------------------------------\n");
  printf(" Enter output time error file name (.txt will be appended): \n"); 
		
  scanf("%s", t_err_filename);
  strcat(t_err_filename, ".txt");
  FILE *f_t_err = fopen(t_err_filename, "w");
  if(f_t_err == NULL)
  {
      printf("ERROR opening timing error output file\n");
      return 0;;
  }

// ---------------------------------------------------------------------------------------
  for(i=1;i<=n_steps;i++)			// phase changes in steps
  {
    printf("\n ----- Adjust phase -----\n");
    val = fpga_read32(&pFPGA_regs->Clk.Phase_stat2);
    printf("%s: Current Phase status = 0x%08X\n", __func__, val);
    printf("--- write phase adjust register ---\n");
    fpga_write32(&pFPGA_regs->Clk.Phase2, phase_data2);	 // bits 11-0 are number of steps, bit 31 starts state machine 
    val = fpga_read32(&pFPGA_regs->Clk.Phase2);
    printf("%s: Phase data = 0x%08X\n", __func__, val);
    val = fpga_read32(&pFPGA_regs->Clk.Phase_stat2);
    printf("%s: Phase status = 0x%08X\n", __func__, val);
    sleep(2);                                            // wait for phase adjustment to finish
    
    val = fpga_read32(&pFPGA_regs->Clk.Phase_stat2);
    printf("%s: Phase status after adjust = 0x%08X\n", __func__, val);

// reset latched errors
    printf("Reset latched errors\n");
    fpga_write32(&pFPGA_regs->Clk.Time_ctrl, 0x80000000);	// writing bit 31 resets latched errors 

    printf(".... acquiring data for 1 second\n");
    sleep(1);                                            // wait

// --------------------------------------------------------
// capture latched errors
    printf("Done acquiring data - capture latched errors\n");
    fpga_write32(&pFPGA_regs->Clk.Time_ctrl, 0x00000001);	// writing bit 0 captures latched errors 

// read time error register
    phase = i * step_size; 
    val5 = fpga_read32(&pFPGA_regs->Clk.Time_err5);
    printf("Phase(ps) = %8.2f  Time error (D1_1,D0_1,D1_2,D0_2): %08X\n", phase, val5);	
// write results to file
    fprintf(f_t_err, "%08X %08X\n", i, val5);    

    sleep(1);				// wait 1 sec
     
  }
  fclose(f_t_err);

// ---------------------------------------------------------------------------------
// ********************* END main data clock phase adjust study ******************************
// ---------------------------------------------------------------------------------
  return 1;  
}

#endif
