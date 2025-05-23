/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench software
--
-- Module Name:    Mvt_RunTest.c
-- Description:    a standalone application for data taking from MVT
--
-- Target Devices: Linux PC
-- Tool versions:  Linux Make
-- 
-- Create Date:    0.0 2017/11/30 IM
-- Revision:       
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifdef Linux_vme

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define ROL_NAME__ "ROL1MVT"

#include "mvtLib.h"
#include "SysConfig.h"
#include "tiUtils.h"
#include "jvme.h"
#include "tiLib.h"

/********************************************
 * Function for non-blocking keyboard input *
/********************************************/
#include <termios.h>
void stdin_set(int cmd)
{
	struct termios t;
	tcgetattr(1,&t);
	switch (cmd)
	{
		case 1:
			t.c_lflag &= ~(ICANON | ECHO);
		break;
		default:
			t.c_lflag |=  (ICANON | ECHO);
		break;
	}
	tcsetattr(1,0,&t);
}
#define DEF_SET_STDIN_NONBLOCKING stdin_set(1)
#define DEF_SET_STDIN_BLOCKING    stdin_set(0)

//#include "../../dac/dac.s/bigbuf.h"
//#include "../../dac/dac.s/circbuf.h"
//#include "rol.h"

extern char *optarg;
extern int   optind;

FILE *log_fptr = (FILE *)NULL;
FILE *dat_fptr = (FILE *)NULL;
FILE *run_fptr = (FILE *)NULL;

// Default values
#define DEF_ConfFileName        ""
#define DEF_RocName             "UKN"
#define DEF_OutType             "None"

// Global variables
int verbose = 0; // if >0 some debug output

// big dma memory
unsigned int i2_from_rol1;
unsigned int *dma_dabufp;
// big out buffer for disentaglement
unsigned int *out_dabufp = (unsigned int *)NULL;

/************************************************************
* Attempt to bring here all CODA rol and format definitions *
************************************************************/
//typedef unsigned int uint32_t;

// Mimic bank types
#define BT_BANKS 0x0e

// Mimic event types
#define ET_RND 0xFE
#define ET_CST 0xFD
#define ET_PHY 0x00

static unsigned int *StartOfEvent;

#define CEOPEN( bnum, btype, syncFlag, nevents ) \
{ \
  StartOfEvent = dma_dabufp; \
  *dma_dabufp++ = 0; /*cleanup first word (will be CODA fragment length), otherwise following loop will think there is an event */ \
  *dma_dabufp++ = (syncFlag<<24) | ((bnum) << 16) | ((btype) << 8) | (nevents & 0xff); \
  /*printf("CEOPEN: tag=%d typ=%d num=%d\n",((*(rol->dabufp))>>16)&0xFF,((*(rol->dabufp))>>8)&0xFF,(*(rol->dabufp))&0xFF);*/ \
}

#define CECLOSE \
{ \
  int syncsync = 0; \
  /* writes bank length (in words) into the first word of the event (CODA header [0]) */ \
  *StartOfEvent = (int) (((char *) (dma_dabufp)) - ((char *) StartOfEvent)); \
  /* align pointer to the 64/128 bits - do we need it now ??? */ \
  if((*StartOfEvent & 1) != 0) \
  { \
    (dma_dabufp) = ((int32_t *)((char *) (dma_dabufp))+1); \
    *StartOfEvent += 1; \
  } \
  if((*StartOfEvent & 2) !=0) \
  { \
    *StartOfEvent = *StartOfEvent + 2; \
    (dma_dabufp) = ((int32_t *)((short *) (dma_dabufp))+1); \
  } \
  *StartOfEvent = ( (*StartOfEvent) >> 2) - 1; \
}

#define CPINIT \
  uint32_t *dataout_save0 = NULL; \
  uint32_t *dataout_save1; \
  uint32_t *dataout_save2; \
  unsigned char *b08; \
  unsigned short *b16; \
  uint32_t *b32; \
  unsigned long long *b64; \
  int32_t lenin = *(dma_dabufp) - 1;	\
  int32_t lenout = 2;  /* start from 3rd word, leaving first two for bank-of-banks header */ \
  int32_t lenev; \
  uint32_t *datain = (uint32_t *)((dma_dabufp)+2); \
  uint32_t *dataout = (uint32_t *)((out_dabufp)+2); \
  uint32_t *header = (uint32_t *)(out_dabufp); \

#define CPOPEN(btag,btyp,bnum) \
{ \
  dataout_save1 = dataout ++; /*remember beginning of the bank address, exclusive bank length will be here*/ \
  *dataout++ = (btag<<16) + (btyp<<8) + bnum; /*bank header*/ \
  b08 = (unsigned char *)dataout; \
}

#define CPOPEN0(btag,btyp,bnum) \
{ \
  dataout_save0 = dataout; \
  /*printf("CPOPEN0: dataout_save0 = 0x%016x\n",dataout_save0);*/ \
  *dataout++ = 0; /*remember beginning of the bank address, exclusive bank length will be here*/ \
  *dataout++ = ((btag)<<16) + ((btyp)<<8) + (bnum); /*bank header*/ \
}

#define CPCLOSE0 \
{ \
  /* writes bank length (in words) into the first word of the bank */ \
  /*printf("CPCLOSE0: dataout=0x%016x dataout_save0=0x%016x exc len=%d\n", dataout, dataout_save0, dataout - dataout_save0 - 1);*/ \
  *dataout_save0 = dataout - dataout_save0 - 1; \
  dataout_save0 = NULL; \
}

#define CPCLOSE \
{ \
  uint32_t padding; \
  /*printf("CPCLOSE: dataout before = 0x%016x, b08=0x%016x (0x%016x)\n",dataout,b08,(uint64_t)b08);*/ \
  dataout = (uint32_t *) ( ( ((uint64_t)b08+3)/4 ) * 4); \
  padding = (uint64_t)dataout - (uint64_t)b08; \
  dataout_save1[1] |= (padding&0x3)<<14; /*update bank header (2nd word) with padding info*/ \
  /*printf("CPCLOSE: 0x%016x(%d) --- 0x%016x(%d) --> padding %d\n",dataout,dataout,b08,b08,((dataout_save1[1])>>14)&0x3);*/ \
  *dataout_save1 = ((uint64_t)dataout-(uint64_t)dataout_save1)/4 - 1; /*write bank length in 32bit words*/ \
  /*printf("CPCLOSE: *dataout_save1 = 0x%016x (0x%016x 0x%016x)\n",*dataout_save1,dataout,dataout_save1);*/ \
  lenout += (*dataout_save1+1); \
  lenev += (*dataout_save1+1); \
  b08 = NULL; \
}

/* open composite bank with nonpacked samples data*/
#define MVT_CCOPEN_NOSMPPACK(btag,fmt,bnum) \
  /*if it is first board, open bank*/ \
  if(a_slot_old==-1) \
  { \
    { \
      int len1, n1; \
      char *ch; \
      len1 = strlen(fmt); /* format length in bytes */ \
      n1 = (len1+5)/4; /* format length in words */ \
      dataout_save1 = dataout ++; /*remember '0xf' bank length location*/ \
      *dataout++ = (btag<<16) + (0xf<<8) + bnum; /*bank header*/ \
      /* tagsegment header following by format */ \
      *dataout++ = (len1<<20) + (0x6<<16) + n1; \
      ch = (char *)dataout; \
      strncpy(ch,fmt,len1); \
      ch[len1]='\0';ch[len1+1]='\4';ch[len1+2]='\4';ch[len1+3]='\4';ch[len1+4]='\4'; \
      dataout += n1; \
      /* 'internal' bank header */ \
      dataout_save2 = dataout ++;  /*remember 'internal' bank length location*/ \
      *dataout++ = (0<<16) + (0x0<<8) + 0; \
    } \
    b08 = (unsigned char *)dataout; \
  } \
  /*if new slot, write stuff*/ \
  if(a_slot != a_slot_old) \
  { \
    a_channel_old = -1; /*for new slot, reset a_channel_old to -1*/ \
    a_slot_old = a_slot; \
    *b08++ = a_slot; \
    b32 = (unsigned int *)b08; \
    *b32++ = a_triggernumber; \
    b08 += 4; \
    *b32++ = a_trigtime[1]; \
    *b32   = a_trigtime[0]; \
    b08 += 8; \
    /*set pointer for the number of channels*/ \
    Nchan = (unsigned int *)b08; \
    Nchan[0] = 0; \
    b08 += 4; \
  }

/* open composite bank : with packed samples */
#define MVT_CCOPEN_PACKSMP(btag,fmt,bnum) \
  /*if it is first board, open bank*/ \
  if(a_slot_old==-1) \
  { \
    { \
      int len1, n1; \
      char *ch; \
      len1 = strlen(fmt); /* format length in bytes */ \
      n1 = (len1+5)/4; /* format length in words */ \
      dataout_save1 = dataout ++; /*remember '0xf' bank length location*/ \
      *dataout++ = (btag<<16) + (0xf<<8) + bnum; /*bank header*/ \
      /* tagsegment header following by format */ \
      *dataout++ = (len1<<20) + (0x6<<16) + n1; \
      ch = (char *)dataout; \
      strncpy(ch,fmt,len1); \
      ch[len1]='\0';ch[len1+1]='\4';ch[len1+2]='\4';ch[len1+3]='\4';ch[len1+4]='\4'; \
      dataout += n1; \
      /* 'internal' bank header */ \
      dataout_save2 = dataout ++;  /*remember 'internal' bank length location*/ \
      *dataout++ = (0<<16) + (0x0<<8) + 0; \
    } \
    b08 = (unsigned char *)dataout; \
  } \
  /*if new slot, write stuff*/ \
  if(a_slot != a_slot_old) \
  { \
    a_channel_old = -1; /*for new slot, reset a_channel_old to -1*/ \
    a_slot_old = a_slot; \
    *b08++ = a_slot; \
    b32 = (unsigned int *)b08; \
    *b32++ = a_triggernumber; \
    b08 += 4; \
    *b32++ = a_trigtime[1]; \
    *b32   = a_trigtime[0]; \
    b08 += 8; \
  }

/* close composite bank */
#define CCCLOSE \
{ \
  unsigned int padding; \
  dataout = (unsigned int *) ( ( ((unsigned int)b08+3)/4 ) * 4); \
  padding = (unsigned int)dataout - (unsigned int)b08; \
  /*dataout_save1[1] |= (padding&0x3)<<14;*/ \
  dataout_save2[1] |= (padding&0x3)<<14; \
  /*printf("CCCLOSE: 0x%08x %d --- 0x%08x %d --> padding %d\n",dataout,dataout,b08,b08,((dataout_save2[1])>>14)&0x3);*/ \
  *dataout_save1 = (dataout-dataout_save1-1); \
  *dataout_save2 = (dataout-dataout_save2-1); \
  /*printf("CCCLOSE:  *dataout_save1=%d *dataout_save2=%d\n", *dataout_save1, *dataout_save2);*/ \
  lenout += (*dataout_save1+1); \
  lenev += (*dataout_save1+1); \
  b08 = NULL; \
}

#define CPEXIT \
  lenout = (dataout-header); \
  lenev = lenout-1; \
  /*printf("CPEXIT: dataout=0x%08x header=0x%08x len=%d\n", dataout, header, lenout);*/\
  /*printf("CPEXIT lenout=%d\n",lenout);fflush(stdout);*/	\
  header[0] = lenout - 1; \
  /*printf("CPEXIT 1\n");fflush(stdout);*/	\
  header[1] = dma_dabufp[1]; \
  /*printf("CPEXIT 2\n");fflush(stdout);*/	\
  out_dabufp[lenout] = 1; /*it is here just for compatibility with new ROC2; will be redefined to the actual number of events in block*/ \
  /*printf("CPEXIT 3\n");fflush(stdout);*/ 


#define MAXBANKS 10

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
    int nw, bank;	\
    uint32_t *ptr, *ptrend; \
    nbanks = 0; \
    ptr = (uint32_t *)((dma_dabufp)); \
    ptrend = ptr + *(dma_dabufp); \
    ptr +=2; /*skip bank-of-banks header*/ \
    /*printf("BANKSCAN: ptr=0x%08x ptrend=0x%08x len=%d\n",ptr,ptrend,*(dma_dabufp));*/ \
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
      if(nbanks >= MAXBANKS) {fprintf(stderr, "BANKSCAN ERROR: nbanks=%d - exit\n");fflush(stderr);break;}; \
      /*printf("BANKSCAN[%d]: while end: ptr=0x%08x ptrenv=0x%08x (%d)\n",nbanks,ptr,ptrend,(ptrend-ptr));fflush(stdout);*/ \
    } \
    /*for(bank=0; bank<nbanks; bank++) printf("bankscan[%d]: tag 0x%08x typ=%d nr=%d nw=%d dataptr=0x%08x\n", \
      bank,banktag[bank],banktyp[bank],banknr[bank],banknw[bank],bankdata[bank]);*/ \
  }


static unsigned int *StartOfBank;
#define BANKOPEN(btag,btyp,bnum) \
{ \
  /*printf("BANKOPEN: StartOfBank=0x%08x\n",StartOfBank);fflush(stdout);*/ \
  /*printf("BANKOPEN: dma_dabufp=0x%08x\n",dma_dabufp);fflush(stdout);*/ \
  StartOfBank = dma_dabufp; \
  *dma_dabufp++ = 0; \
  *dma_dabufp++ = ((btag)<<16) + ((btyp)<<8) + (bnum);	\
  /*printf("BANKOPEN: dma_dabufp=0x%08x\n",dma_dabufp);fflush(stdout);*/ \
}

#define BANKCLOSE \
{ \
  /* writes bank length (in words) into the first word of the bank */ \
  *StartOfBank = dma_dabufp - StartOfBank - 1; \
  /*printf("BANKCLOSE: StartOfBank=0x%08x (0x%08x %d)\n",StartOfBank,*StartOfBank,*StartOfBank);fflush(stdout);*/ \
  /*printf("BANKCLOSE: dma_dabufp=0x%08x (0x%08x %d)\n",dma_dabufp,*dma_dabufp,*dma_dabufp);fflush(stdout);*/ \
  /*printf("BANKCLOSE: header: 0x%08x (%d), 0x%08x\n",*StartOfBank,*StartOfBank,*(StartOfBank+1));fflush(stdout);*/ \
}

// Mvt proper definitions from rol2
#define MAXBLOCK     2  /* MVT/FTT with not more than 2 SSP blocks=boards */
#define MAXEVENT    40  /* max number of events in one block */
#define MAXSAMPLES 128
#define MAXFEU      24
#define MAXCHAN    512

#define DEF_OUT_BUF_SIZE ( MAXEVENT * MAXBLOCK*MAXFEU * ( 1 + 4 + 8 + 4 + MAXCHAN*(2+4+MAXSAMPLES*2) ) / sizeof( int ) )

#define LSWAP(x)        ((((x) & 0x000000ff) << 24) | \
                         (((x) & 0x0000ff00) <<  8) | \
                         (((x) & 0x00ff0000) >>  8) | \
                         (((x) & 0xff000000) >> 24))

#ifdef PASS_AS_IS
	int nASIS;
	int iASIS[MAXBANKS]={0};
#define REPORT_RAW_BANK_IF_REQUESTED if(rol2_report_raw_data) iASIS[nASIS++] = bank
#endif

int mynev; /*defined in tttrans.c */


/* Reshufling tables moved outside the function */
	         int nB[  MAXBANKS]={0};
	         int iB[  MAXBANKS][MAXBLOCK]={0};
	         int sB[  MAXBANKS][MAXBLOCK]={0};

	         int nBT[ MAXBANKS]={0};
	         int iBT[ MAXBANKS][MAXBLOCK]={0}; 

	         int nE[  MAXBANKS][MAXBLOCK]={0};
	         int iE[  MAXBANKS][MAXBLOCK][MAXEVENT]={0};
	         int lenE[MAXBANKS][MAXBLOCK][MAXEVENT]={0};

	         int nSMP[MAXBANKS][MAXBLOCK][MAXEVENT]={0};
	         int iSMP[MAXBANKS][MAXBLOCK][MAXEVENT][MAXSAMPLES]={0};

	         int nFEU[         MAXBANKS][MAXBLOCK][MAXEVENT][MAXSAMPLES]={0};
	         int iFEU[         MAXBANKS][MAXBLOCK][MAXEVENT][MAXSAMPLES][MAXFEU]={0};
	unsigned int mFEU[         MAXBANKS][MAXBLOCK][MAXEVENT][MAXSAMPLES]={0};

	         int nbchannelsFEU[MAXBANKS][MAXBLOCK][MAXEVENT][MAXSAMPLES][MAXFEU]={0};
	         int sizeFEU[      MAXBANKS][MAXBLOCK][MAXEVENT][MAXSAMPLES][MAXFEU]={0};

/* table to reshaffle TPC data */  
#define MAX_FEU_EVT  40  /* max number of events in one block */
#define MAX_FEU_NUM  18  /* max number of FEUs per BEU */
#define MAX_FEU_CHN 512  /* max number of channels per FEU */
#define MAX_FEU_SMP 128  /* max number of samples per FEU */

unsigned short feu_act_msk[MAX_FEU_EVT] = {0};
         short feu_chn_cnt[MAX_FEU_EVT][MAX_FEU_NUM]={0};
          char feu_chn_ent_ind[MAX_FEU_EVT][MAX_FEU_NUM][MAX_FEU_CHN]={-1};
          char feu_chn_ent_cnt[MAX_FEU_EVT][MAX_FEU_NUM][MAX_FEU_CHN]={0};
unsigned short feu_chn_ent_val[MAX_FEU_EVT][MAX_FEU_NUM][MAX_FEU_CHN][2+MAX_FEU_SMP]={0xFFFF};

/* mvt board data type defs */
#define MVT_TYPE_BLKHDR    0xF3BB0000
#define MVT_TYPE_BLKTLR    0xFCCAFCAA
#define MVT_TYPE_EVTHDR    0xF3EE0000
#define MVT_TYPE_SAMPLE    0xF3550000
#define MVT_TYPE_FEUHDR    0xF3110000
#define MVT_TYPE_FILLER    0xFAAAFAAA

