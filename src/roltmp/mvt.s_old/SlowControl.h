/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench
--
-- Module Name:    SlowControl.h
-- Description:    Slow control functions
--
-- Target Devices: Virtex-5 / ML507 development kit
-- Tool versions:  ISE XPS 10.1
-- 
-- Create Date:    0.0 2011/09/20 IM
-- Revision:        
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifndef _SC_H
#define _SC_H

/* common variables */
extern unsigned int sc_reg_adr;
extern unsigned int sc_state;

/* SC library states */
#define SEM_STATE_FREE      0
#define SEM_STATE_BUSY      1
#define SEM_STATE_GRANTED   2
#define SEM_STATE_ERROR     3

/*
 * Prototypes for functions in SC.c
 */
int Sem_Init( unsigned int adr );
int Sem_Acquire();
int Sem_Free();

/*
 * SC Parameters structure
 */
typedef struct _SC_Structure
{
	/* Offsets in the SC control register */
	int Cs_Offset;
	int Clk_Offset;
	int DataOut_RNW_Offset;
	int DataOut_Offset;
	int DataIn_Offset;
} SC_Structure;
void SC_Structure_Init( SC_Structure *sc_struct, int Cs_Offset, int Clk_Offset, int DataOut_RNW_Offset, int DataOut_Offset, int DataIn_Offset );

int SC_Reset(   SC_Structure *sc_struct );
int SC_Start(   SC_Structure *sc_struct, int repeated );
int SC_Stop(    SC_Structure *sc_struct );
int SC_BitOut(  SC_Structure *sc_struct, int bit, int last );
int SC_BitIn(   SC_Structure *sc_struct, int ack );
int SC_ByteOut( SC_Structure *sc_struct, int byte );
int SC_ByteIn(  SC_Structure *sc_struct, int ack );
/*
 * High level interface
 */
int SC_WriteByte(   SC_Structure *sc_struct, int adr, int command, unsigned char  data );
int SC_ReadByte(    SC_Structure *sc_struct, int adr, int command, unsigned char *data );
int SC_ReceiveByte( SC_Structure *sc_struct, int adr,              unsigned char *data );

/*
 * SC error values
 */
#define SC_STATUS_OK          0       /* No error */
#define SC_STATUS_NAK         1       /* No ACK from slave */
#define SC_STATUS_POR_SDA_LOW 2       /* SDA is hold low after POR */
#define SC_STATUS_ACK_SDA_LOW 3       /* SDA is hold low after ACK */

#endif
