
/* LINK_support.c */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <sys/prctl.h>

#include "rolInt.h"
#include "da.h"
#include "bigbuf.h"
#include "circbuf.h"
#include "libdb.h"
//#include "eviofmt.h"
#include "LINK_support.h"

#include "codautil.h"
#include "ipc.h"



#define ABS(x)      ((x) < 0 ? -(x) : (x))

#define TIMERL_VAR			   \
  static hrtime_t startTim, stopTim, dTim; \
  static int nTim; \
  static hrtime_t TTT, Tim, rmsTim, minTim=10000000, maxTim, normTim=1

#define TIMERL_START \
{ \
  startTim = gethrtime(); \
}

#define TIMERL_STOP(whentoprint_macros,id_macros) \
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
    if(nTim >= whentoprint_macros) \
    { \
      TTT = Tim/nTim/normTim; \
      if(TTT>1000LL) {printf("timer[%d]: %7llu microsec (min=%7llu max=%7llu)\n", id_macros, TTT,minTim/normTim,maxTim/normTim);fflush(stdout);} \
      /*if(TTT<500LL) {printf("timer[%d]: %7llu microsec (min=%7llu max=%7llu)\n", id_macros, TTT,minTim/normTim,maxTim/normTim);fflush(stdout);}*/ \
      /*else {printf("timer[%d]: %7llu microsec (min=%7llu max=%7llu) !!!!!!!!\n", id_macros, TTT,minTim/normTim,maxTim/normTim);fflush(stdout);}*/ \
      nTim = Tim = 0; \
    } \
  } \
  else \
  { \
    /*logMsg("bad:  %d %ud %ud -> %d\n",nTim,startTim,stopTim,Tim,5,6);*/ \
  } \
}



//#define DEBUG




#define SSIPC

#ifdef SSIPC
#if defined(Linux_vme) || defined(Linux_x86_64) || defined(Linux_armv7l)
#define SEND_STA_FOR_CMSG_ROCS
#endif
#endif


#define CODA_ERROR 1
#define CODA_OK 0


/* external data */

extern char *mysql_host; /* coda_component.c */
extern char *expid; /* coda_component.c */
extern char configname[128]; /* coda_component.c; defined by coda_ebc.c in Download */

extern int64_t *dataSent; /* see coda_component.c */

/*static*/ extern objClass localobject; //to get run number for cMsg records

int deflt; /* 1 for CODA format, 0 for BOS format (see coda_eb_inc.c) */

WORD128 roc_linked;

CIRCBUF *roc_queues[MAX_ROCS];          /* for cb_put_...; see coda_eb.c; index is just roc index from 0 to nrocs in daq configuration */
int      roc_queue_ix;                  /* see deb_component.c; the number of rocs in daq configuration */

BIGBUF *gbuftmp[MAX_ROCS];              /* for bb_write/bb_read; allocated in coda_ebc.c; first index is just roc index from 0 to nrocs in daq config  */

uint32_t *bufpool[MAX_ROCS][QSIZE]; /* for 'eb_proc_thread'; allocated in coda_ebc.c; first index is just roc index from 0 to nrocs in this configuration  */

static int ending_for_recv;
static ROLPARAMS *rolP3[MAX_ROCS];






/* for cMsg-like buffers processing; index here is just rocid */

/* for MPD readout we assume that recond can be maximum 8 blocks, and the number of events in block can be maximum 25 */
#define MAXCMSGEVENTS (/*25*/36*8) /* maximum number of events in record x maximum number of events in block */

static int neeee[MAX_ROCS]; /*buffers length */
static unsigned int *eeee[MAX_ROCS][MAXCMSGEVENTS]; /*buffers*/

static int32_t total_nevents[MAX_ROCS];
static int64_t total_nlongs[MAX_ROCS];
static int oldevents[MAX_ROCS];
static int64_t oldlongs[MAX_ROCS];
static time_t oldtime[MAX_ROCS];

#ifdef SEND_STA_FOR_CMSG_ROCS
static int ipc_inited[MAX_ROCS];
#endif



#ifdef SEND_STA_FOR_CMSG_ROCS
void
check_timer(int rocid, int interval)
{
  time_t timediff, newtime = time(NULL);
  float data[4];
  float event_rate = 0.0, data_rate = 0.0;
  char name[100];
  int eventdiff;
  int nevents=0; 
  int64_t nlongs = 0L;
  char roc_name[CODAUTIL_STRLEN];

  if ( (newtime - oldtime[rocid]) >= interval )
  {
    get_roc_name(expid, rocid, roc_name);

    event_rate = data_rate = 0.0;
    nevents = total_nevents[rocid];
    nlongs = total_nlongs[rocid];
    //printf("check_timer is running ..\n");fflush(stdout);

    timediff = newtime - oldtime[rocid];
    eventdiff = nevents - oldevents[rocid];

    if(timediff > 0)
    {
      if(eventdiff > 0)
      {
        /* let roc know that we will send 'sta:' messages instead of him */
        //printf("send_control_message(roc_name >%s<)\n",roc_name);fflush(stdout);
        send_control_message("command:coda_ebc", "nostats");

	
        event_rate = eventdiff / timediff;
  	//printf("check_timer(roc_name >%s<): event_rate: %f (%u - %u)\n",roc_name,event_rate,nevents,oldevents);fflush(stdout);
        data_rate = 4.0 * (((float)nlongs) - ((float)oldlongs[rocid])) / ((float)timediff);
	//printf("check_timer(roc_name >%s<): RATE1 data_rate=%f (timediff=%d)\n",roc_name,data_rate,timediff);fflush(stdout);

        sprintf(name,"STA:%s",roc_name);
        data[0] = (float)nevents;
        data[1] = event_rate;
        data[2] = (float)((nlongs*4)/1048576);
        data[3] = data_rate/1048576;
        epics_json_msg_send(name, "float", 4, data);

        oldevents[rocid] = nevents;
        oldlongs[rocid] = nlongs;
        oldtime[rocid] = newtime;
      }
    }

  }
}
#endif





//#define USLEEP 10000 /* usleep parameter: 1000000 is 1 sec*/
#define USLEEP 2000 /* usleep parameter: 1000000 is 1 sec*/




/*
 bufin[]   - input data
 bufout[]  - output data after disentangling
*/

