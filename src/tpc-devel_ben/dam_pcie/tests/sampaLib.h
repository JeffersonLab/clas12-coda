
/* sampaLib.h */



/* SAMPA GLOBAL REGISTERS (table 3.2) */

#define SAMPA_HWADD     0x00  /* R   [3:0]  Chip address (hardware address) */
#define SAMPA_TRCNTL    0x01  /* R   [7:0]  Trigger count, lower byte */
#define SAMPA_TRCNTH    0x02  /* R   [7:0]  Trigger count, upper byte */
#define SAMPA_BXCNTLL   0x03  /* R   [7:0]  Bunch crossing count, lower byte */
#define SAMPA_BXCNTLH   0x04  /* R   [7:0]  Bunch crossing count, mid byte */
#define SAMPA_BXCNTHL   0x05  /* R   [3:0]  Bunch crossing count, upper byte */
#define SAMPA_PRETRG    0x06  /* RW  [7:0]  Number of pre-samples (Pre-trigger delay), max 192 */
#define SAMPA_TWLENL    0x07  /* RW  [7:0]  Number of cycles for time window +1, lower byte */
#define SAMPA_TWLENH    0x08  /* RW  [1:0]  Number of cycles for time window +1, upper byte */
#define SAMPA_ACQSTARTL 0x09  /* RW  [7:0]  Number of cycles to wait before acquisition starts, lower byte */
#define SAMPA_ACQSTARTH 0x0A  /* RW  [1:0]  Number of cycles to wait before acquisition starts, upper byte */
#define SAMPA_ACQENDL   0x0B  /* RW  [7:0]  Number of cycles elapsed from trigger to acquisition end +1, lower byte */
#define SAMPA_ACQENDH   0x0C  /* RW  [1:0]  Number of cycles elapsed from trigger to acquisition end +1, upper byte */
#define SAMPA_VACFG     0x0D  /* RW  [7:0]  Various configuration settings: */
                              /*               [0] Continuous mode enabled */
                              /*               [1] Raw data enable */
                              /*               [2] Cluster sum enable */
                              /*               [3] Huffman enable */
                              /*               [4] Enable header generation for empty channels */
                              /*               [5] Power save enable */
                              /*               [6] Enable automatic clock gating on I2C block */
                              /*               [7] Enable clock gating on neighbour block when number of neighbour is 0 */
#define SAMPA_CMD       0x0E  /* RW  [2:0]  Commands, see table 3.3 */
#define SAMPA_NBCFG     0x0F  /* RW  [7:0]  Neighbor configuration settings: */
                              /*               [5:0] Neighbor input delay, ca. 0.2 ns per bit for a total of ca. 12.5 ns */
                              /*               [7:6] Number of neighbors */
#define SAMPA_ADCDEL    0x10  /* RW  [6:0]  ADC sampling clock delay: */
                              /*               [5:0] ADC sampling clock delay, ca. 1.5 ns per bit for a total of ca. 94.5 ns */
                              /*               [6]   Invert ADC sampling clock */
#define SAMPA_ADCTRIM   0x11  /* RW  [2:0]  Voltage reference trimming */
#define SAMPA_SOCFG     0x12  /* RW  [5:0]  Serial link configuration: */
                              /*               [3:0] Number of serial out, 0-11 */
                              /*               [4]   Disable internal termination of input differential links */
                              /*               [5]   Enable NBflowstop_in pin */
#define SAMPA_SODRVST   0x13  /* RW  [7:0]  Serial link drive strength configuration, see table 3.8: */
                              /*               [1:0] Drive strength of serial out 4-0 */
                              /*               [3:2] Drive strength of neighbor flow stop out/serial out 5 */
                              /*               [5:4] Drive strength of serial out 6,8,10 */
                              /*               [7:6] Drive strength of serial out 7,9 */
#define SAMPA_ERRORS    0x14  /* R   [7:0]  Errors accumulated: */
                              /*               [4:0] Correctable header hamming errors */
                              /*               [7:5] Uncorrectable header hamming errors */
