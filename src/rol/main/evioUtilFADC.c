
#ifdef Linux_armv7l

int
main()
{
  exit(0);
}

#else

/* evioUtilFADC.c - testing FADC formats */

/* for posix */
#define _POSIX_SOURCE_ 1
#define __EXTENSIONS__

/*
#define DEBUG
*/

#define ABS(x)      ((x) < 0 ? -(x) : (x))

#define TIMERL_VAR \
  static hrtime_t startTim, stopTim, dTim; \
  static int nTim; \
  static hrtime_t Tim, rmsTim, minTim=10000000, maxTim, normTim=1

#define TIMERL_START \
{ \
  startTim = gethrtime(); \
}

#define TIMERL_STOP(whentoprint_macros,histid_macros) \
{ \
  stopTim = gethrtime(); \
  if(stopTim > startTim) \
  { \
    nTim ++; \
    dTim = stopTim - startTim; \
    /*if(histid_macros >= 0)   \
    { \
      uthfill(histi, histid_macros, (int)(dTim/normTim), 0, 1); \
    }*/														\
    Tim += dTim; \
    rmsTim += dTim*dTim; \
    minTim = minTim < dTim ? minTim : dTim; \
    maxTim = maxTim > dTim ? maxTim : dTim; \
    /*logMsg("good: %d %ud %ud -> %d\n",nTim,startTim,stopTim,Tim,5,6);*/ \
    if(nTim == whentoprint_macros) \
    { \
      printf("timer: %7llu microsec (min=%7llu max=%7llu rms**2=%7llu)\n", \
                Tim/nTim/normTim,minTim/normTim,maxTim/normTim, \
                ABS(rmsTim/nTim-Tim*Tim/nTim/nTim)/normTim/normTim); \
      nTim = Tim = 0; \
    } \
  } \
  else \
  { \
    /*logMsg("bad:  %d %ud %ud -> %d\n",nTim,startTim,stopTim,Tim,5,6);*/ \
  } \
}










/*  misc macros, etc. */
#define MAXEVIOBUF 1000000

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <evio.h>
#include <evioBankUtil.h>

#include "da.h"
#include "packing.h"

static unsigned int buf[MAXEVIOBUF];
static char *input_filename/* = "/work/dcrb/dcrb2_000049.evio.0_1600V_60mV"*/;
static int input_handle;
static char *output_filename/* = "test.evio"*/;
static int output_handle;

static int nevent         = 0;
static int nwrite         = 0;
static int skip_event     = 0;
static int max_event      = 1000000;

hrtime_t
gethrtime(void)
{
  static double firstsecs = 0.;
  uint64_t ret;
  double microsecs;
  double d1=0.,d2;
  struct timeval to;
  gettimeofday(&to, NULL);
  d1 = to.tv_sec;
  if(firstsecs==0) firstsecs=d1;
  d1 = d1 - firstsecs;
  d2 = to.tv_usec;
  microsecs = d2 + d1*1000000.0;
  ret = microsecs;

  return(ret);
}

