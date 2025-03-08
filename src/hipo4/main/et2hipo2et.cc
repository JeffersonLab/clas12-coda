
/* et2hipo2et.cc */

/*

clondaq7 (sender):

et_start -f /tmp/et_sys_hipotest -n 500 -s 100000
evio2et /data/test/clas_017492.evio.00826 /tmp/et_sys_hipotest

clonfarm11 (receiver):

et_start -f /tmp/et_sys_hipotest -n 50 -s 10000000
et2hipo2et clondaq7:/tmp/et_sys_hipotest clonfarm11:/tmp/et_sys_hipotest ET2ET

##et_2_et clondaq7:/tmp/et_sys_hipotest clonfarm11:/tmp/et_sys_hipotest ET2ET

clonfarm11 (monitor):
et_monitor -f /tmp/et_sys_hipotest

##et2bos_test /tmp/et_sys_hipotest TEST

*/

/*

ET system has following definitions

ET_ENDIAN_BIG     0 - sparc
ET_ENDIAN_LITTLE  1 - intel

ET_ENDIAN_SWITCH   4 - will force ET system to switch endian

  use functions

et_event_setendian()
et_event_getendian()

  to set or get endian.

Inside ET system has another parameter:

pe->byteorder - can be 0x04030201 or 0x01020304

 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#ifdef sun
#include <thread.h>
#endif
#include "et.h"
#include "et2hipo.h"


/*sergey:temp, to print endian*/
#include "et_private.h"


int
et_bridge_HIPO(et_event *src_ev, et_event *dest_ev, int bytes, int same_endian)
{
  int   nlongs;
  nlongs = bytes/sizeof(int);

  /* swap if necessary 
  BOSrecordSwap((int *)src_ev->pdata, (int *)dest_ev->pdata);
  */

  return(ET_OK);
}

/* signal handler prototype */
static void *signal_thread (void *arg);



#define NUMEVENTS 100
#define STRLEN 256