#define SAMPA_PMADDL    0x15  /* RW  [7:0]  Pedestal memory address, lower byte */
#define SAMPA_PMADDH    0x16  /* RW  [1:0]  Pedestal memory address, upper byte */
#define SAMPA_CHRGADD   0x17  /* RW  [4:0]  Channel register address */
#define SAMPA_CHRGWDATL 0x18  /* RW  [7:0]  Channel register write data, lower byte */
#define SAMPA_CHRGWDATH 0x19  /* RW  [4:0]  Channel register write data, upper byte */
#define SAMPA_CHRGCTL   0x1A  /* RW  [7:0]  Channel register control: */
                              /*               [4:0] Channel number */
                              /*               [5]   Broadcast to all channels (channel number ignored) */
                              /*               [6]   Write, not read from register address (returns to read after write) */
                              /*               [7]   Increment PMADD (returns automatically to zero) */
#define SAMPA_CHRGRDATL 0x1B  /* R   [7:0]  Channel register read data, lower byte */
#define SAMPA_CHRGRDATH 0x1C  /* R   [4:0]  Channel register read data, upper byte */
#define SAMPA_CHORDAT   0x1D  /* RW  [4:0]  Channel readout order data */
#define SAMPA_CHORDCTL  0x1E  /* RW  [5:0]  Channel readout order control: */
                              /*               [4:0] Position in order */
                              /*               [5]   Write enable */
#define SAMPA_BYPASS    0x1F  /* RW  [3:0]  Bypass inputs to serial 0, see table 3.13 */
#define SAMPA_SERCHSEL  0x20  /* RW  [4:0]  Channel select for ADC test serializer mode in bypass */
#define SAMPA_RINGCNT   0x21  /* R   [7:0]  Ring oscillator counter difference from reference ADC clock */
#define SAMPA_CLKCONF   0x22  /* R   [6:0]  Clock configuration pin status */
#define SAMPA_BOUNDARY  0x23  /* R   [4:0]  Status of differential input pins: */
                              /*               [0] NBflowstop_in */
                              /*               [1] DinN */
                              /*               [2] hb_trg */
                              /*               [3] trg */
                              /*               [5] bx_sync_trg */
#define SAMPA_CHEN0     0x24  /* RW  [7:0]  Channel enable 7-0 */
#define SAMPA_CHEN1     0x25  /* RW  [7:0]  Channel enable 15-8 */
#define SAMPA_CHEN2     0x26  /* RW  [7:0]  Channel enable 23-16 */
#define SAMPA_CHEN3     0x27  /* RW  [7:0]  Channel enable 31-24 */



/* SAMPA CHANNEL REGISTERS (table 3.15) */

#define SAMPA_CHANNEL_K1         0x00  /* RW  [12:0]  First pole of the TCFU */
#define SAMPA_CHANNEL_K2         0x01  /* RW  [12:0]  Second pole of the TCFU */
#define SAMPA_CHANNEL_K3         0x02  /* RW  [12:0]  Third pole of the TCFU */
#define SAMPA_CHANNEL_K4         0x03  /* RW  [12:0]  Fourth pole of the TCFU */
#define SAMPA_CHANNEL_L1         0x04  /* RW  [12:0]  First zero of the TCFU */
#define SAMPA_CHANNEL_L2         0x05  /* RW  [12:0]  Second zero of the TCFU */
#define SAMPA_CHANNEL_L3         0x06  /* RW  [12:0]  Third zero of the TCFU */
#define SAMPA_CHANNEL_L4         0x07  /* RW  [12:0]  Fourth zero of the TCFU */
#define SAMPA_CHANNEL_L30        0x08  /* RW  [12:0]  TCFU IIR SOS first zero(L3) gain */
#define SAMPA_CHANNEL_ZSTHR      0x09  /* RW  [11:0]  Zero suppression threshold, 2 bit precision */
#define SAMPA_CHANNEL_ZSOFF      0x0A  /* RW  [12:0]  Offset added before truncation, 2’s compliment, 2 bit precision */
#define SAMPA_CHANNEL_ZSCFG      0x0B  /* RW  [8:0]   Zero suppression configuration: */
                                       /*                [1:0] Glitch filter, minimum accepted pulse, all, >1, >2, >2 */
                                       /*                [4:2] Post-samples */
                                       /*                [6:5] Pre-samples */
                                       /*                [7]   Change position of BC3 in pipeline (BC3 after BC2) */
                                       /*                [8]   Enable Raw data output of ZSU */
