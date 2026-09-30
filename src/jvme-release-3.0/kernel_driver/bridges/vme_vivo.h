/*
 * vivo.h
 *
 * Support for the Abaco Systems Vivo VME Bridge chip
 *
 * Copyright 2017 Abaco Systems, Inc.
 *
 * Based on work by Tom Armistead and Ajit Prem
 * Copyright 2004 Motorola Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef VIVO_H
#define VIVO_H

#ifndef	PCI_VENDOR_ID_ABACO
#define	PCI_VENDOR_ID_ABACO 0x1D92
#endif

#ifndef	PCI_DEVICE_ID_ABACO_VIVO
#define	PCI_DEVICE_ID_ABACO_VIVO 0x0070
#endif

/*
 *  Define the number of each that the Vivo supports.
 */
#define VIVO_MAX_MASTER			8	/* Max Master Windows */
#define VIVO_MAX_SLAVE			8	/* Max Slave Windows */
#define VIVO_MAX_DMA			2	/* Max DMA Controllers */
#define VIVO_MAX_MAILBOX		4	/* Max Mail Box registers */
#define VIVO_MAX_SEMAPHORE		4	/* Max Semaphores */

/*
 * VME Address Modifiers
 */
#define VIVO_VME_ADDR_MOD_A32UMB	0x08	/* A32 nonpriv multi blk xfer */
#define VIVO_VME_ADDR_MOD_A32UD		0x09	/* A32 nonpriv data access */
#define VIVO_VME_ADDR_MOD_A32UP		0x0A	/* A32 nonpriv program access */
#define VIVO_VME_ADDR_MOD_A32UB		0x0B	/* A32 nonpriv blk xfer */
#define VIVO_VME_ADDR_MOD_A32SMB	0x0C	/* A32 super multi blk xfer */
#define VIVO_VME_ADDR_MOD_A32SD		0x0D	/* A32 super data access */
#define VIVO_VME_ADDR_MOD_A32SP		0x0E	/* A32 superv program access */
#define VIVO_VME_ADDR_MOD_A32SB		0x0F	/* A32 superv blk xfer */
#define VIVO_VME_ADDR_MOD_2eVME_6U	0x20	/* 2eVME for 6U bus modules */
#define VIVO_VME_ADDR_MOD_2eVME_3U	0x21	/* 2eVME for 3U bus modules */
#define VIVO_VME_ADDR_MOD_A16U		0x29	/* A16 nonprivileged access */
#define VIVO_VME_ADDR_MOD_A16S		0x2D	/* A16 supervisory access */
#define VIVO_VME_ADDR_MOD_CR_CSR	0x2F	/* Conf ROM/Ctl&stat reg acc */
#define VIVO_VME_ADDR_MOD_A24UMB	0x38	/* A24 nonpriv multi blk xfer */
#define VIVO_VME_ADDR_MOD_A24UD		0x39	/* A24 nonpriv data access */
#define VIVO_VME_ADDR_MOD_A24UP		0x3A	/* A24 nonpriv program access */
#define VIVO_VME_ADDR_MOD_A24UB		0x3B	/* A24 nonpriv blk xfer */
#define VIVO_VME_ADDR_MOD_A24SMB	0x3C	/* A24 super multi blk xfer */
#define VIVO_VME_ADDR_MOD_A24SD		0x3D	/* A24 super data access */
#define VIVO_VME_ADDR_MOD_A24SP		0x3E	/* A24 super program access */
#define VIVO_VME_ADDR_MOD_A24SB		0x3F	/* A24 super blk xfer */

/* Structure used to hold driver specific information */
struct vivo_driver
{
  void __iomem *base;		/* Base Address of device registers */
  wait_queue_head_t dma_queue[2];
  wait_queue_head_t iack_queue;
  struct mutex vme_int;		/*
				 * Only one VME interrupt can be
				 * generated at a time, provide locking
				 */
  u64 vme_reg_addr;
  u64 vme_reg_size;
  struct resource pci_resource;
  u32 iack_count;
  u32 dma_int_count[2];		/* Interrupt count for DMA channel */
};

/*
 * Layout of a Scatter Gather Mode Linked-List Descriptor
 *
 * Note: This structure is accessed via the chip and therefore must be
 *       correctly laid out - It must also be aligned on 64-byte boundaries.
 */
struct vivo_dma_descriptor
{
  u32 sa;			/* Source Address */
  u32 sau;			/* Source Address Upper */
  u32 da;			/* Destination Address */
  u32 dau;			/* Destination Address Upper */
  u32 satt;			/* Source Attributes */
  u32 datt;			/* Destination Attributes */
  u32 ndesc;			/* Next Descriptor Address */
  u32 ndescu;			/* Next Descriptor Address Upper */
  u32 bss;			/* Destination Broadcast Slave Select */
  u32 tl;			/* Transaction Length */
  u32 pad44;			/* Descriptors must be on 64-byte bounds */
  u32 pad48;
  u32 pad52;
  u32 pad56;
  u32 pad60;
  u32 pad64;
};

struct vivo_dma_entry
{
  /*
   * The descriptor needs to be aligned on a 64-byte boundary, we increase
   * the chance of this by putting it first in the structure.
   */
  struct vivo_dma_descriptor desc;
  struct vivo_dma_descriptor u_desc;	/* unmapped descriptor */
  struct list_head list;
  dma_addr_t dma_handle;
  struct vivo_dma_entry *kmalloc_address;
};

#define VIVO_DMA_DESCRIPTOR_ALIGN ((u64)0x040)

/*****************************************************************************
 *	Total register block size for VIVO
 *****************************************************************************/
#define	VIVO_BLK_SIZE		((u32)0x00100000)

/*****************************************************************************
 *	Register descriptions for PCI ID
 *****************************************************************************/
#define VIVO_PCI_ID                0x70000

/*****************************************************************************
 *	Base address/size for CRCSR
 *****************************************************************************/
#define	VIVO_CRCSR_GRP_BASE	((u32)0x0007f800)
#define	VIVO_CRCSR_GRP_SIZE	((u32)0x00000800)

/*****************************************************************************
 *	Register descriptions for INT_EBL
 *	(Interrupt Enable Register)
 *****************************************************************************/
#define VIVO_INT_EBL                0x7f800
#define VIVO_INT_EBL_ACFAIL                     (1<<0)
#define VIVO_INT_EBL_SYSFAIL                    (1<<1)
#define VIVO_INT_EBL_IRQ7                       (1<<2)
#define VIVO_INT_EBL_IRQ6                       (1<<3)
#define VIVO_INT_EBL_IRQ5                       (1<<4)
#define VIVO_INT_EBL_IRQ4                       (1<<5)
#define VIVO_INT_EBL_IRQ3                       (1<<6)
#define VIVO_INT_EBL_IRQ2                       (1<<7)
#define VIVO_INT_EBL_IRQ1                       (1<<8)
#define VIVO_INT_EBL_SWIACK                     (1<<9)
#define VIVO_INT_EBL_AXIM_ERR                   (1<<10)
#define VIVO_INT_EBL_AXIS_ERR                   (1<<11)
#define VIVO_INT_EBL_VBERR                      (1<<12)
#define VIVO_INT_EBL_VTIMER                     (1<<13)
#define VIVO_INT_EBL_MBOX0                      (1<<14)
#define VIVO_INT_EBL_MBOX1                      (1<<15)
#define VIVO_INT_EBL_MBOX2                      (1<<16)
#define VIVO_INT_EBL_MBOX3                      (1<<17)
#define VIVO_INT_EBL_PCIE                       (1<<18)
#define VIVO_INT_EBL_DMA0                       (1<<19)
#define VIVO_INT_EBL_DMA1                       (1<<20)
#define VIVO_INT_EBL_ADC                        (1<<21)

static const int VIVO_INT_EBL_IRQEN[7] = {
  VIVO_INT_EBL_IRQ1,
  VIVO_INT_EBL_IRQ2,
  VIVO_INT_EBL_IRQ3,
  VIVO_INT_EBL_IRQ4,
  VIVO_INT_EBL_IRQ5,
  VIVO_INT_EBL_IRQ6,
  VIVO_INT_EBL_IRQ7
};

/*****************************************************************************
 *	Register descriptions for INT_STATUS
 *	(Interrupt Status Register)
 *****************************************************************************/
