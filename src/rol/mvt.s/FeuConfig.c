/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    TiConfig.c
-- Description:    Trigger Interface (TI) configuration
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2014/10/13 IM
-- Revision:       1.0 2014/11/20 IM Take into account new Feu_RunCtrl_EvTstExt parameter
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <execinfo.h>

#include "ReturnCodes.h"
#include "Parser.h"

#include "CBus.h"
#include "CBus_Common.h"
#include "CBus_Common.h"
#include "ComChan.h"
#include "Feu.h"
#include "PedMem.h"
#include "ThreshMem.h"
#include "TrigGen.h"
#include "TrigInt.h"
#include "SelfTrig.h"
#include "Spi_Dream.h"
#include "Spi_Ad9222.h"
#include "Semaphores.h"
#include "DrmClk.h"
#include "UdpChan.h"

#include "BecConfigParams.h"
#include "FeuConfigParams.h"

#include "BeuConfig.h"
#include "FeuConfig.h"

/*
 * External log file pointer
 * Standalone application opens log file
 * Clas12 run : readoutlist 1 opens log file
 */
extern FILE *sys_log_fptr;

/*
 * Beu slow control interface
 */
/*******************************************************************
  *************** Beu slow control interface ***********************
  ******************************************************************/
 #ifndef OK
 #define OK 0
 #endif 

#define DEF_CMD_INDEX_MASK 0x1F
#define DEF_FEU_WRITE         0
#define DEF_FEU_READ       0x20
#define DEF_FEU_BCST       0x40
#define DEF_FEU_ERROR      0x40

static int sem_init_done[DEF_MAX_NB_OF_FEU];
static int sem_init_initialized = 0;
static int cmd_index[DEF_MAX_NB_OF_FEU];
static int cmd_index_initialized = 0;

// This is a hidden function, so we rely on the fact that all parameters were correctly set up and checked in calling function
// So no parameters check
#define DEF_LNK_TIME_OUT_SEC 2
#define DEF_LnkToValInit() {.tv_sec=DEF_LNK_TIME_OUT_SEC, .tv_usec=0}
int Beu_CheckFeuLink(int feu, int beu, int lnk)
{
	struct timeval t0;
	struct timeval t1;
	struct timeval dt;
	struct timeval to = DEF_LnkToValInit();
	int ret;
	volatile struct BEUSSP_A24RegStruct *beu_reg_struct;

	// Identify control structure of active BEU
	if( (beu_reg_struct = beu_reg_control[beu]) == NULL )
	{
		fprintf( stderr, "%s: beu_reg_struct NULL for beu %d\n", __FUNCTION__, beu );
		return D_RetCode_Err_Null_Pointer;
	}

	// Check the link
	gettimeofday(&t0,0);
	while(1)
	{
		ret = beusspCheckFeuLink(beu_reg_struct, lnk);
		if( ret < 0 )
		{
			fprintf( stderr, "%s: beusspCheckFeuLink failed for beu=%d lnk=%d\n\r", __FUNCTION__, beu, lnk );
			if( sys_log_fptr != (FILE *)NULL ) fprintf( sys_log_fptr, "%s: beusspCheckFeuLink failed for beu=%d lnk=%d\n\r", __FUNCTION__, beu, lnk );
		}
		else if( ret == 1 )
			break;
		else
		{
			gettimeofday(&t1, 0);
			timersub(&t1,&t0,&dt);
			if( timercmp(&dt,&to,>) )
			{
				fprintf( stderr, "%s: timeout\n\r", __FUNCTION__);
				if( sys_log_fptr != (FILE *)NULL )
				{
					fprintf( sys_log_fptr, "%s: timeout\n\r", __FUNCTION__);
					fprintf( sys_log_fptr, "%s: timeout: t0 sec=%8d usec=%6d\n\r", __FUNCTION__, t0.tv_sec, t0.tv_usec);
					fprintf( sys_log_fptr, "%s: timeout: t1 sec=%8d usec=%6d\n\r", __FUNCTION__, t1.tv_sec, t1.tv_usec);
					fprintf( sys_log_fptr, "%s: timeout: dt sec=%8d usec=%6d\n\r", __FUNCTION__, dt.tv_sec, dt.tv_usec);
					fprintf( sys_log_fptr, "%s: timeout: to sec=%8d usec=%6d\n\r", __FUNCTION__, to.tv_sec, to.tv_usec);
				}
				return D_RetCode_Err_Timeout;
			}
			else
				usleep(1000);
		}
	}
	return D_RetCode_Sucsess;
}
			

// This is a hidden function, so we rely on the fact that all parameters were correctly set up and checked in calling function
// So no parameters check
int Beu_ReqResp(int feu, int beu, int lnk, unsigned int adr, unsigned int data_in, int dir, unsigned int *data_out)
{
	unsigned int wr_adr_wrd;
	unsigned int rd_adr_wrd;
	unsigned int rd_dat_wrd;
	int ret;
	int chack_fail;
	int index;
	volatile struct BEUSSP_A24RegStruct *beu_reg_struct;
	int retry = 5;
	int com_retry = 5;

	// Initiakize command counters if called for the first time
	if( cmd_index_initialized == 0 )
	{
		cmd_index_initialized = 1;
		for( index=0; index<DEF_MAX_NB_OF_FEU; index++ )
		{
			cmd_index[index] = 0;
		}
	}
	
	// Identify control structure of active BEU
	if( (beu_reg_struct = beu_reg_control[beu]) == NULL )
	{
		fprintf( stderr, "%s: beu_reg_struct NULL for beu %d\n", __FUNCTION__, beu ); 
		return D_RetCode_Err_Null_Pointer;
	}
//printf( "%s: beu_reg_struct=0x%08x beu adrgeo=0x%08x feu=%d beu=%d lnk=%d wr_adr_wrd=0x%08x\n",
//__FUNCTION__, beu_reg_struct, beu_reg_struct->adrgeo, feu, beu, lnk, wr_adr_wrd );
//				beusspDisplayAllReg(beu_reg_struct);

	// Prepare command
	while(1)
	{
		cmd_index[feu] = (cmd_index[feu] + 1) & DEF_CMD_INDEX_MASK;
		wr_adr_wrd = cmd_index[feu];
		if( dir == DEF_FEU_READ )
			wr_adr_wrd |= DEF_FEU_READ;
		wr_adr_wrd = ((wr_adr_wrd << 24) & 0xFF000000) | (adr & 0xFFffFF);
	//printf( "%s: feu=%d beu=%d lnk=%d wr_adr_wrd=0x%08x\n", __FUNCTION__, feu, beu, lnk, wr_adr_wrd );
		// Call BEU slowcontrol function
		while(1)
		{
			if( (ret=beusspSendSlowControl(beu_reg_struct, lnk, &wr_adr_wrd, &data_in, &rd_adr_wrd, data_out)) != OK )
			{
				fprintf( stderr, "%s: beusspSendSlowControl fail for feu=%d beu=%d lnk=%d with ret %d on pass %d\n", __FUNCTION__, feu, beu, lnk, ret, com_retry );
				if( sys_log_fptr != (FILE *)NULL )
        {
					fprintf( sys_log_fptr, "%s: beusspSendSlowControl fail for feu=%d beu=%d lnk=%d with ret %d on pass %d\n",
										__FUNCTION__, feu, beu, lnk, ret, com_retry );
 					fprintf( sys_log_fptr, "%s: beusspSendSlowControl adr=0x%08x wr_adr_wrd=0x%08x data_in=0x%08x rd_adr_wrd=x%08x data_out=0x%08x\n",
										__FUNCTION__, adr, wr_adr_wrd, data_in, rd_adr_wrd, data_out );
        }
				com_retry--;
				if( com_retry == 0 )
					return ret;
			}
			break;
		}

		// Check here as musch as one can
		chack_fail = 0;
		if( ((wr_adr_wrd >> 24) & DEF_CMD_INDEX_MASK) != ((rd_adr_wrd >> 24) & DEF_CMD_INDEX_MASK) )
		{
			fprintf( stderr, "%s: for feu=%d beu=%d lnk=%d cmd index missmatch %d expected from 0x%08x - %d received in 0x%08x on pass %d\n",
				__FUNCTION__, feu, beu, lnk, ((wr_adr_wrd >> 24) & DEF_CMD_INDEX_MASK), wr_adr_wrd, ((rd_adr_wrd >> 24) & DEF_CMD_INDEX_MASK), rd_adr_wrd, retry  ); 
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( stderr, "%s: for feu=%d beu=%d lnk=%d cmd index missmatch %d expected from 0x%08x - %d received in 0x%08x on pass %d\n",
					__FUNCTION__, feu, beu, lnk, ((wr_adr_wrd >> 24) & DEF_CMD_INDEX_MASK), wr_adr_wrd, ((rd_adr_wrd >> 24) & DEF_CMD_INDEX_MASK), rd_adr_wrd, retry  );
			chack_fail++;
		}
		if( ((wr_adr_wrd >> 24) & DEF_FEU_READ) != ((rd_adr_wrd >> 24) & DEF_FEU_READ))
		{
			fprintf( stderr, "%s: for feu=%d beu=%d lnk=%d direction missmatch %d expected from 0x%08x - %d received in 0x%08x on pass %d\n",
				__FUNCTION__, feu, beu, lnk, ((wr_adr_wrd >> 24) & DEF_FEU_READ), wr_adr_wrd, ((rd_adr_wrd >> 24) & DEF_FEU_READ), rd_adr_wrd, retry ); 
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr, "%s: for feu=%d beu=%d lnk=%d direction missmatch %d expected from 0x%08x - %d received in 0x%08x on pass %d\n",
					__FUNCTION__, feu, beu, lnk, ((wr_adr_wrd >> 24) & DEF_FEU_READ), wr_adr_wrd, ((rd_adr_wrd >> 24) & DEF_FEU_READ), rd_adr_wrd, retry ); 
			chack_fail++;
		}
		if( ((rd_adr_wrd >> 24) & DEF_FEU_ERROR))
		{
			fprintf( stderr, "%s: for feu=%d beu=%d lnk=%d error set in response rd_adr_wrd=0x%08x (wr_adr_wrd=0x%08x) on pass %d\n",
				__FUNCTION__, feu, beu, lnk, rd_adr_wrd, wr_adr_wrd, retry ); 
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr, "%s: for feu=%d beu=%d lnk=%d error set in response rd_adr_wrd=0x%08x (wr_adr_wrd=0x%08x) on pass %d\n",
					__FUNCTION__, feu, beu, lnk, rd_adr_wrd, wr_adr_wrd, retry ); 
			chack_fail++;
		}
		if( chack_fail == 0 )
			break;
		else
		{
			retry--;
			if( retry == 0 )
				return D_RetCode_Err_NetIO;
		}
	}
	return D_RetCode_Sucsess;
}

/*******************************************************************
 ****************** Peek / Poke definitions ************************
 ****************** Dirty and ugly quickfix ************************
 *******************************************************************/
int PeekPoke_Feu =  0;
int PeekPoke_Beu = -1;
int PeekPoke_Lnk = -1;
int PeekPoke_Err =  0;
char  feu_msg_container[128];
char *feu_msg = feu_msg_container;
#define BT_BUF_SIZE 100
void *bt_buffer[BT_BUF_SIZE];
unsigned int Peek( unsigned int adr )
{
	int ret;
	int data_out;
	char **bt_strings;
	int    bt_nptrs;
	int    bt_index;
	if( (ret=Beu_ReqResp( PeekPoke_Feu, PeekPoke_Beu, PeekPoke_Lnk, adr, 0, DEF_FEU_READ, &data_out)) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: adr=0x%08x for feu=%d beu=%d lnk=%d failed wirth %d\n",
			__FUNCTION__, adr, PeekPoke_Feu, PeekPoke_Beu, PeekPoke_Lnk, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr, "%s: adr=0x%08x for feu=%d beu=%d lnk=%d failed wirth %d\n",
				__FUNCTION__, adr, PeekPoke_Feu, PeekPoke_Beu, PeekPoke_Lnk, ret );
		PeekPoke_Err = ret;
		// try to get the caller function
		bt_nptrs = backtrace(bt_buffer, BT_BUF_SIZE);
		fprintf( stderr, "%s: backtrace() returned %d adresses\n", __FUNCTION__, bt_nptrs );
		bt_strings = backtrace_symbols(bt_buffer, bt_nptrs);
		if( bt_strings == NULL )
		{
			fprintf( stderr, "%s: backtrace_symbols() failed with\n", __FUNCTION__, strerror(errno) );
		}
		else
		{
			for( bt_index=0; bt_index<bt_nptrs; bt_index++ )
				fprintf( stderr, "%s: %s\n", __FUNCTION__, bt_strings[bt_index] );
			free( bt_strings );
		}
		return ret;
	}
	return data_out;
}
int Poke( unsigned int adr, unsigned int dat )
{
	int ret;
	int data_out;
	char **bt_strings;
	int    bt_nptrs;
	int    bt_index;
	if( (ret=Beu_ReqResp( PeekPoke_Feu, PeekPoke_Beu, PeekPoke_Lnk, adr, dat, DEF_FEU_WRITE, &data_out)) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: adr=0x%08x dat=0x%08x for feu=%d beu=%d lnk=%d failed wirth %d\n",
			__FUNCTION__, adr, dat, PeekPoke_Feu, PeekPoke_Beu, PeekPoke_Lnk, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr, "%s: adr=0x%08x for feu=%d beu=%d lnk=%d failed wirth %d\n",
				__FUNCTION__, adr, PeekPoke_Feu, PeekPoke_Beu, PeekPoke_Lnk, ret );
		PeekPoke_Err = ret;
		// try to get the caller function
		bt_nptrs = backtrace(bt_buffer, BT_BUF_SIZE);
		fprintf( stderr, "%s: backtrace() returned %d adresses\n", __FUNCTION__, bt_nptrs );
		bt_strings = backtrace_symbols(bt_buffer, bt_nptrs);
		if( bt_strings == NULL )
		{
			fprintf( stderr, "%s: backtrace_symbols() failed with\n", __FUNCTION__, strerror(errno) );
		}
		else
		{
			for( bt_index=0; bt_index<bt_nptrs; bt_index++ )
				fprintf( stderr, "%s: %s\n", __FUNCTION__, bt_strings[bt_index] );
			free( bt_strings );
		}
		return ret;
	}
//fprintf( stdoutr, "%s: for feu=%d beu=%d lnk=%d adr=0x%68x data_in=0x%08x dataout=0x%08x\n,
//__FUNCTION__, PeekPoke_Feu, PeekPoke_Beu, PeekPoke_Lnk, adr, dat, data_out);
	return D_RetCode_Sucsess;
}

/*****************************************************************
 ******************       Adc Config      ************************
 *****************************************************************/
