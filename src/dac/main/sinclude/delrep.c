/*
   delrep.c - helicity delay reporting correction

   Author:      Sergey Boyarinov, boiarino@jlab.org

   Created:     Sep 23, 2000
   Last update: Jun 24, 2005

   ifdefs: L3LIB - for Level 3 Trigger
           MPROG - for multiprogram version (default is multithread version)

*/

#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <sys/times.h>
#include <limits.h>
#include <sys/statvfs.h>

#include "uthbook.h"
#include "et.h"
#include "etmacros.h"
#include "etbosio.h"
#include "etbosio.c"



#ifdef L3LIB
#ifdef DELREP /* delayed reporting */

#include "helicity.c"

#define FORCE_RECOVERY \
  forceRecovery(); \
  quad = -1; \
  strob = -1; \
  helicity = -1; \
  quad_old = -1; \
  strob_old = -1; \
  helicity_old = -1; \
  quad_old2 = -1; \
  strob_old2 = -1; \
  helicity_old2 = -1; \
  remember_helicity[0]=remember_helicity[1]=remember_helicity[2]=-1; \
  done = -1; /* will be change to '0' in the beginning of quartet and to '1' when prediction is ready */ \
  offset = 0

/*
#define DRDEBUG
*/

#endif
#endif






#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define ABS(x)   ((x) < 0 ? -(x) : (x))


typedef struct thread
{
  struct timespec   start;
  struct timespec   end;
} THREAD;

#ifndef MPROG
static MM *mm;
#else

#ifdef L3LIB
char *map_file_name = "/tmp/coda_l3.mapfile";
#endif

#endif

/* signal handler prototype */
static void *signal_thread (void *arg);

static int force_exit = 0;

/* input args */

char  project[1000];
char  session[1000];
char  unique_name[1000];
char  et_file_name[1000];

#ifdef L3LIB
int chunk = 100;
int nthreads=1;
char *et_station_name = "LEVEL3";
#endif

ETVARS1;

static int ntimeouts; /* the number of sequential ET timeouts before getting events */

#define MYGET \
  if(mm->nid > 0) \
  { \
    /* get 'myid' from the stack */ \
    myid = mm->idstack[--mm->nid]; \
    /*printf("[%1d] get myid=%d from idstack[%d]\n",threadid,myid,mm->nid);*/ \
    /* get the chunk of events; parameters go to ... */ \
    mm->attach1[myid] = attach; \
    ETGETEVENTS(mm->attach1[myid],mm->pe1[myid],mm->numread1[myid]); \
    mm->numread2[myid] = mm->numread3[myid] = 0; \
    mm->ready[myid] = 0; \
    /*if(mm->nfifo > 20) printf("[%1d] nfifo=%d numread=%d\n",threadid,mm->nfifo,mm->numread1[myid]);*/ \
    /* put 'myid' into fifo */ \
    if(mm->nfifo < NFIFOMAX) \
    { \
      mm->nfifo ++; \
      for(i=mm->nfifo-1; i>0; i--) mm->fifo[i] = mm->fifo[i-1]; \
      mm->fifo[0] = myid; \
    } \
    else \
    { \
      printf("[%1d] ERROR: no place in fifo, nfifo=%d\n",threadid,mm->nfifo); \
      printf("[%1d] fifo-> %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d\n",threadid, \
        mm->fifo[0],mm->fifo[1],mm->fifo[2],mm->fifo[3],mm->fifo[4], \
        mm->fifo[5],mm->fifo[6],mm->fifo[7],mm->fifo[8],mm->fifo[9],mm->fifo[10],mm->fifo[11]); \
      exit(0); \
    } \
  } \
  else \
  { \
    printf("[%1d] ERROR: no myid's in idstack, nid=%d\n",threadid,mm->nid); \
    exit(0); \
  }


