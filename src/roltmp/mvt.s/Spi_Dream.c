/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench
--
-- Module Name:    Spi_Dream.c
-- Description:    Spi functions for Dream
--
-- Target Devices: Virtex-5 / ML507 development kit
-- Tool versions:  ISE XPS 10.1
-- 
-- Create Date:    0.0 2011/09/22 IM
-- Revision:       1.0 2014/01/13 IM val_copy used to avoid pointer overflow for 64-bit operations
--                     2014/04/22 IM Return values apdated according to register length
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
#include "Spi_Dream.h"

#define D_FEU_SC_REG_ADR D_Feu_AdrReg_Set( D_CBus_SetModType( 0, D_CBus_Mod_Feu ), C_Feu_AdrReg_SlowCtrl )

unsigned char spi_dream_reg_size[D_SPI_DREAM_REG_NUM] =
	{ 64, 32, 32, 32, 32, 16, 64, 64, 32, 32, 32, 32, 16 };

/*
 Spi_DreamRegReadWrite
 Returns
	D_RetCode_Err_Wrong_Param
	D_RetCode_Err_WrRd_Missmatch
	D_RetCode_Sucsess
 */
int Spi_DreamRegReadWrite(int op, int dream, unsigned char adr, unsigned short val[4] )
{
	int index;
	int nb_of_loops;
	int dis_cs_at;
	int nb_of_sig_words;

	unsigned short val_copy[5];
	unsigned short *data;

	unsigned int feu_sc_reg_rd;
	unsigned int feu_sc_reg_wr;
	unsigned int spi_dream_rd_bit;

//printf("Spi_DreamRegReadWrite: op=%d dream=%d adr=%d\n\r", op, dream, adr);
	
	// Check parameters
	if( adr >= D_SPI_DREAM_REG_NUM )
	{
		sprintf( feu_msg, "Spi_DreamRegReadWrite: adr=%d >= D_SPI_DREAM_REG_NUM=%d",
			adr, D_SPI_DREAM_REG_NUM );
		return D_RetCode_Err_Wrong_Param;
	}

	// Init value
	if( op == D_SPI_DREAM_READ )
	{
		val_copy[0] = 0;
		val_copy[1] = 0;
		val_copy[2] = 0;
		val_copy[3] = 0;
	}
	else
	{
//		printf("Spi_DreamRegReadWrite: op=%d dream=%d adr=%d val: 0=0x%04x 1=0x%04x 2=0x%04x 3=0x%04x\n\r",
//			op, dream, adr, val[0], val[1], val[2], val[3]);
		val_copy[0] = val[0];
		val_copy[1] = val[1];
		val_copy[2] = val[2];
		val_copy[3] = val[3];
	}
	// read the slow control register
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);

	// Set SPI clock enable bit
	feu_sc_reg_wr = D_Feu_RegSC_Dream_En_Set( feu_sc_reg_rd );
	Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	if( D_Feu_RegSC_Dream_En_Get(feu_sc_reg_rd) != D_Feu_RegSC_Dream_En_Get(feu_sc_reg_wr) )
	{
		sprintf( feu_msg, "Spi_DreamRegReadWrite: WrRd_Missmatch for En_Set wr=0x%08x rd=0x%08x", feu_sc_reg_wr, feu_sc_reg_rd );
		return D_RetCode_Err_WrRd_Missmatch;
	}

	// Set SPI clock bit
	feu_sc_reg_wr = D_Feu_RegSC_Dream_Clk_Set( feu_sc_reg_rd );
	// Prepare the read/write bit on the serial data input of the ASIC
	if( op == D_SPI_DREAM_READ )
		feu_sc_reg_wr = D_Feu_RegSC_Dream_WrD_Set( feu_sc_reg_wr );
	else
		feu_sc_reg_wr = D_Feu_RegSC_Dream_WrD_Clr( feu_sc_reg_wr );
	Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	if
	(
		D_Feu_RegSC_Dream_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Dream_Clk_Get(feu_sc_reg_wr)
		||
		D_Feu_RegSC_Dream_WrD_Get(feu_sc_reg_rd) != D_Feu_RegSC_Dream_WrD_Get(feu_sc_reg_wr)
	)
	{
		sprintf( feu_msg, "Spi_DreamRegReadWrite: WrRd_Missmatch for Clk or WrD wr=0x%08x rd=0x%08x", feu_sc_reg_wr, feu_sc_reg_rd );
		return D_RetCode_Err_WrRd_Missmatch;
	}

	// Set Enable signal of selected dream