#define VIVO_INT_STATUS             0x7f804
#define VIVO_INT_STATUS_ACFAIL                  (1<<0)
#define VIVO_INT_STATUS_SYSFAIL                 (1<<1)
#define VIVO_INT_STATUS_IRQ7                    (1<<2)
#define VIVO_INT_STATUS_IRQ6                    (1<<3)
#define VIVO_INT_STATUS_IRQ5                    (1<<4)
#define VIVO_INT_STATUS_IRQ4                    (1<<5)
#define VIVO_INT_STATUS_IRQ3                    (1<<6)
#define VIVO_INT_STATUS_IRQ2                    (1<<7)
#define VIVO_INT_STATUS_IRQ1                    (1<<8)
#define VIVO_INT_STATUS_SWIACK                  (1<<9)
#define VIVO_INT_STATUS_AXIM_ERR                (1<<10)
#define VIVO_INT_STATUS_AXIS_ERR                (1<<11)
#define VIVO_INT_STATUS_VBERR                   (1<<12)
#define VIVO_INT_STATUS_VTIMER                  (1<<13)
#define VIVO_INT_STATUS_MBOX0                   (1<<14)
#define VIVO_INT_STATUS_MBOX1                   (1<<15)
#define VIVO_INT_STATUS_MBOX2                   (1<<16)
#define VIVO_INT_STATUS_MBOX3                   (1<<17)
#define VIVO_INT_STATUS_PCIE                    (1<<18)
#define VIVO_INT_STATUS_DMA0                    (1<<19)
#define VIVO_INT_STATUS_DMA1                    (1<<20)
#define VIVO_INT_STATUS_ADC                     (1<<21)

static const int VIVO_INT_STATUS_MBOX[4] = {
  VIVO_INT_STATUS_MBOX0,
  VIVO_INT_STATUS_MBOX1,
  VIVO_INT_STATUS_MBOX2,
  VIVO_INT_STATUS_MBOX3
};

static const int VIVO_INT_STATUS_IRQ[7] = {
  VIVO_INT_STATUS_IRQ1,
  VIVO_INT_STATUS_IRQ2,
  VIVO_INT_STATUS_IRQ3,
  VIVO_INT_STATUS_IRQ4,
  VIVO_INT_STATUS_IRQ5,
  VIVO_INT_STATUS_IRQ6,
  VIVO_INT_STATUS_IRQ7
};

/*****************************************************************************
 *	Register descriptions for VINT_EBL
 *	(VME Interrupt Enable Register)
 *****************************************************************************/
#define VIVO_VINT_EBL               0x7f810
#define VIVO_VINT_EBL_SWIRQ                     (1<<0)
#define VIVO_VINT_EBL_UIRQ                      (1<<1)
#define VIVO_VINT_EBL_ASIRQ                     (1<<2)

/*****************************************************************************
 *	Register descriptions for VINT_STATUS
 *	(VME Interrupt Status Register)
 *****************************************************************************/
#define VIVO_VINT_STATUS            0x7f814
#define VIVO_VINT_STATUS_SWIRQ                  (1<<0)
#define VIVO_VINT_STATUS_UIRQ                   (1<<1)
#define VIVO_VINT_STATUS_ASIRQ                  (1<<2)

/*****************************************************************************
 *	Register descriptions for VME_IRQH_CMD
 *	(VME Interrupt Handler Command)
 *****************************************************************************/
#define VIVO_VME_IRQH_CMD           0x7f820
#define VIVO_VME_IRQH_CMD_H7_ERR                (1<<0)
#define VIVO_VME_IRQH_CMD_H6_ERR                (1<<1)
#define VIVO_VME_IRQH_CMD_H5_ERR                (1<<2)
#define VIVO_VME_IRQH_CMD_H4_ERR                (1<<3)
#define VIVO_VME_IRQH_CMD_H3_ERR                (1<<4)
#define VIVO_VME_IRQH_CMD_H2_ERR                (1<<5)
#define VIVO_VME_IRQH_CMD_H1_ERR                (1<<6)
#define     VIVO_VME_IRQH_CMD_H1_TYPE           (3<<8)
#define         VIVO_VME_IRQH_CMD_H1_TYPE_D08           (0<<8)
#define         VIVO_VME_IRQH_CMD_H1_TYPE_D16           (1<<8)
#define         VIVO_VME_IRQH_CMD_H1_TYPE_D32           (2<<8)

/*****************************************************************************
 *	Register descriptions for VME_IRQ7_STAT
 *	(VME IRQ7 STATUS/ID)
 *****************************************************************************/
#define VIVO_VME_IRQ7_STAT          0x7f824
#define     VIVO_VME_IRQ7_STAT_H7_STAT          (0xff<<0)
#define VIVO_VME_IRQ7_STAT_H7_ERR               (1<<8)

/*****************************************************************************
 *	Register descriptions for VME_IRQ6_STAT
 *	(VME IRQ1 STATUS/ID)
 *****************************************************************************/
#define VIVO_VME_IRQ6_STAT          0x7f828
#define     VIVO_VME_IRQ6_STAT_H6_STAT          (0xff<<0)
#define VIVO_VME_IRQ6_STAT_H6_ERR               (1<<8)

/*****************************************************************************
 *	Register descriptions for VME_IRQ5_STAT
 *	(VME IRQ5 STATUS/ID)
 *****************************************************************************/
#define VIVO_VME_IRQ5_STAT          0x7f82c
#define     VIVO_VME_IRQ5_STAT_H5_STAT          (0xff<<0)
#define VIVO_VME_IRQ5_STAT_H5_ERR               (1<<8)

/*****************************************************************************
 *	Register descriptions for VME_IRQ4_STAT
 *	(VME IRQ4 STATUS/ID)
 *****************************************************************************/
#define VIVO_VME_IRQ4_STAT          0x7f830
#define     VIVO_VME_IRQ4_STAT_H4_STAT          (0xff<<0)
#define VIVO_VME_IRQ4_STAT_H4_ERR               (1<<8)

/*****************************************************************************
 *	Register descriptions for VME_IRQ3_STAT
 *	(VME IRQ3 STATUS/ID)
 *****************************************************************************/
#define VIVO_VME_IRQ3_STAT          0x7f834
#define     VIVO_VME_IRQ3_STAT_H3_STAT          (0xff<<0)
#define VIVO_VME_IRQ3_STAT_H3_ERR               (1<<8)

/*****************************************************************************
 *	Register descriptions for VME_IRQ2_STAT
 *	(VME IRQ2 STATUS/ID)
 *****************************************************************************/
#define VIVO_VME_IRQ2_STAT          0x7f838
#define     VIVO_VME_IRQ2_STAT_H2_STAT          (0xff<<0)
#define VIVO_VME_IRQ2_STAT_H2_ERR               (1<<8)

/*****************************************************************************
 *	Register descriptions for VME_IRQ1_STAT
 *	(VME IRQ1 STATUS/ID)
 *****************************************************************************/
#define VIVO_VME_IRQ1_STAT          0x7f83c
#define     VIVO_VME_IRQ1_STAT_H1_STAT          (0xff<<0)
#define VIVO_VME_IRQ1_STAT_H1_ERR               (1<<8)

static const int VIVO_VME_IRQ_STAT[7] = {
  VIVO_VME_IRQ1_STAT,
  VIVO_VME_IRQ2_STAT,
  VIVO_VME_IRQ3_STAT,
  VIVO_VME_IRQ4_STAT,
  VIVO_VME_IRQ5_STAT,
  VIVO_VME_IRQ6_STAT,
  VIVO_VME_IRQ7_STAT
};


/*****************************************************************************
 *	Register descriptions for VME_INT
 *	(VME Interrupter)
 *****************************************************************************/
#define VIVO_VME_INT                0x7f840
#define VIVO_VME_INT_SWIRQ                      (1<<0)

/*****************************************************************************
 *	Register descriptions for VME_INT_STAT
 *	(VME Interrupter STATUS/ID)
*****************************************************************************/
#define VIVO_VME_INT_STAT           0x7f844
#define     VIVO_VME_INT_STAT_VINT_STAT         (0x7f<<1)

/*****************************************************************************
 *	Register descriptions for VME_INT_STAT_SW
 *	(VME Interrupter Software STATUS/ID)
 *****************************************************************************/
#define VIVO_VME_INT_STAT_SW        0x7f848
#define     VIVO_VME_INT_STAT_SW_STAT_SW        (0xffffffff<<0)

/*****************************************************************************
 *	Register descriptions for VME_INT_MAP
 *	(VME Interrupter Map)
 *****************************************************************************/
#define VIVO_VME_INT_MAP            0x7f84c
#define     VIVO_VME_INT_MAP_SWIRQ              (7<<0)
#define     VIVO_VME_INT_MAP_UIRQ               (7<<4)
#define     VIVO_VME_INT_MAP_TYPE               (3<<8)
#define         VIVO_VME_INT_MAP_TYPE_D08               (0<<8)
#define         VIVO_VME_INT_MAP_TYPE_D16               (1<<8)
#define         VIVO_VME_INT_MAP_TYPE_D32               (2<<8)