#define SAMPA_CHANNEL_FPD        0x0C  /* RW  [12:0]  BC1 Fixed pedestal (offset subtracted), 2’s compliment, 2 bit precision */
#define SAMPA_CHANNEL_VPD        0x0D  /* R   [12:0]  BC1 variable pedestal, 2’s compliment */
#define SAMPA_CHANNEL_BC2BSL     0x0E  /* R   [12:0]  BC2 Computed Baseline, 2’s compliment */
#define SAMPA_CHANNEL_BC3BSL     0x0F  /* R   [12:0]  BC3 Computed Baseline, 2’s compliment */
#define SAMPA_CHANNEL_PMDATA     0x10  /* RW  [9:0]   Data to be stored or read from the pedestal memory */
#define SAMPA_CHANNEL_BC2LTHRREL 0x11  /* RW  [9:0]   BC2 lower relative threshold */
#define SAMPA_CHANNEL_BC2HTHRREL 0x12  /* RW  [9:0]   BC2 higher relative threshold */
#define SAMPA_CHANNEL_BC2LTHRBSL 0x13  /* RW  [10:0]  BC2 lower saturation level for baseline */
#define SAMPA_CHANNEL_BC2HTHRBSL 0x14  /* RW  [10:0]  BC2 higher saturation level for baseline */
#define SAMPA_CHANNEL_BC2CFG     0x15  /* RW  [10:0]  BC2 configuration: */
                                       /*                [1:0]  Number of taps in moving average filter */
                                       /*                [3:2]  BC2 pre-samples */
                                       /*                [7:4]  BC2 post-samples */
                                       /*                [8]    BC2 glitch removal */
                                       /*                [10:9] Auto reset configuration */
#define SAMPA_CHANNEL_BC2RSTVAL  0x16  /* RW  [7:0]   Reset value for maf baseline when auto reset is enabled */
#define SAMPA_CHANNEL_BC2RSTCNT  0x17  /* RW  [7:0]   Number of samples outside of thresholds before resetting maf filter (divided by 4) */
#define SAMPA_CHANNEL_DPCFG      0x18  /* RW  [11:0]  Data path configuration: */
                                       /*                [3:0]  BC1 mode, see table 3.19 */
                                       /*                [4]    BC1 data input polarity */
                                       /*                [5]    BC1 pedestal memory polarity */
                                       /*                [6]    BC1 pedestal memory record from input */
                                       /*                [7]    TCFU enabled */
                                       /*                [8]    BC2 moving average filter enable */
                                       /*                [9]    BC3 filter enable */
                                       /*                [10]   TCFU SOS Architecture enable */
                                       /*                [11]   TCFU signed Pole/Zero enable */
#define SAMPA_CHANNEL_BC1THRL    0x19  /* RW  [10:0]  Lower threshold of variable pedestal filter, 2’s compliment */
#define SAMPA_CHANNEL_BC1THRH    0x1A  /* RW  [10:0]  Higher threshold of variable pedestal filter, 2’s compliment */
#define SAMPA_CHANNEL_BC1CFG     0x1B  /* RW  [9:0]   BC1 configuration: */
                                       /*                [3:0]  Number of taps in variable pedestal filter */
                                       /*                [4]    Define open threshold time of 31(high)/15(low) samples after (auto)reset */
                                       /*                [5]    Force enable IIR also inside time window */
                                       /*                [6]    High if BC1THR should be considered absolute, else relative */
                                       /*                [8:7]  Shift output data of pedestal memory */
                                       /*                [9]    BC1 negative clipping enabled */
