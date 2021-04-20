/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream memory manager testbench
--
-- Module Name:    Platform.h
-- Description:    Linux specific declarations
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

#include <stdio.h>
#include <libgen.h>

#include <netinet/in.h>
#include <sys/time.h>

#ifndef u16
#define u16 unsigned short
#endif

#ifndef u32
#define u32 unsigned int
#endif

unsigned int Peek( unsigned int adr );
int Poke( unsigned int adr, unsigned int dat );

#define OS_SPEC_STRIP_FORMAT "%2048c"
#define OS_SPEC_NULL_FILE "/dev/null"

char *rootfilename(char *fullfilename);

#endif // #ifndef H_Platform
