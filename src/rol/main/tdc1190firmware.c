
/* tdc1190firmware.c */

/*USAGE: must be on the VME controller, type:

cd $CLON_PARMS/firmwares

   * Slot  3:  tdc1190firmware 0x11180000 v1190core0.6.rbf
   * Slot  4:  tdc1190firmware 0x11200000 v1190core0.6.rbf
   * Slot  5:  tdc1190firmware 0x11280000 v1190core0.6.rbf
   * Slot  6:  tdc1190firmware 0x11300000 v1190core0.6.rbf
   * Slot  7:  tdc1190firmware 0x11380000 v1190core0.6.rbf
   * Slot  8:  tdc1190firmware 0x11400000 v1190core0.6.rbf
   * Slot  9:  tdc1190firmware 0x11480000 v1190core0.6.rbf
   * Slot 10:  tdc1190firmware 0x11500000 v1190core0.6.rbf
   * Slot 11:  tdc1190firmware 0x11580000 v1190core0.6.rbf
   * Slot 12:  tdc1190firmware 0x11600000 v1190core0.6.rbf
   * Slot 13:  tdc1190firmware 0x11680000 v1190core0.6.rbf
   * Slot 14:  tdc1190firmware 0x11700000 v1190core0.6.rbf
   * Slot 15:  tdc1190firmware 0x11780000 v1190core0.6.rbf
   * Slot 16:  tdc1190firmware 0x11800000 v1190core0.6.rbf
   * Slot 17:  tdc1190firmware 0x11880000 v1190core0.6.rbf
   * Slot 18:  tdc1190firmware 0x11900000 v1190core0.6.rbf
   * Slot 19:  tdc1190firmware 0x11980000 v1190core0.6.rbf
   * Slot 20:  tdc1190firmware 0x11A00000 v1190core0.6.rbf

   * Slot 16:  tdc1190firmware 0x11800000 v1190core_1.1.rbf
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef Linux_vme

#include "jvme.h"
#include "tdc1190.h"

/*sergey*/
#define EIEIO 
#define SYNC 

/* Parameters for the access to the Flash Memory */


#define FIRST_PAGE_STD    768     /* first page of the copy STD */
#define FIRST_PAGE_BCK    1408   /* first page of the copy BCK */


#define PAGE_SIZE       264 /* Number of bytes per page in the target flash */

/* flash memory Opcodes */
#define MAIN_MEM_PAGE_READ          0x00D2
#define MAIN_MEM_PAGE_PROG_TH_BUF1  0x0082


/****************************************************************************
 write_flash_page
    flag=0 for USER flash (default)
        =1 for VME flash
****************************************************************************/
int
write_flash_page2(unsigned int addr, unsigned char *page, int pagenum)
{
  volatile TDC1190 *tdc1190 = (TDC1190 *) addr;
  int i, flash_addr;
  unsigned char addr0, addr1, addr2;
  int res = 0;
  unsigned short data16;
  unsigned short *Sel_Flash; /* VME Address of the FLASH SELECTION REGISTER */
  unsigned short *RW_Flash;  /* VME Address of the FLASH Read/Write REGISTER */

  Sel_Flash = (short *)&(tdc1190->selflash/*USER*/);
  RW_Flash = (short *)&(tdc1190->flash/*USER*/);

  EIEIO;
  SYNC;
  flash_addr = pagenum << 9;
  addr0 = (unsigned char)flash_addr;
  addr1 = (unsigned char)(flash_addr>>8);
  addr2 = (unsigned char)(flash_addr>>16);

  EIEIO;
  SYNC;
  /* enable flash (NCS = 0) */
  data16 = 0;
  vmeWrite16(Sel_Flash,data16); /* *Sel_Flash = data; */

  EIEIO;
  SYNC;
  /* write opcode */
  data16 = MAIN_MEM_PAGE_PROG_TH_BUF1;
  vmeWrite16(RW_Flash,data16); /* *RW_Flash = data; */

  EIEIO;
  SYNC;
  /* write address */
  data16 = addr2;
  vmeWrite16(RW_Flash,data16); /* *RW_Flash = data; */
  data16 = addr1;
  vmeWrite16(RW_Flash,data16); /* *RW_Flash = data; */
  data16 = addr0;
  vmeWrite16(RW_Flash,data16); /* *RW_Flash = data; */

  EIEIO;
  SYNC;
  /* write flash page */
  for(i=0; i<PAGE_SIZE; i++)
  {
    data16 = page[i];
    vmeWrite16(RW_Flash,data16); /* *RW_Flash = data; */
  }

  EIEIO;
  SYNC;
  /* wait 20ms (max time required by the flash to complete the writing) */
  taskDelay(1/*10*/); /* 1 tick = 10ms */

  EIEIO;
  SYNC;
  /* disable flash (NCS = 1) */
  data16 = 1;
  vmeWrite16(Sel_Flash,data16); /* *Sel_Flash = data; */


  EIEIO;
  SYNC;
  /* wait 20ms (max time required by the flash to complete the writing) */
  taskDelay(2/*20*/);
  EIEIO;
  SYNC;

  return(res);
}