int
gem_disentangler(int fd, uint32_t *bufin, uint32_t *bufout)
{
  int roc_id, ltot, lev, ltiblock, ltiev, lmpd, mpd_total_length, syncFlag, evtype, btype, ievent, mpd_bank_second_word, mpd_block_trailer;
  int kk, sync_flag, event_type, time_stamp, event_number, nblocks, nevents, block_length, mpd_block_header;
  int ii, jj, ii_next_evio_bank, ii_mpd_block_header, ii_mpd_event_start, ii_mpd_event_end, needed_size;
  unsigned int *evptr, iev, *record_header, *event_header;
  uint32_t *events[MAXCMSGEVENTS];
  
  //printf("\n[fd=%2d] gem_disentangler reached !!!\n",fd);fflush(stdout);
  //printf("[fd=%2d] gem_disentangler reached: bufin[0]=%d\n",fd,bufin[0]);fflush(stdout);
  //printf("[fd=%2d] gem_disentangler reached !!!\n\n",fd);fflush(stdout);

  /* all data in bufin[]; bufin[0] contains total data length; allocate adequate space if it is not done already, or done but not enough */
  roc_id = bufin[BBIROCID];
  
  needed_size = bufin[0]; /* DO WE NEED THAT MUCH !!! */
  if(needed_size>neeee[roc_id])
  {
    for(ii=0; ii<MAXCMSGEVENTS; ii++)
    {
      //printf("[fd=%2d][ii=%2d] data size %d, buffer size %d\n",fd,ii,needed_size,neeee[roc_id]);
      neeee[roc_id] = needed_size + 64;
      eeee[roc_id][ii] = (unsigned int *)calloc(neeee[roc_id],sizeof(int));
      if(eeee[roc_id][ii]==NULL)
      {
        printf("[fd=%2d] alloc failed - exit\n",fd);
        exit(0);
      }
      else
      {
        printf("[%2d][ii=%2d] allocated %d words for eeee[%d] at %p\n",fd,ii,neeee[roc_id],roc_id,eeee[roc_id][ii]);
      }
    }
  }
  for(ii=0; ii<MAXCMSGEVENTS; ii++)
  {
    events[ii] = (unsigned int *)eeee[roc_id][ii];
#ifdef DEBUG
    //printf("===> [%2d] events[%d] = 0x%08x\n",fd,ii,events[ii]);fflush(stdout);
#endif
  }


  /*for Prestart, cleanup counters and subscribe for IPC*/
  if(bufin[0]<=13) /* non-physics events */
  {
    char roc_name[CODAUTIL_STRLEN];
    printf("111\n");fflush(stdout);
    get_roc_name(expid, roc_id, roc_name);
    printf("222: roc_name >%s<\n",roc_name);fflush(stdout);
    printf("CODA transition ? bufin[9]=0x%08x (type=%d ?)\n",bufin[9],(bufin[9]>>16));fflush(stdout);
      
    total_nevents[roc_id] = 0;
    total_nlongs[roc_id] = 0L;
    oldtime[roc_id] = 0;

#ifdef SEND_STA_FOR_CMSG_ROCS
    if(ipc_inited[roc_id]==0)
    {
      epics_json_msg_sender_init("clasrun", "clasprod", "daq", roc_name, NULL, NULL);
      printf("333\n");fflush(stdout);
      ipc_inited[roc_id] = 1;
    }
#endif

    /*just copy bufin to bufout*/
    memcpy(bufout,bufin,bufin[0]*sizeof(uint32_t));

  }
  else /*if(bufin[0]<=13)*/
  {

    /*RECORD HEADER: points to the beginning of bufout[], it points to 'destination'*/
    /* example:
           bufin[ 0] = 0x00004988 (18824)    <--- record length (contains several blocks !)
           bufin[ 1] = 0x00000001 (1)       - buffer number
           bufin[ 2] = 0x00000008 (8)       - nwords in this header
           bufin[ 3] = 0x00000008 (8)       - the number of blocks
           bufin[ 4] = 0x00000031 (49)      - rocid
           bufin[ 5] = 0x00000204 (516)
           bufin[ 6] = 0x00000000 (0)
           bufin[ 7] = 0xc0da0100 (-1059454720)
    */

    total_nlongs[roc_id] += bufin[0]; /* increment word counter */ 
  
    record_header = bufout;
    nblocks = bufin[BBIEVENTS]; // block may contains 1 or more events
#ifdef DEBUG
    printf("\n !!! nblocks = %d !!!\n\n",nblocks);
#endif
      
    /*reset total word counter*/
    ltot = 0;

    for(jj=0; jj<8; jj++) /* copy record header to bufout[] 'as is' */
    {
      bufout[jj] = bufin[jj];
      ltot++;
    }

    ii = 8;
    /* from now on, 'ii' will be index in bufin[] array, which is source, so 'ii' is source index */
    /* we will bump it while moving through bufin[] array */
    /* bufout will be destination pointer */


    ievent = 0; /* event counter for entire record */
    /***************************************************************************/
    /* loop over all events in bufin[], and create 'correct' record in bufout[] */
    /* bufin[] contains several blocks, each block contains several events      */
    /***************************************************************************/

    while(ii<bufin[0])
    {
      /**************************************************************/
      /* process one block, will come back here for every new block */
      /**************************************************************/
	
      /* we are at the beginning of the block of events, get some info from 2-word header and skip it */
      /* example for block_level=2:
            bufin[ 8] = 0x0000092f (2351)       <--- block length (2 events)
            bufin[ 9] = 0x00311002 (3215362)

            bufin[10] = 0x0000000b (11)         <--- TI data length (2 events)
            bufin[11] = 0xff112002 (-15654910)

             bufin[12] = 0x040001fe (67109374)
             bufin[13] = 0x01000000 (16777216)
             bufin[14] = 0xd83513ec (-667610132)
             bufin[15] = 0x000030d4 (12500)
             bufin[16] = 0x000056da (22234)

             bufin[17] = 0x040001fe (67109374)
             bufin[18] = 0x02000000 (33554432)
             bufin[19] = 0x9cbf40ec (-1665187604)
             bufin[20] = 0x000040fb (16635)
             bufin[21] = 0x000056da (22234)
      */ 

#ifdef DEBUG
      printf("PROCESSING BLOCK, BUFIN[from ii=%d]: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x ...\n",ii,bufin[ii],bufin[ii+1],bufin[ii+2],bufin[ii+3],bufin[ii+4]);
#endif
	
      block_length = bufin[ii]+1; /* block length */
      nevents = bufin[ii+1]&0xFF; /* the number of events in block */
      //printf("[%2d] ii=%d, nevents=%d (0x%08x)\n",fd,ii,nevents,bufin[ii+1]);
      sync_flag = (bufin[ii+1]>>28)&0x1; /*sync flag (bit 28 from 0), will need it below*/

      ii += 2; /* skip block header - bump index to the beginning of the TI data block */

      ltiblock = bufin[ii]+1; /* the length of TI data, it will be several TI events */

      /* some calculations and checks */
      if( (bufin[ii+1]&0xFF) != nevents) /* this error was observed if 'sysctl -w net.core.busy_read=50' was tun as root on EB machine*/
      {
        printf("cMSG: ERROR: the number of events = %d, the number of ti events = %d (ii=%d) -> exit\n",nevents, bufin[ii+1]&0xFF, ii);
        printf("  [%2d] bufin[0]=%d\n",fd,bufin[0]);
        for(jj=/*0*/bufin[0]-10; jj<bufin[0]; jj++) printf("    [%2d] bufin[%d] = 0x%08x (%d)\n",fd,jj,bufin[jj],bufin[jj]);
        exit(1);
      }
      if( nevents > MAXCMSGEVENTS)
      {
        printf("cMSG: ERROR: the number of events = %d, MAXCMSGEVENTS = %d -> exit\n",nevents, MAXCMSGEVENTS);
        printf("  [%2d] bufin[0]=%d\n",fd,bufin[0]);
        for(jj=0; jj<bufin[0]; jj++) printf("    [%2d] bufin[%d] = 0x%08x (%d)\n",fd,jj,bufin[jj],bufin[jj]);
        exit(1);
      }
      if( ((ltiblock-2)%nevents) != 0)
      {
        printf("cMSG: ERROR: the number TI data words = %d, nevents = %d (not even !) -> exit\n",ltiblock-2,nevents);
        printf("  [%2d] bufin[0]=%d\n",fd,bufin[0]);
        for(jj=0; jj<bufin[0]; jj++) printf("    [%2d] bufin[%d] = 0x%08x (%d)\n",fd,jj,bufin[jj],bufin[jj]);
        exit(1);
      }
      ltiev = (ltiblock-2) / nevents; /* the number of TI data words in one event (source) */
	
      ii += 2; /* skip TI header (source) - bump index to the first TI event */



      /*
           ievent - current event number being processed
           *events[MAXCMSGEVENTS] - array to place event to
           iev - current index ?
      */


      /**********************************************************************/
      /* loop over TI events, and in every event destination eeee[][], create event header, TI bank and HEAD bank */

        
#ifdef DEBUG
      printf("\n===> nevents=%d (in block)\n",nevents);fflush(stdout);
#endif	  
      for(iev=ievent; iev<(ievent+nevents); iev++)
      {
        event_header = evptr = events[iev]; /*event start pointer ('destination') */
        lev = 0; /*event length */
#ifdef DEBUG
        printf("\n===> iev=%d, evptr=0x%08x\n",iev,evptr);fflush(stdout);
#endif	  
        /* get needed info from TI bank */
        event_type   = bufin[ii]&0xFF; //event type
        event_number = LSWAP(bufin[ii+1]) & 0xFF; //event number low 8 bits
        time_stamp   = LSWAP(bufin[ii+2]) & 0xFF; //time stamp low 8 bits
#ifdef DEBUG
        printf(" ---> sync_flag=%d, event_type=0x%02x, time_stamp=0x%02x, event_number=%d\n",sync_flag,event_type,time_stamp,event_number);
#endif

        /* fill EVENT HEADER: first word is event length (will update below), second is info needed by event builder a la ROL2 */
        *evptr++ = 0;
        *evptr++ = (sync_flag<<24) | (event_type<<16) | ((time_stamp&0xFF)<<8) |  (event_number&0xFF); /* emulate ROL2 output to make EB happy ! */
        lev +=2;
        ltot +=2;
	
        /* fill TI HEADER */
        /* 'ltiev' is the number of data words in source TI bank (length is exclusive here) */
        /*also can get it from 1st TI word and compare ...*/
        *evptr++ = ltiev+1;
	  
        /* 0xe10A is TI/TS hardware format tag */ 
        *evptr++ = 0xe10a0100;

        /* copy TI DATA */
        for(jj=ii; jj<(ii+ltiev); jj++)
        {
          *evptr++ = LSWAP(bufin[jj]); //TI data from VTP are swapped, swap it back ...?
        }
        ii += ltiev;
        lev += (ltiev+2); //+2 counts for our new TI HEADER
        ltot += (ltiev+2);

        /* fill HEAD BANK */
        *evptr++ = 7;
        *evptr++ = 0xe10f0100;
        *evptr++ = 0; // version number
        *evptr++ = localobject->runNumber; // run number
        *evptr++ = event_number; // event number
        *evptr++ = time(0); // event unix time
        *evptr++ = 0; // roc pattern
        *evptr++ = 0; // event classification (17,18,20 etc)
        lev += 8;
        ltot += 8;

        event_header[0] = lev - 1; /*update event length (length is exclusive here) */
      }



      /**********************************************/
      /* we are done with TI info, will process MPD */
      /**********************************************/
	
      /* at this point, 'ii' is the index of the beginning of MPD data */
      /* example (ii=22):
           bufin[ii  ] = 0x00000921 (2337)        <--- evio bank: mpd data block total length (2 events)
           bufin[ii+1] = 0x0de90101 (233373953)   <--- evio bank: second word

      following indicates the beginning of the data from one MPD board, it may contains several events
           bufin[ii+2] = 0x86c01000 (-2034233344) <-- type=0x10+0x0 -> MPD block header (rotaryid=27, block_level=2)

      have to go through the data looking for the following MPD event headers, this way will find events
           bufin[ii+3] = 0xe0ba9c00 (-524641280)  <-- type=0x10+0xc -> MPD event header word0
           bufin[ii+4] = 0x00000438 (1080)        <--                  MPD event header word1
           bufin[ii+5] = 0x00000001 (1)           <--                  MPD event header word2

           bufin[ii+6] = 0xab070000 (-1425604608) <-- type=0x10+0x5 -> APV header (flags=01100, fiber=7)
           bufin[ii+7] = 0x007a2292 (8004242)
           bufin[ii+8] = 0x007643a9 (7750569)
           bufin[ii+9] = 0x007863f2 (7889906)
           ................................

      when meet following trailer, we finished with current MPD board, set event count to zero and continue to the following MPD board
           bufin[ii+..] = 0x88......                  <-- type=0x10+0x1 -> MPD block trailer (2 words ?)
      */


	  
      mpd_total_length = bufin[ii] + 1;                      /*evio bank header first word: mpd data length*/
      mpd_bank_second_word = (bufin[ii+1]&0xFFFFFF00) | 0x1; /*evio bank header first word: last byte is the number of events in block, we set it to 1 */
      ii_next_evio_bank = ii + mpd_total_length;
      ii += 2; /* now it is the index of the first mpd block header */


	


      /*************************/
      /* search for mpd events */
      iev = ievent; /* */
      ii_mpd_block_header = 0;
      ii_mpd_event_start = 0;
      ii_mpd_event_end = 0;
      for(jj=ii; jj<ii_next_evio_bank; jj++)
      {
        if( ((bufin[jj]>>27)&0x1F) == (0x10+0x0) ) /* MPD block header - we found new MPD board */
        {
	  iev = ievent; 
	  ii_mpd_block_header = jj;
	  mpd_block_header = bufin[jj]; /* remember it, will copy it to every event */
        }
        //else if( ((bufin[jj]>>27)&0x1F) == (0x10+0x1) ) /* found mpd block trailer */
        //{
        //  ;
        //}
        else if( (((bufin[jj]>>27)&0x1F) == (0x10+0xc)) || (((bufin[jj]>>27)&0x1F) == (0x10+0x1)) ) /* found mpd event header or block trailer */
        {
          if(ii_mpd_event_start == 0) /* found new event, and we do not have opened event - remember event start index */
	  {
	    ii_mpd_event_start = jj;
	    ii_mpd_event_end = 0;
#ifdef DEBUG
	    printf("\nSet ii_mpd_event_start=%d\n",ii_mpd_event_start);fflush(stdout);
#endif
	  }
	  else /* found new event, and we do have opened event - copy event */
	  {
	    ii_mpd_event_end = jj; /* remember event end index - NEXT index after event * !!! */

	    /* get information about events collected so far */
            event_header = events[iev]; /*event start pointer ('destination') */
            lev = event_header[0] + 1; /*event length */
            evptr = (unsigned int *)&events[iev][lev]; /*current event pointer ('destination') */
#ifdef DEBUG
	    printf("\n SO FAR: iev=%d, lev=%d\n\n",iev,lev);fflush(stdout);
#endif    
	    lmpd = ii_mpd_event_end - ii_mpd_event_start; /* size of mpd event */

	    /*for every event, copy 3 words: 2 words for evio bank, and block header*/
            *evptr++ = (lmpd+4-1); /* count for 2-word bank header, block header, block trailer, and -1 because evio length is exclusive */
            *evptr++ = mpd_bank_second_word;
	    *evptr++ = mpd_block_header;

            memcpy(evptr, &bufin[ii_mpd_event_start], lmpd * sizeof(int));
            evptr += lmpd;

	    *evptr++ = mpd_block_trailer; /*UNDEFINED !!!*/
#ifdef DEBUG
	    printf("\n---> copied mpd event from ii_mpd_event_start=%d to ii_mpd_event_end-1=%d, lmpd=%d\n",
		     ii_mpd_event_start,ii_mpd_event_end-1,lmpd);fflush(stdout);
#endif
 	    lev += (lmpd+4);
	    ltot += (lmpd+4);
#ifdef DEBUG
	    printf("\n Updating  event_header[0] at 0x%lx to lev-1=%d\n\n",event_header,lev-1);fflush(stdout);
#endif
            event_header[0] = lev - 1; /*update event length in event header (length is exclusive here) */

	    /* settings for the next event */
	    iev ++;
	    //ii_mpd_block_header = jj;
	    if( (((bufin[jj]>>27)&0x1F) == (0x10+0xc)) )
	    {
	      ii_mpd_event_start = ii_mpd_event_end; /* if we found event header - use it as next event start */
	    }
	    else
	    {
	      ii_mpd_event_start = 0; /* if we found block trailer, set it to zero, we will find next event header later */
	    }
	  }    
        }
      }

	
#ifdef DEBUG
      for(iev=ievent; iev<(ievent+nevents); iev++)
      {
        event_header = events[iev]; /*event start pointer ('destination') */
        lev = event_header[0] + 1; /*event length so far*/
        printf("\n===> iev=%d, lev=%d\n",iev,lev);fflush(stdout);
        for(jj=0; jj<lev; jj++) printf("EVT[%d] = 0x%08x (%d)\n",jj,event_header[jj],event_header[jj]);fflush(stdout);
      }
#endif

      ievent += nevents; // 'ievent' now is the number of events processed so far
      /*************************************************************************/
      /* finish processing one block, go back to the beginning of 'while' loop */
      /* it can be more blocks left, or it can be end of data                  */
      /*************************************************************************/


      /* bump 'ii' to the first word after the evio bank just processed */
      ii = ii_next_evio_bank;
	
    } /*while(ii<bufin[0])*/

  
    nevents = ievent; // total number of events found in all blocks
    record_header[0] = ltot; // update length in record header (length is inclusive here)
    record_header[BBIEVENTS] = nevents; // set the number of events to record header (it was the number of blocks)

    total_nevents[roc_id] += nevents; /* increment total event counter */ 

      
    /*************************************************************************************************/
    /* now we have events in events[] arrays; copy them to bufout[] array, right below record header */
    /************************************************************************************************/
      
#ifdef DEBUG
    printf("\n\n=========================== COPING EVENTS TO bufout[], nevents=%d\n",nevents);
#endif

    ii = 8; /* bufout[0]-bufout[7] contains record header already */
    for(iev=0; iev<nevents; iev++)
    {
      event_header = evptr = events[iev]; /*event start pointer (now it is 'source') */
      lev = event_header[0] + 1; /*event length*/
#ifdef DEBUG
      printf("\n--- coping iev=%d, lev=%d -> &bufout[%d]\n",iev,lev,ii);
#endif

      memcpy(&bufout[ii], evptr, lev * sizeof(int));
      ii += lev;
    }


#ifdef DEBUG
    printf("\n  rec: ltot = %d\n\n",ltot);
    for(jj=0; jj<ltot; jj++) printf("  REC[%2d] = 0x%08x (%d)\n",jj,record_header[jj],record_header[jj]);
#endif

  } /*if(bufin[0]<=13)*/

  
  #ifdef SEND_STA_FOR_CMSG_ROCS
  if(ipc_inited[roc_id])
  {
    check_timer(roc_id, 2);
  }
#endif

  return(0);
}











