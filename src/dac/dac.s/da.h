/*----------------------------------------------------------------------------*
 *  Copyright (c) 1991, 1992  Southeastern Universities Research Association, *
 *                            Continuous Electron Beam Accelerator Facility   *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 * CEBAF Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606 *
 *      heyes@cebaf.gov   Tel: (804) 249-7030    Fax: (804) 249-7363          *
 *----------------------------------------------------------------------------*
 * Discription: follows this header.
 *
 * Author:
 *	Graham Heyes
 *	CEBAF Data Acquisition Group
 *----------------------------------------------------------------------------*/

#ifndef _CODA_DA_H
#define	_CODA_DA_H





/* sergey: some general switches */


/* restore old spec events type coding: prestart=17, go=18, end=20 etc */
/* remove highest bit from type to make special event type like it was before: prestart 145->17 etc */
#define RESTORE_OLD_SPEC_EVENT_CODING







#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE !FALSE
#endif

#define _DADEFINED

#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

#include "et.h"

#ifdef SOLARIS
#include <ucontext.h>
#endif

#include "obj.h"

#include <setjmp.h>
#ifdef LINUX
#include <stdarg.h>
#endif

#define MAX_FRAG 150		/* event size */
#define MAX_BLOCK 4		/* events per block */
#define TCP_ON 1
#define LOCK_STEP 2

#define EV_OFFSET   0x80

#define EV_SYNC     (16+EV_OFFSET)
#define EV_PRESTART (17+EV_OFFSET)
#define EV_GO       (18+EV_OFFSET)
#define EV_PAUSE    (19+EV_OFFSET)
#define EV_END      (20+EV_OFFSET)
/*
#define EV_SYNC     16
#define EV_PRESTART 17
#define EV_GO       18
#define EV_PAUSE    19
#define EV_END      20
*/

#define EV_BANK_HDR  0x00000100
#define EV_BAD       0x10000000

#define PHYS_BANK_HDR(t,e) (uint32_t)((((t)&0xf)<<16) | ((e)&0xff) | EV_BANK_HDR)

#define CTL_BANK_HDR(t) (uint32_t)((((t)&0xffff)<<16) | 0x000001CC)

#define CHAR_BANK_HDR(t) (uint32_t)((((t)&0xffff)<<16) | 0x00000300)

#define IS_BANK(b) (((uint32_t) (b) && EV_BANK_HDR)==EV_BANK_HDR)

#define DECODE_BANK_HEADER(b,t,e) { t = (b[1]>>16)&0xffff; e = b[1]&0xff;}


#define EV_BANK_ID 0xc0010100
#define EV_HDR_LEN 4

/* define some things for byte swapping */
#define LSWAP(x)        ((((x) & 0x000000ff) << 24) | \
                         (((x) & 0x0000ff00) <<  8) | \
                         (((x) & 0x00ff0000) >>  8) | \
                         (((x) & 0xff000000) >> 24))

#define SSWAP(x)        ((((x) & 0x00ff) << 8) | \
                         (((x) & 0xff00) >> 8))


#define DT_BANK    0x10
#define DT_LONG    0x01
#define DT_SHORT   0x05
#define DT_BYTE    0x07
#define DT_CHAR    0x03

/* dtswap[1] corresponds type 1, dtswap[2] - type 2 etc; 0 means 'no swap', 1-swap 16bit, 2-swap 32bit, 3-swap 64bit, 4 - composite swap, 5 - bank of banks */
/* EVIO types:
      1    'i'   unsigned int
      2    'F'   floating point
      3    'a'   8-bit char (C++)
      4    'S'   short
      5    's'   unsigned short
      6    'C'   char
      7    'c'   unsigned char
      8    'D'   double (64-bit float)
      9    'L'   long long (64-bit int)
     10    'l'   unsigned long long (64-bit int)
     11    'I'   int
*/
static int32_t dtswap[] = {
  0,2,2,0,1,1,0,0, 3,2,3,0,0,0,5,4, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0 };


