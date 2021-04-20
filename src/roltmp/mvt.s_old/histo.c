/*******************************************************************************
                   ATLAS DEMONSTRATOR "C" PROGRAM
                  ________________________________

 File:        histo.c

 Description: This module provides historgram functions.
              See histo.h for declarations.

 Author:      D. Calvet,        calvet@hep.saclay.cea.fr
              I. Mandjavidze, mandjavi@hep.saclay.cea.fr

 History:     
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "histo.h"

/* Debug Section */
#define ERR_HISTO          printf
#define DBG_Histo_DumpTail 

int Histo_Init(Histo *h, int bin_min, int nb_of_bins, int bin_size)
{
	/* allocate histo storage */
	if ( (h->histo = (unsigned int *)(malloc (nb_of_bins * sizeof(int)))) == 0)
	{
		return -1;
	}

	h->bin_min    = bin_min;
	h->nb_of_bins = nb_of_bins;
	h->bin_size   = bin_size;

	return( Histo_Clear( h ) );
}

int Histo_Set( Histo *h,
               int bin_min, int nb_of_bins, int bin_size,
               unsigned int *mem )
{
	if ( !mem )
	{
		return -1;
	}

	h->histo = mem;
	h->bin_min    = bin_min;
	h->nb_of_bins = nb_of_bins;
	h->bin_size   = bin_size;

	return( Histo_Clear( h ) );
}

int Histo_Free(Histo *h)
{
	if( !(h->histo) )
	{
		return( 0 );
	}
	free(h->histo);
	h->histo = 0;
	return( 1 );
}

int Histo_Clear( Histo *h )
{
	int i;

	if( !(h->histo) )
	{
		return( -1 );
	}

	for (i=0;i<(h->nb_of_bins);i++)
	{
		*(h->histo + i) = 0;
	}

	h->count  = 0;
	h->under  = 0;
	h->over   = 0;
	h->sum_x  = 0;
	h->sum_x2 = 0;
	h->min    = 0x7FFFffff;
	h->max    = 0x80000000;

	return (0);
}

void Histo_Add(Histo *h, int value)
{
	int ix;

	h->count++;
	h->sum_x  +=  (double)value;
	h->sum_x2 += ((double)value) * ((double)value);
	if( value < h->min )
		h->min = value;
	if( value > h->max )
		h->max = value;

	ix = (value - h->bin_min) / h->bin_size;
//if( h->bin_min == -10 )
//printf("min=%d size=%d nb=%d value=%d ix=%d\n",  h->bin_min, h->bin_size, h->nb_of_bins, value, ix );
	if( ix<0 )
		h->under++;	                /* underflow */
	else if( ix >= h->nb_of_bins )
		h->over++;                  /* overflow  */
	else
		(*(h->histo + ix))++;
//if( h->bin_min == -10 )
//printf("min=%d size=%d nb=%d value=%d ix=%d count=%d\n",  h->bin_min, h->bin_size, h->nb_of_bins, value, ix, (*(h->histo + ix)) );
}

void Histo_DumpFormatted( Histo *h, HistoFormat format, FILE *fptr )
{
	int i, *p;

	fprintf( fptr, "Histo_Dump:" );

	if( h->count == 0 )
	{
		fprintf( fptr, " Histo Empty\n" );
		return;
	}
	else
		fprintf( fptr, "\n" );

	p = (int *) h->histo;

	for(i=0; i<h->nb_of_bins; i++)
	{
		if( *p != 0 )
		{
			if( format == HISTO_FORMAT_NORMALIZED )
			{
				fprintf
				(
					fptr,
					"%d %8.6f\n",
					h->bin_min + h->bin_size * i,
					((double)*p) / (double)h->count
				);
			}
			else if( format == HISTO_FORMAT_PERCENT )
			{
				fprintf
				(
					fptr,
					"%d %6.2f\n",
					h->bin_min + h->bin_size * i,
					100. * ((double)*p)
					     / (double)h->count
				);
			}
			else
			{
				fprintf
				(
					fptr,
					"%d %d\n",
					h->bin_min + h->bin_size * i,
					*p
				);
			}
		}
		p++;
	}

	if( format == HISTO_FORMAT_NORMALIZED )
	{
		fprintf( fptr, "uf %8.6f\n",
			((double)h->under) / (double)h->count );
		fprintf( fptr, "of %8.6f\n",
			((double)h->over) / (double)h->count );
	}
	else if( format == HISTO_FORMAT_PERCENT )
	{
		fprintf( fptr, "uf %6.2f\n",
			((double)h->under) / (double)h->count );
		fprintf( fptr, "of %8.6f\n",
			((double)h->over) / (double)h->count );
	}
	else
	{
		fprintf( fptr, "uf %d\n", h->under );
		fprintf( fptr, "of %d\n", h->over  );
	}
} 

