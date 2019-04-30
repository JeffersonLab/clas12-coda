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
//      Implementation of daqNetData  
//
// Author:  
//      Jie Chen
//      CEBAF Data Acquisition Group
//
// Revision History:
//   $Log: daqNetData.cc,v $
//   Revision 1.2  1999/11/29 19:54:55  rwm
//   Nix compiler warnings: cast.
//
//   Revision 1.1.1.1  1996/10/11 13:39:30  chen
//   run control source
//
//

#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <daqArbStructFactory.h>

#include "daqNetData.h"


#undef _TRACE_OBJECTS

const int CODA_FDCONV_LEN = 32;

/* split a char buffer contains multiple char string terminated by NULL
   char into an array of char string */

static void codaSplitBuffer (char **s, char *buffer, int count, int& size)
{
  char *p, *q;
  int  i = 0;

  p = q = buffer;
  size = 0;
  while (i < count)
  {
    int len = 0;
    while (*q != '\0')
    {
      q++; len++; size++;
    }
    s[i] = new char[len + 1];
    ::strncpy (s[i], p, len+1);

    /* move cursor to next string */
    q++; size++;
    p = q;
    i++;
  }
}
  
/*sergey: changed type for 'data' from 'int' to 'int64_t*/
#ifdef Linux_x86_64
daqNetData::daqNetData (char* compname, char* attrname, int64_t data)
:type_ (CODA_INT64), count_ (1)
{
#ifdef _TRACE_OBJECTS
  printf ("Create daqNetData Class Object 1\n");
#endif
  ctrNameAndAttr (compname, attrname);
  u_.lval = data;
}
#endif

daqNetData::daqNetData (char* compname, char* attrname, int data)
:type_ (CODA_INT32), count_ (1)
{
#ifdef _TRACE_OBJECTS
  printf ("Create daqNetData Class Object 2\n");
#endif
  ctrNameAndAttr (compname, attrname);
  u_.ival = data;
}

daqNetData::daqNetData (char* compname, char* attrname, float data)
:type_ (CODA_FLT), count_ (1)
{
#ifdef _TRACE_OBJECTS
  printf ("Create daqNetData Class Object 3\n");
#endif
  ctrNameAndAttr (compname, attrname);
  u_.fval = data;
}

daqNetData::daqNetData (char* compname, char* attrname, double data)
:type_ (CODA_DBL), count_ (1)
{
#ifdef _TRACE_OBJECTS
  printf ("Create daqNetData Class Object 4\n");
#endif
  ctrNameAndAttr (compname, attrname);
  u_.dval = data;
}

daqNetData::daqNetData (char* compname, char* attrname, char* data)
:type_ (CODA_STR), count_ (1)
{
#ifdef _TRACE_OBJECTS
  printf ("Create daqNetData Class Object 5\n");
#endif
  // data cannot be null
  assert (data);

  ctrNameAndAttr (compname, attrname);
  // This buffer will be around until this data is deleted
  u_.sval = new char[CODA_CONV_LEN];
  ::strncpy (u_.sval, data, CODA_CONV_LEN);
}

daqNetData::daqNetData (char* compname, char* attrname, daqArbStruct* data)
:type_ (CODA_STRUCT), count_ (1)
{
#ifdef _TRACE_OBJECTS
  printf ("Create daqNetData Class Object 6\n");
#endif
  // data cannot be null
  assert (data);

  ctrNameAndAttr (compname, attrname);
  u_.arb = data->dup ();
}



#ifdef Linux_x86_64
daqNetData::daqNetData (char* compname, char* attrname, int64_t* data, int count)
:type_ (CODA_INT64), count_ (count)
{
#ifdef _TRACE_OBJECTS
  printf ("Create daqNetData Class Object 7\n");
#endif
  // data cannot be null
  assert (data);
  assert (count > 0);

  ctrNameAndAttr (compname, attrname);

  if (count_ == 1) 
    u_.lval = data[0];
  else {
    int64_t *tdata = new int64_t[count];
    for (int i = 0; i < count; i++)
      tdata[i] = data[i];
    u_.data = (void *)tdata;
  }
}
#endif

daqNetData::daqNetData (char* compname, char* attrname, int* data, int count)
:type_ (CODA_INT32), count_ (count)
{
#ifdef _TRACE_OBJECTS
  printf ("Create daqNetData Class Object 8\n");
#endif
  // data cannot be null
  assert (data);
  assert (count > 0);

  ctrNameAndAttr (compname, attrname);

  if (count_ == 1) 
    u_.ival = data[0];
  else {
    int *tdata = new int[count];
    for (int i = 0; i < count; i++)
      tdata[i] = data[i];
    u_.data = (void *)tdata;
  }
}

daqNetData::daqNetData (char* compname, char* attrname, float* data, int count)
:type_ (CODA_FLT), count_ (count)
{
#ifdef _TRACE_OBJECTS
  printf ("Create daqNetData Class Object 9\n");
#endif
  // data cannot be null
  assert (data);
  assert (count > 0);

  ctrNameAndAttr (compname, attrname);

  if (count_ == 1) 
    u_.fval = data[0];
  else {
    float *tdata = new float[count];
    for (int i = 0; i < count; i++) 
      tdata[i] = data[i];
    u_.data = (void *)tdata;
  }
}

daqNetData::daqNetData (char* compname, char* attrname, double* data, int count)
:type_ (CODA_DBL), count_ (count)
{
#ifdef _TRACE_OBJECTS
  printf ("Create daqNetData Class Object 10\n");
#endif
  // data cannot be null
  assert (data);
  assert (count > 0);

  ctrNameAndAttr (compname, attrname);

  if (count_ == 1)
    u_.dval = data[0];
  else {
    double *tdata = new double[count];
    for (int i = 0; i < count; i++) 
      tdata[i] = data[i];
  
    u_.data = (void *)tdata;
  }
}

daqNetData::daqNetData (char* compname, char* attrname, char** data, int count)
:type_ (CODA_STR), count_ (count)
{
#ifdef _TRACE_OBJECTS
  printf ("Create daqNetData Class Object 11: count=%d\n",count);
#endif
  // data cannot be null
  assert (data);
  assert (count > 0);

  ctrNameAndAttr (compname, attrname);
  
  if (count_ == 1)
  {
    /* This buffer will be around until this data is deleted */
    u_.sval = new char[CODA_CONV_LEN];
    ::strncpy (u_.sval, data[0], CODA_CONV_LEN);
  }
  else
  {
    char** tdata = new char*[count];
    for (int i = 0; i < count; i++)
    {
#ifdef _TRACE_OBJECTS
      printf("[%d] len=%d\n",i,::strlen (data[i]));
#endif
      tdata[i] = new char[::strlen (data[i]) + 1];
      ::strcpy (tdata[i], data[i]);
#ifdef _TRACE_OBJECTS
	  printf("[%d] >%s<\n",i,tdata[i]);
#endif
   }
    u_.data = (void *)tdata;
  }
}

