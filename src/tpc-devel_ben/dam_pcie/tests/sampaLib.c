
/* sampaLib.c */

#ifndef Linux_armv7l

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

//#include "libdam.c"

static int fd = 0;

void
fee_device_set(int device)
{
  fd = device;
  printf("==> set fd=%d\n",fd);
}

int
fee_device_get()
{
  printf("==> fd=%d\n",fd);
  return(fd);
}



/*
def fee_reg_write(fee, addr, data):
    d.fee_request[fee].data_write = data
    d.fee_request[fee].reg_address = addr
    d.fee_request[fee].reg_write = 1
*/
uint32_t
fee_register_write(int fee, uint32_t addr, uint32_t data)
{
  usleep(1000);
  dam_register_write(fd, FEE_REQUEST_DATA[fee], data);
  usleep(1000);
  dam_register_write(fd, FEE_REQUEST_CTRL[fee], addr);
  usleep(1000);
  dam_register_write(fd, FEE_REQUEST_CTRL[fee], addr | 0x10000);
  usleep(1000);
  return(0);
}

/*
def fee_reg_read(fee, addr):
    d.fee_request[fee].reg_address = addr
    d.fee_request[fee].reg_read = 1
    return d.fee_reply[fee].data_read
 */
uint32_t
fee_register_read(int fee, uint32_t addr)
{
  usleep(1000);
  dam_register_write(fd, FEE_REQUEST_CTRL[fee], addr);
  usleep(1000);
  dam_register_write(fd, FEE_REQUEST_CTRL[fee], addr | 0x20000);
  usleep(1000);
  return(dam_register_read(fd, FEE_REPLY_DATA[fee]));
}




/*write to sampa global register*/

/* see page 45
     fee - [3:0] from 0 to 7
     chip - [3:0] from 0 to 7
     addr - [5:0] ??? see table 3.2 (page 53) ???
     data - [7:0]
     ctrl - [6:0]: [6:5]-i2c_opcode, [4]-i2c_en_i, [3:0]-i2c_addr
 */
int
sampa_register_write(uint8_t fee, uint8_t chip, uint8_t addr, uint8_t data)
{
  uint8_t ctrl, opcode=2, enable=1;

  if(fee>7) {printf("fee out of range (%d) - return\n",fee); return(1);}
  if(chip>15) {printf("chip out of range (%d) - return\n",chip); return(1);}
  if(addr>0x3F) {printf("addr out of range (0x%08x) - return\n",addr); return(1);}
  if(data>0xFF) {printf("data out of range (0x%08x) - return\n",data); return(1);}

  ctrl = (opcode<<5) | (enable<<4) | chip; //0x051 for 2nd chip

  fee_register_write (fee, FEE_REG_0601, addr);
  fee_register_write (fee, FEE_REG_0602, data);
  fee_register_write (fee, FEE_REG_0600, ctrl);

  return(0);
}

/*read from sampa global register*/
int
sampa_register_read(uint8_t fee, uint8_t chip, uint8_t addr)
{
  uint8_t ctrl, opcode=3, enable=1;
  uint16_t output;
  int data;

  ctrl = (opcode<<5) | (enable<<4) | chip; //0x051 for 2nd chip

  fee_register_write (fee, FEE_REG_0601, addr);
  fee_register_write (fee, FEE_REG_0600, ctrl);

  /* returns: 0x0[15:10], sampa_i2c_busy[9:9], sampa_i2c_error[8:8], i2c_reg_read_data[7:0] */
  output = fee_register_read (fee, FEE_REG_0600);

  if( (output & 0x300) != 0 )
  {
    data = - (int)((output & 0x300)>>8);
    printf("ERROR in sampa_register_read(), returns %d\n",data);
  }
  else
  {
    data = output & 0xFF;
  }

  return(data);
}





























/******************************/
/* high level sampa functions */
/******************************/

int
sampaGetChipAddress(int fee, int chip)
{
  int ret;
  ret = sampa_register_read(fee, chip, SAMPA_HWADD);
  return(ret);
}

int
sampaGetTriggerCount(int fee, int chip)
{
  int ret;
  uint8_t w[2];
  w[0] = sampa_register_read(fee, chip, SAMPA_TRCNTL);
  w[1] = sampa_register_read(fee, chip, SAMPA_TRCNTH);
  ret = (w[1]<<8) | w[0];
  return(ret);
}

int
sampaGetBunchCrossingCount(int fee, int chip)
{
  int ret;
  uint8_t w[3];
  w[0] = sampa_register_read(fee, chip, SAMPA_BXCNTLL);
  w[1] = sampa_register_read(fee, chip, SAMPA_BXCNTLH);
  w[2] = sampa_register_read(fee, chip, SAMPA_BXCNTHL);
  ret = ((w[2]&0xF)<<16) | (w[1]<<8) | w[0];
  return(ret);
}

