
/* marocinit.c */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "marocLib.h"
#include "marocConfig.h"

#define SCALERS_PRINT             1
#define EVENT_DATA_PRINT          1
#define EVENT_STAT_PRINT          1

#define EVENT_BUFFER_NWORDS       1024

static unsigned long long nwords_current = 0;
static unsigned long long nwords_total = 0;
static unsigned long long nevents_current = 0;
static unsigned long long nevents_total = 0;

static int nmaroc;

void scaler_print()
{
  unsigned int scalers[192];
  unsigned int pixels[192];
  int i, j, slot;

  for(i=0; i<nmaroc; i++)
  {
    slot = marocSlot(i);

    printf("%s - SLOT=%d\n", __func__, slot);

    maroc_get_scalers(slot, scalers, 1);

    for(i=0;i<192;i++)
    {
      int asic=i/64;
      int maroc_ch=i%64;
      int pixel = asic*64+maroc_ch_to_pixel[maroc_ch];
      pixels[pixel] = scalers[i];
    }

    for(i=0;i<8;i++)
    {
      for(j=0;j<24;j++)
      {
        int pmt = j/8;
        int col = j%8;
        int row = i;
        printf("%7u",pixels[64*pmt+8*row+col]);

        if(col==7 && pmt<2)
          printf(" | ");
      }
      printf("\n");
    }
    printf("\n");

    fflush(stdout);
  }
}

void event_stat_print(double diff)
{
  printf("Average bytes/sec     = %lf\n", (double)(nwords_current*4L)/diff);
  printf("Total bytes received  = %lf\n", (double)(nwords_total*4L)/diff);
  printf("Average events/sec    = %lf\n", (double)nevents_current/diff);
  printf("Total events received = %lf\n", (double)nevents_total/diff);
  printf("\n");

  nwords_total+= nwords_current;
  nevents_total+= nevents_current;

  nwords_current = 0;
  nevents_current = 0;
}

int main(int argc, char *argv[])
{
  int event_buffer[EVENT_BUFFER_NWORDS];
  int event_buffer_len;
  FILE *f_events = fopen("pmt_selftrigger_data.bin", "wb");
  time_t start, curr;
  double diff;

  printf("\nmarocinit started ..\n\n");fflush(stdout);

  nmaroc = marocInit(0, MAROC_MAX_NUM);

  printf("\nmarocinit: nmaroc=%d\n\n",nmaroc);fflush(stdout);
  if(nmaroc<=0) exit(0);

  marocInitGlobals();
  marocConfig("");
  printf("\nmaroc initialized\n\n");fflush(stdout);

exit(0);



  time(&start);

  while(1)
  {
    event_buffer_len = marocReadBlock(event_buffer, EVENT_BUFFER_NWORDS);

    if(event_buffer_len > 0)
    {
      fwrite(event_buffer, event_buffer_len, sizeof(event_buffer[0]), f_events);

      nwords_current+= event_buffer_len;

      nevents_current += maroc_process_buf(event_buffer, event_buffer_len, NULL, EVENT_DATA_PRINT);
    }
    else
    {
      usleep(1000);
    }

    time(&curr);
    diff = difftime(curr, start);
    if(diff > 1.0)
    {
#if EVENT_STAT_PRINT
      event_stat_print(diff);
#endif

#if SCALERS_PRINT
      scaler_print();
#endif
      time(&start);
    }
  }
  marocEnd();
  fclose(f_events);
  
  exit(0);
}

