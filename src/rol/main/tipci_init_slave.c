/*
 * This file is part of the Xilinx DMA IP Core driver tools for Linux
 *
 * Copyright (c) 2016-present,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is licensed under BSD-style license (found in the
 * LICENSE file in the root directory of this source tree)
 */


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <byteswap.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>

#include <sys/types.h>
#include <sys/mman.h>

/* ltoh: little to host */
/* htol: little to host */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ltohl(x)       (x)
#define ltohs(x)       (x)
#define htoll(x)       (x)
#define htols(x)       (x)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define ltohl(x)     __bswap_32(x)
#define ltohs(x)     __bswap_16(x)
#define htoll(x)     __bswap_32(x)
#define htols(x)     __bswap_16(x)
#endif

#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)

#define MAP_SIZE (8*1024UL)
#define MAP_MASK (MAP_SIZE - 1)
void *map_base ;

void FlashCommand(uint32_t CmdByte)
{ void *virt_addr;
  uint32_t ibits, DataIn;
  virt_addr = map_base;
  // CmdByte(9:8): control the last bit shift DataIn(9:8),
  // CmdByte(7:0): MSB shifted first

  for (ibits = 7; ibits >0 ; ibits --) {
    DataIn = 0x1e0 + 15*((CmdByte >> ibits) & 0x01);  // bit(3:0) is either 0000 or 1111
    *((uint32_t *) (virt_addr + 0xE4 )) = DataIn;
    // Enable the dummy write to reg 0x00 (next two lines) for firmware delay
    *((uint32_t *) (virt_addr + 0x00 )) = 0x56;  
    *((uint32_t *) (virt_addr + 0x00 )) = 0x9a; }
  DataIn = 0xe0 + (CmdByte & 0x300) + 15*(CmdByte & 0x01);
  //  printf("Data shifted %08x \n", DataIn);
  *((uint32_t *) (virt_addr + 0xE4 )) = DataIn;
  // Enable the dummy write to reg 0x00 (next two lines) for firmware delay
  *((uint32_t *) (virt_addr + 0x00 )) = 0x56;
  *((uint32_t *) (virt_addr + 0x00 )) = 0x9a;
  return; }

void FlashWEnable(void)
{ void *virt_addr;
  virt_addr = map_base;
  //to add more cose here
  // write the flash WRITE enable 0x06
  FlashCommand(0x206); 
  printf(" write to reg 0xE4: %08x WRITE_ENABLE\n", 0x06);
  return; }

uint32_t FlashStatusReg(void)
{ void *virt_addr;
  uint32_t read_result, FlashMemID, irepeat;
  virt_addr = map_base;
  // Check the status register
  // write the flash read code 0x05
  FlashCommand(0x305);
  FlashMemID = 0;
  for (irepeat =0; irepeat<6; irepeat ++) {
    usleep(10);
    read_result = *((uint32_t *) (virt_addr+0xE4) );  // 4-bit 
    FlashMemID = FlashMemID + (((read_result & 0x20000) >> 16) << (6-irepeat));
    *((uint32_t *) (virt_addr + 0xE4 )) = 0x1F0; }
  usleep(10);
  read_result = *((uint32_t *) (virt_addr+0xE4) );  // 4-bit 
  FlashMemID = FlashMemID + ((read_result & 0x20000) >> 16);
  *((uint32_t *) (virt_addr + 0xE4 )) = 0x0F0; 
  usleep(10);
  read_result = *((uint32_t *) (virt_addr+0xE4) );  // 4-bit 
  FlashMemID = FlashMemID + ((read_result & 0x20000) >> 17);
  printf(" Flash Status Register: %08x \n", FlashMemID);
  return FlashMemID; }

uint32_t FlashFlagReg(void)
{ void *virt_addr;
  uint32_t read_result, FlashMemID, irepeat;
  virt_addr = map_base;
  FlashCommand(0x370);
  FlashMemID = 0;
  for (irepeat =0; irepeat<6; irepeat ++) {
    usleep(10);
    read_result = *((uint32_t *) (virt_addr+0xE4) );  // 4-bit 
    FlashMemID = FlashMemID + (((read_result & 0x20000) >> 16) << (6-irepeat));
    *((uint32_t *) (virt_addr + 0xE4 )) = 0x1F0;}
  usleep(10);
  read_result = *((uint32_t *) (virt_addr+0xE4) );  // 4-bit 
  FlashMemID = FlashMemID + ((read_result & 0x20000) >> 16);
  *((uint32_t *) (virt_addr + 0xE4 )) = 0x0F0; 
  usleep(10);
  read_result = *((uint32_t *) (virt_addr+0xE4) );  // 4-bit 
  FlashMemID = FlashMemID + ((read_result & 0x20000) >> 17);
  return FlashMemID; }
  
uint32_t FlashID(void)
{ void *virt_addr;
  uint32_t read_result, FlashMemID, irepeat;
  virt_addr = map_base;
  // write the flash read code 0xAF
  FlashCommand(0x3AF);
  FlashMemID = 0;
  for (irepeat =0; irepeat<30; irepeat ++) {
    usleep(10);
    read_result = *((uint32_t *) (virt_addr+0xE4) );  // 4-bit 
    FlashMemID = FlashMemID + (((read_result & 0x20000) >> 16) << (30-irepeat));
    *((uint32_t *) (virt_addr + 0xE4 )) = 0x1F0; }
  usleep(10);
  read_result = *((uint32_t *) (virt_addr+0xE4) );  // 4-bit 
  FlashMemID = FlashMemID + ((read_result & 0x20000) >> 16);
  *((uint32_t *) (virt_addr + 0xE4 )) = 0x0F0; 
  usleep(10);
  read_result = *((uint32_t *) (virt_addr+0xE4) );  // 4-bit 
  FlashMemID = FlashMemID + ((read_result & 0x20000) >> 17);
  return FlashMemID; }

