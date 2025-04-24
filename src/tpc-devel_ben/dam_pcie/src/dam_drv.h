#ifndef _DAM_DRV_H
#define _DAM_DRV_H

#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/version.h>

#define DRV_MODULE_NAME     "dam"
#define DRV_MODULE_VERSION  "2.1"
#define DRV_DESCRIPTION     "sPHENIX Data Aggregation Module PCIe Driver"

#define FIRST_MINOR   0
#define DRV_MOD_CLASS "dam"

// BAR Defines (MSI-X is searched for)
#define DAM_PCIE_WUPPER_BAR   0
#define DAM_PCIE_MSIX_INT_BAR 1
#define DAM_PCIE_CONTROL_BAR  2

#define MAX_MSIX           8  // Number of interrupts (MSI-X) per endpoint 
#define MAX_PCIE_ENDPOINTS 1  // Number of PCI-E endpoints
#define MAX_DMA_BUFFERS    4

//Register model
typedef struct
{
    volatile u_long start_address;        /*  low half, bits  63:00 */
    volatile u_long end_address;          /*  low half, bits 127:64 */
    volatile u_long tlp         :11;      /* high half, bits  10:00 */
    volatile u_long read        : 1;      /* high half, bit      11 */
    volatile u_long wrap_around : 1;      /* high half, bit      12 */
    volatile u_long reserved    :51;      /* high half, bits  63:13 */
    volatile u_long read_ptr;             /* high half, bits 127:64 */
} dma_descriptor_t;

typedef struct
{
    volatile u_long current_address;      /* bits  63:00 */
    volatile u_long descriptor_done : 1;  /* bit      64 */
    volatile u_long even_addr_dma   : 1;  /* bit      65 */
    volatile u_long even_addr_pc    : 1;  /* bit      66 */
} dma_status_t;

typedef struct
{
    volatile u_long THRESHOLD_NEGATE         :  7;  /* bits   6: 0 */
    volatile u_long unused0                  :  9;  /* bits  15: 7 */
    volatile u_long THRESHOLD_ASSERT         :  7;  /* bits  22:16 */
} dma_fromhost_full_thresh_t;

typedef struct
{
    volatile u_long THRESHOLD_NEGATE         : 12;  /* bits  11: 0 */
    volatile u_long unused0                  :  4;  /* bits  15:12 */
    volatile u_long THRESHOLD_ASSERT         : 12;  /* bits  27:16 */
} dma_tohost_full_thresh_t;

typedef struct
{
    volatile u_long TOHOST_BUSY              :  1;  /* bits   0: 0 */
    volatile u_long FROMHOST_BUSY            :  1;  /* bits   1: 1 */
} dma_busy_status_t;

