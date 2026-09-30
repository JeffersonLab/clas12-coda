
/* mpddec.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "mpddec.h"
#include <evio.h>
#include <evioBankUtil.h>

void
mpdDataDecode(uint32_t data)
{
  static uint32_t data_count = 0;
  int type_current = 0;
  generic_data_word_t gword;

  gword.raw = data;

  type_current = gword.bf.data_type_tag;

  switch( type_current )
    {
    case 0:		/* BLOCK HEADER */
      {
	mpd_block_header_t d; d.raw = data;

	printf("%08X - BLOCK HEADER - modID = %d   event_per_block = %d   block_count = %d\n",
	       d.raw,
	       d.bf.module_id,
	       d.bf.event_per_block,
	       d.bf.block_count);

	break;
      }

    case 1:		/* BLOCK TRAILER */
      {
	mpd_block_trailer_t d; d.raw = data;

	printf("%08X - BLOCK TRAILER - n_words_in_block = %d\n",
		     d.raw,
		     d.bf.n_words_in_block);
	break;
      }

    case 2:		/* EVENT HEADER */
      {
	mpd_event_header_t d; d.raw = data;

	printf("%08X - EVENT HEADER - event_count = %d\n",
	       d.raw,
	       d.bf.event_count);
	break;
      }

    case 3:		/* TRIGGER TIME */
      {
	mpd_trigger_time_t d; d.raw = data;
	printf("%08X - TRIGGER TIME %d - coarse_trigger_time = %08x\n",
	       d.raw,
	       (d.bf.cont) ? 2 : 1,
	       d.bf.coarse_trigger_time );
	break;
      }

    case 4:		/* APV CHANNEL DATA */
      {
	mpd_apv_channel_data_t d; d.raw = data;
	mpd_data_header_t dh;
	mpd_reduced_data_t rd;
	mpd_apv_trailer_t at;
	mpd_data_trailer_t dt;

	printf("%08X - APV ",//- proc_data_type = %d  processed_data = %x\n",
	       d.raw);/* , */
	       /* d.bf.proc_data_type, */
	       /* d.bf.processed_data); */

	switch ( d.bf.proc_data_type )
	  {
	  case 0: /* HEADER */
	    dh.raw = data;

	    printf("HEADER: what %x  apv_header %x  apv_id %x\n",
		   dh.bf.what,
		   dh.bf.apv_header,
		   dh.bf.apv_id);
	    data_count = 0;
	    break;

	  case 1: /* Reduced Data */
	    rd.raw = data;

	    printf("DATA (%3d): channel_number = %3d  data = 0x%03x (%4d)\n",
		   data_count++,
		   rd.bf.channel_number,
		   rd.bf.data, rd.bf.data);
	    break;

	  case 2: /* APV Trailer */
	    at.raw = data;
	    printf("TRAILER 1: mod_id = %d  sample_count = %d  frame_counter = %d\n",
		   at.bf.mod_id,
		   at.bf.sample_count,
		   at.bf.frame_counter);
	    break;

	  case 3: /* Trailer */
	  default:
	    dt.raw = data;
	    printf("TRAILER 2: baseline_value = %d  word_count = %d\n",
		   dt.bf.baseline_value,
		   dt.bf.word_count);
	    break;
	  }
	break;
      }

    case 5:		/* EVENT TRAILER */
      {
	mpd_event_trailer_t d; d.raw = data;

	printf("%08X - EVENT TRAILER - n_words_in_event = %d  fine_trigger_time = %d\n",
	       d.raw,
	       d.bf.n_words_in_event,
	       d.bf.fine_trigger_time);
	break;
      }

    case 7:		/* FILLER WORD */
      {
	mpd_filler_t d; d.raw = data;

	printf("%08X - FILLER WORD\n",
	       d.raw);
	break;
      }

    default:
      {
	printf("%08X - UNDEFINED TYPE = %d\n",
	       gword.raw,
	       gword.bf.data_type_tag);
	break;
      }

    }

}


#define MAXEVIOBUF 1000000
static unsigned int buf[MAXEVIOBUF];
static char *input_filename/* = "/scratch/mpd/mpd_000054.evio.00000"*/;
static int input_handle;
static int nevent         = 0;
static int skip_event     = 0;


int
main(int argc, char **argv)
{
  int status, fragtag, fragnum, banktag, banknum, ind, nbytes, ind_data;
  unsigned char *end, *start;
  unsigned int data;
  GET_PUT_INIT;

  if(argc!=2)
  {
    printf("Usage: mpddec <input evio file>\n");
    exit(0);
  }

  input_filename = strdup(argv[1]);

  if((status = evOpen(input_filename,"r",&input_handle))!=0)
  {
    printf("\n ?Unable to open input file %s, status=%d\n\n",input_filename,status);
    exit(1);
  }

  while ((status=evRead(input_handle,buf,MAXEVIOBUF))==0)
  {
    nevent++;
    if(!(nevent%10000)) printf("processed %d events\n",nevent);
    if(skip_event>=nevent) continue;
    /*if(user_event_select(buf)==0) continue;*/

    fragtag = 46,
    fragnum = -1;
    banktag = 0xe140; /* mpd bank */

    for(banknum=0; banknum<40; banknum++)
    {
      ind = evLinkBank(buf, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data);
      /*printf("evLinkBank returns %d\n",ind);*/
      if(ind<=0) continue;

      start = (unsigned char *) &buf[ind_data];
      end = start + nbytes;

      b08 = start;
      while(b08<end)
      {
	GET32(data);
        mpdDataDecode(data);
      }

    } /*for*/

  } /*while*/

  evClose(input_handle);

  exit(0);
}
