/*
 * JLab VME driver
 *
 *  Author: Bryan Moffit <moffit@jlab.org>
 *          Copyright (c) 2018 Southeastern Universities Research Association,
 *                             Thomas Jefferson National Accelerator Facility
 *
 * Based on work by:
 *   Tom Armistead and Ajit Prem
 *     Copyright 2004 Motorola Inc.
 *   Martyn Welch <martyn.welch@ge.com>
 *     Copyright 2008 GE Intelligent Platforms Embedded Systems, Inc.
 *
 *
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/pci.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/syscalls.h>
#include <linux/types.h>

#include <linux/io.h>
#include <linux/uaccess.h>
/* #include <linux/vme.h> */
#include "vme.h"
#include "vme_bridge.h"
#include "jvme.h"

static DEFINE_MUTEX(jvme_mutex);
static const char driver_name[] = "jvme";

#define JVME_NAME	"jvme"

struct jvme_type {
	int openCnt;
	/* mast_wnd_t mast; */
	struct vme_dev *pVmeDev;
	struct vme_resource *pMastRes;
	dma_addr_t mast_pci_buf;
	void __iomem *mast_kern_buf;
	struct vme_resource *pSlaveRes;
	/* slave_wnd_t slave; */
	dma_addr_t slave_pci_buf;
	void __iomem *slave_kern_buf;
	vme_irq_t vme_irq;
	dma_t dma;
	struct vme_resource *pDMARes;
	struct vme_dma_list *dma_list;
	struct vme_dma_attr *dma_local;
	struct vme_dma_attr *dma_remote;
	void __iomem *dma_kern_buf;
	dma_addr_t dma_pci_buf;
	struct vme_resource *pLMRes;
	struct vme_error_handler *pMastErrHdl;
	struct vme_error_handler *pDmaErrHdl;
	lm_t lm;
};

/* Module parameters */
static bool err_chk;
static int slave_enable = 1;