/*****************************************************************************
 *	Register descriptions for SEMAPHORE
 *	(Semaphore Register)
 *****************************************************************************/
#define VIVO_SEMAPHORE              0x7f850
#define     VIVO_SEMAPHORE_SEMA0                (0xff<<0)
#define     VIVO_SEMAPHORE_SEMA1                (0xff<<8)
#define     VIVO_SEMAPHORE_SEMA2                (0xff<<16)
#define     VIVO_SEMAPHORE_SEMA3                (0xff<<24)

/*****************************************************************************
 *	Register descriptions for MAILBOX4
 *	(Mailbox Register 4)
 *****************************************************************************/
#define VIVO_MAILBOX4               0x7f860
#define     VIVO_MAILBOX4_MB4                   (0xffffffff<<0)

/*****************************************************************************
 *	Register descriptions for MAILBOX3
 *	(Mailbox Register 3)
 *****************************************************************************/
#define VIVO_MAILBOX3               0x7f864
#define     VIVO_MAILBOX3_MB3                   (0xffffffff<<0)

/*****************************************************************************
 *	Register descriptions for MAILBOX2
 *	(Mailbox Register 2)
 *****************************************************************************/
#define VIVO_MAILBOX2               0x7f868
#define     VIVO_MAILBOX2_MB2                   (0xffffffff<<0)

/*****************************************************************************
 *	Register descriptions for MAILBOX1
 *	(Mailbox Register 1)
 *****************************************************************************/
#define VIVO_MAILBOX1               0x7f86c
#define     VIVO_MAILBOX1_MB1                   (0xffffffff<<0)

static const int VIVO_MAILBOX[4] = {
  VIVO_MAILBOX1,
  VIVO_MAILBOX2,
  VIVO_MAILBOX3,
  VIVO_MAILBOX4
};

/****************************************************************************
 *	These defines are used to identify a slave window's AM access.
 ****************************************************************************/
#define	VIVO_AMCODE_A16             0x00000020
#define	VIVO_AMCODE_A24             0x00000030
#define	VIVO_AMCODE_A32             0x00000000
#define	VIVO_AMCODE_USER            0x00000008
#define	VIVO_AMCODE_SUPER           0x0000000C
#define	VIVO_AMCODE_DATA            0x00000001
#define	VIVO_AMCODE_PROG            0x00000002
#define	VIVO_XAMCODE_A32_2ESST      0x00000011
#define	VIVO_XAMCODE_A32_2ESSTB     0x00000012
#define	VIVO_XAMCODE_A64_2ESST      0x00000021
#define	VIVO_XAMCODE_A64_2ESSTB     0x00000022

/*****************************************************************************
 *	Register descriptions for SLVW_CTRL1
 *	(Slave Window Control 1)
 *****************************************************************************/
#define VIVO_SLVW_CTRL0             0x7f880
#define VIVO_SLVW_CTRL1             0x7f890
#define VIVO_SLVW_CTRL2             0x7f8a0
#define VIVO_SLVW_CTRL3             0x7f8b0
#define VIVO_SLVW_CTRL4             0x7f8c0
#define VIVO_SLVW_CTRL5             0x7f8d0
#define VIVO_SLVW_CTRL6             0x7f8e0
#define VIVO_SLVW_CTRL7             0x7f8f0

static const int VIVO_SLVW_CTRL[8] = {
  VIVO_SLVW_CTRL0, VIVO_SLVW_CTRL1,
  VIVO_SLVW_CTRL2, VIVO_SLVW_CTRL3,
  VIVO_SLVW_CTRL4, VIVO_SLVW_CTRL5,
  VIVO_SLVW_CTRL6, VIVO_SLVW_CTRL7
};

#define VIVO_SLVW_CTRLX_EBL                     (1<<0)
#define VIVO_SLVW_CTRLX_FAF                     (1<<1)
#define VIVO_SLVW_CTRLX_DFS                     (1<<2)
#define VIVO_SLVW_CTRLX_NPRIV_EBL               (1<<3)
#define VIVO_SLVW_CTRLX_SUP_EBL                 (1<<4)
#define VIVO_SLVW_CTRLX_DAT_EBL                 (1<<5)
#define VIVO_SLVW_CTRLX_PRG_EBL                 (1<<6)
#define VIVO_SLVW_CTRLX_BLT_EBL                 (1<<7)
#define VIVO_SLVW_CTRLX_MBLT_EBL                (1<<8)
#define VIVO_SLVW_CTRLX_SBRA                    (1<<9)
#define VIVO_SLVW_CTRLX_BE                      (1<<10)
#define VIVO_SLVW_CTRLX_AREH                    (1<<11)
#define VIVO_SLVW_CTRLX_ATO                     (1<<12)
#define     VIVO_SLVW_CTRLX_2ESST_RATE          (3<<13)
#define         VIVO_SLVW_CTRLX_2ESST_RATE_160          (0<<13)
#define         VIVO_SLVW_CTRLX_2ESST_RATE_267          (1<<13)
#define         VIVO_SLVW_CTRLX_2ESST_RATE_320          (2<<13)
#define     VIVO_SLVW_CTRLX_BLEN                (0xff<<24)

/*****************************************************************************
 *	Register descriptions for SLVW_MSK1
 *	(Slave Window Address Decoder Mask Register 1)
 *****************************************************************************/
#define VIVO_SLVW_MSK0              0x7f884
#define VIVO_SLVW_MSK1              0x7f894
#define VIVO_SLVW_MSK2              0x7f8a4
#define VIVO_SLVW_MSK3              0x7f8b4
#define VIVO_SLVW_MSK4              0x7f8c4
#define VIVO_SLVW_MSK5              0x7f8d4
#define VIVO_SLVW_MSK6              0x7f8e4
#define VIVO_SLVW_MSK7              0x7f8f4
#define     VIVO_SLVW_MSK_ADEM                  (0xffffff<<8)

static const int VIVO_SLVW_MSK[8] = {
  VIVO_SLVW_MSK0, VIVO_SLVW_MSK1,
  VIVO_SLVW_MSK2, VIVO_SLVW_MSK3,
  VIVO_SLVW_MSK4, VIVO_SLVW_MSK5,
  VIVO_SLVW_MSK6, VIVO_SLVW_MSK7
};

/*****************************************************************************
 *	Register descriptions for SLVW_OFFSET1
 *	(Slave Window Offset 1)
 *****************************************************************************/
#define VIVO_SLVW_OFFSET0           0x7f888
#define VIVO_SLVW_OFFSET1           0x7f898
#define VIVO_SLVW_OFFSET2           0x7f8a8
#define VIVO_SLVW_OFFSET3           0x7f8b8
#define VIVO_SLVW_OFFSET4           0x7f8c8
#define VIVO_SLVW_OFFSET5           0x7f8d8
#define VIVO_SLVW_OFFSET6           0x7f8e8
#define VIVO_SLVW_OFFSET7           0x7f8f8
#define     VIVO_SLVW_OFFSETX_OFFSET            (0xffffff<<8)

static const int VIVO_SLVW_OFFSET[8] = {
  VIVO_SLVW_OFFSET0, VIVO_SLVW_OFFSET1,
  VIVO_SLVW_OFFSET2, VIVO_SLVW_OFFSET3,
  VIVO_SLVW_OFFSET4, VIVO_SLVW_OFFSET5,
  VIVO_SLVW_OFFSET6, VIVO_SLVW_OFFSET7
};

/*****************************************************************************
 *	Register descriptions for SLVW_OFFSETU1
 *	(Slave Window Upper Offset 1)
 *****************************************************************************/
#define VIVO_SLVW_OFFSETU0          0x7f88c
#define VIVO_SLVW_OFFSETU1          0x7f89c
#define VIVO_SLVW_OFFSETU2          0x7f8ac
#define VIVO_SLVW_OFFSETU3          0x7f8bc
#define VIVO_SLVW_OFFSETU4          0x7f8cc
#define VIVO_SLVW_OFFSETU5          0x7f8dc
#define VIVO_SLVW_OFFSETU6          0x7f8ec
#define VIVO_SLVW_OFFSETU7          0x7f8fc
#define     VIVO_SLVW_OFFSETUX_OFFSETU          (0xffffffff<<0)