void Histo_Dump( Histo *h, FILE *fptr )
{
	Histo_DumpFormatted( h, HISTO_FORMAT_ABSOLUTE, fptr );
} 

void Histo_DumpTail( Histo *h, FILE *fptr )
{
	int i;
	int min_ix;
	int max_ix;
	int count;

	if( h->count > 0 )
	{
		fprintf(fptr, "Histo_DumpTail:\n");

		min_ix = (h->min - h->bin_min) / h->bin_size;
		if( min_ix <= 0 )
			min_ix=0;	                /* underflow */
		else if( min_ix >= h->nb_of_bins )
			min_ix = h->nb_of_bins;     /* overflow  */
		else
			min_ix -= 1;

		max_ix = (h->max - h->bin_min) / h->bin_size;
		if( max_ix < 0 )
			max_ix=0;	                /* underflow */
		else if( max_ix >= h->nb_of_bins )
			max_ix = h->nb_of_bins;     /* overflow  */

		DBG_Histo_DumpTail( "Histo_DumpTail: min_ix=%d max_ix=%d count=%d\n",
			min_ix, max_ix, h->count );

		count = h->count;
		for( i=min_ix; i<max_ix; i++ )
		{
			DBG_Histo_DumpTail( "Histo_DumpTail: count[%d]=%f\n", i, count );
			count -= (*(h->histo + i));
			fprintf(fptr, "%6d %10.8f\n",
				h->bin_min + h->bin_size * i,
				((double)count) / ((double)h->count) );
		}
	}
} 

void Histo_Stat( Histo *h, FILE *fptr )
{
	int i, *p;
	int quant_1, quant_99, quant;
	int q1_done, q99_done;
	
	fprintf( fptr, "Histo_Stat: Ent = %d",  h->count );
	if( h->count )
	{
		fprintf( fptr, "  Min = %4d", h->min   );
		fprintf( fptr, "  Max = %4d", h->max   );
		fprintf( fptr, "  Und = %4d", h->under );
		fprintf( fptr, "  Ovr = %4d", h->over  );
	
		fprintf( fptr, " Avr = %8.3f", (h->sum_x/h->count) );
		if( h->count > 1 )
		{
			fprintf( fptr, "  Std = %6.2f",
				sqrt( ( h->sum_x2 - (h->sum_x*h->sum_x)/h->count ) / ( h->count - 1 ) ) );

			q1_done  = 0;
			q99_done = 0;
			quant_1  = (int) ( (float)(h->count) * ( 1./100.) );
			quant_99 = (int) ( (float)(h->count) * (99./100.) );
			i        = 0;
			quant    = 0;
			p = (int *) h->histo;
			for(i=0; i<h->nb_of_bins; i++)
			{
				quant+= *p++;
				if( (quant>= quant_1) && (!q1_done) )
				{
					fprintf( fptr, "  1%% = %4d",
						h->bin_min + h->bin_size * i );
					q1_done = 1;
				}
				if( (quant>= quant_99) && (!q99_done) )
				{
					fprintf( fptr, "  99%% = %4d",
						h->bin_min + h->bin_size * i );
					q99_done = 1;
				}
			}
			fprintf( fptr, "\n" );
		}
		else
			fprintf( fptr, "\n" );
	}
	else
		fprintf( fptr, "\n" );
}

int Histo_GetAvrStd( Histo *h, double *avr, double *std )
{
	if( h->count )
	{
		*avr = h->sum_x/h->count;
		if( h->count > 1 )
			*std = sqrt( ( h->sum_x2 - (h->sum_x*h->sum_x)/h->count ) / ( h->count - 1 ) );
		else
			*std = 0.;
	}
	else
	{
		*avr = 0.;
		*std = 0.;
	}
	return h->count;
}