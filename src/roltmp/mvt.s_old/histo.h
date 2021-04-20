/*******************************************************************************
                   ATLAS DEMONSTRATOR "C" PROGRAM
                  ________________________________

 File:        histo.h

 Description: This module declares histogram stucture and functions.
              See histo.c for the functions definitions.

 Author:      D. Calvet,        calvet@hep.saclay.cea.fr
              I. Mandjavidze, mandjavi@hep.saclay.cea.fr

 History:     
*******************************************************************************/

#ifndef _HISTO_H
#define _HISTO_H

#include <stdio.h>
#include <stdlib.h>

/* Define Histogram save format */
typedef enum
{
	HISTO_FORMAT_ABSOLUTE,
	HISTO_FORMAT_NORMALIZED,
	HISTO_FORMAT_PERCENT
} HistoFormat;

typedef struct _Histo
{
	         int     bin_min;
	         int     bin_size;
	         int     nb_of_bins;

	unsigned int     count;
	unsigned int     under;
	unsigned int     over;
	         int     min;
	         int     max;

	         double  sum_x;
	         double  sum_x2;

	unsigned int    *histo;
} Histo;


int  Histo_Init(  Histo *h, int bin_min, int nb_of_bins, int bin_size );
int  Histo_Set(   Histo *h,
                            int bin_min, int nb_of_bins, int bin_size,
                            unsigned int *mem );
int   Histo_Free(  Histo *h );
int   Histo_Clear( Histo *h );
void  Histo_Add(   Histo *h, int value  );
void  Histo_Dump(  Histo *h, FILE *fptr );
void  Histo_DumpFormatted(  Histo *h, HistoFormat format, FILE *fptr );
void  Histo_DumpTail( Histo *h, FILE *fptr );
void  Histo_Stat(  Histo *h, FILE *fptr );
int  Histo_GetAvrStd( Histo *h, double *avr, double *std );

extern int Histo_HtoN(  Histo *from, void *to   );
extern int Histo_NtoH(  Histo *to,   void *from );

#endif /* _HISTO_H */
