
/* rol3.c - readout list for 'eb_proc_thread' (Event Builder) */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifndef VXWORKS
/*for fchmod*/
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifndef VXWORKS
#include <sys/types.h>
#include <time.h>
#endif



#ifndef Linux_armv7l
#define SSIPC
#endif
#undef SSIPC



#ifdef SSIPC_HIDE
#include <rtworks/ipc.h>
#include "epicsutil.h"
static char ssname[80];
#endif

#include "daqLib.h"
#include "circbuf.h"
int getTdcTypes(int *typebyslot);
int getTdcSlotNumbers(int *slotnumbers);


#define ROL_NAME__ "ROL3"
#define INIT_NAME rol3__init

#define POLLING_MODE
#define EVENT_MODE

#include "rol.h"
#include "EVENT_source.h"

static char rcname[5];
 

/* user routines */

void rol3trig(int a, int b);
void rol3trig_done();

static void
__download()
{
  rol->poll = 1;

  printf("INFO: Download ROL3 reached\n");
  
  sprintf(rcname,"RS%02d",rol->pid);
  printf("rcname >%4.4s<\n",rcname);

#ifdef SSIPC_HIDE
  sprintf(ssname,"%s_%s",getenv("HOST"),rcname);
  printf("Smartsockets unique name >%s<\n",ssname);
  epics_msg_sender_init(getenv("EXPID"), ssname); /* SECOND ARG MUST BE UNIQUE !!! */
#endif

  printf("INFO: Download ROL3 executed\n");
  
  return;
}

static void
__prestart()
{
  int ii, ntdcs, status;

  printf("INFO: Entering Prestart ROL3, rol->pid=%d\n",rol->pid);fflush(stdout);

  /* Clear some global variables etc for a clean start */
  CTRIGINIT;

  /* init trig source EVENT */
  EVENT_INIT;

  /* Register a sync trigger source (up to 32 sources) */
  CTRIGRSS(EVENT, 1, rol3trig, rol3trig_done); /* second arg=1 - what is that ? */

  rol->poll = 1;

  rol->recNb = 0;

  printf("INFO: Prestart ROL3 executed\n");fflush(stdout);

  return;
}

static void
__end()
{
  printf("INFO: User End 3 Executed\n");

  return;
}

static void
__pause()
{
  printf("INFO: User Pause 3 Executed\n");

  return;
}

static void
__go()
{
  printf("User Go 3 Reached\n");fflush(stdout);

  printf("INFO: User Go 3 Executed\n");fflush(stdout);

  return;
}



int gem_disentangler(int fd, uint32_t *bufin, uint32_t *bufout);

void
rol3trig(int a, int b)
{
  //printf("ROL3: rol3trig reached, rol at %p\n",rol);fflush(stdout);
  //printf("ROL3: rol->pid=%d\n",rol->pid);fflush(stdout);
  //printf("ROL3: rol->dabufpi at %p, rol->dabufp at %p\n",rol->dabufpi, rol->dabufp);fflush(stdout);

#if 0
  CPINIT;
#endif

  if(rol->pid == 146 || rol->pid == 147) /* gem1vtp or gem2vtp */
  {
    //printf("rol3: calling gem_disentangler, rol->pid=%d ..\n",rol->pid);fflush(stdout);

    gem_disentangler(rol->pid, rol->dabufpi, rol->dabufp);
    //memcpy(rol->dabufp, rol->dabufpi, rol->dabufpi[0] * sizeof(int));

    
    //printf("rol3: .. gem_disentangler done, rol->pid=%d\n",rol->pid);fflush(stdout);
  }
  else
  {
    //printf("rol3: coping %d words ..\n",rol->dabufpi[0]);fflush(stdout);
    memcpy(rol->dabufp, rol->dabufpi, rol->dabufpi[0] * sizeof(int));
    //printf("rol3: .. copied\n");fflush(stdout);
  }
  
#if 0
  CPEXIT;
#endif
  
  //printf("INFO: rol3trig executed\n");fflush(stdout);

  return;
}

void
rol3trig_done()
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

