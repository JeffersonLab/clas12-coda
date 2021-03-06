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
//      CODA run control: monitoring parameters
//
// Author:  
//      Jie Chen
//      CEBAF Data Acquisition Group
//
// Revision History:
//   $Log: daqMonitorStruct.cc,v $
//   Revision 1.3  1998/09/18 14:56:22  heyes
//   change monitor interval
//
//   Revision 1.2  1996/12/04 18:32:21  chen
//   port to 1.4 on hp and ultrix
//
//   Revision 1.1  1996/11/04 16:15:42  chen
//   monitoring option data structure
//
//
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <daqArbStructFactory.h>
#include "daqMonitorStruct.h"

int daqMonitorStruct::maxNumComps = 150;
int daqMonitorStruct::maxCompNameLen = 80;

daqMonitorStruct::daqMonitorStruct (void)
:daqArbStruct (), id_ (daqArbStructFactory::MONITOR_INFO), numComponents_ (0),
 autoend_ (1), interval_ (240)
{
#ifdef _TRACE_OBJECTS
  printf ("    Create daqMonitorStruct Clas Object\n");
#endif
  m_.monitored_ = new int64_t/*long*/[daqMonitorStruct::maxNumComps];

  c_.components_ = new char*[daqMonitorStruct::maxNumComps];

  for (int i = 0; i < daqMonitorStruct::maxNumComps; i++)
  {
    m_.monitored_[i] = -1;
    c_.components_[i] = 0;
  }
}

daqMonitorStruct::~daqMonitorStruct (void)
{
#ifdef _TRACE_OBJECTS
  printf ("    Delete daqMonitorStruct Clas Object\n");
#endif

  for (int i = 0; i < numComponents_; i++)
    delete []c_.components_[i];

  delete []m_.monitored_;
  delete []c_.components_;
}

daqArbStruct*
daqMonitorStruct::dup (void)
{
  daqMonitorStruct* tmp = new daqMonitorStruct ();

  if (numComponents_ > 0)
  {
    for (int i = 0; i < numComponents_; i++)
    {
      tmp->m_.monitored_[i] = m_.monitored_[i];
      tmp->c_.components_[i] = new char[daqMonitorStruct::maxCompNameLen];
      ::strcpy (tmp->c_.components_[i], c_.components_[i]);
    }
  }
  tmp->numComponents_ = numComponents_;
  tmp->autoend_ = autoend_;
  tmp->interval_ = interval_;
  
  tmp->id_ = id_;

  return tmp;
}

void
daqMonitorStruct::insertInfo (char* component, int monitored)
{
  if (numComponents_ == (daqMonitorStruct::maxNumComps)) 
    fprintf (stderr, "daqMonitorStruct Error: overflow on insert\n");

  int i = numComponents_;
  c_.components_[i] = new char[daqMonitorStruct::maxCompNameLen];
  ::strcpy (c_.components_[i], component);
  m_.monitored_[i] = monitored;

  numComponents_ ++;
}

void
daqMonitorStruct::enableAutoEnd (void)
{
  autoend_ = 1;
}

void
daqMonitorStruct::disableAutoEnd (void)
{
  autoend_ = 0;
}

/*long*/int64_t
daqMonitorStruct::autoend (void) const
{
  return autoend_;
}

void
daqMonitorStruct::timerInterval (long interval)
{
  interval_ = interval;
}

/*long*/int64_t
daqMonitorStruct::timerInterval (void) const
{
  return interval_;
}


/*long*/int64_t
daqMonitorStruct::monitorParms (char** &components, int64_t*/*long*/ &monitored, long& autoend, long& interval)
{
  if (numComponents_ > 0)
  {
    components = c_.components_;
    monitored = m_.monitored_;
  }
  else
  {
    components = 0;
    monitored = 0;
  }
  autoend = autoend_;
  interval = interval_;

  return numComponents_;
}

void
daqMonitorStruct::cleanUp (void)
{
  if (numComponents_ > 0) {
    for (int i = 0; i < numComponents_; i++)
      delete []c_.components_[i];
  }
  numComponents_ = 0;
}    