daqNetData::daqNetData (const daqNetData& data)
:type_ (data.type_),  count_ (data.count_)
{
#ifdef _TRACE_OBJECTS
  printf ("Create daqNetData Class Object 12\n");
#endif
  if (data.c_.compname_ && data.a_.attrname_)
  {
    ctrNameAndAttr (data.c_.compname_, data.a_.attrname_);
  }
  else
  {
    c_.compname_ = 0;
    a_.attrname_ = 0;
  }

  switch (type_)
  {

  case CODA_INT64:
    if (count_ == 1)
    {
      u_.lval = data.u_.lval;
    }
    else
    {
      int64_t *tdata = new int64_t[count_];
      int64_t *cdata = (int64_t *)data.u_.data;
      for (int i = 0; i < count_; i++)
      {
	    tdata[i] = cdata[i];
      }
      u_.data = (void *)tdata;
    }
    break;

  case CODA_INT32:
    if (count_ == 1)
    {
      u_.ival = data.u_.ival;
    }
    else
    {
      int *tdata = new int[count_];
      int *cdata = (int *)data.u_.data;
      for (int i = 0; i < count_; i++)
      {
	    tdata[i] = cdata[i];
      }
      u_.data = (void *)tdata;
    }
    break;

  case CODA_FLT:
    if (count_ == 1) {
      u_.fval = data.u_.fval;
    }
    else {
      float *tdata = new float[count_];
      float *cdata = (float *)data.u_.data;
      for (int i = 0; i < count_; i++) {
	tdata[i] = cdata[i];
      }
      u_.data = (void *)tdata;
    }
    break;

  case CODA_DBL:
    if (count_ == 1) {
      u_.dval = data.u_.dval;
    }
    else {
      double *tdata = new double[count_];
      double *cdata = (double *)data.u_.data;
      for (int i = 0; i < count_; i++) {
	tdata[i] = cdata[i];
      }
      u_.data = (void *)tdata;
    }
    break;

  case CODA_STR:
#ifdef _TRACE_OBJECTS
    printf ("Create daqNetData Class Object 105: count_=%d\n",count_);
#endif
    if (count_ == 1)
    {
      u_.sval = new char[CODA_CONV_LEN];
      ::strncpy (u_.sval, data.u_.sval, CODA_CONV_LEN);
    }
    else
    {
      char** tdata = new char*[count_];
      char** cdata = (char **)data.u_.data;
      for (int i = 0; i < count_; i++)
      {
	    tdata[i] = new char[::strlen (cdata[i]) + 1];
	    ::strcpy (tdata[i], cdata[i]);
#ifdef _TRACE_OBJECTS
        printf("105: [%d] >%s<\n",i,tdata[i]);
#endif
      }
      u_.data = (void *)tdata;
    }
    break;

  case CODA_STRUCT:
    u_.arb = data.u_.arb->dup ();
    break;

  default:
    break;
  }
}


daqNetData&
daqNetData::operator = (const daqNetData& data)
{
#ifdef _TRACE_OBJECTS
  printf ("Create daqNetData: operator =\n");
#endif
  if (this != &data)
  {
    // delete old component and attribute name
    if (c_.compname_)
      delete []c_.compname_;
    nameLen_ = data.nameLen_;
    if (nameLen_ == 0)
      c_.compname_ = 0;
    else {
      c_.compname_ = new char[nameLen_];
      ::strcpy (c_.compname_, data.c_.compname_);
    }

    if (a_.attrname_)
      delete []a_.attrname_;
    attrLen_ = data.attrLen_;
    if (attrLen_ == 0)
      a_.attrname_ = 0;
    else {
      a_.attrname_ = new char[attrLen_];
      ::strcpy (a_.attrname_, data.a_.attrname_);
    }

    // free old memory
    if (count_ > 1)
      freeBufferMemory ();
    else if (count_ == 1) {
      if (type_ == CODA_STR)
	delete []u_.sval;
      else if (type_ = CODA_STRUCT)
	delete u_.arb;
    }
    // assign new type
    type_ = data.type_;
    count_ = data.count_;
    
    if (count_ != 0 && type_ != CODA_UNKNOWN) {
      switch (type_)
      {

      case CODA_INT64:
	if (count_ == 1) 
	  u_.lval = data.u_.lval;
	else {
	  int64_t *tdata = new int64_t[count_];
	  int64_t *cdata = (int64_t *)data.u_.data;
	  for (int i = 0; i < count_; i++) {
	    tdata[i] = cdata[i];
	  }
	  u_.data = (void *)tdata;
	}
	break;

      case CODA_INT32:
	if (count_ == 1) 
	  u_.ival = data.u_.ival;
	else {
	  int *tdata = new int[count_];
	  int *cdata = (int *)data.u_.data;
	  for (int i = 0; i < count_; i++) {
	    tdata[i] = cdata[i];
	  }
	  u_.data = (void *)tdata;
	}
	break;

      case CODA_FLT:
	if (count_ == 1) 
	  u_.fval = data.u_.fval;
	else {
	  float *tdata = new float[count_];
	  float *cdata = (float *)data.u_.data;
	  for (int i = 0; i < count_; i++) {
	    tdata[i] = cdata[i];
	  }
	  u_.data = (void *)tdata;
	}
	break;
      case CODA_DBL:
	if (count_ == 1) 
	  u_.dval = data.u_.dval;
	else {
	  double *tdata = new double[count_];
	  double *cdata = (double *)data.u_.data;
	  for (int i = 0; i < count_; i++) {
	    tdata[i] = cdata[i];
	  }
	  u_.data = (void *)tdata;
	}
	break;
      case CODA_STR:
	if (count_ == 1) {
	  u_.sval = new char[CODA_CONV_LEN];
	  ::strncpy (u_.sval, data.u_.sval, CODA_CONV_LEN);
	}
	else {
	  char** tdata = new char*[count_];
	  char** cdata = (char **)data.u_.data;
	  for (int i = 0; i < count_; i++) {
	    tdata[i] = new char[::strlen (cdata[i]) + 1];
	    ::strcpy (tdata[i], cdata[i]);
	  }
	  u_.data = (void *)tdata;
	}
	break;
      case CODA_STRUCT:
	u_.arb = data.u_.arb->dup ();
	break;
      default:
	break;
      }
    }
  }
  return *this;
}

daqNetData::daqNetData (void)
{
#ifdef _TRACE_OBJECTS
  printf ("Create daqNetData Class Object 00\n");
#endif
  a_.attrname_ = 0;
  c_.compname_ = 0;
  count_ = 0;
  nameLen_ = 0;
  attrLen_ = 0;
  type_ = CODA_UNKNOWN;
}
  
daqNetData::~daqNetData (void)
{
#ifdef _TRACE_OBJECTS
  printf ("Delete daqNetData Class Object\n");
#endif
  if (a_.attrname_)
    delete []a_.attrname_;
  if (c_.compname_)
    delete []c_.compname_;

  if (count_ == 1) {
    if (type_ == CODA_STR) {
      delete [](u_.sval);
    }
    else if (type_ == CODA_STRUCT)
      delete u_.arb;
  }
  else if (count_ > 1) {
    switch (type_)
    {
    case CODA_INT64:
    case CODA_INT32:
    case CODA_FLT:
    case CODA_DBL:
	  {
      double *ddata = (double *)u_.data;
      delete []ddata;
	  }
      break;
    case CODA_STR:
      {
	char **cdata = (char **)u_.data;
	for (int i = 0; i < count_; i++)
	  delete []cdata[i];
	delete []cdata;
      }
      break;
    default:
      break;
    }
  }
}

void
daqNetData::ctrNameAndAttr (char *compname, char *attrname)
{

  /* sergey: some checks */
  if(compname==NULL || attrname==NULL)
  {
    printf("daqNetData::ctrNameAndAttr: ERROR: compname >%s<, attrname >%s<\n",compname,attrname);
    fflush(stdout);
    return;
  }

  nameLen_ = ::strlen (compname) + 1;
  c_.compname_ = new char[nameLen_];
  ::strcpy (c_.compname_, compname);  

  attrLen_ = ::strlen (attrname) + 1;
  a_.attrname_ = new char[attrLen_];
  ::strcpy (a_.attrname_, attrname);
}

void
daqNetData::freeBufferMemory (void)
{
  switch (type_){
  case CODA_INT64:
  case CODA_INT32:
  case CODA_FLT:
  case CODA_DBL:
	{
      double *ddata = (double *)u_.data;
      delete []ddata;
	}
    break;
  case CODA_STR:
    {
      char **cdata = (char **)u_.data;
      for (int i = 0; i < count_; i++)
	delete []cdata[i];
      delete []cdata;
    }
    break;
  default:
    break;
  }
}

