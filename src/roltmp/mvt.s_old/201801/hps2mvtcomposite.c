
/* hps2mvtcomposite.c - second readout list for VXS crates with MVT */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>


#include "circbuf.h"
#include "mvtLib.h"
#include "BecConfigParams.h"


#undef DEBUG
#undef DEBUG10 /*TI*/
#undef DEBUG1 /*TI*/
#undef DEBUG6 /*MVT*/
#undef DEBUG7 /*MVT*/

#define DEBUG_MVT_DAT_ERR

#define ROL_NAME__ "HPS2MVTCOMPOSITE"
#define INIT_NAME hps2mvtcomposite__init

#define POLLING_MODE
#define EVENT_MODE

#include "rol.h"
#include "EVENT_source.h"
/************************/
/************************/

#define MYNEV 21000

int mynev; /*defined in tttrans.c */

#define SPLIT_BLOCKS

#define PASS_AS_IS

#define ABS(x)      ((x) < 0 ? -(x) : (x))

/* open composite bank */
#define MVT_CCOPEN(btag,fmt,bnum) \
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
  lenout += (*dataout_save1+1); \
  lenev += (*dataout_save1+1); \
  b08 = NULL; \
}

//#define MAXBLOCK 22  /* 22-max# of blocks=boards of certain type */
//#define MAXEVENT 256 /* max number of events in one block */
//#define MAXSAMPLES 10    
//#define MAXFEU 32    

#define MAXBLOCK    2  /* MVT/FTT with not more than 2 SSP blocks=boards */
#define MAXEVENT   40  /* max number of events in one block */
#define MAXSAMPLES 128
#define MAXFEU     24

/* mvt board data type defs */
#define MVT_TYPE_BLKHDR    0xF3BB0000
#define MVT_TYPE_BLKTLR    0xFCCAFCAA
#define MVT_TYPE_EVTHDR    0xF3EE0000
#define MVT_TYPE_SAMPLE    0xF3550000
#define MVT_TYPE_FEUHDR    0xF3110000
#define MVT_TYPE_FILLER    0xFAAAFAAA

int MVT_ZS_MODE = -1;
int MVT_PRESCALE = 0;
int MVT_NBR_OF_BEU = 0;
int MVT_NBR_EVENTS_PER_BLOCK = 0;
int MVT_NBR_SAMPLES_PER_EVENT = 0;
int MVT_NBR_OF_FEU[DEF_MAX_NB_OF_BEU] = {0}; // PROBLEM WITH THIS ARRAY !!! Pour acceder au nombre de feu sur une beu, j'utilise soit un index de beu soit un beuid ...bref, je me mélange.
					     // Il me faut une correspondance beuID numero de beu ... bref, il faut harmoniser tout cela !!			
int mvt_event_number = 0; 

#define MVT_ERROR_NBR_OF_BEU            0x00000001
#define MVT_ERROR_NBR_EVENTS_PER_BLOCK  0x00000002
#define MVT_ERROR_NBR_SAMPLES_PER_EVENT 0x00000004
#define MVT_ERROR_NBR_OF_FEU            0x00000008
#define MVT_ERROR_EVENT_NUM             0x00000010
#define MVT_ERROR_BLOCK_NUM             0x00000020

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

int nFEU[MAXBANKS][MAXBLOCK][MAXEVENT][MAXSAMPLES]={0};
int iFEU[MAXBANKS][MAXBLOCK][MAXEVENT][MAXSAMPLES][MAXFEU]={0};
unsigned int mFEU[MAXBANKS][MAXBLOCK][MAXEVENT][MAXSAMPLES]={0};

int nbchannelsFEU[MAXBANKS][MAXBLOCK][MAXEVENT][MAXSAMPLES][MAXFEU]={0};
int sizeFEU[MAXBANKS][MAXBLOCK][MAXEVENT][MAXSAMPLES][MAXFEU]={0};

/* daq stuff */
static int rol2_report_raw_data;
/* daq stuff */

#define NTRIGBITS 6
static unsigned int bitscalers[NTRIGBITS];
static int havebits = 0;

/* user routines */

void rol2trig(int a, int b);
void rol2trig_done();

FILE *mvt_fptr_err_2 = (FILE *)NULL;

static void
__download()
{
	char logfilename[128];
	// time variables
	time_t      cur_time;
	struct tm  *time_struct;

	// Get current time
	cur_time = time(NULL);
	time_struct = localtime(&cur_time);
	if( mvt_fptr_err_2 == (FILE *)NULL )
	{
		sprintf(logfilename, "mvt_roc_%d_rol_2.log", rol->pid);
		if( (mvt_fptr_err_2 = fopen(logfilename, "w")) == (FILE *)NULL )
		{
			fprintf(stderr, "%s: fopen failed to open log file %s in write mode with %d %s\n",
				__FUNCTION__, logfilename, errno, strerror( errno ));
		}
	}
	if( mvt_fptr_err_2 != (FILE *)NULL )
	{
		fprintf( mvt_fptr_err_2, "**************************************************\n" );
		fprintf( mvt_fptr_err_2, "%s at %02d%02d%02d %02dH%02d\n", __FUNCTION__,
			time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday,
			time_struct->tm_hour, time_struct->tm_min );
		fflush(  mvt_fptr_err_2 );
	}
	rol->poll = 1;
  return;
}

