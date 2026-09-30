#ifndef PRADTriggerBoardRegs_H
#define PRADTriggerBoardRegs_H


/* Board Supports VME A32/A24 D32 Accesses (BLT32 only in address range 0x0000-0x0FFC */

#define CLOCK_PERIOD_NS					5
#define	MAX_PRESCALE					1023
#define MAX_DELAY_LONG					1023
#define MAX_DELAY						31
#define MAX_STMULT						24
#define MAX_PERSIST_LONG				255
#define MAX_PERSIST						7

#define BOARDID_A395A 					0x00	// 32CH IN LVDS/ECL INTERFACE
#define BOARDID_A395B 					0x01	// 32CH OUT LVDS INTERFACE
#define BOARDID_A395C 					0x02	// 32CH OUT ECL INTERFACE
#define BOARDID_A395D					0x03	// 8CH I/O SELECT NIM/TTL INTER



//#define PRAD_BOARD_ADDRESS_1			0x11700000
#define PRAD_BOARD_ADDRESS_1			0x700000

//#define PRAD_BOARD_ADDRESS_1			0x100000


#define PRAD_FW_REVISION				0x1000
#define PRAD_CFG_SECTOR					0x1004




#define NTRIG 16

#define PRAD_TRIG0_SCALER				0x1008
#define PRAD_TRIG1_SCALER				0x100c
#define PRAD_TRIG2_SCALER				0x1010
#define PRAD_TRIG3_SCALER				0x1014
#define PRAD_TRIG4_SCALER				0x1018
#define PRAD_TRIG5_SCALER				0x101c
#define PRAD_TRIG6_SCALER				0x1020
#define PRAD_TRIG7_SCALER				0x1024
#define PRAD_TRIG8_SCALER				0x1028
#define PRAD_TRIG9_SCALER				0x102c
#define PRAD_TRIG10_SCALER				0x1030
#define PRAD_TRIG11_SCALER				0x1034
#define PRAD_TRIG12_SCALER				0x1038
#define PRAD_TRIG13_SCALER				0x103c
#define PRAD_TRIG14_SCALER				0x1040
#define PRAD_TRIG15_SCALER				0x1044

const int PRAD_TRIG_SCALER_ADRS[NTRIG] = {PRAD_TRIG0_SCALER,
                                          PRAD_TRIG1_SCALER,
					  PRAD_TRIG2_SCALER,
					  PRAD_TRIG3_SCALER,
					  PRAD_TRIG4_SCALER,
					  PRAD_TRIG5_SCALER,
					  PRAD_TRIG6_SCALER,
					  PRAD_TRIG7_SCALER,
					  PRAD_TRIG8_SCALER,
					  PRAD_TRIG9_SCALER,
					  PRAD_TRIG10_SCALER,
					  PRAD_TRIG11_SCALER,
					  PRAD_TRIG12_SCALER,
					  PRAD_TRIG13_SCALER,
					  PRAD_TRIG14_SCALER,
					  PRAD_TRIG15_SCALER };





#define PRAD_TRIG0_LATENCY				0x1048
#define PRAD_TRIG1_LATENCY				0x104c
#define PRAD_TRIG2_LATENCY				0x1050
#define PRAD_TRIG3_LATENCY				0x1054
#define PRAD_TRIG4_LATENCY				0x1058
#define PRAD_TRIG5_LATENCY				0x105c
#define PRAD_TRIG6_LATENCY				0x1060
#define PRAD_TRIG7_LATENCY				0x1064
#define PRAD_TRIG8_LATENCY				0x1068
#define PRAD_TRIG9_LATENCY				0x106c
#define PRAD_TRIG10_LATENCY				0x1070
#define PRAD_TRIG11_LATENCY				0x1074
#define PRAD_TRIG12_LATENCY				0x1078
#define PRAD_TRIG13_LATENCY				0x107c
#define PRAD_TRIG14_LATENCY				0x1080
#define PRAD_TRIG15_LATENCY				0x1084

const int PRAD_TRIG_LATENCY_ADRS[NTRIG] = {PRAD_TRIG0_LATENCY,
                                           PRAD_TRIG1_LATENCY,
                                           PRAD_TRIG2_LATENCY,
                                           PRAD_TRIG3_LATENCY,
                                           PRAD_TRIG4_LATENCY,
                                           PRAD_TRIG5_LATENCY,
                                           PRAD_TRIG6_LATENCY,
                                           PRAD_TRIG7_LATENCY,
                                           PRAD_TRIG8_LATENCY,
                                           PRAD_TRIG9_LATENCY,
                                           PRAD_TRIG10_LATENCY,
                                           PRAD_TRIG11_LATENCY,
                                           PRAD_TRIG12_LATENCY,
                                           PRAD_TRIG13_LATENCY,
                                           PRAD_TRIG14_LATENCY,
                                           PRAD_TRIG15_LATENCY };


#define PRAD_TRIG_PRESCALE				0x1088
#define PRAD_SUM_THRESHOLD				0x108c
#define PRAD_MUL_THRESHOLD				0x1090

#define PRAD_REVISION				        0x2000
#define PRAD_ENABLE_SCALERS				0x2004
#define PRAD_REF_SCALER				        0x2008


/************************************************/
/************** BEGIN SCALER REGISTERS **********/
/************************************************/
/* Notes:
   1) Scalers are all 32bits, BIG-ENDIAN.
   2) PRAD_REF_SCALER is a reference scaler which contains gate time of all scalers (in 25ns ticks)
   3) Set TS_ENABLE_SCALERS to '1' to enable scalers. Set to '0' to stop scalers for readout.
      Setting back to '1' will clear all scalers and allow them to count again.
   4) Scalers are capable of counting at 100MHz, which is about 43sec before overflowing at this high rate
*/


#define PRAD_U_DELAY_BASE               0x1200
#define PRAD_V_DELAY_BASE               0x1400
#define PRAD_W_DELAY_BASE               0x1600

/*
#define PRAD_U_SCALER_BASE              0x1800
#define PRAD_V_SCALER_BASE              0x1A00
#define PRAD_W_SCALER_BASE              0x1C00
*/

#define PRAD_D1_SCALER_BASE              0x1800
#define PRAD_D2_SCALER_BASE              0x1840
#define PRAD_D3_SCALER_BASE              0x1880
#define PRAD_D4_SCALER_BASE              0x18c0
#define PRAD_D5_SCALER_BASE              0x1900
#define PRAD_D6_SCALER_BASE              0x1940
#define PRAD_D7_SCALER_BASE              0x1980
#define PRAD_D8_SCALER_BASE              0x19c0


#endif
