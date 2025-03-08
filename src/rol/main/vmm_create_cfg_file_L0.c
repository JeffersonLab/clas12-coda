// program to interactively create VMM configuration file (type 'txt') for VMM prototype ******** L0 mode ********

// values are 32-bit words to write to FPGA memory on VMM prototype - values for 10-bit ADC mode
// EJ - 7/24/22, 9/12/22, 5/10/23, 5/21/23, 6/20/23

// reverse bit order of multi-bit values in Global register bank 2 - 4/9/24 - EJ

// ---------------------------------------------------------------------------------------------
// based on program "write_cfg_file_L0_xxx.c"
// interactive features added - 7/16/24 - EJ
// make VMM parameters and memory words GLOBAL variables so that printing and modification of 
// parameters is easier without extensive code changes.  Not elegant but functional. 
// Program is for L0 mode and can:
//    1 - print all current parameters
//    2 - print and decode current major parameters
//    3 - modify front-end parameters
//    4 - modify L0 parameters
//    5 - import starting parameters from existing configuration file
//    6 - write out the modified configuration file 
//----------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define ENABLED  1
#define DISABLED 0
#define NEGATIVE 0

void make_mem_array(void);
void print_menu(void);
void print_cfg_parameters(void);
void print_cfg_major(void);
void modify_L0_parameters(void);
void modify_front_end_parameters(void);
void import_cfg_parameters(void);
unsigned int reverse_bits(unsigned int value, int size);

// -------------------------------------------------------------------------------------------------------------
// --------------------- GLOBAL variables for ease of use in functions -----------------------------------------
// -------------------------------------------------------------------------------------------------------------
	int mem_word[54];

// original data was from .xml file for VMM eval board
// global_registers
		int sdp_dac = 250;		// sdp9-sdp0 [0:0 through 1:1] test pulse DAC (10 bits)	<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		int sdt = 250;			// sdt9-sdt0 [0:0 through 1:1] coarse threshold DAC (10 bits) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		int st = 3;			// st1,st0 [00 01 10 11] peaktime (2 bits) [00 01 10 11] = [ 200, 100, 50, 25 ns ] <<<<<<<<<<
		int sm5 = 0;			// monitor multiplexing: Common monitor: scmx, sm5-sm0 [0 000001 to 000100]
							// 	pulser DAC (after pulser switch), threshold DAC, bandgap reference, temperature sensor)
		int scmx = ENABLED;		// channel monitor: scmx, sm5-sm0 [1 000000 to 111111], channels 0 to 63
		int sbmx = DISABLED;		// routes analog monitor to PDO output
		int slg = ENABLED;		// leakage current disable ([0] enabled)
		int sp = NEGATIVE;		// input charge polarity ([0] negative, [1] positive)
		int sng = DISABLED;		// neighbor (channel and chip) triggering enable
		int sdrv = DISABLED;		// tristates analog outputs with token, used in analog mode
		int sg = 5;			// sg2,sg1,sg0 [000:111]  gain (0.5, 1, 3, 4.5, 6, 9, 12, 16 mV/fC) <<<<<<<<<<<<<<<<<<<<<<<<<
//		int stc = 2;			// stc1,stc0 [00 01 10 11]  TAC slope adjustment (60, 100, 350, 650 ns ) <<<<<<<<<<<<<<<<<<<<
		int stc = 0;			// stc1,stc0 [00 01 10 11]  TAC slope adjustment (60, 100, 350, 650 ns ) <<<<<<<<<<<<<<<<<<<<
		int sfm = ENABLED;		// sfm [0 1] enables full-mirror (AC) and high-leakage operation (enables SLH) <<<<<<<<<<<<<<
		int ssh = DISABLED;		// ssh [0 1] enables sub-hysteresis discrimination
		int sdp = DISABLED;		// disable-at-peak
		int sbft = DISABLED;		// sbft [0 1], sbfp [0 1], sbfm [0 1] analog output buffers, [1] enable (TDO, PDO, MO)
		int sbfp = DISABLED;		//
		int sbfm = ENABLED;		// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		int s6b = ENABLED;		// enables 6-bit ADC (requires sttt enabled) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		int s8b = ENABLED;		// enables 8-bit ADC <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		int s10b = ENABLED;		// enables high resolution ADCs (10/8-bit ADC enable)  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		int sttt = ENABLED;		// sttt [0 1] enables direct-output logic (both timing and s6b) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		int stpp = 0;			// timing outputs control 1 (s6b must be disabled)
		int stot = 1; 			// stpp,stot[00,01,10,11]: TtP,ToT,PtP,PtT
							// TtP: threshold-to-peak
							// ToT: time-over-threshold
							// PtP: pulse-at-peak (10ns) (not available with s10b)
							// PtT: peak-to-threshold (not available with s10b)

		int sfa = DISABLED;		// ART enable: sfa [0 1], sfam [0 1] (sfa [1]) and mode (sfam [0] timing at threshold,[1] timing at peak)
		int sfam = 0;			//
		int sc10b = 3;			// sc010b,sc110b 10-bit ADC conv. time (increase subtracts 60 ns) <<<<<<<<<<<<<<<<<<<<<<<<<<<<
		int sc8b = 3;			// sc08b,sc18b 8-bit ADC conv. time (increase subtracts 60 ns) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		int sc6b = 2;			// sc06b, sc16b, sc26b 6-bit ADC conversion time <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		int sdcks = DISABLED;		// dual clock edge serialized data enable
		int sdcka = DISABLED;		// dual clock edge serialized ART enable
		int sdck6b = DISABLED;		// dual clock edge serialized 6-bit enable
		int slvs = DISABLED;		// enables direct output IOs <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		int stlc = DISABLED;		// enables mild tail cancellation (when enabled, overrides sbip)
		int stcr = DISABLED;		// enables auto-reset (at the end of the ramp, if no stop occurs) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		int ssart = DISABLED;		// enables ART flag synchronization (trail to next trail)
		int srec = DISABLED;		// enables fast recovery from high charge
		int sfrst = DISABLED;		// enables fast reset at 6-b completion <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		int s32 = DISABLED;		// skips channels 16-47 and makes 15 and 48 neighbors
		int sbip = ENABLED;		// enables bipolar shape (!!!!!!!! ALWAYS ENABLE THIS !!!!!!!!)
		int srat = DISABLED;		// enables timing ramp at threshold <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		int slvsbc = DISABLED;		// enable slvs 100Ω termination on ckbc
		int slvsart = DISABLED;		// enable slvs 100Ω termination on ckart
		int slvstp = DISABLED;		// enable slvs 100Ω termination on cktp
		int slvstki = DISABLED;		// enable slvs 100Ω termination on cktki
		int slvstk = DISABLED;		// enable slvs 100Ω termination on cktk
		int slvsena = DISABLED;		// enable slvs 100Ω termination on ckena
		int slvsdt = DISABLED;		// enable slvs 100Ω termination on ckdt
		int slvs6b = DISABLED;		// enable slvs 100Ω termination on ck6b
		
		int sL0ena = ENABLED;		// enable L0 core / reset core & gate clk if 0 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		int sL0enaV = ENABLED;		// disable mixed signal functions when L0 enabled <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