module_param(slave_enable, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(slave_enable,
		 "1 = enable slave window   0 = disable slave window");

static struct jvme_type jvme_data;

/* For now, defining our own device naming scheme... trying to be more descriptive.
 *
 * 221 char	VME bus
 *		  0 = /dev/bus/vme/m_a16	A16 master image
 *		  1 = /dev/bus/vme/m_a24	A24 master image
 *		  2 = /dev/bus/vme/m_a32	A32 master image
 *		  3 = /dev/bus/vme/m_crcsr	CRCSR master image
 *		  4 = /dev/bus/vme/s_a32	A32 slave image
 *		  5 = /dev/bus/vme/s_rsvd1	Unused slave image
 *		  6 = /dev/bus/vme/s_rsvd2	Unused slave image
 *		  7 = /dev/bus/vme/s_rsvd3	Unused slave image
 *		  8 = /dev/bus/vme/ctl		Control
 *
 */

#define VME_MAJOR	221	/* VME Major Device Number */
#define VME_DEVS	9	/* Number of dev entries */

#define MASTER_MINOR	0
#define MASTER_MAX	3
#define SLAVE_MINOR	4
#define SLAVE_MAX	7
#define CONTROL_MINOR	8

#define PCI_BUF_SIZE  0x20000	/* Size of one slave image buffer */

/* JLab default windows */

#define JLAB_MASTER_IMAGES 4
static struct vme_master jlab_master[JLAB_MASTER_IMAGES] = {
	{			/* A16 */
	 .enable = 1,
	 .vme_addr = 0x0ULL,
	 .size = 0x00010000ULL,
	 .aspace = VME_A16,
	 .cycle = VME_SCT | VME_USER | VME_DATA,
	 .dwidth = VME_D32,
	 },
	{			/* A24 */
	 .enable = 1,
	 .vme_addr = 0x0ULL,
	 .size = 0x01000000ULL,
	 .aspace = VME_A24,
	 .cycle = VME_SCT | VME_USER | VME_DATA,
	 .dwidth = VME_D32,
	 },
	{			/* A32 */
	 .enable = 1,
	 .vme_addr = 0x08000000ULL,
	 .size = 0x0A000000ULL,
	 .aspace = VME_A32,
	 .cycle = VME_SCT | VME_USER | VME_DATA,
	 .dwidth = VME_D32,
	 },
	{			/* CRCSR */
	 .enable = 1,
	 .vme_addr = 0x0ULL,
	 .size = 0x01000000ULL,
	 .aspace = VME_CRCSR,
	 .cycle = VME_SCT | VME_USER | VME_DATA,
	 .dwidth = VME_D32,
	 },
};

static char jlab_master_names[JLAB_MASTER_IMAGES][24] = {
	"bus/vme/m_a16",
	"bus/vme/m_a24",
	"bus/vme/m_a32",
	"bus/vme/m_crcsr",
};

#define JLAB_SLAVE_IMAGES 1
static struct vme_slave jlab_slave[JLAB_SLAVE_IMAGES] = {
	{
	 .enable = 1,
	 .vme_addr = 0x18000000ULL,
	 .size = 0x00400000ULL,
	 .aspace = VME_A32,
	 .cycle = VME_MBLT | VME_SCT | VME_USER | VME_PROG,
	 },
};

static char jlab_slave_names[JLAB_SLAVE_IMAGES][24] = {
	"bus/vme/s_a32",
};

/*
 * Structure to handle image related parameters.
 */
struct image_desc {
	void *kern_buf;		/* Buffer address in kernel space */
	dma_addr_t pci_buf;	/* Buffer address in PCI address space */
	unsigned long long size_buf;	/* Buffer size */
	struct mutex mutex;	/* Mutex for locking image */
	struct device *device;	/* Sysfs device */
	struct vme_resource *resource;	/* VME resource */
	int users;		/* Number of current users */
};
static struct image_desc image[VME_DEVS];

struct driver_stats {
	u_long reads;
	u_long writes;
	u_long ioctls;
	u_long irqs;
	u_long berrs;
	u_long dmaErrors;
	u_long timeouts;
	u_long external;
};
static struct driver_stats statistics;

static struct cdev *jvme_cdev;	/* Character device */
static struct class *jvme_sysfs_class;	/* Sysfs class */
static struct vme_dev *jvme_bridge;	/* Pointer to user device */

static const int type[VME_DEVS] = { MASTER_MINOR, MASTER_MINOR,
	MASTER_MINOR, MASTER_MINOR,
	SLAVE_MINOR, SLAVE_MINOR,
	SLAVE_MINOR, SLAVE_MINOR,
	CONTROL_MINOR
};

static int jvme_open(struct inode *, struct file *);
static int jvme_release(struct inode *, struct file *);
static ssize_t jvme_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t jvme_write(struct file *, const char __user *, size_t, loff_t *);
static loff_t jvme_llseek(struct file *, loff_t, int);
static int jvme_mmap(struct file *file, struct vm_area_struct *vma);
static long jvme_unlocked_ioctl(struct file *, unsigned int, u_long);

static int jvme_match(struct vme_dev *);
static int jvme_probe(struct vme_dev *);
static int jvme_remove(struct vme_dev *);

static const struct file_operations jvme_fops = {
	.open = jvme_open,
	.release = jvme_release,
	.read = jvme_read,
	.write = jvme_write,
	.llseek = jvme_llseek,
	.unlocked_ioctl = jvme_unlocked_ioctl,
	.mmap = jvme_mmap,
};

/*
 * Reset all the statistic counters
 */
static void reset_counters(void)
{
	statistics.reads = 0;
	statistics.writes = 0;
	statistics.ioctls = 0;
	statistics.irqs = 0;
	statistics.berrs = 0;
	statistics.dmaErrors = 0;
	statistics.timeouts = 0;
}

static void jvme_dma_display(dma_t dma, char *msg)
{
	printk(JVME_NAME ": vme_dma_display: %s\n", msg);
	printk(JVME_NAME ": vme_dma_display: route: 0x%x\n",
	       (int)dma.req_route);
	printk(JVME_NAME ": vme_dma_display: address: 0x%llx\n", dma.addr);
	printk(JVME_NAME ": vme_dma_display: size: 0x%x\n", (int)dma.size);
	printk(JVME_NAME ": vme_dma_display: aspace: 0x%x\n", (int)dma.aspace);
	printk(JVME_NAME ": vme_dma_display: cycle: 0x%x\n", (int)dma.cycle);
	printk(JVME_NAME ": vme_dma_display: data width: 0x%x\n",
	       (int)dma.dwidth);
	printk(JVME_NAME ": vme_dma_display: buffer: 0x%p\n", dma.buffer);

	if(jvme_data.pDmaErrHdl) {

		printk(JVME_NAME ": vme_dma_display: Hdlr start: 0x%llx\n",
		       jvme_data.pDmaErrHdl->start);
		printk(JVME_NAME ": vme_dma_display: Hdlr end: 0x%llx\n",
		       jvme_data.pDmaErrHdl->end);
		printk(JVME_NAME
		       ": vme_dma_display: Hdlr first_error: 0x%llx\n",
		       jvme_data.pDmaErrHdl->first_error);
		printk(JVME_NAME ": vme_dma_display: Hdlr aspace: 0x%x\n",
		       jvme_data.pDmaErrHdl->aspace);
		printk(JVME_NAME ": vme_dma_display: Hdlr num_errors: 0x%x\n",
		       jvme_data.pDmaErrHdl->num_errors);
	}
}

static int jvme_open(struct inode *inode, struct file *file)
{
	int err;
	unsigned int minor = MINOR(inode->i_rdev);

	mutex_lock(&image[minor].mutex);
	/* Allow device to be opened if a resource is needed and allocated. */
	if(minor < CONTROL_MINOR && image[minor].resource == NULL) {
		pr_err("No resources allocated for device\n");
		err = -EINVAL;
		goto err_res;
	}

	/* Increment user count */
	image[minor].users++;

	mutex_unlock(&image[minor].mutex);

	return 0;

 err_res:
	mutex_unlock(&image[minor].mutex);

	return err;
}

static int jvme_release(struct inode *inode, struct file *file)
{
	unsigned int minor = MINOR(inode->i_rdev);

	mutex_lock(&image[minor].mutex);

	/* Decrement user count */
	image[minor].users--;

	mutex_unlock(&image[minor].mutex);

	return 0;
}

/*
 * We are going ot alloc a page during init per window for small transfers.
 * Small transfers will go VME -> buffer -> user space. Larger (more than a
 * page) transfers will lock the user space buffer into memory and then
 * transfer the data directly into the user space buffers.
 */
static ssize_t
resource_to_user(int minor, char __user * buf, size_t count, loff_t * ppos)
{
	ssize_t retval;
	ssize_t copied = 0;

	if(count <= image[minor].size_buf) {
		/* We copy to kernel buffer */
		copied = vme_master_read(image[minor].resource,
					 image[minor].kern_buf, count, *ppos);
		if(copied < 0)
			return (int)copied;

		retval = __copy_to_user(buf, image[minor].kern_buf,
					(u_long) copied);
		if(retval != 0) {
			copied = (copied - retval);
			pr_info("User copy failed\n");
			return -EINVAL;
		}

	} else {
		/* XXX Need to write this */
		pr_info("Currently don't support large transfers\n");
		/* Map in pages from userspace */

		/* Call vme_master_read to do the transfer */
		return -EINVAL;
	}

	return copied;
}

/*
 * We are going to alloc a page during init per window for small transfers.
 * Small transfers will go user space -> buffer -> VME. Larger (more than a
 * page) transfers will lock the user space buffer into memory and then
 * transfer the data directly from the user space buffers out to VME.
 */
static ssize_t
resource_from_user(unsigned int minor, const char __user * buf,
		   size_t count, loff_t * ppos)
{
	ssize_t retval;
	ssize_t copied = 0;

	if(count <= image[minor].size_buf) {
		retval = __copy_from_user(image[minor].kern_buf, buf,
					  (u_long) count);
		if(retval != 0)
			copied = (copied - retval);
		else
			copied = count;

		copied = vme_master_write(image[minor].resource,
					  image[minor].kern_buf, copied, *ppos);
	} else {
		/* XXX Need to write this */
		pr_info("Currently don't support large transfers\n");
		/* Map in pages from userspace */

		/* Call vme_master_write to do the transfer */
		return -EINVAL;
	}

	return copied;
}

static ssize_t
buffer_to_user(unsigned int minor, char __user * buf,
	       size_t count, loff_t * ppos)
{
	void *image_ptr;
	ssize_t retval;

	image_ptr = image[minor].kern_buf + *ppos;

	retval = __copy_to_user(buf, image_ptr, (u_long) count);
	if(retval != 0) {
		retval = (count - retval);
		pr_warn("Partial copy to userspace\n");
	} else
		retval = count;

	/* Return number of bytes successfully read */
	return retval;
}

static ssize_t
buffer_from_user(unsigned int minor, const char __user * buf,
		 size_t count, loff_t * ppos)
{
	void *image_ptr;
	size_t retval;

	image_ptr = image[minor].kern_buf + *ppos;

	retval = __copy_from_user(image_ptr, buf, (u_long) count);
	if(retval != 0) {
		retval = (count - retval);
		pr_warn("Partial copy to userspace\n");
	} else
		retval = count;

	/* Return number of bytes successfully read */
	return retval;
}

static ssize_t
jvme_read(struct file *file, char __user * buf, size_t count, loff_t * ppos)
{
	unsigned int minor = MINOR(file_inode(file)->i_rdev);
	ssize_t retval;
	size_t image_size;
	size_t okcount;

	mutex_lock(&image[minor].mutex);

	if(minor == CONTROL_MINOR) {
		image_size = image[minor].size_buf;
	} else {
		/* XXX Do we *really* want this helper - we can use vme_*_get ? */
		image_size = vme_get_size(image[minor].resource);
	}

	/* Ensure we are starting at a valid location */
	if((*ppos < 0) || (*ppos > (image_size - 1))) {
		mutex_unlock(&image[minor].mutex);
		return 0;
	}

	/* Ensure not reading past end of the image */
	if(*ppos + count > image_size)
		okcount = image_size - *ppos;
	else
		okcount = count;

	switch (type[minor]) {
	case MASTER_MINOR:
		retval = resource_to_user(minor, buf, okcount, ppos);
		break;
	case SLAVE_MINOR:
	case CONTROL_MINOR:
		retval = buffer_to_user(minor, buf, okcount, ppos);
		break;
	default:
		retval = -EINVAL;
	}

	mutex_unlock(&image[minor].mutex);
	if(retval > 0)
		*ppos += retval;

	return retval;
}

static ssize_t
jvme_write(struct file *file, const char __user * buf,
	   size_t count, loff_t * ppos)
{
	unsigned int minor = MINOR(file_inode(file)->i_rdev);
	ssize_t retval;
	size_t image_size;
	size_t okcount;

	if(minor == CONTROL_MINOR)
		return 0;

	mutex_lock(&image[minor].mutex);

	image_size = vme_get_size(image[minor].resource);

	/* Ensure we are starting at a valid location */
	if((*ppos < 0) || (*ppos > (image_size - 1))) {
		mutex_unlock(&image[minor].mutex);
		return 0;
	}

	/* Ensure not reading past end of the image */
	if(*ppos + count > image_size)
		okcount = image_size - *ppos;
	else
		okcount = count;

	switch (type[minor]) {
	case MASTER_MINOR:
		retval = resource_from_user(minor, buf, okcount, ppos);
		break;
	case SLAVE_MINOR:
		retval = buffer_from_user(minor, buf, okcount, ppos);
		break;
	default:
		retval = -EINVAL;
	}

	mutex_unlock(&image[minor].mutex);

	if(retval > 0)
		*ppos += retval;

	return retval;
}

static loff_t jvme_llseek(struct file *file, loff_t off, int whence)
{
	loff_t absolute = -1;
	unsigned int minor = MINOR(file_inode(file)->i_rdev);
	size_t image_size;

	mutex_lock(&image[minor].mutex);

	if(minor == CONTROL_MINOR) {
		image_size = image[minor].size_buf;
	} else {
		image_size = vme_get_size(image[minor].resource);
	}

	switch (whence) {
	case SEEK_SET:
		absolute = off;
		break;
	case SEEK_CUR:
		absolute = file->f_pos + off;
		break;
	case SEEK_END:
		absolute = image_size + off;
		break;
	default:
		mutex_unlock(&image[minor].mutex);
		return -EINVAL;
		break;
	}

	if((absolute < 0) || (absolute >= image_size)) {
		mutex_unlock(&image[minor].mutex);
		return -EINVAL;
	}

	file->f_pos = absolute;

	mutex_unlock(&image[minor].mutex);

	return absolute;
}

static int jvme_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned int minor = MINOR(file_inode(file)->i_rdev);
#ifndef VM_RESERVED
#define VM_RESERVED (VM_DONTEXPAND | VM_DONTDUMP)
#endif
	vma->vm_flags |= VM_RESERVED | VM_IO;

	if(minor == CONTROL_MINOR)
		return (vme_bridge_mmap(jvme_bridge, vma));

	if(minor == SLAVE_MINOR) {
		return (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
					vma->vm_end - vma->vm_start,
					vma->vm_page_prot));
	}

	return (vme_master_mmap(image[minor].resource, vma));
}