int MVT_ZS_MODE = -1;
int MVT_PRESCALE = 0;
int MVT_CMP_DATA_FMT = 0;
int MVT_NBR_OF_BEU = 0;
int MVT_NBR_EVENTS_PER_BLOCK = 0;
int MVT_NBR_SAMPLES_PER_EVENT = 0;
int MVT_NBR_OF_FEU[DEF_MAX_NB_OF_BEU] = {0}; // PROBLEM WITH THIS ARRAY !!! Pour acceder au nombre de feu sur une beu, j'utilise soit un index de beu soit un beuid ...bref, je me mélange.
                                             // Il me faut une correspondance beuID numero de beu ... bref, il faut harmoniser tout cela !!			
int mvt_event_number = 0; 

FILE *mvt_fptr_err_2 = (FILE *)NULL;

#define MVT_ERROR_NBR_OF_BEU            0x00000001
#define MVT_ERROR_NBR_EVENTS_PER_BLOCK  0x00000002
#define MVT_ERROR_NBR_SAMPLES_PER_EVENT 0x00000004
#define MVT_ERROR_NBR_OF_FEU            0x00000008
#define MVT_ERROR_EVENT_NUM             0x00000010
#define MVT_ERROR_BLOCK_NUM             0x00000020

/* daq stuff */
static int rol2_report_raw_data = 0;
/* daq stuff */

// TI staff
#define NTRIGBITS 6
static unsigned int bitscalers[NTRIGBITS];
static int havebits = 0;

//#define DEBUG_FP_TI
//#define DEBUG_SP_TI

//#define DEBUG_FP_MVT
//#define DEBUG_SP_MVT

int mvt_error_counter;

