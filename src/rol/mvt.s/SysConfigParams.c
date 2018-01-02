/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    SysConfigParams.c
-- Description:    System Configuration parameters
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2014/09/29 IM
-- Revision:       1.0 2015/08/28 IM: SysParams_Parse added
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
#include "BecConfigParams.h"
#include "SdConfigParams.h"
#include "TiConfigParams.h"
#include "BeuConfigParams.h"
#include "FeuConfigParams.h"
#include "SysConfigParams.h"

typedef struct _SysClcParams
{
	int    DrmRdClkPeriod;
	double DrmRdClkDiv;
	int    DrmWrClkPeriod;
	double DrmWrClkDiv;
	int    DrmWrClkPhase;
	int    AdcInClkPhase;
	int    AdcOutClkPhase;
} SysClcParams;

const SysClcParams PredefinedSysClcParams[SysClkLast] =
{
	{48, 6.0, 48, 6.0,  2, 5, 3},
	{44, 5.5, 44, 5.5,  2, 5, 3},
	{40, 5.0, 40, 5.0,  2, 5, 3},
	{36, 4.5, 36, 4.5,  2, 6, 0},
	{32, 4.0, 32, 4.0,  2, 7, 0},
	{48, 6.0, 24, 3.0, 14, 5, 3}
};

// Conversion function from RunMode to string
char *SysRunMode2Str( SysRunMode mode )
{
	if( mode == SysModUdef )
		return ("SysModUdef");
	else if( mode == Clas12 )
		return ("Clas12    ");
	else if( mode == Standalone )
		return ("Standalone");
	else if( mode == Expert )
		return ("Expert    ");
	else if( mode == Spy )
		return ("Spy       ");
	else
		return ("Unknown   ");
}

// Conversion function from ClkMode to string
char *SysClkMode2Str( SysClkMode mode )
{
	if( mode == SysClkUdef )
		return ("SysClkUdef");
	else if( mode == Smp48_Rd48 )
		return ("Smp48_Rd48");
	else if( mode == Smp44_Rd44 )
		return ("Smp44_Rd44");
	else if( mode == Smp40_Rd40 )
		return ("Smp40_Rd40");
	else if( mode == Smp36_Rd36 )
		return ("Smp36_Rd36");
	else if( mode == Smp32_Rd32 )
		return ("Smp32_Rd32");
	else if( mode == Smp24_Rd48 )
		return ("Smp24_Rd48");
	else
		return ("Unknown   ");
}

