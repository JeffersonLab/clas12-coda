/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream memory manager testbench
--
-- Module Name:    TrigGen.h
-- Description:    Trigger genarator library
--
-- Target Devices: Virtex-5 / ML507 development kit
-- Tool versions:  ISE XPS 10.1
-- 
-- Create Date:    0.0 2011/01/28 IM
-- Revision:       1.1 2011/03/04 IM Commands and status added
--                                   Software trigger added
--                                   Trigger counter register added
--                     2011/03/07 IM Trigger pipeline added
--                     2013/02/12 IM Change constant trigger rate to 1, 10 & 100 Hz 
--                     2013/04/04 IM Change Any trigger to NegExp trigger 
--                 1.2 2016/01/11 IM Veto parameter added
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
#ifndef H_TrigGen
#define H_TrigGen

	#include "bitmanip.h"

	// Trigger Generator address space
	#define D_Tg_Adr_Ofs  0
	#define D_Tg_Adr_Len 13

	// Trigger Generator address space is divided to registry and memory zones
	#define D_Tg_AdrZone_Reg 0
	#define D_Tg_AdrZone_Mem 1

	// Offset and Length of Trigger Generator address zone field  
	#define D_Tg_AdrZone_Ofs 12
	#define D_Tg_AdrZone_Len  1

	// Offset and Length of Trigger Generator memory address zone field  
	#define D_Tg_MemAdr_Ofs   0
	#define D_Tg_MemAdr_Len  12
	#define D_Tg_Mem_Size    (1<<D_Tg_MemAdr_Len)

	// Offset and Length of Trigger Generator registers address zone field  
	#define D_Tg_RegAdr_Ofs  0
	#define D_Tg_RegAdr_Len  3

	// Offset and Length of Trigger Generator registers address zone field  
	#define D_Tg_RegAdr_Conf 0
	#define D_Tg_RegAdr_Cntr 4

	// Address manipulation user macros
	#define D_Tg_Adr_Get( adr )            GetBits(adr, D_Tg_Adr_Ofs, D_Tg_Adr_Len )
	#define D_Tg_Adr_Clr( adr )            ClrBits(adr, D_Tg_Adr_Ofs, D_Tg_Adr_Len )

	#define D_Tg_AdrZone_Get( adr )        GetBits(adr, D_Tg_AdrZone_Ofs, D_Tg_AdrZone_Len )
	#define D_Tg_AdrZone_Set( adr, zone )  PutBits(adr, D_Tg_AdrZone_Ofs, D_Tg_AdrZone_Len, zone )

	#define D_Tg_AdrReg_Get( adr )         GetBits(adr, D_Tg_RegAdr_Ofs, D_Tg_RegAdr_Len )
	#define D_Tg_AdrReg_Set( adr, subadr ) PutBits(adr, D_Tg_RegAdr_Ofs, D_Tg_RegAdr_Len, subadr )

	#define D_Tg_AdrMem_Get( adr )         GetBits(adr, D_Tg_MemAdr_Ofs, D_Tg_MemAdr_Len )
	#define D_Tg_AdrMem_Clr( adr )         ClrBits(adr, D_Tg_MemAdr_Ofs, D_Tg_MemAdr_Len )
	#define D_Tg_AdrMem_Set( adr, subadr ) PutBits(adr, D_Tg_MemAdr_Ofs, D_Tg_MemAdr_Len, subadr )

	/*
	 * Config register definitions
	 */
	// Possible rate settings
	typedef enum _Tg_Rate
	{
		Tg_Rate_Zero_Hz    = 0,
		Tg_Rate_One_Hz     = 1,
		Tg_Rate_Ten_Hz     = 2,
		Tg_Rate_Hundred_Hz = 3
	} Tg_Rate;
	char *Tg_Rate2Str(Tg_Rate rate);
	// Possible trigger source settings
	typedef enum _Tg_Src
	{
		Tg_Src_Int        = 0,
		Tg_Src_ExtAsyn    = 1,
		Tg_Src_ExtSyn     = 2,
		Tg_Src_PushButton = 3,
		Tg_Src_Soft       = 4,
		Tg_Src_Constant   = 5,
		Tg_Src_Memory     = 6,
		Tg_Src_NegExp     = 7
	} Tg_Src;
	char *Tg_Src2Str(Tg_Src Tg_Src);

	/* Configuration, command and status register mapped at baseaddress + 0
	-- |      Status     |      Command     |                Config                  |
	-- | Enabled | Ready | Trig | En | Init | TrigVetoLen | TrigPipeLen | Src | Rate |
	-- |   31    |  30   |  29  | 28 | 27   |    26-17    |    16-5     | 4-2 | 1-0  |
	-- Src: 0-Push button; 1-Ext trig; 2-Const rate; 3-Bram
	-- Rate: 0-0Hz; 1-1Hz; 2-2Hz; 3-3Hz
	*/
	#define D_Tg_RegConf_Rate_Ofs          0
	#define D_Tg_RegConf_Rate_Len             2
	#define D_Tg_RegConf_Src_Ofs           2
	#define D_Tg_RegConf_Src_Len              3
	#define D_Tg_RegConf_TrigPipeLen_Ofs   5
	#define D_Tg_RegConf_TrigPipeLen_Len     12
	#define D_Tg_RegConf_TrigVetoLen_Ofs   17
	#define D_Tg_RegConf_TrigVetoLen_Len     10
	#define D_Tg_RegCmd_Init_Ind          27
	#define D_Tg_RegCmd_En_Ind            28
	#define D_Tg_RegCmd_Trig_Ind          29
	#define D_Tg_RegStat_Raedy_Ind        30
	#define D_Tg_RegStat_Enabled_Ind      31

	// Field manipulation macros
	// Config
	#define D_Tg_RegConf_Rate_Get( word )               GetBits(word, D_Tg_RegConf_Rate_Ofs, D_Tg_RegConf_Rate_Len )
	#define D_Tg_RegConf_Rate_Set( word, val )          PutBits(word, D_Tg_RegConf_Rate_Ofs, D_Tg_RegConf_Rate_Len, val )
	#define D_Tg_RegConf_Src_Get(  word )               GetBits(word, D_Tg_RegConf_Src_Ofs,  D_Tg_RegConf_Src_Len )
	#define D_Tg_RegConf_Src_Set(  word, val )          PutBits(word, D_Tg_RegConf_Src_Ofs,  D_Tg_RegConf_Src_Len, val )
	#define D_Tg_RegConf_TrigPipeLen_Get(  word )       GetBits(word, D_Tg_RegConf_TrigPipeLen_Ofs,  D_Tg_RegConf_TrigPipeLen_Len )
	#define D_Tg_RegConf_TrigPipeLen_Set(  word, val )  PutBits(word, D_Tg_RegConf_TrigPipeLen_Ofs,  D_Tg_RegConf_TrigPipeLen_Len, val )
	#define D_Tg_RegConf_TrigVetoLen_Get(  word )       GetBits(word, D_Tg_RegConf_TrigVetoLen_Ofs,  D_Tg_RegConf_TrigVetoLen_Len )
	#define D_Tg_RegConf_TrigVetoLen_Set(  word, val )  PutBits(word, D_Tg_RegConf_TrigVetoLen_Ofs,  D_Tg_RegConf_TrigVetoLen_Len, val )

	#define D_Tg_RegConf_GetConfig(reg) \
		GetBits \
		( \
			reg, \
			D_Tg_RegConf_Rate_Ofs, \
			D_Tg_RegConf_TrigVetoLen_Len+D_Tg_RegConf_TrigPipeLen_Len+D_Tg_RegConf_Rate_Len+D_Tg_RegConf_Src_Len \
		)

	#define D_Tg_RegConf_Set( reg, trig_veto_len, trig_pipe_len, src, rate ) \
		PutBits \
		( \
			reg, \
			D_Tg_RegConf_Rate_Ofs, \
			(D_Tg_RegConf_TrigVetoLen_Len+D_Tg_RegConf_TrigPipeLen_Len+D_Tg_RegConf_Rate_Len+D_Tg_RegConf_Src_Len), \
			(trig_veto_len<<D_Tg_RegConf_TrigVetoLen_Ofs)+(trig_pipe_len<<D_Tg_RegConf_TrigPipeLen_Ofs)+(src<<D_Tg_RegConf_Src_Ofs)+ rate \
		)

	// Default value
	#define D_Def_Tg_Conf D_Tg_RegConf_Set( 0, 0, 1, Tg_Src_ExtSyn, Tg_Rate_One_Hz )

	// Commands
	#define D_Tg_RegCmd_Init_Get( word )    GetBits(word, D_Tg_RegCmd_Init_Ind,  1 )
	#define D_Tg_RegCmd_Init_Clr( word )    ClrBits(word, D_Tg_RegCmd_Init_Ind,  1 )
	#define D_Tg_RegCmd_Init_Set( word )    SetBits(word, D_Tg_RegCmd_Init_Ind,  1 )

	#define D_Tg_RegCmd_En_Get( word )      GetBits(word, D_Tg_RegCmd_En_Ind,  1 )
	#define D_Tg_RegCmd_En_Clr( word )      ClrBits(word, D_Tg_RegCmd_En_Ind,  1 )
	#define D_Tg_RegCmd_En_Set( word )      SetBits(word, D_Tg_RegCmd_En_Ind,  1 )

	#define D_Tg_RegCmd_Trig_Get( word )    GetBits(word, D_Tg_RegCmd_Trig_Ind,  1 )
	#define D_Tg_RegCmd_Trig_Clr( word )    ClrBits(word, D_Tg_RegCmd_Trig_Ind,  1 )
	#define D_Tg_RegCmd_Trig_Set( word )    SetBits(word, D_Tg_RegCmd_Trig_Ind,  1 )

	// Possible command settings
	typedef enum _Tg_Cmd
	{
		Tg_Cmd_Init = D_Tg_RegCmd_Init_Ind,
		Tg_Cmd_En   = D_Tg_RegCmd_En_Ind,
		Tg_Cmd_Trig = D_Tg_RegCmd_Trig_Ind,
	} Tg_Cmd;

	#define D_Tg_RegCmd_Set( reg, cmd ) SetBits(reg, cmd, 1 )
	#define D_Tg_RegCmd_Clr( reg, cmd ) ClrBits(reg, cmd, 1 )

	// Status
	#define D_Tg_RegStat_Ready_Get(   word )  GetBits(word, D_Tg_RegStat_Raedy_Ind,    1 )
	#define D_Tg_RegStat_Enabled_Get( word )  GetBits(word, D_Tg_RegStat_Enabled_Ind,  1 )

	/* Counter register mapped at baseaddress + 4
	-- | TrigCntr |
	-- |   31-0   |
	*/

	// Trigger generator CSR structure and functions
	typedef struct _Tg_Csr
	{
		// Config
		unsigned long config;
		unsigned long counter;
	} Tg_Csr;
	int TgCsr_Init(    Tg_Csr *csr );
	int TgCsr_Sprintf( Tg_Csr *csr, char *buf );

	int Tg_GetCsr( Tg_Csr *csr );
	int Tg_Config( unsigned int config, unsigned int *mem );
	int Tg_SetMemory( char *file_name );
	int Tg_Command( Tg_Cmd cmd, int flipflop );

	// Optional message container
	extern char *tg_msg;

#endif // #ifndef H_TrigGen
