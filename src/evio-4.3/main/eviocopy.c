/*
 *  eviocopy.c
 *
 *   extracts evio events and copies them to another evio file
 *
 *   Author: Elliott Wolin, JLab, 12-sep-2001
*/

/* still to do
 * -----------
 *
*/



/* for posix */
#define _POSIX_SOURCE_ 1
#define __EXTENSIONS__


/*  misc macros, etc. */
#define MAXEVIOBUF 1000000


/* include files */
#include <evio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "evio.h"
#include "evioBankUtil.h"


#define MYSLOT 7
#define MYCRATE 57 /*dc61=56, dc62=57, dc63=58*/
#define MINHITS 5

/*  misc variables */
static unsigned int buf[MAXEVIOBUF];
static unsigned int bufout[MAXEVIOBUF];
static char *input_filename;
static int input_handle;
static char *output_filename;
static int output_handle;
static int nevent         = 0;
static int nwrite         = 0;
static int skip_event     = 0;
static int max_event      = 0;
static int nevok          = 0;
static int evok[100];
static int nnoev          = 0;
static int noev[100];
static int nnonum         = 0;
static int nonum[100];
static int debug          = 0;


/* prototypes */
void decode_command_line(int argc, char **argv);
int user_event_select(unsigned int *buf, unsigned int *bufout);

/*--------------------------------------------------------------------------*/

/*dcrb*/

#define NSHIFT_HALF 16
#define NSHIFT (NSHIFT_HALF*2+1)
  static int dshift[NSHIFT][5];
  static unsigned int shift[NSHIFT][5];
  const unsigned int shift_half[NSHIFT_HALF][5] = {
                          /* l1,l2,l3,l4,l5 */
                            { 1, 0, 0, 0, 0, },
                            { 1, 0, 1, 0, 0, },
                            { 0, 0, 0, 0, 1, },
                            { 0, 0, 1, 0, 1, },
                            { 0, 0, 1, 1, 1, },
                            { 1, 0, 1, 0, 1, },
                            { 1, 0, 1, 1, 1, },
                            { 1, 1, 1, 1, 1, },
                            { 0, 1, 1, 2, 2, },
                            { 0, 0, 1, 1, 2, },
                            { 0, 1, 0, 1, 0, },
                            { 0, 0, 0, 1, 1, },
                            { 1, 0, 1, 1, 2, },
                            { 1, 1, 1, 1, 2, },
                            { 1, 1, 2, 1, 2, },
                            { 1, 1, 2, 2, 2, } };

/*dcrb*/


int
main (int argc, char **argv)
{

  int status;
  int i,j,l;
  

  /* decode command line */
  decode_command_line(argc,argv);


  /*dcrb*/

  /* create full shift table using shift_half */

  for(i=0; i<NSHIFT_HALF; i++)
  {
    for(j=0; j<5; j++)
    {
      shift[i][j] = - shift_half[NSHIFT_HALF-1-i][j];
      shift[i+NSHIFT_HALF+1][j] = shift_half[i][j];
    }
  }
  for(j=0; j<5; j++) shift[NSHIFT_HALF][j] =0;

  printf("\n\nShift table:\n");
  for(i=0; i<NSHIFT; i++)
  {
    printf("%3d >>> %2d %2d %2d %2d %2d\n",i,shift[i][0],
                          shift[i][1],shift[i][2],shift[i][3],shift[i][4]);
  }
  printf("\n\n");

  /* create differential shift table */

  for(j=0; j<5; j++) dshift[0][j] = 0;
  printf("\n\nDifferential shift table:\n");
  for(i=1; i<NSHIFT; i++)
  {
    for(j=0; j<5; j++)
    {
      dshift[i][j] = shift[i][j] - shift[i-1][j];
    }
    printf("%3d >>> %2d %2d %2d %2d %2d\n",i,dshift[i][0],
                          dshift[i][1],dshift[i][2],dshift[i][3],dshift[i][4]);
  }
  printf("\n\n");

  /*dcrb*/


  /* open evio input file */
  if((status=evOpen(input_filename,"r",&input_handle))!=0) {
    printf("\n ?Unable to open input file %s, status=%d\n\n",input_filename,status);
    exit(EXIT_FAILURE);
  }


  /* open evio output file */
  if((status=evOpen(output_filename,"w",&output_handle))!=0) {
    printf("\n ?Unable to open output file %s, status=%d\n\n",output_filename,status);
    exit(EXIT_FAILURE);
  }


  /* debug...need to set large block size ??? */
/*   l=0x8000; */
/*   status=evIoctl(output_handle,"b",(void*)&l); */
/*   if(status!=0) { */
/*     printf("\n ?evIoctl error on output file %s, status=%d\n\n",output_filename,status); */
/*     exit(EXIT_FAILURE); */
/*   } */


  /* loop over events, skip some, copy up to max_event events */
  nevent=0;
  nwrite=0;
  while ((status=evRead(input_handle,buf,MAXEVIOBUF))==0)
  {
    nevent++;
    if(skip_event>=nevent)continue;
    if(user_event_select(buf,bufout)==0)continue;
    printf("WRITING EVENT\n");
    nwrite++;
    status=evWrite(output_handle,bufout);
    if(status!=0) {
      printf("\n ?evWrite error output file %s, status=%d\n\n",output_filename,status);
      exit(EXIT_FAILURE);
    }
    if( (max_event>0) && (nevent>=max_event+skip_event) )break;
  }


  /* done */
  printf("\n  Read %d events, copied %d events\n\n",nevent,nwrite);
  evClose(output_handle);
  evClose(input_handle);
  exit(EXIT_SUCCESS);
}