void *
eb_proc_thread(DATA_LINK theLink)
{
  int fd, ix, roc_id, ii, ifend, nev, nw, len, nrols, res;
  uint32_t *bufin, *bufout;
#ifdef Linux
  char thread_name[1024];
  sprintf(thread_name,"PROC.%s\0",theLink->name);
  prctl(PR_SET_NAME,thread_name);
#endif
  char *compname, *dash_pos;
  char rolnames[MAX_NUM_ROLS][LISTARGV2], rolparams[MAX_NUM_ROLS][LISTARGV2];

  ROLPARAMS *rolP = theLink->rolP;
  printf("MMM: roc_queue_ix=%d, theLink->rolP=%p\n",roc_queue_ix,theLink->rolP);fflush(stdout);

  compname = strdup(theLink->name);
  dash_pos = strchr(compname, '-');
  /* Check if the character '-'  was found */
  if(dash_pos != NULL)
  {
    /* Replace the '-' with a null terminator */
    *dash_pos = '\0';
  }
  
  printf("[%2d] eb_proc_thread: theLink->name=%s -> compname=%s\n",ix,theLink->name,compname);
  printf("[%2d] eb_proc_thread: set rolP at %p\n",ix,rolP);


  
  fd = theLink->fd;
  ix = theLink->roc_queue->roc;


  nrols = codaGetReadoutLists(configname, compname, rolnames, rolparams);
  if(nrols<=0) exit(0);
  printf("[%2d] eb_proc_thread: name=%s, nrols=%d\n",ix,compname,nrols);fflush(stdout);
  for(ii=0; ii<nrols; ii++) printf("[%2d] eb_proc_thread:   rol[%d]: name=%s, usr=%s\n",ix,ii,rolnames[ii],rolparams[ii]);fflush(stdout);
  
  if( (nrols==3) && (strcmp(rolnames[2],"none")!=0) )   /* ROL3 is defined and not 'none', load it */
  {
    printf("[%2d] BEFOR: %p\n",ix,rolP->rol_code);fflush(stdout);
    res = codaLoadROL3(rolP, rolnames[2], rolparams[2]);
    printf("[%2d] AFTER: %p (res=%d)\n",ix,rolP->rol_code,res);fflush(stdout);
    if(res)
    {
      printf("[%2d] eb_proc_thread: ERROR: codaLoadROL3 returned %d\n",ix,res);fflush(stdout);
      goto exit;
    }

    rolP->daproc = DA_INIT_PROC;
    rolP->pid = theLink->roc_queue->rocid;

    /* execute ROL init procedure (described in 'rol.h') */
    (*(rolP->rol_code)) (rolP);

    /* check if initialization was successful */
    if(rolP->inited != 1)
    {
      printf ("[%2d] eb_proc_thread: ERROR: ROL3 initialization failed\n",ix);fflush(stdout);
      goto exit;
    }
    else
    {
      printf ("[%2d] eb_proc_thread: INFO: ROL3 initialized\n",ix);fflush(stdout);
    }

    rolP->daproc = DA_DOWNLOAD_PROC;
    (*rolP->rol_code) (rolP);
    
    rolP->daproc = DA_PRESTART_PROC;
    (*rolP->rol_code) (rolP);
    
    rolP->daproc = DA_GO_PROC;
    (*rolP->rol_code) (rolP);
  }
  else
  {
    printf("[%2d] eb_proc_thread will just copy\n",ix);fflush(stdout);
  }


  while(theLink->exit==0)
  {
    /* wait for input buffer */
    bufin = bb_read(&(theLink->gbufin));
    if(bufin == NULL)
    {
      printf("[%2d] eb_proc_thread: ERROR: bufin==NULL\n",ix);fflush(stdout);
      break;
    }

    /* get some info from input buffer */
    nw = bufin[BBIWORDS];
    len = nw << 2;
    ifend = bufin[BBIEND];
    nev   = bufin[BBIEVENTS];
    roc_id = bufin[BBIROCID];

    /* get free output buffer from pool; will wait if nothing is available */
    bufout = NULL;
    while(bufout == NULL)
    {
      uint32_t *buftmp;
      for(ii=0; ii<QSIZE; ii++)
      {
	buftmp = bufpool[ix][ii];
	if(buftmp[0] == 0) /* means 'free buffer'; it is marked as 'free' in cb_events_get() */
        {
          bufout = buftmp;
#ifdef DEBUG
          printf("[%2d] handle_link(): rocid=%d (ix=%d): got free buffer %d at %p\n",ix,theLink->roc_queue->rocid,ix,ii,bufout);
          fflush(stdout);
#endif
          break;
	}
      }
      if(bufout == NULL)
      {
        usleep(USLEEP);/*sleep some short time*/
      }
    }


    /* it means that it was no data from recv() */
    if( (bufin[0]==2) && (bufin[1]==0xFFFFFFFF) )
    {
      if(put_cb_data(fd, &theLink->roc_queue, (void *) -1) < 0)
      {
        printf("[%2d] ----------------------------------------------\n",ix);
        printf("[%2d] ----------------------------------------------\n",ix);
        printf("[%2d] handle_link(): put_cb_data(1) returns < 0     \n",ix);
        printf("[%2d] ----------------------------------------------\n",ix);
        printf("[%2d] ----------------------------------------------\n",ix);
        break;
      }
      printf("[%2d] eb_proc_thread: received empty bufin - exit thread\n",ix);fflush(stdout);
      break;
    }

    
    
    if(rolP->rol_code == NULL) /* if ROL3 not loaded, just copy events to output buffer */
    {
      memcpy(bufout, bufin, bufin[0] * sizeof(int));
    }
    else
    {
      //printf("[%2d] eb_proc_thread: bufin at %p, bufout at %p\n",ix,bufin,bufout);fflush(stdout);
      //printf("[%2d] eb_proc_thread: rolP at %p\n",ix,rolP);fflush(stdout);
      //printf("[%2d] eb_proc_thread: rolP->pid = %d\n",ix,rolP->pid);fflush(stdout);
      
      rolP->dabufpi = (int32_t *) bufin;
      rolP->dabufp = (int32_t *) bufout;
      bufout[0] = 0;

      
      rolP->daproc = DA_POLL_PROC;
      (*rolP->rol_code) (rolP);
      
      /* let see now what we've got from ROL3 */
      ///???printf("len=%d, nev=%d\n",rolP->user_storage[0],rolP->user_storage[1]);

      /*
      printf("\n[%2d] eb_proc_thread: start disentangling, roc_id=%d, bufin[0]=%d\n\n",ix,roc_id,bufin[0]);fflush(stdout);
      gem_disentangler(fd, bufin, bufout);
      printf("\n[%2d] eb_proc_thread: done disentangling, roc_id=%d, bufout[0]=%d\n\n",ix,roc_id,bufout[0]);fflush(stdout);
      */
    }


    if(put_cb_data(fd, &theLink->roc_queue, (void *) bufout) < 0)
    {
      printf("[%2d] ----------------------------------------------\n",ix);
      printf("[%2d] ----------------------------------------------\n",ix);
      printf("[%2d] handle_link(): put_cb_data(2) returns < 0     \n",ix);
      printf("[%2d] ----------------------------------------------\n",ix);
      printf("[%2d] ----------------------------------------------\n",ix);
      fflush(stdout);
      break;
    }
    
  } /*while()*/

exit:
  
  pthread_exit(NULL); // Terminate the thread
}




