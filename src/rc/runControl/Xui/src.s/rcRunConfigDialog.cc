//-----------------------------------------------------------------------------
// Copyright (c) 1994,1995 Southeastern Universities Research Association,
//                         Continuous Electron Beam Accelerator Facility
//
// This software was developed under a United States Government license
// described in the NOTICE file included as part of this distribution.
//
// CEBAF Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606
//       coda@cebaf.gov  Tel: (804) 249-7030     Fax: (804) 249-5800
//-----------------------------------------------------------------------------
//
// Description:
//      Implementation of run config dialog
//
// Author:  
//      Jie Chen
//      CEBAF Data Acquisition Group
//
// Revision History:
//   $Log: rcRunConfigDialog.cc,v $
//   Revision 1.6  1998/06/18 12:20:42  heyes
//   new GUI ready I think
//
//   Revision 1.5  1998/04/08 18:31:33  heyes
//   new look and feel GUI
//
//   Revision 1.4  1997/10/15 16:08:32  heyes
//   embed dbedit, ddmon and codaedit
//
//   Revision 1.3  1997/08/01 18:38:17  heyes
//   nobody will believe this!
//
//   Revision 1.2  1997/06/16 12:26:50  heyes
//   add dbedit and so on
//
//   Revision 1.1.1.1  1996/10/11 13:39:26  chen
//   run control source
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*for chmod */
#include <sys/types.h>
#include <sys/stat.h>


#include <Xm/Form.h>
#include <Xm/PushBG.h>

#include <rcDbaseHandler.h>
#include <XcodaErrorDialog.h>
/*
#ifdef USE_CREG
#include <codaRegistry.h>
#endif
*/
#include "rcRunConfigDialog.h"
#include "rcXpmComdButton.h"


/*
#define _TRACE_OBJECTS
*/

rcRunConfigDialog::rcRunConfigDialog (Widget parent,
				  char* name,
				  char* title,
				  rcClientHandler& handler)
:XcodaFileSelDialog (parent, name, title), netHandler_ (handler),
 option_ (0), errDialog_ (0)
{
#ifdef _TRACE_OBJECTS
  printf ("rcRunConfigDialog: Create rcRunConfigDialog Class Object, _w=0x%08x !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",_w);
#endif

  /* _w = parent; sergey ??? */
}

rcRunConfigDialog::~rcRunConfigDialog (void)
{
#ifdef _TRACE_OBJECTS
  printf ("rcRunConfigDialog: Delete rcRunConfigDialog Class Object\n");
#endif
  // empty
  // option_ and dialog_ will be destroyed by Xt destroy mechanism
}



void
rcRunConfigDialog::download (void)
{ 
  rcClient& client = netHandler_.clientHandler ();
#ifdef _CODA_DEBUG
  printf(">>> rcRunConfigDialog::download reached\n");
#endif
  daqData data ((char *)"RCS", (char *)"command", (int)DADOWNLOAD/*currentruntype*/);

  /* sending download command to rcServer::daqRun() */
  if (client.sendCmdCallback (DADOWNLOAD, data,
		 (rcCallback)&(rcRunConfigDialog::downloadCallback),
		 (void *)this) != CODA_SUCCESS)
  {
    reportErrorMsg ((char *)"Cannot communication with the RunControl Server\n");
  }

}


void
rcRunConfigDialog::popup (void)
{
#ifdef _TRACE_OBJECTS
  printf("rcRunConfigDialog::popup reached\n");
#endif
  XcodaFileSelDialog::init(); /* creates new 'FileSelectionDialog', so it picks newly created files in directory */
#ifdef _TRACE_OBJECTS
  printf("rcRunConfigDialog::popup 1, _w=0x%08x\n",_w);
#endif
  XcodaFileSelDialog::popup();
#ifdef _TRACE_OBJECTS
  printf("rcRunConfigDialog::popup 2, _w=0x%08x\n",_w);
#endif
}





#define STRLEN 256

