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
//	CODA pushbutton Command class header file
//	
// Author:  Jie Chen
//       CEBAF Data Acquisition Group
//
// Revision History:
//   $Log: codaPbtnComd.h,v $
//   Revision 1.3  1998/04/08 17:29:53  heyes
//   get in there
//
//   Revision 1.2  1997/06/06 19:00:46  heyes
//   new RC
//
//   Revision 1.1.1.1  1996/10/10 19:25:00  chen
//   coda motif C++ library
//
//
#ifndef _CODA_PBTN_COMD
#define _CODA_PBTN_COMD

#include <codaComd.h>

class XcodaPbtnInterface;

class codaPbtnComd:public codaComd{

 public:
  codaPbtnComd (char *name, int active);
  codaPbtnComd (char *name, int active, char *acc, char *acc_text);
  virtual ~codaPbtnComd (void);
  virtual void createXInterface(Widget);
  virtual const char* className (void) const {return "codaPbtnComd";}
  void  defaultButton();
 protected:
  XcodaPbtnInterface *pb;

 private:
  char *_acc;
  char *_acc_text;

};
#endif
