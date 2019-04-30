//-----------------------------------------------------------------------------
// Copyright (c) 1991,1992 Southeastern Universities Research Association,
//                         Continuous Electron Beam Accelerator Facility
//
// This software was developed under a United States Government license
// described in the NOTICE file included as part of this distribution.
//
// CEBAF Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606
// Email: coda@cebaf.gov  Tel: (804) 249-7101  Fax: (804) 249-7363
//-----------------------------------------------------------------------------
// 
// Description:
//	
//	
// Author:  Jie Chen
//       CEBAF Data Acquisition Group
//
// Revision History:
//   $Log: XcodaFileSelDialog.cc,v $
//   Revision 1.2  1998/04/08 17:28:54  heyes
//   get in there
//
//   Revision 1.1.1.1  1996/10/10 19:24:56  chen
//   coda motif C++ library
//
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

#include <XcodaErrorDialog.h>
#include <XcodaFileSelDialog.h>

/*
#define _TRACE_OBJECTS
*/

XcodaFileSelDialog::XcodaFileSelDialog(Widget parent, char *name, char *title)
:XcodaUi(name)
{
#ifdef _TRACE_OBJECTS
  printf("XcodaFileSelDialog: Create XcodaFileSelDialog Object\n");
#endif
  _errorDialog = 0;
  _filename = 0;
  _old_filename = 0;
  _parent = parent;
  _title = new char[strlen(title)+1];
  strcpy (_title, title);
  
  alreadyManaged = 0;
}

XcodaFileSelDialog::~XcodaFileSelDialog()
{
#ifdef _TRACE_OBJECTS
  printf("XcodaFileSelDialog: Delete XcodaFileSelDialog Object\n");
#endif
  if(_filename) delete []_filename;
  if(_old_filename) delete []_old_filename;
  /* no need to delete _errorDialog here, it will be freed upon X destruction */
  if (_title) delete []_title;
}


void
XcodaFileSelDialog::init()
{
  Arg arg[10];
  int ac = 0;
  XmString xms;
  char str[256];

#ifdef _TRACE_OBJECTS
  printf("------------------------------- XcodaFileSelDialog::init reached\n");
#endif

  if(_w == NULL)
  {
    _w = XmCreateFileSelectionDialog(_parent, _name, NULL, 0);
#ifdef _TRACE_OBJECTS
	printf("------------------------------- XcodaFileSelDialog::init: _w=0x%08x\n",_w);
#endif
    XtSetSensitive(XmFileSelectionBoxGetChild(_w, XmDIALOG_HELP_BUTTON), False);
    XtAddCallback (_w, XmNokCallback, (XtCallbackProc)&XcodaFileSelDialog::okCallback, (XtPointer)this);
    XtAddCallback (_w, XmNcancelCallback, (XtCallbackProc)&XcodaFileSelDialog::cancelCallback, (XtPointer)this);
    installDestroyHandler();

	/*sergey: set starting directory*/
    sprintf(str,"%s/trigger/*.trg",getenv("CLON_PARMS")); /**/
#ifdef _TRACE_OBJECTS
    printf("XcodaFileSelDialog::init: use starting directory >%s<\n",str);
#endif
    xms = XmStringCreateLocalized(str);
    XtVaSetValues( _w, XmNdirMask, xms, NULL );
    XmStringFree( xms ) ;


  }
}

void
XcodaFileSelDialog::popup()
{
  XmString xms;
  char str[256];

#ifdef _TRACE_OBJECTS
  printf("------------------------------- XcodaFileSelDialog::popup() reached\n");
#endif

  assert(_w);
  XtManageChild (_w);
  XtPopup(XtParent(_w), XtGrabNone);
  alreadyManaged = 1;

  if (_filename)
  {
#ifdef _TRACE_OBJECTS
    printf("UUUUUUUUUUUU1 _filename >%s<\n",_filename);
#endif
    if(_old_filename) delete []_old_filename;
    _old_filename = new char[strlen(_filename)+1];
    strcpy(_old_filename, _filename);
#ifdef _TRACE_OBJECTS
    printf("UUUUUUUUUUUU2 _old_filename >%s<\n",_old_filename);
#endif
    delete []_filename;
    _filename = 0;
  }


  /*sergey: set the name of the file (to be) chosen - by default always empty 
  sprintf(str,NULL);
  printf("starting with selection >%s<\n",str);
  xms = XmStringCreateLocalized(str);
  XtVaSetValues( _w, XmNdirSpec, xms, NULL );
  XmStringFree( xms ) ;
  */

}

void
XcodaFileSelDialog::popdown()
{
#ifdef _TRACE_OBJECTS
  printf("------------------------------- XcodaFileSelDialog::popdown() reached\n");
#endif

  assert(_w);
  XtPopdown (XtParent(_w));

  /*sergey: to force next popup's init() to create new 'FileSelectionDialog', so it picks newly created files in directory*/
  XtUnmanageChild(_w);
#ifdef _TRACE_OBJECTS
  printf("XcodaFileSelDialog::popdown: _w=0x%08x !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",_w);
#endif
  /*_w = NULL;*/ /* it comes from XcodaBasic.h, so it is rcRunConfigDialog's '_w' as well; if set to NULL,
                error will occur in rcRunConfigDialog::reportErrorMsg(_w,...)  */

  alreadyManaged = 0;
}