/* parse run config file *.trg resolving 'include' etc, and create new file in the same directory
with name <fname>.cnf; 'fname' assumed to be full path name */
int
rcRunConfigDialog::parseConfigFile(char *fname, int iter, char errmsg[STRLEN])
{
  FILE *fin, *fout, *fd;
  char fnameout[STRLEN];
  char fntmp[STRLEN];
  char filename[STRLEN], filename_full[STRLEN];
  char str_tmp[STRLEN];
  char keyword[STRLEN];
  char *clonparms;
  char *expid;
  char *ch, *ptr;
  int nargs;
  mode_t mode;

  clonparms = getenv("CLON_PARMS");

  if((fin=fopen(fname,"r")) == NULL)
  {
    sprintf(errmsg,"\nrcRunConfigDialog::parseConfigFile: ERROR: Can't open input run config file >%s<\n",fname);
    return(-1);
  }

  strcpy(fnameout,fname);
  ptr = strrchr(fnameout,'.');
  if(ptr != NULL)
  {
    *ptr = '\0';
    if(iter>0) sprintf(fntmp,"%s.cnf_%d\0",fnameout,iter);
    else       sprintf(fntmp,"%s.cnf\0",fnameout);
    strcpy(fnameout,fntmp);
  }
  else
  {
    sprintf(errmsg, "rcRunConfigDialog::parseConfigFile: ERROR: there is no '.' in file name >%s<\n",fnameout);
    return(-2);
  }

  if((fout=fopen(fnameout,"w")) == NULL)
  {
    sprintf(errmsg, "\nrcRunConfigDialog::parseConfigFile: ERROR: Can't open output run config file >%s<\n",fnameout);
    return(-3);
  }

#ifdef _TRACE_OBJECTS
  printf("\nrcRunConfigDialog::parseConfigFile: parsing run config file >%s<\n",fname);
  printf("\nrcRunConfigDialog::parseConfigFile: writing run config file >%s<\n",fnameout);
#endif

  while ((ch = fgets(str_tmp, STRLEN, fin)) != NULL)
  {
    nargs = sscanf (str_tmp, "%s %s", keyword, filename);

	/*
    printf("keyword >%s<\n",keyword);
	*/





    /* Start parsing real config inputs */
	if(nargs==2 && (strcmp(keyword,"include")==0 || strcmp(keyword,"INCLUDE")==0) )
    {

      if(strlen(filename)!=0) /* filename specified */
      {
        if ( filename[0]=='/' || (filename[0]=='.' && filename[1]=='/') )
	    {
          sprintf(filename_full, "%s", filename);
	    }
        else
	    {
          sprintf(filename_full, "%s/%s", clonparms, filename);
	    }

        if((fd = fopen(filename_full,"r")) == NULL)
        {
          sprintf(errmsg, "\nrcRunConfigDialog::ReadConfigFile: Can't open config file >%s<\n",filename_full);
          return(-4);
        }
#ifdef _TRACE_OBJECTS
        else
		{
          printf("\nrcRunConfigDialog::ReadConfigFile: including config file >%s<\n",filename_full);
	    }
#endif

        /* write 'include statement commenting it out */
        strcpy(str_tmp,"\n# ");
        strcat(str_tmp,keyword);
        strcat(str_tmp," ");
        strcat(str_tmp,filename);
        strcat(str_tmp,"\n\n");
        if( fputs(str_tmp, fout) == EOF)
	    {
          sprintf(errmsg, "rcRunConfigDialog::ReadConfigFile: ERROR writing %s\n",fnameout);
          return(-5);
	    }
        /*else
	    {
          printf("Write >%s<\n",str_tmp);
		}*/



        /* open and copy included file contents */
        while ((ch = fgets(str_tmp, STRLEN, fd)) != NULL)
		{
          if( fputs(str_tmp, fout) == EOF)
	      {
            sprintf(errmsg, "rcRunConfigDialog::ReadConfigFile: ERROR writing %s\n",fnameout);
            return(-6);
	      }
          /*else
	      {
            printf("Write >%s<\n",str_tmp);
		  }*/
		}
		fclose(fd);




      }
      else
	  {
        sprintf(errmsg, "rcRunConfigDialog::ReadConfigFile: ERROR included file is not specified\n");
        return(-7);
	  }





    }
    else /* just copy string 'as is' */
	{
      if( fputs(str_tmp, fout) == EOF)
	  {
        sprintf(errmsg, "rcRunConfigDialog::ReadConfigFile: ERROR writing %s\n",fnameout);
        return(-8);
	  }
      /*else
	  {
        printf("Write >%s<\n",str_tmp);
	  }*/
    }


  } /* end of while */

  fclose(fin);
  fclose(fout);

  /* open output file for everybody */
  mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;
  if(chmod(fnameout, mode) != 0) /*open file for everybody*/
  {
    printf("rcRunConfigDialog::ReadConfigFile: ERROR: cannot change mode on output run config file\n");
  }
  else
  {
    printf("rcRunConfigDialog::ReadConfigFile: INFO: changed mode on output run config file - opened for everybody\n");
  }

  return(0);
}










/* update database by sending request through rcClientHandler->rcClient->rcServer->...*/
void
rcRunConfigDialog::updateConfFile (char *fname)
{
  /* get client handler */
  rcClient& client=netHandler_.clientHandler();
  daqData data (client.exptname (), (char *)"confFile", fname);
  if (client.setValueCallback (data,
                  (rcCallback)&(rcRunConfigDialog::confFileCallback),
                  (void *)this) != CODA_SUCCESS)
  {
    reportErrorMsg ((char *)"Cannot send new token interval value to the server !");
    return;
  }

  return;
}



void                                                                                                                    
rcRunConfigDialog::confFileCallback (int status, void* arg, daqNetData* )                                                     
{                                                                                                                       
  rcRunConfigDialog* obj = (rcRunConfigDialog *)arg;                                                                          
  if (status != CODA_SUCCESS)                                                                                           
  {                                                                                                                     
    obj->reportErrorMsg ((char *)"Setting new token interval to the server failed !");                                          
    return;                                                                                                             
  }                                                                                                                     
}



