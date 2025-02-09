#ifndef ECALTriggerBoardRegs_H
#define ECALTriggerBoardRegs_H


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

#define ECAL_BOARD_ADDRESS_1			0x11980000/*0x11A00000*/
#define ECAL_BOARD_ADDRESS_2			0x11980000/*0x11A00000*/

#define ECAL_FW_REVISION				0x1000
#define ECAL_CFG_SECTOR					0x1004
#define ECAL_TRIG0_SCALER				0x1008

#define ECAL_REVISION				        0x2000
#define ECAL_ENABLE_SCALERS				0x2004
#define ECAL_REF_SCALER				        0x2008


/************************************************/
/************** BEGIN SCALER REGISTERS **********/
/************************************************/
/* Notes:
   1) Scalers are all 32bits, BIG-ENDIAN.
   2) ECAL_REF_SCALER is a reference scaler which contains gate time of all scalers (in 25ns ticks)
   3) Set TS_ENABLE_SCALERS to '1' to enable scalers. Set to '0' to stop scalers for readout.
      Setting back to '1' will clear all scalers and allow them to count again.
   4) Scalers are capable of counting at 100MHz, which is about 43sec before overflowing at this high rate
*/


#define ECAL_U_DELAY_BASE               0x1200
#define ECAL_V_DELAY_BASE               0x1400
#define ECAL_W_DELAY_BASE               0x1600
#define ECAL_U_SCALER_BASE              0x1800
#define ECAL_V_SCALER_BASE              0x1A00
#define ECAL_W_SCALER_BASE              0x1C00



/************************************************/
/******** BEGIN SECTOR TRIGGER REGISTERS ********/
/************************************************/
/* Notes:
	bits (17:16) ECPCC config
		"00" => ECP and CC
		"01" => CC
		"10" => ECP
		"11" => 0
*/


/****************/
/* DSC2 scalers */

#define ECAL_DSC2_ADDRESS_1			0x00100000
#define ECAL_DSC2_ADDRESS_2			0x00380000
#define ECAL_DSC2_ADDRESS_3			0x00600000
#define ECAL_DSC2_ADDRESS_4			0x00880000

#define ECAL_DSC2_SCALER_LATCH      0x009C
#define ECAL_DSC2_SCALER_GATE       0x00BC
#define ECAL_DSC2_SCALER_BASE       0x0100
#define ECAL_DSC2_SCALER_REF        0x0204

#endif