int disentanglement(int roc_id)
{
	// Common section
	int bank;
	int banknum = 0;
	int error;
	int printing;
	int have_time_stamp;
	int ii;
	int iii;
	int k;
	int m;

	// TI section
	int a_slot;
	int a_slot2;
	int a_slot_prev;
	int a_module_id;
	int a_blocknumber;
	int a_nevents;
	int a_nevents2;
	int a_event_type;
	int a_nwords;
	int a_event_number_l;
	int a_timestamp_l;
	int a_event_number_h;
	int a_timestamp_h;
	int a_bitpattern;

	// Mvt section
	int i_sample;
	int ibl;
	int iev;
	int currentFeuId;
	int currentBeuId;
	int current_block_number;
	int current_event_number_high;
	int current_event_number_low;
	int local_event_number_high;
	int local_event_number_low;
//	int mvt_error_counter;
	int mvt_error_type;
	int l;
	int p;
  int cur_chan_1st_smp_ptr;
  int cur_chan_smp_ptr;

	// Second pass : common section
	int nnE;

	// Second pass : TI section
	int rlen;
	int i;

	// Second pass: MVT section
	int a_slot_old;
	int current_timestamp_high;
	int current_timestamp_low;
	int i_feu;
	int i_dream;
	int i_channel;
  int i_pul;
	int a_channel_old;
	int a_triggernumber;
	int a_trigtime[4];
	int current_channel_id;
	int current_feu_tmstmp;
	unsigned int *Nchan;
	int current_sample;
  unsigned char c_number_of_bytes;
  unsigned short *pul_dat_ptr;
  unsigned char nb_of_smp;

	CPINIT;
	BANKINIT;

	mynev++; /* needed by ttfa.c */

	BANKSCAN;

	/* first for() over banks from rol1 */
#ifdef PASS_AS_IS
	nASIS = 0; /*cleanup AS IS banks counter*/
#endif
	for(bank=0; bank<nbanks; bank++)
	{
		datain = bankdata[bank];
		lenin = banknw[bank];
//printf("%s: bank=%d datain=0x%08x lenin=%d typ=0x%08x tag=0x%08x\n",__FUNCTION__, bank, datain, lenin, banktyp[bank], banktag[bank]);
		/* swap input buffer (assume that data from VME is big-endian, and we are on little-endian Intel) */
		if( banktyp[bank] != 3 )
			for(ii=0; ii<lenin; ii++)
				datain[ii] = LSWAP(datain[ii]);
//printf("%s: bank=%d tag=0x%08x\n",__FUNCTION__, bank, banktag[bank]);
		if(banktag[bank] == 0xe10A) /* TI hardware format */
		{
			banknum = roc_id;
#ifdef DEBUG_FP_TI
			printf("\nFIRST PASS TI\n\n");
#endif
			error = 0;
			ii=0;
			printing=1;
			a_slot_prev = -1;
			have_time_stamp = 1;
			nB[bank]=0; /*cleanup block counter*/

			while(ii<lenin)
			{
#ifdef DEBUG_FP_TI
				printf("[%5d] 0x%08x (lenin=%d)\n",ii,datain[ii],lenin);
#endif
				if( ((datain[ii]>>27)&0x1F) == 0x10) /*block header*/
				{
					a_slot_prev   = a_slot;
					a_slot        = ((datain[ii]>>22)&0x1F);
					a_module_id   = ((datain[ii]>>18)&0xF);
					a_blocknumber = ((datain[ii]>> 8)&0x3FF);
					a_nevents     = ( datain[ii]     &0xFF);
#ifdef DEBUG_FP_TI
					printf("[%3d] BLOCK HEADER: slot %d, nevents %d, block number %d module id %d\n",ii,
						a_slot,a_nevents,a_blocknumber,a_module_id);
					printf(">>> update iB and nB\n");
#endif
					nB[bank]++;                    /*increment block counter*/
					iB[bank][nB[bank]-1] = ii;     /*remember block start index*/
					sB[bank][nB[bank]-1] = a_slot; /*remember slot number*/
					nE[bank][nB[bank]-1] = 0;      /*cleanup event counter in current block*/
#ifdef DEBUG_FP_TI
					printf("0xe10A: bank=%d nB[bank]=%d\n",bank,nB[bank]);
#endif
					ii++;

					/* second block header word */
					if(((datain[ii]>>17)&0x7FFF) != 0x7F88)
						fprintf(stderr, "%s: ERROR in TI second block header word\n", __FUNCTION__);
					have_time_stamp = ((datain[ii]>>16)&0x1);
					if(((datain[ii]>>8)&0xFF) != 0x20)
						fprintf(stderr, "%s: ERROR in TI second block header word\n", __FUNCTION__);
					a_nevents2 = (datain[ii]&0xFF);
					if(a_nevents != a_nevents2)
						fprintf(stderr, "%s: ERROR in TI: a_nevents=%d != a_nevents2=%d\n", __FUNCTION__, a_nevents,a_nevents2);
					ii++;

					for(iii=0; iii<a_nevents; iii++)
					{
						/* event header */
						a_event_type = ((datain[ii]>>24)&0xFF);
//						if(a_event_type==0)
//							printf("%s: 1st pass ERROR in TI event header word: datain[%d]=0x%08X : event type = %d for event %d\n",
//								__FUNCTION__, ii, datain[ii], a_event_type, datain[ii+1]);
						if(((datain[ii]>>16)&0xFF)!=0x01)
							printf("%s: 1st pass ERROR in TI event header word: datain[%d]=0x%08X : (0x%02x) != 0x01\n",
								__FUNCTION__, ii, datain[ii], ((datain[ii]>>16)&0xFF));
						a_nwords = datain[ii]&0xFF;
#ifdef DEBUG_FP_TI
						printf("[%3d] EVENT HEADER, a_nwords = %d\n",ii,a_nwords);
#endif

						/*"close" previous event if any*/
						k = nB[bank]-1; /*current block index*/
#ifdef DEBUG_FP_TI
						printf("0xe10A: k=%d\n",k);
#endif
						if(nE[bank][k] > 0)
						{
							m = nE[bank][k]-1; /*current event number*/
							lenE[bank][k][m] = ii-iE[bank][k][m]; /*#words in current event*/
#ifdef DEBUG_FP_TI
							printf("0xe10A: m=%d lenE=%d\n",m,lenE[bank][k][m]);
#endif
						}
		
						/*"open" next event*/
						nE[bank][k] ++; /*increment event counter in current block*/
						m = nE[bank][k]-1; /*current event number*/
						iE[bank][k][m] = ii; /*remember event start index*/
#ifdef DEBUG_FP_TI
						printf("0xe10A: nE=%d m=%d iE=%d\n",nE[bank][k],m,iE[bank][k][m]);
#endif
						ii++;

						if(a_nwords>0)
						{
							a_event_number_l = datain[ii];
#ifdef DEBUG_FP_TI
							printf("[%3d] a_event_number_1 = %d\n",ii,a_event_number_l);
#endif
							ii++;
						}

						if(a_nwords>1)
						{
							a_timestamp_l = datain[ii];
#ifdef DEBUG_FP_TI
							printf("[%3d] a_timestamp_l = %d\n",ii,a_timestamp_l);
#endif
							ii++;
						}

						if(a_nwords>2)
						{
							a_event_number_h = (datain[ii]>>16)&0xFFFF;
							a_timestamp_h = datain[ii]&0xFFFF;
#ifdef DEBUG_FP_TI
							printf("[%3d] a_event_number_h = %d a_timestamp_h = %d \n",ii,a_event_number_h,a_timestamp_h);
#endif
							ii++;
						}

						if(a_nwords>3)
						{
							a_bitpattern = datain[ii]&0xFF;
#ifdef DEBUG_FP_TI
							printf("[%3d] a_bitpattern = 0x%08x\n",ii,a_bitpattern);
#endif
							ii++;
						}

						if(a_nwords>4)
						{
#ifdef DEBUG_FP_TI
							printf("[%3d] TS word4 = 0x%08x\n",ii,datain[ii]);
#endif
							ii++;
						}

						if(a_nwords>5)
						{
#ifdef DEBUG_FP_TI
							printf("[%3d] TS word5 = 0x%08x\n",ii,datain[ii]);
#endif
							ii++;
						}

						if(a_nwords>6)
						{
#ifdef DEBUG_FP_TI
							printf("[%3d] TS word6 = 0x%08x\n",ii,datain[ii]);
#endif
						ii++;
						}

						if(a_nwords>7)
						{
							fprintf(stderr, "%s: ERROR: TS has more then 7 data words - exit\n", __FUNCTION__);
							return(-1);
						}
					} // for(iii=0; iii<a_nevents; iii++)
				}
				else if( ((datain[ii]>>27)&0x1F) == 0x11) /*block trailer*/
				{
					a_slot2 = ((datain[ii]>>22)&0x1F);
					a_nwords = (datain[ii]&0x3FFFFF);
#ifdef DEBUG_FP_TI
					printf("[%3d] BLOCK TRAILER: slot %d, nwords %d\n", ii, a_slot2, a_nwords);
					printf(">>> data check\n");
#endif

					/*"close" previous event if any*/
					k = nB[bank]-1; /*current block index*/
					if(nE[bank][k] > 0)
					{
						m = nE[bank][k]-1; /*current event number*/
						lenE[bank][k][m] = ii-iE[bank][k][m]; /*#words in current event*/
					}

					if(a_slot2 != a_slot)
					{
						error ++;
						if(printing)
						{
							printf("[%3d][%3d] ERROR1 in TI data: blockheader slot %d != blocktrailer slot %d\n",mynev,
								ii,a_slot,a_slot2);
							printing=0;
						}
					}
					if(a_nwords != (ii-iB[bank][nB[bank]-1]+1))
					{
						error ++;
						if(printing)
						{
							printf("[%3d][%3d] ERROR2 in TI data: trailer #words %d != actual #words %d\n",mynev,
					 			ii,a_nwords,ii-iB[bank][nB[bank]-1]+1);
							printing=0;
						}
					}
					ii++;
				}
				else if( ((datain[ii]>>27)&0x1F) == 0x1F)
				{
#ifdef DEBUG_FP_TI
					printf("[%3d] FILLER WORD: \n",ii);
					printf(">>> do nothing\n");
#endif
					ii++;
				}
				else
				{
					fprintf(stderr, "%s: TI UNKNOWN data: [%3d] 0x%08x\n",__FUNCTION__, ii, datain[ii]);
					{
						int bankj;
						fprintf(stderr, "\n   Previous stuff\n");
						for(bankj=ii-20; bankj<ii; bankj++) fprintf(stderr, "          [%3d][%3d] 0x%08x\n",bankj,ii,datain[bankj]);
						for(bankj=ii; bankj<ii+10; bankj++) fprintf(stderr, "          [%3d][%3d] 0x%08x\n",bankj,ii,datain[bankj]);
						fprintf(stderr, "   End Of Previous stuff\n");fflush(stdout);
						return(-2);
					}
					ii++;
				}
			} /* while(ii<lenin) */
		} /* if(banktag[bank] == 0xe10A) TI hardware format */

		//***********************************************************************************
		//***                     MVT_ERROR_BLOCK_NUM                                     ***
		//***********************************************************************************
		/* newer format 27/08/2015
			F3BB BEUIDBlkNum
			F3EE Fine/TSTP4 TSTP3 TSTP2 TSTP1 EvtNum1 EvtNum2 EvtNum3 SmpNum AcceptEvtNum
			F311 LnkNumSize FEUdata...
			F311 LnkNumSize FEUdata...
			F355 FineTSTP TSTP1 TSTP2 TSTP3 EvtNum1 EvtNum2 EvtNum3 SmpNum AcceptEvtNum
			F311 LnkNumSize FEUdata...
			F311 LnkNumSize FEUdata...
			Filler...
			FCCA FCAA
		*/
		else if(banktag[bank] == 0xe118) /* MVT hardware format */
		{
#ifdef PASS_AS_IS
			if( ((mvt_event_number%MVT_PRESCALE)==0) && (MVT_PRESCALE!=1000000) )
				REPORT_RAW_BANK_IF_REQUESTED;
#endif
			banknum = roc_id;

			error   = 0;
			ii      = 0;
			printing= 1;
			nB[bank]  = 0; /* cleanup block counter */            
			nBT[bank] = 0; /* cleanup block trailer counter */
#ifdef DEBUG_FP_MVT
			if( mvt_fptr_err_2 != (FILE *)NULL )
			{
				fprintf(mvt_fptr_err_2,"%s: FIRST PASS MVT STARTS lenin=%d ii=%d datain[ii]=0x%08x address=0x%08x\n", __FUNCTION__, lenin, ii, datain[ii], datain);
				fflush(mvt_fptr_err_2);
			}
#endif

			while(ii<lenin)
			{
#ifdef DEBUG_FP_MVT
				if( mvt_fptr_err_2 != (FILE *)NULL )
				{
					fprintf(mvt_fptr_err_2,"%s: FIRST PASS MVT Go through lenin=%d ii=%d datain[ii]=0x%08x\n", __FUNCTION__, lenin, ii, datain[ii]);
					fflush(mvt_fptr_err_2);
				}
#endif
				if( (datain[ii]&0xFFFF0000) == MVT_TYPE_BLKHDR) /* block header */
				{
					nB[bank]++;                 /* increment block counter */
					iB[bank][nB[bank]-1] = ii;    /* remember block start index */
					nE[bank][nB[bank]-1] = 0;     /* cleanup event counter in current block */
					ii++;
#ifdef DEBUG_FP_MVT
					if( mvt_fptr_err_2 != (FILE *)NULL )
					{
						fprintf(mvt_fptr_err_2,"%s: FIRST PASS MVT MVT_TYPE_BLKHDR=0x%08x bank=%d ii=%d datain[ii]=0x%08x nB[bank]=%d\n",
							__FUNCTION__, MVT_TYPE_BLKHDR, bank, ii, datain[ii], nB[bank]);
						fflush(mvt_fptr_err_2);
					}
#endif
				}
				else if( (datain[ii]&0xFFFFFFFF) == MVT_TYPE_BLKTLR) /*block trailer*/
				{
					/* "close" previous event if any */
					k = nB[bank]-1; /* current block index*/
					if(nE[bank][k] > 0)
					{
						m = nE[bank][k]-1;                    /* current event number*/
						lenE[bank][k][m] = ii-iE[bank][k][m]; /* #words in current event */
					}	      
					nBT[bank]++; 
					ii++; 
				}
				else if ( (datain[ii]&0xFFFF0000) == MVT_TYPE_EVTHDR ) /*event header */
				{	    
					k = nB[bank]-1; /*current block index*/		

					/*"close" previous event if any*/
					if(nE[bank][k] > 0)
					{
						m = nE[bank][k]-1;                  /* current event number */
						lenE[bank][k][m] = ii-iE[bank][k][m]; /* #words in current event */
					}
					/*"open" next event*/
					nE[bank][k]++;           /* increment event counter in current block*/
					m = nE[bank][k]-1;       /* current event number*/
					iE[bank][k][m]      =ii; /* remember event start index*/	
					nSMP[bank][k][m]    = 1; /* initialize sample number in current event */
					iSMP[bank][k][m][0] = ii;
					nFEU[bank][k][m][0] = 0; /* cleanup FEU counter in current sample*/
					mFEU[bank][k][m][0] = 0; /* cleanup FEU map in current sample*/
					ii++;	      
				}
				else if (  (datain[ii]&0xFFFF0000) == MVT_TYPE_SAMPLE ) /*sample header*/
				{
					k = nB[bank]-1;            /* current block index */
					m = nE[bank][k]-1;         /* current event number */
					nSMP[bank][k][m]++;        /* initialize sample number in current event */
					l = nSMP[bank][k][m]-1;
					iSMP[bank][k][m][l] = ii;
					nFEU[bank][k][m][l] = 0;   /* cleanup FEU counter in current sample */
					mFEU[bank][k][m][l] = 0;   /* cleanup FEU map in current sample */
					ii++;	  
				}
				else if( (datain[ii]&0xFFFF0000) == MVT_TYPE_FEUHDR) /* mvt sample */
				{
					k = nB[bank]-1;             /* current block index*/
					m = nE[bank][k]-1;          /* current event number*/
					l = nSMP[bank][k][m]-1;     /* current sample number*/
					nFEU[bank][k][m][l]++;      /* incrment FEU counter in current sample*/
					p = nFEU[bank][k][m][l]-1;  /* current feu number*/
					iFEU[bank][k][m][l][p]=ii;  /* remember event start index*/
					nbchannelsFEU[bank][k][m][l][p]=0;
					sizeFEU[bank][k][m][l][p] = (datain[ii]&0x000003FF);

					currentFeuId = ((( datain[ ii] ) & 0x00007C00 ) >> 10 );
					if( (currentFeuId < 0) || (currentFeuId >23) )
						fprintf(mvt_fptr_err_2,"%s: FIRST PASS strange feu id %d in datain[ii]=0x%08x \n", __FUNCTION__, currentFeuId, ii, datain[ii]);
					else
						mFEU[bank][k][m][l] |= (1<<currentFeuId); 

#ifdef DEBUG_FP_MVT
					if( mvt_fptr_err_2 != (FILE *)NULL )
					{
						fprintf(mvt_fptr_err_2,"%s: MVT_ZS_MODE : %d ii=%d datain[ii]=0x%08x\n", __FUNCTION__, MVT_ZS_MODE, ii, datain[ii] );
						fflush(mvt_fptr_err_2);
					}
#endif
					if (!MVT_ZS_MODE)
					{ 
						nbchannelsFEU[bank][k][m][l][p] =  ( sizeFEU[bank][k][m][l][p]  - 2 - 6  ) / 74 * 64;		//????? ATTENTION : this size has an offset
					}
					else
					{
						nbchannelsFEU[bank][k][m][l][p] =  ( sizeFEU[bank][k][m][l][p]  - 2 - 6  ) / 2;
					}
#ifdef DEBUG_FP_MVT					
					if( mvt_fptr_err_2 != (FILE *)NULL )
					{
						fprintf(mvt_fptr_err_2,"%s: FIRST PASS 0xe118 : bnk=%d blk=%d evt=%d smp=%d feu=%d size %d (0x%08x) nbchannelsFEU %d\n",
							__FUNCTION__, bank, k, m, l, p, sizeFEU[bank][k][m][l][p], sizeFEU[bank][k][m][l][p], nbchannelsFEU[bank][k][m][l][p]);
						fflush(mvt_fptr_err_2);
					}
#endif
					if( nbchannelsFEU[bank][k][m][l][p] > 0 )
          {
            if( MVT_ZS_MODE != 2 ) // Treat No ZS and Tracker ZS cases
						  ii+= ((sizeFEU[bank][k][m][l][p]  -2) >> 1 ) ;   //big jump over the data
            else // Treat TPC ZS case
            {
              // skip FEU packet header F311 LnkNumSize (2 16-bit words) and FEU data header 4 16-bit words
              ii+=3;
              do
              {
                current_channel_id = ((( datain[ ii ] ) & 0x01FF0000) >> 16 );
#ifdef DEBUG_FP_MVT					
					      if( mvt_fptr_err_2 != (FILE *)NULL )
					      {
						      fprintf(mvt_fptr_err_2,
                    "%s: FIRST PASS 0xe118: ZS TPC: smp=%d feu=%d size %d nbchannelsFEU %d currentFeuId=%d current_channel_id=%d\n",
							      __FUNCTION__, l, p, sizeFEU[bank][k][m][l][p], nbchannelsFEU[bank][k][m][l][p], currentFeuId, current_channel_id);
						      fflush(mvt_fptr_err_2);
					      }
#endif
                cur_chan_1st_smp_ptr = feu_chn_ent_ind[m][p][current_channel_id];
#ifdef DEBUG_FP_MVT					
				        if( mvt_fptr_err_2 != (FILE *)NULL )
				        {
					        fprintf(mvt_fptr_err_2,
                    "%s: FIRST PASS 0xe118: ZS TPC: currentFeuId=%d current_channel_id=%d cur_chan_1st_smp_ptr=%d\n",
						        __FUNCTION__, currentFeuId, current_channel_id, cur_chan_1st_smp_ptr);
					        fflush(mvt_fptr_err_2);
				        }
#endif
                if( cur_chan_1st_smp_ptr == -1 )
                {
                  cur_chan_1st_smp_ptr = 0;
                  feu_chn_ent_ind[m][p][current_channel_id]=cur_chan_1st_smp_ptr;
                  feu_chn_ent_val[m][p][current_channel_id][cur_chan_1st_smp_ptr  ]=l;
                  feu_chn_ent_val[m][p][current_channel_id][cur_chan_1st_smp_ptr+1]=0;
                  feu_chn_ent_cnt[m][p][current_channel_id]=1;
                  feu_chn_cnt[m][p]++;
#ifdef DEBUG_FP_MVT					
					        if( mvt_fptr_err_2 != (FILE *)NULL )
					        {
						        fprintf(mvt_fptr_err_2,
                      "%s: FIRST PASS 0xe118: ZS TPC: First: currentFeuId=%d current_channel_id=%d cur_chan_1st_smp_ptr=%d feu_chn_cnt=%d feu_chn_ent_cnt=%d\n",
							        __FUNCTION__, currentFeuId, current_channel_id, cur_chan_1st_smp_ptr, feu_chn_cnt[m][p], feu_chn_ent_cnt[m][p][current_channel_id]);
						        fflush(mvt_fptr_err_2);
					        }
#endif
                }
                else if( (feu_chn_ent_val[m][p][current_channel_id][cur_chan_1st_smp_ptr] + feu_chn_ent_val[m][p][current_channel_id][cur_chan_1st_smp_ptr+1] + 1) < l )
                {
                  cur_chan_1st_smp_ptr += (feu_chn_ent_val[m][p][current_channel_id][cur_chan_1st_smp_ptr+1]+2);
                  feu_chn_ent_ind[m][p][current_channel_id]=cur_chan_1st_smp_ptr;
                  feu_chn_ent_val[m][p][current_channel_id][cur_chan_1st_smp_ptr  ]=l;
                  feu_chn_ent_val[m][p][current_channel_id][cur_chan_1st_smp_ptr+1]=0;
                  feu_chn_ent_cnt[m][p][current_channel_id]++;
#ifdef DEBUG_FP_MVT					
					        if( mvt_fptr_err_2 != (FILE *)NULL )
					        {
						        fprintf(mvt_fptr_err_2,
                      "%s: FIRST PASS 0xe118: ZS TPC: Next: currentFeuId=%d current_channel_id=%d cur_chan_1st_smp_ptr=%d feu_chn_cnt=%d feu_chn_ent_cnt=%d\n",
							        __FUNCTION__, currentFeuId, current_channel_id, cur_chan_1st_smp_ptr, feu_chn_cnt[m][p], feu_chn_ent_cnt[m][p][current_channel_id]);
						        fflush(mvt_fptr_err_2);
					        }
#endif
                }
                cur_chan_smp_ptr = feu_chn_ent_ind[m][p][current_channel_id] + feu_chn_ent_val[m][p][current_channel_id][cur_chan_1st_smp_ptr+1] + 2;
                feu_chn_ent_val[m][p][current_channel_id][cur_chan_smp_ptr] =  datain[ii]&0x00000FFF; // store data
                feu_chn_ent_val[m][p][current_channel_id][cur_chan_1st_smp_ptr+1]++;
#ifdef DEBUG_FP_MVT					
				        if( mvt_fptr_err_2 != (FILE *)NULL )
				        {
					        fprintf(mvt_fptr_err_2,
                    "%s: FIRST PASS 0xe118: ZS TPC: currentFeuId=%d current_channel_id=%d cur_pul=%d cur_pul_1st_smp_ptr=%d cur_pul_smp_ptr=%d cur_pul_smp_cnt=%d\n",
						        __FUNCTION__, currentFeuId, current_channel_id, feu_chn_ent_cnt[m][p][current_channel_id],
                    cur_chan_1st_smp_ptr, cur_chan_smp_ptr, feu_chn_ent_val[m][p][current_channel_id][cur_chan_1st_smp_ptr+1]);
					        fflush(mvt_fptr_err_2);
				        }
#endif
                ii++;
              } while( (datain[ii]&0x70000000) != 0x70000000 ); // Check for FEU trailer
            } // else of if( MVT_ZS_MODE != 2 ): trating TPC ZS case
          }
					else
						ii++;
#ifdef DEBUG_FP_MVT								
					if( mvt_fptr_err_2 != (FILE *)NULL )
					{
						fprintf(mvt_fptr_err_2,"%s: FIRST PASS 0xe118 after big jump over data: ii=%d datain[ii]=0x%08x \n", __FUNCTION__, ii, datain[ii]);
						fflush(mvt_fptr_err_2);
					}
#endif
				}
				else if( (datain[ii]&0xFFFFFFFF) == MVT_TYPE_FILLER) /*filler*/
				{
					ii++;
				}
				else
				{
					ii++;
				}
			} /* while(ii<lenin) */

			//check data structure  		
			mvt_error_counter = 0;
			mvt_error_type    = 0;
			if (nB[bank] != MVT_NBR_OF_BEU )
			{
				mvt_error_counter ++;
				mvt_error_type |= MVT_ERROR_NBR_OF_BEU;
#ifdef DEBUG_FP_MVT
				if( mvt_fptr_err_2 != (FILE *)NULL )
				{
					fprintf(mvt_fptr_err_2,"%s: MVT FIRST PASS 0xe118 ERROR: bank=%d nB[bank]=%d != MVT_NBR_OF_BEU=%d\n",
						__FUNCTION__, bank, nB[bank], MVT_NBR_OF_BEU);
					fflush(mvt_fptr_err_2);
				}
#endif
			}
			else
			{
				for( ibl=0; ibl<MVT_NBR_OF_BEU; ibl++ )
				{ 		
					currentBeuId = ((( datain[ iB[bank][ibl] ] ) & 0x0000F000 ) >> 12 )  - 1;		
					if( nE[bank][ibl] != MVT_NBR_EVENTS_PER_BLOCK )
					{
						mvt_error_counter ++;
						mvt_error_type |= MVT_ERROR_NBR_EVENTS_PER_BLOCK;
					} 
					else
					{
						for( iev=0; iev<MVT_NBR_EVENTS_PER_BLOCK; iev++ )
						{ 
							if( nSMP[bank][ibl][iev] != MVT_NBR_SAMPLES_PER_EVENT )
							{
								mvt_error_counter ++;
								mvt_error_type |= MVT_ERROR_NBR_SAMPLES_PER_EVENT;
								printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
								printf("0xe118 : mvt error currentBeuId =%d bank =%d ibl=%d iev=%d nsmp =%d expected=%d\n",
									currentBeuId, bank, ibl, iev, nSMP[bank][ibl][iev], MVT_NBR_SAMPLES_PER_EVENT );
							} 
							else
							{
								for( i_sample=0; i_sample<MVT_NBR_SAMPLES_PER_EVENT; i_sample++)
								{ 
									if( nFEU[bank][ibl][iev][i_sample] != MVT_NBR_OF_FEU[currentBeuId] )
									{
										printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
//										printf("0xe118 : mvt error currentBeuId =%d bank =%d ibl=%d iev=%d i_sample=%d nfeu=%d expected=%d\n",
//											currentBeuId, bank, ibl, iev, i_sample, nFEU[bank][ibl][iev][i_sample],
//											MVT_NBR_OF_FEU[currentBeuId] );
										printf("0xe118 : mvt error currentBeuId =%d bank =%d ibl=%d iev=%d i_sample=%d nfeu=%d expected=%d received from 0x%08x\n",
											currentBeuId, bank, ibl, iev, i_sample, nFEU[bank][ibl][iev][i_sample],
											MVT_NBR_OF_FEU[currentBeuId], mFEU[bank][ibl][iev][i_sample]);
										printf("0xe118 : nfeu for beu 0 %d 1 %d 2 %d\n", MVT_NBR_OF_FEU[0], MVT_NBR_OF_FEU[1], MVT_NBR_OF_FEU[2] );
										mvt_error_counter ++;
										mvt_error_type |= MVT_ERROR_NBR_OF_FEU;
										if( mvt_fptr_err_2 != (FILE *)NULL )
										{
											fprintf
											(
												mvt_fptr_err_2,
												"%s: MVT FIRST PASS 0xe118 ERROR: beu=%d bnk=%d ibl=%d iev=%d ismp=%d nfeu=%d exp=%d rcv 0x%08x\n",
												__FUNCTION__, currentBeuId, bank, ibl, iev, i_sample, nFEU[bank][ibl][iev][i_sample],
												MVT_NBR_OF_FEU[currentBeuId], mFEU[bank][ibl][iev][i_sample]
											);
											fflush(mvt_fptr_err_2);
										}
									} // if( nFEU[bank][ibl][iev][i_sample] != MVT_NBR_OF_FEU[currentBeuId] ) 
								} // for (i_sample=0; i_sample < MVT_NBR_SAMPLES_PER_EVENT ; i_sample ++)
							} // else of if( nSMP[bank][ibl][iev] != MVT_NBR_SAMPLES_PER_EVENT )
						} // for (iev=0; iev < MVT_NBR_EVENTS_PER_BLOCK ; iev ++)
					}
				} // for(ibl=0; ibl < MVT_NBR_OF_BEU ; ibl++)
			} // else of if (nB[bank] != MVT_NBR_OF_BEU )

			//check block numbers BEUSSP block headers
			// - test that all blocks have the same number
			// - test that block number increments correctly
			current_block_number = (( datain[  iB[bank][0]  ]  )& 0x00000FFF ) ; 
			for(ibl=0; ibl < MVT_NBR_OF_BEU ; ibl++)
			{ 
				if (   ( ( datain[  iB[bank][ibl]  ]  )& 0x00000FFF ) != current_block_number )
				{
					mvt_error_counter ++;
					mvt_error_type |= MVT_ERROR_BLOCK_NUM;
				}
				//if ( ( ( ( datain[  iB[bank][ibl]  ]  )& 0x00007000 )>> 12) != ibl ) {mvt_error_counter ++; mvt_error_type |= MVT_ERROR_BLOCK_ID;}
			}

			//check event numbers and timestamps from BEUSSP event headers
			for (iev=0; iev < MVT_NBR_EVENTS_PER_BLOCK ; iev ++)
			{
				current_event_number_high = (( datain[ ( iSMP[bank][0][iev][0] + 2 ) ] ) & 0x00007FFC) >> 2;
				current_event_number_low  =
					((( datain[ ( iSMP[bank][0][iev][0] + 2 ) ] ) & 0x00000003) << 30) + 
					((( datain[ ( iSMP[bank][0][iev][0] + 3 ) ] ) & 0x7FFF0000) >>  1) + 
					((  datain[ ( iSMP[bank][0][iev][0] + 3 ) ] ) & 0x00007FFF);		
				for(ibl=0; ibl < MVT_NBR_OF_BEU ; ibl++)
				{ 
					for (i_sample=0; i_sample < MVT_NBR_SAMPLES_PER_EVENT ; i_sample ++)
					{ 
						local_event_number_high = (( datain[ ( iSMP[bank][ibl][iev][i_sample] + 2 ) ] ) & 0x00007FFC) >> 2;
						local_event_number_low  =
							((( datain[ ( iSMP[bank][ibl][iev][i_sample] + 2 ) ] ) & 0x00000003) << 30) + 
							((( datain[ ( iSMP[bank][ibl][iev][i_sample] + 3 ) ] ) & 0x7FFF0000) >>  1) + 
							((  datain[ ( iSMP[bank][ibl][iev][i_sample] + 3 ) ] ) & 0x00007FFF);		
						if (( local_event_number_high != current_event_number_high) || ( local_event_number_low != current_event_number_low))
						{
							mvt_error_counter ++;
							mvt_error_type |= MVT_ERROR_EVENT_NUM;
printf("0xe118 : mvt error MVT_ERROR_EVENT_NUM counter 0x%8x %d mvt error type 0x%8x for buflen %d\n", mvt_error_counter, mvt_error_counter, mvt_error_type, lenin );
printf("iev=%d   current_event_number_high=%d low=%d\n", iev, current_event_number_high, current_event_number_low );
printf("smp=%d   event_number_high=%d low=%d\n", i_sample, local_event_number_high, local_event_number_low );
printf("0x%08x 0x%08x 0x%08x 0x%08x \n", i_sample, local_event_number_high, local_event_number_low );
//exit(1);
						}      		
					}
				}
			} // for (iev=0; iev < MVT_NBR_EVENTS_PER_BLOCK ; iev ++)

			if (mvt_error_counter !=0)
			{
				printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
				printf("0xe118 : mvt error counter 0x%8x %d mvt error type 0x%8x for buflen %d\n", mvt_error_counter, mvt_error_counter, mvt_error_type, lenin );
				if( mvt_fptr_err_2 != (FILE *)NULL )
				{
					fprintf(mvt_fptr_err_2,"%s: 0xe118 : mvt error counter 0x%8x %d mvt error type 0x%8x for buflen %d\n",
						__FUNCTION__, mvt_error_counter, mvt_error_counter, mvt_error_type, lenin );
#ifdef DEBUG_MVT_DAT_ERR
					for(ii=0; ii<lenin; ii++)
					{
						if( (ii%8) == 0 )
							fprintf(mvt_fptr_err_2,"%4d:", ii );
						fprintf(mvt_fptr_err_2," 0x%08x", datain[ii] );
						if( (ii%8) == 7 )
							fprintf(mvt_fptr_err_2,"\n" );
					}
					fprintf(mvt_fptr_err_2,"\n" );
					fflush(mvt_fptr_err_2);
#endif
				}
				else
				{
					printf("PPPPPPPPPPPPPPPAAAAAAAAAAAAAAAAAAANNNNNNNNNNNNNNNNNNNIIIIIIIIIIIIIIIIIIIIIICCCCCCCCCCCCC\n");
				}
			}
					
			/*
			//check event numbers and timestamps from FEUs 
			for (iev=0; iev < MVT_NBR_EVENTS_PER_BLOCK ; iev ++)
			{ 
				for(ibl=0; ibl < MVT_NBR_OF_BEU ; ibl++)
				{ 				
					for (ismp=0; ismp < MVT_NBR_SAMPLES_PER_EVENT ; ismp ++)
					{ 
						for (ifeu = 0; i_feu < MVT_NBR_OF_FEU[ibl]; ifeu++)
						{	
						}
					}
				}
			}
			*/
		} /* else if(banktag[bank] == 0xe118) /* MVT hardware format */

//***********************************************************************************
//***********************************************************************************
//***********************************************************************************
#ifdef PASS_AS_IS
		else /*any other bank will be passed 'as is' */
		{
			iASIS[nASIS++] = bank; /* remember bank number as it reported by BANKSCAN */
			/*printf("mynev=%d: remember bank number %d\n",mynev,bank);*/
		}
#endif
	} // for(bank=0; bank<nbanks; bank++)

	/********************************************************/
	/********************************************************/
	/********************************************************/
	/* SECOND PASS: disantangling and filling output buffer */
#ifdef DEBUG
	fprintf(stderr, "\n\n\n%s: SECOND PASS\n\n", __FUNCTION__);
#endif

	lenout   = 2; /* already done in CPINIT !!?? */
	b08      = NULL;
	printing = 1;

	nnE = nE[0][0]; // ALL BLOCKS in ALL BANKS ARE EXPECTED TO HAVE THE SAME NUMBER OF EVENTS
#ifdef DEBUG
	printf("nnE=%d\n",nnE);
#endif

	/*loop over events*/
	for(iev=0; iev<nnE; iev++)
	{
//		mvt_event_number ++;
		lenev = 2;

		banknum = iev; /* using event number inside block as bank number - for now */

		for(bank=0; bank<nbanks; bank++) /* loop over evio banks */
		{

			datain = bankdata[bank];
#ifdef DEBUG
			printf("iev=%d bank=%d nB=%d\n",iev,bank,nB[bank]);
#endif

			if(banktag[bank] == 0xe10A) /* TI hardware format */
			{
				banknum = iev; /*rol->pid;*/
#ifdef DEBUG_SP_TI
				printf("SECOND PASS TI\n");
#endif
				for(ibl=0; ibl<nB[bank]; ibl++) /*loop over blocks*/
				{
#ifdef DEBUG_SP_TI0
					printf("\n\n\n0xe10A: Block %d, Event %2d, event index %2d, event lenght %2d\n",
						ibl,iev,iE[bank][ibl][iev],lenE[bank][ibl][iev]);
#endif
					a_slot = sB[bank][ibl];
					ii     = iE[bank][ibl][iev];
					rlen   = ii + lenE[bank][ibl][iev];
					//printf("TI EVENT HEADER, ii = %d , rlen = %d \n",ii, rlen);
					while(ii<rlen)
					{
#ifdef DEBUG_SP_TI0
						printf("[%5d] 0x%08x (rlen=%d)\n",ii,datain[ii],rlen);
#endif
CPOPEN0( roc_id, 0xe, bank );
						CPOPEN(0xe10A,1,banknum);
						/* event header */
						a_event_type = ((datain[ii]>>24)&0xFF);
						if(((datain[ii]>>16)&0xFF)!=0x01) printf("%s: 2nd pass ERROR in TI event header word (0x%02x) != 0x01\n",__FUNCTION__, ((datain[ii]>>16)&0xFF));
						a_nwords = datain[ii]&0xFF;
#ifdef DEBUG_SP_TI
						printf("[%3d] EVENT HEADER, a_nwords = %d\n",ii,a_nwords);
#endif
						dataout[0] = datain[ii];
						b08 += 4;

						ii++;

						if(a_nwords>0)
						{
							a_event_number_l = datain[ii];
#ifdef DEBUG_SP_TI
							printf("[%3d] a_event_number_1 = %d\n",ii,a_event_number_l);
#endif
							dataout[1] = datain[ii];
							b08 += 4;
							ii++;
						}
						if(a_nwords>1)
						{
							a_timestamp_l = datain[ii];
#ifdef DEBUG_SP_TI
							printf("[%3d] a_timestamp_l = %d\n",ii,a_timestamp_l);
#endif

							dataout[2] = datain[ii];
							b08 += 4;
							ii++;
						}
						if(a_nwords>2)
						{
							a_event_number_h = (datain[ii]>>16)&0xFFFF;
							a_timestamp_h = datain[ii]&0xFFFF;
#ifdef DEBUG_SP_TI
							printf("[%3d] a_event_number_h = %d a_timestamp_h = %d \n",ii,a_event_number_h,a_timestamp_h);
#endif
							dataout[3] = datain[ii];
							b08 += 4;
							ii++;
						}
						if(a_nwords>3)
						{
							a_bitpattern = datain[ii]&0xFF;
#ifdef DEBUG_SP_TI
							printf("[%3d] a_bitpattern = 0x%08x\n",ii,a_bitpattern);
#endif
							for(i=0; i<NTRIGBITS; i++)
								bitscalers[i] += ((a_bitpattern>>i)&0x1);
							havebits = 1;
							dataout[4] = datain[ii];
							b08 += 4;
							ii++;
						}
						if(a_nwords>4)
						{
#ifdef DEBUG_SP_TI
							printf("[%3d] TS word4 = 0x%08x\n",ii,datain[ii]);
#endif
							ii++;
						}
						if(a_nwords>5)
						{
#ifdef DEBUG_SP_TI
							printf("[%3d] TS word5 = 0x%08x\n",ii,datain[ii]);
#endif
							ii++;
						}
						if(a_nwords>6)
						{
#ifdef DEBUG_SP_TI
							printf("[%3d] TS word6 = 0x%08x\n",ii,datain[ii]);
#endif
							ii++;
						}
						if(a_nwords>7)
						{
							printf("ERROR: TS has more then 7 data words - exit\n");
							return -1;
						}
#ifdef DEBUG_SP_TI
						printf("[%3d] CPCLOSE b08=0x%08x\n",ii,b08);
#endif
						CPCLOSE;
					} /* while(ii<rlen) */
				} /* for(ibl=0; ibl<nB[bank]; ibl++)loop over blocks */
			} /* if(banktag[bank] == 0xe10A) /* TI hardware format */
			//***********************************************************************************
			//***                     MVT_ERROR_BLOCK_NUM                                     ***
			//***********************************************************************************
			/* newer format 27/08/2015
				F3BB BEUIDBlkNum
				F3EE Fine/TSTP4 TSTP3 TSTP2 TSTP1 EvtNum1 EvtNum2 EvtNum3 SmpNum AcceptEvtNum
				F311 LnkNumSize FEUdata...
				F311 LnkNumSize FEUdata...
				F355 FineTSTP TSTP1 TSTP2 TSTP3 EvtNum1 EvtNum2 EvtNum3 SmpNum AcceptEvtNum
				F311 LnkNumSize FEUdata...
				F311 LnkNumSize FEUdata...
				Filler...
				FCCA FCAA
			*/
			else if(banktag[bank] == 0xe118) /* MVT hardware format */	// entering an MVT data bank
			{
#ifdef DEBUG_SP_MVT
				if( mvt_fptr_err_2 != (FILE *)NULL )
				{
					fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118 \n", __FUNCTION__);
					fflush(mvt_fptr_err_2);
				}
#endif
				banknum = iev; /*rol->pid;*/	// event number 
    				a_slot_old = -1;

				// Prescale events
				if ( (mvt_event_number % MVT_PRESCALE ) ||( MVT_PRESCALE == 1000000 ) )
				{
/*
					CPOPEN(0xefef,1,banknum);
					*dataout ++ = 0xCAFEFADE;
					b08 += 4;
					for(ibl=0; ibl < MVT_NBR_OF_BEU; ibl++)
					{ 
						*dataout ++ = lenE[bank][ibl][iev];
						b08 += 4;
					}
					*dataout ++ = 0xDEADBEEF;
					b08 += 4;
					CPCLOSE;
*/
				}
				else // Real Data
				{
					if( mvt_error_counter==0 )
					{
					 	//output the FEU data samples 				
				 		for( ibl=0; ibl<MVT_NBR_OF_BEU; ibl++ )
						{   							
							currentBeuId = ((( datain[ iB[bank][ibl] ] ) & 0x0000F000 ) >> 12 ) - 1;
#ifdef DEBUG_SP_MVT
							if( mvt_fptr_err_2 != (FILE *)NULL )
							{
								fprintf(mvt_fptr_err_2,"%s: 2nd PASS 0xe118, currentBeuId = %d \n", __FUNCTION__, currentBeuId);
								fflush(mvt_fptr_err_2);
							}
#endif
							for( i_feu=0; i_feu<MVT_NBR_OF_FEU[currentBeuId]; i_feu++ )
							{
								// Treat the FEU if it has fired channels
								if
                (
                  ((MVT_ZS_MODE != 2) && (nbchannelsFEU[bank][ibl][iev][0][i_feu]>0))
                  ||
                  ((MVT_ZS_MODE == 2) && (feu_chn_cnt[iev][i_feu]>0))
                )
								{
									a_channel_old = -1;

                  // Find pointer to feu data
                  if( MVT_ZS_MODE != 2 )
									  ii = iFEU[bank][ibl][iev][0][i_feu];
                  else
                  {
                    for( i_channel=0; i_channel<MAX_FEU_CHN; i_channel++ ) 
                    {
                      // Check that the channel has data 
                      if( feu_chn_ent_cnt[iev][i_feu][i_channel] > 0 )
                        break;
                    }
                    ii = iFEU[bank][ibl][iev][ feu_chn_ent_val[iev][i_feu][i_channel][0] ][i_feu];
                  } 

									// CURRENT SLOT NUMBER built from beu id and feu link id
									currentFeuId = ((( datain[ ii] ) & 0x00007C00 ) >> 10 );		
									a_slot= ((currentBeuId << 5) + currentFeuId  + 1 )& 0x000000FF ;
#ifdef DEBUG_SP_MVT
									if( mvt_fptr_err_2 != (FILE *)NULL )
									{
										fprintf(mvt_fptr_err_2,"%s: 2nd PASS 0xe118, data[%d]=0x%08x currentFeuId=%d data+1=0x%08x data+2=0x%08x\n",
											__FUNCTION__, ii, datain[ii], currentFeuId, datain[ii+1], datain[ii+2]);
										fflush(mvt_fptr_err_2);
									}
#endif
									// OUTPUT CURRENT TRIGGER/EVNET NUMBER
									current_event_number_low  = ((( datain[ ( iSMP[bank][ibl][iev][0] + 2 ) ] ) & 0x00000003) << 30) + 
												                      ((( datain[ ( iSMP[bank][ibl][iev][0] + 3 ) ] ) & 0x7FFF0000) >>  1) +
												                       (( datain[ ( iSMP[bank][ibl][iev][0] + 3 ) ] ) & 0x00007FFF);
									a_triggernumber = current_event_number_low;
#ifdef DEBUG_SP_MVT
									if( mvt_fptr_err_2 != (FILE *)NULL )
									{
										fprintf(mvt_fptr_err_2,"%s: 2nd PASS 0xe118, current_event_number_low = %d\n",
											__FUNCTION__, current_event_number_low);
										fflush(mvt_fptr_err_2);
									}
#endif
									//OUTPUT CURRENT TIMESTAMP 
									current_timestamp_low  = ((( datain[ ( iSMP[bank][ibl][iev][0] + 2 ) ] )    & 0x7FFF0000) << 1) +
 											                     (((~( datain[ ( iSMP[bank][ibl][iev][0]     ) ] )) & 0x00000800) << 5) ;
	
									current_timestamp_high = ((( datain[ ( iSMP[bank][ibl][iev][0] + 1 ) ] ) & 0x7FFF0000) >>  1) + 
											                     ((( datain[ ( iSMP[bank][ibl][iev][0] + 1 ) ] ) & 0x00007FFF));		
                  // Feu Time stamp
                  current_feu_tmstmp  = current_timestamp_low +
										                    ((( datain[ ii + 2] ) & 0x0FFF0000) >>  13 ) + (( datain[ ii + 2] ) & 0x00000007);
                  if( MVT_ZS_MODE )
									  current_feu_tmstmp |= (1<<15);
                  a_trigtime[0] = current_timestamp_high;
                  a_trigtime[1] = current_feu_tmstmp;
#ifdef DEBUG_SP_MVT
									if( mvt_fptr_err_2 != (FILE *)NULL )
									{
										fprintf(mvt_fptr_err_2,"%s: 2nd PASS 0xe118, current_timestamp_high = %08x current_feu_tmstmp = %08x\n",
											__FUNCTION__, current_event_number_low, current_feu_tmstmp);
										fflush(mvt_fptr_err_2);
									}
#endif
                  if( MVT_ZS_MODE != 2 ) // Treat No ZS and Tracket ZS cases
                  {
                    if( MVT_CMP_DATA_FMT == 0 ) // Unpacked data format
                    {
									    MVT_CCOPEN_NOSMPPACK(0xe11b,"c,i,l,N(s,Ns)",banknum);
									    Nchan[0] = nbchannelsFEU[bank][ibl][iev][0][i_feu];
#ifdef DEBUG_SP_MVT
								      if( mvt_fptr_err_2 != (FILE *)NULL )
								      {
									      fprintf(mvt_fptr_err_2,"%s: 2nd PASS 0xe118, nbchannelsFEU = %d\n",
										      __FUNCTION__, nbchannelsFEU[bank][ibl][iev][0][i_feu]);
									      fflush(mvt_fptr_err_2);
								      }
#endif
								      if( MVT_ZS_MODE )
								      {
									      for( i_channel = 0; i_channel < nbchannelsFEU[bank][ibl][iev][0][i_feu]; i_channel++ ) 
									      {
										      //OUTPUT CHANNEL NUMBER
										      ii = iFEU[bank][ibl][iev][0][i_feu] + 1 + 2 + i_channel;
										      current_channel_id = ((( datain[ ii ] ) & 0x01FF0000) >> 16 ) + 1;  
										      b16 = (unsigned short *)( b08 );
										      *b16++ = current_channel_id;
										      b08 += 2;
							
										      //OUTPUT  NUMBER OF SAMPLES			
										      b32 = (unsigned int *)( b08 );
										      * b32 ++ = MVT_NBR_SAMPLES_PER_EVENT;
										      b16+=2;
										      b08+=4;

										      for( i_sample = 0; i_sample < MVT_NBR_SAMPLES_PER_EVENT; i_sample++ )
										      {		
											      ii = iFEU[bank][ibl][iev][i_sample][i_feu] + 1 + 2 + i_channel;
											      current_sample = ((( datain[ ii ] ) & 0x00000FFF) );
											      *b16++ = current_sample;
											      b08 += 2;
										      }/* loop over samples */
									      } /* loop over channels */
								      } // if( MVT_ZS_MODE )
								      else // No ZS
								      {
		   							    for( i_channel=0; i_channel<nbchannelsFEU[bank][ibl][iev][0][i_feu]; i_channel++ ) 
									      {
										      //GET CURRENT DREAM ID
										      i_dream = i_channel >> 6 ;
										      // point to dream header for the current channel
										      ii = iFEU[bank][ibl][iev][0][i_feu] + 1 + 2 + 1 + (32 + 5)*i_dream ;
										      current_channel_id = ((( datain[ ii ] ) & 0x00000E00) >> 3 ) + (i_channel%64) + 1;
     
										      //OUTPUT CHANNEL NUMBER
										      b16 = (unsigned short *)( b08 );
										      *b16++ = current_channel_id;
										      b08 += 2;

										      //OUTPUT  NUMBER OF SAMPLES 							
										      b32 = (unsigned int *)( b08 );
										      *b32 ++ = MVT_NBR_SAMPLES_PER_EVENT;
										      b16+=2;
										      b08+=4;
#ifdef DEBUG_SP_MVT
										      if( mvt_fptr_err_2 != (FILE *)NULL )
										      {
											      fprintf
											      (
												      mvt_fptr_err_2,
												      "%s: 2nd PASS 0xe118, i_drm=%d i_chan=%d data[%d]=0x%08x cur_chan_id=%d SAMPLES_PER_EVENT=%d\n",
												      __FUNCTION__, i_dream, i_channel, ii, datain[ ii ], current_channel_id, MVT_NBR_SAMPLES_PER_EVENT
											      );
											      fflush(mvt_fptr_err_2);
										      }
#endif
										      for ( i_sample = 0; i_sample < MVT_NBR_SAMPLES_PER_EVENT; i_sample++)
										      {
											      // Point to the first channel of the Dream
											      // 1 for BEU extra word, 2 for FEU header + 2 for Dream header
											      ii = iFEU[bank][ibl][iev][i_sample][i_feu] + 1 + 2 + 2 + (32+5)*i_dream;
											      // jump to the required channel
											      ii += ((i_channel%64) >> 1);
											      if (i_channel%2)
											      {
												      current_sample = ((( datain[ ii ] ) & 0x00000FFF) );
											      }
											      else
											      {
												      current_sample = ((( datain[ ii ] ) & 0x0FFF0000)>> 16 );
											      }
											      *b16++ = current_sample;
											      b08 += 2;
										      } /* loop over samples */	
#ifdef DEBUG_SP_MVT
										      if( mvt_fptr_err_2 != (FILE *)NULL )
										      {
											      fprintf(mvt_fptr_err_2,"%s: 2nd PASS 0xe118, LOOP OVER SAMPLES DONE\n", __FUNCTION__);
											      fflush(mvt_fptr_err_2);
										      }
#endif
									      } //loop over channels
									    } //end if ZS MODE
                    }
                    else // of if( MVT_CMP_DATA_FMT == 0 ) -> packed data format
                    {
                      MVT_CCOPEN_PACKSMP(0xe128,"c,i,l,n(s,mc)",banknum);
                      // set number of channels
                      b16 = (unsigned short *)( b08 );
                      *b16++ = ((short)nbchannelsFEU[bank][ibl][iev][0][i_feu]);
                      b08 += 2;
#ifdef DEBUG6_MVT_2ND_PASS								
                      if( mvt_fptr_err_2 != (FILE *)NULL )
                      {
                        fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118, nbchannelsFEU = %d\n",
                          __FUNCTION__, nbchannelsFEU[bank][ibl][iev][0][i_feu]);
                        fflush(mvt_fptr_err_2);
                      }
#endif
                      if( MVT_ZS_MODE )
                      {
                        for( i_channel=0; i_channel<nbchannelsFEU[bank][ibl][iev][0][i_feu]; i_channel++ ) 
                        {
                          //OUTPUT CHANNEL NUMBER
                          ii = iFEU[bank][ibl][iev][0][i_feu] + 1 + 2 + i_channel;
                          current_channel_id = ((( datain[ ii ] ) & 0x01FF0000) >> 16 ) + 1;  
                          b16 = (unsigned short *)( b08 );
                          *b16++ = current_channel_id;
                          b08 += 2;
                          // set number of bytes in packed byte stream
                          c_number_of_bytes = (((MVT_NBR_SAMPLES_PER_EVENT - 1)/2)*3+1)+1;
                          if( (MVT_NBR_SAMPLES_PER_EVENT % 2) == 0 )
                              c_number_of_bytes++;
                          *b08 ++ = c_number_of_bytes;
#ifdef DEBUG6_MVT_2ND_PASS								
                          if( mvt_fptr_err_2 != (FILE *)NULL )
                          {
                              fprintf
                              (
                                  mvt_fptr_err_2,
                                  "%s: SECOND PASS 0xe128, MVT_ZS_MODE=1 current_channel_id =%d MVT_NBR_SAMPLES_PER_EVENT = %d c_number_of_bytes=%d\n",
                                      __FUNCTION__, current_channel_id, MVT_NBR_SAMPLES_PER_EVENT, c_number_of_bytes
                              );
                              fflush(mvt_fptr_err_2);
                          }
#endif
                          // Pack samples in unsigned integers
                          for( i_sample=0; i_sample<MVT_NBR_SAMPLES_PER_EVENT; i_sample++)
                          {		
                            ii = iFEU[bank][ibl][iev][i_sample][i_feu] + 1 + 2 + i_channel;
                            current_sample = ((( datain[ ii ] ) & 0x00000FFF) );
                            if( (i_sample % 2) == 0 )
                            {
                              *b08++ = ((char)( current_sample    &0xFF)); // 8 LSB-s
                              *b08   = ((char)((current_sample>>8)&0x0F)); // 4 MSB-s
                            }
                            else
                            {
                              *b08  |= ((char)((current_sample>>4)&0xF0)); // 4 MSB-s
                              b08++;
                              *b08++ = ((char)( current_sample    &0xFF)); // 8 LSB-s
                            }
                          } /* loop over samples */
                          if( MVT_NBR_SAMPLES_PER_EVENT % 2 )
                          {
                            *b08 &= 0x0F; // force high nibble to 0
                            b08++;
                          }
#ifdef DEBUG6_MVT_2ND_PASS								
                          if( mvt_fptr_err_2 != (FILE *)NULL )
                          {
                            fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118, LOOP OVER SAMPLES DONE \n", __FUNCTION__);
                            fflush( mvt_fptr_err_2);
                          }
#endif
                        } /* loop over channels */
                      }
                      else // of if( MVT_ZS_MODE )
                      {
                        for ( i_channel = 0; i_channel < nbchannelsFEU[bank][ibl][iev][0][i_feu] ; i_channel ++ ) 
                        {
                          //GET CURRENT DREAM ID
                          i_dream = i_channel >> 6 ;
                          // point to dream header for the current channel
                          ii = iFEU[bank][ibl][iev][0][i_feu] + 1 + 2 + 1 + (32 + 5)*i_dream ;
                          current_channel_id = ((( datain[ ii ] ) & 0x00000E00) >> 3 ) + (i_channel%64) + 1;

                          //OUTPUT CHANNEL NUMBER
                          b16 = (unsigned short *)( b08 );
                          *b16++ = current_channel_id;
                          b08 += 2;

                          // set number of bytes in packed byte stream
                          c_number_of_bytes = (((MVT_NBR_SAMPLES_PER_EVENT - 1)/2)*3+1)+1;
                          if( (MVT_NBR_SAMPLES_PER_EVENT % 2) == 0 )
                              c_number_of_bytes++;
                          *b08 ++ = c_number_of_bytes;
#ifdef DEBUG6_MVT_2ND_PASS								
                          if( mvt_fptr_err_2 != (FILE *)NULL )
                          {
                            fprintf
                            (
                                mvt_fptr_err_2,
                                "%s: SECOND PASS 0xe128, MVT_ZS_MODE=0 i_dream = %d i_channel = %d , data[%d]=0x%08x current_channel_id =%d MVT_NBR_SAMPLES_PER_EVENT = %d \n",
                                    __FUNCTION__, i_dream, i_channel, ii, datain[ ii ], current_channel_id, MVT_NBR_SAMPLES_PER_EVENT
                            );
                            fflush(mvt_fptr_err_2);
                          }
#endif
                          for ( i_sample = 0; i_sample < MVT_NBR_SAMPLES_PER_EVENT; i_sample++)
                          {
                            // Point to the first channel of the Dream
                            // 1 for BEU extra word, 2 for FEU header + 2 for Dream header
                            ii = iFEU[bank][ibl][iev][i_sample][i_feu] + 1 + 2 + 2 + (32+5)*i_dream;
                            // jump to the required channel
                            ii += ((i_channel%64) >> 1);
                            if (i_channel%2)
                            {
                                current_sample = ((( datain[ ii ] ) & 0x00000FFF) );
                            }
                            else
                            {
                                current_sample = ((( datain[ ii ] ) & 0x0FFF0000)>> 16 );
                            }
                            if( (i_sample % 2) == 0 )
                            {
                                *b08++ = ((char)( current_sample    &0xFF)); // 8 LSB-s
                                *b08   = ((char)((current_sample>>8)&0x0F)); // 4 MSB-s
                            }
                            else
                            {
                                *b08  |= ((char)((current_sample>>4)&0xF0)); // 4 MSB-s
                                 b08++;
                                *b08++ = ((char)( current_sample    &0xFF)); // 8 LSB-s
                            }
                          } /* loop over samples */
                          if( MVT_NBR_SAMPLES_PER_EVENT % 2 )
                          {
                            *b08 &= 0x0F; // force high nibble to 0
                            b08++;
                          }
#ifdef DEBUG6_MVT_2ND_PASS								
                          if( mvt_fptr_err_2 != (FILE *)NULL )
                          {
                            fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118, LOOP OVER SAMPLES DONE \n", __FUNCTION__);
                            fflush( mvt_fptr_err_2);
                          }
#endif
                        } // loop over channels
                      } // else of if( MVT_ZS_MODE )
                    } // else of if( MVT_CMP_DATA_FMT == 0 )
                  }
                  else // Treat TPC ZS mode
                  {
                    // packed data format only
                    MVT_CCOPEN_PACKSMP(0xe129,"c,i,l,n(s,m(c,mc))",banknum);
                    // set number of channels
                    b16 = (unsigned short *)( b08 );
                    *b16++ = ((short)feu_chn_cnt[iev][i_feu]);
                    b08 += 2;
#ifdef DEBUG_SP_MVT								
                    if( mvt_fptr_err_2 != (FILE *)NULL )
                    {
                      fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118/0xe129, nbchannelsFEU = %d\n",
                        __FUNCTION__, feu_chn_cnt[iev][i_feu]);
                      fflush(mvt_fptr_err_2);
                    }
#endif
                    /* loop over channels */
                    for( i_channel=0; i_channel<MAX_FEU_CHN; i_channel++ ) 
                    {
                      // Check that the channel has data 
                      if( feu_chn_ent_cnt[iev][i_feu][i_channel] > 0 )
                      {
                        // OUTPUT CHANNEL NUMBER
                        b16 =    (unsigned short *)( b08 );
                        *b16++ = (unsigned short  )(i_channel+1);
                        b08+= 2;

                        // Output number of pulses
                        *b08 ++ = feu_chn_ent_cnt[iev][i_feu][i_channel];

#ifdef DEBUG_SP_MVT								
                        if( mvt_fptr_err_2 != (FILE *)NULL )
                        {
                          fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118/0xe129, chan=%d npul=%d\n",
                            __FUNCTION__, i_channel, feu_chn_ent_cnt[iev][i_feu][i_channel]);
                          fflush(mvt_fptr_err_2);
                        }
#endif
                        // loop over pulses
                        pul_dat_ptr = &(feu_chn_ent_val[iev][i_feu][i_channel][0]);
                        for( i_pul=0; i_pul<feu_chn_ent_cnt[iev][i_feu][i_channel]; i_pul++ )
                        {
#ifdef DEBUG_SP_MVT								
                          if( mvt_fptr_err_2 != (FILE *)NULL )
                          {
                            fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118/0xe129, pul=%d fsmp=%d",
                              __FUNCTION__, i_pul, *pul_dat_ptr);
                            fflush(mvt_fptr_err_2);
                          }
#endif
                          // Output first sample of the pulse
                          *b08 ++ = (unsigned char)(*pul_dat_ptr++);
                          // set number of bytes in packed byte stream
                          // according to number of samples
                          nb_of_smp = (unsigned char)(*pul_dat_ptr++);
                          c_number_of_bytes = (((nb_of_smp - 1)/2)*3+1)+1;
                          if( (nb_of_smp % 2) == 0 )
                              c_number_of_bytes++;
#ifdef DEBUG_SP_MVT								
                          if( mvt_fptr_err_2 != (FILE *)NULL )
                          {
                            fprintf(mvt_fptr_err_2," nsmp=%d nbytes=%d\n",
                              nb_of_smp, c_number_of_bytes);
                            fflush(mvt_fptr_err_2);
                          }
#endif
                          *b08 ++ = c_number_of_bytes;
                          for( i_sample=0; i_sample<nb_of_smp; i_sample++)
                          {		
                            current_sample = *pul_dat_ptr++;
                            if( (i_sample % 2) == 0 )
                            {
                              *b08++ = ((char)( current_sample    &0xFF)); // 8 LSB-s
                              *b08   = ((char)((current_sample>>8)&0x0F)); // 4 MSB-s
                            }
                            else
                            {
                              *b08  |= ((char)((current_sample>>4)&0xF0)); // 4 MSB-s
                              b08++;
                              *b08++ = ((char)( current_sample    &0xFF)); // 8 LSB-s
                            }
                          } /* loop over samples */
                          if( nb_of_smp % 2 )
                          {
                            *b08 &= 0x0F; // force high nibble to 0
                            b08++;
                          }
#ifdef DEBUG_SP_MVT								
                          if( mvt_fptr_err_2 != (FILE *)NULL )
                          {
                            fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118/0xe129, LOOP OVER SAMPLES DONE \n", __FUNCTION__);
                            fflush( mvt_fptr_err_2);
                          }
#endif
                        } // loop over pulses 
                        // Clear channel for posterity
                        feu_chn_ent_cnt[iev][i_feu][i_channel]=0;
                        feu_chn_ent_ind[iev][i_feu][i_channel]=-1;
                        feu_chn_ent_val[iev][i_feu][i_channel][0]=0;
                        feu_chn_ent_val[iev][i_feu][i_channel][1]=0;
                      } // Check that the channel has data
                    } /* loop over channels */
                  } // End of TCP ZS mode
							  } // if( nbchannelsFEU[bank][ibl][iev][0][i_feu] > 0 ) do it if the feu has fired channels
                // Clear FEU channel count for posterity
                feu_chn_cnt[iev][i_feu] = 0;
						  } // for (i_feu = 0; i_feu < MVT_NBR_OF_FEU[currentBeuId]; i_feu++) loop over feus
					  } // for(ibl=0; ibl < MVT_NBR_OF_BEU; ibl++) loop over blocks */
						if(b08 != NULL)
						{
							CCCLOSE;
if( dataout_save0 != NULL )
CPCLOSE0;
#ifdef DEBUG_SP_MVT								
							if( mvt_fptr_err_2 != (FILE *)NULL )
							{
								fprintf(mvt_fptr_err_2,"%s: 2nd PASS 0xe118, CCCLOSE DONE \n", __FUNCTION__);
								fflush(mvt_fptr_err_2);
							}
#endif
						}
					}
					else // of if (mvt_error_counter ==0)
					{
//CPOPEN(0xeffe,1,banknum);
						printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
						printf("0xe118 : mvt error counter 0x%8x %d mvt error type 0x%8x \n", mvt_error_counter, mvt_error_counter, mvt_error_type );
						//b32 = (unsigned int *)( b08 );
						//*dataout ++ = mvt_error_counter;
						//b08 += 4;
						//*dataout ++ = mvt_error_type;
						//b08 += 4;
//CPCLOSE;
					}
				} // else of if ( (mvt_event_number % MVT_PRESCALE ) ||( MVT_PRESCALE == 1000000 ) )
			} // else if(banktag[bank] == 0xe118) /* MVT hardware format */
