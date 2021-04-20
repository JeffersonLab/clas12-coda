//-----------------------------------------------------------------------------
// File          : ControlCmdMem.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 01/11/2012
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Interface Server
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 01/11/2012: created
//-----------------------------------------------------------------------------
#ifndef __CONTROL_CMD_MEM_H__
#define __CONTROL_CMD_MEM_H__

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>   
#include <string.h>   
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

// Sizes
#define CONTROL_CMD_STR_SIZE  1024
#define CONTROL_CMD_XML_SIZE  1048576
#define CONTROL_CMD_NAME_SIZE 200

// Command Constants
#define CONTROL_CMD_TYPE_SEND_XML      1 // One arg,  XML string, No Result
#define CONTROL_CMD_TYPE_SET_CONFIG    2 // Two args, Config Variable and String Value, No Result
#define CONTROL_CMD_TYPE_GET_CONFIG    3 // One Arg, Config Variable, Result is Value
#define CONTROL_CMD_TYPE_GET_STATUS    4 // One Arg, Status Variable, Resilt is Value
#define CONTROL_CMD_TYPE_EXEC_COMMAND  5 // Two Args, Command and arg, No Result
#define CONTROL_CMD_TYPE_SET_REGISTER  6 // Three Args, Device Path, register name and value, No Result
#define CONTROL_CMD_TYPE_GET_REGISTER  7 // Two Args, Device Path, register name, Result is value

typedef struct {

   // Commands
   char         cmdRdyCount;
   char         cmdAckCount;
   char         cmdType;
   char         cmdArgA[CONTROL_CMD_XML_SIZE];
   char         cmdArgB[CONTROL_CMD_STR_SIZE];
   char         cmdResult[CONTROL_CMD_STR_SIZE];

   // Error, Config and Status 
   char         errorBuffer[CONTROL_CMD_STR_SIZE];
   char         xmlStatusBuffer[CONTROL_CMD_XML_SIZE];
   char         xmlConfigBuffer[CONTROL_CMD_XML_SIZE];

   // Shared name
   char         sharedName[CONTROL_CMD_NAME_SIZE];

} ControlCmdMemory;

// Open and map shared memory
int
controlCmdOpenAndMap ( ControlCmdMemory **ptr, const char *system, unsigned int id, int uid )
{
   int           smemFd;
   char          shmName[200];
   int           lid;

   // ID to use?
   if ( uid == -1 ) lid = getuid();
   else lid = uid;

   // Generate shared memory
   sprintf(shmName,"control_cmd.%i.%s.%i",lid,system,id);

   // Attempt to open existing shared memory
   if ( (smemFd = shm_open(shmName, O_RDWR, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) ) < 0 ) {

      // Otherwise open and create shared memory
      if ( (smemFd = shm_open(shmName, (O_CREAT | O_RDWR), (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) ) < 0 ) return(-1);

      // Force permissions regardless of umask
      fchmod(smemFd, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));
    
      // Set the size of the shared memory segment
      ftruncate(smemFd, sizeof(ControlCmdMemory));
   }

   // Map the shared memory
   if((*ptr = (ControlCmdMemory *)mmap(0, sizeof(ControlCmdMemory),
              (PROT_READ | PROT_WRITE), MAP_SHARED, smemFd, 0)) == MAP_FAILED) return(-2);

   // Store name
   strcpy((*ptr)->sharedName,shmName);

   return(smemFd);
}

// Close shared memory
void
controlCmdClose ( ControlCmdMemory *ptr )
{
   char shmName[200];

   // Get shared name
   strcpy(shmName,ptr->sharedName);

   // Unlink
   shm_unlink(shmName);
}

// Init data structure, called by ControlServer
void
controlCmdInit ( ControlCmdMemory *ptr )
{
   memset(ptr->cmdArgA,   0, CONTROL_CMD_STR_SIZE);
   memset(ptr->cmdArgB,   0, CONTROL_CMD_STR_SIZE);
   memset(ptr->cmdResult, 0, CONTROL_CMD_STR_SIZE);
   memset(ptr->errorBuffer, 0, CONTROL_CMD_STR_SIZE);
   memset(ptr->xmlStatusBuffer, 0, CONTROL_CMD_XML_SIZE);
   memset(ptr->xmlConfigBuffer, 0, CONTROL_CMD_XML_SIZE);

   ptr->cmdType     = 0;
   ptr->cmdRdyCount = 0;
   ptr->cmdAckCount = 0;
}

// Send command
void
controlCmdSetCommand ( ControlCmdMemory *ptr, char cmdType, const char *argA, const char *argB )
{
   if ( argA != NULL ) strcpy(ptr->cmdArgA,argA);
   if ( argB != NULL ) strcpy(ptr->cmdArgB,argB);
   strcpy(ptr->errorBuffer,"");

   ptr->cmdType = cmdType;
 
   ptr->cmdRdyCount++;
}

// Check for pending command
int
controlCmdGetCommand ( ControlCmdMemory *ptr, char *cmdType, char **argA, char **argB )
{
   if ( ptr->cmdRdyCount == ptr->cmdAckCount ) return(0);
   else {
      *cmdType = ptr->cmdType;
      *argA    = ptr->cmdArgA;
      *argB    = ptr->cmdArgB;
      return(1);
   }
}

// Command Set Result
void
controlCmdSetResult ( ControlCmdMemory *ptr, const char *result )
{
   if ( result != NULL ) strcpy(ptr->cmdResult,result);
}

// Command ack
void
controlCmdAckCommand ( ControlCmdMemory *ptr )
{
   ptr->cmdAckCount = ptr->cmdRdyCount;
}

// Wait for command completion
int
controlCmdGetResult ( ControlCmdMemory *ptr, char *result )
{
   if ( ptr->cmdRdyCount != ptr->cmdAckCount ) return(0);
   else { 
      if ( result != NULL ) strcpy(result,ptr->cmdResult);
      return(1);
   }
}

// Wait for command completion with timeout in milliseconds
int
controlCmdGetResultTimeout ( ControlCmdMemory *ptr, char *result, int timeout )
{
   struct timeval now;
   struct timeval sum;
   struct timeval add;

   gettimeofday(&now,NULL);
   add.tv_sec =  (timeout / 1000);
   add.tv_usec = (timeout % 1000);
   timeradd(&now,&add,&sum);

   while ( ! controlCmdGetResult(ptr,result) ) {
      gettimeofday(&now,NULL);
      if ( timercmp(&now,&sum,>) ) return(0);
      usleep(1000); // Wait 1 millisecond
   }
   return(1);
}

// Set Config
void
controlCmdSetConfig ( ControlCmdMemory *ptr, const char *config )
{
   strcpy(ptr->xmlConfigBuffer,config);
}

// Get Config
const char *
controlCmdGetConfig ( ControlCmdMemory *ptr )
{
   return(ptr->xmlConfigBuffer);
}

// Set Status
void
controlCmdSetStatus ( ControlCmdMemory *ptr, const char *status )
{
   strcpy(ptr->xmlStatusBuffer,status);
}

// Get Status
const char *
controlCmdGetStatus ( ControlCmdMemory *ptr )
{
   return(ptr->xmlStatusBuffer);
}

// Set Error 
void
controlCmdSetError ( ControlCmdMemory *ptr, const char *error )
{
   strcpy(ptr->errorBuffer,error);
}

// Get Error
const char *
controlCmdGetError ( ControlCmdMemory *ptr )
{
   return(ptr->errorBuffer);
}

#endif

