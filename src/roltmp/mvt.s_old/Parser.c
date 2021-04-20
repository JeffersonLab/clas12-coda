/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    Parser.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "Parser.h"

/***************************************************************/
/* Parse a command line                                        */
/***************************************************************/
int        argc;
ArgvArray  argv;

void parse_line(char *s)
{
	int i = 0;
	int j = 0;

	argc=0;
	while (    (*s != '\0')
			&& (*s != '\n')
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

/********************************************************************/
/****** Extract a word from line ************************************/
/********************************************************************/
char *getword( char *line, char *word )
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
