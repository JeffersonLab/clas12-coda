/*======================================================================*/
/*
  Filename     : SFI.H

  Autor        : PS
  Datum        : 95/02/09

  Sprache      : C
  Standard     : K&R

  Inhalt       : Constant definitions for SFI.C

*/
/*======================================================================*/
/*
  Datum, Name    Bemerkungen
  ----------------------------------------------------------------------
  95/02/09 PS    Ersterstellung

  3/10/96  DJA   Updated for new SFI resisters for AUX port access
*/
/*======================================================================*/


#ifndef __SFI_H
#define __SFI_H

struct sfiStruct 
{
  volatile unsigned int  VMESlaveAddress;
  volatile unsigned int* lca1Vout;     /* output register -> LEDs, ECLout, NIMout */
  volatile unsigned int* outregWrite;  
  volatile unsigned int* outregRead;
  volatile unsigned int* dfctrlReg;
  volatile unsigned int* fbProtReg;
  volatile unsigned int* fbArbReg;
  volatile unsigned int* fbCtrlReg;
  volatile unsigned int* lca1Reset;
  volatile unsigned int* lca2Reset;
  volatile unsigned int* fastbusreadback;
  volatile unsigned int* sequencerRamAddressReg;
  volatile unsigned int* sequencerFlowControlReg;

  volatile unsigned int* resetVme2SeqFifo;
  volatile unsigned int* readClockVme2SeqFifo;
  volatile unsigned int* resetSeq2VmeFifo;
  volatile unsigned int* writeClockSeq2VmeFifo;
  volatile unsigned int* writeVme2SeqFifoBase;
  volatile unsigned int* readSeq2VmeFifoBase;

  volatile unsigned int* readSeqFifoFlags;
  volatile unsigned int* readVme2SeqAddressReg;
  volatile unsigned int* readVme2SeqDataReg;

  volatile unsigned int  sequencerOutputFifoSize;

  volatile unsigned int* sequencerReset;
  volatile unsigned int* sequencerEnable;
  volatile unsigned int* sequencerDisable;
  volatile unsigned int* sequencerRamLoadEnable;
  volatile unsigned int* sequencerRamLoadDisable;

  volatile unsigned int* sequencerStatusReg;
  volatile unsigned int* FastbusStatusReg1;
  volatile unsigned int* FastbusStatusReg2;

  volatile unsigned int* FastbusTimeoutReg;
  volatile unsigned int* FastbusArbitrationLevelReg;
  volatile unsigned int* FastbusProtocolInlineReg;
  volatile unsigned int* sequencerFifoFlagAndEclNimInputReg;
  volatile unsigned int* nextSequencerRamAddressReg;
  volatile unsigned int* lastSequencerProtocolReg;
  volatile unsigned int* VmeIrqLevelAndVectorReg;
  volatile unsigned int* VmeIrqMaskReg;
  volatile unsigned int* resetRegisterGroupLca2;
  volatile unsigned int* sequencerTryAgain;

  volatile unsigned int* readVmeTestReg;
  volatile unsigned int* readLocalFbAdBus;
  volatile unsigned int* readVme2SeqDataFifo;
  volatile unsigned int* writeVmeOutSignalReg;
  volatile unsigned int* clearBothLca1TestReg;
  volatile unsigned int* writeVmeTestReg;
  volatile unsigned int* writeAuxReg;
  volatile unsigned int* generateAuxB40Pulse;
};

/* Base address for A24/D32 access of SFI VME slave */
#define SFI_VME_BASE_ADDR    0x00E00000

/* address offsets for test design */

#define SFI_TEST_LCA1_RESET          (0x1004)
#define SFI_TEST_LCA1_VOUT           (0x1000)
#define SFI_TEST_OUTREG_WRITE        (0x1008)
#define SFI_TEST_OUTREG_READ 	     (0x1004)
#define SFI_TEST_FASTBUS_READBACK    (0x1008)


#define SFI_TEST_SEQ_FIFO_FLAG_REGISTER     (0x200C)


#define SFI_TEST_DATA_FLOW_CONTROL_REGISTER (0x2000)
#define SFI_TEST_FB_PROTOCOL_REGISTER       (0x2004)
#define SFI_TEST_FB_ARBITRATION_REGISTER    (0x2008)
#define SFI_TEST_FB_CONTROL_REGISTER        (0x200C)
#define SFI_TEST_RESET                      (0x2020)

#define SFI_TEST_SEQUENCER_RAM_ADDRESS_REGISTER  (0x2010)
#define SFI_TEST_SEQUENCER_FLOW_CONTROL_REGISTER (0x2014)
#define SFI_TEST_RESET_VME2SEQ_FIFO              (0x2030)
#define SFI_TEST_READ_CLOCK_VME2SEQ_FIFO         (0x2034)
#define SFI_TEST_RESET_SEQ2VME_FIFO              (0x2038)
#define SFI_TEST_WRITE_CLOCK_SEQ2VME_FIFO        (0x203C)

#define SFI_TEST_READ_SEQ2VME_FIFO_BASE          (0x4000)
#define SFI_TEST_WRITE_VME2SEQ_FIFO_BASE         (0x10000)

#define SFI_TEST_READ_VME2SEQ_ADDRESS_REGISTER   (0x2008)
#define SFI_TEST_READ_VME2SEQ_DATA_REGISTER      (0x100C)

#define SFI_TEST_SEQUENCER_RESET  	         (0x3000)
#define SFI_TEST_SEQUENCER_ENABLE	         (0x3000)
#define SFI_TEST_SEQUENCER_DISABLE	         (0x3000)
#define SFI_TEST_SEQUENCER_RAM_LOAD_ENABLE	 (0x3000)
#define SFI_TEST_SEQUENCER_RAM_LOAD_DISABLE	 (0x3000)