extern jmp_buf global_env[8][32];
extern int32_t use_recover;

extern int32_t global_env_depth[32];
extern char *global_routine[8][32];

#define SIGVECTOR sigvec

#define recoverContext(name,res) \
{ \
  { \
	res = 0; \
  } \
}

#define DATA_DEBUG 1
#define API_DEBUG  2

/* defines for listSplit1() */

/* ERROR: MUST BE AT LEAST >MAX_ROCS, because it is used to parse links (see coda_ebc.c linkArgv[LISTARGV1][LISTARGV2]) */
/* HAVE TO CHECK IT SOMEHOW , OR JUST USE MAX_ROCS HERE ! */
#define LISTARGV1 /*40*/128 

#define LISTARGV2 256

/* Linux and Darwin returns microsecs, Solaris returns nanosecs */
#ifdef Linux
#define NANOMICRO 1
#else
#ifdef Darwin
#define NANOMICRO 1
#else
#define NANOMICRO 1000
#endif
#endif

/* Linux does not have hrtime */
#ifdef Linux

#ifdef  __cplusplus
extern "C" {
#endif

typedef	long long	hrtime_t; /* or uint64_t ??? */
hrtime_t/*uint64_t*/ gethrtime(void);

#ifdef  __cplusplus
}
#endif

#endif



#define USE_128

/* to handle 128-bit words, needed by event building process */

typedef struct
{
  uint32_t words[4]; /* words[0] is least significant */  

} WORD128;



/* function prototypes */

void Print128(WORD128 *hw);
void String128(WORD128 *hw, char *str, int len);
void Copy128(WORD128 *hws, WORD128 *hwd);
void AND128(WORD128 *hwa, WORD128 *hwb, WORD128 *hwc);
void OR128(WORD128 *hwa, WORD128 *hwb, WORD128 *hwc);
void XOR128(WORD128 *hwa, WORD128 *hwb, WORD128 *hwc);
int32_t CheckBit128(WORD128 *hw, int32_t n);
void SetBit128(WORD128 *hw, int32_t n);
int32_t EQ128(WORD128 *hwa, WORD128 *hwb);
int IFZERO128(WORD128 *hwa);
void Clear128(WORD128 *hw);
void Negate128(WORD128 *hw);

char *dacGetExpid();

int tcpServer(char *name, char *mysqlhost);

int resetHeartBeats();
int setHeartBeat(int system, int bit, int countdown);
int getHeartBeat(int system);
int checkHeartBeats();
int codaFindFreeTcpPort();
char *loadwholefile(char *file, int *size, int *padding);
int codaLoadROL(ROLPARAMS *rolP, char *rolname, char *params);
int codaUnloadROL(ROLPARAMS *rolP);
int isBigEndian(void);
void debug_printf(int level, char *fmt,...);
int pr_time(char *stuff) ;
int lastContext ();
void *signal_thread (void *arg);
void Recover_Init ();
int coda_constructor();
int coda_destructor();
int listSplit2(char *list, char *separator, int *argc, char argv[LISTARGV1][LISTARGV2]);
void CODA_Init(int argc, char **argv);
void CODA_Execute ();
int CODA_bswap(int32_t *cbuf, int32_t ndata);
void getSessionName(char *sessionName, int lname);
void getConfFile(char *configname, char *conffile, int lname);
int UDP_establish(char *host, int port);
int UDP_close(int socket);
int UDP_standard_request(char *name, char *state);
int UDP_user_request(int msgclass, char *name, char *message);
int UDP_reset();
int UDP_cancel(char *str);
int UDP_request(char *str);
int UDP_send(int socket);
void UDP_show();
int codaUpdateStatus(char *status);
void UDP_loop();
int UDP_start();
int listSplit1(char *list, int flag, int *argc, char argv[LISTARGV1][LISTARGV2]);
hrtime_t gethrtime(void);
void gethrtimetest();

int dacgethostbyname(char *hostname, char *ipaddress);


#endif /* _CODA_DA_H */