int
sampaSetNumberOfPresamples(int fee, int chip, int value)
{
  int ret = 0;
  if(value>191) value = 191;
  if(value<0) value = 0;
  ret = sampa_register_write(fee, chip, SAMPA_PRETRG, value);
  return(ret);
}

int
sampaGetNumberOfPresamples(int fee, int chip)
{
  int ret;
  ret = sampa_register_read(fee, chip, SAMPA_PRETRG);
  return(ret);
}

int
sampaSetNumberOfCyclesInWindow(int fee, int chip, int value)
{
  int ret = 0;
  sampa_register_write (fee, chip, SAMPA_TWLENL, value&0xFF);
  sampa_register_write (fee, chip, SAMPA_TWLENH, (value>>8)&0x3);
  return(ret);
}

int
sampaGetNumberOfCyclesInWindow(int fee, int chip)
{
  int ret;
  uint8_t w[2];

  w[0] = sampa_register_read(fee, chip, SAMPA_TWLENL);
  w[1] = sampa_register_read(fee, chip, SAMPA_TWLENH);
  ret = (w[1]<<8) | w[0];
  return(ret);
}

int
sampaSetNumberOfCyclesBeforeWindow(int fee, int chip, int value)
{
  int ret = 0;
  sampa_register_write (fee, chip, SAMPA_ACQSTARTL, value&0xFF);
  sampa_register_write (fee, chip, SAMPA_ACQSTARTH, (value>>8)&0x3);
  return(ret);
}

int
sampaGetNumberOfCyclesBeforeWindow(int fee, int chip)
{
  int ret;
  uint8_t w[2];

  w[0] = sampa_register_read(fee, chip, SAMPA_ACQSTARTL);
  w[1] = sampa_register_read(fee, chip, SAMPA_ACQSTARTH);
  ret = (w[1]<<8) | w[0];
  return(ret);
}

int
sampaSetNumberOfCyclesElapsed(int fee, int chip, int value)
{
  int ret = 0;
  sampa_register_write (fee, chip, SAMPA_ACQENDL, value&0xFF);
  sampa_register_write (fee, chip, SAMPA_ACQENDH, (value>>8)&0x3);
  return(ret);
}

int
sampaGetNumberOfCyclesElapsed(int fee, int chip)
{
  int ret;
  uint8_t w[2];

  w[0] = sampa_register_read(fee, chip, SAMPA_ACQENDL);
  w[1] = sampa_register_read(fee, chip, SAMPA_ACQENDH);
  ret = (w[1]<<8) | w[0];
  return(ret);
}

/* value bits:
          [0] - Continuous mode enabled (default = 0)
          [1] - Raw data enable (default = 0)
          [2] - Cluster sum enable (default = 0)
          [3] - Huffman enable (default = 0)
          [4] - Enable header generation for empty channels (default = 1)
          [5] - Power save enable (default = 1)
          [6] - Enable automatic clock gating on I2C block (default = 0)
          [7] - Enable clock gating on neighbour block when number of neighbour is 0 (default = 0)
*/
int
sampaSetConfiguration(int fee, int chip, int value)
{
  int ret = 0;
  sampa_register_write (fee, chip, SAMPA_VACFG, value&0xFF);
  return(ret);
}

int
sampaGetConfiguration(int fee, int chip)
{
  int ret;
  ret = sampa_register_read(fee, chip, SAMPA_VACFG);
  return(ret);
}

/* Commands (table 3.3):
          0  (NOP)        No operation
          1  (SWTRG)      Software trigger
          2  (TRCLR)      Clear trigger counter
          3  (ERCLR)      Clear errors
          4  (BXCLR)      Clear bunch crossing counter
          5  (SOFTRST)    Software reset
          6  (LNKSYNC)    Generates sync packet on serial links
          7  (RINGOSCTST) Run ring oscillator test
   NOTE: register returns to 0 after command is executed
*/
int
sampaSetCommand(int fee, int chip, int value)
{
  int ret = 0;
  sampa_register_write (fee, chip, SAMPA_CMD, value&0x7);
  return(ret);
}

int
sampaGetCommand(int fee, int chip)
{
  int ret;
  ret = sampa_register_read(fee, chip, SAMPA_CMD);
  return(ret);
}

/* Neighbor configuration settings:
        [5:0] - Neighbor input delay, ca. 0.2 ns per bit for a total of ca. 12.5 ns
        [7:6] - Number of neighbors
 */
int
sampaSetNeighborDelay(int fee, int chip, int nn_delay)
{
  int ret = 0;
  sampa_register_write (fee, chip, SAMPA_NBCFG, (nn_delay&0xFF));
  return(ret);
}

int
sampaGetNeighborDelay(int fee, int chip)
{
  int ret;
  ret = sampa_register_read(fee, chip, SAMPA_NBCFG);
  return(ret);
}

/* ADC sampling clock delay:
        [5:0] - ADC sampling clock delay, ca. 1.5 ns per bit for a total of ca. 94.5 n
        [6] - Invert ADC sampling clock
 */
