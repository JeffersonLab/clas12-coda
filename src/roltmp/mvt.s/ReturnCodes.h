/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream memory manager testbench
--
-- Module Name:    ReturnCodes.h
-- Description:    Defines function return codes
--
-- Target Devices: Virtex-5 / ML507 development kit
-- Tool versions:  ISE XPS 10.1
-- 
-- Create Date:    0.0 2011/01/28 IM
-- Revision:       
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
#ifndef H_ReturnCodes
#define H_ReturnCodes

// Warning codes
#define D_RetCode_Wrn_Null_Pointer    1
#define D_RetCode_Wrn_SemBusy         2
#define D_RetCode_Wrn_NothingToDo     3

#define D_RetCode_Sucsess 0

// Error codes
#define D_RetCode_Err_Null_Pointer   -1
#define D_RetCode_Err_WrRd_Missmatch -2
#define D_RetCode_Err_FileIO         -3
#define D_RetCode_Err_NetIO          -4
#define D_RetCode_Err_Sem            -5
#define D_RetCode_Err_Wrong_Param    -6
#define D_RetCode_Err_Timeout        -7
#define D_RetCode_Err_DevNotAvail    -8
#define D_RetCode_Err_I2cSpi_NoAck   -9

#endif // #ifndef H_ReturnCodes
