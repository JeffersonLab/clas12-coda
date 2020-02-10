/*----------------------------------------------------------------------------*
 *  Copyright (c) 1991, 1992  Southeastern Universities Research Association, *
 *                            Continuous Electron Beam Accelerator Facility   *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 * CEBAF Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606 *
 *      heyes@cebaf.gov   Tel: (804) 249-7030    Fax: (804) 249-7363          *
 *----------------------------------------------------------------------------*
 * Discription: follows this header.
 *
 * Author:
 *	Graham Heyes
 *	CEBAF Data Acquisition Group
 *
 *----------------------------------------------------------------------------*/

#define ERROR -1
#define OK 0
#define NOT_TYPE 1

#include <stdio.h>
#include <pthread.h>

#include "rolInt.h"
#include "da.h"
#include "CODA_format.h"
#include "circbuf.h"

char *loadwholefile(char *file, int *size, int *padding);


/* on arrival from ROCs: bit 7 - coda control events, bit 6 - sync bit, bits 0-5 - TI bit pattern */
/* following hack makes TIs softtrig etc types in according to that scheme */
#define SOFT_TRIG_FIX \
  /*printf("befor: desc->type=%d\n",desc->type);*/ \
  /*if(desc->type==253) desc->type=0x3D;*/ /*61*/		   \
  /*if(desc->type==254) desc->type=0x3E;*/ /*62*/		   \
  /*printf("after: desc->type=%d\n",desc->type)*/


extern char confFile[256]; /* defined in coda_ebc.c */

/****************************************************************************************************/
/* _decode_ functions decodes input banks headers and bump 'dabufp' pointers to the first data word */


/* decode fragment header */
int 
CODA_decode_frag(unsigned int **datap, evDesc desc)
{
  desc->length = ((*datap)[0] - 1) << 2;
  desc->evnb   = (*datap)[1] & 0xff;
  /*printf("??? iev=%d\n",desc->evnb);*/
  desc->type   = ((*datap)[1] >> 16) & 0x00ff;
  SOFT_TRIG_FIX;
  desc->fragments[0] = &(*datap)[2];
  desc->bankTag[0] = desc->rocid;
  desc->fragLen[0] = desc->length;
  desc->soe = *datap; /* remember start of event */

  /* desc->user[2] = ((*datap)[1] >> 8) & 0x3f; sergey: store contentType*/
  desc->user[2] = ((*datap)[1] >> 8) & 0xff; /*sergey: store timestamp*/
  desc->user[3] = (((*datap)[1] >> 24) & 0x1) << 6; /*sergey: store syncFlag in bit 6*/
  /*  
  if(desc->user[3]) printf("CODA_decode_frag: syncFlag detected (0x%08x), event nuber %d type 0x%08x\n",
						   desc->user[3],desc->evnb,desc->type);
  */
  *datap += 2; /* CODA headers are two words */

  /* ??? fragment headers have a variable format and are hard to 
     check, we assume here that if the length is resonable 
     then so is the rest of the event. NOTE: limit 0.5MBy.
     */
  if (desc->length < (MAX_EVENT_LENGTH/4))
  {
    return(OK);
  }
  else
  {
    return(ERROR);
  }
}














/* decode special event header */
int 
CODA_decode_spec(unsigned int **datap, evDesc desc)
{
  unsigned int marker;

  desc->length = ((*datap)[0] - 1) << 2;
  desc->type  = ((*datap)[1] >> 16) & 0x00ff;
  SOFT_TRIG_FIX;
  marker = (*datap)[1] & 0xffff;
  desc->time = (*datap)[2];


desc->soe = *datap;


  printf("CODA_decode_spec: len=%d type=%d\n",desc->length,desc->type);

  switch(desc->type)
  {
    case EV_SYNC:
    {
      desc->syncev = (*datap)[3];
      desc->evnb = (*datap)[4];
      desc->err[1] = (*datap)[5];
      break;
    }
 
    case EV_PRESTART: 
    {
      desc->runnb = (*datap)[3];
      desc->runty = (*datap)[4];
      break;
    }

    default: 
    {
      desc->evnb = (*datap)[4];
      break;
    }
  }
  *datap += ((*datap)[0] + 1);

  if(marker == 0x01CC)
  {
    return(OK);
  }
  else
  {
    return(ERROR);
  }  
}






/*****************************************************************/
/* _reserv_ functions remember current 'dabufp' and increment it */

/* reserve space for event header */
int 
CODA_reserv_head(unsigned int **datap, evDesc desc)
{
  /* CODA headers are two int words */

  desc->head_start = *datap;
  *datap +=2;
}

/* reserve space for event 'ID bank' */
int 
CODA_reserv_desc(unsigned int **datap, evDesc desc)
{
  /* CODA ID banks are five int words */

  desc->desc_start = *datap;
  *datap +=5;
}

/* reserve space for */
int 
CODA_reserv_frag(unsigned int **datap, evDesc desc)
{
  /* CODA headers are two int words */

  desc->frag_start = *datap;
  *datap +=2;
}









/*****************************************************************/
/* _encode_ functions ... */


/*
create two banks for special event, for example:

     <event format="evio" count="2" content="bank" data_type="0x10" tag="18" padding="0" num="204" length="7" ndata="5">
        <bank content="uint32" data_type="0x1" tag="18" padding="0" num="204" length="5" ndata="3">
               0x54efcc10              0              0 
        </bank>
     </event>

 */