int main(int argc, char **argv)
{
  int fd, BatchN, SerialN, LockOTP, PageEnd;
  //	void *map_base,
        void *virt_addr, *virt_addrN;
	uint32_t read_result, read_result2, read_resultN, writeval, SingleData;
	uint32_t FlashMemID, PageAdd;
	uint32_t FlashData[256], WriteData, ExtraData;
	off_t target;
	/* access width */
	int access_width = 'w', LineRead = 0, Counter = 0;
	char *device, bufRead[1024], SkipChar;
        int option, ireg, irepeat, SingleAdd, ShiftBit, SyncDelay, NRepeat;
	int BlockLevel, BufferLevel, iPage, iByte, iBit, iPageAdd;
	FILE *mcsFile;
	uint32_t SectorAdd, ByteAdd, mcsType;
	int iWord, SectorEdge, EndOfFile, bytePosition, Area128, BulkErease;

	/* not enough arguments given? */


	/*sergey
	if (argc < 2) {
		fprintf(stderr,
			"\nUsage:\t%s <device> <address> [[type] data]\n"
			"\tdevice  : character device to access\n"
			"\taddress : memory address to access\n"
			"\ttype    : access operation type : [b]yte, [h]alfword, [w]ord\n"
			"\tdata    : data to be written for a write\n\n",
			argv[0]);
		exit(1);
	}
	*/
        argc = 2;
        argv[1] = strdup("/dev/xdma0_user");



	printf("argc = %d\n", argc);

	device = strdup(argv[1]);
	printf("device: %s\n", device);

        device = strdup(argv[1]);


	// move the device open and mmap here (up from the reg_rw.c)
	if ((fd = open(argv[1], O_RDWR | O_SYNC)) == -1)
		FATAL;
	printf("character device %s opened.\n", argv[1]);
	fflush(stdout);

	/* map one page */
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map_base == (void *)-1)
		FATAL;
	printf("Memory mapped at address %p.\n", map_base);
	fflush(stdout);

        if (argc == 2) {
        ChooseAgain:
          printf("\n enter 1  for TImasterSet \n");
          printf(" enter 15 for TIfanout (slave/Master) \n");
          printf(" enter 2  for TIsetup  (slave, Fiber#1) \n");
          printf(" enter 21 for Fiber#1 Length Meas. \n");
          printf(" enter 25 for TIsetup5 (slave, Fiber#5) \n");
          printf(" enter 26 for Fiber#5 Length Meas. \n");
          printf(" enter 3  for TrgStart \n");
          printf(" enter 4  for I2CopticTrx \n");
	  printf(" enter 45 for I2Coptic register dump \n");
          printf(" enter 5  for tidsRead, one word \n");
          printf(" enter 51 for Flash Memory Reset \n");
          printf(" enter 52 for Flash Memory ID read back \n");
          printf(" enter 53 for Flash Memory Erase \n");
          printf(" enter 54 for Flash Memory Program \n");
          printf(" enter 55 for Flash Memory Flag_Status_register Read\n");
          printf(" enter 551 for Flash Memory Status_register Write\n");
          printf(" enter 552 for Flash Memory Status_register Read \n");
          printf(" enter 56 for Flash Memory UserCode (OTP) Program \n");
          printf(" enter 57 for Flash Memory UserCode (OTP) read \n");
          printf(" enter 58 for Flash Memory Page Program \n");
          printf(" enter 59 for Flash Memory Pages read \n");
	  printf(" enter 6  for TI register dump \n");
          printf(" enter 7  for single Register (32-bit) Read\n");
          printf(" enter 75 for repeated same Register (32-bit) Read\n");
          printf(" enter 8  for single Register (32-bit) Write\n");
          printf(" enter 85 for repeated same Register (32-bit) Write\n");
          printf(" enter 9  to quit \n");
          printf(" enter 99 to reload the FPGA from flash and quit \n");
          printf(" \n enter : ");
          fflush(stdout);



	  /*sergey
          scanf("%d", &option);
	  */
          option=2;


          if (option == 9) {
	    if (munmap(map_base, MAP_SIZE) == -1)  FATAL;
	    close(fd);
	    printf(" exit \n"); exit(1);}
          if (option == 1) goto TImasterSet;
          if (option == 15) goto TIfanout;
          if (option == 2) goto TIsetup;
          if (option == 21) goto Fiber1Meas;
	  if (option == 25) goto TIsetup5;
          if (option == 26) goto Fiber5Meas;
          if (option == 3) goto TrgStart;
          if (option == 4) goto I2CopticTrx;
	  if (option == 45) goto I2CopticDump;
          if (option == 5) goto tidsRead;
          if (option == 51) goto FlashReset;
	  if (option == 52) goto FlashID;
	  if (option == 53) goto FlashErase;
	  if (option == 54) goto FlashProgram;
	  if (option == 55) goto FlashFlag;
	  if (option == 551) goto FlashStatusWrite;
	  if (option == 552) goto FlashStatusRead;
	  if (option == 56) goto FlashUserCodeProgram;
	  if (option == 57) goto FlashUserCodeRead;
	  if (option == 58) goto FlashLastPageProgram;
	  if (option == 59) goto FlashLastPageRead;
	  if (option == 6) goto TIstatus;
	  if (option == 7) goto SingleRegRead;
	  if (option == 75) goto RepeatRegRead;
	  if (option == 8) goto SingleRegWrite;
	  if (option == 85) goto RepeatRegWrite;
	  if (option == 99) goto ReloadQuit;
          goto ChooseAgain; 

 	ReloadQuit:
          printf(" FPGA reload from Flash Memory......\n");
          fflush(stdout);
 	  virt_addr = map_base;  // Register base address
	  // Check the 0xB8 register and make sure it is 0xE.......
	  read_result = *((uint32_t *) (virt_addr + 0xb8));
	  printf("Board Status 0xB8: %x\n", read_result);
	  printf("!!! quit the program \n");
	  printf("!!! copy the config file back to /sys/bus.../config \n");
	  printf("!!! reload the driver ../tests/load_driver \n");
	  printf("!!! restart the TIcontrol \n");
	  // TIpcieUS FPGA reload from SPI flash
	  *((uint32_t *) (virt_addr + 0x100) ) = 0x7e;
	  if (munmap(map_base, MAP_SIZE) == -1)  FATAL;
	  close(fd);
	  printf(" exit \n"); exit(1);

	TImasterSet:
          printf(" TImasterSet......\n");
          fflush(stdout);
 	  virt_addr = map_base;  // Register base address
	  // Check the 0xB8 register and make sure it is 0xE.......
	  read_result = *((uint32_t *) (virt_addr + 0xb8));
	  printf("Board Status 0xB8: %x\n", read_result);
          if (((read_result >>24)&0xff) < 0xe0) {
	    // EMFPGA reload
	    printf("!!! quit the program \n");
	    printf("!!! copy the config file back to /sys/bus.../config \n");
	    printf("!!! reload the driver ../tests/load_driver \n");
	    printf("!!! restart the TIcontrol \n");
            // TIpcieUS FPGA reload from SPI flash
            *((uint32_t *) (virt_addr + 0x100) ) = 0x7e;
	    if (munmap(map_base, MAP_SIZE) == -1)  FATAL;
	    close(fd);
	    printf(" exit \n"); exit(1);  }

 	  // TI_clock setup
	  // Disable Sync first, to avoid unwanted Sync_Resets
          *((uint32_t *) (virt_addr + 0x24) ) = 0x00;
	  usleep(1000);
          read_result = *((uint32_t *) (virt_addr+0x2C) );  //Clock source?
          printf("TI clock source 0x2C: %08x, set to '00000000'\n", read_result);
          *((uint32_t *) (virt_addr + 0x2C) ) = 0x00;
	  usleep(100000);
          // Clk250 reset
          *((uint32_t *) (virt_addr + 0x100) ) = 0x100;
	  usleep(10000);
	  // clk125 reset
          *((uint32_t *) (virt_addr + 0x100) ) = 0x200;
	  usleep(10000);
	  sleep(1);
	  // Check if the FPGA is ready, board programmed?
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
	  printf("Board Status 0xB8: %x\n", read_result);
          if (((read_result >>24)&0xe4) < 0xe4) {
	    printf("\n Board setup is aborted !!! \n");
	    goto TImasterEnd; }
	  // TI register reset to its default value
          *((uint32_t *) (virt_addr + 0x100) ) = 0x10;
	  usleep(1000);
          // Fiber reset
          *((uint32_t *) (virt_addr + 0x100) ) = 0x20;
          sleep(2);

	  // Load TI trigger table  (mode#2, TS#5-1 generate trigger_1, TS#6 generate trigger_2, if both trigger_1 and trigger_2, that is SyncEvent
	  *((uint32_t *) (virt_addr + 0x140)) = 0x43424100; //0c0c0c00;
          usleep(10);
	  *((uint32_t *) (virt_addr + 0x144)) = 0x47464544; //0c0c0c0c;
          usleep(10);
	  *((uint32_t *) (virt_addr + 0x148)) = 0x4b4a4948; //ccccccc0;
          usleep(10);
	  *((uint32_t *) (virt_addr + 0x14C)) = 0x4f4e4d4c; //CCCCCCCC;
          usleep(10);
	  *((uint32_t *) (virt_addr + 0x150)) = 0x53525150; //CCCCCCC0;
          usleep(10);
	  *((uint32_t *) (virt_addr + 0x154)) = 0x57565554; //CCCCCCCC;
          usleep(10);
	  *((uint32_t *) (virt_addr + 0x158)) = 0x5b5a5958; //CCCCCCC0;
          usleep(10);
	  *((uint32_t *) (virt_addr + 0x15C)) = 0x5f5e5d5c; //CCCCCCCC;
          usleep(10);
	  *((uint32_t *) (virt_addr + 0x160)) = 0xe3e2e1a0; //CCCCCCC0;
          usleep(10);
	  *((uint32_t *) (virt_addr + 0x164)) = 0xe7e6e5e4; //CCCCCCCC;
          usleep(10);
	  *((uint32_t *) (virt_addr + 0x168)) = 0xebeae9e8; //CCCCCCC0;
          usleep(10);
	  *((uint32_t *) (virt_addr + 0x16C)) = 0xefeeedec; //CCCCCCCC;
          usleep(10);
	  *((uint32_t *) (virt_addr + 0x170)) = 0xf3f2f1f0; //CCCCCCC0;
          usleep(10);
	  *((uint32_t *) (virt_addr + 0x174)) = 0xf7f6f5f4; //CCCCCCCC;
          usleep(10);
	  *((uint32_t *) (virt_addr + 0x178)) = 0xfbfaf9f8; //CCCCCCC0;
          usleep(10);
	  *((uint32_t *) (virt_addr + 0x17C)) = 0xfffefdfc; //CCCCCCCC;
          usleep(10);

	  // load the front panel TS_code input delay to avoid the BUSY (if using the BUSY as TS_code inputs)
	  *((uint32_t *) (virt_addr + 0x104)) = 0xfffefdfc; //CCCCCCCC;
          usleep(10);
	  *((uint32_t *) (virt_addr + 0x108)) = 0x0ff3fcff; //CCCCCCCC;
          usleep(10);

	  // Sync Interface
          *((uint32_t *) (virt_addr + 0x7C) ) = 0x52; // sync delay before being serialized
          *((uint32_t *) (virt_addr + 0x80) ) = 0x20; // Sync (reset) width
	  // sync phase alignment for Fiber#1 input
          *((uint32_t *) (virt_addr + 0x100) ) = 0x800;
	  // sync phase alignment for Fiber#5 input  Oct. 7, 2022
          *((uint32_t *) (virt_addr + 0x100) ) = 0x1000;
	  usleep(100000);
	  // Sync source enable
          *((uint32_t *) (virt_addr + 0x24) ) = 0x10;
	  usleep(10000);
	  //FPGA IOdelay reset
          *((uint32_t *) (virt_addr + 0x100) ) = 0x4000;
	  sleep(1);
	  // sync phase alignment, moved above
	  //          *((uint32_t *) (virt_addr + 0x100) ) = 0x800;
	  usleep(100000);
	  // Sync delay on master
          *((uint32_t *) (virt_addr + 0x50) ) = 0x2fbf2f2f;
	  usleep(100);
	  //SyncReset
          *((uint32_t *) (virt_addr + 0x78) ) = 0xdd;
	  usleep(10000);
	  // TI data format
          *((uint32_t *) (virt_addr + 0x18) ) = 0x60000003;
	  //TI block size
          *((uint32_t *) (virt_addr + 0x14) ) = 0x28;
	  // TI crate ID
          *((uint32_t *) (virt_addr + 0x00) ) = 0xEA; // tiE#A
          read_result = *((uint32_t *) (virt_addr+0x0) );  // TI ID?
          printf("TI board ID register 0x00: %8x \n", read_result);
          // Set Busy source
          *((uint32_t *) (virt_addr + 0x28) ) = 0x80; // TImaster itself
	  // trigger pulse width
          *((uint32_t *) (virt_addr + 0x0c) ) = 0x0f000f00;
	  // trigger prescale
          *((uint32_t *) (virt_addr + 0x30) ) = 0x00;
	  // trigger rules
	  *((uint32_t *) (virt_addr + 0x38) ) = 0x0a0a0a0a;
	  // Trigger fiber output disable
          *((uint32_t *) (virt_addr + 0x78) ) = 0x77;
	  sleep(1);
	  // MGT sync
          *((uint32_t *) (virt_addr + 0x100) ) = 0x400;
	  sleep(1);
	  // trigger source enable
          *((uint32_t *) (virt_addr + 0x20) ) = 0x94;
	  // TS buffer level
          *((uint32_t *) (virt_addr + 0x34) ) = 0x28;
	  // TI IRQ ID
          *((uint32_t *) (virt_addr + 0x08) ) = 0x5c0;
	  usleep(100);
	  // TI reset
          *((uint32_t *) (virt_addr + 0x78) ) = 0xdd;
	  usleep(10000);
	  // one more reset
          *((uint32_t *) (virt_addr + 0x78) ) = 0xee;
	  usleep(10000);

          printf(" TImasterSet finished \n");
          TImasterEnd:
          fflush(stdout);
	  goto ChooseAgain;

	TIfanout:  // TISMsetup as in trigger.c
          printf(" TI fanout setup, -->Fiber#5, Fiber#1--> ......\n");
          fflush(stdout);
  	  virt_addr = map_base;  // Register base address
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
	  printf("Board Status 0xB8: %x\n", read_result);
 	  // Check the 0xB8 register and make sure it is 0xE.......
          if (((read_result >>24)&0xff) < 0xe0) {
	    // EMFPGA reload
	    printf("!!! quit the program \n");
	    printf("!!! copy the config file back to /sys/bus.../config \n");
	    printf("!!! reload the driver ../tests/load_driver \n");
	    printf("!!! restart the TIcontrol \n");
            // TIpcieUS FPGA reload from SPI flash
            *((uint32_t *) (virt_addr + 0x100) ) = 0x7e; 
	    if (munmap(map_base, MAP_SIZE) == -1)  FATAL;
	    close(fd);
	    printf(" exit \n"); exit(1);  }

         if (((read_result >>24)&0xff) < 0xe0) goto TIfanoutEnd;
	  // Disable Sync first, to avoid unwanted Sync_Resets
          *((uint32_t *) (virt_addr + 0x24) ) = 0x00;
	  usleep(1000);
	  // Register reset to default 
          *((uint32_t *) (virt_addr + 0x100) ) = 0x10;
	  usleep(100);
	  // Disable Sync first, to avoid unwanted Sync_Resets
          *((uint32_t *) (virt_addr + 0x24) ) = 0x00;
	  usleep(1000);
	  // check the DCM lock
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
	  printf("Board Status before fiber reset 0xB8: %x\n", read_result);
 	  // Fiber reset
          *((uint32_t *) (virt_addr + 0x100) ) = 0x20;
	  sleep(2);
	  // check the DCM lock
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
	  printf("Board Status after fiber reset and 2 seconds delay 0xB8: %x\n", read_result);
 	  // TI clock selection
          *((uint32_t *) (virt_addr + 0x2C) ) = 0x50001;
	  sleep(2);
	  // check the DCM lock
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
	  printf("Board Status after clock switch and 2 seconds delay 0xB8: %x\n", read_result);
 	  // TI  MGT reset
          *((uint32_t *) (virt_addr + 0x100) ) = 0x400;
	  sleep(1);
	  // TI clock DCM resets
          *((uint32_t *) (virt_addr + 0x100) ) = 0x100;
	  usleep(100000);
          *((uint32_t *) (virt_addr + 0x100) ) = 0x200;
	  usleep(100000);
	  // check the DCM lock
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
	  printf("Board Status after DCM reset 0xB8: %x\n", read_result);
 	  // FPGA IOdelay reset
          *((uint32_t *) (virt_addr + 0x100) ) = 0x4000;
	  usleep(100000);
	  sleep(1);
	  // check the DCM lock
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
	  printf("Board Status 0xB8: %x\n", read_result);
          if (((read_result >>24)&0xe4) < 0xe4) {
	    printf("\n Board setup is aborted !!! \n");
	    goto TIfanoutEnd; }

	  // sync phase alignment, The following two are disabled on gu-pc, but enabled on gu-pchome
	  *((uint32_t *) (virt_addr + 0x100) ) = 0x800;
	  usleep(100000);
	  *((uint32_t *) (virt_addr + 0x100) ) = 0x1000;
	  usleep(100000);
	  usleep(100000);
	  // Fiber Length measurement
          *((uint32_t *) (virt_addr + 0x100) ) = 0x2000;
	  usleep(100000);
          *((uint32_t *) (virt_addr + 0x100) ) = 0x8000;
	  usleep(10000);
	  // readout the measurement
          read_result = *((uint32_t *) (virt_addr+0xa4) );
	  printf("Fiber Measurement 0xA4: %x before sync phase adjustment\n", read_result);
 	  // readout the measurement
          read_result = *((uint32_t *) (virt_addr+0xa4) );
	  printf("Fiber Measurement 0xA4: %x\n", read_result);
          SyncDelay = (0xbf - (((read_result >>23)&0x1ff)/2));
	  // set the sync delay to bit(15:8)
          *((uint32_t *) (virt_addr + 0x50) ) = ((SyncDelay <<24) & 0xff000000);
	  usleep(1000);
          read_result = *((uint32_t *) (virt_addr+0x50) );
	  printf("SyncDelay measured: %02x, set as: %08x\n", (SyncDelay & 0xff), read_result);
         *((uint32_t *) (virt_addr + 0x100) ) = 0x1800; // sync phase adjustment, both fibers
	  sleep(1);
	  // Trigger source setting
          *((uint32_t *) (virt_addr + 0x20 ) ) = 0x490; // VME and HFBR#5
	  usleep(1000);
 	  // Set trigger pulse width and delay
          *((uint32_t *) (virt_addr + 0xc  ) ) = 0x0f000f00;
	  usleep(1000);
 	  // Set SYNC pulse width
          *((uint32_t *) (virt_addr + 0x80) ) = 0x20;
	  usleep(1000);
 	  // Set SYNC source
  	  // TI SYNC auto alignment first, enable this on Oct. 7, 2022
	 *((uint32_t *) (virt_addr + 0x100) ) = 0x1000;
	  usleep(100000);
         *((uint32_t *) (virt_addr + 0x24) ) = 0x4;
	  usleep(1000);
 	  // Set Block Size
         *((uint32_t *) (virt_addr + 0x14) ) = 0xa;
	  usleep(1000);
 	  // Set Block Threshold
         *((uint32_t *) (virt_addr + 0x34) ) = 0x22;
	  usleep(1000);
 	  // Set crate ID
         *((uint32_t *) (virt_addr + 0x00) ) = 0xEB;
	  usleep(1000);
 	  // Set data format
         *((uint32_t *) (virt_addr + 0x18) ) = 0x60000003;
	  usleep(1000);
	  // disable the following 
 	  // TI IOdelay reset
	  //         *((uint32_t *) (virt_addr + 0x100) ) = 0x4000;
	  usleep(100000);
 	  // TI SYNC auto alignment
	  //        *((uint32_t *) (virt_addr + 0x100) ) = 0x1000;
	  usleep(100000);
 	  // TI auto fiber delay measurement; TI fiber delay measurement
	  // TI MGT synchronization
         *((uint32_t *) (virt_addr + 0x100) ) = 0x400;
	  usleep(100000);
 	  // set TI to running mode
	  //         *((uint32_t *) (virt_addr + 0x9c) ) = 0x71;

          printf(" TI reg0x00 %08x, is set in FanOut mode \n", *((uint32_t *)(virt_addr)) );
          fflush(stdout);
  	  TIfanoutEnd:
	  goto ChooseAgain;

	TIsetup:
          printf(" TIsetup......\n");
          fflush(stdout);
  	  virt_addr = map_base;  // Register base address
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
	  printf("Board Status 0xB8: %x\n", read_result);
	  // Check the 0xB8 register and make sure it is 0xE.......
          if (((read_result >>24)&0xff) < 0xe0) {
	    // EMFPGA reload
	    printf("!!! quit the program \n");
	    printf("!!! copy the config file back to /sys/bus.../config \n");
	    printf("!!! reload the driver ../tests/load_driver \n");
	    printf("!!! restart the TIcontrol \n");
            // TIpcieUS FPGA reload from SPI flash
            *((uint32_t *) (virt_addr + 0x100) ) = 0x7e; 
	    if (munmap(map_base, MAP_SIZE) == -1)  FATAL;
	    close(fd);
	    printf(" exit \n"); exit(1); }

          if (((read_result >>24)&0xff) < 0xe0) goto TIsetupEnd;
	  // Disable Sync first, to avoid unwanted Sync_Resets
          *((uint32_t *) (virt_addr + 0x24) ) = 0x00;
	  usleep(1000);
	  // Register reset to default 
          *((uint32_t *) (virt_addr + 0x100) ) = 0x10;
	  usleep(100);
	  // Disable Sync first, to avoid unwanted Sync_Resets
          *((uint32_t *) (virt_addr + 0x24) ) = 0x00;
	  usleep(1000);
	  // Fiber reset
          *((uint32_t *) (virt_addr + 0x100) ) = 0x20;
	  sleep(2);
	  // check the DCM lock
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
	  printf("Board Status after fiber reset and 2 seconds delay 0xB8: %x\n", read_result);
 	  // TI clock selection
          *((uint32_t *) (virt_addr + 0x2C) ) = 0x02;
	  sleep(1);
	  // check the DCM lock
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
	  printf("Board Status after clock change and 1 second delay 0xB8: %x\n", read_result);
 	  // TI clock MGT reset
          *((uint32_t *) (virt_addr + 0x100) ) = 0x400;
	  sleep(1);
	  // TI clock DCM resets
          *((uint32_t *) (virt_addr + 0x100) ) = 0x100;
	  usleep(100000);
          *((uint32_t *) (virt_addr + 0x100) ) = 0x200;
	  usleep(100000);
	  // FPGA IOdelay reset
          *((uint32_t *) (virt_addr + 0x100) ) = 0x4000;
	  usleep(100000);
	  sleep(1);
	  // check the DCM lock
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
	  printf("Board Status after clock switch, 0xB8: %x\n", read_result);
          if (((read_result >>24)&0xe4) < 0xe4) {
	    printf("\n Board setup is aborted !!!\n");
	    goto TIsetupEnd; }

	  // Fiber Length measurement
          *((uint32_t *) (virt_addr + 0x100) ) = 0x2000;
	  usleep(100000);
          *((uint32_t *) (virt_addr + 0x100) ) = 0x8000;
	  usleep(10000);
	  // readout the measurement
          read_result = *((uint32_t *) (virt_addr+0xa0) );
	  printf("Fiber Measurement 0xA0: %x before sync phase adjustment\n", read_result);
	  // readout the measurement
          read_result = *((uint32_t *) (virt_addr+0xa0) );
	  printf("Fiber Measurement 0xA0: %x\n", read_result);
          SyncDelay = (0xbf - (((read_result >>23)&0x1ff)/2));
	  // set the sync delay to bit(15:8)
          *((uint32_t *) (virt_addr + 0x50) ) = ((SyncDelay <<8) & 0xff00);
	  usleep(1000);
          read_result = *((uint32_t *) (virt_addr+0x50) );
	  printf("SyncDelay measured: %02x, set as: %08x\n", (SyncDelay & 0xff), read_result);
          *((uint32_t *) (virt_addr + 0x100) ) = 0x800; // sync phase adjustment
	  sleep(1);

	  // Enable TI trigger source
          *((uint32_t *) (virt_addr + 0x20) ) = 0x92;
	  usleep(1000);
	  // Set trigger pulse width and Delay
          *((uint32_t *) (virt_addr + 0x0c) ) = 0x0f000f00;
	  usleep(1000);
	  // Set Sync pulse width
          *((uint32_t *) (virt_addr + 0x80) ) = 0x20;
	  usleep(1000);
	  // Set Sync Source
	  // TI sync (Fiber#1) auto_alignment first, Oct. 7, 2022
	  *((uint32_t *) (virt_addr + 0x100) ) = 0x800;
	  usleep(100000);
           *((uint32_t *) (virt_addr + 0x24) ) = 0x02;
	  usleep(1000);
	  // set block size
          *((uint32_t *) (virt_addr + 0x14) ) = 0x0a;
	  usleep(1000);
	  // set buffer level
          *((uint32_t *) (virt_addr + 0x34) ) = 0x28;
	  usleep(1000);
	  // set crate ID
          *((uint32_t *) (virt_addr + 0x00) ) = 0xEB;
	  usleep(1000);
	  // set the data format
          *((uint32_t *) (virt_addr + 0x18) ) = 0x60000003;
	  usleep(10000);
	  // disable the following, enable the two lines on 1/7/2022
	  // TI iodelay reset
	  //	  *((uint32_t *) (virt_addr + 0x100) ) = 0x4000;
	  usleep(10000);
	  // TI sync auto_alignment
	  // *((uint32_t *) (virt_addr + 0x100) ) = 0x800;
	  usleep(10000);
	  // TI auto align fiber delay
          // *((uint32_t *) (virt_addr + 0x100) ) = 0x2000;
	  usleep(100000);
	  // TI auto fiber delay measurement
          // *((uint32_t *) (virt_addr + 0x100) ) = 0x8000;
	  usleep(100000);
	  // TI sync delay
          *((uint32_t *) (virt_addr + 0x80) ) = 0x20;
	  usleep(1000);
	  // TI MGT reset
          *((uint32_t *) (virt_addr + 0x100) ) = 0x400;
	  sleep(1);
          read_result = *((uint32_t *) (virt_addr+0x00) );  //FPGA ready?
	  printf("Board ID (reg 0x00): %x\n", read_result);

          printf(" TI is set into slave mode with Fiber#1 as input\n");
          fflush(stdout);
 	  TIsetupEnd:



	  /*sergey
	  goto ChooseAgain;
	  */
          if (munmap(map_base, MAP_SIZE) == -1)  FATAL;
	  close(fd);
	  printf(" exit \n");
          exit(0);



	Fiber1Meas:
          printf(" TI Fiber#1 Length Measurement ......\n");
          fflush(stdout);
  	  virt_addr = map_base;  // Register base address
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
	  printf("Board Status 0xB8: %x\n", read_result);
          if (((read_result >>24)&0xff) < 0xa0) goto Fiber1MeasEnd;
	  usleep(1000);
	  // Fiber Length measurement
          for (irepeat =1; irepeat < 10; irepeat ++) {
            *((uint32_t *) (virt_addr + 0x100) ) = 0x2000;
	    usleep(100000);
            *((uint32_t *) (virt_addr + 0x100) ) = 0x8000;
	    usleep(10000);
	    // readout the measurement
            read_result = *((uint32_t *) (virt_addr+0xa0) );
	    printf("Measurement Result 0xA0: %x\n", read_result); }
          SyncDelay = (0xbf - (((read_result >>23)&0x1ff)/2));
	  // disable the SYNC source
          read_result2 = *((uint32_t *) (virt_addr+0x24) );  //FPGA ready?
          *((uint32_t *) (virt_addr + 0x24) ) = 0x00;
	  usleep(1000);
	  // set the sync delay to bit(15:8)
          *((uint32_t *) (virt_addr + 0x50) ) = ((SyncDelay <<8) & 0xff00);
	  usleep(1000);
          read_result = *((uint32_t *) (virt_addr+0x50) );
	  printf("SyncDelay measured: %02x, set as: %08x\n", (SyncDelay & 0xff), read_result);
	  // load back the SYNC setting
          *((uint32_t *) (virt_addr + 0x100) ) = 0x1800;
          sleep(1);
          *((uint32_t *) (virt_addr + 0x24) ) = read_result2;

          printf(" TI Fiber#1 Length Measurement finished \n");
          fflush(stdout);
	  Fiber1MeasEnd:
	  goto ChooseAgain;

	Fiber5Meas:
          printf(" TI Fiber#5 Length Measurement ......\n");
          fflush(stdout);
  	  virt_addr = map_base;  // Register base address
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
	  printf("Board Status 0xB8: %x\n", read_result);
          if (((read_result >>24)&0xff) < 0xa0) goto Fiber5MeasEnd;
	  // Fiber Length measurement
          for (irepeat =1; irepeat < 10; irepeat ++) {
            *((uint32_t *) (virt_addr + 0x100) ) = 0x2000;
	    usleep(100000);
            *((uint32_t *) (virt_addr + 0x100) ) = 0x8000;
	    usleep(10000);
	    // readout the measurement
            read_result = *((uint32_t *) (virt_addr+0xa4) );
	    printf("Measurement Result 0xA4: %x\n", read_result); }
          SyncDelay = (0xbf - (((read_result >>23)&0x1ff)/2));
	  // disable the SYNC source
          read_result2 = *((uint32_t *) (virt_addr+0x24) );  //FPGA ready?
          *((uint32_t *) (virt_addr + 0x24) ) = 0x00;
	  usleep(1000);
	  // set the sync delay to bit(23:16)
          *((uint32_t *) (virt_addr + 0x50) ) = ((SyncDelay <<24) & 0xff000000);
	  usleep(1000);
          read_result = *((uint32_t *) (virt_addr+0x50) );
	  printf("SyncDelay measured: %02x, set as: %08x\n", (SyncDelay & 0xff), read_result);
          *((uint32_t *) (virt_addr + 0x100) ) = 0x1800;
          sleep(1);
	  // load back the SYNC setting
          *((uint32_t *) (virt_addr + 0x24) ) = read_result2;

          printf(" TI Fiber#5 Length Measurement finished \n");
          fflush(stdout);
	  Fiber5MeasEnd:
	  goto ChooseAgain;

	TIsetup5:
          printf(" TIsetup5, Activate Fiber#5 as slave ......\n");
          fflush(stdout);
  	  virt_addr = map_base;  // Register base address
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
	  printf("Board Status 0xB8: %x\n", read_result);
	  // Check the 0xB8 register and make sure it is 0xE.......
          if (((read_result >>24)&0xff) < 0xe0) {
	    // EMFPGA reload
	    printf("!!! quit the program \n");
	    printf("!!! copy the config file back to /sys/bus.../config \n");
	    printf("!!! reload the driver ../tests/load_driver \n");
	    printf("!!! restart the TIcontrol \n");
            // TIpcieUS FPGA reload from SPI flash
            *((uint32_t *) (virt_addr + 0x100) ) = 0x7e; 
	    if (munmap(map_base, MAP_SIZE) == -1)  FATAL;
	    close(fd);
	    printf(" exit \n"); exit(1); }

         if (((read_result >>24)&0xff) < 0xe0) goto TIsetup5End;

	  // Disable Sync first, to avoid unwanted Sync_Resets
          *((uint32_t *) (virt_addr + 0x24) ) = 0x00;
	  usleep(1000);

	  // Register reset to default 
          *((uint32_t *) (virt_addr + 0x100) ) = 0x10;
	  usleep(1000);
	  // Disable Sync first, to avoid unwanted Sync_Resets
          *((uint32_t *) (virt_addr + 0x24) ) = 0x00;
	  usleep(1000);
	  // Fiber reset
          *((uint32_t *) (virt_addr + 0x100) ) = 0x20;
	  sleep(2);
	  // check the DCM lock
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
	  printf("Board Status after fiber reset and 2 seconds delay 0xB8: %x\n", read_result);
 	  // TI clock selection
          *((uint32_t *) (virt_addr + 0x2C) ) = 0x01;
	  sleep(2);
	  // check the DCM lock
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
	  printf("Board Status after clock switch and 1 second delay 0xB8: %x\n", read_result);
 	  // TI clock MGT reset
	  //          *((uint32_t *) (virt_addr + 0x100) ) = 0x20;
          // TI MGT reset
          *((uint32_t *) (virt_addr + 0x100) ) = 0x400;
	  sleep(1);
	  // TI clock DCM resets
          *((uint32_t *) (virt_addr + 0x100) ) = 0x100;
	  usleep(100000);
          *((uint32_t *) (virt_addr + 0x100) ) = 0x200;
	  usleep(100000);
	  // IOdelay reset
          *((uint32_t *) (virt_addr + 0x100) ) = 0x4000;
	  usleep(100000);
	  sleep(1);
	  // check the DCM lock
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
	  printf("Board Status after DCM reset 0xB8: %x\n", read_result);
          if (((read_result >>24)&0xe4) < 0xe4) {  // requires the IOdelay reset ready
	    printf("\n Board setup is aborted !!!\n");
	    goto TIsetup5End; }

	  // Fiber Length measurement
          *((uint32_t *) (virt_addr + 0x100) ) = 0x2000;
	  usleep(100000);
          *((uint32_t *) (virt_addr + 0x100) ) = 0x8000;
	  usleep(10000);
	  // readout the measurement
          read_result = *((uint32_t *) (virt_addr+0xa4) );
	  printf("Fiber Measurement 0xA4: %x before sync phase adjustment\n", read_result);
	  // readout the measurement
          read_result = *((uint32_t *) (virt_addr+0xa4) );
	  printf("Fiber Measurement 0xA4: %x\n", read_result);
          SyncDelay = (0xbf - (((read_result >>23)&0x1ff)/2));
	  // set the sync delay to bit(31:24)
          *((uint32_t *) (virt_addr + 0x50) ) = ((SyncDelay <<24) & 0xff000000);
	  usleep(1000);
          read_result = *((uint32_t *) (virt_addr+0x50) );
	  printf("SyncDelay measured: %02x, set as: %08x\n", (SyncDelay & 0xff), read_result);
          *((uint32_t *) (virt_addr + 0x100) ) = 0x1000; // sync phase adjustment
	  sleep(1);

	  // Enable TI trigger source
          *((uint32_t *) (virt_addr + 0x20) ) = 0x490;
	  usleep(1000);
	  // Set trigger pulse width and Delay
          *((uint32_t *) (virt_addr + 0x0c) ) = 0x0f000f00;
	  usleep(1000);
	  // Set Sync pulse width
          *((uint32_t *) (virt_addr + 0x80) ) = 0x20;
	  usleep(1000);
	  // Set Sync Source, move it here
          *((uint32_t *) (virt_addr + 0x100) ) = 0x1000; // sync phase adjustment first Oct. 7, 2022
	  sleep(1);
          *((uint32_t *) (virt_addr + 0x24) ) = 0x04;
	  usleep(1000);
	  usleep(1000);
	  // set block size
          *((uint32_t *) (virt_addr + 0x14) ) = 0x0a;
	  usleep(1000);
	  // set buffer level
          *((uint32_t *) (virt_addr + 0x34) ) = 0x28;
	  usleep(1000);
	  // set crate ID
          *((uint32_t *) (virt_addr + 0x00) ) = 0xEC;
	  usleep(1000);
	  // set the data format
          *((uint32_t *) (virt_addr + 0x18) ) = 0x60000003;
	  usleep(10000);
	  // TI iodelay reset
	  //          *((uint32_t *) (virt_addr + 0x100) ) = 0x4000;
	  usleep(10000);
	  // TI sync auto_alignment (fiber#5)
	  //          *((uint32_t *) (virt_addr + 0x100) ) = 0x1000;
	  usleep(100000);
	  // TI sync auto_alignment (fiber#1)
	  //          *((uint32_t *) (virt_addr + 0x100) ) = 0x800;
	  usleep(100000);
	  // TI auto align fiber delay
	  //          *((uint32_t *) (virt_addr + 0x100) ) = 0x2000;
	  usleep(10000);
	  // TI auto fiber delay measurement
	  //          *((uint32_t *) (virt_addr + 0x100) ) = 0x8000;
	  usleep(10000);
	  // TI sync delay
          *((uint32_t *) (virt_addr + 0x80) ) = 0x20;
	  usleep(1000);
	  // TI MGT reset, change to MGT Rx reset (only)
          *((uint32_t *) (virt_addr + 0x100) ) = 0x400;
	  sleep(1);
          read_result = *((uint32_t *) (virt_addr+0x00) );  //FPGA ready?
	  printf("Board ID (reg 0x00): %x\n", read_result);

          printf(" TI is set into slave mode with Fiber#5 as input\n");
          fflush(stdout);
	  TIsetup5End:
	  goto ChooseAgain;

        TrgStart:
          printf(" TrgStart......\n  Enter the Block level:");
          fflush(stdout);
	  scanf("%d", &BlockLevel);
	  BlockLevel = (BlockLevel & 0xff);
          printf("\n  Enter the Buffer Level: ");
	  scanf("%d", &BufferLevel);
          BufferLevel = (BufferLevel & 0xff);

  	  virt_addr = map_base;  // Register base address
	  // disable the trigger link first (set to idle)
	  *((uint32_t *) (virt_addr + 0x78) ) = 0x77;
	  sleep(1);

          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?
          if (((read_result >>24)&0xe4) < 0xe4) {
            printf(" FPGA is not ready 0xB8: %x\n", read_result);
	    printf("\n TrgStart aborted !!! \n");
            goto TrgStartEnd;  }

	  // change "MGT reset, data 0x400" to "MGT RxReset, data 0x400000", Oct. 24, 2022 
          *((uint32_t *) (virt_addr + 0x100) ) = 0x400000;
	  sleep(2);

	  // TS buffer level
          *((uint32_t *) (virt_addr + 0x34) ) = BufferLevel;
	  // TS Block level
          *((uint32_t *) (virt_addr + 0x14) ) = BlockLevel;
	  usleep(1000);

	  // disable the sync on the TImaster
          read_result = *((uint32_t *) (virt_addr+0x24) );  //FPGA ready?
          *((uint32_t *) (virt_addr + 0x24) ) = 0x00;
	  usleep(1000);
	  // MGT reset to the TI_slave
          *((uint32_t *) (virt_addr + 0x78) ) = 0x77;
	  usleep(100000);
	  // Changing "0x78->0x22" to "0x33, clock phase reset only" will not work properly. Change back to 0x22. Oct. 24, 2022
          *((uint32_t *) (virt_addr + 0x78) ) = 0x22;
	  sleep(2);
          *((uint32_t *) (virt_addr + 0x24) ) = (read_result & 0xff);
	  usleep(1000);
	  // TI mgt RX reset
          *((uint32_t *) (virt_addr + 0x100) ) = 0x400000;
	  sleep(1);
	  // comment the 0x80, 0x7C setting out Oct. 7, 2022
	  //          *((uint32_t *) (virt_addr + 0x80) ) = 0x20;
	  usleep(1000);
	  //          *((uint32_t *) (virt_addr + 0x7c) ) = 0x62;
	  usleep(1000);
	  // trigger link reset
          *((uint32_t *) (virt_addr + 0x78) ) = 0x77;  // disable the link
	  usleep(100000);
          *((uint32_t *) (virt_addr + 0x78) ) = 0x77;  // disable the link
	  sleep(1);
          *((uint32_t *) (virt_addr + 0x78) ) = 0x55; // enable the trigger link
	  sleep(2);
	  printf(" Trigger link reseted \n");
          *((uint32_t *) (virt_addr + 0x78) ) = 0xee; // sync_reset
	  usleep(100000);
          *((uint32_t *) (virt_addr + 0x78) ) = 0xbb;
	  usleep(1000);
	  sleep(2);
	  printf(" broadcast the block_level \n");
	  // set the block level 
          *((uint32_t *) (virt_addr + 0x84) ) = ((0x800 + BlockLevel) & 0xfff); // block level 18 --> 250
	  usleep(10000);
	  printf(" Broadcast the Buffer_level \n");
	  // set the buffer level
          *((uint32_t *) (virt_addr + 0x84) ) = ((0xc00 + BufferLevel) & 0xfff); // buffer_level 8 --> 40
	  usleep(10000);
          *((uint32_t *) (virt_addr + 0x78) ) = 0xee; // sync_reset
	  usleep(100000);
	  // reset the event number
          *((uint32_t *) (virt_addr + 0x78) ) = 0xbb;
	  usleep(10000);
	  // 1/7/2022     *((uint32_t *) (virt_addr + 0x78) ) = 0xee;  //SyncReset just before triggering
	  usleep(1000);
	  // set the sync event period
          *((uint32_t *) (virt_addr + 0xd4) ) = 0x00; // disable the periodic sync events
	  usleep(1000);
	  printf(" starting the periodic trigger ... 0x89750100 \n");
	  // Periodic event trigger
          *((uint32_t *) (virt_addr + 0x8c) ) = 0x87950100;
	  sleep(5);
          read_result = *((uint32_t *) (virt_addr+0x34) );  //Any data?
          printf(" 0x34 status after 5 seconds delay after trigger_start: %08x \n", read_result);

	  //          printf(" TrgStarted \n");
          fflush(stdout);
          TrgStartEnd: 
	  goto ChooseAgain;

	I2CopticTrx:
          printf(" I2CopticTrx......\n");
          fflush(stdout);

  	  virt_addr = map_base + 0x800;  // Fiber#1 base address
          read_result = *((uint32_t *) (virt_addr+88) );  //temp
          printf(" Optic#1 Temperature: %d \n", read_result&0xff);
          read_result = *((uint32_t *) (virt_addr+104) );  //Supply Voltage
          read_resultN = *((uint32_t *) (virt_addr+108) );  //lower byte
          printf(" Optic#1 Voltage: %d mV\n", ((read_result&0xff)*256/10 + (read_resultN & 0xff)/10));
          read_result = *((uint32_t *) (virt_addr+136) );  //RxLevel
          read_resultN = *((uint32_t *) (virt_addr+140) );  //lower byte
          printf(" Optic#1 Rx#1Level: %d uW\n", ((read_result&0xff)*256/10 + (read_resultN & 0xff)/10));
          read_result = *((uint32_t *) (virt_addr+144) );  //RxLevel
          read_resultN = *((uint32_t *) (virt_addr+148) );  //lower byte
          printf(" Optic#1 Rx#2Level: %d uW\n", ((read_result&0xff)*256/10 + (read_resultN & 0xff)/10));
          read_result = *((uint32_t *) (virt_addr+152) );  //RxLevel
          read_resultN = *((uint32_t *) (virt_addr+156) );  //lower byte
          printf(" Optic#1 Rx#3Level: %d uW\n", ((read_result&0xff)*256/10 + (read_resultN & 0xff)/10));
          read_result = *((uint32_t *) (virt_addr+160) );  //RxLevel
          read_resultN = *((uint32_t *) (virt_addr+164) );  //lower byte
          printf(" Optic#1 Rx#4Level: %d uW\n\n", ((read_result&0xff)*256/10 + (read_resultN & 0xff)/10));
          read_result = *((uint32_t *) (virt_addr+168) );  //TxBias
          read_resultN = *((uint32_t *) (virt_addr+172) );  //lower byte
          printf(" Optic#1 Tx#1biasLevel: %d uA\n", ((read_result&0xff)*512 + (read_resultN & 0xff)*2));
          read_result = *((uint32_t *) (virt_addr+176) );  //TxBias
          read_resultN = *((uint32_t *) (virt_addr+180) );  //lower byte
          printf(" Optic#1 Tx#2biasLevel: %d uA\n", ((read_result&0xff)*512 + (read_resultN & 0xff)*2));
          read_result = *((uint32_t *) (virt_addr+184) );  //TxBias
          read_resultN = *((uint32_t *) (virt_addr+188) );  //lower byte
          printf(" Optic#1 Tx#3biasLevel: %d uA\n", ((read_result&0xff)*512 + (read_resultN & 0xff)*2));
          read_result = *((uint32_t *) (virt_addr+192) );  //TxBias
          read_resultN = *((uint32_t *) (virt_addr+196) );  //lower byte
          printf(" Optic#1 Tx#4biasLevel: %d uA\n", ((read_result&0xff)*512 + (read_resultN & 0xff)*2));
          read_resultN = *((uint32_t *) (virt_addr+344) );  // register byte#86, 0x56
          printf(" Optic#1 Tx Disable, Ch#3-0: %x \n\n", (read_resultN & 0x0f) );

  	  virt_addr = map_base + 0xC00;  // Fiber#5 base address
          read_result = *((uint32_t *) (virt_addr+88) );  //temp
          printf(" Optic#5 Temperature: %d \n", read_result&0xff);
          read_result = *((uint32_t *) (virt_addr+104) );  //Supply Voltage
          read_resultN = *((uint32_t *) (virt_addr+108) );  //lower byte
          printf(" Optic#5 Voltage: %d mV\n", ((read_result&0xff)*256/10 + (read_resultN & 0xff)/10));
          read_result = *((uint32_t *) (virt_addr+136) );  //RxLevel
          read_resultN = *((uint32_t *) (virt_addr+140) );  //lower byte
          printf(" Optic#5 Rx#1Level: %d uW\n", ((read_result&0xff)*256/10 + (read_resultN & 0xff)/10));
          read_result = *((uint32_t *) (virt_addr+144) );  //RxLevel
          read_resultN = *((uint32_t *) (virt_addr+148) );  //lower byte
          printf(" Optic#5 Rx#2Level: %d uW\n", ((read_result&0xff)*256/10 + (read_resultN & 0xff)/10));
          read_result = *((uint32_t *) (virt_addr+152) );  //RxLevel
          read_resultN = *((uint32_t *) (virt_addr+156) );  //lower byte
          printf(" Optic#5 Rx#3Level: %d uW\n", ((read_result&0xff)*256/10 + (read_resultN & 0xff)/10));
          read_result = *((uint32_t *) (virt_addr+160) );  //RxLevel
          read_resultN = *((uint32_t *) (virt_addr+164) );  //lower byte
          printf(" Optic#5 Rx#4Level: %d uW\n\n", ((read_result&0xff)*256/10 + (read_resultN & 0xff)/10));
          read_result = *((uint32_t *) (virt_addr+168) );  //TxBias
          read_resultN = *((uint32_t *) (virt_addr+172) );  //lower byte
          printf(" Optic#5 Tx#1biasLevel: %d uA\n", ((read_result&0xff)*512 + (read_resultN & 0xff)*2));
          read_result = *((uint32_t *) (virt_addr+176) );  //TxBias
          read_resultN = *((uint32_t *) (virt_addr+180) );  //lower byte
          printf(" Optic#5 Tx#2biasLevel: %d uA\n", ((read_result&0xff)*512 + (read_resultN & 0xff)*2));
          read_result = *((uint32_t *) (virt_addr+184) );  //TxBias
          read_resultN = *((uint32_t *) (virt_addr+188) );  //lower byte
          printf(" Optic#5 Tx#3biasLevel: %d uA\n", ((read_result&0xff)*512 + (read_resultN & 0xff)*2));
          read_result = *((uint32_t *) (virt_addr+192) );  //TxBias
          read_resultN = *((uint32_t *) (virt_addr+196) );  //lower byte
          printf(" Optic#5 Tx#4biasLevel: %d uA\n", ((read_result&0xff)*512 + (read_resultN & 0xff)*2));
          read_resultN = *((uint32_t *) (virt_addr+344) );  // register byte#86, 0x56
          printf(" Optic#5 Tx Disable, Ch#3-0: %01x \n\n", (read_resultN&0x0f) );
          fflush(stdout);

	  goto ChooseAgain;

 	I2CopticDump:
          printf(" I2CopticTrx full register dump......\n");
          fflush(stdout);

	  // full register dump
  	  virt_addr = map_base + 0x800;  // Fiber#1 base address
	  printf(" Fiber#1, Base address readout \n");
	  for (ireg = 0; ireg<128; ireg++) {
	    if (ireg%16 == 0) printf("\n reg 0x%02x ", ireg);
            read_result = *((uint32_t *) (virt_addr+ireg*4) );
            printf(" %02x", read_result&0xff); }
	  for (irepeat =0; irepeat<4; irepeat ++) {
	    printf("\n Fiber#1, Higher address readout area# %d\n", irepeat);
	    *((uint32_t *) (virt_addr + 0x1fc) ) = irepeat;
	    for (ireg = 0; ireg<128; ireg++) {
	      if (ireg%16 == 0) printf("\n reg 0x%02x ", ireg);
	      read_result = *((uint32_t *) (virt_addr + 0x200 + ireg*4));  
	      printf(" %02x", read_result&0xff); } }

  	  virt_addr = map_base + 0xC00;  // Fiber#5 base address
	  printf("\n\n Fiber#5, Base address readout \n");
	  for (ireg = 0; ireg<128; ireg++) {
	    if (ireg%16 == 0) printf("\n reg 0x%02x ", ireg);
            read_result = *((uint32_t *) (virt_addr+ireg*4) );
            printf(" %02x", read_result&0xff); }
	  for (irepeat =0; irepeat<4; irepeat ++) {
	    printf("\n Fiber#5, Higher address readout area# %d\n", irepeat);
	    *((uint32_t *) (virt_addr + 0x1fc) ) = irepeat;
	    for (ireg = 0; ireg<128; ireg++) {
	      if (ireg%16 == 0) printf("\n reg 0x%02x ", ireg);
	      usleep(1);
	      read_result = *((uint32_t *) (virt_addr + 0x200 + ireg*4));  
	      printf(" %02x", read_result&0xff); } }
	  printf("\n \n Full register dumped \n");
	  goto ChooseAgain;

        tidsRead:
          printf(" tidsRead......\n");
          fflush(stdout);
  	  virt_addr = map_base;  // Register base address
          read_result = *((uint32_t *) (virt_addr+0xB8) );  //FPGA ready?

          printf(" TrgStart......\n");
          fflush(stdout);
	  goto ChooseAgain;

        FlashReset:
          printf("\n Flash Memory Reset... \n");
          virt_addr = map_base;  // Register base address
	  FlashCommand(0x266);
	  FlashCommand(0x299);
          goto ChooseAgain;
  
        FlashID:
          printf("\n Flash Memory ID readout... \n");
	  //          scanf("%x", &SingleAdd);
  	  virt_addr = map_base;  // Register base address
	  FlashMemID = FlashID();

          printf(" Flsah Memory ID: %08x \n", FlashMemID);
          fflush(stdout);
	  goto ChooseAgain;

        FlashErase:
          printf(" Flash erase......\n  Do you want BULK erase? 1 for yes, 0 for no:");
          fflush(stdout);
	  scanf("%d", &BulkErease);
	  if (BulkErease >0 ) {
	    printf("\n Flash Memory bulk Erase... \n");
	    virt_addr = map_base;  // Register base address

	    FlashWEnable();
	    
	    FlashMemID = FlashStatusReg();
	    printf(" Flash Status Register: %08x \n", FlashMemID);
	    if ((FlashMemID & 0x5c) >0 ) {
	      printf(" Flash Memory cannot be BULK erased !!!" );
	      goto ChooseAgain; }

	    // write the flash bulk erase 0xC7
	    FlashCommand(0x2C7);
	    goto ChooseAgain; }
	  else {
	    printf("\n Flash Memory Sector Erase... \n");
	    printf(" Which half? 1 for upper 128 Mb, 0 for lower 128Mb: ");
	    fflush(stdout);
	    scanf("%d", &Area128);
	    virt_addr = map_base;  // Register base address
	    FlashWEnable();
	    
	    FlashMemID = FlashStatusReg();
	    printf(" Flash Status Register: %08x \n", FlashMemID);
	    FlashWEnable();

	    // write the flash page address(31:24) to 0x01, command code: 0xC5
	    FlashCommand(0x1C5);  // 1xx,  keep the CS_B
	    // Shift in the extended address 0x01
	    if (Area128 >0) {
	      FlashCommand(0x201); } 
	    else {	
	      FlashCommand(0x200); } 

	    for (iPage = 0; iPage < 256; iPage ++) {	    // write the Sector erase 0xD8

	      FlashWEnable();
	      FlashCommand(0x1D8);
	      // write the flash page address
	      PageAdd = ((iPage<<16) & 0xff0000);  // 3-byte page address
	      printf("Erase page address %08x \n", PageAdd);
	      WriteData = 0x100 + ((PageAdd>>16) & 0xff);
	      FlashCommand(WriteData);
	      WriteData = 0x100 + ((PageAdd>>8) & 0xff);
	      FlashCommand(WriteData);
	      WriteData = 0x200 + (PageAdd & 0xff);
	      FlashCommand(WriteData);
	      printf(" write to reg 0xE4: %02x sector_ERASE\n", iPage);
	      fflush(stdout);
	      sleep(1); }
	    goto ChooseAgain; }

	  goto ChooseAgain; 

        FlashProgram:
          printf("\n Flash Memory page program... \n");
          virt_addr = map_base;  // Register base address
	  mcsFile = fopen("TIpcieUS.mcs","r");
	  printf(" Which 128Mb area, 0 for lower, 1 for higher 128Mb area: ");
          fflush(stdout);
          scanf("%d", &Area128);
	  LineRead = 0;
	  SectorEdge = 0;
	  EndOfFile = 0;
	  for (iPage = 0; iPage < 65536; iPage ++) {
	    printf(" Program page# 0x%05x ......\n", iPage);
	    LineRead = 0;
	    do { 
	      fgets(bufRead,2,mcsFile);
	      //	      printf("\n skip cahracter %01c 0, %01c 1.\n", bufRead[0], bufRead[1]);
	      //fflush(stdout);
	      fscanf(mcsFile, "%08x", &mcsType);
	      //printf("\n mcsType: %08x", mcsType);
	      //fflush(stdout);
	      if ((((mcsType >> 24) & 0xff) == 0) ) { //&& (LineRead == 16)) {
		EndOfFile = 1;
		// reset the FlashData to 0xff
		for (iByte =0; iByte < 256; iByte ++) {
		  FlashData[iByte] = 0xff; }
		printf(" \n End of File Reached, set the FlashData to 0xff, quit the program \n");
		iPage = 0xFFFFF; }
	      if (((mcsType >>24) & 0xff) == 2) {  // Sector address
		fscanf(mcsFile, "%06x \n", &SectorAdd); 
		printf("\n Next sector: %08x \n", SectorAdd); 
		fflush(stdout); }
	      if (((mcsType >>24) & 0xff) == 0x10) {
		//                printf("  Page address %08x, LineRead: %08x \n", mcsType, LineRead);
		for (iByte = 0; iByte < 16; iByte++) {
		  fscanf(mcsFile, "%02x", &ExtraData);
		  bytePosition = LineRead * 16 + iByte;
		  //		  printf("%02x %08x ", (ExtraData & 0xff), bytePosition);
		  FlashData[bytePosition] = (ExtraData & 0xff);}
		fscanf(mcsFile, "%02x\n", &ExtraData); 
		LineRead += 1; }
	      if (((mcsType >>24) & 0xff) == 0x0C) {
		for (iByte = 0; iByte < 12; iByte++) {
		  fscanf(mcsFile, "%02x", &ExtraData);
		  FlashData[LineRead*16+iByte] = (ExtraData & 0xff);}
		fscanf(mcsFile, "%02x\n", &ExtraData); 
		// fill in the rest with 0xFF
		for (iByte = LineRead*16+12; iByte < 256; iByte ++) {
		  FlashData[iByte] = 0xff; }
		LineRead = 16; }
	      // print out the data set
	      //     if (LineRead == 16) {
	      //	      for (iByte = 0; iByte<16; iByte ++) {
	      //printf("%02x", (FlashData[(LineRead-1)*16 + iByte] & 0xff) ); }
		  //  if ((iByte%16) == 15)
	      //printf("\n");
	      // fflush(stdout);
	    }  while ((LineRead < 16) && (EndOfFile == 0));

	    FlashWEnable();
	    
	    // write the flash page address(31:24) to 0x01, command code: 0xC5
	    FlashCommand(0x1C5); // keep CS_B
	    // Shift in the extended address 0x01
	    if (Area128 >0) {
	      FlashCommand(0x201); }
	    else {	
	      FlashCommand(0x200); }
	    Counter = 0;
            do {  FlashWEnable();
	      FlashMemID = FlashStatusReg();
	      Counter += 1; }  while ( ((FlashMemID & 0x83) != 2) && (Counter <10));
	    //	    if (FlashMemID = FlashStatusReg();
	    printf(" Flash Status Register: %08x \n", FlashMemID);
	    if ((FlashMemID & 0x83) != 2 ) {
	      printf(" Flash Memory is not in WRITE mode, and can not be programmed !!!" );
	      goto ChooseAgain; }
	    // write the flash page program code 0x02
	    FlashCommand(0x102);
          // write the flash page address
	    PageAdd = ((iPage<<8) & 0xffff00);  // 3-byte page address
	    WriteData = 0x100 + ((PageAdd>>16) & 0xff);
	    FlashCommand(WriteData);
	    WriteData = 0x100 + ((PageAdd>>8) & 0xff);
	    FlashCommand(WriteData);
	    WriteData = 0x100 + (PageAdd & 0xff);
	    FlashCommand(WriteData);

	    // load the data
	    for (iByte = 0; iByte < 255; iByte ++) {
	      WriteData = 0x100 + (FlashData[iByte] & 0xff);
	      FlashCommand(WriteData); }
	    WriteData = 0x200 + (FlashData[255] & 0xff);
	    FlashCommand(WriteData);
       	    usleep(1800);
	    usleep(2000); }  // wait for the program to finish 

          goto ChooseAgain;

        FlashFlag:
          printf("\n Flash Memory Flag_Status_register readout... \n");
	  //          scanf("%x", &SingleAdd);
  	  virt_addr = map_base;  // Register base address
	  FlashMemID = FlashFlagReg();
          printf(" Flsah Memory Flag_Status_Reg: %08x \n", FlashMemID);
          fflush(stdout);
	  goto ChooseAgain;

        FlashStatusWrite:
          printf("\n Enter the Status register value to write, \n 0x64 for lower 128Mb protection, 0x44 for upper 128Mb protection, 0x00 for un-protection:");
	  scanf("%x", &BatchN);
	  printf("\n You entered: %x", BatchN);
	  FlashWEnable();
	  
	  // write the status register program code 0x01
	  FlashCommand(0x101);
	  printf("\n write to reg 0xE4: %08x \n", 0x01);
	  WriteData = 0x200 + (BatchN & 0xff);
	  FlashCommand(WriteData);
	  usleep(1800);  // wait for the program to finish 

          goto ChooseAgain;

        FlashStatusRead:
          printf("\n Flash Memory Status_register readout... \n");
	  FlashMemID =  FlashStatusReg();
          printf(" Flsah Memory Status_Register: %08x \n", FlashMemID);
          fflush(stdout);
	  goto ChooseAgain;

        FlashUserCodeProgram:
          printf("\n Flash Memory UserCode program... \n");
          virt_addr = map_base;  // Register base address
	  iPage = 0;
	  for (iByte =0; iByte < 256; iByte ++) {
	    FlashData[iByte] = 0xff;
	    if (iByte<30) FlashData[iByte] = 0; }
	  FlashData[20] = 0xAA;
	  FlashData[21] = 0x55; 
	  FlashData[22] = 0x66; 
	  FlashData[23] = 0x99; 
          printf("\n Enter the production batch number:");
	  scanf("%d", &BatchN);
	  printf("\n Enter the Serial number:");
	  scanf("%d", &SerialN);
      	  FlashData[24] = (0xff & SerialN); // Serial number
	  FlashData[25] = (0x0f & BatchN); // Production batch #
	  FlashData[26] = 0xE2;
	  FlashData[27] = 0x71;
	  FlashData[28] = 0x55; // U
	  FlashData[29] = 0x47; // G
	  FlashData[30] = 0x20; //
	  FlashData[31] = 0x6c; // l
	  FlashData[32] = 0x6c; // l
	  FlashData[33] = 0x69; // i
	  FlashData[34] = 0x57; // W
	  FlashData[35] = 0x4a; // J

	  FlashData[36] = 0x62; // b
	  FlashData[37] = 0x61; // a
	  FlashData[38] = 0x4c; // L
	  FlashData[39] = 0x4a; // J
	  FlashData[40] = 0x20; //
	  FlashData[41] = 0x45; // E
	  FlashData[42] = 0x4f; // O
	  FlashData[43] = 0x44; // D
	  FlashData[44] = 0x55;
	  FlashData[45] = 0xaa;
	  FlashData[46] = 0x99;
	  FlashData[47] = 0x66;
          printf("\n Do you want to lock the OTP registers? 1 for YES: ");
	  scanf("%d", &LockOTP);
	  if (LockOTP == 1) {
	    FlashData[63] = 0xFE;
	    FlashData[64] = 0xFE; }
          printf("\n Do you want to test the OTP lock? 1 for YES: ");
	  scanf("%d", &BatchN);
	  if (BatchN == 1) {
	    FlashData[63] = (FlashData[63] & (0xF1));
	    FlashData[64] = (FlashData[64] & (0xF1)); }
	  for (iByte =0; iByte < 68; iByte ++) {
            printf("\n byte#%2d is %2x", iByte, FlashData[iByte]); }

           //          goto ChooseAgain;
          //FlashData[16]= 0x00;
	  // write the flash WRITE enable 0x06
	  FlashWEnable();
	  // write the flash extended address, page address(31:24) to 0x01, command code: 0xC5
	  FlashCommand(0x1C5);
	  // Shift in the extended address 0x01 (actually, 0 for 24-bit mode)
	  FlashCommand(0x200); // 0x201 for higher 128 Mb region, 0x200 for lower 128 Mb

	  FlashWEnable();
	  
	  FlashMemID = FlashStatusReg();
	  printf(" Flash Status Register: %08x \n", FlashMemID);
	  if ((FlashMemID & 0x80) >0 ) {
	    printf(" Flash Memory is not in WRITE mode, and can not be programmed !!!" );
	    goto ChooseAgain; }
	  // write the flash OTP program code 0x42
	  FlashCommand(0x142);

          // write the flash page address
	  PageAdd = ((iPage<<8) & 0xffff00);  // 3-byte page address
	  WriteData = 0x100 + ((PageAdd>>16) & 0xff);
	  FlashCommand(WriteData);
	  WriteData = 0x100 + ((PageAdd>>8) & 0xff);
	  FlashCommand(WriteData);
	  WriteData = 0x100 + (PageAdd & 0xff);
	  FlashCommand(WriteData);
	  // load the data
	  PageEnd = 63;
	  if (LockOTP == 1) PageEnd = 64;
	  for (iByte = 0; iByte < PageEnd; iByte ++) {
	    WriteData = 0x100 + (FlashData[iByte] &0xff);
	    FlashCommand(WriteData); }
	  // load the data, last byte (#63)
	  //	    FlashData[63] = 0xff, bit0=0 will permanently disable the OTP write;
	  WriteData = 0x200 + (FlashData[PageEnd] &0xff);
	  FlashCommand(WriteData);
	  usleep(1800);  // wait for the program to finish 
          goto ChooseAgain;

        FlashUserCodeRead:
          printf("\n Flash Memory UserCode readout... \n");
	  //          scanf("%x", &SingleAdd);
  	  virt_addr = map_base;  // Register base address
	  // write the flash OTP read 0x4B, address 0
	  FlashCommand(0x14B);
	  printf("Flash OTP read 0x4B \n");
	  // write the byte address  0 -- 3 bytes plus 8 dummy clock cycles
	  FlashCommand(0x100);
	  FlashCommand(0x100);
	  FlashCommand(0x100);
	  FlashCommand(0x300);
	  printf(" write to reg 0xE4: %08x \n", 0x4B, 0x00);
          FlashMemID = 0;
	  for (iByte = 0; iByte < 20; iByte ++) {
	    for (irepeat =0; irepeat<32; irepeat ++) {
	      usleep(10);
	      read_result = *((uint32_t *) (virt_addr+0xE4) );  // 4-bit 
	      FlashMemID = FlashMemID + (((read_result & 0x20000) >> 17) << (31-irepeat));
	      *((uint32_t *) (virt_addr + 0xE4 )) = 0x1F0; }
	    printf(" Flsah Memory ID: Word# %02x : %08x \n", iByte, FlashMemID);
	    FlashMemID = 0; }
	  for (irepeat =0; irepeat<30; irepeat ++) {
	    usleep(10);
	    read_result = *((uint32_t *) (virt_addr+0xE4) );  // 4-bit 
            FlashMemID = FlashMemID + (((read_result & 0x20000) >> 16) << (30-irepeat));
	    *((uint32_t *) (virt_addr + 0xE4 )) = 0x1F0; }
	  usleep(10);
	  read_result = *((uint32_t *) (virt_addr+0xE4) );  // 4-bit 
	  FlashMemID = FlashMemID + ((read_result & 0x20000) >> 16);
	  *((uint32_t *) (virt_addr + 0xE4 )) = 0x0F0; 
	  usleep(10);
	  read_result = *((uint32_t *) (virt_addr+0xE4) );  // 4-bit 
	  FlashMemID = FlashMemID + ((read_result & 0x20000) >> 17);

	  printf(" Read result 0xE4: %08x \n", read_result);
 
          printf(" Flsah Memory ID: %08x \n", FlashMemID);
          fflush(stdout);
	  goto ChooseAgain;

        FlashLastPageProgram:
          printf("\n Flash Memory Last Page program... \n");
	  printf(" Enter the page number (0--FFFF): ");
          fflush(stdout);
          scanf("%x", &iPage);
          virt_addr = map_base;  // Register base address
	  for (iByte =0; iByte < 256; iByte ++) {
	    FlashData[iByte] = 0xff; }
	  FlashData[0] = 0x88;
	  FlashData[1] = 0x11;
	  FlashData[2] = 0x55;
	  FlashData[3] = 0xaa;
	  FlashData[4] = 0x12; // (BoardID & 0xff);
	  FlashData[5] = 0x11; // (Firmware & 0xff);
	  FlashData[6] = 0xe5;
	  FlashData[7] = 0x71;
	  FlashData[8] = 0x55;
	  FlashData[9] = 0xaa;
	  FlashData[10]= 0x66;
	  FlashData[11]= 0x99;

	  FlashWEnable();
	  // write the flash page address(31:24) to 0x01, command code: 0xC5
	  FlashCommand(0x1C5);
	  printf(" write to reg 0xE4: %08x \n", 0xC5);
	  // Shift in the extended address 0x01
	  FlashCommand(0x200); // 0x201 for higher 128 Mb region, 0x200 for lower 128 Mb
	  printf(" write to reg 0xE4: %08x WRITE_ENABLE\n", 0x200);

	  // write the flash WRITE enable 0x06, for page programming
	  FlashWEnable();
	  printf(" write to reg 0xE4: %08x WRITE_ENABLE\n", 0x06);

	  FlashMemID = FlashStatusReg();
	  printf(" Flash Status Register: %08x \n", FlashMemID);
	  if ((FlashMemID & 0x80) >0 ) {
	    printf(" Flash Memory is not in WRITE mode, and can not be programmed !!!" );
	    goto ChooseAgain; }
	  // write the flash page program code 0x02
	  FlashCommand(0x102);  // keep the CS_B
	  printf(" write to reg 0xE4: %08x \n", 0x02);

          // write the flash page address
	  PageAdd = ((iPage<<8) & 0xffff00);  // 3-byte page address
	  WriteData = 0x100 + ((PageAdd>>16) & 0xff);
	  FlashCommand(WriteData);
	  WriteData = 0x100 + ((PageAdd>>8) & 0xff);
	  FlashCommand(WriteData);
	  WriteData = 0x100 + (PageAdd & 0xff);
	  FlashCommand(WriteData);

	  // load the data
	  for (iByte = 0; iByte < 255; iByte ++) {
	    WriteData = 0x100 + (FlashData[iByte] & 0xff);
	    FlashCommand(WriteData); }
	  // load the data, last byte (#255)
	  //	    FlashData[255] = 0xCC;
	  WriteData = 0x200 + (FlashData[255] & 0xff);
	  FlashCommand(WriteData);
	  usleep(1800);  // wait for the program to finish 

          goto ChooseAgain;

        FlashLastPageRead:
          printf("\n Flash Memory Page readout... \n");
	  printf(" Enter the page number (0--FFFF): ");
          fflush(stdout);
          scanf("%x", &SingleAdd);
  	  virt_addr = map_base;  // Register base address
	  // write the flash read 0x03, address 0xffff00
	  FlashCommand(0x103);
	  // write the byte address 
	  WriteData = 0x100 + ((SingleAdd>>8) & 0xff);
	  FlashCommand(WriteData);
	  WriteData = 0x100 + (SingleAdd & 0xff);
	  FlashCommand(WriteData);
	  FlashCommand(0x300); // lower address as 00
	  printf(" write to reg 0xE4: %08x \n", 0x03, SingleAdd);
          FlashMemID = 0;
	  for (iByte = 0; iByte < 127; iByte ++) {
	    for (irepeat =0; irepeat<32; irepeat ++) {
	      usleep(10);
	      read_result = *((uint32_t *) (virt_addr+0xE4) );  // 4-bit 
	      FlashMemID = FlashMemID + (((read_result & 0x20000) >> 17) << (31-irepeat));
	      *((uint32_t *) (virt_addr + 0xE4 )) = 0x1F0; }
	    printf(" Flsah Memory: Word# %02x : %08x \n", iByte, FlashMemID);
	    FlashMemID = 0; }
	  for (irepeat =0; irepeat<30; irepeat ++) {
	    usleep(10);
 	    read_result = *((uint32_t *) (virt_addr+0xE4) );  // 4-bit 
            FlashMemID = FlashMemID + (((read_result & 0x20000) >> 16) << (30-irepeat));
	    *((uint32_t *) (virt_addr + 0xE4 )) = 0x1F0; }
	  usleep(10);
	  read_result = *((uint32_t *) (virt_addr+0xE4) );  // 4-bit 
	  FlashMemID = FlashMemID + ((read_result & 0x20000) >> 16);
	  *((uint32_t *) (virt_addr + 0xE4 )) = 0x0F0; 
	  usleep(10);
	  read_result = *((uint32_t *) (virt_addr+0xE4) );  // 4-bit 
	  FlashMemID = FlashMemID + ((read_result & 0x20000) >> 17);

	  //	  printf(" Read result 0xE4: %08x \n", read_result);
 
          printf(" Flsah Memory: Word# 7f : %08x \n", FlashMemID);
          fflush(stdout);
	  goto ChooseAgain;
	  
        TIstatus:
          printf(" TI register read......\n");
          fflush(stdout);
  	  virt_addr = map_base;  // Register base address
          for (ireg = 0; ireg< 64; ireg++) {
            read_result = *((uint32_t *) (virt_addr+ireg*4) );  //FPGA ready?
            printf(" Reg %02x, Value: %08x \n", ireg*4, read_result);
	  }
	  //          printf(" TrgStart......\n");
          fflush(stdout);
	  goto ChooseAgain;

        SingleRegRead:
          printf("\n Single Register read, enter the address (hex): ");
          scanf("%x", &SingleAdd);
  	  virt_addr = map_base;  // Register base address
          read_result = *((uint32_t *) (virt_addr+(SingleAdd & 0x1ffc)) );  //FPGA ready?
          printf(" Reg %04x, Value: %08x \n", SingleAdd, read_result);
          fflush(stdout);
	  goto ChooseAgain;

        RepeatRegRead:
          printf("\n Repeated Register read, enter the number of repeat: ");
          scanf("%d", &NRepeat);
	  NRepeat = (NRepeat & 0xffff);
          if (NRepeat <= 0) NRepeat = 1;  // limit the repeat to 1 ~ 66535 
          printf("\n enter the address (hex): ");
          scanf("%x", &SingleAdd);
  	  virt_addr = map_base;  // Register base address
          for (irepeat = 0; irepeat < NRepeat; irepeat ++) {
            read_result = *((uint32_t *) (virt_addr+(SingleAdd & 0x1ffc)) );  //FPGA ready?
            printf(" %d of %d: Reg %04x, Value: %08x \n", irepeat, NRepeat, SingleAdd, read_result);
            fflush(stdout);
	    usleep(1000); }
	  goto ChooseAgain;

        SingleRegWrite:
          printf("\n Single Register write, enter the address (hex): ");
	  scanf("%x", &SingleAdd);
          printf("\n Enter the Value (hex): ");
	  scanf("%x", &SingleData);
  	  virt_addr = map_base;  // Register base address
          *((uint32_t *) (virt_addr + (SingleAdd & 0x1ffc) ) ) = (SingleData & 0xffffffff);
          printf(" Write Reg 0x%04x with Value: 0x%08x \n", (SingleAdd &0x1ffc), SingleData);
          fflush(stdout);
	  goto ChooseAgain;


        RepeatRegWrite:
          printf("\n Repeat same Register write, enter the number of repeat: ");
	  scanf("%d", &NRepeat);
	  NRepeat = (NRepeat & 0xffff);
          if (NRepeat <= 0) NRepeat = 1;  // limit the repeat to 1 ~ 66535 
          printf("\n Repeated Register write, enter the address (hex): ");
          scanf("%x", &SingleAdd);
          printf("\n Enter the Value (hex): ");
	  scanf("%x", &SingleData);
  	  virt_addr = map_base;  // Register base address
	  for (irepeat = 0; irepeat < NRepeat; irepeat ++) {
	    *((uint32_t *) (virt_addr + (SingleAdd & 0x1ffc) ) ) = (SingleData & 0xffffffff);
	    printf("%d of %d: Write Reg 0x%04x with Value: 0x%08x \n", irepeat, NRepeat, (SingleAdd &0x1ffc), SingleData);
	    fflush(stdout);
	    usleep(1000); }
	  goto ChooseAgain;

	}

	target = strtoul(argv[2], 0, 0);
	printf("address: 0x%08x\n", (unsigned int)target);

	printf("access type: %s\n", argc >= 4 ? "write" : "read");

	/* data given? */
	if (argc >= 4) {
		printf("access width given.\n");
		access_width = tolower(argv[3][0]);
	}
	printf("access width: ");
	if (access_width == 'b')
		printf("byte (8-bits)\n");
	else if (access_width == 'h')
		printf("half word (16-bits)\n");
	else if (access_width == 'w')
		printf("word (32-bits)\n");
	else {
		printf("word (32-bits)\n");
		access_width = 'w';
	}


	/* calculate the virtual address to be accessed */
	virt_addr = map_base + target;
        virt_addrN = map_base + target + 4;
	/* read only */
	if (argc <= 4) {
		//printf("Read from address %p.\n", virt_addr); 
		switch (access_width) {
		case 'b':
			read_result = *((uint8_t *) virt_addr);
			printf
			    ("Read 8-bits value at address 0x%08x (%p): 0x%02x\n",
			     (unsigned int)target, virt_addr,
			     (unsigned int)read_result);
			break;
		case 'h':
			read_result = *((uint16_t *) virt_addr);
			/* swap 16-bit endianess if host is not little-endian */
			read_result = ltohs(read_result);
			printf
			    ("Read 16-bit value at address 0x%08x (%p): 0x%04x\n",
			     (unsigned int)target, virt_addr,
			     (unsigned int)read_result);
			break;
		case 'w':
			read_result = *((uint32_t *) virt_addr);
			/*			printf
			    ("Read 32-bit value at address 0x%08x (%p): 0x%08x\n",
			     (unsigned int)target, virt_addr,
			     (unsigned int)read_result);
			/* swap 32-bit endianess if host is not little-endian */
			read_result = ltohl(read_result);
			printf
			    ("Read 32-bit value at address 0x%08x (%p): 0x%08x\n",
			     (unsigned int)target, virt_addr,
			     (unsigned int)read_result);
			return (int)read_result;
			break;
		default:
			fprintf(stderr, "Illegal data type '%c'.\n",
				access_width);
			exit(2);
		}
		fflush(stdout);
	}
	/* data value given, i.e. writing? */
	if (argc >= 5) {
		writeval = strtoul(argv[4], 0, 0);
		switch (access_width) {
		case 'b':
			printf("Write 8-bits value 0x%02x to 0x%08x (0x%p)\n",
			       (unsigned int)writeval, (unsigned int)target,
			       virt_addr);
			*((uint8_t *) virt_addr) = writeval;
#if 0
			if (argc > 4) {
				read_result = *((uint8_t *) virt_addr);
				printf("Written 0x%02x; readback 0x%02x\n",
				       writeval, read_result);
			}
#endif
			break;
		case 'h':
			printf("Write 16-bits value 0x%04x to 0x%08x (0x%p)\n",
			       (unsigned int)writeval, (unsigned int)target,
			       virt_addr);
			/* swap 16-bit endianess if host is not little-endian */
			writeval = htols(writeval);
			*((uint16_t *) virt_addr) = writeval;
#if 0
			if (argc > 4) {
				read_result = *((uint16_t *) virt_addr);
				printf("Written 0x%04x; readback 0x%04x\n",
				       writeval, read_result);
			}
#endif
			break;
		case 'w':
			printf("Write 32-bits value 0x%08x to 0x%08x (0x%p)\n",
			       (unsigned int)writeval, (unsigned int)target,
			       virt_addr);
			/* swap 32-bit endianess if host is not little-endian */
			writeval = htoll(writeval);
			*((uint32_t *) virt_addr) = writeval;
#if 0
			if (argc > 4) {
				read_result = *((uint32_t *) virt_addr);
				read_result2 = *((uint32_t *) virt_addrN);
				printf("Written 0x%08x; readback 0x%08x; NextWord 0x%08x \n",
				       writeval, read_result, read_result2);
			}
#endif
			break;
		}
		fflush(stdout);
	}
	if (munmap(map_base, MAP_SIZE) == -1)
		FATAL;
	close(fd);
	return 0;
}