void *
l3_thread(void *arg)
{
#ifdef L3LIB
  int cand, ind_tgbi;
  int oldevnum;

#ifdef L3NOTHREADS
  UThistf *histf;
  float l1ungated[12], l1ungated_old[12], l1ungated_diff[12];
  float l1gated[12], l1gated_old[12], l1gated_diff[12];
  float l2gated[6], l2gated_old[6], l2gated_diff[6];
  float l2fpsc[4], l2fpsc_old[4], l2fpsc_diff[4];
  float clock, clock_old, clock_diff;
#endif

#ifdef DELREP
  int ind10, ind11, ind12, ind13, ncol, nrow;
  unsigned int hel,count1,str,strob,helicity,quad;
  unsigned int strob_old,helicity_old,quad_old,strob_old2,helicity_old2,quad_old2;
  unsigned int helicity1, strob1, quad1, offset, quadextr[4];
  int done;
  int present_reading;
  int skip = 0;
  int final_helicity;
  int remember_helicity[3];

  int predicted_reading;      /* prediction of present reading */
  int present_helicity;       /* present helicity (using prediction) */
  unsigned int iseed;         /* value of iseed for present_helicity */
  unsigned int iseed_earlier; /* iseed for predicted_reading */

  /* following for 1 flip correction */
  unsigned int offset1;
  int pred_read;
  int pres_heli;
  int tmp0, temp0;
#endif
#endif


  ETSYS *sysptr;
  et_att_id  attach;
  struct timespec timeout;
  int lock, keep_event;
  int nevents1_chunk, nevents2_chunk; /* local event counters for one chunk */

  int i, threadid, myid, hisid, mynev, mynev0, ififo, tmp;
  float fnev;
  const int forever = 1;
  int numread, status, j, jj, ind, *jw, len, ntrk, size;

#ifdef MPROG
  /* map */
  MM *mm;
  tmp = mm_mem_create(map_file_name,sizeof(MM),(void **) &mm, &size);
  if(tmp == ET_ERROR_EXISTS)
  {
    mm = (MM *) mm_mem_attach(map_file_name,sizeof(MM));
  }
  else
  {
    printf("mm_mem_create returns %d\n",tmp);
    exit(0);
  }
#endif


#ifdef L3LIB

#ifdef DELREP
  FORCE_RECOVERY;
#endif

#ifdef L3NOTHREADS
  histf = (UThistf *) malloc(NHIST*sizeof(UThistf));
  printf("coda_l3: histf ->%08x\n",histf);  /* to be filled by 'coda_l3' */

  uthbook1(histf, 32, "EVELB 1L STIAGNU DET", 12, 0., 12.);
  uthbook1(histf, 33, "EVELB 1L STIETAG   D", 12, 0., 12.);
  uthbook1(histf, 34, "EVELB 2L STIETAG   D", 6, 0., 6.);
  uthbook1(histf, 35, "F 2L LIASSAPATS C TRRAEL", 4, 0., 4.);
  /*
  uthbook1(histf, 36, " ", 6, 0., 6.);
  uthbook1(histf, 37, " ", 6, 0., 6.);
  uthbook1(histf, 38, " ", 6, 0., 6.);
  uthbook1(histf, 39, " ", 6, 0., 6.);
  */
#endif
#endif

  /* space allocated in main() */


  /* set 'threadid' */
  threadid = ++ mm->nthread;
  printf("[%1d] set 'threadid'\n",threadid);

  ETATTACH(attach); /* using local 'attach' */

  /* lock input */
  printf("[%1d] 0: nfifo=%d fifo=%d %d %d %d\n",
     threadid,mm->nfifo,mm->fifo[0],mm->fifo[1],mm->fifo[2],mm->fifo[3]);

  pthread_mutex_lock(&mm->mutex_fifo);
  printf("[%1d] 1: nfifo=%d fifo=%d %d %d %d\n",
     threadid,mm->nfifo,mm->fifo[0],mm->fifo[1],mm->fifo[2],mm->fifo[3]);

  MYGET;
  mm->nevents += mm->numread1[myid];
  mm->Nevents += mm->numread1[myid];
  mm->time0 = time(NULL);

  pthread_mutex_unlock(&mm->mutex_fifo);

  while(forever)
  {
    if(mm->nfifo>=NFIFOMAX || mm->nid>=NIDMAX || mm->nfifo<0 || mm->nid<0)
    {
      printf("ERROR: [%1d] nfifo=%d nid=%d\n",threadid,mm->nfifo,mm->nid);
      fflush(stdout);
    }
    nevents1_chunk = nevents2_chunk = 0;
#ifdef ERLIB
    stat4 = stat5 = 0;
#endif
    /* loop over all events in chunk */
    for(jj=0; jj<mm->numread1[myid]; jj++)
    {
      keep_event = 1; /* keep event by default */
      ETGETDATA(mm->pe1[myid]);
	  /*printf("local no. %d\n",jj);*/

      if((ind = etNlink(jw,"HEAD",0)) != 0)
      {
		/*printf("event no. %d\n",jw[ind+2]);*/

        if(jw[ind+4] == 10) /* scaler event */
        {
          ;
        }
        else if(jw[ind+4] < 10) /* physics event */
        {
          if(mynev == 0)
          {
            mynev ++;
            mynev0 = jw[ind+2] - 1;
          }
          else
          {
            mynev ++;
            /*printf("mynev=%d mynev0=%d event#=%d\n",mynev,mynev0,jw[ind+2]);*/
            if((jw[ind+2]-mynev0)!=0) fnev=((float)mynev/((float)(jw[ind+2]-mynev0)))*100.;
          }

          /*printf("===> got %d(%6.2f)%% events\n",mynev,fnev);*/
          if(jw[ind+6] == 17)
          {
            printf("Prestart ..\n"); fflush(stdout);
            mynev = 0;
            mm->nevents = mm->nevents1 = mm->nevents2 = 0;
            mm->Nevents = mm->Nevents1 = mm->Nevents2 = 0;
            for(i=0; i<8; i++)
            {
              mm->Count[i] = 0;
              mm->Ratio[i] = -1.0;
            }
#ifdef L3LIB

#ifdef L3NOTHREADS
            for(i=0; i<12; i++)
			{
              l1ungated_old[i] = 0.0;
              l1gated_old[i] = 0.0;
			}
            for(i=0; i<6; i++)
			{
              l2gated_old[i] = 0.0;
			}
            for(i=0; i<4; i++)
			{
              l2fpsc_old[i] = 0.0;
			}
			clock_old = 0.0;
#endif

            /* event order check */
            if((oldevnum+1) != jw[ind+2])
            {
              printf("l3 error: event no %d, old no %d\n",jw[ind+2],oldevnum);
              printf("prestart: event type = %d\n",jw[ind+4]); fflush(stdout);
            }
            oldevnum = jw[ind+2];
#endif
          }
          else if(jw[ind+6] == 18)
          {
            printf("Go ..\n"); fflush(stdout);
#ifdef L3LIB

            /* event order check */
            if((oldevnum+1) != jw[ind+2])
            {
              printf("l3 error: event no %d, old no %d\n",jw[ind+2],oldevnum);
              printf("go: event type = %d\n",jw[ind+4]); fflush(stdout);
            }
            oldevnum = jw[ind+2];
#endif
          }
          else if(jw[ind+6] == 20)
          {
            printf("End ..\n"); fflush(stdout);
#ifdef L3LIB
            oldevnum = 0;
#endif
          }
#ifdef L3LIB
          else
          {
            /* events order check */
            if((oldevnum+1) != jw[ind+2])
            {
              printf("l3 error: event no %d, old no %d\n",jw[ind+2],oldevnum);
              printf("event: event type = %d (coda type = %d)\n",jw[ind+4],jw[ind+6]); fflush(stdout);
            }
            oldevnum = jw[ind+2];
          }
#endif

          if(mm->runnum != jw[ind+1])
          {
            pthread_mutex_lock(&mm->newmap);
            if(mm->runnum != jw[ind+1]) /* check if it is done already by another thread */
            {
              printf("[%1d] current run number = %d\n", threadid,mm->runnum);
                fflush(stdout);
              mm->runnum = jw[ind+1];
              printf("[%1d] set run number = %d\n",threadid,mm->runnum);
                fflush(stdout);
              ;
            }
            pthread_mutex_unlock(&mm->newmap);
          }


          /* data analysis */

/* about 1kHz on mizar */
/*for(i=0; i<10000; i++) i=i;*/



#ifdef L3LIB

          /* delayed reporting */
#ifdef DELREP
          {
            ind11 = etNlink(jw,"HEAD",0);

            if((ind12 = etNlink(jw,"TGBI",0))>0)
            {
              quad = (jw[ind12]&0x00001000)>>12;
              strob = (jw[ind12]&0x00004000)>>14;
              helicity = (jw[ind12]&0x00008000)>>15;
	        }
            else
	        {
              printf("DELREP ERROR: TGBI\n");
	        }

            if((ind10=etNlink(jw,"HLS ",1))>0)
            {
              /*printf("===== event %8d type# %2d\n",iev,jw[ind11+4]);*/
              ncol = etNcol(jw,ind10);
              nrow = etNrow(jw,ind10);
	          /*printf("ncol=%d nrow=%d\n",ncol,nrow);*/
              for(i=0; i<nrow; i++)
              {
                str = (jw[ind10+i*ncol]&0x80000000)>>31;
                hel = (jw[ind10+i*ncol]&0x40000000)>>30;
                count1 = jw[ind10+i*ncol]&0xFFFFFF;

                if(count1>10) /* ignore 500us intervals */
                {
#ifdef DRDEBUG		  
                  printf("event# %8d type# %2d row# %1d count=%7d: str=%1d hel=%1d -> ",
                    jw[ind11+2],jw[ind11+4],i,count1,str,hel);
#endif
                  if(ind12>0)
		          {
                    if((strob == str) && (helicity == hel))
                    {
#ifdef DRDEBUG		  
                      printf("strob=%1d helicity=%1d (current ) [%1d]\n",
                        strob,helicity,quad);
#endif
                      helicity1 = helicity;
                      strob1 = strob;
                      quad1 = quad;
                    }
                    else if((strob_old == str) && (helicity_old == hel))
                    {
#ifdef DRDEBUG		  
                      printf("strob=%1d helicity=%1d (previous) [%1d]\n",
                        strob_old,helicity_old,quad_old);
#endif
                      helicity1 = helicity_old;
                      strob1 = strob_old;
                      quad1 = quad_old;
                    }
                    else if((strob_old2 == str) && (helicity_old2 == hel))
                    {
#ifdef DRDEBUG		  
                      printf("strob=%1d helicity=%1d (prepre--) [%1d]\n",
                        strob_old2,helicity_old2,quad_old2);
#endif
                      helicity1 = helicity_old2;
                      strob1 = strob_old2;
                      quad1 = quad_old2;
                    }
                    else
                    {
                      printf("strob=%1d helicity=%1d (err =========) [%1d]\n",
                        strob,helicity,quad);
                      helicity1 = hel;   /* from HLS */
                      strob1 = str;      /* from HLS */
                      quad1 = quad_old2; /* ??? */
                      FORCE_RECOVERY;
                    }


                    /*flip helicity if necessary */
                    helicity1 ^= 1;

                    if(done==1)
                    {
                      if(quad1==0)
                      {

                        /* if we are here, 'offset' must be 3, otherwise most likely
                        we've got unexpected quad1==0; we'll try to set quad1=1 and
                        see what happens */
                        if(offset<3)
                        {
                          printf("ERROR: unexpected offset=%d - trying to recover --------------------\n",offset);
                          quad1 = 1;
                          offset++;
	                      /*FORCE_RECOVERY;*/
	                    }
                        else if(offset>3)
                        {
                          printf("ERROR: unexpected offset=%d - forcing recovery 1 -------------------\n",offset);
	                      FORCE_RECOVERY;
                        }
                        else
                        {
                          offset=0;
	                    }
                      }
                      else
                      {
                        offset++;
                        if(offset>3)
                        {
                          /* if we are here, 'offset' must be <=3, otherwise most likely
                          we've got unexpected quad1==1; we'll try to set quad1=0, offset=0
                          and see what happens */
                          printf("ERROR: unexpected offset=%d - forcing recovery 2 -------------------\n",offset);
                          quad1 = 0;
                          offset = 0;
	                      /*FORCE_RECOVERY;*/
                        }
                      }
                    }

#ifdef DRDEBUG
                    printf("quad=%1d(%1d) hel=%1d\n",quad1,offset,helicity1);
#endif

                    /* looking for the beginning of quartet; set done=0 when found */
                    if(done==-1)
                    {
                      printf("looking for the begining of quartet ..\n");
                      if(quad1==0)
                      {
                        done=0; /* quad1==0 means first in quartet */
                        printf("found the begining of quartet !\n");
	                  }
                    }

                    /* search for pattern: use first 24 quartets (not flips!) to get all necessary info */
                    if(done==0)
                    {
                      if(quad1==0)
	                  {
                        done = loadHelicity(helicity1, &iseed, &iseed_earlier);
                        printf("loaded one, done=%d\n",done);
                      }

                      if(done==1)
                      {
                        printf("=============== READY TO PREDICT (ev=%6d) =============== \n",jw[ind11+2]);
	                  }
                    }



                    /* pattern is found, can determine helicity */
                    if(done==1)
                    {
                      if(quad1==0)
                      {
                        /* following two must be the same */
                        present_reading = helicity1;
                        predicted_reading = ranBit(&iseed_earlier);

                        present_helicity = ranBit(&iseed);
	        
#ifdef DRDEBUG		  
                        printf("helicity: predicted=%d reading=%d corrected=%d\n",
                          predicted_reading,present_reading,present_helicity);
#endif


                        /****************/
                        /****************/
                        /* direct check */
                        remember_helicity[0] = remember_helicity[1];
                        remember_helicity[1] = remember_helicity[2];
                        remember_helicity[2] = present_helicity;
#ifdef DEBUG		
                        printf("============================== %1d %1d\n",remember_helicity[0],present_reading);
#endif
                        if( remember_helicity[0] != -1 )
				        {
                          if( remember_helicity[0] != present_reading )
                          {
                            printf("ERROR: direct check failed !!! %1d %1d\n",remember_helicity[0],present_reading);
                            FORCE_RECOVERY;
                          }
			            }
                        /****************/
                        /****************/



                        if(predicted_reading != present_reading)
                        {
                          printf("ERROR !!!!!!!!! predicted_reading != present_reading\n");
                          FORCE_RECOVERY;
                        }
                      }
                    }
	              }
                  else
                  {
                    printf("ERROR: TGBI bank does not exist !!!!! - exit\n");
                    exit(0);
                  }
		        }
              }
            }


            /*******************************/
            /* we are here for every event */

            if(done==1)
            {
              /* first check, if we are still in sync */
              tmp0 = helicity; /* helicity from current event */

              /* 1 flip correction */
              if(offset<3)
              {
                offset1 = offset+1;
                pred_read = predicted_reading;
                pres_heli = present_helicity;
	          }
              else
              {
                offset1 = 0;
                pred_read = ranBit0(&iseed_earlier);
                pres_heli = ranBit0(&iseed);
              }

	  
              if((offset1==0) || (offset1==3))
              {
                temp0 = pred_read^1;
                final_helicity = pres_heli;
              }
              else if((offset1==1) || (offset1==2))
              {
                temp0 = pred_read;
                final_helicity = pres_heli^1;
              }
              else
              {
                printf("ERROR: illegal offset=%d at temp0\n",offset1);
                FORCE_RECOVERY;
              }
              final_helicity ^= 1; /* flip it back */
	  

#ifdef DEBUG
              printf("tmp0=%d temp0=%d final=%d\n",tmp0,temp0,final_helicity);
#endif  

              if(tmp0 != temp0)
	          {
                printf("ERROR: ev %6d: reading=%d predicted=%d\n",
                  jw[ind11+2],tmp0,temp0);
	          }


              /* update helicity info in databank */
              if((ind13=etNlink(jw,"RC26",0))>0)
              {
                unsigned int tmp;
#ifdef DRDEBUG
                printf("befor: 0x%08x (event=%d)\n",jw[ind13],jw[ind11+2]);
#endif

                tmp = jw[ind13];
                if(final_helicity==1) tmp = tmp | 0x00008000; /* set helicity */
                if(strob1==1) tmp = tmp | 0x00004000; /* set strob */
                if(quad1==1) tmp = tmp | 0x00001000; /* set quad */
                tmp = tmp | 0x80000000; /* always set signature bit */
                jw[ind13] = tmp;
#ifdef DRDEBUG
                printf("after: 0x%08x (event=%d)\n",jw[ind13],jw[ind11+2]);
#endif

	          }
              else
	          {
                printf("ERROR: no RC26 bank !!!\n");
	          }
	        }



            /*******************************/
            /*******************************/


            if(ind12>0)
	        {
              quad_old2 = quad_old;
              strob_old2 = strob_old;
              helicity_old2 = helicity_old;

              quad_old = quad;
              strob_old = strob;
              helicity_old = helicity;
	        }

          }

#endif /* DELREP */


#endif




        } /* event type from HEAD bank */


      } /* if((ind = etNlink(jw,"HEAD",0)) != 0)*/






      /* put event into 'dump' or 'put' lists */
	  if(keep_event == 0)
      {
        mm->pe3[myid][mm->numread3[myid]++] = mm->pe1[myid][jj]; /* dump list*/
      }
	  else
      {
        mm->pe2[myid][mm->numread2[myid]++] = mm->pe1[myid][jj]; /* put list */
      }




      ETPUTDATA(mm->pe1[myid]); /* call it for accepted and rejected events */
    }



	/*printf("[%1d] %d %d\n",threadid,nevents1_chunk,nevents2_chunk);*/

    /* put events back */

    pthread_mutex_lock(&mm->mutex_fifo);

    {
      int time1;
      float tmp, tmp1, tmp2, tmp3, tmp4;

      time1 = time(NULL) - mm->time0;
      mm->nevents1 += nevents1_chunk;
      mm->nevents2 += nevents2_chunk;
      mm->Nevents1 += nevents1_chunk;
      mm->Nevents2 += nevents2_chunk;
      if(time1 > 60)
      {
        tmp  = mm->Rate  = ((float)mm->nevents)/((float)time1);
        tmp1 = mm->Rate1 = ((float)mm->nevents1)/((float)time1);
        tmp2 = mm->Rate2 = ((float)mm->nevents2)/((float)time1);
/*temporary*/
tmp2 = mm->Rate2 = tmp;
/*temporary*/
        tmp3 = tmp1/tmp*100.;
        tmp4 = tmp2/tmp1*100.;
        /*
        printf("\nEvent rate: overall %4.0f Hz\n"
                 "            accepted  %4.0f Hz [%4.1f %% from overall]\n"
                 "            processed %4.0f Hz [%4.1f %% from accepted]\n",
          tmp,tmp1,tmp3,tmp2,tmp4);
		*/
        mm->time0 = time(NULL);
        mm->nevents = mm->nevents1 = mm->nevents2 = 0;


#ifdef L3LIB
#ifdef L3NOTHREADS
        /* if some events were obtained then get again
        last accepted event from last chunk and put stat bank there */
		
        if(mm->numread2[myid] > 0)
        {
          ETGETDATALAST(mm->pe2[myid]);

          etNformat(jw,"HISF","F");

printf("put 32\n");
          uth2bos(histf,32,(long *)jw);
          uth2bos(histf,33,(long *)jw);
          uth2bos(histf,34,(long *)jw);
          uth2bos(histf,35,(long *)jw);

          ETPUTDATALAST(mm->pe2[myid]);
        }
		
#endif
#endif

      }

    }




	/*printf("10: threadid=%d fifo=%d %d %d %d\n",
	  threadid,mm->fifo[0],mm->fifo[1],mm->fifo[2],mm->fifo[3]); fflush(stdout);*/
	/*printf("MYTURN?: threadid=%d myid=%d nfifo=%d\n",threadid,myid,mm->nfifo); fflush(stdout);*/
    if(mm->fifo[mm->nfifo-1] == myid) /* my turn */
    {
	  /*printf("MYTURN: threadid=%d myid=%d fifo=%d\n",threadid,myid,mm->fifo[mm->nfifo-1]); fflush(stdout);*/

if(mm->numread1[myid]!=(mm->numread2[myid]+mm->numread3[myid]))
printf("coda_prtr: ERROR in put-dump 1: %d! = (%d+%d)\n",
mm->numread1[myid],mm->numread2[myid],mm->numread3[myid]);
      ETPUTEVENTS(mm->attach1[myid],mm->pe2[myid],mm->numread2[myid]);
      ETDUMPEVENTS(mm->attach1[myid],mm->pe3[myid],mm->numread3[myid]);
      /*printf("threadid=%d myid=%d -> puts back 1.\n",threadid,myid); fflush(stdout);*/

      mm->numread1[myid] = 0;
      if(mm->nfifo>0){mm->nfifo--; mm->fifo[mm->nfifo]=0;}else{printf("ERROR1\n");exit(0);}
      /* put 'myid' back to stack */
      if(mm->nid < NIDMAX)
      {
        mm->idstack[mm->nid++] = myid;
      }
      /* check if somebody is waiting to put events */
      while(mm->nfifo > 0)
	  {
        hisid = mm->fifo[mm->nfifo-1];
		/*printf("threadid=%d hisid=%d -> puts back ???\n",threadid,hisid); fflush(stdout);*/
        if(mm->ready[hisid] == 0) break;

if(mm->numread1[hisid]!=(mm->numread2[hisid]+mm->numread3[hisid]))
printf("coda_prtr: ERROR in put-dump 2: %d! = (%d+%d)\n",
mm->numread1[hisid],mm->numread2[hisid],mm->numread3[hisid]);
        ETPUTEVENTS(mm->attach1[hisid],mm->pe2[hisid],mm->numread2[hisid]);
        ETDUMPEVENTS(mm->attach1[hisid],mm->pe3[hisid],mm->numread3[hisid]);
        /*printf("threadid=%d myid=%d -> puts back 2.\n",threadid,myid); fflush(stdout);*/

        mm->numread1[hisid] = 0;
        if(mm->nfifo>0){mm->nfifo--; mm->fifo[mm->nfifo]=0;}else{printf("ERROR1\n");exit(0);}
        /* put 'hisid' back to stack */
        if(mm->nid < NIDMAX)
        {
          mm->idstack[mm->nid++] = hisid;
        }
	  }
    }
    else /* not my turn */
	{
	  /*printf("NOT MYTURN: threadid=%d myid=%d lastfifo=%d\n",threadid,myid,mm->fifo[mm->nfifo-1]); fflush(stdout);*/
      mm->ready[myid] = 1;

      if(force_exit)
      {
        /* wait until all 'my' chunks will be released; otherwise
           after we will detach they can not be released !!! */
        int remains;
        pthread_mutex_unlock(&mm->mutex_fifo);
        do
        {
          remains = 0;
          sleep(1);
          pthread_mutex_lock(&mm->mutex_fifo);
          for(i=0; i<mm->nfifo;i++)
          {
            if(mm->attach1[mm->fifo[i]] == attach)
            {
              printf("[%1d] chunk attached to 0x%08x remains - waiting ...\n",
                        threadid,attach); 
              remains = 1;
              break;
            }
          }
          pthread_mutex_unlock(&mm->mutex_fifo);
        } while(remains);
	  }

    }

    if(force_exit)
    {
      /* lock, cleanup, unlock, detach and exit */
      /*pthread_mutex_lock(&mm->mutex_fifo);*/
      /* return 'threadid'
      threadid = ++ mm->nthread;
      printf("[%1d] return 'threadid'\n",threadid);*/
      pthread_mutex_unlock(&mm->mutex_fifo);
#ifdef MPROG
      tmp = mm_mem_unmap(map_file_name, mm);
      if(tmp != 0)
      {
        printf("coda_tr: ERROR: mm_mem_unmap() returns %d\n",tmp);
      }
      else
      {
        printf("coda_tr: file >%s< unmapped successfully\n",map_file_name);
      }
#endif
      break;
    }

    MYGET;
    mm->nevents += mm->numread1[myid];
    mm->Nevents += mm->numread1[myid];

    pthread_mutex_unlock(&mm->mutex_fifo);

  } /* end forever loop */

  ETDETACH;

  return;
}







