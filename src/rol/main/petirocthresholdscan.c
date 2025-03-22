#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "petirocLib.h"
#include "petirocConfig.h"
#include <getopt.h>

static int npetiroc;
extern PETIROC_CONF *petirocConfPtr; 

int scalers[32][52];

int   debug         = 0;
int   verbose_flag  = 0;
char* output_file   = NULL;
int   THR_START     = 300;
int   THR_STEP      = 5;
int   THR_NUM       = 40;
int   slot_number   = -1;

void print_help() {
  printf("Usage: petirocthresholdscan [-o OUTPUT_FILE] [-I START_THR] [-n N_STEPS] [-S STEP_SIZE] \n"
         "ATOF petiroc readout threshold scan\n"
         "Options: \n"
         "  -o  --output     output file \n"
         "  -s, --slot       slot (board) number to scan. Default is all (-1)\n"
         "  -I, --start      initial threshold (300)\n"
         "  -n, --nsteps     (ignored) number of steps (40)\n"
         "  -S, --step       threshold step size (5)\n"
         "      --debug      sets the debug flag\n"
         "      --verbose    verbose printing\n"
         "  -h, --help       print help\n"
         );
}

int parse_args(int argc, char **argv)
{
  int c;
  while (1)
  {
    static struct option long_options[] =
    {
      {"output",    required_argument, 0, 'o'},
      {"step",      required_argument, 0, 'S'},
      {"start",     required_argument, 0, 'I'},
      {"nsteps",    required_argument, 0, 'n'},
      {"slot",      required_argument, 0, 's'},
      {"debug",     no_argument,       &debug, 1},
      {"verbose",   no_argument,       &verbose_flag, 1},
      {"brief",     no_argument,       &verbose_flag, 0},
      {"help",      no_argument,       0, 'h'},
      {0, 0, 0, 0}
    };
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long (argc, argv, "o:S:I:n:s:h",
        long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    //printf("%d\n", c);
    //printf("%s\n",long_options[option_index].name);
    switch (c)
    {
      case 0:
        //if ( strcmp("output", long_options[option_index].name) == 0 ) {
        //  output_file =  optarg;
        //}
        //if ( strcmp("step", long_options[option_index].name) == 0 ) {
        //  THR_STEP =  atoi(optarg);
        //}
        //if ( strcmp("start", long_options[option_index].name) == 0 ) {
        //  THR_START =  atoi(optarg);
        //}
        //if ( strcmp("nsteps", long_options[option_index].name) == 0 ) {
        //  THR_NUM =  atoi(optarg);
        //}
        //if ( strcmp("slot", long_options[option_index].name) == 0 ) {
        //  slot_number =  atoi(optarg);
        //}

        if (long_options[option_index].flag != 0)
          break;
        printf ("option %s", long_options[option_index].name);
        if (optarg)
          printf (" with arg %s", optarg);
        printf ("\n");
        break;

      case 'o':
        output_file = optarg;
        break;

      case 's':
        slot_number = atoi(optarg);
        break;

      case 'I':
        THR_START = atoi(optarg);
        break;

      case 'S':
        THR_STEP = atoi(optarg);
        break;

      case 'n':
        THR_NUM = atoi(optarg);
        break;

      case 'h':
        print_help();
        exit(0);
        break;

      case '?':
        /* getopt_long already printed an error message. */
        print_help();
        break;

      default:
        abort ();
    }
  }

  /* Instead of reporting ‘--verbose’
   *      and ‘--brief’ as they are encountered,
   *           we report the final status resulting from them. */
  if (verbose_flag)
    puts ("verbose flag is set");

  /* Print any remaining command line arguments (not options). */
  if (optind < argc)
  {
    printf ("non-option ARGV-elements: ");
    while (optind < argc)
      printf ("%s ", argv[optind++]);
    putchar ('\n');
  }

  return (0);
}


int main(int argc, char *argv[])
{
  parse_args(argc,argv);

  memset(scalers, 0, sizeof(scalers));

  PETIROC_Regs chip[2];
  printf("\npetirocthresholdscan started ..\n\n");fflush(stdout);

  npetiroc = petirocInit(0, PETIROC_MAX_NUM, PETIROC_INIT_REGSOCKET);

  printf("\npetirocinit: npetiroc=%d\n\n",npetiroc);fflush(stdout);
  if(npetiroc<=0) exit(0);

  petirocInitGlobals();
  petirocConfig("");
  printf("\npetiroc initialized\n\n");fflush(stdout);

  FILE* fileptr;
  if(output_file) {
    fileptr = fopen(output_file, "w");
    printf("opening file %s\n",output_file);
    if (fileptr == NULL) {
      printf("Error Occurred While creating a file !");
      exit(1);
    }
  }

  //if(output_file) fprintf(fileptr, "%10s %10s [atof channels 0-47] [test channels 48-51] \n", "threshold", "slot");

  for(int nthr=0; nthr<THR_NUM; nthr++)
  {
    int thr = THR_START+nthr*THR_STEP;
    printf("scanning threshold = %d  (%d/%d)\n", thr,nthr,THR_NUM); fflush(stdout);
    for(int ch=0; ch<52; ch++)
    { 
      // loop over this channel for each board
      for(int j=0; j<npetiroc; j++)
      {
        int slot = petirocSlot(j);
        if((slot_number>=0) && (slot_number !=slot)) {
          continue;
        }
        petirocConfPtr[slot].chip[0].SlowControl.vth_time = thr;
        petirocConfPtr[slot].chip[1].SlowControl.vth_time = thr;
        petirocConfPtr[slot].chip[0].SlowControl.mask_discri_time = 0xFFFFFFFF;
        petirocConfPtr[slot].chip[1].SlowControl.mask_discri_time = 0xFFFFFFFF;
        petirocConfPtr[slot].chip[0].SlowControl.mask_discri_charge = 0xFFFFFFFF;
        petirocConfPtr[slot].chip[1].SlowControl.mask_discri_charge = 0xFFFFFFFF;
        if(ch<32)
          petirocConfPtr[slot].chip[0].SlowControl.mask_discri_time^=(1<<ch);
        else
          petirocConfPtr[slot].chip[1].SlowControl.mask_discri_time^=(1<<(ch-32));

        petiroc_cfg_rst(slot);
        petiroc_slow_control(slot, petirocConfPtr[slot].chip);
      
        usleep(10000);
        petiroc_clear_scalers(slot);
      } // loop over slots/boards
      usleep(100000);
      /// \todo stop/latch all scalers first. then read them out
      for(int j=0; j<npetiroc; j++)
      {
        int slot = petirocSlot(j);
        if((slot_number>=0) && (slot_number !=slot)) {
          continue;
        }
        scalers[slot][ch] = petiroc_get_scaler(slot, ch);
      } //loop over boards
    } //loop over channels
    
    for(int j=0; j<npetiroc; j++)
    {
      int slot = petirocSlot(j);
      if((slot_number>=0) && (slot_number !=slot)) {
        continue;
      }
      printf("%d ", thr); 
      printf(" %d ", slot);
      if(output_file) fprintf(fileptr, "%d %d", thr, slot);
      for(int ch=0; ch<52; ch++)
      {
        printf(" %4d", scalers[slot][ch]);
        if(output_file) fprintf(fileptr, " %5d", scalers[slot][ch]);
      }
      printf("\n");
      if(output_file) fprintf(fileptr,"\n");
    } //loop over boards
  } // loop over thesholds

  petirocEnd();

  if(output_file) {
    fclose(fileptr);
    printf("file %s closed\n",output_file);
  }

  exit(0);
}