static const int VIVO_SLVW_OFFSETU[8] = {
  VIVO_SLVW_OFFSETU0, VIVO_SLVW_OFFSETU1,
  VIVO_SLVW_OFFSETU2, VIVO_SLVW_OFFSETU3,
  VIVO_SLVW_OFFSETU4, VIVO_SLVW_OFFSETU5,
  VIVO_SLVW_OFFSETU6, VIVO_SLVW_OFFSETU7
};

/*****************************************************************************
 *	Register descriptions for MW_CTRL1
 *	(Master Window Control 1)
 *****************************************************************************/
#define VIVO_MW_CTRL0               0x7f900
#define VIVO_MW_CTRL1               0x7f920
#define VIVO_MW_CTRL2               0x7f940
#define VIVO_MW_CTRL3               0x7f960
#define VIVO_MW_CTRL4               0x7f980
#define VIVO_MW_CTRL5               0x7f9a0
#define VIVO_MW_CTRL6               0x7f9c0
#define VIVO_MW_CTRL7               0x7f9e0

static const int VIVO_MW_CTRL[8] = {
  VIVO_MW_CTRL0, VIVO_MW_CTRL1,
  VIVO_MW_CTRL2, VIVO_MW_CTRL3,
  VIVO_MW_CTRL4, VIVO_MW_CTRL5,
  VIVO_MW_CTRL6, VIVO_MW_CTRL7
};

#define VIVO_MW_CTRLX_EBL                       (1<<0)
#define VIVO_MW_CTRLX_AM_DA                     (1<<1)
#define         VIVO_MW_CTRLX_AM_DA_DATA                (0<<1)
#define         VIVO_MW_CTRLX_AM_DA_PROGRAM             (1<<1)
#define VIVO_MW_CTRLX_AM_NPA                    (1<<2)
#define         VIVO_MW_CTRLX_AM_NPA_USER               (0<<2)
#define         VIVO_MW_CTRLX_AM_NPA_SUPER              (1<<2)
#define     VIVO_MW_CTRLX_AM_AS                 (7<<3)
#define         VIVO_MW_CTRLX_AM_AS_A16                 (0<<3)
#define         VIVO_MW_CTRLX_AM_AS_A24                 (1<<3)
#define         VIVO_MW_CTRLX_AM_AS_A32                 (2<<3)
#define         VIVO_MW_CTRLX_AM_AS_CRCSR               (3<<3)
#define         VIVO_MW_CTRLX_AM_AS_USER1               (4<<3)
#define         VIVO_MW_CTRLX_AM_AS_USER2               (5<<3)
#define     VIVO_MW_CTRLX_BT                    (7<<6)
#define         VIVO_MW_CTRLX_BT_SCT                    (0<<6)
#define         VIVO_MW_CTRLX_BT_BLT                    (1<<6)
#define         VIVO_MW_CTRLX_BT_MBLT                   (2<<6)
#define         VIVO_MW_CTRLX_BT_2eSST                  (4<<6)
#define         VIVO_MW_CTRLX_BT_2eSSTB                 (5<<6)
#define     VIVO_MW_CTRLX_SST                   (3<<9)
#define         VIVO_MW_CTRLX_SST_160                   (0<<9)
#define         VIVO_MW_CTRLX_SST_267                   (1<<9)
#define         VIVO_MW_CTRLX_SST_320                   (2<<9)
#define VIVO_MW_CTRLX_SCTPWR                    (1<<11)
#define         VIVO_MW_CTRLX_SCTPWR_COUPLED            (0<<11)
#define         VIVO_MW_CTRLX_SCTPWR_POSTED             (1<<11)

/*****************************************************************************
 *	Register descriptions for MW_ADDR1
 *	(Master Window Address 1)
 *****************************************************************************/
#define VIVO_MW_ADDR0               0x7f904
#define VIVO_MW_ADDR1               0x7f924
#define VIVO_MW_ADDR2               0x7f944
#define VIVO_MW_ADDR3               0x7f964
#define VIVO_MW_ADDR4               0x7f984
#define VIVO_MW_ADDR5               0x7f9a4
#define VIVO_MW_ADDR6               0x7f9c4
#define VIVO_MW_ADDR7               0x7f9e4
#define     VIVO_MW_ADDRX_ADDR                  (0xffffff<<8)

static const int VIVO_MW_ADDR[8] = {
  VIVO_MW_ADDR0, VIVO_MW_ADDR1,
  VIVO_MW_ADDR2, VIVO_MW_ADDR3,
  VIVO_MW_ADDR4, VIVO_MW_ADDR5,
  VIVO_MW_ADDR6, VIVO_MW_ADDR7
};

/*****************************************************************************
 *	Register descriptions for MW_MASK1
 *	(Master Window Address Mask 1)
 *****************************************************************************/
#define VIVO_MW_MASK0               0x7f908
#define VIVO_MW_MASK1               0x7f928
#define VIVO_MW_MASK2               0x7f948
#define VIVO_MW_MASK3               0x7f968
#define VIVO_MW_MASK4               0x7f988
#define VIVO_MW_MASK5               0x7f9a8
#define VIVO_MW_MASK6               0x7f9c8
#define VIVO_MW_MASK7               0x7f9e8
#define     VIVO_MW_MASKX_MASK                  (0xffffff<<8)

static const int VIVO_MW_MASK[8] = {
  VIVO_MW_MASK0, VIVO_MW_MASK1,
  VIVO_MW_MASK2, VIVO_MW_MASK3,
  VIVO_MW_MASK4, VIVO_MW_MASK5,
  VIVO_MW_MASK6, VIVO_MW_MASK7
};

/*****************************************************************************
 *	Register descriptions for MW_OFFSET1
 *	(Master Window Offset 1)
 *****************************************************************************/
#define VIVO_MW_OFFSET0             0x7f90c
#define VIVO_MW_OFFSET1             0x7f92c
#define VIVO_MW_OFFSET2             0x7f94c
#define VIVO_MW_OFFSET3             0x7f96c
#define VIVO_MW_OFFSET4             0x7f98c
#define VIVO_MW_OFFSET5             0x7f9ac
#define VIVO_MW_OFFSET6             0x7f9cc
#define VIVO_MW_OFFSET7             0x7f9ec
#define     VIVO_MW_OFFSETX_OFFSET              (0xffffff<<8)

static const int VIVO_MW_OFFSET[8] = {
  VIVO_MW_OFFSET0, VIVO_MW_OFFSET1,
  VIVO_MW_OFFSET2, VIVO_MW_OFFSET3,
  VIVO_MW_OFFSET4, VIVO_MW_OFFSET5,
  VIVO_MW_OFFSET6, VIVO_MW_OFFSET7
};

/*****************************************************************************
 *	Register descriptions for MW_2ESST1
 *	(Master Window 2ESST Broadcast Slave Select 1)
 *****************************************************************************/
#define VIVO_MW_2ESST0              0x7f910
#define VIVO_MW_2ESST1              0x7f930
#define VIVO_MW_2ESST2              0x7f950
#define VIVO_MW_2ESST3              0x7f970
#define VIVO_MW_2ESST4              0x7f990
#define VIVO_MW_2ESST5              0x7f9b0
#define VIVO_MW_2ESST6              0x7f9d0
#define VIVO_MW_2ESST7              0x7f9f0
#define     VIVO_MW_2ESSTX_SSTBBS               (0x1fffff<<0)

static const int VIVO_MW_2ESST[8] = {
  VIVO_MW_2ESST0, VIVO_MW_2ESST1,
  VIVO_MW_2ESST2, VIVO_MW_2ESST3,
  VIVO_MW_2ESST4, VIVO_MW_2ESST5,
  VIVO_MW_2ESST6, VIVO_MW_2ESST7
};

/*****************************************************************************
 *	Register descriptions for AXIS_ECR
 *	(AXI Slave Error Capture Register)
 *****************************************************************************/
#define VIVO_AXIS_ECR               0x7fa10
#define     VIVO_AXIS_ECR_TYPE                  (7<<0)
#define         VIVO_AXIS_ECR_TYPE_NORMAL               (0<<0)
#define         VIVO_AXIS_ECR_TYPE_RETRY                (1<<0)
#define         VIVO_AXIS_ECR_TYPE_BERR                 (2<<0)
#define         VIVO_AXIS_ECR_TYPE_MW_DISABLE           (3<<0)
#define         VIVO_AXIS_ECR_TYPE_INV_ADDRGN           (4<<0)
#define         VIVO_AXIS_ECR_TYPE_UNSUP_ACC            (5<<0)
#define VIVO_AXIS_ECR_RWT                       (1<<3)
#define         VIVO_AXIS_ECR_RWT_WRITE                 (0<<3)
#define         VIVO_AXIS_ECR_RWT_READ                  (1<<3)
#define VIVO_AXIS_ECR_TT                        (1<<4)
#define         VIVO_AXIS_ECR_TT_MASTER                 (0<<4)
#define         VIVO_AXIS_ECR_TT_DMA                    (1<<4)
#define VIVO_AXIS_ECR_LOST                      (1<<5)
#define         VIVO_AXIS_ECR_LOST_WRITE                (0<<5)
#define         VIVO_AXIS_ECR_LOST_ERR_MISSED           (1<<5)

