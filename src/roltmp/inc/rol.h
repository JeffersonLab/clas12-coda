
/* rol.h */

#ifndef _ROLDEFINED
#define _ROLDEFINED

#include <stdint.h>

#define EIEIO    __asm__ volatile ("eieio")
#define SYNC     __asm__ volatile ("sync")

#define logMsg printf
typedef struct semaphore *SEM_ID;

static void __download ();
static void __prestart ();
static void __end ();
static void __pause ();
static void __go ();
static void __done ();
static void __status ();

static int32_t theIntHandler ();


#ifndef INT16
#define INT16  short
#endif

#ifndef UINT16
#define UINT16 unsigned short
#endif

#ifndef INT32
#define INT32  int
#endif

#ifndef UINT32
#define UINT32 unsigned int
#endif

#ifndef STATUS
#define STATUS int
#endif


/*************************************************************************************/
/* Include the memory partition structures and routines (former libpart.h/mempart.h) */
#ifndef NULL
#define NULL ((void *) 0)
#endif

#define maximum(a,b) (a<b ? b : a)

typedef void (*VOIDFUNCPTR) ();
typedef int32_t (*FUNCPTR) ();
typedef void (*VOIDFUNCPTR1) (uint32_t);
typedef int32_t (*FUNCPTR1) (uint32_t, uint32_t);
typedef int32_t (*FUNCPTR2) (uint32_t);
typedef int32_t (*FUNCPTR3) ();
typedef int32_t (*FUNCPTR4) (uint32_t);

#include <unistd.h>
#include <pthread.h>
#include <time.h>



/******************************************************************************/
/* The following structure is used per readout list to store it's global data */
#include "rolInt.h"     


#include "rc.h" /* for da.h */
#include "da.h" /* for gethrtime() */

/* Define external variables and internal rol structures */
extern ROLPARAMS rolP;
static rolParam rol;

static int32_t syncFlag;
static int32_t lateFail;

/* Event type source */
static int32_t EVENT_type;
static int32_t *StartOfEvent;
static int32_t *StartOfBank;


/* Macros */

#define BT_UI4_ty  0x01
#define BT_BANKS_ty  0x0e

#define RUN_NUMBER rol->runNumber
#define RUN_TYPE   rol->runType
#define EVENT_NUMBER *(rol->nevents)

static int event_number;


#include "../../dac/dac.s/bigbuf.h" /*for prototypes, SEND_BUF_MARGIN, etc */


#ifdef EVENT_MODE

/* ROL2 macros: call CPINIT first, then CPOPEN/CPCLOSE for every bank, and CPEXIT in the end */

#define MAXBANKS 10

#define CPINIT \
  uint32_t *dataout_save1; \
  uint32_t *dataout_save2; \
  unsigned char *b08; \
  unsigned short *b16; \
  uint32_t *b32; \
  unsigned long long *b64; \
  int32_t lenin = *(rol->dabufpi) - 1;	\
  int32_t lenout = 2;  /* start from 3rd word, leaving first two for bank-of-banks header */ \
  int32_t lenev; \
  uint32_t *datain = (uint32_t *)((rol->dabufpi)+2); \
  uint32_t *dataout = (uint32_t *)((rol->dabufp)+2); \
  uint32_t *header = (uint32_t *)(rol->dabufp); \

#define CPOPEN(btag,btyp,bnum) \
{ \
  dataout_save1 = dataout ++; /*remember beginning of the bank address, exclusive bank length will be here*/ \
  *dataout++ = (btag<<16) + (btyp<<8) + bnum; /*bank header*/ \
  b08 = (unsigned char *)dataout; \
}

#define CPCLOSE \
{ \
  uint32_t padding; \
  /*printf("CPCLOSE: dataout before = 0x%016x, b08=0x%016x (0x%016x)\n",dataout,b08,(uint64_t)b08);*/ \
  dataout = (uint32_t *) ( ( ((unsigned long int)b08+3)/4 ) * 4); \
  padding = (unsigned long int)dataout - (unsigned long int)b08; \
  dataout_save1[1] |= (padding&0x3)<<14; /*update bank header (2nd word) with padding info*/ \
  /*printf("CPCLOSE: 0x%016x(%d) --- 0x%016x(%d) --> padding %d\n",dataout,dataout,b08,b08,((dataout_save1[1])>>14)&0x3);*/ \
  *dataout_save1 = ((unsigned long int)dataout-(unsigned long int)dataout_save1)/4 - 1; /*write bank length in 32bit words*/ \
  /*printf("CPCLOSE: *dataout_save1 = 0x%016x (0x%016x 0x%016x)\n",*dataout_save1,dataout,dataout_save1);*/ \
  lenout += (*dataout_save1+1); \
  lenev += (*dataout_save1+1); \
  b08 = NULL; \
}

