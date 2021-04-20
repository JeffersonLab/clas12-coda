/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    ConfigParams.c
-- Description:    Configuration parameters
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2011/12/16 IM
-- Revision:       1.0 2012/07/26 IM CmOffset parameter added
--                     2012/09/07 IM Pedestal & Threshold memory configuration added
--                     2012/09/20 IM Triger generator memory configuration added
--                     2012/09/21 IM UdpChan_Delay parameter added
--                     2013/11/26 IM Eeprom added
--                     2014/02/25 IM Add DrDblSmpClk parameter
--                     2014/03/29 IM Common parameter propagation added
--                     2014/04/09 IM UdpChan_MultiPackEnb parameter added
--                                   Feu_InterPacket_Delay replaces UdpChan_Delay
--                     2014/04/15 IM UdpChan_MultiPackThr parameter added
--                     2014/04/22 IM Trigger Interface parameter added
--                     2014/04/30 IM Self Trigger parameter added
--                     2014/07/04 IM Self Trigger topology parameter added
--                     2014/09/03 IM Trigger Interface Bert parameter added
--                     2014/09/11 IM Cast AdcRegFlag_Unset to char in assignement & comparison
--                     2014/09/24 IM Improve dumping of parameters to file
--                     2014/10/09 IM Common Parser instead of local 
--                 1.1 2014/11/17 IM Feu_RunCtrl_EvTstExt parameter added
--                 1.2 2016/01/11 IM Trig_Conf_TrigVetoLen parameter added to TrigGen
--                 1.3 2016/02/24 IM Number of Topological trigger registers increased from 8 to 32
--                 1.4 2016/03/15 IM Max16031 registers added
--                     2016/03/18 IM Gradually replace Main_Conf_DataPipeLen by Main_Conf_SparseRd parameter
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "Parser.h"
#include "ReturnCodes.h"
#include "FeuConfigParams.h"

/***************************************************************/
/* Parse a command line                                        */
/***************************************************************/
/*
#define LINE_SIZE	256
#define NBMAX_ARGV 	32

//typedef char ArgvArray[NBMAX_ARGV][LINE_SIZE];
int        argc;
ArgvArray  argv;

void parse_line(char *s)
{
	int i = 0;
	int j = 0;

	argc=0;
	while (    (*s != '\0')
			&& (*s != '\n')
			&& (*s != '\r')
//			&& (*s != '*' )
			&& (*s != '#' )  )
	{
		if ( (*s != ' ') && (*s != '\t') && (*s != '=') )
		{
			if( !i )
				j = argc++;
			argv[j][i++] = *s;
		}
		else
		{
			if( i )
				argv[j][i++] = '\0';
			i=0;
		}
		s++;
	}
	if( i )
		argv[j][i++] = '\0';
}
*/
/********************************************************************/
/****** Extract a word from line ************************************/
/********************************************************************/
/*
char *getword( line, word )
char *line;
char *word;
{
	int done;

	done = 0;
	while( (*line != '\0') && ((*line == ' ' ) || (*line == '\t') || (*line == '=')) )
		line++;
	while(done==0)
	{
		switch(*line)
		{
			case '=':
			case ' ':
			case '\t':
			case '\0':
			case '\n':
				*word++ = '\0';
				done = 1;
				break;
			default:
				*word++ = *line++;
		}
	}
	while( (*line != '\0') && ((*line == ' ' ) || (*line == '\t') || (*line == '=')) )
		line++;

	return line;
}
*/

int FeuParams_Init( FeuParams *feu_params )
{
	int index;
	int dream;
	int reg;

	if( feu_params == (FeuParams *)NULL )
	{
		fprintf( stderr, "FeuParams_Init: feu_params=0\n" );
		return D_RetCode_Err_Null_Pointer;
	}

	// Main module config parameters
	feu_params->Main_Conf_ClkSel[0]      ='\0';
	feu_params->Main_Conf_AdcDtp         =  0; // Depricated
	feu_params->Main_Conf_DataPipeLen    = -1; // Use for sparce readout
	feu_params->Main_Conf_DreamMask      = -1;
	feu_params->Main_Conf_Samples        = -1;
	// Main module Trigger logic parameters
	feu_params->Main_Trig_TimeStamp      = -1;
	feu_params->Main_Trig_OvrWrnLwm      = -1;
	feu_params->Main_Trig_OvrWrnHwm      = -1;
	feu_params->Main_Trig_OvrThersh      = -1;
	feu_params->Main_Trig_LocThrot       = -1;

	// FEU PowerUp Register
	feu_params->Feu_Pwr_Dream            = -1;
	feu_params->Feu_Pwr_PrtFlt           = -1; 
	feu_params->Feu_Pwr_Adc              =  0; // Depricated
	// FEU Run Control parameters
	feu_params->Feu_RunCtrl_Pd           = -1;
	feu_params->Feu_RunCtrl_CM           = -1;
	feu_params->Feu_RunCtrl_ZS           = -1;
	feu_params->Feu_RunCtrl_DrOvr        = -1;
	feu_params->Feu_RunCtrl_DrDblSmpClk  = -1;
	feu_params->Feu_RunCtrl_ZsChkSmp     = -1;
	feu_params->Feu_RunCtrl_Id           = -1;
	feu_params->Feu_RunCtrl_AdcDatRdyDel = -1;
	feu_params->Feu_RunCtrl_EvTstExt     = -1;
	feu_params->Feu_RunCtrl_RdDel        = -1;
	feu_params->Feu_RunCtrl_CmOffset     = -1;
	feu_params->Feu_RunCtrl_PdFile[0]    ='\0';
	feu_params->Feu_RunCtrl_ZsFile[0]    ='\0';

	// Common Pulser parameters
	feu_params->Feu_Pulser_Enable        = -1;
	// TbDream Pulser parameters
	feu_params->Feu_Pulser_BaseVal       = -1;
	feu_params->Feu_Pulser_TrigVal       = -1;
	feu_params->Feu_Pulser_TestFun       = -1;
	// Feu Pulser parameters
	feu_params->Feu_Pulser_DreamTst      = -1;
	feu_params->Feu_Pulser_PulseWid      = -1;
	// FEU Prescale parameters
	feu_params->Feu_PreScale_EvtData     = -1;
	feu_params->Feu_InterPacket_Delay    = -1;

	// Trigger Interface/Generator parameters
	feu_params->Trig_Conf_Rate           = -1;
	feu_params->Trig_Conf_Src[0]         ='\0';
	feu_params->Trig_Conf_TrigPipeLen    = -1;
	feu_params->Trig_Conf_TrigVetoLen    = -1;
	feu_params->Trig_Conf_File[0]        ='\0';

	// Auxiliari Trigger Interface
	feu_params->TI_Ignore                =  -1;
	feu_params->TI_DcBal_Enc             =  -1;
	feu_params->TI_DcBal_Dec             =  -1;
	feu_params->TI_Bert                  =  -1;

	// Self Trigger parameters
	feu_params->SelfTrig_DreamMask       = -1;
	feu_params->SelfTrig_Mult            = -1;
	feu_params->SelfTrig_CmbHitPropFb    = -1;
	feu_params->SelfTrig_CmbHitPropOl    = -1;
	feu_params->SelfTrig_DrmHitWid       = -1;
	feu_params->SelfTrig_CmbHitWid       = -1;
	feu_params->SelfTrig_TrigTopo        = -1;
	feu_params->SelfTrig_Veto            = -1;
	for( index=0; index<D_FeuPar_SelfTrigTopo_Size; index++ )
		feu_params->SelfTrig_Topology[index] = 0;

	// Communication registers
	feu_params->UdpChan_Enable           = -1;
	feu_params->UdpChan_Delay            =  0; // Depricated
	feu_params->UdpChan_MultiPackEnb     = -1;
	feu_params->UdpChan_MultiPackThr     = -1;
	feu_params->ComChan_Enable           = -1;
	feu_params->NetChan_Ip[0]            ='\0';

	// Dream parameters
	for( dream=0; dream<D_FeuPar_NumOfDreams; dream++ )
	{
		for( reg=0; reg<D_DreamPar_NumOfRegs; reg++ )
			feu_params->dream_params[dream].dream_reg[reg].reg[0]=-1;
	}
	// Dream clock parameters
	feu_params->dream_clk_params.RdClk_Div    = (double)0.0;
	feu_params->dream_clk_params.WrClk_Div    = (double)0.0;
	feu_params->dream_clk_params.WrClk_Phase  = -1;
	feu_params->dream_clk_params.AdcClk_Phase = -1;

	// Adc parameters
	for( reg=0; reg<D_AdcPar_NumOfRegs; reg++ )
		feu_params->adc_params.adc_reg[reg].flg=(char)AdcRegFlag_Unset;

	// EE Prom values
	for( reg=0; reg<D_FeuPar_EeProm_Size; reg++ )
		feu_params->ee_prom[reg]=(short)(-1);

	// max16031 values
	for( reg=0; reg<D_FeuPar_Max16031_Size; reg++ )
		feu_params->max16031[reg]=(short)(-1);

	return D_RetCode_Sucsess;
}