int
sampaSetADCSamplingClockDelay(int fee, int chip, int inv_delay)
{
  int ret = 0;
  sampa_register_write (fee, chip, SAMPA_ADCDEL, (inv_delay&0x7F));
  return(ret);
}

int
sampaGetADCSamplingClockDelay(int fee, int chip)
{
  int ret;
  ret = sampa_register_read(fee, chip, SAMPA_ADCDEL);
  return(ret);
}

/* Voltage reference trimming [2:0]
 */
int
sampaSetADCVoltageReferenceTrim(int fee, int chip, int trim)
{
  int ret = 0;
  sampa_register_write (fee, chip, SAMPA_ADCTRIM, (trim&0x7));
  return(ret);
}

int
sampaGetADCVoltageReferenceTrim(int fee, int chip)
{
  int ret;
  ret = sampa_register_read(fee, chip, SAMPA_ADCTRIM);
  return(ret);
}

/* Serial link configuration:
        [3:0] - Number of serial out, 0-11                              (default=4)
        [4] - Disable internal termination of input differential links  (default=1)
        [5] - Enable NBflowstop_in pin                                  (default=1)
 */
int
sampaSetSerialLinkConfig(int fee, int chip, int value)
{
  int ret = 0;
  sampa_register_write (fee, chip, SAMPA_SOCFG, (value&0x3F));
  return(ret);
}

int
sampaGetSerialLinkConfig(int fee, int chip)
{
  int ret;
  ret = sampa_register_read(fee, chip, SAMPA_SOCFG);
  return(ret);
}

/* Serial link drive strength configuration:
        [1:0] - Drive strength of serial out 4-0                        (default=1)
        [3:2] - Drive strength of neighbor flow stop out/serial out 5   (default=1)
        [5:4] - Drive strength of serial out 6,8,10                     (default=1)
        [7:6] - Drive strength of serial out 7,9                        (default=1)
NOTE: see table 3.8:
Drive strength [1:0]  |  Iout mean (mA)  |  Vdiff (mV)  |  Description
       00                     1.95            438          Normal mode
       01                     1.61            348          Low power mode
       10                     2.87            610          High drive strength mode
 */
int
sampaSetSerialLinkDriveConfig(int fee, int chip, int value)
{
  int ret = 0;
  sampa_register_write (fee, chip, SAMPA_SODRVST, (value&0xFF));
  return(ret);
}

int
sampaGetSerialLinkDriveConfig(int fee, int chip)
{
  int ret;
  ret = sampa_register_read(fee, chip, SAMPA_SODRVST);
  return(ret);
}

/* Errors accumulated:
        [4:0] - Correctable header hamming errors
        [7:5] - Uncorrectable header hamming errors
 */
int
sampaGetErrors(int fee, int chip)
{
  int ret;
  ret = sampa_register_read(fee, chip, SAMPA_ERRORS);
  return(ret);
}




/******************************/
/* channel-specific functions */
/******************************/

/*
Pedestal memory read
1. Make sure the data path configuration for the channel to be read from (DPCFG, see table 3.19)
is not using a lookup function f( ), as the lookup function utilizes the pedestal memory and will
cause corrupted reads.
2. Set the address in the pedestal memory that you wish to read from at PMADDL and PMADDH (0x15[7:0] and 0x16[1:0])
3. Put the register address for the channel register PMDATA (0x10) at CHRGADD (0x17[4:0]).
4. Set CHRGCTL[7] (0x1A[7:0]) high to automatically increment the currently set pedestal memory address
(increment before read), set CHRGCTL[6] low for read, set CHRGCTL[4:0] to the channel
that you wish to read from. Broadcast (CHRGCTL[5]) is ignored for reads.
5. The data will appear at CHRGRDATL and CHRGRDATH (0x1B[7:0] and 0x1C[4:0]).

NOTE:Table 3.19: Operating modes of the first Baseline Correction.
     The lookup function f() is the pedestal memory with the argument as the address.

DPCFG[3:0] |  Effect
0x0           din - FPD
0x1           din - f(t)
0x2           din - f(din)
0x3           din - f(din - VPD)
0x4           din - VPD - FPD
0x5           din - VPD - f(t)
0x6           din - VPD - f(din)
0x7           din - VPD - f(din - VPD)
0x8           f(din) - FPD
0x9           f(din - VPD) - FPD
0xA           f(t) - FPD
0xB           f(t) - f(t)
0xC           f(din) - f(din)
0xD           f(din - VPD) - f(din - VPD)
0xE           din - FPD
0xF           din - FPD
*/
int
sampaChannelPedestalRead(uint8_t fee, uint8_t chip, uint8_t chan, uint16_t pedaddr)
{
  uint8_t ctrl, datal, datah;
  int data;

  ctrl = (1<<7) | (0<<6) | (0<<5) | chan; // increment[7] | write[6] | broadcast[5] | chan[4:0]

  sampa_register_write (fee, chip, SAMPA_PMADDL, pedaddr&0xFF); // address in pedestal memory to read from (low 8 bits)
  sampa_register_write (fee, chip, SAMPA_PMADDH, (pedaddr>>8)&0x3); // address in pedestal memory to read from (high 2 bits)
  sampa_register_write (fee, chip, SAMPA_CHRGADD, 0x10); // write channel specific register 0x10 to sampa global register 0x17
  sampa_register_write (fee, chip, SAMPA_CHRGCTL, ctrl); // write configuration scheme to 0x1A

  datal = sampa_register_read (fee, chip, SAMPA_CHRGRDATL); // read data[7:0] from sampa global register 0x1B
  datah = sampa_register_read (fee, chip, SAMPA_CHRGRDATH); // read data[12:8] from sampa global register 0x1C

  data = ((datah<<8) | datal) & 0x1FFF;

  return(data);
}