#define SAMPA_CHANNEL_BC1RSTCNT  0x1C  /* RW  [7:0]   Number of samples outside of thresholds before resetting vpd filter (divided by 4) */
				       /*             0 disables the auto reset */
#define SAMPA_CHANNEL_BC3SLD     0x1D  /* RW  [7:0]   Rate of the BC3 baseline down counter */
#define SAMPA_CHANNEL_BC3SLU     0x1E  /* RW  [7:0]   Rate of the BC3 baseline up counter */



/* defines */

#define NFEE 8

static int REG_EVT_FIFO_STATUS[NFEE] = {
  REG_EVT_FIFO_STATUS_0,
  REG_EVT_FIFO_STATUS_1,
  REG_EVT_FIFO_STATUS_2,
  REG_EVT_FIFO_STATUS_3,
  REG_EVT_FIFO_STATUS_4,
  REG_EVT_FIFO_STATUS_5,
  REG_EVT_FIFO_STATUS_6,
  REG_EVT_FIFO_STATUS_7
};

static int REG_EVT_FIFO_DATA[NFEE] = {
  REG_EVT_FIFO_DATA_0,
  REG_EVT_FIFO_DATA_1,
  REG_EVT_FIFO_DATA_2,
  REG_EVT_FIFO_DATA_3,
  REG_EVT_FIFO_DATA_4,
  REG_EVT_FIFO_DATA_5,
  REG_EVT_FIFO_DATA_6,
  REG_EVT_FIFO_DATA_7
};

static int REG_EVT_FIFO_RD[NFEE] = {
  REG_EVT_FIFO_RD_0,
  REG_EVT_FIFO_RD_1,
  REG_EVT_FIFO_RD_2,
  REG_EVT_FIFO_RD_3,
  REG_EVT_FIFO_RD_4,
  REG_EVT_FIFO_RD_5,
  REG_EVT_FIFO_RD_6,
  REG_EVT_FIFO_RD_7
};


static int FEE_REQUEST_DATA[NFEE] = {
  FEE_REQUEST_DATA_0,
  FEE_REQUEST_DATA_1,
  FEE_REQUEST_DATA_2,
  FEE_REQUEST_DATA_3,
  FEE_REQUEST_DATA_4,
  FEE_REQUEST_DATA_5,
  FEE_REQUEST_DATA_6,
  FEE_REQUEST_DATA_7
};

static int FEE_REQUEST_CTRL[NFEE] = {
  FEE_REQUEST_CTRL_0,
  FEE_REQUEST_CTRL_1,
  FEE_REQUEST_CTRL_2,
  FEE_REQUEST_CTRL_3,
  FEE_REQUEST_CTRL_4,
  FEE_REQUEST_CTRL_5,
  FEE_REQUEST_CTRL_6,
  FEE_REQUEST_CTRL_7
};

static int FEE_REPLY_DATA[NFEE] = {
  FEE_REPLY_DATA_0,
  FEE_REPLY_DATA_1,
  FEE_REPLY_DATA_2,
  FEE_REPLY_DATA_3,
  FEE_REPLY_DATA_4,
  FEE_REPLY_DATA_5,
  FEE_REPLY_DATA_6,
  FEE_REPLY_DATA_7
};

/*not used
static int FEE_REPLY_CTRL[NFEE] = {
  FEE_REPLY_CTRL_0,
  FEE_REPLY_CTRL_1,
  FEE_REPLY_CTRL_2,
  FEE_REPLY_CTRL_3,
  FEE_REPLY_CTRL_4,
  FEE_REPLY_CTRL_5,
  FEE_REPLY_CTRL_6,
  FEE_REPLY_CTRL_7
};
*/


/* routines */