int AdcConfig( AdcParams *adc_params, int feu_id, int beu_id, int beu_lnk_id )
{
	int ret;
	int reg;
	unsigned char wr_val;
	unsigned char rd_val;

	// First set the ADC registers
	for( reg=1; reg<D_AdcPar_NumOfRegs; reg++ )
	{
		if( adc_params->adc_reg[reg].flg != ((char)AdcRegFlag_Unset) )
		{
//fprintf( stderr,  "%s: Spi_AdcRegWrite(0x%02x, 0x%02x) for feu=%d beu=%d lnk=%d\n",
//	__FUNCTION__, reg, adc_params->adc_reg[reg].val, feu_id, beu_id, beu_lnk_id );
			wr_val = adc_params->adc_reg[reg].val;
			if( (ret = Spi_AdcRegWrite( (unsigned short)reg, &wr_val )) !=  D_RetCode_Sucsess )
			{
				fprintf( stderr,  "%s: Spi_AdcRegWrite(0x%02x, 0x%02x) failed for feu=%d beu=%d lnk=%d with %d\n",
					__FUNCTION__, reg, adc_params->adc_reg[reg].val, feu_id, beu_id, beu_lnk_id, ret );
				if( sys_log_fptr != (FILE *)NULL )
					fprintf( sys_log_fptr,  "%s: Spi_AdcRegWrite(0x%02x, 0x%02x) failed for feu=%d beu=%d lnk=%d with %d\n",
						__FUNCTION__, reg, adc_params->adc_reg[reg].val, feu_id, beu_id, beu_lnk_id, ret );
				return ret;
			}
			// If requested: force Adc to take into account internal register values
			if( adc_params->adc_reg[reg].flg == AdcRegFlag_SetAndUpd )
			{
				wr_val = 0x01;
				if( Spi_AdcRegWrite( 0xFF, &wr_val ) !=  D_RetCode_Sucsess )
				{
					fprintf( stderr,  "%s: Spi_AdcRegWrite( 0xFF, 0x01 ) failed for feu=%d beu=%d lnk=%d with %d\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
					if( sys_log_fptr != (FILE *)NULL )
						fprintf( sys_log_fptr,  "%s: Spi_AdcRegWrite( 0xFF, 0x01 ) failed for feu=%d beu=%d lnk=%d with %d\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
					return ret;
				}
				wr_val = 0x00;
				if( Spi_AdcRegWrite( 0xFF, &wr_val ) !=  D_RetCode_Sucsess )
				{
					fprintf( stderr,  "%s: Spi_AdcRegWrite( 0xFF, 0x00 ) failed for feu=%d beu=%d lnk=%d with %d\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
					if( sys_log_fptr != (FILE *)NULL )
						fprintf( sys_log_fptr,  "%s: Spi_AdcRegWrite( 0xFF, 0x00 ) failed for feu=%d beu=%d lnk=%d with %d\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
					return ret;
				}
			} // if( adc_params->adc_reg[reg].flg == AdcRegFlag_SetAndUpd )
		} // if( feu_params->adc_params.adc_reg[reg].flg != AdcRegFlag_Unset )
	} // for( reg=1; reg<D_AdcPar_NumOfRegs; reg++ )

	// Now verify the ADC registers
	for( reg=1; reg<D_AdcPar_NumOfRegs; reg++ )
	{
		if( adc_params->adc_reg[reg].flg != ((char)AdcRegFlag_Unset) )
		{
			if( (ret = Spi_AdcRegRead( (unsigned short)reg, &rd_val )) !=  D_RetCode_Sucsess )
			{
				fprintf( stderr,  "%s: Spi_AdcRegRead(0x%02x) failed for feu=%d beu=%d lnk=%d with %d\n",
					__FUNCTION__, reg, feu_id, beu_id, beu_lnk_id, ret );
				if( sys_log_fptr != (FILE *)NULL )
					fprintf( sys_log_fptr,  "%s: Spi_AdcRegRead(0x%02x) failed for feu=%d beu=%d lnk=%d with %d\n",
						__FUNCTION__, reg, feu_id, beu_id, beu_lnk_id, ret );
				return ret;
			}
//fprintf( stderr,  "%s: Spi_AdcRegRead(0x%02x)=0x%02x for feu=%d beu=%d lnk=%d with %d\n",
//	__FUNCTION__, reg, (rd_val&0xFF), feu_id, beu_id, beu_lnk_id, ret );
			if( adc_params->adc_reg[reg].val != (rd_val&0xFF) )
			{
				fprintf( stderr, "%s: AdcRead failed for feu=%d beu=%d lnk=%d Adc reg=0x%02x (%d) reg_val_exp=0x%02x != reg_val_rcv=0x%02x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg, reg, adc_params->adc_reg[reg].val, rd_val );
				if( sys_log_fptr != (FILE *)NULL )
					fprintf( sys_log_fptr, "%s: AdcRead failed for feu=%d beu=%d lnk=%d Adc reg=0x%02x (%d) reg_val_exp=0x%02x != reg_val_rcv=0x%02x\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg, reg, adc_params->adc_reg[reg].val, rd_val );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		} // if( adc_params->adc_reg[reg].flg != AdcRegFlag_Unset )
	} // for( reg=1; reg<D_AdcPar_NumOfRegs; reg++ )
	return D_RetCode_Sucsess;
}

/*****************************************************************
 ******************     EEPROM Config     ************************
 *****************************************************************/
int EePromConfig( short ee_prom[D_FeuPar_EeProm_Size], int feu_id, int beu_id, int beu_lnk_id )
{
	int ret;
	int reg;
	unsigned char rd_val;

	// First Write to EE Prom
	for( reg=0; reg<D_FeuPar_EeProm_Size; reg++ )
	{
		if( ee_prom[reg] >= 0 )
		{
			// Write in EE Prom
			if( (ret = I2C_Eeprom_WriteByte( reg, ee_prom[reg] )) !=  D_RetCode_Sucsess )
			{
				fprintf( stderr,  "%s: I2C_Eeprom_WriteByte(0x%02x, 0x%02x) failed for feu=%d beu=%d lnk=%d with %d\n",
					__FUNCTION__, reg, ee_prom[reg], feu_id, beu_id, beu_lnk_id, ret );
				return ret;
			}
		} // if( ee_prom[reg] >= 0 )
	} // for( reg=0; reg<D_FeuPar_EeProm_Size; reg++ )

	// Now verify the Ee prom
	for( reg=0; reg<D_FeuPar_EeProm_Size; reg++ )
	{
		if( ee_prom[reg] >= 0 )
		{
			if( (ret = I2C_Eeprom_ReadByte( reg, &rd_val )) !=  D_RetCode_Sucsess )
			{
				fprintf( stderr,  "%s: I2C_Eeprom_ReadByte(0x%02x) failed for feu=%d beu=%d lnk=%d with %d\n",
					__FUNCTION__, reg, feu_id, beu_id, beu_lnk_id, ret );
				return ret;
			}
			if( (ee_prom[reg]&0xFF) != rd_val )
			{
				fprintf( stderr, "%s: EePromRead failed for feu=%d beu=%d lnk=%d EeProm adr=0x%02x (%d) reg_val_exp=0x%02x != reg_val_rcv=0x%02x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg, reg, ee_prom[reg], rd_val );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		} // if( ee_prom[reg] >= 0 )
	} // for( reg=1; reg<D_FeuPar_EeProm_Size; reg++ )

	return D_RetCode_Sucsess;
}

/*****************************************************************
 *****************     Max16031 Config     ***********************
 *****************************************************************/
int Max16031Config( short max16031[D_FeuPar_Max16031_Size], int feu_id, int beu_id, int beu_lnk_id )
{
	int ret;
	int reg;
	unsigned char wr_val;
	unsigned char rd_val;
	int eeprom_unlocked;

	// First Write to EE Prom
	eeprom_unlocked = 0;
	for( reg=0; reg<D_FeuPar_Max16031_Size; reg++ )
	{
		if( max16031[reg] >= 0 )
		{
			// Starting from the &ddress 128 (0x80) the range is EEPROM
			// One first needs to unlock the EEPROM to allow writing
			// For this check the Lock register 0x5F and if needed togle the lock bit
			if( reg >= 128 )
			{
				if( eeprom_unlocked == 0 )
				{
					if( (ret = I2C_Max16031_ReadByte( 0x5F, &rd_val )) !=  D_RetCode_Sucsess )
					{
						fprintf( stderr,  "%s: I2C_Max16031_ReadByte(0x5F) failed for feu=%d beu=%d lnk=%d with %d\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
						return ret;
					}
					if( rd_val & 0x1 )
					{
						// Write 1 to set the lock bit to 0
						wr_val = 0x01;
						if( (ret = I2C_Max16031_WriteByte( 0x5F, wr_val )) !=  D_RetCode_Sucsess )
						{
							fprintf( stderr,  "%s: I2C_Max16031_WriteByte(0x5F, 0x%02x) failed for feu=%d beu=%d lnk=%d with %d\n",
								__FUNCTION__, wr_val, feu_id, beu_id, beu_lnk_id, ret );
							return ret;
						}
						if( (ret = I2C_Max16031_ReadByte( 0x5F, &rd_val )) !=  D_RetCode_Sucsess )
						{
							fprintf( stderr,  "%s: I2C_Max16031_ReadByte(0x5F) failed for feu=%d beu=%d lnk=%d with %d\n",
								__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
							return ret;
						}
						if( rd_val & 0x1 )
						{
							fprintf( stderr, "%s: Max EEPROM Unlock failed for feu=%d beu=%d lnk=%d rd_val=0x%02x with bit 0 set\n",
								__FUNCTION__, feu_id, beu_id, beu_lnk_id, rd_val );
							return D_RetCode_Err_WrRd_Missmatch;
						}
						eeprom_unlocked = 1;
					}
				}
			}
			// Write in Max16031 register
			wr_val = (unsigned char)(max16031[reg] & 0xFF);
			if( (ret = I2C_Max16031_WriteByte( reg, wr_val )) !=  D_RetCode_Sucsess )
			{
				fprintf( stderr,  "%s: I2C_Max16031_WriteByte(0x%02x, 0x%02x) failed for feu=%d beu=%d lnk=%d with %d\n",
					__FUNCTION__, reg, wr_val, feu_id, beu_id, beu_lnk_id, ret );
				return ret;
			}
			// if the data was written in the EEPROM, whait for 15 ms to finish the write cycle
			if( reg >= 128 )
			{
				usleep(15000);
			}
		} // if( max16031[reg] >= 0 )
	} // for( reg=0; reg<D_FeuPar_Max16031_Size; reg++ )

	// If the EEPROM has been unlocked, lock it again
	if( eeprom_unlocked )
	{
		// Recheck the lock bit
		if( (ret = I2C_Max16031_ReadByte( 0x5F, &rd_val )) !=  D_RetCode_Sucsess )
		{
			fprintf( stderr,  "%s: I2C_Max16031_ReadByte(0x5F) failed for feu=%d beu=%d lnk=%d with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
			return ret;
		}
		if( rd_val & 0x1 )
		{
			fprintf( stderr, "%s: Max EEPROM lock bit expected for feu=%d beu=%d lnk=%d reg_val=0x%02x with bit 1 set\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
		// Write 1 to set the lock bit to 1
		wr_val = 0x01;
		if( (ret = I2C_Max16031_WriteByte( 0x5F, wr_val )) !=  D_RetCode_Sucsess )
		{
			fprintf( stderr,  "%s: I2C_Max16031_WriteByte(0x5F, 0x%02x) failed for feu=%d beu=%d lnk=%d with %d\n",
				__FUNCTION__, wr_val, feu_id, beu_id, beu_lnk_id, ret );
			return ret;
		}
		if( (ret = I2C_Max16031_ReadByte( 0x5F, &rd_val )) !=  D_RetCode_Sucsess )
		{
			fprintf( stderr,  "%s: I2C_Max16031_ReadByte(0x5F) failed for feu=%d beu=%d lnk=%d with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
			return ret;
		}
		if( (rd_val & 0x1) == 0 )
		{
			fprintf( stderr, "%s: Max EEPROM Lock failed for feu=%d beu=%d lnk=%d reg_val=0x%02x with bit 0 set to 0\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}

	// Now verify the max16031
	for( reg=0; reg<D_FeuPar_Max16031_Size; reg++ )
	{
		if( max16031[reg] >= 0 )
		{
			if( (ret = I2C_Max16031_ReadByte( reg, &rd_val )) !=  D_RetCode_Sucsess )
			{
				fprintf( stderr,  "%s: I2C_Max16031_ReadByte(0x%2x) failed for feu=%d beu=%d lnk=%d with %d\n",
					__FUNCTION__, reg, feu_id, beu_id, beu_lnk_id, ret );
				return ret;
			}
			if( (max16031[reg]&0xFF) != rd_val )
			{
				fprintf( stderr, "%s: MaxRead failed for feu=%d beu=%d lnk=%d Max16031 adr=0x%02x (%d) reg_val_exp=0x%02x != reg_val_rcv=0x%02x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg, reg, max16031[reg], rd_val );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		} // if( max16031[reg] >= 0 )
	} // for( reg=1; reg<D_FeuPar_Max16031_Size; reg++ )

	return D_RetCode_Sucsess;
}

/*******************************************************************
 ******************       Dream Write       ************************
 ****************** Dirty and Uglu quickfix ************************
 *******************************************************************/
int DreamWrite( int id, unsigned char reg, int val[4] )
{
	int ret;
	unsigned short dream_val[4];
	dream_val[0] = (unsigned short)val[0];
	dream_val[1] = (unsigned short)val[1];
	dream_val[2] = (unsigned short)val[2];
	dream_val[3] = (unsigned short)val[3];
//fprintf( stdout, "%s: id=%d reg=%2d val[0]=0x%08x dream_val[0]=0x%04x\n", __FUNCTION__, id, reg, val[0], dream_val[0]);
	if( (ret=Spi_DreamRegWrite(id, reg, dream_val)) !=  D_RetCode_Sucsess )
	{
		fprintf(stderr, "%s: Spi_DreamRegWrite failed for d=%d a=%d d=0x%08x 0x%08x 0x%08x 0x%08x with %s\n\r",
			__FUNCTION__, id, reg, dream_val[0], dream_val[1], dream_val[2], dream_val[3], feu_msg );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf(sys_log_fptr, "%s: Spi_DreamRegWrite failed for d=%d a=%d d=0x%08x 0x%08x 0x%08x 0x%08x with %s\n\r",
				__FUNCTION__, id, reg, dream_val[0], dream_val[1], dream_val[2], dream_val[3], feu_msg );
		return ret;
	}
	return D_RetCode_Sucsess;
}
	
/*******************************************************************
 ******************       Dream Read        ************************
 ****************** Dirty and Uglu quickfix ************************
 *******************************************************************/
int DreamRead( int id, unsigned char reg, int val[4] )
{
	int ret;
	int dream;
	unsigned short dream_val[4];
	if( (ret=Spi_DreamRegRead(id, reg, dream_val)) !=  D_RetCode_Sucsess )
	{
		fprintf(stderr, "%s: Spi_DreamRegRead failed for d=%d a=%d d=0x%08x 0x%08x 0x%08x 0x%08x with %s\n\r",
			__FUNCTION__, id, reg, dream_val[0], dream_val[1], dream_val[2], dream_val[3], feu_msg );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf(sys_log_fptr, "%s: Spi_DreamRegRead failed for d=%d a=%d d=0x%08x 0x%08x 0x%08x 0x%08x with %s\n\r",
				__FUNCTION__, id, reg, dream_val[0], dream_val[1], dream_val[2], dream_val[3], feu_msg );
		return ret;
	}
	val[0] = (int)( dream_val[0] & 0xFFFF );
	val[1] = (int)( dream_val[1] & 0xFFFF );
	val[2] = (int)( dream_val[2] & 0xFFFF );
	val[3] = (int)( dream_val[3] & 0xFFFF );
//fprintf( stdout, "%s: id=%d reg=%2d val[0]=0x%08x dream_val[0]=0x%04x\n", __FUNCTION__, id, reg, val[0], dream_val[0]);
	return D_RetCode_Sucsess;
}

/*******************************************************************
 ******************       Dream Config      ************************
 *******************************************************************/
#define Def_DreamRegConfigRetry 5
int DreamRegConfig( int dream, int reg, unsigned short reg_val[4], int feu_id, int beu_id, int beu_lnk_id )
{
	int ret;
	unsigned int wr_val[4];
	unsigned int rd_val[4];
	int retry;

	retry = 0;
	do
	{
		// Write
		wr_val[0] = reg_val[0];
		wr_val[1] = reg_val[1];
		wr_val[2] = reg_val[2];
		wr_val[3] = reg_val[3];
		if( (ret=DreamWrite( dream, reg, wr_val )) != D_RetCode_Sucsess )
		{
			fprintf( stderr,  "%s: DreamWrite failed for feu=%d beu=%d lnk=%d dream=%d reg=%d with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, D_FeuPar_NumOfDreams-1, reg, ret );
			return ret;
		}
		// Read
		if( (ret=DreamRead( dream, reg, rd_val )) != D_RetCode_Sucsess )
		{
			fprintf( stderr,  "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, D_FeuPar_NumOfDreams-1, reg, ret );
			return ret;
		}
		// Compare
		// all types of registers
		if( reg_val[0] != (rd_val[0] & 0xFFFF) )
		{
			if( retry == Def_DreamRegConfigRetry )
			{
				fprintf( stderr, "%s: WrRd_Missmatch for feu=%d beu=%d lnk=%d dream=%d reg=%d wr0=0x%x != rd0=0x%x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_val[0], rd_val[0] );
				return D_RetCode_Err_WrRd_Missmatch;
			}
			retry++;
		}
		// > 16-bit registers
		else if( (reg!=12) && ( reg_val[1] != (rd_val[1] & 0xFFFF) ) )
		{
			if( retry == Def_DreamRegConfigRetry )
			{
				fprintf( stderr, "%s: WrRd_Missmatch for feu=%d beu=%d lnk=%d dream=%d reg=%d wr1=0x%x != rd1=0x%x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_val[1], rd_val[0] );
				return D_RetCode_Err_WrRd_Missmatch;
			}
			retry++;
		}
		// 64-bit registers
		else if( ((6<=reg) || (reg<=7)) && ( reg_val[2] != (rd_val[2] & 0xFFFF) ) )
		{
			if( retry == Def_DreamRegConfigRetry )
			{
				fprintf( stderr, "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr1=0x%x != rd1=0x%x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_val[2], rd_val[2] );
				return D_RetCode_Err_WrRd_Missmatch;
			}
			retry++;
		}
		else if( ((6<=reg) || (reg<=7)) && ( reg_val[3] != (rd_val[3] & 0xFFFF) ) )
		{
			if( retry == Def_DreamRegConfigRetry )
			{
				fprintf( stderr, "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr1=0x%x != rd1=0x%x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_val[3], rd_val[3] );
				return D_RetCode_Err_WrRd_Missmatch;
			}
			retry++;
		}
		else
			retry = 0;
	} while( retry > 0 );

	return D_RetCode_Sucsess;
}

int DreamConfig( DreamParams dream_params[D_FeuPar_NumOfDreams], int feu_id, int beu_id, int beu_lnk_id, int msk )
{
	int ret;
	int dream;
	int reg;
	unsigned int rd_val[4];

	/*
	 * Dream configuration: common to all dreams
	 */
/*
	for( reg=1; reg<D_DreamPar_NumOfRegs; reg++ )
	{
		// Write
		if( dream_params[D_FeuPar_NumOfDreams-1].dream_reg[reg].reg[0] != -1 )
		{
			for( dream=0; dream<D_FeuPar_NumOfDreams-1; dream++ )
			{
				if( (msk & (1 << dream)) == 0 )
				{
					if( (ret=DreamWrite( dream, reg, dream_params[D_FeuPar_NumOfDreams-1].dream_reg[reg].reg )) != D_RetCode_Sucsess )
					{
						fprintf( stderr,  "%s: DreamWrite failed for feu=%d beu=%d lnk=%d dream=%d reg=%d with %d\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, D_FeuPar_NumOfDreams-1, reg, ret );
						return ret;
					}
				}
			}

			// Read & compare
			for( dream=0; dream<D_FeuPar_NumOfDreams-1; dream++ )
			{
				if( (msk & (1 << dream)) == 0 )
				{
					// Read
					if( (ret=DreamRead( dream, reg, rd_val )) != D_RetCode_Sucsess )
					{
						fprintf( stderr,  "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d with %d\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, D_FeuPar_NumOfDreams-1, reg, ret );
						return ret;
					}
					// Compare
					// all types of registers
					if( dream_params[D_FeuPar_NumOfDreams-1].dream_reg[reg].reg[0] != rd_val[0] )
					{
						fprintf( stderr, "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr0=0x%x != rd0=0x%x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, dream_params[D_FeuPar_NumOfDreams-1].dream_reg[reg].reg[0], rd_val[0] );
						return D_RetCode_Err_WrRd_Missmatch;
					}
					// > 16-bit registers
					if( (reg!=12) && (dream_params[D_FeuPar_NumOfDreams-1].dream_reg[reg].reg[1]) != rd_val[1] )
					{
						fprintf( stderr, "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr1=0x%x != rd1=0x%x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, dream_params[D_FeuPar_NumOfDreams-1].dream_reg[reg].reg[1], rd_val[1] );
						return D_RetCode_Err_WrRd_Missmatch;
					}
					// 64-bit registers
					if( (6<=reg) && (reg<=9) )
					{
						if( dream_params[D_FeuPar_NumOfDreams-1].dream_reg[reg].reg[2] != rd_val[2] )
						{
							fprintf( stderr, "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr1=0x%x != rd1=0x%x\n",
								__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, dream_params[D_FeuPar_NumOfDreams-1].dream_reg[reg].reg[2], rd_val[2] );
							return D_RetCode_Err_WrRd_Missmatch;
						}
						if( dream_params[D_FeuPar_NumOfDreams-1].dream_reg[reg].reg[3] != rd_val[3] )
						{
							fprintf( stderr, "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr1=0x%x != rd1=0x%x\n",
								__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, dream_params[D_FeuPar_NumOfDreams-1].dream_reg[reg].reg[3], rd_val[3] );
							return D_RetCode_Err_WrRd_Missmatch;
						}
					}
				} // if( (msk & (1 << dream)) == 0 )
			} // for( dream=0; dream<DEF_MAX_NB_OF_DREAM; dream++ )
		} // if( dream_params[D_FeuPar_NumOfDreams-1].dream_reg[reg].reg[0] != -1 )
	} // for( reg=1; reg<D_DreamPar_NumOfRegs; reg++ )
*/
	/*
	 * Dream configuration: dream specific parameters if any
	 */
	for( dream=0; dream<D_FeuPar_NumOfDreams-1; dream++ )
	{
		if( (msk & (1 << dream)) == 0 )
		{
			for( reg=1; reg<D_DreamPar_NumOfRegs; reg++ )
			{
				if( dream_params[dream].dream_reg[reg].reg[0] != -1 )
				{
					// Write
					if( (ret=DreamWrite( dream, reg, dream_params[dream].dream_reg[reg].reg )) != D_RetCode_Sucsess )
					{
						fprintf( stderr,  "%s: DreamWrite failed for feu=%d beu=%d lnk=%d dream=%d reg=%d with %d\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, D_FeuPar_NumOfDreams-1, reg, ret );
						return ret;
					}
					// Read
					if( (ret=DreamRead( dream, reg, rd_val )) != D_RetCode_Sucsess )
					{
						fprintf( stderr,  "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d with %d\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, D_FeuPar_NumOfDreams-1, reg, ret );
						return ret;
					}
					// Compare
					// all types of registers
					if( dream_params[dream].dream_reg[reg].reg[0] != rd_val[0] )
					{
						fprintf( stderr, "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr0=0x%x != rd0=0x%x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, dream_params[dream].dream_reg[reg].reg[0], rd_val[0] );
						return D_RetCode_Err_WrRd_Missmatch;
					}
					// > 16-bit registers
					if( (reg!=12) && (dream_params[dream].dream_reg[reg].reg[1]) != rd_val[1] )
					{
						fprintf( stderr, "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr1=0x%x != rd1=0x%x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, dream_params[dream].dream_reg[reg].reg[1], rd_val[1] );
						return D_RetCode_Err_WrRd_Missmatch;
					}
					// 64-bit registers
					if( (6<=reg) && (reg<=7) )
					{
						if( dream_params[dream].dream_reg[reg].reg[2] != rd_val[2] )
						{
							fprintf( stderr, "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr1=0x%x != rd1=0x%x\n",
								__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, dream_params[dream].dream_reg[reg].reg[2], rd_val[2] );
							return D_RetCode_Err_WrRd_Missmatch;
						}
						if( dream_params[dream].dream_reg[reg].reg[3] != rd_val[3] )
						{
							fprintf( stderr, "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr1=0x%x != rd1=0x%x\n",
								__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, dream_params[dream].dream_reg[reg].reg[3], rd_val[3] );
							return D_RetCode_Err_WrRd_Missmatch;
						}
					}
				} // if( dream_params[dream].dream_reg[reg].reg[0] != -1 )
			} // for( reg=1; reg<D_DreamPar_NumOfRegs; reg++ )
		} // if( (msk & (1 << dream)) == 0 )
	} // for( dream=0; dream<D_FeuPar_NumOfDreams-1; dream++ )

	return D_RetCode_Sucsess;
}

int DreamConfigCheck( DreamParams dream_params[D_FeuPar_NumOfDreams], int feu_id, int beu_id, int beu_lnk_id, int msk )
{
	int ret;
	int dream;
	int reg;
	unsigned int rd_val[4];

	/*
	 * Dream configuration: dream specific parameters if any
	 */
	for( dream=0; dream<D_FeuPar_NumOfDreams-1; dream++ )
	{
		if( (msk & (1 << dream)) == 0 )
		{
			for( reg=1; reg<D_DreamPar_NumOfRegs; reg++ )
			{
				if( dream_params[dream].dream_reg[reg].reg[0] != -1 )
				{
					// Read
					if( (ret=DreamRead( dream, reg, rd_val )) != D_RetCode_Sucsess )
					{
						fprintf( stderr,  "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d with %d\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, D_FeuPar_NumOfDreams-1, reg, ret );
						return ret;
					}
					// Compare
					// all types of registers
					if( dream_params[dream].dream_reg[reg].reg[0] != rd_val[0] )
					{
						fprintf( stderr, "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr0=0x%x != rd0=0x%x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, dream_params[dream].dream_reg[reg].reg[0], rd_val[0] );
						return D_RetCode_Err_WrRd_Missmatch;
					}
					// > 16-bit registers
					if( (reg!=12) && (dream_params[dream].dream_reg[reg].reg[1]) != rd_val[1] )
					{
						fprintf( stderr, "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr1=0x%x != rd1=0x%x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, dream_params[dream].dream_reg[reg].reg[1], rd_val[1] );
						return D_RetCode_Err_WrRd_Missmatch;
					}
					// 64-bit registers
					if( (6<=reg) && (reg<=7) )
					{
						if( dream_params[dream].dream_reg[reg].reg[2] != rd_val[2] )
						{
							fprintf( stderr, "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr1=0x%x != rd1=0x%x\n",
								__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, dream_params[dream].dream_reg[reg].reg[2], rd_val[2] );
							return D_RetCode_Err_WrRd_Missmatch;
						}
						if( dream_params[dream].dream_reg[reg].reg[3] != rd_val[3] )
						{
							fprintf( stderr, "%s: DreamRead failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr1=0x%x != rd1=0x%x\n",
								__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, dream_params[dream].dream_reg[reg].reg[3], rd_val[3] );
							return D_RetCode_Err_WrRd_Missmatch;
						}
					}
				} // if( dream_params[dream].dream_reg[reg].reg[0] != -1 )
			} // for( reg=1; reg<D_DreamPar_NumOfRegs; reg++ )
		} // if( (msk & (1 << dream)) == 0 )
	} // for( dream=0; dream<D_FeuPar_NumOfDreams-1; dream++ )

	return D_RetCode_Sucsess;
}

/*******************************************************************
 ******************       Dream SPI Config      ********************
 *******************************************************************/

/*****************************************************************
 ******************     Dream Register SPI Config      ***********
 *****************************************************************/
int DreamSpiRegConfig( int dream, int reg, unsigned short reg_val[4], int feu_id, int beu_id, int beu_lnk_id )
{
	int ret;
	int retry;

	unsigned int reg_wr_val_31_00;
	unsigned int reg_wr_val_63_32;

	unsigned int reg_wr_adr_31_00;
	unsigned int reg_wr_adr_63_32;

	unsigned int reg_rd_val_31_00;
	unsigned int reg_rd_val_63_32;

	retry = 0;
	do
	{
		// Write
		reg_wr_adr_31_00 = 0x700000 + (dream << (8+2)) + (reg<<(2+1));
		reg_wr_adr_63_32 = reg_wr_adr_31_00 + 4;
		if( reg == 12 ) // 16-bit register
		{
			reg_wr_val_63_32 = 0;
			reg_wr_val_31_00 = reg_val[0];
		}
		else if( (reg == 6) || (reg == 7) ) // 64-bit registers
		{
			reg_wr_val_63_32 = (reg_val[0] << 16) | reg_val[1];
			reg_wr_val_31_00 = (reg_val[2] << 16) | reg_val[3];
		}
		else // 32-bit registers
		{
			reg_wr_val_63_32 = 0;
			reg_wr_val_31_00 = (reg_val[0] << 16) | reg_val[1];
		}

		// Write LSB-s
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_wr_adr_31_00, reg_wr_val_31_00, DEF_FEU_WRITE, &reg_rd_val_31_00 ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_wr_adr_31_00 );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_wr_adr_31_00 );
			return D_RetCode_Err_NetIO;
		}
		// Write MSB-s
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_wr_adr_63_32, reg_wr_val_63_32, DEF_FEU_WRITE, &reg_rd_val_63_32 ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_wr_adr_63_32 );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_wr_adr_63_32 );
			return D_RetCode_Err_NetIO;
		}
		// Read MSB-s
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_wr_adr_63_32, 0, DEF_FEU_READ, &reg_rd_val_63_32 ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_wr_adr_63_32 );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_wr_adr_63_32 );
			return D_RetCode_Err_NetIO;
		}
		// Read LSB-s
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_wr_adr_31_00, 0, DEF_FEU_READ, &reg_rd_val_31_00 ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_wr_adr_31_00 );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_wr_adr_31_00 );
			return D_RetCode_Err_NetIO;
		}

	//fprintf( stdout, "%s: feu=%d beu=%d lnk=%d dream=%d reg=%d WR 63_32=0x%08x 31_00=0x%08x RD 63_32=0x%08x 31_00=0x%08x\n",
	//__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_63_32, reg_wr_val_31_00, reg_rd_val_63_32, reg_rd_val_31_00 );

		// Compare
		if( reg == 12 ) // 16-bit register
		{
			if( reg_wr_val_31_00 != ((reg_rd_val_63_32 >> 16) & 0xFFFF)  )
			{
				if( retry == Def_DreamRegConfigRetry )
				{
					fprintf( stderr, "%s: WrRd_Missmatch failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr0=0x%x != rd0=0x%x\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_31_00, (reg_rd_val_63_32 >> 16) & 0xFFFF );
					if( sys_log_fptr != (FILE *)NULL )
						fprintf( sys_log_fptr, "%s: WrRd_Missmatch failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr0=0x%x != rd0=0x%x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_31_00, (reg_rd_val_63_32 >> 16) & 0xFFFF );
					return D_RetCode_Err_WrRd_Missmatch;
				}
				retry++;
			}
			else
				retry = 0;
		}
		else if( (reg == 6) || (reg == 7) ) // 64-bit registers
		{
			if( reg_wr_val_63_32 != reg_rd_val_63_32 )
			{
				if( retry == Def_DreamRegConfigRetry )
				{
					fprintf( stderr, "%s: WrRd_Missmatch failed for feu=%d beu=%d lnk=%d dream=%d reg=%d MSB wr0=0x%x != rd0=0x%x\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_63_32, reg_rd_val_63_32 );
					if( sys_log_fptr != (FILE *)NULL )
						fprintf( sys_log_fptr, "%s: WrRd_Missmatch failed for feu=%d beu=%d lnk=%d dream=%d reg=%d MSB wr0=0x%x != rd0=0x%x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_63_32, reg_rd_val_63_32 );
					return D_RetCode_Err_WrRd_Missmatch;
				}
				retry++;
			}
			else if( reg_wr_val_31_00 != reg_rd_val_31_00 )
			{
				if( retry == Def_DreamRegConfigRetry )
				{
					fprintf( stderr, "%s: WrRd_Missmatch failed for feu=%d beu=%d lnk=%d dream=%d reg=%d LSB wr0=0x%x != rd0=0x%x\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_31_00, reg_rd_val_31_00 );
					if( sys_log_fptr != (FILE *)NULL )
						fprintf( sys_log_fptr, "%s: WrRd_Missmatch failed for feu=%d beu=%d lnk=%d dream=%d reg=%d LSB wr0=0x%x != rd0=0x%x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_31_00, reg_rd_val_31_00 );
					return D_RetCode_Err_WrRd_Missmatch;
				}
				retry++;
			}
			else
				retry = 0;
		}
		else // 32-bit registers
		{
			if( reg_wr_val_31_00 != reg_rd_val_63_32 )
			{
				if( retry == Def_DreamRegConfigRetry )
				{
					fprintf( stderr, "%s: WrRd_Missmatch failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr0=0x%x != rd0=0x%x\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_31_00, reg_rd_val_63_32 );
					if( sys_log_fptr != (FILE *)NULL )
						fprintf( sys_log_fptr, "%s: WrRd_Missmatch failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr0=0x%x != rd0=0x%x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_31_00, reg_rd_val_63_32 );
					return D_RetCode_Err_WrRd_Missmatch;
				}
				retry++;
			}
			else
				retry = 0;
		}
/*
if( retry )
{
fprintf( stderr, "%s: Retry %d for feu=%d beu=%d lnk=%d dream=%d reg=%d WR 63_32=0x%08x 31_00=0x%08x RD 63_32=0x%08x 31_00=0x%08x\n",
__FUNCTION__, retry, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_63_32, reg_wr_val_31_00, reg_rd_val_63_32, reg_rd_val_31_00 );
}
*/
	} while( retry > 0 );

	return D_RetCode_Sucsess;
}

int DreamSpiConfig( DreamParams dream_params[D_FeuPar_NumOfDreams], int feu_id, int beu_id, int beu_lnk_id, int msk )
{
	int ret;
	int dream;
	int reg;

	unsigned int reg_wr_val_31_00;
	unsigned int reg_wr_val_63_32;

	unsigned int reg_wr_adr_31_00;
	unsigned int reg_wr_adr_63_32;

	unsigned int reg_rd_val_31_00;
	unsigned int reg_rd_val_63_32;
	
	int retry;

	/*
	 * Dream configuration: dream specific parameters if any
	 */
	for( dream=0; dream<D_FeuPar_NumOfDreams-1; dream++ )
	{
		if( (msk & (1 << dream)) == 0 )
		{
			retry = 0;
			for( reg=1; reg<D_DreamPar_NumOfRegs; reg++ )
			{
				reg_wr_adr_31_00 = 0x700000 + (dream << (8+2)) + (reg<<(2+1));
				reg_wr_adr_63_32 = reg_wr_adr_31_00 + 4;
				if( dream_params[dream].dream_reg[reg].reg[0] != -1 )
				{
					if( reg == 12 ) // 16-bit register
					{
						reg_wr_val_63_32 = 0;
						reg_wr_val_31_00 = dream_params[dream].dream_reg[reg].reg[0];
					}
					else if( (reg == 6) || (reg == 7) ) // 64-bit registers
					{
						reg_wr_val_63_32 = (dream_params[dream].dream_reg[reg].reg[0] << 16) | dream_params[dream].dream_reg[reg].reg[1];
						reg_wr_val_31_00 = (dream_params[dream].dream_reg[reg].reg[2] << 16) | dream_params[dream].dream_reg[reg].reg[3];
					}
					else // 32-bit registers
					{
						reg_wr_val_63_32 = 0;
						reg_wr_val_31_00 = (dream_params[dream].dream_reg[reg].reg[0] << 16) | dream_params[dream].dream_reg[reg].reg[1];
					}

					// Write LSB-s
					if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_wr_adr_31_00, reg_wr_val_31_00, DEF_FEU_WRITE, &reg_rd_val_31_00 ) ) < 0 )
					{
						fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_wr_adr_31_00 );
						return D_RetCode_Err_NetIO;
					}
					// Write MSB-s
					if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_wr_adr_63_32, reg_wr_val_63_32, DEF_FEU_WRITE, &reg_rd_val_63_32 ) ) < 0 )
					{
						fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_wr_adr_63_32 );
						return D_RetCode_Err_NetIO;
					}
					// Read MSB-s
					if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_wr_adr_63_32, 0, DEF_FEU_READ, &reg_rd_val_63_32 ) ) < 0 )
					{
						fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_wr_adr_63_32 );
						return D_RetCode_Err_NetIO;
					}
					// Read LSB-s
					if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_wr_adr_31_00, 0, DEF_FEU_READ, &reg_rd_val_31_00 ) ) < 0 )
					{
						fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_wr_adr_31_00 );
						return D_RetCode_Err_NetIO;
					}

//fprintf( stdout, "%s: feu=%d beu=%d lnk=%d dream=%d reg=%d WR 63_32=0x%08x 31_00=0x%08x RD 63_32=0x%08x 31_00=0x%08x\n",
//__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_63_32, reg_wr_val_31_00, reg_rd_val_63_32, reg_rd_val_31_00 );

					// Compare
					if( reg == 12 ) // 16-bit register
					{
						if( reg_wr_val_31_00 != ((reg_rd_val_63_32 >> 16) & 0xFFFF)  )
						{
							if( retry == Def_DreamRegConfigRetry )
							{
								fprintf( stderr, "%s: WrRd_Missmatch failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr0=0x%x != rd0=0x%x\n",
									__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_31_00, (reg_rd_val_63_32 >> 16) & 0xFFFF );
								return D_RetCode_Err_WrRd_Missmatch;
							}
							retry++;
						}
						else
							retry = 0;
					}
					else if( (reg == 6) || (reg == 7) ) // 64-bit registers
					{
						if( reg_wr_val_63_32 != reg_rd_val_63_32 )
						{
							if( retry == Def_DreamRegConfigRetry )
							{
								fprintf( stderr, "%s: WrRd_Missmatch failed for feu=%d beu=%d lnk=%d dream=%d reg=%d MSB wr0=0x%x != rd0=0x%x\n",
									__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_63_32, reg_rd_val_63_32 );
								return D_RetCode_Err_WrRd_Missmatch;
							}
							retry++;
						}
						else if( reg_wr_val_31_00 != reg_rd_val_31_00 )
						{
							if( retry == Def_DreamRegConfigRetry )
							{
								fprintf( stderr, "%s: WrRd_Missmatch failed for feu=%d beu=%d lnk=%d dream=%d reg=%d LSB wr0=0x%x != rd0=0x%x\n",
									__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_31_00, reg_rd_val_31_00 );
								return D_RetCode_Err_WrRd_Missmatch;
							}
							retry++;
						}
						else
							retry = 0;
					}
					else // 32-bit registers
					{
						if( reg_wr_val_31_00 != reg_rd_val_63_32 )
						{
							if( retry == Def_DreamRegConfigRetry )
							{
								fprintf( stderr, "%s: WrRd_Missmatch failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr0=0x%x != rd0=0x%x\n",
									__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_31_00, reg_rd_val_63_32 );
								return D_RetCode_Err_WrRd_Missmatch;
							}
							retry++;
						}
						else
							retry = 0;
					}
					if( retry )
					{
/*
fprintf( stderr, "%s: Retry %d for feu=%d beu=%d lnk=%d dream=%d reg=%d WR 63_32=0x%08x 31_00=0x%08x RD 63_32=0x%08x 31_00=0x%08x\n",
__FUNCTION__, retry, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_63_32, reg_wr_val_31_00, reg_rd_val_63_32, reg_rd_val_31_00 );
*/
						reg--;
					}
				} // if( dream_params[dream].dream_reg[reg].reg[0] != -1 )
			} // for( reg=1; reg<D_DreamPar_NumOfRegs; reg++ )
		} // if( (msk & (1 << dream)) == 0 )
	} // for( dream=0; dream<D_FeuPar_NumOfDreams-1; dream++ )

	return D_RetCode_Sucsess;
}

int DreamSpiConfigCheck( DreamParams dream_params[D_FeuPar_NumOfDreams], int feu_id, int beu_id, int beu_lnk_id, int msk )
{
	int ret;
	int dream;
	int reg;

	unsigned int reg_wr_val_31_00;
	unsigned int reg_wr_val_63_32;

	unsigned int reg_wr_adr_31_00;
	unsigned int reg_wr_adr_63_32;

	unsigned int reg_rd_val_31_00;
	unsigned int reg_rd_val_63_32;
	
	int retry;

	/*
	 * Dream configuration: dream specific parameters if any
	 */
	for( dream=0; dream<D_FeuPar_NumOfDreams-1; dream++ )
	{
		if( (msk & (1 << dream)) == 0 )
		{
			retry = 0;
			for( reg=1; reg<D_DreamPar_NumOfRegs; reg++ )
			{
				reg_wr_adr_31_00 = 0x700000 + (dream << (8+2)) + (reg<<(2+1));
				reg_wr_adr_63_32 = reg_wr_adr_31_00 + 4;
				if( dream_params[dream].dream_reg[reg].reg[0] != -1 )
				{
					if( reg == 12 ) // 16-bit register
					{
						reg_wr_val_63_32 = 0;
						reg_wr_val_31_00 = dream_params[dream].dream_reg[reg].reg[0];
					}
					else if( (reg == 6) || (reg == 7) ) // 64-bit registers
					{
						reg_wr_val_63_32 = (dream_params[dream].dream_reg[reg].reg[0] << 16) | dream_params[dream].dream_reg[reg].reg[1];
						reg_wr_val_31_00 = (dream_params[dream].dream_reg[reg].reg[2] << 16) | dream_params[dream].dream_reg[reg].reg[3];
					}
					else // 32-bit registers
					{
						reg_wr_val_63_32 = 0;
						reg_wr_val_31_00 = (dream_params[dream].dream_reg[reg].reg[0] << 16) | dream_params[dream].dream_reg[reg].reg[1];
					}

					// Read MSB-s
					if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_wr_adr_63_32, 0, DEF_FEU_READ, &reg_rd_val_63_32 ) ) < 0 )
					{
						fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_wr_adr_63_32 );
						return D_RetCode_Err_NetIO;
					}
					// Read LSB-s
					if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_wr_adr_31_00, 0, DEF_FEU_READ, &reg_rd_val_31_00 ) ) < 0 )
					{
						fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_wr_adr_31_00 );
						return D_RetCode_Err_NetIO;
					}

//fprintf( stdout, "%s: feu=%d beu=%d lnk=%d dream=%d reg=%d WR 63_32=0x%08x 31_00=0x%08x RD 63_32=0x%08x 31_00=0x%08x\n",
//__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_63_32, reg_wr_val_31_00, reg_rd_val_63_32, reg_rd_val_31_00 );

					// Compare
					if( reg == 12 ) // 16-bit register
					{
						if( reg_wr_val_31_00 != ((reg_rd_val_63_32 >> 16) & 0xFFFF)  )
						{
							if( retry == Def_DreamRegConfigRetry )
							{
								fprintf( stderr, "%s: WrRd_Missmatch failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr0=0x%x != rd0=0x%x\n",
									__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_31_00, (reg_rd_val_63_32 >> 16) & 0xFFFF );
								return D_RetCode_Err_WrRd_Missmatch;
							}
							retry++;
						}
						else
							retry = 0;
					}
					else if( (reg == 6) || (reg == 7) ) // 64-bit registers
					{
						if( reg_wr_val_63_32 != reg_rd_val_63_32 )
						{
							if( retry == Def_DreamRegConfigRetry )
							{
								fprintf( stderr, "%s: WrRd_Missmatch failed for feu=%d beu=%d lnk=%d dream=%d reg=%d MSB wr0=0x%x != rd0=0x%x\n",
									__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_63_32, reg_rd_val_63_32 );
								return D_RetCode_Err_WrRd_Missmatch;
							}
							retry++;
						}
						else if( reg_wr_val_31_00 != reg_rd_val_31_00 )
						{
							if( retry == Def_DreamRegConfigRetry )
							{
								fprintf( stderr, "%s: WrRd_Missmatch failed for feu=%d beu=%d lnk=%d dream=%d reg=%d LSB wr0=0x%x != rd0=0x%x\n",
									__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_31_00, reg_rd_val_31_00 );
								return D_RetCode_Err_WrRd_Missmatch;
							}
							retry++;
						}
						else
							retry = 0;
					}
					else // 32-bit registers
					{
						if( reg_wr_val_31_00 != reg_rd_val_63_32 )
						{
							if( retry == Def_DreamRegConfigRetry )
							{
								fprintf( stderr, "%s: WrRd_Missmatch failed for feu=%d beu=%d lnk=%d dream=%d reg=%d wr0=0x%x != rd0=0x%x\n",
									__FUNCTION__, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_31_00, reg_rd_val_63_32 );
								return D_RetCode_Err_WrRd_Missmatch;
							}
							retry++;
						}
						else
							retry = 0;
					}
					if( retry )
					{
/*
fprintf( stderr, "%s: Retry %d for feu=%d beu=%d lnk=%d dream=%d reg=%d WR 63_32=0x%08x 31_00=0x%08x RD 63_32=0x%08x 31_00=0x%08x\n",
__FUNCTION__, retry, feu_id, beu_id, beu_lnk_id, dream, reg, reg_wr_val_63_32, reg_wr_val_31_00, reg_rd_val_63_32, reg_rd_val_31_00 );
*/
						reg--;
					}
				} // if( dream_params[dream].dream_reg[reg].reg[0] != -1 )
			} // for( reg=1; reg<D_DreamPar_NumOfRegs; reg++ )
		} // if( (msk & (1 << dream)) == 0 )
	} // for( dream=0; dream<D_FeuPar_NumOfDreams-1; dream++ )

	return D_RetCode_Sucsess;
}

/********************
 ****  FEU Lock  ****
 *******************/
#define DEF_SEM_TIME_OUT_SEC 5
#define DEF_TimeValInit() {.tv_sec=DEF_SEM_TIME_OUT_SEC, .tv_usec=0}
 int FeuConfig_Lock()
 {
 	int index;
	struct timeval t0;
	struct timeval t1;
	struct timeval dt;
	struct timeval to = DEF_TimeValInit();
	int ret;

	// Initialize sem init done table when called for the first time
	if( sem_init_initialized == 0 )
	{
		sem_init_initialized = 1;
		for( index=0; index<DEF_MAX_NB_OF_FEU; index++ )
		{
			sem_init_done[index] = 0;
		}
	}

	// Initialize semaphore of the FEU when called for the first time
	if( sem_init_done[PeekPoke_Feu] == 0 )
	{
		// Get exclusive access to FEU
		if( (ret=Sem_Init(D_Feu_AdrReg_Set( D_CBus_SetModType( 0, D_CBus_Mod_Feu ), C_Feu_AdrReg_SlowCtrl ))) ==  D_RetCode_Wrn_SemBusy )
		{
			fprintf( stderr,  "%s: Sem_Init warning Semaphore Busy for feu=%d beu=%d lnk=%d\n",
				__FUNCTION__, PeekPoke_Feu, PeekPoke_Beu, PeekPoke_Lnk );
		}
		sem_init_done[PeekPoke_Feu] = 1;
	}

	// Get the semaphore
	gettimeofday(&t0,0);
	while(1)
	{
		ret = Sem_Acquire();
		if( ret == D_RetCode_Sucsess )
			break;
		else if( ret == D_RetCode_Err_Sem )
		{
			fprintf( stderr,  "%s: Sem_Acquire failed with %s\n", __FUNCTION__, sem_msg);
			return D_RetCode_Err_Sem;
		}
		gettimeofday(&t1, 0);
		timersub(&t1,&t0,&dt);
		if( timercmp(&dt,&to,>) )
		{
			fprintf( stderr, "%s: Sem_Acquire timeout\n\r", __FUNCTION__);
printf("%s: Sem_Acquire timeout: t0 sec=%8d usec=%6d\n\r", __FUNCTION__, t0.tv_sec, t0.tv_usec);
printf("%s: Sem_Acquire timeout: t1 sec=%8d usec=%6d\n\r", __FUNCTION__, t1.tv_sec, t1.tv_usec);
printf("%s: Sem_Acquire timeout: dt sec=%8d usec=%6d\n\r", __FUNCTION__, dt.tv_sec, dt.tv_usec);
printf("%s: Sem_Acquire timeout: to sec=%8d usec=%6d\n\r", __FUNCTION__, to.tv_sec, to.tv_usec);
			return D_RetCode_Err_Timeout;
		}
	};
	return D_RetCode_Sucsess;
}

 int FeuConfig_UnLock()
 {
 	int ret;
	// Free the semaphore for others
	if( (ret = Sem_Free()) != D_RetCode_Sucsess )
	{
		fprintf(stderr, "%s: Sem_Free failed with %s\n", __FUNCTION__, sem_msg);
		return ret;
	}
	return D_RetCode_Sucsess;
}
/*******************************************************************
 ************       FEU Memory configuration      ******************
 *******************************************************************/
// Global variable for feu memory configuration files
static FILE *feu_cfg_mem_fptr = (FILE *)NULL;

// Cleanup function
void FeuConfig_CleanUp()
{
	// At least try to free semaphore
	Sem_Free();
	// Close file pointer if any open
	if(feu_cfg_mem_fptr != (FILE *)NULL)
	{
		fclose(feu_cfg_mem_fptr);
		feu_cfg_mem_fptr = (FILE *)NULL;
	}
}

// Pedestal and threshold memory configuration
int FeuMemConfig( char *conf_file_name, int mem_type, int feu_id, int beu_id, int beu_lnk_id )
{
	char line[LINE_SIZE];
	int line_num;
	char *end_ptr;

	unsigned int reg_adr;
	unsigned int wr_val;
	unsigned int rd_val;
	int ret;
//	unsigned int wr_val_swap;

	int buf_index;
	int mem_adr;
	int entry;

	// Check parameters
	if( conf_file_name==NULL )
	{
		fprintf( stderr, "%s: conf_file_name=Null", __FUNCTION__ );
		return D_RetCode_Err_Wrong_Param;
	}
	// Open configuration file
	if( (feu_cfg_mem_fptr=fopen(conf_file_name, "r")) == NULL )
	{
		fprintf( stderr, "%s: fopen failed for config file %s type=%d in read mode\n", __FUNCTION__, conf_file_name, mem_type );
		fprintf( stderr, "%s: fopen failed with %s\n", __FUNCTION__, strerror(errno) );
		return D_RetCode_Err_FileIO;
	}

	// Check parameters a initialize variables
	if( mem_type == D_CBus_Mod_Pedestal )
	{
		reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Pedestal );
		reg_adr = D_PedMem_AdrZone_Set( reg_adr, D_PedMem_AdrZone_Mem );
	}
	else if( mem_type == D_CBus_Mod_Threshold )
	{
		reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Threshold );
		reg_adr = D_ThreshMem_AdrZone_Set( reg_adr, D_ThreshMem_AdrZone_Mem );
	}
	else
	{
		fprintf( stderr, "%s: memory type=%d is neither D_CBus_Mod_Pedestal=%d nor D_CBus_Mod_Threshold=%d",
			__FUNCTION__, mem_type, D_CBus_Mod_Pedestal, D_CBus_Mod_Threshold );
		return D_RetCode_Err_Wrong_Param;
	}

	// Process memory configuration file
	line_num = 0;
	entry = 0;
	while( fgets( line, LINE_SIZE, feu_cfg_mem_fptr ) != NULL )
	{
		line_num++;
		/* parse the line */
		parse_line(line);
		if( argc > 0 )
		{
			// determine bufdfer index and address within the buffer
			buf_index = entry / 64;
			mem_adr = entry % 64;
			entry++;

			// Check if this is a new buffer
			if( mem_adr == 0 )
			{
				// Set address address
				if( mem_type == D_CBus_Mod_Pedestal )
					reg_adr = D_PedMem_BramId_Set( reg_adr, buf_index );
				else
					reg_adr = D_ThreshMem_BramId_Set( reg_adr, buf_index );
			}
			if( mem_type == D_CBus_Mod_Pedestal )
				reg_adr = D_PedMem_BramAdr_Set(reg_adr, (mem_adr<<2));
			else 
				reg_adr = D_ThreshMem_BramAdr_Set(reg_adr, (mem_adr<<2));
			wr_val = strtol( argv[0], &end_ptr, 16 );
//			wr_val_swap = strtol( argv[0], &end_ptr, 16 );
//			wr_val = ((wr_val_swap & 0xFFFF) << 16) + ((wr_val_swap >> 16)&0xFFFF);
//printf("%s: line_num=%d line=%s reg_adr=0x%08x wr_val=0x%08x\n", __FUNCTION__, line_num, line, reg_adr, wr_val);
//getchar();
			// Write
			if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
			{
				fprintf( stderr, "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
				return D_RetCode_Err_NetIO;
			}
			else
			{
				if( wr_val != rd_val )
				{
					fprintf( stderr, "%s: Main config WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, rd_val );
					return D_RetCode_Err_WrRd_Missmatch;
				}
			}
		}
	} // while( 1 ); // Keep reading until end of file

	return D_RetCode_Sucsess;
}

// Pedestal and threshold memory intialisation
int FeuMemInit( int mem_type, int feu_id, int beu_id, int beu_lnk_id )
{
	unsigned int reg_adr;
	unsigned int wr_val;
	unsigned int rd_val;
	int ret;

	int buf_index;
	int max_mem_adr;
	int mem_adr;

	// Check parameters a initialize variables
	if( mem_type == D_CBus_Mod_Pedestal )
	{
		reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Pedestal );
		reg_adr = D_PedMem_AdrZone_Set( reg_adr, D_PedMem_AdrZone_Mem );
//		max_mem_adr = 1024;
		max_mem_adr = 64;
	}
	else if( mem_type == D_CBus_Mod_Threshold )
	{
		reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Threshold );
		reg_adr = D_ThreshMem_AdrZone_Set( reg_adr, D_ThreshMem_AdrZone_Mem );
		max_mem_adr = 64;
	}
	else
	{
		fprintf( stderr, "%s: memory type=%d is neither D_CBus_Mod_Pedestal=%d nor D_CBus_Mod_Threshold=%d",
			__FUNCTION__, mem_type, D_CBus_Mod_Pedestal, D_CBus_Mod_Threshold );
		return D_RetCode_Err_Wrong_Param;
	}

	for( buf_index=0; buf_index<4; buf_index++ )
	{
		if( mem_type == D_CBus_Mod_Pedestal )
			reg_adr = D_PedMem_BramId_Set(  reg_adr, buf_index );
		else
			reg_adr = D_ThreshMem_BramId_Set(  reg_adr, buf_index );
		for( mem_adr=0; mem_adr<max_mem_adr; mem_adr++ )
		{
			if( mem_type == D_CBus_Mod_Pedestal ) 
				reg_adr = D_PedMem_BramAdr_Set(reg_adr, (mem_adr<<2));
			else
				reg_adr = D_ThreshMem_BramAdr_Set(reg_adr, (mem_adr<<2));
//printf("%s: reg_adr=0x%08x\n", __FUNCTION__, reg_adr);
			// Write
			wr_val = 0;
			if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
			{
				fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
				return D_RetCode_Err_NetIO;
			}
			else
			{
				if( rd_val != wr_val )
				{
					fprintf( stderr,  "%s: Main config WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, rd_val );
					return D_RetCode_Err_WrRd_Missmatch;
				}
			}
		} // for( mem_adr=0; mem_adr<max_mem_adr; mem_adr++ )
	} // for( buf_index=0; buf_index<4; buf_index++ )

	return D_RetCode_Sucsess;
}

/************************************************************
 ************      Dream Clock Config      ******************
 ************************************************************/
// The trigger clock frequency is 125 MHz: 8 ns
#define Def_TrgClk_Period_ns 8
#define Drm_RdClk_Div_Min 3.5
#define Drm_WrClk_Div_Min 3

int DrmClkConfig( DrmClkParams *params, int feu_id, int beu_id, int beu_lnk_id )
{
	u32 reg_adr;
	u32 wr_val;
	u32 rd_val;

	int reg;
	char reg_name[64];
	unsigned int half_period;
	unsigned int phase_delay;
	int ret;

	// Check parameters
	if( params == (DrmClkParams *)NULL )
	{
		fprintf( stderr, "%s: DrmClkParams=NULL\n", __FUNCTION__ ); 
		return D_RetCode_Err_Null_Pointer;
	}
	// This function is called from main configuration function
	// The parameters have been already tested

	// Check if the parameters have been set
	if( (params->RdClk_Div < 0) && (params->WrClk_Div < 0) && (params->WrClk_Phase < 0) && (params->AdcClk_Phase < 0) )
	{
		fprintf( stderr, "%s: Nothing to do\n", __FUNCTION__ );
//getchar();
		return D_RetCode_Wrn_NothingToDo;
	}

	// Check if the parameters are correct
	if( (params->RdClk_Div >= 0) && (params->RdClk_Div < Drm_RdClk_Div_Min) )
	{
		fprintf( stderr, "%s: RdClk_Div=%3.1f less than %3.1f\n", __FUNCTION__, params->RdClk_Div, Drm_RdClk_Div_Min ); 
		return D_RetCode_Err_Wrong_Param;
	}
	if( (params->WrClk_Div >= 0) && (params->WrClk_Div < Drm_WrClk_Div_Min) )
	{
		fprintf( stderr, "%s: WrClk_Div=%3.1f less than %3.1f\n", __FUNCTION__, params->WrClk_Div, Drm_WrClk_Div_Min ); 
		return D_RetCode_Err_Wrong_Param;
	}
	if( (params->WrClk_Phase >= 0) && (params->WrClk_Phase>63) )
	{
		fprintf( stderr, "%s: WrClk_Phase=%d not in range [0;63]\n", __FUNCTION__, params->WrClk_Phase ); 
		return D_RetCode_Err_Wrong_Param;
	}
	if( (params->AdcClk_Phase >= 0) && (params->AdcClk_Phase>63) )
	{
		fprintf( stderr, "%s: AdcClk_Phase=%d not in range [0;63]\n", __FUNCTION__, params->AdcClk_Phase ); 
		return D_RetCode_Err_Wrong_Param;
	}

	/*
	 * First reset the MMCM
	 */
/*
	// Send DrmClkRst command
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegCommand );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, 0, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp DEF_FEU_READ failed for feu=%d beu=%d lnk=%d DrmClkRst reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}
	// Set the DrmClkRst bit
	wr_val = D_Main_Cmd_DrmClkRst_Set( rd_val );
//fprintf( stderr, "%s: DrmClkRst adr=0x%08x wr_val=0x%04x\n", __FUNCTION__, reg_adr, wr_val );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp DEF_FEU_WRITE failed for feu=%d beu=%d lnk=%d DrmClkRst reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Config_Get(wr_val) != D_Main_Cmd_Config_Get(rd_val) )
		{
			fprintf( stderr,  "%s: DrmClkRst Set WrRd_Missmatch for feu=%d beu=%d lnk=%d DrmClkRst reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}
//fprintf( stderr, "%s: After MMCM reset\n", __FUNCTION__ );
*/
	/*
	 * Next set MMCM power bits
	 */
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_DrmClk );
	reg_adr = D_TrgClk_AdrReg_Set( reg_adr, C_TrgClk_PwrBits );
	// Set all Power bits
	wr_val = D_TrgClk_RegPower_Set( 0 );
//fprintf( stderr, "%s: PwrBits adr=0x%08x wr_val=0x%04x\n", __FUNCTION__, reg_adr, wr_val );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp DEF_FEU_WRITE failed for feu=%d beu=%d lnk=%d PwrBits reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_TrgClk_RegPower_Get(wr_val) != D_TrgClk_RegPower_Get(rd_val) )
		{
			fprintf( stderr,  "%s: PwrBits Set WrRd_Missmatch for feu=%d beu=%d lnk=%d PwrBits reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}
//fprintf( stderr, "%s: After MMCP Power Bits\n", __FUNCTION__ );

	/*
	 * Next set MMCM Register: RdClock_Div
	 */
	for( reg=0; reg<5; reg++ )
	{
		// Perform operations only for set registers
		// skip others
		if
		(
			( (reg == 0) && (params->RdClk_Div >= 0) )
			||
			( (reg == 1) && (params->RdClk_Div >= 0) )
			||
			( (reg == 2) && (params->WrClk_Div >= 0) )
			||
			( (reg == 3) && (params->WrClk_Phase >= 0) )
			||
			( (reg == 4) && (params->AdcClk_Phase >= 0) )
		)
		{
			// Set address
			if( reg == 0 )
			{
				reg_adr = D_TrgClk_AdrReg_Set( reg_adr, C_TrgClk_RdClk_Freq );
				sprintf( reg_name, "%s%", "RdClk_Freq" );
			}
			if( reg == 1 )
			{
				reg_adr = D_TrgClk_AdrReg_Set( reg_adr, C_TrgClk_AdcClk_Freq );
				sprintf( reg_name, "%s%", "AdcClk_Freq" );
			}
			else if( reg == 2 )
			{
				reg_adr = D_TrgClk_AdrReg_Set( reg_adr, C_TrgClk_WrClk_Freq );
				sprintf( reg_name, "%s%", "WrClk_Freq" );
			}
			else if( reg == 3 )
			{
				reg_adr = D_TrgClk_AdrReg_Set( reg_adr, C_TrgClk_WrClk_Phase );
				sprintf( reg_name, "%s%", "WrClk_Phase" );
			}
			else if( reg == 4 )
			{
				reg_adr = D_TrgClk_AdrReg_Set( reg_adr, C_TrgClk_AdcClk_Phase );
				sprintf( reg_name, "%s%", "AdcClk_Phase" );
			}
//fprintf( stderr, "%s: reg=%d %s adr=0x%08x\n", __FUNCTION__, reg, reg_name, reg_adr );
			// Read
			if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, 0, DEF_FEU_READ, &rd_val ) ) < 0 )
			{
				fprintf( stderr,  "%s: Beu_ReqResp DEF_FEU_READ failed for feu=%d beu=%d lnk=%d %s reg=0x%06x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_name, reg_adr );
				return D_RetCode_Err_NetIO;
			}
//fprintf( stderr, "%s: reg=%d %s adr=0x%08x rd_val=0x%04x\n", __FUNCTION__, reg, reg_name, reg_adr, rd_val );
			// Set values to be written
			if( reg == 0 )
			{
				half_period = (unsigned int)(Def_TrgClk_Period_ns * params->RdClk_Div / 2.);
				wr_val = D_TrgClk_RegFreq_LowTime_Set(  rd_val, half_period );
				wr_val = D_TrgClk_RegFreq_HighTime_Set( wr_val, half_period );
			}
			else if( reg == 1 )
			{
				half_period = (unsigned int)(Def_TrgClk_Period_ns * params->RdClk_Div / 2.);
				wr_val = D_TrgClk_RegFreq_LowTime_Set(  rd_val, half_period );
				wr_val = D_TrgClk_RegFreq_HighTime_Set( wr_val, half_period );
			}
			else if( reg == 2 )
			{
				half_period = (unsigned int)(Def_TrgClk_Period_ns * params->WrClk_Div / 2.);
				wr_val = D_TrgClk_RegFreq_LowTime_Set(  rd_val, half_period );
				wr_val = D_TrgClk_RegFreq_HighTime_Set( wr_val, half_period );
			}
			else if( reg == 3 )
			{
/*
				phase_delay = (params->WrClk_Phase * 64) / 360;
				wr_val = D_TrgClk_RegPhase_Delay_Set( rd_val, phase_delay );
*/
				wr_val = D_TrgClk_RegPhase_Delay_Set( rd_val, params->WrClk_Phase );
				wr_val = D_TrgClk_RegPhase_Mx_Clr( wr_val );
			}
			else if( reg == 4 )
			{
/*
				phase_delay = (params->AdcClk_Phase * 64) / 360;
//fprintf( stderr, "%s: reg=%d %s AdcClk_Phase=%d phase_delay=%d\n", __FUNCTION__, reg, reg_name, params->AdcClk_Phase, phase_delay );
				wr_val = D_TrgClk_RegPhase_Delay_Set( rd_val, phase_delay );
*/
				wr_val = D_TrgClk_RegPhase_Delay_Set( rd_val, params->AdcClk_Phase );
				wr_val = D_TrgClk_RegPhase_Mx_Clr( wr_val );
			}
			// Write
//fprintf( stderr, "%s: reg=%d %s adr=0x%08x wr_val=0x%04x\n", __FUNCTION__, reg, reg_name, reg_adr, wr_val );
			if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
			{
				fprintf( stderr,  "%s: Beu_ReqResp DEF_FEU_WRITE failed for feu=%d beu=%d lnk=%d %s reg=0x%06x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_name, reg_adr );
				return D_RetCode_Err_NetIO;
			}
			else
			{
				if( (reg == 0) || (reg == 1) || (reg == 2) )
				{
					if( D_TrgClk_RegFreq_LowTime_Get(wr_val) != D_TrgClk_RegFreq_LowTime_Get(rd_val) )
					{
						fprintf( stderr,  "%s: DrmClk reg %s LowTime Set WrRd_Missmatch for feu=%d beu=%d lnk=%d %s reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
							__FUNCTION__, reg_name, feu_id, beu_id, beu_lnk_id, reg_name, reg_adr, wr_val, rd_val );
						return D_RetCode_Err_WrRd_Missmatch;
					}
					if( D_TrgClk_RegFreq_HighTime_Get(wr_val) != D_TrgClk_RegFreq_HighTime_Get(rd_val) )
					{
						fprintf( stderr,  "%s: DrmClk reg %s HighTime Set WrRd_Missmatch for feu=%d beu=%d lnk=%d %s reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
							__FUNCTION__, reg_name, feu_id, beu_id, beu_lnk_id, reg_name, reg_adr, wr_val, rd_val );
						return D_RetCode_Err_WrRd_Missmatch;
					}
				}
				else if( (reg == 3) || (reg == 4) )
				{
					if( D_TrgClk_RegPhase_Delay_Get(wr_val) != D_TrgClk_RegPhase_Delay_Get(rd_val) )
					{
						fprintf( stderr,  "%s: DrmClk reg %s Delay Set WrRd_Missmatch for feu=%d beu=%d lnk=%d %s reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
							__FUNCTION__, reg_name, feu_id, beu_id, beu_lnk_id, reg_name, reg_adr, wr_val, rd_val );
						return D_RetCode_Err_WrRd_Missmatch;
					}
				}
			}
		} // if to Perform operations only for set registers
	} // for( reg=0; reg<5; reg++ )

	/*
	 * Next clear MMCM power bits
	 */
/*
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_DrmClk );
	reg_adr = D_MainAdr_Set( reg_adr, C_TrgClk_PwrBits );
	// Set all Power bits
	wr_val = D_TrgClk_RegPower_Clr( 0 );
	// Write
	sprintf(sc_tx_buf, "poke 0x%x 0x%x", reg_adr, wr_val);
	sc_tx_len = strlen(sc_tx_buf)+1;
	if( (sc_rx_len = UdpSocket_ReqResp(&sc_socket, feu, feu_ip, feu_port, sc_tx_buf, sc_tx_len, sc_rx_buf, 256 ) ) < 0 )
	{
		fprintf( stderr,  "%s: UdpSocket_ReqResp failed for feu_id=%d port=%d for PwrBits Poke\n", __FUNCTION__, feu, feu_port );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		sscanf(sc_rx_buf, "poke 0x%x 0x%x = 0x%x", &reg_adr, &wr_val, &rd_val );
		if( D_TrgClk_RegPower_Get(wr_val) != D_TrgClk_RegPower_Get(rd_val) )
		{
			fprintf( stderr,  "%s: PwrBits Clear WrRd_Missmatch for %s\n", __FUNCTION__, sc_rx_buf );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}
*/
	/*
	 * Last remove Reset from the MMCM
	 */
/*
	// Send DrmClkRst command
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegCommand );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, 0, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp DEF_FEU_READ failed for feu=%d beu=%d lnk=%d DrmClkRst reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}
	// Set the DrmClkRst bit
	wr_val = D_Main_Cmd_DrmClkRst_Clr( rd_val );
	// Write
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp DEF_FEU_WRITE failed for feu=%d beu=%d lnk=%d DrmClkRst reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Config_Get(wr_val) != D_Main_Cmd_Config_Get(rd_val) )
		{
			fprintf( stderr,  "%s: DrmClkRst Clr WrRd_Missmatch for feu=%d beu=%d lnk=%d DrmClkRst reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}
*/
//fprintf( stderr, "%s: Finished\n", __FUNCTION__ );
//getchar();

	// All went fine
	return D_RetCode_Sucsess;
}

/*
 * Get Feu Serial Number
 */
int _FeuGetSn( int feu_id, int beu_id, int beu_lnk_id, int *feu_sn )
{
	unsigned int reg_adr;
	unsigned int wr_val;
	unsigned int rd_val;

	int ret;

	/*
	 *  Try to get FEU ID
	 */
	// Those are RO registers, so no semaphore is needed
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	// 4 LSB-s of SN are in Firmware revision register
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegFwRev );
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d D_MainAdr_RegFwRev reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}

	// Get the 4 LSB-s
	*feu_sn = D_Main_Rev_GetSrN(rd_val);
	// 4 MSB-s of SN are in Software revision register
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegSwRev );
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d D_MainAdr_RegSwRev reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}

	// Complete with 4 MSB-s
	*feu_sn = (D_Main_Rev_GetSrN(rd_val)<<4) + (*feu_sn);
	printf("%s: Feu %d SN %d (beu %d lnk %d)\n", __FUNCTION__, feu_id, *feu_sn, beu_id, beu_lnk_id);

	// All went fine
	return D_RetCode_Sucsess;
}

/*
 * Internal Reset function
 */
int _FeuReset( FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id )
{
	unsigned int reg_adr;
	unsigned int wr_val;
	unsigned int rd_val;

	int ret;

	// Program the Dream clocks
	if( (ret = DrmClkConfig( &(feu_params->dream_clk_params), feu_id, beu_id, beu_lnk_id )) < 0 )
	{
		fprintf( stderr,  "%s: DrmClkConfig failed for feu=%d beu=%d lnk=%d with %d\n", __FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		return ret;
	}

	// Make sure the FEU is not in running state
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegCommand );
	wr_val = D_Main_Cmd_Run_Clr( 0 );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Reset_Get(wr_val) != D_Main_Cmd_Reset_Get(rd_val) )
		{
			fprintf( stderr,  "%s: Main Command (Reset) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}
	// Make sure the FEU is not in Idle state
	// Set address 
	wr_val = D_Main_Cmd_Config_Clr( 0 );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Reset_Get(wr_val) != D_Main_Cmd_Reset_Get(rd_val) )
		{
			fprintf( stderr,  "%s: Main Command (Reset) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}

	// Reset FEU
	// Send reset command only once when the function is called first time with default parameters 
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegCommand );
	wr_val = D_Main_Cmd_Reset_Clr( 0 );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Reset_Get(wr_val) != D_Main_Cmd_Reset_Get(rd_val) )
		{
			fprintf( stderr,  "%s: Main Command (Reset) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}
	// Set reset command
	wr_val = D_Main_Cmd_Reset_Set( wr_val );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Reset_Get(wr_val) != D_Main_Cmd_Reset_Get(rd_val) )
		{
			fprintf( stderr,  "%s: Main Command (Reset) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}
	// Clear reset command
	wr_val = D_Main_Cmd_Reset_Clr( wr_val );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Reset_Get(wr_val) != D_Main_Cmd_Reset_Get(rd_val) )
		{
			fprintf( stderr,  "%s: Main Command (Reset) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}

	return D_RetCode_Sucsess;
}

/*
 * Reset function
 */
int FeuReset( FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id )
{
	int ret;

	int feu_sn;

	/*
	 * Check input parameters
	 */
	if( feu_params == (FeuParams *)NULL )
	{
		fprintf( stderr, "%s: feu_params=NULL\n", __FUNCTION__ ); 
		return D_RetCode_Err_Null_Pointer;
	}
	if( (feu_id < 1) || (DEF_MAX_NB_OF_FEU<=feu_id) )
	{
		fprintf( stderr, "%s: feu_index=%d<1 or >=%d\n", __FUNCTION__, feu_id, DEF_MAX_NB_OF_FEU ); 
		return D_RetCode_Err_Wrong_Param;
	}
	if( (beu_id < 1) || (DEF_MAX_NB_OF_BEU<=beu_id) )
	{
		fprintf( stderr, "%s: feu_id=%d<0 or >=%d\n", __FUNCTION__, beu_id, DEF_MAX_NB_OF_BEU ); 
		return D_RetCode_Err_Wrong_Param;
	}
	if( (beu_lnk_id < 0) || (DEF_MAX_NB_OF_FEU_PER_BEU<=beu_lnk_id) )
	{
		fprintf( stderr, "%s: feu_id=%d<0 or >=%d\n", __FUNCTION__, beu_lnk_id, DEF_MAX_NB_OF_FEU_PER_BEU ); 
		return D_RetCode_Err_Wrong_Param;
	}

	// First Fix the Peek/Poke global variables
	PeekPoke_Feu = feu_id;
	PeekPoke_Beu = beu_id;
	PeekPoke_Lnk = beu_lnk_id;

	// Make sure the Link is active
	if( (ret = Beu_CheckFeuLink(feu_id, beu_id, beu_lnk_id)) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ChekFeuLink failed with %d for %d %d %d\n",
			__FUNCTION__, ret, feu_id, beu_id, beu_lnk_id );
		return ret;
	} 

	/*
	 *  Try to get FEU ID
	 */
	if( (ret = _FeuGetSn( feu_id, beu_id, beu_lnk_id, &feu_sn )) != D_RetCode_Sucsess )
	{
		fprintf( stderr,  "%s: _FeuGetSn failed for feu=%d beu=%d lnk=%d with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		return ret;
	}
	if( feu_sn != feu_params->Feu_RunCtrl_Id )
	{
		fprintf( stderr,  "%s: Unexpected Feu SN=%d for feu=%d beu=%d lnk=%d; SN=%d expected\n",
			__FUNCTION__, feu_sn, feu_id, beu_id, beu_lnk_id, feu_params->Feu_RunCtrl_Id );
		return D_RetCode_Err_Wrong_Param;
	}

	// Get exclussive access to the FEU
	if( (ret = FeuConfig_Lock()) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: Feu_Lock fail for feu=%d sn=%d beu=%d lnk=%d with ret %d\n", __FUNCTION__, feu_id, feu_sn, beu_id, beu_lnk_id, ret ); 
		return ret;
	}

	// Reset FEU
	if( (ret = _FeuReset( feu_params, feu_id, beu_id, beu_lnk_id )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: _FeuReset fail for feu=%d sn=%d beu=%d lnk=%d with ret %d\n", __FUNCTION__, feu_id, feu_sn, beu_id, beu_lnk_id, ret ); 
		return ret;
	}

	// Note: After reset the FEU is unlocked by firmware: no need to unlock in software
	// But the software state of the semaphore needs to be changed
	if( (ret = Sem_ClearState()) != D_RetCode_Sucsess )
	{
		fprintf(stderr, "%s: Sem_ClearState failed with %s\n", __FUNCTION__, sem_msg);
		return ret;
	}

	return D_RetCode_Sucsess;
}

/*
 * Internal Configuration function
 */
int _FeuConfig( FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id )
{
	unsigned int reg_adr;
	unsigned int wr_val;
	unsigned int rd_val;

	int ret;

	int dream_pair;
	int dream_power;

	int reg;
	int dream_min;
	int dream_max;
	int dream;

	// Main module config parameters
	if
	(
		( feu_params->Main_Conf_DreamMask >= 0 )
		||
		( feu_params->Main_Conf_Samples >= 0 )
		||
		( feu_params->Main_Conf_DataPipeLen >= 0 )
	)
	{
		// Main module config parameters
		// Only recovered clock is supported, this is even nonsence to setup 
		wr_val = D_Main_Conf_Set( 0, feu_params->Main_Conf_Samples, (feu_params->Main_Conf_DreamMask&0xFF), feu_params->Main_Conf_DataPipeLen, 0, RecClk );
		// Set address address 
		reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
		reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegConfig );
		// Write
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
			return D_RetCode_Err_NetIO;
		}
		else
		{
			if( wr_val != rd_val )
			{
				fprintf( stderr,  "%s: Main config WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		}
//printf("%s: Feu %d Main configure done (beu %d lnk %d)\n", __FUNCTION__, feu_id, beu_id, beu_lnk_id);
	}

	// Main module trigger register
	if
	(
		( feu_params->Main_Trig_TimeStamp >= 0 )
		||
		( feu_params->Main_Trig_OvrWrnLwm >= 0 )
		||
		( feu_params->Main_Trig_OvrWrnHwm >= 0 )
		||
		( feu_params->Main_Trig_OvrThersh >= 0 )
		||
		( feu_params->Main_Trig_LocThrot >= 0 )
	)
	{
		// Main module trigger parameters
		wr_val = D_Main_Trig_Set( 0, feu_params->Main_Trig_TimeStamp, feu_params->Main_Trig_OvrWrnLwm, feu_params->Main_Trig_OvrWrnHwm, feu_params->Main_Trig_OvrThersh, feu_params->Main_Trig_LocThrot );
		// Set address address 
		reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
		reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegTrigConfig );
		// Write
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
			return D_RetCode_Err_NetIO;
		}
		else
		{
			if( wr_val != rd_val )
			{
				fprintf( stderr,  "%s: Main trigger WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		}
//printf("%s: Feu %d Main trigger done (beu %d lnk %d)\n", __FUNCTION__, feu_id, beu_id, beu_lnk_id);
	}

/*
	// FEU PowerUp Register
	if
	(
		( feu_params->Feu_Pwr_Dream >= 0 )
		||
		( feu_params->Feu_Pwr_PrtFlt >= 0 )
	)
	{
		// Set address address
		reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Feu );
		reg_adr = D_MainAdr_Set( reg_adr, C_Feu_AdrReg_PwrUp );
		wr_val = D_Feu_RegPwr_PrtFlt_Set( 0, feu_params->Feu_Pwr_PrtFlt );
		if( feu_params->Feu_Pwr_PrtFlt == 0 )
		{
			// Write
			if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
			{
				fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
				return D_RetCode_Err_NetIO;
			}
			else
			{
				if( D_Feu_RegPwr_Get(wr_val) != D_Feu_RegPwr_Get(rd_val) )
				{
					fprintf( stderr,  "%s: FEU PowerUp WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
					return D_RetCode_Err_WrRd_Missmatch;
				}
			}
		}
		// Power dreams
		dream_power = 0;
		for( dream_pair=0; dream_pair<4; dream_pair++ )
		{
			if( (feu_params->Feu_Pwr_Dream) & (1 << dream_pair) )
			{
				dream_power += (1 << dream_pair);
				wr_val = D_Feu_RegPwr_Dream_Set( wr_val, dream_power );
				// Write
				if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
				{
					fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
					return D_RetCode_Err_NetIO;
				}
				else
				{
					if( D_Feu_RegPwr_Get(wr_val) != D_Feu_RegPwr_Get(rd_val) )
					{
						fprintf( stderr,  "%s: FEU PowerUp WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
						return D_RetCode_Err_WrRd_Missmatch;
					}
				}
				usleep(50000);
			} // if( feu_params->Feu_Pwr_Dream & (1 << dream_pair) )
		} // for( dream_pair=0; dream_pair<4; dream_pair++ )
//printf("%s: Feu %d power 0x%08x done (beu %d lnk %d)\n", __FUNCTION__, feu_id, rd_val, beu_id, beu_lnk_id);
	}
*/

//printf("%s: Feu %d power flt 0x%04x drm =0x%01x (beu %d lnk %d)\n", __FUNCTION__, feu_id, feu_params->Feu_Pwr_PrtFlt, feu_params->Feu_Pwr_Dream, beu_id, beu_lnk_id);
	// FEU PowerUp Register
	if
	(
		( feu_params->Feu_Pwr_Dream >= 0 )
		||
		( feu_params->Feu_Pwr_PrtFlt >= 0 )
	)
	{
		// Set address address
		reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Feu );
		reg_adr = D_MainAdr_Set( reg_adr, C_Feu_AdrReg_PwrUp );

		// First read current content
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp DEF_FEU_READ failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
			return D_RetCode_Err_NetIO;
		}
//printf("%s: Feu %d power rd_val 0x%08x (beu %d lnk %d)\n", __FUNCTION__, feu_id, rd_val, beu_id, beu_lnk_id);
		wr_val = D_Feu_RegPwr_PrtFlt_Set( rd_val, feu_params->Feu_Pwr_PrtFlt );
//printf("%s: Feu %d power wr_val 0x%08x (beu %d lnk %d)\n", __FUNCTION__, feu_id, wr_val, beu_id, beu_lnk_id);
/*
		if( feu_params->Feu_Pwr_PrtFlt == 0 )
		{
			// Write
			if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
			{
				fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
				return D_RetCode_Err_NetIO;
			}
			else
			{
				if( D_Feu_RegPwr_Get(wr_val) != D_Feu_RegPwr_Get(rd_val) )
				{
					fprintf( stderr,  "%s: FEU PowerUp WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
					return D_RetCode_Err_WrRd_Missmatch;
				}
			}
		}
*/
		// Power dreams
		for( dream_pair=0; dream_pair<4; dream_pair++ )
		{
			if( ((feu_params->Feu_Pwr_Dream) & (1 << dream_pair)) != (D_Feu_RegPwr_Get(wr_val) & (1 << dream_pair)) )
			{
				dream_power = D_Feu_RegPwr_Dream_Get( wr_val);
				if( feu_params->Feu_Pwr_Dream & (1 << dream_pair) )
					dream_power |= (1 << dream_pair);
				else
					dream_power &= ((~(1 << dream_pair))&0xF);
				wr_val = D_Feu_RegPwr_Dream_Set( wr_val, dream_power );
/*
printf("%s: Feu %d dream_pair=%d shift=0x%1x inf=0x%1x mask=0x%1x dream_power 0x%08x (beu %d lnk %d)\n",
	__FUNCTION__, feu_id, dream_pair, (1 << dream_pair), ~(1 << dream_pair), dream_power, beu_id, beu_lnk_id);
printf("%s: Feu %d dream_pair=%d power wr_val 0x%08x (beu %d lnk %d)\n",
	__FUNCTION__, feu_id, dream_pair, wr_val, beu_id, beu_lnk_id);
*/
				// Write
				if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
				{
					fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
					return D_RetCode_Err_NetIO;
				}
				else
				{
					if( D_Feu_RegPwr_Get(wr_val) != D_Feu_RegPwr_Get(rd_val) )
					{
						fprintf( stderr,  "%s: FEU PowerUp WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
							__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
						return D_RetCode_Err_WrRd_Missmatch;
					}
				}
				usleep(500000);
			} // if( feu_params->Feu_Pwr_Dream & (1 << dream_pair) )
		} // for( dream_pair=0; dream_pair<4; dream_pair++ )
//printf("%s: Feu %d power 0x%08x done (beu %d lnk %d)\n", __FUNCTION__, feu_id, rd_val, beu_id, beu_lnk_id);
	}

	// FEU Run Control parameters
	if
	(
		( feu_params->Feu_RunCtrl_Pd >= 0 )
		||
		( feu_params->Feu_RunCtrl_CM >= 0 )
		||
		( feu_params->Feu_RunCtrl_ZS >= 0 )
		||
		( feu_params->Feu_RunCtrl_DrOvr >= 0 )
		||
		( feu_params->Feu_RunCtrl_DrDblSmpClk >= 0 )
		||
		( feu_params->Feu_RunCtrl_RdDel >= 0 )
		||
		( feu_params->Feu_RunCtrl_EvTstExt >= 0 )
		||
		( feu_params->Feu_RunCtrl_ZsChkSmp >= 0 )
		||
		( feu_params->Feu_RunCtrl_CmOffset >= 0 )
		||
		( feu_params->Feu_RunCtrl_AdcDatRdyDel >= 0 )
	)
	{
		// FEU Run Control parameters
		wr_val = D_Feu_RunCtrl_Set( 0, feu_params->Feu_RunCtrl_Pd, feu_params->Feu_RunCtrl_CM, feu_params->Feu_RunCtrl_ZS, feu_params->Feu_RunCtrl_ZsChkSmp, feu_id, feu_params->Feu_RunCtrl_AdcDatRdyDel, feu_params->Feu_RunCtrl_DrDblSmpClk, feu_params->Feu_RunCtrl_DrOvr, feu_params->Feu_RunCtrl_EvTstExt, feu_params->Feu_RunCtrl_RdDel, feu_params->Feu_RunCtrl_CmOffset );
		// Set address address 
		reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Feu );
		reg_adr = D_Feu_AdrReg_Set( reg_adr, C_Feu_AdrReg_RunCtrl );
		// Write
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
			return D_RetCode_Err_NetIO;
		}
		else
		{
			if( wr_val != rd_val )
			{
				fprintf( stderr,  "%s: FEU Run Control WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		}
//printf("%s: Feu %d run ctrl 0x%08x (beu %d lnk %d) extracted id=%d\n", __FUNCTION__, feu_id, rd_val, beu_id, beu_lnk_id, D_Feu_RegRC_Id_Get(wr_val));
	}

	// FEU Pedestal memory
	if( (feu_params->Feu_RunCtrl_PdFile[0]!='\0') && (strcmp(feu_params->Feu_RunCtrl_PdFile,"None")!= 0) )
	{
		if( (ret = FeuMemConfig(feu_params->Feu_RunCtrl_PdFile, D_CBus_Mod_Pedestal, feu_id, beu_id, beu_lnk_id )) < 0 )
		{
			fprintf( stderr,  "%s: FeuMemConfig failed for feu=%d beu=%d lnk=%d pedestal file=0xs with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, feu_params->Feu_RunCtrl_PdFile, ret );
			return D_RetCode_Err_NetIO;
		}
//printf("%s: Feu %d Feu Ped Mem configured (beu %d lnk %d) with %s\n", __FUNCTION__, feu_id, beu_id, beu_lnk_id, feu_params->Feu_RunCtrl_PdFile);
	}
	else if( feu_id != 0 )
	{
		if( (ret = FeuMemInit(D_CBus_Mod_Pedestal, feu_id, beu_id, beu_lnk_id )) < 0 )
		{
			fprintf( stderr,  "%s: FeuMemInit failed for feu=%d beu=%d lnk=%d pedestals with ret=%d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
			return D_RetCode_Err_NetIO;
		}
//printf("%s: Feu %d Feu Ped Mem initialized (beu %d lnk %d)\n", __FUNCTION__, feu_id, beu_id, beu_lnk_id);
	}
	// FEU Threshold memory
	if( (feu_params->Feu_RunCtrl_ZsFile[0]!='\0') && (strcmp(feu_params->Feu_RunCtrl_ZsFile,"None")!= 0) )
	{
		if( (ret = FeuMemConfig(feu_params->Feu_RunCtrl_ZsFile, D_CBus_Mod_Threshold, feu_id, beu_id, beu_lnk_id )) < 0 )
		{
			fprintf( stderr,  "%s: FeuMemConfig failed for feu=%d beu=%d lnk=%d thershold file=0xs with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, feu_params->Feu_RunCtrl_ZsFile, ret );
			return D_RetCode_Err_NetIO;
		}
//printf("%s: Feu %d Feu Thresh Mem configured (beu %d lnk %d) with %s\n", __FUNCTION__, feu_id, beu_id, beu_lnk_id, feu_params->Feu_RunCtrl_ZsFile);
	}
	else if( feu_id != 0 )
	{
		if( (ret = FeuMemInit(D_CBus_Mod_Threshold, feu_id, beu_id, beu_lnk_id )) < 0 )
		{
			fprintf( stderr,  "%s: FeuMemInit failed for feu=%d beu=%d lnk=%d thresholds with ret=%d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
			return D_RetCode_Err_NetIO;
		}
//printf("%s: Feu %d Feu Thresh Mem initialized (beu %d lnk %d)\n", __FUNCTION__, feu_id, beu_id, beu_lnk_id);
	}

	// FEU Pulser Register
	if
	(
		( feu_params->Feu_Pulser_DreamTst >= 0 )
		||
		( feu_params->Feu_Pulser_PulseWid >= 0 )
	)
	{
		// FEU Pulser parameters
		wr_val = feu_params->Feu_Pulser_DreamTst + (feu_params->Feu_Pulser_PulseWid << 8);
		// Enable Pulser
		if( feu_params->Feu_Pulser_Enable >= 0 )
			wr_val += (feu_params->Feu_Pulser_Enable << 24);
		// Set address 
		reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Feu );
		reg_adr = D_Feu_AdrReg_Set( reg_adr, C_Feu_AdrReg_Pulser );
		// Write
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
			return D_RetCode_Err_NetIO;
		}
		else
		{
			if( wr_val != rd_val )
			{
				fprintf( stderr,  "%s: FEU Pulser WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		}
//printf("%s: Feu %d pulser 0x%08x (beu %d lnk %d)\n", __FUNCTION__, feu_id, rd_val, beu_id, beu_lnk_id);
	}

	// FEU Prescale Register
	if
	(
		(feu_params->Feu_PreScale_EvtData > 0)
		||
		(feu_params->Feu_InterPacket_Delay > 0)
	)
	{
		// FEU Pulser parameters
		wr_val = D_Feu_RegPreScale_EvtData_Put( 0, feu_params->Feu_PreScale_EvtData );
		// Delay is given in ns; The Inter Packet delay is in ticks of 8 ns
		wr_val = D_Feu_RegPreScale_Ipd_Put( wr_val, (int)(((double)feu_params->Feu_InterPacket_Delay) / 8.) );
		// Set address address 
		reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Feu );
		reg_adr = D_Feu_AdrReg_Set( reg_adr, C_Feu_AdrReg_PreScale );
		// Write
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
			return D_RetCode_Err_NetIO;
		}
		else
		{
			if( wr_val != rd_val )
			{
				fprintf( stderr,  "%s: FEU Prescale WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		}
//printf("%s: Feu %d prescale (beu %d lnk %d)\n", __FUNCTION__, feu_id, beu_id, beu_lnk_id);
	}

	// Trigger generator parameters
	if( feu_params->Trig_Conf_TrigPipeLen >= 0 )
	{
		// Trigger module config parameters
		// Only optical link trigger is supported 
		wr_val = D_Tg_RegConf_Set( 0, 0, feu_params->Trig_Conf_TrigPipeLen, Tg_Src_Int, 0 );
		// Set configuration register address 
		reg_adr = D_CBus_SetModType(      0, D_CBus_Mod_TrigGen );
		reg_adr = D_Tg_AdrZone_Set( reg_adr, D_Tg_AdrZone_Reg );
		reg_adr = D_Tg_AdrReg_Set(  reg_adr, D_Tg_RegAdr_Conf );
		// Write
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
			return D_RetCode_Err_NetIO;
		}
		else
		{
			if( D_Tg_RegConf_GetConfig(wr_val) != D_Tg_RegConf_GetConfig(rd_val) )
			{
				fprintf( stderr,  "%s: Trigger generator WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		}
//printf("%s: Feu %d trig gen (beu %d lnk %d)\n", __FUNCTION__, feu_id, beu_id, beu_lnk_id);
	}
	// No need to configure Trigger generator memory

	// Auxiliary Trigger Interface parameters
	{
		// Ignore Auxiliary Trigger Interface
		wr_val = D_TrigInt_Csr_Conf_Set( 0, 0, 1, 0, 0  );
		// Set configuration register address 
		reg_adr = D_CBus_SetModType(          0, D_CBus_Mod_TrigInt );
		reg_adr = D_TrigInt_AdrReg_Set( reg_adr, C_TrigInt_AdrReg_Csr );
		// Write
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
			return D_RetCode_Err_NetIO;
		}
		else
		{
			if( D_TrigInt_Csr_Conf_Get(wr_val) != D_TrigInt_Csr_Conf_Get(rd_val) )
			{
				fprintf( stderr,  "%s: Aux Trigger Interface WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		}
//printf("%s: Feu %d trig aux (beu %d lnk %d)\n", __FUNCTION__, feu_id, beu_id, beu_lnk_id);
	}

	// Self Trigger Interface parameters
	if
	(
		(feu_params->SelfTrig_DreamMask >= 0)
		||
		(feu_params->SelfTrig_Mult >= 0)
		||
		(feu_params->SelfTrig_CmbHitPropOl >= 0)
		||
		(feu_params->SelfTrig_DrmHitWid >= 0)
		||
		(feu_params->SelfTrig_CmbHitWid >= 0)
		||
		(feu_params->SelfTrig_TrigTopo >= 0)
	)
	// Self Trigger parameters
	{
		// Set configuration register address 
		wr_val = D_SelfTrig_Csr_Conf_Set( 0, feu_params->SelfTrig_DreamMask, feu_params->SelfTrig_Mult, 0, feu_params->SelfTrig_DrmHitWid, feu_params->SelfTrig_CmbHitWid, feu_params->SelfTrig_TrigTopo, feu_params->SelfTrig_CmbHitPropOl );
		reg_adr = D_CBus_SetModType(            0, D_CBus_Mod_SelfTrig );
		reg_adr = D_SelfTrig_AdrZone_Set( reg_adr, D_SelfTrig_AdrZone_Reg );
		reg_adr = D_SelfTrig_AdrReg_Set(  reg_adr, C_SelfTrig_AdrReg_Csr );
		// Write
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
			return D_RetCode_Err_NetIO;
		}
		else
		{
			if( D_SelfTrig_Csr_Conf_Get(wr_val) != D_SelfTrig_Csr_Conf_Get(rd_val) )
			{
				fprintf( stderr,  "%s: Self Trigger Csr WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, rd_val );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		}
//printf("%s: Feu %d trig slf (beu %d lnk %d)\n", __FUNCTION__, feu_id, beu_id, beu_lnk_id);
	}
	// Self Trigger Interface parameters
	if( feu_params->SelfTrig_Veto >= 0 )
	{
		// Set configuration register address 
		wr_val  = D_SelfTrig_RegVeto_Veto_Put(     0, feu_params->SelfTrig_Veto );
		reg_adr = D_CBus_SetModType(               0, D_CBus_Mod_SelfTrig );
		reg_adr = D_SelfTrig_AdrZone_Set( reg_adr, D_SelfTrig_AdrZone_Reg );
		reg_adr = D_SelfTrig_AdrReg_Set(  reg_adr, C_SelfTrig_AdrReg_Veto );
		// Write
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
			return D_RetCode_Err_NetIO;
		}
		else
		{
			if( D_SelfTrig_RegVeto_Veto_Get(wr_val) != D_SelfTrig_RegVeto_Veto_Get(rd_val) )
			{
				fprintf( stderr,  "%s: Self Trigger Veto WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, rd_val );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		}
//printf("%s: Feu %d trig slf (beu %d lnk %d)\n", __FUNCTION__, feu_id, beu_id, beu_lnk_id);
	}
	// Self Trigger topology parameters : Not yet implemented

	// FEU Communication parameters
	// Optical link
	if( feu_params->ComChan_Enable >= 0 )
	{
		// Set address address 
		reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_ComChan );
		reg_adr = D_ComChan_AdrReg_Set( reg_adr, C_ComChan_AdrReg_Csr );

		// Communication Channel parameter
		wr_val = D_ComChan_Csr_Enable_Put( 0, feu_params->ComChan_Enable );
		// Write
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
		{
			fprintf( stderr,  "%s: ComChan Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: ComChan Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
			return D_RetCode_Err_NetIO;
		}
		else
		{
			if( D_ComChan_Csr_Enable_Get(wr_val) != D_ComChan_Csr_Enable_Get(rd_val) )
			{
				fprintf( stderr,  "%s: ComChan WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, rd_val );
				if( sys_log_fptr != (FILE *)NULL )
					fprintf( sys_log_fptr,  "%s: ComChan WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, rd_val );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		}
	}

	// FEU Communication parameters
	// Not clear what to do with UDP/IP link
	{
		// Disable it for a while
		// Set address 
		// Set configuration register address 
		reg_adr = D_CBus_SetModType(          0, D_CBus_Mod_UdpChan );
		reg_adr = D_UdpChan_AdrReg_Set( reg_adr, C_UdpChan_AdrReg_Csr );

		// Disable everything
		wr_val = D_UdpChan_Csr_Enable_Put(0, 0);
		wr_val = D_UdpChan_Csr_IntEnable_Put(wr_val, 0);
		wr_val = D_UdpChan_Csr_MultiPackEnb_Put(wr_val, 0);
		wr_val = D_UdpChan_Csr_MultiPackThr_Put(wr_val, 0);
		// Write
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
		{
			fprintf( stderr,  "%s: UdpChan Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: UdpChan Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
			return D_RetCode_Err_NetIO;
		}
		else
		{
			if
			(
				( D_UdpChan_Csr_Enable_Get(wr_val)       != D_UdpChan_Csr_Enable_Get(rd_val)       )
				||
				( D_UdpChan_Csr_IntEnable_Get(wr_val)    != D_UdpChan_Csr_IntEnable_Get(rd_val)    )
				||
				( D_UdpChan_Csr_MultiPackEnb_Get(wr_val) != D_UdpChan_Csr_MultiPackEnb_Get(rd_val) )
				||
				( D_UdpChan_Csr_MultiPackThr_Get(wr_val) != D_UdpChan_Csr_MultiPackThr_Get(rd_val) )
			)
			{
				fprintf( stderr,  "%s: UdpChan WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, rd_val );
				if( sys_log_fptr != (FILE *)NULL )
					fprintf( sys_log_fptr,  "%s: UdpChan WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, rd_val );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		}
		// Confirm that UDP data channel has been disabled
//		if( sys_log_fptr != (FILE *)NULL )
//			fprintf( sys_log_fptr,  "%s: UdpChan disabled for feu=%d beu=%d lnk=%d\n",
//				__FUNCTION__, feu_id, beu_id, beu_lnk_id );
	}

	/*
	 * EeProm configuration
	 */
	if( (ret=EePromConfig( feu_params->ee_prom, feu_id, beu_id, beu_lnk_id )) != D_RetCode_Sucsess )
	{
		fprintf( stderr,  "%s: EePromConfig failed for feu=%d beu=%d lnk=%d with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		return D_RetCode_Err_NetIO;
	}

	/*
	 * Adc configuration
	 */
	if( (ret=AdcConfig( &(feu_params->adc_params), feu_id, beu_id, beu_lnk_id )) != D_RetCode_Sucsess )
	{
		fprintf( stderr,  "%s: AdcConfig failed for feu=%d beu=%d lnk=%d with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		return D_RetCode_Err_NetIO;
	}

	/*
	 * Max16031 configuration
	 */
	if( (ret=Max16031Config( feu_params->max16031, feu_id, beu_id, beu_lnk_id )) != D_RetCode_Sucsess )
	{
		fprintf( stderr,  "%s: Max16031Config failed for feu=%d beu=%d lnk=%d with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		return D_RetCode_Err_NetIO;
	}

	/*
	 * Dream configuration
	 */
	if( (ret=DreamSpiConfig( feu_params->dream_params, feu_id, beu_id, beu_lnk_id, feu_params->Main_Conf_DreamMask )) != D_RetCode_Sucsess )
	{
		fprintf( stderr,  "%s: DreamSpiConfig failed for feu=%d beu=%d lnk=%d with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		fprintf( stderr,  "%s: Attempt with DreamConfig\n" );
		if( sys_log_fptr != (FILE *)NULL )
    {
		  fprintf( sys_log_fptr,  "%s: DreamSpiConfig failed for feu=%d beu=%d lnk=%d with %d\n",
			  __FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		  fprintf( sys_log_fptr,  "%s: Attempt with DreamConfig\n" );
    }
		if( (ret=DreamConfig( feu_params->dream_params, feu_id, beu_id, beu_lnk_id, feu_params->Main_Conf_DreamMask )) != D_RetCode_Sucsess )
		{
			fprintf( stderr,  "%s: DreamConfig failed for feu=%d beu=%d lnk=%d with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
			if( sys_log_fptr != (FILE *)NULL )
			  fprintf( sys_log_fptr,  "%s: DreamConfig failed for feu=%d beu=%d lnk=%d with %d\n",
				  __FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
			return D_RetCode_Err_NetIO;
		}
	}

	/*
	 * Check Dream configuration
	 */
	if( (ret=DreamSpiConfigCheck( feu_params->dream_params, feu_id, beu_id, beu_lnk_id, feu_params->Main_Conf_DreamMask )) != D_RetCode_Sucsess )
	{
    fprintf( stderr,  "%s: DreamConfigCheck failed for feu=%d beu=%d lnk=%d with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		if( sys_log_fptr != (FILE *)NULL )
		  fprintf( sys_log_fptr,  "%s: DreamConfigCheck failed for feu=%d beu=%d lnk=%d with %d\n",
			  __FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
//		getchar();
		if( (ret=DreamConfig( feu_params->dream_params, feu_id, beu_id, beu_lnk_id, feu_params->Main_Conf_DreamMask )) != D_RetCode_Sucsess )
		{
			fprintf( stderr,  "%s: DreamConfig failed for feu=%d beu=%d lnk=%d with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
			if( sys_log_fptr != (FILE *)NULL )
			  fprintf( sys_log_fptr,  "%s: DreamConfig failed for feu=%d beu=%d lnk=%d with %d\n",
				  __FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
			return D_RetCode_Err_NetIO;
		}
		if( (ret=DreamConfigCheck( feu_params->dream_params, feu_id, beu_id, beu_lnk_id, feu_params->Main_Conf_DreamMask )) != D_RetCode_Sucsess )
		{
      fprintf( stderr,  "%s: DreamConfigCheck failed for feu=%d beu=%d lnk=%d with %d\n",
			  __FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		  if( sys_log_fptr != (FILE *)NULL )
		    fprintf( sys_log_fptr,  "%s: DreamConfigCheck failed for feu=%d beu=%d lnk=%d with %d\n",
			    __FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		}
	}


	// Send config command
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegCommand );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	wr_val = D_Main_Cmd_Config_Set( rd_val );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Config_Get(wr_val) != D_Main_Cmd_Config_Get(rd_val) )
		{
			fprintf( stderr,  "%s: Main Command (Config) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}
printf("%s: Feu %d Config done (beu %d lnk %d)\n", __FUNCTION__, feu_id, beu_id, beu_lnk_id);

	return D_RetCode_Sucsess;
}

/*
 * Configuration function
 */
int FeuConfig( FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id )
{
	int ret;

	int feu_sn;

	/*
	 * Check input parameters
	 */
	if( feu_params == (FeuParams *)NULL )
	{
		fprintf( stderr, "%s: feu_params=NULL\n", __FUNCTION__ ); 
		return D_RetCode_Err_Null_Pointer;
	}
	if( (feu_id < 1) || (DEF_MAX_NB_OF_FEU<=feu_id) )
	{
		fprintf( stderr, "%s: feu_index=%d<1 or >=%d\n", __FUNCTION__, feu_id, DEF_MAX_NB_OF_FEU ); 
		return D_RetCode_Err_Wrong_Param;
	}
	if( (beu_id < 1) || (DEF_MAX_NB_OF_BEU<=beu_id) )
	{
		fprintf( stderr, "%s: feu_id=%d<0 or >=%d\n", __FUNCTION__, beu_id, DEF_MAX_NB_OF_BEU ); 
		return D_RetCode_Err_Wrong_Param;
	}
	if( (beu_lnk_id < 0) || (DEF_MAX_NB_OF_FEU_PER_BEU<=beu_lnk_id) )
	{
		fprintf( stderr, "%s: feu_id=%d<0 or >=%d\n", __FUNCTION__, beu_lnk_id, DEF_MAX_NB_OF_FEU_PER_BEU ); 
		return D_RetCode_Err_Wrong_Param;
	}

	// First Fix the Peek/Poke global variables
	PeekPoke_Feu = feu_id;
	PeekPoke_Beu = beu_id;
	PeekPoke_Lnk = beu_lnk_id;

	// Make sure the Link is active
	if( (ret = Beu_CheckFeuLink(feu_id, beu_id, beu_lnk_id)) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ChekFeuLink failed with %d for %d %d %d\n",
			__FUNCTION__, ret, feu_id, beu_id, beu_lnk_id );
		return ret;
	} 

	/*
	 *  Try to get FEU ID
	 */
	if( (ret = _FeuGetSn( feu_id, beu_id, beu_lnk_id, &feu_sn )) != D_RetCode_Sucsess )
	{
		fprintf( stderr,  "%s: _FeuGetSn failed for feu=%d beu=%d lnk=%d with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		return D_RetCode_Err_NetIO;
	}
	if( feu_sn != feu_params->Feu_RunCtrl_Id )
	{
		fprintf( stderr,  "%s: Unexpected Feu SN=%d for feu=%d beu=%d lnk=%d; SN=%d expected\n",
			__FUNCTION__, feu_sn, feu_id, beu_id, beu_lnk_id, feu_params->Feu_RunCtrl_Id );
		return D_RetCode_Err_Wrong_Param;
	}

	// Get exclussive access to the FEU
	if( (ret = FeuConfig_Lock()) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: Feu_Lock fail for feu=%d sn=%d beu=%d lnk=%d with ret %d\n", __FUNCTION__, feu_id, feu_sn, beu_id, beu_lnk_id, ret ); 
		return ret;
	}

	// Configure FEU
	if( (ret = _FeuConfig( feu_params, feu_id, beu_id, beu_lnk_id )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: _FeuConfig fail for feu=%d sn=%d beu=%d lnk=%d with ret %d\n", __FUNCTION__, feu_id, feu_sn, beu_id, beu_lnk_id, ret ); 
		// Attempt to unlock FEU
		FeuConfig_UnLock();
		return ret;
	}

	// Free FEU
	if( (ret = FeuConfig_UnLock()) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: Feu_Lock fail for feu=%d sn=%d beu=%d lnk=%d with ret %d\n", __FUNCTION__, feu_id, feu_sn, beu_id, beu_lnk_id, ret ); 
		return ret;
	}

	return D_RetCode_Sucsess;
}

/*
 * Enable or disable trigger processing
 */
int FeuRun( FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id, int run )
{
	unsigned int reg_adr;
	unsigned int wr_val;
	unsigned int rd_val;

	int ret;

	int dream_pair;
	int dream_power;

	int reg;
	int dream_min;
	int dream_max;
	int dream;

	int feu_sn;

	if( feu_params == (FeuParams *)NULL )
	{
		fprintf( stderr, "%s: feu_params=NULL\n", __FUNCTION__ ); 
		return D_RetCode_Err_Null_Pointer;
	}
	if( (feu_id < 1) || (DEF_MAX_NB_OF_FEU<=feu_id) )
	{
		fprintf( stderr, "%s: feu_index=%d<1 or >=%d\n", __FUNCTION__, feu_id, DEF_MAX_NB_OF_FEU ); 
		return D_RetCode_Err_Wrong_Param;
	}
	if( (beu_id < 1) || (DEF_MAX_NB_OF_BEU<=beu_id) )
	{
		fprintf( stderr, "%s: feu_id=%d<0 or >=%d\n", __FUNCTION__, beu_id, DEF_MAX_NB_OF_BEU ); 
		return D_RetCode_Err_Wrong_Param;
	}
	if( (beu_lnk_id < 0) || (DEF_MAX_NB_OF_FEU_PER_BEU<=beu_lnk_id) )
	{
		fprintf( stderr, "%s: feu_id=%d<0 or >=%d\n", __FUNCTION__, beu_lnk_id, DEF_MAX_NB_OF_FEU_PER_BEU ); 
		return D_RetCode_Err_Wrong_Param;
	}

	// First Fix the Peek/Poke global variables
	PeekPoke_Feu = feu_id;
	PeekPoke_Beu = beu_id;
	PeekPoke_Lnk = beu_lnk_id;

	/*
	 *  Try to get FEU ID
	 */
	// Those are RO registers, so no semaphore is needed
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	// 4 LSB-s of SN are in Firmware revision register
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegFwRev );
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}
//printf("Press CR to continue <-\n");
//getchar();
	// Get the 4 LSB-s
//	printf("%s: rd_val=0x%08x\n", __FUNCTION__, rd_val);
	feu_sn = D_Main_Rev_GetSrN(rd_val);
	// 4 MSB-s of SN are in Software revision register
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegSwRev );
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}
//printf("Press CR to continue <-\n");
//getchar();
	// Complete with 4 MSB-s
//	printf("%s: rd_val=0x%08x\n", __FUNCTION__, rd_val);
	feu_sn = (D_Main_Rev_GetSrN(rd_val)<<4) + feu_sn;
	printf("%s: Feu %d SN %d (beu %d lnk %d)\n", __FUNCTION__, feu_id, feu_sn, beu_id, beu_lnk_id);

	/*
	 *  Try to get Exclusive access to FEU
	 */
	if( (ret=Sem_Init(D_Feu_AdrReg_Set( D_CBus_SetModType( 0, D_CBus_Mod_Feu ), C_Feu_AdrReg_SlowCtrl ))) ==  D_RetCode_Wrn_SemBusy )
	{
		fprintf( stderr,  "%s: Sem_Init warning Semaphore Busy for feu=%d beu=%d lnk=%d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id );
	}
	else if( ret != D_RetCode_Sucsess )
	{
		fprintf( stderr,  "%s: Sem_Init failed for feu=%d beu=%d lnk=%d with %s %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, sem_msg, ret );
		return ret;
	}
	// Get exclusive access to FEU
	if( (ret = FeuConfig_Lock()) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: Feu_Lock fail for feu=%d sn=%d beu=%d lnk=%d with ret %d\n", __FUNCTION__, feu_id, feu_sn, beu_id, beu_lnk_id, ret ); 
		return ret;
	}

	/*
	 *  Set or reset the command
	 */
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegCommand );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	if( run )
		wr_val = D_Main_Cmd_Run_Set( rd_val );
	else
		wr_val = D_Main_Cmd_Run_Clr( rd_val );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Run_Get(wr_val) != D_Main_Cmd_Run_Get(rd_val) )
		{
			fprintf( stderr,  "%s: Main Command (Run) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}
	// Check
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, 0, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Run_Get(wr_val) != D_Main_Cmd_Run_Get(rd_val) )
		{
			fprintf( stderr,  "%s: Main Command (Run) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}	

	/*
	 *  Free exclusive access to FEU
	 */
	if( (ret = FeuConfig_UnLock()) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: Feu_Lock fail for feu=%d sn=%d beu=%d lnk=%d with ret %d\n", __FUNCTION__, feu_id, feu_sn, beu_id, beu_lnk_id, ret ); 
		return ret;
	}

        if( run )
        {
                // Clear stat
                if( (ret = FeuToggleCommand( feu_params, feu_id, beu_id, beu_lnk_id, ClearStat ) ) != D_RetCode_Sucsess )
                {
                        fprintf( stderr,  "%s: FeuToggleCommand ClearStat failed for feu=%d beu=%d lnk=%d with %d\n",
                                __FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
                        return D_RetCode_Err_NetIO;
                }
        }

	return D_RetCode_Sucsess;
}

/*
 * Toggle FEU commands
 */
int FeuToggleCommand( FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id, FeuTogCmd tog_cmd )
{
	unsigned int reg_adr;
	unsigned int wr_val;
	unsigned int rd_val;

	int ret;

	int dream_pair;
	int dream_power;

	int reg;
	int dream_min;
	int dream_max;
	int dream;

	int feu_sn;

	if( feu_params == (FeuParams *)NULL )
	{
		fprintf( stderr, "%s: feu_params=NULL\n", __FUNCTION__ ); 
		return D_RetCode_Err_Null_Pointer;
	}
	if( (feu_id < 1) || (DEF_MAX_NB_OF_FEU<=feu_id) )
	{
		fprintf( stderr, "%s: feu_index=%d<1 or >=%d\n", __FUNCTION__, feu_id, DEF_MAX_NB_OF_FEU ); 
		return D_RetCode_Err_Wrong_Param;
	}
	if( (beu_id < 1) || (DEF_MAX_NB_OF_BEU<=beu_id) )
	{
		fprintf( stderr, "%s: feu_id=%d<0 or >=%d\n", __FUNCTION__, beu_id, DEF_MAX_NB_OF_BEU ); 
		return D_RetCode_Err_Wrong_Param;
	}
	if( (beu_lnk_id < 0) || (DEF_MAX_NB_OF_FEU_PER_BEU<=beu_lnk_id) )
	{
		fprintf( stderr, "%s: feu_id=%d<0 or >=%d\n", __FUNCTION__, beu_lnk_id, DEF_MAX_NB_OF_FEU_PER_BEU ); 
		return D_RetCode_Err_Wrong_Param;
	}

	// First Fix the Peek/Poke global variables
	PeekPoke_Feu = feu_id;
	PeekPoke_Beu = beu_id;
	PeekPoke_Lnk = beu_lnk_id;

	/*
	 *  Try to get FEU ID
	 */
/*
	// Those are RO registers, so no semaphore is needed
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	// 4 LSB-s of SN are in Firmware revision register
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegFwRev );
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}
//printf("Press CR to continue <-\n");
//getchar();
	// Get the 4 LSB-s
//	printf("%s: rd_val=0x%08x\n", __FUNCTION__, rd_val);
	feu_sn = D_Main_Rev_GetSrN(rd_val);
	// 4 MSB-s of SN are in Software revision register
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegSwRev );
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}
//printf("Press CR to continue <-\n");
//getchar();
	// Complete with 4 MSB-s
//	printf("%s: rd_val=0x%08x\n", __FUNCTION__, rd_val);
	feu_sn = (D_Main_Rev_GetSrN(rd_val)<<4) + feu_sn;
//	printf("%s: Feu %d SN %d (beu %d lnk %d)\n", __FUNCTION__, feu_id, feu_sn, beu_id, beu_lnk_id);
*/
	/*
	 *  Try to get Exclusive access to FEU
	 */
	if( (ret=Sem_Init(D_Feu_AdrReg_Set( D_CBus_SetModType( 0, D_CBus_Mod_Feu ), C_Feu_AdrReg_SlowCtrl ))) ==  D_RetCode_Wrn_SemBusy )
	{
		fprintf( stderr,  "%s: Sem_Init warning Semaphore Busy for feu=%d beu=%d lnk=%d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id );
	}
	else if( ret != D_RetCode_Sucsess )
	{
		fprintf( stderr,  "%s: Sem_Init failed for feu=%d beu=%d lnk=%d with %s %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, sem_msg, ret );
		return ret;
	}
//printf("%s: After SemInit...\n", __FUNCTION__);
//getchar();
	// Get exclusive access to FEU
	if( (ret = FeuConfig_Lock()) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: Feu_Lock fail for feu=%d sn=%d beu=%d lnk=%d with ret %d\n", __FUNCTION__, feu_id, feu_sn, beu_id, beu_lnk_id, ret ); 
		return ret;
	}
//FeuConfig_UnLock();
//printf("%s: After FeuConfig_Lock...\n", __FUNCTION__);
//getchar();
//FeuConfig_Lock();
	/*
	 *  Set the command
	 */
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegCommand );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
//FeuConfig_UnLock();
//printf("%s: After Beu_ReqResp Read ...\n", __FUNCTION__);
//getchar();
//FeuConfig_Lock();
	wr_val = rd_val | tog_cmd;
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( (wr_val & tog_cmd) != (rd_val & tog_cmd) )
		{
			fprintf( stderr,  "%s: Main Command (%d) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, tog_cmd, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}
//FeuConfig_UnLock();
//printf("%s: After Beu_ReqResp Write...\n", __FUNCTION__);
//getchar();
//FeuConfig_Lock();
	// Check
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, 0, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( (wr_val & tog_cmd) != (rd_val & tog_cmd) )
		{
			fprintf( stderr,  "%s: Main Command (%d) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, tog_cmd, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}
//FeuConfig_UnLock();
//printf("%s: After Beu_ReqResp Check...\n", __FUNCTION__);
//getchar();
//FeuConfig_Lock();
//printf("%s: command 0x%x should be set in 0x%08x\n", __FUNCTION__, tog_cmd, rd_val);

	/*
	 *  Reset the command
	 */
	wr_val = rd_val & (~tog_cmd);
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( (wr_val & tog_cmd) != (rd_val & tog_cmd) )
		{
			fprintf( stderr,  "%s: Main Command (%d) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, tog_cmd, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}
//FeuConfig_UnLock();
//printf("%s: After Beu_ReqResp Write 2...\n", __FUNCTION__);
//getchar();
//FeuConfig_Lock();
	// Check
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, 0, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( (wr_val & tog_cmd) != (rd_val & tog_cmd) )
		{
			fprintf( stderr,  "%s: Main Command (%d) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, tog_cmd, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}
//printf("%s: command 0x%x should be set in 0x%08x\n", __FUNCTION__, tog_cmd, rd_val);

//FeuConfig_UnLock();
//printf("%s: After Beu_ReqResp Check 2...\n", __FUNCTION__);
//getchar();
//FeuConfig_Lock();
	/*
	 *  Free exclusive access to FEU
	 */
	if( (ret = FeuConfig_UnLock()) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: Feu_Lock fail for feu=%d sn=%d beu=%d lnk=%d with ret %d\n", __FUNCTION__, feu_id, feu_sn, beu_id, beu_lnk_id, ret ); 
		return ret;
	}
//printf("%s: After FeuConfig_UnLock...\n", __FUNCTION__);
//getchar();

	return D_RetCode_Sucsess;
}

/***************************************************************************
 ***************************************************************************
            Trigger Scan Functions
 ***************************************************************************
 ***************************************************************************/
FeuTrgScan feu_trg_scan[DEF_MAX_NB_OF_FEU];
struct timeval dream_scan_period;

int FeuTrgScan_Init( FeuTrgScan *scan, FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id )
{
	int dream;
	int dream_reg;
	unsigned short dream_reg_val[4];

	unsigned int reg_adr;
	unsigned int wr_val;
	unsigned int rd_val;

	int ret;

	int rx_err_cntr;
	int par_err_cntr;
	int pac_err_cnt;

	scan->drm_msk = feu_params->SelfTrig_DreamMask;
	scan->running_drm  = -1;
	scan->running_thr  = -1;
	scan->nb_of_checks = -1;
	scan->prev_cntr    = 0;

	dream_scan_period.tv_sec = DEF_DRM_TRG_CHK_PER;
	dream_scan_period.tv_usec = 0;

	// First Fix the Peek/Poke global variables
	PeekPoke_Feu = feu_id;
	PeekPoke_Beu = beu_id;
	PeekPoke_Lnk = beu_lnk_id;

	// Reset
	// Clear config command
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegCommand );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	wr_val = D_Main_Cmd_Config_Clr( rd_val );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Config_Get(wr_val) != D_Main_Cmd_Config_Get(rd_val) )
		{
			fprintf( stderr,  "%s: Main Command (Config_Clr) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Main Command (Config_Clr) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}
//printf("%s: the system should be in Init state\n", __FUNCTION__ );
//getchar();
	// Programme High threshold
	for( dream=0; dream<DEF_MAX_NB_OF_DREAM; dream++ )
	{
		scan->drm_thr[dream] = 0;

		// Set Highest threshold
		dream_reg = 1;
		dream_reg_val[0] = 0x081F;
		dream_reg_val[1] = 0xC000 | (feu_params->dream_params[dream].dream_reg[1].reg[1]&0x00FF);
		dream_reg_val[2] = 0;
		dream_reg_val[3] = 0;
		if( DreamSpiRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
		{
			if( DreamRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
			{
				fprintf( stderr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				if( sys_log_fptr != (FILE *)NULL )
					fprintf( sys_log_fptr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
						__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				return D_RetCode_Err_NetIO;
			}
			if( sys_log_fptr != (FILE *)NULL )
			{
				fprintf( sys_log_fptr,  "%s: DreamRegConfig OK for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				if( FeuComChanRxErr_Get( feu_params, feu_id, beu_id, beu_lnk_id, &rx_err_cntr, &par_err_cntr, &pac_err_cnt ) == D_RetCode_Sucsess )
				{
					fprintf( sys_log_fptr,  "%s: Link errors detecteod for feu=%d beu=%d lnk=%d rx=%d par=%d pac=%d\n\r",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, rx_err_cntr, par_err_cntr, pac_err_cnt );
				}
				else
				{
					fprintf( sys_log_fptr,  "%s: FeuComChanRxErr_Get failed for feu=%d beu=%d lnk=%d\n\r",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id );
				}
			}
		}

		// Mask channels 0 to 31
		dream_reg = 8;
		dream_reg_val[0] = 0xFFFF;
		dream_reg_val[1] = 0xFFFF;
		dream_reg_val[2] = 0;
		dream_reg_val[3] = 0;
		if( DreamSpiRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
		{
			if( DreamRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
			{
				fprintf( stderr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				if( sys_log_fptr != (FILE *)NULL )
					fprintf( sys_log_fptr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
						__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				return D_RetCode_Err_NetIO;
			}
			if( sys_log_fptr != (FILE *)NULL )
			{
				fprintf( sys_log_fptr,  "%s: DreamRegConfig OK for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				if( FeuComChanRxErr_Get( feu_params, feu_id, beu_id, beu_lnk_id, &rx_err_cntr, &par_err_cntr, &pac_err_cnt ) == D_RetCode_Sucsess )
				{
					fprintf( sys_log_fptr,  "%s: Link errors detecteod for feu=%d beu=%d lnk=%d rx=%d par=%d pac=%d\n\r",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, rx_err_cntr, par_err_cntr, pac_err_cnt );
				}
				else
				{
					fprintf( sys_log_fptr,  "%s: FeuComChanRxErr_Get failed for feu=%d beu=%d lnk=%d\n\r",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id );
				}
			}
		}
		// Mask channels 32 to 63
		dream_reg = 9;
		dream_reg_val[0] = 0xFFFF;
		dream_reg_val[1] = 0xFFFF;
		dream_reg_val[2] = 0;
		dream_reg_val[3] = 0;
		if( DreamSpiRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
		{
			if( DreamRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
			{
				fprintf( stderr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				if( sys_log_fptr != (FILE *)NULL )
					fprintf( sys_log_fptr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
						__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				return D_RetCode_Err_NetIO;
			}
			if( sys_log_fptr != (FILE *)NULL )
			{
				fprintf( sys_log_fptr,  "%s: DreamRegConfig OK for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				if( FeuComChanRxErr_Get( feu_params, feu_id, beu_id, beu_lnk_id, &rx_err_cntr, &par_err_cntr, &pac_err_cnt ) == D_RetCode_Sucsess )
				{
					fprintf( sys_log_fptr,  "%s: Link errors detecteod for feu=%d beu=%d lnk=%d rx=%d par=%d pac=%d\n\r",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, rx_err_cntr, par_err_cntr, pac_err_cnt );
				}
				else
				{
					fprintf( sys_log_fptr,  "%s: FeuComChanRxErr_Get failed for feu=%d beu=%d lnk=%d\n\r",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id );
				}
			}
		}
	}

	// Mask all Dreams out of self trigger
	reg_adr = D_CBus_SetModType(            0, D_CBus_Mod_SelfTrig );
	reg_adr = D_SelfTrig_AdrZone_Set( reg_adr, D_SelfTrig_AdrZone_Reg );
	reg_adr = D_SelfTrig_AdrReg_Set(  reg_adr, C_SelfTrig_AdrReg_Csr );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	wr_val = D_SelfTrig_Csr_DrmMsk_Set(rd_val);
	wr_val = D_SelfTrig_Csr_Mult_Set(wr_val);
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Config_Get(wr_val) != D_Main_Cmd_Config_Get(rd_val) )
		{
			fprintf( stderr,  "%s: SelfTrig_AdrReg_Csr(Dream Mask) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, rd_val );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: SelfTrig_AdrReg_Csr(Dream Mask) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}

	// FEU Prescale parameters
	// Set address address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Feu );
	reg_adr = D_Feu_AdrReg_Set( reg_adr, C_Feu_AdrReg_PreScale );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	// Disable data packet transfer through prescale
	wr_val = D_Feu_RegPreScale_EvtData_Clr( rd_val );
//fprintf( stdout, "%s: Prescale before settings rd_val=0x%08x wr_val=0x%08x\n", __FUNCTION__, rd_val, wr_val);
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( wr_val != rd_val )
		{
			fprintf( stderr,  "%s: FEU Prescale WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: FEU Prescale WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}
//fprintf( stdout, "%s: Prescale after settings rd_val=0x%08x wr_val=0x%08x\n", __FUNCTION__, rd_val, wr_val);
//getchar();

	// Switch trigger source to self trigger
	// Set configuration register address 
	reg_adr = D_CBus_SetModType(      0, D_CBus_Mod_TrigGen );
	reg_adr = D_Tg_AdrZone_Set( reg_adr, D_Tg_AdrZone_Reg );
	reg_adr = D_Tg_AdrReg_Set(  reg_adr, D_Tg_RegAdr_Conf );
	// Write
	// Only optical link trigger is supported 
	wr_val = D_Tg_RegConf_Set( 0, 0, feu_params->Trig_Conf_TrigPipeLen, Tg_Src_PushButton, 0 );
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Tg_RegConf_GetConfig(wr_val) != D_Tg_RegConf_GetConfig(rd_val) )
		{
			fprintf( stderr,  "%s: Trigger generator WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Trigger generator WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}

	// Initialize
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegCommand );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	wr_val = D_Main_Cmd_Config_Set( rd_val );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Config_Get(wr_val) != D_Main_Cmd_Config_Get(rd_val) )
		{
			fprintf( stderr,  "%s: Main Command (Config_Set) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Main Command (Config_Set) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}
//printf("%s: the system should be in IDLE state\n", __FUNCTION__ );
//getchar();
	return D_RetCode_Sucsess;
}

int FeuTrgScan_SetThr( FeuTrgScan *scan, FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id )
{
	unsigned int reg_adr;
	unsigned int wr_val;
	unsigned int rd_val;

	int dream;
	int dream_reg;
	unsigned short dream_reg_val[4];

	int ret;

	int rx_err_cntr;
	int par_err_cntr;
	int pac_err_cnt;

	// Nothing to do
	if( scan->drm_msk == 0xFF )
		return( D_RetCode_Sucsess );

	// First Fix the Peek/Poke global variables
	PeekPoke_Feu = feu_id;
	PeekPoke_Beu = beu_id;
	PeekPoke_Lnk = beu_lnk_id;

	// Stop run
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegCommand );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	wr_val = D_Main_Cmd_Run_Clr( rd_val );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Config_Get(wr_val) != D_Main_Cmd_Config_Get(rd_val) )
		{
			fprintf( stderr,  "%s: Main Command (Run_Clr) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Main Command (Run_Clr) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}

	// Reset
	// Clear config command
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegCommand );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	wr_val = D_Main_Cmd_Config_Clr( rd_val );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Config_Get(wr_val) != D_Main_Cmd_Config_Get(rd_val) )
		{
			fprintf( stderr,  "%s: Main Command (Config_Clr) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Main Command (Config_Clr) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}

	// Check if scan of a new dream has to be initiated
//fprintf(stderr, "%s: running_thr=%d\n\r", __FUNCTION__, scan->running_thr );
	if( scan->running_thr < 0 )
	{
		do
		{
			scan->running_drm++;
		} while( scan->drm_msk & (1<<scan->running_drm) );
//fprintf(stderr, "%s: running_drm=%d drm_msk=0x%02x\n\r", __FUNCTION__, scan->running_drm, scan->drm_msk );
//getchar();
		// Unmask the found Dream for self trigger
		// Mask all Dreams out of self trigger
		reg_adr = D_CBus_SetModType(            0, D_CBus_Mod_SelfTrig );
		reg_adr = D_SelfTrig_AdrZone_Set( reg_adr, D_SelfTrig_AdrZone_Reg );
		reg_adr = D_SelfTrig_AdrReg_Set(  reg_adr, C_SelfTrig_AdrReg_Csr );
		// Read
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
			return D_RetCode_Err_NetIO;
		}
		reg_adr = D_CBus_SetModType(            0, D_CBus_Mod_SelfTrig );
		reg_adr = D_SelfTrig_AdrZone_Set( reg_adr, D_SelfTrig_AdrZone_Reg );
		reg_adr = D_SelfTrig_AdrReg_Set(  reg_adr, C_SelfTrig_AdrReg_Csr );
		wr_val = (rd_val & 0xFFFFFF00) | ((~(1<<scan->running_drm)) & 0xFF);
		wr_val = D_SelfTrig_Csr_Mult_Clr(wr_val);
		// Write to register
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
			return D_RetCode_Err_NetIO;
		}
		else
		{
			if( D_Main_Cmd_Config_Get(wr_val) != D_Main_Cmd_Config_Get(rd_val) )
			{
				fprintf( stderr,  "%s: SelfTrig_AdrReg_Csr(Dream Mask) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, rd_val );
				if( sys_log_fptr != (FILE *)NULL )
					fprintf( sys_log_fptr,  "%s: SelfTrig_AdrReg_Csr(Dream Mask) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, rd_val );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		}

		dream = scan->running_drm;
		// Set channels 0 to 31
		dream_reg = 8;
		dream_reg_val[0] = feu_params->dream_params[dream].dream_reg[dream_reg].reg[0];
		dream_reg_val[1] = feu_params->dream_params[dream].dream_reg[dream_reg].reg[1];
		dream_reg_val[2] = 0;
		dream_reg_val[3] = 0;
		if( DreamSpiRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
		{
			if( DreamRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
			{
				fprintf( stderr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				if( sys_log_fptr != (FILE *)NULL )
					fprintf( sys_log_fptr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
						__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				return D_RetCode_Err_NetIO;
			}
			if( sys_log_fptr != (FILE *)NULL )
			{
				fprintf( sys_log_fptr,  "%s: DreamRegConfig OK for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				if( FeuComChanRxErr_Get( feu_params, feu_id, beu_id, beu_lnk_id, &rx_err_cntr, &par_err_cntr, &pac_err_cnt ) == D_RetCode_Sucsess )
				{
					fprintf( sys_log_fptr,  "%s: Link errors detecteod for feu=%d beu=%d lnk=%d rx=%d par=%d pac=%d\n\r",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, rx_err_cntr, par_err_cntr, pac_err_cnt );
				}
				else
				{
					fprintf( sys_log_fptr,  "%s: FeuComChanRxErr_Get failed for feu=%d beu=%d lnk=%d\n\r",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id );
				}
			}
		}
		// Set channels 32 to 63
		dream_reg = 9;
		dream_reg_val[0] = feu_params->dream_params[dream].dream_reg[dream_reg].reg[0];
		dream_reg_val[1] = feu_params->dream_params[dream].dream_reg[dream_reg].reg[1];
		dream_reg_val[2] = 0;
		dream_reg_val[3] = 0;
		if( DreamSpiRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
		{
			if( DreamRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
			{
				fprintf( stderr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				if( sys_log_fptr != (FILE *)NULL )
					fprintf( sys_log_fptr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
						__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				return D_RetCode_Err_NetIO;
			}
			if( sys_log_fptr != (FILE *)NULL )
			{
				fprintf( sys_log_fptr,  "%s: DreamRegConfig OK for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				if( FeuComChanRxErr_Get( feu_params, feu_id, beu_id, beu_lnk_id, &rx_err_cntr, &par_err_cntr, &pac_err_cnt ) == D_RetCode_Sucsess )
				{
					fprintf( sys_log_fptr,  "%s: Link errors detecteod for feu=%d beu=%d lnk=%d rx=%d par=%d pac=%d\n\r",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, rx_err_cntr, par_err_cntr, pac_err_cnt );
				}
				else
				{
					fprintf( sys_log_fptr,  "%s: FeuComChanRxErr_Get failed for feu=%d beu=%d lnk=%d\n\r",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id );
				}
			}
		}
		scan->running_thr = DEF_DRM_TRG_MIN_THR;
	}
	else
		scan->running_thr++;

	if( scan->running_thr <= DEF_DRM_TRG_MAX_THR )
	{
		// Set threshold
		dream = scan->running_drm;
		dream_reg = 1;
		dream_reg_val[0] = (feu_params->dream_params[dream].dream_reg[dream_reg].reg[0] & 0xFFE0) | (( scan->running_thr >> 2 ) & 0x1F);
		dream_reg_val[1] = ((scan->running_thr&0x3)<<14) | (feu_params->dream_params[dream].dream_reg[1].reg[1]&0x00FF);
		dream_reg_val[2] = 0;
		dream_reg_val[3] = 0;
//fprintf( stderr,  "%s: feu=%d dream=%d reg_val 0x%04x 0x%04x\n\r", __FUNCTION__, feu_id, dream, dream_reg_val[0], dream_reg_val[1] );
		if( DreamSpiRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
		{
			if( DreamRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
			{
				fprintf( stderr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				if( sys_log_fptr != (FILE *)NULL )
					fprintf( sys_log_fptr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
						__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				return D_RetCode_Err_NetIO;
			}
			if( sys_log_fptr != (FILE *)NULL )
			{
				fprintf( sys_log_fptr,  "%s: DreamRegConfig OK for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
				if( FeuComChanRxErr_Get( feu_params, feu_id, beu_id, beu_lnk_id, &rx_err_cntr, &par_err_cntr, &pac_err_cnt ) == D_RetCode_Sucsess )
				{
					fprintf( sys_log_fptr,  "%s: Link errors detecteod for feu=%d beu=%d lnk=%d rx=%d par=%d pac=%d\n\r",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, rx_err_cntr, par_err_cntr, pac_err_cnt );
				}
				else
				{
					fprintf( sys_log_fptr,  "%s: FeuComChanRxErr_Get failed for feu=%d beu=%d lnk=%d\n\r",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id );
				}
			}
		}
	}

	// Initialize
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegCommand );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	wr_val = D_Main_Cmd_Config_Set( rd_val );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Config_Get(wr_val) != D_Main_Cmd_Config_Get(rd_val) )
		{
			fprintf( stderr,  "%s: Main Command (Config_Set) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Main Command (Config_Set) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}

	// Enable run
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegCommand );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	wr_val = D_Main_Cmd_Run_Set( rd_val );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Config_Get(wr_val) != D_Main_Cmd_Config_Get(rd_val) )
		{
			fprintf( stderr,  "%s: Main Command (Run_Set) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Main Command (Run_Set) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}

	// Clear stat
	if( (ret = FeuToggleCommand( feu_params, feu_id, beu_id, beu_lnk_id, ClearStat ) ) != D_RetCode_Sucsess )
	{
		fprintf( stderr,  "%s: FeuToggleCommand ClearStat failed for feu=%d beu=%d lnk=%d with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: FeuToggleCommand ClearStat failed for feu=%d beu=%d lnk=%d with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		return D_RetCode_Err_NetIO;
	}

	scan->nb_of_checks = 0;
	scan->prev_cntr = 0;
	gettimeofday(&(scan->tstart), 0);

	return D_RetCode_Sucsess;
}

int FeuTrgScan_MskDrm( FeuTrgScan *scan, FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id )
{
	unsigned int reg_adr;
	unsigned int wr_val;
	unsigned int rd_val;

	int dream;
	int dream_reg;
	unsigned short dream_reg_val[4];

	int ret;

	int rx_err_cntr;
	int par_err_cntr;
	int pac_err_cnt;

	// First Fix the Peek/Poke global variables
	PeekPoke_Feu = feu_id;
	PeekPoke_Beu = beu_id;
	PeekPoke_Lnk = beu_lnk_id;

	// Stop run
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegCommand );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	wr_val = D_Main_Cmd_Run_Clr( rd_val );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Run_Get(wr_val) != D_Main_Cmd_Run_Get(rd_val) )
		{
			fprintf( stderr,  "%s: Main Command (Run_Clr) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Main Command (Run_Clr) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}

	// Reset
	// Clear config command
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegCommand );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	wr_val = D_Main_Cmd_Config_Clr( rd_val );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Config_Get(wr_val) != D_Main_Cmd_Config_Get(rd_val) )
		{
			fprintf( stderr,  "%s: Main Command (Config_Clr) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Main Command (Config_Clr) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}

	// Set threshold
	dream = scan->running_drm;
	dream_reg = 1;
	dream_reg_val[0] = 0x081F;
	dream_reg_val[1] = 0xC000 | (feu_params->dream_params[dream].dream_reg[1].reg[1]&0x00FF);
	dream_reg_val[2] = 0;
	dream_reg_val[3] = 0;
//fprintf( stderr,  "%s: feu=%d dream=%d reg_val 0x%04x 0x%04x\n\r", __FUNCTION__, feu_index, dream, dream_reg_val[0], dream_reg_val[1] );
	if( DreamSpiRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
	{
		if( DreamRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
		{
			fprintf( stderr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
				__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
			return D_RetCode_Err_NetIO;
		}
		if( sys_log_fptr != (FILE *)NULL )
		{
			fprintf( sys_log_fptr,  "%s: DreamRegConfig OK for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
				__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
			if( FeuComChanRxErr_Get( feu_params, feu_id, beu_id, beu_lnk_id, &rx_err_cntr, &par_err_cntr, &pac_err_cnt ) == D_RetCode_Sucsess )
			{
				fprintf( sys_log_fptr,  "%s: Link errors detecteod for feu=%d beu=%d lnk=%d rx=%d par=%d pac=%d\n\r",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, rx_err_cntr, par_err_cntr, pac_err_cnt );
			}
			else
			{
				fprintf( sys_log_fptr,  "%s: FeuComChanRxErr_Get failed for feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id );
			}
		}		
	}

	// Mask channels 0 to 31
	dream_reg = 8;
	dream_reg_val[0] = 0xFFFF;
	dream_reg_val[1] = 0xFFFF;
	dream_reg_val[2] = 0;
	dream_reg_val[3] = 0;
	if( DreamSpiRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
	{
		if( DreamRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
		{
			fprintf( stderr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
				__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
			return D_RetCode_Err_NetIO;
		}
		if( sys_log_fptr != (FILE *)NULL )
		{
			fprintf( sys_log_fptr,  "%s: DreamRegConfig OK for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
				__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
			if( FeuComChanRxErr_Get( feu_params, feu_id, beu_id, beu_lnk_id, &rx_err_cntr, &par_err_cntr, &pac_err_cnt ) == D_RetCode_Sucsess )
			{
				fprintf( sys_log_fptr,  "%s: Link errors detecteod for feu=%d beu=%d lnk=%d rx=%d par=%d pac=%d\n\r",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, rx_err_cntr, par_err_cntr, pac_err_cnt );
			}
			else
			{
				fprintf( sys_log_fptr,  "%s: FeuComChanRxErr_Get failed for feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id );
			}
		}
	}
	// Mask channels 32 to 63
	dream_reg = 9;
	dream_reg_val[0] = 0xFFFF;
	dream_reg_val[1] = 0xFFFF;
	dream_reg_val[2] = 0;
	dream_reg_val[3] = 0;
	if( DreamSpiRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
	{
		if( DreamRegConfig( dream, dream_reg, dream_reg_val, feu_id, beu_id, beu_lnk_id ) != D_RetCode_Sucsess )
		{
			fprintf( stderr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
				__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: DreamSpiRegConfig & DreamRegConfig failed for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
			return D_RetCode_Err_NetIO;
		}
		if( sys_log_fptr != (FILE *)NULL )
		{
			fprintf( sys_log_fptr,  "%s: DreamRegConfig OK for drm=%d reg=%d feu=%d beu=%d lnk=%d\n\r",
				__FUNCTION__, dream, dream_reg, feu_id, beu_id, beu_lnk_id );
			if( FeuComChanRxErr_Get( feu_params, feu_id, beu_id, beu_lnk_id, &rx_err_cntr, &par_err_cntr, &pac_err_cnt ) == D_RetCode_Sucsess )
			{
				fprintf( sys_log_fptr,  "%s: Link errors detecteod for feu=%d beu=%d lnk=%d rx=%d par=%d pac=%d\n\r",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, rx_err_cntr, par_err_cntr, pac_err_cnt );
			}
			else
			{
				fprintf( sys_log_fptr,  "%s: FeuComChanRxErr_Get failed for feu=%d beu=%d lnk=%d\n\r",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id );
			}
		}
	}

	// Mask all Dreams out of self trigger
	reg_adr = D_CBus_SetModType(            0, D_CBus_Mod_SelfTrig );
	reg_adr = D_SelfTrig_AdrZone_Set( reg_adr, D_SelfTrig_AdrZone_Reg );
	reg_adr = D_SelfTrig_AdrReg_Set(  reg_adr, C_SelfTrig_AdrReg_Csr );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x read with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x read with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
	}
	wr_val = D_SelfTrig_Csr_DrmMsk_Set(rd_val);
	wr_val = D_SelfTrig_Csr_Mult_Set(wr_val);
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x write with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x write with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Config_Get(wr_val) != D_Main_Cmd_Config_Get(rd_val) )
		{
			fprintf( stderr,  "%s: SelfTrig_AdrReg_Csr (Dream Mask) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, rd_val );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: SelfTrig_AdrReg_Csr (Dream Mask) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}


	// ReSync
	if( (ret = FeuToggleCommand( feu_params, feu_id, beu_id, beu_lnk_id, ReSync ) ) != D_RetCode_Sucsess )
	{
		fprintf( stderr,  "%s: FeuToggleCommand ReSync failed for feu=%d beu=%d lnk=%d with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: FeuToggleCommand ReSync failed for feu=%d beu=%d lnk=%d with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		return D_RetCode_Err_NetIO;
	}

	// Set running threshold negative
	scan->running_thr = -1;
	scan->nb_of_checks = -1;
	scan->prev_cntr = 0;

	// Mask in the flags also
	scan->drm_msk |= (1<<scan->running_drm);
	if( scan->drm_msk == 0xFF )
	{
		// Reset hardware
		if( (ret = FeuToggleCommand( feu_params, feu_id, beu_id, beu_lnk_id, HwReset ) ) != D_RetCode_Sucsess )
		{
			fprintf( stderr,  "%s: FeuToggleCommand LatchStat failed for feu=%d beu=%d lnk=%d with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: FeuToggleCommand LatchStat failed for feu=%d beu=%d lnk=%d with %d\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
			return D_RetCode_Err_NetIO;
		}
//fprintf( stderr,  "%s: FeuToggleCommand HwReset for feu=%d beu=%d lnk=%d\n\r", __FUNCTION__, feu_id, beu_id, beu_lnk_id );
		// looks like we've done with the FEU
		// Restore trigger module config parameters
		// Set configuration register address 
		reg_adr = D_CBus_SetModType(      0, D_CBus_Mod_TrigGen );
		reg_adr = D_Tg_AdrZone_Set( reg_adr, D_Tg_AdrZone_Reg );
		reg_adr = D_Tg_AdrReg_Set(  reg_adr, D_Tg_RegAdr_Conf );
		// Write
		// Only optical link trigger is supported 
		wr_val = D_Tg_RegConf_Set( 0, 0, feu_params->Trig_Conf_TrigPipeLen, Tg_Src_Int, 0 );
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
			return D_RetCode_Err_NetIO;
		}
		else
		{
			if( D_Tg_RegConf_GetConfig(wr_val) != D_Tg_RegConf_GetConfig(rd_val) )
			{
				fprintf( stderr,  "%s: Trigger generator WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
				if( sys_log_fptr != (FILE *)NULL )
					fprintf( sys_log_fptr,  "%s: Trigger generator WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		}

		// FEU Prescale parameters
		// Set address address 
		reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Feu );
		reg_adr = D_Feu_AdrReg_Set( reg_adr, C_Feu_AdrReg_PreScale );
		// Read
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
			return D_RetCode_Err_NetIO;
		}
		// Restore prescale
		wr_val = D_Feu_RegPreScale_EvtData_Put( rd_val, feu_params->Feu_PreScale_EvtData );
		// Write
		if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
		{
			fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr );
			return D_RetCode_Err_NetIO;
		}
		else
		{
			if( wr_val != rd_val )
			{
				fprintf( stderr,  "%s: FEU Prescale WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
				if( sys_log_fptr != (FILE *)NULL )
					fprintf( sys_log_fptr,  "%s: FEU Prescale WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
						__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
				return D_RetCode_Err_WrRd_Missmatch;
			}
		}
	}

	// Initialize
	// Set address 
	reg_adr = D_CBus_SetModType( 0, D_CBus_Mod_Main );
	reg_adr = D_MainAdr_Set( reg_adr, D_MainAdr_RegCommand );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	wr_val = D_Main_Cmd_Config_Set( rd_val );
	// Write
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_WRITE, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	else
	{
		if( D_Main_Cmd_Config_Get(wr_val) != D_Main_Cmd_Config_Get(rd_val) )
		{
			fprintf( stderr,  "%s: Main Command (Config_Set) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( sys_log_fptr,  "%s: Main Command (Config_Set) WrRd_Missmatch for feu=%d beu=%d lnk=%d reg=0x%06x wr_val=0x%08x rd_val=0x%08x\n",
					__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr,  wr_val, rd_val );
			return D_RetCode_Err_WrRd_Missmatch;
		}
	}

	if( scan->drm_msk == 0xFF )
		return 1; // looks like we've done with the FEU

	// Continue with other Dream
	return 0;
}

int FeuTrgScan_ChkCoin( FeuTrgScan *scan, FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id, int chk_cnt )
{
	unsigned int reg_adr;
	unsigned int wr_val;
	unsigned int rd_val;
	struct timeval tcur;
	struct timeval dt;

	int ret;

	scan->nb_of_checks++;

	// Latch stat
	if( (ret = FeuToggleCommand( feu_params, feu_id, beu_id, beu_lnk_id, LatchStat ) ) != D_RetCode_Sucsess )
	{
		fprintf( stderr,  "%s: FeuToggleCommand LatchStat failed for feu=%d beu=%d lnk=%d with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: FeuToggleCommand LatchStat failed for feu=%d beu=%d lnk=%d with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		return D_RetCode_Err_NetIO;
	}

	// First Fix the Peek/Poke global variables
	PeekPoke_Feu = feu_id;
	PeekPoke_Beu = beu_id;
	PeekPoke_Lnk = beu_lnk_id;

	// Get Coin LSB register
	reg_adr = D_CBus_SetModType(            0, D_CBus_Mod_SelfTrig );
	reg_adr = D_SelfTrig_AdrZone_Set( reg_adr, D_SelfTrig_AdrZone_Reg );
	reg_adr = D_SelfTrig_AdrReg_Set(  reg_adr, C_SelfTrig_AdrReg_CoinCntrLsb );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}

	// Compare counts
	if( scan->nb_of_checks == 1 )
	{
		scan->prev_cntr = rd_val;
		gettimeofday(&(scan->tstart), 0);
	}

	// Stop scanning threshold found
	if( scan->prev_cntr == rd_val )
	{
		// Chek if scan count finished
		if( scan->nb_of_checks == chk_cnt )
		{
			scan->drm_thr[scan->running_drm] = scan->running_thr;
			return 1;
		}
		// Chek if scan period finished
		gettimeofday(&tcur, 0);
		timersub(&tcur,&(scan->tstart),&dt);
		if( timercmp(&dt,&dream_scan_period,>) )
		{
			scan->drm_thr[scan->running_drm] = scan->running_thr;
			return 1;
		}
	}
	if( scan->prev_cntr != rd_val )
		if( scan->running_thr == DEF_DRM_TRG_MAX_THR )
		{
			// Stop scanning threshold not found
			scan->drm_thr[scan->running_drm] = scan->running_thr;
			return 1;
		}
		else
		{
			// Increase threshold, continue scan
			scan->nb_of_checks = -1;
			scan->prev_cntr = 0;
			return 0; // Increase threshold
		}

	// Continue to check
	return 0;
}

int FeuTrgScan_Sprintf( FeuTrgScan *scan, FeuParams *feu_params, int feu_index, char *buf )
{
	sprintf( buf, "# Feu %d Dream Trg Mask 0x%02x Thr 0=%3d 1=%3d 2=%3d 3=%3d 4=%3d 5=%3d 6=%3d 7=%3d\n\r",
		feu_index,
		feu_params->SelfTrig_DreamMask,
		scan->drm_thr[0], scan->drm_thr[1], scan->drm_thr[2], scan->drm_thr[3],
		scan->drm_thr[4], scan->drm_thr[5], scan->drm_thr[6], scan->drm_thr[7] );
	return D_RetCode_Sucsess;
}

int FeuTrgScan_Fprintf( FeuTrgScan *scan, FeuParams *feu_params, int feu_index, FILE *fptr )
{
	char buf[256];
	FeuTrgScan_Sprintf( scan, feu_params, feu_index, buf );
	fprintf( fptr, "%s", buf );
	fflush( fptr );
	return D_RetCode_Sucsess;
}

int FeuTrgScan_DumpConfigInfo( FeuTrgScan *scan, FeuParams *feu_params, int feu_index, FILE *fptr )
{
	int dream;
	fprintf( fptr, "# Feu %d Dream Trg Mask 0x%02x Thr 0=%3d 1=%3d 2=%3d 3=%3d 4=%3d 5=%3d 6=%3d 7=%3d\n\r",
		feu_index,
		feu_params->SelfTrig_DreamMask,
		scan->drm_thr[0], scan->drm_thr[1], scan->drm_thr[2], scan->drm_thr[3],
		scan->drm_thr[4], scan->drm_thr[5], scan->drm_thr[6], scan->drm_thr[7] );
	// Programme High threshold
	for( dream=0; dream<DEF_MAX_NB_OF_DREAM; dream++ )
	{
		if( ( feu_params->SelfTrig_DreamMask & (1<<dream) ) == 0 )
			fprintf( fptr, "Feu %d Dream %d 1 0x%04x 0x%04x 0x0000 0x0000 # thr=%d\n", feu_index, dream,
				0x0940 | ((scan->drm_thr[dream] >> 2) & 0x1F),
				((scan->drm_thr[dream]&0x3)<<14) | (feu_params->dream_params[dream].dream_reg[1].reg[1]&0x00FF),
				scan->drm_thr[dream] );
	}
	fprintf( fptr, "#\n" );
	return D_RetCode_Sucsess;
}

int FeuTrgScan_UpdateConfig( FeuTrgScan *scan, FeuParams *feu_params, int feu_index )
{
	int dream;
	int dream_reg_val;
	// Programme High threshold
	for( dream=0; dream<DEF_MAX_NB_OF_DREAM; dream++ )
	{
		if( ( feu_params->SelfTrig_DreamMask & (1<<dream) ) == 0 )
		{
			dream_reg_val = feu_params->dream_params[dream].dream_reg[1].reg[0];
			dream_reg_val = (dream_reg_val & 0xFFE0) | ((scan->drm_thr[dream]>>2) & 0x1F);
			feu_params->dream_params[dream].dream_reg[1].reg[0] = dream_reg_val;

			dream_reg_val = feu_params->dream_params[dream].dream_reg[1].reg[1];
			dream_reg_val = (dream_reg_val & 0x3FFF) | ((scan->drm_thr[dream]&0x3)<<14);
			feu_params->dream_params[dream].dream_reg[1].reg[1] = dream_reg_val;
		}
	}
	return D_RetCode_Sucsess;
}
/***************************************************************************
 ***************************************************************************
            Monitoring Functions
 ***************************************************************************
 ***************************************************************************/
int  FeuMonit_Sprintf( FeuMonit *feu_monit, char *buf  )
{
	char append_str[256];
	sprintf(buf, "Ti=%4.1fC ", feu_monit->Temp_MaxIntSensor );

	sprintf(append_str, "Tx=%4.1fC ", feu_monit->Temp_XilSideSensor );
	strcat( buf, append_str);

	sprintf(append_str, "Ta=%4.1fC ", feu_monit->Temp_AdcSideSensor );
	strcat( buf, append_str);

	sprintf(append_str, "I=%4.2fA ", feu_monit->Curent );
	strcat( buf, append_str);

	return D_RetCode_Sucsess;
}
int  FeuMonit_Fprintf( FeuMonit *feu_monit, FILE *fptr )
{
	char buf[256];
	FeuMonit_Sprintf( feu_monit, buf );
	fprintf( fptr, "%s", buf );
	fflush( fptr );
	return D_RetCode_Sucsess;
}

int FeuMonit_Get( FeuMonit *feu_monit, FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id )
{
	int ret;
	int reg;
	unsigned char rd_val;
	unsigned char rd_val_prev;
//	int feu_sn;

	// First Fix the Peek/Poke global variables
	PeekPoke_Feu = feu_id;
	PeekPoke_Beu = beu_id;
	PeekPoke_Lnk = beu_lnk_id;

	// Make sure the Link is active
	if( (ret = Beu_CheckFeuLink(feu_id, beu_id, beu_lnk_id)) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ChekFeuLink failed with %d for %d %d %d\n",
			__FUNCTION__, ret, feu_id, beu_id, beu_lnk_id );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ChekFeuLink failed with %d for %d %d %d\n",
				__FUNCTION__, ret, feu_id, beu_id, beu_lnk_id );
		return ret;
	} 

	/*
	 *  Try to get FEU ID
	 */
/*
	if( (ret = _FeuGetSn( feu_id, beu_id, beu_lnk_id, &feu_sn )) != D_RetCode_Sucsess )
	{
		fprintf( stderr,  "%s: _FeuGetSn failed for feu=%d beu=%d lnk=%d with %d\n",
			__FUNCTION__, feu_id, beu_id, beu_lnk_id, ret );
		return ret;
	}
	if( feu_sn != feu_params->Feu_RunCtrl_Id )
	{
		fprintf( stderr,  "%s: Unexpected Feu SN=%d for feu=%d beu=%d lnk=%d; SN=%d expected\n",
			__FUNCTION__, feu_sn, feu_id, beu_id, beu_lnk_id, feu_params->Feu_RunCtrl_Id );
		return D_RetCode_Err_Wrong_Param;
	}
*/
	// Get exclussive access to the FEU
	if( (ret = FeuConfig_Lock()) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: Feu_Lock fail for feu=%d sn=%d beu=%d lnk=%d with ret %d\n",
			__FUNCTION__, feu_id, feu_params->Feu_RunCtrl_Id, beu_id, beu_lnk_id, ret ); 
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr, "%s: Feu_Lock fail for feu=%d sn=%d beu=%d lnk=%d with ret %d\n",
				__FUNCTION__, feu_id, feu_params->Feu_RunCtrl_Id, beu_id, beu_lnk_id, ret ); 
		return ret;
	}

	// Now verify the max16031
	for( reg=16; reg<=22; reg++ )
	{
		if( (ret = I2C_Max16031_ReadByte( reg, &rd_val )) !=  D_RetCode_Sucsess )
		{
			fprintf( stderr,  "%s: I2C_Max16031_ReadByte(0x%2x) failed for feu=%d sn=%d beu=%d lnk=%d with %d\n",
				__FUNCTION__, reg, feu_id, feu_params->Feu_RunCtrl_Id, beu_id, beu_lnk_id, ret );
			if( sys_log_fptr != (FILE *)NULL )
				fprintf( stderr,  "%s: I2C_Max16031_ReadByte(0x%2x) failed for feu=%d sn=%d beu=%d lnk=%d with %d\n",
					__FUNCTION__, reg, feu_id, feu_params->Feu_RunCtrl_Id, beu_id, beu_lnk_id, ret );
			if( FeuConfig_UnLock() != D_RetCode_Sucsess )
			{
				fprintf( stderr, "%s: Feu_Lock fail for feu=%d sn=%d beu=%d lnk=%d with ret %d\n",
					__FUNCTION__, feu_id, feu_params->Feu_RunCtrl_Id, beu_id, beu_lnk_id, ret ); 
				if( sys_log_fptr != (FILE *)NULL )
					fprintf( sys_log_fptr, "%s: Feu_Lock fail for feu=%d sn=%d beu=%d lnk=%d with ret %d\n",
						__FUNCTION__, feu_id, feu_params->Feu_RunCtrl_Id, beu_id, beu_lnk_id, ret ); 
			}
			return ret;
		}
		if( (reg < 22) && (reg&0x1) )
		{
			// Check for MAX16031 manual for conversion
			if( (reg & 0xFE) == 16 )
				feu_monit->Temp_MaxIntSensor = ((double)((rd_val_prev<<2)+rd_val)-512)*0.5;
			else if( (reg & 0xFE) == 18 )
				feu_monit->Temp_XilSideSensor = ((double)((rd_val_prev<<2)+rd_val)-512)*0.5;
			else
				feu_monit->Temp_AdcSideSensor = ((double)((rd_val_prev<<2)+rd_val)-512)*0.5;
		}
		else if( reg == 22 )
		{
			// Check for MAX16031 manual for conversion
			// The last 1.2 factor is due to FEU design
			feu_monit->Curent = ((double)(rd_val))*1.4/6/255/0.01/1.2;
		}
		rd_val_prev = rd_val;
	} // for( reg=16; reg<=23; reg++ )

	/*
	 *  Free exclusive access to FEU
	 */
	if( (ret = FeuConfig_UnLock()) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: Feu_Lock fail for feu=%d sn=%d beu=%d lnk=%d with ret %d\n",
			__FUNCTION__, feu_id, feu_params->Feu_RunCtrl_Id, beu_id, beu_lnk_id, ret ); 
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr, "%s: Feu_Lock fail for feu=%d sn=%d beu=%d lnk=%d with ret %d\n",
				__FUNCTION__, feu_id, feu_params->Feu_RunCtrl_Id, beu_id, beu_lnk_id, ret ); 
		return ret;
	}

	return D_RetCode_Sucsess;
}

/***************************************************************************
 ***************************************************************************
            Attempt to get FEU optical link statistics
 ***************************************************************************
 ***************************************************************************/
int FeuComChanRxErr_Get( FeuParams *feu_params, int feu_id, int beu_id, int beu_lnk_id, int *rx_err_cntr, int *par_err_cntr, int *pac_err_cntr )
{
	int ret;
	unsigned int reg_adr;
	unsigned int wr_val;
	unsigned int rd_val;

	// First Fix the Peek/Poke global variables
	PeekPoke_Feu = feu_id;
	PeekPoke_Beu = beu_id;
	PeekPoke_Lnk = beu_lnk_id;

	// Readonly register no lock needed
	reg_adr = D_CBus_SetModType(           0, D_CBus_Mod_ComChan       );
	reg_adr = D_ComChan_AdrReg_Set(  reg_adr, C_ComChan_AdrReg_RxError );
	// Read
	if( (ret = Beu_ReqResp(feu_id, beu_id, beu_lnk_id, reg_adr, wr_val, DEF_FEU_READ, &rd_val ) ) < 0 )
	{
		fprintf( stderr,  "%s: Beu_ReqResp failed for feu=%d sn=%d beu=%d lnk=%d reg=0x%06x with %d\n",
			__FUNCTION__, feu_id, feu_params->Feu_RunCtrl_Id, beu_id, beu_lnk_id, reg_adr, ret );
		if( sys_log_fptr != (FILE *)NULL )
			fprintf( sys_log_fptr,  "%s: Beu_ReqResp failed for feu=%d sn=%d beu=%d lnk=%d reg=0x%06x with %d\n",
				__FUNCTION__, feu_id, feu_params->Feu_RunCtrl_Id, beu_id, beu_lnk_id, reg_adr, ret );
		return D_RetCode_Err_NetIO;
	}
	*rx_err_cntr  = D_ComChan_RxErr_Error_Get(rd_val);
	*par_err_cntr = D_ComChan_RxErr_Parity_Get(rd_val);
	*pac_err_cntr = D_ComChan_RxErr_RxPacErr_Get(rd_val);
	return D_RetCode_Sucsess;
}
