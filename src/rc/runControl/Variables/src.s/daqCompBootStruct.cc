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
//      CODA run control: component auto boot values
//
// Author:  
//      Jie Chen
//      CEBAF Data Acquisition Group
//
// Revision History:
//   $Log: daqCompBootStruct.cc,v $
//   Revision 1.3  1997/08/25 16:02:02  heyes
//   fix some display problems
//
//   Revision 1.2  1996/12/04 18:32:21  chen
//   port to 1.4 on hp and ultrix
//
//   Revision 1.1.1.1  1996/10/11 13:39:30  chen
//   run control source
//
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <daqArbStructFactory.h>
#include "daqCompBootStruct.h"

int daqCompBootStruct::maxNumComps = 150;
int daqCompBootStruct::maxCompNameLen = 80;

daqCompBootStruct::daqCompBootStruct (void)
:daqArbStruct (), id_ (daqArbStructFactory::BOOT_INFO), numComponents_ (0)
{
#ifdef _TRACE_OBJECTS
  printf ("    Create daqCompBootStruct Class Object\n");
#endif
  a_.autoboot_ = new /*long*/int64_t[daqCompBootStruct::maxNumComps];
  c_.components_ = new char*[daqCompBootStruct::maxNumComps];

  for (int i = 0; i < daqCompBootStruct::maxNumComps; i++) {
    a_.autoboot_[i] = 0;
    c_.components_[i] = 0;
  }
}

daqCompBootStruct::~daqCompBootStruct (void)
{
#ifdef _TRACE_OBJECTS
  printf ("    Delete daqCompBootStruct Clas Object\n");
#endif

  for (int i = 0; i < numComponents_; i++)
    delete []c_.components_[i];

  delete []a_.autoboot_;
  delete []c_.components_;
}

daqArbStruct*
daqCompBootStruct::dup (void)
{
  daqCompBootStruct* tmp = new daqCompBootStruct ();

  if (numComponents_ > 0) {
    for (int i = 0; i < numComponents_; i++) {
      tmp->a_.autoboot_[i] = a_.autoboot_[i];
      tmp->c_.components_[i] = new char[daqCompBootStruct::maxCompNameLen];
      ::strcpy (tmp->c_.components_[i], c_.components_[i]);
    }
    tmp->numComponents_ = numComponents_;
  }

  tmp->id_ = id_;

  return tmp;
}

void
daqCompBootStruct::insertInfo (char* component, int autoboot)
{
  if (numComponents_ == (daqCompBootStruct::maxNumComps)) 
    fprintf (stderr, "daqCompBootStruct Error: overflow on insert\n");

  int i = numComponents_;
  c_.components_[i] = new char[daqCompBootStruct::maxCompNameLen];
  ::strcpy (c_.components_[i], component);
  a_.autoboot_[i] = autoboot;

  numComponents_ ++;

#ifdef _TRACE_OBJECTS
  printf("daqCompBootStruct::insertInfo: component >%s< numComponents_=%d ====================\n",component,numComponents_);
#endif
}

/*long*/int64_t
daqCompBootStruct::compBootInfo (char** &components, /*long*/int64_t* &autoboot)
{
  if (numComponents_ > 0) {
    components = c_.components_;
    autoboot = a_.autoboot_;
  }
  else {
    components = 0;
    autoboot = 0;
  }
  return numComponents_;
}

void
daqCompBootStruct::cleanUp (void)
{
  if (numComponents_ > 0) {
    for (int i = 0; i < numComponents_; i++)
      delete []c_.components_[i];
  }
  numComponents_ = 0;
#ifdef _TRACE_OBJECTS
  printf("daqCompBootStruct::cleanUp =====================\n");
#endif
}    

void
daqCompBootStruct::encodeData (void)
{
  if (numComponents_ > 0) {
    for (int i = 0; i < numComponents_; i++)
      a_.autoboot_[i] = htonl (a_.autoboot_[i]);
  }

#ifdef _TRACE_OBJECTS
  printf("daqCompBootStruct::encodeData(1) numComponents_=%d =============================\n",numComponents_);
#endif
  numComponents_ = htonl (numComponents_);
  id_ = htonl (id_);
#ifdef _TRACE_OBJECTS
  printf("daqCompBootStruct::encodeData(2) numComponents_=%d =============================\n",numComponents_);
#endif
}