/*---------------------------------------------------------------- */

  /*dcrb test*/

  static int board_layer[96] = {
    2, 4, 6, 1, 3, 5,
    2, 4, 6, 1, 3, 5,
    2, 4, 6, 1, 3, 5,
    2, 4, 6, 1, 3, 5,
    2, 4, 6, 1, 3, 5,  
    2, 4, 6, 1, 3, 5,
    2, 4, 6, 1, 3, 5,
    2, 4, 6, 1, 3, 5,
    2, 4, 6, 1, 3, 5,
    2, 4, 6, 1, 3, 5,
    2, 4, 6, 1, 3, 5,
    2, 4, 6, 1, 3, 5,
    2, 4, 6, 1, 3, 5,
    2, 4, 6, 1, 3, 5,
    2, 4, 6, 1, 3, 5,
    2, 4, 6, 1, 3, 5
  };
  static int board_wire[96] = {
    1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8,
    9, 9, 9, 9, 9, 9,
   10,10,10,10,10,10,
   11,11,11,11,11,11,
   12,12,12,12,12,12,
   13,13,13,13,13,13,
   14,14,14,14,14,14,
   15,15,15,15,15,15,
   16,16,16,16,16,16
  };

  /*dcrb test*/


int
user_event_select(unsigned int *buf, unsigned int *bufout)
{
  int i, j, ret, status;
  int event_tag = buf[1]>>16;
  int fragtag, fragnum, banktag, banknum, ind, ind12, ind_data, ind_out, slot, nbytes, nchan, nwires, *nhits;
  int chan, iev, trig, nsamples, nn, mm, layer, wire, ihit, layers[6], ngoodhits, accepted;
  int display[6][3], display_shifted[6][3], ishift;
  uint64_t fadctime, timestamp;
  unsigned char *end;
  unsigned short dat16;
  unsigned char dat8;
  int threshold = 30;
  /*                0  01  02  03  04  05  06  07  08  09 010 011 012 013 014 015*/
  //int peds[16] = {100,106,102,106,107,119,127,119,122,120,139,102, 80,117, 94, 92};
  int peds[16] = {201,191,195,227,208,225,215,236,215,199,243,210,205,214,202,193};
  unsigned short data[16][128], crossing_bin[16], crossing_width[16];

  /*dcrb test*/

  GET_PUT_INIT;
  unsigned int *bufptr = buf;
  unsigned int *bufoutptr = bufout;




  /* processing fragtag=75 (mmft1) */

  fragtag = 75;
  fragnum = -1;
  banktag = 0xe101;
  banknum = 0;

  ind = 0;
  accepted= 0;
  for(nn=0; nn<16; nn++)
  {
    crossing_bin[nn] = 0;
    crossing_width[nn] = 0;
    for(mm=0; mm<128; mm++) data[nn][mm] = 0;
  }
  for(nn=0; nn<6; nn++)
  {
    layers[nn] = 0;
    for(mm=0; mm<3; mm++) display[nn][mm] = 0;
  }



  for(banknum=0; banknum<40; banknum++)
  {
    /*printf("looking for %d %d  - 0x%04x %d\n",fragtag, fragnum, banktag, banknum);*/
    ind12 = evLinkBank(bufptr, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data);
    if(ind12>0) break;
  }

  if(ind12<=0)
  {
    //printf("ERROR: cannot find FADC bank (fragtag=%d banktag=%d)\n",fragtag,banktag);
    goto next1;
  }
  else
  {
    //printf("found FADC banknum=%d\n",banknum);
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

	  if(slot==3)
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

	  /*SELECTION!!!*/
      if(/*slot==3 &&*/ nchan>=MINHITS)
	  {
        accepted = 1;
	  }

	  for(nn=0; nn<nchan; nn++)
      {
	    GET8(chan);
        if(accepted)
        {
          layer = board_layer[chan] - 1;
          wire = board_wire[chan] - 1;
          layers[layer] = 1;
          display[layer][wire] = 1;
          printf("chan=%d -> layer=%d wire=%d\n",chan,layer,wire+32);
        }
		GET32(nsamples);
		for (mm=0; mm<nsamples; mm++)
        {
		  GET16(dat16);
          data[chan][mm] = dat16;

          if(accepted)
          {
            if((crossing_bin[chan]==0) && ((dat16-peds[chan])>=threshold))
		    {
              crossing_bin[chan] = mm;
              //printf("chan=%d, crossing_bin=%d\n",chan,crossing_bin[chan]);
		    }
            if((crossing_bin[chan]!=0) && (crossing_width[chan]==0) && ((dat16-peds[chan])<threshold))
		    {
              crossing_width[chan] = mm - crossing_bin[chan];
              //printf("chan=%d, crossing_width=%d\n",chan,crossing_width[chan]);
		    }
		  }

		}
	  }

#ifdef DEBUG
	  printf("fadcs: end loop: b08=0x%08x\n",b08);fflush(stdout);
#endif
    }

  }

  /* end of processing fragtag=75 (mmft1) */

  ngoodhits = 0;
  for(chan=0; chan<16; chan++)
  {
    if(crossing_bin[chan]>0) ngoodhits++;
  }
  if(ngoodhits>=MINHITS) accepted = 1;
  else                   accepted = 0;


  if(accepted)
  {
	printf("\nEvent %6d\n",iev);
    for(nn=0; nn<6;  nn++)
    {
      printf("             ");
      for(mm=0; mm<3; mm++) 
      {
        if(display[nn][mm]==1) printf("X "); else printf("_ ");
      }
      printf("\n");
    }


    evOpenEvent(bufout,129);
    evCopyFrag(buf,37,fragnum,bufout);
    evCopyFrag(buf,75,fragnum,bufout);
	for(mm=56; mm<=58; mm++)
	{
      if(mm!=MYCRATE) evCopyFrag(buf,mm,fragnum,bufout);
    }


    int fragtag = MYCRATE;
    int fragnum = -1;
    int banknum = 0;
    int banktyp = 0xf;

    status = evOpenFrag(bufout, fragtag, fragnum);
    printf("\n\nevOpenFrag returns %d\n",status);
    if(status<0) {printf("error in evOpenFrag - return\n");return(0);}

    /*copy existing DCRB bank contents */

    fragtag = MYCRATE;
    fragnum = -1;
    banknum = 0;
    banktyp = 0xf;
    banktag = 0xe116;
    char *fmt2 = "c,i,l,N(c,s)"; /* slot,event#,timestamp,Nhits(channel,tdc) */

    ind = 0;
    for(banknum=0; banknum<40; banknum++)
    {
      /*printf("looking for %d %d  - 0x%04x %d\n",fragtag, fragnum, banktag, banknum);*/
      ind12 = evLinkBank(bufptr, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data);
      if(ind12>0) break;
    }

    if(ind12<=0)
    {
      //printf("ERROR: cannot find DCRB bank (fragtag=%d banktag=%d)\n",fragtag,banktag);
      return(0);
    }
    else
    {
	  /*open output bank*/
      ret = evOpenBank(bufout, fragtag, fragnum, banktag, banknum, banktyp, fmt2, &ind_out);
      printf("evOpenBank returns = %d\n",ret);
      b08out = (unsigned char *)&bufout[ind_out];
      printf("first b08out = 0x%08x\n",b08out);

      /*copy existing data*/
      int have_myslot = 0;
      printf("banknum=%d\n",banknum);
      b08 = (unsigned char *) &bufptr[ind_data];
      end = b08 + nbytes;
      while (b08 < end)
      {
	    GET8(slot);
	    PUT8(slot);
	    GET32(trig);
	    PUT32(trig);
	    GET64(timestamp);
	    PUT64(timestamp);
        nhits = (unsigned int *)b08out; /*remember channel counter position*/
	    GET32(nchan);
	    PUT32(nchan);
        for(nn=0; nn<nchan; nn++)
	    {
          GET8(dat8);
          PUT8(dat8);
          GET16(dat16);
          PUT16(dat16);
	    }
        if(slot==MYSLOT) /*if slot=MYSLOT already there, add fadc data to it*/
		{
	      have_myslot = 1;
          for(nn=0; nn<16; nn++)
	      {
            if(crossing_bin[nn]>0)
	        {
              (*nhits)++;
              PUT8((nn));
              PUT16((crossing_bin[nn]*4+48)); /*add artificial constant to make it similar to dcrb data*/
	        }
	      }
        }
      }

      if(have_myslot == 0)
	  {
        /*add fadc data*/
        PUT8(MYSLOT);
        PUT32(trig);
        PUT64(timestamp);
        nhits = (unsigned int *)b08out;
	    PUT32(0);
        for(nn=0; nn<16; nn++)
	    {
          if(crossing_bin[nn]>0)
	      {
            (*nhits)++;
            PUT8((nn));
            PUT16((crossing_bin[nn]*4+48));
	      }
	    }
      }

      evCloseBank(bufout, fragtag, fragnum, banktag, banknum, b08out);
    }


 	/*form dcrb bank with width*/
    int banktag1 = 0xe130;
    char *fmt1 = "c,i,l,N(c,s,s)"; /* slot,event#,timestamp,Nhits(channel,tdc,width) */
    ret = evOpenBank(bufout, fragtag, fragnum, banktag1, banknum, banktyp, fmt1, &ind_out);
    printf("evOpenBank returns = %d\n",ret);
    b08out = (unsigned char *)&bufout[ind_out];
    /*printf("first b08out = 0x%08x\n",b08out);*/
    PUT8(MYSLOT);
    PUT32(iev);
    PUT64(timestamp);
    nhits = (unsigned int *)b08out;
	PUT32(0);
    for(nn=0; nn<16; nn++)
	{
      if(crossing_bin[nn]>0)
	  {
        (*nhits)++;
        PUT8(nn);
        PUT16((crossing_bin[nn]*4+48));
        PUT16((crossing_width[nn]*4));
	  }
	}
    evCloseBank(bufout, fragtag, fragnum, banktag1, banknum, b08out);

  }

  return(accepted);


