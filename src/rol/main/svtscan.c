
/* svtscan.c */
/*
./Linux_i686_vme/bin/svtscan 3 3 junk 25 4 0 3 0 127
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "vscmLib.h"


#ifdef Linux_vme

#include "jvme.h"

typedef struct thdata
{
  int board_id;
  char filename[256];
  int beg_chip;
  int end_chip;
  int beg_chan;
  int end_chan;
  int start_thr;
  int chan_mult;
} THDATA;


/* prototype for thread routine */
void pthread_function( void *ptr );


int
main(int argc, char *argv[])
{
  int board_id, start_slot, end_slot;
  char filename[256];
  int beg_chip, end_chip, beg_chan, end_chan;
  int start_thr, chan_mult;
  int ii, jj, nvscm1;

  if(argc!=6 && argc!=8 && argc!=10)
  {
    printf("usage: svtscan <start slot#> <end slot#> <output file name> <start threshold> <channel multiplicity> [beg_chip(0-7) end_chip(0-7)] [beg_chan(0-127) end_chan(0-127)]\n");
    exit(0);
  }
  else
  {
    start_slot = atoi(argv[1]);
    end_slot = atoi(argv[2]);
    strncpy(filename, argv[3], 255);
    start_thr = atoi(argv[4]);
    chan_mult = atoi(argv[5]);
    printf("use arguments: %d %d >%s< %d %d\n",start_slot,end_slot,filename,start_thr,chan_mult);

    beg_chip = 0;
    end_chip = 3;
    beg_chan = 0;
    end_chan = 127;

    if(argc==8||argc==10)
	{
      beg_chip = atoi(argv[6]);
      end_chip = atoi(argv[7]);
    }

    if(argc==10)
	{
      beg_chan = atoi(argv[8]);
      end_chan = atoi(argv[9]);
    }

  }

  /* Open the default VME windows */
  vmeOpenDefaultWindows();

  /* initialize VSCM board(s) */
  nvscm1 = vscmInit((unsigned int)(3<<19),(1<<19),21,1);

  vscmPrestart("clasdev.cnf");
  for(ii=0; ii<nvscm1; ii++)
  {
    for(jj=0; jj<8; jj++)
    {
      fssrSCR(vscmSlot(ii), jj);
	}
  }

  if(start_slot == end_slot)
  {
    board_id = start_slot;
    fssrGainScan(board_id, filename, beg_chip, end_chip, beg_chan, end_chan, start_thr, chan_mult);
//    fssrThresholdScan(board_id, NULL, beg_chip, end_chip, beg_chan, end_chan, start_thr);
  }
  else
  {
    int id;
    pthread_t pthread_id[VSCM_MAX_BOARDS+1];
    pthread_attr_t attr;
    THDATA data[VSCM_MAX_BOARDS+1];
    
    pthread_attr_init(&attr); /* initialize attr with default attributes */
	/* pthread_join() will not work with following attributes, must sleep until threads are finished
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	*/

    for(ii=0; ii<end_slot-start_slot+1; ii++)
	{
      data[ii].board_id = start_slot + ii;
      strcpy(data[ii].filename,filename);
      data[ii].beg_chip = beg_chip;
      data[ii].end_chip = end_chip;
      data[ii].beg_chan = beg_chan;
      data[ii].end_chan = end_chan;
      data[ii].start_thr = start_thr;
      data[ii].chan_mult = chan_mult;

      pthread_create(&pthread_id[ii], &attr, (void *) &pthread_function, (void *)&data[ii]);
	}

    /* make sure main program exits only after thread is finished */
    for(ii=0; ii<end_slot-start_slot+1; ii++)
	{
      pthread_join(pthread_id[ii], NULL);
	} 

  }

  exit(0);
}

void
pthread_function( void *ptr )
{
  int board_id, start_slot, end_slot;
  char filename[256];
  int beg_chip, end_chip, beg_chan, end_chan;
  int start_thr, chan_mult;

  THDATA *data;            
  data = (THDATA *) ptr;

  printf("Starting scan for slot %d\n",data->board_id);
  fssrGainScan(data->board_id, data->filename, data->beg_chip, data->end_chip,
               data->beg_chan, data->end_chan, data->start_thr, data->chan_mult);
  printf("Finished scan for slot %d\n",data->board_id);
    
  pthread_exit(0);
}

#else

int
main()
{
  return(0);
}

#endif