void
daqCompBootStruct::restoreData (void)
{
#ifdef _TRACE_OBJECTS
  printf("daqCompBootStruct::restoreData(1) numComponents_=%d =============================\n",numComponents_);
#endif
  numComponents_ = ntohl (numComponents_);
#ifdef _TRACE_OBJECTS
  printf("daqCompBootStruct::restoreData(2) numComponents_=%d =============================\n",numComponents_);
#endif
  id_ = ntohl (id_);

  if (numComponents_ > 0) {
    for (int i = 0; i < numComponents_; i++)
      a_.autoboot_[i] = ntohl (a_.autoboot_[i]);
  }
}





void
daqCompBootStruct::encode (char* buffer, size_t& bufsize)
{
  int    i = 0, j = 0;
  int    numComps = numComponents_;

  long realsize = sizeof (daqCompBootStruct) - sizeof (daqArbStruct);


  // encode information data
  encodeData ();
  ::memcpy (buffer, (void *)&(this->id_), realsize);
  i += realsize;

  // copy autoboot to the buffer
  ::memcpy (&(buffer[i]), (void *)a_.autoboot_, numComps*sizeof (/*long*/int64_t));
  i += numComps*sizeof (/*long*/int64_t);

  // copy all components to the buffer
  for (j = 0; j < numComps; j++) {
    ::memcpy (&(buffer[i]), (void *)c_.components_[j], 
    daqCompBootStruct::maxCompNameLen);
    i = i + daqCompBootStruct::maxCompNameLen;
  }
  // restore data
  restoreData ();

  bufsize = (size_t)i;
}



void
daqCompBootStruct::decode (char* buffer, size_t size)
{
  int i = 0;
  int j = 0;
  
#ifdef _TRACE_OBJECTS
  printf("daqCompBootStruct::decode: size=%d (%d %d)\n",size,sizeof(daqCompBootStruct),sizeof(daqArbStruct));
#endif
  /* clean up old information (in particular sets 'numComponents_' to 0) */
  cleanUp ();

  long realsize = sizeof (daqCompBootStruct) - sizeof (daqArbStruct);
  
  /* copy header information (2 words: id_ and numComponents_, see header file) */
  ::memcpy ((void *)&(this->id_), buffer, 2*sizeof (int64_t/*long*/));

  // skip all other elements
  i += realsize;

#ifdef _TRACE_OBJECTS
  printf("daqCompBootStruct::decode: i(1)=%d\n",i);fflush(stdout);
  printf("daqCompBootStruct::decode: befor: numComponents_=%d id_=%d\n",numComponents_,id_);fflush(stdout);
#endif

  /* get number of components value */
  numComponents_ = ntohl (numComponents_);

  /* get id number */
  id_ = ntohl (id_);

#ifdef _TRACE_OBJECTS
  printf("daqCompBootStruct::decode: after: numComponents_=%d id_=%d\n",numComponents_,id_);fflush(stdout);
#endif

  if (numComponents_)
  {
    // make sure number of components < maximum number of components
    assert (numComponents_ < (daqCompBootStruct::maxNumComps));

    // copy auto boot information and convert to native byte order
    ::memcpy ((void *)a_.autoboot_, &(buffer[i]), numComponents_*sizeof (/*long*/int64_t));

    for (j = 0; j < numComponents_; j++) 
	{ 
      a_.autoboot_[j] = ntohl (a_.autoboot_[j]);
	}

    i += numComponents_*sizeof (/*long*/int64_t);
#ifdef _TRACE_OBJECTS
    printf("daqCompBootStruct::decode: i(2)=%d\n",i);fflush(stdout);
#endif
    // copy components name info
    for (j = 0; j < numComponents_; j++)
    {
      c_.components_[j] = new char[(daqCompBootStruct::maxCompNameLen)];
      ::memcpy ((void *)c_.components_[j], &(buffer[i]),
      daqCompBootStruct::maxCompNameLen);
      i = i + daqCompBootStruct::maxCompNameLen;
#ifdef _TRACE_OBJECTS
	  printf("daqCompBootStruct::decode: i(3)=%d (j=%d)\n",i,j);fflush(stdout);
#endif
    }
  }
  
  assert (i == size);
}


size_t
daqCompBootStruct::size (void)
{
  size_t s = 0;

  long realsize = sizeof (daqCompBootStruct) - sizeof (daqArbStruct);

  s += realsize;

  if (numComponents_ > 0) {
    s = s + sizeof (/*long*/int64_t) * numComponents_;
    s = s + numComponents_ * (daqCompBootStruct::maxCompNameLen);
  }
  return(s);
}

/*long*/int64_t
daqCompBootStruct::id (void)
{
  return id_;
}

void
daqCompBootStruct::dumpAll (void)
{
  for (int i = 0; i < numComponents_; i++)
  {
    printf ("daqCompBootStruct::dumpAll: Component %s boot info %d\n", c_.components_[i], a_.autoboot_[i]);
  }
}



  
  