daqNetData&
daqNetData::operator = (int val)
{
  if (count_ == 0)
    return *this;

  int needStrAlloc = 0;
  if (count_ > 1) {
    freeBufferMemory ();
    needStrAlloc = 1;
  }
  else {
    if (type_ == CODA_STRUCT) {
      delete u_.arb;
      type_ = CODA_INT32;
    }
  }

  count_ = 1;
  switch (type_) {
  case CODA_INT64:
    u_.lval = val;
    break;
  case CODA_INT32:
    u_.ival = val;
    break;
  case CODA_FLT:
    u_.fval = (float) val;
    break;
  case CODA_DBL:
    u_.dval = (double) val;
    break;
  case CODA_STR:
    if (needStrAlloc)
      u_.sval = new char[CODA_CONV_LEN];
    ::sprintf (u_.sval, "%d", val);
    break;
  default:
    break;
  }
  return *this;
}

daqNetData&
daqNetData::operator = (int64_t val)
{
  if (count_ == 0)
    return *this;
  
  int needStrAlloc = 0;
  if (count_ > 1) {
    freeBufferMemory ();
    needStrAlloc = 1;
  }
  else {
    if (type_ == CODA_STRUCT) {
      delete u_.arb;
      type_ = CODA_INT32;
    }
  }

  count_ = 1;
  switch (type_) {
  case CODA_INT64:
    u_.lval = (int)val;
    break;
  case CODA_INT32:
    u_.ival = (int)val;
    break;
  case CODA_FLT:
    u_.fval = (float) val;
    break;
  case CODA_DBL:
    u_.dval = (double) val;
    break;
  case CODA_STR:
    if (needStrAlloc)
      u_.sval = new char[CODA_CONV_LEN];
    ::sprintf (u_.sval, "%ld", val);
    break;
  default:
    break;
  }
  return *this;
}

daqNetData&
daqNetData::operator = (float val)
{
  if (count_ == 0)
    return *this;

  int needStrAlloc = 0;
  if (count_ > 1) {
    freeBufferMemory ();
    needStrAlloc = 1;
  }
  else {
    if (type_ == CODA_STRUCT) {
      delete u_.arb;
      type_ = CODA_FLT;
    }
  }

  count_ = 1;
  switch (type_) {
  case CODA_INT64:
    u_.lval = (int)val;
    break;
  case CODA_INT32:
    u_.ival = (int)val;
    break;
  case CODA_FLT:
    u_.fval = val;
    break;
  case CODA_DBL:
    u_.dval = (double) val;
    break;
  case CODA_STR:
    if (needStrAlloc)
      u_.sval = new char[CODA_CONV_LEN];
    sprintf (u_.sval, "%f", val);
    break;
  default:
    break;
  }
  return *this;
}

daqNetData&
daqNetData::operator = (double val)
{
  if (count_ == 0)
    return *this;

  int needStrAlloc = 0;
  if (count_ > 1) {
    freeBufferMemory ();
    needStrAlloc = 1;
  }
  else {
    if (type_ == CODA_STRUCT) {
      delete u_.arb;
      type_ = CODA_DBL;
    }
  }

  count_ = 1;
  switch (type_) {
  case CODA_INT64:
    u_.lval = (int)val;
    break;
  case CODA_INT32:
    u_.ival = (int)val;
    break;
  case CODA_FLT:
    u_.fval = (float)val;
    break;
  case CODA_DBL:
    u_.dval = val;
    break;
  case CODA_STR:
    if (needStrAlloc)
      u_.sval = new char[CODA_CONV_LEN];
    sprintf (u_.sval, "%lf", val);
    break;
  default:
    break;
  }
  return *this;
}

daqNetData&
daqNetData::operator = (char* val)
{
  if (count_ == 0)
    return *this;
  
  int needStrAlloc = 0;
  if (count_ > 1) { 
    freeBufferMemory ();
    needStrAlloc = 1;
  }
  else {
    if (type_ == CODA_STRUCT) {
      delete u_.arb;
      type_ = CODA_STR;
      needStrAlloc = 1;
    }
  }

  int status;
  count_ = 1;

  switch (type_) {
  case CODA_INT64:
    {
      int64_t tval;
      status = sscanf (val, "%lld", &tval);
      if (status == 1) 
	u_.lval = tval;
    }
    break;
  case CODA_INT32:
    {
      int tval;
      status = sscanf (val, "%d", &tval);
      if (status == 1) 
	u_.ival = tval;
    }
    break;
  case CODA_FLT:
    {
      float tval;
      status = sscanf (val, "%f", &tval);
      if (status == 1) 
	u_.fval = tval;
    }
    break;
  case CODA_DBL:
    {
      double tval;
      status = sscanf (val, "%lf", &tval);
      if (status == 1) 
	u_.dval = tval;
    }
    break;
  case CODA_STR:
    if (needStrAlloc)
      u_.sval = new char[CODA_CONV_LEN];
    ::strncpy (u_.sval, val, CODA_CONV_LEN);
    break;
  default:
    break;
  }
  return *this;
}

daqNetData&
daqNetData::operator = (daqArbStruct* val)
{
  if (count_ == 0)
    return *this;
  
  if (count_ > 1) 
    freeBufferMemory ();
  else {
    if (type_ == CODA_STR)
      delete []u_.sval;
    else if (type_ == CODA_STRUCT) {
      delete u_.arb;
    }
  }

  type_ = CODA_STRUCT;
  count_ = 1;

  u_.arb = val->dup ();
  return *this;
}
  
void
daqNetData::assignData (int64_t* data, int count)
{
  if (count_ == 0)
    return;

  if (count == 1) {
    operator = (*data);
    return;
  }
  if (count_ > 1)
    freeBufferMemory ();
  else {
    if (type_ == CODA_STR)
      delete []u_.sval;
    else if (type_ == CODA_STRUCT) {
      delete u_.arb;
      type_ = CODA_INT32;
    }
  }
      
  count_ = count;
  switch (type_) {
  case CODA_INT64:
    {
      int64_t *tdata = new int64_t[count];
      for (int i = 0; i < count; i++)
	tdata[i] = data[i];
      u_.data = (void *)tdata;
    }
    break;
  case CODA_INT32:
    {
      int *tdata = new int[count];
      for (int i = 0; i < count; i++)
	tdata[i] = data[i];
      u_.data = (void *)tdata;
    }
    break;
  case CODA_FLT:
    {
      float *tdata = new float[count];
      for (int i = 0; i < count; i++)
	tdata[i] = (float)data[i];
      u_.data = (void *)tdata;
    }
    break;
  case CODA_DBL:
    {
      double *tdata = new double[count];
      for (int i = 0; i < count; i++)
	tdata[i] = (double)data[i];
      u_.data = (void *)tdata;
    }
    break;
  case CODA_STR:
    {
      char **tdata = new char*[count];
      char temp[32];
      for (int i = 0; i < count; i++) {
	sprintf (temp, "%-12d", data[i]);
	tdata[i] = new char[::strlen (temp) + 1];
	::strcpy (tdata[i], temp);
      }
      u_.data = (void *)tdata;
    }
    break;
  default:
    break;
  }
}

void
daqNetData::assignData (int* data, int count)
{
  if (count_ == 0)
    return;

  if (count == 1) {
    operator = (*data);
    return;
  }
  if (count_ > 1)
    freeBufferMemory ();
  else {
    if (type_ == CODA_STR)
      delete []u_.sval;
    else if (type_ == CODA_STRUCT) {
      delete u_.arb;
      type_ = CODA_INT32;
    }
  }
      
  count_ = count;
  switch (type_) {
  case CODA_INT64:
    {
      int64_t *tdata = new int64_t[count];
      for (int i = 0; i < count; i++)
	tdata[i] = data[i];
      u_.data = (void *)tdata;
    }
    break;
  case CODA_INT32:
    {
      int *tdata = new int[count];
      for (int i = 0; i < count; i++)
	tdata[i] = data[i];
      u_.data = (void *)tdata;
    }
    break;
  case CODA_FLT:
    {
      float *tdata = new float[count];
      for (int i = 0; i < count; i++)
	tdata[i] = (float)data[i];
      u_.data = (void *)tdata;
    }
    break;
  case CODA_DBL:
    {
      double *tdata = new double[count];
      for (int i = 0; i < count; i++)
	tdata[i] = (double)data[i];
      u_.data = (void *)tdata;
    }
    break;
  case CODA_STR:
    {
      char **tdata = new char*[count];
      char temp[32];
      for (int i = 0; i < count; i++) {
	sprintf (temp, "%-12d", data[i]);
	tdata[i] = new char[::strlen (temp) + 1];
	::strcpy (tdata[i], temp);
      }
      u_.data = (void *)tdata;
    }
    break;
  default:
    break;
  }
}