/*****************************************************************************
 *	Register descriptions for AXIS_ECR_A
 *	(AXI Slave Error Capture Register Address)
 *****************************************************************************/
#define VIVO_AXIS_ECR_A             0x7fa14
#define     VIVO_AXIS_ECR_A_ADDR                (0x3fffffff<<2)

/*****************************************************************************
 *	Register descriptions for AXIM_ECR
 *	(AXI Master Error Capture Register)
 *****************************************************************************/
#define VIVO_AXIM_ECR               0x7fa18
#define     VIVO_AXIM_ECR_TYPE                  (7<<0)
#define         VIVO_AXIM_ECR_TYPE_NORMAL               (0<<0)
#define         VIVO_AXIM_ECR_TYPE_SLAVE_ERR            (1<<0)
#define         VIVO_AXIM_ECR_TYPE_DECODE_ERR           (2<<0)
#define VIVO_AXIM_ECR_RWT                       (1<<3)
#define         VIVO_AXIM_ECR_RWT_WRITE                 (0<<3)
#define         VIVO_AXIM_ECR_RWT_READ                  (1<<3)
#define VIVO_AXIM_ECR_LOST                      (1<<4)
#define         VIVO_AXIM_ECR_LOST_WRITE                (0<<4)
#define         VIVO_AXIM_ECR_LOST_ERR_MISSED           (1<<4)

/*****************************************************************************
 *	Register descriptions for AXIM_ECR_A
 *	(AXI Master Error Capture Register Address)
 *****************************************************************************/
#define VIVO_AXIM_ECR_A             0x7fa1c
#define     VIVO_AXIM_ECR_A_ADDR                (0xffffffff<<0)

/*****************************************************************************
 *	Register descriptions for VME_MSTR
 *	(VME Master Controller)
 *****************************************************************************/
#define VIVO_VME_MSTR               0x7fa20
#define     VIVO_VME_MSTR_VMSTREQ               (3<<0)
#define         VIVO_VME_MSTR_VMSTREQ_0                 (0<<0)
#define         VIVO_VME_MSTR_VMSTREQ_1                 (1<<0)
#define         VIVO_VME_MSTR_VMSTREQ_2                 (2<<0)
#define         VIVO_VME_MSTR_VMSTREQ_3                 (3<<0)
#define     VIVO_VME_MSTR_VMSTFAIR              (1<<2)
#define         VIVO_VME_MSTR_VMSTFAIR_DIRECT           (0<<2)
#define         VIVO_VME_MSTR_VMSTFAIR_FAIR             (1<<2)
#define     VIVO_VME_MSTR_VMSTREL               (3<<3)
#define         VIVO_VME_MSTR_VMSTREL_WHEN_DONE         (0<<3)
#define         VIVO_VME_MSTR_VMSTREL_ON_REQ            (1<<3)
#define         VIVO_VME_MSTR_VMSTREL_DONE_OR_BCLR      (2<<3)
#define         VIVO_VME_MSTR_VMSTREL_DONE_AND_REQ      (3<<3)
#define     VIVO_VME_MSTR_VMSTRBR               (1<<5)
#define     VIVO_VME_MSTR_VMSTRTON              (7<<8)
#define         VIVO_VME_MSTR_VMSTRTON_4us              (0<<8)
#define         VIVO_VME_MSTR_VMSTRTON_8us              (1<<8)
#define         VIVO_VME_MSTR_VMSTRTON_16us             (2<<8)
#define         VIVO_VME_MSTR_VMSTRTON_32us             (3<<8)
#define         VIVO_VME_MSTR_VMSTRTON_64us             (4<<8)
#define         VIVO_VME_MSTR_VMSTRTON_128us            (5<<8)
#define         VIVO_VME_MSTR_VMSTRTON_256us            (6<<8)
#define         VIVO_VME_MSTR_VMSTRTON_512us            (7<<8)
#define     VIVO_VME_MSTR_VMSTRTOFF             (7<<11)
#define         VIVO_VME_MSTR_VMSTRTOFF_0us             (0<<11)
#define         VIVO_VME_MSTR_VMSTRTOFF_1us             (1<<11)
#define         VIVO_VME_MSTR_VMSTRTOFF_2us             (2<<11)
#define         VIVO_VME_MSTR_VMSTRTOFF_4us             (3<<11)
#define         VIVO_VME_MSTR_VMSTRTOFF_8us             (4<<11)
#define         VIVO_VME_MSTR_VMSTRTOFF_16us            (5<<11)
#define         VIVO_VME_MSTR_VMSTRTOFF_32us            (6<<11)
#define         VIVO_VME_MSTR_VMSTRTOFF_64us            (7<<11)
#define     VIVO_VME_MSTR_USERAM0               (0xf<<16)
#define         VIVO_VME_MSTR_USERAM0_10h               (0<<16)
#define     VIVO_VME_MSTR_FIXED0                (3<<20)
#define     VIVO_VME_MSTR_USERAM1               (0xf<<24)
#define         VIVO_VME_MSTR_USERAM1_14h               (4<<24)
#define     VIVO_VME_MSTR_FIXED1                (3<<28)

/*****************************************************************************
 *	Register descriptions for VME_MSTR_STAT
 *	(VME Master Status)
 *****************************************************************************/
#define VIVO_VME_MSTR_STAT          0x7fa24
#define VIVO_VME_MSTR_STAT_VMSTR_OWNER          (1<<0)
#define VIVO_VME_MSTR_STAT_VMSTR_BUSY           (1<<1)
#define VIVO_VME_MSTR_STAT_VMSTRBG              (1<<3)

/*****************************************************************************
 *	Register descriptions for SYS_CTRL
 *	(System Controller)
 *****************************************************************************/
#define VIVO_SYS_CTRL               0x7fa30
#define     VIVO_SYS_CTRL_GA                    (0x1f<<0)
#define VIVO_SYS_CTRL_GAP                       (1<<5)
#define VIVO_SYS_CTRL_SYSCTRL                   (1<<6)
#define VIVO_SYS_CTRL_SYSCTRL_SET               (1<<8)
#define VIVO_SYS_CTRL_BUS_ARB                   (1<<9)
#define         VIVO_SYS_CTRL_BUS_ARB_ROUND_ROBIN       (0<<9)
#define         VIVO_SYS_CTRL_BUS_ARB_PRIORITY          (1<<9)
#define VIVO_SYS_CTRL_SRESET                    (1<<10)
#define VIVO_SYS_CTRL_LRESET                    (1<<11)
#define VIVO_SYS_CTRL_ACFAIL_EBL                (1<<12)
#define     VIVO_SYS_CTRL_BERRTIMER             (0x7ff<<16)
#define         VIVO_SYS_CTRL_BERRTIMER_DISABLE         (0<<16)
#define         VIVO_SYS_CTRL_BERRTIMER_8us             (8<<16)
#define         VIVO_SYS_CTRL_BERRTIMER_16us            (0x10<<16)
#define         VIVO_SYS_CTRL_BERRTIMER_32us            (0x20<<16)
#define         VIVO_SYS_CTRL_BERRTIMER_64us            (0x40<<16)
#define         VIVO_SYS_CTRL_BERRTIMER_128us           (0x80<<16)
#define         VIVO_SYS_CTRL_BERRTIMER_256us           (0x100<<16)
#define         VIVO_SYS_CTRL_BERRTIMER_512us           (0x200<<16)
#define         VIVO_SYS_CTRL_BERRTIMER_1024us          (0x400<<16)
#define         VIVO_SYS_CTRL_BERRTIMER_2047us          (0x7ff<<16)

/*****************************************************************************
 *	Register descriptions for DEV_CTRL
 *	(Device Control Register)
 *****************************************************************************/
#define VIVO_DEV_CTRL               0x7faf0
#define VIVO_DEV_CTRL_LENDIAN                   (1<<0)
#define VIVO_DEV_CTRL_LENDIAN_CSR               (1<<1)

/*****************************************************************************
 *	Register descriptions for DEV_VER
 *	(Device Identification and Version Register)
 *****************************************************************************/
