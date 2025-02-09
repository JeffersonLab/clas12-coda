/*
 helicitytest.cc - 

 example:   ./Linux_i686/bin/trigtest1 /work/boiarino/data/c.evio
 */



#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <string>
#include <vector>
#include <memory>

using namespace std;


#define ENCORR 10000. /* sergey: clara applies 1/10000 to ADC values */ 

#define NHITS 100
static int inIBuf[25][NHITS];
static double inFBuf[25][NHITS];

#include "ipc.h"

#include "evio.h"
#include "evioBankUtil.h"

#include "helicity.h"


#define MAXBUF 10000000
unsigned int buf[MAXBUF];
unsigned int *bufptr;

#define SKIPEVENTS 40
#define MAXEVENTS 1000000000

int
main(int argc, char **argv)
{
  int run = 11; /* sergey: was told to use 11, do not know why .. */
  int ii, ind, fragtag, fragnum, tag, num, nbytes, ind_data;
  int nhitp, nhiti, nhito, nhitp_offline, nhiti_offline, nhito_offline;
  float tmp;
  int event, type;

  int runnum = 0;

  char fnamein[1024];
  char fnameout[1024];
  int nfile, status, handlerin, handlerout, maxevents, iev;
  nfile = 0;



  printf("Connecting to IPC server ..\n");
  /*epics_json_msg_sender_init(getenv("EXPID"), getenv("SESSION"), "daq", "HallB_DAQ");*/
  epics_json_msg_sender_init("clasrun", "clasprod", "daq", "HallB_DAQ");
  printf(".. done connecting to IPC server.\n");



  /* emulate Prestart event */
  helicity(bufptr, 17);
  iev = 0;

  for(nfile=0; nfile<=2; nfile++)
  {
    if(nfile==1) continue;

	/* input evio file */

	sprintf(fnamein, "%s.%05d", argv[1], nfile);
	printf("opening input file >%s<\n", fnamein);
	status = evOpen(fnamein, (char *)"r", &handlerin);
	printf("status=%d\n", status);
	if (status != 0) {
		printf("evOpen(in) error %d - exit\n", status);
		exit(-1);
	}



	/* output evio file */

	sprintf(fnameout, "%s_out.%05d", argv[1], nfile);
	printf("opening output file >%s<\n", fnameout);
	status = evOpen(fnameout, (char *)"w", &handlerout);
	printf("status=%d\n", status);
	if (status != 0) {
		printf("evOpen(out) error %d - exit\n", status);
		exit(-1);
	}

	maxevents = MAXEVENTS;

	if( argc >= 3 ){
	  maxevents = atoi(argv[2]);
	  printf("Number of events to process is %d\n",maxevents);
	}




	while (iev < maxevents) {
		iev++;

		/*if(!(iev%1000))*/ /*printf("\n\n\nEvent %d\n\n", iev);*/

		status = evRead(handlerin, buf, MAXBUF);

		if (iev < SKIPEVENTS)
			continue;
		//if(iev==905) continue;
		//printf("Event %d processing\n", iev);

		if (status < 0) {
			if (status == EOF) {
				printf("evRead: end of file after %d events - exit\n", iev);
				break;
			} else {
				printf("evRead error=%d after %d events - exit\n", status, iev);
				break;
			}
		}
		bufptr = buf;

		//printf("\n\n\nEvent %d ===================================================================\n\n", iev);
		fflush(stdout);
		nhitp = 0;
		nhiti = 0;
		nhito = 0;



		helicity(bufptr, type);




		status = evWrite(handlerout, buf);
		if (status < 0) {
			printf("evWrite error=%d after %d events - exit\n", status, iev);
			break;
		}
		

	} /*while*/


	printf("file %d, %d events processed\n\n",nfile,iev);

	evClose(handlerin);
	evClose(handlerout);

  } /* for */

  epics_json_msg_close();

  exit(0);
}
