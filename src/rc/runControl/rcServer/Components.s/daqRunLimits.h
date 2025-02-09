#ifndef _CODA_DAQ_RUN_LIMITS_H
#define _CODA_DAQ_RUN_LIMITS_H

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
//      Data Acquisition DAQ Run Class. This Class handles state 
//      transition requests by forwarding them onto one DAQ
//      system object. It also contains some DAQ variables such as
//      event number and so on.
//
// Author:  
//      Randy MacLeod
//      Jefferson Lab, Data Acquisition Group
//
// Revision History:
//   $Log: daqRunLimits.h,v $
//   Revision 2.1  1999/07/29 14:21:08  rwm
//   List of arbitrary rc limits put in one place.
//
//
//

// maximum number of run types
#define MAX_NUM_RUNTYPES 150 /* sergey: was 50; actually max number of configurations ? */

// maximum number of components
#define MAX_NUM_COMPONENTS 400 /*sergey: was 100; it is used as process table limit as well !!!!!*/

// maximum number of dynamic varaibles
#define MAX_NUM_DYN_VARS (MAX_NUM_COMPONENTS*NUM_ATTR_DYN) /* sergey: was 200; every component has NUM_ATTR_DYN attributes */

#endif