typedef struct
{
    dma_descriptor_t DMA_DESC[8];         /* 0x000 - 0x0ff */
    u_char           unused1[256];        /* 0x100 - 0x1ff */
    dma_status_t     DMA_DESC_STATUS[8];  /* 0x200 - 0x27f */
    u_char           unused2[128];        /* 0x280 - 0x2ff */
    volatile u_int   BAR0_VALUE;          /* 0x300 - 0x303 */
    u_char           unused3[12];         /* 0x304 - 0x30f */
    volatile u_int   BAR1_VALUE;          /* 0x310 - 0x313 */
    u_char           unused4[12];         /* 0x314 - 0x31f */
    volatile u_int   BAR2_VALUE;          /* 0x320 - 0x323 */
    u_char           unused5[220];        /* 0x324 - 0x3ff */
    volatile u_int   DMA_DESC_ENABLE;     /* 0x400 - 0x403 */
    u_char           unused7[28];         /* 0x404 - 0x41f */  
    volatile u_int   DMA_RESET;           /* 0x420 - 0x423 */
    u_char           unused8[12];         /* 0x424 - 0x42f */
    volatile u_int   SOFT_RESET;          /* 0x430 - 0x433 */
    u_char           unused9[12];         /* 0x434 - 0x43f */
    volatile u_long                REGISTER_RESET;                /* 0x0440 - 0x0447 (8) */
    u_char                         unused10[8];                   /* 0x0448 - 0x044F (8) */

    dma_fromhost_full_thresh_t  FROMHOST_FULL_THRESH;             /* 0x0450 - 0x0457 (8) */
    u_char                         unused11[8];                   /* 0x0458 - 0x045F (8) */

    dma_tohost_full_thresh_t   TOHOST_FULL_THRESH;                /* 0x0460 - 0x0467 (8) */
    u_char                         unused12[8];                   /* 0x0468 - 0x046F (8) */

    volatile u_long                BUSY_THRESHOLD_ASSERT;         /* 0x0470 - 0x0477 (8) */
    u_char                         unused13[8];                   /* 0x0478 - 0x047F (8) */

    volatile u_long                BUSY_THRESHOLD_NEGATE;         /* 0x0480 - 0x0487 (8) */
    u_char                         unused14[8];                   /* 0x0488 - 0x048F (8) */

    dma_busy_status_t              BUSY_STATUS;                   /* 0x0490 - 0x0497 (8) */
    u_char                         unused15[8];                   /* 0x0498 - 0x049F (8) */

    volatile u_long                PC_PTR_GAP;                    /* 0x04A0 - 0x04A7 (8) */
    u_char                         unused16[8];                   /* 0x04A8 - 0x04AF (8) */
} wupper_regs_t;

int dam_pci_probe(struct pci_dev *pdev,
        const struct pci_device_id *entry);
void dam_pci_remove(struct pci_dev *pdev);
#ifdef CONFIG_PM
int dam_pci_suspend(struct pci_dev *pdev, pm_message_t state);
int dam_pci_resume(struct pci_dev *pdev);
#endif /* CONFIG_PM */

static const struct pci_device_id dam_pci_id_table[] = {
    { PCI_DEVICE(0x10dc, 0x0427) },
    { 0, },
};

static struct pci_driver dam_pci_driver = {
    .name = KBUILD_MODNAME,
    .probe = dam_pci_probe,
    .remove = dam_pci_remove,
    .id_table = dam_pci_id_table,
#ifdef CONFIG_PM
    .suspend = dam_pci_suspend,
    .resume = dam_pci_resume,
#endif /* CONFIG_PM */
};

enum FW_CHAN_TYPE {
    FW_CHAN_TYPE_OUT=0,
    FW_CHAN_TYPE_IN=1,
    FW_CHAN_TYPE_UNI_IN=2,
};

struct fw_channel {
    u32 offset;
    u32 size;
    u32 source;
    u32 type;
    spinlock_t lock;
    /* waitqueue for signaling completion */
    wait_queue_head_t wq;
    char *name;
};

struct dam_private {
    struct pci_dev *pdev;

    int msix_int_available;
    u32 msix_enable_offset;

    unsigned int dma_mask;

    struct mutex ioctl_lock;
    /* lock for synchronizing with irq/workqueue */
    spinlock_t io_lock;

    int users;

    /* Mapped PCI resources */
    void __iomem *wupper_io;
    u32 wupper_io_len;

    void __iomem *msix_mem;
    u32 msix_mem_len;

    void __iomem *pcie_io;
    u32 pcie_io_len;
    uint64_t pcie_io_baseaddr;

    struct work_struct irq_work;

    /* Hardware info */
    u32 flx_model;
    u32 reg_map;
    u32 block_size;

    /* Root resource for memory management */
    struct resource *mem;
    /* Resource for managing IO mmu slots */
    struct resource *iommu;

    /* Firmware channels */
    int num_channels;

    struct list_head buffer_queue;

    uint32_t   *kbuf_dma;  // CPU side access
    dma_addr_t dma_addr;   // Hardware (PL) side access
    size_t     dma_size;   // PAGE_SIZE aligned
    uint32_t   dma_offset; // Copy offset

    struct dentry *debugfs;
};


#endif /* _DAM_DRV_H */
