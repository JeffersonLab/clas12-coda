/* cron_exec.cxx - Valery Sytnik */

/*

Usage:

0-59/1 * * * *  /bin/csh -c "(source /home/clasioc/.cshrc; cioc_cron 1) > ~/.crontab_ioccaen1 "

cioc_cron:

  #!/bin/bash
  cron_exec $1 ioccaen cstart "/usr/local/clas12/release/0.1/epics/drivers/CAEN_HV/level0/IocShell"

$1 - can be 0, 1 or 2:
  0 - forced stop (call from crontab will be disabled)
  1 - call from crontab to check status
  2 - forced start (restart)  (call from crontab will be enabled)

$2 ioccaen - program name, will be used in 'ps' to find/kill/etc

$3 cstart: startup script, for example
  #!/bin/bash
  O.linux-x86/ioccaen startup.all &> /dev/null &
or executable

$4 - path to go



for DiagGuiServer:

@reboot  /bin/csh -c "(source /home/clasioc/.cshrc; diagguiserver_init_cron 1) > ~/.crontab_diagguiserver_init "
0-59/1 * * * *  /bin/csh -c "(source /home/clasioc/.cshrc; diagguiserver_cron 1) > ~/.crontab_diagguiserver "



diagguiserver_cron:

  #!/bin/bash
  cron_exec $1 DiagGuiServer diagguiserver "/usr/local/clas12/release/current/coda/common/scripts"

diagguiserver_init_cron:

  #!/bin/bash
  cron_exec $1 DiagGuiServer diagguiserver_init "/usr/local/clas12/release/current/coda/common/scripts"



diagguiserver:

  #!/bin/bash
  DiagGuiServer &> /dev/null &

diagguiserver_init:

  #!/bin/bash
  DiagGuiServer init &> /dev/null &

*/

#if defined(Linux_vme)

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <limits.h>

#include "string"
#include <semaphore.h>

using namespace std;

static FILE *fp_check;
static char file_ps[300];

int is_process_running(int is_start, char *process_template, char *semkey );

int
main(int argc, char *argv[])
{
  printf("argv[0]..[4] >%s< >%s< >%s< >%s< >%s<\n",argv[0],argv[1],argv[2],argv[3],argv[4]);
  
  char SEM_NAME[40]= "hall_b_sc_";
  sem_t *mutex;
  int ret;
  char tmp[PATH_MAX];
  char *get_user;
  char *get_home;
  char tmp_check[100];
  char *myargv[10];

  get_home = getenv("HOME");
  get_user = getenv("USER");

  strcpy(tmp_check, get_home);
  strcat(tmp_check, "/");
  strcat(tmp_check, ".cron_check_");
  strcat(tmp_check,argv[2]);
  fp_check=fopen(tmp_check,"a+");

  strcat(SEM_NAME,get_user);
  strcat(SEM_NAME,"_");
  strcat(SEM_NAME,argv[3]);

  strcpy(file_ps, get_home);
  strcat(file_ps,"/");
  strcat(file_ps,".");
  strcat(file_ps,argv[2]);
  // strcpy(file_status,file_ps);
  strcat(file_ps,".txt");
  // strcat(file_status,".txt");
  printf("file_ps=%s sem_name=%s\n", file_ps, SEM_NAME);

  printf("====1\n");
  int is_start = atoi(argv[1]);
  char semkey[100];
  if(is_start)
  {
    if(argc>=6)strcpy(semkey, argv[5]);
    else strcpy(semkey,"");
  }

  if(is_start==2)
  {
    ret=sem_unlink(SEM_NAME);
    if(ret<0)
    {
      perror("unable to kill semaphore");
    }
  }
  mutex = sem_open(SEM_NAME,O_CREAT,0777,1);
  if(mutex == SEM_FAILED)
  {
    perror("unable to create semaphore");
    sem_unlink(SEM_NAME);
    exit(-1);
  }

  ret = 0;
  if(is_start==1)
  {
    ret=sem_trywait(mutex);
  }
  else sem_wait(mutex);
  if(ret<0) return 1;

  if(is_process_running(is_start, argv[2], semkey ))
  {
    printf("process is running - return 1\n");
    if(is_start!=0)sem_post(mutex);
    sem_close(mutex);
    return(1);
  }
  else
  {
    printf("process is not running - will start it\n");
  }

  sem_post(mutex);
  sem_close(mutex);

  sleep(1);

  fprintf(fp_check, "start %s\n", argv[2]);

  for(int i=0;i<10;i++)    //name of program and /0
  {
    myargv[i]= (char *) malloc(600); // alloc 100 bytes
  }

  printf("launch \n");

  if(fork()==0)
  {
    printf("launch %s\n", argv[2]);
    int ret; 
    ret= chdir(argv[4]);
    printf("launch ret=%d dir=%s \n", ret, argv[4]);
            strcpy(myargv[0], argv[3]);
            myargv[1]=(char *)0;

    strcpy(tmp,"./");
    strcat(tmp,argv[3]);
    execv(tmp,myargv);
    printf("not started \n");
  }

  wait(0);

  return(1);
}