void
daqNetData::assignData (float* data, int count)
{
  if (count_ == 0)
    return;

  if (count == 1) {
    operator = (*data);
    return;
  }
  if (count_ > 1) 
    freeBufferMemory ();
  else {
    if (type_ == CODA_STR)
      delete []u_.sval;
    else if (type_ == CODA_STRUCT) {
      delete u_.arb;
      type_ = CODA_FLT;
    }
  }

  count_ = count;
  switch (type_) {
  case CODA_INT64:
    {
      int64_t *tdata = new int64_t[count];
      for (int i = 0; i < count; i++)
	tdata[i] = (int64_t)data[i];
      u_.data = (void *)tdata;
    }
    break;
  case CODA_INT32:
    {
      int *tdata = new int[count];
      for (int i = 0; i < count; i++)
	tdata[i] = (int)data[i];
      u_.data = (void *)tdata;
    }
    break;
  case CODA_FLT:
    {
      float *tdata = new float[count];
      for (int i = 0; i < count; i++)
	tdata[i] = data[i];
      u_.data = (void *)tdata;
    }
    break;
  case CODA_DBL:
    {
      double *tdata = new double[count];
      for (int i = 0; i < count; i++)
	tdata[i] = (double)data[i];
      u_.data = (void *)tdata;
    }
    break;
  case CODA_STR:
    {
      char **tdata = new char*[count];
      char temp[32];
      for (int i = 0; i < count; i++) {
	sprintf (temp, "%-20.4f", data[i]);
	tdata[i] = new char[::strlen (temp) + 1];
	::strcpy (tdata[i], temp);
      }
      u_.data = (void *)tdata;
    }
    break;
  default:
    break;
  }
}

void
daqNetData::assignData (double* data, int count)
{
  if (count_ == 0)
    return;

  if (count == 1) {
    operator = (*data);
    return;
  }
  if (count_ > 1) 
    freeBufferMemory ();
  else {
    if (type_ == CODA_STR)
      delete []u_.sval;
    else if (type_ == CODA_STRUCT) {
      delete u_.arb;
      type_ = CODA_DBL;
    }
  }
  count_ = count;
  switch (type_) {
  case CODA_INT64:
    {
      int64_t *tdata = new int64_t[count];
      for (int i = 0; i < count; i++)
	tdata[i] = (int64_t)data[i];
      u_.data = (void *)tdata;
    }
    break;
  case CODA_INT32:
    {
      int *tdata = new int[count];
      for (int i = 0; i < count; i++)
	tdata[i] = (int)data[i];
      u_.data = (void *)tdata;
    }
    break;
  case CODA_FLT:
    {
      float *tdata = new float[count];
      for (int i = 0; i < count; i++)
	tdata[i] = (float)data[i];
      u_.data = (void *)tdata;
    }
    break;
  case CODA_DBL:
    {
      double *tdata = new double[count];
      for (int i = 0; i < count; i++)
	tdata[i] = data[i];
      u_.data = (void *)tdata;
    }
    break;
  case CODA_STR:
    {
      char **tdata = new char*[count];
      char temp[32];
      for (int i = 0; i < count; i++) {
	sprintf (temp, "%-20.4lf", data[i]);
	tdata[i] = new char[::strlen (temp) + 1];
	::strcpy (tdata[i], temp);
      }
      u_.data = (void *)tdata;
    }
    break;
  default:
    break;
  }
}

void
daqNetData::assignData (char** data, int count)
{
  if (count_ == 0)
    return;

  if (count == 1) {
    operator = (*data);
    return;
  }
  if (count_ > 1) 
    freeBufferMemory ();
  else {
    if (type_ == CODA_STR)
      delete []u_.sval;
    else if (type_ == CODA_STRUCT) {
      delete u_.arb;
      type_ = CODA_STR;
    }
  }
  count_ = count;
  switch (type_) {
  case CODA_INT64:
    {
      int64_t *tdata = new int64_t[count];
      for (int i = 0; i < count; i++) {
	if (::sscanf (data[i], "%lld", &tdata[i]) != 1)
	  tdata[i] = 0;
      }
      u_.data = (void *)tdata;
    }
    break;
  case CODA_INT32:
    {
      int *tdata = new int[count];
      for (int i = 0; i < count; i++) {
	if (::sscanf (data[i], "%d", &tdata[i]) != 1)
	  tdata[i] = 0;
      }
      u_.data = (void *)tdata;
    }
    break;
  case CODA_FLT:
    {
      float *tdata = new float[count];
      for (int i = 0; i < count; i++) {
	if (::sscanf (data[i],"%f", &tdata[i]) != 1) 
	tdata[i] = 0.0;
      }
      u_.data = (void *)tdata;
    }
    break;
  case CODA_DBL:
    {
      double *tdata = new double[count];
      for (int i = 0; i < count; i++) {
	if (::sscanf (data[i], "%lf", &(tdata[i])) != 1)
	  tdata[i] = (double)0.0;
      }
      u_.data = (void *)tdata;
    }
    break;
  case CODA_STR:
    {
      char **tdata = new char*[count];
      for (int i = 0; i < count; i++) {
	tdata[i] = new char[::strlen (data[i]) + 1];
	::strcpy (tdata[i],data[i]);
      }
      u_.data = (void *)tdata;
    }
    break;
  default:
    break;
  }
}

void
daqNetData::assignData (const daqNetData& data)
{
  switch (data.type_) {
  case CODA_INT64:
    if (data.count_ > 1)
      assignData ((int64_t *)data.u_.data, data.count_);
    else if (data.count_ == 1)
      operator = (data.u_.lval);
    break;
  case CODA_INT32:
    if (data.count_ > 1)
      assignData ((int *)data.u_.data, data.count_);
    else if (data.count_ == 1)
      operator = (data.u_.ival);
    break;
  case CODA_FLT:
    if (data.count_ > 1)
      assignData ((float *)data.u_.data, data.count_);
    else if (data.count_ == 1)
      operator = (data.u_.fval);
    break;
  case CODA_DBL:
    if (data.count_ > 1)
      assignData ((double *)data.u_.data, data.count_);
    else if (data.count_ == 1)
      operator = (data.u_.dval);
    break;
  case CODA_STR:
    if (data.count_ > 1)
      assignData ((char **)data.u_.data, data.count_);
    else if (data.count_ == 1)
      operator = (data.u_.sval);
    break;
  case CODA_STRUCT:
    operator = (data.u_.arb);
  default:
    break;
  }
}

daqNetData::operator int (void)
{
  int val;
  int status;

  if (count_ == 1) {
    switch (type_){
    case CODA_INT64:
      val = u_.lval;
      break;
    case CODA_INT32:
      val = u_.ival;
      break;
    case CODA_FLT:
      val = (int)(u_.fval);
      break;
    case CODA_DBL:
      val = (int)(u_.dval);
      break;
    case CODA_STR:
      status = sscanf (u_.sval,"%d",&val);
      if (status != 1)
	val = 0xc0da;
      break;
    default:
      val = 0xc0da;
      break;
    }
  }
  else if (count_ > 1){
    switch (type_){
    case CODA_INT64:
      {
	int64_t* cdata = (int64_t *)u_.data;
	val = cdata[0];
      }
      break;
    case CODA_INT32:
      {
	int* cdata = (int *)u_.data;
	val = cdata[0];
      }
      break;
    case CODA_FLT:
      {
	float *cdata = (float *)u_.data;
	val = (int)cdata[0];
      }
      break;
    case CODA_DBL:
      {
	double *cdata = (double *)u_.data;
	val = (int)cdata[0];
      }
      break;
    case CODA_STR:
      {
	char **cdata = (char **)u_.data;
	status = sscanf (cdata[0],"%d",&val);
	if (status != 1)
	  val = 0xc0da;
      }
      break;
    default:
      val = 0xc0da;
      break;
    }
  }
  else
    val = 0xc0da;
  return val;
}