//***********************************************************************************
//***********************************************************************************
//***********************************************************************************
		} // for(bank=0; bank<nbanks; bank++) /* loop over evio banks *//* loop over banks  */

#ifdef PASS_AS_IS
		/* if last event, loop over banks to be passed 'as is' and attach them to that last event */
		if( iev==(nnE-1) )
		{
			for(ii=0; ii<nASIS; ii++)
			{
				bank = iASIS[ii]; /* bank number as it reported by BANKSCAN */
				datain = bankdata[bank];
				lenin  = banknw[bank];
				/*printf("mynev=%d: coping bank number %d (header %d 0x%08x))\n",mynev,bank,*(datain-2),*(datain-1));*/

				CPOPEN(banktag[bank],banktyp[bank],banknr[bank]);
				for(kk=0; kk<lenin; kk++)
				{
					dataout[kk] = datain[kk];
					b08 += 4;
				}
				CPCLOSE;
			}
		}
#endif

#ifdef SPLIT_BLOCKS
		/*
			at that point we want to close previout bank-of-banks and create header for new one;
			if iev==0, header already exist (came from ROL1); if iev==(nnE-1), do not create next header
		*/

		header[0] = lenev - 1;

		/*
			rol->dabufpi[1] comes from ROL1 and contains information for block of events; we'll replace it
			with correct info for every event using data from TI
		*/
		header[1] = rol->dabufpi[1];

		/* header created by CEOPEN macros (see rol.h) */

		/* event type obtained from TI board have to be recorded into fragment header - event builder need it */
		/*TEMP while(a_event_type>15) a_event_type --; TEMP*/
		header[1] = (header[1]&0xFF00FFFF) + (a_event_type<<16);

		/* time stamp obtained from TI board have to be recorded into fragment header - event builder need it */
		header[1] = (header[1]&0xFFFF00FF) + ((a_timestamp_l&0xFF)<<8);

		/* event number obtained from TI board have to be recorded into fragment header - event builder need it */
		header[1] = (header[1]&0xFFFFFF00) + (a_event_number_l&0xFF);

		/*
			if NOT the last event, clear syncflag; should do it only for blocks with sync event in the end,
			but do not bother chacking, will do it for all blocks
		*/
		if(iev<(nnE-1))
		{
			header[1] = header[1]&0x00FFFFFF;
		}

		/*
		printf("HEADER: sync_flag=%d event_type=%d bank_type=%d event_number=%d\n",
			(header[1]>>24)&0xFF,(header[1]>>16)&0xFF,(header[1]>>8)&0xFF,header[1]&0xFF);
		*/

		/* printf("(%d) header[0]=0x%08x (0x%08x)\n",iev,header[0],lenev - 1);*/
		/* printf("(%d) header[1]=0x%08x (0x%08x)\n",iev,header[1],rol->dabufpi[1]);*/

		/* if NOT the last event, create header for the next event */
		if( iev<(nnE-1) )
		{
			/*printf("bump header pointer, iev=%d\n",iev);*/
			header = dataout;
			dataout += 2;
			lenout  += 2;
		}