/* address offsets for normal operation design */

#define SFI_READ_LOCAL_FB_AD_BUS	        (0x1000)
#define SFI_READ_LAST_FB_PRIM_ADDR_REGISTER	(0x1004)
#define SFI_READ_PRIM_ADDR_COUNT_REGISTER	(0x1008)
#define SFI_READ_VME2SEQ_DATA_FIFO		(0x100C)

#define SFI_WRITE_VME_OUT_SIGNAL_REGISTER	(0x1100)
#define SFI_CLEAR_BOTH_LCA1_TEST_REGISTER	(0x1104)
#define SFI_WRITE_VME_TEST_REGISTER 		(0x1108)
#define SFI_WRITE_AUX_REGISTER                  (0x1110)
#define SFI_GENERATE_AUX_B40_PULSE              (0x1114)

#define SFI_FASTBUS_TIMEOUT_REGISTER		                (0x2000)
#define SFI_FASTBUS_ARBITRATION_LEVEL_REGISTER	                (0x2004)
#define SFI_FASTBUS_PROTOCOL_INLINE_REGISTER	                (0x2008)
#define SFI_SEQUENCER_FIFO_FLAG_AND_ECL_NIM_INPUT_REGISTER 	(0x200C)

#define SFI_NEXT_SEQUENCER_RAM_ADDRESS_REGISTER	                (0x2018)
#define SFI_LAST_SEQUENCER_PROTOCOL_REGISTER	                (0x201C)
#define SFI_SEQUENCER_STATUS_REGISTER		                (0x2220)
#define SFI_FASTBUS_STATUS_REGISTER1		                (0x2224)
#define SFI_FASTBUS_STATUS_REGISTER2		                (0x2228)

#define SFI_VME_IRQ_LEVEL_AND_VECTOR_REGISTER	(0x2010)
#define SFI_VME_IRQ_MASK_REGISTER		(0x2014)
#define SFI_SEQUENCER_RAM_ADDRESS_REGISTER	(0x2018)
#define SFI_RESET_REGISTER_GROUP_LCA2		(0x201C)

#define SFI_SEQUENCER_ENABLE			(0x2020)
#define SFI_SEQUENCER_DISABLE			(0x2024)
#define SFI_SEQUENCER_RAM_LOAD_ENABLE		(0x2028)
#define SFI_SEQUENCER_RAM_LOAD_DISABLE		(0x202C)
#define SFI_SEQUENCER_RESET  			(0x2030)
#define SFI_SEQUENCER_TRY_AGAIN			(0x2034) 

#define SFI_READ_SEQ2VME_FIFO_BASE          (0x4000)
#define SFI_WRITE_VME2SEQ_FIFO_BASE         (0x10000)


/* Masks for FB DMA transfers */
#define SFI_DMA_MASK             0x3f000000
#define SFI_DMA_VME_TO           0x20000000
#define SFI_DMA_FB_TO            0x10000000
#define SFI_DMA_LIMIT            0x08000000
#define SFI_DMA_SS2              0x02000000

/* Masks for SFI Output Signals */
#define SFI_TTL1                0x0001
#define SFI_TTL2                0x0002
#define SFI_TTL3                0x0004
#define SFI_TTL4                0x0008
#define SFI_ECL1                0x0010
#define SFI_ECL2                0x0020
#define SFI_ECL3                0x0040
#define SFI_ECL4                0x0080
#define SFI_NIM1                0x0100
#define SFI_NIM2                0x0200
#define SFI_NIM3                0x0400

/* constants for sequencer key addresses */
#define PRIM_DSR		(0x0004)
#define PRIM_CSR		(0x0104)
#define PRIM_DSRM		(0x0204)
#define PRIM_CSRM		(0x0304)
#define PRIM_AMS4		(0x0404)
#define PRIM_AMS5		(0x0504)
#define PRIM_AMS6		(0x0604)
#define PRIM_AMS7		(0x0704)

#define PRIM_HM_DSR		(0x0014)
#define PRIM_HM_CSR		(0x0114)
#define PRIM_HM_DSRM		(0x0214)
#define PRIM_HM_CSRM		(0x0314)
#define PRIM_HM_AMS4		(0x0414)
#define PRIM_HM_AMS5		(0x0514)
#define PRIM_HM_AMS6		(0x0614)
#define PRIM_HM_AMS7		(0x0714)

#define PRIM_EG			(0x1000)

#define RNDM_R			(0x0844)
#define RNDM_W			(0x0044)
#define SECAD_R			(0x0A44)
#define SECAD_W			(0x0244)

#define RNDM_R_DIS		(0x0854)
#define RNDM_W_DIS		(0x0054)
#define SECAD_R_DIS		(0x0A54)
#define SECAD_W_DIS		(0x0254)

#define DISCON			(0x0024)
#define DISCON_RM		(0x0034)

#define START_FRDB_WITH_CLEAR_WORD_COUNTER  (0x08A4)
#define START_FRDB              (0x08B4)

#define STORE_FRDB_WC 		(0x00E4)
#define STORE_FRDB_AP		(0x00D4)

#define LOAD_DMA_ADDRESS_POINTER (0x0094)

/* Non-FASTBUS Sequencer Commands */
#define WRITE_SEQUENCER_OUT_REG (0x0008)

#define DISABLE_SEQUENCER       (0x0018)

#define ENABLE_RAM_SEQUENCER    (0x0028)

#define DISABLE_RAM_MODE	(0x0038)

#endif