next1:


  /* processing fragtag=MYCRATE (dc61, dc62 or dc63) */

  fragtag = MYCRATE;
  fragnum = -1;
  banknum = 0;
  int banktyp = 0xf;
  banktag = 0xe116;
  char *fmt2 = "c,i,l,N(c,s)"; /* slot,event#,timestamp,Nhits(channel,tdc) */




  ind = 0;
  accepted= 0;
  for(banknum=0; banknum<40; banknum++)
  {
    /*printf("looking for %d %d  - 0x%04x %d\n",fragtag, fragnum, banktag, banknum);*/
    ind12 = evLinkBank(bufptr, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data);
    if(ind12>0) break;
  }

  if(ind12<=0)
  {
    //printf("ERROR: cannot find DCRB bank (fragtag=%d banktag=%d)\n",fragtag,banktag);
    return(0);
  }
  else
  {
    b08 = (unsigned char *) &bufptr[ind_data];
    end = b08 + nbytes;

    while (b08 < end)
    {
#ifdef DEBUG
	  printf("dcrb: begin while: b08=0x%08x\n",b08);fflush(stdout);
#endif

	  GET8(slot);
	  GET32(trig);
	  GET64(fadctime); /* time stamp for DCRBs 2 counts bigger then for VTPs ??? */

#ifdef DEBUG2
	  printf("cbtime=%lld(0x%llx)\n",fadctime,fadctime);fflush(stdout);
#endif

#if 0
	  fadctime = ((fadctime & 0xFFFFFF) << 24) | ((fadctime >> 24) & 0xFFFFFF); /* UNTILL FIXED IN ROL2 !!!!!!!!!!!!!!!!! */
#endif

	  iev = trig;

	  if(slot==3)
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

      nwires = 0;
      for(nn=0; nn<nchan; nn++)
	  {
        GET8(dat8);
        GET16(dat16);
        if(slot==MYSLOT && dat8<16)
		{
          nwires ++;
          printf("iev=%d chan=%d tdc=%d\n",iev,dat8,dat16);
		}
        if(nwires>=MINHITS)
		{
          accepted = 1;
          printf("--> accepted\n");
		}
	  }

	  /*SELECTION!!!*/

#ifdef DEBUG
	  printf("dcrb: end loop: b08=0x%08x\n",b08);fflush(stdout);
#endif
    }
  }

  /* end of processing fragtag=MYCRATE */



  //printf("returns accepted=%d\n",accepted);
  return(accepted);


  /*dcrb test*/



  /*
  if((nevok<=0)&&(nnoev<=0)&&(nnonum<=0)) {
    return(1);

  } else if(nevok>0) {
    for(i=0; i<nevok; i++) if(event_tag==evok[i])return(1);
    return(0);
    
  } else if(nnoev>0) {
    for(i=0; i<nnoev; i++) if(event_tag==noev[i])return(0);
    return(1);

  } else if(nnonum>0) {
    for(i=0; i<nnonum; i++) if(nevent==nonum[i])return(0);
    return(1);
  }
  */

}