static void
__prestart()
{
	int ii,ibeu;
//	int bank, block, event, sample, feu;

	char logfilename[128];
	// time variables
	time_t      cur_time;
	struct tm  *time_struct;

	printf("INFO: Entering Prestart ROL2\n");

	/* Clear some global variables etc for a clean start */
	 CTRIGINIT;

	/* init trig source EVENT */
	EVENT_INIT;

	/* Register a sync trigger source (up to 32 sources) */
	CTRIGRSS(EVENT, 1, rol2trig, rol2trig_done); /* second arg=1 - what is that ? */

	rol2_report_raw_data = daqGetReportRawData() | mvtGetRepRawData();
	printf("ROL2: rol2_report_raw_data set to %d\n",rol2_report_raw_data);

	rol->poll = 1;

	rol->recNb = 0;

	// Get current time
	cur_time = time(NULL);
	time_struct = localtime(&cur_time);
	if( mvt_fptr_err_2 == (FILE *)NULL )
	{
		sprintf(logfilename, "mvt_roc_%d_rol_2.log", rol->pid);
		if( (mvt_fptr_err_2 = fopen(logfilename, "w")) == (FILE *)NULL )
		{
			fprintf(stderr, "%s: fopen failed to open log file %s in write mode with %d %s\n",
				__FUNCTION__, logfilename, errno, strerror( errno ));
		}
	}
	if( mvt_fptr_err_2 != (FILE *)NULL )
	{
		fprintf( mvt_fptr_err_2, "**************************************************\n" );
		fprintf( mvt_fptr_err_2, "%s at %02d%02d%02d %02dH%02d\n", __FUNCTION__,
			time_struct->tm_year%100, time_struct->tm_mon+1, time_struct->tm_mday,
			time_struct->tm_hour, time_struct->tm_min );
		fflush(  mvt_fptr_err_2 );
	}
	if( (MVT_ZS_MODE = mvtGetZSMode(rol->pid)) < 0 )
	{
		printf("ERROR: MVT ZS mode negative\n");
		if( mvt_fptr_err_2 != (FILE *)NULL )
		{
			fprintf(mvt_fptr_err_2,"%s: ERROR MVT_ZS_MODE=%d\n", __FUNCTION__,  MVT_ZS_MODE  );
			fflush( mvt_fptr_err_2);
		}
	}
	MVT_PRESCALE              = mvtGetPrescale(rol->pid);
	MVT_NBR_OF_BEU            = mvtGetNbrOfBeu(rol->pid);
	MVT_NBR_EVENTS_PER_BLOCK  = mvtGetNbrOfEventsPerBlock(rol->pid);
	MVT_NBR_SAMPLES_PER_EVENT = mvtGetNbrOfSamplesPerEvent(rol->pid);
	for (ibeu = 0; ibeu <DEF_MAX_NB_OF_BEU; ibeu++)
	{
		MVT_NBR_OF_FEU[ibeu] = mvtGetNbrOfFeu(rol->pid, ibeu+1);
	}

	printf("INFO: MVT_ZS_MODE %d\n",                MVT_ZS_MODE             );
	printf("INFO: MVT_PRESCALE %d\n",               MVT_PRESCALE             );
	printf("INFO: MVT_NBR_OF_BEU %d\n",             MVT_NBR_OF_BEU           );
	printf("INFO: MVT_NBR_EVENTS_PER_BLOCK %d\n",   MVT_NBR_EVENTS_PER_BLOCK );
	printf("INFO: MVT_NBR_SAMPLES_PER_EVENT %d\n",  MVT_NBR_SAMPLES_PER_EVENT);
	for (ibeu = 0; ibeu<DEF_MAX_NB_OF_BEU; ibeu++)
	{
		printf("INFO: MVT_NBR_OF_FEU %d %d\n", ibeu, MVT_NBR_OF_FEU[ibeu] );
	}
/*
	// Clean reshafling tables
	for( bank=0; bank<MAXBANKS; bank++ )
	{
		nB[bank]=0;
		nBT[bank]=0;
		for( block=0; block<MAXBLOCK; block++ )
		{
			iB[ bank][block]=0;
			sB[ bank][block]=0;
			iBT[bank][block]=0; 
			nE[ bank][block]=0;
			for( event=0; event<MAXEVENT; event++ )
			{
				iE[  bank][block][event]=0;
				lenE[bank][block][event]=0;
				nSMP[bank][block][event]=0;
				for( sample=0; sample<MAXSAMPLES; sample++ )
				{
					iSMP[bank][block][event][sample]=0;
					nFEU[bank][block][event][sample]=0;
					mFEU[bank][block][event][sample]=0;
					for( feu=0; feu<MAXFEU; feu++ )
					{
						iFEU[         bank][block][event][sample][feu]=0;
						nbchannelsFEU[bank][block][event][sample][feu]=0;
						sizeFEU      [bank][block][event][sample][feu]=0;
					} // for( feu=0; feu<MAXFEU; feu++ )
				} // for( sample=0; sample<MAXSAMPLES; sample++ )
			} // for( event=0; event<MAXEVENT; event++ )
		} // for( block=0; block<MAXBLOCK; block++ )
	} //for( bank=0; bank<MAXBANKS; bank++ )
*/
	printf("INFO: Prestart ROL22 executed\n");
	return;
}

static void
__end()
{
	if( mvt_fptr_err_2 != (FILE *)NULL )
	{
		fflush( mvt_fptr_err_2 );
		fclose( mvt_fptr_err_2 );
		mvt_fptr_err_2 = (FILE *)NULL;
	}
	printf("INFO: User End 2 Executed\n");
	return;
}

static void
__pause()
{
	printf("INFO: User Pause 2 Executed\n");
	return;
}

static void
__go()
{
	printf("User Go 2 Reached\n");
	fflush(stdout);
	mynev = 0;
	printf("INFO: User Go 2 Executed\n");
	fflush(stdout);
	MVT_NBR_EVENTS_PER_BLOCK  = mvtGetNbrOfEventsPerBlock(rol->pid);
	printf("INFO: %s MVT_NBR_EVENTS_PER_BLOCK %d\n", __FUNCTION__, MVT_NBR_EVENTS_PER_BLOCK);
	fflush(stdout);
	return;
}

#ifdef PASS_AS_IS
	int nASIS;
	int iASIS[MAXBANKS]={0};
#endif

#define REPORT_RAW_BANK_IF_REQUESTED if(rol2_report_raw_data) iASIS[nASIS++] = jj