int FeuParamsCol_Init( FeuParamsCol *feu_params_col )
{
	int index;
	int feu;
	int dream;
	int reg;
	int ret;

	if( feu_params_col == (FeuParamsCol *)NULL )
	{
		fprintf( stderr, "FeuParamsCol_Init: feu_params_col=0\n" );
		return D_RetCode_Err_Null_Pointer;
	}

	for( feu=0; feu<D_FeuParamsCol_NumOfFeuParams; feu++ )
	{
		ret = FeuParams_Init( &(feu_params_col->feu_params[feu]) );
		if( ret != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: FeuParams_Init failed for feu=%d with %d\n", __FUNCTION__, feu, ret );
			return ret;
		}
	}
	return D_RetCode_Sucsess;
}
int FeuParamsCol_Sprintf( FeuParamsCol *feu_params_col, char *buf  )
{
	int feu;
	char feu_str[16];
	int dream;
	char dream_str[16];
	int reg;
	char append_str[256];

	if( feu_params_col == (FeuParamsCol *)NULL )
	{
		fprintf( stderr, "%s: feu_params_col=Null\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	if( buf == (char *)NULL )
	{
		fprintf( stderr, "%s: buf=Null\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Create header
//	sprintf( buf, "###########################\n" );

	// Go through all feu-s
	for( feu=0; feu<D_FeuParamsCol_NumOfFeuParams; feu++ )
	{
		if( feu == 0 ) // Default
			sprintf( feu_str, "%s", "*" );
		else
			sprintf( feu_str, "%d", feu );
		// Treat only the FEUs with ID set
//fprintf( stderr, "%s: feu=%d ID=%d\n", __FUNCTION__, feu, feu_params_col->feu_params[feu].Feu_RunCtrl_Id );
		if( feu_params_col->feu_params[feu].Feu_RunCtrl_Id != -1 )
		{
			strcat( buf,         "########################\n" );
			sprintf( append_str, "# Parameters for Feu=%s\n", feu_str );
			strcat( buf, append_str);
			strcat( buf,         "########################\n" );

			// Main configuration register
			strcat( buf, "# Main module configuration\n" );
			if( (feu == 0) || (strcmp( feu_params_col->feu_params[feu].Main_Conf_ClkSel, feu_params_col->feu_params[0].Main_Conf_ClkSel ) != '\0') )
			{
				sprintf( append_str, "Feu %s Main_Conf_ClkSel %s\n", feu_str, feu_params_col->feu_params[feu].Main_Conf_ClkSel );
				strcat( buf, append_str);
			}
			if
			(
				( (feu == 0) && (feu_params_col->feu_params[0].Main_Conf_AdcDtp != 0) )
				|| 
				( feu_params_col->feu_params[feu].Main_Conf_AdcDtp != feu_params_col->feu_params[0].Main_Conf_AdcDtp )
			)
			{
				sprintf( append_str, "Feu %s Main_Conf_AdcDtp %d\n", feu_str, feu_params_col->feu_params[feu].Main_Conf_AdcDtp );
				strcat( buf, append_str);
			}
			if( (feu == 0) || ( feu_params_col->feu_params[feu].Main_Conf_DataPipeLen != feu_params_col->feu_params[0].Main_Conf_DataPipeLen) )
			{
				sprintf( append_str, "Feu %s Main_Conf_SparseRd %d\n", feu_str, feu_params_col->feu_params[feu].Main_Conf_DataPipeLen );
//				sprintf( append_str, "Feu %s Main_Conf_DataPipeLen %d\n", feu_str, feu_params_col->feu_params[feu].Main_Conf_DataPipeLen );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Main_Conf_DreamMask != feu_params_col->feu_params[0].Main_Conf_DreamMask) )
			{
				sprintf( append_str, "Feu %s Main_Conf_DreamMask 0x%02x\n", feu_str, feu_params_col->feu_params[feu].Main_Conf_DreamMask );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Main_Conf_Samples != feu_params_col->feu_params[0].Main_Conf_Samples) )
			{
				sprintf( append_str, "Feu %s Main_Conf_Samples %d\n", feu_str, feu_params_col->feu_params[feu].Main_Conf_Samples );
				strcat( buf, append_str);
			}
//fprintf( stderr, "%s: Main conf OK\n", __FUNCTION__ );

			// Main module trigger register
			strcat( buf, "# Main module trigger configuration\n" );
			if( (feu == 0) || (feu_params_col->feu_params[feu].Main_Trig_TimeStamp != feu_params_col->feu_params[0].Main_Trig_TimeStamp) )
			{
				sprintf( append_str, "Feu %s Main_Trig_TimeStamp %d\n", feu_str, feu_params_col->feu_params[feu].Main_Trig_TimeStamp );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Main_Trig_OvrWrnLwm != feu_params_col->feu_params[0].Main_Trig_OvrWrnLwm) )
			{
				sprintf( append_str, "Feu %s Main_Trig_OvrWrnLwm %d\n", feu_str, feu_params_col->feu_params[feu].Main_Trig_OvrWrnLwm );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Main_Trig_OvrWrnHwm != feu_params_col->feu_params[0].Main_Trig_OvrWrnHwm) )
			{
				sprintf( append_str, "Feu %s Main_Trig_OvrWrnHwm %d\n", feu_str, feu_params_col->feu_params[feu].Main_Trig_OvrWrnHwm );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Main_Trig_OvrThersh != feu_params_col->feu_params[0].Main_Trig_OvrThersh) )
			{
				sprintf( append_str, "Feu %s Main_Trig_OvrThersh %d\n", feu_str, feu_params_col->feu_params[feu].Main_Trig_OvrThersh );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Main_Trig_LocThrot !=  feu_params_col->feu_params[0].Main_Trig_LocThrot) )
			{
				sprintf( append_str, "Feu %s Main_Trig_LocThrot %d\n", feu_str, feu_params_col->feu_params[feu].Main_Trig_LocThrot );
				strcat( buf, append_str);
			}
//fprintf( stderr, "%s: Main trig OK\n", __FUNCTION__ );

			// FEU PowerUp Register
			strcat( buf, "# FEU PowerUp Register\n" );
			if( (feu == 0) || (feu_params_col->feu_params[feu].Feu_Pwr_Dream != feu_params_col->feu_params[0].Feu_Pwr_Dream) )
			{
				sprintf( append_str, "Feu %s Feu_Pwr_Dream 0x%1x\n", feu_str, feu_params_col->feu_params[feu].Feu_Pwr_Dream );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Feu_Pwr_PrtFlt != feu_params_col->feu_params[0].Feu_Pwr_PrtFlt) )
			{
				sprintf( append_str, "Feu %s Feu_Pwr_PrtFlt 0x%04x\n", feu_str, feu_params_col->feu_params[feu].Feu_Pwr_PrtFlt );
				strcat( buf, append_str);
			}
			if
			(
				( (feu == 0) && (feu_params_col->feu_params[0].Feu_Pwr_Adc != 0) ) 
				||
				(feu_params_col->feu_params[feu].Feu_Pwr_Adc != feu_params_col->feu_params[0].Feu_Pwr_Adc)
			)
			{
				sprintf( append_str, "Feu %s Feu_Pwr_Adc %d\n", feu_str, feu_params_col->feu_params[feu].Feu_Pwr_Adc );
				strcat( buf, append_str);
			}
//fprintf( stderr, "%s: Feu power OK\n", __FUNCTION__ );

			// FEU Run Control parameters
			strcat( buf, "# FEU Control parameters\n" );
			if( (feu == 0) || (feu_params_col->feu_params[feu].Feu_RunCtrl_Pd != feu_params_col->feu_params[0].Feu_RunCtrl_Pd) )
			{
				sprintf( append_str, "Feu %s Feu_RunCtrl_Pd %d\n", feu_str, feu_params_col->feu_params[feu].Feu_RunCtrl_Pd );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Feu_RunCtrl_CM != feu_params_col->feu_params[0].Feu_RunCtrl_CM) )
			{
				sprintf( append_str, "Feu %s Feu_RunCtrl_CM %d\n", feu_str, feu_params_col->feu_params[feu].Feu_RunCtrl_CM );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Feu_RunCtrl_ZS != feu_params_col->feu_params[0].Feu_RunCtrl_ZS) )
			{
				sprintf( append_str, "Feu %s Feu_RunCtrl_ZS %d\n", feu_str, feu_params_col->feu_params[feu].Feu_RunCtrl_ZS );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Feu_RunCtrl_DrOvr != feu_params_col->feu_params[0].Feu_RunCtrl_DrOvr) )
			{
				sprintf( append_str, "Feu %s Feu_RunCtrl_DrOvr %d\n", feu_str, feu_params_col->feu_params[feu].Feu_RunCtrl_DrOvr );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Feu_RunCtrl_DrDblSmpClk != feu_params_col->feu_params[0].Feu_RunCtrl_DrDblSmpClk) )
			{
				sprintf( append_str, "Feu %s Feu_RunCtrl_DrDblSmpClk %d\n", feu_str, feu_params_col->feu_params[feu].Feu_RunCtrl_DrDblSmpClk );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Feu_RunCtrl_EvTstExt != feu_params_col->feu_params[0].Feu_RunCtrl_EvTstExt) )
			{
				sprintf( append_str, "Feu %s Feu_RunCtrl_EvTstExt %d\n", feu_str, feu_params_col->feu_params[feu].Feu_RunCtrl_EvTstExt );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Feu_RunCtrl_RdDel != feu_params_col->feu_params[0].Feu_RunCtrl_RdDel) )
			{
				sprintf( append_str, "Feu %s Feu_RunCtrl_RdDel %d\n", feu_str, feu_params_col->feu_params[feu].Feu_RunCtrl_RdDel );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Feu_RunCtrl_ZsChkSmp != feu_params_col->feu_params[0].Feu_RunCtrl_ZsChkSmp) )
			{
				sprintf( append_str, "Feu %s Feu_RunCtrl_ZsChkSmp %d\n", feu_str, feu_params_col->feu_params[feu].Feu_RunCtrl_ZsChkSmp );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Feu_RunCtrl_CmOffset != feu_params_col->feu_params[0].Feu_RunCtrl_CmOffset) )
			{
				sprintf( append_str, "Feu %s Feu_RunCtrl_CmOffset %d\n", feu_str, feu_params_col->feu_params[feu].Feu_RunCtrl_CmOffset );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Feu_RunCtrl_AdcDatRdyDel != feu_params_col->feu_params[0].Feu_RunCtrl_AdcDatRdyDel) )
			{
				sprintf( append_str, "Feu %s Feu_RunCtrl_AdcDatRdyDel %d\n", feu_str, feu_params_col->feu_params[feu].Feu_RunCtrl_AdcDatRdyDel );
				strcat( buf, append_str);
			}

//			if( feu_params_col->feu_params[feu].Feu_RunCtrl_Id != 255 )
			{
				sprintf( append_str, "Feu %s Feu_RunCtrl_Id %d\n", feu_str, feu_params_col->feu_params[feu].Feu_RunCtrl_Id );
				strcat( buf, append_str);
			}
//fprintf( stderr, "%s: Feu run ctrl OK\n", __FUNCTION__ );

			// FEU Pedestal Memory
			if( (feu == 0) || (strcmp( feu_params_col->feu_params[feu].Feu_RunCtrl_PdFile, feu_params_col->feu_params[0].Feu_RunCtrl_PdFile ) != 0) )
			{
				sprintf( append_str, "Feu %s Feu_RunCtrl_PdFile %s\n", feu_str, feu_params_col->feu_params[feu].Feu_RunCtrl_PdFile );
				strcat( buf, append_str);
			}
			// FEU Threshold Memory
			if( (feu == 0) || (strcmp( feu_params_col->feu_params[feu].Feu_RunCtrl_ZsFile, feu_params_col->feu_params[0].Feu_RunCtrl_ZsFile ) != 0) )
			{
				sprintf( append_str, "Feu %s Feu_RunCtrl_ZsFile %s\n", feu_str, feu_params_col->feu_params[feu].Feu_RunCtrl_ZsFile );
				strcat( buf, append_str);
			}
//fprintf( stderr, "%s: Feu mem OK\n", __FUNCTION__ );

			// FEU Pulser Register
			strcat( buf, "# FEU Pulser parameters\n" );
			if( (feu == 0) || (feu_params_col->feu_params[feu].Feu_Pulser_Enable != feu_params_col->feu_params[0].Feu_Pulser_Enable) )
			{
				sprintf( append_str, "Feu %s Feu_Pulser_Enable %d\n", feu_str, feu_params_col->feu_params[feu].Feu_Pulser_Enable );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Feu_Pulser_DreamTst != feu_params_col->feu_params[0].Feu_Pulser_DreamTst) )
			{
				sprintf( append_str, "Feu %s Feu_Pulser_DreamTst 0x%2x\n", feu_str, feu_params_col->feu_params[feu].Feu_Pulser_DreamTst );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Feu_Pulser_PulseWid != feu_params_col->feu_params[0].Feu_Pulser_PulseWid) )
			{
				sprintf( append_str, "Feu %s Feu_Pulser_PulseWid %d\n", feu_str, feu_params_col->feu_params[feu].Feu_Pulser_PulseWid );
				strcat( buf, append_str);
			}
//fprintf( stderr, "%s: Feu pulser OK\n", __FUNCTION__ );

			// FEU Prescale Register
			strcat( buf, "# FEU Prescale parameters\n" );
			if( (feu == 0) || (feu_params_col->feu_params[feu].Feu_PreScale_EvtData != feu_params_col->feu_params[0].Feu_PreScale_EvtData) )
			{
				sprintf( append_str, "Feu %s Feu_PreScale_EvtData %4d\n", feu_str, feu_params_col->feu_params[feu].Feu_PreScale_EvtData );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Feu_InterPacket_Delay != feu_params_col->feu_params[0].Feu_InterPacket_Delay) )
			{
				sprintf( append_str, "Feu %s Feu_InterPacket_Delay %4d\n", feu_str, feu_params_col->feu_params[feu].Feu_InterPacket_Delay );
				strcat( buf, append_str);
			}
