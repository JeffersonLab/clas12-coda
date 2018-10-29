/*-----------------------------------------------------------------------------
 * Copyright (c) 2012  Jefferson Science Associates
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 * Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606
 * Email: timmer@jlab.org  Tel: (757) 249-7100  Fax: (757) 269-6248
 *-----------------------------------------------------------------------------
 * 
 * Description:
 *  Code to swap an array containing EVIO composite format
 *  data in place. To be used with the function eviofmt for
 *  analyzing the format string of the composite data.
 *  
 * Author:  Sergey Boiarinov, Hall B
 */
 

#include <stdio.h>

#include "evio.h"
#include "eviofmt.h"

#undef DEBUG

#define MIN(a,b) ( (a) < (b) ? (a) : (b) )

#define SWAP64(x) ( (((x) & 0x00000000000000ff) << 56) | \
                    (((x) & 0x000000000000ff00) << 40) | \
                    (((x) & 0x0000000000ff0000) << 24) | \
                    (((x) & 0x00000000ff000000) <<  8) | \
                    (((x) & 0x000000ff00000000) >>  8) | \
                    (((x) & 0x0000ff0000000000) >> 24) | \
                    (((x) & 0x00ff000000000000) >> 40) | \
                    (((x) & 0xff00000000000000) >> 56) )

#define SWAP32(x) ( (((x) & 0x000000ff) << 24) | \
                    (((x) & 0x0000ff00) <<  8) | \
                    (((x) & 0x00ff0000) >>  8) | \
                    (((x) & 0xff000000) >> 24) )

#define SWAP16(x) ( (((x) & 0x00ff) << 8) | \
                    (((x) & 0xff00) >> 8) )

typedef struct {
    int left;    /* index of ifmt[] element containing left parenthesis */
    int nrepeat; /* how many times format in parenthesis must be repeated */
    int irepeat; /* right parenthesis counter, or how many times format
                    in parenthesis already repeated */
} LV;


/**
 * This function converts (swaps) an array of EVIO composite type data
 * between IEEE (big endian) and DECS (little endian) in place. This
 * data does <b>NOT</b> include the composite type's beginning tagsegment and
 * the format string it contains. It also does <b>NOT</b> include the data's
 * bank header words.
 *
 * Converts the data of array (iarr[i], i=0...nwrd-1)
 * using the format code      (ifmt[j], j=0...nfmt-1) .
 *
 * <p>
 * Algorithm description:<p>
 * Data processed inside while (ib &lt; nwrd)
 * loop, where 'ib' is iarr[] index;
 * loop breaks when 'ib' reaches the number of elements in iarr[]
 *
 *
 * @param iarr    pointer to data to be swapped
 * @param nwrd    number of data words (32-bit ints) to be swapped
 * @param ifmt    unsigned short array holding translated format
 * @param nfmt    length of unsigned short array, ifmt, in # of shorts
 * @param tolocal if 0 data is of same endian as local host,
 *                else data is of opposite endian
 *
 * @return  0 if success
 * @return -1 if nwrd or nfmt arg(s) < 0
 */
