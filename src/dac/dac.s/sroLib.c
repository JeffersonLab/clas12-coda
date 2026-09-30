
/* sroLib.c */

#if defined(Linux_armv7l)

int
sroLib_dummy()
{
}

#else


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <sched.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>

#ifdef Linux
#include <sys/prctl.h>
#endif
#if defined __sun||LINUX
#include <dlfcn.h>
#endif



#include "rc.h"
#include "rolInt.h"
#include "da.h"
#include "circbuf.h"
#include "libdb.h"
#include "et_private.h"

#include "srobuf.h"
#include "sroLib.h"

//evio
#include "evio.h"
#include "evioBankUtil.h"
//evio



//#define PEAK_FINDER


/* payload->slot translation table (payload=0 not used) */
static int payload2slot[17] = 
{
/*0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16 - payloads*/
  0, 10, 13,  9, 14,  8, 15,  7, 16,  6, 17,  5, 18,  4, 19,  3, 20 /*slots*/
};


/* sector numbers are [0..5] */
static int ecal_rocid2sector[64] =
{
  -1,  0, -1, -1, -1, -1, -1,  1, -1, -1, -1, -1, -1,  2, -1, -1, -1, -1, -1,  3, -1, -1, -1, -1, -1,  4, -1, -1, -1, -1, -1,  5,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

/* sector numbers are [0..5] */
static int pcal_rocid2sector[64] =
{
  -1, -1, -1,  0, -1, -1, -1, -1, -1,  1, -1, -1, -1, -1, -1,  2, -1, -1, -1, -1, -1,  3, -1, -1, -1, -1, -1,  4, -1, -1, -1, -1,
  -1,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};





static int dcrb_sector[18];
static int dcrb_region[18];
static int dcrb_superlayer[21][96];
static int dcrb_layer[21][96];
static int dcrb_wire[21][96];


/* sector numbers are [0..5] */
static int dcrb_rocid2sector[64] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 0, 0, 0, 0, 0
};

/* region numbers are [0..2] */
static int dcrb_rocid2region[64] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 0, 0, 0, 0
};

/* superlayer numbers are [0..1] */
static int dcrb_slot2sl[22] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0 
};


void
initDcrbTranslationTable()
{
  int slot, chan;
  
  for(slot=3; slot<=9; slot++)
  {
    for(chan=0; chan<96; chan++)
    {
      dcrb_superlayer[slot][chan] = 0;
      dcrb_layer[slot][chan] = board_layer[chan]-1;
      dcrb_wire[slot][chan] = (board_wire[chan]-1)+(slot-3)*16;
      //printf("translation table: slot %d, chan %d -> sl=%d la=%d w=%d\n",slot,chan,dcrb_superlayer[slot][chan],dcrb_layer[slot][chan],dcrb_wire[slot][chan]);
    }
  }
  for(slot=14; slot<=20; slot++)
  {
    for(chan=0; chan<96; chan++)
    {
      dcrb_superlayer[slot][chan] = 1;
      dcrb_layer[slot][chan] = board_layer[chan]-1;
      dcrb_wire[slot][chan] = (board_wire[chan]-1)+(slot-14)*16;
      //printf("translation table: slot %d, chan %d -> sl=%d la=%d w=%d\n",slot,chan,dcrb_superlayer[slot][chan],dcrb_layer[slot][chan],dcrb_wire[slot][chan]);
    }
  }
  
  return;
}


//#define DEBUG
//#define DEBUG1


int
read_socket(int connection, unsigned char *buf, int len, int swap, int ix)
{
  uint32_t *ptr;
  int ii, nread = 0;
  int flags = MSG_DONTWAIT; /*MSG_DONTWAIT | MSG_WAITALL*/
  /*TIMERL_VAR;*/

  while(nread < len)
  {

    //printf("BEFOR: Buffer address %lx, length %d\n",buf + nread, len - nread);
    //printf("  read_socket[%d]: requesting %d bytes\n",connection,len - nread);fflush(stdout);

    /*TIMERL_START;*/
    int n = read(connection, buf + nread, len - nread);
    //int n = recv(connection, buf + nread, len - nread, 0/*flags*/); sometimes returns 0 at Prestart ...
    /*TIMERL_STOP(1000,ix);*/

    //printf("  read_socket[%d]: received %d bytes\n",connection,n);fflush(stdout);

    /* 'n' must be not zero */
    if( n<=0 )
    {
      perror("read");
      printf("ERROR: Buffer address %lx, length %d (len=%d)\n",buf + nread, len - nread, len);
      printf("  read_socket[%d]: ERROR: n=%d, errno=%d (ix=%d) - return\n",connection,n,errno,ix);fflush(stdout);
      return(n);
    }

    nread += n;
  }

  /* 'nread' must be even to 4 bytes - we always send 32bit words */
  if( (nread%4)!=0 )
  {
    printf("  read_socket[%d]: ERROR: nread=%d - not even to 4 bytes\n",connection,nread);fflush(stdout);
    exit(0);
  }

  if(swap)
  {
    ptr = (uint32_t *)buf;
    //for(ii=0; ii<nread/4; ii++) printf("BEFOR %08X\n", ptr[ii]);
    for(ii=0; ii<nread/4; ii++) ptr[ii] = ntohl/*LSWAP*/(ptr[ii]);
    //for(ii=0; ii<nread/4; ii++) printf("AFTER %08X\n", ptr[ii]);
  }

  //printf("  read_socket[%d]: INFO: return %d bytes\n",connection,nread);fflush(stdout);

  return(nread);
}

int
read_socket_timed(int connection, unsigned char *buf, int len, int swap, long seconds)
{
  uint32_t *ptr;
  int ii, nread = 0;
  int flags = MSG_DONTWAIT; /*MSG_DONTWAIT | MSG_WAITALL*/

  //timed
  fd_set readfds;
  struct timeval tv;
  int retval;
  FD_ZERO(&readfds);
  FD_SET(connection, &readfds);
  tv.tv_sec = seconds;
  tv.tv_usec = 0;
  retval = select(connection + 1, &readfds, NULL, NULL, &tv);
  //timed



  if(retval == -1)
  {
    perror("select");
    return -1; // Error
  }
  else if (retval == 0)
  {
    printf("Timeout occurred.\n");
    return 0; // Timeout
  }
  else
  {
    while(nread < len)
    {
      /*int n = recv(connection, buf + nread, len - nread, flags);*/

      //printf("BEFOR: Buffer address %lx, length %d\n",buf + nread, len - nread);
      //printf("  read_socket[%d]: requesting %d bytes\n",connection,len - nread);fflush(stdout);
      int n = read(connection, buf + nread, len - nread);
      //printf("  read_socket[%d]: received %d bytes\n",connection,n);fflush(stdout);

      /* 'n' must be not zero */
      if( n<=0 )
      {
        perror("read");
        printf("ERROR: Buffer address %lx, length %d\n",buf + nread, len - nread);
        printf("  read_socket[%d]: ERROR: n=%d, errno=%d - return\n",connection,n,errno);fflush(stdout);
        return(n);
      }

      nread += n;
    }
  }




  /* 'nread' must be even to 4 bytes - we always send 32bit words */
  if( (nread%4)!=0 )
  {
    printf("  read_socket[%d]: ERROR: nread=%d - not even to 4 bytes\n",connection,nread);fflush(stdout);
    exit(0);
  }

  if(swap)
  {
    ptr = (uint32_t *)buf;
    //for(ii=0; ii<nread/4; ii++) printf("BEFOR %08X\n", ptr[ii]);
    for(ii=0; ii<nread/4; ii++) ptr[ii] = ntohl/*LSWAP*/(ptr[ii]);
    //for(ii=0; ii<nread/4; ii++) printf("AFTER %08X\n", ptr[ii]);
  }

  //printf("  read_socket[%d]: INFO: return %d bytes\n",connection,nread);fflush(stdout);

  return(nread);
}



/* gets ONE set of frames; return value: 1 - if still in the current 'dabufpi' buffer, 0 - if end of 'dabufpi' buffer reached */


/*fill aggregated header*/
#define FILL_AGGREGATED_HEADER \
  aggrtmp = aggr; \
  if(print) printf("FILL_AGGREGATED_HEADER from index %d\n",theProc->index); \
  *aggrtmp++ = set_len;                                     /*bank of banks header: length (exclusive)*/	\
  *aggrtmp++ = (0xff60<<16) | (0x10<<8) | 0;                /*bank of banks header: second word*/ \
  *aggrtmp++ = AGGR_HEADER_LENGTH + nstreams_total - 3;     /*bank of segments header: length (exclusive)*/ \
  *aggrtmp++ = (0xff31<<16) | (0x20<<8) | 0;                /*bank of segments header: second word*/  \
  *aggrtmp++ = (0x32<<24) | (0x1<<16) | 3;                  /*segment header*/ \
  *aggrtmp++ = frame_number_save;                                 /*frame number*/ \
  *aggrtmp++ = timestamp_l_save;                                  /*timestamp_l*/ /*spec said it can be 'average'; we assumes it should be exact - for now*/ \
  *aggrtmp++ = timestamp_h_save;                                  /*timestamp_h*/ \
  *aggrtmp++ = (0x42<<24) | (0x1<<16) | nstreams_total;     /*segment header*/ \
  for(i=0; i<nstreams_total; i++)			\
  { \
    *aggrtmp++ = (roc_id<<16) | (nstreams[roc_id]<<4) | stream_id;	/*ERROR: roc_id for the last one (or first one ?) here !!!*/ \
  }