/* called from 'handle_link'; reads one 'big' buffer from 'fd' */
/* returns number of bytes read or -1 if error (i.e. EOF) */

int
LINK_sized_read(int fd, int ix, char **buf)
{
  int size;	/* size of incoming packet */
  int cc, ii, jj, len, llenw, *tmp;
  int rembytes;	/* remaining bytes */
  int n_retries = 0;
  int n_retry2;
  unsigned int netlong;	/* network byte ordered length */
  char *bufferp = 0;
  unsigned int lwd, magic;
  unsigned int *bigbuf;
  int n_ending;
  int cMSG = 0; //will be set to 1 id cMSG arrives
  int swap_is_needed;
  TIMERL_VAR;


#ifdef DEBUG
  printf("\n[%2d] LINK_sized_read reached\n",fd);fflush(stdout);
#endif


  /* Wait for all the data requested */
  int recv_flags = MSG_WAITALL;

  n_ending = 0;

recv_again:

  /* read header off socket */
  rembytes = sizeof(netlong); //always 4 (?)
  bufferp = (char *) &netlong;

#ifdef DEBUG
  printf("[%2d] LINK_sized_read: at the beginning rembytes=%d\n",fd,rembytes);fflush(stdout);
#endif


  while(rembytes)
  {

    /*
    {
      int nbytes;
      socklen_t lbytes = sizeof(nbytes);
      //setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (int *) &nbytes, lbytes); 
      getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &nbytes, &lbytes); 
	  
      printf("[%2d] socket buffer size nbytes=%d(0x%08x) bytes, lbytes=%d\n",fd,nbytes,nbytes,lbytes);
	  
    }
    */

#if 1
    TIMERL_START;
#endif
    
#ifdef DEBUG
    printf("[%2d] RECV1: bufferp=0x%08x, rembytes=0x%08x, recv_flags=%d\n",
      fd, bufferp, rembytes, recv_flags);fflush(stdout);
    printf("[%2d] befor recv(1): rembytes=%d\n",fd,rembytes);
#endif
    //printf("[%2d] befor recv(1): rembytes=%d\n",ix,rembytes);
    cc = recv(fd, bufferp, rembytes, /*MSG_DONTWAIT*/recv_flags);
    //printf("[%2d] after recv(1): rembytes=%d cc=%d\n",ix,rembytes,cc);
#ifdef DEBUG
    printf("[%2d] after recv(1): rembytes=%d cc=%d\n",fd,rembytes,cc);
    bigbuf = (unsigned int *)bufferp; //just for print
    printf("[%2d] after recv(1): char buffer[0-3] = 0x%02x 0x%02x 0x%02x 0x%02x, netlong = 0x%08x, %d)\n",fd,
      bigbuf[0],bigbuf[1],bigbuf[2],bigbuf[3],netlong,LSWAP(netlong));
#endif

#if 1
    TIMERL_STOP(100,ix);
#endif
    
    if(cc == -1)
    {
      if(errno == EWOULDBLOCK)
      {

        if(ending_for_recv)
        {
          n_ending ++;
          printf("[%2d] ending_for_recv=%d n_ending=%d - wait for ROC\n",
            fd,ending_for_recv,n_ending);
          if(n_ending >= 10)
	  {
            printf("[%2d] ROC is not reporting -> force ending\n",fd);
            return(0);
	  }
        }

        /* retry */
	usleep(USLEEP);
        /* we do not know if ROC still alive and will send more data later,
        or ROC is dead ... */
		
        printf("[%2d] LINK_sized_read(): recv would block, retrying ...",fd);
        fflush(stdout);
		
      }
      else
      {
        printf("[%2d] LINK_sized_read() ERROR1\n",fd);
        fflush(stdout);
        if(errno != ECONNRESET)
        {
          perror("read ");
        }
        return(-1);
      }
    }
    else
    {
      /* It is OK for a socket to return with wrong number of bytes. */
      if(cc != rembytes)
      {
        if(cc > rembytes)
        {
          /* read() returned more bytes than requested!?!?!?! */
          /* this can't happen, but appears to anyway */
          printf("[%2d] ERROR: LINK_sized_read(,,%d) = read(,,%d) = %d!?!?!\n",
				 fd,size,rembytes,cc);
          printf("[%2d] ERROR: recv() returned more chars than requested - exit.\n",fd);
          fflush(stdout);
          exit(0);
        }
        else if(cc == 0) /* we are here if ROC closed connection ?! */
        {

          /* let other LINK threads know we are ending, in case if some ROC
          crashed and cannot send buffer with 'End' transition */
          ending_for_recv = 1;
          printf("\n[%2d] LINK_sized_read(): set ending_for_recv = 1 !!!!!!!!!!!!!!!!!!!!!!!!\n",fd);
          printf("[%2d] LINK_sized_read(): set ending_for_recv = 1 !!!!!!!!!!!!!!!!!!!!!!!!\n",fd);
          printf("[%2d] LINK_sized_read(): set ending_for_recv = 1 !!!!!!!!!!!!!!!!!!!!!!!!\n\n",fd);

          printf("[%2d] LINK_sized_read(): closed\n",fd);
          fflush(stdout);
          return(0);
        }
      
        printf("[%2d] LINK_sized_read(): recv(,,%d) returned %d\n",fd,rembytes,cc);
        fflush(stdout);
      }

      /* Always adjust these to get out of the while. */
      bufferp += cc;
      rembytes -= cc;
    }

  } /* first 'recv' loop */



  

  size = (int) ntohl(netlong); /* ntohl() return the argument value
                                  converted from network to host byte order */
#ifdef DEBUG
  printf("[%2d] LINK_sized_read(): ===== size = %d (netlong = 0x%08x (%d))\n",fd,size,netlong,netlong);
  fflush(stdout);
#endif

  /* read data */
  if(size == 0)
  {
    /* GHGHGH */
    unsigned int *p;
    printf("[%2d] WARNING: LINK_sized_read(): WARNING: zero length block from ROC\n",fd);
    fflush(stdout);

    p = (unsigned int *) *buf;
    p[BBIWORDS]  = -1;
    p[BBIBUFNUM] = -1;
    p[BBIROCID]  = -1;
    p[BBIEVENTS] = -1; /* setting this to -1 makes handle_link ignore the buffer */
    p[BBHEAD]    = -1;
    p[BBHEAD+1]  = -1;

    return(24);
  }
  else if(size==1)
  {
    if( strcmp(localobject->state,"active") ) /*print if NOT active*/
    {
      printf("[%2d] LOOKS LIKE cMSG - just ignore it and call recv() again\n",fd);
    }
    cMSG = 1;
    goto recv_again;
  }
  else
  {
    if( strcmp(localobject->state,"active") ) /*print if NOT active*/
    {
      printf("[%2d] INFO: LINK_sized_read(): size=%d bytes - just proceed\n",fd,size);
    }    
  }

  if(size > TOTAL_RECEIVE_BUF_SIZE)
  {
    printf("[%2d] ERROR: LINK_sized_read(): ERROR2: buffer size=%d bigger than TOTAL_RECEIVE_BUF_SIZE=%d - exit.\n",fd,size+6,TOTAL_RECEIVE_BUF_SIZE);
    fflush(stdout);
    exit(0);
  }

#ifdef DEBUG
  printf("[%2d] LINK_sized_read(): have %d bytes buffer at %p\n",fd,size+6,(*buf));
  fflush(stdout);
#endif



  
  /* Sergey: CHANGE THIS IF BIGBUFS CHANGED IN roc_component.c */
  *((unsigned int *) *buf) = (size >> 2); /* put buffer size in 1st word */
  /*printf("[%2d] 12345: %d (%d 0x%08x)\n",fd,*((unsigned int *) *buf),size,*buf);fflush(stdout);*/
  bufferp = *buf/* + sizeof(size)*/;           /* set pointer to 2nd word */


  

  rembytes = size;
  n_retry2 = 0;

  while(rembytes)
  {

retry1:

	/*
    printf("[%2d] RECV2: bufferp=0x%08x, rembytes=0x%08x, recv_flags=%d\n",
    fd, bufferp, rembytes, recv_flags);fflush(stdout);
	*/


    //printf("[%2d] befor recv(2): rembytes=%d cc=%d\n",ix,rembytes,cc);fflush(stdout);
    cc = recv(fd, bufferp, rembytes, recv_flags);
    //printf("[%2d] after recv(2): rembytes=%d cc=%d\n",ix,rembytes,cc);fflush(stdout);


#ifdef DEBUG
    bigbuf = (unsigned int *)bufferp;
    printf("[%2d] after recv(2): data = 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",fd,
           bigbuf[0],bigbuf[1],bigbuf[2],bigbuf[3],bigbuf[4],bigbuf[5],bigbuf[6],bigbuf[7],bigbuf[8],bigbuf[9]);fflush(stdout);
#endif


    /*printf("[%2d] 2: %d %d\n",fd,rembytes,cc);*/
    /*
    printf("[%2d] cc=0x%08x\n",fd,cc);fflush(stdout);
    */
    if(cc == -1)
    {
      if(errno == EWOULDBLOCK)
      {
	printf("[%2d] goto retry1\n",fd);fflush(stdout);
	goto retry1;
      }
      if(errno != ECONNRESET) perror("read2");
      puts("Error 2.");
      printf("[%2d] LINK_sized_read(): cc = %d, Errno is %d.\n",fd, cc, errno);
      fflush(stdout);
      return(-1);
    }

    if(cc == 0)
    { /* EOF - process died */
      /* GHGHGH */
      printf("[%2d] process died (cc==0) - return\n",fd);
      fflush(stdout);
      return(0);
    }
    else if(cc > rembytes)
    {
      /* read() returned more bytes than requested!?!?!?! */
      /* this can't happen, but appears to anyway */

      printf("[%2d] LINK_sized_read(): returned more bytes than requested!?!?!?!\n",fd);
      printf("[%2d] LINK_sized_read(,,%d) = read(,,%d) = %d!?!?!\n",fd,
              size,rembytes,cc);
      printf("[%2d] LINK_sized_read(): recv() returned more chars than requested - exit.\n",fd);
      fflush(stdout);
      exit(0);
    }
    else if(cc != rembytes)
    {
      /* (cc > 0) && (cc < rembytes) */
      printf("[%2d] LINK_sized_read(): cc=%d != rembytes=%d -> retry ...\n",fd,cc,rembytes);
      fflush(stdout);
      n_retry2++;
    }
#ifdef DEBUG
    printf("[%2d] LINK_sized_read(): recv(,,%d) returned %d\n",fd,rembytes,cc);
    fflush(stdout);
#endif
    /* Always adjust these to get out of the while loop */
    bufferp += cc;
    rembytes -= cc;

  } /*while(rembytes)*/



  /**********************/
  /* we received buffer */

  bigbuf = (unsigned int *) *buf;

  /*GEM check
  if(cMSG)
  {
    len = LSWAP(bigbuf[0]);
    printf("[%2d] GEM: len=%d\n",fd,len);
    printf("[%2d] GEM: last 5 words: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",fd,
	   LSWAP(bigbuf[len-5]),LSWAP(bigbuf[len-4]),LSWAP(bigbuf[len-3]),LSWAP(bigbuf[len-2]),LSWAP(bigbuf[len-1]));
    fflush(stdout);
  }
  GEM check*/
  
  /* check buffer integrity */
  /*
  bb_check(bigbuf);
  */

#ifdef DEBUG
  printf("[%2d] bigbuf: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",fd,
  bigbuf[0],bigbuf[1],bigbuf[2],bigbuf[3],bigbuf[4],bigbuf[5],bigbuf[6],bigbuf[7]);
#endif



  /*************************/
  /* swap buffer if needed */
  swap_is_needed = 0;
  magic = bigbuf[BBIFD];
  if(cMSG)
  {
    if(magic==0x0001dac0) swap_is_needed = 1;
  }
  else
  {
    if(magic == 0x01020304) swap_is_needed = 1;
  }

  if(swap_is_needed)
  {
    llenw = size >> 2;
#ifdef DEBUG
    printf("[%2d] SWAP (0x%08x), llenw=%d\n",fd,magic,llenw);
#endif
    bufferSwap(bigbuf,llenw);
  }
  else
  {
#ifdef DEBUG
    printf("[%2d] DO NOT SWAP (0x%08x)\n",fd,magic);
#endif
  }

#ifdef DEBUG
  printf("\n[%2d] buffer header: length=%d words, buffer#=%d, rocid=%d, #events=%d, fd/magic=0x%08x, end=%d\n",fd,
  bigbuf[BBIWORDS],bigbuf[BBIBUFNUM],bigbuf[BBIROCID],bigbuf[BBIEVENTS],bigbuf[BBIFD],bigbuf[BBIEND]);
#endif



  tmp = (int *)(*buf);

#ifdef DEBUG
  printf("\n[%2d] buffer contents:\n",fd);fflush(stdout);
  for(ii=0; ii<tmp[0]; ii++) printf("   [%2d] tmp[%2d] = 0x%08x (%d)\n",fd,ii,tmp[ii],tmp[ii]);
  printf("[%2d] end\n",fd);fflush(stdout);
#endif



  /* set appropriate bit letting building thread know we are ready */
  SetBit128(&roc_linked, tmp[BBIROCID]);
  /*printf("[%2d] LINK_sized_read(): set roc_linked for rocid=%d\n",fd,tmp[BBIROCID]);
  Print128(&roc_linked);*/
  
  return(size);
}



