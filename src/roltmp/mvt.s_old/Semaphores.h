/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench
--
-- Module Name:    Semaphores.h
-- Description:    Semaphore gfunctions for exclusive acesses
--
-- Target Devices: Virtex-5 / ML507 development kit
-- Tool versions:  ISE XPS 10.1
-- 
-- Create Date:    0.0 2011/09/20 IM
-- Revision:        
--
-- Comments:
--
--------------------------------------------------------------------------------
*/

#ifndef _SEM_H
#define _SEM_H

// Optional message container
extern char *sem_msg;

/* SC library states */
#define SEM_STATE_FREE      0
#define SEM_STATE_BUSY      1
#define SEM_STATE_GRANTED   2
#define SEM_STATE_ERROR     3

/*
 * Prototypes for functions in SC.c
 */
int Sem_Init( unsigned int adr );
int Sem_Acquire();
int Sem_Free();
int Sem_ClearState();

// Wait function
int Spi_Wait( int max_us, int max_iter );

#endif // #ifndef _SEM_H