int
XcodaFileSelDialog::isMapped()
{
  return(alreadyManaged);
}

void
XcodaFileSelDialog::okCallback(Widget w, XtPointer client_data, XmFileSelectionBoxCallbackStruct *cbs)
{
  XcodaFileSelDialog *obj = (XcodaFileSelDialog *)client_data;
  obj->setCbs(cbs);
  obj->ok();
}

void
XcodaFileSelDialog::cancelCallback(Widget w, XtPointer client_data, XmFileSelectionBoxCallbackStruct *cbs)
{
  XcodaFileSelDialog *obj = (XcodaFileSelDialog *)client_data;
  obj->setCbs(cbs);
  obj->cancel();
}
 
void
XcodaFileSelDialog::ok()
{
  char *file;

#ifdef _TRACE_OBJECTS
  printf("XcodaFileSelDialog::ok() reached\n");
#endif
  if (!XmStringGetLtoR (_cbs->value, XmSTRING_DEFAULT_CHARSET, &file))
  {
    if (_errorDialog == NULL)
    {
      _errorDialog = new XcodaErrorDialog(_w, "FileSelectionError", "FileSelectionError");
      _errorDialog->init();
      _errorDialog->setMessage((char *)"Cannot open the file, Try again!");
    }
    _errorDialog->popup();
    file_status = NOTFOUND; 
    return;
  }

  if (*file != '/')
  {
    /* if it is not a directory, determine the full pathname 
       of the selection by concatenating it to the "dir" part */
    char *dir, *newfile;
    if (XmStringGetLtoR (_cbs->dir, XmSTRING_DEFAULT_CHARSET, &dir))
    {
      newfile = new char[strlen(dir)+1+strlen(_filename)+1];
      sprintf(newfile,"%s/%s",dir,file);
      XtFree (file);
      delete []dir;
      _filename = new char[strlen(newfile) + 1];
      strcpy (_filename, newfile);
      delete []newfile;
    }
  }
  else
  {
    if (_filename) delete []_filename;
    _filename = new char[strlen(file)+1];
    strcpy(_filename, file);
    XtFree (file);
  }

  switch(status(_filename))
  {
    case 1:
      file_status = READABLE;
      break;
    case 2:
      file_status = WRITABLE;
      break;
    case 0:
      file_status = DIREC;
      break;
    case -1:
      file_status = OPEN_ERR;
      break;
    case -2:
      file_status = NOTFOUND;
      break;
  }

  if(!checkFile())
  {
    char temp[100];
    if (_errorDialog == NULL)
    {
      _errorDialog = new XcodaErrorDialog(_w, "FileSelectionError", "FileSelectionError");
      _errorDialog->init();
    }
    else
	{
      _errorDialog->popdown();
	}
    sprintf(temp,"Cannot open file %s, try others!",_filename);
    _errorDialog->setMessage(temp);
    _errorDialog->popup();    
  }
  else
  {
#ifdef _TRACE_OBJECTS
    printf("XcodaFileSelDialog::ok() 111\n");
#endif
	/*
    char temp[100];
    sprintf(temp,"File >%s< selected\n",_filename);
    printf("File >%s< selected\n",_filename);
	*/
	//selectedFileName();
    popdown();
	//selectedFileName();
    execute();
	//selectedFileName();
  }
}

void
XcodaFileSelDialog::cancel()
{
  if (_filename)
  {
    delete []_filename;
  }
  if (_old_filename)
  {
    _filename = new char[strlen(_old_filename)+1];
    strcpy (_filename, _old_filename);
  }
  popdown();
}

/* if it exist, derived class's execute() never called ...
void
XcodaFileSelDialog::execute()
{
  printf("XcodaFileSelDialog::execute() reached\n");
  //_parent->execute();
  //empty
}
*/

int
XcodaFileSelDialog::status(char *fname)
{
  struct stat s_buf;

  if(stat(fname, &s_buf) == -1)
  {
    return(-2);
  }
  else if((s_buf.st_mode & S_IFMT) == S_IFDIR) // it is a directory
  {
    return(0);
  }
  else if(!(s_buf.st_mode & S_IFREG))
  {
    return(-1);
  }
  else /* regular file, check readable or writable */
  {
    int st = access(fname,R_OK);
    if(st <= -1)
	{
      return(-1);
	}
    else
    {
      if(access(fname,W_OK) > -1) return(2);
      else                        return(1);
    }
  }
}

char *
XcodaFileSelDialog::selectedFileName()
{
  char *p;
#ifdef _TRACE_OBJECTS
  printf("XcodaFileSelDialog::selectedFileName(): _filename >%s  _old_filename >%s\n", _filename, _old_filename);
#endif
  if (_filename)
  {
    return _filename;
  }
  else
  {
    return(0);
  }
}

int
XcodaFileSelDialog::checkFile()
{
  if((fileStatus()) < READABLE) return(0);
  else                          return(1);
}

void
XcodaFileSelDialog::setCbs(XmFileSelectionBoxCallbackStruct *cbs)
{
  _cbs = cbs;
}


/*sergey: add following function(s)*/
int
XcodaFileSelDialog::deleteFileName()
{
#ifdef _TRACE_OBJECTS
  printf("------------------------------- XcodaFileSelDialog::deleteFileName() reached\n");
#endif
  if(_filename) delete []_filename;
  _filename = 0;
  return(0);
}