// Initialize the SysParams structure with default values
int SysParams_Init( SysParams *params )
{
	int ret;
	int bec;

	// Check for Null pointer
	if( params == (SysParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Common parameters
	sprintf( params->Name, "Undefined" );
	params->RunMode           = SysModUdef;
	params->ClkMode           = SysClkUdef;
	params->SparseSmp         = 0;
	params->NbOfSmpPerEvt     = 0;
	params->NbOfEvtPerBlk     = 0;
	params->BlockPrescale     = 1;
	params->EventLimit        = 0;
	params->RepRawData        = 0;
	params->SelfTrigLat       = 0;
	params->SelfTrigWin       = 0;

	// System topology
	for( bec=0; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		ret = BecParams_Init( &(params->Bec_Params[bec]) );
		if( ret != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: BecParams_Init failed for bec %d with %d\n", __FUNCTION__, bec, ret );
			return ret;
		}
		ret = SdParams_Init( &(params->Sd_Params[bec]) );
		if( ret != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: SdParams_Init failed for sd %d with %d\n", __FUNCTION__, bec, ret );
			return ret;
		}
		ret = TiParams_Init( &(params->Ti_Params[bec]) );
		if( ret != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: TiParams_Init failed for ti %d with %d\n", __FUNCTION__, bec, ret );
			return ret;
		}
	}

	// Beu parameters
	ret = BeuSspConfCol_Init( &(params->BeuSspConf_Col) );
	if( ret != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: BeuSspConfCol_Init failed with %d\n", __FUNCTION__, ret );
		return ret;
	}

	// Feu parameters
	ret = FeuParamsCol_Init( &(params->FeuParams_Col) );
	if( ret != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: FeuParamsCol_Init failed with %d\n", __FUNCTION__, ret );
		return ret;
	}

	// All went fine
	return D_RetCode_Sucsess;
}

// Dump in ASCII the SysParams structure to a buffer
int SysParams_Sprintf( SysParams *params, char *buf  )
{
	int bec;
	int ret;

	// Check for Null pointer
	if( params == (SysParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}
	if( buf == (char *)NULL )
	{
		fprintf( stderr, "%s: buf=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Common parameters
	sprintf( buf, "############################\n" );
	sprintf( buf, "%s# Global System parameters #\n", buf );
	sprintf( buf, "%s############################\n", buf );
	sprintf( buf, "%sSys Name             %s\n", buf, params->Name );
	sprintf( buf, "%sSys RunMode          %s\n", buf, SysRunMode2Str( params->RunMode ) );
	sprintf( buf, "%sSys NbOfSmpPerEvt    %d\n", buf, params->NbOfSmpPerEvt );
	sprintf( buf, "%sSys NbOfEvtPerBlk    %d\n", buf, params->NbOfEvtPerBlk );
	sprintf( buf, "%sSys ClkMode          %s\n", buf, SysClkMode2Str( params->ClkMode ) );
	sprintf( buf, "%sSys SparseSmp        %d\n", buf, params->SparseSmp );
	sprintf( buf, "%sSys BlockPrescale    %d\n", buf, params->BlockPrescale );
	sprintf( buf, "%sSys EventLimit       %d\n", buf, params->EventLimit );
	sprintf( buf, "%sSys BRepRawData      %d\n", buf, params->RepRawData );
	sprintf( buf, "%sSys SelfTrigLat      %d # ns\n", buf, params->SelfTrigLat );
	sprintf( buf, "%sSys SelfTrigWin      %d # ns\n", buf, params->SelfTrigWin );

//fprintf( stderr, "%s: Global parameters OK\n", __FUNCTION__ );

	// Backend crate configurations
	sprintf( buf, "%s################################\n", buf );
	sprintf( buf, "%s# Backend crate configurations #\n", buf );
	sprintf( buf, "%s################################\n", buf );
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		ret = BecParams_Sprintf( &(params->Bec_Params[bec]), buf );
		if( ret != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: BecParams_Sprintf failed for bec %d with %d\n", __FUNCTION__, bec, ret );
			return ret;
		}
	}
//fprintf( stderr, "%s: BecParams_Sprintf OK\n", __FUNCTION__ );

	// TI configurations
	sprintf( buf, "%s#####################\n", buf );
	sprintf( buf, "%s# TI configurations #\n", buf );
	sprintf( buf, "%s#####################\n", buf );
	for( bec=0; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		if( params->RunMode == Clas12 )
			sprintf( buf, "%s# TI parameters do not apply in %s mode #\n", buf, SysRunMode2Str( params->RunMode ) );
		ret = TiParams_Sprintf( &(params->Ti_Params[bec]), buf );
		if( ret != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: TiParams_Sprintf failed for sd %d with %d\n", __FUNCTION__, bec, ret );
			return ret;
		}
		if( params->RunMode == Clas12 )
			sprintf( buf, "%s# TI parameters do not apply in %s mode #\n", buf, SysRunMode2Str( params->RunMode ) );
	}
	sprintf( buf, "%s\n", buf );
//fprintf( stderr, "%s: TiParams_Sprintf OK\n", __FUNCTION__ );

	// SD configurations
	sprintf( buf, "%s#####################\n", buf );
	sprintf( buf, "%s# SD configurations #\n", buf );
	sprintf( buf, "%s#####################\n", buf );
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		ret = SdParams_Sprintf( &(params->Sd_Params[bec]), buf );
		if( ret != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: SdParams_Sprintf failed for sd %d with %d\n", __FUNCTION__, bec, ret );
			return ret;
		}
	}
	sprintf( buf, "%s\n", buf );
//fprintf( stderr, "%s: SdParams_Sprintf OK\n", __FUNCTION__ );

	// Backend unit configurations
	sprintf( buf, "%s###############################\n", buf );
	sprintf( buf, "%s# Backend unit configurations #\n", buf );
	sprintf( buf, "%s###############################\n", buf );
	ret = BeuSspConfCol_Sprintf( &(params->BeuSspConf_Col), buf );
	if( ret != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: BeuSspConfCol_Sprintf failed with %d\n", __FUNCTION__, ret );
		return ret;
	}
	sprintf( buf, "%s\n", buf );
//fprintf( stderr, "%s: BeuSspConfCol_Sprintf OK\n", __FUNCTION__ );

	// Frontend unit configurations
	sprintf( buf, "%s################################\n", buf );
	sprintf( buf, "%s# Frontend unit configurations #\n", buf );
	sprintf( buf, "%s################################\n", buf );
	ret = FeuParamsCol_Sprintf( &(params->FeuParams_Col), buf );
	if( ret != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: FeuParamsCol_Sprintf failed with %d\n", __FUNCTION__, ret );
		return ret;
	}
//fprintf( stderr, "%s: FeuParamsCol_Sprintf OK\n", __FUNCTION__ );

	// All went fine
	return D_RetCode_Sucsess;
}

// Dump in ASCII the SysParams structure to a file
int SysParams_Fprintf( SysParams *params, FILE *fptr )
{
	char buf[256*1024];
	int ret;

	// Check for Null pointer
	if( fptr == (FILE *)NULL )
	{
		fprintf( stderr, "%s: fptr=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// First produce the ASCII representation of the MVT parameters structure 
	if( (ret = SysParams_Sprintf( params, buf )) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: SysParams_Sprintf failed with  %d\n", __FUNCTION__, ret );
		return ret;
	}

	// Then save it in the file  
	if( (ret = fprintf( fptr, "%s", buf ) ) < 0 )
	{
		fprintf( stderr, "%s: fprintf failed with %d\n", __FUNCTION__, ret );
		fprintf( stderr, "%s: fprintf failed with %s\n", __FUNCTION__, strerror(errno) );
		return ret;
	}

	// All went fine
	return D_RetCode_Sucsess;
}

// Fill the SysParams structure from an ASCII file
int SysParams_Fread( SysParams *params, FILE *fptr )
{
	char line[LINE_SIZE];
	int line_num;
	int ret;

	// Check for Null pointer
	if( params == (SysParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}
	if( fptr == (FILE *)NULL )
	{
		fprintf( stderr, "%s: fptr=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Process parameter file
	line_num = 0;
	while( fgets( line, LINE_SIZE, fptr ) != NULL )
	{
		/* parse the line */
		parse_line(line);
		line_num++;
		// Parse parameters
		if( (ret = SysParams_Parse( params, line_num )) != D_RetCode_Sucsess )
		{
			fprintf( stderr, "%s: SysParams_Parse failed with %d\n", __FUNCTION__, ret );
			return ret;
		}
	} // while( fgets( line, LINE_SIZE, fptr ) != NULL )

	// All went fine
	return D_RetCode_Sucsess;
}

// Propagate global parameters to system devices
int SysParams_Prop( SysParams *params )
{
	int ret;
	int bec;
	int beu;
	int feu;
	int feu_id;
	int ti;
	BecParams  *bec_params;
	SdParams   *sd_params;
	TiParams   *ti_params;
	BeuSspConf *beu_conf;
	FeuParams  *feu_params;

	// For BEU
	unsigned int near_end_loop_enb;
	unsigned int rol_enb;
	unsigned int feu_emu_msk;
	int Beu_TrgFifo_Hwm;
	int Beu_TrgFifo_Lwm;
	int drm_derand_buf;
	int drm_evt_buf;
	// For SD
	int sd_act_slot_flg;
	int sd_act_trig_flg;
	// For TI
	int trg_rule_0;
	int drm_trg_dur;


	// Check for Null pointer
	if( params == (SysParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	// Check all backend crates
	for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )
	{
		bec_params = &(params->Bec_Params[bec]);
		if( bec_params->Config_Id > 0 )
		{
			sd_act_slot_flg = 0;
			sd_act_trig_flg = 0;
			// propagate parapeters to BEU-s
			for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
			{
				beu_conf = &(params->BeuSspConf_Col.beu_conf[beu]);
				if( bec_params->Beu_Id[beu] > 0 )
				{
					beu_conf->Id               = bec_params->Beu_Id[beu];
					beu_conf->Slot             = bec_params->Beu_Slot[beu];
					beu_conf->base_adr_reg     = bec_params->Beu_BaseAdr_A24_Reg[beu];
					beu_conf->base_adr_fifo    = bec_params->Beu_BaseAdr_A32_Fifo[beu];
					if( bec_params->Beu_Mblk_Rank[beu] == MblkRank_First )
						beu_conf->mblk_rank = Beu_MblkRank_First;
					else if( bec_params->Beu_Mblk_Rank[beu] == MblkRank_Last )
						beu_conf->mblk_rank = Beu_MblkRank_Last;
					else
						beu_conf->mblk_rank = Beu_MblkRank_Inter;
					beu_conf->base_adr_com_min = bec_params->BaseAdr_A32m_Com_Min;
					beu_conf->base_adr_com_max = bec_params->BaseAdr_A32m_Com_Max;

					// Establish connectivity
					near_end_loop_enb = 0;
					rol_enb           = 0xFFFFffff;
					feu_emu_msk       = 0xFFFFffff;
					for( feu=0; feu<DEF_MAX_NB_OF_FEU_PER_BEU; feu++ )
					{
						// Emulated channel
						if( bec_params->BeuFeuConnectivity[beu][feu] < 0 )
						{
							rol_enb = rol_enb & ((~(1<<feu)) & 0xFFFFffff);
						}
						// Masked channel
						else if( bec_params->BeuFeuConnectivity[beu][feu] == 0 )
						{
							near_end_loop_enb += (1<<feu);
							feu_emu_msk = feu_emu_msk & ((~(1<<feu)) & 0xFFFFffff);
						}
						// Enabled channels
						else
						{
//							feu_id = bec_params->BeuFeuConnectivity[beu][feu];
//							params->FeuParams_Col.feu_params[feu_id].Feu_RunCtrl_Id = feu_id;

							feu_id = ((beu-1)<<5) + feu + 1;
							params->FeuParams_Col.feu_params[feu_id].Feu_RunCtrl_Id = bec_params->BeuFeuConnectivity[beu][feu];
							if( bec_params->BeuFeuSlfTrigEn[beu] & (1<<feu) )
								params->FeuParams_Col.feu_params[feu_id].SelfTrig_CmbHitPropOl = 1;
							else
								params->FeuParams_Col.feu_params[feu_id].SelfTrig_CmbHitPropOl = 0;
						}
					}
					beu_conf->near_end_loop_enb = near_end_loop_enb;
					beu_conf->rol_enb = rol_enb;
					beu_conf->feu_emu_msk = feu_emu_msk;

					beu_conf->SelfTrigLnkEnb   = bec_params->BeuFeuSlfTrigEn[beu];
					beu_conf->SelfTrigLat      = params->SelfTrigLat;
					beu_conf->SelfTrigWin      = params->SelfTrigWin;

					// Construct SD active slot flags
					sd_act_slot_flg |= (1<<beu_conf->Slot);
					if( beu_conf->SelfTrigLnkEnb )
						sd_act_trig_flg |= (1<<beu_conf->Slot);
				}
			} // for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )

			// propagate parameters to TI
			ti_params = &(params->Ti_Params[bec]);
			ti_params->Id                   = bec_params->Crate_Id;
			ti_params->Slot                 = DEF_TI_SLOT;
			ti_params->BaseAdr_A24_Reg      = bec_params->Ti_BaseAdr_A24_Reg;
			ti_params->BaseAdr_A32_Fifo     = bec_params->Ti_BaseAdr_A32_Fifo;
			ti_params->BaseAdr_A32m_Com_Min = bec_params->BaseAdr_A32m_Com_Min;
			ti_params->BaseAdr_A32m_Com_Max = bec_params->BaseAdr_A32m_Com_Max;
			ti_params->Mblk_Rank            = bec_params->Ti_Mblk_Rank;

			// propagate parameters to SD
			sd_params = &(params->Sd_Params[bec]);
			sd_params->Id              = bec_params->Crate_Id;
			sd_params->Slot            = DEF_SD_SLOT;
			sd_params->ActiveSlotFlags = sd_act_slot_flg;
			sd_params->ActiveTrigFlags = sd_act_trig_flg;
			sd_params->TrigMult        = bec_params->SelfTrigMult;
			sd_params->TrigWin         = bec_params->SelfTrigWin;

			// and updaet SD entries in the bec
			bec_params->Sd_Id   = sd_params->Id;
			bec_params->Sd_Slot = DEF_SD_SLOT;

		} // if( bec_params->Config_Id > 0 )
	} // for( bec=1; bec<DEF_MAX_NB_OF_BEC; bec++ )

	// propagate common beu parameters
	if( ret = BeuSspConfCol_Prop( &(params->BeuSspConf_Col) ) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: BeuSspConfCol_Prop railed with %d\n", __FUNCTION__, ret );

		return D_RetCode_Err_Null_Pointer;
	}

	// propagate common ti parameters
	if( ret = TiParams_Prop( params->Ti_Params ) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: TiParams_Prop failed with %d\n", __FUNCTION__, ret );
		return D_RetCode_Err_Null_Pointer;
	}

	// propagate system wide configuration parameters
	feu_params = &(params->FeuParams_Col.feu_params[0]);
	for( beu=1; beu<DEF_MAX_NB_OF_BEU; beu++ )
	{
		beu_conf = &(params->BeuSspConf_Col.beu_conf[beu]);
		if( beu_conf->Id > 0 )
		{
			beu_conf->NbOfSamples        = params->NbOfSmpPerEvt;
			beu_conf->NbOfEventsPerBlock = params->NbOfEvtPerBlk;
			beu_conf->SelfTrigLat        = params->SelfTrigLat;
			beu_conf->SelfTrigWin        = params->SelfTrigWin;
			if( (params->RunMode == Clas12) || (params->RunMode == Standalone) )
			{
				beu_conf->ClkSrc = BeuClkSrc_TiVxs;
			}
			else if( params->RunMode == Expert )
			{
				beu_conf->ClkSrc = BeuClkSrc_OnBoard_250MHz;
			}
			else
			{
				fprintf( stderr, "%s: Unsupported run mode %d\n", __FUNCTION__, params->RunMode );
				return D_RetCode_Err_Wrong_Param;
			}
			// Try to be smart and setup trigger fifo limits
			// For the moment assume that all Dreams on all FEUs have the same pipeline
			drm_derand_buf = 512 - feu_params->dream_params[D_FeuPar_NumOfDreams-1].dream_reg[12].reg[0];
			drm_evt_buf = drm_derand_buf / params->NbOfSmpPerEvt;
			if( drm_evt_buf > 20 )
			{
				Beu_TrgFifo_Hwm = drm_evt_buf - 4;
				Beu_TrgFifo_Lwm = drm_evt_buf -16;
			}
			if( drm_evt_buf > 16 )
			{
				Beu_TrgFifo_Hwm = drm_evt_buf - 4;
				Beu_TrgFifo_Lwm = drm_evt_buf - 8;
			}
			else if( drm_evt_buf > 8 )
			{
				Beu_TrgFifo_Hwm = drm_evt_buf - 4;
				Beu_TrgFifo_Lwm = drm_evt_buf - 4;
			}
			else if( drm_evt_buf > 4 )
			{
				Beu_TrgFifo_Hwm = drm_evt_buf - 2;
				Beu_TrgFifo_Lwm = drm_evt_buf - 4;
			}
			else if( drm_evt_buf > 2 )
			{
				Beu_TrgFifo_Hwm = 2;
				Beu_TrgFifo_Lwm = 1;
			}
			else if( drm_evt_buf > 0 )
			{
				Beu_TrgFifo_Hwm = 1;
				Beu_TrgFifo_Lwm = 0;
			}
			else
			{
				fprintf( stderr, "%s: Unebale to setup TRG FIFO thresholds for BEU %d with DrmPipeLine=%d and num of samples=%d; dream evt buffer=%d\n",
					__FUNCTION__, beu_conf->Id,
					feu_params->dream_params[D_FeuPar_NumOfDreams-1].dream_reg[12].reg[0],
					params->NbOfSmpPerEvt,
					drm_evt_buf ); 
				return D_RetCode_Err_Wrong_Param;
			}
			if( beu_conf->TrgFifo_Hwm > Beu_TrgFifo_Hwm )
			{
				fprintf( stdout, "%s: Warning BEU %d TrgFifo_Hwm will be forced to %d instead of requested %d\n",
					__FUNCTION__, beu_conf->Id, Beu_TrgFifo_Hwm, beu_conf->TrgFifo_Hwm );
				beu_conf->TrgFifo_Hwm = Beu_TrgFifo_Hwm;
			}
			if( beu_conf->TrgFifo_Lwm > Beu_TrgFifo_Lwm )
			{
				fprintf( stdout, "%s: Warning BEU %d TrgFifo_Lwm will be forced to %d instead of requested %d\n",
					__FUNCTION__, beu_conf->Id, Beu_TrgFifo_Lwm, beu_conf->TrgFifo_Lwm );
				beu_conf->TrgFifo_Lwm = Beu_TrgFifo_Lwm;
			}
		}
	}

	// propagate system wide configuration parameters
	for( ti=1; ti<DEF_MAX_NB_OF_BEC; ti++ )
	{
		ti_params = &(params->Ti_Params[ti]);
		if( ti_params->Id > 0 )
		{
			ti_params->NbOfEvtPerBlk = params->NbOfEvtPerBlk;
			if( params->EventLimit )
			{
				ti_params->BlockLimit    = params->EventLimit/params->NbOfEvtPerBlk;
				if( params->EventLimit%params->NbOfEvtPerBlk )
					ti_params->BlockLimit++;
			}
			else
				ti_params->BlockLimit = 0;
			if( params->RunMode == Clas12 ) 
			{
				ti_params->ClkSrc = TiClkSrc_HFBR1;
				ti_params->TrgSrc = TiTrgSrc_HFBR1;
				ti_params->SynSrc = TiSynSrc_HFBR1;
				ti_params->BsySrc = TiBsySrc_SwB;
			}
			else if( (params->RunMode == Expert) || (params->RunMode == Standalone) )
			{
				ti_params->ClkSrc = TiClkSrc_Internal;
			}
			else
			{
				fprintf( stderr, "%s: Unsupported run mode %d\n", __FUNCTION__, params->RunMode );
				return D_RetCode_Err_Null_Pointer;
			}
			// Recalculate trigger rule 0 for NbOfSamples
			if( params->RunMode != Clas12 )
			{
				if( ti_params->TrgRules_TimeUnit[0] == 0 )
					trg_rule_0 = ti_params->TrgRules_UnitCount[0] * Def_Ti_TrgRules_TimeUnit_016ns;
				else if( ti_params->TrgRules_TimeUnit[0] == 1 )
					trg_rule_0 = ti_params->TrgRules_UnitCount[0] * Def_Ti_TrgRules_TimeUnit_480ns;
				else
				{
					fprintf( stderr, "%s: Wrong trigger rule 0 time unit %d for ti=%d\n", __FUNCTION__, ti_params->TrgRules_TimeUnit[0], ti );
					return D_RetCode_Err_Wrong_Param;
				}
			}
			drm_trg_dur = (params->NbOfSmpPerEvt+1) * PredefinedSysClcParams[params->ClkMode].DrmWrClkPeriod*(1+params->SparseSmp);
			if( (params->RunMode != Clas12) && (trg_rule_0 < drm_trg_dur) )
			{
				if( ( drm_trg_dur / Def_Ti_TrgRules_TimeUnit_016ns ) <= Def_Ti_TrgRules_MaxNumOfUnitCnt )
				{
					ti_params->TrgRules_UnitCount[0] = (drm_trg_dur+Def_Ti_TrgRules_TimeUnit_016ns) / Def_Ti_TrgRules_TimeUnit_016ns;
					ti_params->TrgRules_TimeUnit[0]  = 0;
				}
				else if( ( drm_trg_dur / Def_Ti_TrgRules_TimeUnit_480ns ) <= Def_Ti_TrgRules_MaxNumOfUnitCnt )
				{
					ti_params->TrgRules_UnitCount[0] = (drm_trg_dur + Def_Ti_TrgRules_TimeUnit_480ns) / Def_Ti_TrgRules_TimeUnit_480ns;
					ti_params->TrgRules_TimeUnit[0]  = 1;
				}
				else
				{
					fprintf( stderr, "%s: Trigger rule 0 cannot be calculated for NbOfSamples=%d DrmSmpPeriod=%d and drm_trg_dur=%d ns\n",
						__FUNCTION__, params->NbOfSmpPerEvt, PredefinedSysClcParams[params->ClkMode].DrmWrClkPeriod, drm_trg_dur );
					return D_RetCode_Err_Wrong_Param;
				}
			} // if( trg_rule_0 < (params->NbOfSmpPerEvt * 48) )
		} // if( ti_params->Id > 0 )
	} // for( ti=1; ti<DEF_MAX_NB_OF_BEC; ti++ )

	// propagate system wide configuration parameters
//	for( feu=0; feu<=DEF_MAX_NB_OF_FEU; feu++ )
//	{
		feu_params = &(params->FeuParams_Col.feu_params[0]);
//		if( feu_params->Feu_RunCtrl_Id > 0 )
//		{
			feu_params->Main_Conf_Samples = params->NbOfSmpPerEvt;
			sprintf( feu_params->Main_Conf_ClkSel, "RecClk" );
//			feu_params->Feu_Pwr_Dream = 0xF;
			feu_params->Feu_Pwr_PrtFlt = 0xFFFF;
			feu_params->Feu_RunCtrl_DrDblSmpClk = 0;
			feu_params->Main_Conf_DataPipeLen = params->SparseSmp;
			feu_params->Feu_RunCtrl_AdcDatRdyDel = 8;
			feu_params->Feu_RunCtrl_EvTstExt = 0;
//			feu_params->Feu_Pulser_Enable = 0;
//			feu_params->Feu_Pulser_DreamTst = 0;
//			feu_params->Feu_Pulser_PulseWid = 0;
			feu_params->Feu_PreScale_EvtData = 1;
			sprintf( feu_params->Trig_Conf_Src, "Tg_Src_Int" );
			sprintf( feu_params->Trig_Conf_File, "None" );
			feu_params->Trig_Conf_TrigVetoLen = 0;
			feu_params->Trig_Conf_Rate = 0;
			feu_params->TI_Ignore = 1;
			feu_params->TI_DcBal_Enc = 0;
			feu_params->TI_DcBal_Dec = 0;
			feu_params->TI_Bert = 0;
			feu_params->SelfTrig_CmbHitPropFb = 0;
			feu_params->SelfTrig_CmbHitPropOl = 0;
/*
			feu_params->SelfTrig_DreamMask = 0xFF;
			feu_params->SelfTrig_Mult = 7;
			feu_params->SelfTrig_CmbHitPropOl = 0;
			feu_params->SelfTrig_TrigTopo = 0;
			feu_params->SelfTrig_DrmHitWid = 0x3F;
			feu_params->SelfTrig_CmbHitWid = 0x3F;
			feu_params->SelfTrig_Veto = 0xFFFFFF;
*/
			feu_params->UdpChan_Enable = 0;
			feu_params->ComChan_Enable = 1;
			// Dream clock parameters
			feu_params->dream_clk_params.RdClk_Div    = PredefinedSysClcParams[params->ClkMode].DrmRdClkDiv;
			feu_params->dream_clk_params.WrClk_Div    = PredefinedSysClcParams[params->ClkMode].DrmWrClkDiv;
			feu_params->dream_clk_params.WrClk_Phase  = PredefinedSysClcParams[params->ClkMode].DrmWrClkPhase;
			feu_params->dream_clk_params.AdcClk_Phase = PredefinedSysClcParams[params->ClkMode].AdcInClkPhase;
			// Adc clock parameters
			feu_params->adc_params.adc_reg[22].val = (unsigned char)(PredefinedSysClcParams[params->ClkMode].AdcOutClkPhase);
			feu_params->adc_params.adc_reg[22].flg = AdcRegFlag_SetAndUpd;
			// Make sure the ZS window is not bigger than number of samples
			// ZS window: sample 0 never compared to threshold
			// 0 compare samples 1 through 2  min nb of samples 4
			// 1                 1 through 3                    5
			// 2                 1 through 4                    6
			// 3                 1 through 5                    7
                        // 4                 1 through 6                    8
			//
			if( feu_params->Feu_RunCtrl_ZS == 1 )
			{
				if( feu_params->Feu_RunCtrl_ZsChkSmp > (params->NbOfSmpPerEvt-4) )
				{
					fprintf( stdout, "%s: Warning default value of Feu_RunCtrl_ZsChkSmp will be forced to %d instead of requested %d\n",
						__FUNCTION__, params->NbOfSmpPerEvt-4, feu_params->Feu_RunCtrl_ZsChkSmp );
					feu_params->Feu_RunCtrl_ZsChkSmp = params->NbOfSmpPerEvt-4;
				}
			}
//		}
//	}

	// propagate common beu parameters
	if( ret = FeuParamsCol_PropComParams( &(params->FeuParams_Col) ) != D_RetCode_Sucsess )
	{
		fprintf( stderr, "%s: FeuParamsCol_PropComParams failed with %d\n", __FUNCTION__, ret );
		return D_RetCode_Err_Null_Pointer;
	}

	// All went fine
	return D_RetCode_Sucsess;
}

// Fill the SysParams structure entries from externally parsed line
int SysParams_Parse( SysParams *params, int line_num )
{
	int bec;
	int ret;

	// Check for Null pointer
	if( params == (SysParams *)NULL )
	{
		fprintf( stderr, "%s: params=0\n", __FUNCTION__ );
		return D_RetCode_Err_Null_Pointer;
	}

	if( argc > 0 )
	{
		if( ( strcmp( argv[0], "Sys" ) == 0 ) || ( strcmp( argv[0], "MVT_Sys" ) == 0 ) || ( strcmp( argv[0], "FTT_Sys" ) == 0 ) )
		{
			// Common system parameters
			if( strcmp( argv[1], "Name" ) == 0 )
				sprintf( params->Name, "%s", argv[2]);
			else if( strcmp( argv[1], "RunMode" ) == 0 )
			{
				if( strcmp( argv[2], "Standalone" ) == 0 )
					params->RunMode = Standalone;
				else if( strcmp( argv[2], "Clas12" ) == 0 )
					params->RunMode = Clas12;
				else if( strcmp( argv[2], "Expert" ) == 0 )
					params->RunMode = Expert;
				else if( strcmp( argv[2], "Undefined" ) == 0 )
				{
					params->RunMode = SysModUdef;
					fprintf( stderr, "%s: line %d: Sys RunMode explicitly set to Undefined\n", __FUNCTION__, line_num ); 
					return D_RetCode_Err_Wrong_Param;
				}
				else
				{
					params->RunMode = SysModUdef;
					fprintf( stderr, "%s: line %d: attempt to set unknown Sys RunMode %s\n", __FUNCTION__, line_num, argv[2] ); 
					return D_RetCode_Err_Wrong_Param;
				}
			}
			else if( strcmp( argv[1], "ClkMode" ) == 0 )
			{
				if( strcmp( argv[2], "Smp48_Rd48" ) == 0 )
					params->ClkMode = Smp48_Rd48;
				else if( strcmp( argv[2], "Smp44_Rd44" ) == 0 )
					params->ClkMode = Smp44_Rd44;
				else if( strcmp( argv[2], "Smp40_Rd40" ) == 0 )
					params->ClkMode = Smp40_Rd40;
				else if( strcmp( argv[2], "Smp36_Rd36" ) == 0 )
					params->ClkMode = Smp36_Rd36;
				else if( strcmp( argv[2], "Smp32_Rd32" ) == 0 )
					params->ClkMode = Smp32_Rd32;
				else if( strcmp( argv[2], "Smp24_Rd48" ) == 0 )
					params->ClkMode = Smp24_Rd48;
				else if( strcmp( argv[2], "SysClkUdef" ) == 0 )
				{
					params->ClkMode = SysClkUdef;
					fprintf( stderr, "%s: line %d: Sys ClkMode explicitly set to Undefined\n", __FUNCTION__, line_num ); 
					return D_RetCode_Err_Wrong_Param;
				}
				else
				{
					params->ClkMode = SysClkUdef;
					fprintf( stderr, "%s: line %d: attempt to set unknown Sys ClkMode %s\n", __FUNCTION__, line_num, argv[2] ); 
					return D_RetCode_Err_Wrong_Param;
				}
			}
			else if( strcmp( argv[1], "SparseSmp" ) == 0 )
			{
				params->SparseSmp = atoi( argv[2] );
				// Check the parameter: has to be derived from FeuConfigParams.h
				if( (params->SparseSmp < 0) || (7 < params->SparseSmp) )
				{
					params->SparseSmp = 0;
					fprintf( stderr, "%s: line %d: attempt to set unsupported SparseSmp %s; must be in [0,7]\n", __FUNCTION__, line_num, argv[2] ); 
					return D_RetCode_Err_Wrong_Param;
				}
			}
			else if( strcmp( argv[1], "NbOfSmpPerEvt" ) == 0 )
			{
				params->NbOfSmpPerEvt = atoi( argv[2] );
				// Check the parameter: has to be derived from FeuConfigParams.h
				if( (params->NbOfSmpPerEvt < 1) || (256 < params->NbOfSmpPerEvt) )
				{
					params->NbOfSmpPerEvt = 0;
					fprintf( stderr, "%s: line %d: attempt to set unsupported NbOfSmpPerEvt %s; must be in [1,256]\n", __FUNCTION__, line_num, argv[2] ); 
					return D_RetCode_Err_Wrong_Param;
				}
			}
			else if( strcmp( argv[1], "NbOfEvtPerBlk" ) == 0 )
			{
				params->NbOfEvtPerBlk = atoi( argv[2] );
				// Check the parameter: has to be derived from BeuConfigParams.h
				if( (params->NbOfEvtPerBlk < 1) || (256 < params->NbOfEvtPerBlk) )
				{
					params->NbOfEvtPerBlk = 0;
					fprintf( stderr, "%s: line %d: attempt to set unsupported NbOfEvtPerBlk %s; must be in [1,256]\n", __FUNCTION__, line_num, argv[2] ); 
					return D_RetCode_Err_Wrong_Param;
				}
			}
			else if( strcmp( argv[1], "BlockPrescale" ) == 0 )
			{
				params->BlockPrescale = atoi( argv[2] );
				if( (params->BlockPrescale <= 0) )
				{
					params->BlockPrescale = 1;
					fprintf( stderr, "%s: line %d: attempt to set negative or 0 BlockPrescale %s; must be positive\n", __FUNCTION__, line_num, argv[2] ); 
					return D_RetCode_Err_Wrong_Param;
				}
			}
			else if( strcmp( argv[1], "EventLimit" ) == 0 )
			{
				params->EventLimit = atoi( argv[2] );
				if( (params->EventLimit < 0) )
				{
					params->EventLimit = 0;
					fprintf( stderr, "%s: line %d: attempt to set negative EventLimit %s; must be >=\n", __FUNCTION__, line_num, argv[2] ); 
					return D_RetCode_Err_Wrong_Param;
				}
			}
			else if( strcmp( argv[1], "RepRawData" ) == 0 )
			{
				params->RepRawData = atoi( argv[2] );
				if( (params->RepRawData < 0) || (1 < params->RepRawData) )
				{
					params->RepRawData = 0;
					fprintf( stderr, "%s: line %d: attempt to set unsupported RepRawData %s; must be 0 or 1\n", __FUNCTION__, line_num, argv[2] ); 
					return D_RetCode_Err_Wrong_Param;
				}
			}
			else if( strcmp( argv[1], "SelfTrigLat" ) == 0 )
			{
				params->SelfTrigLat = atoi( argv[2] );
				if( (params->SelfTrigLat < 0) || (8191 < params->SelfTrigLat) )
				{
					params->SelfTrigLat = 0;
					fprintf( stderr, "%s: line %d: attempt to set unsupported SelfTrigLat %s; must be in [0;8191] ns range \n", __FUNCTION__, line_num, argv[2] ); 
					return D_RetCode_Err_Wrong_Param;
				}
			}
			else if( strcmp( argv[1], "SelfTrigWin" ) == 0 )
			{
				params->SelfTrigWin = atoi( argv[2] );
				if( (params->SelfTrigWin < 0) || (511 < params->SelfTrigWin) )
				{
					params->SelfTrigWin = 0;
					fprintf( stderr, "%s: line %d: attempt to set unsupported SelfTrigWin %s; must be in [0;511] ns range \n", __FUNCTION__, line_num, argv[2] ); 
					return D_RetCode_Err_Wrong_Param;
				}
			}
			else
			{
				fprintf( stderr, "%s: line %d: Unknown or empty keywod for Sys entry %s\n", __FUNCTION__, line_num, argv[1] ); 
				return D_RetCode_Err_Wrong_Param;
			}
		}			
		else if( ( strcmp( argv[0], "Bec" ) == 0 ) || ( strcmp( argv[0], "MVT_Bec" ) == 0 ) || ( strcmp( argv[0], "FTT_Bec" ) == 0 ) )
		{
			bec = atoi( argv[1] );
			if( (bec < 1) || (DEF_MAX_NB_OF_BEC < bec) )
			{
				fprintf( stderr, "%s: line %d: bec id %d out of range [1,%d]\n", __FUNCTION__, line_num, bec, DEF_MAX_NB_OF_BEC ); 
				return D_RetCode_Err_Wrong_Param;
			}
			if( (ret = BecParams_Parse( &(params->Bec_Params[bec]), line_num )) != D_RetCode_Sucsess )
			{
				fprintf( stderr, "%s: BecParams_Parse failed for bec %d with %d\n", __FUNCTION__, bec, ret );
				return ret;
			}
		}
		else if( ( strcmp( argv[0], "Ti" ) == 0 ) || ( strcmp( argv[0], "MVT_Ti" ) == 0 ) || ( strcmp( argv[0], "FTT_Ti" ) == 0 ) )
		{
			// Read TI parameters only in Standalone and Expert modes 
			if( params->RunMode != Clas12 )
			{
				bec = atoi( argv[1] );
				if( (bec < 0) || (DEF_MAX_NB_OF_BEC < bec) )
				{
					fprintf( stderr, "%s: line %d: ti id %d out of range [1,%d]\n", __FUNCTION__, line_num, bec, DEF_MAX_NB_OF_BEC ); 
					return D_RetCode_Err_Wrong_Param;
				}
				if( (ret = TiParams_Parse( &(params->Ti_Params[bec]), line_num )) != D_RetCode_Sucsess )
				{
					fprintf( stderr, "%s: TiParams_Parse failed for ti %d with %d\n", __FUNCTION__, bec, ret );
					return ret;
				}
			}
		}
		else if( ( strcmp( argv[0], "Beu" ) == 0 ) || ( strcmp( argv[0], "MVT_Beu" ) == 0 ) || ( strcmp( argv[0], "FTT_Beu" ) == 0 ) )
		{
			if( (ret = BeuSspConfCol_Parse( &(params->BeuSspConf_Col), line_num )) != D_RetCode_Sucsess )
			{
				fprintf( stderr, "%s: BeuSspConfCol_Parse failed with %d\n", __FUNCTION__, ret );
				return ret;
			}
		}
		else if( ( strcmp( argv[0], "Feu" ) == 0 ) || ( strcmp( argv[0], "MVT_Feu" ) == 0 ) || ( strcmp( argv[0], "FTT_Feu" ) == 0 ) )
		{
			if( (ret = FeuParamsCol_Parse( &(params->FeuParams_Col), line_num )) != D_RetCode_Sucsess )
			{
				fprintf( stderr, "%s: FeuParamsCol_Parse failed with %d\n", __FUNCTION__, ret );
				return ret;
			}
		}
	} // if( argc > 0 )

	// All went fine
	return D_RetCode_Sucsess;
}