daqNetData::operator int64_t (void)
{
  int64_t val;
  int status;

  if (count_ == 1) {
    switch (type_){
    case CODA_INT64:
      val = (int64_t)u_.lval; /*sergey: error */
      break;
    case CODA_INT32:
      val = (int64_t)u_.ival;
      break;
    case CODA_FLT:
      val = (int64_t)(u_.fval);
      break;
    case CODA_DBL:
      val = (int64_t)(u_.dval);
      break;
    case CODA_STR:
      status = sscanf (u_.sval,"%ld",&val);
      if (status != 1)
	val = 0xc0da;
      break;
    default:
      val = 0xc0da;
      break;
    }
  }
  else if (count_ > 1){
    switch (type_){
    case CODA_INT64:
      {
	int64_t* cdata = (int64_t *)u_.data;
	val = (int64_t)cdata[0];
      }
      break;
    case CODA_INT32:
      {
	int* cdata = (int *)u_.data;
	val = (int64_t)cdata[0];
      }
      break;
    case CODA_FLT:
      {
	float *cdata = (float *)u_.data;
	val = (int64_t)cdata[0];
      }
      break;
    case CODA_DBL:
      {
	double *cdata = (double *)u_.data;
	val = (int64_t)cdata[0];
      }
      break;
    case CODA_STR:
      {
	char **cdata = (char **)u_.data;
	status = sscanf (cdata[0],"%ld",&val);
	if (status != 1)
	  val = 0xc0da;
      }
      break;
    default:
      val = 0xc0da;
      break;
    }
  }
  else
    val = 0xc0da;
  return val;
}

daqNetData::operator float (void)
{
  float val;
  int   status;

  if (count_ == 1) {
    switch (type_){
    case CODA_INT64:
      val = (float)(u_.lval);
      break;
    case CODA_INT32:
      val = (float)(u_.ival);
      break;
    case CODA_FLT:
      val = u_.fval;
      break;
    case CODA_DBL:
      val = (float)(u_.dval);
      break;
    case CODA_STR:
      status = sscanf (u_.sval,"%f",&val);
      if (status != 1)
	val = 0.0001;
      break;
    default:
      val = 0.0001;
      break;
    }
  }
  else if (count_ > 1) {
    switch (type_){
    case CODA_INT64:
      {
	int64_t* cdata = (int64_t *)u_.data;
	val = (float)cdata[0];
      }
      break;
    case CODA_INT32:
      {
	int* cdata = (int *)u_.data;
	val = (float)cdata[0];
      }
      break;
    case CODA_FLT:
      {
	float *cdata = (float *)u_.data;
	val = cdata[0];
      }
      break;
    case CODA_DBL:
      {
	double *cdata = (double *)u_.data;
	val = (float)cdata[0];
      }
      break;
    case CODA_STR:
      {
	char **cdata = (char **)u_.data;
	status = sscanf (cdata[0],"%f",&val);
	if (status != 1)
	  val = 0.001;
      }
      break;
    default:
      val = 0.001;
      break;
    }
  }    
  else
    val = 0.001;
  return val;
}

daqNetData::operator double (void)
{
  double val;
  double dval;
  int    status;

  if (count_ == 1) {
    switch (type_){
    case CODA_INT64:
      val = (double)(u_.lval);
      break;
    case CODA_INT32:
      val = (double)(u_.ival);
      break;
    case CODA_FLT:
      val = (double)(u_.fval);
      break;
    case CODA_DBL:
      val = u_.dval;
      break;
    case CODA_STR:
      status = sscanf (u_.sval,"%lf",&dval);
      if (status != 1)
	val = 0.0001;
      else
	val = (double)dval;
      break;
    default:
      val = 0.0001;
      break;
    }
  }
  else if (count_ > 1) {
    switch (type_){
    case CODA_INT64:
      {
	int64_t* cdata = (int64_t *)u_.data;
	val = (double)cdata[0];
      }
      break;
    case CODA_INT32:
      {
	int* cdata = (int *)u_.data;
	val = (double)cdata[0];
      }
      break;
    case CODA_FLT:
      {
	float *cdata = (float *)u_.data;
	val = (double)cdata[0];
      }
      break;
    case CODA_DBL:
      {
	double *cdata = (double *)u_.data;
	val = cdata[0];
      }
      break;
    case CODA_STR:
      {
	char **cdata = (char **)u_.data;
	status = sscanf (cdata[0],"%lf",&dval);
	if (status != 1)
	  val = 0.0001;
	else
	  val = dval;
      }
      break;
    default:
      val = 0.0001;
      break;
    }
  }    
  else
    val = 0.001;
  return val;
}

daqNetData::operator char* (void)
{
  if (count_ == 1) {
    static char buffer[CODA_CONV_LEN];
    switch (type_){
    case CODA_INT64:
      sprintf (buffer,"%lld",u_.lval);
      break;
    case CODA_INT32:
      sprintf (buffer,"%d",u_.ival);
      break;
    case CODA_FLT:
      sprintf (buffer,"%f",u_.fval);
      break;
    case CODA_DBL:
      sprintf (buffer,"%lf",u_.dval);
      break;
    case CODA_STR:
      sprintf (buffer,"%s",u_.sval);
      break;
    default:
      sprintf (buffer,"%&^**&^%");
      break;
    }
    char *tmp = buffer;
    return tmp;
  }
  else if (count_ > 1) {
    if (count_ > 100) {
      fprintf (stderr, "Exceeds maximum 100 number of elements\n");
      exit (1);
    }
    static char buffer[CODA_MAX_STRLEN];
    buffer[0] = '\0';
    int i = 0;
    switch (type_) {
    case CODA_INT64:
      {
	int64_t *cdata = (int64_t *)u_.data;
	for (i = 0; i < count_; i++)
	  sprintf (&(buffer[i*12]), "%-12lld", cdata[i]);
      }
      break;
    case CODA_INT32:
      {
	int *cdata = (int *)u_.data;
	for (i = 0; i < count_; i++)
	  sprintf (&(buffer[i*12]), "%-12d", cdata[i]);
      }
      break;
    case CODA_FLT:
      {
	float *cdata = (float *)u_.data;
	for (i = 0; i < count_; i++)
	  sprintf (&(buffer[i*12]), "%-20.4f", cdata[i]);
      }
      break;
    case CODA_DBL:
      {
	double *cdata = (double *)u_.data;
	for (i = 0; i < count_; i++)
	  sprintf (&(buffer[i*12]), "%-20.4lf", cdata[i]);
      }
      break;
    case CODA_STR:
      {
	char **cdata = (char **)u_.data;
	::strcpy (buffer, cdata[0]);
	for (i = 1; i < count_; i++) {
	  ::strcat (buffer, " ");
	  ::strcat (buffer, cdata[i]);
	}
      }
      break;
    default:
      buffer[0] = '\0';
      break;
    }
    char *tmp = buffer;
    return tmp;
  }
  else
    return 0;
}

daqNetData::operator daqArbStruct* (void)
{
  daqArbStruct* newstr = 0;

  if (type_ == CODA_STRUCT)
    newstr = u_.arb->dup ();

  return newstr;
}