int
eviofmtswap(int32_t *iarr, int nwrd, unsigned short *ifmt, int nfmt, int tolocal, int padding)
{
  int i, imt, ncnf, kcnf, mcnf, lev, iterm;
  int64_t *b64, *b64end;
  int32_t *b32, *b32end;
  int16_t *b16, *b16end;
  int8_t  *b8,  *b8end;
  LV lv[10];

  if (nwrd <= 0 || nfmt <= 0)
  {
    printf("ERROR in eviofmtdump: nwrd=%d, nfmt=%d\n",nwrd,nfmt);
    return(0);
  }

  imt   = 0; /* ifmt[] index */
  ncnf  = 0; /* how many times must repeat a format */
  lev   = 0; /* parenthesis level */
  iterm = 0; /*  */

  b8    = (int8_t *)&iarr[0];    /* beginning of data */
  b8end = (int8_t *)&iarr[nwrd] - padding; /* end of data + 1 - padding*/

#ifdef DEBUG
    printf("\n=== eviofmtswap start ===\n");
#endif

  while (b8 < b8end)
  {
#ifdef DEBUG
    printf("+++ begin = 0x%08x, end = 0x%08x\n", b8, b8end);
#endif

    while(1) /* get next format code */
    {
      imt++;
      /* end of format statement reached, back to iterm - last parenthesis or format begining */
      if (imt > nfmt)
      {
		imt = 0/*iterm*/; /* sergey: will always start format from the begining - for now ...*/
#ifdef DEBUG
        printf("1\n");
#endif
      }
      else if (ifmt[imt-1] == 0) /* meet right parenthesis, so we're finished processing format(s) in parenthesis */
      {
        lv[lev-1].irepeat ++; /* increment counter */
        if (lv[lev-1].irepeat >= lv[lev-1].nrepeat) /* if format in parenthesis was processed */
        {                                           /* required number of times */
          iterm = lv[lev-1].left - 1; /* store left parenthesis index minus 1
                                      (if will meet end of format, will start from format index imt=iterm;
                                       by default we continue from the beginning of the format (iterm=0)) */
          lev--; /* done with this level - decrease parenthesis level */
#ifdef DEBUG
          printf("2\n");
#endif
        }
        else /* go for another round of processing by setting 'imt' to the left parenthesis */
        {
          imt = lv[lev-1].left; /* points ifmt[] index to the left parenthesis from the same level 'lev' */
#ifdef DEBUG
          printf("3\n");
#endif
        }
      }
      else
      {
        ncnf = (ifmt[imt-1]>>8)&0x3F; /* how many times to repeat format code */
        kcnf = ifmt[imt-1]&0xFF; /* format code */
        mcnf = (ifmt[imt-1]>>14)&0x3; /* repeat code */

        if(kcnf == 0) /* left parenthesis - set new lv[] */
        {

          /* check repeats for left parenthesis */

          if(mcnf==1) /* left parenthesis, SPECIAL case: #repeats must be taken from int32 data */
          {
            mcnf = 0;
            b32 = (uint32_t *)b8;
            ncnf = *b32 = SWAP32(*b32); /* get #repeats from data */
            /*printf("ncnf(: %d\n",ncnf);*/
            b8 += 4;
#ifdef DEBUG
            printf("ncnf from data = %d (code 15)\n",ncnf);
#endif
          }

          if(mcnf==2) /* left parenthesis, SPECIAL case: #repeats must be taken from int16 data */
          {
            mcnf = 0;
            b16 = (uint16_t *)b8;
            ncnf = *b16 = SWAP16(*b16); /* get #repeats from data */
            /*printf("ncnf(: %d\n",ncnf);*/
            b8 += 2;
#ifdef DEBUG
            printf("ncnf from data = %d (code 14)\n",ncnf);
#endif
          }

          if(mcnf==3) /* left parenthesis, SPECIAL case: #repeats must be taken from int8 data */
          {
            mcnf = 0;
            ncnf = *((uint8_t *)b8); /* get #repeats from data */
            /*printf("ncnf(: %d\n",ncnf);*/
            b8 ++;
#ifdef DEBUG
            printf("ncnf from data = %d (code 13)\n",ncnf);
#endif
          }


          lv[lev].left = imt; /* store ifmt[] index */
          lv[lev].nrepeat = ncnf; /* how many time will repeat format code inside parenthesis */
          lv[lev].irepeat = 0; /* cleanup place for the right parenthesis (do not get it yet) */
          lev++; /* increase parenthesis level */
#ifdef DEBUG
          printf("left parenthesis - set new lv[]\n");
#endif
        }
        else /* format F or I or ... */
        {
          if (lev == 0) /* there are no parenthesis, just simple format, go to processing */
          {
#ifdef DEBUG
            printf("there are no parenthesis, just simple format, go to processing\n");
#endif
            break;
          }
          else if (imt != (nfmt-1)) /* have parenthesis, NOT the pre-last format element (last assumed ')' ?) */
          {
#ifdef DEBUG
            printf("have parenthesis, NOT the pre-last format element: %d %d\n",imt,nfmt-1);
#endif
            break;
          }
          else if (imt != lv[lev-1].left+1)  /* have parenthesis, NOT the first format after the left parenthesis */
          {
#ifdef DEBUG
            printf("have parenthesis, NOT the first format after the left parenthesis: %d %d\n",imt,nfmt-1);
#endif
            break;
          }
          else
          {
            /* if none of above, we are in the end of format */
            ncnf = 999999999; /* set format repeat to the big number */
#ifdef DEBUG
            printf("none of above, we are in the end of format\n");
#endif
            break;
          }
        }
      }
    } /* while(1) */


    /* if 'ncnf' is zero, get it from data */
	/*printf("ncnf=%d\n",ncnf);fflush(stdout);*/
    if(ncnf==0)
    {
      if(mcnf==1)
	  {
        b32 = (int *)b8;
        ncnf = *b32 = SWAP32(*b32);
		/*printf("ncnf32=%d\n",ncnf);fflush(stdout);*/
        b8 += 4;
	  }
      else if(mcnf==2)
	  {
        b16 = (short *)b8;
        ncnf = *b16 = SWAP16(*b16);
		/*printf("ncnf16=%d\n",ncnf);fflush(stdout);*/
        b8 += 2;
	  }
      else if(mcnf==3)
	  {
        ncnf = *b8;
		/*printf("ncnf08=%d\n",ncnf);fflush(stdout);*/
        b8 += 1;
	  }
      /*else printf("ncnf00=%d\n",ncnf);fflush(stdout);*/

#ifdef DEBUG
      printf("ncnf from data = %d\n",ncnf);
#endif
    }


    /* at that point we are ready to process next piece of data; we have following entry info:
         kcnf - format type
         ncnf - how many times format repeated
         b8 - starting data pointer (it comes from previously processed piece)
    */

    /* Convert data in according to type kcnf */
    if(kcnf==8||kcnf==9||kcnf==10) /* 64-bits */
    {
      b64 = (int64_t *)b8;
      b64end = b64 + ncnf;
      if (b64end > (int64_t *)b8end) b64end = (int64_t *)b8end;
      while (b64 < b64end) *b64++ = SWAP64(*b64);
      b8 = (int8_t *)b64;
#ifdef DEBUG
      printf("64bit: %d elements\n",ncnf);
#endif
    }       
    else if(kcnf==1||kcnf==2||kcnf==11||kcnf==12) /* 32-bits */
    {
      b32 = (int32_t *)b8;
      b32end = b32 + ncnf;
      if (b32end > (int32_t *)b8end) b32end = (int32_t *)b8end;
      while (b32 < b32end) *b32++ = SWAP32(*b32);
      b8 = (int8_t *)b32;
#ifdef DEBUG
      printf("32bit: %d elements, b8 = 0x%08x\n",ncnf, b8);
#endif
    }
    else if(kcnf==4||kcnf==5) /* 16 bits */
    {
      b16 = (int16_t *)b8;
      b16end = b16 + ncnf;
      if (b16end > (int16_t *)b8end) b16end = (int16_t *)b8end;
      while (b16 < b16end) *b16++ = SWAP16(*b16);
      b8 = (int8_t *)b16;
#ifdef DEBUG
      printf("16bit: %d elements\n",ncnf);
#endif
    }
    else if(kcnf==6||kcnf==7||kcnf==3) /* 8 bits */
    {
      /* do nothing for characters */
      b8 += ncnf;
#ifdef DEBUG
      printf("8bit: %d elements\n",ncnf);
#endif
    }

  } /* while(b8 < b8end) */

#ifdef DEBUG
  printf("\n=== eviofmtswap end ===\n");
#endif

  return(0);
}