#if 0

/* main function */

main(int argc, char **argv)
{
  sigset_t  sigblock;
  pthread_t tid;

  pthread_t idth[NTHREADMAX];
  THREAD stats[NTHREADMAX];
  pthread_mutexattr_t mattr;
  int status,i,j,ith,tmp,size;
  struct tms t1,t2;
  double ddd, utime;

#ifdef MPROG
  MM *mm;
#endif


  if(argc > 1 && !strncmp("-h",argv[1],2))
  {
#ifdef L3LIB
    printf("Usage: coda_l3 -a <project> -s <session>\n");
#endif
    printf("       If no parameters specified 'sertest' will be used\n");
    exit(0);
  }

  /* default settings */
  strcpy(project,"sertest");
  strcpy(session,"sertest");
  strcpy(et_file_name,"/tmp/et_sys_sertest");

  /* change defaults if parameters are specified */
  for(i=1; i<argc; i+=2)
  {
    if(!strncmp("-a",argv[i],2) && argc > i+1)
    {
      strcpy(project,argv[i+1]);
    }
    else if(!strncmp("-s",argv[i],2) && argc > i+1)
    {
      strcpy(session,argv[i+1]);
      strcpy(et_file_name,"/tmp/et_sys_");
      strcat(et_file_name,session);
    }
  }

  if(chunk > NCHUNKMAX) chunk = NCHUNKMAX;
  if(nthreads > NTHREADMAX)
  {
    printf("coda_prtr: maximum number of threads is %d\n",NTHREADMAX);
    nthreads = NTHREADMAX;
  }

  printf("coda_prtr:  project >%s<   session >%s<   ET file >%s<\n",
    project,session,et_file_name);
  printf("            chunk size = %d   nthreads = %d\n",chunk,nthreads);


  /* setup signal handling */
  sigfillset(&sigblock);
  /* block all signals */
  tmp = pthread_sigmask(SIG_BLOCK, &sigblock, NULL);
  if(tmp != 0)
  {
    printf("coda_prtr: pthread_sigmask failure\n");
    exit(1);
  }
#ifdef SunOS
  thr_setconcurrency(thr_getconcurrency() + 1);
#endif
  /* spawn signal handling thread */
  pthread_create(&tid, NULL, signal_thread, (void *)NULL);



  /* allocate and initialize mm */
#ifdef MPROG
  tmp = mm_mem_create(map_file_name,sizeof(MM),(void **) &mm, &size);
  if (tmp == ET_OK)
#else
  size = sizeof(MM);
  if((mm = (MM *) calloc(1,sizeof(MM))) != NULL)
#endif
  {
    printf("coda_prtr: allocate and initialize new mm at 0x%08x\n",mm);

    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&(mm->newmap), &mattr);
    pthread_mutex_init(&(mm->mutex_fifo), &mattr);

    mm->totalsize = size;

    mm->nid = NIDMAX;
    for(i=0; i<NIDMAX; i++) mm->idstack[i] = i;

    mm->nfifo = 0;
    for(i=0; i<NFIFOMAX; i++) mm->fifo[i] = 0;

    mm->runnum = 0;
    mm->nthread = 0;
    mm->nevents = mm->nevents1 = mm->nevents2 = 0;
    mm->Nevents = mm->Nevents1 = mm->Nevents2 = 0;
    for(i=0; i<8; i++)
    {
      mm->Count[i] = 0;
      mm->Ratio[i] = -1.0;
    }

#ifdef TRLIB
	
    dcstatinit(&mm->trstat);
	
#endif
  }