/****************************************************************************
 read_flash_page
****************************************************************************/
int
read_flash_page2(unsigned int addr, unsigned char *page, int pagenum)
{
  volatile TDC1190 *tdc1190 = (TDC1190 *) addr;
  int i, flash_addr;
  unsigned char addr0,addr1,addr2;
  int res = 0;
  unsigned short data16;
  unsigned short *Sel_Flash; /* VME Address of the FLASH SELECTION REGISTER */
  unsigned short *RW_Flash;  /* VME Address of the FLASH Read/Write REGISTER */

  Sel_Flash = (short *)&(tdc1190->selflash);
  RW_Flash = (short *)&(tdc1190->flash);

  EIEIO;
  SYNC;
  flash_addr = pagenum << 9;
  addr0 = (unsigned char)flash_addr;
  addr1 = (unsigned char)(flash_addr>>8);
  addr2 = (unsigned char)(flash_addr>>16);

  EIEIO;
  SYNC;
  /* enable flash (NCS = 0) */
  data16 = 0;
  vmeWrite16(Sel_Flash,data16); /* *Sel_Flash = data; */


  EIEIO;
  SYNC;
  /* write opcode */
  data16 = MAIN_MEM_PAGE_READ;
  vmeWrite16(RW_Flash,data16); /* *RW_Flash = data; */



  EIEIO;
  SYNC;
  /* write address */
  data16 = addr2;
  vmeWrite16(RW_Flash,data16); /* *RW_Flash = data; */
  data16 = addr1;
  vmeWrite16(RW_Flash,data16); /* *RW_Flash = data; */
  data16 = addr0;
  vmeWrite16(RW_Flash,data16); /* *RW_Flash = data; */


  EIEIO;
  SYNC;
  /* additional don't care bytes */
  data16 = 0;
  for(i=0; i<4; i++)
  {
    vmeWrite16(RW_Flash,data16); /* *RW_Flash = data; */
  }





  printf("->RW_Flash=0x%08x\n",RW_Flash); /* does not read correctly without print ??!! */

  EIEIO;
  SYNC;
  /* read flash page */
  for(i=0; i<PAGE_SIZE; i++)
  {
    data16 = vmeRead16(RW_Flash); /* data16 = *RW_Flash; */
    page[i] = (unsigned char)data16;
    /*printf("page[%3d]=0x%04x\n",i,page[i]);*/
  }
  EIEIO;
  SYNC;






  /* disable flash (NCS = 1) */
  data16 = 1;
  vmeWrite16(Sel_Flash,data16); /* *Sel_Flash = data; */
  EIEIO;
  SYNC;

  return(res);
}


int
tdc1190firmware_test1(unsigned int baseaddr)
{
  unsigned char pdr[264];

  read_flash_page2(baseaddr, pdr, FIRST_PAGE_STD);  /* read page */
}