static int jvme_dma_request(u_long arg)
{
	if(jvme_bridge == 0) {
		pr_err(" jvme_dma_request: no VME device\n");
		return -1;
	}

	if(jvme_data.pDMARes) {
		vme_dma_free(jvme_data.pDMARes);
		pr_debug("jvme_dma_request: freed dma resource\n");
		jvme_data.pDMARes = 0;
	}

	if(arg == 0) {
		pr_err(" VME_DMA_REQUEST: argument is NULL\n");
		return -1;
	}

	if(copy_from_user(&(jvme_data.dma), (void *)arg, sizeof(dma_t))) {
		pr_err(" jvme_dma_request: failed to copy data from user\n");
		return -1;
	}

	jvme_data.pDMARes =
	    vme_dma_request(jvme_bridge, jvme_data.dma.req_route);
	if(jvme_data.pDMARes == 0) {
		pr_err(" failed to get dma resource\n");
		return -1;
	}

	pr_debug(" got dma resource\n");

	return 0;
}

int jvme_dma_request_ext(u_long * dmares)
{
	if(jvme_bridge == 0) {
		pr_err(" jvme_dma_request: no VME device\n");
		return -1;
	}

	if(jvme_data.pDMARes) {
		vme_dma_free(jvme_data.pDMARes);
		pr_debug("jvme_dma_request: freed dma resource\n");
		jvme_data.pDMARes = 0;
	}

	jvme_data.dma.req_route = VME_DMA_VME_TO_MEM;
	jvme_data.dma.size = 4 * 1024 * 1024;

	jvme_data.pDMARes =
	    vme_dma_request(jvme_bridge, jvme_data.dma.req_route);
	if(jvme_data.pDMARes == 0) {
		pr_err(" failed to get dma resource\n");
		return -1;
	}

	*dmares = (u_long) & jvme_data.pDMARes;

	pr_debug("got dma resource\n");

	return 0;
}

