/*
--------------------------------------------------------------------------------
-- Company:        IRFU / CEA Saclay
-- Engineers:      Irakli.MANDJAVIDZE@cea.fr (IM)
-- 
-- Project Name:   Clas12 Micromegas Vertex Tracker
-- Design Name:    Clas12 Dream testbench
--
-- Module Name:    Semaphores.c
-- Description:    Semaphore functions for exclusive acesses
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

#include <stdio.h>
#include <sys/time.h>
#include "TimerLib.h"
#include "Feu.h"
#include "ReturnCodes.h"
#include "Platform.h"
#include "bitmanip.h"
#include "Semaphores.h"

// Optional message container
char  sem_msg_container[128];
char *sem_msg = sem_msg_container;

/* common variables */
unsigned int sem_reg_adr; // address of the semaphore register
unsigned int sem_reg_val; // Its value to avoid unnecessary re-reads
unsigned int sem_state;   // State of the semaphores: Free, Busy or Granted

/* Semaphores */
#define SEM_OFF D_Feu_RegSC_Semaphores_Ofs
#define SEM_LEN D_Feu_RegSC_Semaphores_Len
#define Sem_Get( word ) GetBits(word, SEM_OFF, SEM_LEN )
#define Sem_Clr( word ) ClrBits(word, SEM_OFF, SEM_LEN )

#ifdef D_SC_OVER_SERLINK // Backend
#	define SEM_MSK_OFF (SEM_OFF+1)
#	define SEM_VAL     2
#endif

#ifdef D_SC_OVER_EMBPROC // Embedded proc for himself or on behalf of Ethernet
#	define SEM_MSK_OFF SEM_OFF
#	define SEM_VAL     1
#else
#       define SEM_MSK_OFF (SEM_OFF+1)
#       define SEM_VAL     2
#endif

#define Sem_GetMine(reg) GetBits(reg, SEM_MSK_OFF, 1 )
#define Sem_ClrMine(reg) ClrBits(reg, SEM_MSK_OFF, 1 )
#define Sem_SetMine(reg) SetBits(reg, SEM_MSK_OFF, 1 )

/*
 * sets the sem_reg_adr global variable
 * checks semaphore value
 * sets semaphore state
 */
int Sem_Init( unsigned int adr )
{
	/* Check address */
	if( adr == 0 )
	{
		sprintf( sem_msg, "Sem_Init: reg adr 0" );
		sem_state = SEM_STATE_ERROR;
		return D_RetCode_Err_Null_Pointer;
	}
	sem_reg_adr = adr;

	/* Check semaphore value */
	sem_reg_val = Peek(sem_reg_adr);
//printf( "%s: sem_reg_val 0x%08x\n\r", __FUNCTION__, sem_reg_val );
	if( Sem_GetMine(sem_reg_val) )
	{
		sprintf( sem_msg, "Sem_Init: SEM_STATE_ERROR" );
		sem_state = SEM_STATE_ERROR;
		return D_RetCode_Err_Sem;
	}

	/* Determine the semaphore state */
	if( Sem_Get(sem_reg_val) )
	{
		sem_state = SEM_STATE_BUSY;
		return D_RetCode_Wrn_SemBusy;
	}

	sem_state = SEM_STATE_FREE;
	return D_RetCode_Sucsess;
}

/*
 * Acqures access to the SC register
 * return values: -2 if called when already granted
 *                -1 if not granted but semaphore set
 *                 0 if access is not granted
 *                 1 if succeeded to acuire exclusive access
 */