#define VIVO_DEV_VER                0x7faf4
#define     VIVO_DEV_VER_VERN                   (0xf<<0)
#define     VIVO_DEV_VER_VERZ                   (0xf<<4)
#define     VIVO_DEV_VER_VERY                   (0xf<<8)
#define     VIVO_DEV_VER_VERX                   (0xf<<12)
#define     VIVO_DEV_VER_COREID                 (0xffff<<16)

/*****************************************************************************
 *	Register descriptions for USER_VER
 *	(User Version Register)
 *****************************************************************************/
#define VIVO_USER_VER               0x7faf8
#define     VIVO_USER_VER_USERVER               (0xffffffff<<0)

/*****************************************************************************
 *	Register descriptions for CSR_ADER_HH1
 *	(ADER [24:31] 1)
 *****************************************************************************/
#define VIVO_CSR_ADER_HH1           0x7ff60
#define VIVO_CSR_ADER_HH2           0x7ff70
#define VIVO_CSR_ADER_HH3           0x7ff80
#define VIVO_CSR_ADER_HH4           0x7ff90
#define VIVO_CSR_ADER_HH5           0x7ffa0
#define VIVO_CSR_ADER_HH6           0x7ffb0
#define VIVO_CSR_ADER_HH7           0x7ffc0
#define VIVO_CSR_ADER_HH8           0x7ffd0
#define     VIVO_CSR_ADER_HHX_ADDR              (0xff<<24)

static const int VIVO_CSR_ADER_HH[8] = {
  VIVO_CSR_ADER_HH1, VIVO_CSR_ADER_HH2,
  VIVO_CSR_ADER_HH3, VIVO_CSR_ADER_HH4,
  VIVO_CSR_ADER_HH5, VIVO_CSR_ADER_HH6,
  VIVO_CSR_ADER_HH7, VIVO_CSR_ADER_HH8
};

/*****************************************************************************
 *	Register descriptions for CSR_ADER_HL1
 *	(ADER [16:23] 1)
 *****************************************************************************/
#define VIVO_CSR_ADER_HL1           0x7ff64
#define VIVO_CSR_ADER_HL2           0x7ff74
#define VIVO_CSR_ADER_HL3           0x7ff84
#define VIVO_CSR_ADER_HL4           0x7ff94
#define VIVO_CSR_ADER_HL5           0x7ffa4
#define VIVO_CSR_ADER_HL6           0x7ffb4
#define VIVO_CSR_ADER_HL7           0x7ffc4
#define VIVO_CSR_ADER_HL8           0x7ffd4
#define     VIVO_CSR_ADER_HLX_ADDR              (0xff<<24)

static const int VIVO_CSR_ADER_HL[8] = {
  VIVO_CSR_ADER_HL1, VIVO_CSR_ADER_HL2,
  VIVO_CSR_ADER_HL3, VIVO_CSR_ADER_HL4,
  VIVO_CSR_ADER_HL5, VIVO_CSR_ADER_HL6,
  VIVO_CSR_ADER_HL7, VIVO_CSR_ADER_HL8
};

/*****************************************************************************
 *	Register descriptions for CSR_ADER_LH1
 *	(ADER [8:15] 1)
 *****************************************************************************/
#define VIVO_CSR_ADER_LH1           0x7ff68
#define VIVO_CSR_ADER_LH2           0x7ff78
#define VIVO_CSR_ADER_LH3           0x7ff88
#define VIVO_CSR_ADER_LH4           0x7ff98
#define VIVO_CSR_ADER_LH5           0x7ffa8
#define VIVO_CSR_ADER_LH6           0x7ffb8
#define VIVO_CSR_ADER_LH7           0x7ffc8
#define VIVO_CSR_ADER_LH8           0x7ffd8
#define     VIVO_CSR_ADER_LHX_ADDR              (0xff<<24)

static const int VIVO_CSR_ADER_LH[8] = {
  VIVO_CSR_ADER_LH1, VIVO_CSR_ADER_LH2,
  VIVO_CSR_ADER_LH3, VIVO_CSR_ADER_LH4,
  VIVO_CSR_ADER_LH5, VIVO_CSR_ADER_LH6,
  VIVO_CSR_ADER_LH7, VIVO_CSR_ADER_LH8
};

/*****************************************************************************
 *	Register descriptions for CSR_ADER_LL1
 *	(ADER [0:7] 1)
 *****************************************************************************/
#define VIVO_CSR_ADER_LL1           0x7ff6c
#define VIVO_CSR_ADER_LL2           0x7ff7c
#define VIVO_CSR_ADER_LL3           0x7ff8c
#define VIVO_CSR_ADER_LL4           0x7ff9c
#define VIVO_CSR_ADER_LL5           0x7ffac
#define VIVO_CSR_ADER_LL6           0x7ffbc
#define VIVO_CSR_ADER_LL7           0x7ffcc
#define VIVO_CSR_ADER_LL8           0x7ffdc
#define     VIVO_CSR_ADER_LLX_ADDR              (0xff<<24)

static const int VIVO_CSR_ADER_LL[8] = {
  VIVO_CSR_ADER_LL1, VIVO_CSR_ADER_LL2,
  VIVO_CSR_ADER_LL3, VIVO_CSR_ADER_LL4,
  VIVO_CSR_ADER_LL5, VIVO_CSR_ADER_LL6,
  VIVO_CSR_ADER_LL7, VIVO_CSR_ADER_LL8
};

/*****************************************************************************
 *	Register descriptions for CSR_ADER1
 *	(ADER [0:31] 1)
 *****************************************************************************/
#define VIVO_CSR_ADER1              0x7ff60
#define VIVO_CSR_ADER2              0x7ff70
#define VIVO_CSR_ADER3              0x7ff80
#define VIVO_CSR_ADER4              0x7ff90
#define VIVO_CSR_ADER5              0x7ffa0
#define VIVO_CSR_ADER6              0x7ffb0
#define VIVO_CSR_ADER7              0x7ffc0
#define VIVO_CSR_ADER8              0x7ffd0
#define VIVO_CSR_ADDRX_XAM_MODE                 (1<<0)
#define VIVO_CSR_ADDRX_DFSR                     (1<<1)
#define     VIVO_CSR_ADDRX_AM                   (0x3f<<2)
#define     VIVO_CSR_ADDRX_XAM                  (3<<8)
#define     VIVO_CSR_ADDRX_C                    (0x3fffff<<10)

/*****************************************************************************
 *	Register descriptions for UBIT_CLEAR
 *	(User Bit Clear Register)
 *****************************************************************************/
#define VIVO_UBIT_CLEAR             0x7ffe8
#define VIVO_UBIT_CLEAR_UBITCLR0                (1<<24)
#define VIVO_UBIT_CLEAR_UBITCLR1                (1<<25)
#define VIVO_UBIT_CLEAR_UBITCLR2                (1<<26)
#define VIVO_UBIT_CLEAR_UBITCLR3                (1<<27)
#define VIVO_UBIT_CLEAR_UBITCLR4                (1<<28)
#define VIVO_UBIT_CLEAR_UBITCLR5                (1<<29)
#define VIVO_UBIT_CLEAR_UBITCLR6                (1<<30)
#define VIVO_UBIT_CLEAR_UBITCLR7                (1<<31)

/*****************************************************************************
 *	Register descriptions for UBIT_SET
 *	(User Bit Set Register)
 *****************************************************************************/
#define VIVO_UBIT_SET               0x7ffec
#define VIVO_UBIT_SET_UBITSET0                  (1<<24)
#define VIVO_UBIT_SET_UBITSET1                  (1<<25)
#define VIVO_UBIT_SET_UBITSET2                  (1<<26)
#define VIVO_UBIT_SET_UBITSET3                  (1<<27)
#define VIVO_UBIT_SET_UBITSET4                  (1<<28)
#define VIVO_UBIT_SET_UBITSET5                  (1<<29)
#define VIVO_UBIT_SET_UBITSET6                  (1<<30)
#define VIVO_UBIT_SET_UBITSET7                  (1<<31)

/*****************************************************************************
 *	Register descriptions for CRAM_OWNER
 *	(Configuration RAM OwnerRegister)
 *****************************************************************************/
#define VIVO_CRAM_OWNER             0x7fff0
#define     VIVO_CRAM_OWNER_CRAM_OWN            (0xff<<24)

/*****************************************************************************
 *	Register descriptions for BIT_CLEAR
 *	(Bit Clear Register)
 *****************************************************************************/
#define VIVO_BIT_CLEAR              0x7fff4
#define VIVO_BIT_CLEAR_CRAMOC                   (1<<26)
#define VIVO_BIT_CLEAR_BERRSC                   (1<<27)
#define VIVO_BIT_CLEAR_MODEBLC                  (1<<28)
#define VIVO_BIT_CLEAR_SDEC                     (1<<30)
#define VIVO_BIT_CLEAR_LRSTC                    (1<<31)