EXPORT_SYMBOL(jvme_dma_request_ext);

int jvme_dma_remove_ext(u_long dmares)
{
	if(jvme_data.pDMARes == 0) {
		pr_err("jvme_dma_remove: no dma resource\n");
		return -1;
	}
	if(jvme_data.dma_kern_buf) {
		vme_free_consistent(jvme_data.pDMARes,
				    jvme_data.dma.size,
				    jvme_data.dma_kern_buf,
				    jvme_data.dma_pci_buf);
		jvme_data.dma_pci_buf = 0;
		jvme_data.dma_kern_buf = 0;
	}
	// maybe doesn't matter
	if(dmares != (u_long) & jvme_data.pDMARes) {
		pr_err(" dmares != jvme_data.pDMARes\n");
		pr_err(" 0x%lx != 0x%lx\n", dmares,
		       (u_long) & jvme_data.pDMARes);
	}

	vme_dma_free(jvme_data.pDMARes);
	pr_debug(" freed dma resource\n");
	jvme_data.pDMARes = 0;

	return 0;
}

EXPORT_SYMBOL(jvme_dma_remove_ext);

static int jvme_dma_remove(void)
{
	if(jvme_data.pDMARes == 0) {
		pr_debug(" jvme_dma_remove: no dma resource\n");
		return -1;
	}

	if(jvme_data.dma_kern_buf) {
		vme_free_consistent(jvme_data.pDMARes,
				    jvme_data.dma.size,
				    jvme_data.dma_kern_buf,
				    jvme_data.dma_pci_buf);
		jvme_data.dma_pci_buf = 0;
		jvme_data.dma_kern_buf = 0;
	}

	vme_dma_free(jvme_data.pDMARes);
	pr_debug(" freed dma resource\n");
	jvme_data.pDMARes = 0;

	return 0;
}

void *jvme_dma_alloc_consistent(size_t size, dma_addr_t * dma)
{
#ifdef RESERVEPAGES
	struct page *page_ptr;
#endif
	void *kern_buf = vme_alloc_consistent(jvme_data.pDMARes, size, dma);
	jvme_data.dma_local = vme_dma_pci_attribute(jvme_data.dma_pci_buf);
#ifdef RESERVEPAGES
	for(page_ptr = virt_to_page(kern_buf);
	    page_ptr <= virt_to_page(kern_buf + size - 1); ++page_ptr) {
		SetPageReserved(page_ptr);
	}
#endif

	pr_debug(" kernel buffer 0x%p, pci buffer 0x%lx size = 0x%x\n",
		 kern_buf, (u_long) * dma, (int)size);

	return kern_buf;
}

EXPORT_SYMBOL(jvme_dma_alloc_consistent);

void jvme_dma_free_consistent(size_t size, void *vaddr, dma_addr_t dma)
{
#ifdef RESERVEPAGES
	struct page *page_ptr;
	for(page_ptr = virt_to_page(vaddr);
	    page_ptr <= virt_to_page(vaddr + size - 1); ++page_ptr) {
		ClearPageReserved(page_ptr);
	}
#endif
	vme_free_consistent(jvme_data.pDMARes, size, vaddr, dma);
	pr_debug(" freed dma kernel buffer\n");
}

EXPORT_SYMBOL(jvme_dma_free_consistent);

static int jvme_dma_new_list(void)
{
	if(jvme_bridge == 0) {
		pr_err(" jvme_dma_new_list: no VME device\n");
		return -1;
	}

	if(jvme_data.pDMARes == 0) {
		pr_err(" jvme_dma_new_list: no dma resource\n");
		return -1;
	}

	if(jvme_data.dma_list) {
		vme_dma_list_free(jvme_data.dma_list);
		jvme_data.dma_list = 0;
		pr_debug(" freed dma list\n");
	}

	jvme_data.dma_list = vme_new_dma_list(jvme_data.pDMARes);
	if(jvme_data.dma_list == 0) {
		pr_err(" Failed to get new dma list\n");
		return -1;
	} else
		pr_debug(" got new dma list\n");

	return 0;
}

static int jvme_dma_free_list(void)
{
	if(jvme_data.dma_list) {
		vme_dma_list_free(jvme_data.dma_list);
		jvme_data.dma_list = 0;
		pr_debug(" freed dma list\n");
		return 0;
	} else {
		pr_err(" no dma list to free\n");
		return -1;
	}
}

static int jvme_dma_set_attr(u_long arg)
{
	if(jvme_bridge == 0) {
		pr_err(" jvme_dma_set_attr: no VME device\n");
		return -1;
	}

	if(jvme_data.pDMARes == 0) {
		pr_err(" jvme_dma_set_attr: no dma resource\n");
		return -1;
	}

	if(jvme_data.dma_remote) {
		vme_dma_free_attribute(jvme_data.dma_remote);
		jvme_data.dma_remote = 0;
		pr_debug(" freed dma vme attribute\n");
	}

	if(jvme_data.dma_kern_buf) {
		vme_free_consistent(jvme_data.pDMARes,
				    jvme_data.dma.size,
				    jvme_data.dma_kern_buf,
				    jvme_data.dma_pci_buf);
		jvme_data.dma_pci_buf = 0;
		jvme_data.dma_kern_buf = 0;
		pr_debug(" freed dma kernel buffer\n");
	}

	if(arg == 0) {
		pr_err(" VME_DMA_SET_ATTR: argument is NULL\n");
		return -1;
	}

	if(copy_from_user(&(jvme_data.dma), (void *)arg, sizeof(dma_t))) {
		pr_err("jvme_dma_set_attr: failed to copy data from user\n");
		return -1;
	}

	jvme_data.dma_remote =
	    vme_dma_vme_attribute(jvme_data.dma.addr,
				  jvme_data.dma.aspace,
				  jvme_data.dma.cycle, jvme_data.dma.dwidth);
	if(jvme_data.dma_remote == NULL) {
		pr_err(" jvme_dma_set_attr: failed to set dma vme attribute\n");
		return -1;
	}
	pr_debug(" set dma attributes\n");

	jvme_data.dma_kern_buf =
	    vme_alloc_consistent(jvme_data.pDMARes,
				 jvme_data.dma.size, &jvme_data.dma_pci_buf);
	pr_debug(" kernel buffer 0x%p, pci buffer 0x%x\n",
		 jvme_data.dma_kern_buf, (int)jvme_data.dma_pci_buf);
	if(jvme_data.dma_kern_buf) {
		jvme_data.dma_local =
		    vme_dma_pci_attribute(jvme_data.dma_pci_buf);
		if(jvme_data.dma_local == NULL) {
			pr_err
			    ("jvme_dma_set_attr: failed to set dma pci attribute\n");
			return -1;
		}

		jvme_dma_display(jvme_data.dma, "DMA SETTINGS");

		if(err_chk) {

			if(jvme_data.pDmaErrHdl) {
				vme_unregister_error_handler(jvme_data.pDmaErrHdl);
				jvme_data.pDmaErrHdl = 0;
			}

			jvme_data.pDmaErrHdl =
			    vme_register_error_handler(jvme_bridge->bridge,
						       jvme_data.dma.aspace,
						       jvme_data.dma.addr,
						       jvme_data.dma.size);
		}

		return 0;
	}

	pr_err("jvme_dma_set_attr: failed to allocate pci attributes\n");

	return -1;
}