//printf("Spi_DreamRegReadWrite: feu_sc_reg_rd=0x%08x dream=%d (1<<dream)=%d... ", feu_sc_reg_rd, dream, 1<<dream);
	feu_sc_reg_wr = D_Feu_RegSC_Dream_Cs_Set( feu_sc_reg_rd, (1<<dream) );
//printf("feu_sc_reg_wr=0x%08x\n\r", feu_sc_reg_wr);
	Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	if( D_Feu_RegSC_Dream_Cs_Get(feu_sc_reg_rd) != D_Feu_RegSC_Dream_Cs_Get(feu_sc_reg_wr) )
	{
		sprintf( feu_msg, "Spi_DreamRegReadWrite: WrRd_Missmatch for Cs of dream=%d wr=0x%08x rd=0x%08x", dream, feu_sc_reg_wr, feu_sc_reg_rd );
		return D_RetCode_Err_WrRd_Missmatch;
	}

	// Determin number of data loops
	nb_of_loops = spi_dream_reg_size[adr] + 4;
	if( op == D_SPI_DREAM_READ )
		nb_of_loops += 2;
	dis_cs_at = nb_of_loops - 4;
	nb_of_sig_words = spi_dream_reg_size[adr] / 16;

	// Load the 7 address bits serially, MSB first
	for( index=0; index<7; index++ )
	{
		// Clear SPI clock bit
		feu_sc_reg_wr = D_Feu_RegSC_Dream_Clk_Clr( feu_sc_reg_rd );
		Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
		feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
		if( D_Feu_RegSC_Dream_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Dream_Clk_Get(feu_sc_reg_wr) )
		{
			sprintf( feu_msg, "Spi_DreamRegReadWrite: %d WrRd_Missmatch for Clk_Clr wr=0x%08x rd=0x%08x", index, feu_sc_reg_wr, feu_sc_reg_rd );
			return D_RetCode_Err_WrRd_Missmatch;
		}

		// Set SPI clock bit
		feu_sc_reg_wr = D_Feu_RegSC_Dream_Clk_Set( feu_sc_reg_rd );
		// Prepare the write bit on the serial data input of the ASIC
//printf("Spi_DreamRegReadWrite: %d feu_sc_reg_rd=0x%08x feu_sc_reg_wr=0x%08x adr=0x%02x adr&0x40=%D... ", index, feu_sc_reg_rd, feu_sc_reg_wr, adr, adr & 0x40);
		if( adr & 0x40 )
			feu_sc_reg_wr = D_Feu_RegSC_Dream_WrD_Set( feu_sc_reg_wr );
		else
			feu_sc_reg_wr = D_Feu_RegSC_Dream_WrD_Clr( feu_sc_reg_wr );
//printf("feu_sc_reg_wr=0x%08x\n\r", feu_sc_reg_wr);
		Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
		feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
		if
		(
			D_Feu_RegSC_Dream_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Dream_Clk_Get(feu_sc_reg_wr)
			||
			D_Feu_RegSC_Dream_WrD_Get(feu_sc_reg_rd) != D_Feu_RegSC_Dream_WrD_Get(feu_sc_reg_wr)
		)
		{
			sprintf( feu_msg, "Spi_DreamRegReadWrite: %d adr=%d WrRd_Missmatch for Clk or WrD wr=0x%08x rd=0x%08x", index, adr, feu_sc_reg_wr, feu_sc_reg_rd );
			return D_RetCode_Err_WrRd_Missmatch;
		}
		adr = ( adr << 1 ) & 0x7E;
	}

	// loop nb_of_loops times for serial read or write
	data = &(val_copy[0]);
	for( index=0; index<nb_of_loops; index++ )
	{
		// Clear SPI clock bit
		feu_sc_reg_wr = D_Feu_RegSC_Dream_Clk_Clr( feu_sc_reg_rd );
		Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
		feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
		if( D_Feu_RegSC_Dream_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Dream_Clk_Get(feu_sc_reg_wr) )
		{
			sprintf( feu_msg, "Spi_DreamRegReadWrite: %d WrRd_Missmatch for Clk_Clr wr=0x%08x rd=0x%08x", index, feu_sc_reg_wr, feu_sc_reg_rd );
			return D_RetCode_Err_WrRd_Missmatch;
		}

		// de-activate chip select on all ASICs
		if( index == dis_cs_at )
		{
			// Clear Enable signal on all dream-s
			feu_sc_reg_wr = D_Feu_RegSC_Dream_Cs_Clr( feu_sc_reg_rd );
			Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
			feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
			if( D_Feu_RegSC_Dream_Cs_Get(feu_sc_reg_rd) != D_Feu_RegSC_Dream_Cs_Get(feu_sc_reg_wr) )
			{
				sprintf( feu_msg, "Spi_DreamRegReadWrite: %d WrRd_Missmatch for Cs_Clr wr=0x%08x rd=0x%08x", index, feu_sc_reg_wr, feu_sc_reg_rd );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		}

		// Capture the serial data output in case of a read
		// but skip the first bit shifted out of the serial link
		if( (op == D_SPI_DREAM_READ) && (index > 0) )
		{
			spi_dream_rd_bit = D_Feu_RegSC_Dream_RdD_Get( feu_sc_reg_rd );
			*data = ((*data << 1) & 0xFFFE) | spi_dream_rd_bit;
//			*data = *data | spi_dream_rd_bit;
			if( (index % 16) == 0 )
			{
//				printf("Spi_DreamRegReadWrite: index=%d data=0x%08x val=0x%08x\n\r", index, data, *data);
				data++;
//printf("Spi_DreamRegReadWrite: index=%d data=0x%08x range 0x%08x - 0x%08x\n\r", index, data, &(val_copy[0]), &(val_copy[4]));
			}
//			if( data == &(val_copy[2]) )
//printf("Spi_DreamRegReadWrite: index=%d data=0x%08x val2=0x%08x\n\r", index, data, *data);
		}

		// Set SPI clock bit
		feu_sc_reg_wr = D_Feu_RegSC_Dream_Clk_Set( feu_sc_reg_rd );

		// Prepare next serial bit to be written in case of writes
		if( op == D_SPI_DREAM_WRITE )
		{
			if( index < dis_cs_at ) // get data bit from supplied parameter
			{
				// Prepare the write bit on the serial data input of the ASIC
				if( *data & 0x8000 )
					feu_sc_reg_wr = D_Feu_RegSC_Dream_WrD_Set( feu_sc_reg_wr );
				else
					feu_sc_reg_wr = D_Feu_RegSC_Dream_WrD_Clr( feu_sc_reg_wr );
				if( (index % 16) == 15 )
//				{
					data++;
//printf("Spi_DreamRegReadWrite: index=%d data=0x%04x\n\r", index, *data);
//				}
				else
					*data = (*data << 1) & 0xFFFE;
			}
			else // no more data bits to be written from parameter (4 trailer serial clock ticks)
				feu_sc_reg_wr = D_Feu_RegSC_Dream_WrD_Clr( feu_sc_reg_wr );
		}

		// Put the serial clock
		Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
		feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
		if
		(
			D_Feu_RegSC_Dream_WrD_Get(feu_sc_reg_rd) != D_Feu_RegSC_Dream_WrD_Get(feu_sc_reg_wr)
			||
			D_Feu_RegSC_Dream_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Dream_Clk_Get(feu_sc_reg_wr)
		)
		{
			sprintf( feu_msg, "Spi_DreamRegReadWrite: %d WrRd_Missmatch for WrD wr=0x%08x rd=0x%08x",
				index, feu_sc_reg_wr, feu_sc_reg_rd );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}

	// reset the serial clock without changing the serial data
	feu_sc_reg_wr = D_Feu_RegSC_Dream_Clk_Clr( feu_sc_reg_rd );
	Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	if( D_Feu_RegSC_Dream_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Dream_Clk_Get(feu_sc_reg_wr) )
	{
		sprintf( feu_msg, "Spi_DreamRegReadWrite: WrRd_Missmatch for data Clk_Clr wr=0x%08x rd=0x%08x", feu_sc_reg_wr, feu_sc_reg_rd );
		return D_RetCode_Err_WrRd_Missmatch;
	}

	// Clear SPI clock enable bit
	feu_sc_reg_wr = D_Feu_RegSC_Dream_En_Clr( feu_sc_reg_rd );
	Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	if( D_Feu_RegSC_Dream_En_Get(feu_sc_reg_rd) != D_Feu_RegSC_Dream_En_Get(feu_sc_reg_wr) )
	{
		sprintf( feu_msg, "Spi_DreamRegReadWrite: WrRd_Missmatch for En_Clr wr=0x%08x rd=0x%08x", feu_sc_reg_wr, feu_sc_reg_rd );
		return D_RetCode_Err_WrRd_Missmatch;
	}

	// Copy acquired data to output
	val[0] = val_copy[0];
	val[1] = ( nb_of_sig_words > 1 ) ? val_copy[1] : 0;
	if( nb_of_sig_words > 2 )
	{
		val[2] = val_copy[2];
		val[3] = val_copy[3];
	}
	else
	{
		val[2] = 0;
		val[3] = 0;
	}
	return( D_RetCode_Sucsess );
}
