
/* helicity.c - delay reporting correction main function */
/*
./Linux_x86_64/bin/helicitytest /data/totape/clas_005584/clas_005584.evio > yyy
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "helicity.h"


/******************************************************************

 g0helicity.c    R. Michaels   July 29, 2002

      modified 6-mar-2006 by Sergey Boyarinov
 This code performs the following tasks as would be

 expected of an analysis code:

   1. Reads the helicity info -- readHelicity()
      In this case the helicity comes from a text file 
      for test purposes, but one can imagine it coming
      from the datastream.

   2. Loads the shift register to calibrate it. 
      The shift register is a word of NBIT bits of 0's and 1's.
      After loading is done, helicity predictions are available
      as global variables like "present_helicity".
        -- loadHelicity(int)
            returns 0  if loading not finished yet
            returns 1  if loading finished (helicity available)

   3. If the helicity is wrong, the code sets up recovery.
      by setting global variable recovery_flag.

  Other routines used:

   ranBit(unsigned int *seed)  -- returns helicity based on seed value
                                 also modifies the seed (arg)
   getSeed()  -- returns seed value based on string of NBIT bits.

******************************************************************/


//#define DEBUG
//#define DEBUG1
//#define DEBUG2

#define NDELAY 2         /* number of quartets between */
						 /* present reading and present helicity  */
						 /* (i.e. the delay in reporting the helicity) */


/* static variables */
#define NBIT 30
static int hbits[NBIT];       /* The NBIT shift register */
static int recovery_flag = 1; /* flag to determine if we need to recover */
						      /* from an error (1) or not (0) */


void
forceRecovery()
{
  recovery_flag = 1;
}


/*************************************************************
 loadHelicity() - loads the helicity to determine the seed.

   on first NBIT calls it just fills an array hbits[NBIT] with
   helicity information from data stream and returns 0;

   on (NBIT+1)th call it does all necessary calculations and
   returns 1 telling main program that it is ready to predict

   so after loading (nb==NBIT), predicted helicities are available

*************************************************************/
int
loadHelicity(int input_helicity, unsigned int *iseed, unsigned int *iseed_earlier)
{
  static int nb;
  unsigned int iseed0, iseed_earlier0;
  int tmp, i;

#ifdef DEBUG
  printf("NB=%d\n",nb);
#endif

  if(recovery_flag) nb = 0;
  recovery_flag = 0;

  if(nb < NBIT)
  {
    hbits[nb] = input_helicity;
    printf("loadHelicity: hbits[%2d]=%d\n",nb,hbits[nb]);
    nb++;
    return(0);
  }
  else if(nb == NBIT) /* Have finished loading */
  {
    /* obtain the seed value from a collection of NBIT bits */
    iseed_earlier0 = getSeed();

    /* get predicted reading (must coinside with current readout) */
    /* skipping 24 times since we read them already ??? */
    /* why (NBIT+1) ?? maybe because of former main() .. */
    for(i=0; i<NBIT/*+1*/; i++)
	{
      tmp = ranBit(&iseed_earlier0);
      printf("loadHelicity: hbits[%d]=%d -> tmp[%2d]=%d\n",i,hbits[i],i,tmp);
	}

    /* get prediction for present helicity */
    iseed0 = iseed_earlier0;
    for(i=0; i<NDELAY; i++) tmp = ranBit(&iseed0);

    printf("iseed_earlier=0x%08x iseed=0x%08x (nb=%d)\n",iseed_earlier0,iseed0,nb);
    nb++; /*?*/

    *iseed = iseed0;
    *iseed_earlier = iseed_earlier0;

    return(1);
  }
}
 

#define NEWRANBIT

#ifndef NEWRANBIT
/*************************************************************
// getSeed
// Obtain the seed value from a collection of NBIT bits.
// This code is the inverse of ranBit.
// Input:
//       int hbits[NBIT]  -- global array of bits of shift register
// Return:
//       seed value
*************************************************************/
unsigned int
getSeed()
{
  int seedbits[NBIT];
  unsigned int ranseed = 0;
  int i;

  if(NBIT != 24)
  {
    printf("getSeed: ERROR: NBIT is not 24.  This is unexpected.\n");
    printf("getSeed: Code failure...\n");
    return(0);
  }

  for(i=0; i<20; i++) seedbits[23-i] = hbits[i];
  seedbits[3] = hbits[20]^seedbits[23];
  seedbits[2] = hbits[21]^seedbits[22]^seedbits[23];
  seedbits[1] = hbits[22]^seedbits[21]^seedbits[22];
  seedbits[0] = hbits[23]^seedbits[20]^seedbits[21]^seedbits[23];

  for(i=NBIT-1; i>=0; i--) ranseed = (ranseed<<1)|(seedbits[i]&1);
  printf("getSeed: ranseed=0x%08x\n",ranseed);

  ranseed = ranseed&0xFFFFFF;

  return(ranseed);
}

/*************************************************************
// This is the random bit generator according to the G0
// algorithm described in "G0 Helicity Digital Controls" by 
// E. Stangland, R. Flood, H. Dong, July 2002.
// Argument:
//        ranseed = seed value for random number. 
//                  This value gets modified.
// Return value:

//        helicity (0 or 1)
*************************************************************/
int
ranBit(unsigned int *ranseed)
{
  static int IB1 = 0x1;           /* Bit 1 */
  static int IB3 = 0x4;           /* Bit 3 */
  static int IB4 = 0x8;           /* Bit 4 */
  static int IB24 = 0x800000;     /* Bit 24 */
  static int MASK;
  unsigned int seed;

  MASK = IB1+IB3+IB4+IB24;
  seed = *ranseed;
  if(seed & IB24)
  {
    seed = ((seed^MASK)<<1) | IB1;
	printf("RANBIT=1 (0x%06x->0x%06x)\n",*ranseed,seed);
    *ranseed = seed;
    return(1);
  }
  else
  {
    seed <<= 1;
	printf("RANBIT=0 (0x%06x->0x%06x)\n",*ranseed,seed);
    *ranseed = seed;
    return(0);
  }
}


/* following function does NOT modifying *ranseed */
int
ranBit0(unsigned int *ranseed)
{
  static int IB1 = 0x1;           /* Bit 1 */
  static int IB3 = 0x4;           /* Bit 3 */
  static int IB4 = 0x8;           /* Bit 4 */
  static int IB24 = 0x800000;     /* Bit 24 */
  static int MASK;
  unsigned int seed;

  MASK = IB1+IB3+IB4+IB24;
  seed = *ranseed;
  if(seed & IB24)
  {    
    seed = ((seed^MASK)<<1) | IB1;
    /**ranseed = seed;*/
    return(1);
  }
  else
  {
    seed <<= 1;
    /**ranseed = seed;*/
    return(0);
  }
}

#else /* NEWRANBIT */

int
ranBit(unsigned int *ranseed)
{
  unsigned int bit7, bit28, bit29, bit30, newbit;
  unsigned int seed = *ranseed;

  bit7   = (seed & 0x00000040) != 0;
  bit28  = (seed & 0x08000000) != 0;
  bit29  = (seed & 0x10000000) != 0;
  bit30  = (seed & 0x20000000) != 0;
  newbit = (bit30 ^ bit29 ^ bit28 ^ bit7) & 0x1;

  if((*ranseed)<=0) newbit = 0;

  seed = ( (seed<<1) | newbit ) & 0x3FFFFFFF;
  *ranseed = seed;

#ifdef DEBUG
  printf("RANBIT=%d (0x%06x->0x%06x)\n",newbit,*ranseed,seed);
#endif

  return(newbit);
}