int
daqNetData::getData (int data[], int& count)
{
  if (count == 0 || count_ == 0) return CODA_ERROR;
  int warning = CODA_SUCCESS;
  if (count != count_) warning = CODA_WARNING;

  if (count_ == 1)
  {
    switch (type_)
    {
    case CODA_INT64:
      data[0] = u_.lval;
      break;
    case CODA_INT32:
      data[0] = u_.ival;
      break;
    case CODA_FLT:
      data[0] = (int)u_.fval;
      break;
    case CODA_DBL:
      data[0] = (int)u_.dval;
      break;
    case CODA_STR:
      if (::sscanf (u_.sval, "%d", &(data[0])) < 1)
	data[0] = 0;
      break;
    default:
      break;
    }
  }
  else if (count_ > 1)
  {
    int realCount = (count_ < count) ? count_ : count;
    switch (type_)
    {
    case CODA_INT32:
      {
	int *cdata = (int *)u_.data;
	for (int i = 0; i < realCount; i++) 
	  data[i] = cdata[i];
      }
      break;
    case CODA_FLT:
      {
	float *cdata = (float *)u_.data;
	for (int i = 0; i < realCount; i++) 
	  data[i] = (int)cdata[i];
      }
      break;
    case CODA_DBL:
      {
	double *cdata = (double *)u_.data;
	for (int i = 0; i < realCount; i++) 
	  data[i] = (int)cdata[i];
      }
      break;
    case CODA_STR:
      {
	char **cdata = (char **)u_.data;
	for (int i = 0; i < realCount; i++) {
	  if (::sscanf (u_.sval, "%d", &data[i]) < 1)
	    data[i] = 0;
	}
      }
      break;
    default:
      break;
    }
  }
  count = count_;
  return warning;
}

int
daqNetData::getData (float data[], int& count)
{
  if (count == 0 || count_ == 0)
    return CODA_ERROR;
  int warning = CODA_SUCCESS;
  if (count != count_)
    warning = CODA_WARNING;

  if (count_ == 1) {
    switch (type_) {
    case CODA_INT32:
      data[0] = (float)u_.ival;
      break;
    case CODA_FLT:
      data[0] = u_.fval;
      break;
    case CODA_DBL:
      data[0] = (float)u_.dval;
      break;
    case CODA_STR:
      if (::sscanf (u_.sval, "%f", &(data[0])) < 1)
	data[0] = 0.0;
      break;
    default:
      break;
    }
  }
  else {
    int realCount = (count_ < count) ? count_ : count;
    switch (type_) {
    case CODA_INT32:
      {
	int *cdata = (int *)u_.data;
	for (int i = 0; i < realCount; i++) 
	  data[i] = (float)cdata[i];
      }
      break;
    case CODA_FLT:
      {
	float *cdata = (float *)u_.data;
	for (int i = 0; i < realCount; i++) 
	  data[i] = cdata[i];
      }
      break;
    case CODA_DBL:
      {
	double *cdata = (double *)u_.data;
	for (int i = 0; i < realCount; i++) 
	  data[i] = (float)cdata[i];
      }
      break;
    case CODA_STR:
      {
	char **cdata = (char **)u_.data;
	for (int i = 0; i < realCount; i++) {
	  if (::sscanf (u_.sval, "%f", &data[i]) < 1)
	    data[i] = 0.0;
	}
      }
      break;
    default:
      break;
    }
  }
  count = count_;
  return warning;
}

int
daqNetData::getData (double data[], int& count)
{
  if (count == 0 || count_ == 0)
    return CODA_ERROR;
  int warning = CODA_SUCCESS;
  if (count != count_)
    warning = CODA_WARNING;

  if (count_ == 1) {
    switch (type_) {
    case CODA_INT32:
      data[0] = (double)u_.ival;
      break;
    case CODA_FLT:
      data[0] = (double)u_.fval;
      break;
    case CODA_DBL:
      data[0] = u_.dval;
      break;
    case CODA_STR:
      if (::sscanf (u_.sval, "%lf", &(data[0])) < 1)
	data[0] = 0;
      break;
    default:
      break;
    }
  }
  else {
    int realCount = (count_ < count) ? count_ : count;
    switch (type_) {
    case CODA_INT32:
      {
	int *cdata = (int *)u_.data;
	for (int i = 0; i < realCount; i++) 
	  data[i] = (double)cdata[i];
      }
      break;
    case CODA_FLT:
      {
	float *cdata = (float *)u_.data;
	for (int i = 0; i < realCount; i++) 
	  data[i] = (double)cdata[i];
      }
      break;
    case CODA_DBL:
      {
	double *cdata = (double *)u_.data;
	for (int i = 0; i < realCount; i++) 
	  data[i] = cdata[i];
      }
      break;
    case CODA_STR:
      {
	char **cdata = (char **)u_.data;
	for (int i = 0; i < realCount; i++) {
	  if (::sscanf (u_.sval, "%lf", &data[i]) < 1)
	    data[i] = 0;
	}
      }
      break;
    default:
      break;
    }
  }
  count = count_;
  return warning;
}

int
daqNetData::getData (char* data[], int& count)
{
  if (count == 0 || count_ == 0) return CODA_ERROR;
  int warning = CODA_SUCCESS;
  if (count != count_) warning = CODA_WARNING;

#ifdef _TRACE_OBJECTS
  printf("11: count=%d count_=%d\n",count,count_);
#endif

  if (count_ == 1)
  {
    switch (type_)
    {
    case CODA_INT32:
      {
	    char temp[32];
	    ::sprintf (temp, "%d",u_.ival);
	    data[0] = new char[::strlen (temp) + 1];
	    ::strcpy (data[0], temp);
      }
      break;

    case CODA_FLT:
      {
	    char temp[CODA_FDCONV_LEN];
	    ::sprintf (temp, "%-30.4f",u_.fval);
	    data[0] = new char[::strlen (temp) + 1];
	    ::strcpy (data[0], temp);
      }
      break;

    case CODA_DBL:
      {
	    char temp[CODA_FDCONV_LEN];
	    ::sprintf (temp, "%-30.4lf",u_.dval);
	    data[0] = new char[::strlen (temp) + 1];
	    ::strcpy (data[0], temp);
      }
      break;

    case CODA_STR:
      data[0] = new char[::strlen (u_.sval) + 1];
      ::strcpy (data[0], u_.sval);
      break;

    default:
      break;
    }
  }
  else
  {
    int realCount = (count_ < count) ? count_ : count;
    switch (type_)
    {
    case CODA_INT32:
      {
	    int *cdata = (int *)u_.data;
	    char temp[32];
	    for (int i = 0; i < realCount; i++)
        {
	      ::sprintf (temp, "%d", cdata[i]);
	      data[i] = new char[::strlen (temp) + 1];
	      ::strcpy (data[i], temp);
	    }
      }
      break;

    case CODA_FLT:
      {
	    float *cdata = (float *)u_.data;
	    char temp[CODA_FDCONV_LEN];
	    for (int i = 0; i < realCount; i++)
        {
	      ::sprintf (temp, "%-32.4f", cdata[i]);
	      data[i] = new char[::strlen (temp) + 1];
	      ::strcpy (data[i], temp);
	    }
      }
      break;

    case CODA_DBL:
      {
	    double *cdata = (double *)u_.data;
	    char temp[CODA_FDCONV_LEN];
	    for (int i = 0; i < realCount; i++)
        {
	      ::sprintf (temp, "%-32.4lf", cdata[i]);
	      data[i] = new char[::strlen (temp) + 1];
	      ::strcpy (data[i], temp);
	    }
      }
      break;

    case CODA_STR:
      {
	    char **cdata = (char **)u_.data;
	    for (int i = 0; i < realCount; i++)
        {
	      data[i] = new char[::strlen (cdata[i]) + 1];
	      ::strcpy (data[i], cdata[i]);
	    }
      }
      break;

    default:
      break;
    }
  }

  count = count_;

  return(warning);
}


int
daqNetData::type (void) const
{
  return type_;
}

int64_t
daqNetData::namelength (void) const
{
  return nameLen_;
}

int64_t
daqNetData::attrlength (void) const
{
  return attrLen_;
}

int64_t
daqNetData::count (void) const
{
  return count_;
}

char*
daqNetData::name (void) const
{
  return c_.compname_;
}

char*
daqNetData::attribute (void) const
{
  return a_.attrname_;
}