#ifdef MPROG
  else if(tmp == ET_ERROR_EXISTS)
  {
    mm = (MM *) mm_mem_attach(map_file_name,sizeof(MM));
    printf("coda_prtr: memory exist at 0x%08x, nid=%d\n",mm,mm->nid);
  }
#endif
  else
  {
    printf("coda_prtr: malloc returns %d\n",mm);
    exit(0);
  }


  printf("coda_prtr starts, et_file_name=>%s< chunk=%d nthreads=%d\n",
              et_file_name,chunk,nthreads);

  /* open/create ET station: ETOPEN - non-blocking, ETOPENB - blocking */
#ifdef L3LIB
  ETOPENB;
#endif

  /* initialization */

#ifdef MPROG
  l3_thread((void *)&stats[ith]);
#else
  for(ith=0; ith<nthreads; ith++)
  {

    if(pthread_create(&idth[ith], NULL, l3_thread, (void *)&stats[ith]) != 0)
    {
      printf("coda_prtr: pthread_create(0x%08x[%d],...) failure\n",idth[ith],ith);
      return(-1);
    }
    else
	{
      printf("coda_prtr: pthread_create(0x%08x[%d],...) done\n",idth[ith],ith);
    }
  }
#endif



  while(1)
  {
    if(force_exit)
    {
      printf("coda_prtr: exit main() function\n");
      break;
    }
  }


  /* remove ET station 
  ETCLOSE;*/

  printf("coda_prtr done.\n");
  exit(0);
}

#endif /* if 0 */



/************************************************************/
/*              separate thread to handle signals           */
/************************************************************/

static void *
signal_thread (void *arg)
{
  int status;
  sigset_t   signal_set;
  int        sig_number;

  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGINT);
  
  sigwait(&signal_set, &sig_number);
  printf("coda_prtr: got a control-C, exiting ...\n");

  /* set condition to clean up from the everything
     related to the current program */
  force_exit = 1;  
  sleep(1);

  return;
}