int
ranBit0(unsigned int *ranseed)
{
  unsigned int bit7, bit28, bit29, bit30, newbit;
  unsigned int seed;

  bit7   = ((*ranseed) & 0x00000040) != 0;
  bit28  = ((*ranseed & 0x08000000)) != 0;
  bit29  = ((*ranseed & 0x10000000)) != 0;
  bit30  = ((*ranseed & 0x20000000)) != 0;
  newbit = (bit30 ^ bit29 ^ bit28 ^ bit7) & 0x1;

  if((*ranseed)<=0) newbit = 0;
  /*
  *ranseed = ( ((*ranseed)<<1) | newbit ) & 0x3FFFFFFF;
  */
  return(newbit);
}



/* Back track the seed by 30 samples */
unsigned int
getSeed()
{
  int i;
  unsigned int seed;
  unsigned int bit1, bit8, bit29, bit30, newbit30;
  int seedbits[NBIT];

  if(NBIT != 30)
  {
    printf("getSeed: ERROR: NBIT is not 30.  This is unexpected.\n");
    printf("getSeed: Code failure...\n");
    exit(0);
  }

  seed = 0;
  for(i=0; i<30; i++)
  {
    printf("getSeed: hbits[%d]=%d\n",i,hbits[i]);
    if(hbits[i]==1) seed = ((seed<<1) | 1) & 0x3FFFFFFF;
    else            seed = (seed<<1) & 0x3FFFFFFF;
  }

  printf("getSeed: seed=0x%08x\n",seed);

  for(i=0; i<30; i++)
  {
    bit1    = (seed & 0x00000001) != 0;
    bit8    = (seed & 0x00000080) != 0;
    bit29   = (seed & 0x10000000) != 0;
    bit30   = (seed & 0x20000000) != 0;
    
    newbit30 = (bit30 ^ bit29 ^ bit8 ^ bit1) & 0x1;

    seed = (seed >> 1) | (newbit30<<29);
  }

  return(seed);
}


#endif /* NEWRANBIT */







#include "evio.h"
#include "evioBankUtil.h"

#define NPREV 3
#define NFLIP 5
#define NSCAL 4
#define MIN(a,b)  ( (a) < (b) ? (a) : (b) )

/*#define DELTATIME 8457488LL*/ /* 30Hz */
#define DELTATIME 2083336LL /* 120Hz */

/********************************************************************************/
/*                           HALLB SECTION                                      */
/********************************************************************************/