static int jvme_dma_free_attr(void)
{
	if(jvme_data.dma_remote == 0) {
		pr_err(" jvme_dma_free_attr: no dma vme attribute to free\n");
		return -1;
	}

	jvme_dma_display(jvme_data.dma, "DMA FREE ATTRIBUTES");

	if(err_chk && jvme_data.pDmaErrHdl) {
		vme_unregister_error_handler(jvme_data.pDmaErrHdl);
		jvme_data.pDmaErrHdl = 0;
	}

	vme_dma_free_attribute(jvme_data.dma_remote);
	jvme_data.dma_remote = 0;
	pr_debug(" freed dma vme attribute\n");

	if(jvme_data.dma_local) {
		vme_dma_free_attribute(jvme_data.dma_local);
		jvme_data.dma_local = 0;
		pr_debug(" freed dma pci attribute\n");
	}

	return 0;
}

static int jvme_dma_xfer(u_long arg)
{
	int ret = 0;

	if(jvme_data.dma_list == 0) {
		pr_err(" jvme_dma_xfer: no dma list\n");
		return -1;
	}

	if(jvme_data.dma_remote == 0) {
		pr_err(" jvme_dma_xfer: no dma vme attributes\n");
		return -1;
	}

	if(jvme_data.dma_local == 0) {
		pr_err(" jvme_dma_xfer: no dma pci attributes\n");
		return -1;
	}

	if(arg == 0) {
		pr_err(" jvme_dma_xfer: argument is NULL\n");
		return -1;
	}

	if(copy_from_user(&(jvme_data.dma), (void *)arg, sizeof(dma_t))) {
		pr_err("jvme_dma_xfer: failed to copy dma data from user\n");
		return -1;
	}

	if(copy_from_user((void *)jvme_data.dma_kern_buf,
			  (void *)jvme_data.dma.buffer, jvme_data.dma.size)) {
		pr_err(" jvme_dma_xfer: failed to copy data from user\n");
		return -1;
	}

	if(jvme_data.dma.xfer_route == VME_DMA_VME_TO_MEM) {
		ret = vme_dma_list_add(jvme_data.dma_list,
				       jvme_data.dma_remote,
				       jvme_data.dma_local, jvme_data.dma.size);
		pr_debug(" jvme_dma_xfer: Add List: VME_DMA_VME_TO_MEM\n");
	} else {
		ret = vme_dma_list_add(jvme_data.dma_list,
				       jvme_data.dma_local,
				       jvme_data.dma_remote,
				       jvme_data.dma.size);
		pr_debug(" jvme_dma_xfer: Add List: VME_DMA_MEM_TO_VME\n");
	}

	if(ret != 0) {
		pr_err(" jvme_dma_xfer: failed to add dma list\n");
		return -1;
	}

	ret = vme_dma_list_exec(jvme_data.dma_list);
	if(ret != 0) {
		pr_err(" jvme_dma_xfer: failed to execute dma list\n");
#if 0
		// want to see what is transferred regardless of return value
		return -1;
#endif
	}

	if(jvme_data.dma.xfer_route == VME_DMA_VME_TO_MEM) {
		if(copy_to_user((void *)jvme_data.dma.buffer,
				(void *)jvme_data.dma_kern_buf,
				jvme_data.dma.size)) {
			pr_err(" jvme_dma_xfer: failed to copy data to user\n");
			return -1;
		}
		pr_debug(" jvme_dma_xfer: VME_DMA_VME_TO_MEM transfer\n");
	} else {
		pr_debug(" jvme_dma_xfer: VME_DMA_MEM_TO_VME transfer\n");
	}

	return 0;
}

static void jvme_callback(int level, int statid, void *pData)
{
	pr_debug(" jvme_callback: Interrupt Callback\n");
	wake_up_process(jvme_data.vme_irq.task);
}

static int jvme_wait_for_interrupt(u_long arg)
{
	u_long timeout, us;
	int ret = 0;

	if(copy_from_user(&(jvme_data.vme_irq), (void *)arg, sizeof(vme_irq_t))) {
		pr_err
		    (" jvme_wait_for_interrupt: failed to copy data from user\n");
		return -1;
	}

	us = (jvme_data.vme_irq.tv.tv_usec * HZ) / 1000000UL;
	timeout = (jvme_data.vme_irq.tv.tv_sec * HZ);
	timeout = timeout + us;

	pr_debug(" HZ = 0x%x\n", HZ);
	pr_debug(" Seconds 0x%lx\n", jvme_data.vme_irq.tv.tv_sec);
	pr_debug(" Microseconds 0x%lx\n", jvme_data.vme_irq.tv.tv_usec);
	pr_debug(" Jiffies 0x%lx\n", timeout);
	pr_debug(" IRQ Level 0x%x\n", jvme_data.vme_irq.level);
	pr_debug(" IRQ Vector 0x%x\n", jvme_data.vme_irq.statid);

	if(timeout == 0)
		timeout = 1;
	timeout &= 0x7FFFFFFF;

	ret = vme_irq_request(jvme_data.pVmeDev,
			      jvme_data.vme_irq.level,
			      jvme_data.vme_irq.statid, jvme_callback, NULL);

	if(ret < 0) {
		if(ret == -EBUSY) {
			vme_irq_free(jvme_data.pVmeDev,
				     jvme_data.vme_irq.level,
				     jvme_data.vme_irq.statid);
			pr_debug
			    (" jvme_wait_for_interrupt: busy: release request\n");
		} else {
			pr_debug
			    (" jvme_wait_for_interrupt: failed to request interrupt\n");
		}
		return -1;
	}

	jvme_data.vme_irq.task = current;
	set_current_state(TASK_INTERRUPTIBLE);
	pr_debug(" Timeout Seconds 0x%lx, Jiffies 0x%lx\n",
		 jvme_data.vme_irq.tv.tv_sec, timeout);
	timeout = schedule_timeout(timeout);
	if(timeout == 0) {
		pr_debug(" jvme_wait_for_interrupt: Timed Out\n");
		jvme_data.vme_irq.timedout = 1;
	} else
		pr_debug(" Interrupt: Timeout=0x%lx\n", timeout);

	if(copy_to_user((void *)arg, &(jvme_data.vme_irq), sizeof(vme_irq_t))) {
		pr_debug
		    (" jvme_wait_for_interrupt: failed to copy data from user\n");
		return -1;
	}

	return 0;
}

