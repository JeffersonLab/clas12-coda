/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    
--
-- Module Name:    PedMem_VhdPkg.h
-- Description:    Declares functions to Produce VHDL Packahe header and trailer
--                 for the pedestal memory
--
-- Target Devices: Windows or Linux PC
-- Tool versions:  Windows Visual C++ or Linux Make
-- 
-- Create Date:    0.0 2011/03/29 IM
-- Revision:       1.0 2012/09/03 IM 
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifndef H_Pd_VhdPkg
#define H_Pd_VhdPkg

// Produce Package header
char *Pd_VhdPkgHeader
(
	const char    *progname,
	const char    *ped_type,
	unsigned long  ped_value,
	unsigned long  ped_delta,
	unsigned int   nb_of_dreams,
	unsigned int   dream_mask,
	unsigned int   nb_of_samples,
	unsigned int   nb_of_events,
	unsigned int   seed,
	unsigned int   mask
);
// Produce Package trailer
char *Pd_VhdPkgTrailer
(
	unsigned long last_address
);

#endif // #ifndef H_Pd_VhdPkg

