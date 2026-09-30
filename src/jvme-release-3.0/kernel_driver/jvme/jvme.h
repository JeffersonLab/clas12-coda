#pragma once

#define JVME_BUS_MAX	1

/*
 * VMEbus Master Window Configuration Structure
 */
struct vme_master
{
  int enable;			/* State of Window */
  unsigned long long vme_addr;	/* Starting Address on the VMEbus */
  unsigned long long size;	/* Window Size */
  u32 aspace;			/* Address Space */
  u32 cycle;			/* Cycle properties */
  u32 dwidth;			/* Maximum Data Width */
#if 0
  char prefetchEnable;		/* Prefetch Read Enable State */
  int prefetchSize;		/* Prefetch Read Size (Cache Lines) */
  char wrPostEnable;		/* Write Post State */
#endif
};


/*
 * IOCTL Commands and structures
 */

/* Magic number for use in ioctls */
#define VME_IOC_MAGIC 0xAE


/* VMEbus Slave Window Configuration Structure */
struct vme_slave
{
  int enable;			/* State of Window */
  unsigned long long vme_addr;	/* Starting Address on the VMEbus */
  unsigned long long pci_addr;	/* Starting Address on the VMEbus */
  unsigned long long size;	/* Window Size */
  u32 aspace;			/* Address Space */
  u32 cycle;			/* Cycle properties */
#if 0
  char wrPostEnable;		/* Write Post State */
  char rmwLock;			/* Lock PCI during RMW Cycles */
  char data64BitCapable;	/* non-VMEbus capable of 64-bit Data */
#endif
};

struct vme_irq_id
{
  __u8 level;
  __u8 statid;
};

struct vme_bridge_id
{
  int deviceId;
  uint32_t mapSize;
};

typedef struct
{
	unsigned long req_route;
	unsigned long xfer_route;
	unsigned long long size;
	unsigned long dwidth;
	unsigned long aspace;
	unsigned long long addr;
	unsigned long cycle;
	unsigned long *buffer;
} dma_t;

typedef struct
{
	int level;
	int statid;
	struct timeval tv;
	struct task_struct *task;
	int timedout;
	int monitor;
} vme_irq_t;

typedef struct
{
	unsigned long count;
	unsigned long long addr;
	unsigned int aspace;
	unsigned int cycle;
} lm_t;



#define VME_GET_SLAVE			_IOR(VME_IOC_MAGIC, 0x01, struct vme_slave)
#define VME_SET_SLAVE			_IOW(VME_IOC_MAGIC, 0x02, struct vme_slave)
#define VME_GET_MASTER			_IOR(VME_IOC_MAGIC, 0x03, struct vme_master)
#define VME_SET_MASTER			_IOW(VME_IOC_MAGIC, 0x04, struct vme_master)
#define VME_IRQ_GEN			_IOW(VME_IOC_MAGIC, 0x05, struct vme_irq_id)
#define VME_GET_BRIDGE_INFO		_IOR(VME_IOC_MAGIC, 0x06, struct vme_bridge_id)
#define VME_DMA_REQ			_IOWR(VME_IOC_MAGIC, 0x0C, dma_t)
#define VME_DMA_REMOVE			_IO(VME_IOC_MAGIC, 0x0D)
#define VME_DMA_NEW_LIST		_IO(VME_IOC_MAGIC, 0x0E)
#define VME_DMA_FREE_LIST		_IO(VME_IOC_MAGIC, 0x0F)
#define VME_DMA_ATTR_SET		_IOR(VME_IOC_MAGIC, 0x10, dma_t)
#define VME_DMA_FREE_ATTR		_IO(VME_IOC_MAGIC, 0x11)
#define VME_DMA_XFER			_IOWR(VME_IOC_MAGIC, 0x12, dma_t)
#define VME_WAIT_IRQ			_IOWR(VME_IOC_MAGIC, 0x13, vme_irq_t)
#define VME_REM_IRQ			_IOWR(VME_IOC_MAGIC, 0x14, vme_irq_t)
#define VME_GEN_IRQ			_IOWR(VME_IOC_MAGIC, 0x15, vme_irq_t)