/*
Pedestal memory write
1. Make sure the data path configuration for the channel to be written to (DPCFG, see table 3.19)
is not using a lookup function f( ), as the lookup function utilizes the pedestal memory and will
cause corrupted writes.
2. Set PMADDL and PMADDH (0x15[7:0] and 0x16[1:0]) (Pedestal Memory ADDress) to the address in the pedestal memory
that you wish to write to.
3. Set CHRGADD (0x17[4:0]) to 0x10 which is the address for PMDATA (Pedestal Memory DATA) in the
channel register.
4. Set the data to write at CHRGWDATL and CHRGWDATH (0x18[7:0] and 0x19[4:0]
5. Set (0x1A) CHRGCTL[7] high to automatically increment the currently set pedestal memory address
(increment before write), set CHRGCTL[6] high for write, set CHRGCTL[5] high if you wish
to write the same value to all channels (broadcasting), set CHRGCTL[4:0] to the channel num-
ber that you wish to write to. When broadcasting the channel number is ignored.
*/
int
sampaChannelPedestalWrite(uint8_t fee, uint8_t chip, uint8_t chan, uint16_t pedaddr, uint16_t data)
{
  uint8_t ctrl;

  ctrl = (1<<7) | (1<<6) | (0<<5) | chan; // increment[7] | write[6] | broadcast[5] | chan[4:0]

  sampa_register_write (fee, chip, SAMPA_PMADDL, pedaddr&0xFF); // address in pedestal memory to read from (low 8 bits)
  sampa_register_write (fee, chip, SAMPA_PMADDH, (pedaddr>>8)&0x3); // address in pedestal memory to read from (high 2 bits)
  sampa_register_write (fee, chip, SAMPA_CHRGADD, 0x10); // write channel specific register 0x10 to sampa global register 0x17
  sampa_register_write (fee, chip, SAMPA_CHRGWDATL, (data&0xFF)); // write data[7:0] to sampa global register 0x18
  sampa_register_write (fee, chip, SAMPA_CHRGWDATH, ((data>>8)&0x1F)); // write data[12:8] to sampa global register 0x19
  sampa_register_write (fee, chip, SAMPA_CHRGCTL, ctrl); // write configuration scheme to 0x1A

  return(0);
}









/*******************************************************/
/* channel specific registers are listed in table 3.15 */

/*The register layout has been designed to minimize the amount of writes needed to update the channel
registers and pedestal memories of each channel. By using the I2C automatic address increment feature,
it is possible to complete all writes needed to update one channel or pedestal address in one continuous
I2C command. An additional broadcast bit can be set to write the same data to all channels so that indi-
vidual access is not needed. When filling the pedestal memory it is possible to set the pedestal memory to
increment on each write avoiding the need to update the two registers each time. Table 3.5 lists the register
needed to access the channel registers.*/

/*write to sampa channel register
  1. Set CHRGADD (0x17[4:0], CHannel ReGister ADDress) to the address of the channel register that you wish to write to.
  2. Set the data to write at CHRGWDATL and CHRGWDATH (0x18[7:0] and 0x19[4:0], CHannel ReGister Write DATa).
  3. Set CHRGCTL[6] (0x1A, CHannel ReGister ConTroL) high for write, set CHRGCTL[5] high if you
     wish to write the same value to all channels (broadcasting), set CHRGCTL[4:0] to the channel
     number that you wish to write the data to. When broadcasting the channel number is ignored.
*/
int
sampaChannelRegisterWrite(uint8_t fee, uint8_t chip, uint8_t chan, uint8_t addr, uint16_t data)
{
  uint8_t ctrl;

  ctrl = (0<<7) | (1<<6) | (0<<5) | chan; // increment[7] | write[6] | broadcast[5] | chan[4:0]

  sampa_register_write (fee, chip, SAMPA_CHRGADD, addr); // write addr to sampa global register 0x17
  sampa_register_write (fee, chip, SAMPA_CHRGWDATL, (data&0xFF)); // write data[7:0] to sampa global register 0x18
  sampa_register_write (fee, chip, SAMPA_CHRGWDATH, ((data>>8)&0x1F)); // write data[12:8] to sampa global register 0x19
  sampa_register_write (fee, chip, SAMPA_CHRGCTL, ctrl); // write configuration scheme to 0x1A

  return(0);
}