int
helicity(unsigned int *bufptr, int type)
{
  int i, j, ii, ind10, ind12, ind13, ncol, nrow;
  static unsigned int hel, count1, qd, strob, helicity, quad, strob1, helicity1, quad1;
  static unsigned int offset, quadextr[4];
  static int present_reading;
  static int skip = 0;
  static int final_helicity;
  static int remember_helicity[3];

  static int predicted_reading;      /* prediction of present reading */
  static int present_helicity;       /* present helicity (using prediction) */
  static unsigned int iseed;         /* value of iseed for present_helicity */
  static unsigned int iseed_earlier; /* iseed for predicted_reading */

  /* following for 1 flip correction */
  static unsigned int offset1;
  static int pred_read;
  static int pres_heli;
  static int tmp0, temp0;
  static int str_pipe[NPREV];
  static int hel_pipe[NPREV];
  static int quad_pipe[NPREV];
  static uint64_t time_pipe[NPREV];

  GET_PUT_INIT;
  int fragtag, fragnum, banktag, banknum, ind, nbytes, ind_data, itmp;
  int slot, trig, chan, nchan, ievent, iev, nn, mm, nsamples;
  unsigned char *end;
  uint64_t fadctime, tstime, dtstime;
  uint64_t timestamp;
  uint64_t deltatime;
  int64_t dtime;
  static int64_t dtime_avg;
  static int Navg = 1;
  float missed_strobs;
  static uint64_t timestamp_old, tstime_old;
  uint32_t tstime1, tstime2;
  unsigned int dat32;
  unsigned short dat16, *place_for_processed, *place_for_helicity;
  int datasaved[1000];

  static int nquads;
  static int str;
  static int done;
  static int nevent;
  static int missed_helicity_triggers;

  /* do it in the begining of the run */
  if(type==17||type==18)
  {
    FORCE_RECOVERY;
    nevent = 0;
	missed_helicity_triggers = 0;
    printf("TYPE=%d -> calls FORCE_RECOVERY and set nevent=%d\n",type,nevent);fflush(stdout);
    return(0);
  }


  nevent ++;

#ifdef DEBUG
  printf("\nBUFFER (type=%d):\n",type);
  for(i=0; i<50; i++) printf("  0x%08x (%d)\n",bufptr[i],bufptr[i]);
  printf("\n");
#endif

  /*************/
  /* head bank */

  fragtag = 37;
  fragnum = -1;
  banktag = 0xe10a;
  banknum = 0;

  ind = 0;
  for(banknum=0; banknum<40; banknum++)
  {
    /*printf("looking for %d %d  - 0x%04x %d\n",fragtag, fragnum, banktag, banknum);*/
    ind = evLinkBank(bufptr, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data);
    if(ind>0) break;
  }

  if(ind<=0)
  {
    printf("ERROR: cannot find HEAD bank (fragtag=%d tag=%d)\n",fragtag,banktag);
    ievent = 0;
    /*return(0);*/
  }
  else
  {
    b08 = (unsigned char *) &bufptr[ind_data];
    GET32(itmp);
    GET32(ievent);

    GET32(tstime1);
    GET32(tstime2);

    tstime = ((uint64_t)tstime2<<32) | (uint64_t)tstime1;
    /*printf("tstime2=0x%06x tstime1=0x%06x -> tstime=0x%llx\n",tstime2,tstime1,tstime);*/

    tstime += 4LL; /* FADC timestamp 4 ticks bigger, we will compare them */

    GET32(itmp);
    GET32(itmp);
    if(itmp==0x1000)
	{
      missed_helicity_triggers = 0;
      dtstime = tstime - tstime_old;
#ifdef DEBUG1
	  /*printf("+++++> 0x%llx - 0x%llx = 0x%llx\n",tstime,tstime_old,dtstime);*/
      printf("TRIGGER BIT 0x1000 TIMESTAMP: event=%d, fp_trig=0x%08x tstime=%lld(0x%llx) dtstime=%lld(0x%llx)\n",ievent,itmp,tstime,tstime,dtstime,dtstime);
#endif
      tstime_old = tstime;
	}
 }




  /**********************/
  /* helicity fadc bank */

  helicity = 0;
  strob = 0;
  quad = 0;
  place_for_processed = 0;
  place_for_helicity = 0;
  timestamp = 0LL;


  fragtag = 19;
  fragnum = -1;
  banktag = 0xe101;
  banknum = 0;

  ind = 0;
  for(banknum=0; banknum<40; banknum++)
  {
    /*printf("looking for %d %d  - 0x%04x %d\n",fragtag, fragnum, banktag, banknum);*/
    ind12 = evLinkBank(bufptr, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data);
    if(ind12>0) break;
  }

  if(ind12<=0)
  {
    printf("ERROR: cannot find FADC bank (fragtag=%d banktag=%d)\n",fragtag,banktag);
    return(0);
  }
  else
  {
    b08 = (unsigned char *) &bufptr[ind_data];
    end = b08 + nbytes;

    while (b08 < end)
    {
#ifdef DEBUG
	  printf("fadcs: begin while: b08=0x%08x\n",b08);fflush(stdout);
#endif

	  GET8(slot);
	  GET32(trig);
	  GET64(fadctime); /* time stamp for FADCs 2 counts bigger then for VTPs */

#ifdef DEBUG
	  printf("fadctime=%lld(0x%llx)\n",fadctime,fadctime);fflush(stdout);
#endif

#if 0
	  fadctime = ((fadctime & 0xFFFFFF) << 24) | ((fadctime >> 24) & 0xFFFFFF); /* UNTILL FIXED IN ROL2 !!!!!!!!!!!!!!!!! */
#endif

	  iev = trig;
	  if(slot==19)
	  {
        timestamp = fadctime;
#ifdef DEBUG
        printf("++timestamp=0x%llx(%llu) (event=%d)\n",timestamp,timestamp,ievent);
#endif
	  }

	  GET32(nchan);
#ifdef DEBUG
	  printf("slot=%d, trig=%d, time=%lld(0x%llx) nchan=%d\n",slot,trig,fadctime,fadctime,nchan);fflush(stdout);
#endif

	  for(nn=0; nn<nchan; nn++)
      {
	    GET8(chan);
		GET32(nsamples);
#ifdef DEBUG
		printf("==> slot=%d, chan=%d, nsamples=%d\n",slot,chan,nsamples);fflush(stdout);
#endif

		/*save data*/
		for (mm=0; mm<nsamples; mm++)
        {
          if(slot==19&&chan==0)
		  {
            if(mm==mm-2)      place_for_processed = (unsigned short *)b08;
            else if(mm==mm-1) place_for_helicity = (unsigned short *)b08;
		  }
		  GET16(dat16);
		  datasaved[mm] = dat16;
		}

#ifdef DEBUG
		printf("data[sl=%2d][ch=%2d]:\n",slot,chan);
		for(mm=0; mm<nsamples; mm++)
		{
		  printf(" [%2d]%4d,",mm,datasaved[mm]);
          /*if(((mm+1)%10)==0) printf("\n");*/
		}
        printf("\n");
#endif		


        /*helicity info: slot 19, channels 0,2,4*/
        if(slot==19&&chan==0&&dat16>1000) helicity = 1;
        if(slot==19&&chan==2&&dat16>1000) strob = 1;
        if(slot==19&&chan==4&&dat16>1000) quad = 1;
        /*helicity info: slot 19, channels 0,2,4*/


	  } /*for(nn=0; nn<nchan; nn++)*/

#ifdef DEBUG
	  printf("fadcs: end loop: b08=0x%08x\n",b08);fflush(stdout);
#endif
    }

    /* update pipelines */
    for(ii=0; ii<NPREV-1; ii++)
    {
      str_pipe[ii] = str_pipe[ii+1];
      hel_pipe[ii] = hel_pipe[ii+1];
      quad_pipe[ii] = quad_pipe[ii+1];
      time_pipe[ii] = time_pipe[ii+1];
#ifdef DEBUG
      printf("-----> str_pipe[%2d]=%d hel_pipe=%d quad_pipe=%d time=0x%llx\n",ii,str_pipe[ii],hel_pipe[ii],quad_pipe[ii],time_pipe[ii]);
#endif
    }
    str_pipe[NPREV-1] = strob;
    hel_pipe[NPREV-1] = helicity;
    quad_pipe[NPREV-1] = quad;
    time_pipe[NPREV-1] = timestamp;
#ifdef DEBUG
    printf("--=--> str_pipe[%2d]=%d hel_pipe=%d quad_pipe=%d time=0x%llx\n",NPREV-1,str_pipe[NPREV-1],hel_pipe[NPREV-1],quad_pipe[NPREV-1],time_pipe[NPREV-1]);
#endif

  }




  if(done==1)
  {

    /********************************/
    /* CHECK IF WE MISSED ANY FLIPS */

    /* 'tstime_old' is timestamp for last received helicity strob trigger (front panel bit 0x1000) */

    deltatime = timestamp - tstime_old;
#ifdef DEBUG1
    printf("!!! timestamp=0x%llx(%llu) - tstime=0x%llx(%llu) = deltatime=0x%llx(%llu),  DELTATIME=0x%llx(%llu)\n",
                timestamp,timestamp,tstime_old,tstime_old,deltatime,deltatime,DELTATIME,DELTATIME);
#endif
    if((timestamp>0) && (tstime_old>0))
	{
	  if(deltatime > DELTATIME)
      {
        missed_helicity_triggers ++;
#ifdef DEBUG1
        printf("\nMISSED helicity trigger %d times ??? (event=%d) (timestamp=0x%llx(%llu)) [deltatime=0x%llx(%llu) > DELTATIME=0x%llx(%llu)]\n",
			   missed_helicity_triggers,ievent,timestamp,timestamp,deltatime,deltatime,DELTATIME,DELTATIME);
        printf("Event %d SHOULD be in next helicity interval, expecting to see flip from FADC information\n",ievent);
#endif

        tstime = tstime_old + DELTATIME; /* bump helicity trigger timestamp, emulating what it should be if we did not miss it */

        dtstime = tstime - tstime_old;
#ifdef DEBUG1
	    printf("++-++> tstime=0x%llx - tstime_old=0x%llx = dtstime=0x%llx\n",tstime,tstime_old,dtstime);
        printf("HEAD: tstime=%lld(0x%llx) dtstime=%lld\n",tstime,tstime,dtstime);
#endif
        tstime_old = tstime;

      }
	}

  /********************************/
  /********************************/

  }




  /******************* we are here for FLIP only ***********************/

  if(str_pipe[NPREV-2]==-1) return(0); /*skip first event, we need at least two events to check for flip */





  /*if any value changed, we flipped 
  if((strob!=str_pipe[NPREV-2])||(helicity!=hel_pipe[NPREV-2])||(quad!=quad_pipe[NPREV-2]))*/

  /*
missed_strobs = 0.989922

== str/hel/quad befo: str=0 hel=1 quad=0 (event 20800) timestamp=0x63e1ab602
== str/hel/quad afte: str=0 hel=0 quad=1 (event 20801) timestamp=0x63e1abd67 (dtime=0x7fbab4, dtime_avg=0x810765)

missed_strobs = 0.000118

== str/hel/quad befo: str=0 hel=0 quad=1 (event 20801) timestamp=0x63e1abd67
== str/hel/quad afte: str=1 hel=0 quad=1 (event 20802) timestamp=0x63e1ac14a (dtime=0x3e3, dtime_avg=0x80f355)

missed_strobs = 1.023029

== str/hel/quad befo: str=1 hel=0 quad=1 (event 20898) timestamp=0x63e9bc313
== str/hel/quad afte: str=0 hel=0 quad=1 (event 20899) timestamp=0x63e9eacd5 (dtime=0x83eb8b, dtime_avg=0x80f3cb)

  ERROR: reading=1 predicted=0 (event 20899)
missed_strobs = 0.980884
  */






  /* we flipped only if strob changed */
  if((strob!=str_pipe[NPREV-2]))
  {
    dtime = timestamp-timestamp_old;

    if(dtime_avg > 0.0) missed_strobs = (float)dtime/(float)dtime_avg;
    else                missed_strobs = 0LL;
    printf("missed_strobs = %f\n",missed_strobs);
    if( missed_strobs > 1.5 )
	{
      printf("IT SEEMS WE MISSED ABOUT %f HELICITY STROB(S)\n",missed_strobs);

      for(ii=0; ii<(int)missed_strobs/4; ii++)
	  {
        predicted_reading = ranBit(&iseed_earlier);
        present_helicity = ranBit(&iseed);
	  }
	}

    dtime_avg = dtime_avg + (dtime-dtime_avg)/Navg;
    Navg ++;
#ifdef DEBUG2
    printf("\n== str/hel/quad befo: str=%d hel=%d quad=%d (event %d) timestamp=0x%llx\n",
		   str_pipe[NPREV-2],hel_pipe[NPREV-2],quad_pipe[NPREV-2],ievent-1,time_pipe[NPREV-2]);
    printf("== str/hel/quad afte: str=%d hel=%d quad=%d (event %d) timestamp=0x%llx (dtime=0x%llx, dtime_avg=0x%llx)\n\n",
           strob,helicity,quad,ievent,timestamp,dtime,dtime_avg);
#endif


    timestamp_old = timestamp;









	/*
    if((dtime<8300000)||(dtime>8600000))
	{
      printf("ERROR: dtime=%llu - recovering\n",dtime);
      FORCE_RECOVERY;
      return(0);
	}
	*/




	/*use event after flip */
    strob1 = strob;
    helicity1 = helicity;
    quad1 = quad;
	


	/*flip helicity*/
    helicity1 ^= 1;
	

    /* if we sync'ed already, check offset and bump it if necessary */
    if(done==1)
    {
#ifdef DEBUG
      printf("  checking offset=%d: strob1=%1d helicity1=%1d quad1=%d\n",offset,strob1,helicity1,quad1);
#endif
      if(quad1==0)
      {
        /* if we are here, 'offset' must be 3, otherwise most likely
        we've got unexpected quad1==0; we'll try to set quad1=1 and
        see what happens */
        if(offset<3)
        {
          printf("    ERROR: unexpected offset=%d - trying to recover --------------------\n",offset);
          quad1 = 1;
          offset++;
          printf("       WARN: bump[1] offset to %d\n",offset);
	      /*FORCE_RECOVERY;*/
	    }
        else if(offset>3)
        {
          printf("    ERROR: unexpected offset=%d - forcing recovery 1 -------------------\n",offset);
	      FORCE_RECOVERY;
        }
        else
        {
          offset=0;
#ifdef DEBUG
          printf("    INFO: set[1] offset to %d\n",offset);
#endif
	    }
      }
      else
      {
        offset++;
#ifdef DEBUG
        printf("    INFO: bump[2] offset to %d\n",offset);
#endif
        if(offset>3)
        {
          /* if we are here, 'offset' must be <=3, otherwise most likely
          we've got unexpected quad1==1; we'll try to set quad1=0, offset=0
          and see what happens */
          printf("    ERROR: unexpected offset=%d - forcing recovery 2 -------------------\n",offset);
          quad1 = 0;
          offset = 0;
          printf("       WARN: set[1] offset to %d and quad1 to %d\n",offset,quad1);
	      /*FORCE_RECOVERY;*/
        }
      }
    }

#ifdef DEBUG
    printf("  checking offset=%d done: strob1=%d helicity1=%1d quad1=%1d\n",offset,strob1,helicity1,quad1);
#endif








    /* looking for the beginning of quartet; set done=0 when found */
    if(done==-1)
    {
      printf("done=%d -> looking for the begining of quartet ..\n",done);
      Navg = 1;
      dtime_avg = 0LL;
      if(quad1==0)
      {
        done=0; /* quad1==0 means first in quartet */
        printf("found the begining of quartet !\n");
	  }
    }

    /* search for pattern: use first 24 quartets (not flips!) to get all necessary info */
    if(done==0)
    {
      printf("done=%d -> loading ..\n",done);
      if(quad1==0)
	  {
        done = loadHelicity(helicity1, &iseed, &iseed_earlier); /* 24 times returns 0, on 25th call returns 1 */
        printf("call loadHelicity, loaded one, done=%d\n",done);
      }

      if(done==1)
      {
        printf("=============== READY TO PREDICT (ev=%6d) =============== \n",ievent);
        Navg = 1;
        dtime_avg = 0LL;
	  }
    }






    if(done==1)
    {
#ifdef DEBUG
      printf("PATTERN WAS FOUND BEFORE, can determine helicity: done=%d quad1=%d\n",done,quad1);
#endif
      if(quad1==0)
      {

        /* following two must be the same */
        present_reading = helicity1;
        predicted_reading = ranBit(&iseed_earlier);

#ifdef DEBUG
        printf("  quad1=%d, ranBit(&iseed_earlier) returns %d\n",quad1,predicted_reading);
#endif

        present_helicity = ranBit(&iseed);
	        
#ifdef DEBUG
        printf("  helicity: predicted=%d present=%d corrected=%d\n",predicted_reading,present_reading,present_helicity);
#endif



        /****************/
        /****************/
        /* direct check */
		/*SERGEY: TEMPORARY !!!
        remember_helicity[0] = remember_helicity[1];
        remember_helicity[1] = remember_helicity[2];
        remember_helicity[2] = present_helicity;
#ifdef DEBUG
        printf("  ============================== %1d %1d\n",remember_helicity[0],present_reading);
#endif
        if( remember_helicity[0] != -1 )
		{
          if( remember_helicity[0] != present_reading )
          {
            printf("  ERROR: direct check failed !!! %1d %1d\n",remember_helicity[0],present_reading);
            FORCE_RECOVERY;
          }
		}
		*/
        /****************/
        /****************/



		/*SERGEY: TEMPORARY !!!!!!!!!!!!!
        if(predicted_reading != present_reading)
        {
          printf("  ERROR !!!!!!!!! predicted_reading=%d != present_reading=%d\n",predicted_reading,present_reading);
          FORCE_RECOVERY;
        }
		*/


      }
#ifdef DEBUG
      else
	  {
        printf("  quad1=%d - do nothing\n",quad1);
	  }
#endif

    }

  }

  /******************* we are here for FLIP only ***********************/








  /*******************************/
  /* we are here for every event */

  if(done==1)
  {
#ifdef DEBUG
    printf("CHECKING IF WE STILL IN SYNC\n");		  
#endif
    /* first check, if we are still in sync */
    tmp0 = helicity1; /* helicity from current event */

    pred_read = predicted_reading;
    pres_heli = present_helicity;
#ifdef DEBUG
    printf("-------------------------------------------------------- offset=%d -> pred_read=%d\n",offset,pred_read);
#endif
    if((offset==0) || (offset==3))
    {
	  
      temp0 = pred_read;
      final_helicity = pres_heli^1;
    }
    else if((offset==1) || (offset==2))
    {	  	  
      temp0 = pred_read^1;
      final_helicity = pres_heli;
    }
    else
    {
      printf("  ERROR: illegal offset1=%d at temp0\n",offset1);
      FORCE_RECOVERY;
    }



    final_helicity ^= 1; /* flip it back */

	  

#ifdef DEBUG
    printf("  tmp0=%d temp0=%d final=%d\n",tmp0,temp0,final_helicity);
#endif  

    if(tmp0 != temp0)
	{
      printf("  ERROR: reading=%d predicted=%d (event %d)\n",tmp0,temp0,ievent);
      FORCE_RECOVERY;
	}
    else
	{
#ifdef DEBUG
      printf("  INFO: reading=%d predicted=%d (event %d)\n",tmp0,temp0,ievent);
#endif


#if 1

      fragtag = 37;
      fragnum = -1;
      banktag = 0xe10f;
      banknum = 0;

      ind = 0;
      for(banknum=0; banknum<40; banknum++)
      {
        /*printf("looking for %d %d  - 0x%04x %d\n",fragtag, fragnum, banktag, banknum);*/
        ind = evLinkBank(bufptr, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data);
        if(ind>0) break;
      }

      if(ind<=0)
      {
        printf("ERROR: cannot find HEAD bank (fragtag=%d)\n",fragtag);
        ievent = 0;
        /*return(0);*/
      }
      else
      {
        b08 = (unsigned char *) &bufptr[ind_data];

        /* skip first 5 words in HEAD bank */
        GET32(itmp);
        GET32(itmp);
        GET32(itmp);
        GET32(itmp);
        GET32(itmp);

        /* put results into 6th 32bit word */
        itmp = itmp | (final_helicity<<1) | 1;
#ifdef DEBUG
        printf("put e10f: %d\n",itmp);
#endif
        b08out = b08;
        PUT32(itmp);

        /* read 6th 32bit word */
        GET32(itmp);
#ifdef DEBUG
        printf("get e10f: %d\n",itmp);
#endif
      }

#endif




	}

  }


  return(0);
}