void
daqMonitorStruct::encodeData (void)
{
  if (numComponents_ > 0)
  {
    for (int i = 0; i < numComponents_; i++)
      m_.monitored_[i] = htonl (m_.monitored_[i]);
  }
  numComponents_ = htonl (numComponents_);
  id_ = htonl (id_);
  autoend_ = htonl (autoend_);
  interval_ = htonl (interval_);
}

void
daqMonitorStruct::restoreData (void)
{
  numComponents_ = ntohl (numComponents_);
  id_ = ntohl (id_);
  autoend_ = ntohl (autoend_);
  interval_ = ntohl (interval_);

  if (numComponents_ > 0)
  {
    for (int i = 0; i < numComponents_; i++)
      m_.monitored_[i] = ntohl (m_.monitored_[i]);
  }
}

void
daqMonitorStruct::encode (char* buffer, size_t& bufsize)
{
  int    i = 0, j = 0;
  int    numComps = numComponents_;

  long realsize = sizeof (daqMonitorStruct) - sizeof (daqArbStruct);

  // encode information data
  encodeData ();
  ::memcpy (buffer, (void *)&(this->id_), realsize);
  i += realsize;

  // copy autoboot to the buffer
  ::memcpy (&(buffer[i]), (void *)m_.monitored_, numComps*sizeof (/*long*/int64_t));
  i += numComps*sizeof (/*long*/int64_t);

  // copy all components to the buffer
  for (j = 0; j < numComps; j++)
  {
    ::memcpy (&(buffer[i]), (void *)c_.components_[j], 
    daqMonitorStruct::maxCompNameLen);
    i = i + daqMonitorStruct::maxCompNameLen;
  }

  // restore data
  restoreData ();

  bufsize = (size_t)i;
}

void
daqMonitorStruct::decode (char* buffer, size_t size)
{
  int i = 0;
  int j = 0;
  
  // clean up old information
  cleanUp ();

  long realsize = sizeof (daqMonitorStruct) - sizeof (daqArbStruct);
  
  // copy header information
  ::memcpy ((void *)&(this->id_), buffer, 4*sizeof (/*long*/int64_t));
  // skip all other elements
  i += realsize;
  // get number of components value
  numComponents_ = ntohl (numComponents_);
  // get id number and more
  id_ = ntohl (id_);
  autoend_ = ntohl (autoend_);
  interval_ = ntohl (interval_);

  if (numComponents_)
  {
    // make sure number of components < maximum number of components
    assert (numComponents_ < (daqMonitorStruct::maxNumComps));

    // copy auto boot information and convert to native byte order
    ::memcpy ((void *)m_.monitored_, &(buffer[i]), numComponents_*sizeof (/*long*/int64_t));

    for (j = 0; j < numComponents_; j++) 
      m_.monitored_[j] = ntohl (m_.monitored_[j]);

    i += numComponents_*sizeof (/*long*/int64_t);

    // copy components name info
    for (j = 0; j < numComponents_; j++)
    {
      c_.components_[j] = new char[(daqMonitorStruct::maxCompNameLen)];
      ::memcpy ((void *)c_.components_[j], &(buffer[i]),
      daqMonitorStruct::maxCompNameLen);
      i = i + daqMonitorStruct::maxCompNameLen;
    }
  }
  
  assert (i == size);
}

size_t
daqMonitorStruct::size (void)
{
  size_t s = 0;

  long realsize = sizeof (daqMonitorStruct) - sizeof (daqArbStruct);

  s += realsize;

  if (numComponents_ > 0)
  {
    s = s + sizeof (/*long*/int64_t)*numComponents_;
    s = s + numComponents_*(daqMonitorStruct::maxCompNameLen);
  }
  return s;
}

/*long*/int64_t
daqMonitorStruct::id (void)
{
  return id_;
}

void
daqMonitorStruct::dumpAll (void)
{
  for (int i = 0; i < numComponents_; i++)
  {
    printf ("Component %s monitored info %d\n", c_.components_[i],
	    m_.monitored_[i]);
  }
  printf ("Autoend: %d and interval %d\n", autoend_, interval_);
}



  
  