/* thread receiving data from rocs; it puts data into 'gbuftmp' circular buffer to be used by 'eb_proc_thread' */

void *
handle_link(DATA_LINK theLink)
{
  int fd;
  int numRead;
  int headerSize;
  char *errMsg;
  int count, i, itmp;
  char *name_from;
  char *dash_pos;
  TIMERL_VAR;
  
#ifdef Linux
  char thread_name[1024];
  sprintf(thread_name,"LINK.%s\0",(char *)theLink->name);
  prctl(PR_SET_NAME,thread_name);
  /*prctl(PR_SET_NAME,"coda_er1");*/
#endif

  //sergey: for rhel9 char ipaddress[20];
  char ipaddress[INET6_ADDRSTRLEN];
  char ipaddress1[INET6_ADDRSTRLEN];
  char ipaddress2[INET6_ADDRSTRLEN];

  struct sockaddr_in from;
  int len;
  char *address;
  char host_from[80];
  MYSQL *dbsock;
  char tmpp[1000];
  unsigned int *bufout;


  printf("[%d] handle_link: cleanup pool of buffers for roc=%d (rocid=%d) ..\n",theLink->fd,theLink->roc_queue->roc,theLink->roc_queue->rocid);
  for(i=0; i<QSIZE; i++)
  {
    uint32_t *buftmp;
    buftmp = bufpool[theLink->roc_queue->roc][i];
    if(buftmp != NULL) buftmp[0] = 0; /* mark buffer as free */
    else
    {
      printf("[%d] handle_link ERROR: bufout==NULL\n",theLink->fd);
      exit(0);
    }
  }

  /* accept socket connection (listen() must be called already) */

acceptagain:


  bzero((char *)&from, sizeof(from));
  len = sizeof (from);

  printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  printf("[%d] handle_link: Wait on 'accept(%d,0x%08x,%d)' ..\n",theLink->fd,theLink->sock,&from,len);
  printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

  {
    FILE *ff;
    char fname[80];
    sprintf(fname,"/home/clasrun/ccscans/good_waiting_%s",theLink->name);
    ff = fopen(fname,"w");
    if(ff > 0)
    {
      chmod(fname,777);
      fprintf(ff,"waiting on accept\n");
      fclose(ff);
    }
  }

  fflush(stdout);

  /* NOTE: the original socket theLink->sock remains open for accepting further connections */
  while((theLink->fd = accept(theLink->sock, (struct sockaddr *)&from, &len)) == -1)
  {
    printf("[%d] handle_link: accept: wait for connection\n",theLink->fd);
    usleep(USLEEP);
  }
  printf("[%d] handle_link: accept() returned (name >%s<, len=%d)\n",theLink->fd,inet_ntoa(from.sin_addr),len);fflush(stdout);



  /*
  accept() returns Ok, we expected it from >clondaq14<, received from>129.57.86.27<
IP addresses for clondaq14:
  129.57.167.162
ERRORRRRRRRRRRRRRRRRRRRRRR: UNAUTORIZED ACCESS FROM >129.57.86.27<, goto accept() again
ERRORRRRRRRRRRRRRRRRRRRRRR: UNAUTORIZED ACCESS FROM >129.57.86.27<, goto accept() again
ERRORRRRRRRRRRRRRRRRRRRRRR: UNAUTORIZED ACCESS FROM >129.57.86.27<, goto accept() again
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   */




  
  /****************************************************************************************/
  /* sergey: accept only from specified address (trying to block CC scans and other junk) */

  /* first, obtain actual host from process table, it can be different from name */
  dbsock = dbConnect(mysql_host, expid);
  if(dbsock==NULL)
  {
    printf("[%d] handle_link: Cannot connect to the database - exit\n",theLink->fd);fflush(stdout);
    pthread_exit(0);
  }

  /* 'theLink->name' may have extension starting from '-', like gem0vtp-s1, have to remove '-s1', because
     process table contains name gem0vtp, not gem0vtp-s1 */

  name_from = strdup(theLink->name);
  dash_pos = strchr(name_from, '-');
  /* Check if the character was found */
  if (dash_pos != NULL)
  {
    /* Replace the '-' with a null terminator */
    *dash_pos = '\0';
  }
  printf("theLink->name=%s -> name_from=%s\n",theLink->name,name_from);
    
  
  sprintf(tmpp,"SELECT host FROM process WHERE name='%s'",name_from);
  if(dbGetStr(dbsock, tmpp, host_from)==CODA_ERROR)
  {
    printf("Cannot get host_from from database for name >%s< - exit\n",name_from);fflush(stdout);
    printf("  theLink->host: %s\n",theLink->host);fflush(stdout);
    printf("  theLink->linkname: %s\n",theLink->linkname);fflush(stdout);
    printf("  query: %s\n",tmpp);fflush(stdout);
    pthread_exit(0);
  }
  printf("==> name_from is >%s<, it is running on host_from >%s<, theLink->name >%s<\n",name_from,host_from,theLink->name);fflush(stdout);
  dbDisconnect(dbsock);



  
  /* we checked that 'name_from' is in 'process' table; now check if we accepted link from it (this time using name with possible '-' extension) */
  address = inet_ntoa(from.sin_addr);

  
/* check if request came from 'host_from' or 'theLink->name' */

  printf("   accept() returns Ok, we expected it from host_from=>%s< or theLink->name=>%s<, received from >%s<\n",host_from,theLink->name,address);
  if(dacgethostbyname(host_from, ipaddress1) != 0)
  {
    printf("ERROR in dacgethostbyname(ipaddress1) !!!!!\n");
  }
  else if(dacgethostbyname(theLink->name, ipaddress2) != 0)
  {
    printf("ERROR in dacgethostbyname(ipaddress2) !!!!!\n");
  }
  else if( strcmp(address,ipaddress1) && strcmp(address,ipaddress2))
  {
    printf("ERRORRRRRRRRRRRRRRRRRRRRRR: UNAUTORIZED ACCESS FROM >%s<, goto accept() again\n",address);
    printf("ERRORRRRRRRRRRRRRRRRRRRRRR: UNAUTORIZED ACCESS FROM >%s<, goto accept() again\n",address);
    printf("ERRORRRRRRRRRRRRRRRRRRRRRR: UNAUTORIZED ACCESS FROM >%s<, goto accept() again\n",address);
    fflush(stdout);

    {
      FILE *ff;
      char fname[80];
      sprintf(fname,"/home/clasrun/ccscans/bad_%s",host_from);
      ff = fopen(fname,"w");
      if(ff > 0)
      {
        chmod(fname,777);
        fprintf(ff,"UNAUTORIZED ACCESS FROM >%s<\n",address);
        fclose(ff);
      }
    }
    
    close(theLink->fd);
    
    goto acceptagain;
  }

  
  {
    FILE *ff;
    char fname[80];
    sprintf(fname,"/home/clasrun/ccscans/good_%s",host_from);
    ff = fopen(fname,"w");
    if(ff > 0)
    {
      chmod(fname,0777);
      fprintf(ff,"accepted link from >%s<\n",address);
      fclose(ff);
    }
  }



  
  fd = theLink->fd;
  printf("[%d] connection accepted !!!\n",fd);fflush(stdout);






  

  /*************/
  /* main loop */
  /*************/

  while(theLink->exit==0)
  {
    /* reading data from roc */
    //printf("V1[%d]\n",theLink->roc_queue->roc);fflush(stdout);
    bufout = bb_write_current(&gbuftmp[theLink->roc_queue->roc]);
    //printf("V2[%d]\n",theLink->roc_queue->roc);fflush(stdout);
    if(bufout==NULL)
    {
      printf("ERROR: handle_link: bufout(1)=%p (theLink->roc_queue->roc=%d)\n",bufout,theLink->roc_queue->roc);fflush(stdout);
      exit(1);
    }
    /* reading data from roc */
    numRead = LINK_sized_read(fd, theLink->roc_queue->roc, (char **)&bufout);

    /* if 'LINK_sized_read' returned <=0, we exiting */
    if(numRead <= 0)
    {
      printf("[%2d] handle_link(): LINK_sized_read() returns %d\n",fd,numRead);fflush(stdout);
      bufout[0] = 2; /* let 'eb_proc_thread' know that it was no data from recv() */
      bufout[1] = 0xFFFFFFFF;
      printf("U3[%d]\n",theLink->roc_queue->roc);fflush(stdout);
      bb_write(&gbuftmp[theLink->roc_queue->roc]);
      printf("U4[%d]\n",theLink->roc_queue->roc);fflush(stdout);
 
      break; /* this is the first exit from while(1) loop; will call 'pthread_exit(0)' and return */
    }

    /* count total amount of data words */
    *dataSent += (numRead>>2);

    unsigned int *buf_long_p = (unsigned int *) bufout;
    if(buf_long_p[BBIWORDS] == BBHEAD)
    {
      printf("[%2d] handle_link(): WARNING got empty buffer from ROC !\n",fd);fflush(stdout);
      continue;
    }

    /* Check for test_link data */
    if(buf_long_p[BBIEVENTS] < 0)
    {
      printf("[%2d] WARNING - handle_link discarding buffer, count = %d.\n",fd,buf_long_p[BBIEVENTS]);fflush(stdout);
      continue;
    }
  
#ifndef DO_NOT_PUT
    //printf("[ix=%d] numRead=%d\n",theLink->roc_queue->roc,numRead);fflush(stdout);
    //printf("V3[%d]\n",theLink->roc_queue->roc);fflush(stdout);
#if 0
    TIMERL_START;
#endif
    bb_write(&gbuftmp[theLink->roc_queue->roc]);
#if 0
    TIMERL_STOP(100,0/*ix*/);
#endif
    //printf("V4[%d]\n",theLink->roc_queue->roc);fflush(stdout);
#endif /*#ifndef DO_NOT_PUT*/


  }
  
  /*************/
  /* main loop */
  /*************/


  
  /*printf("Checking for exit command theLink->exit=%d\n",theLink->exit);*/
  /* received command to exit */
  if(theLink->exit == 1)
  {
    printf("[%2d] handle_link(): got exit command inside reading loop !!!\n",fd); fflush(stdout);
  }
  else
  {
    while(theLink->exit != 1)
    {
      printf("[%2d][%s] CHECKING FOR theLink->exit TO BECOME 1, currently it is %d\n",fd,theLink->name,theLink->exit); fflush(stdout);
      /*usleep(USLEEP);*/
      sleep(1);
    }
  }
  
  printf("[%2d] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",fd); fflush(stdout);
  printf("[%2d] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",fd); fflush(stdout);
  printf("[%2d][%s] handle_link(): got exit command\n",fd,theLink->name); fflush(stdout);
  printf("[%2d] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",fd); fflush(stdout);
  printf("[%2d] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",fd); fflush(stdout);

  /*printf("handle_link(): thread exit for %9.9s\n",theLink->name); fflush(stdout);*/
  /*NOTE: segm fault printing 'theLink->name', probably pointer is not good any more ??? free'ed in debCloseLink ? */
  printf("[%2d] handle_link(): thread exiting\n",fd); fflush(stdout);


  printf("[%2d] 907\n",fd); fflush(stdout);
  theLink->exit = -1;
  pthread_exit(0);
}





  /*               croctest1    EB5        clon10-daq1
DATA_LINK           argv[1]   argv[2]       argv[3]
debOpenLink(char *fromname, char *toname, char *tohost)
  */

