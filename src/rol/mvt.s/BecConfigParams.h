/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    BecConfigParams.h
-- Description:    Backend crate configuration parameters
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2014/09/29 IM
-- Revision:       
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
#ifndef H_BecConfigParams
#define H_BecConfigParams

#ifndef DEF_MAX_NB_OF_BEC
#define DEF_MAX_NB_OF_BEC 3
#endif

#ifndef DEF_MAX_NB_OF_BEU
#define DEF_MAX_NB_OF_BEU 4
#endif

#ifndef DEF_MAX_NB_OF_FEU
#define DEF_MAX_NB_OF_FEU 64
#endif

#ifndef DEF_MAX_FEU_SN
#define DEF_MAX_FEU_SN 256
#endif

#ifndef DEF_MAX_NB_OF_FEU_PER_BEU
#define DEF_MAX_NB_OF_FEU_PER_BEU 32
#endif

/*
 * Crate slot definitions
 */
#ifndef DEF_SWITCH_A_SLOT
#define DEF_SWITCH_A_SLOT 11
#endif

#ifndef DEF_SWITCH_B_SLOT
#define DEF_SWITCH_B_SLOT 12
#endif

#ifndef DEF_SD_SLOT
#define DEF_SD_SLOT DEF_SWITCH_B_SLOT
#endif

#define DEF_MIN_BEU_SLOT 13
#define DEF_MAX_BEU_SLOT 20

#ifndef DEF_TI_SLOT
#define DEF_TI_SLOT 21
#endif

// Define Ti trigger sources
typedef enum _MblkRank
{
	MblkRank_Undefined = -1,
	MblkRank_Disable   =  0,
	MblkRank_First     =  1,
	MblkRank_Inter     =  2,
	MblkRank_Last      =  3,
} MblkRank;
char *MblkRank2Str( MblkRank rank );
MblkRank MblkStr2Rank( char *str );

typedef struct _BecParams
{
	// Backend crate configurateion and crate Id
	int Config_Id;
	int Crate_Id;

	/*
	 * Devices
	 */
	// TI
	int Ti_Id;
	int Ti_Slot;
	unsigned int Ti_BaseAdr_A24_Reg;
	unsigned int Ti_BaseAdr_A32_Fifo;
	MblkRank     Ti_Mblk_Rank;

	// SD
	int Sd_Id;
	int Sd_Slot;

	// Beu
	int Beu_Id[DEF_MAX_NB_OF_BEU];
	int Beu_Slot[DEF_MAX_NB_OF_BEU];
	unsigned int Beu_BaseAdr_A24_Reg[DEF_MAX_NB_OF_BEU];
	unsigned int Beu_BaseAdr_A32_Fifo[DEF_MAX_NB_OF_BEU];
	MblkRank     Beu_Mblk_Rank[DEF_MAX_NB_OF_BEU];

	// Common A32 memory space for chained block transfers
	         int BaseAdr_A32m_Com_Enb;
	unsigned int BaseAdr_A32m_Com_Min;
	unsigned int BaseAdr_A32m_Com_Max;

	// System topology
	int BeuFeuConnectivity[DEF_MAX_NB_OF_BEU][DEF_MAX_NB_OF_FEU_PER_BEU];
	int FeuSn2BeuLnkId[DEF_MAX_FEU_SN+1];

	// Self trigger parameters & topology
	int SelfTrigMult;
	int SelfTrigWin;
	int BeuFeuSlfTrigEn[DEF_MAX_NB_OF_BEU];
} BecParams;
int BecParams_Init(    BecParams *bec_params );
int BecParams_Sprintf( BecParams *bec_params, char *buf  );
int BecParams_Fprintf( BecParams *bec_params, FILE *fptr );
int BecParams_Parse(   BecParams *bec_params, int line_num );
int BecParams_Fread(   BecParams *bec_params, FILE *fptr );
int BecParams_Prop(    BecParams *params );

#endif // #ifndef H_BecConfigParams