/*****************************************************************************
 *	Register descriptions for BIT_SET
 *	(Bit Set Register)
 *****************************************************************************/
#define VIVO_BIT_SET                0x7fff8
#define VIVO_BIT_SET_CRAMOS                     (1<<26)
#define VIVO_BIT_SET_BERRSS                     (1<<27)
#define VIVO_BIT_SET_MODEBLS                    (1<<28)
#define VIVO_BIT_SET_SDES                       (1<<30)
#define VIVO_BIT_SET_LRSTS                      (1<<31)

/*****************************************************************************
 *	Register descriptions for CRBAR
 *	(CR/CSR Base Address Register)
 *****************************************************************************/
#define VIVO_CRBAR                  0x7fffc
#define     VIVO_CRBAR_CRBAR                    (0x1f<<27)

/*****************************************************************************
 *	Base address/size for DMA
 *****************************************************************************/
#define	VIVO_DMA_GRP_BASE	((u32)0x0007b000)
#define	VIVO_DMA_GRP_SIZE	((u32)0x00000000)

/*****************************************************************************
 *	Register descriptions for DMACTRL0
 *	(DMA Control Register 0)
 *****************************************************************************/
#define VIVO_DMACTRL0               0x7b000
#define VIVO_DMACTRL1               0x7c000

static const int VIVO_DMACTRL[2] = {
  VIVO_DMACTRL0, VIVO_DMACTRL1
};

#define     VIVO_DMACTRLX_AHBOT                 (7<<0)
#define         VIVO_DMACTRLX_AHBOT_0us                 (0<<0)
#define         VIVO_DMACTRLX_AHBOT_1us                 (1<<0)
#define         VIVO_DMACTRLX_AHBOT_2us                 (2<<0)
#define         VIVO_DMACTRLX_AHBOT_4us                 (3<<0)
#define         VIVO_DMACTRLX_AHBOT_8us                 (4<<0)
#define         VIVO_DMACTRLX_AHBOT_16us                (5<<0)
#define         VIVO_DMACTRLX_AHBOT_32us                (6<<0)
#define         VIVO_DMACTRLX_AHBOT_64us                (7<<0)
#define     VIVO_DMACTRLX_AHTS                  (7<<4)
#define         VIVO_DMACTRLX_AHTS_32                   (0<<4)
#define         VIVO_DMACTRLX_AHTS_64                   (1<<4)
#define         VIVO_DMACTRLX_AHTS_128                  (2<<4)
#define         VIVO_DMACTRLX_AHTS_256                  (3<<4)
#define         VIVO_DMACTRLX_AHTS_512                  (4<<4)
#define         VIVO_DMACTRLX_AHTS_1024                 (5<<4)
#define         VIVO_DMACTRLX_AHTS_2048                 (6<<4)
#define     VIVO_DMACTRLX_VBOT                  (7<<8)
#define         VIVO_DMACTRLX_VBOT_0us                  (0<<8)
#define         VIVO_DMACTRLX_VBOT_1us                  (1<<8)
#define         VIVO_DMACTRLX_VBOT_2us                  (2<<8)
#define         VIVO_DMACTRLX_VBOT_4us                  (3<<8)
#define         VIVO_DMACTRLX_VBOT_8us                  (4<<8)
#define         VIVO_DMACTRLX_VBOT_16us                 (5<<8)
#define         VIVO_DMACTRLX_VBOT_32us                 (6<<8)
#define         VIVO_DMACTRLX_VBOT_64us                 (7<<8)
#define     VIVO_DMACTRLX_VTS                   (7<<12)
#define         VIVO_DMACTRLX_VTS_32                    (0<<12)
#define         VIVO_DMACTRLX_VTS_64                    (1<<12)
#define         VIVO_DMACTRLX_VTS_128                   (2<<12)
#define         VIVO_DMACTRLX_VTS_256                   (3<<12)
#define         VIVO_DMACTRLX_VTS_512                   (4<<12)
#define         VIVO_DMACTRLX_VTS_1024                  (5<<12)
#define         VIVO_DMACTRLX_VTS_2048                  (6<<12)
#define VIVO_DMACTRLX_AHFAR                     (1<<16)
#define VIVO_DMACTRLX_VFAR                      (1<<17)
#define VIVO_DMACTRLX_MODE                      (1<<23)
#define         VIVO_DMACTRLX_MODE_DIRECT               (0<<23)
#define         VIVO_DMACTRLX_MODE_SG                   (1<<23)
#define VIVO_DMACTRLX_START                     (1<<25)
#define         VIVO_DMACTRLX_START_NOP                 (0<<25)
#define         VIVO_DMACTRLX_START_START               (1<<25)
#define VIVO_DMACTRLX_PAUSE                     (1<<26)
#define         VIVO_DMACTRLX_PAUSE_NOP                 (0<<26)
#define         VIVO_DMACTRLX_PAUSE_PAUSE               (1<<26)
#define VIVO_DMACTRLX_ABORT                     (1<<27)
#define         VIVO_DMACTRLX_ABORT_NOP                 (0<<27)
#define         VIVO_DMACTRLX_ABORT_ABORT               (1<<27)

/*****************************************************************************
 *	Register descriptions for DMASTAT0
 *	(DMA Status Register 0)
 *****************************************************************************/
#define VIVO_DMASTAT0               0x7b004
#define VIVO_DMASTAT1               0x7c004

static const int VIVO_DMASTAT[2] = {
  VIVO_DMASTAT0, VIVO_DMASTAT1
};

#define     VIVO_DMASTATX_ERR_STAT              (7<<16)
#define         VIVO_DMASTATX_ERR_STAT_NONE             (0<<16)
#define         VIVO_DMASTATX_ERR_STAT_RD_ERR           (1<<16)
#define         VIVO_DMASTATX_ERR_STAT_WR_ERR           (2<<16)
#define         VIVO_DMASTATX_ERR_STAT_FETCH_ERR        (3<<16)
#define         VIVO_DMASTATX_ERR_STAT_TBD4             (4<<16)
#define         VIVO_DMASTATX_ERR_STAT_TBD5             (5<<16)
#define         VIVO_DMASTATX_ERR_STAT_TBD6             (6<<16)
#define         VIVO_DMASTATX_ERR_STAT_TBD7             (7<<16)
#define VIVO_DMASTATX_BUSY                      (1<<24)
#define VIVO_DMASTATX_DONE                      (1<<25)
#define VIVO_DMASTATX_PAUSE                     (1<<26)
#define VIVO_DMASTATX_ABORT                     (1<<27)
#define VIVO_DMASTATX_ERROR                     (1<<28)

/*****************************************************************************
 *	Register descriptions for DMAIS0
 *	(DMA Interrupt Status Register 0)
 *****************************************************************************/
#define VIVO_DMAIS0                 0x7b008
#define VIVO_DMAIS1                 0x7c008
#define VIVO_DMAISX_DONE                        (1<<0)
#define VIVO_DMAISX_PAUSED                      (1<<1)
#define VIVO_DMAISX_ABORTED                     (1<<2)
#define VIVO_DMAISX_ERROR                       (1<<3)

/*****************************************************************************
 *	Register descriptions for DMAIE0
 *	(DMA Interrupt Enable Register 0)
 *****************************************************************************/
#define VIVO_DMAIE0                 0x7b00c
#define VIVO_DMAIE1                 0x7c00c
#define VIVO_DMAIEX_DONE                        (1<<0)
#define VIVO_DMAIEX_PAUSED                      (1<<1)
#define VIVO_DMAIEX_ABORTED                     (1<<2)
#define VIVO_DMAIEX_ERROR                       (1<<3)

/*****************************************************************************
 *	Register descriptions for DMACDESCL0
 *	(Current Descriptor Address 0)
 *****************************************************************************/
#define VIVO_DMACDESCL0             0x7b010
#define VIVO_DMACDESCL1             0x7c010
#define     VIVO_DMACDESCLX_CDESC               (0x3ffffff<<6)

static const int VIVO_DMACDESCL[2] = {
  VIVO_DMACDESCL0, VIVO_DMACDESCL1
};

/*****************************************************************************
 *	Register descriptions for DMACDESCU0
 *	(Current Descriptor Address Upper 0)
 *****************************************************************************/
#define VIVO_DMACDESCU0             0x7b014
#define VIVO_DMACDESCU1             0x7c014
#define     VIVO_DMACDESCUX_CDESC               (0xffffffff<<0)