#define CPEXIT \
  /*printf("CPEXIT lenout=%d\n",lenout);fflush(stdout);*/	\
  header[0] = lenout - 1; \
  /*printf("CPEXIT 1\n");fflush(stdout);*/	\
  header[1] = rol->dabufpi[1]; \
  /*printf("CPEXIT 2\n");fflush(stdout);*/	\
  rol->dabufp[lenout] = 1; /*it is here just for compatibility with new ROC2; will be redefined to the actual number of events in block*/ \
  /*printf("CPEXIT 3\n");fflush(stdout);*/




/* following macros loops over input bank of banks, set pointers to internal banks data areas,
and extract info about banks found; to be used in ROL2 */ \

#define BANKINIT \
  int nbanks; \
  unsigned int banktag[MAXBANKS]; \
  unsigned int banknr[MAXBANKS]; \
  unsigned int banknw[MAXBANKS]; \
  unsigned int banktyp[MAXBANKS]; \
  unsigned int bankpad[MAXBANKS]; \
  uint32_t *bankdata[MAXBANKS]

#define BANKSCAN \
  { \
    int nw, jj;	\
    uint32_t *ptr, *ptrend; \
    nbanks = 0; \
    ptr = (uint32_t *)((rol->dabufpi));	\
	ptrend = ptr + *(rol->dabufpi); \
    ptr +=2; /*skip bank-of-banks header*/ \
	/*printf("BANKSCAN: tag=%d typ=%d num=%d\n",((*(ptr-1))>>16)&0xFF,((*(ptr-1))>>8)&0xFF,(*(ptr-1))&0xFF);*/ \
	while(ptr < ptrend) \
    { \
      /*printf("BANKSCAN[%d]: while begin: ptr=0x%08x ptrenv=0x%08x (%d)\n",nbanks,ptr,ptrend,(ptrend-ptr));fflush(stdout);*/ \
      banknw[nbanks] = (*ptr) - 1; /*data length only*/ \
      /*printf("nw=%d\n",banknw[nbanks]);fflush(stdout);*/ \
      ptr ++; \
      banktag[nbanks] = ((*ptr)>>16)&0xffff; \
      /*printf("tag=0x%08x\n",banktag[nbanks]);fflush(stdout);*/ \
      bankpad[nbanks] = ((*ptr)>>14)&0x3; \
      /*printf("pad=%d\n",bankpad[nbanks]);fflush(stdout);*/ \
      banktyp[nbanks] = ((*ptr)>>8)&0x3f; \
      /*printf("typ=0x%08x\n",banktyp[nbanks]);fflush(stdout);*/ \
      banknr[nbanks] = (*ptr)&0xff; \
      /*printf("nr=%d\n",banknr[nbanks]);fflush(stdout);*/ \
      ptr ++; \
      bankdata[nbanks] = ptr; \
      /*printf("data(ptr)=0x%08x\n",bankdata[nbanks]);fflush(stdout);*/ \
      ptr += banknw[nbanks]; \
      nbanks ++; \
      if(nbanks >= MAXBANKS) {printf("rol.h ERROR: nbanks=%d - exit\n");fflush(stdout);break;}; \
      /*printf("BANKSCAN[%d]: while end: ptr=0x%08x ptrenv=0x%08x (%d)\n",nbanks,ptr,ptrend,(ptrend-ptr));fflush(stdout);*/ \
    } \
    /*for(jj=0; jj<nbanks; jj++) printf("bankscan[%d]: tag 0x%08x typ=%d nr=%d nw=%d dataptr=0x%08x\n", \
      jj,banktag[jj],banktyp[jj],banknr[jj],banknw[jj],bankdata[jj]);*/ \
  }

#else



/********************************************************/
/* sergey: new bank handling: using big buffer directly */
extern BIGBUF *gbigDMA;

extern int object_nlongs;
extern int clear_to_send;
extern int last_event_check;
extern uint32_t *dabufp;
extern unsigned int dataInBuf;
extern unsigned int g_max_ev_in_buf;
extern unsigned int g_events_in_buffer;
extern int tsendBufSize;
extern int rocp_primefd;
extern int rocp_recNb;
extern int this_roc_id;
extern int token_interval;
static/*extern*/ /*time_t*/hrtime_t timeout; /* from coda_roc.c */