/*****************************************************************************
   MAIN

     baseaddr: full board address (for example 0x80510000)
     filename: RBF file name
     page: =0 for standard, =1 for backup
     user_vme: Firmware to update selector = 0 => USER, 1 => VME

*****************************************************************************/
int
tdc1190firmware(unsigned int baseaddr, char *filename, int page, int user_vme)
{
  unsigned short *reload = (unsigned short *) (baseaddr+0x8016);
  int finish,i;
  int bp, bcnt, pa;
  char c;
  unsigned char pdw[PAGE_SIZE], pdr[PAGE_SIZE];
  unsigned long vboard_base_address;
  FILE *cf;

  page = 0;     /* ONLY STD !!!!!!!!!!!!! */
  user_vme = 0; /* ONLY USER !!!!!!!!!!!! */

  printf("\n");
  printf("********************************************************\n");
  printf("* CAEN SpA - Front-End Division                        *\n");
  printf("* ---------------------------------------------------- *\n");
  printf("* Firmware Upgrade of the TDC1190                      *\n");
  printf("* Version 1.1 (27/07/06)                               *\n");
  printf("*   Sergey Boyarinov: CLAS version 23-Apr-2007         *\n");
  printf("********************************************************\n\n");

  /* open the configuration file */
  cf = fopen(filename,"rb");
  if(cf==NULL)
  {
    printf("\n\nCan't open tdc1190 firmware file >%s< - exit\n",filename);
    exit(0);
  }

  if(user_vme == 0) /* FPGA "User" */
  {
    if(page == 0)
    {
      pa = FIRST_PAGE_STD/*USR_FIRST_PAGE_STD*/;
    }
    else if(page == 1)
    {
      printf("Backup image not supported for USER FPGA\n");
      exit(0);
	}
    else
    {
      printf("Bad Image.\n");
	  exit(0);
    }

    printf("Updating firmware of the FPGA USER with the file >%s<\n",filename);
  }
  else if(user_vme == 1) /* FPGA "VME_Interface" */
  {
    if(page == 0)
    {
      printf("Writing STD page of the VME FPGA\n");
      /*pa = VME_FIRST_PAGE_STD;*/
      exit(0);
	}
    else if(page == 1)
    {
      printf("Writing BCK page of the VME FPGA\n");
      /*pa = VME_FIRST_PAGE_BCK;*/
      exit(0);
	}
    else
    {
      printf("Bad Image.\n");
      exit(0);
	}

    printf("Updating firmware of the FPGA VME with the file %s\n", filename);
  }
  else
  {
    printf("Bad FPGA Target.\n");
	exit(0);
  }






  bcnt = 0; /* byte counter */
  bp = 0;   /* byte pointer in the page */
  finish = 0;

  /* while loop */
  while(!finish)
  {
    c = (unsigned char) fgetc(cf); /* read one byte from file */

    /* mirror byte (lsb becomes msb) */
    pdw[bp] = 0;
    for(i=0; i<8; i++)
    {
      if(c & (1<<i))
	  {
        pdw[bp] = pdw[bp] | (0x80>>i);
	  }
	}

    bp++;
    bcnt++;
    if(feof(cf))
    {
      printf("End of file: bp=%d bcnt=%d\n",bp,bcnt);fflush(stdout);
      finish = 1;
    }

    /* write and verify a page */
    if((bp == PAGE_SIZE) || finish)
    {
      write_flash_page2(baseaddr, pdw, pa);
      read_flash_page2(baseaddr, pdr, pa);
      for(i=0; i<PAGE_SIZE; i++)
      {
        if(pdr[i] != pdw[i])
        {
          printf("[%3d] written 0x%02x, read back 0x%02x",i,pdw[i],pdr[i]);
          printf(" -> Flash writing failure (byte %d of page %d)!\n",i,pa);
		  /*
          printf("\nFirmware not loaded !\n");
          exit(0);
		  */
        }
        /*else
		{
          printf("GOOD: [%3d] written 0x%02x, read back 0x%02x\n",i,pdw[i],pdr[i]);
		}*/
	  }
      bp=0;
      pa++;
    }
  } /* end of while loop */

  fclose(cf);
  printf("\nFirmware loaded successfully. Written %d bytes\n", bcnt);

  /* reload new firmware for USER only */
  if(user_vme == 0)
  {
    printf("Activating updated version of the User FPGA, should be running now\n");
    vmeWrite16(reload,1); /* *reload = 1; */
  }
  else
  {
    printf("Write 1 at address 0x8016 to reload updated version of the User FPGA\n");
  }

  exit(0);
}








int
main(int argc, char *argv[])
{
  int res;
  char myname[256];
  unsigned int addr, laddr;

  if(argc==3)
  {
    addr = strtol(argv[1], (char **)NULL, 16);
    printf("use argument >0x%08x< as board VME address\n",addr);

    strncpy(myname, argv[2], 255);
    printf("use argument >%s< as rbf file name\n",myname);
  }
  else
  {
    printf("Usage: tdc1190firmware <vme_address> <rbf file>\n");
    exit(0);
  }

  /* Open the default VME windows */
  vmeOpenDefaultWindows();

  /* get address in A24 space */
  res = vmeBusToLocalAdrs(0x09,(char *)addr,(char **)&laddr);
  if (res != 0) 
  {
	printf("ERROR in vmeBusToLocalAdrs(0x39,0x%x,&laddr) \n",addr);
  }
  else
  {
    printf("INFO: addr=0x%08x, laddr=0x%08x\n",addr,laddr);

	/*tdc1190firmware_test1(laddr);*/

    /*tdc1190Init(0x11100000,0x80000,20,0);*/

    tdc1190firmware(laddr,myname,0,0);
  }

  exit(0);
}

#else

int
main()
{
  return(0);
}

#endif