/* return size which rounded to word boundary */
int64_t
daqNetData::size (void) const
{
  int64_t size = RC_DAQ_NETDATA_SIZE;

  if (count_ == 0) return size;

  size += nameLen_;
  size += attrLen_;
  
  if (count_ > 1)
  {
    switch (type_)
    {
      case CODA_INT32:
        size += (sizeof (int) * count_);
        break;

      case CODA_FLT:
      case CODA_DBL:
        size += (CODA_FDCONV_LEN * count_);
        break;

      case CODA_STR:
        {
	      char **cdata = (char **)u_.data;
	      for (int i = 0; i < count_; i++)
		  {
	        size += (::strlen (cdata[i]) + 1);
		  }
        }
        break;

      default:
        break;
    }
  }
  else
  {
    switch (type_)
    {
      case CODA_STR:
        size += (::strlen (u_.sval) + 1);
        break;

      case CODA_FLT:
      case CODA_DBL:
        size += CODA_FDCONV_LEN;
        break;

      case CODA_STRUCT:
        size += 2*sizeof (int64_t);  /* run time id + size */
        size += u_.arb->size ();
        break;

      default:
        break;
    }
  }

  // roundup size to align with word boundary
  // now put everything into the buffer
  int64_t bufsize = roundLen (size);

  return(bufsize);
}




/* Convert Float and Double into String With Length 32 and 4 after decimal
   Point, Data Value stays unchanged */
void
encodeNetData (daqNetData& data, char* &buffer, int64_t& bufsize)
{

#ifdef _TRACE_OBJECTS
  printf("RC_DAQ_NETDATA_SIZE sizes: %d %d %d %d\n",sizeof(dataType),sizeof(int),sizeof(int64_t),sizeof(double));
#endif

  /*32bit: RC_DAQ_NETDATA_SIZE sizes: 4 4 4 8
    64bit: RC_DAQ_NETDATA_SIZE sizes: 4 4 8 8
  */


  int64_t size = RC_DAQ_NETDATA_SIZE;
#ifdef _TRACE_OBJECTS
  printf("size1=%d\n",size);
#endif
  size += data.nameLen_;
#ifdef _TRACE_OBJECTS
  printf("size2=%d\n",size);
#endif
  size += data.attrLen_;
#ifdef _TRACE_OBJECTS
  printf("size3=%d\n",size);
#endif
  
  if (data.count_ > 1)
  {
    switch (data.type_)
    {
	  /*sergey*/
    case CODA_INT64:
      size += (sizeof (int64_t) * data.count_);
#ifdef _TRACE_OBJECTS
  printf("size44=%d\n",size);
#endif
      break;

    case CODA_INT32:
      size += (sizeof (int) * data.count_);
#ifdef _TRACE_OBJECTS
  printf("size4=%d\n",size);
#endif
      break;
    case CODA_FLT:
    case CODA_DBL:
      size += (CODA_FDCONV_LEN * data.count_);
#ifdef _TRACE_OBJECTS
  printf("size5=%d\n",size);
#endif
      break;
    case CODA_STR:
    {
	  char **cdata = (char **)data.u_.data;
	  for (int i = 0; i < data.count_; i++)
	  {
	    size += (::strlen (cdata[i]) + 1);
	  }
#ifdef _TRACE_OBJECTS
  printf("size6=%d\n",size);
#endif
    }
      break;
    default:
      break;
    }
  }
  else if (data.count_ == 1)
  {
    switch (data.type_)
    {
    case CODA_STR:
      size += (::strlen (data.u_.sval) + 1);
#ifdef _TRACE_OBJECTS
  printf("size7=%d\n",size);
#endif
      break;
    case CODA_FLT:
    case CODA_DBL:
      size += CODA_FDCONV_LEN;
#ifdef _TRACE_OBJECTS
  printf("size8=%d\n",size);
#endif
      break;
    case CODA_STRUCT:
      size += 2*sizeof(int64_t); /* run time id + size info */
#ifdef _TRACE_OBJECTS
  printf("size9=%d\n",size);
#endif
      size += data.u_.arb->size ();
#ifdef _TRACE_OBJECTS
  printf("size10=%d\n",size);
#endif
      break;
    default:
      break;
    }
  }

#ifdef _TRACE_OBJECTS
  printf("size=%d\n",size);
  printf("befor: bufsize=%lld\n",bufsize);
#endif

  /* roundup size to align with word boundary
     now put everything into the buffer */
  bufsize = roundLen(size);
  buffer = new char[bufsize];

#ifdef _TRACE_OBJECTS
  printf("after: bufsize=%lld (%lld)\n",bufsize,roundLen(size));
#endif


  /* convert all integer values into the network byte ordering */
  if (data.count_ == 1 && data.type_ == CODA_INT32) data.u_.ival = htonl (data.u_.ival);
  data.type_ = (dataType) htonl (data.type_);
  data.nameLen_ = htonl (data.nameLen_);
  data.attrLen_ = htonl (data.attrLen_);
  data.count_ = htonl (data.count_);

  /* reset words counter */  
  int i = 0;



  /* put class itself first */
  ::memcpy (buffer, &(data.type_), RC_DAQ_NETDATA_SIZE);
  i += RC_DAQ_NETDATA_SIZE;



  /* restore the original values */
  data.type_ = (dataType) ntohl (data.type_);
  data.nameLen_ = ntohl (data.nameLen_);
  data.attrLen_ = ntohl (data.attrLen_);
  data.count_ = ntohl (data.count_);

  if (data.count_ == 1 && data.type_ == CODA_INT32)
  {
    data.u_.ival = ntohl (data.u_.ival);
  }

  if (data.count_ == 0) /* no need to go more for an empty data */
  {
    return;
  }

  /* put component name */
  ::memcpy (&(buffer[i]), data.c_.compname_, data.nameLen_);
  i += data.nameLen_;

  /* put attribute name */
  ::memcpy (&(buffer[i]), data.a_.attrname_, data.attrLen_);
  i += data.attrLen_;

  /* put array of data */
  if (data.count_ > 1)
  {
    switch (data.type_)
    {
      case CODA_INT32:
        {
	      int *cdata = (int *)data.u_.data;
	      int tmp;
	      for (int j = 0; j < data.count_; j++)
          {
	        tmp = htonl (cdata[j]);
	        ::memcpy (&(buffer[i]), &tmp, sizeof (int));
	        i += sizeof (int);
	      }
        }
        break;

      case CODA_FLT:
        {
	      char temp[CODA_FDCONV_LEN];
	      float *cdata = (float *)data.u_.data;
	      for (int j = 0; j < data.count_; j++)
          {
	        sprintf (temp, "%-30.4f",cdata[j]);
	        ::memcpy (&(buffer[i]), temp, CODA_FDCONV_LEN);
	        i += CODA_FDCONV_LEN;
	      }
        }
        break;

      case CODA_DBL:
        {
	      char temp[CODA_FDCONV_LEN];
	      double *cdata = (double *)data.u_.data;
	      for (int j = 0; j < data.count_; j++)
          {
	        sprintf (temp, "%-30.4lf",cdata[j]);
	        ::memcpy (&(buffer[i]), temp, CODA_FDCONV_LEN);
	        i = i + CODA_FDCONV_LEN;
	      }
        }
        break;

      case CODA_STR:
        {
	      char **cdata = (char **)data.u_.data;
	      int len;
	      for (int j = 0; j < data.count_; j++)
          {
	        len = ::strlen (cdata[j]) + 1;
	        ::memcpy (&(buffer[i]), cdata[j], len);
	        i += len;
	      }
        }
        break;

      default:
        break;
    }
  }
  else if (data.count_ == 1)
  {
    switch (data.type_)
    {
      case CODA_FLT:
        {
	      char temp[CODA_FDCONV_LEN];
	      ::sprintf (temp, "%-30.4f", data.u_.fval);
	      ::memcpy (&(buffer[i]), temp, CODA_FDCONV_LEN);
	      i += CODA_FDCONV_LEN;
        }
        break;

      case CODA_DBL:
        {
	      char temp[CODA_FDCONV_LEN];
	      ::sprintf (temp, "%-30.4lf", data.u_.dval);
	      ::memcpy (&(buffer[i]), temp, CODA_FDCONV_LEN);
	      i += CODA_FDCONV_LEN;
        }
        break;

      case CODA_STR:
        {
	      int len = ::strlen (data.u_.sval) + 1;
	      ::memcpy (&(buffer[i]), data.u_.sval, len);
	      i += len;
        }
        break;

      case CODA_STRUCT:
        {
	      int64_t tmp[2];
	      tmp[0] = data.u_.arb->id ();
	      tmp[1] = data.u_.arb->size ();

	      tmp[0] = htonl (tmp[0]);
          tmp[1] = htonl (tmp[1]);

	      /* copy prefix information */
	      ::memcpy (&(buffer[i]), (void *)tmp, 2*sizeof (int64_t));
	      i = i + 2*sizeof (int64_t);

	      /* copy structure */
	      size_t bufsize = 0;
	      data.u_.arb->encode (&(buffer[i]), bufsize);
	      i += bufsize;
        }
        break;

      default:
        break;
    }
  }

  if (i != size) /* something is wrong */
  {
    fprintf (stderr, "Something is wrong in encoding for data %s %s\n",
	     data.c_.compname_, data.a_.attrname_);
  }
}


