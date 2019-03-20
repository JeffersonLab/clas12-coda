/*-----------------------------------------------------------------------------
 * Copyright (c) 1991,1992 Southeastern Universities Research Association,
 *                         Continuous Electron Beam Accelerator Facility
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 * CEBAF Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606
 * Email: coda@cebaf.gov  Tel: (804) 249-7101  Fax: (804) 249-7363
 *-----------------------------------------------------------------------------
 * 
 * Description:
 *	CODA Editor header file
 *	
 * Author:  Jie Chen, CEBAF Data Acquisition Group
 *
 * Revision History:
 *   $Log: Editor.h,v $
 *   Revision 1.6  1998/08/17 22:43:25  rwm
 *   Allow 32 ROCs + TS + EB + ER + FILE + 4 extra components.
 *
 *   Revision 1.5  1997/09/08 17:27:15  heyes
 *   fixed memory free bugs (I think!)
 *
 *   Revision 1.4  1997/09/08 15:19:09  heyes
 *   fix dd icon etc
 *
 *   Revision 1.3  1997/07/08 15:30:28  heyes
 *   add stuff
 *
 *   Revision 1.2  1997/06/20 16:59:45  heyes
 *   clean up GUI!
 *
 *   Revision 1.1.1.2  1996/11/05 17:45:03  chen
 *   coda source
 *
 *	  
 */
#ifndef _CODA_EDITOR_H
#define _CODA_EDITOR_H

/* sergey: to add new conponent type, add new CODA_xxx below
and modify following files (search for CODA_EB, ADD_EB_ACTION, eb_btn
Editor_converter.c +
Editor_graph.h +
Editor_graph.c +
Editor_xmisc.c +

Editor_drawing.c
Editor_icon_box.c
Editor_syntax_checker.c

Editor_pixmap.c - assign pixmap file
*/

#define CODA_TRIG      0
#define CODA_ROC       1
#define CODA_EB        2
#define CODA_ET        3
#define CODA_ETT       4
#define CODA_L3        5
#define CODA_ER        6
#define CODA_LOG       7
#define CODA_SC        8
#define CODA_UC        9
#define CODA_RCS       10
#define CODA_FILE      11
#define CODA_CODAFILE  12
#define CODA_DEBUG     13
#define CODA_UNKNOWN   14
#define CODA_MON       15
#define CODA_NONE      16

#define CODA_EBANA     17 /*was 4 - temporary to resolve references */

#define MAX_NUM_PORTS  5
#define MAX_NUM_IOS    130 /* was 40 */
#define MAX_NUM_COMPS  500

typedef struct _comp
{
  int      type;
  char     *comp_name;     /* component unique name           */
  char     *node_name;     /* default address                 */
  int      id_num;         /* unique id number within a class */
  int      status;         /* component status from RC        */
  char     *boot_string;   /* boot string, how to start       */
  char     *code[3];       /* readout lists code              */
} daqComp;

typedef struct _whole_comp
{
  daqComp daq;
  char    **port_name;
  int     num_ports;
} rcNetComp;   /* complete info of a component */

typedef struct _line_
{
  char    *copy;
  int     head;
  int     tail;
  int     length;
  char    *cursor;
  char*   (*next_word)();
  char*   (*remnant)();
  void    (*open)();
  void    (*close)();
  int     (*eol)();
  int     (*isempty)();
} ioLine;

typedef struct _s_script_
{
  char*              state;
  char*              script;
  struct _s_script_* next;
} codaScript;


typedef struct _inputs_comp
{
  char *comp_name;   /* FILE: filename, DEBUG: arbitrary */
  char *port_name;   /* EMPTY for FILE and DEBUG class   */
} IoId;

typedef struct _config__info
{
  char*       comp_name;
  IoId**      inputs;
  int         num_inputs;
  IoId**      outputs;
  int         num_outputs;
  char*       code[3];
  codaScript* scripts;
  /* private data */
  int  row, col;
} ConfigInfo;

/*
extern ioLine *createIoLine();
extern rcNetComp* newRcNetComp ();
extern void       freeRcNetComp ();
extern void       setRcNetComp ();
extern IoId*      newIoId ();
extern void       freeIoId ();
extern ConfigInfo* newConfigInfo ();
extern int         matchConfigInfo ();
extern void        freeConfigInfo ();
extern void        setConfigInfoName ();
extern void        setConfigInfoCode ();
extern void        setConfigInfoInputs ();
extern void        setConfigInfoOutputs ();
extern void        setConfigInfoPosition ();
*/

#endif




