/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
--
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Front End Unit
--
-- Module Name:    i2c_Max16031.c
-- Description:    Functions to control I2C Max16031 system monitor chip
--
-- Target Devices: V6 / FEU development kit
-- Tool versions:  Xilinx SDK 14.3
--
-- Create Date:    0.0 2013/05/31 IM
-- Revision:
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#include "i2c_Max16031.h"
#include "Semaphores.h"
#include "CBus.h"
#include "Feu.h"
#include "ReturnCodes.h"
#include "Platform.h"
#include "bitmanip.h"

#define DEF_MAX_WRITE    0
#define DEF_MAX_READ     1
#define DEF_MAX_CTRLCODE 0x30
#define DEF_MAX_WrData_Wait_Us 5000
#define DEF_MAX_Wait_Us 10
#define DEF_MAX_TimeOut 1000000

#define D_FEU_SC_REG_ADR D_Feu_AdrReg_Set( D_CBus_SetModType( 0, D_CBus_Mod_Feu ), C_Feu_AdrReg_SlowCtrl )
int Spi_Max_SdaRd()
{
	unsigned int feu_sc_reg_rd;

	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
//printf( "Spi_Max_SdaRd: rd=0x%08x %d\n\r", feu_sc_reg_rd, D_Feu_RegSC_Sc_RdD_Getv(feu_sc_reg_rd) );
	return D_Feu_RegSC_Sc_RdD_Get(feu_sc_reg_rd);
}
int Spi_Max_Scl(int val)
{
	unsigned int feu_sc_reg_rd;
	unsigned int feu_sc_reg_wr;

	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	feu_sc_reg_wr = D_Feu_RegSC_Sc_Clk_Put( feu_sc_reg_rd, val );
	Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	if( D_Feu_RegSC_Sc_Clk_Get(feu_sc_reg_rd) != D_Feu_RegSC_Sc_Clk_Get(feu_sc_reg_wr) )
	{
		sprintf( feu_msg, "Spi_Max_Scl: WrRd_Missmatch wr=0x%08x rd=0x%08x", feu_sc_reg_wr, feu_sc_reg_rd );
		return D_RetCode_Err_WrRd_Missmatch;
	}
//printf( "Spi_Max_Scl: rd=0x%08x %d\n\r", feu_sc_reg_rd, D_Feu_RegSC_1Wire_LdAct_Get(feu_sc_reg_rd) );
	Spi_Wait(DEF_MAX_Wait_Us, DEF_MAX_TimeOut);
	return D_RetCode_Sucsess;
}
int Spi_Max_SdaWr(int val)
{
	unsigned int feu_sc_reg_rd;
	unsigned int feu_sc_reg_wr;

	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	feu_sc_reg_wr = D_Feu_RegSC_Sc_WrD_Put( feu_sc_reg_rd, val );
	Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	if( D_Feu_RegSC_Sc_WrD_Get(feu_sc_reg_rd) != D_Feu_RegSC_Sc_WrD_Get(feu_sc_reg_wr) )
	{
		sprintf( feu_msg, "Spi_Max_SdaWr: WrRd_Missmatch wr=0x%08x rd=0x%08x", feu_sc_reg_wr, feu_sc_reg_rd );
//printf( "Spi_Max_SdaWr: WrRd_Missmatch wr=0x%08x rd=0x%08x\n\r", feu_sc_reg_wr, feu_sc_reg_rd );
		return D_RetCode_Err_WrRd_Missmatch;
	}
//printf( "Spi_Max_SdaWr: rd=0x%08x %d\n\r", feu_sc_reg_rd, D_Feu_RegSC_Sc_WrD_Get(feu_sc_reg_rd) );
	Spi_Wait(DEF_MAX_Wait_Us, DEF_MAX_TimeOut);
	return D_RetCode_Sucsess;
}
int Spi_Max_SdaDir(int val)
{
	unsigned int feu_sc_reg_rd;
	unsigned int feu_sc_reg_wr;

	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	feu_sc_reg_wr = D_Feu_RegSC_Sc_TsD_Put( feu_sc_reg_rd, val );
	Poke(D_FEU_SC_REG_ADR, feu_sc_reg_wr);
	feu_sc_reg_rd = Peek(D_FEU_SC_REG_ADR);
	if( D_Feu_RegSC_Sc_TsD_Get(feu_sc_reg_rd) != D_Feu_RegSC_Sc_TsD_Get(feu_sc_reg_wr) )
	{
		sprintf( feu_msg, "Spi_Max_SdaDir: WrRd_Missmatch wr=0x%08x rd=0x%08x", feu_sc_reg_wr, feu_sc_reg_rd );
//printf( "Spi_Max_SdaDir: WrRd_Missmatch wr=0x%08x rd=0x%08x\n\r", feu_sc_reg_wr, feu_sc_reg_rd );
		return D_RetCode_Err_WrRd_Missmatch;
	}
//printf( "Spi_Max_SdaDir: rd=0x%08x %d\n\r", feu_sc_reg_rd, D_Feu_RegSC_Sc_TsD_Get(feu_sc_reg_rd) );
//	Spi_Max_Wait();
	return D_RetCode_Sucsess;
}