/*---------------------------------------------------------------- */


void decode_command_line(int argc, char**argv) {
  
  const char *help = 
    "\nusage:\n\n  eviocopy [-max max_event] [-skip skip_event] \n"
    "           [-ev evtag] [-noev evtag] [-nonum evnum] [-debug] input_filename output_filename\n";
  int i;
    
    
  if(argc<2) {
    printf("%s\n",help);
    exit(EXIT_SUCCESS);
  } 


  /* loop over arguments */
  i=1;
  while (i<argc) {
    if (strncasecmp(argv[i],"-h",2)==0) {
      printf("%s\n",help);
      exit(EXIT_SUCCESS);

    } else if (strncasecmp(argv[i],"-debug",6)==0) {
      debug=1;
      i=i+1;

    } else if (strncasecmp(argv[i],"-max",4)==0) {
      max_event=atoi(argv[i+1]);
      i=i+2;

    } else if (strncasecmp(argv[i],"-skip",5)==0) {
      skip_event=atoi(argv[i+1]);
      i=i+2;

    } else if (strncasecmp(argv[i],"-ev",3)==0) {
      if(nevok<(sizeof(evok)/sizeof(int))) {
	evok[nevok++]=atoi(argv[i+1]);
	i=i+2;
      } else {
	printf("?too many ev flags: %s\n",argv[i+1]);
      }

    } else if (strncasecmp(argv[i],"-noev",5)==0) {
      if(nnoev<(sizeof(noev)/sizeof(int))) {
	noev[nnoev++]=atoi(argv[i+1]);
	i=i+2;
      } else {
	printf("?too many noev flags: %s\n",argv[i+1]);
      }

    } else if (strncasecmp(argv[i],"-nonum",6)==0) {
      if(nnonum<(sizeof(nonum)/sizeof(int))) {
	nonum[nnonum++]=atoi(argv[i+1]);
	i=i+2;
      } else {
	printf("?too many nonum flags: %s\n",argv[i+1]);
      }

    } else if (strncasecmp(argv[i],"-",1)==0) {
      printf("\n  ?unknown command line arg: %s\n\n",argv[i]);
      exit(EXIT_FAILURE);

    } else {
      break;
    }
  }
  
  /* last two args better be filenames */
  input_filename=argv[argc-2];
  output_filename=argv[argc-1];

  return;
}


/*---------------------------------------------------------------- */