#define PRINT_AGGREGATED_HEADER		\
  for(i=0; i<AGGR_HEADER_LENGTH + nstreams_total; i++) printf(" AGGR[%2d] = 0x%08x (%6d)\n",i,aggr[i],aggr[i])


static int roc_active[MAXROCS];
static int ndata[MAXROCS][2];
static int mdata[MAXROCS][2][21];

int
sroGetSet(int *dabufpi, PROC theProc, int print, int *dataindex, int ithread)
{
  int *dabufp = theProc->bufout;
  int i, ii, jj, kk, padding, tot_len, set_len, len, slot_len, type, slot, nslots;
  int payload, slots[MAXCHAN]; 
  unsigned int val,q,ch,t;
  unsigned short *ptr16;
  int roc_id, stream_id, nstreams[MAX_ROCS];
  unsigned int timestamp_l, timestamp_h, timestamp_l_save, timestamp_h_save, timestamp_ref;
  //int record_number;
  int frame_number, frame_number_save;
  int nstreams_total, mstreams, nrocs_total, found;
  unsigned int *aggr, *aggrtmp;

  if(dabufpi == NULL)
  {
    printf("sroGetSet: dabufpi=NULL - do nothing\n");
    return(-1);
  }

  mstreams = 0;
  nrocs_total = 0;
  timestamp_ref = -1;
  frame_number_save = -1;

  tot_len = theProc->tot_len; /*big buffer total length*/
  ii = theProc->index; /* points 'ii' to INCOMPLETE aggregated header */
  if(print) printf("\nsroGetSet: BIG BUFFER tot_len=%d, current ii=%d\n",tot_len,ii);fflush(stdout);

  /* get some info from INCOMPLETE aggregated header, we will complete it in this routine; assume it has at least the number of streams */
  aggr = (unsigned int *)&dabufpi[ii];
  nstreams_total = aggr[8] & 0xFF;
  if(print)
  {
    printf("INCOMPLETE aggregated header at ii=%d===\n",ii);
    PRINT_AGGREGATED_HEADER;
    printf("nstreams_total = %d\n",nstreams_total);
  }
  ii += (AGGR_HEADER_LENGTH + nstreams_total);

  
  /*process data until end of set of frames, or until end of buffer*/

  while(ii<tot_len) /*loop over big buffer*/
  {
    if(print) printf("------------------ started frame at ii=%d -------------------------------\n",ii);


    /********************************************************/
    /* extract some info from SIB (Stream Information Bank) */

    roc_id = (dabufpi[ii+1]>>16)&0xFFFF;
    if(roc_id>0&&roc_id<MAX_ROCS)
    {
      nstreams[roc_id] = (dabufpi[ii+1]>>4)&0x7;
    }
    else
    {
      printf("[%d] ERROR: wrong roc_id=%d in ROC bank - exit (ii=%d, tot_len=%d)\n",ithread,roc_id,ii,tot_len);
      for(i=0; i<tot_len; i++) printf("[%d] [%5d] 0x%08x (%6d)\n",ithread,i,dabufpi[i],dabufpi[i]);
      exit(1);
    }
    stream_id = (dabufpi[ii+1])&0xF;

    frame_number = dabufpi[ii+5];
    timestamp_l = dabufpi[ii+6];
    timestamp_h = dabufpi[ii+7];
    if(print) printf("[%d] sroGetSet: ii=%d, Frame# %d, roc_id=%d, stream_id=%d (nstreams=%d) -- timestamp_h=0x%08x, timestamp_l=0x%08x\n",ithread,ii,frame_number,roc_id,stream_id,nstreams[roc_id],timestamp_h,timestamp_l);

    if(frame_number_save == -1)
    {
      frame_number_save = frame_number;
      timestamp_l_save = timestamp_l;
      timestamp_h_save = timestamp_h;
    }

    /* extract timestamp if record is NOT fake */
    if(print)
    {
      if(dabufpi[ii]<9) printf("[%d] =     FAKE frame# %d\n",ithread,frame_number);
      else              printf("[%d] = NOT FAKE frame# %d\n",ithread,frame_number);
    }

    /*for the first set: if it is new roc_id, add it to roc's table */
    if(theProc->nstreams_active == 0)
    {
      found = 0;
      for(i=0; i<nrocs_total; i++)
      {
        if(roc_id == theProc->rocid2roc_id[i])
        {
          found = 1;
          break;
        }
      }
      if(found==0) /* new roc_id */
      {
        theProc->rocid2roc_id[nrocs_total] = roc_id;
        theProc->roc_id2rocid[roc_id] = nrocs_total;
        printf("===== roc_id=%d -> theProc->rocid2roc_id[%d]=%d\n",roc_id,nrocs_total,theProc->rocid2roc_id[nrocs_total]);
        nrocs_total ++;
      }
    }

    if(print) printf("[%d] sroGetSet: ii=%d, Frame# %d, roc_id=%d, stream_id=%d (nstreams=%d)\n",ithread,ii,frame_number,roc_id,stream_id,nstreams[roc_id]);

    /*** if we are here, then record is NOT fake ***/
    if(print) printf(", timestamp=0x%08x 0x%08x",timestamp_h,timestamp_l);
    /* if 'timestamp_ref' is not set yet, do it here*/
    if(timestamp_ref == -1) timestamp_ref = timestamp_l;

    /*compare timestamp with the one from first not-fake frame in current set*/
    if(timestamp_l != timestamp_ref)
    {
      printf("[%d] ERROR: TIMESTAMP MISMATCH (0x%08x != 0x%08x) (ii=%d, tot_len=%d) - exit\n",ithread,timestamp_l,timestamp_ref,ii,tot_len);
      for(i=0; i<tot_len; i++) printf("[%d] [%5d] 0x%08x (%6d)\n",ithread,i,dabufpi[i],dabufpi[i]);
      exit(1);
    }
    else
    {
      if(print) printf("\n");
    }

    /* done with SIB */
    /*****************/



    len = dabufpi[ii];
    if(print) printf("ROC bank length = %d, second word = 0x%08x (roc_id=%d, SS_err=%d, SS_total_streams=%d, SS_stream_mask=%d)\n",
		     len,dabufpi[ii+1],(dabufpi[ii+1]>>16)&0xFFFF,(dabufpi[ii+1]>>7)&0x1,(dabufpi[ii+1]>>4)&0x7,stream_id);fflush(stdout);
    if(len<9)
    {
      //printf("fake frame, len=%d\n",len);
      ii += (len+1);
    }
    else /*if ROC bank length =1, it will be no stream bank; if =7, it is fake frame*/
    {
      //printf("NOT fake frame, len=%d\n",len);
      ii += 2;
      len = dabufpi[ii];
      if(print) printf("Stream bank length = %d, second word = 0x%08x\n",len,dabufpi[ii+1]);fflush(stdout);
      ii += 2;
      len = dabufpi[ii]&0xFFFF;
      if(print) printf("Timestamp segment length = %d, word = 0x%08x\n",len,dabufpi[ii]);fflush(stdout);
      if(print) printf("   Frame# = %d, timestamp_l=0x%08x, timestamp_h = 0x%08x\n",dabufpi[ii+1],dabufpi[ii+2],dabufpi[ii+3]);fflush(stdout);
      ii += 4;
      len = dabufpi[ii]&0xFFFF;
      padding = (dabufpi[ii]>>23)&0x1;
      if(print) printf("Payloads segment length = %d, word = 0x%08x, padding=%d\n",len,dabufpi[ii],padding);fflush(stdout);
      ii += 1;
      if(print) printf("   First payload word 0x%08x\n",dabufpi[ii]);fflush(stdout);
      ptr16 = (unsigned short *)&dabufpi[ii];
      if(padding==0) nslots = len*2;
      else       nslots = len*2-1;
      for(jj=0; jj<nslots; jj++)
      {
        payload = (*ptr16)&0x1F;
        slot = payload2slot[payload];
        /*if(print) printf("  Found payload# %d - slot# %d (module_id=%d, line_id=%d)\n",payload,slot,((*ptr16)>>8)&0xF,((*ptr16)>>5)&0x3);*/
        slots[jj] = slot;
        ptr16 ++;
      }
      ii += len;

      /* process slots */
      for(jj=0; jj<nslots; jj++)
      {
        slot = slots[jj];
        slot_len = dabufpi[ii];
        payload = (dabufpi[ii+1]>>16)&0xFFFF;
        if(print) printf("\n   Payload bank length = %d, second word = 0x%08x, payload# %d, slot# %d\n",slot_len,dabufpi[ii+1],payload,slot);

	//printf("===> roc_id=%d, stream_id=%d -> payload bank length = %d, second word = 0x%08x, payload# %d, slot# %d\n",roc_id,stream_id,slot_len,dabufpi[ii+1],payload,slot);
//for statistics
roc_active[roc_id] = 1;
ndata[roc_id][stream_id-1] += (slot_len-1);
mdata[roc_id][stream_id-1][slot] += (slot_len-1);
//for statistics

        if(print)
	{
          for(kk=2; kk<=slot_len; kk++)
          {
            val = dabufpi[ii+kk];
            q  = (val>> 0) & 0x1FFF;
            ch = (val>>13) & 0x000F;
            t  = ((val>>17) & 0x3FFF) * 4;
            //unsigned long long hit_time = frame_time_ns + t;
            //////////////if(print) printf("   Hit[%4d] = 0x%08x (ch=%2d t=%4d q=%4d)\n",kk-2,val,ch,t,q);
          }
	}

        ii += (slot_len+1);
      }
    }
    
    mstreams ++;
    //printf("[%d] ii=%d, mstreams=%d tot_len+AGGR_HEADER_MAX_LENGTH=%d,timestamp_l=0x%08x\n",ithread,ii,mstreams,tot_len+AGGR_HEADER_MAX_LENGTH,timestamp_l);

    if(print) printf("------------------ finished frame at ii=%d ------------------------------- mstreams=%d, nstreams_total=%d\n",ii,mstreams,nstreams_total);

    if(mstreams==nstreams_total) /* all streams processed */
    {
      /* for the first set: record nrocs/nstreams/etc information for the future use */
      if(theProc->nstreams_active == 0 && theProc->nsets == 0)
      {
        theProc->nstreams_active = nstreams_total;
        theProc->nrocs_active = nrocs_total;
        if(print) printf("\nsroGetSet: END OF FIRST SET\n");fflush(stdout);
      }
      theProc->nsets ++;
      set_len = ii - theProc->index - 1;
      FILL_AGGREGATED_HEADER;
      if(print)
      {
	PRINT_AGGREGATED_HEADER;
        printf("\nsroGetSet: END OF SET: found first frame from next time slice, returning 1: ii=%d words, nstreams_total=%d, nrocs_total=%d -> set_len = %d words\n\n",ii,nstreams_total,nrocs_total,set_len);fflush(stdout);
      }

      *dataindex = theProc->index; /*return current aggregated header index*/

      if(ii>=tot_len) /*end of big buffer*/
      {
        theProc->index = 1; /* set index for the new big buffer */
        return(0); /*whole big buffer processed*/
      }
      else
      {
        theProc->index = ii; /*point it to the next aggregated header*/      
        return(1); /* buffer still has some data*/
      }
    }
    
  } /*while(ii<tot_len)*/


  printf("ERROR: SHOULD NEVER BE HERE !!!!!\n");fflush(stdout);
  return(-1);
}