/*******************************************************************************
 I2C_Eeprom_ReadWriteByte
*******************************************************************************/
int I2C_Max16031_ReadWriteByte(int adr, unsigned char dir, unsigned char cw, unsigned char *cr)
{
	int i;
	int j;
	unsigned char byt[4];
	unsigned char do_start[4];
	unsigned char do_ack[4];	
	unsigned char do_stop[4];
	int nb_loop;
	unsigned char ack_b;
	
	if (dir == DEF_MAX_WRITE)
	{
		do_start[0] = 1;
		byt[0]      = DEF_MAX_CTRLCODE | DEF_MAX_WRITE;
//printf("I2C_Max16031_ReadWriteByte: write byt[0]=0x%02x\n\r", byt[0]);
		do_ack[0]   = 1;
		do_stop[0]  = 0;
		
		do_start[1] = 0;
		byt[1]      = adr & 0xFF;
		do_ack[1]   = 1;
		do_stop[1]  = 0;
		
		do_start[2] = 0;
		byt[2]      = cw;
		do_ack[2]   = 1;
		do_stop[2]  = 1;
		
		nb_loop     = 3;
	}
	else
	{
		do_start[0] = 1;
		byt[0]      = DEF_MAX_CTRLCODE | DEF_MAX_WRITE;
//printf("I2C_Max16031_ReadWriteByte: read byt[0]=0x%02x\n\r", byt[0]);
		do_ack[0]   = 1;
		do_stop[0]  = 0;
		
		do_start[1] = 0;
		byt[1]      = adr & 0xFF;
		do_ack[1]   = 1;
		do_stop[1]  = 0;

		do_start[2] = 1;
		byt[2]      = DEF_MAX_CTRLCODE | DEF_MAX_READ;
		do_ack[2]   = 1;
		do_stop[2]  = 0;	
		
		do_start[3] = 0;
		byt[3]      = 0;
		do_ack[3]   = 0;
		do_stop[3]  = 1;	
		
		nb_loop     = 4;
		*cr         = 0;
	}
	
	// Set SCL and SDA high (default inactive state)
	Spi_Max_Scl(1);
	Spi_Max_SdaDir(DEF_MAX_WRITE);
	Spi_Max_SdaWr(1);
	
	for (j=0;j<nb_loop;j++)
	{
//printf("I2C_Eeprom_ReadWriteByte: j=%d byt=0x%02x\n\r", j, byt[j]);
		// Make a START
		if (do_start[j])
		{
			Spi_Max_SdaWr(1);
			Spi_Max_Scl(1);
			Spi_Max_SdaWr(0);
		}

		// Last byte in read is actual data
		if (j == 3)
		{
			// Set SDA as input
			Spi_Max_SdaDir(DEF_MAX_READ);
		}

		// Send the current byte MSB first
		for (i=0;i<8;i++)
		{
			// Set the clock low
			Spi_Max_Scl(0);

			// Put bit of data
			if (byt[j] & 0x80)
			{
				Spi_Max_SdaWr(1);
			}
			else
			{
				Spi_Max_SdaWr(0);
			}
			byt[j] = byt[j] << 1;

			// Set the clock high
			Spi_Max_Scl(1);
			/*
			if( (j == 2) && (dir == DEF_MAX_WRITE) )
			{
				// Data Write operation
				Spi_Max_Wait(DEF_MAX_WrData_Wait_Us);
			}
			*/
			// Last byte in READ is actual data
			if (j == 3)
			{
				*cr = (*cr << 1)| Spi_Max_SdaRd();
			}
		}
	
		// Set the clock low
		Spi_Max_Scl(0);

		// Set SDA low
		Spi_Max_SdaWr(0);
		
		// Set SDA as input
		Spi_Max_SdaDir(DEF_MAX_READ);
	
		// Set the clock high
		Spi_Max_Scl(1);
			
		// Watch ACK
		if (do_ack[j])
		{
			// Get ACK
			ack_b = Spi_Max_SdaRd();
	
			// Check ACK 
			if (ack_b)
			{
				Spi_Max_SdaDir(DEF_MAX_WRITE);
				Spi_Max_SdaWr(1);
				return (D_RetCode_Err_I2cSpi_NoAck);
			}

			// Set SDA as output
			Spi_Max_SdaDir(DEF_MAX_WRITE);
		}

		// Set the clock low
		Spi_Max_Scl(0);

		// Make a STOP
		if (do_stop[j])
		{
			// Set SDA as output
			Spi_Max_SdaDir(DEF_MAX_WRITE);
			
			// Set SDA low
			Spi_Max_SdaWr(0);
			
			// Set the clock high
			Spi_Max_Scl(1);
			
			// Set SDA high
			Spi_Max_SdaWr(1);
		}
	} // for (j=0;j<nb_loop;j++)

	if( dir == DEF_MAX_WRITE )
	{
		// Data Write operation
		Spi_Wait(DEF_MAX_WrData_Wait_Us, DEF_MAX_TimeOut);
	}
	return (0);
}

/*******************************************************************************
 I2C_Eeprom_ReadByte
*******************************************************************************/
int I2C_Max16031_ReadByte(int adr, unsigned char *c)
{
	return (I2C_Max16031_ReadWriteByte(adr, DEF_MAX_READ, 0, c));
}

/*******************************************************************************
 I2C_Eeprom_WriteByte
*******************************************************************************/
int I2C_Max16031_WriteByte(int adr, unsigned char c)
{
	return (I2C_Max16031_ReadWriteByte(adr, DEF_MAX_WRITE, c, (unsigned char *)0));
}