//		int l0offset = 0;		// l0offset i0:11 L0 BC offset - TRIGGER LATENCY - 25 ns/count <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< 
//		int offset = 80;		// offset i0:11 Channel tagging BC offse//t
		int l0offset = 4015;		// l0offset i0:11 L0 BC offset - TRIGGER LATENCY - 25 ns/count <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< 
		int offset = 0;			// offset i0:11 Channel tagging BC offset
		int rollover = 4095;		// rollover i0:11 Channel tagging BC rollover
		int window = 7;			// window i0:2 Size of trigger window - TRIGGER WINDOW - 25 ns * (count+1) <<<<<<<<<<<<<<<<<<<<
		int truncate = 0;		// truncate i0:5 Max hits per L0 (0 = NO truncation) ***************************** !!!!!!!!!!!!
		int nskip = 0;			// nskip i0:6 Number of L0 triggers to skip on overflow
		int sL0cktest = DISABLED;	// enable clocks when L0 core disabled (test)
		int sL0ckinv = DISABLED;	// invert BCCLK  *****************************************************************
		int sL0dckinv = DISABLED;	// invert DCK
		int nskipm = DISABLED;		// magic number on BCID - 0xFE8 (4072)
		int res00 = 0;			// ????? reserved?
		int res0 = 0;
		int res1 = 0;
		int res2 = 0;
		int res3 = 0;
		int reset = 0;
		int slh = 0;
		int slxh = 0;
		int stgc = 0;
		
				
// channel_registers
		// channel_register_mask  ?????
		int sc_ch = 0;		// sc [0 1] large sensor capacitance mode ([0] <∼200 pF , [1] >∼200 pF ) 
		int sl = 0;  		// sl [0 1] leakage current disable [0=enabled]
		int sth = 0; 		// sth [0 1] multiplies test capacitor by 10
		int smx = 0; 		// smx [0 1] channel monitor mode ( [0] analog output, [1] trimmed threshold))
		int sz010b = 0; 	// ????? sz010b, sz110b, sz210b, sz310b, sz410b 10-bit ADC offset subtraction
		int sz08b = 0;  	// ????? sz08b, sz18b, sz28b, sz38b 8-bit ADC offset subtraction
		int sz06b = 0;  	// ????? sz06b, sz16b, sz26b 6-bit ADC offset subtraction

// --------------------------------------------------------------------------------------------------------
//  				sm_ch - Channel mask enable (1 = channel OFF)                                  
//
//                                     ch 0-7            8-15              16-23             24-31        
		int sm_ch[64] = { 0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  
//                                    ch 32-39           40-47             48-55             56-63         
			          0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0   };	// NONE OFF

// --------------------------------------------------------------------------------------------------------
//  				st_ch - Test pulse enable = 1 
                                  
//                                     ch 0-7            8-15              16-23             24-31        
		int st_ch[64] = { 0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  	//
//                                    ch 32-39           40-47             48-55             56-63         
			          0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0   };	// pulse NONE
                                  
// --------------------------------------------------------------------------------------------------------
//  			sd0-sd4 [0:0 through 1:1] trim threshold DAC, 1mV step ([0:0] trim 0V , [1:1] trim -29mV ) 
                                  
//                                     ch 0-7            8-15              16-23             24-31         
		int sd_ch[64] = { 0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  
//                                    ch 32-39           40-47             48-55             56-63        
			          0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0   };

// -------------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------------


int main(int argc, char *argv[])
{
  char cfg_filename[200];
  char next[18];
  
  printf("\n\n------------------------------------------------------------- \n");
  printf("Program is for L0 mode and can: \n");
  printf("       1 - print all current configuration parameters\n");
  printf("       2 - print and decode current major configuration parameters\n");
  printf("       3 - modify front-end configuration parameters\n");
  printf("       4 - modify L0 configuration parameters\n");
  printf("       5 - import starting configuration parameters from existing file\n");
  printf("       6 - write out the modified configuration file\n"); 
  printf("------------------------------------------------------------- \n\n");
  
// --------------------------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------------------------
  make_mem_array();		// load memory array with starting configuration parameters 

// Interactive code to change values here

  while(1)
  {
      print_menu();
      scanf("%s", next);
      if( *next == 'q' )		// quit
      {
          break;
      }
      else if( *next == 'p' )		// print all configuration values
      {
          make_mem_array();		// load memory array 
  	  print_cfg_parameters();	// print values
      }
      else if( *next == 'P' )		// print and decode major configuration parameters
      {
          make_mem_array();		// load memory array 
      	  print_cfg_major();		// print values
      }
      else if( *next == 'i' )		// import configuration parameters from existing file
      {
  	  import_cfg_parameters();	// import values
          make_mem_array();		// load memory array 
      }
      else if( *next == 'f' )		// change front-end parameters
      {
            modify_front_end_parameters();
            make_mem_array();		// update memory array 
      }
      else if( *next == 'L' )		// change L0 parameters
      {
            modify_L0_parameters();
            make_mem_array();		// update memory array 
      }
      else if( *next == 'F' )		// write configuration file
      {
          make_mem_array();		// load memory array 
          printf("\n\n------------------------------------------------\n");
          printf(" Enter output cfg file name (.txt will be appended):  "); 
		
  	  scanf("%s", cfg_filename);
  	  strcat(cfg_filename, ".txt");
  	  FILE *f_cfg = fopen(cfg_filename, "w");
  	  if(f_cfg == NULL)
  	  {
      	      printf("ERROR opening cfg output file\n");
      	      exit(0);
  	  }
	  // write mem data words to file
  	  printf("Writing cfg data output file:  %s\n", cfg_filename);
  	  for(int i = 0; i < 54; i++)
  	  {
              fprintf(f_cfg, "%X\n", mem_word[i]);
  	  }
  	  fclose(f_cfg);
       } 
  }

  return 0;
}


