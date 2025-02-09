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
 *	Pixmap header file
 *	
 * Author:  Jie Chen
 * CEBAF Data Acquisition Group
 *
 * Revision History:
 *   $Log: Editor_pixmap.h,v $
 *   Revision 1.3  1997/09/08 15:19:37  heyes
 *   fix dd icon etc
 *
 *   Revision 1.2  1997/06/20 17:00:21  heyes
 *   clean up GUI!
 *
 *   Revision 1.1.1.2  1996/11/05 17:45:16  chen
 *   coda source
 *
 *	  
 */
#ifndef _EDITOR_PIXMAP_H
#define _EDITOR_PIXMAP_H

typedef struct _pixmap_s
{
  Pixmap roc;
  Pixmap eb;
  Pixmap et; /*sergey*/
  Pixmap ett; /*sergey*/
  Pixmap sro; /*sergey*/
  Pixmap spr; /*sergey*/
  Pixmap l3; /*sergey*/
  Pixmap ana; /*sergey: remove ? */
  Pixmap ebana; /*sergey: remove ? */
  Pixmap trig;
  Pixmap hl_roc;
  Pixmap hl_eb;
  Pixmap hl_et; /*sergey*/
  Pixmap hl_ett; /*sergey*/
  Pixmap hl_sro; /*sergey*/
  Pixmap hl_spr; /*sergey*/
  Pixmap hl_l3; /*sergey*/
  Pixmap hl_ana;
  Pixmap hl_ebana;
  Pixmap hl_trig;
  Pixmap eth_input;
  Pixmap eth_input_sel;
  Pixmap move_node;
  Pixmap move_node_sel;
  Pixmap roc_btn;
  Pixmap eb_btn;
  Pixmap et_btn; /*sergey*/
  Pixmap ett_btn; /*sergey*/
  Pixmap sro_btn; /*sergey*/
  Pixmap spr_btn; /*sergey*/
  Pixmap l3_btn; /*sergey*/
  Pixmap ana_btn;
  Pixmap ebana_btn;
  Pixmap er_btn;
  Pixmap dd_btn;
  Pixmap cfi_btn;
  Pixmap fi_btn;
  Pixmap dbg_btn;
  Pixmap trig_btn;
  Pixmap more_type;
  Pixmap icon;
  Pixmap zoom;

  Pixmap file;
  Pixmap codaFile;
  Pixmap debug;
  Pixmap dd;
  Pixmap trash;

} XcodaEditorPixmaps;

#ifdef __cplusplus
extern "C" {
#endif

extern XcodaEditorPixmaps btn_pixmaps;  /* this is a global object */
extern void    XcodaEditorCreatePixmaps (Widget parent, Pixel bg, Pixel fg);

#ifdef __cplusplus
}
#endif


#endif