//fprintf( stderr, "%s: Feu Prescale OK\n", __FUNCTION__ );

			// Trigger generator parameters
			strcat( buf, "# FEU Trigger Generator parameters\n" );
			if( (feu == 0) || (feu_params_col->feu_params[feu].Trig_Conf_Rate != feu_params_col->feu_params[0].Trig_Conf_Rate) )
			{
				sprintf( append_str, "Feu %s Trig_Conf_Rate %d\n", feu_str, feu_params_col->feu_params[feu].Trig_Conf_Rate );
				strcat( buf, append_str);
			}
			if( (feu == 0)  || ( strcmp( feu_params_col->feu_params[feu].Trig_Conf_Src, feu_params_col->feu_params[0].Trig_Conf_Src ) != 0 ) )
			{
				sprintf( append_str, "Feu %s Trig_Conf_Src %s\n", feu_str, feu_params_col->feu_params[feu].Trig_Conf_Src );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Trig_Conf_TrigPipeLen != feu_params_col->feu_params[0].Trig_Conf_TrigPipeLen) )
			{
				sprintf( append_str, "Feu %s Trig_Conf_TrigPipeLen %d\n", feu_str, feu_params_col->feu_params[feu].Trig_Conf_TrigPipeLen );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].Trig_Conf_TrigVetoLen != feu_params_col->feu_params[0].Trig_Conf_TrigVetoLen) )
			{
				sprintf( append_str, "Feu %s Trig_Conf_TrigVetoLen %d\n", feu_str, feu_params_col->feu_params[feu].Trig_Conf_TrigVetoLen );
				strcat( buf, append_str);
			}
			if( (feu == 0) || ( strcmp( feu_params_col->feu_params[feu].Trig_Conf_File, feu_params_col->feu_params[0].Trig_Conf_File ) != 0 ) )
			{
				sprintf( append_str, "Feu %s Trig_Conf_File %s\n", feu_str, feu_params_col->feu_params[feu].Trig_Conf_File );
				strcat( buf, append_str);
			}
//fprintf( stderr, "%s: trig gen OK\n", __FUNCTION__ );

			// Auxiliary Trigger Interface parameters
			strcat( buf, "# FEU Auxiliary Trigger Interface parameters\n" );
			if( (feu == 0) || (feu_params_col->feu_params[feu].TI_Ignore != feu_params_col->feu_params[0].TI_Ignore) )
			{
				sprintf( append_str, "Feu %s TI_Ignore %d\n", feu_str, feu_params_col->feu_params[feu].TI_Ignore );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].TI_DcBal_Enc != feu_params_col->feu_params[0].TI_DcBal_Enc) )
			{
				sprintf( append_str, "Feu %s TI_DcBal_Enc %d\n", feu_str, feu_params_col->feu_params[feu].TI_DcBal_Enc );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].TI_DcBal_Dec != feu_params_col->feu_params[0].TI_DcBal_Dec) )
			{
				sprintf( append_str, "Feu %s TI_DcBal_Dec %d\n", feu_str, feu_params_col->feu_params[feu].TI_DcBal_Dec );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].TI_Bert != feu_params_col->feu_params[0].TI_Bert) )
			{
				sprintf( append_str, "Feu %s TI_Bert %d\n", feu_str, feu_params_col->feu_params[feu].TI_Bert );
				strcat( buf, append_str);
			}
//fprintf( stderr, "%s: Aux OK\n", __FUNCTION__ );

			// Self Trigger parameters
			strcat( buf, "# FEU Self Trigger parameters\n" );
			if( (feu == 0) || (feu_params_col->feu_params[feu].SelfTrig_DreamMask != feu_params_col->feu_params[0].SelfTrig_DreamMask) )
			{
				sprintf( append_str, "Feu %s SelfTrig_DreamMask 0x%02x\n", feu_str, feu_params_col->feu_params[feu].SelfTrig_DreamMask );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].SelfTrig_Mult != feu_params_col->feu_params[0].SelfTrig_Mult) )
			{
				sprintf( append_str, "Feu %s SelfTrig_Mult %d\n", feu_str, feu_params_col->feu_params[feu].SelfTrig_Mult );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].SelfTrig_CmbHitPropFb != feu_params_col->feu_params[0].SelfTrig_CmbHitPropFb) )
			{
				sprintf( append_str, "Feu %s SelfTrig_CmbHitPropFb %d\n", feu_str, feu_params_col->feu_params[feu].SelfTrig_CmbHitPropFb );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].SelfTrig_CmbHitPropOl != feu_params_col->feu_params[0].SelfTrig_CmbHitPropOl) )
			{
				sprintf( append_str, "Feu %s SelfTrig_CmbHitPropOl %d\n", feu_str, feu_params_col->feu_params[feu].SelfTrig_CmbHitPropOl );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].SelfTrig_DrmHitWid != feu_params_col->feu_params[0].SelfTrig_DrmHitWid) )
			{
				sprintf( append_str, "Feu %s SelfTrig_DrmHitWid %d\n", feu_str, feu_params_col->feu_params[feu].SelfTrig_DrmHitWid );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].SelfTrig_CmbHitWid != feu_params_col->feu_params[feu].SelfTrig_CmbHitWid) )
			{
				sprintf( append_str, "Feu %s SelfTrig_CmbHitWid %d\n", feu_str, feu_params_col->feu_params[feu].SelfTrig_CmbHitWid );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].SelfTrig_Veto != feu_params_col->feu_params[feu].SelfTrig_Veto) )
			{
				sprintf( append_str, "Feu %s SelfTrig_Veto %d\n", feu_str, feu_params_col->feu_params[feu].SelfTrig_Veto );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].SelfTrig_TrigTopo != feu_params_col->feu_params[feu].SelfTrig_TrigTopo) )
			{
				sprintf( append_str, "Feu %s SelfTrig_TrigTopo %d\n", feu_str, feu_params_col->feu_params[feu].SelfTrig_TrigTopo );
				strcat( buf, append_str);
			}
//fprintf( stderr, "%s: Self OK\n", __FUNCTION__ );
			strcat( buf, "# Self Trigger topology parameters\n" );
			for( reg=0; reg<D_FeuPar_SelfTrigTopo_Size; )
			{
				if
				( 
					(feu == 0)
					|| 
					(
						feu_params_col->feu_params[feu].SelfTrig_Topology[reg+0] != feu_params_col->feu_params[0].SelfTrig_Topology[reg+0]
						||
						feu_params_col->feu_params[feu].SelfTrig_Topology[reg+1] != feu_params_col->feu_params[0].SelfTrig_Topology[reg+1]
						||
						feu_params_col->feu_params[feu].SelfTrig_Topology[reg+2] != feu_params_col->feu_params[0].SelfTrig_Topology[reg+2]
						||
						feu_params_col->feu_params[feu].SelfTrig_Topology[reg+3] != feu_params_col->feu_params[0].SelfTrig_Topology[reg+3]
					)
				)
				{
					sprintf
					(
						append_str,
						"Feu %s SelfTrig_Topology %2d 0x%08x 0x%08x 0x%08x 0x%08x\n",
						feu_str,
						reg,
						feu_params_col->feu_params[feu].SelfTrig_Topology[reg+0],
						feu_params_col->feu_params[feu].SelfTrig_Topology[reg+1],
						feu_params_col->feu_params[feu].SelfTrig_Topology[reg+2],
						feu_params_col->feu_params[feu].SelfTrig_Topology[reg+3]
					);
					strcat( buf, append_str);
				}
				reg += 4;
			}
//fprintf( stderr, "%s: Topo OK\n", __FUNCTION__ );

			// FEU Communication parameters
			strcat( buf, "# FEU Communication parameters\n" );
			if( (feu == 0) || (feu_params_col->feu_params[feu].UdpChan_Enable != feu_params_col->feu_params[0].UdpChan_Enable) )
			{
				sprintf( append_str, "Feu %s UdpChan_Enable %d\n", feu_str, feu_params_col->feu_params[feu].UdpChan_Enable );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].UdpChan_MultiPackEnb != feu_params_col->feu_params[0].UdpChan_MultiPackEnb) )
			{
				sprintf( append_str, "Feu %s UdpChan_MultiPackEnb %d\n", feu_str, feu_params_col->feu_params[feu].UdpChan_MultiPackEnb );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].UdpChan_MultiPackThr != feu_params_col->feu_params[0].UdpChan_MultiPackThr) )
			{
				sprintf( append_str, "Feu %s UdpChan_MultiPackThr %d\n", feu_str, feu_params_col->feu_params[feu].UdpChan_MultiPackThr );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].ComChan_Enable != feu_params_col->feu_params[0].ComChan_Enable) )
			{
				sprintf( append_str, "Feu %s ComChan_Enable %d\n", feu_str, feu_params_col->feu_params[feu].ComChan_Enable );
				strcat( buf, append_str);
			}
			if( feu_params_col->feu_params[feu].NetChan_Ip[0] != '\0' )
			{
				sprintf( append_str, "Feu %s NetChan_Ip %s\n", feu_str, feu_params_col->feu_params[feu].NetChan_Ip );
				strcat( buf, append_str);
			}
//fprintf( stderr, "%s: Com OK\n", __FUNCTION__ );

			// Dream clock parameters
			strcat( buf, "# Dream Clock Parameters\n" );
			if( (feu == 0) || (feu_params_col->feu_params[feu].dream_clk_params.RdClk_Div != feu_params_col->feu_params[0].dream_clk_params.RdClk_Div) )
			{
				sprintf( append_str, "Feu %s DrmClk RdClk_Div %3.1f\n", feu_str, feu_params_col->feu_params[feu].dream_clk_params.RdClk_Div );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].dream_clk_params.WrClk_Div != feu_params_col->feu_params[0].dream_clk_params.WrClk_Div) )
			{
				sprintf( append_str, "Feu %s DrmClk WrClk_Div %3.1f\n", feu_str, feu_params_col->feu_params[feu].dream_clk_params.WrClk_Div );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].dream_clk_params.WrClk_Phase != feu_params_col->feu_params[0].dream_clk_params.WrClk_Phase) )
			{
				sprintf( append_str, "Feu %s DrmClk WrClk_Phase %d\n", feu_str, feu_params_col->feu_params[feu].dream_clk_params.WrClk_Phase );
				strcat( buf, append_str);
			}
			if( (feu == 0) || (feu_params_col->feu_params[feu].dream_clk_params.AdcClk_Phase != feu_params_col->feu_params[0].dream_clk_params.AdcClk_Phase) )
			{
				sprintf( append_str, "Feu %s DrmClk AdcClk_Phase %d\n", feu_str, feu_params_col->feu_params[feu].dream_clk_params.AdcClk_Phase );
				strcat( buf, append_str);
			}
//fprintf( stderr, "%s: DrmClk OK\n", __FUNCTION__ );

			// Adc parameters
			strcat( buf, "# Adc parameters\n" );
			for( reg=0; reg<D_AdcPar_NumOfRegs; reg++ )
			{
				if
				(
					((feu == 0) && (feu_params_col->feu_params[0].adc_params.adc_reg[reg].flg != ((char)AdcRegFlag_Unset)))
					||
					(feu_params_col->feu_params[feu].adc_params.adc_reg[reg].flg != feu_params_col->feu_params[0].adc_params.adc_reg[reg].flg)
				)
				{
					sprintf
					(
						append_str,
						"Feu %s Adc 0x%02x 0x%02x %d \n",
						feu_str,
						reg,
						feu_params_col->feu_params[feu].adc_params.adc_reg[reg].val,
						feu_params_col->feu_params[feu].adc_params.adc_reg[reg].flg
					);
					strcat( buf, append_str);
				}
			}
//fprintf( stderr, "%s: feu=%d Adc OK\n", __FUNCTION__, feu );

			// Ee Prom values
			strcat( buf, "# EeProm Values\n" );
			for( reg=0; reg<D_FeuPar_EeProm_Size; reg++ )
			{
				if
				(
					((feu == 0) && (feu_params_col->feu_params[0].ee_prom[reg] >= 0))
					||
					(feu_params_col->feu_params[feu].ee_prom[reg] != feu_params_col->feu_params[0].ee_prom[reg])
				)
				{
					sprintf
					(
						append_str,
						"Feu %s EeProm 0x%02x 0x%02x\n",
						feu_str,
						reg,
						feu_params_col->feu_params[feu].ee_prom[reg]
					);
					strcat( buf, append_str);
				}
			}