int Sem_Acquire()
{
//printf( "%s: Started\n\s", __FUNCTION__ );
	/* Check if already granted */
	if( sem_state == SEM_STATE_GRANTED )
	{
		sprintf( sem_msg, "Sem_Acquire: already granted" );
		sem_state = SEM_STATE_ERROR;
		return D_RetCode_Err_Sem;
	}

	/* Get SC register value */
	sem_reg_val = Peek(sem_reg_adr);
//printf( "%s: sem_reg_val 0x%08x\n\r", __FUNCTION__, sem_reg_val );
	/* Check if semaphore is set */
	if( Sem_GetMine(sem_reg_val) )
	{
		sprintf( sem_msg, "Sem_Acquire: mine already set in 0x%08x", sem_reg_val );
		sem_state = SEM_STATE_ERROR;
		return D_RetCode_Err_Sem;
	}

	/* Check if someone already holds the access */
	if( Sem_Get(sem_reg_val) )
	{
		sem_state = SEM_STATE_BUSY;
		return D_RetCode_Wrn_SemBusy;
	}

	/* Try to get access */
	sem_reg_val = Sem_SetMine(sem_reg_val);
	Poke( sem_reg_adr, sem_reg_val );
	sem_reg_val = Peek( sem_reg_adr );
//printf( "%s: sem_reg_val 0x%08x\n\r", __FUNCTION__, sem_reg_val );
	if( Sem_Get(sem_reg_val) != SEM_VAL )
	{
		/* Contention detected: back-off to try later */
		sem_reg_val = Sem_Clr(sem_reg_val);
		Poke( sem_reg_adr, sem_reg_val );
		return D_RetCode_Wrn_SemBusy;
	}

	/* Acquired */
	sem_state = SEM_STATE_GRANTED;
//printf( "%s: Done\n\s", __FUNCTION__ );
	return D_RetCode_Sucsess;
}

/*
 * Frees access to the SC register
 * return values: -1 if called when not granted
 *                 0 otherwise
 */
int Sem_Free()
{
//printf( "%s: Started\n\s", __FUNCTION__ );
	/* Check if access was granted */
	if( sem_state != SEM_STATE_GRANTED )
	{
		sprintf( sem_msg, "Sem_Free: not granted" );
		sem_state = SEM_STATE_ERROR;
		return D_RetCode_Err_Sem;
	}

	/* Check SC register value */
	sem_reg_val = Peek(sem_reg_adr);
//printf( "%s: sem_reg_val 0x%08x\n\r", __FUNCTION__, sem_reg_val );
	if( Sem_GetMine(sem_reg_val) == 0 )
	{
		sprintf( sem_msg, "Sem_Free: mine is not set" );
		sem_state = SEM_STATE_ERROR;
		return D_RetCode_Err_Sem;
	}

	/* clear semaphore */
	sem_reg_val = Sem_Clr(sem_reg_val);
	Poke( sem_reg_adr, sem_reg_val );

	/* Freed */
	sem_state = SEM_STATE_FREE;
//printf( "%s: Done\n\s", __FUNCTION__ );
	return D_RetCode_Sucsess;
}

// Clear semaphore state
int Sem_ClearState()
{
	if( sem_state != SEM_STATE_GRANTED )
        {
                sprintf( sem_msg, "Sem_ClearState: not granted" );
                sem_state = SEM_STATE_ERROR;
                return D_RetCode_Err_Sem;
        }
        /* Freed */
        sem_state = SEM_STATE_FREE;
        return D_RetCode_Sucsess;
}

// Wait function
int Spi_Wait( int max_us, int max_iter )
{
	struct timeval t0;
	struct timeval t1;
	int i;

	gettimeofday(&t0,0);
	i=0;
	do
	{
		i++;
		if( i > max_iter )
		{
			printf("Spi_Wait: timeout %d %d %d %d\n\r", (int)t1.tv_sec, (int)t1.tv_usec, (int)t0.tv_sec, (int)t0.tv_usec);
			return D_RetCode_Err_Timeout;
		}
		gettimeofday(&t1, 0);
	} while( ((t1.tv_sec*1000000+t1.tv_usec)-(t0.tv_sec*1000000+t0.tv_usec)) < max_us );
	return (0);
}