int
main(int argc, char **argv)
{
  int status;
  int j, l, nn, mm, tag;
  int ind, fragtag, fragnum, nbytes, ind_data, timestamp_flag, type, *nhits;
  int slot, slot_old, event, chan, trig, nchan, nsamples, result, counter=0;
  int banktag = 0xe116, banknum, banktyp = 0xf;
  char *fmt = "c,i,l,N(c,s)"; /* slot,event#,timestamp,Nhits(channel,tdc) */
  unsigned int ret, word;
  unsigned long long timestamp, timestamp_old;
  unsigned long long time;
  unsigned char *end, *start;
  unsigned short datasaved[1000];
  char buffer[324];
  int bytesSumm = 0;
  double summ, average;
  char *pointer;
  unsigned short dat16;
  FILE *fd;
  GET_PUT_INIT;
  TIMERL_VAR;

  if(argc!=3) printf("Usage: evioUtilTest <input evio file> <output evio file>\n");
  input_filename = strdup(argv[1]);
  output_filename = strdup(argv[2]);
  printf("Use >%s< as input file\n",input_filename);
  printf("Use >%s< as output file\n",output_filename);
  if(!strcmp(input_filename,output_filename))
  {
    printf("input and output files must be different - exit\n");
    exit(0);
  }

  fd = fopen(output_filename,"r");
  if(fd>0)
  {
    printf("Output file %s exist - exit\n",output_filename);
    fclose(fd);
    exit(0);
  }

  /* open evio input file */
  if((status = evOpen(input_filename,"r",&input_handle))!=0)
  {
    printf("\n ?Unable to open input file %s, status=%d\n\n",input_filename,status);
    exit(1);
  }

#if 0
  /* open evio output file */
  if((status = evOpen(output_filename,"w",&output_handle))!=0)
  {
    printf("\n ?Unable to open output file %s, status=%d\n\n",output_filename,status);
    exit(1);
  }
#endif


  nevent=0;
  nwrite=0;
  while ((status=evRead(input_handle,buf,MAXEVIOBUF))==0)
  {
    nevent++;
    if(!(nevent%10000)) printf("processed %d events\n",nevent);
    if(skip_event>=nevent) continue;
    /*if(user_event_select(buf)==0) continue;*/








    /**********************/
	/* evioBankUtil stuff */

    fragnum = -1;
    tag = 0xe101; /* Input fadc bank */
    banktag = 0xe126; /*output fadc bank (packed) */

	for(fragtag=72; fragtag<73; fragtag++)
	/*for(fragtag=1; fragtag<100; fragtag++)*/
	{

	  ind = evLinkFrag(buf, fragtag, fragnum);
      /*printf("evLinkFrag returns %d\n",ind);*/
	  if(ind<=0) continue;

	  for(banknum=0; banknum<40; banknum++)
	  {
        ind = evLinkBank(buf, fragtag, fragnum, tag, banknum, &nbytes, &ind_data);
        /*printf("evLinkBank returns %d\n",ind);*/
	    if(ind<=0) continue;

        start = (unsigned char *) &buf[ind_data];
        end = start + nbytes;
	    /*printf("input: nbytes=%d (%d words)\n",nbytes,nbytes>>2);*/

#if 0
        if(ind > 0)
	    {
          ret = evOpenBank(buf, fragtag, fragnum, banktag, banknum, banktyp, fmt, &ind_data);
          printf("evOpenBank returns = %d\n",ret);

          b08out = (unsigned char *)&buf[ind_data];
          /*printf("first b08out = 0x%08x\n",b08out);*/
#endif 

TIMERL_START;

          timestamp_flag = 0;
          slot_old = -1;
          b08 = start;
          while(b08<end)
	      {
#ifdef DEBUG
			printf("fadcs: begin while: b08=0x%08x\n",b08);fflush(stdout);
#endif
			GET8(slot);
			GET32(trig);
			GET64(time);
			GET32(nchan);

			for(nn=0; nn<nchan; nn++)
            {
			  GET8(chan);
			  GET32(nsamples);
#ifdef DEBUG
			  printf("==> crate=%d, slot=%d, chan=%d, nsamples=%d\n",fragtag, slot,chan,nsamples);fflush(stdout);
#endif

			  /*save data*/
			  for(mm = 0; mm < nsamples; mm++)
              {
				GET16(dat16);
				datasaved[mm] = dat16;
#ifdef DEBUG
			    printf("====> [%3d] %d\n",mm,datasaved[mm]);fflush(stdout);
#endif
			  }

              /* fill extra 16 samples using last data sample - pack() works by groups of 16 samples */
              for(mm=nsamples; mm<nsamples+16; mm++)
			  {
                datasaved[mm] = datasaved[nsamples-1];
#ifdef DEBUG
			    printf("----> [%3d] %d\n",mm,datasaved[mm]);fflush(stdout);
#endif
			  }

			  pointer = (char *)datasaved;

/*TIMERL_START;*/
              result = pack(nsamples, pointer, buffer);
/*TIMERL_STOP(100000,1000);*/

              bytesSumm += (double)result;
#ifdef DEBUG
              printf(" %d : pulse size = %d shorts (%d bytes), result = %d bytes\n",counter, nsamples, nsamples*2, result);
              for(j = 0; j < result; j++) printf(" %02hhx ",buffer[j]);
              printf("\n");
#endif
              counter++;

#ifdef DEBUG
			  printf("fadcs: end loop: b08=0x%08x\n",b08);fflush(stdout);
#endif
            }

          }

TIMERL_STOP(10000,1000);




#if 0
          evCloseBank(buf, fragtag, fragnum, banktag, banknum, b08out);

	    }
#endif


	  } /* banknum loop */
	} /* fragtag loop */


	/* evioBankUtil stuff */
    /**********************/



    nwrite++;
#if 0
    status = evWrite(output_handle,buf);
    if(status!=0)
    {
      printf("\n ?evWrite error output file %s, status=%d\n\n",output_filename,status);
      exit(1);
    }
#endif
    if( (max_event>0) && (nevent>=max_event+skip_event) ) break;
  }


  /* done */
  printf("\n  Read %d events, copied %d events\n\n",nevent,nwrite);
#if 0
  evClose(output_handle);
#endif
  evClose(input_handle);

  summ = (double) bytesSumm;
  average = summ/counter;
  printf(" pulse packing average %d/%d # %.4f bytes per set of samples\n", bytesSumm, counter,average);

  exit(0);
}

#endif