static const int VIVO_DMACDESCU[2] = {
  VIVO_DMACDESCU0, VIVO_DMACDESCU1
};

/*****************************************************************************
 *	Register descriptions for DMACSA0
 *	(Current Source Address 0)
 *****************************************************************************/
#define VIVO_DMACSA0                0x7b018
#define VIVO_DMACSA1                0x7c018
#define     VIVO_DMACSAX_CSA                    (0xffffffff<<0)

/*****************************************************************************
 *	Register descriptions for DMACSAU0
 *	(Current Source Address Upper 0)
 *****************************************************************************/
#define VIVO_DMACSAU0               0x7b01c
#define VIVO_DMACSAU1               0x7c01c
#define     VIVO_DMACSAUX_CSAU                  (0xffffffff<<0)

/*****************************************************************************
 *	Register descriptions for DMACDA0
 *	(Current Destination Address 0)
 *****************************************************************************/
#define VIVO_DMACDA0                0x7b020
#define VIVO_DMACDA1                0x7c020
#define     VIVO_DMACDAX_CDA                    (0xffffffff<<0)

/*****************************************************************************
 *	Register descriptions for DMACDAU0
 *	(Current Destination Address Upper 0)
 *****************************************************************************/
#define VIVO_DMACDAU0               0x7b024
#define VIVO_DMACDAU1               0x7c024
#define     VIVO_DMACDAUX_CDAU                  (0xffffffff<<0)

/*****************************************************************************
 *	Register descriptions for DMASA0
 *	(Source Address 0)
 *****************************************************************************/
#define VIVO_DMASA0                 0x7b030
#define VIVO_DMASA1                 0x7c030
#define     VIVO_DMASAX_SA                      (0x1fffffff<<3)

/*****************************************************************************
 *	Register descriptions for DMASAU0
 *	(Source Address Upper 0)
 *****************************************************************************/
#define VIVO_DMASAU0                0x7b034
#define VIVO_DMASAU1                0x7c034
#define     VIVO_DMASAUX_SAU                    (0xffffffff<<0)

/*****************************************************************************
 *	Register descriptions for DMADA0
 *	(Destination Address 0)
 *****************************************************************************/
#define VIVO_DMADA0                 0x7b038
#define VIVO_DMADA1                 0x7c038
#define     VIVO_DMADAX_DA                      (0x1fffffff<<3)

/*****************************************************************************
 *	Register descriptions for DMADAU0
 *	(Destination Address Upper 0)
 *****************************************************************************/
#define VIVO_DMADAU0                0x7b03c
#define VIVO_DMADAU1                0x7c03c
#define     VIVO_DMADAUX_DAU                    (0xffffffff<<0)

/*****************************************************************************
 *	Register descriptions for DMASATT0
 *	(Source Attributes 0)
 *****************************************************************************/
#define VIVO_DMASATT0               0x7b040
#define VIVO_DMASATT1               0x7c040
#define VIVO_DMASATTX_AM_DA                     (1<<0)
#define         VIVO_DMASATTX_AM_DA_DATA                (0<<0)
#define         VIVO_DMASATTX_AM_DA_PROGRAM             (1<<0)
#define VIVO_DMASATTX_AM_NPA                    (1<<1)
#define         VIVO_DMASATTX_AM_NPA_USER               (0<<1)
#define         VIVO_DMASATTX_AM_NPA_SUPER              (1<<1)
#define     VIVO_DMASATTX_AM_AS                 (7<<2)
#define         VIVO_DMASATTX_AM_AS_A16                 (0<<2)
#define         VIVO_DMASATTX_AM_AS_A24                 (1<<2)
#define         VIVO_DMASATTX_AM_AS_A32                 (2<<2)
#define         VIVO_DMASATTX_AM_AS_CRCSR               (3<<2)
#define         VIVO_DMASATTX_AM_AS_USER1               (4<<2)
#define         VIVO_DMASATTX_AM_AS_USER2               (5<<2)
#define     VIVO_DMASATTX_BT                    (7<<5)
#define         VIVO_DMASATTX_BT_SCT                    (0<<5)
#define         VIVO_DMASATTX_BT_BLT                    (1<<5)
#define         VIVO_DMASATTX_BT_MBLT                   (2<<5)
#define         VIVO_DMASATTX_BT_2eSST                  (4<<5)
#define         VIVO_DMASATTX_BT_2eSSTB                 (5<<5)
#define     VIVO_DMASATTX_SST                   (3<<8)
#define         VIVO_DMASATTX_SST_160                   (0<<8)
#define         VIVO_DMASATTX_SST_267                   (1<<8)
#define         VIVO_DMASATTX_SST_320                   (2<<8)
#define VIVO_DMASATTX_DPRT                      (1<<10)
#define         VIVO_DMASATTX_DPRT_PCISRC               (0<<10)
#define         VIVO_DMASATTX_DPRT_VMESRC               (1<<10)
#define VIVO_DMASATTX_FA                        (1<<11)

/*****************************************************************************
 *	Register descriptions for DMADATT0
 *	(Destination Attributes 0)
 *****************************************************************************/
#define VIVO_DMADATT0               0x7b044
#define VIVO_DMADATT1               0x7c044
#define VIVO_DMADATTX_AM_DA                     (1<<0)
#define         VIVO_DMADATTX_AM_DA_DATA                (0<<0)
#define         VIVO_DMADATTX_AM_DA_PROGRAM             (1<<0)
#define VIVO_DMADATTX_AM_NPA                    (1<<1)
#define         VIVO_DMADATTX_AM_NPA_USER               (0<<1)
#define         VIVO_DMADATTX_AM_NPA_SUPER              (1<<1)
#define     VIVO_DMADATTX_AM_AS                 (7<<2)
#define         VIVO_DMADATTX_AM_AS_A16                 (0<<2)
#define         VIVO_DMADATTX_AM_AS_A24                 (1<<2)
#define         VIVO_DMADATTX_AM_AS_A32                 (2<<2)
#define         VIVO_DMADATTX_AM_AS_CRCSR               (3<<2)
#define         VIVO_DMADATTX_AM_AS_USER1               (4<<2)
#define         VIVO_DMADATTX_AM_AS_USER2               (5<<2)
#define     VIVO_DMADATTX_BT                    (7<<5)
#define         VIVO_DMADATTX_BT_SCT                    (0<<5)
#define         VIVO_DMADATTX_BT_BLT                    (1<<5)
#define         VIVO_DMADATTX_BT_MBLT                   (2<<5)
#define         VIVO_DMADATTX_BT_2eSST                  (4<<5)
#define         VIVO_DMADATTX_BT_2eSSTB                 (5<<5)
#define     VIVO_DMADATTX_SST                   (3<<8)
#define         VIVO_DMADATTX_SST_160                   (0<<8)
#define         VIVO_DMADATTX_SST_267                   (1<<8)
#define         VIVO_DMADATTX_SST_320                   (2<<8)
#define VIVO_DMADATTX_DPRT                      (1<<10)
#define         VIVO_DMADATTX_DPRT_PCISRC               (0<<10)
#define         VIVO_DMADATTX_DPRT_VMESRC               (1<<10)
#define VIVO_DMADATTX_FA                        (1<<11)

/*****************************************************************************
 *	Register descriptions for DMANDESC0
 *	(Next Descriptor Address 0)
 *****************************************************************************/
#define VIVO_DMANDESC0              0x7b048
#define VIVO_DMANDESC1              0x7c048
#define VIVO_DMANDESCX_EOL                      (1<<0)
#define     VIVO_DMANDESCX_NDESC                (0x3ffffff<<6)

/*****************************************************************************
 *	Register descriptions for DMANDESCU0
 *	(Next Descriptor Address Upper 0)
 *****************************************************************************/
#define VIVO_DMANDESCU0             0x7b04c
#define VIVO_DMANDESCU1             0x7c04c
#define     VIVO_DMANDESCUX_NDESCU              (0xffffffff<<0)

/*****************************************************************************
 *	Register descriptions for DMABSS0
 *	(Broadcast Slave Select 0)
 *****************************************************************************/
#define VIVO_DMABSS0                0x7b050
#define VIVO_DMABSS1                0x7c050
#define     VIVO_DMABSSX_BSS                    (0x1fffff<<0)

/*****************************************************************************
 *	Register descriptions for DMATL0
 *	(Tranaction Length 0)
 *****************************************************************************/
#define VIVO_DMATL0                 0x7b054
#define VIVO_DMATL1                 0x7c054
#define     VIVO_DMATLX_TL                      (0xffffff<<0)

#endif /* VIVO_H */
