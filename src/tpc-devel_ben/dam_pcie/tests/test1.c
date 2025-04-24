
/* test1.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "libdam.h"
#include "regs_map.h"
#include "sampaLib.h"

extern int usleep(__useconds_t __useconds);

static int fd = 0;

#define NFEE 8

/* load driver after machine reboot
insmod /usr/clas12/release/1.4.0/coda/src/tpc-devel_ben/dam_pcie/src/dam_pcie.ko
*/

#define MAXDATA 30000

int
main(void)
{
  int ii, jj, status, nw, len;
  int word;
  uint32_t data[MAXDATA];
  uint8_t chip;
  uint8_t chan;
  uint16_t addr, pedaddr;
  //size_t size = (sysconf(_SC_PAGESIZE) * 256 * 3);
  //uint32_t *buff = NULL;
  int fee = 7;

#if 1
  fd = sampaInit();
  if(fd <= 0)
  {
    printf("ERROR in sampaInit() - exit\n");
    exit(0);
  }

  //exit(0);

#else

  dam_open(&fd, "/dev/dam0");
  fee_device_set(fd);

  
  dam_register_write(fd, 0x900, 1<<fee); 
  

  /*
  usleep(1000);
  dam_register_write(fd, PHY_RESET, 0x2);
  usleep(1000);
  dam_register_write(fd, PHY_RESET, 0x0);
  
  usleep(1000);
  dam_register_write(fd, PHY_RESET, 0x4);
  usleep(1000);
  dam_register_write(fd, PHY_RESET, 0x0);
  
  usleep(1000);
  dam_register_write(fd, PHY_RESET, 0x8);
  usleep(1000);
  dam_register_write(fd, PHY_RESET, 0x0);
  
  usleep(1000);
  dam_register_write(fd, PHY_RESET, 0x10);
  usleep(1000);
  dam_register_write(fd, PHY_RESET, 0x0);
  */




  /*  
  usleep(1000);
  fee_register_write(fee, 0x200, 0xffff);
  usleep(1000);
  fee_register_write(fee, 0x201, 0xffff);
  usleep(1000);
  */

  usleep(1000);
  fee_register_write(fee, 0x201, 0x0001);
  usleep(1000);

#endif



  /*
  for(chip=0; chip<=7; chip++)
  {
    int trim = 0;
    sampaSetADCVoltageReferenceTrim(fee, chip, trim);
  }
  sleep(1);

   for(chip=0; chip<=7; chip++)
  {
    int trim;
    trim = sampaGetADCVoltageReferenceTrim(fee, chip);
    printf("Chip=%d, Trim = %d\n",chip,trim);    
  }
  sleep(1);
  */


  sampaSetTimeWindowWidth(16);



  /*****************************/
  /*test global sampa registers*/
#if 1
  for(chip=0; chip<=7; chip++)
  {
    //uint8_t w[2];
    //uint16_t window;
    printf("Chip=%d, Chip address (hardware address) = %d\n",chip,sampaGetChipAddress(fee,chip));
  }
#endif



#if 0
  /*pedestal write*/
  for(chip=0; chip<8; chip++)
  {
    chan=0x20;//for(chan=0; chan<32; chan++) // 0x20 - broadcast to all channels
    {
      pedaddr = 0;
      word = 0x51;
      sampaChannelPedestalWrite(fee, chip, chan, pedaddr, word);
      //usleep(10);
    }
  }
#endif

#if 0
  /*pedestal read*/
  for(chip=0; chip<8; chip++)
  {
    for(chan=0; chan<32; chan++)
    {
      pedaddr = 0;
      word = sampaChannelPedestalRead(fee, chip, chan, pedaddr);
      //usleep(10);
      printf("[chip=%1d][chan=%2d] -> ped=0x%03x (%3d)\n",chip,chan,word,word);
    }
  }

  //exit(0);
#endif



#if 0
  /*channel register read*/
  chip = 0;
  chan = 0;
  for(chip=0; chip<=7; chip++)
  {
    addr = SAMPA_CHANNEL_BC2LTHRBSL;
    word = sampaChannelRegisterRead(fee, chip, chan, addr);

    if(word<0) printf("[chip=%2d] error = %d\n",chip,word);
    else       printf("[chip=%2d] word=0x%02x\n",chip,word);
  }
  exit(0);
#endif




#if 0
  /* set time window width */
  sampaSetTimeWindowWidth(56);
  sampaGetTimeWindowWidth();
#endif



#if 1

  fee_register_write(fee, 0xa001, 0x1); /*soft trigger*/
  usleep(10000);

  // fifo reset ??
  //dam_register_write(fd, PHY_RESET, 0x10);
  //usleep(100000);
  //sleep(1);

  nw = sampaReadBlock(data, MAXDATA);
  if(nw > 0) sampaPrintBlock(data, nw);

#endif


#if 0
  chip = 0;
  word = sampaGetTriggerCount(fee, chip);
  printf("trigger count = %d\n",word);
#endif



















#if 0
  /*for DMA only ?*/
  if (ioctl(fd, DEBUG_DMA, NULL) == -1)
  {
    perror(__func__);
    return -1;
  }
  dam_register_write(fd, DMA_TEST, 0xc0da);
  usleep(10000);
  printf("DMA_TEST: %x\n", dam_register_read(fd, DMA_TEST));
  if (ioctl(fd, DEBUG_DMA, NULL) == -1)
  {
    perror(__func__);
    return -1;
  }
  /*for DMA only ?*/
#endif




  dam_close(fd);

  return(0);
}



/*

d = dam()
fee = int(sys.argv[1])

print(fee)

d.reg_write(CR_INTERNAL_LOOPBACK_MODE, 1);
d.reg_write(DMA_TEST, 4096);
time.sleep(2);

for i in range(0, 8):
    print(i, hex(fee_reg_read(i, 0x200)));

fee_reg_write(fee, 0x200, 0xffff)
time.sleep(1)
fee_reg_write(fee, 0x201, 0)

print(fee, hex(fee_reg_read(fee, 0x0)));
print(fee, hex(fee_reg_read(fee, 0x1)));
print(fee, hex(fee_reg_read(fee, 0xa000)));
print(fee, hex(fee_reg_read(fee, 0xa001)));
print(fee, hex(fee_reg_read(fee, 0xa002)));
print(fee, hex(fee_reg_read(fee, 0xa003)));

print("dma_status=", d.dump_dma_status())

fee_reg_write(fee, 0xa001, 0x1)
time.sleep(1)

for i in range(0, 10):
    print(i, ": dma_status=", d.dump_dma_status())
    data = d.read(256)
    print(data)

*/