static int jvme_rem_interrupt(u_long arg)
{
	if(jvme_data.pVmeDev == 0) {
		pr_debug(" jvme_rem_interrupt: no VME device\n");
		return -1;
	}

	if(copy_from_user(&(jvme_data.vme_irq), (void *)arg, sizeof(vme_irq_t))) {
		pr_debug
		    (" jvme_rem_interrupt: failed to copy data from user\n");
		return -1;
	}

	vme_irq_free(jvme_data.pVmeDev,
		     jvme_data.vme_irq.level, jvme_data.vme_irq.statid);

	pr_debug(" Removed irq level 0x%x, vector 0x%x\n",
		 jvme_data.vme_irq.level, jvme_data.vme_irq.statid);

	return 0;
}

static int jvme_gen_interrupt(u_long arg)
{
	int ret = 0;

	if(jvme_data.pVmeDev == 0) {
		pr_debug(" jvme_gen_interrupt: no VME device\n");
		return -1;
	}

	if(copy_from_user(&(jvme_data.vme_irq), (void *)arg, sizeof(vme_irq_t))) {
		pr_debug
		    (" jvme_gen_interrupt: failed to copy data from user\n");
		return -1;
	}

	ret = vme_irq_generate(jvme_data.pVmeDev,
			       jvme_data.vme_irq.level,
			       jvme_data.vme_irq.statid);

	return ret;
}

/*
 * The ioctls provided by the old VME access method (the one at vmelinux.org)
 * are most certainly wrong as the effectively push the registers layout
 * through to user space. Given that the VME core can handle multiple bridges,
 * with different register layouts this is most certainly not the way to go.
 *
 * We aren't using the structures defined in the Motorola driver either - these
 * are also quite low level, however we should use the definitions that have
 * already been defined.
 */
static int
jvme_ioctl(struct inode *inode, struct file *file, unsigned int cmd, u_long arg)
{
	struct vme_master master;
	struct vme_slave slave;
	struct vme_irq_id irq_req;
	struct vme_bridge_id bridge_info;
	u_long copied;
	unsigned int minor = MINOR(inode->i_rdev);
	int retval;
	void __user *argp = (void __user *)arg;

	statistics.ioctls++;

	pr_debug("type[%d] = %x, cmd = %x\n", minor, type[minor], cmd);

	switch (type[minor]) {
	case CONTROL_MINOR:
		switch (cmd) {
		case VME_IRQ_GEN:
			copied =
			    copy_from_user(&irq_req, argp,
					   sizeof(struct vme_irq_id));
			if(copied != 0) {
				pr_warn("Partial copy from userspace\n");
				return -EFAULT;
			}

			retval = vme_irq_generate(jvme_bridge,
						  irq_req.level,
						  irq_req.statid);

			return retval;
			break;

		case VME_GET_BRIDGE_INFO:

			retval =
			    vme_bridge_info(jvme_bridge, &bridge_info.deviceId,
					    &bridge_info.mapSize);

			copied =
			    copy_to_user(argp, &bridge_info,
					 sizeof(struct vme_bridge_id));
			if(copied != 0) {
				pr_warn("Partial copy to userspace\n");
				return -EFAULT;
			}

			return retval;
			break;

		case VME_DMA_REQ:
			return jvme_dma_request(arg);
		case VME_DMA_REMOVE:
			return jvme_dma_remove();
		case VME_DMA_NEW_LIST:
			return jvme_dma_new_list();
		case VME_DMA_FREE_LIST:
			return jvme_dma_free_list();
		case VME_DMA_ATTR_SET:
			return jvme_dma_set_attr(arg);
		case VME_DMA_FREE_ATTR:
			return jvme_dma_free_attr();
		case VME_DMA_XFER:
			return jvme_dma_xfer(arg);
		case VME_WAIT_IRQ:
			return jvme_wait_for_interrupt(arg);
		case VME_REM_IRQ:
			return jvme_rem_interrupt(arg);
		case VME_GEN_IRQ:
			return jvme_gen_interrupt(arg);
		}
		break;
	case MASTER_MINOR:
		switch (cmd) {
		case VME_GET_MASTER:
			memset(&master, 0, sizeof(struct vme_master));

			/* XXX  We do not want to push aspace, cycle and width
			 *      to userspace as they are
			 */
			retval = vme_master_get(image[minor].resource,
						&master.enable,
						&master.vme_addr, &master.size,
						&master.aspace, &master.cycle,
						&master.dwidth);

			copied =
			    copy_to_user(argp, &master,
					 sizeof(struct vme_master));
			if(copied != 0) {
				pr_warn("Partial copy to userspace\n");
				return -EFAULT;
			}

			return retval;
			break;

		case VME_SET_MASTER:

			copied = copy_from_user(&master, argp, sizeof(master));
			if(copied != 0) {
				pr_warn("Partial copy from userspace\n");
				return -EFAULT;
			}

			/* XXX  We do not want to push aspace, cycle and width
			 *      to userspace as they are
			 */
			return vme_master_set(image[minor].resource,
					      master.enable, master.vme_addr,
					      master.size, master.aspace,
					      master.cycle, master.dwidth);

			break;
		}
		break;
	case SLAVE_MINOR:
		switch (cmd) {
		case VME_GET_SLAVE:
			memset(&slave, 0, sizeof(struct vme_slave));

			/* XXX  We do not want to push aspace, cycle and width
			 *      to userspace as they are
			 */
			retval = vme_slave_get(image[minor].resource,
					       &slave.enable, &slave.vme_addr,
					       &slave.size, &slave.pci_addr,
					       &slave.aspace, &slave.cycle);

			pr_debug("vme_slave_get returned %d\n", retval);
			copied =
			    copy_to_user(argp, &slave,
					 sizeof(struct vme_slave));
			if(copied != 0) {
				pr_warn("Partial copy to userspace\n");
				return -EFAULT;
			}

			return retval;
			break;

		case VME_SET_SLAVE:

			copied = copy_from_user(&slave, argp, sizeof(slave));
			if(copied != 0) {
				pr_warn("Partial copy from userspace\n");
				return -EFAULT;
			}

			/* XXX  We do not want to push aspace, cycle and width
			 *      to userspace as they are
			 */
			return vme_slave_set(image[minor].resource,
					     slave.enable, slave.vme_addr,
					     slave.size, image[minor].pci_buf,
					     slave.aspace, slave.cycle);

			break;

		default:
			pr_warn("Invalid cmd\n");
		}
		break;
	}

	return -EINVAL;
}