//fprintf( stderr, "%s: feu=%d Eeprom OK\n", __FUNCTION__, feu );

			// Max16031 values
			strcat( buf, "# Max16031 Values\n" );
			for( reg=0; reg<D_FeuPar_Max16031_Size; reg++ )
			{
				if
				(
					((feu == 0) && (feu_params_col->feu_params[0].max16031[reg] >= 0))
					||
					(feu_params_col->feu_params[feu].max16031[reg] != feu_params_col->feu_params[0].max16031[reg])
				)
				{
					sprintf
					(
						append_str,
						"Feu %s Max16031 0x%02x 0x%02x\n",
						feu_str,
						reg,
						feu_params_col->feu_params[feu].max16031[reg]
					);
					strcat( buf, append_str);
				}
			}

			// Dream parameters
			for( dream=0; dream<D_FeuPar_NumOfDreams; dream++ )
			{
				if( dream == DEF_MAX_NB_OF_DREAM )
					sprintf( dream_str, "%s", "*" );
				else
					sprintf( dream_str, "%1d", dream );
				sprintf( append_str, "# Dream %s parameters\n", dream_str );
				strcat( buf, append_str);
				for( reg=1; reg<D_DreamPar_NumOfRegs; reg++ )
				{
//if( feu==6 && dream==7 && reg==6)
//fprintf(stdout, "%s: feu=%d dream=%d reg=%d val[0]=%d\n", __FUNCTION__, feu, dream, reg, feu_params_col->feu_params[feu].dream_params[dream].dream_reg[reg].reg[0]);
//if( feu==6 && dream==7 && reg==6)
//fprintf(stdout, "%s: feu=%d dream=%d reg=%d val[0]=%d\n", __FUNCTION__, 0, DEF_MAX_NB_OF_DREAM, reg, feu_params_col->feu_params[0].dream_params[DEF_MAX_NB_OF_DREAM].dream_reg[reg].reg[0]);
					if
					(
						(
							(
								// default feu default Dream: if set print it
								(feu == 0)
								&&
								(dream == DEF_MAX_NB_OF_DREAM)
							)
							||
							(
								// default feu but active Dream: if set it will overwrite precedent
								(feu == 0)
								&&
								(dream != DEF_MAX_NB_OF_DREAM)
								&& 
								(feu_params_col->feu_params[0].dream_params[dream].dream_reg[reg].reg[0] != feu_params_col->feu_params[0].dream_params[DEF_MAX_NB_OF_DREAM].dream_reg[reg].reg[0])
							)
							||
							(
								// Active feu default Dream: if set it will overwrite two precedents
								(feu != 0)
								&&
								(dream == DEF_MAX_NB_OF_DREAM)
								&&
								(feu_params_col->feu_params[feu].dream_params[DEF_MAX_NB_OF_DREAM].dream_reg[reg].reg[0] != feu_params_col->feu_params[0].dream_params[DEF_MAX_NB_OF_DREAM].dream_reg[reg].reg[0])
							)
							||
							(
								// Active feu active Dream: if set it will overwrite everybody
								(feu != 0)
								&&
								(dream != DEF_MAX_NB_OF_DREAM)
								&&
								(
									(
										(feu_params_col->feu_params[feu].dream_params[dream].dream_reg[reg].reg[0] != feu_params_col->feu_params[feu].dream_params[DEF_MAX_NB_OF_DREAM].dream_reg[reg].reg[0])
										&&
										(feu_params_col->feu_params[feu].dream_params[dream].dream_reg[reg].reg[0] != feu_params_col->feu_params[0].dream_params[dream].dream_reg[reg].reg[0])
										&&
										(feu_params_col->feu_params[feu].dream_params[dream].dream_reg[reg].reg[0] != feu_params_col->feu_params[0].dream_params[DEF_MAX_NB_OF_DREAM].dream_reg[reg].reg[0])
									)
								)
							)
						)
						&&
						(feu_params_col->feu_params[feu].dream_params[dream].dream_reg[reg].reg[0] != -1)
					)
					{
						sprintf
						(
							append_str,
							"Feu %s Dream %s %2d 0x%04x 0x%04x 0x%04x 0x%04x \n",
							feu_str,
							dream_str,
							reg,
							feu_params_col->feu_params[feu].dream_params[dream].dream_reg[reg].reg[0],
							feu_params_col->feu_params[feu].dream_params[dream].dream_reg[reg].reg[1],
							feu_params_col->feu_params[feu].dream_params[dream].dream_reg[reg].reg[2],
							feu_params_col->feu_params[feu].dream_params[dream].dream_reg[reg].reg[3]
						);
						strcat( buf, append_str);
					}
				}
//fprintf( stderr, "%s: feu=%d Dream OK\n", __FUNCTION__, feu );
			} // for( dream=0; dream<D_FeuPar_NumOfDreams; dream++ )
		} // if( feu_params_col->feu_params[feu].Feu_RunCtrl_Id != -1 )
	} // for( feu=0; feu<D_FeuParamsCol_NumOfFeuParams; feu++ )

	return D_RetCode_Sucsess;
}
int FeuParamsCol_Fprintf( FeuParamsCol *feu_params_col, FILE *fptr )
{
	char buf[64*1024];
	int ret;

	if( fptr == (FILE *)NULL )
	{
		fprintf( stderr, "FeuParamsCol_Fprintf: fptr=0\n" );
		return D_RetCode_Err_Null_Pointer;
	}
	buf[0]='\0';
	if( (ret = FeuParamsCol_Sprintf( feu_params_col, buf )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "FeuParamsCol_Fprintf: FeuParamsCol_Sprintf failed with  %d\n", ret );
		return ret;
	}

	if( (ret = fprintf( fptr, "%s", buf ) ) < 0 )
	{
		fprintf( stderr, "FeuParamsCol_Fprintf: fprintf failed with  %d\n", ret );
		fprintf( stderr, "FeuParamsCol_Fprintf: fprintf failed with %s\n", strerror(errno) );
		return ret;
	}

	return D_RetCode_Sucsess;
}


