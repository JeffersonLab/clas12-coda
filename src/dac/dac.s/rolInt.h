
/* Readout list parameters */
/* default values all zero, cleaned up in roc_component.c in Download */

#include <stdint.h>

#ifdef __cplusplus
typedef int 		(*FUNCPTR) (...);     /* ptr to function returning int */
typedef void 		(*VOIDFUNCPTR) (...); /* ptr to function returning void */
#else
typedef int 		(*FUNCPTR) ();	   /* ptr to function returning int */
typedef void 		(*VOIDFUNCPTR) (); /* ptr to function returning void */
#endif			/* _cplusplus */




typedef struct rolParameters *rolParam;     
typedef struct rolParameters
{
  char          *name;	           /* name of parent process */
  char           tclName[20];	   /* Tcl name of this list */ 
  char          *listName;	       /* name of this list */
  int32_t        runType;	       /* run type */
  int32_t        runNumber;	       /* run number */
  VOIDFUNCPTR    rol_code;	       /* init procedure */
  int32_t        daproc;	       /* list of function pointers */
  void          *id;		       /* ID of storage used during load */
  int32_t        nounload;	       /* if !=0 module can't be unloaded */
  int32_t        inited;	       /* we have been initialised */
  int32_t       *dabufp;	       /* output  write pointer */
  int32_t       *dabufpi;	       /* input   read  pointer */

  uint32_t       recNb;	           /* count of output buffers processed */
  uint32_t      *nevents;          /* number of events taken */
  int32_t        classid;          /* object class id (0-slave, 1-master, 2-single) */
  char           confFile[256];    /* contains config file name selected by runcontrol */
  char           usrString[32];    /* string passed in download */
  int32_t        user_storage[2];  /* user storage */
  int32_t        pid;              /* ROC ID */
  int32_t        poll;             /* to poll or not to poll */

  int32_t        doDone;		   /* should we call 'done' in ROL1 ? */
} ROLPARAMS;


/* READOUT LIST PROCEDURES  */

#define DA_INIT_PROC        0
#define DA_DOWNLOAD_PROC    1
#define DA_PRESTART_PROC    2
#define DA_END_PROC         3
#define DA_PAUSE_PROC       4
#define DA_GO_PROC          5
#define DA_POLL_PROC        6
#define DA_DONE_PROC        7
#define DA_REPORT_PROC      8
#define DA_FREE_PROC        9






/* for UDP_user_request */

#define MSGINF 0
#define MSGWRN 1
#define MSGERR 2

int UDP_user_request(int32_t msgclass, char *name, char *message);
