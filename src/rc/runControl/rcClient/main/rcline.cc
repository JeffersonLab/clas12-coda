
/* TEST program to communicate with run control server */

/*
rcServer -m clondb1 -d clasdev -s clastest0
./Linux_i686/bin/rcline clasdev clastest0 clondb1

type commands:

  load
  configure
    sector5_er
  download
  prestart
  go
  end

  monitorOn
    ER15 erate

  getvalue
    ER15 state

  setvalue
    clastest (session)
    confFile
    bla
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <rcSvcProtocol.h>
/* inside:
#define DYN_ATTR0         "nlongs" 
#define DYN_ATTR1         "nevents" 
#define DYN_ATTR2         "status"  
#define DYN_ATTR3         "erate"  
#define DYN_ATTR4         "drate" 
#define DYN_ATTR5         "livetime"
*/


#include <XcodaApp.h>
#include <rcMsg.h>
#include <rcClient.h>


/* to resolve reference in rcClient.cc */
Display *MainDisplay = NULL;


static void
callback (int status, void* arg, daqNetData* data)
{
  if (status == CODA_SUCCESS)
  {
    printf ("command finished successfully\n");
  }
  else
  {
    printf ("command failed ++++++++++++++++++\n");    
  }
}

static void
getValCallback (int status, void* arg, daqNetData* data)
{
  rcClient* obj = (rcClient *)arg;

  if (status == CODA_SUCCESS)
  {
    printf ("get comp >%s< attr>%s< data >%s<\n",data->name(), data->attribute(),
	    (char *)(*data));
    //printf ("Pend IO command returns %d\n", obj->pendIO (2.0));
  }
  else
  { 
    printf ("Get value has bad value\n");
  }
}

static void
setValCallback (int status, void* arg, daqNetData* data)
{
  if (status == CODA_SUCCESS)
  {
    printf ("set comp >%s< attr >%s< data >%s<\n",data->name(), data->attribute(),
	    (char *)(*data));
  }
  else
  { 
    printf ("set value has bad value\n");
  }
}

static void
monCallback (int status, void* arg, daqNetData* data)
{
  if (status == CODA_SUCCESS)
  {
    printf ("comp >%s< attr >%s< data >%s<\n",data->name(), data->attribute(), (char *)(*data));
  }
  else
  { 
    printf ("monitor value has bad value\n");
  }
}

static void
msgCallback (int status, void* arg, daqNetData* data)
{
  if (status == CODA_SUCCESS)
  {
    printf ("%s", (char *)(*data));
  }
}

static void
monOffCallback (int status, void* arg, daqNetData* data)
{
  if (status == CODA_SUCCESS)
  {
    printf ("data %s %s monitor off \n",data->name(), data->attribute());
  }
  else
  { 
    printf ("monitor off failed\n");
  }
}



static void
statusCallback (int status, void* arg, daqNetData* data)
{
  if (status == CODA_SUCCESS)
  {
    printf ("statusCallback: %s", (char *)(*data));
  }
  else
  {
    printf("statusCallback: NOT CODA_SUCCESS (status=%d)\n",status);
  }
}