DATA_LINK
debOpenLink(char *fromname, char *toname, char *tohost,  MYSQL *dbsock)
{
  DATA_LINK theLink;
  int i, len, itmp, res, numRows;
  char host[100], hostmp[100], type[100], state[100], chport[100];
  char inhost[100];
  char name[100];
  char tmp[256], tmpp[256], *ch;
  int port = 0;

  MYSQL_RES *result;

  struct hostent *hp, *gethostbyname();
  struct sockaddr_in sin, from;
  int s, slen;


  /*******************************************************/
  /* allocate memory for structure 'theLink' and fill it */
  /* will be free'd in debCloseLink()                    */
  /*******************************************************/

  theLink = (DATA_LINK) calloc(sizeof(DATA_LINK_S),1);

  theLink->name = (char *) calloc(strlen(fromname)+1,1);
  strcpy(theLink->name, fromname); /* croctest1 etc */

  /* ROCs will use DB host name to send data to */
  /* 'inhost' will be set to 'links' table to let ROCs know where to send data */
  strncpy(inhost,tohost,99);
  printf("debOpenLink: set inhost to >%s<\n",inhost);
  strncpy(host,inhost,99);
  strncpy(theLink->host,host,99);


  /* construct database table row name */
  strncpy(name,fromname,98);
  len = strlen(name);
  strcpy((char *)&name[len],"->");
  strncpy((char *)&name[len+2],toname,(100-(len+2)));
  theLink->linkname = strdup(name); /* croctest1->EB5 etc */
  printf("debOpenLink: theLink->linkname is >%s<\n",theLink->linkname);

  /* set connection type to TCP */
  strcpy(type,"TCP");


  /* */
  bzero((char *)&sin, sizeof(sin));
  hp = gethostbyname(host);
  if(hp == 0 && (sin.sin_addr.s_addr = inet_addr(host)) == -1)
  {
    printf("debOpenLink: unkown host >%s<\n",host);
    return(NULL);
  }
  if(hp != 0) bcopy(hp->h_addr, &sin.sin_addr, hp->h_length);
  sin.sin_port = htons(port);
  sin.sin_family = AF_INET;

  /* create a socket */
  s = socket(AF_INET, SOCK_STREAM, 0); /* tcl: PF_INET !!?? */
  if(s < 0)
  {
    printf("debOpenLink: cannot open socket\n");
    return(NULL);
  }
  else
  {
    theLink->sock = s;
    printf("debOpenLink: listening socket # %d\n",theLink->sock);
  }

  /* if want, set socket options here, but better do not */
  /* SO_BUSY_POLL, SO_PREFER_BUSY_POLL, SO_BUSY_POLL_BUDGET, SO_REUSEPORT */
  
  {

    /* for the following to work, run as root:
          sysctl -w net.core.busy_read=50
          sysctl -w net.core.busy_poll=50
       or set it permanently by ...

     */
    
    int opt = 1;
    int microsec = 50;
    if(setsockopt(s, SOL_SOCKET, SO_BUSY_POLL, &microsec, sizeof(microsec)))
    {
      perror("setsockopt SO_BUSY_POLL failed");
      exit(EXIT_FAILURE);
    }
    else
    {
      printf("SO_BUSY_POLL is set to %d microseconds\n",microsec);
    }
    
    /*
    if(setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)))
    {
      perror("setsockopt SO_REUSEPORT failed");
      exit(EXIT_FAILURE);
    }
    */
  }
  
  
  /* bind and listen for server only (EB) */
  if(bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  {
    printf("debOpenLink: bind failed: host %s port %d\n",
      inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
    close(s);
	return(NULL);
  }

  if(listen(s, 5) < 0)
  {
    printf("debOpenLink: listen failed\n");
    close(s);
	return(NULL);
  }


  /* get the port number */
  len = sizeof(sin);
  if(getsockname (s, (struct sockaddr *) &sin, &len) < 0)
  {
    printf("debOpenLink: getsockname failed\n");
    close(s);
	return(NULL);
  }

  port = ntohs(sin.sin_port);
  printf("debOpenLink: socket is listening: host %s port %d\n",
      inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));


  /* create 'links' table if it does not exist (database must be opened in calling function) */
  sprintf(tmpp,"SELECT * FROM links");
  if(mysql_query(dbsock, tmpp) != 0)
  {
    /*need to check it !!!*/
    printf("No 'links' table -> we will create it (%s)\n",mysql_error(dbsock));
    sprintf(tmp,"create table links (name char(100) not null, type char(4) not null,host char(30),state char(10),port int)");
    if(mysql_query(dbsock, tmp) != 0)
    {
      printf("ERROR: cannot create table 'links' (%s)\n",mysql_error(dbsock));
      return(NULL);
    }
    else
    {
      printf("table 'links' created\n");
    }
  }
  else
  {
    MYSQL_RES *res;
    printf("Table 'links' exist\n");

    /*store and free results, otherwise mysql gives error on following mysql_query ..*/
    if(!(res = mysql_store_result (dbsock) ))
    {
      printf("ERROR in mysql_store_result (%s)\n",mysql_error(dbsock));
      return(NULL);
    }
    else
    {
      mysql_free_result(res);
    }
  }




  /*SERGEY: in 'links' table, remove all links wich names started from 'fromname' */
  sprintf(tmp,"SELECT name FROM links WHERE name LIKE '%s->%%' ",fromname);
  printf("query >%s<\n",tmp);
  if(mysql_query(dbsock, tmp) != 0)
  {
    printf("mysql error (%s)\n",mysql_error(dbsock));
    return(NULL);
  }
  if( !(result = mysql_store_result(dbsock)) )
  {
    printf("ERROR in mysql_store_result (%s)\n",mysql_error(dbsock));
    return(NULL);
  }
  else
  {
    numRows = mysql_num_rows(result);
    mysql_free_result(result);

    printf("nrow=%d\n",numRows);
    if(numRows>0)
    {
      sprintf(tmp,"DELETE FROM links WHERE name LIKE '%s->%%' ",fromname);
      printf("query >%s<\n",tmp);
      if(mysql_query(dbsock, tmp) != 0)
      {
        printf("mysql error (%s)\n",mysql_error(dbsock));
        return(NULL);
      }
      printf("Number of rows affected by DELETE: %d\n", mysql_affected_rows(dbsock) );
    }
  }
  /*SERGEY*/



  printf("DELETED !!!!!!!! %s-> !!!!!!!!!!\n",fromname);
  //sleep(3);



  /* trying to select our link from 'links' table */
  /* SERGEY: it should be nothing since we just deleted it !!! */
  sprintf(tmp,"SELECT name FROM links WHERE name='%s'",name);
  printf("query >%s<\n",tmp);
  if(mysql_query(dbsock, tmp) != 0)
  {
    printf("debOpenLink: mysql error (%s)\n",mysql_error(dbsock));
    return(NULL);
  }

  /* gets results from previous query */
  /* we assume that numRows=0 if our link does not exist,
     or numRows=1 if it does exist */
  if( !(result = mysql_store_result(dbsock)) )
  {
    printf("ERROR in mysql_store_result (%s)\n",mysql_error(dbsock));
    return(NULL);
  }
  else
  {
    numRows = mysql_num_rows(result);
    mysql_free_result(result);

    printf("nrow=%d\n",numRows);
    /* insert/update with state='down' */
    if(numRows == 0)
    {
      sprintf(tmp,"INSERT INTO links (name,type,host,port,state) VALUES ('%s','%s','%s',%d,'down')",name,type,host,port);
      printf("!!=1=> >%s<\n",tmp);
    }
    else if(numRows == 1) //should never be here if DELETE is performed above
    {
      sprintf(tmp,"UPDATE links SET host='%s',type='%s', port=%d, state='down' WHERE name='%s'",host,type,port,name);
      printf("!!=2=> >%s<\n",tmp);
    }
    else
    {
      printf("debOpenLink: ERROR: unknown nrow=%d",numRows);
      return(NULL);
    }

    if(mysql_query(dbsock, tmp) != 0)
    {
      printf("debOpenLink: ERROR 20-2\n");
      return(NULL);
    }
    else
    {
      printf("Query >%s< succeeded\n",tmp);
    }
  }




  /* set state 'waiting' (MAYBE THIS MUST BE LAST ACTION, AFTER SETTING port etc ???!!!) */
  sprintf(tmp,"UPDATE links SET state='waiting' WHERE name='%s'",name);
  if(mysql_query(dbsock, tmp) != 0)
  {
    printf("debOpenLink: ERROR 22-2\n");
    return(NULL);
  }

  /* database must be closed in calling function */

  /* ================ end of database update =================== */


/* cleanup .. */
ending_for_recv = 0;



  {
    /*Sergey: better be detached !!??*/
    pthread_attr_t detached_attr;

    pthread_attr_init(&detached_attr);
    pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED); /* default is PTHREAD_CREATE_JOINABLE, can use pthread_join() but
                                                                             it seems stuck if thread dies */
    pthread_attr_setscope(&detached_attr, PTHREAD_SCOPE_SYSTEM);


    /*******************************************************************/
    /* create 'handle_thread' which will handle input data from the roc */

    theLink->roc_queue = roc_queues[roc_queue_ix];
    /*theLink->roc_queue->parent = theLink->name;donotneedit???*/
    printf("theLink->name=%s\n",theLink->name);
    theLink->exit = 0;
    theLink->roc_queue->roc = roc_queue_ix;
    
    if(pthread_create( &theLink->link_thread, &detached_attr, (void *(*)(void *)) handle_link, (void *) theLink) != 0)
    {
      printf("LINK_thread_init(): ERROR in thread creating\n"); fflush(stdout);
      perror("pthread_create: ");
      return(NULL);
    }
    else
    {
      printf("LINK_thread_init(): thread is created\n"); fflush(stdout);
    }

    /*************************/
    /* create eb_proc_thread */
    
    theLink->fd = roc_queue_ix; //for printing in debug mode
    theLink->roc_queue = roc_queues[roc_queue_ix];
    theLink->gbufin = gbuftmp[roc_queue_ix];

    
    rolP3[roc_queue_ix] = (ROLPARAMS *) malloc(sizeof(ROLPARAMS));
    if(rolP3[roc_queue_ix]==NULL)
    {
      printf("ERROR: cannot allocate rolP3[roc_queue_ix] !!!\n");fflush(stdout);
      exit(1);
    }
    else
    {
      printf("INFO: allocated rolP3[%d] at %p\n",roc_queue_ix,rolP3[roc_queue_ix]);fflush(stdout);
    }
    theLink->rolP = rolP3[roc_queue_ix];
    theLink->rolP->rol_code = NULL;
    theLink->rolP->pid = roc_queues[roc_queue_ix]->rocid;
    printf("LLL: roc_queue_ix=%d, theLink->rolP=%p\n",roc_queue_ix,theLink->rolP);fflush(stdout);
    //procargs[roc_id].exit = 0;
	  
    if(pthread_create(&theLink->proc_thread, &detached_attr, (void *(*)(void *)) eb_proc_thread, (void *) theLink) != 0)
    {
      printf("Error: Failed to create eb_proc_thread\n");fflush(stdout);
      perror("pthread_create: ");
      return(NULL);
    }
    else
    {
      printf("INFO: eb_proc_thread created\n");fflush(stdout);
    }

  }

  roc_queue_ix ++; /*update global counter of the rocs in current daq configuration*/
  
  return(theLink);
}