/*read from sampa channel register
  1. Set (0x17[4:0]) CHRGADD to the address of the channel register that you wish to read from.
  2. Set (0x1A) CHRGCTL[6] low for read, set CHRGCTL[4:0] to the channel number that you wish to
     read from. Broadcast (CHRGCTL[5]) is ignored for reads.
  3. The data will appear at CHRGRDATL and CHRGRDATH (0x1B[7:0] and 0x1C[4:0], CHannel ReGister Read DATa).
*/
int
sampaChannelRegisterRead(uint8_t fee, uint8_t chip, uint8_t chan, uint8_t addr)
{
  uint8_t ctrl, datal, datah;
  int data;

  ctrl = (0<<7) | (0<<6) | (0<<5) | chan; // increment[7] | write[6] | broadcast[5] | chan[4:0]

  sampa_register_write (fee, chip, SAMPA_CHRGADD, addr); // write addr to sampa global register 0x17
  sampa_register_write (fee, chip, SAMPA_CHRGCTL, ctrl); // write configuration scheme to 0x1A
  datal = sampa_register_read (fee, chip, SAMPA_CHRGRDATL); // read data[7:0] from sampa global register 0x1B
  datah = sampa_register_read (fee, chip, SAMPA_CHRGRDATH); // read data[12:8] from sampa global register 0x1C

  data = (datah<<8) | datal;

  return(data);
}



/* baseline handling:
3.2.3 BC1
3.2.4 BC2
3.2.5 BC3
3.2.6 Zero suppression
 */

/*
ZSTHR (Zero-Suppression THReshold) Signals below this value will be suppressed and not included in
the data stream. The value is given with 0.25 bits precision. Setting this value to zero effectively
disables zero suppression as long as no filters are enabled which can bring the sample value below
zero.
*/
int
sampaChannelZeroSuppressionThresholdWrite(uint8_t fee, uint8_t chip, uint8_t chan, uint16_t data)
{
  sampaChannelRegisterWrite(fee, chip, chan, SAMPA_CHANNEL_ZSTHR, data&0xFFF);
  return(0);
}
int
sampaChannelZeroSuppressionThresholdRead(uint8_t fee, uint8_t chip, uint8_t chan)
{
  int data;
  data = sampaChannelRegisterRead(fee, chip, chan, SAMPA_CHANNEL_ZSTHR);
  return(data);
}

/*
ZSOFF (Zero-Suppression OFFset) Since data is transmitted as 10 bits unsigned, there is a necessity to
truncate the data to only positive values. To avoid losing information, like the tale of pulses that can
pass below the average, it is possible to add an offset to all values before truncation. As this happens
before the zero-suppression threshold decision, any offset added here needs to be taken into account
when setting the threshold.
*/
int
sampaChannelZeroSuppressionOffsetWrite(uint8_t fee, uint8_t chip, uint8_t chan, uint16_t data)
{
  sampaChannelRegisterWrite(fee, chip, chan, SAMPA_CHANNEL_ZSOFF, data&0x1FFF);
  return(0);
}
int
sampaChannelZeroSuppressionOffsetRead(uint8_t fee, uint8_t chip, uint8_t chan)
{
  int data;
  data = sampaChannelRegisterRead(fee, chip, chan, SAMPA_CHANNEL_ZSOFF);
  return(data);
}

/*ZSCFG (Zero-Suppression ConFiGuration)
[1:0] Glitch filtering setting to remove spurious signals above zero-suppression threshold. With this
set to 4 , then pulses that are 3 samples or shorter will be remove. If it is 3, then pulses that are
2 samples or shorter will be removed. With this set to 2, then single samples above threshold
will be removed. Setting it to 0 will accept all pulses.
[4:2] This sets the number of samples before the signal passes above the threshold that should be
included. (Samples before pulse starts)
[6:5] This sets the number of samples after the signal has passed below the threshold that should also
be included. (Samples after pulse ends)
[7] Change position of BC3 in pipeline (BC3 after BC2)
[8] This enables sending of samples that have not been modified by the BC2 or BC3 filter, but the
zero-suppression threshold decision is still done on the filtered baseline.
*/
int
sampaChannelZeroSuppressionConfigWrite(uint8_t fee, uint8_t chip, uint8_t chan, uint16_t data)
{
  sampaChannelRegisterWrite(fee, chip, chan, SAMPA_CHANNEL_ZSCFG, data&0x1FF);
  return(0);
}
int
sampaChannelZeroSuppressionConfigRead(uint8_t fee, uint8_t chip, uint8_t chan)
{
  int data;
  data = sampaChannelRegisterRead(fee, chip, chan, SAMPA_CHANNEL_ZSCFG);
  return(data);
}


















