/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench
--
-- Module Name:    Spi_Ad9222.c
-- Description:    Spi functions
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

#include "bitmanip.h"
#include "Platform.h"
#include "CBus.h"
#include "Feu.h"
#include "ReturnCodes.h"
#include "Spi_Ad9222.h"

#define D_FEU_SC_REG_ADR D_Feu_AdrReg_Set( D_CBus_SetModType( 0, D_CBus_Mod_Feu ), C_Feu_AdrReg_SlowCtrl )

/*
 Spi_AdcRegReadWrite
 Returns
	D_RetCode_Err_Wrong_Param
	D_RetCode_Err_WrRd_Missmatch
	D_RetCode_Sucsess
 */
int Spi_AdcRegReadWrite(int op, unsigned short adr, unsigned char *val )
{
	int index;

	unsigned char *data;

	unsigned int feu_sc_reg_rd;
	unsigned int feu_sc_reg_wr;

	// read the slow control register
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);

	// Clr SPI CS bit
	feu_sc_reg_wr = D_Feu_RegSC_Adc_Cs_Clr( feu_sc_reg_rd );
	// Set SPI clock bit
	feu_sc_reg_wr = D_Feu_RegSC_Adc_Clk_Set( feu_sc_reg_wr );
	Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	if
	(
		D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_wr)
		||
		D_Feu_RegSC_Adc_Cs_Get(feu_sc_reg_rd)  != D_Feu_RegSC_Adc_Cs_Get(feu_sc_reg_wr)
	)
	{
		sprintf( feu_msg, "Spi_AdcRegReadWrite: WrRd_Missmatch for Clk/CS wr=0x%08x rd=0x%08x", feu_sc_reg_wr, feu_sc_reg_rd );
		return D_RetCode_Err_WrRd_Missmatch;
	}

	// Clear TsD bit: enable output towards ADC
	feu_sc_reg_wr = D_Feu_RegSC_Adc_TsD_Clr( feu_sc_reg_rd );
	// Set SPI CS bit
	feu_sc_reg_wr = D_Feu_RegSC_Adc_Cs_Set( feu_sc_reg_wr );
	// Prepare the read/write bit on the serial data input of the ASIC
	if( op == D_SPI_ADC_READ )
		feu_sc_reg_wr = D_Feu_RegSC_Adc_WrD_Set( feu_sc_reg_wr );
	else
		feu_sc_reg_wr = D_Feu_RegSC_Adc_WrD_Clr( feu_sc_reg_wr );
	// Clr SPI clock bit
	feu_sc_reg_wr = D_Feu_RegSC_Adc_Clk_Clr( feu_sc_reg_wr );
	Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	if
	(
		D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_wr)
		||
		D_Feu_RegSC_Adc_WrD_Get(feu_sc_reg_rd) != D_Feu_RegSC_Adc_WrD_Get(feu_sc_reg_wr)
		||
		D_Feu_RegSC_Adc_Cs_Get(feu_sc_reg_rd) != D_Feu_RegSC_Adc_Cs_Get(feu_sc_reg_wr)
	)
	{
		sprintf( feu_msg, "Spi_AdcRegReadWrite: WrRd_Missmatch for Clk/WrD/CS wr=0x%08x rd=0x%08x", feu_sc_reg_wr, feu_sc_reg_rd );
		return D_RetCode_Err_WrRd_Missmatch;
	}

	// Set SPI clock bit: Write R/NW bit
	feu_sc_reg_wr = D_Feu_RegSC_Adc_Clk_Set( feu_sc_reg_rd );
	Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	if
	(
		D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_wr)
	)
	{
		sprintf( feu_msg, "Spi_AdcRegReadWrite: WrRd_Missmatch for Clk_Set wr=0x%08x rd=0x%08x", feu_sc_reg_wr, feu_sc_reg_rd );
		return D_RetCode_Err_WrRd_Missmatch;
	}

	// Prepare W0
	feu_sc_reg_wr = D_Feu_RegSC_Adc_WrD_Clr( feu_sc_reg_rd );
	// Clr SPI clock bit
	feu_sc_reg_wr = D_Feu_RegSC_Adc_Clk_Clr( feu_sc_reg_wr );
	Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	if
	(
		D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_wr)
		||
		D_Feu_RegSC_Adc_WrD_Get(feu_sc_reg_rd) != D_Feu_RegSC_Adc_WrD_Get(feu_sc_reg_wr)
	)
	{
		sprintf( feu_msg, "Spi_AdcRegReadWrite: WrRd_Missmatch for Clk/WrD wr=0x%08x rd=0x%08x", feu_sc_reg_wr, feu_sc_reg_rd );
		return D_RetCode_Err_WrRd_Missmatch;
	}
	// Set SPI clock bit: Write W0 bit
	feu_sc_reg_wr = D_Feu_RegSC_Adc_Clk_Set( feu_sc_reg_rd );
	Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	if
	(
		D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_wr)
	)
	{
		sprintf( feu_msg, "Spi_AdcRegReadWrite: WrRd_Missmatch for Clk_Set wr=0x%08x rd=0x%08x", feu_sc_reg_wr, feu_sc_reg_rd );
		return D_RetCode_Err_WrRd_Missmatch;
	}

	// Prepare W1
	feu_sc_reg_wr = D_Feu_RegSC_Adc_WrD_Clr( feu_sc_reg_rd );
	// Clr SPI clock bit
	feu_sc_reg_wr = D_Feu_RegSC_Adc_Clk_Clr( feu_sc_reg_wr );
	Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	if
	(
		D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_wr)
		||
		D_Feu_RegSC_Adc_WrD_Get(feu_sc_reg_rd) != D_Feu_RegSC_Adc_WrD_Get(feu_sc_reg_wr)
	)
	{
		sprintf( feu_msg, "Spi_AdcRegReadWrite: WrRd_Missmatch for Clk/WrD wr=0x%08x rd=0x%08x", feu_sc_reg_wr, feu_sc_reg_rd );
		return D_RetCode_Err_WrRd_Missmatch;
	}
	// Set SPI clock bit: Write W1 bit
	feu_sc_reg_wr = D_Feu_RegSC_Adc_Clk_Set( feu_sc_reg_rd );
	Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	if
	(
		D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_wr)
	)
	{
		sprintf( feu_msg, "Spi_AdcRegReadWrite: WrRd_Missmatch for Clk_Set wr=0x%08x rd=0x%08x", feu_sc_reg_wr, feu_sc_reg_rd );
		return D_RetCode_Err_WrRd_Missmatch;
	}

	// write address
	for( index=0; index<13; index++ )
	{
		// Prepare Address bit
		if( adr & 0x1000 )
			feu_sc_reg_wr = D_Feu_RegSC_Adc_WrD_Set( feu_sc_reg_rd  );
		else
			feu_sc_reg_wr = D_Feu_RegSC_Adc_WrD_Clr( feu_sc_reg_rd  );
		// Clr SPI clock bit
		feu_sc_reg_wr = D_Feu_RegSC_Adc_Clk_Clr( feu_sc_reg_wr );
		Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
		feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
		if
		(
			D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_wr)
			||
			D_Feu_RegSC_Adc_WrD_Get(feu_sc_reg_rd) != D_Feu_RegSC_Adc_WrD_Get(feu_sc_reg_wr)
		)
		{
			sprintf( feu_msg, "Spi_AdcRegReadWrite: %d WrRd_Missmatch for Clk/WrD wr=0x%08x rd=0x%08x", index, feu_sc_reg_wr, feu_sc_reg_rd );
			return D_RetCode_Err_WrRd_Missmatch;
		}
		// Set SPI clock bit: Write address bit
		feu_sc_reg_wr = D_Feu_RegSC_Adc_Clk_Set( feu_sc_reg_rd );
		Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
		feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
		if
		(
			D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_wr)
		)
		{
			sprintf( feu_msg, "Spi_AdcRegReadWrite: %d WrRd_Missmatch for Clk_Set wr=0x%08x rd=0x%08x", index, feu_sc_reg_wr, feu_sc_reg_rd );
			return D_RetCode_Err_WrRd_Missmatch;
		}

		// Shift address
		adr = (adr << 1) & 0x1FFE;
	}

	// For read operations Tristate output
	// and init read value
	if( op == D_SPI_ADC_READ )
	{
		// Set TsD bit: tristate output towards ADC
		feu_sc_reg_wr = D_Feu_RegSC_Adc_TsD_Set( feu_sc_reg_rd );
		Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
		feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
		if
		(
			D_Feu_RegSC_Adc_TsD_Get(feu_sc_reg_rd) != D_Feu_RegSC_Adc_TsD_Get(feu_sc_reg_wr)
		)
		{
			sprintf( feu_msg, "Spi_AdcRegReadWrite: WrRd_Missmatch for TsD_Set wr=0x%08x rd=0x%08x", feu_sc_reg_wr, feu_sc_reg_rd );
			return D_RetCode_Err_WrRd_Missmatch;
		}
		// Init read value
		*val = 0;
	}

	// perform data byte operations
	data = val;
	for( index=0; index<8; index++ )
	{
		// Clr SPI clock bit
		feu_sc_reg_wr = D_Feu_RegSC_Adc_Clk_Clr( feu_sc_reg_rd );
		// Prepare next serial bit to be written in case of writes
		if( op == D_SPI_ADC_WRITE )
		{
			if( *data & 0x80 )
				feu_sc_reg_wr = D_Feu_RegSC_Adc_WrD_Set( feu_sc_reg_wr );
			else
				feu_sc_reg_wr = D_Feu_RegSC_Adc_WrD_Clr( feu_sc_reg_wr );
//printf( "Spi_AdcRegReadWrite: val=0x%02x index=%d bit=%d\n", *val, index, D_Feu_RegSC_Adc_WrD_Get( feu_sc_reg_wr ) );
			*data = ((*data) << 1) & 0xFE;
		}
		// Put the serial clock
		Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
		feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
		if
		(
			D_Feu_RegSC_Pulse_WrD_Get(feu_sc_reg_rd) != D_Feu_RegSC_Pulse_WrD_Get(feu_sc_reg_wr)
			||
			D_Feu_RegSC_Pulse_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Pulse_Clk_Get(feu_sc_reg_wr)
		)
		{
			sprintf( feu_msg, "Spi_AdcRegReadWrite: %d WrRd_Missmatch for WrD wr=0x%08x rd=0x%08x",
				index, feu_sc_reg_wr, feu_sc_reg_rd );
			return D_RetCode_Err_WrRd_Missmatch;
		}

		// Capture the serial data output in case of a read
		if( op == D_SPI_ADC_READ )
		{
			*data = ( ( (*data) << 1) & 0xFE ) | D_Feu_RegSC_Adc_RdD_Get( feu_sc_reg_rd );
		}

		// Set SPI clock bit
		feu_sc_reg_wr = D_Feu_RegSC_Adc_Clk_Set( feu_sc_reg_rd );
		// Put the serial clock
		Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
		feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
		if
		(
			D_Feu_RegSC_Pulse_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Pulse_Clk_Get(feu_sc_reg_wr)
		)
		{
			sprintf( feu_msg, "Spi_AdcRegReadWrite: %d WrRd_Missmatch for Clk wr=0x%08x rd=0x%08x",
				index, feu_sc_reg_wr, feu_sc_reg_rd );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}

	// Clr SPI clock bit
	feu_sc_reg_wr = D_Feu_RegSC_Adc_Clk_Clr( feu_sc_reg_rd );
	Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	if
	(
		D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Adc_Clk_Get(feu_sc_reg_wr)
	)
	{
		sprintf( feu_msg, "Spi_AdcRegReadWrite: WrRd_Missmatch for Clk_Clr wr=0x%08x rd=0x%08x", feu_sc_reg_wr, feu_sc_reg_rd );
		return D_RetCode_Err_WrRd_Missmatch;
	}

	// Clr SPI CS bit
	feu_sc_reg_wr = D_Feu_RegSC_Adc_Cs_Clr( feu_sc_reg_rd );
	// Set TsD bit: tristate output towards ADC
	feu_sc_reg_wr = D_Feu_RegSC_Adc_TsD_Set( feu_sc_reg_wr );
	Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	if
	(
		D_Feu_RegSC_Adc_TsD_Get(feu_sc_reg_rd) != D_Feu_RegSC_Adc_TsD_Get(feu_sc_reg_wr)
		||
		D_Feu_RegSC_Adc_Cs_Get(feu_sc_reg_rd)  != D_Feu_RegSC_Adc_Cs_Get(feu_sc_reg_wr)
	)
	{
		sprintf( feu_msg, "Spi_AdcRegReadWrite: WrRd_Missmatch for TsD/CS wr=0x%08x rd=0x%08x", feu_sc_reg_wr, feu_sc_reg_rd );
		return D_RetCode_Err_WrRd_Missmatch;
	}

	// All went fine
	return( D_RetCode_Sucsess );
}