void     fee_device_set(int device);
int      fee_device_get();
uint32_t fee_register_write(int fee, uint32_t addr, uint32_t data);
uint32_t fee_register_read(int fee, uint32_t addr);
int sampa_register_write(uint8_t fee, uint8_t chip, uint8_t addr, uint8_t data);
int sampa_register_read(uint8_t fee, uint8_t chip, uint8_t addr);
int sampaGetChipAddress(int fee, int chip);
int sampaGetTriggerCount(int fee, int chip);
int sampaGetBunchCrossingCount(int fee, int chip);
int sampaSetNumberOfPresamples(int fee, int chip, int value);
int sampaGetNumberOfPresamples(int fee, int chip);
int sampaSetNumberOfCyclesInWindow(int fee, int chip, int value);
int sampaGetNumberOfCyclesInWindow(int fee, int chip);
int sampaSetNumberOfCyclesBeforeWindow(int fee, int chip, int value);
int sampaGetNumberOfCyclesBeforeWindow(int fee, int chip);
int sampaSetNumberOfCyclesElapsed(int fee, int chip, int value);
int sampaGetNumberOfCyclesElapsed(int fee, int chip);
int sampaSetConfiguration(int fee, int chip, int value);
int sampaGetConfiguration(int fee, int chip);
int sampaSetCommand(int fee, int chip, int value);
int sampaGetCommand(int fee, int chip);
int sampaSetNeighborDelay(int fee, int chip, int nn_delay);
int sampaGetNeighborDelay(int fee, int chip);
int sampaSetADCSamplingClockDelay(int fee, int chip, int inv_delay);
int sampaGetADCSamplingClockDelay(int fee, int chip);
int sampaSetADCVoltageReferenceTrim(int fee, int chip, int trim);
int sampaGetADCVoltageReferenceTrim(int fee, int chip);
int sampaSetSerialLinkConfig(int fee, int chip, int value);
int sampaGetSerialLinkConfig(int fee, int chip);
int sampaSetSerialLinkDriveConfig(int fee, int chip, int value);
int sampaGetSerialLinkDriveConfig(int fee, int chip);
int sampaGetErrors(int fee, int chip);
int sampaChannelPedestalRead(uint8_t fee, uint8_t chip, uint8_t chan, uint16_t pedaddr);
int sampaChannelPedestalWrite(uint8_t fee, uint8_t chip, uint8_t chan, uint16_t pedaddr, uint16_t data);
int sampaChannelRegisterWrite(uint8_t fee, uint8_t chip, uint8_t chan, uint8_t addr, uint16_t data);
int sampaChannelRegisterRead(uint8_t fee, uint8_t chip, uint8_t chan, uint8_t addr);
int sampaChannelZeroSuppressionThresholdWrite(uint8_t fee, uint8_t chip, uint8_t chan, uint16_t data);
int sampaChannelZeroSuppressionThresholdRead(uint8_t fee, uint8_t chip, uint8_t chan);
int sampaChannelZeroSuppressionOffsetWrite(uint8_t fee, uint8_t chip, uint8_t chan, uint16_t data);
int sampaChannelZeroSuppressionOffsetRead(uint8_t fee, uint8_t chip, uint8_t chan);
int sampaChannelZeroSuppressionConfigWrite(uint8_t fee, uint8_t chip, uint8_t chan, uint16_t data);
int sampaChannelZeroSuppressionConfigRead(uint8_t fee, uint8_t chip, uint8_t chan);
int sampaSetBypass(int fee, int chip, int value);
int sampaGetBypass(int fee, int chip);
int sampaSetBypassChannels(int fee, int chip, int value);
int sampaGetBypassChannels(int fee, int chip);
int sampaGetRingCounter(int fee, int chip);
int sampaGetClockConfig(int fee, int chip);
int sampaGetBoundary(int fee, int chip);
int sampaSetChannelGroupEnableMask(int fee, int chip, int group, int mask);
int sampaGetChannelGroupEnableMask(int fee, int chip, int group);
int sampaInit();
int sampaSetTimeWindowWidth(int ncycles_in_window);
int sampaGetTimeWindowWidth();
int sampaSetTimeWindowOffset(int npresamples);
int sampaGetTimeWindowOffset();
int sampaReadBlock(uint32_t *data, int MAXWORDS);
int sampaPrintBlock(uint32_t *data, int nw);