int
main (int argc, char **argv)
{
  rcClient handler_;
  int status;
  char dbase[256];
  char session[256];

  if (argc != 4)
  {
    fprintf (stderr, "Usage: %s expid session mysqld\n", argv[0]);
    exit (1);
  }
  
  strcpy (dbase, argv[1]);
  strcpy (session, argv[2]);

  status = handler_.connect (argv[1], argv[2], argv[3]);
  if (status != CODA_SUCCESS)
  {
    printf ("%s RunControl Server is not running\n", argv[2]);
    exit (1);
  }
  else
  {
    printf ("Connected to %s RunControl Server\n", argv[2]);
  }


  handler_.monitorOnCallback ((char *)"RCS", (char *)"runMessage", msgCallback, 0);

  /* in runcontrol something like that::
  if (handler_.monitorOnCallback (session, "status", statusCallback, 0) != CODA_SUCCESS)
  {
    printf("Cannot monitor on >%s< session\n", session);
  }
  */


  char command[32];
  int count;
  printf ("Enter rcServer Command\n");
  while (1)
  {
    ioctl (fileno (stdin), FIONREAD, &count);
    if (count > 0)
    {
      scanf ("%s", command);
      if (::strcmp (command, "disconnect") == 0)
	  {
        handler_.disconnect ();
	  }
      else if (::strcmp (command, "quit") == 0)
	  {
        break;
	  }
      else if (::strcmp (command, "load") == 0)
      {
	    char* temp[2];

	    daqData data ((char *)"RCS", (char *)"command", (char *)"unknown");
	    // insert database name + session name into data object
	    temp[0] = new char[::strlen (dbase) + 1];
	    ::strcpy (temp[0], dbase);
	    temp[1] = new char[::strlen (session) + 1];
	    ::strcpy (temp[1], session);
	    data.assignData (temp, 2);
	    // free memory
	    delete []temp[0]; 
	    delete []temp[1];
  	
	    status = handler_.sendCmdCallback (DALOADDBASE, data, callback, 0);
      }
      else if (::strcmp (command, "getruntypes") == 0)
	  {
	    status = handler_.getValueCallback (argv[2], (char *)"allRunTypes", getValCallback, (void *)&handler_);
	  }
      else if (::strcmp (command, "getconffile") == 0)
	  {
	    status = handler_.getValueCallback (argv[2], (char *)"confFile", getValCallback, (void *)&handler_);
	  }
      else if (::strcmp (command, "configure") == 0)
      {
	    printf ("Enter configuration name\n");
	    {
	      char runtype[32];
	      scanf ("%s",runtype);
	      daqData data ((char *)"RCS",(char *)"command",runtype);
	      status = handler_.sendCmdCallback (DACONFIGURE, data, callback, 0);
	    }
      }
      else if (::strcmp (command, "download") == 0)
      {
	    daqData data ((char *)"RCS", (char *)"command", (int)DADOWNLOAD);
	    status = handler_.sendCmdCallback (DADOWNLOAD, data, callback, 0);
      }	
      else if (::strcmp (command, "prestart") == 0)
      {
	    daqData data ((char *)"RCS", (char *)"command", (int)DAPRESTART);
	    status = handler_.sendCmdCallback (DAPRESTART, data, callback, 0);
      }	
      else if (::strcmp (command, "go") == 0)
      {
	    daqData data ((char *)"RCS", (char *)"command", (int)DAGO);
	    status = handler_.sendCmdCallback (DAGO, data, callback, 0);
      }	
      else if (::strcmp (command, "end") == 0)
      {
	    daqData data ((char *)"RCS", (char *)"command", (int)DAEND);
	    status = handler_.sendCmdCallback (DAEND, data, callback, 0);
      }	
      else if (::strcmp (command, "abort") == 0)
      {
	    daqData data ((char *)"RCS", (char *)"command", (int)DAABORT);
	    status = handler_.sendCmdCallback (DAABORT, data, callback, 0);
      }
      else if (::strcmp (command, "reset") == 0)
      {
	    daqData data ((char *)"RCS", (char *)"command", (int)DATERMINATE);
	    status = handler_.sendCmdCallback (DATERMINATE, data, callback, 0);
      }
      else if (::strcmp (command, "getvalue") == 0)
      {
	    printf ("Enter component + attribute\n");
	    {
	      char compname[32];
	      char attr[32];
	      scanf ("%s %s",compname, attr);
	      handler_.getValueCallback (compname, attr, getValCallback, (void *)&handler_);
	    }
      }
      else if (::strcmp (command, "setvalue") == 0)
      {
	    printf ("Enter component + attribute + value (string)\n");
	    {
	      char compname[32];
	      char attr[32];
	      char val[256];
	      scanf ("%s %s %s",compname, attr, val);
	      daqData data (compname, attr, val);
	      handler_.setValueCallback (data, setValCallback, 0);
	    }
      }
      else if (::strcmp (command, "monitorOn") == 0)
      {
	    printf ("Enter component + attribute\n");
	    {
	      char compname[32];
	      char attr[32];
	      scanf ("%s %s",compname, attr);
	      handler_.monitorOnCallback (compname, attr, monCallback, 0);
	    }
      }
      else if (::strcmp (command, "monitorOff") == 0)
      {
	    printf ("Enter component + attribute\n");
	    {
	      char compname[32];
	      char attr[32];
	      scanf ("%s %s",compname, attr);
	      handler_.monitorOffCallback (compname, attr, monCallback, 0,
				     monOffCallback, 0);
	    }
      }
      else if (::strcmp (command, "changeState") == 0)
      {
	    printf ("Enter initial and final state\n");
	    {
	      int state[2];
	      scanf ("%d %d",&state[0], &state[1]);
	      daqData data ((char *)"RCS",(char *)"command", state, 2);
	      handler_.sendCmdCallback (DACHANGE_STATE, data, callback, 0);
	    }
      }
      else if (::strcmp (command, "test") == 0)
      {
	    daqData data ((char *)"RCS", (char *)"command", (int)DATEST);	
	    status = handler_.sendCmdCallback (DATEST, data, callback, 0);
      }
      else
	  {
	    printf ("Illegal command \n");
	  }
      printf ("Enter rcServer Command\n");
      fflush (stdout);
    }
    handler_.pendIO (0.1);
  }
}