int
sroPrintSet(int *buf)
{
  int ii, jj, kk, payload, slot, val, q, ch, chgroup, t, pattern_28_00, pattern_47_29;
  int nslots, padding, module_id, slot_len, totlen, len, len1, len2, len3, len4, len5;
  int slots[MAXCHAN]; 
  unsigned short *ptr16;

  totlen=buf[0]+1;
#ifdef DEBUG
  printf("\n\ntotlen=%d\n",totlen);
#endif
  //for(ii=0; ii<totlen; ii++) printf("=============== [%2d] 0x%08x\n",ii,buf[ii]);printf("\n");
  ii = 0;

  printf("\n=== AGGREGATOR BANK ===\n\n");
  printf("[%5d] AGG length = %d words\n",ii,buf[ii]); ii++;
  printf("[%5d] AGG 2nd word: 0x%08x\n",ii,buf[ii]); ii++;
  printf("[%5d] SIB length = %d words\n",ii,buf[ii]); ii++;
  printf("[%5d] SIB 2nd word: 0x%08x\n",ii,buf[ii]); ii++;
  len=buf[ii]&0xFFFF;
  printf("[%5d] TSS: 1st word=0x%08x (len=%d), frame#=%d, timestamp_l=0x%08x, timestamp_h=0x%08x\n",
	 ii,buf[ii],len,buf[ii+1],buf[ii+2],buf[ii+3]);
  ii+=4;
  len1=buf[ii]&0xFFFF;
  printf("[%5d] AIS: 1st word=0x%08x (len1=%d)\n",ii,buf[ii],len1); ii++;
  for(jj=0; jj<len1; jj++)
  {
    printf("[%5d] ROC[%2d]: 0x%08x\n",ii,jj,buf[ii]);
    ii++;
  }
  
  printf("\n=== DATA FROM ROCS ===\n\n");
  while(ii<totlen)
  {
    len2 = buf[ii];
    printf("\n[%5d] Time slice bank length len2=%d words\n",ii,len2); ii++;
    printf("[%5d] Time slice bank 2nd word = 0x%08x (ROCID=%d)\n",ii,buf[ii],buf[ii]>>16); ii++;
    len3 = buf[ii];
    printf("[%5d] SIB length len3=%d words\n",ii,len3); ii++;
    printf("[%5d] SIB 2nd word: 0x%08x\n",ii,buf[ii]); ii++;
    len4=buf[ii]&0xFFFF;
    printf("[%5d] TSS: 1st word=0x%08x (len4=%d), frame#=%d, timestamp_l=0x%08x, timestamp_h=0x%08x\n",ii,buf[ii],len4,buf[ii+1],buf[ii+2],buf[ii+3]);
    ii+=4;
    len5=buf[ii]&0xFFFF;
    if(len5==0)
    {
      printf("[%5d] fake frame: 0x%08x, len5==0\n",ii,buf[ii]); /*there is no payload, probably fake frame*/
      ii++;
      continue;
    }
    padding = (buf[ii]>>23)&0x1;
    ptr16 = (unsigned short *)&buf[ii];
    if(padding==0) nslots = len5 * 2;
    else       nslots = len5 * 2 - 1;
    printf("[%5d] AIS: 1st word=0x%08x (len5=%d, padding=%d -> nslots=%d)\n",ii,buf[ii],len5,padding,nslots); ii++;
    for(jj=0; jj<nslots; jj++)
    {
      payload = (*ptr16)&0x1F;
      slot = payload2slot[payload];
      module_id = ((*ptr16)>>8)&0xF;
      printf("[%5d] Payload[%2d] = %d, slot = %d (module_id=%d, line_id=%d)\n",ii+jj,jj,payload,slot,module_id,((*ptr16)>>5)&0x3);
      slots[jj] = slot;
      ptr16 ++;
    }
    ii += len5;

    for(jj=0; jj<nslots; jj++)
    {
      slot_len = buf[ii];
      payload = (buf[ii+1]>>16)&0xFFFF;
      slot = slots[jj];
      printf("   Payload bank length = %d, second word = 0x%08x, payload# %d, slot# %d\n",slot_len,buf[ii+1],payload,slot);
      
      if(module_id==0) /*fadc250*/
      {
        for(kk=2; kk<=slot_len; kk++)
        {
          val = buf[ii+kk];
          q  = (val>> 0) & 0x1FFF;
          ch = (val>>13) & 0x000F;
          t  = ((val>>17) & 0x3FFF);
          printf("   FADC250 Hit[%4d] : slot=%2d ch=%2d t=%6d(4ns ticks) q=%4d\n",kk-2,slot,ch,t,q);
        }
      }
      else if(module_id==1) /*dcrb*/
      {
        for(kk=2; kk<=slot_len; kk+=2)
        {
	  val = buf[ii+kk];
	  chgroup = (val>>29)&0x7;
	  pattern_28_00 = val&0x1FFFFFFF;
	  val = buf[ii+kk+1];
	  pattern_47_29 = val&0x7FFFF;
	  t = (val>>19)&0x7FF;
          printf("   DCRB Hit[%4d] : slot=%2d  channels_group=%2d  t=%6d  pattern_47_29=0x%05x  pattern_28_00=0x%08x\n",kk-2,slot,chgroup,t,pattern_47_29,pattern_28_00);
	  if(chgroup==0) printf("   DCRB Hit[%5d] : slot=%2d,  time(32ns ticks)=%5d,  pattern for channels 47..00 is 0x%05x%08x\n",kk-2,slot,t,pattern_47_29,pattern_28_00);
	  else      printf("   DCRB Hit[%5d] : slot=%2d,  time(32ns ticks)=%5d,  pattern for channels 95..48 is 0x%05x%08x\n",kk-2,slot,t,pattern_47_29,pattern_28_00);
	}
      }
      else
      {
	printf("UNKNOWN MODULE_ID=%d\n",module_id);
      }
      
      ii += (slot_len+1);
    }

  }
  printf("\nEND OF DATA, ii=%d\n\n",ii);
  
  return(0);
}