/*sergey: '_w' here from XcodaBasic.h !!!???*/

void
rcRunConfigDialog::reportErrorMsg (char* error)
{
#ifdef _TRACE_OBJECTS
  printf("rcRunConfigDialog::reportErrorMsg: _w=0x%08x error >%s<\n",_w,error);fflush(stdout);
#endif
  /*sergey*/
  if(_w==NULL)
  {
    printf("rcRunConfigDialog::reportErrorMsg: return\n");fflush(stdout);
    return;
  }
  /*sergey*/

  if (!errDialog_)
  {
#ifdef _TRACE_OBJECTS
    printf("rcRunConfigDialog::reportErrorMsg: _w = 0x%08x, errDialog_ = 0x%08x\n",_w,errDialog_);fflush(stdout);
#endif
    errDialog_ = new XcodaErrorDialog (_w, "runConfigError", "Configuration Error");
#ifdef _TRACE_OBJECTS
    printf("rcRunConfigDialog::reportErrorMsg: errDialog_ = 0x%08x\n",errDialog_);fflush(stdout);
#endif
    errDialog_->init ();
  }
  if (errDialog_->isMapped ()) errDialog_->popdown ();
  errDialog_->setMessage (error);
  errDialog_->popup ();
}


void
rcRunConfigDialog::downloadCallback (int status, void* arg, daqNetData* data)
{
  rcRunConfigDialog* obj = (rcRunConfigDialog *)arg;

  if (status != CODA_SUCCESS) obj->reportErrorMsg ((char *)"Downloading a run failed !!!");
}




void
rcRunConfigDialog::execute (void)
{
  char filename[STRLEN], fname[STRLEN], fnam1[STRLEN], fnam2[STRLEN], *fn, *ptr;
  char errmsg[STRLEN];
  int len, ret = 0;

#ifdef _CODA_DEBUG
  printf("rcRunConfigDialog::execute: ------------> execute reached\n");
  printf("rcRunConfigDialog::execute: ------------> selected file name >%s<\n",XcodaFileSelDialog::selectedFileName());
#endif

  strcpy(filename,"none");

  /* get file name selected */
  fn = XcodaFileSelDialog::selectedFileName();

  if(fn) strcpy(filename,fn);
  else strcpy(filename,"none");

  /* cleanup filename so next popup returns 0 if file not selected - DOES NOT WORK !!!*/
  XcodaFileSelDialog::deleteFileName();

  len = strlen(filename);
#ifdef _CODA_DEBUG
  printf("rcRunConfigDialog::execute:  >>%s<<\n",(char *)&filename[len-4]);
#endif
  if( !strncmp((char *)&filename[len-4],"NONE",4) ) strcpy(filename,"none");


#ifdef _CODA_DEBUG
  printf("rcRunConfigDialog::execute: >>>>>>>>>>>>> FILE1 >%s<\n",filename);fflush(stdout);
#endif




  /* resolve INCLUDE statements in the file */
  strcpy(fname,filename);
  ptr = strrchr(fname,'.');
  if(ptr != NULL)
  {
    *ptr = '\0'; /* remove extension from 'fname' */

    strcpy(fnam1,filename);
    for(int iter=5; iter>=0; iter--)
    {
      if( (ret = parseConfigFile(fnam1, iter, errmsg)) < 0 )
      {
        printf("rcRunConfigDialog::execute: ERROR in parseRunConfigFile: ret=%d\n",ret);
        strcpy(filename,"none");
        reportErrorMsg(errmsg);
        break;
      }
      else /* change extension for next iteration, in last iteration it has to become '.cnf' */
      {
        if(iter>0) sprintf(fnam1,"%s.cnf_%d\0",fname,iter);
        else       sprintf(fnam1,"%s.cnf\0",fname);
      }
    }

    /* remove intermediate files */
    for(int iter=5; iter>0; iter--)
    {
      sprintf(fnam1,"%s.cnf_%d\0",fname,iter);
      remove(fnam1);
    }

    /* if not 'none': config file in database (and runcontrol gui) need .cnf extension, not .trg */
    if(strncmp(filename,"none",4)) sprintf(filename,"%s.cnf\0",fname);

  }
  else
  {
    printf("rcRunConfigDialog::execute: ERROR: there is no '.' in file name >%s<\n",fname);
    strcpy(filename,"none");
  }





#ifdef _CODA_DEBUG
  printf("rcRunConfigDialog::execute: >>>>>>>>>>>>> FILE2 >%s<\n",filename);fflush(stdout);
#endif
  
  /*update database*/
  updateConfFile(filename);

#ifdef _CODA_DEBUG
  printf("rcRunConfigDialog::execute: >>>>>>>>>>>>> FILE3 >%s<\n",filename);fflush(stdout);
#endif

  /* download if it was no error; from inside 'download()' we'll send DADOWNLOAD command to daqRun.cc */
  if(ret==0)
  {
    download();
  }
  else
  {
    printf("rcRunConfigDialog::execute: do NOT download !!!\n");
  }
}