int 
CODA_encode_spec(unsigned int **datap, evDesc desc)
{
  int len, ii, nw, *ptr;
  char *chbuf;
  int len_in_words;
  int padding;

#ifdef RESTORE_OLD_SPEC_EVENT_CODING
  (*datap)[1] = 0x10CC | ((desc->type & 0x7F) << 16);
  (*datap)[3] = 0x01CC | ((desc->type & 0x7F) << 16);
#else
  (*datap)[1] = 0x10CC | ((desc->type & 0x00ff) << 16);
  (*datap)[3] = 0x01CC | (desc->type << 16);
#endif

  (*datap)[4] = desc->time;


  switch(desc->type)
  {
    case EV_SYNC:
    {
      (*datap)[2] = 5;
      (*datap)[5] = desc->syncev;
      (*datap)[6] = desc->evnb;
      (*datap)[7] = desc->err[1];

      (*datap)[0] = (*datap)[2] + 2;

      break;
    }
 
    case EV_PRESTART: 
    {

      (*datap)[2] = 4;
      (*datap)[5] = desc->runnb;
      (*datap)[6] = desc->runty;

      (*datap)[0] = (*datap)[2] + 2;



#define INSERT_CONFIG_FILE


#ifdef INSERT_CONFIG_FILE

      printf("confFile >%s<\n",confFile);
      if( strncmp(confFile,"none",4) && strncmp(confFile,"NONE",4) )
      {
	    chbuf = loadwholefile(confFile, &len_in_words, &padding);
		printf("chbuf=0x%08x\n",chbuf);
        if(chbuf == NULL)
	    {
          printf("ERROR: coda_roc: cannot read conffile - does not insert it into data stream !!!\n");
	    }
        else
	    {
          /* bank header */
          (*datap)[7] = len_in_words + 1;
          (*datap)[8] = 0xe10e0300; /* bank tag '0xe10e', bank type is 'char', bank number is '0' */
          (*datap)[8] |= (padding&0x3)<<14;

          /* bank data */
          ptr = (int *)chbuf;
		  printf("CODA_encode_spec: len_in_words1=%d\n",len_in_words);fflush(stdout);
          for(ii=0; ii<len_in_words; ii++)
		  {
            /*printf("[%4d] 0x%08x\n",ii,ptr[ii]);*/
            (*datap)[9+ii] = ptr[ii];
		  }
		  printf("CODA_encode_spec: len_in_words2=%d\n",len_in_words);fflush(stdout);
          free(chbuf);
          (*datap)[0] += (len_in_words + 2);
	    }
      }


#endif

	  /* extra data in prestart bank (usually run config file contents 
	  printf("!!!!!!!! CODA_encode_spec: desc->length=%d\n",desc->length);
	  if(desc->length > 12)
	  {
        nw = ((desc->length-12)>>2);

        (*datap)[7] = nw + 1;
        (*datap)[8] = 0x0300;
        for(ii=0; ii<nw; ii++) (*datap)[9+ii] = desc->soe[5+ii];

        (*datap)[0] += nw + 2;

	  }
*/

      break;
    }

    default: 
    {
      (*datap)[2] = 4;
      (*datap)[5] = 0;
      (*datap)[6] = desc->evnb;

      (*datap)[0] = (*datap)[2] + 2;

      break;
    }
  }

  desc->length = ((*datap)[0] + 1)<<2;
  *datap += ((*datap)[0] + 1);
}






/*
create bank-of-banks header for every fragment (one fragment corresponds to one roc), for example:

     <bank content="bank" data_type="0xe" tag="51" padding="0" num="1" length="7752" ndata="7750">

*/
int 
CODA_encode_frag(unsigned int **datap, evDesc desc)
{
  /* CODA event lengths are "non inclusive" i.e. don't include
     the length itself */

  desc->frag_start[0] = *datap - desc->frag_start - 1;
  desc->frag_start[1] = ((desc->rocid & 0xffff) << 16) | 0x0e00 | (desc->evnb & 0xff);
  /*printf("CODA_encode_frag: 0x%08x %x\n",desc->frag_start[1],desc->user[2]);*/
}














/*
     <event format="evio" count="3" content="bank" data_type="0x10" tag="253" padding="0" num="204" length="10520" ndata="10518">
 */
int
CODA_encode_head(unsigned int **datap, evDesc desc)
{
  /* CODA event lengths are "non inclusive" i.e. don't include
     the length itself */

  desc->head_start[0] = *datap - desc->head_start - 1;
#ifdef RESTORE_OLD_SPEC_EVENT_CODING
  desc->head_start[1] = 0x10CC | (( (desc->type & 0x007f) | 0x80 | desc->user[3]) << 16);
  /*
  if(desc->user[3]) printf("CODA_encode_head: syncFlag detected (0x%08x), desc->head_start[1] = 0x%08x\n",
						   desc->user[3],desc->head_start[1]);
  */
#else
  desc->head_start[1] = 0x10CC | ((desc->type & 0x00ff) << 16);
#endif
  desc->length = (*datap - desc->head_start)<<2;
}





/*
   <bank content="uint32" data_type="0x1" tag="49152" padding="0" num="0" length="5" ndata="3">
                 0x1           0x41              0 
   </bank>  
*/
int
CODA_encode_desc(unsigned int **datap, evDesc desc)
{
  /* CODA event lengths are "non inclusive" i.e. don't include
     the length itself */

  desc->desc_start[0] = 4;
  desc->desc_start[1] = 0xC0000100;
  desc->desc_start[2] = desc->evnb;
#ifdef RESTORE_OLD_SPEC_EVENT_CODING
  desc->desc_start[3] = desc->type | 0x80 | desc->user[3];
  /*
  if(desc->user[3]) printf("CODA_encode_desc: syncFlag detected (0x%08x), desc->desc_start[3] = 0x%08x\n",
						   desc->user[3],desc->desc_start[3]);
  */
#else
  desc->desc_start[3] = desc->type;
#endif
  desc->desc_start[4] = desc->err[1];
}