extern pthread_mutex_t sendbuffer_lock;
#define SENDBUFFER_LOCK pthread_mutex_lock(&sendbuffer_lock)
#define SENDBUFFER_UNLOCK pthread_mutex_unlock(&sendbuffer_lock)

/* - cbopen ---------------------------------------------------------- 
   crl	: open bank <bank_name> of <bank_type>
   - <bank_name> : converted in bnum through Bank_Dic_ID 
   - <bank_type> : UI2 , UI4, UB1 
   
   example	: open bank 0x1234 of UI2 
   
   call	: cbopen (int bnum, int btype); 
   
   Function: 
   open a CODA bank with CODA Bank Header Format 
   leaves (rol->dabufp) pointing to ready next address 
   keep pointer to length of bank in GblTopBp for length adjustment 
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* get current pointer in the big buffer and fill CODA header info */ 
/* called in rol1: CEOPEN(EVTYPE, BT_BANKS)*/
/* NOTE: in blocking mode *(rol->nevents) count blocks, not events; rol2 will put event number in during disentangling */
#define CEOPEN(bnum, btype) \
{ \
dabufp[0] = 0; /*cleanup first word (will be CODA fragment length), otherwise following loop will think there is an event */ \
  StartOfEvent = rol->dabufp = (int32_t *)dabufp; \
  *(++(rol->dabufp)) = (syncFlag<<24) | ((bnum) << 16) | ((btype##_ty) << 8) | ( (*(rol->nevents)) & 0xff); \
  /*printf("CEOPEN: tag=%d typ=%d num=%d\n",((*(rol->dabufp))>>16)&0xFF,((*(rol->dabufp))>>8)&0xFF,(*(rol->dabufp))&0xFF);*/ \
  ((rol->dabufp))++; \
}

/*for rol1.c, rol1_vme.c etc - in 'parallel' readout mode coda header comes from circular buffer, so do not bump rol->dabufp*/
#define CEOPEN1 \
{ \
dabufp[0] = 0; /*cleanup first word (will be CODA fragment length), otherwise following loop will think there is an event */ \
  StartOfEvent = rol->dabufp = dabufp; \
}

/* - cbclose --------------------------------------------------------- 
 crl	: close bank <buff_len> of <bank_type> 
  - <bank_type> : UI2 , UI4, UB1 
  - <buff_len>  : number of "bank_type" word written 

 example	: close bank

  Call	: cbclose (btype, &buflen); 

  Function: 
    Close a CODA bank created by "cbopen" 
    leaves the (rol->dabufp) pointing to next empty int after bank 
    returns the actual length of that bank 
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#define CECLOSE \
{ \
  int syncsync = 0; \
/*printf("11: %d 0x%08x\n",StartOfEvent[0],StartOfEvent[1]);*/\
  /* writes bank length (in words) into the first word of the event (CODA header [0]) */ \
  *StartOfEvent = (int) (((char *) (rol->dabufp)) - ((char *) StartOfEvent)); \
  /*if(this_roc_id==39&&dabufp[0]!=8536) printf("CECLOSE: 1: dabufp[0]=%d\n",dabufp[0]);*/					\
  /* align pointer to the 64/128 bits - do we need it now ??? */ \
  if((*StartOfEvent & 1) != 0) \
  { \
    (rol->dabufp) = ((int32_t *)((char *) (rol->dabufp))+1); \
    *StartOfEvent += 1; \
  /*printf("CECLOSE: 2: dabufp[0]=%d\n",dabufp[0]);*/					\
  } \
  if((*StartOfEvent & 2) !=0) \
  { \
    *StartOfEvent = *StartOfEvent + 2; \
  /*printf("CECLOSE: 3: dabufp[0]=%d\n",dabufp[0]);*/					\
    (rol->dabufp) = ((int32_t *)((short *) (rol->dabufp))+1); \
  } \
  /*if(this_roc_id==39&&dabufp[0]!=8536) printf("CECLOSE: 4: dabufp[0]=%d\n",dabufp[0]);*/					\
  *StartOfEvent = ( (*StartOfEvent) >> 2) - 1; \
  /*if(this_roc_id==39&&dabufp[0]!=2133) printf("CECLOSE: 5: dabufp[0]=%d\n",dabufp[0]);*/					\
  /*****fromcoda_roc****/ \
  /*********************/ \
  /* let see now what we've got from ROL1 */ \
  len = dabufp[0]; \
  if(len > 0) \
  { \
    static int oldevnb = 0; \
    int newevnb; \
    int ev_type; \
    if(len > NWBOS) logMsg("===> ERROR: len = %d (NWBOS=%d)\n",len,NWBOS,3,4,5,6);	\
    /* check some stuff and increment some counters */ \
    newevnb = dabufp[1] & 0xff; \
    /*printf("rol1: newevnb=%d\n",newevnb);*/ \
    if(newevnb < oldevnb) \
    { \
      newevnb += 256; \
      /*printf("rol1: +256\n");*/				\
	} \
    if(newevnb - oldevnb != 1) \
    { \
      /*printf("nevents %d newevnb %d old %d\n",object->nevents, newevnb, oldevnb);*/ \
      logMsg("dabufp[0] = 0x%08x (%d)\n",dabufp[0],dabufp[0],3,4,5,6);	\
      logMsg("dabufp[1] = 0x%08x (%d)\n",dabufp[1],dabufp[1],3,4,5,6);	\
    } \
    oldevnb = dabufp[1] & 0xff; \
    ev_type = (dabufp[1] >> 16) & 0xff; \
    syncsync = (dabufp[1] >> 24); \
    /*if(syncsync) printf("CECLOSE: sync=0x%02x (ev# %d)\n",syncsync,oldevnb);*/ \
    /*if((ev_type >= 0) && (ev_type < 16))*/ \
    if( (ev_type < EV_SYNC) || (ev_type >= (EV_SYNC+16)) )	\
    { \
	  /*rocp->last_event++;*/ /* Increment Physics Events processed */ \
      last_event_check ++; \
    } \
    else \
    { \
      /* printf("NON Physics Event type=%d\n",ev_type); */ \
    } \
 /* time profiling event counter \
#ifndef Darwin \
    nevent ++; \
#endif*/ \
    /* really important stuff */ \
    dabufp += (len+1);       /* set pointer .. to the next event */ \
    dataInBuf += (len+1)<<2; /* set pointer .. to the next event */ \
    g_events_in_buffer++;    /* increment event counter in buffer */ \
	/* check if big buffer is full */ \
    if( dataInBuf > (tsendBufSize - SEND_BUF_MARGIN) || (g_events_in_buffer>=g_max_ev_in_buf))	\
    { \
	  /*printf("clear_to_send 1: dataInBuf=%d tsendBufSize=%d SEND_BUF_MARGIN=%d\n",dataInBuf,tsendBufSize,SEND_BUF_MARGIN);*/ \
      clear_to_send = 1; \
    } \
	/* \
#ifdef VXWORKS \
    end = sysTimeBaseLGet(); \
    time2 += (end-start)/MYCLOCK; \
#else \
    end = gethrtime(); \
    time2 += (end-start)/MYCLOCK; \
#endif \
	*/ \
  } \
  else if(len < 0) \
  { \
    logMsg("ERROR: len = %d\n",len,2,3,4,5,6);	\
  } \
  else /* len == 0 - it was no event in this pooling iteration */ \
  { \
    logMsg("SHOULD NEVER BE HERE: len = %d\n",len,2,3,4,5,6);	\
	/* \
#ifdef VXWORKS \
    end = sysTimeBaseLGet(); \
    time1 += (end-start)/MYCLOCK; \
#else \
    end = gethrtime(); \
    time1 += (end-start)/MYCLOCK; \
#endif \
	*/ \
	; \
  } \
  /***************/ \
  /***************/ \
}


#define CECLOSE1 \
{ \
  int nw = 0; \
  int syncsync = 0; \
  while(nw < nwords) \
  { \
	/*printf("CECLOSE1: fragment header: %d 0x%08x\n",dabufp[0],dabufp[1]);*/ \
    len = dabufp[0]; \
    if(len > 0)	   \
    { \
      static int oldevnb = 0; \
      int newevnb; \
      int ev_type; \
      if(len > NWBOS) logMsg("CECLOSE1: ===> ERROR: len = %d (NWBOS=%d)\n",len,NWBOS,3,4,5,6); \
      newevnb = dabufp[1] & 0xff; \
      if(newevnb < oldevnb) \
      { \
        newevnb += 256; \
	  } \
      if(newevnb - oldevnb != 1) \
      { \
        logMsg("CECLOSE1: dabufp[0] = 0x%08x (%d)\n",dabufp[0],dabufp[0],3,4,5,6);	\
        logMsg("CECLOSE1: dabufp[1] = 0x%08x (%d)\n",dabufp[1],dabufp[1],3,4,5,6);	\
      } \
      oldevnb = dabufp[1] & 0xff; \
      ev_type = (dabufp[1] >> 16) & 0xff; \
      syncsync = (dabufp[1] >> 24); \
      /*if(syncsync) printf("CECLOSE1: sync=0x%02x (ev# %d)\n",syncsync,oldevnb);*/ \
      if( (ev_type < EV_SYNC) || (ev_type >= (EV_SYNC+16)) )	\
      { \
        last_event_check ++; \
      } \
      dabufp += (len+1);       /* set pointer .. to the next event */ \
      dataInBuf += (len+1)<<2; /* set pointer .. to the next event */ \
      g_events_in_buffer++;    /* increment event counter in buffer */ \
      if(dataInBuf > (tsendBufSize - SEND_BUF_MARGIN)) \
      { \
	    /*printf("clear_to_send 2\n");*/		\
        clear_to_send = 1; \
      } \
    } \
    else if(len < 0) \
    { \
      logMsg("CECLOSE1: ERROR: len = %d\n",len,2,3,4,5,6);	\
      sleep(1); \
    } \
    else /* len == 0 - it was no event in this pooling iteration */ \
    { \
      logMsg("CECLOSE1: SHOULD NEVER BE HERE: len = %d\n",len,2,3,4,5,6);	\
      sleep(1); \
    } \
    nw += (len+1); \
  } \
}






/* rol1: following 2 macros called from rol1 to create banks inside bank-of-banks created by CEOPEN */

#define BANKOPEN(btag,btyp,bnum) \
{ \
  /*printf("BANKOPEN: StartOfBank=0x%08x\n",StartOfBank);fflush(stdout);*/ \
  /*printf("BANKOPEN: rol->dabufp=0x%08x\n",rol->dabufp);fflush(stdout);*/ \
  StartOfBank = rol->dabufp; \
  *(++(rol->dabufp)) = ((btag)<<16) + ((btyp)<<8) + (bnum);	\
  ((rol->dabufp))++; \
  /*printf("BANKOPEN: rol->dabufp=0x%08x\n",rol->dabufp);fflush(stdout);*/ \
}

#define BANKCLOSE \
{ \
  /* writes bank length (in words) into the first word of the bank */ \
  *StartOfBank = rol->dabufp - StartOfBank - 1; \
  /*printf("BANKCLOSE: StartOfBank=0x%08x (0x%08x %d)\n",StartOfBank,*StartOfBank,*StartOfBank);fflush(stdout);*/ \
  /*printf("BANKCLOSE: rol->dabufp=0x%08x (0x%08x %d)\n",rol->dabufp,*rol->dabufp,*rol->dabufp);fflush(stdout);*/ \
  /*printf("BANKCLOSE: header: 0x%08x (%d), 0x%08x\n",*StartOfBank,*StartOfBank,*(StartOfBank+1));fflush(stdout);*/ \
}

#define CPOPEN(btag,btyp,bnum) \
{ \
  dataout_save1 = dataout ++; /*remember beginning of the bank address, exclusive bank length will be here*/ \
  *dataout++ = (btag<<16) + (btyp<<8) + bnum; /*bank header*/ \
  b08 = (unsigned char *)dataout; \
}

#define CPCLOSE \
{ \
  unsigned int padding; \
  /*printf("CPCLOSE: dataout before = 0x%x\n",dataout);*/		 \
  dataout = (uint32_t *) ( ( ((unsigned int)b08+3)/4 ) * 4); \
  padding = (unsigned int)dataout - (unsigned int)b08; \
  dataout_save1[1] |= (padding&0x3)<<14; /*update bank header (2nd word) with padding info*/ \
  /*printf("CPCLOSE: 0x%x %d --- 0x%x %d --> padding %d\n",dataout,dataout,b08,b08,((dataout_save1[1])>>14)&0x3); */ \
  *dataout_save1 = (dataout-dataout_save1-1); /*write bank length*/ \
  /*printf("CPCLOSE: *dataout_save1 = %d\n",*dataout_save1);*/		\
  lenout += (*dataout_save1+1); \
  b08 = NULL; \
}






#define SET_TIMEOUT \
    if(*(rol->nevents) < 10)						\
    { \
      timeout = gethrtime() + 1*100000;	\
	  /*printf("SET_TIMEOUT 1: timeout=%d\n",timeout);*/	\
    } \
    else \
    { \
      timeout = gethrtime() + (token_interval/60)*200000;	\
	  /*printf("SET_TIMEOUT 2: timeout=%d\n",timeout);*/	\
    }

/*
#define SET_TIMEOUT \
    if(*(rol->nevents) < 10)						\
    { \
      timeout = time(0) + 1;	\
    } \
    else \
    { \
      timeout = time(0) + token_interval/60;	\
    }
*/


#define SEND_BUFFER_ \
  /*SENDING big buffer*/ \
  tdisp18++; \
  SENDBUFFER_LOCK; \
  tdisp19++; \
  if((clear_to_send==1) && (rocp_primefd>=0))	\
  { \
	/*printf("ROL1: clear_to_send = %d === \n",clear_to_send);*/ \
    /*logMsg("SEND_BUFFER_ROL1 (0x%08x)\n",dabufp);*/	\
    /*setHeartBeat(HB_ROL,14,5);*/ \
    /* send current output buffer */ \
    /* set now 'dabufp' to the beginning of */ \
    /* 'big' buffer just to fill a header   */ \
  tdisp20++; \
    dabufp = bb_write_current(&gbigDMA); \
  tdisp21++; \
    /*setHeartBeat(HB_ROL,15,5);*/ \
    if(dabufp == NULL) \
    { \
      logMsg("ERROR in bb_write: FAILED1\n",1,2,3,4,5,6);	\
      /*setHeartBeat(HB_ROL,0,-1);*/ \
      SENDBUFFER_UNLOCK; \
      return; \
    } \
    /* fill 'big' buffer header */ \
    dabufp[BBIWORDS] = dataInBuf>>2;        /* buffer length in words */ \
    dabufp[BBIBUFNUM] = ++ rocp_recNb;      /* buffer number */ \
    dabufp[BBIROCID]  = this_roc_id/*object->codaid*/;     /* ROC id */ \
    dabufp[BBIEVENTS] = g_events_in_buffer; /* the number of events */ \
	if(dabufp[BBIEVENTS]<=0) {printf("ERROR(rol.h): nev=%d\n",dabufp[BBIEVENTS]);exit(0);} \
    dabufp[BBIFD]     = rocp_primefd; \
    dabufp[BBIEND]    = 0; \
	/*bb_check(dabufp);*/												\
    /*printf("==== ROL: %d %d %d %d 0x%08x\n",dabufp[0],dabufp[1],dabufp[2],dabufp[3],dabufp[4]);*/ \
	/* main send */ \
    if(dabufp[BBIWORDS] > BBHEAD) /*check if buffer is not empty ?*/ \
    { \
      /*trying to get next buffer; if not available - do nothing, rols_loop will take care*/ \
	  /*printf("rol.h: bb_write_nodelay 1\n");*/ \
  tdisp22++; \
  dabufp = bb_write_nodelay(&gbigDMA);		\
  tdisp23++; \
      if(dabufp == NULL) \
      { \
        rocp_recNb --; /* decrement it back */ \
/*logMsg("INFO from bb_write: NO BUFS1\n",1,2,3,4,5,6);*/	\
        /*setHeartBeat(HB_ROL,0,-1);*/ \
      } \
      else \
	  { \
        /* cleanup 'event counter in buffer' */ \
        g_events_in_buffer = 0; \
        /* determine total # of bytes to be sent */    \
        object_nlongs += dataInBuf >> 2; \
        /* pickup next buffer */ \
        dabufp += BBHEAD; \
        /* reserve space for header */ \
        dataInBuf = BBHEAD_BYTES; \
        clear_to_send = 0; \
        SET_TIMEOUT; \
	  } \
    } \
    else \
    { \
      logMsg("attempt to send short buffer failed1 !!!\n",1,2,3,4,5,6);	\
    } \
  } \
  tdisp24++; \
  SENDBUFFER_UNLOCK



#define WRITE_EVENT_ \
  rol->dabufp = NULL; \
  /* call done routine */ \
  tdisp16++; \
  (*doneRtns)(); \
  tdisp17++; \
  /* if buffer if full, trying to send it and get new one; \
	in case of problem - do NOT call "__done()", leave it for rols_loop */ \
  SEND_BUFFER_; \
  if(dabufp == NULL) /* remember that we have event(s) to process ??? cdopolldispatch() will check it .. */ \
  { \
    poolEmpty = 1; /* must be cleared in __done() */ 							\
    rol->doDone = 1; \
  } \
  else /*enable interrupts*/ \
  { \
	/*printf("%s calls __done()\n",__FUNCTION__);*/	\
    __done(); \
    rol->doDone = 0; \
  }

/********************************************************/
/********************************************************/

#endif /*EVENT_MODE*/





		     



static unsigned int tdisp1=0;
static unsigned int tdisp2=0;
static unsigned int tdisp3=0;
static unsigned int tdisp4=0;
static unsigned int tdisp5=0;
static unsigned int tdisp6=0;
static unsigned int tdisp7=0;
static unsigned int tdisp8=0;
static unsigned int tdisp9=0;
static unsigned int tdisp10=0;
static unsigned int tdisp11=0;
static unsigned int tdisp12=0;
static unsigned int tdisp13=0;
static unsigned int tdisp14=0;
static unsigned int tdisp15=0;
static unsigned int tdisp16=0;
static unsigned int tdisp17=0;
static unsigned int tdisp18=0;
static unsigned int tdisp19=0;
static unsigned int tdisp20=0;
static unsigned int tdisp21=0;
static unsigned int tdisp22=0;
static unsigned int tdisp23=0;
static unsigned int tdisp24=0;




/* private global */

static unsigned char dispatch_busy; 
static int poolEmpty;


static FUNCPTR1 trigRtns;
static FUNCPTR2 syncTRtns;
static FUNCPTR3 doneRtns;
static FUNCPTR4 ttypeRtns;


static unsigned int Tcode;

/* 
   Clear some global variables etc for a clean start.
   Should be done at Prestart.
*/

#define CTRIGINIT \
{ \
    dispatch_busy = 0; \
    syncTRtns = NULL; \
    ttypeRtns = NULL; \
    Tcode = 0; \
    poolEmpty = 0; \
}



#define CTRIGRSA(source,code,handler,done) \
{ \
  trigRtns  = (FUNCPTR1) (handler); \
  doneRtns  = (FUNCPTR3) (done); \
  Tcode     = (code); \
  ttypeRtns = (FUNCPTR4) source##_TTYPE; \
  source##_ASYNC(code); \
  logMsg("CTRIGRSA: set handler and done, code=%d\n",code,2,3,4,5,6);	\
  logMsg("CTRIGRSA: 0x%08x 0x%08x 0x%08x 0x%08x\n", \
		(unsigned long int)doneRtns,(unsigned long int)trigRtns,Tcode,(unsigned long int)ttypeRtns,5,6);	   \
}

#define CTRIGRSS(source,code,handler,done) \
{ \
  trigRtns  = (FUNCPTR1) (handler) ; \
  doneRtns  = (FUNCPTR3) (done) ; \
  syncTRtns = (FUNCPTR2) source##_TEST ; \
  Tcode     = (code) ; \
  ttypeRtns = (FUNCPTR4) source##_TTYPE ; \
  source##_SYNC(code); \
  logMsg("CTRIGRSS_##source: set handler and done, code=%d\n",code,2,3,4,5,6); \
  logMsg("CTRIGRSS_##source: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n", \
		(unsigned long int)doneRtns,(unsigned long int)trigRtns,(unsigned long int)syncTRtns,Tcode,(unsigned long int)ttypeRtns,6);			   \
}



#define LOCKINTS
#define UNLOCKINTS

#define CDOPOLL {cdopolldispatch();}

#define CDOINIT(source,code) source##_INIT(code)

#define CDOENABLE(source,code,val) source##_ENA(code,val)
#define CDODISABLE(source,code,val) source##_DIS(code,val)
#define CDOACK(source,code,val) source##_ACK(code,val)
#define CDOSTART(source,val) source##_START(val)
#define CDOSTOP(source,val) source##_STOP(val)



#ifndef EVENT_MODE

/* Actually dispatch the triggers to the correct routines */

static void
cdodispatch(unsigned int theType)
{
  dispatch_busy = 1;

  (*(rol->nevents))++;

  /*printf("cdodispatch: increment 'nevents to %d\n",*(rol->nevents));*/

  /*UNLOCKINTS;?????*/
  tdisp13++;
  (*trigRtns)(theType, Tcode);
  tdisp14++;
  /*LOCKINTS;?????*/

  WRITE_EVENT_;

  tdisp15++;

  dispatch_busy = 0;
}



/* Generic interrupt handler.
This routine looks to see if the dispatcher is already running.
If dispatcher is busy the trigger is queued, this code is protected
by INTLOCK so we know the flag dispatch_busy can't change behind our backs */

static int
theIntHandler(int theSource)
{
  unsigned int theType;

  if(theSource == 0) return(0);

  LOCKINTS;

  theType = (*ttypeRtns)(Tcode); 
  if(!dispatch_busy) cdodispatch(theType);

  UNLOCKINTS;
}

#endif /*EVENT_MODE*/



/* polling handler: if running in polling mode, called instead of interrupt
   handler */

void
tdispprint()
{
  printf("tdisp = %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
		 tdisp1,tdisp2,tdisp3,tdisp4,tdisp5,tdisp6,tdisp7,tdisp8,tdisp9,tdisp10,tdisp11,tdisp12,tdisp13,tdisp14,tdisp15,
         tdisp16,tdisp17,tdisp18,tdisp19,tdisp20,tdisp21,tdisp22,tdisp23,tdisp24);
}

static int
cdopolldispatch()
{
  unsigned int theType;
  int stat = 0;

#ifdef EVENT_MODE

  (*trigRtns)(0,0);

#else

  tdisp1++;
  if(!poolEmpty)
  {
    tdisp2++;
    if(syncTRtns) /*for VME_source.h it is vmettest*/
    {
      tdisp3++;
      stat=(*syncTRtns)(Tcode);
      tdisp4++;
      if(stat) /*mostly 0; if 1 - call trigger routine*/
      {
        tdisp5++;
        LOCKINTS;
        tdisp6++;
        if(!dispatch_busy)
        {
          tdisp7++;
          theType = (*ttypeRtns)(Tcode);  
          tdisp8++;
          cdodispatch(theType);
          tdisp9++;
        }
        tdisp10++;
        UNLOCKINTS;
        tdisp11++;
      }
      tdisp12++;
    }   
  }
  else
  {
    stat = -1;
  }

#endif

  return(stat);
}


/************************************************************************/
/************************************************************************/


static char rol_name__[40];


static void
__poll()
{
  CDOPOLL;
}


#ifdef	__cplusplus
extern "C"
#endif
/*static-error in dlsym*/ void
INIT_NAME(rolParam rolp)
{
  /*
  printf("INIT_NAME: rolp->daproc=%d\n",rolp->daproc);fflush(stdout);
  */
  if( (rolp->daproc != DA_DONE_PROC) && (rolp->daproc != DA_POLL_PROC) )
  {
    logMsg("INIT_NAME: INFO: rolp->daproc = %d\n",rolp->daproc,2,3,4,5,6);
  }

  switch(rolp->daproc)
  {
    case DA_INIT_PROC:
	{
	  char name[40];
	  rol = rolp;
	  rolp->inited = 1;
	  strcpy(rol_name__, ROL_NAME__);
	  rolp->listName = rol_name__;
	  printf("INIT_NAME: Initializing new rol structures for >%s<\n",rol_name__);fflush(stdout);
	  printf("INIT_NAME: MAX_EVENT_LENGTH = %d bytes, MAX_EVENT_POOL = %d\n",
			MAX_EVENT_LENGTH, MAX_EVENT_POOL);fflush(stdout);
	  strcpy(name, rolp->listName);
	  strcat(name, ":pool");
      printf("INIT_NAME: name >%s<\n",name);fflush(stdout);

	  rolp->inited = 1;
	  printf("Init - Done\n");
	  break;
	}

    case DA_FREE_PROC:
      rolp->inited = 0;
      break;
    case DA_DOWNLOAD_PROC:
	  __download();
	  break;
    case DA_PRESTART_PROC:
	  __prestart();
	  break;
    case DA_PAUSE_PROC:
	  __pause();
	  break;
    case DA_END_PROC:
	  __end();
	  break;
    case DA_GO_PROC:
tdisp1=0;
tdisp2=0;
tdisp3=0;
tdisp4=0;
tdisp5=0;
tdisp6=0;
tdisp7=0;
tdisp8=0;
tdisp9=0;
tdisp10=0;
tdisp11=0;
tdisp12=0;
tdisp13=0;
tdisp14=0;
tdisp15=0;
tdisp16=0;
tdisp17=0;
tdisp18=0;
tdisp19=0;
tdisp20=0;
tdisp21=0;
tdisp22=0;
tdisp23=0;
tdisp24=0;
	  __go();
	  break;
    case DA_POLL_PROC:
	  __poll();
	  break;
    case DA_DONE_PROC:
	  __done();
	  break;
    default:
	  logMsg("WARN: unsupported rol action = %d\n",rolp->daproc,2,3,4,5,6);
	  break;
  }
}

#endif