void
decodeNetData (daqNetData& data, char* buffer, int64_t bufsize)
{
#ifdef _TRACE_OBJECTS
  printf("daqNetData::decodeNetData: bufsize=%lld\n",bufsize);fflush(stdout);
#endif

  /* first clean up old information and remember old count value */
  int64_t oldcount = data.count_;
  char *comp = 0;
  char *attr = 0;

  if (data.count_ != 0)
  {
    if (data.count_ > 1)
	{
      data.freeBufferMemory ();
	}
    else if (data.count_ == 1)
    {
      if (data.type_ == CODA_STR) delete []data.u_.sval;
      else if (data.type_ == CODA_STRUCT) delete data.u_.arb;
    }


    /* remember address for attribute and component name which will be
       altered when one get the class content from the buffer */
    comp = data.c_.compname_;
    attr = data.a_.attrname_;



  }


  /* copy class itself */
  int i = 0;
  ::memcpy (&(data.type_), buffer, RC_DAQ_NETDATA_SIZE);
  i += RC_DAQ_NETDATA_SIZE;


  /* convert all integer value back into host byte ordering */
  data.type_ = (dataType) ntohl (data.type_);
  data.nameLen_ = ntohl (data.nameLen_);
  data.attrLen_ = ntohl (data.attrLen_);
  data.count_ = ntohl (data.count_);
  if (data.count_ == 1 && data.type_ == CODA_INT32)
    data.u_.ival = ntohl (data.u_.ival);

  if (oldcount == 0) // get component name and attribute name
  {
    assert (data.nameLen_ > 0);
    assert (data.attrLen_ > 0);

    data.c_.compname_ = new char[data.nameLen_];
    data.a_.attrname_ = new char[data.attrLen_];

	/*sergey
    printf("decodeNetData: data.attrLen_=%d\n",data.attrLen_);
	*/
    ::memcpy (data.c_.compname_, &(buffer[i]), data.nameLen_);
    i += data.nameLen_;
    ::memcpy (data.a_.attrname_, &(buffer[i]), data.attrLen_);
    i += data.attrLen_;
  }
  else
  {
    // do not get component name and attribute name
    // since this data already has component name and attribute
    // restore the name and attribute address
    data.c_.compname_ = comp;
    data.a_.attrname_ = attr;
    i += (data.nameLen_ + data.attrLen_);
  }

#ifdef _TRACE_OBJECTS
  printf("daqNetData::decodeNetData: data.count_=%d data.type_=%d\n",data.count_,data.type_);fflush(stdout);
#endif

  /* get values */
  if (data.count_ > 1)
  {
    switch (data.type_)
    {
      case CODA_INT32:
      {
	    int *tdata = new int [data.count_];
	    for (int j = 0; j < data.count_; j++)
        {
	      ::memcpy (&(tdata[j]), &(buffer[i]), sizeof (int));
	      tdata[j] = ntohl (tdata[j]);
	      i += sizeof (int);
	    }
	    data.u_.data = (void *)tdata;
      }
      break;

      case CODA_FLT:
      {
	    float *tdata = new float[data.count_];
	    char temp[CODA_FDCONV_LEN];
	    for (int j = 0; j < data.count_; j++)
        {
	      ::memcpy (temp, &(buffer[i]), CODA_FDCONV_LEN);
	      ::sscanf (temp, "%f", &(tdata[j]));
	      i += CODA_FDCONV_LEN;
	    }
	    data.u_.data = (void *)tdata;
      }
      break;

      case CODA_DBL:
      {
	    double *tdata = new double[data.count_];
	    char temp[CODA_FDCONV_LEN];
	    double tmp;
	    for (int j = 0; j < data.count_; j++)
        {
	      ::memcpy (temp, &(buffer[i]), CODA_FDCONV_LEN);
	      ::sscanf (temp, "%lf", &tmp);
	      tdata[j] = (double)tmp;
	      i += CODA_FDCONV_LEN;
	    }
	    data.u_.data = (void *)tdata;
      }
      break;

      case CODA_STR:
      {
	    char **tdata = new char*[data.count_];
	    int  bfsize = 0;
	    codaSplitBuffer (tdata, &(buffer[i]), data.count_, bfsize);
	    i += bfsize;
	    data.u_.data = (void *)tdata;
      }
      break;

      default:
        break;
    }
  }
  else if (data.count_ == 1)
  {
    switch (data.type_)
    {
      case CODA_FLT:
      {
	    char temp[CODA_FDCONV_LEN];
	    ::memcpy (temp, &(buffer[i]), CODA_FDCONV_LEN);
	    ::sscanf (temp, "%f", &(data.u_.fval));
	    i += CODA_FDCONV_LEN;
      }
      break;

      case CODA_DBL:
      {
	    char temp[CODA_FDCONV_LEN];
	    double ftmp;
	    ::memcpy (temp, &(buffer[i]), CODA_FDCONV_LEN);
	    ::sscanf (temp, "%lf", &ftmp);
	    data.u_.dval = (double)ftmp;
	    i += CODA_FDCONV_LEN;
      }
      break;

      case CODA_STR:
      {
	    int len = ::strlen (&(buffer[i])) + 1;
	    data.u_.sval = new char[len];
	    ::strcpy (data.u_.sval, &(buffer[i]));
	    i += len;
      }
      break;

      case CODA_STRUCT:
      {
#ifdef _TRACE_OBJECTS
        printf("daqNetData::decodeNetData: CODA_STRUCT\n");fflush(stdout);
#endif
		/* first get prefix information */
	    int64_t tmp[2];
	    ::memcpy ((void *)tmp, &(buffer[i]), 2*sizeof (int64_t));

	    tmp[0] = ntohl (tmp[0]);
	    tmp[1] = ntohl (tmp[1]);

	    i = i + 2*sizeof (int64_t);


		/* create comp boot struct */
	    daqArbStructFactory factory;
	    data.u_.arb = factory.create (tmp[0]);
	    assert (data.u_.arb);

#ifdef _TRACE_OBJECTS
        printf("daqNetData::decodeNetData: CODA_STRUCT 1\n");fflush(stdout);
#endif
	    /* get content out of buffer */
	    data.u_.arb->decode (&(buffer[i]), tmp[1]);
#ifdef _TRACE_OBJECTS
        printf("daqNetData::decodeNetData: CODA_STRUCT 2\n");fflush(stdout);
#endif
	    i += tmp[1];

      }
      break;

      default:
#ifdef _TRACE_OBJECTS
  printf("daqNetData::decodeNetData: default ..\n");fflush(stdout);
#endif
        break;
    }
  }

  if (i > bufsize)
  {
    fprintf (stderr, "Something is wrong in decoding for data %s %s\n", data.c_.compname_, data.a_.attrname_);
    fprintf (stderr, "decode byte size %d and expected byte size %d\n", i, bufsize);
  }
}