int FeuParamsCol_Parse( FeuParamsCol *feu_params_col, int line_num )
{
	int feu;
	int dream;
	int dream_reg;
	char *end_ptr;

	if( feu_params_col == (FeuParamsCol *)NULL )
	{
		fprintf( stderr, "FeuParamsCol_Fread: feu_params_col=Null\n" );
		return D_RetCode_Err_Null_Pointer;
	}

	if( argc > 0 )
	{
		if( ( strcmp( argv[0], "Feu" ) == 0 ) || ( strcmp( argv[0], "MVT_Feu" ) == 0 ) || ( strcmp( argv[0], "FTT_Feu" ) == 0 ) )
		{
			if( strcmp( argv[1], "*" ) == 0 )
			{
//				feu=D_FeuParamsCol_NumOfFeuParams-1;
				feu=0;
//				feu_params_col->feu_params[feu].Feu_RunCtrl_Id = 255;
				feu_params_col->feu_params[feu].Feu_RunCtrl_Id = 0;
			}
			else
			{
				feu=atoi(argv[1]);
				if( (feu<0) || ((D_FeuParamsCol_NumOfFeuParams-1)<feu) )
				{
					fprintf( stderr, "FeuParamsCol_Fread: line=%d wrong feu=%d\n", line_num, feu );
					return D_RetCode_Err_Wrong_Param;
				}
			}
//fprintf( stderr, "FeuParamsCol_Fread: argc=%d argv %s %s %s %s\n", argc, argv[0], argv[1], argv[2], argv[3] );
			// Trigger generator parameters
			if( strcmp( argv[2], "Trig_Conf_Rate" ) == 0 )
			{
				feu_params_col->feu_params[feu].Trig_Conf_Rate=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Trig_Conf_Src" ) == 0 )
			{
				strcpy( feu_params_col->feu_params[feu].Trig_Conf_Src, argv[3] );
			}
			else if( strcmp( argv[2], "Trig_Conf_TrigPipeLen" ) == 0 )
			{
				feu_params_col->feu_params[feu].Trig_Conf_TrigPipeLen=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Trig_Conf_TrigVetoLen" ) == 0 )
			{
				feu_params_col->feu_params[feu].Trig_Conf_TrigVetoLen=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Trig_Conf_File" ) == 0 )
			{
				strcpy( feu_params_col->feu_params[feu].Trig_Conf_File, argv[3] );
			}
			// Auxiliary Trigger Interface parameters
			if( strcmp( argv[2], "TI_Ignore" ) == 0 )
			{
				feu_params_col->feu_params[feu].TI_Ignore=atoi(argv[3]);
			}
			if( strcmp( argv[2], "TI_DcBal_Enc" ) == 0 )
			{
				feu_params_col->feu_params[feu].TI_DcBal_Enc=atoi(argv[3]);
			}
			if( strcmp( argv[2], "TI_DcBal_Dec" ) == 0 )
			{
				feu_params_col->feu_params[feu].TI_DcBal_Dec=atoi(argv[3]);
			}
			if( strcmp( argv[2], "TI_Bert" ) == 0 )
			{
				feu_params_col->feu_params[feu].TI_Bert=atoi(argv[3]);
			}
			// Self Trigger parameters
			if( strcmp( argv[2], "SelfTrig_DreamMask" ) == 0 )
			{
				feu_params_col->feu_params[feu].SelfTrig_DreamMask=strtol( argv[3], &end_ptr, 16 );
			}
			if( strcmp( argv[2], "SelfTrig_Mult" ) == 0 )
			{
				feu_params_col->feu_params[feu].SelfTrig_Mult=atoi(argv[3]);
			}
			if( strcmp( argv[2], "SelfTrig_CmbHitPropFb" ) == 0 )
			{
				feu_params_col->feu_params[feu].SelfTrig_CmbHitPropFb=atoi(argv[3]);
			}
			if( strcmp( argv[2], "SelfTrig_CmbHitPropOl" ) == 0 )
			{
				feu_params_col->feu_params[feu].SelfTrig_CmbHitPropOl=atoi(argv[3]);
			}
			if( strcmp( argv[2], "SelfTrig_DrmHitWid" ) == 0 )
			{
				feu_params_col->feu_params[feu].SelfTrig_DrmHitWid=atoi(argv[3]);
			}
			if( strcmp( argv[2], "SelfTrig_CmbHitWid" ) == 0 )
			{
				feu_params_col->feu_params[feu].SelfTrig_CmbHitWid=atoi(argv[3]);
			}
			if( strcmp( argv[2], "SelfTrig_Veto" ) == 0 )
			{
				feu_params_col->feu_params[feu].SelfTrig_Veto=atoi(argv[3]);
			}
			if( strcmp( argv[2], "SelfTrig_TrigTopo" ) == 0 )
			{
				feu_params_col->feu_params[feu].SelfTrig_TrigTopo=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "SelfTrig_Topology" ) == 0 )
			{
				if( argc != 8 )
				{
					fprintf( stderr, "FeuParamsCol_Fread: line=%d wrong SelfTrig_Topology argc=%d != 8\n", line_num, argc );
					return D_RetCode_Err_Wrong_Param;
				}
				dream_reg = atoi( argv[3] );
				if( (dream_reg<0) || (D_FeuPar_SelfTrigTopo_Size<=dream_reg) || ((dream_reg % 4)!=0) )
				{
					fprintf( stderr, "FeuParamsCol_Fread: line=%d wrong SelfTrig_Topology adr=%d\n", line_num, dream_reg );
					return D_RetCode_Err_Wrong_Param;
				}
				//feu_params_col->feu_params[feu].SelfTrig_Topology[dream_reg]=(unsigned int)strtoll( argv[4], &end_ptr, 16 );
				for( dream=0; dream<4; dream++ )
				{
					sscanf( argv[4+dream], "0x%08x", &(feu_params_col->feu_params[feu].SelfTrig_Topology[dream_reg+dream]) );
				}
			}
			// Main module configuration register parameters
			else if( strcmp( argv[2], "Main_Conf_ClkSel"   ) == 0 )
			{
				strcpy( feu_params_col->feu_params[feu].Main_Conf_ClkSel, argv[3] );
			}
			else if( (strcmp( argv[2], "Main_Conf_DataPipeLen" ) == 0) || (strcmp( argv[2], "Main_Conf_SparseRd" ) == 0) )
			{
				feu_params_col->feu_params[feu].Main_Conf_DataPipeLen=atoi(argv[3]);
			}
			/*
			else if( strcmp( argv[2], "Main_Conf_AdcDtp"   ) == 0 )
			{
				feu_params_col->feu_params[feu].Main_Conf_AdcDtp=atoi(argv[3]);
			}
			*/
			else if( strcmp( argv[2], "Main_Conf_DreamMask"   ) == 0 )
			{
				feu_params_col->feu_params[feu].Main_Conf_DreamMask=strtol( argv[3], &end_ptr, 16 );
			}
			else if( strcmp( argv[2], "Main_Conf_Samples"   ) == 0 )
			{
				feu_params_col->feu_params[feu].Main_Conf_Samples=atoi(argv[3]);
			}
			// Main module trigger register parameters
			else if( strcmp( argv[2], "Main_Trig_TimeStamp"   ) == 0 )
			{
				feu_params_col->feu_params[feu].Main_Trig_TimeStamp=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Main_Trig_OvrWrnLwm"   ) == 0 )
			{
				feu_params_col->feu_params[feu].Main_Trig_OvrWrnLwm=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Main_Trig_OvrWrnHwm"   ) == 0 )
			{
				feu_params_col->feu_params[feu].Main_Trig_OvrWrnHwm=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Main_Trig_OvrThersh"   ) == 0 )
			{
				feu_params_col->feu_params[feu].Main_Trig_OvrThersh=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Main_Trig_LocThrot"   ) == 0 )
			{
				feu_params_col->feu_params[feu].Main_Trig_LocThrot=atoi(argv[3]);
			}
			// FEU power register parameters
			else if( strcmp( argv[2], "Feu_Pwr_Dream"     ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_Pwr_Dream=strtol( argv[3], &end_ptr, 16 );
			}
			else if( strcmp( argv[2], "Feu_Pwr_PrtFlt"     ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_Pwr_PrtFlt=strtol( argv[3], &end_ptr, 16 );
			}
			/*
			else if( strcmp( argv[2], "Feu_Pwr_Adc"     ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_Pwr_Adc=atoi(argv[3]);
			}
			*/
			// FEU run control register parameters
			else if( strcmp( argv[2], "Feu_RunCtrl_Pd" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_RunCtrl_Pd=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Feu_RunCtrl_CM" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_RunCtrl_CM=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Feu_RunCtrl_ZS" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_RunCtrl_ZS=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Feu_RunCtrl_DrOvr" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_RunCtrl_DrOvr=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Feu_RunCtrl_DrDblSmpClk" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_RunCtrl_DrDblSmpClk=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Feu_RunCtrl_EvTstExt" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_RunCtrl_EvTstExt=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Feu_RunCtrl_RdDel" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_RunCtrl_RdDel=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Feu_RunCtrl_ZsChkSmp" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_RunCtrl_ZsChkSmp=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Feu_RunCtrl_CmOffset" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_RunCtrl_CmOffset=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Feu_RunCtrl_Id" ) == 0 )
			{
				if( feu_params_col->feu_params[feu].Feu_RunCtrl_Id == -1 )
					feu_params_col->feu_params[feu].Feu_RunCtrl_Id=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Feu_RunCtrl_AdcDatRdyDel" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_RunCtrl_AdcDatRdyDel=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Feu_RunCtrl_PdFile" ) == 0 )
			{
				strcpy( feu_params_col->feu_params[feu].Feu_RunCtrl_PdFile, argv[3] );
			}
			else if( strcmp( argv[2], "Feu_RunCtrl_ZsFile" ) == 0 )
			{
				strcpy( feu_params_col->feu_params[feu].Feu_RunCtrl_ZsFile, argv[3] );
			}
			// FEU Pulser parameters
			else if( strcmp( argv[2], "Feu_Pulser_BaseVal" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_Pulser_BaseVal=strtol( argv[3], &end_ptr, 16 );
			}
			else if( strcmp( argv[2], "Feu_Pulser_TrigVal" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_Pulser_TrigVal=strtol( argv[3], &end_ptr, 16 );
			}
			else if( strcmp( argv[2], "Feu_Pulser_TestFun" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_Pulser_TestFun=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Feu_Pulser_Enable" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_Pulser_Enable=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Feu_Pulser_DreamTst" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_Pulser_DreamTst=strtol( argv[3], &end_ptr, 16 );
			}
			else if( strcmp( argv[2], "Feu_Pulser_PulseWid" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_Pulser_PulseWid=atoi(argv[3]);
			}
			// FEU PreScale parameters
			else if( strcmp( argv[2], "Feu_PreScale_EvtData" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_PreScale_EvtData=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "Feu_InterPacket_Delay" ) == 0 )
			{
				feu_params_col->feu_params[feu].Feu_InterPacket_Delay=atoi(argv[3]);
			}
			// Communication channels parameters
			else if( strcmp( argv[2], "NetChan_Ip" ) == 0 )
			{
				strcpy( feu_params_col->feu_params[feu].NetChan_Ip, argv[3] );
			}
			else if( strcmp( argv[2], "UdpChan_Enable" ) == 0 )
			{
				feu_params_col->feu_params[feu].UdpChan_Enable=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "UdpChan_MultiPackEnb" ) == 0 )
			{
				feu_params_col->feu_params[feu].UdpChan_MultiPackEnb=atoi(argv[3]);
			}
			else if( strcmp( argv[2], "UdpChan_MultiPackThr" ) == 0 )
			{
				feu_params_col->feu_params[feu].UdpChan_MultiPackThr=atoi(argv[3]);
			}
/*
			else if( strcmp( argv[2], "UdpChan_Delay" ) == 0 )
			{
				feu_params_col->feu_params[feu].UdpChan_Delay=atoi(argv[3]);
			}
*/
			else if( strcmp( argv[2], "ComChan_Enable" ) == 0 )
			{
				feu_params_col->feu_params[feu].ComChan_Enable=atoi(argv[3]);
			}
			// Adc
			else if( strcmp( argv[2], "Adc" ) == 0 )
			{
				if( argc != 6 )
				{
					fprintf( stderr, "FeuParamsCol_Fread: line=%d wrong Adc argc=%d != 6\n", line_num, argc );
					return D_RetCode_Err_Wrong_Param;
				}
				dream_reg = strtol( argv[3], &end_ptr, 16 );
				if( (dream_reg<0) || (D_AdcPar_NumOfRegs<=dream_reg) )
				{
					fprintf( stderr, "FeuParamsCol_Fread: line=%d wrong adc reg=%d\n", line_num, dream_reg );
					return D_RetCode_Err_Wrong_Param;
				}
				feu_params_col->feu_params[feu].adc_params.adc_reg[dream_reg].val=(unsigned char)strtol( argv[4], &end_ptr, 16 );
				feu_params_col->feu_params[feu].adc_params.adc_reg[dream_reg].flg=atoi(argv[5]);
			}
			// EeProm
			else if( strcmp( argv[2], "EeProm" ) == 0 )
			{
				if( argc != 5 )
				{
					fprintf( stderr, "FeuParamsCol_Fread: line=%d wrong EeProm argc=%d != 5\n", line_num, argc );
					return D_RetCode_Err_Wrong_Param;
				}
				dream_reg = strtol( argv[3], &end_ptr, 16 );
				if( (dream_reg<0) || (D_FeuPar_EeProm_Size<=dream_reg) )
				{
					fprintf( stderr, "FeuParamsCol_Fread: line=%d wrong EeProm reg=%d\n", line_num, dream_reg );
					return D_RetCode_Err_Wrong_Param;
				}
				feu_params_col->feu_params[feu].ee_prom[dream_reg]=(short)(strtol( argv[4], &end_ptr, 16 ));
			}
			// Max16031
			else if( strcmp( argv[2], "Max16031" ) == 0 )
			{
				if( argc != 5 )
				{
					fprintf( stderr, "%s: line=%d wrong Max16031 argc=%d != 5\n", __FUNCTION__, line_num, argc );
					return D_RetCode_Err_Wrong_Param;
				}
				dream_reg = strtol( argv[3], &end_ptr, 16 );
				if( (dream_reg<0) || (D_FeuPar_Max16031_Size<=dream_reg) )
				{
					fprintf( stderr, "FeuParamsCol_Fread: line=%d wrong Max16031 reg=%d\n", line_num, dream_reg );
					return D_RetCode_Err_Wrong_Param;
				}
				feu_params_col->feu_params[feu].max16031[dream_reg]=(short)(strtol( argv[4], &end_ptr, 16 ));
			}
			// Dream parameters
			else if( strcmp( argv[2], "Dream" ) == 0 )
			{
				if( argc != 9 )
				{
					fprintf( stderr, "FeuParamsCol_Fread: line=%d wrong Dream argc=%d != 9\n", line_num, argc );
					return D_RetCode_Err_Wrong_Param;
				}
				if( strcmp( argv[3], "*" ) == 0 )
				{
					dream=D_FeuPar_NumOfDreams-1;
				}
				else
				{
					dream=atoi(argv[3]);
					if( (dream<0) || ((D_FeuPar_NumOfDreams-1)<dream) )
					{
						fprintf( stderr, "FeuParamsCol_Fread: line=%d wrong dream=%d\n", line_num, dream );
						return D_RetCode_Err_Wrong_Param;
					}
				}
				dream_reg = atoi( argv[4] );
				if( (dream_reg<0) || (D_DreamPar_NumOfRegs<=dream_reg) )
				{
					fprintf( stderr, "FeuParamsCol_Fread: line=%d wrong dream reg=%d\n", line_num, dream_reg );
					return D_RetCode_Err_Wrong_Param;
				}
				feu_params_col->feu_params[feu].dream_params[dream].dream_reg[dream_reg].reg[0]=strtol( argv[5], &end_ptr, 16 );
				feu_params_col->feu_params[feu].dream_params[dream].dream_reg[dream_reg].reg[1]=strtol( argv[6], &end_ptr, 16 );
				feu_params_col->feu_params[feu].dream_params[dream].dream_reg[dream_reg].reg[2]=strtol( argv[7], &end_ptr, 16 );
				feu_params_col->feu_params[feu].dream_params[dream].dream_reg[dream_reg].reg[3]=strtol( argv[8], &end_ptr, 16 );
//fprintf(stderr, "%s: feu=%d dream=%d reg=%d val[0]=%d\n", __FUNCTION__, feu, dream, dream_reg, feu_params_col->feu_params[feu].dream_params[dream].dream_reg[dream_reg].reg[0]);
//if( (dream==7) && (feu == 6) && (dream_reg==6) )
//fprintf(stderr, "%s: ******************* feu=%d dream=%d reg=%d val[0]=%d\n", __FUNCTION__, feu, dream, dream_reg, feu_params_col->feu_params[feu].dream_params[dream].dream_reg[dream_reg].reg[0]);
			}
			else if( strcmp( argv[2], "DrmClk" ) == 0 )
			{
				if( argc != 5 )
				{
					fprintf( stderr, "%s: line=%d wrong DrmClk argc=%d != 5\n", __FUNCTION__, line_num, argc );
					return D_RetCode_Err_Wrong_Param;
				}
				if( strcmp( argv[3], "RdClk_Div" ) == 0 )
					feu_params_col->feu_params[feu].dream_clk_params.RdClk_Div = atof( argv[4] );
				else if( strcmp( argv[3], "WrClk_Div" ) == 0 )
					feu_params_col->feu_params[feu].dream_clk_params.WrClk_Div = atof( argv[4] );
				else if( strcmp( argv[3], "WrClk_Phase" ) == 0 )
					feu_params_col->feu_params[feu].dream_clk_params.WrClk_Phase = atoi( argv[4] );
				else if( strcmp( argv[3], "AdcClk_Phase" ) == 0 )
					feu_params_col->feu_params[feu].dream_clk_params.AdcClk_Phase = atoi( argv[4] );
				else
				{
					fprintf( stderr, "%s: line=%d wrong DrmClk keyword %s\n", __FUNCTION__, line_num, argv[3] );
					return D_RetCode_Err_Wrong_Param;
				}
			}
		} // if( strcmp( argv[0], "Feu" ) == 0 )
	} // if( argc > 0 )
	return D_RetCode_Sucsess;
}



int FeuParamsCol_Fread( FeuParamsCol *feu_params_col, FILE *fptr )
{
	char line[LINE_SIZE];
	int line_num;
	int ret;

	if( feu_params_col == (FeuParamsCol *)NULL )
	{
		fprintf( stderr, "FeuParamsCol_Fread: feu_params_col=Null\n" );
		return D_RetCode_Err_Null_Pointer;
	}

	if( fptr == (FILE *)NULL )
	{
		fprintf( stderr, "FeuParamsCol_Fread: fptr=Null\n" );
		return D_RetCode_Err_Null_Pointer;
	}

	// Process parameter file
	line_num = 0;
	while( fgets( line, LINE_SIZE, fptr ) != NULL )
	{
//fprintf( stderr, "FeuParamsCol_Fread: line=%s\n", line );
		/* parse the line */
		parse_line(line);
		line_num++;
		ret = FeuParamsCol_Parse( feu_params_col, line_num );
		if( ret != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: FeuParamsCol_Parse failed with %d\n", __FUNCTION__, ret );
			return ret;
		}
	} // while( fgets( line, LINE_SIZE, fptr ) != NULL )
	return D_RetCode_Sucsess;
}

int FeuParamsCol_PropComParams( FeuParamsCol *feu_params_col )
{
	int feu;
	int dream;
	int index;

	// Check input parameter
	if( feu_params_col == (FeuParamsCol *)NULL )
	{
		fprintf( stderr, "FeuParamsCol_PropComParams: Null feu_params_col\n" );
		return D_RetCode_Err_Null_Pointer;
	}

	for( feu=1; feu<D_FeuParamsCol_NumOfFeuParams; feu++ )
	{
		// Main module config parameters
		if( feu_params_col->feu_params[feu].Main_Conf_ClkSel[0]=='\0' )
			sprintf(feu_params_col->feu_params[feu].Main_Conf_ClkSel, "%s", feu_params_col->feu_params[0].Main_Conf_ClkSel);
		if( feu_params_col->feu_params[feu].Main_Conf_DreamMask < 0 )
			feu_params_col->feu_params[feu].Main_Conf_DreamMask = feu_params_col->feu_params[0].Main_Conf_DreamMask;
		if( feu_params_col->feu_params[feu].Main_Conf_Samples < 0 )
			feu_params_col->feu_params[feu].Main_Conf_Samples = feu_params_col->feu_params[0].Main_Conf_Samples;
		feu_params_col->feu_params[feu].Main_Conf_AdcDtp = 0;
		if( feu_params_col->feu_params[feu].Main_Conf_DataPipeLen < 0 )
			feu_params_col->feu_params[feu].Main_Conf_DataPipeLen = feu_params_col->feu_params[0].Main_Conf_DataPipeLen;

		// Dream Clock parameters
		if( feu_params_col->feu_params[feu].dream_clk_params.RdClk_Div == 0.0 )
			feu_params_col->feu_params[feu].dream_clk_params.RdClk_Div = feu_params_col->feu_params[0].dream_clk_params.RdClk_Div;
		if( feu_params_col->feu_params[feu].dream_clk_params.WrClk_Div == 0.0 )
			feu_params_col->feu_params[feu].dream_clk_params.WrClk_Div = feu_params_col->feu_params[0].dream_clk_params.WrClk_Div;
		if( feu_params_col->feu_params[feu].dream_clk_params.WrClk_Phase < 0 )
			feu_params_col->feu_params[feu].dream_clk_params.WrClk_Phase = feu_params_col->feu_params[0].dream_clk_params.WrClk_Phase;
		if( feu_params_col->feu_params[feu].dream_clk_params.AdcClk_Phase < 0 )
			feu_params_col->feu_params[feu].dream_clk_params.AdcClk_Phase = feu_params_col->feu_params[0].dream_clk_params.AdcClk_Phase;
		
		// Main module trigger register
		if( feu_params_col->feu_params[feu].Main_Trig_TimeStamp < 0 )
			feu_params_col->feu_params[feu].Main_Trig_TimeStamp = feu_params_col->feu_params[0].Main_Trig_TimeStamp;
		if( feu_params_col->feu_params[feu].Main_Trig_OvrWrnLwm < 0 )
			feu_params_col->feu_params[feu].Main_Trig_OvrWrnLwm = feu_params_col->feu_params[0].Main_Trig_OvrWrnLwm;
		if( feu_params_col->feu_params[feu].Main_Trig_OvrWrnHwm < 0 )
			feu_params_col->feu_params[feu].Main_Trig_OvrWrnHwm = feu_params_col->feu_params[0].Main_Trig_OvrWrnHwm;
		if( feu_params_col->feu_params[feu].Main_Trig_OvrThersh < 0 )
			feu_params_col->feu_params[feu].Main_Trig_OvrThersh = feu_params_col->feu_params[0].Main_Trig_OvrThersh;
		if( feu_params_col->feu_params[feu].Main_Trig_LocThrot < 0 )
			feu_params_col->feu_params[feu].Main_Trig_LocThrot = feu_params_col->feu_params[0].Main_Trig_LocThrot;

		// FEU PowerUp Register
		if( feu_params_col->feu_params[feu].Feu_Pwr_Dream < 0 )
			feu_params_col->feu_params[feu].Feu_Pwr_Dream = feu_params_col->feu_params[0].Feu_Pwr_Dream;
		if( feu_params_col->feu_params[feu].Feu_Pwr_PrtFlt < 0 )
			feu_params_col->feu_params[feu].Feu_Pwr_PrtFlt = feu_params_col->feu_params[0].Feu_Pwr_PrtFlt;
		feu_params_col->feu_params[feu].Feu_Pwr_Adc = 0;

		// FEU Run Control parameters
		if( feu_params_col->feu_params[feu].Feu_RunCtrl_Pd < 0 )
			feu_params_col->feu_params[feu].Feu_RunCtrl_Pd = feu_params_col->feu_params[0].Feu_RunCtrl_Pd;
		if( feu_params_col->feu_params[feu].Feu_RunCtrl_CM < 0 )
			feu_params_col->feu_params[feu].Feu_RunCtrl_CM = feu_params_col->feu_params[0].Feu_RunCtrl_CM;
		if( feu_params_col->feu_params[feu].Feu_RunCtrl_ZS < 0 )
			feu_params_col->feu_params[feu].Feu_RunCtrl_ZS = feu_params_col->feu_params[0].Feu_RunCtrl_ZS;
		if( feu_params_col->feu_params[feu].Feu_RunCtrl_DrOvr < 0 )
			feu_params_col->feu_params[feu].Feu_RunCtrl_DrOvr = feu_params_col->feu_params[0].Feu_RunCtrl_DrOvr;
		if( feu_params_col->feu_params[feu].Feu_RunCtrl_DrDblSmpClk < 0 )
			feu_params_col->feu_params[feu].Feu_RunCtrl_DrDblSmpClk = feu_params_col->feu_params[0].Feu_RunCtrl_DrDblSmpClk;
		if( feu_params_col->feu_params[feu].Feu_RunCtrl_ZsChkSmp < 0 )
			feu_params_col->feu_params[feu].Feu_RunCtrl_ZsChkSmp = feu_params_col->feu_params[0].Feu_RunCtrl_ZsChkSmp;
//		if( feu_params_col->feu_params[feu].Feu_RunCtrl_Id < 0 )
//			feu_params_col->feu_params[feu].Feu_RunCtrl_Id = feu_params_col->feu_params[0].Feu_RunCtrl_Id;
		if( feu_params_col->feu_params[feu].Feu_RunCtrl_AdcDatRdyDel < 0 )
			feu_params_col->feu_params[feu].Feu_RunCtrl_AdcDatRdyDel = feu_params_col->feu_params[0].Feu_RunCtrl_AdcDatRdyDel;
		if( feu_params_col->feu_params[feu].Feu_RunCtrl_EvTstExt < 0 )
			feu_params_col->feu_params[feu].Feu_RunCtrl_EvTstExt = feu_params_col->feu_params[0].Feu_RunCtrl_EvTstExt;
		if( feu_params_col->feu_params[feu].Feu_RunCtrl_RdDel < 0 )
			feu_params_col->feu_params[feu].Feu_RunCtrl_RdDel = feu_params_col->feu_params[0].Feu_RunCtrl_RdDel;
		if( feu_params_col->feu_params[feu].Feu_RunCtrl_CmOffset < 0 )
			feu_params_col->feu_params[feu].Feu_RunCtrl_CmOffset = feu_params_col->feu_params[0].Feu_RunCtrl_CmOffset;
		// Make sure the ZS window is not bigger than number of samples
		// ZS window: sample 0 never compared to threshold
		// 0 compare samples 1 through 2  min nb of samples 4
		// 1                 1 through 3                    5
		// 2                 1 through 4                    6
		// 3                 1 through 5                    7
                // 4                 1 through 6                    8
		//
		if( feu_params_col->feu_params[feu].Feu_RunCtrl_ZS == 1 )
		{
			if( feu_params_col->feu_params[feu].Feu_RunCtrl_ZsChkSmp > (feu_params_col->feu_params[feu].Main_Conf_Samples-4) )
				feu_params_col->feu_params[feu].Feu_RunCtrl_ZsChkSmp = feu_params_col->feu_params[feu].Main_Conf_Samples-4;
		}

		// FEU Pedestal memory
		if( feu_params_col->feu_params[feu].Feu_RunCtrl_PdFile[0]=='\0' )
			sprintf(feu_params_col->feu_params[feu].Feu_RunCtrl_PdFile, "%s", feu_params_col->feu_params[0].Feu_RunCtrl_PdFile);
		// FEU Threshold memory
		if( feu_params_col->feu_params[feu].Feu_RunCtrl_ZsFile[0]=='\0' )
			sprintf(feu_params_col->feu_params[feu].Feu_RunCtrl_ZsFile, "%s", feu_params_col->feu_params[0].Feu_RunCtrl_ZsFile);

		// FEU Pulser Register
		if( feu_params_col->feu_params[feu].Feu_Pulser_DreamTst < 0 )
			feu_params_col->feu_params[feu].Feu_Pulser_DreamTst = feu_params_col->feu_params[0].Feu_Pulser_DreamTst;
		if( feu_params_col->feu_params[feu].Feu_Pulser_PulseWid < 0 )
			feu_params_col->feu_params[feu].Feu_Pulser_PulseWid = feu_params_col->feu_params[0].Feu_Pulser_PulseWid;
		if( feu_params_col->feu_params[feu].Feu_Pulser_Enable < 0 )
			feu_params_col->feu_params[feu].Feu_Pulser_Enable = feu_params_col->feu_params[0].Feu_Pulser_Enable;

		// FEU Prescale Register
		if( feu_params_col->feu_params[feu].Feu_PreScale_EvtData < 0 )
			feu_params_col->feu_params[feu].Feu_PreScale_EvtData = feu_params_col->feu_params[0].Feu_PreScale_EvtData;
		if( feu_params_col->feu_params[feu].Feu_InterPacket_Delay < 0 )
			feu_params_col->feu_params[feu].Feu_InterPacket_Delay = feu_params_col->feu_params[0].Feu_InterPacket_Delay;

		// Trigger generator parameters
		if( feu_params_col->feu_params[feu].Trig_Conf_Rate < 0 )
			feu_params_col->feu_params[feu].Trig_Conf_Rate = feu_params_col->feu_params[0].Trig_Conf_Rate;
		if( feu_params_col->feu_params[feu].Trig_Conf_Src[0]=='\0' )
			sprintf(feu_params_col->feu_params[feu].Trig_Conf_Src, "%s", feu_params_col->feu_params[0].Trig_Conf_Src);
		if( feu_params_col->feu_params[feu].Trig_Conf_TrigPipeLen < 0 )
			feu_params_col->feu_params[feu].Trig_Conf_TrigPipeLen = feu_params_col->feu_params[0].Trig_Conf_TrigPipeLen;
		if( feu_params_col->feu_params[feu].Trig_Conf_TrigVetoLen < 0 )
			feu_params_col->feu_params[feu].Trig_Conf_TrigVetoLen = feu_params_col->feu_params[0].Trig_Conf_TrigVetoLen;
		if( feu_params_col->feu_params[feu].Trig_Conf_File[0]=='\0' )
			sprintf(feu_params_col->feu_params[feu].Trig_Conf_File, "%s", feu_params_col->feu_params[0].Trig_Conf_File);

		// Auxiliary Trigger Interface parameters
		if( feu_params_col->feu_params[feu].TI_Ignore < 0 )
			feu_params_col->feu_params[feu].TI_Ignore = feu_params_col->feu_params[0].TI_Ignore;
		if( feu_params_col->feu_params[feu].TI_DcBal_Enc < 0 )
			feu_params_col->feu_params[feu].TI_DcBal_Enc = feu_params_col->feu_params[0].TI_DcBal_Enc;
		if( feu_params_col->feu_params[feu].TI_DcBal_Dec < 0 )
			feu_params_col->feu_params[feu].TI_DcBal_Dec = feu_params_col->feu_params[0].TI_DcBal_Dec;
		if( feu_params_col->feu_params[feu].TI_Bert < 0 )
			feu_params_col->feu_params[feu].TI_Bert = feu_params_col->feu_params[0].TI_Bert;

		// Self-Trigger parameters
		if( feu_params_col->feu_params[feu].SelfTrig_DreamMask < 0 )
			feu_params_col->feu_params[feu].SelfTrig_DreamMask = feu_params_col->feu_params[0].SelfTrig_DreamMask;
		if( feu_params_col->feu_params[feu].SelfTrig_Mult < 0 )
			feu_params_col->feu_params[feu].SelfTrig_Mult = feu_params_col->feu_params[0].SelfTrig_Mult;
		if( feu_params_col->feu_params[feu].SelfTrig_CmbHitPropFb < 0 )
			feu_params_col->feu_params[feu].SelfTrig_CmbHitPropFb = feu_params_col->feu_params[0].SelfTrig_CmbHitPropFb;
		if( feu_params_col->feu_params[feu].SelfTrig_CmbHitPropOl < 0 )
			feu_params_col->feu_params[feu].SelfTrig_CmbHitPropOl = feu_params_col->feu_params[0].SelfTrig_CmbHitPropOl;
		if( feu_params_col->feu_params[feu].SelfTrig_DrmHitWid < 0 )
			feu_params_col->feu_params[feu].SelfTrig_DrmHitWid = feu_params_col->feu_params[0].SelfTrig_DrmHitWid;
		if( feu_params_col->feu_params[feu].SelfTrig_CmbHitWid < 0 )
			feu_params_col->feu_params[feu].SelfTrig_CmbHitWid = feu_params_col->feu_params[0].SelfTrig_CmbHitWid;
		if( feu_params_col->feu_params[feu].SelfTrig_Veto < 0 )
			feu_params_col->feu_params[feu].SelfTrig_Veto = feu_params_col->feu_params[0].SelfTrig_Veto;
		if( feu_params_col->feu_params[feu].SelfTrig_TrigTopo < 0 )
			feu_params_col->feu_params[feu].SelfTrig_TrigTopo = feu_params_col->feu_params[0].SelfTrig_TrigTopo;
		// Topology parameters
		for( index=0; index<D_FeuPar_SelfTrigTopo_Size; index++ )
		{
			if( feu_params_col->feu_params[feu].SelfTrig_Topology[index] == 0 )
				feu_params_col->feu_params[feu].SelfTrig_Topology[index] = feu_params_col->feu_params[0].SelfTrig_Topology[index];
		}

		// Network Interface parameters
		if( feu_params_col->feu_params[feu].UdpChan_Enable < 0 )
			feu_params_col->feu_params[feu].UdpChan_Enable = feu_params_col->feu_params[0].UdpChan_Enable;
/*
		if( feu_params_col->feu_params[feu].UdpChan_Delay < 0 )
			feu_params_col->feu_params[feu].UdpChan_Delay = feu_params_col->feu_params[0].UdpChan_Delay;
*/
		feu_params_col->feu_params[feu].UdpChan_Delay = 0;
		if( feu_params_col->feu_params[feu].UdpChan_MultiPackEnb < 0 )
			feu_params_col->feu_params[feu].UdpChan_MultiPackEnb = feu_params_col->feu_params[0].UdpChan_MultiPackEnb;
		if( feu_params_col->feu_params[feu].UdpChan_MultiPackThr < 0 )
			feu_params_col->feu_params[feu].UdpChan_MultiPackThr = feu_params_col->feu_params[0].UdpChan_MultiPackThr;
		if( feu_params_col->feu_params[feu].ComChan_Enable < 0 )
			feu_params_col->feu_params[feu].ComChan_Enable = feu_params_col->feu_params[0].ComChan_Enable;

		// Dream parameters
		for( dream=0; dream<D_FeuPar_NumOfDreams-1; dream++ )
		{
			for( index=1; index<D_DreamPar_NumOfRegs; index++ )
			{
				if( feu_params_col->feu_params[feu].dream_params[dream].dream_reg[index].reg[0] < 0 )
				{
					// Check if the default value of this particular FEU is set
					if( feu_params_col->feu_params[feu].dream_params[D_FeuPar_NumOfDreams-1].dream_reg[index].reg[0] >= 0 )
					{
						feu_params_col->feu_params[feu].dream_params[dream].dream_reg[index].reg[0]
						=
						feu_params_col->feu_params[feu].dream_params[D_FeuPar_NumOfDreams-1].dream_reg[index].reg[0];
						feu_params_col->feu_params[feu].dream_params[dream].dream_reg[index].reg[1]
						=
						feu_params_col->feu_params[feu].dream_params[D_FeuPar_NumOfDreams-1].dream_reg[index].reg[1];
						feu_params_col->feu_params[feu].dream_params[dream].dream_reg[index].reg[2]
						=
						feu_params_col->feu_params[feu].dream_params[D_FeuPar_NumOfDreams-1].dream_reg[index].reg[2];
						feu_params_col->feu_params[feu].dream_params[dream].dream_reg[index].reg[3]
						=
						feu_params_col->feu_params[feu].dream_params[D_FeuPar_NumOfDreams-1].dream_reg[index].reg[3];
					}
					else
					{
						// Looks like information has to be found in the default FEU
						if( feu_params_col->feu_params[0].dream_params[dream].dream_reg[index].reg[0] >= 0 )
						{
							feu_params_col->feu_params[feu].dream_params[dream].dream_reg[index].reg[0]
							=
							feu_params_col->feu_params[0].dream_params[dream].dream_reg[index].reg[0];
							feu_params_col->feu_params[feu].dream_params[dream].dream_reg[index].reg[1]
							=
							feu_params_col->feu_params[0].dream_params[dream].dream_reg[index].reg[1];
							feu_params_col->feu_params[feu].dream_params[dream].dream_reg[index].reg[2]
							=
							feu_params_col->feu_params[0].dream_params[dream].dream_reg[index].reg[2];
							feu_params_col->feu_params[feu].dream_params[dream].dream_reg[index].reg[3]
							=
							feu_params_col->feu_params[0].dream_params[dream].dream_reg[index].reg[3];
						}
						else
						{
							// Looks like information has to be found in the default Dream of the default FEU
							feu_params_col->feu_params[feu].dream_params[dream].dream_reg[index].reg[0]
							=
							feu_params_col->feu_params[0].dream_params[D_FeuPar_NumOfDreams-1].dream_reg[index].reg[0];
							feu_params_col->feu_params[feu].dream_params[dream].dream_reg[index].reg[1]
							=
							feu_params_col->feu_params[0].dream_params[D_FeuPar_NumOfDreams-1].dream_reg[index].reg[1];
							feu_params_col->feu_params[feu].dream_params[dream].dream_reg[index].reg[2]
							=
							feu_params_col->feu_params[0].dream_params[D_FeuPar_NumOfDreams-1].dream_reg[index].reg[2];
							feu_params_col->feu_params[feu].dream_params[dream].dream_reg[index].reg[3]
							=
							feu_params_col->feu_params[0].dream_params[D_FeuPar_NumOfDreams-1].dream_reg[index].reg[3];
						}
					}
				} // if( feu_params_col->feu_params[feu].dream_params[dream].dream_reg[index].reg[0] < 0 )
			} // for( index=1; index<D_DreamPar_NumOfRegs; index++ )
		} // for( dream=0; dream<D_FeuPar_NumOfDreams-1; dream++ )
		
		// Adc parameters
		for( index=0; index<D_AdcPar_NumOfRegs; index++ )
		{
			if( feu_params_col->feu_params[feu].adc_params.adc_reg[index].flg == ((char)AdcRegFlag_Unset) )
			{
				feu_params_col->feu_params[feu].adc_params.adc_reg[index].flg = feu_params_col->feu_params[0].adc_params.adc_reg[index].flg;
				feu_params_col->feu_params[feu].adc_params.adc_reg[index].val = feu_params_col->feu_params[0].adc_params.adc_reg[index].val;
			}
		}

		// Max16031 parameters
		for( index=0; index<D_FeuPar_Max16031_Size; index++ )
		{
			if( feu_params_col->feu_params[feu].max16031[index] < 0 )
				feu_params_col->feu_params[feu].max16031[index] = feu_params_col->feu_params[0].max16031[index];
		}

	} // for( feu=1; feu<D_FeuParamsCol_NumOfFeuParams; feu++ )

	return D_RetCode_Sucsess;
}

/* Old version
int FeuParamsCol_PropComParams( FeuParamsCol *feu_params_col )
{
	int feu;
	int index;

	// Check input parameter
	if( feu_params_col == (FeuParamsCol *)NULL )
	{
		fprintf( stderr, "FeuParamsCol_PropComParams: Null feu_params_col\n" );
		return D_RetCode_Err_Null_Pointer;
	}

	for( feu=1; feu<D_FeuParamsCol_NumOfFeuParams; feu++ )
	{
		// Main module config parameters
		if
		(
			( feu_params_col->feu_params[feu].Main_Conf_ClkSel[0] != '\0' )
			||
			( feu_params_col->feu_params[feu].Main_Conf_DreamMask >= 0 )
			||
			( feu_params_col->feu_params[feu].Main_Conf_Samples >= 0 )
		)
		{
			if( feu_params_col->feu_params[feu].Main_Conf_ClkSel[0]=='\0' )
				sprintf(feu_params_col->feu_params[feu].Main_Conf_ClkSel, "%s", feu_params_col->feu_params[0].Main_Conf_ClkSel);
			if( feu_params_col->feu_params[feu].Main_Conf_DreamMask < 0 )
				feu_params_col->feu_params[feu].Main_Conf_DreamMask = feu_params_col->feu_params[0].Main_Conf_DreamMask;
			if( feu_params_col->feu_params[feu].Main_Conf_Samples < 0 )
				feu_params_col->feu_params[feu].Main_Conf_Samples = feu_params_col->feu_params[0].Main_Conf_Samples;
			feu_params_col->feu_params[feu].Main_Conf_AdcDtp = 0;
			feu_params_col->feu_params[feu].Main_Conf_DataPipeLen = 0;
		}


		// Main module trigger register
		if
		(
			( feu_params_col->feu_params[feu].Main_Trig_TimeStamp >= 0 )
			||
			( feu_params_col->feu_params[feu].Main_Trig_OvrWrnLwm >= 0 )
			||
			( feu_params_col->feu_params[feu].Main_Trig_OvrWrnHwm >= 0 )
			||
			( feu_params_col->feu_params[feu].Main_Trig_OvrThersh >= 0 )
			||
			( feu_params_col->feu_params[feu].Main_Trig_LocThrot >= 0 )
		)
		{
			if( feu_params_col->feu_params[feu].Main_Trig_TimeStamp < 0 )
				feu_params_col->feu_params[feu].Main_Trig_TimeStamp = feu_params_col->feu_params[0].Main_Trig_TimeStamp;
			if( feu_params_col->feu_params[feu].Main_Trig_OvrWrnLwm < 0 )
				feu_params_col->feu_params[feu].Main_Trig_OvrWrnLwm = feu_params_col->feu_params[0].Main_Trig_OvrWrnLwm;
			if( feu_params_col->feu_params[feu].Main_Trig_OvrWrnHwm < 0 )
				feu_params_col->feu_params[feu].Main_Trig_OvrWrnHwm = feu_params_col->feu_params[0].Main_Trig_OvrWrnHwm;
			if( feu_params_col->feu_params[feu].Main_Trig_OvrThersh < 0 )
				feu_params_col->feu_params[feu].Main_Trig_OvrThersh = feu_params_col->feu_params[0].Main_Trig_OvrThersh;
			if( feu_params_col->feu_params[feu].Main_Trig_LocThrot < 0 )
				feu_params_col->feu_params[feu].Main_Trig_LocThrot = feu_params_col->feu_params[0].Main_Trig_LocThrot;
		}

		// FEU PowerUp Register
		if
		(
			( feu_params_col->feu_params[feu].Feu_Pwr_Dream >= 0 )
			||
			( feu_params_col->feu_params[feu].Feu_Pwr_PrtFlt >= 0 )
		)
		{
			if( feu_params_col->feu_params[feu].Feu_Pwr_Dream < 0 )
				feu_params_col->feu_params[feu].Feu_Pwr_Dream = feu_params_col->feu_params[0].Feu_Pwr_Dream;
			if( feu_params_col->feu_params[feu].Feu_Pwr_PrtFlt < 0 )
				feu_params_col->feu_params[feu].Feu_Pwr_PrtFlt = feu_params_col->feu_params[0].Feu_Pwr_PrtFlt;
			feu_params_col->feu_params[feu].Feu_Pwr_Adc = 0;
		}

		// FEU Run Control parameters
		if
		(
			( feu_params_col->feu_params[feu].Feu_RunCtrl_Pd >= 0 )
			||
			( feu_params_col->feu_params[feu].Feu_RunCtrl_CM >= 0 )
			||
			( feu_params_col->feu_params[feu].Feu_RunCtrl_ZS >= 0 )
			||
			( feu_params_col->feu_params[feu].Feu_RunCtrl_DrOvr >= 0 )
			||
			( feu_params_col->feu_params[feu].Feu_RunCtrl_DrDblSmpClk >= 0 )
			||
			( feu_params_col->feu_params[feu].Feu_RunCtrl_EvTstExt >= 0 )
			||
			( feu_params_col->feu_params[feu].Feu_RunCtrl_RdDel >= 0 )
			||
			( feu_params_col->feu_params[feu].Feu_RunCtrl_ZsChkSmp >= 0 )
			||
			( feu_params_col->feu_params[feu].Feu_RunCtrl_CmOffset >= 0 )
			||
			( feu_params_col->feu_params[feu].Feu_RunCtrl_AdcDatRdyDel >= 0 )
		)
		{
			if( feu_params_col->feu_params[feu].Feu_RunCtrl_Pd < 0 )
				feu_params_col->feu_params[feu].Feu_RunCtrl_Pd = feu_params_col->feu_params[0].Feu_RunCtrl_Pd;
			if( feu_params_col->feu_params[feu].Feu_RunCtrl_CM < 0 )
				feu_params_col->feu_params[feu].Feu_RunCtrl_CM = feu_params_col->feu_params[0].Feu_RunCtrl_CM;
			if( feu_params_col->feu_params[feu].Feu_RunCtrl_ZS < 0 )
				feu_params_col->feu_params[feu].Feu_RunCtrl_ZS = feu_params_col->feu_params[0].Feu_RunCtrl_ZS;
			if( feu_params_col->feu_params[feu].Feu_RunCtrl_DrOvr < 0 )
				feu_params_col->feu_params[feu].Feu_RunCtrl_DrOvr = feu_params_col->feu_params[0].Feu_RunCtrl_DrOvr;
			if( feu_params_col->feu_params[feu].Feu_RunCtrl_DrDblSmpClk < 0 )
				feu_params_col->feu_params[feu].Feu_RunCtrl_DrDblSmpClk = feu_params_col->feu_params[0].Feu_RunCtrl_DrDblSmpClk;
			if( feu_params_col->feu_params[feu].Feu_RunCtrl_ZsChkSmp < 0 )
				feu_params_col->feu_params[feu].Feu_RunCtrl_ZsChkSmp = feu_params_col->feu_params[0].Feu_RunCtrl_ZsChkSmp;
			if( feu_params_col->feu_params[feu].Feu_RunCtrl_Id < 0 )
				feu_params_col->feu_params[feu].Feu_RunCtrl_Id = feu_params_col->feu_params[0].Feu_RunCtrl_Id;
			if( feu_params_col->feu_params[feu].Feu_RunCtrl_AdcDatRdyDel < 0 )
				feu_params_col->feu_params[feu].Feu_RunCtrl_AdcDatRdyDel = feu_params_col->feu_params[0].Feu_RunCtrl_AdcDatRdyDel;
			if( feu_params_col->feu_params[feu].Feu_RunCtrl_EvTstExt < 0 )
				feu_params_col->feu_params[feu].Feu_RunCtrl_EvTstExt = feu_params_col->feu_params[0].Feu_RunCtrl_EvTstExt;
			if( feu_params_col->feu_params[feu].Feu_RunCtrl_RdDel < 0 )
				feu_params_col->feu_params[feu].Feu_RunCtrl_RdDel = feu_params_col->feu_params[0].Feu_RunCtrl_RdDel;
			if( feu_params_col->feu_params[feu].Feu_RunCtrl_CmOffset < 0 )
				feu_params_col->feu_params[feu].Feu_RunCtrl_CmOffset = feu_params_col->feu_params[0].Feu_RunCtrl_CmOffset;
		}

		// FEU Pulser Register
		if
		(
			( feu_params_col->feu_params[feu].Feu_Pulser_DreamTst >= 0 )
			||
			( feu_params_col->feu_params[feu].Feu_Pulser_PulseWid >= 0 )
			||
			( feu_params_col->feu_params[feu].Feu_Pulser_Enable >= 0 )
		)
		{
			if( feu_params_col->feu_params[feu].Feu_Pulser_DreamTst < 0 )
				feu_params_col->feu_params[feu].Feu_Pulser_DreamTst = feu_params_col->feu_params[0].Feu_Pulser_DreamTst;
			if( feu_params_col->feu_params[feu].Feu_Pulser_PulseWid < 0 )
				feu_params_col->feu_params[feu].Feu_Pulser_PulseWid = feu_params_col->feu_params[0].Feu_Pulser_PulseWid;
			if( feu_params_col->feu_params[feu].Feu_Pulser_Enable < 0 )
				feu_params_col->feu_params[feu].Feu_Pulser_Enable = feu_params_col->feu_params[0].Feu_Pulser_Enable;
		}

		// Trigger generator parameters
		if
		(
			( feu_params_col->feu_params[feu].Trig_Conf_Rate >= 0 )
			||
			( feu_params_col->feu_params[feu].Trig_Conf_Src[0] != '\0' )
			||
			( feu_params_col->feu_params[feu].Trig_Conf_TrigPipeLen >= 0 )
		)
		{
			if( feu_params_col->feu_params[feu].Trig_Conf_Rate < 0 )
				feu_params_col->feu_params[feu].Trig_Conf_Rate = feu_params_col->feu_params[0].Trig_Conf_Rate;
			if( feu_params_col->feu_params[feu].Trig_Conf_Src[0]=='\0' )
				sprintf(feu_params_col->feu_params[feu].Trig_Conf_Src, "%s", feu_params_col->feu_params[0].Trig_Conf_Src);
			if( feu_params_col->feu_params[feu].Trig_Conf_TrigPipeLen < 0 )
				feu_params_col->feu_params[feu].Trig_Conf_TrigPipeLen = feu_params_col->feu_params[0].Trig_Conf_TrigPipeLen;
		}

		// Auxiliary Trigger Interface parameters
		if
		(
			( feu_params_col->feu_params[feu].TI_Ignore >= 0 )
			||
			( feu_params_col->feu_params[feu].TI_DcBal_Enc >= 0 )
			||
			( feu_params_col->feu_params[feu].TI_DcBal_Dec >= 0 )
			||
			( feu_params_col->feu_params[feu].TI_Bert >= 0 )
		)
		{
			if( feu_params_col->feu_params[feu].TI_Ignore < 0 )
				feu_params_col->feu_params[feu].TI_Ignore = feu_params_col->feu_params[0].TI_Ignore;
			if( feu_params_col->feu_params[feu].TI_DcBal_Enc < 0 )
				feu_params_col->feu_params[feu].TI_DcBal_Enc = feu_params_col->feu_params[0].TI_DcBal_Enc;
			if( feu_params_col->feu_params[feu].TI_DcBal_Dec < 0 )
				feu_params_col->feu_params[feu].TI_DcBal_Dec = feu_params_col->feu_params[0].TI_DcBal_Dec;
			if( feu_params_col->feu_params[feu].TI_Bert < 0 )
				feu_params_col->feu_params[feu].TI_Bert = feu_params_col->feu_params[0].TI_Bert;
		}

		// Self Trigger parameters
		if
		(
			( feu_params_col->feu_params[feu].SelfTrig_DreamMask >= 0 )
			||
			( feu_params_col->feu_params[feu].SelfTrig_Mult >= 0 )
			||
			( feu_params_col->feu_params[feu].SelfTrig_CmbHitProp >= 0 )
			||
			( feu_params_col->feu_params[feu].SelfTrig_DrmHitWid >= 0 )
			||
			( feu_params_col->feu_params[feu].SelfTrig_CmbHitWid >= 0 )
			||
			( feu_params_col->feu_params[feu].SelfTrig_TrigTopo >= 0 )
		)
		{
			if( feu_params_col->feu_params[feu].SelfTrig_DreamMask < 0 )
				feu_params_col->feu_params[feu].SelfTrig_DreamMask = feu_params_col->feu_params[0].SelfTrig_DreamMask;
			if( feu_params_col->feu_params[feu].SelfTrig_Mult < 0 )
				feu_params_col->feu_params[feu].SelfTrig_Mult = feu_params_col->feu_params[0].SelfTrig_Mult;
			if( feu_params_col->feu_params[feu].SelfTrig_CmbHitProp < 0 )
				feu_params_col->feu_params[feu].SelfTrig_CmbHitProp = feu_params_col->feu_params[0].SelfTrig_CmbHitProp;
			if( feu_params_col->feu_params[feu].SelfTrig_DrmHitWid < 0 )
				feu_params_col->feu_params[feu].SelfTrig_DrmHitWid = feu_params_col->feu_params[0].SelfTrig_DrmHitWid;
			if( feu_params_col->feu_params[feu].SelfTrig_CmbHitWid < 0 )
				feu_params_col->feu_params[feu].SelfTrig_CmbHitWid = feu_params_col->feu_params[0].SelfTrig_CmbHitWid;
			if( feu_params_col->feu_params[feu].SelfTrig_TrigTopo < 0 )
				feu_params_col->feu_params[feu].SelfTrig_TrigTopo = feu_params_col->feu_params[0].SelfTrig_TrigTopo;
		}
		// Topology parameters
		for( index=0; index<D_FeuPar_SelfTrigTopo_Size; index++ )
		{
			if( feu_params_col->feu_params[feu].SelfTrig_Topology[index] == 0 )
					feu_params_col->feu_params[feu].SelfTrig_Topology[index] = feu_params_col->feu_params[0].SelfTrig_Topology[index];
		}

		// FEU Prescale Register
		if
		(
			(feu_params_col->feu_params[feu].Feu_PreScale_EvtData > 0)
			||
			(feu_params_col->feu_params[feu].Feu_InterPacket_Delay > 0)
		)
		{
			if( feu_params_col->feu_params[feu].Feu_PreScale_EvtData < 0 )
				feu_params_col->feu_params[feu].Feu_PreScale_EvtData = feu_params_col->feu_params[0].Feu_PreScale_EvtData;
			if( feu_params_col->feu_params[feu].Feu_InterPacket_Delay < 0 )
				feu_params_col->feu_params[feu].Feu_InterPacket_Delay = feu_params_col->feu_params[0].Feu_InterPacket_Delay;
		}
	}

	return D_RetCode_Sucsess;
}
*/