int
debForceCloseLink(DATA_LINK theLink, MYSQL *dbsock)
{
  if(theLink == NULL)
  {
    printf("debForceCloseLink: theLink=NULL -> return\n");
    return(CODA_OK);
  }
  printf("debForceCloseLink: theLink=0x%08x -> closing\n",theLink);

  theLink->exit = 1; /* tells thread to exit */

  return(0);
}

int
debCloseLink(DATA_LINK theLink, MYSQL *dbsock)
{
  void *status;
  char tmp[1000];
  int exittimeout;

  /*
  if(theLink == NULL)
  {
    printf("debCloseLink: theLink=NULL -> return\n");
    return(CODA_OK);
  }
  printf("debCloseLink: theLink=0x%08x -> closing\n",theLink);

  theLink->exit = 1;
*/

  exittimeout = 3;
  printf("debCloseLink reached, fd=%d sock=%d exit=%d (exittimeout=%d)\n",
		   theLink->fd,theLink->sock,theLink->exit,exittimeout);
  fflush(stdout);

  /* give it a time to exit */
  while((theLink->exit!=-1) && (exittimeout>0))
  {
    printf("debCloseLink: waiting for link thread to exit, exittimeout=%d\n",exittimeout);
    sleep(1);
    /*usleep(USLEEP);*/
    exittimeout --;
  }

  /* shutdown socket connection */
  printf("11: preceeding with shutdown fd=%d\n",theLink->fd);fflush(stdout);
  if(shutdown(theLink->fd, SHUT_RDWR)==0) /*SHUT_RD,SHUT_WR,SHUT_RDWR*/
  {
    printf("12\n");fflush(stdout);
    printf("debCloseLink: socket fd=%d sock=%d connection closed\n",theLink->fd,theLink->sock);

    printf("903 %d\n",theLink->fd); fflush(stdout);
    close(theLink->fd);
    printf("904 %d\n",theLink->fd); fflush(stdout);
    close(theLink->sock);
    printf("905 %d\n",theLink->fd); fflush(stdout);
  }
  else
  {
    printf("13\n");fflush(stdout);
    printf("debCloseLink: ERROR in socket fd=%d sock=%d connection closing - exiting !!!\n",
          theLink->fd,theLink->sock);
    exit(0);
  }
  printf("906\n"); fflush(stdout);

  /* shut down a connection by telling the other end to shutdown (database must be opened in calling function) */
  /* set state 'down' */
  sprintf(tmp,"UPDATE links SET state='down' WHERE name='%s'",theLink->linkname);
  if(mysql_query(dbsock, tmp) != 0)
  {
    printf("debCloseLink: ERROR in database query {UPDATE ..}\n");
    return(CODA_ERROR);
  }
  else
  {
    printf("debCloseLink: link is down\n");
  }

  /* database must be closed in calling function */

  /* ================ end of database update =================== */



  /* cancel thread if still exists */
  if(theLink->exit!=-1)
  {
    pthread_t link_thread = theLink->link_thread;
    printf("debCloseLink: canceling link_thread .\n");
    pthread_cancel(link_thread);
    printf("debCloseLink: canceling link_thread ..\n");
    /*pthread_join(link_thread,&status); stuck here if one of the ROCs crashed */
    printf("debCloseLink: canceling link_thread !\n");
  }

  theLink->link_thread = 0;
  theLink->exit = 0;

  /* SHOULD DO FOLLOWING ONLY IF handle_thread() IS DONE !!! (AND PREVIOUS AS WELL ??) */

  /* release memory */
  printf("debCloseLink: free memory\n");
  free((char *) theLink->name); //sergey: was 'cfree'
  /*cfree((char *) theLink->parent);donotneedit???*/


  /* sergey: probably error, after following call 'put_cb_data(fd, &theLink->roc_queue, (void *) -1)'
     from 'handle_link()' will fail since  'theLink' does not exist any more; this is probably why there
     is check 'if(cbp <(CIRCBUF *)100000)' inside put_cb_data ... */
  /* probably 'handle_link()' must set some flag when done, and we'll wait for that flag here ... */
  free((char *) theLink); //sergey: was 'cfree'
  printf("debCloseLink: done.\n");
  
  return(CODA_OK);
}