static long jvme_unlocked_ioctl(struct file *file, unsigned int cmd, u_long arg)
{
	int ret;

	mutex_lock(&jvme_mutex);
	ret = jvme_ioctl(file_inode(file), file, cmd, arg);
	mutex_unlock(&jvme_mutex);

	return ret;
}

/*
 * Unallocate a previously allocated buffer
 */
static void buf_unalloc(int num)
{
	if(image[num].kern_buf) {
#ifdef VME_DEBUG
		pr_debug("Releasing buffer at %p\n", image[num].pci_buf);
#endif

		vme_free_consistent(image[num].resource, image[num].size_buf,
				    image[num].kern_buf, image[num].pci_buf);

		image[num].kern_buf = NULL;
		image[num].pci_buf = 0;
		image[num].size_buf = 0;

#ifdef VME_DEBUG
	} else {
		pr_debug("Buffer not allocated\n");
#endif
	}
}

static struct vme_driver jvme_driver = {
	.name = driver_name,
	.match = jvme_match,
	.probe = jvme_probe,
	.remove = jvme_remove,
};

static int __init jvme_init(void)
{
	int retval = 0;

	pr_info("JLab VME Driver\n");

	/*
	 * Here we just register the maximum number of devices we can and
	 * leave jvme_match() to allow only 1 to go through to probe().
	 * This way, if we later want to allow multiple user access devices,
	 * we just change the code in jvme_match().
	 */
	retval = vme_register_driver(&jvme_driver, VME_MAX_SLOTS);
	if(retval != 0)
		goto err_reg;

	return retval;

 err_reg:
	return retval;
}

static int jvme_match(struct vme_dev *vdev)
{
	if(vdev->num >= JVME_BUS_MAX)
		return 0;
	return 1;
}

/*
 * In this simple access driver, the old behaviour is being preserved as much
 * as practical. We will therefore reserve the buffers and request the images
 * here so that we don't have to do it later.
 */