/********************************************************************************/
/********************************************************************************/
/* hdice version */
/********************************************************************************/
/********************************************************************************/

#define DELTATIMEHDICE 8457488LL /* 30Hz */  /* 8330109 */
/*#define DELTATIMEHDICE 2083336LL*/ /* 120Hz */

int
helicity_hdice(unsigned int *bufptr, int type)
{
  int i, j, ii, jj, ind10, ind12, ind13, ncol, nrow;
  static unsigned int hel, count1, qd, strob, helicity, quad, strob1, helicity1, quad1;
  static unsigned int offset, quadextr[4];
  static int present_reading;
  static int skip = 0;
  static int final_helicity;
  static int remember_helicity[3];

  static int scaler_quad, scaler_helicity;

  static int predicted_reading;      /* prediction of present reading */
  static int present_helicity;       /* present helicity (using prediction) */
  static unsigned int iseed;         /* value of iseed for present_helicity */
  static unsigned int iseed_earlier; /* iseed for predicted_reading */

  /* following for 1 flip correction */
  static unsigned int offset1;
  static int pred_read;
  static int pres_heli;
  static int tmp0, temp0;

  /* pipeline of the all events with FADC bank */
  static int str_pipe[NPREV];
  static int hel_pipe[NPREV];
  static int quad_pipe[NPREV];
  static uint64_t time_pipe[NPREV];

  /* pipeline of the events where helicity fliped in FADC bank */
  static int pipequad[NFLIP], pipehel[NFLIP], pipecorr[NFLIP];

  /* pipeline of the scaler events */
  static int scalquad[NSCAL], scalhel[NSCAL];

  GET_PUT_INIT;
  int fragtag, fragnum, banktag, banknum, ind, nbytes, nw, ind_data, itmp;
  unsigned int itemp;
  int slot, trig, chan, nchan, ievent, iev, nn, mm, nsamples;
  unsigned char *end;
  uint64_t fadctime, tstime, dtstime;
  uint64_t timestamp;
  uint64_t deltatime;
  int64_t dtime;
  static int64_t dtime_avg;
  static int Navg = 1;
  float missed_strobs;
  static uint64_t timestamp_old, tstime_old;
  uint32_t tstime1, tstime2, tstime22;
  unsigned int dat32;
  unsigned short dat16, *place_for_processed, *place_for_helicity;
  int datasaved[1000];

  static int nquads;
  static int str;
  static int done;
  static int nevent;
  static int missed_helicity_triggers;

  /* do it in the begining of the run */
  if(type==17||type==18)
  {
    FORCE_RECOVERY;
    nevent = 0;
	missed_helicity_triggers = 0;
    printf("TYPE=%d -> calls FORCE_RECOVERY and set nevent=%d\n",type,nevent);fflush(stdout);
    return(0);
  }


  nevent ++;

#ifdef DEBUG
  printf("\nBUFFER (type=%d):\n",type);
  for(i=0; i<50; i++) printf("  0x%08x (%d)\n",bufptr[i],bufptr[i]);
  printf("\n");
#endif

  /*************/
  /* head bank */

  fragtag = 82/*37*/;
  fragnum = -1;
  banktag = 0xe10a;
  banknum = 0;

  ind = 0;
  for(banknum=0; banknum<40; banknum++)
  {
    /*printf("looking for %d %d  - 0x%04x %d\n",fragtag, fragnum, banktag, banknum);*/
    ind = evLinkBank(bufptr, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data);
    if(ind>0) break;
  }

  if(ind<=0)
  {
    printf("ERROR: cannot find HEAD bank (fragtag=%d tag=%d)\n",fragtag,banktag);
    ievent = 0;
    /*return(0);*/
  }
  else
  {
    b08 = (unsigned char *) &bufptr[ind_data];
    GET32(itmp);
    GET32(ievent);

    GET32(tstime1);
    GET32(tstime2);

    tstime2 = tstime2 & 0xFFFF; /* drop unrelated parts ([31:20] - trigger code received as the trigger(the code is decoded as trigger),
                                                         [19:16] - event number bits [35:32]) */

    tstime = ((uint64_t)tstime2<<32) | (uint64_t)tstime1;
    //printf("2: ievent=%d tstime2=0x%08x tstime1=0x%08x -> tstime=0x%llx\n",ievent,tstime2,tstime1,tstime);

    tstime += 4LL; /* FADC timestamp 4 ticks bigger, we will compare them */

    /*GET32(itmp);*/
    GET32(itmp); /*helicity <strob?> must be latched here !!!*/
    //printf("itmp1=%d (0x%08x)\n",itmp,itmp);
    itmp = itmp & 0x3F;
    //printf("itmp2=%d (0x%08x)\n",itmp,itmp);
    if(itmp==0x20) /* bit 6, signal is in TS#6 */
	{
      missed_helicity_triggers = 0;
      dtstime = tstime - tstime_old;
#ifdef DEBUG1
	  /*printf("+++++> 0x%llx - 0x%llx = 0x%llx\n",tstime,tstime_old,dtstime);*/
      printf("TRIGGER BIT 0x1000 TIMESTAMP: event=%d, fp_trig=0x%08x tstime=%lld(0x%llx) dtstime=%lld(0x%llx)\n",ievent,itmp,tstime,tstime,dtstime,dtstime);
#endif
      tstime_old = tstime;
	}
 }




  /**********************/
  /* helicity fadc bank */

  helicity = 0;
  strob = 0;
  quad = 0;
  place_for_processed = 0;
  place_for_helicity = 0;
  timestamp = 0LL;


  fragtag = 82/*19*/;
  fragnum = -1;
  banktag = 0xe101;
  banknum = 0;

  ind = 0;
  for(banknum=0; banknum<40; banknum++)
  {
    /*printf("looking for %d %d  - 0x%04x %d\n",fragtag, fragnum, banktag, banknum);*/
    ind12 = evLinkBank(bufptr, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data);
    if(ind12>0) break;
  }

  if(ind12<=0)
  {
    printf("ERROR: cannot find FADC bank (fragtag=%d banktag=%d)\n",fragtag,banktag);
    return(0);
  }
  else
  {
    b08 = (unsigned char *) &bufptr[ind_data];
    end = b08 + nbytes;

    while (b08 < end)
    {
#ifdef DEBUG
	  printf("fadcs: begin while: b08=0x%08x\n",b08);fflush(stdout);
#endif

	  GET8(slot);
	  GET32(trig);
	  GET64(fadctime); /* time stamp for FADCs 2 counts bigger then for VTPs */

#ifdef DEBUG2
	  printf("fadctime=%lld(0x%llx)\n",fadctime,fadctime);fflush(stdout);
#endif

#if 0
	  fadctime = ((fadctime & 0xFFFFFF) << 24) | ((fadctime >> 24) & 0xFFFFFF); /* UNTILL FIXED IN ROL2 !!!!!!!!!!!!!!!!! */
#endif

	  iev = trig;

	  /*
       helicity signals in FADC slot 7:
           channel 3 - T-SETTLE (STROB 30Hz)
           channel 2 - PAT SYNC (QUAD)
           channel 1 - PAIR SYNC (STROB 15Hz)
           channel 0 - DLY RPT (HELICITY)
	   */

	  if(slot==7)
	  {
        timestamp = fadctime;
#ifdef DEBUG2
        printf("++timestamp=0x%llx(%llu) (event=%d)\n",timestamp,timestamp,ievent);
#endif
	  }

	  GET32(nchan);
#ifdef DEBUG2
	  printf("SLOT=%d, trig=%d, time=%lld(0x%llx) nchan=%d\n",slot,trig,fadctime,fadctime,nchan);fflush(stdout);
#endif

	  for(nn=0; nn<nchan; nn++)
      {
	    GET8(chan);
		GET32(nsamples);
#ifdef DEBUG
		printf("==> slot=%d, chan=%d, nsamples=%d\n",slot,chan,nsamples);fflush(stdout);
#endif


		/*save data*/
		for (mm=0; mm<nsamples; mm++)
        {
          if(slot==7&&chan==0)
		  {
            if(mm==mm-2)      place_for_processed = (unsigned short *)b08;
            else if(mm==mm-1) place_for_helicity = (unsigned short *)b08;
		  }
		  GET16(dat16);
		  datasaved[mm] = dat16;
		}

		//printf("TRIG=%5d, SLOT=%2d, CHAN=%2d, VAL=%4d, time=0x%llx nchan=%d\n",trig,slot,chan,dat16,fadctime,nchan);fflush(stdout);

#ifdef DEBUG
		printf("data[sl=%2d][ch=%2d]:\n",slot,chan);
		for(mm=0; mm<nsamples; mm++)
		{
		  printf(" [%2d]%4d,",mm,datasaved[mm]);
          /*if(((mm+1)%10)==0) printf("\n");*/
		}
        printf("\n");
#endif		

        /*helicity info: slot 7, channels 0-3*/
		if(slot==7)
		{
		  //if(chan==0) printf("4 signals (hel/str/quad/tsettle: ");
          //if(chan==0) {if(dat16>1000) printf(" 1"); else printf(" 0");} /*hel*/
		  //if(chan==1) {if(dat16>1000) printf(" 1"); else printf(" 0");} /*str*/
		  //if(chan==2) {if(dat16>1000) printf(" 1"); else printf(" 0");} /*quad*/
		  //if(chan==3) {if(dat16>1000) printf(" 1"); else printf(" 0");}
          //if(chan==3) printf("\n");

          if(chan==0&&dat16>1000) helicity = 1;
          if(chan==1&&dat16>1000) strob = 1;
          if(chan==2&&dat16>1000) quad = 1;
		}

	  } /*for(nn=0; nn<nchan; nn++)*/

#ifdef DEBUG
	  printf("fadcs: end loop: b08=0x%08x\n",b08);fflush(stdout);
#endif
    }

    /* update pipelines */
    for(ii=0; ii<NPREV-1; ii++)
    {
      str_pipe[ii] = str_pipe[ii+1];
      hel_pipe[ii] = hel_pipe[ii+1];
      quad_pipe[ii] = quad_pipe[ii+1];
      time_pipe[ii] = time_pipe[ii+1];
#ifdef DEBUG2
      printf("-----> str_pipe[%2d]=%d hel_pipe=%d quad_pipe=%d time=0x%llx\n",ii,str_pipe[ii],hel_pipe[ii],quad_pipe[ii],time_pipe[ii]);
#endif
    }
    str_pipe[NPREV-1] = strob;
    hel_pipe[NPREV-1] = helicity;
    quad_pipe[NPREV-1] = quad;
    time_pipe[NPREV-1] = timestamp;
#ifdef DEBUG2
    printf("--=--> str_pipe[%2d]=%d hel_pipe=%d quad_pipe=%d time=0x%llx\n",NPREV-1,str_pipe[NPREV-1],hel_pipe[NPREV-1],quad_pipe[NPREV-1],time_pipe[NPREV-1]);
#endif

  }








  /***************/
  /* scaler bank */

  fragtag = 82/*37*/;
  fragnum = -1;
  banktag = 0xe125;
  banknum = 0;

  ind = 0;
  for(banknum=0; banknum<1; banknum++) /*banknum=0 - gated, banknum=1 - ungated*/
  {
    /*printf("looking for %d %d  - 0x%04x %d\n",fragtag, fragnum, banktag, banknum);*/
    ind = evLinkBank(bufptr, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data);
    if(ind>0) break;
  }

  if(ind<=0)
  {
    /*printf("SCAL: cannot find SCALER bank (fragtag=%d banktag=0x%04x)\n",fragtag,banktag)*/;
  }
  else
  {
    nw = nbytes / 4;
    //printf("SCAL: nbytes=%d nw=%d\n",nbytes,nw);

    b08 = (unsigned char *) &bufptr[ind_data];
    end = b08 + nbytes;

    //printf("begin: b08=0x%llx end=0x%llx\n",b08,end);

    while (b08 < end)
    {
      GET32(itmp); /* get first channels */
      scaler_quad = ((itmp>>31)&0x1)^1;
      scaler_helicity = ((itmp>>30)&0x1)^1;

      /* update scaler pipelines */
      for(ii=0; ii<NSCAL-1; ii++)
      {
        scalquad[ii] = scalquad[ii+1];
        scalhel[ii]  = scalhel[ii+1];
      }
      scalquad[NSCAL-1] = scaler_quad;
      scalhel[NSCAL-1]  = scaler_helicity;

      //printf("SCAL: quad=%d, hel=%d\n",scaler_quad,scaler_helicity);

      itemp = 0;
      /*if(scaler_quad==pipequad[NFLIP-2] && scaler_helicity==pipehel[NFLIP-2])*/
      if( (scalquad[3]==pipequad[3]) && (scalhel[3]==pipehel[3]) &&
          (scalquad[2]==pipequad[2]) && (scalhel[2]==pipehel[2]) &&
          (scalquad[1]==pipequad[1]) && (scalhel[1]==pipehel[1]) &&
          (scalquad[0]==pipequad[0]) && (scalhel[0]==pipehel[0]) )
	  {
	    printf("SCAL in sync !\n");

        /* put results into the last (32th) word */
        itemp = (0x80<<24) | ((pipecorr[2]&1)<<16) | ((pipecorr[3]&1)<<8) | (pipecorr[4]&1);
	  }
      else
	  {
        printf("SCAL OUT OF sync - ERROR !!!!\n");

	    printf(" scal[3]: quad=%d, hel=%d\n",scalquad[3],scalhel[3]);
	    printf(" scal[2]: quad=%d, hel=%d\n",scalquad[2],scalhel[2]);
	    printf(" scal[1]: quad=%d, hel=%d\n",scalquad[1],scalhel[1]);
	    printf(" scal[0]: quad=%d, hel=%d\n",scalquad[0],scalhel[0]);

	    printf(" pipe[4]: quad=%d, hel=%d, corr=%d\n",pipequad[4],pipehel[4],pipecorr[4]);
	    printf(" pipe[3]: quad=%d, hel=%d, corr=%d\n",pipequad[3],pipehel[3],pipecorr[3]);
	    printf(" pipe[2]: quad=%d, hel=%d, corr=%d\n",pipequad[2],pipehel[2],pipecorr[2]);
	    printf(" pipe[1]: quad=%d, hel=%d, corr=%d\n",pipequad[1],pipehel[1],pipecorr[1]);
	    printf(" pipe[0]: quad=%d, hel=%d, corr=%d\n",pipequad[0],pipehel[0],pipecorr[0]);
	  }

      for(jj=1; jj<31; jj++) {GET32(itmp);} /* get the rest of channels except last one */


	  /* write corrected helicities into 32th word */
      printf("put hel: 0x%08x\n",itemp);
      b08out = b08;
      PUT32(itemp);

      /* read 32th word back */
      GET32(itmp);
      printf("get hel: 0x%08x\n",itmp);


      //printf("b08=0x%llx end=0x%llx\n",b08,end);

	} /*while (b08 < end)*/

  }













  if(done==1)
  {

    /********************************/
    /* CHECK IF WE MISSED ANY FLIPS */

    /* 'tstime_old' is timestamp for last received helicity strob trigger (front panel bit 0x1000) */

    deltatime = timestamp - tstime_old;
#ifdef DEBUG1
    printf("!!! timestamp=0x%llx(%llu) - tstime=0x%llx(%llu) = deltatime=0x%llx(%llu),  DELTATIME=0x%llx(%llu)\n",
                timestamp,timestamp,tstime_old,tstime_old,deltatime,deltatime,DELTATIMEHDICE,DELTATIMEHDICE);
#endif
    if((timestamp>0) && (tstime_old>0))
	{
	  if(deltatime > DELTATIMEHDICE)
      {
        missed_helicity_triggers ++;
#ifdef DEBUG1
        printf("\nMISSED helicity trigger %d times ??? (event=%d) (timestamp=0x%llx(%llu)) [deltatime=0x%llx(%llu) > DELTATIME=0x%llx(%llu)]\n",
			   missed_helicity_triggers,ievent,timestamp,timestamp,deltatime,deltatime,DELTATIMEHDICE,DELTATIMEHDICE);
        printf("Event %d SHOULD be in next helicity interval, expecting to see flip from FADC information\n",ievent);
#endif

        tstime = tstime_old + DELTATIMEHDICE; /* bump helicity trigger timestamp, emulating what it should be if we did not miss it */

        dtstime = tstime - tstime_old;
#ifdef DEBUG1
	    printf("++-++> tstime=0x%llx - tstime_old=0x%llx = dtstime=0x%llx\n",tstime,tstime_old,dtstime);
        printf("HEAD: tstime=%lld(0x%llx) dtstime=%lld\n",tstime,tstime,dtstime);
#endif
        tstime_old = tstime;

      }
	}

  /********************************/
  /********************************/

  }




  /*********************************************************************/
  /******************* we are here for FLIP only ***********************/

  if(str_pipe[NPREV-2]==-1) return(0); /*skip first event, we need at least two events to check for flip */


  /*if any value changed, we flipped 
  if((strob!=str_pipe[NPREV-2])||(helicity!=hel_pipe[NPREV-2])||(quad!=quad_pipe[NPREV-2]))*/

  /*
missed_strobs = 0.989922

== str/hel/quad befo: str=0 hel=1 quad=0 (event 20800) timestamp=0x63e1ab602
== str/hel/quad afte: str=0 hel=0 quad=1 (event 20801) timestamp=0x63e1abd67 (dtime=0x7fbab4, dtime_avg=0x810765)

missed_strobs = 0.000118

== str/hel/quad befo: str=0 hel=0 quad=1 (event 20801) timestamp=0x63e1abd67
== str/hel/quad afte: str=1 hel=0 quad=1 (event 20802) timestamp=0x63e1ac14a (dtime=0x3e3, dtime_avg=0x80f355)

missed_strobs = 1.023029

== str/hel/quad befo: str=1 hel=0 quad=1 (event 20898) timestamp=0x63e9bc313
== str/hel/quad afte: str=0 hel=0 quad=1 (event 20899) timestamp=0x63e9eacd5 (dtime=0x83eb8b, dtime_avg=0x80f3cb)

  ERROR: reading=1 predicted=0 (event 20899)
missed_strobs = 0.980884
  */






  /* we flipped only if strob changed */

  //printf("STROB=%d  str_pipe[NPREV-2]=%d QUAD=%d\n",strob,str_pipe[NPREV-2],quad);
  if((strob!=str_pipe[NPREV-2]))
  {
    //printf("TIMESTAMPS = 0x%llx 0x%llx\n",timestamp,timestamp_old);
    dtime = timestamp-timestamp_old;

    if(dtime_avg > 0.0) missed_strobs = (float)dtime/(float)dtime_avg;
    else                missed_strobs = 0LL;
    //printf("missed_strobs = %f\n",missed_strobs);
    if( missed_strobs > 1.5 )
	{
      printf("IT SEEMS WE MISSED ABOUT %f HELICITY STROB(S)\n",missed_strobs);
      if( ((int)missed_strobs/4) > 0 ) printf("CORRECTING BY %d STEPS !!!!!!!!\n",((int)missed_strobs/4));

      for(ii=0; ii<(int)missed_strobs/4; ii++)
	  {
        predicted_reading = ranBit(&iseed_earlier);
        present_helicity = ranBit(&iseed);
	  }
	}

    dtime_avg = dtime_avg + (dtime-dtime_avg)/Navg;
    Navg ++;
#ifdef DEBUG2
    printf("\n== str/hel/quad befo: str=%d hel=%d quad=%d (event %d) timestamp=0x%llx\n",
		   str_pipe[NPREV-2],hel_pipe[NPREV-2],quad_pipe[NPREV-2],ievent-1,time_pipe[NPREV-2]);
    printf("== str/hel/quad afte: str=%d hel=%d quad=%d (event %d) timestamp=0x%llx (dtime=0x%llx, dtime_avg=0x%llx)\n\n",
           strob,helicity,quad,ievent,timestamp,dtime,dtime_avg);
#endif

    timestamp_old = timestamp;





#define QUAD1 1

	/*use event after flip */
    strob1 = strob;
    helicity1 = helicity;
    quad1 = quad;	

	/*flip helicity*/
    //helicity1 ^= 1;
	
    //printf("USE EVENT AFTER FLIP: strob1=%d helicity1=%d quad1=%d (%d %d)\n",strob1,helicity1,quad1,QUAD1,(1-QUAD1));


    /* if we sync'ed already, check offset and bump it if necessary */
    if(done==1)
    {
#ifdef DEBUG
      printf("  checking offset=%d: strob1=%1d helicity1=%1d quad1=%d\n",offset,strob1,helicity1,quad1);
#endif
      if(quad1==QUAD1)
      {
        /* if we are here, 'offset' must be 3, otherwise most likely
        we've got unexpected quad1==QUAD1; we'll try to set quad1=(1-QUAD1) and
        see what happens */
        if(offset<3)
        {
          printf("    ERROR: unexpected offset=%d - trying to recover --------------------\n",offset);
          quad1 = 1-QUAD1;
          offset++;
          printf("       WARN: bump[1] offset to %d\n",offset);
	      /*FORCE_RECOVERY;*/
	    }
        else if(offset>3)
        {
          printf("    ERROR: unexpected offset=%d - forcing recovery 1 -------------------\n",offset);
	      FORCE_RECOVERY;
        }
        else
        {
          offset=0;
#ifdef DEBUG
          printf("    INFO: set[1] offset to %d\n",offset);
#endif
	    }
      }
      else
      {
        offset++;
#ifdef DEBUG
        printf("    INFO: bump[2] offset to %d\n",offset);
#endif
        if(offset>3)
        {
          /* if we are here, 'offset' must be <=3, otherwise most likely
          we've got unexpected quad1==1-QUAD1; we'll try to set quad1=QUAD1, offset=0
          and see what happens */
          printf("    ERROR: unexpected offset=%d - forcing recovery 2 -------------------\n",offset);
          quad1 = QUAD1;
          offset = 0;
          printf("       WARN: set[1] offset to %d and quad1 to %d\n",offset,quad1);
	      /*FORCE_RECOVERY;*/
        }
      }

    } /*if(done==1)*/



#ifdef DEBUG
    printf("  checking offset=%d done: strob1=%d helicity1=%1d quad1=%1d\n",offset,strob1,helicity1,quad1);
#endif








    /* looking for the beginning of quartet; set done=0 when found */
    if(done==-1)
    {
      printf("done=%d -> looking for the begining of quartet, quad1=%d\n",done,quad1);
      Navg = 1;
      dtime_avg = 0LL;
      if(quad1==QUAD1)
      {
        done=0; /* quad1==0 means first in quartet */
        printf("found the begining of quartet !\n");
	  }
    } /*if(done==-1)*/




    /* search for pattern: use first 24 quartets (not flips!) to get all necessary info */
    if(done==0)
    {
      printf("done=%d -> loading ..\n",done);
      if(quad1==QUAD1)
	  {
        done = loadHelicity(helicity1, &iseed, &iseed_earlier); /* 24 times returns 0, on 25th call returns 1 */
        printf("call loadHelicity, loaded one, done=%d\n",done);
      }

      if(done==1)
      {
        printf("=============== READY TO PREDICT (ev=%6d) =============== \n",ievent);
        Navg = 1;
        dtime_avg = 0LL;
	  }
    } /*if(done==0)*/






    if(done==1)
    {
#ifdef DEBUG
      printf("PATTERN WAS FOUND BEFORE, can determine helicity: done=%d quad1=%d\n",done,quad1);
#endif

      //printf("     FADC: quad=%d hel=%d\n",quad1,helicity1);

      /* update pipelines */
      for(ii=0; ii<NFLIP-1; ii++)
      {
        pipequad[ii] = pipequad[ii+1];
        pipehel[ii]  = pipehel[ii+1];
        pipecorr[ii] = pipecorr[ii+1];
      }
      pipequad[NFLIP-1] = quad1;
      pipehel[NFLIP-1]  = helicity1;


      if(quad1==QUAD1)
      {

        /* following two must be the same */
        present_reading = helicity1;
        predicted_reading = ranBit(&iseed_earlier);

#ifdef DEBUG
        printf("  quad1=%d, ranBit(&iseed_earlier) returns %d\n",quad1,predicted_reading);
#endif

        present_helicity = ranBit(&iseed);
	        
#ifdef DEBUG
        printf("  helicity: predicted=%d present=%d corrected=%d\n",predicted_reading,present_reading,present_helicity);
#endif



        /****************/
        /****************/
        /* direct check */
		/*SERGEY: TEMPORARY !!!
        remember_helicity[0] = remember_helicity[1];
        remember_helicity[1] = remember_helicity[2];
        remember_helicity[2] = present_helicity;
#ifdef DEBUG
        printf("  ============================== %1d %1d\n",remember_helicity[0],present_reading);
#endif
        if( remember_helicity[0] != -1 )
		{
          if( remember_helicity[0] != present_reading )
          {
            printf("  ERROR: direct check failed !!! %1d %1d\n",remember_helicity[0],present_reading);
            FORCE_RECOVERY;
          }
		}
		*/
        /****************/
        /****************/



		/*SERGEY: TEMPORARY !!!!!!!!!!!!!
        if(predicted_reading != present_reading)
        {
          printf("  ERROR !!!!!!!!! predicted_reading=%d != present_reading=%d\n",predicted_reading,present_reading);
          FORCE_RECOVERY;
        }
		*/


      }
#ifdef DEBUG
      else
	  {
        printf("  quad1=%d - do nothing\n",quad1);
	  }
#endif




	  /* filling pipe with corrected helicity */
      pres_heli = present_helicity;
      if((offset==0) || (offset==3))
	  {
        pipecorr[NFLIP-1] = pres_heli^1;
      }
      else if((offset==1) || (offset==2))
      {	  	  
        pipecorr[NFLIP-1] = pres_heli;
      }
      else
      {
        printf("  ERROR in pipe filling: illegal offset=%d\n",offset);
        FORCE_RECOVERY;
      }




    }

  } /******************* end of 'FLIP only '***********************/








  /*******************************/
  /* we are here for every event */

  if(done==1)
  {
#ifdef DEBUG
    printf("CHECKING IF WE STILL IN SYNC\n");		  
#endif
    /* first check, if we are still in sync */
    tmp0 = helicity1; /* helicity from current event */

    pred_read = predicted_reading;
    pres_heli = present_helicity;
#ifdef DEBUG
    printf("-------------------------------------------------------- offset=%d -> pred_read=%d\n",offset,pred_read);
#endif
    if((offset==0) || (offset==3))
    {
	  
      temp0 = pred_read;
      final_helicity = pres_heli^1;
    }
    else if((offset==1) || (offset==2))
    {	  	  
      temp0 = pred_read^1;
      final_helicity = pres_heli;
    }
    else
    {
      printf("  ERROR: illegal offset1=%d at temp0\n",offset1);
      FORCE_RECOVERY;
    }



    //final_helicity ^= 1; /* flip it back */

	  

#ifdef DEBUG
    printf("  tmp0=%d temp0=%d final=%d\n",tmp0,temp0,final_helicity);
#endif  

    if(tmp0 != temp0)
	{
      printf("  ERROR: reading=%d predicted=%d (event %d)\n",tmp0,temp0,ievent);
      FORCE_RECOVERY;
	}
    else
	{
#ifdef DEBUG
      printf("  INFO: reading=%d predicted=%d (event %d)\n",tmp0,temp0,ievent);
#endif


#if 1

      fragtag = 82/*37*/;
      fragnum = -1;
      banktag = 0xe10f;
      banknum = 0;

      ind = 0;
      for(banknum=0; banknum<40; banknum++)
      {
        /*printf("looking for %d %d  - 0x%04x %d\n",fragtag, fragnum, banktag, banknum);*/
        ind = evLinkBank(bufptr, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data);
        if(ind>0) break;
      }

      if(ind<=0)
      {
        printf("ERROR: cannot find HEAD bank (fragtag=%d)\n",fragtag);
        ievent = 0;
        /*return(0);*/
      }
      else
      {
        b08 = (unsigned char *) &bufptr[ind_data];

        /* skip first 5 words in HEAD bank */
        GET32(itmp);
        GET32(itmp);
        GET32(itmp);
        GET32(itmp);
        GET32(itmp);

        /* put results into 6th 32bit word */
        itmp = /*itmp |*/ (final_helicity<<1) | 1;
#ifdef DEBUG
        printf("put e10f: %d\n",itmp);
#endif
        b08out = b08;
        PUT32(itmp);

        /* read 6th 32bit word */
        GET32(itmp);
#ifdef DEBUG
        printf("get e10f: %d\n",itmp);
#endif
      }

#endif




	}

  }


  return(0);
}