int
sroEventBuilder(int *buf, int *bufout, int *evind)
{
  int ii, jj, kk, nn, mm, ii_save, payload, slot, val, q, ch, t, chgroup, pattern_28_00, pattern_47_29, nslots, padding, module_id, roc_id, stream_id, rocid;
  int uvw, strip, ustrip, vstrip, wstrip, dalitz, energy, energy_old, ticks_old, jj1, jj2, same_timing_in_progress, slot_len, totlen, len, len1, len2, len3, len4, len5;
  int slots[MAXCHAN]; 
  unsigned short *ptr16;

  int roc_stream_ii[256][4]; /* index of the data from particular roc/stream */ 
  int newslot[MAXSLOT];

  int peak, npeak, peak1[NARRAY/2], peak2[NARRAY/2], peakadc[NARRAY/2], begin_window_time;
  int ihit, ibin, ich, isec, io, ireg, isl;
  //int hits[NARRAY];
  
  int adc_sum, adc_sum2, adcs[NARRAY];
  int ecadcs[NARRAY][6][2][3][36], echits_per_plane[NARRAY][6][2][3], ecadcs_per_plane[NARRAY][6][2][3];
  int pcadcs[NARRAY][6][3][68], pchits_per_plane[NARRAY][6][3], pcadcs_per_plane[NARRAY][6][3];

  int dcrbs[NARRAY], dcrbs_sec_sl[NARRAY][6][6], dcrb_nsl;

  int nevents = 0, iievt = 0;

  //evio
  GET_PUT_INIT;
  unsigned int *bufoutptr = bufout;
  int status;
  int fragtag;
  int fragnum = 0/*-1*/;
  int banktyp = 0xf;
  int banktag = 0;
  int banknum = 0;
  uint32_t timestamp_l, timestamp_h;
  uint32_t timestamp_l_avg, timestamp_h_avg;
  uint64_t timestamp, timestamp_at_window_begin, timestamp_avg;
  uint64_t timestamp_avg_old = 0;
  uint32_t pattern_gtp;
  uint32_t pattern_fp;
  uint32_t pattern_lowfp_lowgtp_no_prescale;
  uint32_t event_type_word_count;
  uint32_t run_number;

  char *fmt;
  char *fmt_fadc = "c,i,l,N(c,N(s,i))"; /*slot,event#,timestamp,Nchannels(channel,Nhits(tdc,adc))*/
  char *fmt_dcrb = "c,i,l,N(c,s)"; /*slot,event#,timestamp,Nchannels(channel,tdc)*/
  int ind_out, *nchptr, *nhitptr, iev;

  int nhits[MAXCHAN];
#define WIN_SIZE ((WINDOW_BEFOR_IN_BINS+WINDOW_AFTER_IN_BINS+3)*TICKS_PER_BIN)  /*maximum possible window size in 4ns ticks; in theory, it can be up to 'TICKS_PER_BIN' hits in every bin*/
  unsigned short chan_time[MAXCHAN][WIN_SIZE];
  unsigned int chan_adc[MAXCHAN][WIN_SIZE]; 

  int ndcrbhits[MAXDCRBCHAN];
  unsigned short chan_dcrbtdc[MAXDCRBCHAN][WIN_SIZE];

  static int first = 1; 

  if(first)
  {
    initDcrbTranslationTable();
    first = 0;
  }


  //printf("\n====================================== sroEventBuilder reached ===================================\n\n");

  
  //evio

  for(jj=0; jj<256; jj++)
  {
    for(kk=0; kk<4; kk++) roc_stream_ii[jj][kk] = 0;
  }
  
  for(jj=0; jj<NARRAY; jj++)
  {
    //hits[jj] = 0;
    adcs[jj] = 0;
    dcrbs[jj] = 0;
    for(kk=0; kk<6; kk++) for(nn=0; nn<6; nn++) dcrbs_sec_sl[jj][kk][nn] = 0;
  }

  for(jj=0; jj<NARRAY; jj++)
  {
    for(isec=0; isec<6; isec++)
    {
      for(io=0; io<2; io++)
      {
        for(uvw=0; uvw<3; uvw++)
        {
          echits_per_plane[jj][isec][io][uvw] = 0;
          ecadcs_per_plane[jj][isec][io][uvw] = 0;
          for(strip=0; strip<36; strip++)
          {
	    ecadcs[jj][isec][io][uvw][strip] = 0;
          }
        }
      }
      for(uvw=0; uvw<3; uvw++)
      {
        pchits_per_plane[jj][isec][uvw] = 0;
        pcadcs_per_plane[jj][isec][uvw] = 0;
        for(strip=0; strip<68; strip++)
        {
	  pcadcs[jj][isec][uvw][strip] = 0;
        }
      }      
    }
  }
    
  totlen=buf[0]+1;
#ifdef DEBUG
  printf("\n\ntotlen=%d\n",totlen);
#endif
  //for(ii=0; ii<totlen; ii++) printf("=============== [%2d] 0x%08x\n",ii,buf[ii]);printf("\n");
  ii = 0;

  //printf("\n=== AGGREGATOR BANK ===\n\n");
  //printf("[%5d] AGG length = %d words\n",ii,buf[ii]);
  ii++;
  //printf("[%5d] AGG 2nd word: 0x%08x\n",ii,buf[ii]);
  ii++;
  //printf("[%5d] SIB length = %d words\n",ii,buf[ii]);
  ii++;
  //printf("[%5d] SIB 2nd word: 0x%08x\n",ii,buf[ii]);
  ii++;
  len=buf[ii]&0xFFFF;
  timestamp_l_avg = buf[ii+2];
  timestamp_h_avg = buf[ii+3];
  timestamp_avg = ((uint64_t)timestamp_h_avg<<32) | (uint64_t)timestamp_l_avg;
  //printf("[%5d] TSS: 1st word=0x%08x (len=%d), frame#=%d, timestamp_l=0x%08x, timestamp_h=0x%08x\n",
  //	 ii,buf[ii],len,buf[ii+1],buf[ii+2],buf[ii+3]);
  ii+=4;
  len1=buf[ii]&0xFFFF;
  //printf("[%5d] AIS: 1st word=0x%08x (len1=%d)\n",ii,buf[ii],len1);
  ii++;
  for(jj=0; jj<len1; jj++)
  {
    //printf("[%5d] ROC[%2d]: 0x%08x\n",ii,jj,buf[ii]);
    ii++;
  }
  ii_save = ii; //remember current position for second pass

#ifdef DEBUG
  printf("\n=== FIRST PASS - filling hits[], adcs[], dcrbs[] etc ===\n\n");
#endif

  ii = ii_save;
  while(ii<totlen)
  {
    len2 = buf[ii];
    //printf("\n[%5d] Time slice bank length len2=%d words\n",ii,len2);
    ii++;
    
    roc_id = (buf[ii]>>16)&0xFFFF;
    stream_id = (buf[ii]&0xF) - 1; /*stream_id starts from 1, here we want it to start from zero*/
#ifdef DEBUG
    printf("---> roc_id=%d, stream_id=%d\n",roc_id,stream_id);
#endif
    roc_stream_ii[roc_id][stream_id] = ii-1;

    //printf("[%5d] Time slice bank 2nd word = 0x%08x (ROCID=%d)\n",ii,buf[ii],buf[ii]>>16);
    ii++;
    len3 = buf[ii];
    //printf("[%5d] SIB length len3=%d words\n",ii,len3);
    ii++;
    //printf("[%5d] SIB 2nd word: 0x%08x\n",ii,buf[ii]);
    ii++;
    len4=buf[ii]&0xFFFF;
    //printf("[%5d] TSS: 1st word=0x%08x (len4=%d), frame#=%d, timestamp_l=0x%08x, timestamp_h=0x%08x\n",ii,buf[ii],len4,buf[ii+1],buf[ii+2],buf[ii+3]);
    ii+=4;
    len5=buf[ii]&0xFFFF;
    if(len5==0)
    {
      //printf("[%5d] fake frame: 0x%08x, len5==0\n",ii,buf[ii]); /*there is no payload, probably fake frame*/
      ii++;
      continue;
    }
    padding = (buf[ii]>>23)&0x1;
    //printf("[%5d] AIS: 1st word=0x%08x (len5=%d, padding=%d)\n",ii,buf[ii],len5,padding);
    ii++;
    ptr16 = (unsigned short *)&buf[ii];
    if(padding==0) nslots = len5 * 2;
    else       nslots = len5 * 2 - 1;
    for(jj=0; jj<nslots; jj++)
    {
      payload = (*ptr16)&0x1F;
      slot = payload2slot[payload];
      module_id = ((*ptr16)>>8)&0xF;
      //printf("[%5d] Payload[%2d] = %d, slot = %d (module_id=%d, line_id=%d)\n",ii,jj,payload,slot,module_id,((*ptr16)>>5)&0x3);
      slots[jj] = slot;
      ptr16 ++;
    }
    ii += len5;

    for(jj=0; jj<nslots; jj++)
    {
      slot_len = buf[ii];
      payload = (buf[ii+1]>>16)&0xFFFF;
      slot = slots[jj];
#ifdef DEBUG
      printf("   Payload bank length = %d, second word = 0x%08x, payload# %d, slot# %d\n",slot_len,buf[ii+1],payload,slot);
#endif      
      if(module_id==0) /*fadc250*/
      {
        for(kk=2; kk<=slot_len; kk++)
        {
          val = buf[ii+kk];
          q  = (val>> 0) & 0x1FFF;
          ch = (val>>13) & 0x000F;
          t  = ((val>>17) & 0x3FFF);
          //printf("   FADC250 Hit[%4d] : slot=%2d ch=%2d t=%6d(4ns ticks) ticks q=%4d\n",kk-2,slot,ch,t,q);

	  ihit = t;
	  ibin = ihit / TICKS_PER_BIN;
	  if(ibin<NARRAY)
	  {
            //hits[ibin] ++;
            adcs[ibin] += q;
	    //if(hits[ibin]>1) printf("fadcs: ihit=%d -> ibin=%d ---> hits[%d]=%d, adcs[%d]=%d (roc=%d, slot=%d chan=%d))\n",ihit,ibin,ibin,hits[ibin],ibin,adcs[ibin],roc_id,slot,ch);

	    rocid = (roc_id==84) ? 11: roc_id; /*huck for roc 84, which must be 11*/

	    /*ECAL translated hits*/
	    isec = ecal_rocid2sector[rocid];
	    if(isec>=0)
	    {
	      //printf("ECAL: roc_id=%d -> rocid=%d -> ibin=%d, isec=%d\n",roc_id,rocid,ibin,isec);fflush(stdout);
	      io = adcioecal_full[slot][ch] - 1;
	      uvw = adclayerecal_full[slot][ch] - 1;
	      strip = adcstripecal_full[slot][ch] - 1;

	      /*for multiplicity*/
	      echits_per_plane[ibin][isec][io][uvw] ++;
	      ecadcs_per_plane[ibin][isec][io][uvw] += q;
	      
	      /*for peak finder*/
              ecadcs[ibin][isec][io][uvw][strip] += q;
	    }
	    /*ECAL translated hits*/
	    
	    /*PCAL translated hits*/
	    isec = pcal_rocid2sector[rocid];
	    if(isec>=0)
	    {
	      //printf("PCAL: roc_id=%d -> rocid=%d -> ibin=%d, isec=%d\n",roc_id,rocid,ibin,isec);fflush(stdout);
	      uvw = adclayerpcal_full[slot][ch] - 1;
	      strip = adcstrippcal_full[slot][ch] - 1;

	      /*for multiplicity*/
	      pchits_per_plane[ibin][isec][uvw] ++;
	      pcadcs_per_plane[ibin][isec][uvw] += q;
	      
	      /*for peak finder*/
              pcadcs[ibin][isec][uvw][strip] += q;
	      //printf("pchits_per_plane[ibin=%d][isec=%d][uvw=%d]=%d (strip=%d, stripEn=%d)\n",ibin,isec,uvw,pchits_per_plane[ibin][isec][uvw],strip,pcadcs_per_plane[ibin][isec][uvw]);
	    }
	    /*PCAL translated hits*/
	  }
	  else
	  {
	    printf("ERROR: hits[]: ibin=%d >= NARRAY=%d\n",ibin,NARRAY);
	    exit(0);
	  }
	}
      }
      else if(module_id==1) /*dcrb*/
      {
	/* fills in dcrb overall occupancy (dcrbs[ibin]), and multiplicity per superlayer in each sector (dcrbs_sec_sl[ibin][isec][isl]) */
        for(kk=2; kk<=slot_len; kk+=2)
        {
	  val = buf[ii+kk];
	  chgroup = (val>>29)&0x7;
	  pattern_28_00 = val&0x1FFFFFFF;
	  val = buf[ii+kk+1];
	  pattern_47_29 = val&0x7FFFF;
	  t = (val>>19)&0x7FF;
	  //if(chgroup==0) printf("   DCRB Hit[%5d] : slot=%2d,  time(32ns ticks)=%5d,  pattern for channels 47..00 is 0x%05x%08x\n",
	  //		   kk-2,slot,t,pattern_47_29,pattern_28_00);
	  //else      printf("   DCRB Hit[%5d] : slot=%2d,  time(32ns ticks)=%5d,  pattern for channels 95..48 is 0x%05x%08x\n",
	  //		   kk-2,slot,t,pattern_47_29,pattern_28_00);

	  ihit = t * 8; //32ns -> 4ns
	  ibin = ihit / TICKS_PER_BIN;
	  isec = dcrb_rocid2sector[roc_id];
	  ireg = dcrb_rocid2region[roc_id];
	  isl = dcrb_slot2sl[slot] + ireg*2;
	  if(ibin<NARRAY)
	  {
	    for(mm=0; mm<29; mm++)
	    {
	      if( ((pattern_28_00>>mm)&0x1) )
	      {
	        //ich = mm;
		dcrbs[ibin] ++;
		dcrbs_sec_sl[ibin][isec][isl] ++;
	      }
	    }
	    for(mm=0; mm<19; mm++)
	    {
	      if( ((pattern_47_29>>mm)&0x1) )
	      {
	        //ich = mm + 30;
		dcrbs[ibin] ++;
		dcrbs_sec_sl[ibin][isec][isl] ++;
	      }
	    }
	    
	    //if(dcrbs_sec_sl[ibin][isec][isl]>4) printf("dcrbs_sec_sl[ibin=%5d][isec=%1d][isl=%1d]=%3d\n",ibin,isec,isl,dcrbs_sec_sl[ibin][isec][isl]);

	    //printf("dcrbs: ihit=%5d -> ibin=%3d ---> dcrbs[%3d]=%3d, dcrbs[%3d][%1d][%1d]=%3d) [roc_id=%d(sector=%d), slot=%d(sl=%d)]\n",
	    //	   ihit,ibin,ibin,dcrbs[ibin], ibin,isec,isl,dcrbs_sec_sl[ibin][isec][isl], roc_id,isec,slot,isl);

          }
	  else
	  {
	    printf("ERROR: dcrbs[]: ibin=%d >= NARRAY=%d\n",ibin,NARRAY);
	    exit(0);
	  }
	}
      }
      else
      {
	printf("UNKNOWN MODULE_ID=%d\n",module_id);
      }
      
      ii += (slot_len+1);
    }

  }

#ifdef DEBUG
  printf("\nEND OF DATA, ii=%d\n\n",ii);
  printf("\n=== FIND 'PLACES OF INTEREST' ===\n\n");
#endif

  

  /*
  for(isec=0; isec<6; isec++)
    for(isl=0; isl<6; isl++)
      for(ibin=0; ibin<NARRAY; ibin++)
	{
	  if(dcrbs_sec_sl[ibin][isec][isl]>4) printf("dcrbs_sec_sl[ibin=%5d][isec=%1d][isl=%1d]=%3d\n",ibin,isec,isl,dcrbs_sec_sl[ibin][isec][isl]);
	}
  */





  
//#define STRIP_THRESHOLD 100
//#define HIT_THRESHOLD  3000
#define STRIP_THRESHOLD 10
#define HIT_THRESHOLD  30


  /* following produces:
npeak - the number of peaks in particular timeframe
peak1[npeak] - beginning of every peak
peak2[npeak] - end of every peak
peakadc[npeak] - adc sum of every peak
   */

  npeak = 0;
  peak = 0;

#ifdef PEAK_FINDER /* use peak finder algorithm for ECAL/PCAL */

  /********************************************************/
  /* build peaks based on finding algorithm (dalitz rule) */
  /********************************************************/

  for(isec=0; isec<6; isec++)
  {
    //printf("occupancy: %5d ticks -> hits=%2d, adcs=%4d, dcrbs=%4d\n",jj*TICKS_PER_BIN,hits[jj],adcs[jj],dcrbs[jj]);

    for(jj=0; jj<NARRAY; jj++)
    {
      


#if 1  //PCAL


      if( (pchits_per_plane[jj][isec][0])==1 && // PCinner U
	  (pchits_per_plane[jj][isec][1])==1 && // PCinner V
	  (pchits_per_plane[jj][isec][2])==1 ) // PCinner W

      if( (pchits_per_plane[jj+1][isec][0])==0 && // PCinner U
	  (pchits_per_plane[jj+1][isec][1])==0 && // PCinner V
	  (pchits_per_plane[jj+1][isec][2])==0 ) // PCinner W
	
      for(ustrip=0; ustrip<68; ustrip++)
      {
        for(vstrip=0; vstrip<62; vstrip++)
        {
	  for(wstrip=0; wstrip<62; wstrip++)
	  {
	    dalitz = ustrip + vstrip + wstrip;
	    energy = pcadcs[jj][isec][0][ustrip] + pcadcs[jj][isec][1][vstrip] + pcadcs[jj][isec][2][wstrip];
	    
            if( (pcadcs[jj][isec][0][ustrip]>STRIP_THRESHOLD) &&
		(pcadcs[jj][isec][1][vstrip]>STRIP_THRESHOLD) &&
		(pcadcs[jj][isec][2][wstrip]>STRIP_THRESHOLD) &&
	        (dalitz>=115) &&
		(dalitz<=125) &&
		(energy>HIT_THRESHOLD) )
	    {
	      printf("\n\n[jj=%d][isec=%d] energy = %5d (%d %d %d),  ustrip,vstrip,wstrip = %2d %2d %2d -> dalitz=%d\n",jj,isec,energy,
		     pcadcs[jj][isec][0][ustrip],pcadcs[jj][isec][1][vstrip],pcadcs[jj][isec][2][wstrip],
		     ustrip,vstrip,wstrip,dalitz);


	      //#ifdef DEBUG1
	      printf("                    pc cluster[%6d ticks]: ustrip=%2d vstrip=%2d wstrip=%2d energy=%4d\n",jj*TICKS_PER_BIN,ustrip,vstrip,wstrip,energy);
	      //#endif
              if(peak==0) /*peak open*/
              {
                peak=1;
                peak1[npeak] = jj;
	        peakadc[npeak] = energy;
		//#ifdef DEBUG1
                printf("open peak #%d at %d bin (%d 4ns ticks), energy=%6d(peakadc=%6d)\n",npeak,peak1[npeak],peak1[npeak]*TICKS_PER_BIN,energy,peakadc[npeak]);
		//#endif
              }
              else /*peak middle*/
              {
	        peakadc[npeak] += energy;
		//#ifdef DEBUG1
                printf("cont peak #%d at %d bin (%d 4ns ticks), energy=%6d(peakadc=%6d)\n",npeak,peak1[npeak],peak1[npeak]*TICKS_PER_BIN,energy,peakadc[npeak]);
		//#endif
              }
	    }
	    else
	    {
              if(peak==1) /*peak close*/
              {
                peak=0;
                peak2[npeak]=jj;
		//#ifdef DEBUG1
                printf("close peak #%d at %d bin (%d 4ns ticks), energy=%6d(peakadc=%6d)\n",npeak,peak2[npeak],peak2[npeak]*TICKS_PER_BIN,energy,peakadc[npeak]);
		//#endif

		/*
		for(kk=jj-5; kk<jj+50; kk++)
		{
		  printf("==isec=%1d==> dcrb[%4d] ---> ",isec,kk);
		  for(isl=0; isl<6; isl++) printf("%5d",dcrbs_sec_sl[kk][isec][isl]);
		  printf(" <---\n");
		}
		*/



		if(peak1[npeak]==peak1[npeak-1])/*if previous peak has same 'peak1', update it's energy if needed, and do not create new one*/
	        {
		  //#ifdef DEBUG1
		  printf("--> ignore peak\n");
		  //#endif
		  if(peakadc[npeak]>peakadc[npeak-1])
		  {
		    //#ifdef DEBUG1
		    printf("----> update previous peak by energy=%d\n",peakadc[npeak]);
		    //#endif
		    peakadc[npeak-1] = peakadc[npeak];
		    peak2[npeak-1] = peak2[npeak];
		  }
	        }
	        else
	        {
	          npeak ++;
	        }
              }
	    }	  
	  }
        }
      }

#endif //PCAL





#if 0  //ECAL
      
      for(ustrip=0; ustrip<36; ustrip++)
      {
        for(vstrip=0; vstrip<36; vstrip++)
        {
	  for(wstrip=0; wstrip<36; wstrip++)
	  {
	    dalitz = ustrip + vstrip + wstrip;
	    energy = ecadcs[jj][isec][0][0][ustrip] + ecadcs[jj][isec][0][1][vstrip] + ecadcs[jj][isec][0][2][wstrip];

	    if(ecadcs[jj][isec][0][0][ustrip]>STRIP_THRESHOLD &&
               ecadcs[jj][isec][0][1][vstrip]>STRIP_THRESHOLD &&
               ecadcs[jj][isec][0][2][wstrip]>STRIP_THRESHOLD &&
	       dalitz>=72 &&
	       dalitz<=74 &&
	       energy>HIT_THRESHOLD )
	    {
	      //#ifdef DEBUG1
	      printf("                    ec cluster[%6d ticks]: ustrip=%2d vstrip=%2d wstrip=%2d energy=%4d\n",jj*TICKS_PER_BIN,ustrip,vstrip,wstrip,energy);
	      //#endif
              if(peak==0) /*peak open*/
              {
                peak=1;
                peak1[npeak] = jj;
	        peakadc[npeak] = energy;
#ifdef DEBUG1
                printf("open peak #%d at %d bin (%d 4ns ticks), energy=%6d(peakadc=%6d)\n",npeak,peak1[npeak],peak1[npeak]*TICKS_PER_BIN,energy,peakadc[npeak]);
#endif
              }
              else /*peak middle*/
              {
	        peakadc[npeak] += energy;
#ifdef DEBUG1
                printf("cont peak #%d at %d bin (%d 4ns ticks), energy=%6d(peakadc=%6d)\n",npeak,peak1[npeak],peak1[npeak]*TICKS_PER_BIN,energy,peakadc[npeak]);
#endif
              }
	    }
	    else
	    {
              if(peak==1) /*peak close*/
              {
                peak=0;
                peak2[npeak]=jj;
#ifdef DEBUG1
                printf("close peak #%d at %d bin (%d 4ns ticks), energy=%6d(peakadc=%6d)\n",npeak,peak2[npeak],peak2[npeak]*TICKS_PER_BIN,energy,peakadc[npeak]);
#endif	      
	        if(peak1[npeak]==peak1[npeak-1])/*if previous peak has same 'peak1', update it's energy if needed, and do not create new one*/
	        {
#ifdef DEBUG1
		  printf("--> ignore peak\n");
#endif
		  if(peakadc[npeak]>peakadc[npeak-1])
		  {
#ifdef DEBUG1
		    printf("----> update previous peak by energy=%d\n",peakadc[npeak]);
#endif
		    peakadc[npeak-1] = peakadc[npeak];
		    peak2[npeak-1] = peak2[npeak];
		  }
	        }
	        else
	        {
	          npeak ++;
	        }
              }
	    }	  
	  }
        }
      }

#endif //ECAL




      

      
    } /*for(jj=0; jj<NARRAY; jj++)*/
  } /*for(isec=0; isec<6; isec++)*/





  
#else /* PEAK_FINDER: use multiplicity algorithm for ECAL/PCAL */



  
  /******************************************/
  /* build peaks based on overall occupancy */
  /******************************************/

  /* summing current and next bin to avoid boundary effect */
  for(isec=0; isec<6; isec++)
  {
    for(jj=0; jj<(NARRAY-4); jj++)
    {
      /* calculate dcrb multiplicity per sector */
      dcrb_nsl = 0;
      for(isl=0; isl<6; isl++)
      {
	if((dcrbs_sec_sl[jj][isec][isl]+dcrbs_sec_sl[jj+1][isec][isl]+dcrbs_sec_sl[jj+2][isec][isl]+dcrbs_sec_sl[jj+3][isec][isl]+dcrbs_sec_sl[jj+4][isec][isl]+dcrbs_sec_sl[jj+5][isec][isl])>=3)
	{
	  dcrb_nsl ++;
	  //if(dcrb_nsl>4) printf("something in dcrb: isec=%d, isl=%d\n",isec,isl); 
	}
      }

      /* go through hits[]/adcs[] searching for peaks */
 
      if(pcadcs_per_plane[jj][isec][0]>30 && // PCAL U
         pcadcs_per_plane[jj][isec][1]>30 && // PCAL V
         pcadcs_per_plane[jj][isec][2]>30 && // PCAL W
         ecadcs_per_plane[jj][isec][0][0]>300 && // ECALinner U
         ecadcs_per_plane[jj][isec][0][1]>300 && // ECALinner V
         ecadcs_per_plane[jj][isec][0][2]>300 && // ECALinner W
	dcrb_nsl>=6 ) //at least ### superlayers
      {
	adc_sum = 0;
        for(uvw=0; uvw<3; uvw++) adc_sum += ecadcs_per_plane[jj][isec][0][uvw];
        printf("\n+++ YES[jj=%d] +++ isec=%d, adc_sum=%d\n",jj,isec, adc_sum);
	
        if(peak==0) /*peak open*/
        {
          peak=1;
          peak1[npeak] = jj;
	  peakadc[npeak] = adc_sum;
	  //#ifdef DEBUG1
          printf("[jj=%d] open peak #%d at %d bin (%d 4ns ticks), adc=%6d(peakadc=%6d)\n",jj,npeak,peak1[npeak],peak1[npeak]*TICKS_PER_BIN,adc_sum,peakadc[npeak]);
	  //#endif
        }
        else /*peak middle*/
        {
	  peakadc[npeak] += adc_sum;
	  //#ifdef DEBUG1
          printf("[jj=%d] cont peak #%d at %d bin (%d 4ns ticks), adc=%6d(peakadc=%6d)\n",jj,npeak,peak1[npeak],peak1[npeak]*TICKS_PER_BIN,adc_sum,peakadc[npeak]);
	  //#endif
        }
      }
      else
      {
	//printf("[jj=%d] 33 peak=%d\n",jj,peak);
        if(peak==1) /*peak close*/
        {
          peak=0;
          peak2[npeak]=jj;
	  //#ifdef DEBUG1
          printf("[jj=%d] close peak #%d at %d bin (%d 4ns ticks), adc=%6d(peakadc=%6d)\n",jj,npeak,peak2[npeak],peak2[npeak]*TICKS_PER_BIN,adc_sum,peakadc[npeak]);
	  //#endif
	  npeak ++;
        }
      }
      
    } /*for(isec=0; isec<6; isec++)*/
  } /*for(jj=0; jj<(NARRAY-1); jj++)*/



  
#endif /* PEAK_FINDER */



  
  //#ifdef DEBUG1
  if(npeak>0)
  {
    printf("\nPEAKS: npeak=%d\n",npeak);
    for(jj=0; jj<npeak; jj++)
    {
      printf("PEAK[%3d] peak1=%4d(%5d 4ns ticks) peak2=%4d(%5d 4ns ticks) peakadc=%6d\n",
	     jj,peak1[jj],peak1[jj]*TICKS_PER_BIN,peak2[jj],peak2[jj]*TICKS_PER_BIN,peakadc[jj]);
    }
    printf("\n");
  }
  //#endif



  /* NEED TO CLEANUP PEAKS ??? */

  




  
#ifdef DEBUG1
  if(npeak>0) printf("\n=== SECOND PASS - BUILDING ===\n\n");
#endif
  for(nn=0; nn<npeak; nn++) /* loop over peaks */
  {
#ifdef DEBUG1
    printf("\nEVENT: peak1=%d, peak2=%d\n",peak1[nn],peak2[nn]);
#endif
#ifdef DEBUG1
    printf("considering peak #%d: [peak1=%d(%d 4ns ticks), peak2=%d(%d 4ns ticks)], peakadc=%6d\n",
	   nn,peak1[nn],peak1[nn]*TICKS_PER_BIN,peak2[nn],peak2[nn]*TICKS_PER_BIN,peakadc[nn]);
#endif
    begin_window_time = (peak1[nn]-WINDOW_BEFOR_IN_BINS)*TICKS_PER_BIN; // time at the beginning of the window in 4ns ticks
#ifdef DEBUG1
    printf("TIMESTAMPS: peak1-WINDOW_BEFOR_IN_BINS = %4d bins -> begin_window_time=%5d 4ns ticks (timestamp_avg = 0x%llx (%lld))\n",peak1[nn]-WINDOW_BEFOR_IN_BINS,begin_window_time,timestamp_avg,timestamp_avg);
#endif


    /********************/
    /*SELECTION CRITERIA*/
    
    if(peakadc[nn]<HIT_THRESHOLD)
    {
      //#ifdef DEBUG1
      printf("\n======= peak #%d rejected, go to next one ==========\n\n",nn);
      //#endif
      continue;
    }
    //#ifdef DEBUG1
    //printf("\n++++++++ peak #%d accepted ==========\n\n",nn);
    //#endif

    /*SELECTION CRITERIA*/
    /********************/



    
    /* for every peak with appropriate amount of hits, we open event */
#ifdef DEBUG1
    printf("\n======= peak #%d accepted, build NEW EVENT ==========\n\n",nn);
    printf("\n======> calling evOpenEvent(0x%llx, %d)\n\n",&bufoutptr[iievt],129);
#endif
    
    evOpenEvent(&bufoutptr[iievt],129);
    evind[nevents] = iievt;

    for(roc_id=0; roc_id<256; roc_id++)
    {      
      /*skip not-existing rocs*/
      if(roc_stream_ii[roc_id][0]<=0) continue;
 
      /*always open fragment; evCloseFrag() will remove it if it is empty*/
      fragtag = roc_id;

/*HACK for EXPID=hpsrun*/
if(fragtag==84) fragtag=11;
/*HACK for EXPID=hpsrun*/
 
#ifdef DEBUG1
 printf("\n===> calling evOpenFrag(0x%llx, %d, %d) --- b08out=0x%llx\n\n",&bufoutptr[iievt],fragtag,fragnum,b08out);
#endif
      status = evOpenFrag(&bufoutptr[iievt], fragtag, fragnum);
      if(status<=0)
      {
        printf("error in evOpenFrag\n");
	exit(0);
      }

      for(stream_id=0; stream_id<4; stream_id++)
      {
	/*skip not-existing streams*/
	if(roc_stream_ii[roc_id][stream_id]<=0) continue;

        ii = roc_stream_ii[roc_id][stream_id];

        len2 = buf[ii];
        //printf("\n[%5d] Time slice bank length len2=%d words\n",ii,len2);
	ii++;
#ifdef DEBUG
        printf("[%5d] Time slice bank 2nd word = 0x%08x (ROCID=%d)\n",ii,buf[ii],buf[ii]>>16);
#endif
	ii++;
        len3 = buf[ii];
        //printf("[%5d] SIB length len3=%d words\n",ii,len3);
	ii++;
        //printf("[%5d] SIB 2nd word: 0x%08x\n",ii,buf[ii]);
	ii++;
        len4=buf[ii]&0xFFFF;
	timestamp_l = buf[ii+2];
	timestamp_h = buf[ii+3];
        //printf("[%5d] TSS: 1st word=0x%08x (len4=%d), frame#=%d, timestamp_l=0x%08x, timestamp_h=0x%08x\n",ii,buf[ii],len4,buf[ii+1],buf[ii+2],buf[ii+3]);
        ii+=4;
        len5=buf[ii]&0xFFFF;
        if(len5==0)
        {
          //printf("[%5d] fake frame: 0x%08x, len5==0\n",ii,buf[ii]); /*there is no payload, probably fake frame*/
          ii++;
          continue;
        }
        padding = (buf[ii]>>23)&0x1;
        //printf("[%5d] AIS: 1st word=0x%08x (len5=%d, padding=%d)\n",ii,buf[ii],len5,padding);
	ii++;
        ptr16 = (unsigned short *)&buf[ii];
        if(padding==0) nslots = len5 * 2;
        else       nslots = len5 * 2 - 1;
        for(jj=0; jj<nslots; jj++)
        {
          payload = (*ptr16)&0x1F;
          slot = payload2slot[payload];
          module_id = ((*ptr16)>>8)&0xF;
          //printf("[%5d] Payload[%2d] = %d, slot = %d (module_id=%d, line_id=%d)\n",ii,jj,payload,slot,module_id,((*ptr16)>>5)&0x3);
          slots[jj] = slot;
          ptr16 ++;
        }
        ii += len5;

        timestamp = ((uint64_t)timestamp_h<<32) | (uint64_t)timestamp_l;
        timestamp_at_window_begin = timestamp + (uint64_t)begin_window_time;
#ifdef DEBUG
        printf("timestamp: 0x%08x 0x%08x -> 0x%llx, timestamp_at_window_begin=0x%llx (%lld) (4ns ticks)\n",timestamp_h,timestamp_l,timestamp,timestamp_at_window_begin,timestamp_at_window_begin);
#endif
	/*assume we have only one type of boards in one ROC; have to be fixed in future !!!*/
	if(stream_id==0)
	{
          banktyp = 0xf;
	  if(module_id==0) /*FADC*/
	  {
            banktag = 0xe103;
	    fmt = fmt_fadc;
	  }
	  else if(module_id==1) /*DCRB*/
	  {
            banktag = 0xe116;
	    fmt = fmt_dcrb;
	  }
#ifdef DEBUG1
	  printf("\ncalling evOpenBank(0x%llx, %d, %d, 0x%04x, %d, 0x%x, '%s')\n\n",&bufoutptr[iievt],fragtag,fragnum,banktag,banknum,banktyp,fmt);
#endif
	  status = evOpenBank(&bufoutptr[iievt], fragtag, fragnum, banktag, banknum, banktyp, fmt, &ind_out);
 	  if(status<=0)
	  {
	    exit(0);
	  }
	  /*printf("evOpenBank returns = %d\n",status);*/
          b08out = (unsigned char *)&bufoutptr[iievt+ind_out];
          /*printf("first b08out = 0x%08x\n",b08out);*/
	}

	for(jj=0; jj<MAXSLOT; jj++) newslot[jj] = 1;
        for(jj=0; jj<nslots; jj++)
        {
          slot_len = buf[ii];
          payload = (buf[ii+1]>>16)&0xFFFF;
          slot = slots[jj];
#ifdef DEBUG
          printf("   Payload bank length = %d, second word = 0x%08x, payload# %d, slot# %d\n",slot_len,buf[ii+1],payload,slot);
#endif
          if(module_id==0) /*fadc250*/
          {
	    /*first pass - fills nhits[ch], chan_adc [ch][ nhits[ch] ], chan_time[ch][ nhits[ch] ] */
	    for(ch=0; ch<MAXCHAN; ch++) nhits[ch] = 0;
            for(kk=2; kk<=slot_len; kk++)
            {
              val = buf[ii+kk];
              q  = (val>> 0) & 0x1FFF;
              ch = (val>>13) & 0x000F;
              t  = ((val>>17) & 0x3FFF);
              //printf("   FADC250 Hit[%4d]: slot=%2d, ch=%2d, t=%6d(4ns ticks), q=%4d\n",kk-2,slot,ch,t,q);

	      ibin = t / TICKS_PER_BIN;
              if(ibin>=(peak1[nn]-WINDOW_BEFOR_IN_BINS) && ibin<=(peak1[nn]+WINDOW_AFTER_IN_BINS))
	      {
	        chan_time[ch][ nhits[ch] ] = t;
                chan_adc [ch][ nhits[ch] ] = q;
	        nhits[ch] ++;
		//printf("     ibin=%d (must be between %d and %d), nhits[ch=%2d] = %d\n",ibin,peak1[nn]-WINDOW_BEFOR_IN_BINS,peak1[nn]+WINDOW_AFTER_IN_BINS,ch,nhits[ch]);
	        if(nhits[ch]>=WIN_SIZE)
		{
		  printf("ERROR: nhits[ch=%2d]=%d, IT OVERFLOWS WIN_SIZE=%d\n",ch,nhits[ch],WIN_SIZE);
		  exit(0);
		}
	      }
	    }
	    
	    /*second pass - fills evio bank */
	    for(ch=0; ch<MAXCHAN; ch++) /* for every FADC channel .. */
            {
              //printf("   EVENT(FADC): nhits[%d]=%d\n",ch,nhits[ch]);
              if(nhits[ch]>0) /*if channel has at least one hit*/
	      {
		if(newslot[slot])
		{
		  PUT8(slot);
                  PUT32(iev);
                  PUT64(timestamp_at_window_begin); /*fadc timestamp in 4ns ticks*/
	          //printf("EVENT(FADC): slot=%d, iev=%d, timestamp_at_window_begin=0x%llx (%lld) (4ns ticks)\n",slot,iev,timestamp_at_window_begin,timestamp_at_window_begin);
                  nchptr = (unsigned int *)b08out; /*pointer to the number of channels*/
                  PUT32(0); /*the number of channels*/

		  newslot[slot]=0;
		}
		
                (*nchptr)++; /*update the number of channels*/
                PUT8(ch); /*put channel number*/
                PUT32(nhits[ch]); /*the number of hits in this channel*/
		//printf("   EVENT(FADC): chan=%d, nhits=%d\n",ch,nhits[ch]);
                for(kk=0; kk<nhits[ch]; kk++)
		{
                  //printf("      EVENT(FADC): time=%d(%d), adc=%d\n",chan_time[ch][kk],chan_time[ch][kk]-begin_window_time,chan_adc[ch][kk]);
                  PUT16(chan_time[ch][kk] - begin_window_time);
                  PUT32(chan_adc[ch][kk]);
		}
	      }
              //printf("EVENT: ch=%d, nhit=%d\n",ch,nhits[ch]);
            } /* loop over FADC channels */
  
          }
          else if(module_id==1) /*dcrb*/
          {
 	    /*first pass - fills nhits[ch], ... */
	    for(ch=0; ch<MAXDCRBCHAN; ch++) ndcrbhits[ch] = 0;
            for(kk=2; kk<=slot_len; kk+=2)
            {
	      val = buf[ii+kk];
	      chgroup = (val>>29)&0x7;
	      pattern_28_00 = val&0x1FFFFFFF;
	      val = buf[ii+kk+1];
	      pattern_47_29 = val&0x7FFFF;
	      t = (val>>19)&0x7FF; //in 32ns ticks
	      //if(chgroup==0) printf("   DCRB Hit[%5d] : slot=%2d,  time(32ns ticks)=%5d,  pattern for channels 47..00 is 0x%05x%08x\n",
	      //	   kk-2,slot,t,pattern_47_29,pattern_28_00);
	      //else      printf("   DCRB Hit[%5d] : slot=%2d,  time(32ns ticks)=%5d,  pattern for channels 95..48 is 0x%05x%08x\n",
	      //	   kk-2,slot,t,pattern_47_29,pattern_28_00);

#define DCRB_T0 460 //until figure out what is going on
	      
	      ibin = (t*8) / TICKS_PER_BIN; /*convert 't' from 32ns ticks to 4ns ticks, then convert to bins*/
	      //printf("---------------------------------- t = %4d(32ns) -> t*8 = %5d(4ns) -> ibin=%3d (%5d < peak1 < %5d)\n",t,t*8,ibin,(peak1[nn]-WINDOW_BEFOR_IN_BINS),(peak1[nn]+WINDOW_AFTER_IN_BINS));
              if(ibin>=((peak1[nn]-WINDOW_BEFOR_IN_BINS)) && ibin<=((peak1[nn]+WINDOW_AFTER_IN_BINS)))
	      {
		for(mm=0; mm<29; mm++)
		{
		  if((pattern_28_00>>mm)&0x1)
		  {
		    if(chgroup==0) ch = mm;
		    else           ch = mm + 48;
		    //printf("   ch1=%2d\n",ch);
		    
	            chan_dcrbtdc[ch][ ndcrbhits[ch] ] = (t*8 - begin_window_time)/2 + DCRB_T0; /*'t' in 32ns ticks, 'begin_window_time' in 4ns ticks -> equalize all to 8ns ticks*/
	            ndcrbhits[ch] ++;
	            if(ndcrbhits[ch]>255) {printf("OVERFULL\n");exit(0);}
		  }
		}
		for(mm=0; mm<19; mm++)
		{
		  if((pattern_47_29>>mm)&0x1)
		  {
		    if(chgroup==0) ch = mm + 29;
		    else           ch = mm + 29 + 48;
		    //printf("   ch2=%2d\n",ch);
		    
	            chan_dcrbtdc[ch][ ndcrbhits[ch] ] = (t*8 - begin_window_time)/2 + DCRB_T0; /*'t' in 32ns ticks, 'begin_window_time' in 4ns ticks -> equalize all to 8ns ticks*/
	            ndcrbhits[ch] ++;
	            if(ndcrbhits[ch]>255) {printf("OVERFULL\n");exit(0);}
		  }
		}
		
	      }
	    }

	    /*second pass - fills evio bank */
	    for(ch=0; ch<MAXDCRBCHAN; ch++) /* for every FADC channel .. */
            {
              //printf("   EVENT: nhits[%d]=%d\n",ch,nhits[ch]);
              if(ndcrbhits[ch]>0) /*if channel has at least one hit*/
	      {
		if(newslot[slot])
		{
		  PUT8(slot);
                  PUT32(iev);
                  PUT64(timestamp_at_window_begin/((uint64_t)2)); /*dcrb timestamp in 8ns ticks*/
	          //printf("EVENT(DCRB): slot=%d, iev=%d, timestamp_at_window_begin=0x%llx (%lld) (8ns ticks)\n",slot,iev,timestamp_at_window_begin/((uint64_t)2),timestamp_at_window_begin/((uint64_t)2));
                  nchptr = (unsigned int *)b08out; /*pointer to the number of channels*/
                  PUT32(0); /*the number of channels*/

		  newslot[slot]=0;
		}
		
                (*nchptr)++; /*update the number of channels*/
                PUT8(ch); /*put channel number*/
                /*PUT32(nhits[ch]);*/ /*the number of hits in this channel*/
		//printf("   EVENT(DCRB): chan=%d, nhits=%d\n",ch,nhits[ch]);
                for(kk=0; kk<1/*ndcrbhits[ch]*/; kk++)
		{
                  //printf("      EVENT(DCRB): time=%d (8ns ticks)\n",chan_dcrbtdc[ch][kk]);
                  PUT16(chan_dcrbtdc[ch][kk]); 
		}
	      }
              //printf("EVENT: ch=%d, nhit=%d\n",ch,nhits[ch]);
            } /* loop over FADC channels */
  
          }
          else
          {
	    printf("UNKNOWN MODULE_ID=%d\n",module_id);
          }
      
          ii += (slot_len+1);
        } /* loop over slots */
#ifdef DEBUG
        printf("\nEND OF STREAM DATA, ii=%d\n\n",ii);
#endif
      } /*stream_id*/

      /*if something was in stream 0, bank was opened, so close it*/
      if(roc_stream_ii[roc_id][0/*stream_id*/]>0)
      {
#ifdef DEBUG1	
        printf("calling evCloseBank(0x%llx, %d, %d, 0x%04x, %d, 0x%llx)\n",&bufoutptr[iievt],fragtag,fragnum,banktag,banknum,b08out);
#endif
        status = evCloseBank(&bufoutptr[iievt], fragtag, fragnum, banktag, banknum, b08out);
        if(status<0)
        {
	  exit(0);
        }
      }

#ifdef DEBUG1
      printf("\n===> calling evCloseFrag(0x%llx, %d, %d) --- b08out=0x%llx\n\n",&bufoutptr[iievt],fragtag,fragnum,b08out);
#endif
      status = evCloseFrag(&bufoutptr[iievt], fragtag, fragnum);
      if(status<0)
      {
        printf("error in evCloseFrag\n");
	exit(0);
      }

      
    } /*roc_id*/








    /***********************************/
    /* create fake trig1 bank (tag=37) */
    fragtag = 37;
    //fragnum = 0;
#ifdef DEBUG1
    printf("\n===> calling evOpenFrag(0x%llx, %d, %d)\n\n",&bufoutptr[iievt],fragtag,fragnum);
#endif
    status = evOpenFrag(&bufoutptr[iievt], fragtag, fragnum);
    if(status<=0)
    {
      printf("error in evOpenFrag(37)\n");
      exit(0);
    }


    /***********************/
    /* emulate trig1 banks */

    banktag = 0xe10a;
    banknum = 0;
    banktyp = 1;

#ifdef DEBUG1	
    printf("\ncalling evOpenBank(0x%llx, %d, %d, 0x%04x, %d, 0x%x, '%s')\n\n",&bufoutptr[iievt],fragtag,fragnum,banktag,banknum,banktyp,fmt);
#endif
    status = evOpenBank(&bufoutptr[iievt], fragtag, fragnum, banktag, banknum, banktyp, fmt, &ind_out);
    if(status<=0)
    {
      exit(0);
    }
    b08out = (unsigned char *)&bufoutptr[iievt+ind_out];
    /*printf("first b08out = 0x%08x\n",b08out);*/

    event_type_word_count = 0x29010006;
    pattern_gtp = 0xffffffff;
    pattern_fp = 0xffffffff;
    pattern_lowfp_lowgtp_no_prescale = 0xffffffff;

    PUT32(event_type_word_count);
    PUT32(iev); //event number
    PUT32(timestamp_l);
    PUT32(timestamp_h);
    PUT32(pattern_gtp);
    PUT32(pattern_fp);
    PUT32(pattern_lowfp_lowgtp_no_prescale);

    status = evCloseBank(&bufoutptr[iievt], fragtag, fragnum, banktag, banknum, b08out);
    if(status<0)
    {
      exit(0);
    }


    banktag = 0xe10f;
    banknum = 0;
    banktyp = 1;
#ifdef DEBUG1	
    printf("\ncalling evOpenBank(0x%llx, %d, %d, 0x%04x, %d, 0x%x, '%s')\n\n",&bufoutptr[iievt],fragtag,fragnum,banktag,banknum,banktyp,fmt);
#endif
    status = evOpenBank(&bufoutptr[iievt], fragtag, fragnum, banktag, banknum, banktyp, fmt, &ind_out);
    if(status<=0)
    {
      exit(0);
    }
    b08out = (unsigned char *)&bufoutptr[iievt+ind_out];
    /*printf("first b08out = 0x%08x\n",b08out);*/

    run_number = 22700;

    PUT32(0);
    PUT32(run_number);     //run number
    PUT32(iev);            //event number
    PUT32(0);              //event unix time
    PUT32(0);              //event type
    PUT32(1);              //roc pattern ???

    status = evCloseBank(&bufoutptr[iievt], fragtag, fragnum, banktag, banknum, b08out);
    if(status<0)
    {
      exit(0);
    }


    status = evCloseFrag(&bufoutptr[iievt], fragtag, fragnum);
    if(status<0)
    {
      printf("error in evCloseFrag\n");
      exit(0);
    }

		
    //printf("bufoutptr[%d]: length=%d, second word = 0%08x\n",iievt,bufoutptr[iievt],bufoutptr[iievt+1]);
    nevents ++;
    iievt += (bufoutptr[iievt]+1);
    iev ++;
    
    //return(nevents); //temporary for debugging
  
  } /* loop over peaks */
  

  //nevents = 50; // temp !!!!!!!!!!!
  
  return(nevents);
}



void
sroStatPrint(int clean)
{
  int i,j,slot;
  for(i=0; i<MAXROCS; i++)
  {
    if(roc_active[i])
    {
      printf("\n===> ROC[%d]\n",i);
      for(j=0; j<2; j++)
      {
        printf("     stream[%d]: %8d words\n",j,ndata[i][j]);
        if(ndata[i][j]>0)
	{
          for(slot=3; slot<21; slot++) if(mdata[i][j][slot]>0) printf("          slot %2d -> %8d words\n",slot,mdata[i][j][slot]);
	}
      }
    }
  }

  if(clean)
  {
    for(i=0; i<MAXROCS; i++)
    {
      for(j=0; j<2; j++)
      {
        ndata[i][j] = 0;
        for(slot=3; slot<21; slot++) mdata[i][j][slot] = 0;
      }
    }
  }
  
  return;
}



#endif /*if defined(Linux_armv7l)*/
