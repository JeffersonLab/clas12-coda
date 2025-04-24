
/* regs_map.h */


/* DAM registers map */

#define  REG_MAP_VERSION            0x0000 /**/
#define  BOARD_ID_TIMESTAMP         0x0010 /**/
#define  GIT_COMMIT_TIME            0x0020 /**/
#define  GIT_TAG                    0x0030 /**/
#define  GIT_COMMIT_NUMBER          0x0040 /**/
#define  GIT_HASH                   0x0050 /**/
#define  STATUS_LEDS                0x0060 /**/
#define  DMA_BUSY_STATUS            0x0070 /* 0x1 - tohost_busy, 0xe - tohost_busy_latched, 0x10 - enable */
#define  INT_TEST                   0x0800 /* 0xf - irq*/
#define  GENERIC_CONSTANTS          0x0810 /* 0xff - descriptors, 0xff00 - interrupts */
#define  NUM_OF_CHANNELS            0x0820 /**/
#define  CARD_TYPE                  0x0830 /**/
#define  OPTO_TRX_NUM               0x0840 /**/
#define  CR_INTERNAL_LOOPBACK_MODE  0x0850 /**/
#define  DEBUG_MODE                 0x0860 /**/
#define  FIRMWARE_MODE              0x0870 /**/
#define  GTREFCLK_SOURCE            0x0880 /**/
#define  BLOCKSIZE                  0x0890 /**/
#define  PCIE_ENDPOINT              0x08A0 /**/
#define  SCRATCH_PAD                0x08B0 /**/
#define  DMA_TEST                   0x08C0 /* 0xffff - length */
#define  LMK_LOCKED                 0x08D0 /**/
#define  PHY_RESET                  0x08E0 /**/
#define  SI5345_PLL                 0x08F0 /* 0x1 - i2c_busy, 0x2 - nLOL, 0x4 - program*/
#define  FEE_STREAM_ENABLE          0x0900 /**/
#define  DMA_PACKET_CHUNK_SIZE      0x0910 /**/
#define  DMA_FIFO_FULL_COUNT        0x0920 /**/

#define  FEE_REQUEST                0xA000 /*16 32bit words*/
#define  FEE_REQUEST_DATA_0           0xA000 /*32bit: data_write*/
#define  FEE_REQUEST_CTRL_0           0xA004 /*32bit: 0xffff-reg_address, 0x10000-reg_write, 0x20000-reg_read*/
#define  FEE_REQUEST_DATA_1           0xA010
#define  FEE_REQUEST_CTRL_1           0xA014
#define  FEE_REQUEST_DATA_2           0xA020
#define  FEE_REQUEST_CTRL_2           0xA024
#define  FEE_REQUEST_DATA_3           0xA030
#define  FEE_REQUEST_CTRL_3           0xA034
#define  FEE_REQUEST_DATA_4           0xA040
#define  FEE_REQUEST_CTRL_4           0xA044
#define  FEE_REQUEST_DATA_5           0xA050
#define  FEE_REQUEST_CTRL_5           0xA054
#define  FEE_REQUEST_DATA_6           0xA060
#define  FEE_REQUEST_CTRL_6           0xA064
#define  FEE_REQUEST_DATA_7           0xA070
#define  FEE_REQUEST_CTRL_7           0xA074

#define  FEE_REPLY                  0xB000 /*16 32bit words*/
#define  FEE_REPLY_DATA_0             0xB000 /*32bit: data_read*/
#define  FEE_REPLY_CTRL_0             0xB004 /*32bit: 0xffff-reg_address, 0x10000-reg_fail, 0x20000-reg_ack, 0x40000-rx_ready*/
#define  FEE_REPLY_DATA_1             0xB010
#define  FEE_REPLY_CTRL_1             0xB014
#define  FEE_REPLY_DATA_2             0xB020
#define  FEE_REPLY_CTRL_2             0xB024
#define  FEE_REPLY_DATA_3             0xB030
#define  FEE_REPLY_CTRL_3             0xB034
#define  FEE_REPLY_DATA_4             0xB040
#define  FEE_REPLY_CTRL_4             0xB044
#define  FEE_REPLY_DATA_5             0xB050
#define  FEE_REPLY_CTRL_5             0xB054
#define  FEE_REPLY_DATA_6             0xB060
#define  FEE_REPLY_CTRL_6             0xB064
#define  FEE_REPLY_DATA_7             0xB070
#define  FEE_REPLY_CTRL_7             0xB074

#define  TX_LOCKED                  0xC000 /**/
#define  RX_LOCKED                  0xC010 /**/
#define  GTM_RECV_CLOCK_FREQ        0xC020 /**/
#define  GTM_BCO                    0xC030 /**/
#define  FEE_LINK_COUNTERS          0xD000 /*MAYBE: first 32bit - rx_eob, second 32bit - rx_sob*/
                                           /*or/and: 0xffff - control_data, 0xffff0000 - stream_data, 0xffff00000000-fifo_full*/

#define  EVT_STAT                   0xE000 /*16 32bit words*/
#define    REG_EVT_FIFO_STATUS_0      0xE000  
#define    REG_EVT_FIFO_DATA_0        0xE010 
#define    REG_EVT_FIFO_STATUS_1      0xE020  
#define    REG_EVT_FIFO_DATA_1        0xE030 
#define    REG_EVT_FIFO_STATUS_2      0xE040  
#define    REG_EVT_FIFO_DATA_2        0xE050 
#define    REG_EVT_FIFO_STATUS_3      0xE060  
#define    REG_EVT_FIFO_DATA_3        0xE070 
#define    REG_EVT_FIFO_STATUS_4      0xE080  
#define    REG_EVT_FIFO_DATA_4        0xE090 
#define    REG_EVT_FIFO_STATUS_5      0xE0A0  
#define    REG_EVT_FIFO_DATA_5        0xE0B0 
#define    REG_EVT_FIFO_STATUS_6      0xE0C0  
#define    REG_EVT_FIFO_DATA_6        0xE0D0 
#define    REG_EVT_FIFO_STATUS_7      0xE0E0  
#define    REG_EVT_FIFO_DATA_7        0xE0F0 


#define  EVT_CTRL                   0xF000 /*8 32bit words*/
#define    REG_EVT_FIFO_RD_0          0xF000
#define    REG_EVT_FIFO_RD_1          0xF010
#define    REG_EVT_FIFO_RD_2          0xF020
#define    REG_EVT_FIFO_RD_3          0xF030
#define    REG_EVT_FIFO_RD_4          0xF040
#define    REG_EVT_FIFO_RD_5          0xF050
#define    REG_EVT_FIFO_RD_6          0xF060
#define    REG_EVT_FIFO_RD_7          0xF070



/* FEE registers map */

#define FEE_REG_0200 0x0200
#define FEE_REG_0201 0x0201
#define FEE_REG_0300 0x0300
#define FEE_REG_0600 0x0600
#define FEE_REG_0601 0x0601
#define FEE_REG_0602 0x0602

