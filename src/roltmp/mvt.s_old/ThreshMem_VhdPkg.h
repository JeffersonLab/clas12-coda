/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    
--
-- Module Name:    ThreshMem_VhdPkg.h
-- Description:    Declares functions to Produce VHDL Packahe header and trailer
--                 for the threshold memory
--
-- Target Devices: Windows or Linux PC
-- Tool versions:  Windows Visual C++ or Linux Make
-- 
-- Create Date:    0.0 2011/04/22 IM
-- Revision:       
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifndef H_Th_VhdPkg
#define H_Th_VhdPkg

// Produce Package header
char *Th_VhdPkgHeader
(
	const char    *progname,
	const char    *thresh_type,
	unsigned long  thresh_value,
	unsigned long  thresh_delta,
	unsigned int   nb_of_dreams,
	unsigned int   seed
);
// Produce Package trailer
char *Th_VhdPkgTrailer
(
);

#endif // #ifndef H_Th_VhdPkg