// --------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------
void print_menu(void)
{
	printf("\nEnter: 'p <CR>' print all Configuration parameters  ********\n");
	printf(  "       'P <CR>' print and decode major Configuration parameters  ********\n");
	printf(  "       'i <CR>' import Configuration parameters from existing file  ********\n");
	printf(  "       'f <CR>' edit front end Configuration parameters  ********\n");
	printf(  "       'L <CR>' edit L0 Configuration parameters  ********\n");
	printf(  "       'F <CR>' write Configuration File  ********\n");
	printf(  "       'q <CR>' quit  ********\n\n");

}
// --------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------
void print_cfg_parameters(void)
{
// print configuration parameters from current memory array
// code from program "read_cfg_file.c"

  int value;
  int chan;
  int i,j;
  int mem[54];
  unsigned int l0offset_rev, offset_rev, rollover_rev, window_rev, truncate_rev, nskip_rev;	// reversed bit quantities
  unsigned int l0offset_current, offset_current, rollover_current, window_current, truncate_current, nskip_current;
  
  for(int i = 0; i < 54; i++)
  {
      mem[i] = mem_word[i];	// copy to local array
//    printf("i = %d   %X\n", i, value);
  } 

// -----------------------------------------------------------------------------------------------------
// Global registers - first bank	
  printf("-----------------------------------------------------------------\n");
  printf("\n----- Global registers - first bank -----\n\n");

  printf("sp = %d  sdp = %d  sbmx = %d  sbft = %d  sbfp = %d  sbfm = %d\n", 
      ((mem[0] >> 31) & 0x1), ((mem[0] >> 30) & 0x1), ((mem[0] >> 29) & 0x1), ((mem[0] >> 28) & 0x1), ((mem[0] >> 27) & 0x1), ((mem[0] >> 26) & 0x1) ); 

  printf("slg = %d  sm5 = %d  scmx = %d  sfa = %d  sfam = %d  st = %d\n", 
      ((mem[0] >> 25) & 0x1), ((mem[0] >> 19) & 0x3F), ((mem[0] >> 18) & 0x1), ((mem[0] >> 17) & 0x1), ((mem[0] >> 16) & 0x1), ((mem[0] >> 14) & 0x3) ); 

  printf("sfm = %d  sg = %d  sng = %d  stot = %d  sttt = %d  ssh = %d\n", 
      ((mem[0] >> 13) & 0x1), ((mem[0] >> 10) & 0x7), ((mem[0] >> 9) & 0x1), ((mem[0] >> 8) & 0x1), ((mem[0] >> 7) & 0x1), ((mem[0] >> 6) & 0x1) ); 

  printf("stc = %d  sdt = %d  sdp_dac = %d  sc10b = %d  sc8b = %d  sc6b = %d\n", 
      ((mem[0] >> 4) & 0x3), (((mem[0] << 6) & 0x3C0) | ((mem[1] >> 26) & 0x3F)), ((mem[1] >> 16) & 0x3FF), ((mem[1] >> 14) & 0x3), ((mem[1] >> 12) & 0x3), 
      ((mem[1] >> 9) & 0x7) ); 

  printf("s8b = %d  s6b = %d  s10b = %d  sdcks = %d  sdcka = %d  sdck6b = %d\n", 
      ((mem[1] >> 8) & 0x1), ((mem[1] >> 7) & 0x1), ((mem[1] >> 6) & 0x1), ((mem[1] >> 5) & 0x1), ((mem[1] >> 4) & 0x1), ((mem[1] >> 3) & 0x1) ); 

  printf("sdrv = %d  stpp = %d  res00 = %d  res0 = %d  res1 = %d  res2 = %d\n", 
      ((mem[1] >> 2) & 0x1), ((mem[1] >> 1) & 0x1), (mem[1] & 0x1), ((mem[2] >> 31) & 0x1), ((mem[2] >> 30) & 0x1), ((mem[2] >> 29) & 0x1) ); 

  printf("res3 = %d  slvs = %d  s32 = %d  stcr = %d  ssart = %d  srec = %d\n", 
      ((mem[2] >> 28) & 0x1), ((mem[2] >> 27) & 0x1), ((mem[2] >> 26) & 0x1), ((mem[2] >> 25) & 0x1), ((mem[2] >> 24) & 0x1), ((mem[2] >> 23) & 0x1) ); 

  printf("stlc = %d  sbip = %d  srat = %d  sfrst = %d  slvsbc = %d  slvstp = %d\n", 
      ((mem[2] >> 22) & 0x1), ((mem[2] >> 21) & 0x1), ((mem[2] >> 20) & 0x1), ((mem[2] >> 19) & 0x1), ((mem[2] >> 18) & 0x1), ((mem[2] >> 17) & 0x1) ); 

  printf("slvstk = %d  slvsdt = %d  slvsart = %d  slvstki = %d  slvsena = %d  slvs6b = %d\n", 
      ((mem[2] >> 16) & 0x1), ((mem[2] >> 15) & 0x1), ((mem[2] >> 14) & 0x1), ((mem[2] >> 13) & 0x1), ((mem[2] >> 12) & 0x1), ((mem[2] >> 11) & 0x1) ); 

  printf("sL0enaV = %d  slh = %d  slxh = %d  stgc = %d  reset = %d  reset = %d\n", 
      ((mem[2] >> 10) & 0x1), ((mem[2] >> 9) & 0x1), ((mem[2] >> 8) & 0x1), ((mem[2] >> 7) & 0x1), ((mem[2] >> 1) & 0x1), (mem[2] & 0x1) ); 

// ----------------------------------------------------------------------------------------------------
// Global registers - second bank

  l0offset_rev = (mem[53] >> 20) & 0xFFF;	
  offset_rev   = (mem[53] >> 8) & 0xFFF;
  rollover_rev = (mem[52] & 0xFFF);
  window_rev   = (mem[52] >> 12) & 0x7;
  truncate_rev = (mem[52] >> 22) & 0x3F;
  nskip_rev    = (mem[51] & 0x1);

// reverse bit order for second bank parameters - use LOCAL variables here
  l0offset_current = reverse_bits((unsigned int)l0offset_rev, 12);
  offset_current   = reverse_bits((unsigned int)offset_rev, 12);
  rollover_current = reverse_bits((unsigned int)rollover_rev, 12);
  window_current   = reverse_bits((unsigned int)window_rev, 3);
  truncate_current = reverse_bits((unsigned int)truncate_rev, 6);
  nskip_current    = reverse_bits((unsigned int)nskip_rev, 7);			
		
  printf("-----------------------------------------------------------------\n");
  printf("\n----- Global registers - second bank -----\n\n");

  printf("nskipm = %d  sL0cktest = %d  sL0dckinv = %d  sL0ckinv = %d  sL0ena = %d  truncate = %d\n", 
      	(mem[51] & 0x1), ((mem[52] >> 31) & 0x1), ((mem[52] >> 30) & 0x1), ((mem[52] >> 29) & 0x1), ((mem[52] >> 28) & 0x1), truncate_current ); 

  printf("nskip = %d  window = %d  rollover = %d  l0offset = %d  offset = %d\n", 
      		nskip_current, window_current, rollover_current, l0offset_current, offset_current ); 

  printf("-----------------------------------------------------------------\n");

//--------------------------------------------------------------------------------------------------------------------------------------------------
// Channel registers - (24-bits x 64)  4 channels stored in 3 32-bit words  				  
//------------------------------------------------------------------------------	
  printf("\n----- Channel registers -----\n");
 
  chan = 0;
  for(i = 1; i <=16; i++)
  {
	j = 3*i;

// -----------------------------------
  printf("\n----- Channel %d\n", chan);

  printf("sc_ch = %d  sl = %d  st_ch = %d  sth = %d  sm = %d  smx = %d\n", 
      ((mem[j] >> 31) & 0x1), ((mem[j] >> 30) & 0x1), ((mem[j] >> 29) & 0x1), ((mem[j] >> 28) & 0x1), ((mem[j] >> 27) & 0x1), ((mem[j] >> 26) & 0x1) ); 

  printf("sd = %d  sz010b = %d  sz08b = %d  sz06b = %d\n", 
      ((mem[j] >> 21) & 0x1F), ((mem[j] >> 16) & 0x1F), ((mem[j] >> 12) & 0xF), ((mem[j] >> 9) & 0x7) ); 

  chan = chan + 1;

// -----
  printf("\n----- Channel %d\n", chan);

  printf("sc_ch = %d  sl = %d  st_ch = %d  sth = %d  sm = %d  smx = %d\n", 
      ((mem[j] >> 7) & 0x1), ((mem[j] >> 6) & 0x1), ((mem[j] >> 5) & 0x1), ((mem[j] >> 4) & 0x1), ((mem[j] >> 3) & 0x1), ((mem[j] >> 2) & 0x1) ); 

  printf("sd = %d  sz010b = %d  sz08b = %d  sz06b = %d\n", 
      ( ((mem[j] << 3) & 0x18) | ((mem[j+1] >> 29) & 0x7) ), ((mem[j+1] >> 24) & 0x1F), ((mem[j+1] >> 20) & 0xF), ((mem[j+1] >> 17) & 0x7) ); 

  chan = chan + 1;

// -----
  printf("\n----- Channel %d\n", chan);

  printf("sc_ch = %d  sl = %d  st_ch = %d  sth = %d  sm = %d  smx = %d\n", 
      ((mem[j+1] >> 15) & 0x1), ((mem[j+1] >> 14) & 0x1), ((mem[j+1] >> 13) & 0x1), ((mem[j+1] >> 12) & 0x1), ((mem[j+1] >> 11) & 0x1), ((mem[j+1] >> 10) & 0x1) ); 

  printf("sd = %d  sz010b = %d  sz08b = %d  sz06b = %d\n", 
      ((mem[j+1] >> 5) & 0x1F), (mem[j+1] & 0x1F), ((mem[j+2] >> 28) & 0xF), ((mem[j+2] >> 25) & 0x7) ); 

  chan = chan + 1;

// -----
  printf("\n----- Channel %d\n", chan);

  printf("sc_ch = %d  sl = %d  st_ch = %d  sth = %d  sm = %d  smx = %d\n", 
      ((mem[j+2] >> 23) & 0x1), ((mem[j+2] >> 22) & 0x1), ((mem[j+2] >> 21) & 0x1), ((mem[j+2] >> 20) & 0x1), ((mem[j+2] >> 19) & 0x1), ((mem[j+2] >> 18) & 0x1) ); 

  printf("sd = %d  sz010b = %d  sz08b = %d  sz06b = %d\n", 
      ((mem[j+2] >> 13) & 0x1F), ((mem[j+2] >> 8) & 0x1F), ((mem[j+2] >> 4) & 0xF), ((mem[j+2] >> 1) & 0x7) ); 

  chan = chan + 1;

// ----------------------------------
  }
// --------------------------------------------------------------------------------------------------------
  printf("\n\n");
}
// --------------------------------------------------------------------------------------------------------  
// --------------------------------------------------------------------------------------------------------  

 
// --------------------------------------------------------------------------------------------------------  
// --------------------------------------------------------------------------------------------------------  
void print_cfg_major(void)
{
// print and decode major configuration parameters from current memory array
// code from program "read_cfg_file.c"

  int value;
  int chan;
  int i,j;
  int l0offset_value, offset_value, rollover_value, window_value, truncate_value, nskip_value;
  
  int peaktime_value, peaktime, gain_value, neighbor, tac_slope_value, tac_slope;
  int threshold_coarse, test_pulse_dac, L0_enable, latency;
  float gain;
  
  int masked[64];
  int pulsed[64];
  int trim_mv, trim_zero;
  
// -----------------------------------------------------------------------------------------------------
// Global registers - first bank	
  printf("-----------------------------------------------------------------\n");
  printf("----- Global registers - first bank -----\n\n");

  peaktime_value = st;
  if( peaktime_value == 0 )
      peaktime = 200;
  else if(peaktime_value == 1 )
      peaktime = 100;
  else if(peaktime_value == 2 )
      peaktime = 50;
  else if(peaktime_value == 3 )
      peaktime = 25;
  else
      printf("!!!!! peaktime illegal value !!!!!\n");
      
  gain_value = sg;
  if( gain_value == 0 )
      gain = 0.5;
  else if(gain_value == 1 )
      gain = 1.0;
  else if(gain_value == 2 )
      gain = 3.0;
  else if(gain_value == 3 )
      gain = 4.5;
  else if(gain_value == 4 )
      gain = 6.0;
  else if(gain_value == 5 )
      gain = 9.0;
  else if(gain_value == 6 )
      gain = 12.0;
  else if(gain_value == 7 )
      gain = 16.0;
  else
      printf("!!!!! gain illegal value !!!!!\n");
      
  neighbor = sng;
  if( (neighbor != 0) && (neighbor != 1) )
      printf("!!!!! neighbor illegal value !!!!!\n");

  tac_slope_value = stc;
  if( tac_slope_value == 0 )
      tac_slope = 60;
  else if(tac_slope_value == 1 )
      tac_slope = 100;
  else if(tac_slope_value == 2 )
      tac_slope = 350;
  else if(tac_slope_value == 3 )
      tac_slope = 650;
  else
      printf("!!!!! TAC slope illegal value !!!!!\n");
      
  threshold_coarse = sdt;
  test_pulse_dac = sdp_dac;
  
  printf("(st = %d) Peaktime = %d ns   (sg = %d) Gain = %.1f mV/fC   (sng = %d) Neighbor = %d   (stc = %d) TAC slope = %d ns \n", 
      peaktime_value, peaktime, gain_value, gain, neighbor, neighbor, tac_slope_value, tac_slope ); 

  printf("(sdt = %d) Threshold coarse = %d    (sdp = %d) Test pulse DAC = %d \n", 
      threshold_coarse, threshold_coarse, test_pulse_dac, test_pulse_dac); 

// ----------------------------------------------------------------------------------------------------
// Global registers - second bank

  l0offset_value = l0offset;
  offset_value   = offset;
  rollover_value = rollover;
  window_value   = window;
  truncate_value = truncate;
  nskip_value    = nskip;
				
  printf("-----------------------------------------------------------------\n");
  printf("----- Global registers - second bank -----\n\n");

  L0_enable = sL0ena;  
  latency = rollover_value - l0offset_value;    
       
  printf("(sL0ena = %d) L0 eanble = %d   Latency = %d \n", L0_enable, L0_enable, latency);
  
  printf("nskip = %d  window = %d  rollover = %d  l0offset = %d  offset = %d\n", 
      		nskip_value, window_value, rollover_value, l0offset_value, offset_value ); 

  printf("-----------------------------------------------------------------\n");


  // ----------------------------------------------------------------------------------
  // list masked channels  
  j = 0;
  for(i = 0; i < 64; i++)
  {
    if( sm_ch[i] == 1 )
    {
      masked[j] = i;
      j++;
    }
  }
  if( j == 0 )
    printf("\n----- NO channels masked -----\n");
  else
  {
    printf("\n----- Masked channels: \n");
    for(i = 0; i < j; i++) printf("%4d\n", masked[i]);
  }
          
  // list test pulsed channels  
  j = 0;
  for(i = 0; i < 64; i++)
  {
    if( st_ch[i] == 1 )
    {
      pulsed[j] = i;
      j++;
    }
  }
  if( j == 0 )
    printf("\n----- NO channels test pulsed -----\n");
  else
  {
    printf("\n----- Test pulsed channels: \n");
    for(i = 0; i < j; i++) printf("%4d\n", pulsed[i]);
  }
  
  // trim threshold DAC  
  printf("\n----- Trim threshold DAC: \n");
  trim_zero = 1;
  for(i = 0; i < 64; i++)		// scan all channels for non-zero trim value
  {
    if( sd_ch[i] != 0 )		
    {
      trim_zero = 0;		// found at least one channel with non-zero trim
      break;
    }
  }
  
  if( trim_zero )
    printf("\n  ALL channels have trim DAC = 0 -----\n");
  else
  {
    for(i = 0; i < 64; i++)
    {
      trim_mv = sd_ch[i]*(-1);
      printf("ch = %2d  sd_ch = %2d  (%d mV)\n", i, sd_ch[i], trim_mv);
    }
  }
  printf("\n-----------------------------------------------------------------\n\n");
}
// --------------------------------------------------------------------------------------------------------  
// --------------------------------------------------------------------------------------------------------  