int
is_process_running(int is_start, char *process_template, char *semkey )
{
  FILE *fp;
  char line[PATH_MAX];   
  int isEnabled;
  int ret, retpid;

  char command_ps[PATH_MAX];
  char retline[PATH_MAX];
  char *retline1;

  string line_string;

  retline1 = (char *) &retline;

  strcpy(command_ps,"/sbin/pidof ");
  strcat(command_ps,process_template);   
  strcat(command_ps," > ");
  strcat(command_ps, "/dev/null");
  retpid = system(command_ps);  
  printf("execute >%s<\n",command_ps);
  printf("pid=%d\n",retpid);

  if(is_start==1)  /// call from crontab
  {
    if(retpid==0 || retpid==-1) /// no start as the process named process_template is running or error in request of this
	{
      return(1);
	}
    else /// although the process is supposed not running, we do killing just in case (not to have two processes)
    {
      fprintf(fp_check, "start_check %s retpid=%d\n", process_template, retpid);

      strcpy(command_ps,"killall ");
      strcat(command_ps,process_template);
      strcat(command_ps," > /dev/null");
      ret=system(command_ps);
    }
  }
  else if(is_start==0) /// forced stop (call from crontab will be disabled)
  {
    strcpy(command_ps,"killall ");
    strcat(command_ps,process_template);
    strcat(command_ps," > /dev/null");
    ret=system(command_ps);
    return 1; /// no start if we stop 
  }
  else if(is_start==2) /// forced start (restart)  (call from crontab will be enabled)
  {
    strcpy(command_ps,"killall ");
    strcat(command_ps,process_template);
    strcat(command_ps," > /dev/null");
    ret=system(command_ps);
  }

///--------------------------------------------------------------------------------------

  if(!strcmp(semkey,"")) return(0);  /// if no semkey is used (mpod, old caen, e.g.) we do not remove a semaphore

  strcpy(command_ps,"ipcs -s"); 
  strcat(command_ps," > ");
  strcat(command_ps,file_ps);
  ret=system(command_ps);
  if(ret!=0)  /// any error means : no start
  {
    printf("ipcs command error \n");
    return(1);
  }
  sleep(1);
  fp=fopen(file_ps,"r");
  if(fp==NULL) /// any error means : no start 
  {
    printf("file open failure\n");
    return(1);
  }

  char *pos, *pos1=0;
  char *pos_found_not_space;

  while(1)
  {
    retline1 = (char *)fgets((char *)line, PATH_MAX, fp);
    if(retline1==NULL) break; /// normal situation at PC restart

    line_string = string((char *)line);

    if( (pos=strstr(line, semkey)) )
    {
      pos1=pos+strlen(semkey);
      pos_found_not_space=0;
      while(1)
      {
        if(*(pos1)!=0x20)
        {
          pos_found_not_space=pos1;
          break;
        }
        pos1++;
      }

      while(1)
      {
        if(*(pos1)==0x20)
        {
          *(pos1)=0;
          strcpy(line,pos_found_not_space);
          break;
        }
        pos1++;
      }
      printf("semid=%s %d\n",line, atoi(line));
      strcpy(command_ps,"ipcrm sem "); 
      strcat(command_ps,line);
      strcat(command_ps," > /dev/null");
      ret=system(command_ps);
      if(ret!=0){printf("ipcrm command error \n");return 1;}  /// any error means : no start 

      break;
    } // if( (pos=strstr(line, semkey)) )
  } // while(1);

  return 0;
}

#else

int
main()
{
  return(0);
}

#endif