static int jvme_probe(struct vme_dev *vdev)
{
	int i, err;
	char name[24];

	pr_debug("Startup\n");

	/* Save pointer to the bridge device */
	if(jvme_bridge != NULL) {
		dev_err(&vdev->dev, "Driver can only be loaded for 1 device\n");
		err = -EINVAL;
		goto err_dev;
	}
	jvme_bridge = vdev;

	/* Initialise descriptors */
	for(i = 0; i < VME_DEVS; i++) {
		image[i].kern_buf = NULL;
		image[i].pci_buf = 0;
		mutex_init(&image[i].mutex);
		image[i].device = NULL;
		image[i].resource = NULL;
		image[i].users = 0;
	}

	/* Initialise statistics counters */
	reset_counters();

	/* Assign major and minor numbers for the driver */
	err =
	    register_chrdev_region(MKDEV(VME_MAJOR, 0), VME_DEVS, driver_name);
	if(err) {
		dev_warn(&vdev->dev,
			 "Error getting Major Number %d for driver.\n",
			 VME_MAJOR);
		goto err_region;
	}

	/* Register the driver as a char device */
	jvme_cdev = cdev_alloc();
	if(!jvme_cdev) {
		err = -ENOMEM;
		goto err_char;
	}
	jvme_cdev->ops = &jvme_fops;
	jvme_cdev->owner = THIS_MODULE;
	err = cdev_add(jvme_cdev, MKDEV(VME_MAJOR, 0), VME_DEVS);
	if(err) {
		dev_warn(&vdev->dev, "cdev_all failed\n");
		goto err_char;
	}

	/* Request and allocate buffer for control */
	image[CONTROL_MINOR].size_buf = PCI_BUF_SIZE;
	image[CONTROL_MINOR].kern_buf =
	    kmalloc(image[CONTROL_MINOR].size_buf, GFP_KERNEL);
	if(image[CONTROL_MINOR].kern_buf == NULL) {
		err = -ENOMEM;
		goto err_control_buf;
	}

	/* Request slave resources and allocate buffers (128kB wide) */
	jlab_slave[0].enable = (slave_enable) ? 1 : 0;

	for(i = SLAVE_MINOR; i < (SLAVE_MAX + 1); i++) {
		int num = i - SLAVE_MINOR;
		if(num < JLAB_SLAVE_IMAGES) {
			image[i].resource =
			    vme_slave_request(jvme_bridge,
					      jlab_slave[num].aspace,
					      jlab_slave[num].cycle);
		} else {
			image[i].resource =
			    vme_slave_request(jvme_bridge, VME_A32, VME_SCT);
		}

		if(image[i].resource == NULL) {
			dev_warn(&vdev->dev,
				 "Unable to allocate slave resource\n");
			goto err_slave;
		}
		image[i].size_buf = PCI_BUF_SIZE;
		image[i].kern_buf = vme_alloc_consistent(image[i].resource,
							 image[i].size_buf,
							 &image[i].pci_buf);
		if(image[i].kern_buf == NULL) {
			dev_warn(&vdev->dev,
				 "Unable to allocate memory for buffer\n");
			image[i].pci_buf = 0;
			vme_slave_free(image[i].resource);
			err = -ENOMEM;
			goto err_slave;
		}

		if(num < JLAB_SLAVE_IMAGES) {
			err = vme_slave_set(image[i].resource,
					    jlab_slave[num].enable,
					    jlab_slave[num].vme_addr,
					    jlab_slave[num].size,
					    image[i].pci_buf,
					    jlab_slave[num].aspace,
					    jlab_slave[num].cycle);
			if(err) {
				dev_warn(&vdev->dev,
					 "Unable to set slave image %d\n", num);
				goto err_slave;
			}
		}
	}
	/*
	 * Request master resources allocate page sized buffers for small
	 * reads and writes
	 */
	for(i = MASTER_MINOR; i < (MASTER_MAX + 1); i++) {
		int num = i - MASTER_MINOR;
		if(num < JLAB_MASTER_IMAGES) {
			image[i].resource =
			    vme_master_request(jvme_bridge,
					       jlab_master[num].aspace,
					       jlab_master[num].cycle,
					       jlab_master[num].dwidth);
		} else {
			image[i].resource =
			    vme_master_request(jvme_bridge, VME_A32, VME_SCT,
					       VME_D32);
		}

		if(image[i].resource == NULL) {
			dev_warn(&vdev->dev,
				 "Unable to allocate master resource\n");
			goto err_master;
		}
		image[i].size_buf = PCI_BUF_SIZE;
		image[i].kern_buf = kmalloc(image[i].size_buf, GFP_KERNEL);
		if(image[i].kern_buf == NULL) {
			err = -ENOMEM;
			goto err_master_buf;
		}

		if(num < JLAB_MASTER_IMAGES) {
			err = vme_master_set(image[i].resource,
					     jlab_master[num].enable,
					     jlab_master[num].vme_addr,
					     jlab_master[num].size,
					     jlab_master[num].aspace,
					     jlab_master[num].cycle,
					     jlab_master[num].dwidth);
			if(err) {
				dev_warn(&vdev->dev,
					 "Unable to set master image %d\n",
					 num);
				goto err_master;
			}
		}
	}
	/* Create sysfs entries - on udev systems this creates the dev files */
	jvme_sysfs_class = class_create(THIS_MODULE, driver_name);
	if(IS_ERR(jvme_sysfs_class)) {
		dev_err(&vdev->dev, "Error creating jvme class.\n");
		err = PTR_ERR(jvme_sysfs_class);
		goto err_class;
	}

	/* Add sysfs Entries */
	for(i = 0; i < VME_DEVS; i++) {
		int num;
		num = (type[i] == SLAVE_MINOR) ? i - (MASTER_MAX + 1) : i;

		switch (type[i]) {
		case MASTER_MINOR:
			if(num < JLAB_MASTER_IMAGES)
				sprintf(name, "%s", jlab_master_names[num]);
			else
				sprintf(name, "bus/vme/m_rsvd%%d");
			break;
		case CONTROL_MINOR:
			sprintf(name, "bus/vme/ctl");
			break;
		case SLAVE_MINOR:
			if(num < JLAB_SLAVE_IMAGES)
				sprintf(name, "%s", jlab_slave_names[num]);
			else
				sprintf(name, "bus/vme/s_rsvd%%d");
			break;
		default:
			err = -EINVAL;
			goto err_sysfs;
			break;
		}

		image[i].device = device_create(jvme_sysfs_class, NULL,
						MKDEV(VME_MAJOR, i), NULL, name,
						num);
		if(IS_ERR(image[i].device)) {
			dev_info(&vdev->dev, "Error creating sysfs device\n");
			err = PTR_ERR(image[i].device);
			goto err_sysfs;
		}
	}

	return 0;

	/* Ensure counter set correcty to destroy all sysfs devices */
	i = VME_DEVS;
 err_sysfs:
	while(i > 0) {
		i--;
		device_destroy(jvme_sysfs_class, MKDEV(VME_MAJOR, i));
	}
	class_destroy(jvme_sysfs_class);

	/* Ensure counter set correcty to unalloc all master windows */
	i = MASTER_MAX + 1;
 err_master_buf:
	for(i = MASTER_MINOR; i < (MASTER_MAX + 1); i++)
		kfree(image[i].kern_buf);
 err_master:
	while(i > MASTER_MINOR) {
		i--;
		vme_master_free(image[i].resource);
	}

	/*
	 * Ensure counter set correcty to unalloc all slave windows and buffers
	 */
	i = SLAVE_MAX + 1;
 err_slave:
	while(i > SLAVE_MINOR) {
		i--;
		buf_unalloc(i);
		vme_slave_free(image[i].resource);
	}
 err_control_buf:
	kfree(image[CONTROL_MINOR].kern_buf);
 err_class:
	cdev_del(jvme_cdev);
 err_char:
	unregister_chrdev_region(MKDEV(VME_MAJOR, 0), VME_DEVS);
 err_region:
 err_dev:
	return err;
}

int jvme_get(struct vme_dev *dev)
{
	if(jvme_bridge == NULL) {
		dev_err(&jvme_bridge->dev, "Driver not loaded\n");
		return -EINVAL;
	}

	dev = jvme_bridge;
	return 0;
}

EXPORT_SYMBOL(jvme_get);

static int jvme_remove(struct vme_dev *dev)
{
	int i;

	/* Remove sysfs Entries */
	for(i = 0; i < VME_DEVS; i++) {
		mutex_destroy(&image[i].mutex);
		device_destroy(jvme_sysfs_class, MKDEV(VME_MAJOR, i));
	}
	class_destroy(jvme_sysfs_class);

	for(i = MASTER_MINOR; i < (MASTER_MAX + 1); i++) {
		kfree(image[i].kern_buf);
		vme_master_free(image[i].resource);
	}

	for(i = SLAVE_MINOR; i < (SLAVE_MAX + 1); i++) {
		/* vme_slave_set(image[i].resource, 0, 0, 0, 0, VME_A32, 0); */
		buf_unalloc(i);
		vme_slave_free(image[i].resource);
	}

	kfree(image[CONTROL_MINOR].kern_buf);

	/* Unregister device driver */
	cdev_del(jvme_cdev);

	/* Unregiser the major and minor device numbers */
	unregister_chrdev_region(MKDEV(VME_MAJOR, 0), VME_DEVS);

	return 0;
}

static void __exit jvme_exit(void)
{
	vme_unregister_driver(&jvme_driver);
}

MODULE_DESCRIPTION("JLab VME Driver");
MODULE_AUTHOR("Bryan Moffit");
MODULE_LICENSE("GPL");

module_init(jvme_init);
module_exit(jvme_exit);