// --------------------------------------------------------------------------------------------------------  
// --------------------------------------------------------------------------------------------------------  
void modify_front_end_parameters(void)
{
    int peaktime_ns[4] = {200, 100, 50, 25};
    float gain[8] = {0.5, 1.0, 3.0, 4.5, 6.0, 9.0, 12.0, 16.0};
    int tac_slope_ns[4] = {60, 100, 350, 650};
    int masked[64];
    int pulsed[64];
    int trim_dac[64];
    int key_value, key_value1, key_value2, key_value3, key_value4, key_value5;
    int trim_mv, trim_zero, latency_current;
    int i, j;

    printf("\n  Current Peak time = %d  (%d ns)\n", st, peaktime_ns[st]);
    printf("\n----- Enter '1' to change value, '0' to keep value: ");
    scanf("%d", &key_value);
    if( key_value)
    {
        while(1)
        {
            printf("\n----- Select Peak time (0 = 200ns, 1 = 100ns, 2 = 50ns, 3 = 25ns): ");
            scanf("%d", &key_value2);
            if( (key_value2 < 0) || (key_value2 > 3) )
                printf("\n!!!!!!!! illegal peaktime entered - try again !!!!!!!!\n");
            else
            {
                st = key_value2;
                printf("\n  Peak time = %d  (%d ns)\n", st, peaktime_ns[st]);
                break;
            }
        }
    }
    
    printf("\n  Current gain value = %d  (%.1f)\n", sg, gain[sg]);
    printf("\n----- Enter '1' to change value, '0' to keep value: ");
    scanf("%d", &key_value);
    if( key_value)
    {
        while(1)
        {
            printf("\n----- Select gain (0 = 0.5 mV/fC, 1 = 1.0 mV/fC, 2 =  3.0 mV/fC,  3 =  4.5 mV/fC, \n");
            printf(  "                   4 = 6.0 mV/fC, 5 = 9.0 mV/fC, 6 = 12.0 mV/fC,  7 = 16.0 mV/fC ):  ");
            scanf("%d", &key_value2);
            if( (key_value2 < 0) || (key_value2 > 7) )
                printf("\n!!!!!!!! illegal gain entered - try again !!!!!!!!\n");
            else
            {
                sg = key_value2;
                printf("\n  Gain value = %d  (%.1f)\n", sg, gain[sg]);
	        break;
            }
        }
    }

    printf("\n  Current neighbor triggerimg = %d\n", sng);
    printf("\n----- Enter '1' to change value, '0' to keep value: ");
    scanf("%d", &key_value);
    if( key_value)
    {
        while(1)
        {
            printf("\n----- Enter '1' to enable neighbor triggering, '0' to disable:  ");
            scanf("%d", &key_value2);
            if( (key_value2 < 0) || (key_value2 > 1) )
                printf("\n!!!!!!!! illegal value entered - try again !!!!!!!!\n");
            else
            {
                sng = key_value2;
                printf("\n  Neighbor triggerimg = %d\n", sng);
	        break;
            }
        }
    }    

    printf("\n  Current course threshold DAC = %d\n", sdt);
    printf("\n----- Enter '1' to change value, '0' to keep value: ");
    scanf("%d", &key_value);
    if( key_value)
    {
        while(1)
        {
            printf("\n----- Select course threshold DAC value (0 - 1023):  ");
            scanf("%d", &key_value2);
            if( (key_value2 < 0) || (key_value2 > 1023) )
            printf("\n!!!!!!!! illegal coarse threshold entered - try again !!!!!!!!\n");
            else
            {
                sdt = key_value2;
                printf("\n  Current course threshold DAC = %d\n", sdt);
	        break;
            }
        }
    }

    printf("\n  Current test pulse DAC = %d\n", sdp_dac);
    printf("\n----- Enter '1' to change value, '0' to keep value: ");
    scanf("%d", &key_value);
    if( key_value)
    {
        while(1)
        {
            printf("\n----- Select test pulse DAC value (0 - 1023):  ");
            scanf("%d", &key_value2);
            if( (key_value2 < 0) || (key_value2 > 1023) )
                printf("\n!!!!!!!! illegal coarse threshold entered - try again !!!!!!!!\n");
            else
            {
                sdp_dac = key_value2;
                printf("\n  Test pulse DAC = %d\n", sdp_dac);
	        break;
            }
        }
    }

    printf("\n  Current TAC slope value = %d  (%d ns)\n", stc, tac_slope_ns[stc]);
    printf("\n----- Enter '1' to change value, '0' to keep value: ");
    scanf("%d", &key_value);
    if( key_value)
    {
        while(1)
        {
            printf("\n----- Select TAC slope value (0 = 60ns, 1 = 100ns, 2 = 350ns, 3 = 650ns): ");
            scanf("%d", &key_value2);
            if( (key_value2 <= 0) || (key_value2 > 3) )
                printf("\n!!!!!!!! illegal peaktime entered - try again !!!!!!!!\n");
            else
            {
                stc = key_value2;
                printf("\n  TAC slope value = %d  (%d ns)\n", stc, tac_slope_ns[stc]);
                break;
            }
        }
    }
    
// ----------------------------------------------------------------------------------
// list masked channels  
  j = 0;
  for(i = 0; i < 64; i++)
  {
      if( sm_ch[i] == 1 )
      {
          masked[j] = i;
          j++;
      }
  }
  if( j == 0 )
      printf("\n----- NO channels masked -----\n");
  else
  {
      printf("\n----- Masked channels: \n");
      for(i = 0; i < j; i++)
          printf("%4d\n", masked[i]);
  }
  
    printf("\n----- Enter '1' to change masked channel list, '0' to keep as is: ");
    scanf("%d", &key_value);
    if( key_value)
    {
        for(i=0; i<64; i++)
            masked[i] = 0;
        printf("  ALL channels are now unmasked\n\n");
        
        while(1)
        {
            printf("  Enter channel number (0-63) to mask (-1 to end):  ");
            scanf("%d", &key_value);
            if( key_value < 0 )
            {
                break;
            }
            else if( key_value > 63 )
                printf("\n!!!!!!!! illegal channel entered - try again !!!!!!!!\n");
            else
                masked[key_value] = 1;
        }
        for(i=0; i<64; i++)		// copy masked list to global variable
            sm_ch[i] = masked[i];
        printf("  Masked list updated\n\n");
    }    
            
// ----------------------------------------------------------------------------------
// list test pulsed channels  
  j = 0;
  for(i = 0; i < 64; i++)
  {
      if( st_ch[i] == 1 )
      {
          pulsed[j] = i;
          j++;
      }
  }
  if( j == 0 )
      printf("\n----- NO channels test pulsed -----\n");
  else
  {
      printf("\n----- Test pulsed channels: \n");
      for(i = 0; i < j; i++)
          printf("%4d\n", pulsed[i]);
  }
                 
    printf("\n----- Enter '1' to change pulsed channel list, '0' to keep as is: ");
    scanf("%d", &key_value);
    if( key_value )
    {
        printf("\n----- Enter '0' to clear list, '1' to pulse odd, '2' to pulse even, '3' to pulse all,\n");
        printf(  "            '4' to add to list, '5' for new list:  ");
        scanf("%d", &key_value2);
        if( key_value2 == 0 )		// clear pulsed list
        {
            for(i=0; i<64; i++)
                pulsed[i] = 0;
            printf("  Pulsed channel list cleared\n\n");
        }
        else if( key_value2 == 1 )
        {
            for(i=0; i<64; i++)		// clear pulsed list
                pulsed[i] = 0;
            for(i=0; i<32; i++)		// pulse odd
                pulsed[(2*i) + 1] = 1;
            printf("  Pulse odd channels\n\n");
        }
        else if( key_value2 == 2 )
        {
            for(i=0; i<64; i++)		// clear pulsed list
                pulsed[i] = 0;
            for(i=0; i<32; i++)		// pulse even
                pulsed[(2*i)] = 1;
            printf("  Pulse even channels\n\n");
        }
        else if( key_value2 == 3 )
        {
            for(i=0; i<64; i++)		// pulse all
                pulsed[i] = 1;
            printf("  Pulse all channels\n\n");
        }       
        else if( (key_value2 == 4) || (key_value2 == 5)  )
        {
            if( key_value2 == 5 )
            {
                for(i=0; i<64; i++)	// clear list
                    pulsed[i] = 0;
            }
            while(1)
            {
                printf("  Enter channel number (0-63) to pulse (-1 to end):  ");
                scanf("%d", &key_value3);
                if( key_value3 < 0 )
                {
                    break;
                }
                else if( key_value3 > 63 )
                    printf("\n!!!!!!!! illegal channel entered - try again !!!!!!!!\n");
                else
                    pulsed[key_value3] = 1;
            }
        }
        for(i=0; i<64; i++)
            st_ch[i] = pulsed[i];
        printf("  Pulsed list updated\n\n");
    }
    
// ----------------------------------------------------------------------------------
 // trim threshold DAC  
  printf("\n----- Trim threshold DAC: \n");
  trim_zero = 1;
  for(i = 0; i < 64; i++)		// scan all channels for non-zero trim value
  {
      if( sd_ch[i] != 0 )		
      {
          trim_zero = 0;		// found at least one channel with non-zero trim
          break;
      }
  }  
  if( trim_zero )
      printf("\n  ALL channels have trim DAC = 0 -----\n");
  else
  {
      for(i = 0; i < 64; i++)
      {
          trim_mv = sd_ch[i]*(-1);
          printf("ch = %2d  sd_ch = %2d  (%d mV)\n", i, sd_ch[i], trim_mv);
      }
  }

    printf("\n----- Enter '1' to change trim threshold DAC, '0' to keep as is: ");
    scanf("%d", &key_value);
    if( key_value )
    {
        printf("\n----- Enter '0' to zero DAC (all ch), '1' to set common DAC value, '2' to add to list, '3' for new list:  ");
        scanf("%d", &key_value2);
        if( key_value2 == 0 )		// zero all DAC values
        {
            for(i=0; i<64; i++)
                trim_dac[i] = 0;
            printf("  Set trim threshold DAC = 0 (all channels)\n\n");
        }
        else if( key_value2 == 1 )
        {
            printf("\n----- Enter common trim DAC value (0-1023):  ");
            scanf("%d", &key_value3);
            if( (key_value3 < 0) || (key_value3 > 1023) )
                printf("\n!!!!!!!! illegal DAC value entered - no changes made !!!!!!!!\n");
            else
            {
                for(i=0; i<64; i++)	// set common trim DAC value
                    trim_dac[i] = key_value3;
            }
        }
        else if( (key_value2 == 2) || (key_value2 == 3)  )
        {
            if( key_value2 == 3 )
            {
                for(i=0; i<64; i++)	// zero all trim DAC values
                    trim_dac[i] = 0;
            }
            while(1)
            {
                printf("  Enter channel number (0-63) to set trim DAC (-1 to end):  ");
                scanf("%d", &key_value4);
                if( key_value4 < 0 )
                {
                    break;
                }
                else if( key_value4 > 63 )
                    printf("\n!!!!!!!! illegal channel entered - try again !!!!!!!!\n");
                else
                {
                    printf("  Enter trim DAC value (0-1023) for channel %d:  ", key_value3);
                    scanf("%d", &key_value5);
                    if( (key_value5 < 0) || (key_value5 > 1023) )
                        printf("\n!!!!!!!! illegal DAC value entered - no changes made !!!!!!!!\n");
                    else
                        trim_dac[key_value4] = key_value5;
                }
            }
        }
        for(i=0; i<64; i++)		// update all trim DAC values
            sd_ch[i] = trim_dac[i];
        printf("  Trim DAC list updated\n\n");
    }
// ----------------------------------------------------------------------------------
  
  
}
// --------------------------------------------------------------------------------------------------------  
// --------------------------------------------------------------------------------------------------------  

  
// --------------------------------------------------------------------------------------------------------  
// --------------------------------------------------------------------------------------------------------  
void modify_L0_parameters(void)
{
    int latency_value;
    int window_value, l0offset_value, rollover_value, offset_value;
    int window_width, latency_current;
    int key_value;

    latency_current = rollover - l0offset;    
    printf("\n  Current Latency (BC clock periods) = %d\n", latency_current);
    printf("\n----- Enter '1' to change value, '0' to keep value: ");
    scanf("%d", &key_value);
    if( key_value)
    {
        while(1)
        {
            printf("\n----- Enter Latency (BC clock periods): ");
            scanf("%d", &latency_value);
            if( (latency_value <= 0) || (latency_value >= 4095) )
                printf("\n!!!!!!!! illegal latency value entered - try again !!!!!!!!\n");
            else
            {
                l0offset_value = 4095 - latency_value;
                rollover_value = 4095;
                offset_value = 0;
                break;
            }
        }      
        l0offset = l0offset_value;    // modify global values - be sure to update memory array in main
        rollover = rollover_value;
        offset = offset_value;
    }
        
    printf("\n  Current window value (BC clock periods) = %d\n", window);
    printf("\n----- Enter '1' to change value, '0' to keep value: ");
    scanf("%d", &key_value);
    if( key_value)
    {
        while(1)
        {
            printf("\n----- Enter Window value (0-7) (width = value + 1 (BC clock periods)): ");
            scanf("%d", &window_width);
            if( (window_width < 0) || (window_width > 7) )
                printf("\n!!!!!!!! illegal window parameter entered - try again !!!!!!!!\n");
            else
            {
                window = window_value;	   // modify global value - be sure to update memory array in main
                break;
            }
        }
    }

}
// --------------------------------------------------------------------------------------------------------  
// --------------------------------------------------------------------------------------------------------  


