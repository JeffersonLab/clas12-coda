/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    Parser.h
-- Description:    Parses a string
--
-- Target Devices: 
-- Tool versions:  
-- 
-- Create Date:    0.0 2012/09/28 IM
-- Revision:       
--
-- Comments:
--
--------------------------------------------------------------------------------
*/
#ifndef H_Parser
#define H_Parser

#include "Platform.h"

#define LINE_SIZE	256
#define NBMAX_ARGV 	64

typedef char ArgvArray[NBMAX_ARGV][LINE_SIZE];
extern int        argc;
extern ArgvArray  argv;

// Function declarations
void  parse_line(char *s);
char *getword( char *line, char *word );

#endif // #ifndef H_Parser