/* Bypass inputs to serial 0 [3:0]
NOTE: see table 3.13:
 */
int
sampaSetBypass(int fee, int chip, int value)
{
  int ret = 0;
  sampa_register_write (fee, chip, SAMPA_BYPASS, (value&0xF));
  return(ret);
}

int
sampaGetBypass(int fee, int chip)
{
  int ret;
  ret = sampa_register_read(fee, chip, SAMPA_BYPASS);
  return(ret);
}

/* Channel select for ADC test serializer mode in bypass [4:0]
 */
int
sampaSetBypassChannels(int fee, int chip, int value)
{
  int ret = 0;
  sampa_register_write (fee, chip, SAMPA_SERCHSEL, (value&0x1F));
  return(ret);
}

int
sampaGetBypassChannels(int fee, int chip)
{
  int ret;
  ret = sampa_register_read(fee, chip, SAMPA_SERCHSEL);
  return(ret);
}

/* Ring oscillator counter difference from reference ADC clock [7:0] */
int
sampaGetRingCounter(int fee, int chip)
{
  int ret;
  ret = sampa_register_read(fee, chip, SAMPA_RINGCNT);
  return(ret);
}

/* Clock configuration pin status [6:0] */
int
sampaGetClockConfig(int fee, int chip)
{
  int ret;
  ret = sampa_register_read(fee, chip, SAMPA_CLKCONF);
  return(ret);
}

/* Status of differential input pins:
        [0] - NBflowstop_in
        [1] - DinN
        [2] - hb_trg
        [3] - trg
        [4] - bx_sync_trg
 */
int
sampaGetBoundary(int fee, int chip)
{
  int ret;
  ret = sampa_register_read(fee, chip, SAMPA_BOUNDARY);
  return(ret);
}

/* Enable channels in 8-channel groups:
    group 0 - channels 0-7
    group 1 - channels 8-15
    group 2 - channels 16-23
    group 3 - channels 24-31
    mask - [7:0]
 */
int
sampaSetChannelGroupEnableMask(int fee, int chip, int group, int mask)
{
  int ret = 0;

  if(group==0)      sampa_register_write (fee, chip, SAMPA_CHEN0, (mask&0xFF));
  else if(group==1) sampa_register_write (fee, chip, SAMPA_CHEN1, (mask&0xFF));
  else if(group==2) sampa_register_write (fee, chip, SAMPA_CHEN2, (mask&0xFF));
  else if(group==3) sampa_register_write (fee, chip, SAMPA_CHEN3, (mask&0xFF));
  else {printf("ERROR: wrong group=%d\n",group);}

  return(ret);
}

int
sampaGetChannelGroupEnableMask(int fee, int chip, int group)
{
  int ret;
  if(group==0)      ret = sampa_register_read(fee, chip, SAMPA_CHEN0);
  else if(group==1) ret = sampa_register_read(fee, chip, SAMPA_CHEN1);
  else if(group==2) ret = sampa_register_read(fee, chip, SAMPA_CHEN2);
  else if(group==3) ret = sampa_register_read(fee, chip, SAMPA_CHEN3);
  else {printf("ERROR: wrong group=%d\n",group);}

  return(ret);
}




/* ROL-level calls */

static int fee = 7;

/*set the number of cycles for time window*/
int
sampaSetTimeWindowWidth(int ncycles_in_window)
{
  int chip;
  uint16_t window = ncycles_in_window;

  printf("Setting TimeWindowWidth = %d samples\n",window);

  for(chip=0; chip<=7; chip++)
  {
    sampaSetNumberOfCyclesInWindow(fee, chip, window);
  }

  return(0);
}

/*get the number of cycles in time window*/
int
sampaGetTimeWindowWidth()
{
  int chip;
  uint16_t window, window_tmp;

  chip = 0;
  window = sampaGetNumberOfCyclesInWindow(fee, chip);

  for(chip=1; chip<=7; chip++)
  {
    window_tmp = sampaGetNumberOfCyclesInWindow(fee, chip);
    if(window!=window_tmp) printf("[chip=%2d] ERROR: window=0x%03x is different from chip0's window=0x%03x\n",chip,window_tmp,window);
  }

  printf("Got TimeWindowWidth = %d samples\n",window);

  return(window);
}


/*set the number of pre-samples for time window*/
int
sampaSetTimeWindowOffset(int npresamples)
{
  int chip;
  uint16_t offset = npresamples;

  printf("Setting TimeWindowOffset = %d samples\n",offset);

  for(chip=0; chip<=7; chip++)
  {
    sampaSetNumberOfPresamples(fee, chip, offset);
  }

  return(0);
}