// --------------------------------------------------------------------------------------------------------  
// --------------------------------------------------------------------------------------------------------  
void import_cfg_parameters(void)
{
  char cfg_filename[200];
  int value;
  int chan;
  int i,j;
  int mem[54];
  unsigned int l0offset_rev, offset_rev, rollover_rev, window_rev, truncate_rev, nskip_rev;	// reversed bit quantities
  unsigned int l0offset_value, offset_value, rollover_value, window_value, truncate_value, nskip_value;
  int latency, L0_enable;
  int reset2;
  int sc_ch1, sc_ch2, sc_ch3; 
  int sl1, sl2, sl3;  		
  int sth1, sth2, sth3; 		
  int smx1, smx2, smx3; 		
  int sz010b1, sz010b2, sz010b3; 	
  int sz08b1, sz08b2, sz08b3;  	
  int sz06b1, sz06b2, sz06b3;  	

  printf("\n\n------------------------------------------------\n");
  printf(" Enter cfg file name (.txt will be appended): "); 
		
  scanf("%s", cfg_filename);
  strcat(cfg_filename, ".txt");
  FILE *f_cfg = fopen(cfg_filename, "r");
  if(f_cfg == NULL)
  {
    printf("ERROR opening cfg input file %s\n", cfg_filename);
    exit(0);
  }

// read data words of file
  printf("cfg file:  %s\n", cfg_filename);
  for(int i = 0; i < 54; i++)
  {
    fscanf(f_cfg, "%X\n", &value);
    mem[i] = value;
    printf("i = %d   %X\n", i, value);
  } 
  fclose(f_cfg);
  printf("finished reading cfg file %s\n", cfg_filename);fflush(stdout);

//-----------------------------------------------------
  for(int i = 0; i < 54; i++)
  {
    mem_word[i] = mem[i];	// copy to global array
  } 
//-----------------------------------------------------  

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// Global registers - first bank	
//  printf("-----------------------------------------------------------------\n");
//  printf("\n----- Global registers - first bank -----\n\n");

  sp   = ((mem[0] >> 31) & 0x1);
  sdp  = ((mem[0] >> 30) & 0x1); 
  sbmx = ((mem[0] >> 29) & 0x1); 
  sbft = ((mem[0] >> 28) & 0x1); 
  sbfp = ((mem[0] >> 27) & 0x1); 
  sbfm = ((mem[0] >> 26) & 0x1); 

  slg  = ((mem[0] >> 25) & 0x1); 
  sm5  = ((mem[0] >> 19) & 0x3F);
  scmx = ((mem[0] >> 18) & 0x1); 
  sfa  = ((mem[0] >> 17) & 0x1);
  sfam = ((mem[0] >> 16) & 0x1); 
  st   = ((mem[0] >> 14) & 0x3); 

  sfm  = ((mem[0] >> 13) & 0x1); 
  sg   = ((mem[0] >> 10) & 0x7); 
  sng  = ((mem[0] >> 9) & 0x1); 
  stot = ((mem[0] >> 8) & 0x1); 
  sttt = ((mem[0] >> 7) & 0x1); 
  ssh  = ((mem[0] >> 6) & 0x1); 

  stc     = ((mem[0] >> 4) & 0x3); 
  sdt     = (((mem[0] << 6) & 0x3C0) | ((mem[1] >> 26) & 0x3F)); 
  sdp_dac = ((mem[1] >> 16) & 0x3FF); 
  sc10b   = ((mem[1] >> 14) & 0x3); 
  sc8b    = ((mem[1] >> 12) & 0x3); 
  sc6b    = ((mem[1] >> 9) & 0x7); 

  s8b    = ((mem[1] >> 8) & 0x1); 
  s6b    = ((mem[1] >> 7) & 0x1); 
  s10b   = ((mem[1] >> 6) & 0x1); 
  sdcks  = ((mem[1] >> 5) & 0x1); 
  sdcka  = ((mem[1] >> 4) & 0x1);
  sdck6b = ((mem[1] >> 3) & 0x1); 

  sdrv  = ((mem[1] >> 2) & 0x1);
  stpp  = ((mem[1] >> 1) & 0x1); 
  res00 = (mem[1] & 0x1); 
  res0  = ((mem[2] >> 31) & 0x1); 
  res1  = ((mem[2] >> 30) & 0x1); 
  res2  = ((mem[2] >> 29) & 0x1); 

  res3  = ((mem[2] >> 28) & 0x1);  
  slvs  = ((mem[2] >> 27) & 0x1); 
  s32   = ((mem[2] >> 26) & 0x1); 
  stcr  = ((mem[2] >> 25) & 0x1); 
  ssart = ((mem[2] >> 24) & 0x1); 
  srec  = ((mem[2] >> 23) & 0x1); 

  stlc   = ((mem[2] >> 22) & 0x1); 
  sbip   = ((mem[2] >> 21) & 0x1); 
  srat   = ((mem[2] >> 20) & 0x1); 
  sfrst  = ((mem[2] >> 19) & 0x1); 
  slvsbc = ((mem[2] >> 18) & 0x1);  
  slvstp = ((mem[2] >> 17) & 0x1); 

  slvstk  = ((mem[2] >> 16) & 0x1); 
  slvsdt  = ((mem[2] >> 15) & 0x1); 
  slvsart = ((mem[2] >> 14) & 0x1); 
  slvstki = ((mem[2] >> 13) & 0x1);  
  slvsena = ((mem[2] >> 12) & 0x1);
  slvs6b  = ((mem[2] >> 11) & 0x1); 

  sL0enaV = ((mem[2] >> 10) & 0x1); 
  slh = ((mem[2] >> 9) & 0x1); 
  slxh = ((mem[2] >> 8) & 0x1);  
  stgc = ((mem[2] >> 7) & 0x1); 
  reset = ((mem[2] >> 1) & 0x1); 
  reset2 = (mem[2] & 0x1); 

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// Global registers - second bank

  l0offset_rev = (mem[53] >> 20) & 0xFFF;	
  offset_rev   = (mem[53] >> 8) & 0xFFF;
  rollover_rev = (mem[52] & 0xFFF);
  window_rev   = (mem[52] >> 12) & 0x7;
  truncate_rev = (mem[52] >> 22) & 0x3F;
  nskip_rev    = (mem[51] & 0x1);

// reverse bit order for second bank parameters
  l0offset_value = reverse_bits((unsigned int)l0offset_rev, 12);
  offset_value   = reverse_bits((unsigned int)offset_rev, 12);
  rollover_value = reverse_bits((unsigned int)rollover_rev, 12);
  window_value   = reverse_bits((unsigned int)window_rev, 3);
  truncate_value = reverse_bits((unsigned int)truncate_rev, 6);
  nskip_value    = reverse_bits((unsigned int)nskip_rev, 7);
	
  l0offset = (int)l0offset_value;
  offset   = (int)offset_value;			
  rollover = (int)rollover_value;
  window   = (int)window_value;
  truncate = (int)truncate_value;
  nskip    = (int)nskip_value; 

//  printf("-----------------------------------------------------------------\n");
//  printf("\n----- Global registers - second bank -----\n\n");

  nskipm = (mem[51] & 0x1); 
  sL0cktest = ((mem[52] >> 31) & 0x1); 
  sL0dckinv = ((mem[52] >> 30) & 0x1); 
  sL0ckinv = ((mem[52] >> 29) & 0x1);  
  sL0ena = ((mem[52] >> 28) & 0x1); 

  L0_enable = sL0ena;  
  latency = rollover - l0offset;    
 
//  printf("-----------------------------------------------------------------\n");


//--------------------------------------------------------------------------------------------------------------------------------------------------
// Channel registers - (24-bits x 64)  4 channels stored in 3 32-bit words  				  
//------------------------------------------------------------------------------	
//  printf("\n----- Channel registers -----\n");
 
  chan = 0;
  for(i=1; i<=16; i++)
  {
    j=3*i;

// -----------------------------------
//    printf("\n----- Channel %d\n", chan);
  
    sc_ch = ((mem[j] >> 31) & 0x1);  
    sl    = ((mem[j] >> 30) & 0x1);
    st_ch[chan] = ((mem[j] >> 29) & 0x1);  
    sth   = ((mem[j] >> 28) & 0x1);  
    sm_ch[chan]    = ((mem[j] >> 27) & 0x1); 
    smx   = ((mem[j] >> 26) & 0x1); 

    sd_ch[chan] = ((mem[j] >> 21) & 0x1F); 
    sz010b = ((mem[j] >> 16) & 0x1F);
    sz08b  = ((mem[j] >> 12) & 0xF); 
    sz06b  = ((mem[j] >> 9) & 0x7); 

    chan = chan + 1;

// -----
//    printf("\n----- Channel %d\n", chan);

    sc_ch1 = ((mem[j] >> 7) & 0x1);  
    sl1    = ((mem[j] >> 6) & 0x1); 
    st_ch[chan] = ((mem[j] >> 5) & 0x1); 
    sth1   = ((mem[j] >> 4) & 0x1); 
    sm_ch[chan] = ((mem[j] >> 3) & 0x1); 
    smx1   = ((mem[j] >> 2) & 0x1); 

    sd_ch[chan] = ( ((mem[j] << 3) & 0x18) | ((mem[j+1] >> 29) & 0x7) );  
    sz010b1 = ((mem[j+1] >> 24) & 0x1F); 
    sz08b1 = ((mem[j+1] >> 20) & 0xF); 
    sz06b1  = ((mem[j+1] >> 17) & 0x7); 

    chan = chan + 1;

// -----
//    printf("\n----- Channel %d\n", chan);

    sc_ch2 = ((mem[j+1] >> 15) & 0x1);
    sl2 = ((mem[j+1] >> 14) & 0x1); 
    st_ch[chan] = ((mem[j+1] >> 13) & 0x1); 
    sth2 = ((mem[j+1] >> 12) & 0x1); 
    sm_ch[chan] =  ((mem[j+1] >> 11) & 0x1); 
    smx2 =  ((mem[j+1] >> 10) & 0x1); 

    sd_ch[chan] = ((mem[j+1] >> 5) & 0x1F);  
    sz010b2 = (mem[j+1] & 0x1F); 
    sz08b2 = ((mem[j+2] >> 28) & 0xF);
    sz06b2 = ((mem[j+2] >> 25) & 0x7); 

    chan = chan + 1;

// -----
//    printf("\n----- Channel %d\n", chan);

    sc_ch3 = ((mem[j+2] >> 23) & 0x1); 
    sl3 = ((mem[j+2] >> 22) & 0x1); 
    st_ch[chan] = ((mem[j+2] >> 21) & 0x1); 
    sth3 = ((mem[j+2] >> 20) & 0x1); 
    sm_ch[chan] = ((mem[j+2] >> 19) & 0x1); 
    smx3 = ((mem[j+2] >> 18) & 0x1); 

    sd_ch[chan] = ((mem[j+2] >> 13) & 0x1F);
    sz010b3 = ((mem[j+2] >> 8) & 0x1F); 
    sz08b3  = ((mem[j+2] >> 4) & 0xF); 
    sz06b3  = ((mem[j+2] >> 1) & 0x7); 
 
    chan = chan + 1;

// ----------------------------------

  }
// ---------------------------------------------------------------------------------------------------------------------------------------------------
  printf("\n----- Configuration values imported -----\n\n");

}
// --------------------------------------------------------------------------------------------------------  
// --------------------------------------------------------------------------------------------------------  