#endif // #ifdef SPLIT_BLOCKS

		mvt_event_number ++;
	} // for(iev=0; iev<nnE; iev++) /* loop over events */

	/* returns full fragment length (long words) */  
#ifdef DEBUG 
	printf("return lenout=%d\n**********************\n\n",lenout);
#endif

/*
	if(lenout>2)
		printf("return lenout=%d\n**********************\n\n",lenout);
*/

#ifdef SPLIT_BLOCKS
#else
	CPEXIT;
#endif

	return(lenout);
}

/*
 * Usage function
 */
void usage( char *name )
{
	printf( "\nUsage: %s", name );
	printf( " [-c Conf_FileNQame]" );
	printf( " [-o output_type]" );
	printf( " [-n num_event]" );
	printf( " [-S Sys_Name]" );
	printf( " [-R Roc_Name]" );
	printf( " [-s]" );
	printf( " [-v [-v]]" );
	printf( " [-h]" );
	printf( "\n" );

	printf( "-c Conf_FileName        - name for config file; default: %s; \"None\" no file consulted\n", DEF_ConfFileName );
	printf( "-o output_type          - None, Raw, Cmp; default: %s\n", DEF_OutType );
	printf( "-n num_even             - Number of events to acquire: default 0 - infinite\n" );
	printf( "-R Roc_Name             - mvt1, mvt2, mvt3, mmft1, svt3, sedipcq156; default: %s; \n", DEF_RocName );
	printf( "-s                      - Step-by-step execution; default - go through\n" );
	printf( "-v [-v]                 - Forces debug output\n" );
	printf( "-h                      - help\n" );

	printf( "  Get this text:\n"); 
	printf( "   %s -h\n\n", name);
}

/*
 * Cleanup function
 */
void cleanup(int param)
{
	if( param )
	{
		fprintf(stderr, "cleanup: Entering with %d\n", param);
		vmeBusLock();
      mvtStatus(1);
			tiStatus(1);
			sdStatus(1);
		vmeBusUnlock();
	}
	if( verbose )
		printf( "cleanup: Entering with %d\n", param );

	// Just in case
	DEF_SET_STDIN_BLOCKING;

	// Close mamory configuration file if any open
	mvtCleanup();

	// Free big desantanglement buffer if any
	if( out_dabufp != (unsigned int *)NULL )
	{
		free(out_dabufp);
		out_dabufp = (unsigned int *)NULL;
	}

	// Close data file
	if( dat_fptr != (FILE *)NULL )
	{
		fflush( dat_fptr );
		fclose( dat_fptr );
		dat_fptr = (FILE *)NULL;
	}

	// Close run file
	if( run_fptr != (FILE *)NULL )
	{
		fclose( run_fptr );
		run_fptr = (FILE *)NULL;
	}

	// Close log file
	if( log_fptr != (FILE *)NULL )
	{
		fflush( log_fptr );
		fclose( log_fptr );
		log_fptr = (FILE *)NULL;
	}
	// Close rol2 log file
	if( mvt_fptr_err_2 != (FILE *)NULL )
	{
		fflush( mvt_fptr_err_2 );
		fclose( mvt_fptr_err_2 );
		mvt_fptr_err_2 = (FILE *)NULL;
	}
	if( verbose )
		printf( "cleanup: Exiting\n" );
	exit( param );
}