void
rol2trig(int a, int b)
{
	CPINIT;
	BANKINIT;
 
  int local_event_number_high;
  int current_event_number_high;
  int local_event_number_low;
  int current_event_number_low;
  int current_block_number;
  int mvt_error_counter;
  int mvt_error_type;
  int beu;
  int i, j, k, l, m,p, iii, ind, bank, nhits=0, mux_index, rlen, printing, nnE, iev, ibl;
  int i_sample, i_dream, i_channel, i_feu,currentBeuId, currentFeuId, current_sample, current_channel_id;
  int current_timestamp_low, current_timestamp_high, current_feu_tmstmp;

  unsigned int *iw;
  int nr = 0;
  int ncol = 2;
  int a_channel, a_chan1, a_chan2, a_nevents, a_blocknumber, a_triggernumber, a_module_id;
  int a_windowwidth, a_pulsenumber, a_firstsample, samplecount;
  int a_adc1, a_adc2, a_valid1, a_valid2, a_nwords, a_slot, a_slot2, a_slot3;
  int a_hfcb_id, a_chip_id, a_chan;
  unsigned int a_bco, a_bco1;
  int a_slot_prev;
  int a_qualityfactor, a_pulseintegral, a_pulsetime, a_vm, a_vp;
  int a_trigtime[4];
  int a_tdc, a_edge;
  int a_slot_old;
  int a_channel_old;
  int npedsamples, atleastoneslot, atleastonechannel[21];
  time_t now;
  int error;
  int ndnv, nw;
  char errmsg[256];
  unsigned int *StartOfBank;
  char *ch;
  unsigned int *Nchan, *Npuls, *Nsamp;
  int islot, ichan, ii, jj, kk;
  int banknum = 0;
  int have_time_stamp, a_nevents2, a_event_type;
  int a_event_number_l, a_timestamp_l, a_event_number_h, a_timestamp_h, a_bitpattern;
  int a_clusterN, a_clusterE, a_clusterY, a_clusterX, a_clusterT, a_type, a_data, a_time;
  long long timestamp, latency, latency_offset;

	mynev ++; /* needed by ttfa.c */

	BANKSCAN;

#ifdef DEBUG
	printf("\n\n\n\n\n nbanks=%d\n",nbanks);
	for(jj=0; jj<nbanks; jj++) printf("bankscan[%d]: tag 0x%08x typ=%d nr=%d nw=%d dataptr=0x%08x\n",
		jj,banktag[jj],banktyp[jj],banknr[jj],banknw[jj],bankdata[jj]);
#endif


	/* first for() over banks from rol1 */
#ifdef PASS_AS_IS
	nASIS = 0; /*cleanup AS IS banks counter*/
#endif
	for(jj=0; jj<nbanks; jj++)
	{
		datain = bankdata[jj];
		lenin = banknw[jj];
		/* swap input buffer (assume that data from VME is big-endian, and we are on little-endian Intel) */
		if( banktyp[jj] != 3 )
			for(ii=0; ii<lenin; ii++)
				datain[ii] = LSWAP(datain[ii]);











		if(banktag[jj] == 0xe10A) /* TI hardware format */
		{
			banknum = rol->pid;

#ifdef DEBUG1
			printf("\nFIRST PASS TI\n\n");
#endif

			error = 0;
			ii=0;
			printing=1;
			a_slot_prev = -1;
			have_time_stamp = 1;
			nB[jj]=0; /*cleanup block counter*/
			/*
				if(lenin != 8) printf("lenin=%d (index=%d)\n",lenin,ii);
			*/
			while(ii<lenin)
			{
#ifdef DEBUG1
				printf("[%5d] 0x%08x (lenin=%d)\n",ii,datain[ii],lenin);
#endif
				if( ((datain[ii]>>27)&0x1F) == 0x10) /*block header*/
				{
					a_slot_prev = a_slot;
					a_slot = ((datain[ii]>>22)&0x1F);
					a_module_id = ((datain[ii]>>18)&0xF);
					a_blocknumber = ((datain[ii]>>8)&0x3FF);
					a_nevents = (datain[ii]&0xFF);
#ifdef DEBUG1
					printf("[%3d] BLOCK HEADER: slot %d, nevents %d, block number %d module id %d\n",ii,
						a_slot,a_nevents,a_blocknumber,a_module_id);
					printf(">>> update iB and nB\n");
#endif
					nB[jj]++;                  /*increment block counter*/
					iB[jj][nB[jj]-1] = ii;     /*remember block start index*/
					sB[jj][nB[jj]-1] = a_slot; /*remember slot number*/
					nE[jj][nB[jj]-1] = 0;      /*cleanup event counter in current block*/

#ifdef DEBUG1
					printf("0xe10A: jj=%d nB[jj]=%d\n",jj,nB[jj]);
#endif

					ii++;

					/* second block header word */
					if(((datain[ii]>>17)&0x7FFF) != 0x7F88) printf("ERROR in TI second block header word\n");
					have_time_stamp = ((datain[ii]>>16)&0x1);
					if(((datain[ii]>>8)&0xFF) != 0x20) printf("ERROR in TI second block header word\n");
					a_nevents2 = (datain[ii]&0xFF);
					if(a_nevents != a_nevents2) printf("ERROR in TI: a_nevents=%d != a_nevents2=%d\n",a_nevents,a_nevents2);
					ii++;


					for(iii=0; iii<a_nevents; iii++)
					{
						/* event header */
						a_event_type = ((datain[ii]>>24)&0xFF);
//						if(a_event_type==0)               printf("%s: 1st pass ERROR in TI event header word: datain[%d]=0x%08X : event type = %d for event %d\n", __FUNCTION__, ii, datain[ii], a_event_type, datain[ii+1]);
						if(((datain[ii]>>16)&0xFF)!=0x01) printf("%s: 1st pass ERROR in TI event header word: datain[%d]=0x%08X : (0x%02x) != 0x01\n",__FUNCTION__, ii, datain[ii], ((datain[ii]>>16)&0xFF));
						a_nwords = datain[ii]&0xFF;
#ifdef DEBUG1
						printf("[%3d] EVENT HEADER, a_nwords = %d\n",ii,a_nwords);
#endif

						/*"close" previous event if any*/
						k = nB[jj]-1; /*current block index*/
#ifdef DEBUG1
						printf("0xe10A: k=%d\n",k);
#endif
						if(nE[jj][k] > 0)
						{
							m = nE[jj][k]-1; /*current event number*/
							lenE[jj][k][m] = ii-iE[jj][k][m]; /*#words in current event*/
#ifdef DEBUG1
							printf("0xe10A: m=%d lenE=%d\n",m,lenE[jj][k][m]);
#endif
						}
			
						/*"open" next event*/
						nE[jj][k] ++; /*increment event counter in current block*/
						m = nE[jj][k]-1; /*current event number*/
						iE[jj][k][m] = ii; /*remember event start index*/
#ifdef DEBUG1
						printf("0xe10A: nE=%d m=%d iE=%d\n",nE[jj][k],m,iE[jj][k][m]);
#endif
						ii++;

						if(a_nwords>0)
						{
							a_event_number_l = datain[ii];
#ifdef DEBUG1
							printf("[%3d] a_event_number_1 = %d\n",ii,a_event_number_l);
#endif
							ii++;
						}

						if(a_nwords>1)
						{
							a_timestamp_l = datain[ii];
#ifdef DEBUG1
							printf("[%3d] a_timestamp_l = %d\n",ii,a_timestamp_l);
#endif
							ii++;
						}

						if(a_nwords>2)
						{
							a_event_number_h = (datain[ii]>>16)&0xFFFF;
							a_timestamp_h = datain[ii]&0xFFFF;
#ifdef DEBUG1
							printf("[%3d] a_event_number_h = %d a_timestamp_h = %d \n",ii,a_event_number_h,a_timestamp_h);
#endif
							ii++;
						}

		    if(a_nwords>3)
		    {
		      a_bitpattern = datain[ii]&0xFF;
#ifdef DEBUG1
		      printf("[%3d] a_bitpattern = 0x%08x\n",ii,a_bitpattern);
#endif
		      ii++;
		    }

		    if(a_nwords>4)
		    {
#ifdef DEBUG1
		      printf("[%3d] TS word4 = 0x%08x\n",ii,datain[ii]);
#endif
		      ii++;
		    }

		    if(a_nwords>5)
		    {
#ifdef DEBUG1
		      printf("[%3d] TS word5 = 0x%08x\n",ii,datain[ii]);
#endif
		      ii++;
		    }

		    if(a_nwords>6)
		    {
#ifdef DEBUG1
		      printf("[%3d] TS word6 = 0x%08x\n",ii,datain[ii]);
#endif
		      ii++;
		    }

		    if(a_nwords>7)
		    {
		      printf("ERROR: TS has more then 7 data words - exit\n");
		      exit(0);
		    }



		  			} // for(iii=0; iii<a_nevents; iii++)
				}
				else if( ((datain[ii]>>27)&0x1F) == 0x11) /*block trailer*/
				{
					a_slot2 = ((datain[ii]>>22)&0x1F);
					a_nwords = (datain[ii]&0x3FFFFF);
#ifdef DEBUG1
					printf("[%3d] BLOCK TRAILER: slot %d, nwords %d\n",ii,
						a_slot2,a_nwords);
					printf(">>> data check\n");
#endif

					/*"close" previous event if any*/
					k = nB[jj]-1; /*current block index*/
					if(nE[jj][k] > 0)
					{
						m = nE[jj][k]-1; /*current event number*/
						lenE[jj][k][m] = ii-iE[jj][k][m]; /*#words in current event*/
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
					if(a_nwords != (ii-iB[jj][nB[jj]-1]+1))
					{
						error ++;
						if(printing)
						{
							printf("[%3d][%3d] ERROR2 in TI data: trailer #words %d != actual #words %d\n",mynev,
					 			ii,a_nwords,ii-iB[jj][nB[jj]-1]+1);
							printing=0;
						}
					}
					ii++;
				}
				else if( ((datain[ii]>>27)&0x1F) == 0x1F)
				{
#ifdef DEBUG1
					printf("[%3d] FILLER WORD: \n",ii);
					printf(">>> do nothing\n");
#endif
					ii++;
				}
				else
				{
					printf("TI UNKNOWN data: [%3d] 0x%08x\n",ii,datain[ii]);
					{
						int jjj;
						printf("\n   Previous stuff\n");
						for(jjj=ii-20; jjj<ii; jjj++) printf("          [%3d][%3d] 0x%08x\n",jjj,ii,datain[jjj]);
						for(jjj=ii; jjj<ii+10; jjj++) printf("           [%3d][%3d] 0x%08x\n",jjj,ii,datain[jjj]);
						printf("   End Of Previous stuff\n");fflush(stdout);
						exit(0);
					}
					ii++;
				}
			} /* while(ii<lenin) */
		} /* if(banktag[jj] == 0xe10A) TI hardware format */





//***********************************************************************************
//***********************************************************************************
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


		else if(banktag[jj] == 0xe118) /* MVT hardware format */
		{
			if( ((mvt_event_number%MVT_PRESCALE)==0) && (MVT_PRESCALE!=1000000) )
				REPORT_RAW_BANK_IF_REQUESTED;
			banknum = rol->pid;
	
			error   = 0;
			ii      = 0;
			printing= 1;
			nB[jj]  = 0; /* cleanup block counter */            
			nBT[jj] = 0; /* cleanup block trailer counter */
#ifdef DEBUG6
if( mvt_fptr_err_2 != (FILE *)NULL )
{
	fprintf(mvt_fptr_err_2,"%s: FIRST PASS MVT STARTS lenin=%d ii=%d datain[ii]=0x%08x address=0x%08x\n", __FUNCTION__, lenin, ii, datain[ii], datain);
	fflush(mvt_fptr_err_2);
}
#endif

			while(ii<lenin)
			{
#ifdef DEBUG6
if( mvt_fptr_err_2 != (FILE *)NULL )
{
	fprintf(mvt_fptr_err_2,"%s: FIRST PASS MVT Go through lenin=%d ii=%d datain[ii]=0x%08x\n", __FUNCTION__, lenin, ii, datain[ii]);
	fflush(mvt_fptr_err_2);
}
#endif
				if( (datain[ii]&0xFFFF0000) == MVT_TYPE_BLKHDR) /* block header */
				{
					nB[jj]++;                 /* increment block counter */
					iB[jj][nB[jj]-1] = ii;    /* remember block start index */
					nE[jj][nB[jj]-1] = 0;     /* cleanup event counter in current block */
					ii++;
#ifdef DEBUG6
if( mvt_fptr_err_2 != (FILE *)NULL )
{
	fprintf(mvt_fptr_err_2,"%s: FIRST PASS MVT MVT_TYPE_BLKHDR=0x%08x jj=%d ii=%d datain[ii]=0x%08x nB[jj]=%d\n", __FUNCTION__, MVT_TYPE_BLKHDR, jj, ii, datain[ii], nB[jj]);
	fflush(mvt_fptr_err_2);
}
#endif
				}
				else if( (datain[ii]&0xFFFFFFFF) == MVT_TYPE_BLKTLR) /*block trailer*/
				{
					/* "close" previous event if any */
					k = nB[jj]-1; /* current block index*/
					if(nE[jj][k] > 0)
					{
						m = nE[jj][k]-1;                  /* current event number*/
						lenE[jj][k][m] = ii-iE[jj][k][m]; /* #words in current event */
					}	      
					nBT[jj]++; 
					ii++; 
				}
				else if ( (datain[ii]&0xFFFF0000) == MVT_TYPE_EVTHDR ) /*event header */
				{	    
					k = nB[jj]-1; /*current block index*/		

					/*"close" previous event if any*/
					if(nE[jj][k] > 0)
					{
						m = nE[jj][k]-1;                  /* current event number */
						lenE[jj][k][m] = ii-iE[jj][k][m]; /* #words in current event */
					}
					/*"open" next event*/
					nE[jj][k]++;           /* increment event counter in current block*/
					m = nE[jj][k]-1;       /* current event number*/
					iE[jj][k][m]      =ii; /* remember event start index*/	
					nSMP[jj][k][m]    = 1; /* initialize sample number in current event */
					iSMP[jj][k][m][0] = ii;
					nFEU[jj][k][m][0] = 0; /* cleanup FEU counter in current sample*/
					mFEU[jj][k][m][0] = 0; /* cleanup FEU map in current sample*/
					ii++;	      
				}
				else if (  (datain[ii]&0xFFFF0000) == MVT_TYPE_SAMPLE ) /*sample header*/
				{
					k = nB[jj]-1;            /* current block index */
					m = nE[jj][k]-1;         /* current event number */
					nSMP[jj][k][m]++;        /* initialize sample number in current event */
					l = nSMP[jj][k][m]-1;
					iSMP[jj][k][m][l] = ii;
					nFEU[jj][k][m][l] = 0;   /* cleanup FEU counter in current sample */
					mFEU[jj][k][m][l] = 0;   /* cleanup FEU map in current sample */
					ii++;	  
				}
				else if( (datain[ii]&0xFFFF0000) == MVT_TYPE_FEUHDR) /* mvt sample */
				{
					k = nB[jj]-1;             /* current block index*/
					m = nE[jj][k]-1;          /* current event number*/
					l = nSMP[jj][k][m]-1;     /* current sample number*/
					nFEU[jj][k][m][l]++;      /* incrment FEU counter in current sample*/
					p = nFEU[jj][k][m][l]-1;  /* current feu number*/
					iFEU[jj][k][m][l][p]=ii;  /* remember event start index*/
					nbchannelsFEU[jj][k][m][l][p]=0;
					sizeFEU[jj][k][m][l][p] = (datain[ii]&0x000003FF);
currentFeuId = ((( datain[ ii] ) & 0x00007C00 ) >> 10 );
if( (currentFeuId < 0) || (currentFeuId >23) )
	fprintf(mvt_fptr_err_2,"%s: FIRST PASS strange feuid %d in datain[ii]=0x%08x \n", __FUNCTION__, currentFeuId, ii, datain[ii]);
else
	mFEU[jj][k][m][l] |= (1<<currentFeuId); 

#ifdef DEBUG6
if( mvt_fptr_err_2 != (FILE *)NULL )
{
	fprintf(mvt_fptr_err_2,"%s: MVT_ZS_MODE : %d ii=%d datain[ii]=0x%08x\n", __FUNCTION__, MVT_ZS_MODE, ii, datain[ii] );
	fflush(mvt_fptr_err_2);
}
#endif
					if (!MVT_ZS_MODE) { 
						nbchannelsFEU[jj][k][m][l][p] =  ( sizeFEU[jj][k][m][l][p]  - 2 - 6  ) / 74 * 64;		//????? ATTENTION : this size has an offset
					}
					else {
						nbchannelsFEU[jj][k][m][l][p] =  ( sizeFEU[jj][k][m][l][p]  - 2 - 6  ) / 2;
					}
#ifdef DEBUG6					
if( mvt_fptr_err_2 != (FILE *)NULL )
{
	fprintf(mvt_fptr_err_2,"%s: FIRST PASS 0xe118 : bnk=%d blk=%d evt=%d smp=%d feu=%d size %d (0x%08x) nbchannelsFEU %d\n", __FUNCTION__, jj, k, m, l, p, sizeFEU[jj][k][m][l][p] , sizeFEU[jj][k][m][l][p], nbchannelsFEU[jj][k][m][l][p]    );
	fflush(mvt_fptr_err_2);
}
#endif
					if( nbchannelsFEU[jj][k][m][l][p] > 0 )
						ii+= ((sizeFEU[jj][k][m][l][p]  -2) >> 1 ) ;   //big jump over the data
					else
						ii++;
#ifdef DEBUG6								
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
			if (nB[jj] != MVT_NBR_OF_BEU )
			{
				mvt_error_counter ++;
				mvt_error_type |= MVT_ERROR_NBR_OF_BEU;
#ifdef DEBUG6
if( mvt_fptr_err_2 != (FILE *)NULL )
{
	fprintf(mvt_fptr_err_2,"%s: MVT FIRST PASS 0xe118 ERROR: jj=%d nB[jj]=%d != MVT_NBR_OF_BEU=%d\n", __FUNCTION__, jj, nB[jj], MVT_NBR_OF_BEU);
	fflush(mvt_fptr_err_2);
}
#endif
			}
			else
			{
				for(ibl=0; ibl < MVT_NBR_OF_BEU ; ibl++)
				{ 		
					currentBeuId = ((( datain[ iB[jj][ibl] ] ) & 0x0000F000 ) >> 12 )  - 1;		
					if( nE[jj][ibl] != MVT_NBR_EVENTS_PER_BLOCK )
					{
						mvt_error_counter ++;
						mvt_error_type |= MVT_ERROR_NBR_EVENTS_PER_BLOCK;
					} 
					else
					{
						for (iev=0; iev < MVT_NBR_EVENTS_PER_BLOCK ; iev ++)
						{ 
							if( nSMP[jj][ibl][iev] != MVT_NBR_SAMPLES_PER_EVENT )
							{
								mvt_error_counter ++;
								mvt_error_type |= MVT_ERROR_NBR_SAMPLES_PER_EVENT;
								printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
								printf("0xe118 : mvt error currentBeuId =%d jj =%d ibl=%d iev=%d nsmp =%d expected=%d\n",
									currentBeuId, jj, ibl, iev, nSMP[jj][ibl][iev], MVT_NBR_SAMPLES_PER_EVENT );
							} 
							else
							{
								for (i_sample=0; i_sample < MVT_NBR_SAMPLES_PER_EVENT ; i_sample ++)
								{ 
									if( nFEU[jj][ibl][iev][i_sample] != MVT_NBR_OF_FEU[currentBeuId] )
									{
										printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
//										printf("0xe118 : mvt error currentBeuId =%d jj =%d ibl=%d iev=%d i_sample=%d nfeu=%d expected=%d\n",
//											currentBeuId, jj, ibl, iev, i_sample, nFEU[jj][ibl][iev][i_sample],
//											MVT_NBR_OF_FEU[currentBeuId] );
										printf("0xe118 : mvt error currentBeuId =%d jj =%d ibl=%d iev=%d i_sample=%d nfeu=%d expected=%d received from 0x%08x\n",
											currentBeuId, jj, ibl, iev, i_sample, nFEU[jj][ibl][iev][i_sample],
											MVT_NBR_OF_FEU[currentBeuId], mFEU[jj][ibl][iev][i_sample]);
										printf("0xe118 : nfeu for beu 0 %d 1 %d 2 %d\n", MVT_NBR_OF_FEU[0], MVT_NBR_OF_FEU[1], MVT_NBR_OF_FEU[2] );
										mvt_error_counter ++;
										mvt_error_type |= MVT_ERROR_NBR_OF_FEU;
if( mvt_fptr_err_2 != (FILE *)NULL )
{
	fprintf(mvt_fptr_err_2,"%s: MVT FIRST PASS 0xe118 ERROR: currentBeuId =%d jj =%d ibl=%d iev=%d i_sample=%d nfeu=%d expected=%d received from 0x%08x\n",
											__FUNCTION__, currentBeuId, jj, ibl, iev, i_sample, nFEU[jj][ibl][iev][i_sample],
											MVT_NBR_OF_FEU[currentBeuId], mFEU[jj][ibl][iev][i_sample]);
	fflush(mvt_fptr_err_2);
}

									} 
								}
							}
						} // for (iev=0; iev < MVT_NBR_EVENTS_PER_BLOCK ; iev ++)
					}
				} // for(ibl=0; ibl < MVT_NBR_OF_BEU ; ibl++)
			} // else of if (nB[jj] != MVT_NBR_OF_BEU )

			//check block numbers BEUSSP block headers
			// - test that all blocks have the same number
			// - test that block number increments correctly
			current_block_number = (( datain[  iB[jj][0]  ]  )& 0x00000FFF ) ; 
			for(ibl=0; ibl < MVT_NBR_OF_BEU ; ibl++)
			{ 
				if (   ( ( datain[  iB[jj][ibl]  ]  )& 0x00000FFF ) != current_block_number )
				{
					mvt_error_counter ++;
					mvt_error_type |= MVT_ERROR_BLOCK_NUM;
				}
				//if ( ( ( ( datain[  iB[jj][ibl]  ]  )& 0x00007000 )>> 12) != ibl ) {mvt_error_counter ++; mvt_error_type |= MVT_ERROR_BLOCK_ID;}
			}

			//check event numbers and timestamps from BEUSSP event headers
			for (iev=0; iev < MVT_NBR_EVENTS_PER_BLOCK ; iev ++)
			{
				current_event_number_high = (( datain[ ( iSMP[jj][0][iev][0] + 2 ) ] ) & 0x00007FFC) >> 2;
				current_event_number_low  =
					((( datain[ ( iSMP[jj][0][iev][0] + 2 ) ] ) & 0x00000003) << 30) + 
					((( datain[ ( iSMP[jj][0][iev][0] + 3 ) ] ) & 0x7FFF0000) >>  1) + 
					((  datain[ ( iSMP[jj][0][iev][0] + 3 ) ] ) & 0x00007FFF);		
				for(ibl=0; ibl < MVT_NBR_OF_BEU ; ibl++)
				{ 
					for (i_sample=0; i_sample < MVT_NBR_SAMPLES_PER_EVENT ; i_sample ++)
					{ 
						local_event_number_high = (( datain[ ( iSMP[jj][ibl][iev][i_sample] + 2 ) ] ) & 0x00007FFC) >> 2;
						local_event_number_low  =
							((( datain[ ( iSMP[jj][ibl][iev][i_sample] + 2 ) ] ) & 0x00000003) << 30) + 
							((( datain[ ( iSMP[jj][ibl][iev][i_sample] + 3 ) ] ) & 0x7FFF0000) >>  1) + 
							((  datain[ ( iSMP[jj][ibl][iev][i_sample] + 3 ) ] ) & 0x00007FFF);		
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
					fprintf(mvt_fptr_err_2,"%s: 0xe118 : mvt error counter 0x%8x %d mvt error type 0x%8x for buflen %d\n", __FUNCTION__, mvt_error_counter, mvt_error_counter, mvt_error_type, lenin );
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
//fclose(mvt_fptr_err_2);
//exit(1);
				}
				else
				{
					printf("PPPPPPPPPPPPPPPAAAAAAAAAAAAAAAAAAANNNNNNNNNNNNNNNNNNNIIIIIIIIIIIIIIIIIIIIIICCCCCCCCCCCCC\n");
				}
#endif
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
		} /* else if(banktag[jj] == 0xe118) /* MVT hardware format */

//***********************************************************************************
//***********************************************************************************
//***********************************************************************************
#ifdef PASS_AS_IS
		else /*any other bank will be passed 'as is' */
		{
		  iASIS[nASIS++] = jj; /* remember bank number as it reported by BANKSCAN */
		  /*printf("mynev=%d: remember bank number %d\n",mynev,jj);*/
		}
#endif




	} /* first for() over banks from rol1 for(jj=0; jj<nbanks; jj++) */

	/********************************************************/
	/********************************************************/
	/********************************************************/
	/* SECOND PASS: disantangling and filling output buffer */
#ifdef DEBUG
	printf("\n\n\nSECOND PASS\n\n");
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

		for(jj=0; jj<nbanks; jj++) /* loop over evio banks */
		{
			datain = bankdata[jj];
#ifdef DEBUG
			printf("iev=%d jj=%d nB=%d\n",iev,jj,nB[jj]);
#endif
			if(banktag[jj] == 0xe10A) /* TI hardware format */
			{
				banknum = iev; /*rol->pid;*/
#ifdef DEBUG1
				printf("SECOND PASS TI\n");
#endif
				for(ibl=0; ibl<nB[jj]; ibl++) /*loop over blocks*/
				{
#ifdef DEBUG10
					printf("\n\n\n0xe10A: Block %d, Event %2d, event index %2d, event lenght %2d\n",
						ibl,iev,iE[jj][ibl][iev],lenE[jj][ibl][iev]);
#endif
					a_slot = sB[jj][ibl];
					ii     = iE[jj][ibl][iev];
					rlen   = ii + lenE[jj][ibl][iev];
          			//printf("TI EVENT HEADER, ii = %d , rlen = %d \n",ii, rlen);
					while(ii<rlen)
					{
#ifdef DEBUG10
						printf("[%5d] 0x%08x (rlen=%d)\n",ii,datain[ii],rlen);
#endif
						CPOPEN(0xe10A,1,banknum);
						/* event header */
						a_event_type = ((datain[ii]>>24)&0xFF);
						if(((datain[ii]>>16)&0xFF)!=0x01) printf("%s: 2nd pass ERROR in TI event header word (0x%02x) != 0x01\n",__FUNCTION__, ((datain[ii]>>16)&0xFF));
						a_nwords = datain[ii]&0xFF;
#ifdef DEBUG1
						printf("[%3d] EVENT HEADER, a_nwords = %d\n",ii,a_nwords);
#endif
						dataout[0] = datain[ii];
						b08 += 4;

						ii++;

						if(a_nwords>0)
						{
							a_event_number_l = datain[ii];
#ifdef DEBUG1
							printf("[%3d] a_event_number_1 = %d\n",ii,a_event_number_l);
#endif
							dataout[1] = datain[ii];
							b08 += 4;
							ii++;
						}

						if(a_nwords>1)
						{
							a_timestamp_l = datain[ii];
#ifdef DEBUG1
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
#ifdef DEBUG1
							printf("[%3d] a_event_number_h = %d a_timestamp_h = %d \n",ii,a_event_number_h,a_timestamp_h);
#endif
							dataout[3] = datain[ii];
							b08 += 4;
							ii++;
						}




		    if(a_nwords>3)
		    {
		      a_bitpattern = datain[ii]&0xFF;
#ifdef DEBUG1
		      printf("[%3d] a_bitpattern = 0x%08x\n",ii,a_bitpattern);
#endif
              for(i=0; i<NTRIGBITS; i++) bitscalers[i] += ((a_bitpattern>>i)&0x1);
              havebits = 1;

              dataout[4] = datain[ii];
              b08 += 4;
			  
		      ii++;
		    }
			
		    if(a_nwords>4)
		    {
#ifdef DEBUG1
		      printf("[%3d] TS word4 = 0x%08x\n",ii,datain[ii]);
#endif
		      ii++;
		    }

		    if(a_nwords>5)
		    {
#ifdef DEBUG1
		      printf("[%3d] TS word5 = 0x%08x\n",ii,datain[ii]);
#endif
		      ii++;
		    }

		    if(a_nwords>6)
		    {
#ifdef DEBUG1
		      printf("[%3d] TS word6 = 0x%08x\n",ii,datain[ii]);
#endif
		      ii++;
		    }

		    if(a_nwords>7)
		    {
		      printf("ERROR: TS has more then 7 data words - exit\n");
		      exit(0);
		    }


#ifdef DEBUG1
		    printf("[%3d] CPCLOSE b08=0x%08x\n",ii,b08);
#endif






            			CPCLOSE;
					} /* while(ii<rlen) */
				} /* for(ibl=0; ibl<nB[jj]; ibl++)loop over blocks */
			} /* if(banktag[jj] == 0xe10A) /* TI hardware format */

//***********************************************************************************
//***********************************************************************************
//***********************************************************************************
/* newer format 27/08/2015
F3BB BEUIDBlkNum
F3EE FineTSTP TSTP1 TSTP2 TSTP3 EvtNum1 EvtNum2 EvtNum3 SmpNum AcceptEvtNum
F311 LnkNumSize FEUdata...
F311 LnkNumSize FEUdata...
F355 FineTSTP TSTP1 TSTP2 TSTP3 EvtNum1 EvtNum2 EvtNum3 SmpNum AcceptEvtNum
F311 LnkNumSize FEUdata...
F311 LnkNumSize FEUdata...
Filler...
FCCA FCAA
*/

/*
int MVT_NBR_OF_BEU = 2;
int MVT_NBR_EVENTS_PER_BLOCK = 4;
int MVT_NBR_SAMPLES_PER_EVENT = 6;
int MVT_NBR_OF_FEU[MAXFEU];
MVT_NBR_OF_FEU[0] = 4;
MVT_NBR_OF_FEU[1] = 4;
*/

			else if(banktag[jj] == 0xe118) /* MVT hardware format */	// entering an MVT data bank
			{

#ifdef DEBUG7								
if( mvt_fptr_err_2 != (FILE *)NULL )
{
	fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118 \n", __FUNCTION__);
	fflush(mvt_fptr_err_2);
}
#endif

				banknum = iev; /*rol->pid;*/	// event number 
    				a_slot_old = -1;

//CPOPEN(0xe118,1,banknum);
				if ( (mvt_event_number % MVT_PRESCALE ) ||( MVT_PRESCALE == 1000000 ) )
				{
/*
//CPOPEN(0xefef,1,banknum);
					//	printf("0xe118 : event number %8x %8x %8x\n", mvt_event_number,mvt_event_number % MVT_PRESCALE,(mvt_event_number % MVT_PRESCALE ) ||( MVT_PRESCALE == 1000000 ) );
					*dataout ++ = 0xCAFEFADE;
					b08 += 4;
					for(ibl=0; ibl < MVT_NBR_OF_BEU; ibl++)
					{ 
						*dataout ++ = lenE[jj][ibl][iev];
						b08 += 4;
					}
					*dataout ++ = 0xDEADBEEF;
					b08 += 4;
//CPCLOSE;
*/
				}
				else // Real Data
				{		
					if (mvt_error_counter ==0)
					{	
					
					 	//output the FEU data samples 				
				 		for(ibl=0; ibl < MVT_NBR_OF_BEU; ibl++)
						{   							
							currentBeuId = ((( datain[ iB[jj][ibl] ] ) & 0x0000F000 ) >> 12 ) - 1;	

#ifdef DEBUG7								
if( mvt_fptr_err_2 != (FILE *)NULL )
{
	fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118, currentBeuId = %d \n", __FUNCTION__, currentBeuId);
	fflush(mvt_fptr_err_2);
}
#endif
	


							for (i_feu = 0; i_feu < MVT_NBR_OF_FEU[currentBeuId]; i_feu++)
							{
								// Treat the FEU if it has fired channels
								if( nbchannelsFEU[jj][ibl][iev][0][i_feu] > 0 )
								{
								  //	a_slot_old = -1;
									a_channel_old = -1;

									// CURRENT SLOT NUMBER built from beu id and feu link id
									ii = iFEU[jj][ibl][iev][0][i_feu];
									currentFeuId = ((( datain[ ii] ) & 0x00007C00 ) >> 10 );		
									a_slot= ((currentBeuId << 5) + currentFeuId  + 1 )& 0x000000FF ;

#ifdef DEBUG7								
if( mvt_fptr_err_2 != (FILE *)NULL )
{
	fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118, data[%d]=0x%08x currentFeuId = %d \n", __FUNCTION__, ii, datain[ ii], currentFeuId);
	fflush(mvt_fptr_err_2);
}
#endif

									// OUTPUT CURRENT TRIGGER/EVNET NUMBER
									//current_event_number_high =(( datain[ ( iSMP[jj][ibl][iev][0] + 2 ) ] ) & 0x00007FFC) >> 2;
									current_event_number_low  = ((( datain[ ( iSMP[jj][ibl][iev][0] + 2 ) ] ) & 0x00000003) << 30) + 
												    ((( datain[ ( iSMP[jj][ibl][iev][0] + 3 ) ] ) & 0x7FFF0000) >>  1) +
												     (( datain[ ( iSMP[jj][ibl][iev][0] + 3 ) ] ) & 0x00007FFF);

									a_triggernumber = current_event_number_low;
								
#ifdef DEBUG7								
if( mvt_fptr_err_2 != (FILE *)NULL )
{
	fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118, current_event_number_low = %d \n", __FUNCTION__, current_event_number_low);
	fflush(mvt_fptr_err_2);
}
#endif
									//OUTPUT CURRENT TIMESTAMP 
									current_timestamp_low  = ((( datain[ ( iSMP[jj][ibl][iev][0] + 2 ) ] )  & 0x7FFF0000) << 1) +
 											       (((~( datain[ ( iSMP[jj][ibl][iev][0]     ) ] )) & 0x00000800) << 5) ;
	
									current_timestamp_high = ((( datain[ ( iSMP[jj][ibl][iev][0] + 1 ) ] ) & 0x7FFF0000) >>  1) + 
											         ((( datain[ ( iSMP[jj][ibl][iev][0] + 1 ) ] ) & 0x00007FFF));		

									ii = iFEU[jj][ibl][iev][0][i_feu];
									current_feu_tmstmp   = current_timestamp_low +
											         ((( datain[ ii + 2] ) & 0x0FFF0000) >>  13 ) + (( datain[ ii + 2] ) & 0x00000007);
									current_feu_tmstmp |= ((MVT_ZS_MODE &0x1)<<15);
							        	a_trigtime[0] = current_timestamp_high;
             								a_trigtime[1] = current_feu_tmstmp;
#ifdef DEBUG7								
if( mvt_fptr_err_2 != (FILE *)NULL )
{
	fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118, current_timestamp_high = %08x current_feu_tmstmp = %08x \n", __FUNCTION__, current_event_number_low, current_feu_tmstmp);
	fflush(mvt_fptr_err_2);
}
#endif

MVT_CCOPEN(0xe11b,"c,i,l,N(s,Ns)",banknum);
									Nchan[0] = nbchannelsFEU[jj][ibl][iev][0][i_feu];
#ifdef DEBUG7								
if( mvt_fptr_err_2 != (FILE *)NULL )
{
	fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118, nbchannelsFEU = %d\n", __FUNCTION__, nbchannelsFEU[jj][ibl][iev][0][i_feu]);
	fflush(mvt_fptr_err_2);
}
#endif
									if (MVT_ZS_MODE)
									{
										for ( i_channel = 0; i_channel < nbchannelsFEU[jj][ibl][iev][0][i_feu] ; i_channel ++ ) 
										{
											//OUTPUT CHANNEL NUMBER
											ii = iFEU[jj][ibl][iev][0][i_feu] + 1 + 2 + i_channel;
											current_channel_id = ((( datain[ ii ] ) & 0x01FF0000) >> 16 ) + 1;  
											b16 = (unsigned short *)( b08 );
											*b16++ = current_channel_id;
											b08 += 2;
								
											//OUTPUT  NUMBER OF SAMPLES			
											b32 = (unsigned int *)( b08 );
											* b32 ++ = MVT_NBR_SAMPLES_PER_EVENT;
											b16+=2;
											b08+=4;

											for ( i_sample = 0; i_sample < MVT_NBR_SAMPLES_PER_EVENT; i_sample++)
											{		
												ii = iFEU[jj][ibl][iev][i_sample][i_feu] + 1 + 2 + i_channel;
												current_sample = ((( datain[ ii ] ) & 0x00000FFF) );
												*b16++ = current_sample;
												b08 += 2;
											}/* loop over samples */
										} /* loop over channels */
									}	
									else
									{
			 							for ( i_channel = 0; i_channel < nbchannelsFEU[jj][ibl][iev][0][i_feu] ; i_channel ++ ) 
										{
											//GET CURRENT DREAM ID
											i_dream = i_channel >> 6 ;
											// point to dream header for the current channel
											ii = iFEU[jj][ibl][iev][0][i_feu] + 1 + 2 + 1 + (32 + 5)*i_dream ;
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
#ifdef DEBUG7								
if( mvt_fptr_err_2 != (FILE *)NULL )
{
	fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118, i_dream = %d i_channel = %d , data[%d]=0x%08x current_channel_id =%d MVT_NBR_SAMPLES_PER_EVENT = %d \n", __FUNCTION__, i_dream, i_channel, ii, datain[ ii ], current_channel_id, MVT_NBR_SAMPLES_PER_EVENT);
	fflush(mvt_fptr_err_2);
}
#endif
											for ( i_sample = 0; i_sample < MVT_NBR_SAMPLES_PER_EVENT; i_sample++)
											{
											  // Point to the first channel of the Dream
											  // 1 for BEU extra word, 2 for FEU header + 2 for Dream header
											  ii = iFEU[jj][ibl][iev][i_sample][i_feu] + 1 + 2 + 2 + (32+5)*i_dream;
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
#ifdef DEBUG7								
if( mvt_fptr_err_2 != (FILE *)NULL )
{
	fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118, LOOP OVER SAMPLES DONE \n", __FUNCTION__);
	fflush(mvt_fptr_err_2);
}
#endif
										} //loop over channels
									} //end if ZS MODE
//if(b08 != NULL) CCCLOSE;
/*
#ifdef DEBUG7								
if( mvt_fptr_err_2 != (FILE *)NULL )
{
	fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118, CCCLOSE DONE \n", __FUNCTION__);
	fflush(mvt_fptr_err_2);
}
#endif
*/
								} // if( nbchannelsFEU[jj][ibl][iev][0][i_feu] > 0 ) do it if the feu has fired channels
							} // for (i_feu = 0; i_feu < MVT_NBR_OF_FEU[currentBeuId]; i_feu++) loop over feus
						} // for(ibl=0; ibl < MVT_NBR_OF_BEU; ibl++) loop over blocks */
if(b08 != NULL) CCCLOSE;
#ifdef DEBUG7								
if( mvt_fptr_err_2 != (FILE *)NULL )
{
	fprintf(mvt_fptr_err_2,"%s: SECOND PASS 0xe118, CCCLOSE DONE \n", __FUNCTION__);
	fflush(mvt_fptr_err_2);
}
#endif

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
//CPCLOSE;
			} /* else if(banktag[jj] == 0xe118) /* MVT hardware format */
//***********************************************************************************
//***********************************************************************************
//***********************************************************************************
		} /* loop over banks  */

#ifdef PASS_AS_IS
		/* if last event, loop over banks to be passed 'as is' and attach them to that last event */
		if(iev==(nnE-1))
		{
			for(ii=0; ii<nASIS; ii++)
			{
				jj = iASIS[ii]; /* bank number as it reported by BANKSCAN */
				datain = bankdata[jj];
				lenin  = banknw[jj];
				/*printf("mynev=%d: coping bank number %d (header %d 0x%08x))\n",mynev,jj,*(datain-2),*(datain-1));*/

				CPOPEN(banktag[jj],banktyp[jj],banknr[jj]);
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
		if(iev<(nnE-1))
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

	rol->user_storage[0] = lenout;

#ifdef SPLIT_BLOCKS
	rol->user_storage[1] = nnE; /* report the number of events */
#else
	CPEXIT;
	rol->user_storage[1] = 1; /* report the number of events */
#endif

/*	printf("return lenout=%d\n**********************\n\n",lenout);*/
/*	printf("--> %d %d\n",rol->user_storage[0],rol->user_storage[1]);*/


/*
  	// print output buffer
	dataout = (unsigned int *)(rol->dabufp);
	printf("lenout=%d\n",lenout);
	for(ii=0; ii<lenout; ii++)
		printf("  DATA [%3d] 0x%08x (%d)\n",ii,dataout[ii],dataout[ii]);
*/

	return;
}

void
rol2trig_done()
{
	return;
}  


void
__done()
{
  /* from parser */
  poolEmpty = 0; /* global Done, Buffers have been freed */

	return;
}
  
static void
__status()
{
	return;
}  