// --------------------------------------------------------------------------------------------------------  
// --------------------------------------------------------------------------------------------------------  
void make_mem_array(void)
{
// assemble current VMM configuration parameters into memory array suitable for file write 

	unsigned int l0offset_rev, offset_rev, rollover_rev, window_rev, truncate_rev, nskip_rev;	// reversed bit quantities

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
	truncate_rev = reverse_bits((unsigned int)truncate, 6);
	nskip_rev    = reverse_bits((unsigned int)nskip, 7);			
		
// Global registers - second bank	
	mem_word[51] = 	0x00000000 | (nskipm & 0x1) ; 		// 32 BITS
					  
	mem_word[52] = 	((sL0cktest & 0x1) << 31) | ((sL0dckinv & 0x1) << 30) | ((sL0ckinv & 0x1) << 29) | ((sL0ena & 0x1) << 28) | 
			((truncate_rev & 0x3F) << 22) | ((nskip_rev & 0x7F) << 15) | ((window_rev & 0x7) << 12) | (rollover_rev & 0xFFF) ; // 32 BITS
				  
	mem_word[53] = 	((l0offset_rev & 0xFFF) << 20) | ((offset_rev & 0xFFF) << 8) | 0x00000000 ;		// 32 BITS
			
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
}
// --------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------
 
  
// --------------------------------------------------------------------------------------------------------  
// --------------------------------------------------------------------------------------------------------  
unsigned int reverse_bits(unsigned int value, int size)
{
  // Reverses bit order of a bit field of size 'size'.
  unsigned int b[size];
  unsigned int value_rev;
  int i;
  int y;
    
  for(i = 0; i < size; i++) b[size] = 0;

  y = 1;
  for(i = 0; i < size; i++)		// build bit array for 'value'
  {
    b[i] = (value & y) >> i;
    y = y*2;
  }

  value_rev = 0;
  for(i = 0; i < size; i++)		// build bit reversed 'value_rev'
  {
    value_rev = value_rev | ( b[i] << (size - i - 1));
  }
    
  return(value_rev);
}


















