
/* evioUtilSelect.c - select events in according to the file select.txt */

/* for posix */
#define _POSIX_SOURCE_ 1
#define __EXTENSIONS__

#undef DEBUG

#define SORT_EVENTS

/*  misc macros, etc. */
#define MAXBUF 1000000

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <evio.h>
#include <evioBankUtil.h>


static unsigned int buf[MAXBUF];
static char *input_filename/* = "/work/dcrb/dcrb2_000049.evio.0_1600V_60mV"*/;
static int input_handle;
static char *output_filename/* = "test.evio"*/;
static int output_handle;
static char *select_filename = "select.txt";

static int nevent         = 0;
static int nwrite         = 0;
static int skip_event     = 0;
//static int max_event      = 10000000;

#ifdef SORT_EVENTS
#define MAX_EVENTS 1000000
static int event_number[MAX_EVENTS];

static int
events_compare(const void *ev1, const void *ev2)
{
  int tmp1 = *((int *)ev1);
  int tmp2 = *((int *)ev2);

  if(tmp1 > tmp2) return(1);
  if(tmp1 < tmp2) return(-1);
  
  return(0);
}
#endif



#define STRLEN 1024

int
main(int argc, char **argv)
{
  int status;
  int ii, l, tag, num, runnum, i0, i1, i2, i3, i4, filenumber;
  int ind, fragtag, fragnum, nbytes, ind_data, timestamp_flag, type, *nhits;
  int slot, slot_old, event, chan, tdc, itmp, iev, iev_needed, get_next_select, nselected, iselected;
  int banktag, banknum, banktyp = 0xf;
  char *fmt = "c,i,l,N(c,s)";
  unsigned int ret, word;
  unsigned long long timestamp, timestamp_old;
  unsigned char *end, *start;
  char input_filename_full[1024];
  FILE *fd;
  char str_tmp[STRLEN];
  char ch;
  GET_PUT_INIT;

  if(argc!=3 && argc!=4)
  {
    printf("Usage: evioUtilSelect <input evio file> <output evio file> [starting_file_number]\n");
    exit(0);
  }
  input_filename = strdup(argv[1]);
  output_filename = strdup(argv[2]);
  printf("Use >%s< as input file\n",input_filename);
  printf("Use >%s< as output file\n",output_filename);
  if(!strcmp(input_filename,output_filename))
  {
    printf("input and output files must be different - exit\n");
    exit(0);
  }

  filenumber = 0;
  if(argc==4)
  {
    filenumber = atoi(argv[3]);
    printf("Starting from file %d\n",filenumber);
  }


  fd = fopen(output_filename,"r");
  if(fd>0)
  {
    printf("Output file %s exist - exit\n",output_filename);
    fclose(fd);
    exit(0);
  }

  fd = fopen(select_filename,"r");
  if(fd<=0)
  {
    printf("Cannot open select file - exit\n");
    exit(0);
  }
  else
  {
    printf("Use select file >%s<\n",select_filename);
  }

#ifdef SORT_EVENTS

  nselected = 0;
  while ((ch = getc(fd)) != EOF)
  {
    if ( ch == '#' || /*ch == ' ' ||*/ ch == '\t' )
    {
      while ( getc(fd)!='\n' /*&& getc(fd)!=!= EOF*/ ) {} /*ERROR !!!*/
    }
    else if( ch == '\n' ) {}
    else
    {
      ungetc(ch,fd);
      fgets(str_tmp, STRLEN, fd);
      printf("str_tmp >%s<",str_tmp);fflush(stdout);
      sscanf(str_tmp,"%d %d",&runnum,&iev_needed);
      printf("selection list: runnum %5d, event %8d\n",runnum,iev_needed);fflush(stdout);
      event_number[nselected++] = iev_needed;
      if(nselected>=MAX_EVENTS) break;
      if(iev_needed==99999999) break;
    }
  }
  printf("nselected=%d\n",nselected);
  iselected = 0;

  qsort((void *)event_number, nselected, sizeof(int), events_compare);
  //for(ii=0; ii<nselected; ii++) printf("selected[%7d] %7d\n",ii,event_number[ii]);*/
  printf("WILL PROCESS %d events, min event number %d, max event number %d\n",nselected,event_number[0],event_number[nselected-1]);

#endif


  /* open evio output file */
  if((status = evOpen(output_filename,"w",&output_handle))!=0)
  {
    printf("\n ?Unable to open output file %s, status=%d\n\n",output_filename,status);
    exit(1);
  }


  get_next_select = 1;
  nwrite = 0;

next_file:

  nevent = 0;

  /* open evio input file */
  sprintf(input_filename_full,"%s.%05d",input_filename,filenumber);
  if((status = evOpen(input_filename_full,"r",&input_handle))!=0)
  {
    printf("\n ?Unable to open input file %s, status=%d\n\n",input_filename_full,status);
    evClose(output_handle);
    fclose(fd);
    exit(1);
  }
  else
  {
    printf("\n==> Opened input file %s\n",input_filename_full);
  }


  while (1/*nevent<max_event*/)
  {

    /*printf("\n\nProcessing event %d\n",nevent);*/

    nevent++;

    if(!(nevent%10000)) printf("evioUtilSelect: processed %d events, last iev=%d\n",nevent,iev);
    if(skip_event>=nevent) continue;
    /*if(user_event_select(buf)==0) continue;*/


    status = evRead(input_handle, buf, MAXBUF);
    if(status < 0)
    {
      if(status==EOF)
      {
        printf("evRead: end of file after %d events - exit\n",nevent);
        break;
      }
      else
      {
        printf("evRead error=%d after %d events - exit\n",status,nevent);
        break;
      }
    }
    //else
    //{
    //  printf("\nRead event from input file, evRead returned status=%d\n\n",status);
    //}

#ifdef SORT_EVENTS
    if(get_next_select)
    {
      iev_needed = event_number[iselected++];
      if(iselected>nselected)
      {
        printf("\niselected=%d, nselected=%d - selection complete, exiting\n",(iselected-1),nselected);
        evClose(output_handle);
        fclose(fd);
        goto out;
      }
      printf("Looking for iev_needed=%d ...\n",iev_needed);
    }
#else
    if(get_next_select)
    {
      if ((ch = getc(fd)) != EOF)
      {
        if ( ch == '#' || ch == ' ' || ch == '\t' )
        {
          while ( getc(fd)!='\n' /*&& getc(fd)!=!= EOF*/ ) {} /*ERROR !!!*/
        }
        else if( ch == '\n' ) {}
        else
        {
          ungetc(ch,fd);
          fgets(str_tmp, STRLEN, fd);
          /*printf("str_tmp >%s<",str_tmp);fflush(stdout);*/
          sscanf(str_tmp,"%d",&iev_needed);
          /*printf("next iev=%d\n",iev_needed);*/
          if(iev_needed==99999999) goto done;
        }
      }
    }
#endif

    /**********************/
    /* evioBankUtil stuff */

    /* read event number from head bank */
    fragtag = 37;
    fragnum = -1;
    banktag = 0xe10a;
    banknum = 0;

    ind = 0;
    for(banknum=0; banknum<40; banknum++)
    {
      /*printf("looking for %d %d  - 0x%04x %d\n",fragtag, fragnum, banktag, banknum);*/
      ind = evLinkBank(buf, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data);
      if(ind>0) break;
    }

    if(ind<=0)
    {
      printf("ERROR: cannot find head bank - goto next event\n");
      continue;
    }

    b08 = (unsigned char *) &buf[ind_data];
    GET32(itmp);
    GET32(iev);
    //printf("iev=%d\n",iev);

    if(iev==iev_needed)
    {
      printf("WRITE EVENT %7d !!!!!!!!!!!!!!!!!!\n",iev);
      nwrite++;
      status = evWrite(output_handle,&buf[0]);
      if(status!=0)
      {
        printf("\n ?evWrite error output file %s, status=%d\n\n",output_filename,status);
        exit(1);
      }
      get_next_select = 1;
    }
    else
    {
      /*printf("SKIP EVENT %7d, needed %7d\n",iev,iev_needed);*/
      get_next_select = 0;
      if(iev>iev_needed || iev_needed==99999999)
      {
        printf("SOMETHING WRONG (iev=%d iev_needed=%d) - GOTO DONE\n",iev,iev_needed);
        goto done;
      }
    }


    /*printf("nevent=%d max_event=%d\n",nevent,max_event);*/

  }



done:

  /* done */
  printf("\n  Read %d events from current file, copied %d events so far\n\n",nevent,nwrite);
  filenumber++;

goto next_file;


out:

  printf("\n  Read %d events, copied %d events\n\n",nevent,nwrite);

  exit(0);
}
