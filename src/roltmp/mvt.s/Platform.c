/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream memory manager testbench
--
-- Module Name:    Platform.c
-- Description:    Linux specific functions
--
-- Target Devices: Linux PC
-- Tool versions:  Make gcc
-- 
-- Create Date:    0.0 2011/02/16 IM
-- Revision:       
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
#ifndef H_Platform
#define H_Platform

#include <libgen.h>
#include <string.h>

#ifndef u16
#define u16 unsigned short
#endif

#ifndef u32
#define u32 unsigned int
#endif

#define DEF_PLAT_MAX_FILENAME 256
char file_rootname[DEF_PLAT_MAX_FILENAME];
char *rootfilename(char *fullfilename)
{
	char *dot;
	strcpy(file_rootname, basename(fullfilename) );
	if( (dot=strrchr(file_rootname, '.')) )
		*dot = '\0';
	return file_rootname;
}

#endif // #ifndef H_Platform