/*
 * Signal hendler
 */
void sig_hndlr( int param )
{
	cleanup(param);
}

/*
 * RoL declarations
 */
//extern void __download();

/*
 * Main
 */
int main( int argc, char* *argv )
{
	// Internal variables
	int  opt;
	char optformat[128];
	char progname[128];

	// Parameters
	char conf_file_name[128];
	char sys_name[16];
	char roc_name[32];
	char out_type[16];
	int step_by_step;
	int max_num_events;

	char dat_file_name[128];
	int ret;
	int roc_id;
	int run_num;
	int nmvt;
	int mvt_slot;
	int id;
	int do_post_reads;
	int block_num;
	int block_size;
	int total_size;
	int do_out;
	int ibeu;
	int cmb_max_num_events;
	int max_num_bloks;

	int cmp_monit_size;

	// run number variables
	char run_num_file_name[128];
	struct stat run_num_file_stat;
#define DEF_LINE_SIZE 512
	char line[DEF_LINE_SIZE];
	char *env_home;
	char tmp_dir[512];
	char dat_dir[512];
	char mkcmd[512];
	struct  stat log_stat;   

	// rol1 readout variables
	int run;
	char cc;
	int len;
	int mvtgbr;
	int tireadme;
	int event_type;

	// rol1 global variables
	int mvt_to_cntr;
	struct timeval mvt_to;
	struct timeval mvt_max_wait;
	int mvt_max_to_iter;
	FILE *mvt_fptr_err_1;
	unsigned int mvtSlotMask = 0; /* bit=slot (starting from 0) */

	int mvt_to_iter;
	int syncFlag;

	// Software timeout for the MVT readout
	struct timeval mvt_t0;
	struct timeval mvt_t1;
	struct timeval mvt_dt;

	// stat for ti
	// Software timeout for the TI readout
	struct timeval ti_to;
	struct timeval ti_max_wait;
	int ti_max_to_iter;
	int ti_to_cntr;

	// rol1 local variables
	int block_level;
	// VME variables
	int i1, i2, i3;

	// rol1 end variables
	int iwait;
	int blocksLeft;

	// rol2 local variables
	int i_evt;
	int i_feu;
	int i_chan;

	// Log file variables
	char logfilename[128];
	char log_message[256];

	// Timing parameters
	struct timeval post_read_t0;
	struct timeval post_read_t1;
	struct timeval post_read_dt;
	struct timeval post_read_to;
	time_t      cur_time;
	struct tm  *time_struct;

	// Timing parameters
	struct timeval monit_t0;
	struct timeval monit_t1;
	struct timeval monit_dt;
	struct timeval monit_to;
	int monit_block_num;
	int monit_block_size;
	int tideltatime;

int bindex;
unsigned int *bptr;

	// Initialization
	verbose        = 0;
	sprintf(conf_file_name, DEF_ConfFileName );
	sprintf(roc_name,       DEF_RocName      );
	sprintf(out_type,       DEF_OutType      );
	step_by_step   = 0;
	max_num_events = 0;

	sprintf(progname, "%s", basename(argv[0]));
	run_num         = 0;
	mvt_fptr_err_1 = (FILE *)NULL;
	mvt_fptr_err_2 = (FILE *)NULL;

	/******************************/
	/* Check for input parameters */
	/******************************/
	sprintf( optformat, "c:S:o:n:R:svh" );
	while( ( opt = getopt( argc, argv, optformat ) ) != -1 )
	{
		switch( opt )
		{
			case 'c':
				sprintf( conf_file_name, "%s", optarg );
			break;

			case 'S':
				sprintf( sys_name, "%s", optarg );
			break;

			case 'o':
				sprintf( out_type, "%s", optarg );
			break;

			case 'R':
				sprintf( roc_name, "%s", optarg );
			break;

			case 'n':
				max_num_events = atoi( optarg );
			break;

			case 's':
				step_by_step = 1;
			break;

			case 'v':
				verbose++;
			break;

			case 'h':
			default:
				usage( progname );
				return 0;
		}
	}
	if( verbose )
	{
		printf( "conf_file_name          = %s\n",    conf_file_name );
		printf( "roc_name                = %s\n",    roc_name );
		printf( "out_type                = %s\n",    out_type );
		printf( "max_num_events          = %d\n",    max_num_events );
		printf( "step_by_step            = %d\n",    step_by_step );
		printf( "verbose                 = %d\n",    verbose );
	}

	/***********************/
	/* Set signal hendler  */
	/***********************/
	signal( SIGABRT, sig_hndlr);
	signal( SIGFPE,  sig_hndlr);
	signal( SIGILL,  sig_hndlr);
	signal( SIGINT,  sig_hndlr);
	signal( SIGSEGV, sig_hndlr);
	signal( SIGTERM, sig_hndlr);
	if( verbose )
		fprintf( stdout, "%s: signal handler set\n", progname );

	// Find the roc_id out of system name
	if( (roc_id = mvtRocName2RocId( roc_name )) == 0 )
	{
		fprintf( stderr, "%s: unknown roc_name %s\n", progname, roc_name );
		cleanup(0);
	}
	// Find the sys_name out of roc name
	sprintf( sys_name, "%s", mvtRocId2SysName( roc_id ));


	// Determine tmp and dat directories
	if( (env_home = getenv( "HOME" )) == (char *)NULL )
	{
		fprintf(stderr, "%s: Unable to get HOME variable; log file in . directory\n", progname );
		sprintf( tmp_dir, "./" );
		sprintf( dat_dir, "./" );
	}
	else
	{
		//Check that tmp directory exists and if not create it
		sprintf( tmp_dir, "%s/mvt/tmp", env_home );
		if( stat( tmp_dir, &run_num_file_stat ) )
		{
			// create directory
			sprintf( mkcmd, "mkdir -p %s", tmp_dir );
			ret = system( mkcmd );
			if( (ret<0) || (ret==127) )
			{
				fprintf(stderr, "%s: failed to create dir %s with %d\n", progname, tmp_dir, ret );
				cleanup(0);
			}
		}
		else if( !(S_ISDIR(run_num_file_stat.st_mode)) )
		{
			fprintf(stderr, "%s: %s file exists but is not directory\n", progname, tmp_dir );
			cleanup(0);
		}

		//Check that dat directory exists and if not create it
		sprintf( dat_dir, "%s/mvt/dat", env_home );
		if( stat( dat_dir, &run_num_file_stat ) )
		{
			// create directory
			sprintf( mkcmd, "mkdir -p %s", dat_dir );
			ret = system( mkcmd );
			if( (ret<0) || (ret==127) )
			{
				fprintf(stderr, "%s: failed to create dir %s with %d\n", progname, dat_dir, ret );
				cleanup(0);
			}
		}
		else if( !(S_ISDIR(run_num_file_stat.st_mode)) )
		{
			fprintf(stderr, "%s: %s file exists but is not directory\n", progname, dat_dir );
			cleanup(0);
		}
	}

	// Get or create run number file
	sprintf( run_num_file_name, "%s/%s_%d_run_id.txt", tmp_dir, sys_name, roc_id );
	if( stat( run_num_file_name, &run_num_file_stat ) == 0 )
	{
		// file exists, try to get run number
		if( (run_fptr = fopen( run_num_file_name, "r" )) == (FILE *)NULL )
		{
			fprintf(stderr, "%s: fopen failed to open run num file %s in read mode with %d %s\n", progname, run_num_file_name, errno, strerror( errno ));
			cleanup(0);
		}
		if( fgets( line, DEF_LINE_SIZE, run_fptr ) == NULL )
		{
			fprintf(stderr, "%s: Unable to get run number from %s; unexpected EOF at firs line\n", progname, run_num_file_name );
			cleanup(0);
		}
		// Check for comment line and if needed skip it
		if( line[0] == '#' )
		{
			if( fgets( line, DEF_LINE_SIZE, run_fptr ) == NULL )
			{
				fprintf(stderr, "%s: Unable to get run number from %s; unexpected EOF after comment\n", progname, run_num_file_name );
				cleanup(0);
			}
		}
		sscanf( line, "run_num = %d", &run_num );
		fclose( run_fptr );
		run_fptr = (FILE *)NULL;
	}
	
	// Increase run number and save it in file
	run_num++;
	if( (run_fptr = fopen( run_num_file_name, "w" )) == (FILE *)NULL )
	{
		fprintf(stderr, "%s: fopen failed to open run num file %s in write mode with %d %s\n", progname, run_num_file_name, errno, strerror( errno ));
		cleanup(0);
	}
	else
	{
		// Get current time
		cur_time = time(NULL);
		time_struct = localtime(&cur_time);
		fprintf( run_fptr, "# Created at %02d%02d%02d %02dH%02d\n",
			time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday, time_struct->tm_hour, time_struct->tm_min );
		fprintf( run_fptr, "run_num = %d\n", run_num );
		fclose( run_fptr );
		run_fptr = (FILE *)NULL;
	}
	
	if( step_by_step )
	{
		printf("%s INFO: Press Q to quit or <CR> to go to Download section\n", progname );
		cc= getchar();
		if( cc == 'Q' )
			cleanup(0);
	}

  // Set verbosity level
  mvtSetVerbosity( verbose );

	/***********************/
	/* Download section    */
	/***********************/
	// Start by managing the log file
	if( (ret = mvtManageLogFile( &log_fptr, roc_id, 1, "StdApp" ) ) < 0 )
	{
		fprintf( stderr, "%s: Douwnload phase: mvtManageLogFile failed to open log file 1 for %s roc_id %d\n", progname, sys_name, roc_id );
		cleanup(0);
	}
	mvt_fptr_err_1 = log_fptr;

	// Do Config here
	fprintf( stdout, "\n%s: %s download start with confFile=%s, run_num=%d, roc_id=%d\n\n", progname, sys_name, conf_file_name, run_num, roc_id );
//	vmeBusLock(); standalone - VME will be setup in mvtConfig
		nmvt = mvtConfig( conf_file_name, run_num, roc_id );
//	vmeBusUnlock(); standalone - vmeBusLock was not called

	if( ( nmvt <= 0 ) || (3 <= nmvt) )
	{
		sprintf( log_message, "%s: wrong number of BEUs %d in %s crate %d; must be in [1;3] range",
			__FUNCTION__, nmvt, sys_name, roc_id );
		fprintf(stderr, "%s\n", log_message );
		if( mvt_fptr_err_1 != (FILE *)NULL )
		{
			fprintf(mvt_fptr_err_1, "%s\n", log_message );
			fflush( mvt_fptr_err_1 );
		}
	  if( step_by_step )
	  {
		  printf("%s INFO: Press Q to quit or <CR> to go to Prestart section\n", progname );
		  cc= getchar();
		  if( cc == 'Q' )
			  cleanup(0);
	  }
		cleanup(0);
	}
	else
	{
		sprintf( log_message, "%s: found number of BEUs %d in %s crate %d",
			__FUNCTION__, nmvt, sys_name, roc_id );
		fprintf( stdout, "%s\n", log_message );
	}

	// Set SD active slots
	mvtSlotMask=0;
	for(id=0; id<nmvt; id++)
	{
		mvt_slot = mvtSlot(id); 
		mvtSlotMask |= (1<<mvt_slot);
		fprintf( stdout, "%s: =======================> %s SlotMask=0x%08x\n", progname, sys_name, mvtSlotMask);
	}
	vmeBusLock();
		sdSetActiveVmeSlots(mvtSlotMask);
		sdStatus(1);
	vmeBusUnlock();

	/*************************************/
	/* redefine TI settings if neseccary */
	tiSetUserSyncResetReceive(1);

	// Get big memory
	usrVmeDmaInit();
	usrVmeDmaMemory( &i1, &i2, &i3 );
	fprintf( stdout, "%s: phyBase = 0x%08x usrBase=0x%08x size=%d %dk\n", progname, i1, i2, i3, i3/1024);
	// set up dma memory as it is done in ti primamry
	i2_from_rol1 = i2;
    	fprintf( stdout, "%s: tiprimarytinit: i2_from_rol1 = 0x%08x\n", progname, i2_from_rol1);
    	i2_from_rol1 = (i2_from_rol1 & 0xFFFFFFF0);
    	fprintf( stdout, "%s: tiprimarytinit: i2_from_rol1 = 0x%08x\n", progname, i2_from_rol1);
    	i2_from_rol1 = i2_from_rol1 + 0x10;
    	fprintf( stdout, "%s: tiprimarytinit: i2_from_rol1 = 0x%08x\n", progname, i2_from_rol1);
	dma_dabufp = (unsigned int *)i2_from_rol1;
    	fprintf( stdout, "%s: dma_dabufp = 0x%08x\n", progname, dma_dabufp);

	// VME DMA setup
	usrVmeDmaSetConfig(2,5,1); /*A32,2eSST,267MB/s*/

	// Determine event type for future use
	if( sys_params_ptr->Ti_Params[1].TrgSrc == TiTrgSrc_IntCst )
		event_type = ET_CST;
	else if( sys_params_ptr->Ti_Params[1].TrgSrc == TiTrgSrc_IntRnd )
		event_type = ET_RND;
	else
		event_type = ET_PHY;

	// Determine output file type and actions
	if( strcmp( out_type, "Raw" ) == 0 )
		do_out = 1;
	else if( strcmp( out_type, "Cmp" ) == 0 )
		do_out = 2;
	else
		do_out = 0;

	// Only in case of composite output
	if( do_out == 2 )
	{
    // Start by managing the log file it could have been closed during the "end" phase
    if( (ret = mvtManageLogFile( &mvt_fptr_err_2, roc_id, 2, "StdApp" ) ) < 0 )
    {
	    fprintf( stderr, "%s: Download phase: mvtManageLogFile failed to open log file 2 for %s roc_id %d\n", progname, sys_name, roc_id );
	    cleanup(0);
    }
/*
		if( mvt_fptr_err_2 == (FILE *)NULL )
		{
		  //Check that tmp directory exists and if not create it
	    tmp_dir[0] = '\0';
      // First try to find official log directory
	    if( (env_home = getenv( "CLON_LOG" )) )
	    {
		    //Check that mvt/tmp directory exists and if not create it
		    sprintf( tmp_dir, "%s/mvt/tmp/", env_home );
	      fprintf( stdout, "%s: attemmpt to work with log dir %s\n", progname, tmp_dir );
		    if( stat( tmp_dir, &log_stat ) )
		    {
			    // create directory
			    sprintf( mkcmd, "mkdir -p %s", tmp_dir );
			    ret = system( mkcmd );
			    fprintf(stderr, "%s: system call returned with %d\n", progname, ret );
			    if( (ret<0) || (ret==127) || (ret==256) )
			    {
				    fprintf(stderr, "%s: failed to create dir %s with %d\n", progname, tmp_dir, ret );
            tmp_dir[0] = '\0';
			    }
		    }
		    else if( !(S_ISDIR(log_stat.st_mode)) )
		    {
			    fprintf(stderr, "%s: %s file exists but is not directory\n", progname, tmp_dir );
          tmp_dir[0] = '\0';
		    }
      }

      // If official log directory does not exists
      if( tmp_dir[0] == '\0' )
      {
        fprintf( stdout, "%s: failed with official log, attempt with Home\n", progname );
        // Attemept with HOME
		    if( (env_home = getenv( "HOME" )) )
			  //Check that mvt/tmp directory exists and if not create it
			  sprintf( tmp_dir, "%s/mvt/tmp/", env_home );
			  if( stat( tmp_dir, &log_stat ) )
			  {
				  // create directory
				  sprintf( mkcmd, "mkdir -p %s", tmp_dir );
				  ret = system( mkcmd );
				  fprintf(stderr, "%s: system call returned with %d\n", progname, ret );
				  if( (ret<0) || (ret==127) || (ret==256) )
				  {
					  fprintf(stderr, "%s: failed to create dir %s with %d\n", progname, tmp_dir, ret );
					  tmp_dir[0] = '\0';
				  }
			  }
			  else if( !(S_ISDIR(log_stat.st_mode)) )
			  {
				  fprintf(stderr, "%s: %s file exists but is not directory\n", progname, tmp_dir );
				  tmp_dir[0] = '\0';
			  }
		  }

      // If all of the above failed
      if( tmp_dir[0] == '\0' )
      {
        fprintf(stderr, "%s: CLON_LOG and HOME failed; log file in . directory\n", progname );
        sprintf( tmp_dir, "./" );
        //Check that tmp directory exists and if not create it
        sprintf( tmp_dir, "%s/mvt/tmp/", env_home );
        if( stat( tmp_dir, &log_stat ) )
        {
          // create directory
          sprintf( mkcmd, "mkdir -p %s", tmp_dir );
          ret = system( mkcmd );
				  fprintf(stderr, "%s: system call returned with %d\n", progname, ret );
				  if( (ret<0) || (ret==127) || (ret==256) )
          {
            fprintf(stderr, "%s: failed to create dir %s with %d\n", progname, tmp_dir, ret );
				    cleanup(0);
          }
        }
        else if( !(S_ISDIR(log_stat.st_mode)) )
        {
          fprintf(stderr, "%s: %s file exists but is not directory\n", progname, tmp_dir );
				  cleanup(0);
        }
   	  }

      // create the file
			sprintf(logfilename, "%s/mvt_roc_%d_rol_2.log", tmp_dir, roc_id);
			if( (mvt_fptr_err_2 = fopen(logfilename, "w")) == (FILE *)NULL )
			{
				fprintf(stderr, "%s: fopen failed to open log file %s in write mode with %d %s\n", __FUNCTION__, logfilename, errno, strerror( errno ));
				cleanup(0);
			}
		}
		// Get current time
		cur_time = time(NULL);
		time_struct = localtime(&cur_time);
		fprintf( mvt_fptr_err_2, "**************************************************\n" );
		fprintf( mvt_fptr_err_2, "%s at %02d%02d%02d %02dH%02d\n", __FUNCTION__,
			time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday, time_struct->tm_hour, time_struct->tm_min );
		fflush( mvt_fptr_err_2 );
*/
	}
	fprintf(stdout, "%s INFO: Download Executed\n", progname ); fflush(stdout);

	if( step_by_step )
	{
		printf("%s INFO: Press Q to quit or <CR> to go to Prestart section\n", progname );
		cc= getchar();
		if( cc == 'Q' )
			cleanup(0);
	}

	// Get current time
	cur_time = time(NULL);
	time_struct = localtime(&cur_time);
	// Open output file if requested
	if( do_out == 1 )
	{
		sprintf
		(
			dat_file_name,
			"%s/%s_roc_%d_raw_%06d.evio",
			dat_dir,
			sys_name, roc_id,
			run_num
		);
	}
	else if( do_out == 2 )
	{
		sprintf
		(
			dat_file_name,
			"%s/%s_roc_%d_cmp_%06d.evio",
			dat_dir,
			sys_name, roc_id,
			run_num
		);
	}
	if( do_out )
	{
		// Open file
		if( (dat_fptr = fopen(dat_file_name, "wb")) == (FILE *)NULL )
		{
			fprintf(stderr, "%s: fopen failed to open data file %s in wb mode with %d %s\n", __FUNCTION__, dat_file_name, errno, strerror( errno ));
			cleanup(0);
		}
	}

	// Only in case of composite output
	if( do_out == 2 )
	{
		// Allocate output buffer for disentanglement
		if( (out_dabufp = (unsigned int *)calloc(DEF_OUT_BUF_SIZE, sizeof(int))) == (unsigned int *)NULL )
		{
			fprintf( stderr, "%s: calloc failed for %d words\n", __FUNCTION__, DEF_OUT_BUF_SIZE );
			cleanup(-5);
		}
	}

	/***********************/
	/* Prestart section    */
	/***********************/
	// Start by managing the log file it could have been closed during the "end" phase
	if( (ret = mvtManageLogFile( &log_fptr, roc_id, 1, "StdApp" ) ) < 0 )
	{
		fprintf( stderr, "%s: Prestart phase: mvtManageLogFile failed to open log file 1 for %s roc_id %d\n", progname, sys_name, roc_id );
		cleanup(0);
	}
	mvt_fptr_err_1 = log_fptr;

	tiEnableVXSSignals();

	// If the TI is not slave
	if( sys_params_ptr->RunMode == Standalone )
	{
		vmeBusLock();
			tiSetBusySource(TI_BUSY_LOOPBACK,0);
		vmeBusUnlock();
	}

	vmeBusLock();
		tiIntDisable();
	vmeBusUnlock();

	// If the TI is not slave
	if( sys_params_ptr->RunMode == Standalone )
	{
		sleep(1);
		vmeBusLock();
			tiSyncReset(1);
		vmeBusUnlock();
		sleep(1);
		vmeBusLock();
			tiSyncReset(1);
		vmeBusUnlock();
		sleep(1);

		vmeBusLock();
			ret = tiGetSyncResetRequest();
		vmeBusUnlock();
		if(ret)
		{
			if( mvt_fptr_err_1 != (FILE *)NULL )
			{
				fprintf(mvt_fptr_err_1, "%s; Prestart phase: syncrequest still ON after tiSyncReset()\n", progname );
				fflush( mvt_fptr_err_1 );
			}
		}
	}

	// Normally we can't get here if nmvt is not in range
	// But the code is kept to be as close to rol1 as possible
	if(nmvt>0)
	{
		vmeBusLock();
			ret = mvtPrestart();
		vmeBusUnlock();
		if(ret<=0)
		{
			sprintf( log_message, "%s: mvtPrestart failed with %d in %s crate %d",
				__FUNCTION__, ret, mvtRocId2SysName( roc_id ), roc_id );
			fprintf(stderr, "%s\n", log_message );
			if( mvt_fptr_err_1 != (FILE *)NULL )
			{
				fprintf(mvt_fptr_err_1, "%s\n", log_message );
				fflush( mvt_fptr_err_1 );
			}
	    if( step_by_step )
	    {
		    printf("%s INFO: Press Q to quit or <CR> to go to Prestart section\n", progname );
		    cc= getchar();
		    if( cc == 'Q' )
			    cleanup(ret);
	    }
			cleanup(ret);
		}
		else
		{
			sprintf( log_message, "%s: found number of FEUs %d in %s crate %d with %d BEUs",
				__FUNCTION__, ret, mvtRocId2SysName( roc_id ), roc_id, nmvt );
			fprintf( stdout, "%s\n", log_message );
		}
		block_level = tiGetCurrentBlockLevel();
    if( (block_level < 0) || (block_level > MAXEVENT) )
    {
			sprintf( log_message, "%s: mvtPrestart in %s crate %d: unsupported TI block_level=%d; must be in [1;%d] range",
				__FUNCTION__, mvtRocId2SysName( roc_id ), roc_id, block_level, MAXEVENT );
			fprintf(stderr, "%s\n", log_message );
			if( mvt_fptr_err_1 != (FILE *)NULL )
			{
				fprintf(mvt_fptr_err_1, "%s\n", log_message );
				fflush( mvt_fptr_err_1 );
			}
			cleanup(ret);
    }
		mvtSetCurrentBlockLevel( block_level );
		if( mvt_fptr_err_1 != (FILE *)NULL )
		{
			fprintf(mvt_fptr_err_1, "%s: prestart %s block level set to %d\n", progname, sys_name, block_level);
			fflush( mvt_fptr_err_1 );
		}
		mvt_to_cntr = 0;
		mvt_to.tv_sec  = 0;
		mvt_to.tv_usec = 90000;

		mvt_max_wait.tv_sec  = 0;
		mvt_max_wait.tv_usec = 0;
		mvt_max_to_iter      = 0;

		ti_to.tv_sec  = 10;
		ti_to.tv_usec = 0;
		ti_to_cntr = 0;
		ti_max_wait.tv_sec  = 0;
		ti_max_wait.tv_usec = 0;
		ti_max_to_iter      = 0;

		post_read_to.tv_sec  = 3;
		post_read_to.tv_usec = 0;

		monit_to.tv_sec  = 10;
		monit_to.tv_usec = 0;
	}

	// If the TI is not slave
	if( sys_params_ptr->RunMode == Standalone )
	{
		sleep(1);
		vmeBusLock();
			tiSyncReset(1);
		vmeBusUnlock();
		sleep(1);
		vmeBusLock();
			tiSyncReset(1);
		vmeBusUnlock();
		sleep(1);

		/* USER RESET - use it because 'SYNC RESET' produces too short pulse, still need 'SYNC RESET' above because 'USER RESET'
		does not do everything 'SYNC RESET' does (in paticular does not reset event number) */
		vmeBusLock();
			tiUserSyncReset(1,1);
			tiUserSyncReset(0,1);
		vmeBusUnlock();

		vmeBusLock();
			ret = tiGetSyncResetRequest();
		vmeBusUnlock();
		if(ret)
		{
			if( mvt_fptr_err_1 != (FILE *)NULL )
			{
				fprintf(mvt_fptr_err_1, "%s; Prestart phase: syncrequest still ON after tiSyncReset()\n", progname );
				fflush( mvt_fptr_err_1 );
			}
		}
	}

	vmeBusLock();
		tiStatus(1);
		sdStatus(1);
	vmeBusUnlock();
	fprintf(stdout, "%s INFO: Prestart Executed\n", progname ); fflush(stdout);

	if( sys_params_ptr->RunMode == Standalone )
		{ fprintf(stdout, "%s INFO: Prestart as TI_MASRER\n", progname );fflush(stdout); }
	else if( sys_params_ptr->RunMode == Clas12 )
		{ fprintf(stdout, "%s INFO: Prestart as TI_SLAVE\n",  progname );fflush(stdout); }
	else if( sys_params_ptr->RunMode == Spy )
		{ fprintf(stdout, "%s INFO: Prestart as TI_SPY\n",    progname );fflush(stdout); }
	else
		{ fprintf(stdout, "%s INFO: Prestart as TI_NONE\n",   progname );fflush(stdout); }

	// Only in case of composite data output
	if( do_out == 2 )
	{
    // Start by managing the log file it could have been closed during the "end" phase
    if( (ret = mvtManageLogFile( &mvt_fptr_err_2, roc_id, 2, "StdApp" ) ) < 0 )
    {
	    fprintf( stderr, "%s: Prestart phase: mvtManageLogFile failed to open log file 2 for %s roc_id %d\n", progname, sys_name, roc_id );
	    cleanup(0);
    }
/*
		if( mvt_fptr_err_2 == (FILE *)NULL )
		{
			sprintf(logfilename, "%s/mvt_roc_%d_rol_2.log", tmp_dir, roc_id);
			if( (mvt_fptr_err_2 = fopen(logfilename, "w")) == (FILE *)NULL )
			{
				fprintf(stderr, "%s: fopen failed to open log file %s in write mode with %d %s\n", __FUNCTION__, logfilename, errno, strerror( errno ));
				vmeBusLock();
					mvtEnd();
				vmeBusUnlock();
				cleanup(0);
			}
			fprintf( mvt_fptr_err_2,"%s : Opend at prestart\n", __FUNCTION__ );
			fflush(  mvt_fptr_err_2 );
		}
*/
		if( (MVT_ZS_MODE = mvtGetZSMode(roc_id)) < 0 )
		{
			printf("ERROR: MVT ZS mode negative\n");
			if( mvt_fptr_err_2 != (FILE *)NULL )
			{
				fprintf(mvt_fptr_err_2,"%s: ERROR MVT_ZS_MODE=%d\n", __FUNCTION__,  MVT_ZS_MODE  );
				fflush(mvt_fptr_err_2);
			}
		}
		MVT_CMP_DATA_FMT          = mvtGetCmpDataFmt();
		MVT_PRESCALE              = mvtGetPrescale(roc_id);
		MVT_NBR_OF_BEU            = mvtGetNbrOfBeu(roc_id);
		MVT_NBR_EVENTS_PER_BLOCK  = mvtGetNbrOfEventsPerBlock(roc_id);
		MVT_NBR_SAMPLES_PER_EVENT = mvtGetNbrOfSamplesPerEvent(roc_id);
		for (ibeu = 0; ibeu <DEF_MAX_NB_OF_BEU; ibeu++)
		{
			MVT_NBR_OF_FEU[ibeu] = mvtGetNbrOfFeu(roc_id, ibeu+1);
		}

		printf("INFO: MVT_ZS_MODE %d\n",                MVT_ZS_MODE              );
		printf("INFO: MVT_CMP_DATA_FMT %d\n",           MVT_CMP_DATA_FMT         );
		printf("INFO: MVT_PRESCALE %d\n",               MVT_PRESCALE             );
		printf("INFO: MVT_NBR_OF_BEU %d\n",             MVT_NBR_OF_BEU           );
		printf("INFO: MVT_NBR_EVENTS_PER_BLOCK %d\n",   MVT_NBR_EVENTS_PER_BLOCK );
		printf("INFO: MVT_NBR_SAMPLES_PER_EVENT %d\n",  MVT_NBR_SAMPLES_PER_EVENT);
		for (ibeu = 0; ibeu<DEF_MAX_NB_OF_BEU; ibeu++)
		{
			printf("INFO: MVT_NBR_OF_FEU %d %d\n", ibeu, MVT_NBR_OF_FEU[ibeu] );
		}

		rol2_report_raw_data |= mvtGetRepRawData();
		fprintf(stdout, "%s: ROL2: rol2_report_raw_data set to %d\n", progname, rol2_report_raw_data );

    // Initialize data rearangement structures
    for( i_evt=0; i_evt<MAX_FEU_EVT; i_evt++ )
    {
      feu_act_msk[i_evt]=0;
      for( i_feu=0; i_feu<MAX_FEU_NUM; i_feu++ )
      {
        feu_chn_cnt[i_evt][i_feu]=0;
        for( i_chan=0; i_chan<MAX_FEU_CHN; i_chan++ )
        {
          feu_chn_ent_ind[i_evt][i_feu][i_chan]=-1;
          feu_chn_ent_cnt[i_evt][i_feu][i_chan]=0;
          feu_chn_ent_val[i_evt][i_feu][i_chan][0]=0x0;
          feu_chn_ent_val[i_evt][i_feu][i_chan][1]=0x0;
          feu_chn_ent_val[i_evt][i_feu][i_chan][2]=0x0;
        } // for( i_chan=0; i_chan<MAX_FEU_CHN; i_chan++ )
      } // for( i_feu=0; i_feu<MAX_FEU_NUM; i_feu++ )
    } // for( i_evt=0; i_evt<MAX_FEU_EVT; i_evt++ )
	} // if( do_out == 2 )
	// Also needed for Raw data prescale
	if( do_out == 1 || do_out == 2 )
	{
		MVT_PRESCALE = mvtGetPrescale(roc_id);
		fprintf(stdout,"%s: MVT_PRESCALE=%d\n", __FUNCTION__,  MVT_PRESCALE  );
		if( mvt_fptr_err_2 != (FILE *)NULL )
		{
			fprintf(mvt_fptr_err_2,"%s: MVT_PRESCALE=%d\n", __FUNCTION__,  MVT_PRESCALE  );
			fflush(mvt_fptr_err_2);
		}
	}

	if( step_by_step )
	{
		printf("%s INFO: Press Q to quit or <CR> to go to Go section\n", progname );
		cc= getchar();
		if( cc == 'Q' )
		{
			vmeBusLock();
				mvtEnd();
			vmeBusUnlock();
			cleanup(0);
		}
	}

	/**************/
	/* Go section */
	/**************/
	// If the TI is not slave
	if( sys_params_ptr->RunMode == Standalone )
	{
		/* set sync event interval (in blocks) */
		vmeBusLock();
//			tiSetSyncEventInterval(10000/*block_level*/);
			tiSetSyncEventInterval(0/*block_level*/);
		vmeBusUnlock();
	}
	my_tiIntEnable(1);

	// Set kyeboard in no_delay mode
	ret = fcntl(0, F_GETFL, 0);                                                       
	fcntl (0, F_SETFL, (ret | O_NDELAY));
	DEF_SET_STDIN_NONBLOCKING;

	// Normally we can't get here if nmvt is not in range
	// But the code is kept to be as close to rol1 as possible
	if(nmvt>0)
	{
		vmeBusLock();
			mvtGo();
		vmeBusUnlock();
	}

	/* always clear exceptions */
	jlabgefClearException(1);

	// Only in case of composite output
	if( do_out == 2 )
		mynev = 0;

	vmeBusLock();
		tiStatus(1);
		sdStatus(1);
	vmeBusUnlock();

	fprintf(stdout, "%s INFO: Go Executed\n", progname ); fflush(stdout);

	if( step_by_step )
	{
		printf("%s INFO: Press Q to quit or <CR> to go to Readout section\n", progname );
		do
		{
			cc = '\0';
			read(STDIN_FILENO, &cc, 1);
			if( cc == 'Q' )
			{
				vmeBusLock();
					mvtEnd();
				vmeBusUnlock();
				cleanup(0);
			}
		} while ( cc != '\n' );
	}

	/***********************/
	/* Readout section     */
	/***********************/
	// Determine how many blocks to read
	if( (sys_params_ptr->EventLimit) && (max_num_events) )
	{
		if( sys_params_ptr->EventLimit<max_num_events )
			cmb_max_num_events = sys_params_ptr->EventLimit;
		else
			cmb_max_num_events = max_num_events;
	}
	else if( sys_params_ptr->EventLimit )
		cmb_max_num_events = sys_params_ptr->EventLimit;
	else if ( max_num_events )
		cmb_max_num_events = max_num_events;
	else
		cmb_max_num_events = 0;

	max_num_bloks = cmb_max_num_events / sys_params_ptr->NbOfEvtPerBlk;
	if( (0<cmb_max_num_events) && (cmb_max_num_events<sys_params_ptr->NbOfEvtPerBlk) )
		max_num_bloks++;
	if( max_num_bloks == 0 )
		printf("%s INFO: Readout will be started in infinite loop\n\tPress E to end; Q to quit\n", progname);
	else
		printf("%s INFO: Readout will be started for %d blocks (at least %d events)\n\tPress E to end; Q to quit\n",
			progname, max_num_bloks, cmb_max_num_events );

	if( (ret=tiEnableTriggerSource()) < 0 )
	{
		fprintf( stderr, "%s: tiEnableTriggerSource failed with %d\n", progname, ret );
		cleanup(0);
	}

	run = 1;
	do_post_reads = 0;
	block_num = 0;
	monit_block_num = 0;
	total_size = 0;
	monit_block_size = 0;
	cmp_monit_size = 0;
	gettimeofday(&monit_t0, 0);
	while( run )
	{
		// Check that ti has data to read
		// This is done in Coda not in rol1
		gettimeofday(&mvt_t0, 0);
		mvt_to_iter = 0;
		do
		{
			vmeBusLock();
				tireadme = tiBReady();
			vmeBusUnlock();
			syncFlag = tiGetSyncEventFlag();
			if( tireadme || do_post_reads ) 
			{
				break;
			}
			mvt_to_iter++;
			gettimeofday(&mvt_t1, 0);
			timersub(&mvt_t1,&mvt_t0,&mvt_dt);
		} while( timercmp(&mvt_dt,&ti_to,<) );
		if( tireadme == 0 )
		{
			if( do_post_reads == 0 )
			{
				ti_to_cntr++;
				fprintf( stdout, "%s in Readout: TI NOT READY: to_cntr=%d\n", progname, ti_to_cntr );
				if( mvt_fptr_err_1 != (FILE *)NULL )
				{
					fprintf( mvt_fptr_err_1, "%s in Readout: TI NOT READY: to_cntr=%d\n", progname, ti_to_cntr );
					fflush(  mvt_fptr_err_1 );
				}
			}
		}
		else
		{
			// Get some ti stat
			if( mvt_to_iter )
			{
				if( ti_max_to_iter < mvt_to_iter )
					ti_max_to_iter = mvt_to_iter;
				if( timercmp(&mvt_max_wait, &mvt_dt,<) )
				{
					ti_max_wait.tv_sec = mvt_dt.tv_sec;
					ti_max_wait.tv_usec = mvt_dt.tv_usec;
				}
			}

			/* Grab the data from the TI */
			dma_dabufp=(unsigned int *)i2_from_rol1;
			if( do_out )
				CEOPEN(event_type, BT_BANKS, syncFlag, block_num); /* reformatted on CODA_format.c !!! */
			if( do_out )
				BANKOPEN(0xe10A,1,roc_id);
			vmeBusLock();
				len = tiReadBlock(dma_dabufp,900>>2,1);
			vmeBusUnlock();
			if(len<=0)
			{
				fprintf( stdout, "%s in Readout: ERROR in tiReadBlock : No data or error, len = %d\n", progname, len );
				if( mvt_fptr_err_1 != (FILE *)NULL )
				{
					fprintf( mvt_fptr_err_1, "%s in Readout: ERROR in tiReadBlock : No data or error, len = %d\n", progname, len );
					fflush(  mvt_fptr_err_1 );
				}
				cleanup(0);
			}
			else
			{
				if( verbose > 3 )
				{
					fprintf( stdout, "\n%s in Readout: TI Raw data buffer blk=%d (evt %d-%d) size=%d\n",
						progname, block_num, 
						 block_num   *sys_params_ptr->NbOfEvtPerBlk + 1,
						(block_num+1)*sys_params_ptr->NbOfEvtPerBlk,
						len );
					if( verbose > 5 )
					{
						bptr = dma_dabufp;
						for( bindex=0; bindex<len; bindex++ )
						{
							if( (bindex % 8) == 0 )
								fprintf( stdout, "\n %04d:", bindex );
							fprintf( stdout, " 0x%08x", *bptr++ );
						}
						fprintf( stdout, "\n" );
					}
					fflush(  stdout );
				}

				// Update buffer
				dma_dabufp += len;
				if( do_out )
					BANKCLOSE;
				block_size = len;

				// Check that MVT has data to read
				gettimeofday(&mvt_t0, 0);
				mvt_to_iter = 0;
				do
				{
					vmeBusLock();
						mvtgbr = mvtGBReady(roc_id);
					vmeBusUnlock();
					if( mvtgbr == mvtSlotMask ) 
					{
						break;
					}
					mvt_to_iter++;
					gettimeofday(&mvt_t1, 0);
					timersub(&mvt_t1,&mvt_t0,&mvt_dt);
				} while( timercmp(&mvt_dt,&mvt_to,<) );
				if( mvtgbr != mvtSlotMask )
				{
					mvt_to_cntr++;
					fprintf( stdout, "%s in Readout: MVT NOT READY: gbready=0x%08x, expect 0x%08x, to_cntr=%d\n",
						progname, mvtgbr, mvtSlotMask, mvt_to_cntr );
					if( mvt_fptr_err_1 != (FILE *)NULL )
					{
						fprintf( mvt_fptr_err_1, "%s in Readout: MVT NOT READY: gbready=0x%08x, expect 0x%08x, to_cntr=%d\n",
							progname, mvtgbr, mvtSlotMask, mvt_to_cntr );
						mvtStatusDump(0, mvt_fptr_err_1);
						fflush( mvt_fptr_err_1 );
					}
				}
				else
				{
					// Get some mvt stat
					if( mvt_to_iter )
					{
						if( mvt_max_to_iter < mvt_to_iter )
							mvt_max_to_iter = mvt_to_iter;
						if( timercmp(&mvt_max_wait, &mvt_dt,<) )
						{
							mvt_max_wait.tv_sec = mvt_dt.tv_sec;
							mvt_max_wait.tv_usec = mvt_dt.tv_usec;
						}
					}

					/* Grab the data from the TI */
					if( do_out )
						BANKOPEN(0xe118,1,roc_id);
					vmeBusLock();
						len = mvtReadBlock(roc_id, dma_dabufp, 1000000, 1);
					vmeBusUnlock();
					if(len<=0)
					{
						fprintf( stdout, "%s in Readout: ERROR in mvtReadBlock : No data or error, len = %d for roc %d\n",
							progname, len, roc_id );
						if( mvt_fptr_err_1 != (FILE *)NULL )
						{
							fprintf( mvt_fptr_err_1, "%s in Readout: ERROR in mvtReadBlock : No data or error, len = %d for roc %d\n",
								progname, len, roc_id );
							fflush(  mvt_fptr_err_1 );
						}
						cleanup(0);
					}
					else
					{
						if( verbose > 3 )
						{
							fprintf( stdout, "\n%s in Readout: MVT Raw data buffer blk=%d (evt %d-%d) size=%d\n",
								progname, block_num, 
								 block_num   *sys_params_ptr->NbOfEvtPerBlk + 1,
								(block_num+1)*sys_params_ptr->NbOfEvtPerBlk,
								len );
							if( verbose > 5 )
							{
								bptr = dma_dabufp;
								for( bindex=0; bindex<len; bindex++ )
								{
									if( (bindex % 8) == 0 )
										fprintf( stdout, "\n %04d:", bindex );
									fprintf( stdout, " 0x%08x", *bptr++ );
								}
								fprintf( stdout, "\n" );
							}
							fflush(  stdout );
						}

						// Update buffer
						dma_dabufp += len;
						if( do_out )
							BANKCLOSE;
						block_size += len;
					} // else of if(len<=0) : MVT had valid data
				} // else of if( mvtgbr != mvtSlotMask ) : MVT was ready for reading
			} // else of if(len<=0) : ti had valid data
			block_num++;
			monit_block_num++;
			total_size += block_size;
			monit_block_size += block_size;
			// Process syncs if any
			if( syncFlag==1 )
			{
				;
			} // if( syncFlag==1 )

			// Inform TI that readout is done
			my_tiIntAck();
			// Readout finished close the bank
			if( do_out )
				CECLOSE;
			if( do_out == 1 )
			{
				if( verbose > 2 )
				{
					fprintf( stdout, "\n%s in Readout: Raw data buffer blk=%d (evt %d-%d) size=%d block_size=%d\n",
						progname, block_num, 
						(block_num-1)*sys_params_ptr->NbOfEvtPerBlk + 1,
						 block_num   *sys_params_ptr->NbOfEvtPerBlk,
						*StartOfEvent+1, block_size );
					if( verbose > 5 )
					{
						bptr = StartOfEvent;
						for( bindex=0; bindex<(*StartOfEvent+1); bindex++ )
						{
							if( (bindex % 8) == 0 )
								fprintf( stdout, "\n %04d:", bindex );
							fprintf( stdout, " 0x%08x", *bptr++ );
						}
						fprintf( stdout, "\n" );
					}
					fflush(  stdout );
				}
				if( ((block_num==1) || ((block_num%MVT_PRESCALE)==0)) && (MVT_PRESCALE!=1000000) )
				{
					if( (ret=fwrite( StartOfEvent, 4, *StartOfEvent+1, dat_fptr)) != (*StartOfEvent+1) )
					{
						fprintf( stderr, "%s in Readout: failed to store RAW data of size=%d; ret=%d\n", progname, (*StartOfEvent+1), ret );
						fprintf( stderr, "\n" );
						vmeBusLock();
							mvtEnd();
						vmeBusUnlock();
						cleanup(0);
					}	
				}
			}
			else if( do_out == 2 )
			{
				if( verbose > 2 )
				{
					fprintf( stdout, "\n%s in Readout: Raw data buffer blk=%d (evt %d-%d) size=%d (0x%04X) block_size=%d\n",
						progname, block_num, 
						(block_num-1)*sys_params_ptr->NbOfEvtPerBlk + 1,
						 block_num   *sys_params_ptr->NbOfEvtPerBlk,
						*StartOfEvent+1, *StartOfEvent+1, block_size );
					if( verbose > 5 )
					{
						bptr = StartOfEvent;
						for( bindex=0; bindex<*StartOfEvent+1; bindex++ )
						{
							if( (bindex % 8) == 0 )
								fprintf( stdout, "\n %04d:", bindex, bindex );
							fprintf( stdout, " 0x%08x", *bptr++ );
						}
						fprintf( stdout, "\n" );
					}
					fflush(  stdout );
				}
//sleep(1);
				dma_dabufp=(unsigned int *)i2_from_rol1;
				//Disentanglement section comes here
				if( (ret=disentanglement(roc_id)) < 0 )
				{
					fprintf( stdout, "%s in Readout: ERROR disentaglement failed with %d for roc %d\n", progname, ret, roc_id );
					if( mvt_fptr_err_2 != (FILE *)NULL )
					{
						fprintf( mvt_fptr_err_2, "%s in Readout: ERROR disentaglement failed with %d for roc %d\n", progname, ret, roc_id );
						fflush(  mvt_fptr_err_2 );
					}
					vmeBusLock();
						mvtEnd();
					vmeBusUnlock();
					cleanup(0);
				}
				else if( ret == 0 )
				{
					fprintf( stdout, "%s in Readout: disentaglement return 0 for roc %d\n", progname, roc_id );
					if( mvt_fptr_err_2 != (FILE *)NULL )
					{
						fprintf( mvt_fptr_err_2, "%s in Readout: disentaglement return 0 for roc %d\n", progname, roc_id );
						fflush(  mvt_fptr_err_2 );
					}
				}
				else
				{
					if( mvt_error_counter > 0 )
					{
						fprintf( stdout, "%s in Readout: Raw data buffer size=%d block_size=%d\n", progname, *StartOfEvent, block_size );
						bptr = StartOfEvent;
						for( bindex=0; bindex<*StartOfEvent+1; bindex++ )
						{
							if( (bindex % 8) == 0 )
								fprintf( stdout, "\n %04d:", bindex );
							fprintf( stdout, " 0x%08x", *bptr++ );
						}
						fprintf( stdout, "\n" );

						vmeBusLock();
							mvtEnd();
						vmeBusUnlock();
						cleanup(0);
					}

					if( verbose > 2 )
					{
						fprintf( stdout, "\n%s in Readout: after disentaglement blk=%d (evt %d-%d) len=%d embedded len=%d\n",
							progname, block_num, 
							(block_num-1)*sys_params_ptr->NbOfEvtPerBlk + 1,
							 block_num   *sys_params_ptr->NbOfEvtPerBlk,
							ret, *out_dabufp+1 );
						if( verbose > 5 )
						{
							bptr = out_dabufp;
							for( bindex=0; bindex<*out_dabufp+1; bindex++ )
							{
								if( (bindex % 8) == 0 )
									fprintf( stdout, "\n %04d:", bindex );
								fprintf( stdout, " 0x%08x", *bptr++ );
							}
							fprintf( stdout, "\n" );
						}
						fflush( stdout );
					}
				  if( ((block_num==1) || ((block_num%MVT_PRESCALE)==0)) && (MVT_PRESCALE!=1000000) )
				  {
					  if( (ret=fwrite( out_dabufp, 4, *out_dabufp+1, dat_fptr)) !=  (*out_dabufp+1) )
					  {
						  fprintf( stderr, "%s in Readout: failed to store CMP data of size=%d; ret=%d\n", progname, (*out_dabufp+1), ret );
						  fprintf( stderr, "\n" );
						  vmeBusLock();
							  mvtEnd();
						  vmeBusUnlock();
						  cleanup(0);
					  }
          }
					cmp_monit_size += (*out_dabufp+1);
				}
			}
			// Do monitoring
			gettimeofday(&monit_t1, 0);
			timersub(&monit_t1,&monit_t0,&monit_dt);
			if( timercmp(&monit_dt,&monit_to,>) )
			{
				fprintf(stdout, "%s in Readout: INFO DIFF block_size=%d for %d blocks; average ", progname, monit_block_size, monit_block_num );
				if( monit_block_num )
					fprintf(stdout, "%7.3f; ", (double)monit_block_size/(double)monit_block_num );
				else
					fprintf(stdout, "Unknown; " );
				my_tiDeadtime();
				tideltatime = my_tiLive();
				fprintf
				(
					stdout,
					"VME = %6.2f (MB/s) rate = %7.3f ",
					 (double) monit_block_size*4.*1000. / (((double) tideltatime )*30.*256.),
					((double) ( monit_block_num * sys_params_ptr->NbOfEvtPerBlk)*1000.*1000.*1000.) / (((double) tideltatime )*30.*256.)
				);
				fprintf
				(
					stdout,
					"VME = %6.2f (MB/s) ",
					 (double)monit_block_size*4. / ((double)monit_dt.tv_sec+((double)monit_dt.tv_usec)/1000000.) / 1024. / 1024.
				);
				tideltatime = tiLive(0);
				printf("diffdeadtime = %7.3f\n", 100. - (double)tideltatime / 1000. ) ;
				if( do_out == 2 )
				{
					fprintf(stdout, "%s in Readout: INFO DIFF comp_size=%d for %d blocks; average ", progname, cmp_monit_size, monit_block_num );
					if( monit_block_num )
						fprintf(stdout, "%7.3f; ", (double)cmp_monit_size/(double)monit_block_num );
					else
						fprintf(stdout, "Unknown; " );
					fprintf
					(
						stdout,
						"Eth = %6.2f (MB/s)\n\n",
						 (double)cmp_monit_size*4. / ((double)monit_dt.tv_sec+((double)monit_dt.tv_usec)/1000000.) / 1024. / 1024.
					);
					cmp_monit_size=0;
				}
				monit_block_size=0;
				monit_block_num = 0;
				gettimeofday(&monit_t0, 0);
			}
		} // else of if( tireadme == 0 ) : ti was ready for reading

		// Check keyboard input
		if( (ret = read(STDIN_FILENO, &cc, 1)) < 0 )
		{
			if( errno != EAGAIN )
			{
				fprintf(stderr, "%s in Readout phase: ERROR keyboard read failed with %d %s\n",
					progname, errno, strerror( errno ) );
				fflush( stderr );
				run = 0;
			}
		}
		else if( cc == 'E' )
		{
			// Request end of acqusition
			if( do_post_reads == 0 )
			{
				do_post_reads = 1;
				// Disable trigger sources 
				if( (ret=tiDisableTriggerSource(0)) < 0 )
				{
					fprintf( stderr, "%s in Readout phase: ERROR tiDisableTriggerSource failed with %d\n", progname, ret );
				}
				gettimeofday(&post_read_t0, 0);
				fprintf(stdout, "%s in Readout phase: end command issued from keyboard; postreads started for %d sec\n",
					progname, post_read_to.tv_sec );
				fflush( stdout );
			}
		}
		else if( cc == 'Q' )
		{
			// Force the end of data taking
			tiDisableTriggerSource(0);
			run = 0;
		}

		if( (max_num_bloks>0) && (block_num >= max_num_bloks) )
		{
			if( do_post_reads == 0 )
			{
				do_post_reads = 1;
				// Disable trigger sources 
				if( (ret=tiDisableTriggerSource(0)) < 0 )
				{
					fprintf( stderr, "%s in Readout phase: ERROR tiDisableTriggerSource failed with %d\n", progname, ret );
				}
				gettimeofday(&post_read_t0, 0);
				fprintf(stdout, "%s in Readout phase: requested block number limit %d reached; postreads started for %d sec\n",
					progname, max_num_bloks, post_read_to.tv_sec ); fflush( stdout );
			}
		}

		// End of accquisition 
		if( do_post_reads )
		{
			gettimeofday(&post_read_t1, 0);
			timersub(&post_read_t1,&post_read_t0,&post_read_dt);
			if( timercmp(&post_read_dt,&post_read_to,>) )
				run = 0;
		}
	} // while( run )

	// Set kyeboard back to blocking mode
	ret = fcntl(0, F_GETFL, 0);                                                       
	fcntl (0, F_SETFL, (ret & (~O_NDELAY)));
	DEF_SET_STDIN_BLOCKING;
	if( step_by_step )
	{
		printf("%s INFO: Press Q to quit or <CR> to go to End section\n", progname );
		cc= getchar();
		if( cc == 'Q' )
		{
			vmeBusLock();
				mvtEnd();
			vmeBusUnlock();
			cleanup(0);
		}
	}

	/***************/
	/* End section */
	/***************/
	/* Before disconnecting... wait for blocks to be emptied */
	vmeBusLock();
		blocksLeft = tiBReady();
	vmeBusUnlock();
	fprintf( stderr, "%s in End: >>>>>>>>>>>>>>>>>>>>>>> %d blocks left on the TI\n", progname, blocksLeft); fflush(stderr);

	if(nmvt>0)
	{
		vmeBusLock();
			mvtEnd();
		vmeBusUnlock();		
		if( mvt_fptr_err_1 != (FILE *)NULL )
		{
			mvtClrLogFilePointer();
			fprintf(mvt_fptr_err_1,"%s :  ti had to wait %d sec & %d us and iterate %d times\n",
				__FUNCTION__,  ti_max_wait.tv_sec,  ti_max_wait.tv_usec,  ti_max_to_iter );
			fprintf(mvt_fptr_err_1,"%s : mvt had to wait %d sec & %d us and iterate %d times\n",
				__FUNCTION__, mvt_max_wait.tv_sec, mvt_max_wait.tv_usec, mvt_max_to_iter );
			fflush( mvt_fptr_err_1 );
			fclose( mvt_fptr_err_1 );
			mvt_fptr_err_1 = (FILE *)NULL;
			// propper to the program
			log_fptr = (FILE *)NULL;
		}
	}

	// Only in case of composite output
	if( do_out == 2 )
	{
		if( mvt_fptr_err_2 != (FILE *)NULL )
		{
			fflush( mvt_fptr_err_2 );
			fclose( mvt_fptr_err_2 );
			mvt_fptr_err_2 = (FILE *)NULL;
		}
	}

	vmeBusLock();
		tiStatus(1);
		sdStatus(1);
	vmeBusUnlock();

	fprintf(stdout, "%s INFO: End Executed\n", progname ); fflush(stdout);

	/********************/
	/* Cleanup and stop */
	/********************/
	cleanup(0);
	return 0;
}

#else

int
main()
{
  exit(0);
}

#endif