int
main(int argc, char **argv)
{
  sigset_t        sigblock;
  pthread_t       tid;

  int             i, j, status, numread, totalread=0, loops=0;
  int		      con[ET_STATION_SELECT_INTS], ntransferred=0;
  et_statconfig   sconfig;
  et_openconfig   openconfig;
  et_bridgeconfig bconfig;
  struct timespec timeout;
  et_att_id       att_from, att_to;
  et_stat_id      stat_from, stat_to;
  et_sys_id       id_from, id_to;
  int             selections[] = {17,15,-1,-1}; /* 17,15 are arbitrary */

  char from_node[STRLEN], from_et[STRLEN], to_node[STRLEN], to_et[STRLEN];
  char from_station[STRLEN];
  char option[STRLEN];
  char from_node_ssipc[STRLEN];
  char ch[STRLEN], *cc;

  if((argc != 4) && (argc != 5))
  {
    printf("Usage: %s <from_node:from_et> <to_node:to_et> <from_station> [<option>]\n",
            argv[0]);
    exit(1);
  }


  /*******************/
  /* parse arguments */
  /*******************/

  strcpy(ch,argv[1]);
  cc = strchr(ch,':'); /* get pointer to the location of ':' */
  if(cc==NULL)
  {
    printf("wrong arguments in 'from' - exit\n");
    exit(0);
  }
  cc[0] = '\0'; /* replace ':' by the end of string */
  strcpy(from_node,ch);
  strcpy(from_et,(cc+1));
  printf("from_node >%s<, from_et >%s<\n",from_node,from_et);

  strcpy(ch,argv[2]);
  cc = strchr(ch,':'); /* get pointer to the location of ':' */
  if(cc==NULL)
  {
    printf("wrong arguments in 'to' - exit\n");
    exit(0);
  }
  cc[0] = '\0'; /* replace ':' by the end of string */
  strcpy(to_node,ch);
  strcpy(to_et,(cc+1));
  printf("to_node >%s<, to_et >%s<\n",to_node,to_et);

  strcpy(from_station,argv[3]);
  printf("from_station >%s<\n",from_station);

  if(argc == 5)
  {
    strcpy(option,argv[4]);
    printf("option >%s<\n",option);
  }

  if(strlen(from_node)==0)
  {
    printf("wrong 'from_node' - exit\n");
    exit(0);
  }
  if(strlen(from_et)==0)
  {
    printf("wrong 'from_et' - exit\n");
    exit(0);
  }
  if(strlen(to_node)==0)
  {
    printf("wrong 'to_node' - exit\n");
    exit(0);
  }
  if(strlen(to_et)==0)
  {
    printf("wrong 'to_et' - exit\n");
    exit(0);
  }
  if(strlen(from_station)==0)
  {
    printf("wrong 'from_station' - exit\n");
    exit(0);
  }

  timeout.tv_sec  = 0;
  timeout.tv_nsec = 1;


  /*************************/
  /* setup signal handling */
  /*************************/
  /* block all signals */
  sigfillset(&sigblock);
  status = pthread_sigmask(SIG_BLOCK, &sigblock, NULL);
  if (status != 0) {
    printf("%s: pthread_sigmask failure\n", argv[0]);
    exit(1);
  }
  /* spawn signal handling thread */
  pthread_create(&tid, NULL, signal_thread, (void *)NULL);


  /* init ET configuration will be used for both ET systems */
  et_open_config_init(&openconfig);



  /******************/
  /* open remote ET */
  /******************/

  et_open_config_sethost(openconfig, from_node);
  et_open_config_gethost(openconfig, ch);
  printf("remote (from) host >%s<\n",ch);
  /*et_open_config_setport(openconfig, 12345);*/


  et_open_config_addbroadcast(openconfig,"129.57.167.255");
  et_open_config_addbroadcast(openconfig,"129.57.176.255");

  if(et_open(&id_from, from_et, openconfig) != ET_OK)
  {
    printf("%s: et_open 'from' problems\n",argv[0]);
    exit(1);
  }


  /*****************/
  /* open local ET */
  /*****************/

  /*!!!!!!!!!!!!!!!! ET_HOST_ANYWHERE works, to_node does not !!!!!!!!*/
  et_open_config_sethost(openconfig, to_node/*ET_HOST_ANYWHERE*/);
  et_open_config_gethost(openconfig, ch);
  printf("local (to) host >%s<\n",ch);

  if(argc == 5 && !strcmp(option,"direct"))
  {
    printf("Use 'direct' connection: you can connect to any subnet,\n");
    printf("but only one ET system allowed on the same machine\n");
    et_open_config_setcast(openconfig, ET_DIRECT);
  }

  et_open_config_addbroadcast(openconfig,"129.57.167.255");
  et_open_config_addbroadcast(openconfig,"129.57.176.255");

  {
    et_open_config *config = (et_open_config *) openconfig;
    printf("befor et_open: rBufSize=%d, sBufSize=%d, noDelay=%d\n",
      config->tcpSendBufSize,config->tcpRecvBufSize,config->tcpNoDelay);
  }


  if(et_open(&id_to, to_et, openconfig) != ET_OK)
  {
    printf("%s: et_open 'to' problems\n",argv[0]);
    exit(1);
  }

  {
     et_id *idto = (et_id *) id_to;
    printf("11111: idto->endian=0x%08x idto->systemendian=0x%08x\n",idto->endian,idto->systemendian);
  }

printf("11\n");fflush(stdout);
  /* destroy configuration */
  et_open_config_destroy(openconfig);




  /* init station configuration */
  et_station_config_init(&sconfig);
  et_station_config_setuser(sconfig, ET_STATION_USER_MULTI);
  et_station_config_setrestore(sconfig, ET_STATION_RESTORE_OUT);
  et_station_config_setprescale(sconfig, 1);
  et_station_config_setcue(sconfig, 150);



/* ET system "all" mode 
  et_station_config_setselect(sconfig, ET_STATION_SELECT_ALL);
  et_station_config_setblock(sconfig, ET_STATION_BLOCKING);
*/

    /* ET system "on req" mode */
  et_station_config_setselect(sconfig, ET_STATION_SELECT_ALL);
  et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
  





  /* ET system "condition" mode 
  et_station_config_setselect(sconfig, ET_STATION_SELECT_MATCH);
  et_station_config_setblock(sconfig, ET_STATION_BLOCKING);
  et_station_config_setselectwords(sconfig, selections);
  */
  /* new non-blocking "condition" mode 
  et_station_config_setselect(sconfig, ET_STATION_SELECT_MATCH);
  et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
  et_station_config_setselectwords(sconfig, selections);
  */
  /* user's condition, blocking  mode 
  et_station_config_setselect(sconfig, ET_STATION_SELECT_USER);
  et_station_config_setblock(sconfig, ET_STATION_BLOCKING);
  et_station_config_setselectwords(sconfig, selections);
  if (et_station_config_setfunction(sconfig, "et_carls_function") == ET_ERROR) {
	printf("%s: cannot set function\n", argv[0]);
	exit(1);
  }
  if (et_station_config_setlib(sconfig, "/home/timmer/cvs/coda/source/et/src/libet_user.so") == ET_ERROR) {
    printf("%s: cannot set library\n", argv[0]);
	exit(1);
  }
  */
  /* user's condition, nonblocking mode 
  et_station_config_setselect(sconfig, ET_STATION_SELECT_USER);
  et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
  et_station_config_setselectwords(sconfig, selections);
  et_station_config_setfunction(sconfig, "et_carls_function");
  et_station_config_setlib(sconfig, "/home/timmer/cvs/coda/source/et/src/libet_user.so");
  */
  
  /* set debug level */
  et_system_setdebug(id_from, ET_DEBUG_INFO);
  et_system_setdebug(id_to,   ET_DEBUG_INFO);

  if ((status = et_station_create(id_from, &stat_from, from_station, sconfig)) < ET_OK) {
    if (status == ET_ERROR_EXISTS) {
      /* my_stat contains pointer to existing station */;
      printf("%s: station already exists, will attach to it\n", argv[0]);

      /* get id and attach to existing station (must be created by 'et_start' */
      if((status = et_station_name_to_id(id_from, &stat_from, from_station)) < ET_OK)
      {
        printf("%s: error in station_name_to_id\n", argv[0]);
        goto error;
      }

    }
    else
    {
      printf("%s: error in station creation, status=%d\n", argv[0],status);
      goto error;
    }
  }

  et_station_config_destroy(sconfig);
printf("13\n");fflush(stdout);





  /* */
  if (et_station_attach(id_from, stat_from, &att_from) < 0) {
    printf("%s: error in station attach\n", argv[0]);
    goto error;
  }
printf("14\n");fflush(stdout);

  if (et_station_attach(id_to, ET_GRANDCENTRAL, &att_to) < 0) {
    printf("%s: error in station attach\n", argv[0]);
    goto error;
  }
printf("15\n");fflush(stdout);

  et_bridge_config_init_hipo(&bconfig);


  /*et_bridge_config_setfunc_hipo(bconfig, et_bridge_CODAswap_hipo);*/


/*
printf("=====================================\n");
printf("=====================================\n");
printf("=====================================\n");
printf("setting swap function 'et_bridge_HIPO'\n");
printf("=====================================\n");
printf("=====================================\n");
printf("=====================================\n");
  et_bridge_config_setfunc_hipo(bconfig, et_bridge_HIPO);
*/



  /* infinite loop */
  while(status == ET_OK)
  {
    status = et_events_bridge_hipo(id_from, id_to, att_from, att_to, bconfig, NUMEVENTS, &ntransferred);
  }

  et_bridge_config_destroy_hipo(bconfig);

  et_forcedclose(id_from);
  et_forcedclose(id_to);
  exit(0);

error:
  exit(1);
}


/************************************************************/
/*              separate thread to handle signals           */
static void *
signal_thread (void *arg)
{
  sigset_t   signal_set;
  int        sig_number;
 
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGINT);
  
  /* Not necessary to clean up as ET system will do it */
  sigwait(&signal_set, &sig_number);
  printf("et_2_et: got a control-C, exiting\n");

  exit(1);
}