/*get the number of pre-samples in time window*/
int
sampaGetTimeWindowOffset()
{
  int chip;
  uint16_t offset, offset_tmp;

  chip = 0;
  offset = sampaGetNumberOfPresamples(fee, chip);

  for(chip=1; chip<=7; chip++)
  {
    offset_tmp = sampaGetNumberOfPresamples(fee, chip);
    if(offset!=offset_tmp) printf("[chip=%2d] ERROR: offset=0x%02x is different from chip0's offset=0x%02x\n",chip,offset_tmp,offset);
  }

  printf("Got TimeWindowOffset = %d samples\n",offset);

  return(offset);
}




int
sampaReadBlock(uint32_t *data, int MAXWORDS)
{
  int len, nw, status, ii, word;

  len = 0;
  //printf("Working with fee [%d]\n",fee);
  status = dam_register_read(fd, REG_EVT_FIFO_STATUS[fee]); /*read status register*/
  printf("[%d] status=%08X\n",fee,status);
  nw = status & 0x7FFF; /*length*/
  if(nw>MAXWORDS) printf("ERROR: (nw=%d)>(MAXWORDS=%d)\n",nw,MAXWORDS);
  else if(nw<=0) printf("ERROR: nw=%d -> event not ready to read\n",nw);
  else 
  {
    //printf("fee=%d, nw=%d\n",fee,nw);
    for(ii=0; ii<nw; ii++)
    {
      word = dam_register_read(fd, REG_EVT_FIFO_DATA[fee]); /*read event word*/
      status = dam_register_read(fd, REG_EVT_FIFO_STATUS[fee]); /*read status register*/
      //printf("  word[%6d] = 0x%08x, status = 0x%08x\n",ii,word, status);
      data[len++] = word;
      dam_register_write(fd, REG_EVT_FIFO_RD[fee], 1); /*strob*/
    }
  }

  if(len != nw) printf("ERROR: nw=%d != len=%d\n",nw,len);

  return(len);
}

int
sampaPrintBlock(uint32_t *data, int nw)
{
  int len, ii, jj;

  ii=0;
  while(ii<nw)
  {
    //printf("[%5d] 0x%04x (%5d)\n",ii,data[ii],data[ii]);
    len = data[ii];
    printf("len=%d\n",len);
    for(jj=0; jj<len; jj++)
    {



      printf("[%2d] data=0x%04x (%5d) - ",jj,data[ii],data[ii]);
      if(jj==0) printf("the number of words (inclusive)\n");
      else if(jj==1) {printf("chip = %d, chan = %d\n",/*data[ii]&0x7,(data[ii]>>3)&0x1f*/(data[ii]>>5)&0xF,(data[ii])&0x1f);}
      else if(jj==2) printf("bx_counter [10:1] = %d\n",data[ii]);
      else if(jj==3) printf("data parity[?] = %d, bx[19:11] = %d\n",(data[ii]<<9)&0x1,data[ii]&0x1FF);
      else if(jj==4) printf("data start marker\n");
      else if(jj==5) printf("the number of samples (+ 1, exclusive) = %d\n",data[ii]);
      else if(jj==6) printf("??? time of pulse in window ???\n");
      else if(jj>6 && jj<(len-1)) printf("adc sample\n");
      else if(jj==(len-1)) {printf("??? trailer ???\n");}
      else printf("?\n");
      ii++;




#if 0
      printf("[%2d] data=0x%04x (%5d) - ",jj,data[ii],data[ii]);
      if(jj==0) printf("the number of words (inclusive)\n");
      else if(jj==1) printf("chip = %d, chan = %d\n",/*data[ii]&0x7,(data[ii]>>3)&0x1f*/(data[ii]>>5)&0xF,(data[ii])&0x1f);
      else if(jj==5) printf("the number of samples (+ 1, exclusive) = %d\n",data[ii]);
      else if(jj==6) printf("??? time of pulse in window ???\n");
      else if(jj>6 && jj<(len-1)) printf("adc sample\n");
      else if(jj==(len-1)) printf("??? trailer ???\n");
      else printf("?\n");
      ii++;
#endif


    }
  }

  return(0);
}

/* initialidation; returns 'fd', or <=0 for error */
int
sampaInit()
{
  int model, map, fee_mask, ii, ret;
  int chip, value, cts, cg0, cg1, device;

  dam_open(&device, "/dev/dam0");
  fee_device_set(device);

  sleep(1);
  printf("fd=%d\n",fd);
  sleep(1);

  model = dam_register_read(fd, CARD_TYPE);
  map   = dam_register_read(fd, REG_MAP_VERSION);
  printf("FLX Model: %i, Reg Map: %i\n", model, map);

  if(map<=0) return(0);

  dam_register_write(fd, 0x900, 1<<fee); 
  fee_mask = dam_register_read(fd, 0x900);
  printf("stream_en fee mask = 0x%08X\n", fee_mask);

  /*
  dam_register_write(fd, CR_INTERNAL_LOOPBACK_MODE, 1);
  */

  fee_register_write(fee, 0x100, 1234);
  usleep(1000);
  printf("Test reading 0x000: %d\n",fee_register_read(fee, 0x0));
  usleep(1000);
  printf("Test reading 0x100: %d\n",fee_register_read(fee, 0x100));


  
  printf("%d\n",fee_register_read(fee, 0x0));
  printf("%d\n",fee_register_read(fee, 0x1));
  printf("%d\n",fee_register_read(fee, 0xa000));
  printf("%d\n",fee_register_read(fee, 0xa001));
  printf("%d\n",fee_register_read(fee, 0xa002));
  printf("%d\n",fee_register_read(fee, 0xa003));



  /*
write 0x10 - event fifo reset
write 0x8 - gth reset (FEE links/transiever)
write 0x4 - lmk reset (pll)
write 0x2 - hw/vio reset (hardware)
  */


  /*
  usleep(1000);
  dam_register_write(fd, PHY_RESET, 0x1C);
  usleep(1000);
  dam_register_write(fd, PHY_RESET, 0x18);
  usleep(1000);
  dam_register_write(fd, PHY_RESET, 0x10);
  usleep(1000);
  dam_register_write(fd, PHY_RESET, 0x0);
  usleep(1000);
  */

  /*Ben:
Normally do in this order:
assert/release vio_reset
assert/release lmk reset
assert/release gth reset
  */

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






  usleep(1000);
  fee_register_write(fee, 0x200, 0xffff); /*enable elinks from SAMPA 4-7*/
  usleep(1000);
  fee_register_write(fee, 0x201, 0xffff); /*enable elinks from SAMPA 0-3*/
  usleep(1000);

  /*change gain/shaping time*/
  cts=1;
  cg0=1; // 8:40am aug 27 2023 change from 0 to 1
  cg1=1;
  value = (cts<<2) | (cg1<<1) | cg0;
  fee_register_write(fee, 0x300, value);
  usleep(1000);
  value = fee_register_read(fee, 0x300);
  usleep(1000);
  cg0 = value&0x1;
  cg1 = (value>>1)&0x1;
  cts = (value>>2)&0x1;
  printf("\n");
  if( cts==1 && cg0==1 && cg1==1 )      printf("   Gain is 30 mV/fC, Shaping Time is 160 ns\n");
  else if( cts==1 && cg0==0 && cg1==1 ) printf("   Gain is 20 mV/fC, Shaping Time is 160 ns\n");
  else if( cts==0 && cg0==0 && cg1==1 ) printf("   Gain is 30 mV/fC, Shaping Time is 80 ns\n");
  else if( cts==0 && cg0==0 && cg1==0 ) printf("   Gain is 20 mV/fC, Shaping Time is 80 ns\n");
  else                                  printf("   Unknown Gain / Shaping Time configuration: value=0x%04x\n",value);
  printf("\n");

  for(chip=0; chip<=7; chip++)
  {
    value = 0x30; /*default is 0x30, 0x31 for 'continuous' mode, 0x32 for 'raw' mode*/
    sampaSetConfiguration(fee, chip, value);
    value = sampaGetConfiguration(fee, chip);
    printf("[chip=%1d] Config=0x%02x\n",chip,value);
  }



#if 0
  for(chip=0; chip<=7; chip++)
  {
    value = 0x34; //default is 0x34
    sampaSetSerialLinkConfig(fee, chip, value);
  }
#endif

  /*
  for(chip=0; chip<=7; chip++)
  {
    value = sampaGetSerialLinkConfig(fee, chip);
    printf("[chip=%1d] SerialLinkConfig=0x%02x\n",chip,value);
    value = sampaGetSerialLinkDriveConfig(fee, chip);
    printf("[chip=%1d] SerialLinkDriveConfig=0x%02x\n",chip,value);
  }
  */

  /* set time window width */
  sampaSetTimeWindowWidth(16); /*do NOT exceed 56 !!!*/
  sampaGetTimeWindowWidth();

  /* set time window offset */
  sampaSetTimeWindowOffset(0); /*do NOT exceed 191 !!!*/
  sampaGetTimeWindowOffset();




  /*enable/disable chips (DO NOT DISABLE ANYTHING, DOES NOT WORK !!!)
  for(chip=0; chip<=7; chip++)
  {
    for(ii=0; ii<4; ii++)
    {
      ret = sampaSetChannelGroupEnableMask(fee, chip, ii, 0xFF);
    }
  }
*/

  /*
  for(chip=0; chip<=7; chip++)
  {
    for(ii=0; ii<4; ii++)
    {
      ret = sampaGetChannelGroupEnableMask(fee, chip, ii);
      printf("[chip=%1d] [group=%1d] -> channel mask = 0x%02x\n",chip,ii,ret);
    }
  }
  */

  return(fd);
}





#else

void
sampaLib_dummy()
{
  ;
}

#endif
