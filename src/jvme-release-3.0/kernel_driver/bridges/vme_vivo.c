/*
 * Support for the Abaco Systems Vivo VME Bridge chip
 *
 * Copyright 2008, 2017 Abaco Systems, Inc.
 *
 * Based on work by Tom Armistead and Ajit Prem
 * Copyright 2004 Motorola Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/pci.h>
#include <linux/poll.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/byteorder/generic.h>
#include <linux/string.h>
#include "../vme.h"

#include "../vme_bridge.h"
#include "vme_vivo.h"

static int vivo_probe(struct pci_dev *, const struct pci_device_id *);
static void vivo_remove(struct pci_dev *);

/* Module parameter */
static bool err_chk;
static int geoid;
static int berr_irq_enable = 0, dma_irq_enable = 0;	/* default to zero for jlab */

static const char driver_name[] = "vme_vivo";

static const struct pci_device_id vivo_ids[] = {
	{PCI_DEVICE(PCI_VENDOR_ID_ABACO, PCI_DEVICE_ID_ABACO_VIVO)},
	{},
};

static struct pci_driver vivo_driver = {
	.name = driver_name,
	.id_table = vivo_ids,
	.probe = vivo_probe,
	.remove = vivo_remove,
};

static void reg_join(unsigned int high, unsigned int low, unsigned long long *variable)
{
	*variable = (unsigned long long)high << 32;
	*variable |= (unsigned long long)low;
}

static void reg_split(unsigned long long variable, unsigned int *high, unsigned int *low)
{
	*low = (unsigned int)variable & 0xFFFFFFFF;
	*high = (unsigned int)(variable >> 32);
}

/*
 * Determine if the requested value is a power of 2
 */
static inline __attribute__ ((const))
bool is_power_of_2__64(u64 n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}

/*
 * Wakes up DMA queue.
 */
static u32 vivo_dma_irqhandler(struct vme_bridge *vivo_bridge, int channel_mask)
{
	u32 vivo_dmais0, vivo_dmais1, serviced = 0;
	struct vivo_driver *bridge;

	bridge = vivo_bridge->driver_priv;

	if(channel_mask & VIVO_INT_STATUS_DMA0) {

		/* Read the DMA interrupt status for channel 0 */
		vivo_dmais0 = ioread32(bridge->base + VIVO_DMAIS0);

		/* Clear the DMA interrupt status for channel 0 */
		iowrite32(vivo_dmais0, bridge->base + VIVO_DMAIS0);

		if(vivo_dmais0 & (VIVO_DMAISX_PAUSED | VIVO_DMAISX_ABORTED | VIVO_DMAISX_ERROR))
			dev_err(vivo_bridge->parent,
				"DMA Error channel 0. DMAIS=%08X\n", vivo_dmais0);

		bridge->dma_int_count[0]++;
		wake_up(&bridge->dma_queue[0]);
		serviced |= VIVO_INT_STATUS_DMA0;
	}
	if(channel_mask & VIVO_INT_STATUS_DMA1) {

		/* Read the DMA interrupt status for channel 1 */
		vivo_dmais1 = ioread32(bridge->base + VIVO_DMAIS1);

		/* Clear the DMA interrupt status for channel 1 */
		iowrite32(vivo_dmais1, bridge->base + VIVO_DMAIS1);

		if(vivo_dmais1 & (VIVO_DMAISX_PAUSED | VIVO_DMAISX_ABORTED | VIVO_DMAISX_ERROR))
			dev_err(vivo_bridge->parent,
				"DMA Error channel 1. DMAIS=%08X\n", vivo_dmais1);

		bridge->dma_int_count[1]++;
		wake_up(&bridge->dma_queue[1]);
		serviced |= VIVO_INT_STATUS_DMA1;
	}

	return serviced;
}

/*
 * Wake up mail box queue.
 *
 * XXX This functionality is not exposed up though API.
 */
static u32 vivo_mb_irqhandler(struct vme_bridge *vivo_bridge, u32 stat)
{
	int i;
	u32 val;
	u32 serviced = 0;
	struct vivo_driver *bridge;

	bridge = vivo_bridge->driver_priv;

	for(i = 0; i < 4; i++) {
		if(stat & VIVO_INT_STATUS_MBOX[i]) {
			val = ioread32(bridge->base + VIVO_MAILBOX[i]);
			dev_err(vivo_bridge->parent, "VME Mailbox %d received: 0x%x\n", i, val);
			serviced |= VIVO_INT_STATUS_MBOX[i];
		}
	}

	return serviced;
}

/*
 * Display error & status message when PERR (PCIe) exception interrupt occurs.
 */
static u32 vivo_perr_irqhandler(struct vme_bridge *vivo_bridge)
{
	struct vivo_driver *bridge;

	bridge = vivo_bridge->driver_priv;

	dev_err(vivo_bridge->parent, "PCIe Exception encountered\n");

	return VIVO_INT_STATUS_PCIE;
}

/*
 * Identify the master window or DMA address modifier from the address space,
 * address mode, and transfer mode.
 */
static int
vivo_master_derive_am(struct vme_bridge *vivo_bridge,
		u32 aspace, u32 amode, u32 trans_mode, u32 * am_code)
{
	u32 am = 0;
	u32 data = 0;

	struct vivo_driver *bridge;

	bridge = vivo_bridge->driver_priv;

	switch (aspace) {
	case VME_A16:
		/* A16 doesn't care about DATA/PROG, check for SUP */
		if(amode & VME_SUPER)

			/* This is always A16S */
			am = VIVO_VME_ADDR_MOD_A16S;
		else
			/* This is always A16U */
			am = VIVO_VME_ADDR_MOD_A16U;
		break;
	case VME_A24:
		if(amode & VME_SUPER)
			/* Supervisor, check for program */
			if(amode & VME_PROG)
				am = VIVO_VME_ADDR_MOD_A24SP;
			else
				am = VIVO_VME_ADDR_MOD_A24SD;
		else
			/* User, check for program */
			if(amode & VME_PROG)
				am = VIVO_VME_ADDR_MOD_A24UP;
			else
				am = VIVO_VME_ADDR_MOD_A24UD;

		/* A24 BLT/MBLT modes can be different  */
		if(trans_mode & VME_BLT) {
			if(amode & VME_SUPER)
				am = VIVO_VME_ADDR_MOD_A24SB;
			else
				am = VIVO_VME_ADDR_MOD_A24UB;
		} else if(trans_mode & VME_MBLT) {
			if(amode & VME_SUPER)
				am = VIVO_VME_ADDR_MOD_A24SMB;
			else
				am = VIVO_VME_ADDR_MOD_A24UMB;
		}
		break;
	case VME_A32:
		if(amode & VME_SUPER)
			/* Supervisor, check for program */
			if(amode & VME_PROG)
				am = VIVO_VME_ADDR_MOD_A32SP;
			else
				am = VIVO_VME_ADDR_MOD_A32SD;
		else
			/* User, check for program */
			if(amode & VME_PROG)
				am = VIVO_VME_ADDR_MOD_A32UP;
			else
				am = VIVO_VME_ADDR_MOD_A32UD;

		/* A32 BLT/MBLT modes can be different  */
		if(trans_mode & VME_BLT) {
			if(amode & VME_SUPER)
				am = VIVO_VME_ADDR_MOD_A32SB;
			else
				am = VIVO_VME_ADDR_MOD_A32UB;
		} else if(trans_mode & VME_MBLT) {
			if(amode & VME_SUPER)
				am = VIVO_VME_ADDR_MOD_A32SMB;
			else
				am = VIVO_VME_ADDR_MOD_A32UMB;
		}
		break;
	case VME_CRCSR:
		am = VIVO_VME_ADDR_MOD_CR_CSR;
		break;
	case VME_USER1:
	case VME_USER2:

		/* Read VME_MSTR register to get the AM code */
		data = ioread32(bridge->base + VIVO_VME_MSTR);

		if(aspace == VME_USER1)
			am = (data & (VIVO_VME_MSTR_FIXED0 | VIVO_VME_MSTR_USERAM0)) >> 16;
		else if(aspace == VME_USER2)
			am = (data & (VIVO_VME_MSTR_FIXED1 | VIVO_VME_MSTR_USERAM1)) >> 24;
		break;
	default:
		dev_err(vivo_bridge->parent,
			"VERR Derive AM invalid address space: 0x%x\n", aspace);
		return -EINVAL;
	}

	if((trans_mode == VME_2eSST) || (trans_mode == VME_2eSSTB))
		am = VIVO_VME_ADDR_MOD_2eVME_6U;

	*am_code = am;

	return 0;
}

/*
 * Identify a valid master window containing the offending AXIS address.
 */
static int
vivo_master_axis_valid(struct vme_bridge *vivo_bridge,
		u32 axis_addr, u32 * error_addr, u32 * error_am)
{

	struct vivo_driver *bridge;
	int i, status;
	u32 ctl, mw_mask, mw_pci_base, end_addr;
	u32 berr_addr_high, berr_addr_low;
	u64 berr_addr = 0;
	u32 aspace = 0;
	u32 amode = 0;
	u32 am_code = 0;
	u32 trans_mode = 0;

	bridge = vivo_bridge->driver_priv;

	status = -EINVAL;

	for(i = 0; i < VIVO_MAX_MASTER; i++) {

		/* Check to see if this one is enabled */
		ctl = ioread32(bridge->base + VIVO_MW_CTRL[i]);

		if(!(ctl & VIVO_MW_CTRLX_EBL))
			continue;	/* JLAB Fix */

		/* Calculate the AXIS address range */
		mw_mask = ~(ioread32(bridge->base + VIVO_MW_MASK[i]));
		mw_pci_base = ioread32(bridge->base + VIVO_MW_ADDR[i]);
		end_addr = mw_pci_base + mw_mask;

		/* Determine if requested address is in that range */
		if((axis_addr < mw_pci_base) || (axis_addr > end_addr))
			continue;	/* JLAB Fix */

		/* Load the BERR address data */
		berr_addr_high = 0;
		berr_addr_low = ioread32(bridge->base + VIVO_MW_OFFSET[i]) + (axis_addr & mw_mask);
		reg_join(berr_addr_high, berr_addr_low, &berr_addr);

		/* Flag that we found it */
		status = 0;
		break;
	}

	if(status == 0) {

		/* Get the address space from the MW_CTRL register */
		switch (ctl & VIVO_MW_CTRLX_AM_AS) {
		case VIVO_MW_CTRLX_AM_AS_A16:
			aspace = VME_A16;
			break;
		case VIVO_MW_CTRLX_AM_AS_A24:
			aspace = VME_A24;
			break;
		case VIVO_MW_CTRLX_AM_AS_A32:
			aspace = VME_A32;
			break;
		case VIVO_MW_CTRLX_AM_AS_CRCSR:
			aspace = VME_CRCSR;
			break;
		case VIVO_MW_CTRLX_AM_AS_USER1:
			aspace = VME_USER1;
			break;
		case VIVO_MW_CTRLX_AM_AS_USER2:
			aspace = VME_USER2;
			break;
		default:
			dev_err(vivo_bridge->parent,
				"Master Window VERR invalid address space: 0x%x\n", aspace);
			return -EINVAL;
		}

		if((aspace == VME_A16) || (aspace == VME_A24)
			|| (aspace == VME_A32)) {

			/* Get the address mode from MW_CTRL register */
			if(ctl & VIVO_MW_CTRLX_AM_NPA)
				amode |= VME_SUPER;
			else
				amode |= VME_USER;

			if(ctl & VIVO_MW_CTRLX_AM_DA)
				amode |= VME_PROG;
			else
				amode |= VME_DATA;
		}

		/* Get the transfer mode from MW_CTRL register */
		switch (ctl & VIVO_MW_CTRLX_BT) {
		case VIVO_MW_CTRLX_BT_SCT:
			trans_mode = VME_SCT;
			break;
		default:
			dev_err(vivo_bridge->parent,
				"Master Window VERR non-SCT transfer mode invalid: 0x%x\n", aspace);
			return -EINVAL;
		}

		/* Determine the AM code */
		status = vivo_master_derive_am(vivo_bridge, aspace, amode, trans_mode, &am_code);

		if(status == 0) {

			*error_addr = berr_addr;
			*error_am = am_code;
		}
	}

	return status;
}

/*
 * Derive the an AM code from the requested DMA attributes.
 */
static int vivo_dma_derive_am(struct vme_bridge *vivo_bridge, u32 dma_attr, u32 * vme_am)
{
	u32 aspace = 0;
	u32 amode = 0;
	u32 am_code = 0;
	u32 trans_mode = 0;
	int status = 0;

	/* Default to invalid address modifier */
	*vme_am = 0;

	switch (dma_attr & VIVO_DMASATTX_AM_AS) {
	case VIVO_DMASATTX_AM_AS_A16:
		aspace = VME_A16;
		break;
	case VIVO_DMASATTX_AM_AS_A24:
		aspace = VME_A24;
		break;
	case VIVO_DMASATTX_AM_AS_A32:
		aspace = VME_A32;
		break;
	case VIVO_DMASATTX_AM_AS_CRCSR:
		aspace = VME_CRCSR;
		break;
	case VIVO_DMASATTX_AM_AS_USER1:
		aspace = VME_USER1;
		break;
	case VIVO_DMASATTX_AM_AS_USER2:
		aspace = VME_USER2;
		break;
	default:
		dev_err(vivo_bridge->parent, "DMA VERR invalid address space: 0x%x\n", aspace);
		return -EINVAL;
	}

	if((aspace == VME_A16) || (aspace == VME_A24) || (aspace == VME_A32)) {

		if((dma_attr & VIVO_DMASATTX_AM_DA) == VIVO_DMASATTX_AM_DA_PROGRAM)
			amode |= VME_PROG;
		else
			/* Default to data */
			amode |= VME_DATA;

		if((dma_attr & VIVO_DMASATTX_AM_NPA) == VIVO_DMASATTX_AM_NPA_SUPER)
			amode |= VME_SUPER;
		else
			/* Default to user */
			amode |= VME_USER;
	}

	/* Get the transfer mode from MW_CTRL register */
	switch (dma_attr & VIVO_DMASATTX_BT) {
	case VIVO_DMASATTX_BT_SCT:
		trans_mode = VME_SCT;
		break;
	case VIVO_DMASATTX_BT_BLT:
		trans_mode = VME_BLT;
		break;
	case VIVO_DMASATTX_BT_MBLT:
		trans_mode = VME_MBLT;
		break;
	case VIVO_DMASATTX_BT_2eSST:
		trans_mode = VME_2eSST;
		break;
	case VIVO_DMASATTX_BT_2eSSTB:
		trans_mode = VME_2eSSTB;
		break;
	default:
		dev_err(vivo_bridge->parent, "DMA VERR transfer mode invalid: 0x%x\n", aspace);
		return -EINVAL;
	}

	/* Determine the AM code */
	status = vivo_master_derive_am(vivo_bridge, aspace, amode, trans_mode, &am_code);

	if(status == 0)
		*vme_am = am_code;

	return status;
}

/*
 * Identify the DMA attributes for the offending 32-bit VME address.
 */
static int vivo_dma_axis_valid(struct vme_bridge *vivo_bridge, u32 error_addr, u32 * error_am)
{
	struct list_head *dma_pos = NULL;
	struct vme_dma_resource *dma_ctrlr = NULL;
	struct vme_dma_list *active_dma_list = NULL;
	struct vivo_dma_entry *entry = NULL;
	struct list_head *ptr = NULL;
	u32 ctrlr_num = 0;
	u32 vme_addr = 0;
	u32 dma_attr = 0;
	u32 vme_am = 0;
	u32 transfer_size = 0;

	int status = -EINVAL;

	struct vivo_driver *bridge;

	bridge = vivo_bridge->driver_priv;

	/* Loop through DMA resources */
	list_for_each(dma_pos, &vivo_bridge->dma_resources) {
		dma_ctrlr = list_entry(dma_pos, struct vme_dma_resource, list);

		ctrlr_num = dma_ctrlr->number;

		active_dma_list = list_first_entry(&dma_ctrlr->running, struct vme_dma_list, list);

		if(active_dma_list->entries.next != NULL) {
			list_for_each(ptr, &active_dma_list->entries) {

				entry = list_entry(ptr, struct vivo_dma_entry, list);
				if(entry) {

					/* Check if within descriptor range */
					if((entry->u_desc.satt & VIVO_DMASATTX_DPRT) ==
						VIVO_DMASATTX_DPRT_VMESRC) {

						vme_addr = entry->u_desc.sa;
						dma_attr = entry->u_desc.satt;

					} else if((entry->u_desc.datt & VIVO_DMADATTX_DPRT) ==
						VIVO_DMADATTX_DPRT_VMESRC) {

						vme_addr = entry->u_desc.da;
						dma_attr = entry->u_desc.datt;
					}

					transfer_size = entry->u_desc.tl;

					if((error_addr >= vme_addr) &&
						(error_addr <= (vme_addr + transfer_size))) {

						dev_dbg(vivo_bridge->parent,
							"%s VERR in entry %p ctlr %d\n", __func__,
							entry, ctrlr_num);

						status =
							vivo_dma_derive_am(vivo_bridge, dma_attr,
									&vme_am);

						goto found_entry;
					}
				}
			}
		}
	}

found_entry:
	if(!status)
		*error_am = vme_am;

	return status;
}

/*
 * Save address and status when VME error interrupt occurs.
 */
static u32 vivo_verr_irqhandler(struct vme_bridge *vivo_bridge)
{
	u32 axis_addr, axis_ecr;
	u32 error_addr_high = 0;
	u32 error_addr_low = 0;
	u64 error_addr = 0;
	u32 error_am = 0;
	int status = 0;

	struct vivo_driver *bridge;

	bridge = vivo_bridge->driver_priv;

	/* Read the error address */
	axis_addr = ioread32(bridge->base + VIVO_AXIS_ECR_A);

	/* Determine if the address if from a DMA or a master window */
	axis_ecr = ioread32(bridge->base + VIVO_AXIS_ECR);

	if(axis_ecr & VIVO_AXIS_ECR_TT_DMA) {

		dev_info(vivo_bridge->parent,
			"DMA VERR: axis_addr 0x%08x axis_ecr 0x%08x\n", axis_addr, axis_ecr);

		error_addr_low = axis_addr;

		/* Get the VME attributes for that address */
		status = vivo_dma_axis_valid(vivo_bridge, axis_addr, &error_am);
	} else
		/* Get the VME info for that address */
		status = vivo_master_axis_valid(vivo_bridge, axis_addr, &error_addr_low, &error_am);

	if(status)
		dev_err(vivo_bridge->parent,
			"VERR: failure identifying bus error attributes, status 0x%x\n", status);
	else {

		/* A64 is not supported */
		error_addr_high = 0;
		reg_join(error_addr_high, error_addr_low, &error_addr);

		if(err_chk)
			vme_bus_error_handler(vivo_bridge, error_addr, error_am);
		else
			dev_err(vivo_bridge->parent,
				"VERR at address: 0x%llx, AXI Slave ECR: %08x Error AM 0x%x\n",
				error_addr, axis_ecr, error_am);
	}

	/* Check the AXI Slave Error Capture Register (for lost error data) */
	if(axis_ecr & VIVO_AXIS_ECR_LOST_ERR_MISSED)
		dev_err(vivo_bridge->parent, "VME Bus Exception Overflow Occurred\n");

	/* Clear the BERR */
	iowrite32(0, bridge->base + VIVO_AXIS_ECR);
	iowrite32((ioread32(bridge->base + VIVO_BIT_CLEAR) |
			VIVO_BIT_CLEAR_BERRSC), bridge->base + VIVO_BIT_CLEAR);

	dev_err(vivo_bridge->parent, "VME Bus Error occurred\n");

	return VIVO_INT_STATUS_VBERR;
}

/*
 * Wake up IACK queue.
 */
static u32 vivo_iack_irqhandler(struct vme_bridge *vivo_bridge)
{
	struct vivo_driver *bridge;

	bridge = vivo_bridge->driver_priv;

	bridge->iack_count++;
	wake_up(&bridge->iack_queue);

	return VIVO_INT_STATUS_SWIACK;
}

/*
 * Calling VME bus interrupt callback if provided.
 */
static u32 vivo_virq_irqhandler(struct vme_bridge *vivo_bridge, u32 stat)
{
	int vec, i, serviced = 0;
	struct vivo_driver *bridge;

	bridge = vivo_bridge->driver_priv;

	for(i = 7; i > 0; i--) {
		if(stat & VIVO_INT_STATUS_IRQ[i - 1]) {

			/* Read the vector */
			vec = ioread32(bridge->base + VIVO_VME_IRQ_STAT[i - 1]);

			vme_irq_handler(vivo_bridge, i, vec);

			serviced |= VIVO_INT_STATUS_IRQ[i - 1];
		}
	}

	return serviced;
}

/*
 * Top level interrupt handler.  Clears appropriate interrupt status bits and
 * then calls appropriate sub handler(s).
 */
static irqreturn_t vivo_irqhandler(int irq, void *ptr)
{
	u32 stat, enable, serviced = 0;
	struct vme_bridge *vivo_bridge;
	struct vivo_driver *bridge;

	vivo_bridge = ptr;

	bridge = vivo_bridge->driver_priv;

	/* Determine which interrupts are unmasked and set */
	enable = ioread32(bridge->base + VIVO_INT_EBL);
	stat = ioread32(bridge->base + VIVO_INT_STATUS);

	/* Disable all interrupts until handler is finished */
	iowrite32(0, bridge->base + VIVO_INT_EBL);

	dev_dbg(vivo_bridge->parent, "%s stat 0x%x enable 0x%x\n", __func__, stat, enable);

	/* Only look at unmasked interrupts */
	stat &= enable;

	if(unlikely(!stat))
		return IRQ_NONE;

	while(stat != 0) {

		/* Call subhandlers as appropriate */

		/* VME bus error */
		if(stat & VIVO_INT_STATUS_VBERR)
			serviced |= vivo_verr_irqhandler(vivo_bridge);

		/* DMA irqs */
		if(stat & (VIVO_INT_STATUS_DMA1 | VIVO_INT_STATUS_DMA0))
			serviced |= vivo_dma_irqhandler(vivo_bridge, stat);

		/* PCIe bus error */
		if(stat & VIVO_INT_STATUS_PCIE)
			serviced |= vivo_perr_irqhandler(vivo_bridge);

		/* Mail box irqs */
		if(stat & (VIVO_INT_STATUS_MBOX3 | VIVO_INT_STATUS_MBOX2 |
				VIVO_INT_STATUS_MBOX1 | VIVO_INT_STATUS_MBOX0))
			serviced |= vivo_mb_irqhandler(vivo_bridge, stat);

		/* IACK irq */
		if(stat & VIVO_INT_STATUS_SWIACK)
			serviced |= vivo_iack_irqhandler(vivo_bridge);

		/* VME bus irqs */
		if(stat & (VIVO_INT_STATUS_IRQ7 | VIVO_INT_STATUS_IRQ6 |
				VIVO_INT_STATUS_IRQ5 | VIVO_INT_STATUS_IRQ4 |
				VIVO_INT_STATUS_IRQ3 | VIVO_INT_STATUS_IRQ2 | VIVO_INT_STATUS_IRQ1))
			serviced |= vivo_virq_irqhandler(vivo_bridge, stat);

		/* Clear serviced interrupts */
		iowrite32(serviced, bridge->base + VIVO_INT_STATUS);
		serviced = 0;

		/* Re-read the status and mask it with the enable register */
		stat = ioread32(bridge->base + VIVO_INT_STATUS);
		stat &= enable;
	}

	/* Renable the interrupts we disabled above */
	iowrite32(enable, bridge->base + VIVO_INT_EBL);

	return IRQ_HANDLED;
}

static int vivo_irq_init(struct vme_bridge *vivo_bridge)
{
	int result;
	unsigned int tmp;
	struct pci_dev *pdev;
	struct vivo_driver *bridge;

	pdev = to_pci_dev(vivo_bridge->parent);

	bridge = vivo_bridge->driver_priv;

	INIT_LIST_HEAD(&vivo_bridge->vme_error_handlers);

	mutex_init(&vivo_bridge->irq_mtx);

	/* The device needs MSI interrupts if MSI is not enabled */
	if(!(pci_dev_msi_enabled(pdev))) {

		/* Enable MSI */
		pci_enable_msi(pdev);
		dev_info(vivo_bridge->parent, "Enabled MSI Interrupts. IRQ 0x%x\n", pdev->irq);
	}

	result = request_irq(pdev->irq, vivo_irqhandler, IRQF_SHARED, driver_name, vivo_bridge);
	if(result) {
		dev_err(vivo_bridge->parent, "Can't get assigned pci irq vector %02X\n", pdev->irq);
		return result;
	}

	/* Enable the DMA channel 0 and channel 1 interrupts */
	tmp = VIVO_DMAIEX_DONE | VIVO_DMAIEX_PAUSED | VIVO_DMAIEX_ABORTED | VIVO_DMAIEX_ERROR;

	iowrite32(tmp, bridge->base + VIVO_DMAIE0);
	iowrite32(tmp, bridge->base + VIVO_DMAIE1);

	/* Enable and unmask interrupts */
	tmp = VIVO_INT_EBL_PCIE |
		VIVO_INT_EBL_MBOX3 | VIVO_INT_EBL_MBOX2 |
		VIVO_INT_EBL_MBOX1 | VIVO_INT_EBL_MBOX0 | VIVO_INT_EBL_SWIACK;

	if(dma_irq_enable == 1)
		tmp |= VIVO_INT_EBL_DMA1 | VIVO_INT_EBL_DMA0;

	if(berr_irq_enable == 1)
		tmp |= VIVO_INT_EBL_VBERR;

	/* This leaves the following interrupts masked.
	 * VIVO_INT_EBL_ADC
	 * VIVO_INT_EBL_VTIMER
	 * VIVO_INT_EBL_AXIS_ERR
	 * VIVO_INT_EBL_AXIM_ERR
	 * VIVO_INT_EBL_SYSFAIL
	 * VIVO_INT_EBL_ACFAIL
	 */

	/* Don't enable VME interrupts until we add a handler, else the board
	 * will respond to it and we don't want that unless it knows how to
	 * properly deal with it.
	 * VIVO_INT_EBL_IRQ7
	 * VIVO_INT_EBL_IRQ6
	 * VIVO_INT_EBL_IRQ5
	 * VIVO_INT_EBL_IRQ4
	 * VIVO_INT_EBL_IRQ3
	 * VIVO_INT_EBL_IRQ2
	 * VIVO_INT_EBL_IRQ1
	 */

	iowrite32(tmp, bridge->base + VIVO_INT_EBL);

	return 0;
}

static void vivo_irq_exit(struct vme_bridge *vivo_bridge, struct pci_dev *pdev)
{
	/* Detach interrupt handler */
	free_irq(pdev->irq, vivo_bridge);

	/* Disable MSI if the handler was MSI enabled */
	if(pci_dev_msi_enabled(pdev))
		pci_disable_msi(pdev);
}

/*
 * Check to see if an IACK has been received, return true (1) or false (0).
 */
static int vivo_iack_received(struct vivo_driver *bridge)
{
	if(bridge->iack_count > 0) {
		bridge->iack_count--;
		return 1;
	} else
		return 0;
}

/*
 * Configure VME interrupt
 */
static void vivo_irq_set(struct vme_bridge *vivo_bridge, int level, int state, int sync)
{
	struct pci_dev *pdev;
	u32 tmp;
	struct vivo_driver *bridge;

	bridge = vivo_bridge->driver_priv;

	if(state == 0) {
		tmp = ioread32(bridge->base + VIVO_INT_EBL);
		tmp &= ~VIVO_INT_EBL_IRQEN[level - 1];
		iowrite32(tmp, bridge->base + VIVO_INT_EBL);

		if(sync != 0) {
			pdev = to_pci_dev(vivo_bridge->parent);
			synchronize_irq(pdev->irq);
		}
	} else {
		tmp = ioread32(bridge->base + VIVO_INT_EBL);
		tmp |= VIVO_INT_EBL_IRQEN[level - 1];
		iowrite32(tmp, bridge->base + VIVO_INT_EBL);
	}
}

/*
 * Generate a VME bus interrupt at the requested level & vector. Wait for
 * interrupt to be acked.
 */
static int vivo_irq_generate(struct vme_bridge *vivo_bridge, int level, int statid)
{
	u32 tmp;
	struct vivo_driver *bridge;

	bridge = vivo_bridge->driver_priv;

	mutex_lock(&bridge->vme_int);

	/* Put the level into SWIRQ of VME_INT_MAP */
	tmp = ioread32(bridge->base + VIVO_VME_INT_MAP);
	tmp &= ~VIVO_VME_INT_MAP_SWIRQ;
	tmp |= level & VIVO_VME_INT_MAP_SWIRQ;
	iowrite32(tmp, bridge->base + VIVO_VME_INT_MAP);

	/* Put the vector into VME_INT_STAT_SW */
	iowrite32((statid & 0xFF), bridge->base + VIVO_VME_INT_STAT_SW);

	/* Assert VMEbus IRQ */
	iowrite32((ioread32(bridge->base + VIVO_VME_INT) | VIVO_VME_INT_SWIRQ),
		bridge->base + VIVO_VME_INT);

	/* XXX Consider implementing a timeout? */
	wait_event_interruptible(bridge->iack_queue, vivo_iack_received(bridge));

	mutex_unlock(&bridge->vme_int);

	return 0;
}

/*
 * Initialize a slave window with the requested attributes.
 */
static int
vivo_slave_set(struct vme_slave_resource *image, int enabled,
	unsigned long long vme_base, unsigned long long size,
	dma_addr_t pci_base, u32 aspace, u32 cycle)
{
	u32 i = 0;
	u32 temp_ctl = 0;
	u32 vme_base_low, vme_base_high;
	u32 pci_base_low, pci_base_high;
	u32 am_code = 0;
	u32 xam_code = 0;
	u32 csr_ader = 0;
	u64 vme_end_addr = 0;
	struct vme_bridge *vivo_bridge;
	struct vivo_driver *bridge;

	vivo_bridge = image->parent;
	bridge = vivo_bridge->driver_priv;

	i = image->number;

	if(!is_power_of_2__64(size)) {
		dev_err(vivo_bridge->parent, "Size must be a power of 2 (%lld)\n", size);
		return -EINVAL;
	}

	if((size - 1) & vme_base) {
		dev_err(vivo_bridge->parent, "Size and address are not aligned\n");
		return -EINVAL;
	}

	if(size < PAGE_SIZE) {
		dev_err(vivo_bridge->parent, "Size is not on a page boundary\n");
		return -EINVAL;
	}

	/* Convert 64-bit variables to 2x 32-bit variables */
	reg_split(vme_base, &vme_base_high, &vme_base_low);
	reg_split(pci_base, &pci_base_high, &pci_base_low);

	/* Get the ending VME address */
	vme_end_addr = vme_base + size;

	if((cycle & VME_2eSST) || (cycle & VME_2eSSTB)) {

		/* VIVO must have separate windows for 2ESST and 2ESSTB */
		if((cycle & VME_2eSST) && (cycle & VME_2eSSTB)) {

			dev_err(vivo_bridge->parent,
				"Cycle must be SCT/BLT/MBLT or 2ESST or 2ESSTB\n");
			return -EINVAL;
		}

		/* VIVO cannot have SCT/BLT/MBLT with a 2ESST window */
		if(cycle & (VME_SCT | VME_BLT | VME_MBLT)) {

			dev_err(vivo_bridge->parent,
				"Cycle must be SCT/BLT/MBLT or 2ESST or 2ESSTB\n");
			return -EINVAL;
		}

		if(cycle & VME_2eSST)
			xam_code = VIVO_XAMCODE_A32_2ESST;
		else
			xam_code = VIVO_XAMCODE_A32_2ESSTB;

		/* Enable burst */
		temp_ctl |= VIVO_SLVW_CTRLX_BE;

		/* Set up 2eSST speeds */
		switch (cycle & (VME_2eSST160 | VME_2eSST267 | VME_2eSST320)) {
		case VME_2eSST160:
			temp_ctl |= VIVO_SLVW_CTRLX_2ESST_RATE_160;
			break;
		case VME_2eSST267:
			temp_ctl |= VIVO_SLVW_CTRLX_2ESST_RATE_267;
			break;
		case VME_2eSST320:
			temp_ctl |= VIVO_SLVW_CTRLX_2ESST_RATE_320;
			break;
		}
	} else {

		switch (aspace) {
		case VME_A16:

			/* The end address should fit within A16 space */
			if(vme_end_addr > 0x10000) {
				dev_err(vivo_bridge->parent,
					"Invalid size for A16 address space\n");
				return -EINVAL;
			}

			/* Use A16 */
			am_code = VIVO_AMCODE_A16;
			break;
		case VME_A24:

			/* The end address should fit within A24 space */
			if(vme_end_addr > 0x1000000) {
				dev_err(vivo_bridge->parent,
					"Invalid size for A24 address space\n");
				return -EINVAL;
			}

			/* Use A24 */
			am_code = VIVO_AMCODE_A24;
			break;
		case VME_A32:

			/* The end address should fit within A32 space */
			if(vme_end_addr > 0x100000000) {
				dev_err(vivo_bridge->parent,
					"Invalid size for A32 address space\n");
				return -EINVAL;
			}

			/* Use A32 */
			am_code = VIVO_AMCODE_A32;
			break;
		case VME_A64:
		default:
			dev_err(vivo_bridge->parent, "Invalid address space\n");
			return -EINVAL;
		}

		/* Check for address modes */

		/* Ensure program or data is selected */
		if(!(cycle & VME_PROG) && !(cycle & VME_DATA)) {
			dev_err(vivo_bridge->parent, "Invalid address mode\n");
			return -EINVAL;
		}

		/* Ensure supervisor or user is selected */
		if(!(cycle & VME_SUPER) && !(cycle & VME_USER)) {
			dev_err(vivo_bridge->parent, "Invalid address mode\n");
			return -EINVAL;
		}

		if(cycle & VME_DATA) {
			temp_ctl |= VIVO_SLVW_CTRLX_DAT_EBL;

			/* Adjust the AM code */
			am_code |= VIVO_AMCODE_DATA;
		}

		if(cycle & VME_PROG) {
			if(aspace == VME_A16) {
				dev_err(vivo_bridge->parent, "A16 is not valid for program mode\n");
				return -EINVAL;
			}

			temp_ctl |= VIVO_SLVW_CTRLX_PRG_EBL;

			/* Adjust the AM code */
			if(!(am_code & VIVO_AMCODE_DATA))
				am_code |= VIVO_AMCODE_PROG;
		}

		if(cycle & VME_USER) {

			temp_ctl |= VIVO_SLVW_CTRLX_NPRIV_EBL;

			/* Adjust the AM code */
			am_code |= VIVO_AMCODE_USER;
		}

		if(cycle & VME_SUPER) {

			temp_ctl |= VIVO_SLVW_CTRLX_SUP_EBL;

			/* Adjust the AM code */
			if(!(am_code & VIVO_AMCODE_USER))
				am_code |= VIVO_AMCODE_SUPER;
		}

		/* Check for BLT modes */
		if(cycle & VME_BLT)
			temp_ctl |= VIVO_SLVW_CTRLX_BLT_EBL;
		if(cycle & VME_MBLT)
			temp_ctl |= VIVO_SLVW_CTRLX_MBLT_EBL;
	}

	/* Now set the CSR_ADER value appropriately */
	if(xam_code) {

		/* Use XAM mode */
		csr_ader |= VIVO_CSR_ADDRX_XAM_MODE;

		/* Set the XAM field */
		csr_ader |= (xam_code << 2);

		/* Set the VME address */
		csr_ader |= (vme_base_low & VIVO_CSR_ADDRX_C);
	} else {

		/* Set the AM code */
		csr_ader |= (am_code << 2);

		/* Set the VME address */
		csr_ader |= (vme_base_low & (VIVO_CSR_ADDRX_C | VIVO_CSR_ADDRX_XAM));
	}

	/* Set up the burst length */
	temp_ctl |= (255 << 24);

	mutex_lock(&image->mtx);

	/* Disable while we are setting up */
	iowrite32(0, bridge->base + VIVO_SLVW_CTRL[i]);

	/* Set up mapping */
	iowrite32(pci_base_low, bridge->base + VIVO_SLVW_OFFSET[i]);
	iowrite32(pci_base_high, bridge->base + VIVO_SLVW_OFFSETU[i]);
	iowrite32((u32) (~(size - 1)), bridge->base + VIVO_SLVW_MSK[i]);

	iowrite32((csr_ader & 0xFF000000), bridge->base + VIVO_CSR_ADER_HH[i]);
	iowrite32((csr_ader & 0x00FF0000) << 8, bridge->base + VIVO_CSR_ADER_HL[i]);
	iowrite32((csr_ader & 0x0000FF00) << 16, bridge->base + VIVO_CSR_ADER_LH[i]);
	iowrite32((csr_ader & 0x000000FF) << 24, bridge->base + VIVO_CSR_ADER_LL[i]);

	if(enabled)
		temp_ctl |= VIVO_SLVW_CTRLX_EBL;

	iowrite32(temp_ctl, bridge->base + VIVO_SLVW_CTRL[i]);

	mutex_unlock(&image->mtx);

	return 0;
}

/*
 * Get slave window configuration.
 */
static int
vivo_slave_get(struct vme_slave_resource *image, int *enabled,
	unsigned long long *vme_base, unsigned long long *size,
	dma_addr_t * pci_base, u32 * aspace, u32 * cycle)
{
	u32 i = 0;
	u32 ctl = 0;
	u32 pci_base_low, pci_base_high;
	u32 mask;
	u32 csr_ader = 0;
	u32 vme_base_low, vme_base_high;
	u32 xam_code = 0;
	u32 am_code = 0;

	struct vme_bridge *vivo_bridge;
	struct vivo_driver *bridge;

	vivo_bridge = image->parent;
	bridge = vivo_bridge->driver_priv;

	i = image->number;

	mutex_lock(&image->mtx);

	/* Read registers */
	ctl = ioread32(bridge->base + VIVO_SLVW_CTRL[i]);
	pci_base_low = ioread32(bridge->base + VIVO_SLVW_OFFSET[i]);
	pci_base_high = ioread32(bridge->base + VIVO_SLVW_OFFSETU[i]);
	mask = ioread32(bridge->base + VIVO_SLVW_MSK[i]);
	csr_ader = ioread32(bridge->base + VIVO_CSR_ADER_HH[i]);
	csr_ader |= (ioread32(bridge->base + VIVO_CSR_ADER_HL[i]) >> 8);
	csr_ader |= (ioread32(bridge->base + VIVO_CSR_ADER_LH[i]) >> 16);
	csr_ader |= (ioread32(bridge->base + VIVO_CSR_ADER_LL[i]) >> 24);

	mutex_unlock(&image->mtx);

	reg_join(pci_base_high, pci_base_low, pci_base);

	*enabled = 0;
	*aspace = 0;
	*cycle = 0;

	if(ctl & VIVO_SLVW_CTRLX_EBL)
		*enabled = 1;

	/* Calculate the size */
	*size = ~mask + 1;

	/* If XAM mode is on */
	if(csr_ader & VIVO_CSR_ADDRX_XAM_MODE) {

		/* Get the upper 22 bits for the VME address */
		vme_base_high = 0;
		vme_base_low = csr_ader & VIVO_CSR_ADDRX_C;
		reg_join(vme_base_high, vme_base_low, vme_base);

		/* Get the XAM code */
		xam_code = (csr_ader & (VIVO_CSR_ADDRX_AM | VIVO_CSR_ADDRX_XAM)) >> 2;

		if((xam_code & 0x3f) == VIVO_XAMCODE_A32_2ESST)
			*cycle |= VME_2eSST;

		if((xam_code & 0x3f) == VIVO_XAMCODE_A32_2ESSTB)
			*cycle |= VME_2eSSTB;

		/* Get the rate */
		switch (ctl & VIVO_SLVW_CTRLX_2ESST_RATE) {

		case VIVO_SLVW_CTRLX_2ESST_RATE_160:
			*cycle |= VME_2eSST160;
			break;
		case VIVO_SLVW_CTRLX_2ESST_RATE_267:
			*cycle |= VME_2eSST267;
			break;
		case VIVO_SLVW_CTRLX_2ESST_RATE_320:
			*cycle |= VME_2eSST320;
			break;
		default:
			dev_err(vivo_bridge->parent, "Invalid 2eSST rate: reserved\n");
		}

		/* This mode is always A32 */
		*aspace |= VME_A32;
	} else {

		/* Get the upper 24 bits for the VME address */
		vme_base_high = 0;
		vme_base_low = csr_ader & (VIVO_CSR_ADDRX_C | VIVO_CSR_ADDRX_XAM);
		reg_join(vme_base_high, vme_base_low, vme_base);

		/* Get the AM code */
		am_code = (csr_ader & VIVO_CSR_ADDRX_AM) >> 2;

		/* Verify one of SUPER or USER is selected */
		if(am_code & 0xC) {

			/* Determine the address space */
			if((am_code & 0x30) == VIVO_AMCODE_A16)
				*aspace |= VME_A16;
			else if((am_code & 0x30) == VIVO_AMCODE_A24)
				*aspace |= VME_A24;
			else if((am_code & 0x30) == VIVO_AMCODE_A32)
				*aspace |= VME_A32;

			if((am_code & 0xC) == VIVO_AMCODE_USER)
				*aspace |= VME_USER;
			else if((am_code & 0xC) == VIVO_AMCODE_SUPER)
				*aspace |= VME_SUPER;
			else if((am_code & 0x3) == VIVO_AMCODE_DATA)
				*aspace |= VME_DATA;
			else if((am_code & 0x3) == VIVO_AMCODE_PROG)
				*aspace |= VME_PROG;

			/* Now check the other possible settings */
			if(ctl & VIVO_SLVW_CTRLX_PRG_EBL)
				*aspace |= VME_PROG;
			if(ctl & VIVO_SLVW_CTRLX_DAT_EBL)
				*aspace |= VME_DATA;
			if(ctl & VIVO_SLVW_CTRLX_SUP_EBL)
				*aspace |= VME_SUPER;
			if(ctl & VIVO_SLVW_CTRLX_NPRIV_EBL)
				*aspace |= VME_USER;
		}

		/* Transfer mode is always SCT */
		*cycle |= VME_SCT;

		if(ctl & VIVO_SLVW_CTRLX_BLT_EBL)
			*cycle |= VME_BLT;
		if(ctl & VIVO_SLVW_CTRLX_MBLT_EBL)
			*cycle |= VME_MBLT;
	}

	return 0;
}

/*
 * Allocate and map PCI Resource
 */
static int vivo_alloc_resource(struct vme_master_resource *image, unsigned long long size)
{
	unsigned long long existing_size;
	int retval = 0;
	struct pci_dev *pdev;
	struct vme_bridge *vivo_bridge;
	struct vivo_driver *vivo_device;

	vivo_bridge = image->parent;
	vivo_device = vivo_bridge->driver_priv;

	pdev = to_pci_dev(vivo_bridge->parent);

	existing_size = (unsigned long long)(image->bus_resource.end - image->bus_resource.start);

	/* If the existing size is OK, return */
	if((size != 0) && (existing_size == (size - 1)))
		return 0;

	if(existing_size != 0) {
		iounmap(image->kern_base);
		image->kern_base = NULL;
		kfree(image->bus_resource.name);
		release_resource(&image->bus_resource);
		memset(&image->bus_resource, 0, sizeof(struct resource));
	}

	/* Exit here if size is zero */
	if(size == 0)
		return 0;

	if(image->bus_resource.name == NULL) {
		image->bus_resource.name = kmalloc(VMENAMSIZ + 3, GFP_ATOMIC);
		if(image->bus_resource.name == NULL) {
			dev_err(vivo_bridge->parent,
				"Unable to allocate memory for resource name\n");
			retval = -ENOMEM;
			goto err_name;
		}
	}

	sprintf((char *)image->bus_resource.name, "%s.%d", vivo_bridge->name, image->number);

	image->bus_resource.start = 0;
	image->bus_resource.end = (unsigned long)size;
	image->bus_resource.flags = IORESOURCE_MEM;

	retval = allocate_resource(&(vivo_device->pci_resource),
				&(image->bus_resource),
				size,
				vivo_device->pci_resource.start,
				vivo_device->pci_resource.end, size, 0, 0);

	dev_dbg(vivo_bridge->parent,
		"%s allocate_resource start %llx end %llx\n retval 0x%x",
		__func__, image->bus_resource.start, image->bus_resource.end, retval);

	if(retval) {
		dev_err(vivo_bridge->parent,
			"Failed to allocate mem resource for win %d size 0x%lx start 0x%lx\n",
			image->number, (unsigned long)size,
			(unsigned long)image->bus_resource.start);
		goto err_resource;
	}

	image->kern_base = ioremap_nocache(image->bus_resource.start, size);
	if(image->kern_base == NULL) {
		dev_err(vivo_bridge->parent, "Failed to remap resource\n");
		retval = -ENOMEM;
		goto err_remap;
	}

	return 0;

err_remap:
	release_resource(&image->bus_resource);
err_resource:
	kfree(image->bus_resource.name);
	memset(&image->bus_resource, 0, sizeof(struct resource));
err_name:
	return retval;
}

/*
 * Free and unmap PCI Resource
 */
static void vivo_free_resource(struct vme_master_resource *image)
{
	if(image->kern_base != NULL) {

		iounmap(image->kern_base);
		image->kern_base = NULL;
		release_resource(&image->bus_resource);
		kfree(image->bus_resource.name);
		memset(&image->bus_resource, 0, sizeof(struct resource));
	}
}

/*
 * Set the attributes of an outbound window.
 */
static int
vivo_master_set(struct vme_master_resource *image, int enabled,
		unsigned long long vme_base, unsigned long long size,
		u32 aspace, u32 cycle, u32 dwidth)
{
	int retval = 0;
	unsigned int i;
	u32 temp_ctl = 0;
	u32 pci_base_low, pci_base_high;
	u32 vme_base_low, vme_base_high;
	u64 pci_base;
	struct vme_bridge *vivo_bridge;
	struct vivo_driver *bridge;
	struct pci_bus_region region;
	struct pci_dev *pdev;

	vivo_bridge = image->parent;

	bridge = vivo_bridge->driver_priv;

	pdev = to_pci_dev(vivo_bridge->parent);

	if((size == 0) && (enabled != 0)) {
		dev_err(vivo_bridge->parent, "Size must be non-zero for enabled windows\n");
		retval = -EINVAL;
		goto err_window;
	}

	if((size != 0) && (enabled != 0)) {
		// JLAB: Added (size < 0xffffff)
		// Using 0x0A000000 for A32 appears to work fine.
		if((size < 0xffffff) && (!is_power_of_2__64(size))) {
			dev_err(vivo_bridge->parent, "Size (%lld) must be a power of 2\n", size);
			retval = -EINVAL;
			goto err_window;
		}

	}

	spin_lock(&image->lock);

	/* Let's allocate the resource here rather than further up the stack as
	 * it avoids pushing loads of bus dependent stuff up the stack. If size
	 * is zero, any existing resource will be freed.
	 */
	retval = vivo_alloc_resource(image, size);
	if(retval) {
		spin_unlock(&image->lock);
		dev_err(vivo_bridge->parent, "Unable to allocate memory for resource\n");
		goto err_res;
	}

	if(size == 0)
		pci_base = 0;
	else {
		pcibios_resource_to_bus(pdev->bus, &region, &image->bus_resource);
		pci_base = region.start;
	}

	/* Convert 64-bit variables to 2x 32-bit variables */
	reg_split(pci_base, &pci_base_high, &pci_base_low);
	reg_split(vme_base, &vme_base_high, &vme_base_low);

	i = image->number;

	/* Disable while we are setting up */
	iowrite32(0, bridge->base + VIVO_MW_CTRL[i]);

	/* Set up cycle types, defaulting to SCT */
	temp_ctl &= ~VIVO_MW_CTRLX_BT;

	/* Only SCT mode is supported through master windows */
	if(cycle & (VME_BLT | VME_MBLT | VME_2eVME | VME_2eSST | VME_2eSSTB)) {
		spin_unlock(&image->lock);
		dev_err(vivo_bridge->parent, "Invalid VME Transfer Mode\n");
		retval = -EINVAL;
		goto err_cycle;
	}

	/* Set up data width */
	switch (dwidth) {
	case VME_D8:
	case VME_D16:
	case VME_D32:
	case VME_D64:
		break;
	default:
		spin_unlock(&image->lock);
		dev_err(vivo_bridge->parent, "Invalid data width\n");
		retval = -EINVAL;
		goto err_dwidth;
	}

	/* Set up address space */
	temp_ctl &= ~VIVO_MW_CTRLX_AM_AS;
	switch (aspace) {
	case VME_A16:
		temp_ctl |= VIVO_MW_CTRLX_AM_AS_A16;
		break;
	case VME_A24:
		temp_ctl |= VIVO_MW_CTRLX_AM_AS_A24;
		break;
	case VME_A32:
		temp_ctl |= VIVO_MW_CTRLX_AM_AS_A32;
		break;
	case VME_CRCSR:
		temp_ctl |= VIVO_MW_CTRLX_AM_AS_CRCSR;
		break;
	case VME_USER1:
		temp_ctl |= VIVO_MW_CTRLX_AM_AS_USER1;
		break;
	case VME_USER2:
		temp_ctl |= VIVO_MW_CTRLX_AM_AS_USER2;
		break;
	default:
		spin_unlock(&image->lock);
		dev_err(vivo_bridge->parent, "Invalid address space\n");
		retval = -EINVAL;
		goto err_aspace;
	}

	/* Make sure DATA and PROGRAM are not both selected */
	if((cycle & VME_DATA) && (cycle & VME_PROG)) {
		spin_unlock(&image->lock);

		if((size != 0) && (enabled != 0))
			dev_err(vivo_bridge->parent, "Invalid address mode\n");

		retval = -EINVAL;
		goto err_amode;
	}

	/* A16 does not support PROGRAM */
	if((cycle & VME_PROG) && (aspace == VME_A16)) {
		spin_unlock(&image->lock);

		if((size != 0) && (enabled != 0))
			dev_err(vivo_bridge->parent, "Invalid A16 address mode\n");

		retval = -EINVAL;
		goto err_amode;
	}

	/* Set up data/program access, defaulting to DATA */
	temp_ctl &= ~VIVO_MW_CTRLX_AM_DA;
	if(cycle & VME_PROG)
		temp_ctl |= VIVO_MW_CTRLX_AM_DA_PROGRAM;

	/* Set up non-privileged/supervisory access, defaulting to USER */
	temp_ctl &= ~VIVO_MW_CTRLX_AM_NPA;
	if(cycle & VME_SUPER)
		temp_ctl |= VIVO_MW_CTRLX_AM_NPA_SUPER;

	/* Enable posted writes */
	/* temp_ctl |= VIVO_MW_CTRLX_SCTPWR_POSTED; */

	/* Disable posted writes */
	temp_ctl &= ~VIVO_MW_CTRLX_SCTPWR_POSTED;

	/* Set up mapping */
	iowrite32((u32) (~(size - 1)), bridge->base + VIVO_MW_MASK[i]);
	iowrite32(pci_base_low, bridge->base + VIVO_MW_ADDR[i]);
	iowrite32(vme_base_low, bridge->base + VIVO_MW_OFFSET[i]);
	iowrite32(0, bridge->base + VIVO_MW_2ESST[i]);

	if(enabled)
		temp_ctl |= VIVO_MW_CTRLX_EBL;

	iowrite32(temp_ctl, bridge->base + VIVO_MW_CTRL[i]);

	spin_unlock(&image->lock);
	return 0;

err_amode:
err_aspace:
err_dwidth:
err_cycle:
	vivo_free_resource(image);
err_res:
err_window:
	return retval;

}

/*
 * Get the attributes of an outbound window.
 */
static int
__vivo_master_get(struct vme_master_resource *image, int *enabled,
		unsigned long long *vme_base, unsigned long long *size,
		u32 * aspace, u32 * cycle, u32 * dwidth)
{
	unsigned int i;
	u32 ctl;
	u32 mw_pci_base;
	u32 mw_vme_base;
	u32 mw_mask;
	struct vivo_driver *bridge;

	bridge = image->parent->driver_priv;

	i = image->number;

	ctl = ioread32(bridge->base + VIVO_MW_CTRL[i]);

	mw_pci_base = ioread32(bridge->base + VIVO_MW_ADDR[i]);
	mw_vme_base = ioread32(bridge->base + VIVO_MW_OFFSET[i]);
	mw_mask = ioread32(bridge->base + VIVO_MW_MASK[i]);

	*vme_base = (unsigned long long)mw_vme_base;
	*size = (unsigned long long)(~mw_mask + 1);

	*enabled = 0;
	*aspace = 0;
	*cycle = 0;
	*dwidth = 0;

	if(ctl & VIVO_MW_CTRLX_EBL)
		*enabled = 1;

	/* Set up address space */
	if((ctl & VIVO_MW_CTRLX_AM_AS) == VIVO_MW_CTRLX_AM_AS_A16)
		*aspace |= VME_A16;
	if((ctl & VIVO_MW_CTRLX_AM_AS) == VIVO_MW_CTRLX_AM_AS_A24)
		*aspace |= VME_A24;
	if((ctl & VIVO_MW_CTRLX_AM_AS) == VIVO_MW_CTRLX_AM_AS_A32)
		*aspace |= VME_A32;
	if((ctl & VIVO_MW_CTRLX_AM_AS) == VIVO_MW_CTRLX_AM_AS_CRCSR)
		*aspace |= VME_CRCSR;
	if((ctl & VIVO_MW_CTRLX_AM_AS) == VIVO_MW_CTRLX_AM_AS_USER1)
		*aspace |= VME_USER1;
	if((ctl & VIVO_MW_CTRLX_AM_AS) == VIVO_MW_CTRLX_AM_AS_USER2)
		*aspace |= VME_USER2;

	/* Set up cycle types.  Only SCT is supported for Master Windows. */
	if((ctl & VIVO_MW_CTRLX_BT) == VIVO_MW_CTRLX_BT_SCT)
		*cycle |= VME_SCT;
	else
		dev_err(image->parent->parent, "Invalid cycle type for Master window\n");

	if(ctl & VIVO_MW_CTRLX_AM_NPA_SUPER)
		*cycle |= VME_SUPER;
	else
		*cycle |= VME_USER;

	if(ctl & VIVO_MW_CTRLX_AM_DA_PROGRAM)
		*cycle |= VME_PROG;
	else
		*cycle |= VME_DATA;

	/* The data width is not programmable in the master window */
	*dwidth = 0;

	return 0;
}

static int
vivo_master_get(struct vme_master_resource *image, int *enabled,
		unsigned long long *vme_base, unsigned long long *size,
		u32 * aspace, u32 * cycle, u32 * dwidth)
{
	int retval;

	spin_lock(&image->lock);

	retval = __vivo_master_get(image, enabled, vme_base, size, aspace, cycle, dwidth);

	spin_unlock(&image->lock);

	return retval;
}

static ssize_t
vivo_master_read(struct vme_master_resource *image, void *buf, size_t count, loff_t offset)
{
	int retval, enabled;
	unsigned long long vme_base, size;
	u32 aspace, cycle, dwidth;
	struct vme_error_handler *handler = NULL;
	struct vme_bridge *vivo_bridge;
	void __iomem *addr = image->kern_base + offset;
	unsigned int done = 0;
	unsigned int count32;

	vivo_bridge = image->parent;

	spin_lock(&image->lock);

	if(err_chk) {
		__vivo_master_get(image, &enabled, &vme_base, &size, &aspace, &cycle, &dwidth);
		handler = vme_register_error_handler(vivo_bridge, aspace, vme_base + offset, count);
		if(!handler) {
			spin_unlock(&image->lock);
			return -ENOMEM;
		}
	}

	/* The following code handles VME address alignment. We cannot use
	 * memcpy_xxx here because it may cut data transfers in to 8-bit
	 * cycles when D16 or D32 cycles are required on the VME bus.
	 * On the other hand, the bridge itself assures that the maximum data
	 * cycle configured for the transfer is used and splits it
	 * automatically for non-aligned addresses, so we don't want the
	 * overhead of needlessly forcing small transfers for the entire cycle.
	 */
	if((uintptr_t) addr & 0x1) {
		*(u8 *) buf = ioread8(addr);
		done += 1;
		if(done == count)
			goto out;
	}
	if((uintptr_t) (addr + done) & 0x2) {
		if((count - done) < 2) {
			*(u8 *) (buf + done) = ioread8(addr + done);
			done += 1;
			goto out;
		} else {
			*(u16 *) (buf + done) = ioread16(addr + done);
			done += 2;
		}
	}

	count32 = (count - done) & ~0x3;
	while(done < count32) {
		*(u32 *) (buf + done) = ioread32(addr + done);
		done += 4;
	}

	if((count - done) & 0x2) {
		*(u16 *) (buf + done) = ioread16(addr + done);
		done += 2;
	}
	if((count - done) & 0x1) {
		*(u8 *) (buf + done) = ioread8(addr + done);
		done += 1;
	}

out:
	retval = count;

	if(err_chk) {
		if(handler->num_errors) {
			dev_err(image->parent->parent,
				"First VME read error detected an at address 0x%llx\n",
				handler->first_error);
			retval = handler->first_error - (vme_base + offset);
		}
		vme_unregister_error_handler(handler);
	}

	spin_unlock(&image->lock);

	return retval;
}

static ssize_t
vivo_master_write(struct vme_master_resource *image, void *buf, size_t count, loff_t offset)
{
	int retval = 0, enabled;
	unsigned long long vme_base, size;
	u32 aspace, cycle, dwidth;
	void __iomem *addr = image->kern_base + offset;
	unsigned int done = 0;
	unsigned int count32;

	struct vme_error_handler *handler = NULL;
	struct vme_bridge *vivo_bridge;
	struct vivo_driver *bridge;

	vivo_bridge = image->parent;

	bridge = vivo_bridge->driver_priv;

	spin_lock(&image->lock);

	if(err_chk) {
		__vivo_master_get(image, &enabled, &vme_base, &size, &aspace, &cycle, &dwidth);
		handler = vme_register_error_handler(vivo_bridge, aspace, vme_base + offset, count);
		if(!handler) {
			spin_unlock(&image->lock);
			return -ENOMEM;
		}
	}

	/* Here we apply for the same strategy we do in master_read
	 * function in order to assure the correct cycles.
	 */
	if((uintptr_t) addr & 0x1) {
		iowrite8(*(u8 *) buf, addr);
		done += 1;
		if(done == count)
			goto out;
	}
	if((uintptr_t) (addr + done) & 0x2) {
		if((count - done) < 2) {
			iowrite8(*(u8 *) (buf + done), addr + done);
			done += 1;
			goto out;
		} else {
			iowrite16(*(u16 *) (buf + done), addr + done);
			done += 2;
		}
	}

	count32 = (count - done) & ~0x3;
	while(done < count32) {
		iowrite32(*(u32 *) (buf + done), addr + done);
		done += 4;
	}

	if((count - done) & 0x2) {
		iowrite16(*(u16 *) (buf + done), addr + done);
		done += 2;
	}
	if((count - done) & 0x1) {
		iowrite8(*(u8 *) (buf + done), addr + done);
		done += 1;
	}

out:
	retval = count;

	/* Check for saved errors in the written address range/space. */
	if(err_chk) {

		if(handler->num_errors) {
			dev_warn(vivo_bridge->parent,
				"First VME write error detected an at address 0x%llx\n",
				handler->first_error);
			retval = handler->first_error - (vme_base + offset);
		}
		vme_unregister_error_handler(handler);
	}

	spin_unlock(&image->lock);

	return retval;
}

static int
vivo_dma_set_vme_src_attributes(struct device *dev, u32 * attr, u32 aspace, u32 cycle, u32 dwidth)
{
	u32 val;

	val = *attr;

	/* Set up 2eSST speeds */
	switch (cycle & (VME_2eSST160 | VME_2eSST267 | VME_2eSST320)) {
	case VME_2eSST160:
		val |= VIVO_DMASATTX_SST_160;
		break;
	case VME_2eSST267:
		val |= VIVO_DMASATTX_SST_267;
		break;
	case VME_2eSST320:
		val |= VIVO_DMASATTX_SST_320;
		break;
	}

	/* Set up cycle types */
	if(cycle & VME_SCT)
		val |= VIVO_DMASATTX_BT_SCT;

	if(cycle & VME_BLT)
		val |= VIVO_DMASATTX_BT_BLT;

	if(cycle & VME_MBLT)
		val |= VIVO_DMASATTX_BT_MBLT;

	if(cycle & VME_2eSST)
		val |= VIVO_DMASATTX_BT_2eSST;

	if(cycle & VME_2eSSTB) {
		dev_err(dev, "Currently not setting Broadcast Select Registers\n");
		val |= VIVO_DMASATTX_BT_2eSSTB;
	}

	/* Set up address space */
	switch (aspace) {
	case VME_A16:

		/* Non-SCT DMA in A16 space is not a valid operation */
		if(cycle & (VME_BLT | VME_MBLT | VME_2eSST | VME_2eSSTB)) {
			dev_err(dev, "Non-SCT DMA not valid in A16 space\n");
			return -EINVAL;
		}

		val |= VIVO_DMASATTX_AM_AS_A16;
		break;
	case VME_A24:
		val |= VIVO_DMASATTX_AM_AS_A24;
		break;
	case VME_A32:
		val |= VIVO_DMASATTX_AM_AS_A32;
		break;
	case VME_CRCSR:
		val |= VIVO_DMASATTX_AM_AS_CRCSR;
		break;
	case VME_USER1:
		val |= VIVO_DMASATTX_AM_AS_USER1;
		break;
	case VME_USER2:
		val |= VIVO_DMASATTX_AM_AS_USER2;
		break;
	default:
		dev_err(dev, "Invalid address space\n");
		return -EINVAL;
	}

	if(cycle & VME_SUPER)
		val |= VIVO_DMASATTX_AM_NPA_SUPER;
	if(cycle & VME_PROG)
		val |= VIVO_DMASATTX_AM_DA_PROGRAM;

	*attr = val;

	return 0;
}

static int
vivo_dma_set_vme_dest_attributes(struct device *dev, u32 * attr, u32 aspace, u32 cycle, u32 dwidth)
{
	u32 val;

	val = *attr;

	/* Set up 2eSST speeds */
	switch (cycle & (VME_2eSST160 | VME_2eSST267 | VME_2eSST320)) {
	case VME_2eSST160:
		val |= VIVO_DMADATTX_SST_160;
		break;
	case VME_2eSST267:
		val |= VIVO_DMADATTX_SST_267;
		break;
	case VME_2eSST320:
		val |= VIVO_DMADATTX_SST_320;
		break;
	}

	/* Set up cycle types */
	if(cycle & VME_SCT)
		val |= VIVO_DMADATTX_BT_SCT;

	if(cycle & VME_BLT)
		val |= VIVO_DMADATTX_BT_BLT;

	if(cycle & VME_MBLT)
		val |= VIVO_DMADATTX_BT_MBLT;

	if(cycle & VME_2eSST)
		val |= VIVO_DMADATTX_BT_2eSST;

	if(cycle & VME_2eSSTB) {
		dev_err(dev, "Currently not setting Broadcast Select Registers\n");
		val |= VIVO_DMADATTX_BT_2eSSTB;
	}

	/* Set up address space */
	switch (aspace) {
	case VME_A16:

		/* Non-SCT DMA in A16 space is not a valid operation */
		if(cycle & (VME_BLT | VME_MBLT | VME_2eSST | VME_2eSSTB)) {
			dev_err(dev, "Non-SCT DMA not valid in A16 space\n");
			return -EINVAL;
		}

		val |= VIVO_DMADATTX_AM_AS_A16;
		break;
	case VME_A24:
		val |= VIVO_DMADATTX_AM_AS_A24;
		break;
	case VME_A32:
		val |= VIVO_DMADATTX_AM_AS_A32;
		break;
	case VME_CRCSR:
		val |= VIVO_DMADATTX_AM_AS_CRCSR;
		break;
	case VME_USER1:
		val |= VIVO_DMADATTX_AM_AS_USER1;
		break;
	case VME_USER2:
		val |= VIVO_DMADATTX_AM_AS_USER2;
		break;
	default:
		dev_err(dev, "Invalid address space\n");
		return -EINVAL;
	}

	if(cycle & VME_SUPER)
		val |= VIVO_DMADATTX_AM_NPA_SUPER;
	if(cycle & VME_PROG)
		val |= VIVO_DMADATTX_AM_DA_PROGRAM;

	*attr = val;

	return 0;
}

/*
 * Add a link list descriptor to the list
 *
 * Note: DMA engine expects the DMA descriptor to be little endian.
 */
static int
vivo_dma_list_add(struct vme_dma_list *list,
		struct vme_dma_attr *src, struct vme_dma_attr *dest, size_t count)
{
	struct vivo_dma_entry *entry, *prev, *aligned_entry;
	u32 address_high, address_low;
	u32 addr_alignment_mask;
	struct vme_dma_pci *pci_attr;
	struct vme_dma_vme *vme_attr;
	int retval = 0;
	struct vme_bridge *vivo_bridge;

	vivo_bridge = list->parent->parent;

	/* Dma addresses must be on an 8-byte bound, unless 2ESST or 2ESSTB */
	addr_alignment_mask = 0x00000007;

	/* Descriptor must be aligned on 64-byte boundaries. */
	entry = kmalloc(sizeof(struct vivo_dma_entry) + VIVO_DMA_DESCRIPTOR_ALIGN, GFP_KERNEL);
	if(entry == NULL) {
		dev_err(vivo_bridge->parent,
			"Failed to allocate memory for dma resource structure\n");
		retval = -ENOMEM;
		goto err_mem;
	}

	/* Align the vivo_dma_entry on a 64-byte boundary to
	 * ensure the descriptor is aligned.
	 */
	aligned_entry = (struct vivo_dma_entry *)
		(((u64) entry & ~((u64) VIVO_DMA_DESCRIPTOR_ALIGN - (u64) 1)) +
			(u64) VIVO_DMA_DESCRIPTOR_ALIGN);

	aligned_entry->kmalloc_address = entry;

	entry = aligned_entry;

	/* Test descriptor alignment */
	if((unsigned long)&entry->desc & (VIVO_DMA_DESCRIPTOR_ALIGN - 1)) {
		dev_err(vivo_bridge->parent,
			"Descriptor not aligned to 64 byte boundary as required: %p\n",
			&entry->desc);
		retval = -EINVAL;
		goto err_align;
	}

	/* Given we are going to fill out the structure, we probably don't
	 * need to zero it, but better safe than sorry for now.
	 */
	memset(&entry->desc, 0, sizeof(struct vivo_dma_descriptor));

	/* Fill out source part */
	dev_dbg(vivo_bridge->parent,
		"SRC TYPE = %s (%d)\n",
		(src->type == VME_DMA_PCI) ? "VME_DMA_PCI" :
		(src->type == VME_DMA_VME) ? "VME_DMA_VME" : "???", src->type);

	switch (src->type) {
	case VME_DMA_PCI:
		pci_attr = src->private;

		reg_split((unsigned long long)pci_attr->address, &address_high, &address_low);
		entry->desc.sau = address_high;
		entry->desc.sa = address_low;
		entry->desc.satt = VIVO_DMASATTX_DPRT_PCISRC;
		break;
	case VME_DMA_VME:
		vme_attr = src->private;

		reg_split((unsigned long long)vme_attr->address, &address_high, &address_low);

		if(address_high != 0) {
			dev_err(vivo_bridge->parent, "A64 DMA request is invalid\n");
			retval = -EINVAL;
			goto err_source;
		}

		entry->desc.sau = address_high;
		entry->desc.sa = address_low;
		entry->desc.satt = VIVO_DMASATTX_DPRT_VMESRC;

		retval =
			vivo_dma_set_vme_src_attributes(vivo_bridge->parent,
							&entry->desc.satt,
							vme_attr->aspace,
							vme_attr->cycle, vme_attr->dwidth);
		if(retval < 0)
			goto err_source;

		/* If cycle is 2ESST or 2ESSTB
		 * Then addresses must be aligned on a 16-byte bound
		 */
		if((vme_attr->cycle & VME_2eSST)
			|| (vme_attr->cycle & VME_2eSSTB))
			addr_alignment_mask = 0x0000000F;

		break;
	default:
		dev_err(vivo_bridge->parent, "Invalid source type\n");
		retval = -EINVAL;
		break;
	}

	/* Assume last link - this will be over-written by adding another */
	entry->desc.ndescu = 0;
	entry->desc.ndesc = VIVO_DMANDESCX_EOL;

	/* Fill out destination part */
	dev_dbg(vivo_bridge->parent,
		"DST TYPE = %s (%d)\n",
		(dest->type == VME_DMA_PCI) ? "VME_DMA_PCI" :
		(dest->type == VME_DMA_VME) ? "VME_DMA_VME" : "???", dest->type);
	switch (dest->type) {
	case VME_DMA_PCI:
		pci_attr = dest->private;

		reg_split((unsigned long long)pci_attr->address, &address_high, &address_low);
		entry->desc.dau = address_high;
		entry->desc.da = address_low;
		entry->desc.datt = VIVO_DMASATTX_DPRT_PCISRC;
		break;
	case VME_DMA_VME:
		vme_attr = dest->private;

		reg_split((unsigned long long)vme_attr->address, &address_high, &address_low);

		if(address_high != 0) {
			dev_err(vivo_bridge->parent, "A64 DMA request is invalid\n");
			retval = -EINVAL;
			goto err_dest;
		}

		entry->desc.dau = address_high;
		entry->desc.da = address_low;
		entry->desc.datt = VIVO_DMASATTX_DPRT_VMESRC;

		retval =
			vivo_dma_set_vme_dest_attributes(vivo_bridge->parent,
							&entry->desc.datt,
							vme_attr->aspace,
							vme_attr->cycle, vme_attr->dwidth);
		if(retval < 0)
			goto err_dest;

		/* If cycle is 2ESST or 2ESSTB
		 * Then addresses must be aligned on a 16-byte bound
		 */
		if((vme_attr->cycle & VME_2eSST)
			|| (vme_attr->cycle & VME_2eSSTB))
			addr_alignment_mask = 0x0000000F;

		break;
	default:
		dev_err(vivo_bridge->parent, "Invalid destination type\n");
		retval = -EINVAL;
		goto err_dest;
	}

	/* Ensure the lower source address is on correct bound */
	if(entry->desc.sa & addr_alignment_mask) {

		dev_err(vivo_bridge->parent,
			"DMA source address must be aligned on a %d byte bound\n",
			addr_alignment_mask + 1);
		retval = -EINVAL;
		goto err_align;
	}

	/* Ensure the lower source destination address is on correct bound */
	if(entry->desc.da & addr_alignment_mask) {

		dev_err(vivo_bridge->parent,
			"DMA destination address must be aligned on a %d byte bound\n",
			addr_alignment_mask + 1);
		retval = -EINVAL;
		goto err_align;
	}

	/* Fill out count */
	entry->desc.tl = (u32) count;

	if(entry->desc.tl & addr_alignment_mask) {

		dev_err(vivo_bridge->parent,
			"DMA size (0x%x) must be aligned on a %d byte bound\n",
			entry->desc.tl, addr_alignment_mask + 1);
		retval = -EINVAL;
		goto err_align;
	}

	dev_dbg(vivo_bridge->parent,
		"DMA Descriptor:\nsa 0x%08x\nsau 0x%08x\nda 0x%08x\ndau 0x%08x\nsatt 0x%08x\ndatt 0x%08x\n"
		"ndesc 0x%08x\nndescu 0x%08x\nbss 0x%08x\ntl 0x%08x\n\n",
		entry->desc.sa, entry->desc.sau,
		entry->desc.da, entry->desc.dau,
		entry->desc.satt, entry->desc.datt,
		entry->desc.ndesc, entry->desc.ndescu, entry->desc.bss, entry->desc.tl);

	/* Save an unmapped copy of the descriptor for access by the CPU after
	 * being mapped to the DMA device.
	 */
	memcpy(&entry->u_desc, &entry->desc, sizeof(struct vivo_dma_descriptor));

	/* Add to list */
	list_add_tail(&entry->list, &list->entries);

	entry->dma_handle = dma_map_single(vivo_bridge->parent,
					&entry->desc,
					sizeof(struct vivo_dma_descriptor), DMA_TO_DEVICE);
	if(dma_mapping_error(vivo_bridge->parent, entry->dma_handle)) {
		dev_err(vivo_bridge->parent, "DMA mapping error\n");
		retval = -EINVAL;
		goto err_dma;
	}

	/* Fill out previous descriptors "Next Address" */
	if(entry->list.prev != &list->entries) {
		reg_split((unsigned long long)entry->dma_handle, &address_high, &address_low);
		prev = list_entry(entry->list.prev, struct vivo_dma_entry, list);

		dma_sync_single_for_cpu(vivo_bridge->parent,
					prev->dma_handle,
					sizeof(struct vivo_dma_descriptor), DMA_TO_DEVICE);

		prev->desc.ndescu = address_high;
		prev->desc.ndesc = address_low;

		dma_sync_single_for_device(vivo_bridge->parent,
					prev->dma_handle,
					sizeof(struct vivo_dma_descriptor), DMA_TO_DEVICE);
	}

	return 0;

err_dma:
err_dest:
err_source:
err_align:
	kfree(entry->kmalloc_address);
err_mem:
	return retval;
}

/*
 * Check to see if the dma interrupt has been received for a channel,
 * return true (1) or false (0).
 */
static int vivo_dma_received(struct vivo_driver *bridge, int channel)
{
	if(bridge->dma_int_count[channel] > 0) {

		bridge->dma_int_count[channel]--;

		return 1;
	} else
		return 0;
}

/*
 * Execute a previously generated link list
 *
 * XXX Need to provide control register configuration.
 */
static int vivo_dma_list_exec(struct vme_dma_list *list)
{
	struct vme_dma_resource *ctrlr;
	int channel, retval;
	struct vivo_dma_entry *entry;
	u32 bus_addr_high, bus_addr_low;
	u32 val, dctlreg = 0;
	struct vme_bridge *vivo_bridge;
	struct vivo_driver *bridge;
	int doSG = 0;

	ctrlr = list->parent;

	vivo_bridge = ctrlr->parent;

	bridge = vivo_bridge->driver_priv;

	mutex_lock(&ctrlr->mtx);

	channel = ctrlr->number;

	if(!list_empty(&ctrlr->running)) {
		/*
		 * XXX We have an active DMA transfer and currently haven't
		 *     sorted out the mechanism for "pending" DMA transfers.
		 *     Return busy.
		 */
		/* Need to add to pending here */
		mutex_unlock(&ctrlr->mtx);
		return -EBUSY;
	} else {
		list_add(&list->list, &ctrlr->running);
	}

	/* Get first bus address and write into registers */
	entry = list_first_entry(&list->entries, struct vivo_dma_entry, list);

	if(doSG == 1) {
		/* Scatter-Gather method,
		   - program address of descriptor into DMA registers
		   - DMA Controller will get the descriptor contents      */
		reg_split(entry->dma_handle, &bus_addr_high, &bus_addr_low);

		iowrite32(bus_addr_high, bridge->base + VIVO_DMACDESCU[channel]);
		iowrite32(bus_addr_low, bridge->base + VIVO_DMACDESCL[channel]);

		dctlreg = ioread32(bridge->base + VIVO_DMACTRL[channel]);

		dctlreg =
			VIVO_DMACTRLX_AHBOT_0us | VIVO_DMACTRLX_VBOT_0us |
			VIVO_DMACTRLX_AHTS_2048 | VIVO_DMACTRLX_VTS_2048;

		dev_info(vivo_bridge->parent,
			"dma_list_exec: SG: before: dctlreg = 0x%08x\n", dctlreg);

		/* Start the operation */
		iowrite32(dctlreg |
			VIVO_DMACTRLX_START_START |
			VIVO_DMACTRLX_MODE_SG, bridge->base + VIVO_DMACTRL[channel]);

		retval = wait_event_interruptible(bridge->dma_queue[channel],
						vivo_dma_received(bridge, channel));

		if(retval) {
			iowrite32(dctlreg | VIVO_DMACTRLX_ABORT_ABORT,
				bridge->base + VIVO_DMACTRL[channel]);
			/* Wait for the operation to abort */
			wait_event(bridge->dma_queue[channel], vivo_dma_received(bridge, channel));
			retval = -EINTR;
			goto exit;
		}

		dev_info(vivo_bridge->parent,
			"dma_list_exec: SG: after: dctlreg = 0x%08x\n", dctlreg);

		/*
		 * Read status register, this register is valid until we kick off a
		 * new transfer.
		 */
		val = ioread32(bridge->base + VIVO_DMASTAT[channel]);

		if(val & VIVO_DMASTATX_ERROR) {
			dev_err(vivo_bridge->parent, "DMA Error. DMASTAT=%08X\n", val);
			retval = -EIO;
		}
	} else {
		/* Direct method,
		   - copy the contents of the descriptor to the vivo dma control regs
		   - this is not effecicent, just for testing */

		iowrite32(entry->desc.sa, bridge->base + VIVO_DMASA0);
		iowrite32(entry->desc.sau, bridge->base + VIVO_DMASAU0);
		iowrite32(entry->desc.da, bridge->base + VIVO_DMADA0);
		iowrite32(entry->desc.dau, bridge->base + VIVO_DMADAU0);
		iowrite32(entry->desc.satt, bridge->base + VIVO_DMASATT0);
		iowrite32(entry->desc.datt, bridge->base + VIVO_DMADATT0);
		iowrite32(entry->desc.bss, bridge->base + VIVO_DMABSS0);
		iowrite32(entry->desc.tl, bridge->base + VIVO_DMATL0);

		iowrite32(0, bridge->base + VIVO_DMACDESCU[channel]);
		iowrite32(0, bridge->base + VIVO_DMACDESCL[channel]);

		dctlreg = ioread32(bridge->base + VIVO_DMACTRL[channel]);

		dctlreg =
			VIVO_DMACTRLX_AHBOT_0us | VIVO_DMACTRLX_VBOT_0us |
			VIVO_DMACTRLX_AHTS_2048 | VIVO_DMACTRLX_VTS_2048;

		dev_info(vivo_bridge->parent,
			"dma_list_exec: DIRECT before: dctlreg = 0x%08x\n", dctlreg);

		/* Start the operation */
		iowrite32(dctlreg |
			VIVO_DMACTRLX_START_START |
			VIVO_DMACTRLX_MODE_DIRECT, bridge->base + VIVO_DMACTRL[channel]);

		retval = wait_event_interruptible(bridge->dma_queue[channel],
						vivo_dma_received(bridge, channel));

		if(retval) {
			iowrite32(dctlreg | VIVO_DMACTRLX_ABORT_ABORT,
				bridge->base + VIVO_DMACTRL[channel]);
			/* Wait for the operation to abort */
			wait_event(bridge->dma_queue[channel], vivo_dma_received(bridge, channel));
			retval = -EINTR;
			goto exit;
		}

		dev_info(vivo_bridge->parent,
			"dma_list_exec DIRECT after: dctlreg = 0x%08x\n", dctlreg);

		/*
		 * Read status register, this register is valid until we kick off a
		 * new transfer.
		 */
		val = ioread32(bridge->base + VIVO_DMASTAT[channel]);

		if(val & VIVO_DMASTATX_ERROR) {
			dev_err(vivo_bridge->parent, "DMA Error. DMASTAT=%08X\n", val);
			retval = -EIO;
		}

	}

exit:
	/* Remove list from running list */
	list_del(&list->list);
	mutex_unlock(&ctrlr->mtx);

	return retval;
}

/*
 * Clean up a previously generated link list
 *
 * We have a separate function, don't assume that the chain can't be reused.
 */
static int vivo_dma_list_empty(struct vme_dma_list *list)
{
	struct list_head *pos, *temp;
	struct vivo_dma_entry *entry;

	struct vme_bridge *vivo_bridge = list->parent->parent;

	/* detach and free each entry */
	list_for_each_safe(pos, temp, &list->entries) {
		list_del(pos);
		entry = list_entry(pos, struct vivo_dma_entry, list);

		dma_unmap_single(vivo_bridge->parent, entry->dma_handle,
				sizeof(struct vivo_dma_descriptor), DMA_TO_DEVICE);
		kfree(entry->kmalloc_address);
	}

	return 0;
}

/*
 * Determine Geographical Addressing
 */
static int vivo_slot_get(struct vme_bridge *vivo_bridge)
{
	u32 slot = 0;
	struct vivo_driver *bridge;

	bridge = vivo_bridge->driver_priv;

	if(!geoid) {
		slot = ioread32(bridge->base + VIVO_SYS_CTRL);
		slot = slot & VIVO_SYS_CTRL_GA;
	} else
		slot = geoid;

	return (int)slot;
}

static void *vivo_alloc_consistent(struct device *parent, size_t size, dma_addr_t * dma)
{
	struct pci_dev *pdev;

	/* Find pci_dev container of dev */
	pdev = to_pci_dev(parent);

	return pci_alloc_consistent(pdev, size, dma);
}

static void vivo_free_consistent(struct device *parent, size_t size, void *vaddr, dma_addr_t dma)
{
	struct pci_dev *pdev;

	/* Find pci_dev container of dev */
	pdev = to_pci_dev(parent);

	pci_free_consistent(pdev, size, vaddr, dma);
}

static int vivo_bridge_mmap(struct vme_bridge *vivo_bridge, struct vm_area_struct *vma)
{
	phys_addr_t phys_addr;
	unsigned long vma_size;
	struct vivo_driver *bridge;

	bridge = vivo_bridge->driver_priv;

	phys_addr = bridge->vme_reg_addr + VIVO_PCI_ID;
	vma_size = bridge->vme_reg_size;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	return vm_iomap_memory(vma, phys_addr, vma_size);
}

static int vivo_bridge_info(struct vme_bridge *vivo_bridge, int *device_id, u32 * map_size)
{
	struct vivo_driver *bridge;
	u32 data = 0;
	int retval = 0;

	bridge = vivo_bridge->driver_priv;

	data = (ioread32(bridge->base + VIVO_PCI_ID) & 0xFFFF0000) >> 16;
	if(data != PCI_DEVICE_ID_ABACO_VIVO) {
		dev_err(vivo_bridge->parent, "Unexpected Device ID 0x%x\n", data);
		retval = -EIO;
	}
	*device_id = data;

	*map_size = bridge->vme_reg_size;

	return retval;
}

/*
 * Configure CR/CSR space
 *
 * Access to the CR/CSR can be configured at power-up. The location of the
 * CR/CSR registers in the CR/CSR address space is determined by the boards
 * Auto-ID or Geographic address. This function ensures that the window is
 * enabled at an offset consistent with the boards geopgraphic address.
 *
 * Each board has a 512kB window, with the highest 2kB being used for the
 * board registers.
 */
static int vivo_crcsr_init(struct vme_bridge *vivo_bridge, struct pci_dev *pdev)
{
	struct vivo_driver *bridge;

	bridge = vivo_bridge->driver_priv;

	if(geoid) {
		iowrite32(((geoid << 3) & VIVO_CRBAR_CRBAR), bridge->base + VIVO_CRBAR);
		dev_info(vivo_bridge->parent, "Configured the CR/CSR Offset: %d\n", geoid);
	}

	return 0;
}

static void vivo_crcsr_exit(struct vme_bridge *vivo_bridge, struct pci_dev *pdev)
{
	u32 slot;
	struct vivo_driver *bridge;

	bridge = vivo_bridge->driver_priv;

	/* Restore the CR/CSR space to the geographical address if it was moved
	 * using the geoid module parameter.
	 */
	if(geoid) {
		slot = ioread32(bridge->base + VIVO_SYS_CTRL);
		slot = slot & VIVO_SYS_CTRL_GA;

		iowrite32(((slot << 3) & VIVO_CRBAR_CRBAR), bridge->base + VIVO_CRBAR);
		dev_info(vivo_bridge->parent, "Restored the CR/CSR Offset: %d\n", slot);
	}
}

/*
 * Initialize Vivo Device
 */
static void vivo_device_init(struct vme_bridge *vivo_bridge)
{
	u32 data;
	int i;
	struct vivo_driver *bridge;

	bridge = vivo_bridge->driver_priv;

#define         VIVO_VME_MSTR_USERAM0_19h               (9<<16)

	/* Initialize the master control register */
	data = VIVO_VME_MSTR_VMSTRTON_512us |
		VIVO_VME_MSTR_VMSTREL_DONE_AND_REQ |
		VIVO_VME_MSTR_VMSTFAIR_FAIR | VIVO_VME_MSTR_USERAM0_19h | VIVO_VME_MSTR_USERAM1_14h;

	iowrite32(data, bridge->base + VIVO_VME_MSTR);

	/* Disable Interrupt generation on device */
	iowrite32(0, bridge->base + VIVO_INT_EBL);

	/* Disable DMA interrupts */
	iowrite32(0, bridge->base + VIVO_DMAIE0);
	iowrite32(0, bridge->base + VIVO_DMAIE1);

	/* Clear all pending interrupts */
	iowrite32(~((u32) 0), bridge->base + VIVO_INT_STATUS);

	/* Clear any pending VME Bus Error */
	iowrite32((ioread32(bridge->base + VIVO_BIT_CLEAR) |
			VIVO_BIT_CLEAR_BERRSC), bridge->base + VIVO_BIT_CLEAR);

	/* Map interrupts D08 */
	iowrite32(0, bridge->base + VIVO_VME_INT_MAP);

	/* Disable all master windows */
	for(i = 0; i < VIVO_MAX_MASTER; i++)
		iowrite32(0, bridge->base + VIVO_MW_CTRL[i]);

	/* Disable all slave windows */
	for(i = 0; i < VIVO_MAX_SLAVE; i++)
		iowrite32(0, bridge->base + VIVO_SLVW_CTRL[i]);

	/* Set default BERR Timeout to 16us  */
	data = ioread32(bridge->base + VIVO_SYS_CTRL);
	data &= ~VIVO_SYS_CTRL_BERRTIMER;
	data |= VIVO_SYS_CTRL_BERRTIMER_64us;

	/* Priority mode is default */
	data &= ~VIVO_SYS_CTRL_BUS_ARB;
	iowrite32(data, bridge->base + VIVO_SYS_CTRL);

	/* Ensure the SWIRQ is enabled */
	iowrite32((ioread32(bridge->base + VIVO_VINT_EBL) |
			VIVO_VINT_EBL_SWIRQ), bridge->base + VIVO_VINT_EBL);

	/* Enable LITTLE endian */
	iowrite32((ioread32(bridge->base + VIVO_DEV_CTRL) |
			VIVO_DEV_CTRL_LENDIAN), bridge->base + VIVO_DEV_CTRL);

	/* Enable slave window access on the module */
	iowrite32((ioread32(bridge->base + VIVO_BIT_SET) |
			VIVO_BIT_SET_MODEBLS), bridge->base + VIVO_BIT_SET);
}

static int vivo_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int retval, i;
	u32 data;
	u64 bar_addr;
	u64 bar_size;
	struct list_head *pos = NULL, *n;
	struct vme_bridge *vivo_bridge;
	struct vivo_driver *vivo_device;
	struct vme_master_resource *master_image;
	struct vme_slave_resource *slave_image;
	struct vme_dma_resource *dma_ctrlr;

	/* If we want to support more than one of each bridge, we need to
	 * dynamically generate this so we get one per device
	 */
	vivo_bridge = kzalloc(sizeof(struct vme_bridge), GFP_KERNEL);
	if(vivo_bridge == NULL) {
		dev_err(&pdev->dev, "Failed to allocate memory for bridge structure\n");
		retval = -ENOMEM;
		goto err_struct;
	}

	vivo_device = kzalloc(sizeof(struct vivo_driver), GFP_KERNEL);
	if(vivo_device == NULL) {
		dev_err(&pdev->dev, "Failed to allocate memory for device structure\n");
		retval = -ENOMEM;
		goto err_driver;
	}

	vivo_bridge->driver_priv = vivo_device;

	/* Enable the device */
	retval = pci_enable_device(pdev);
	if(retval) {
		dev_err(&pdev->dev, "Unable to enable device\n");
		goto err_enable;
	}

	/* Enable bus mastering */
	pci_set_master(pdev);

	/* Map Registers */
	retval = pci_request_regions(pdev, driver_name);
	if(retval) {
		dev_err(&pdev->dev, "Unable to reserve resources\n");
		goto err_resource;
	}

	/***************************************************************
	 * Get the base address and size of the VME device's
	 * register space from BARs 2-3 (64 bits) of the device
	 ***************************************************************/
	bar_addr = ((u64) pci_resource_start(pdev, 3)) << 32;
	bar_addr |= ((u64) pci_resource_start(pdev, 2));

	bar_size = ((u64) pci_resource_len(pdev, 3)) << 32;
	bar_size |= ((u64) pci_resource_len(pdev, 2));

	/* map registers in BAR 2-3 */
	vivo_device->base = ioremap_nocache(bar_addr, bar_size);

	if(!vivo_device->base) {
		dev_err(&pdev->dev, "Unable to map device register space\n");
		retval = -EIO;
		goto err_remap;
	}

	/* FIXME: This is the info I need to mmap the vme bridge */
	vivo_device->vme_reg_addr = bar_addr;
	vivo_device->vme_reg_size = bar_size;

	/* Check to see if the mapping worked out */
	data = ioread32(vivo_device->base + VIVO_PCI_ID) & 0x0000FFFF;
	if(data != PCI_VENDOR_ID_ABACO) {
		dev_err(&pdev->dev, "Device register region check failed\n");
		retval = -EIO;
		goto err_test;
	}

	/* FIXME: Get the DEVICE ID here, as well */

	/***************************************************************
	 * Get the base address and size of the VME device's
	 * PCI space from BARs 0-1 (64 bits) of the device
	 ***************************************************************/
	bar_addr = ((u64) pci_resource_start(pdev, 1)) << 32;
	bar_addr |= ((u64) pci_resource_start(pdev, 0));

	bar_size = ((u64) pci_resource_len(pdev, 1)) << 32;
	bar_size |= ((u64) pci_resource_len(pdev, 0));

	vivo_device->pci_resource.start = bar_addr;
	vivo_device->pci_resource.end = bar_addr + bar_size;
	vivo_device->pci_resource.flags = pci_resource_flags(pdev, 0);

	/* Initialize wait queues & mutual exclusion flags */
	init_waitqueue_head(&vivo_device->dma_queue[0]);
	init_waitqueue_head(&vivo_device->dma_queue[1]);
	init_waitqueue_head(&vivo_device->iack_queue);
	mutex_init(&vivo_device->vme_int);

	vivo_bridge->parent = &pdev->dev;
	strcpy(vivo_bridge->name, driver_name);

	/* Initialize device */
	vivo_device_init(vivo_bridge);

	/* Set up IRQ */
	retval = vivo_irq_init(vivo_bridge);
	if(retval != 0) {
		dev_err(&pdev->dev, "Chip Initialization failed.\n");
		goto err_irq;
	}

	/* Add master windows to list */
	INIT_LIST_HEAD(&vivo_bridge->master_resources);
	for(i = 0; i < VIVO_MAX_MASTER; i++) {
		master_image = kmalloc(sizeof(struct vme_master_resource), GFP_KERNEL);
		if(master_image == NULL) {
			dev_err(&pdev->dev,
				"Failed to allocate memory for master resource structure\n");
			retval = -ENOMEM;
			goto err_master;
		}
		master_image->parent = vivo_bridge;
		spin_lock_init(&master_image->lock);
		master_image->locked = 0;
		master_image->number = i;
		master_image->address_attr = VME_A16 | VME_A24 | VME_A32 |
			VME_CRCSR | VME_USER1 | VME_USER2;
		master_image->cycle_attr = VME_SCT | VME_SUPER | VME_USER | VME_PROG | VME_DATA;
		master_image->width_attr = VME_D8 | VME_D16 | VME_D32 | VME_D64;
		memset(&master_image->bus_resource, 0, sizeof(struct resource));
		master_image->kern_base = NULL;
		list_add_tail(&master_image->list, &vivo_bridge->master_resources);
	}

	/* Add slave windows to list */
	INIT_LIST_HEAD(&vivo_bridge->slave_resources);
	for(i = 0; i < VIVO_MAX_SLAVE; i++) {
		slave_image = kmalloc(sizeof(struct vme_slave_resource), GFP_KERNEL);
		if(slave_image == NULL) {
			dev_err(&pdev->dev,
				"Failed to allocate memory for slave resource structure\n");
			retval = -ENOMEM;
			goto err_slave;
		}
		slave_image->parent = vivo_bridge;
		mutex_init(&slave_image->mtx);
		slave_image->locked = 0;
		slave_image->number = i;
		slave_image->address_attr = VME_A16 | VME_A24 | VME_A32;
		slave_image->cycle_attr = VME_SCT | VME_BLT | VME_MBLT |
			VME_2eSST | VME_2eSSTB | VME_2eSST160 |
			VME_2eSST267 | VME_2eSST320 | VME_SUPER | VME_USER | VME_PROG | VME_DATA;
		list_add_tail(&slave_image->list, &vivo_bridge->slave_resources);
	}

	/* Add dma engines to list */
	INIT_LIST_HEAD(&vivo_bridge->dma_resources);
	for(i = 0; i < VIVO_MAX_DMA; i++) {
		dma_ctrlr = kzalloc(sizeof(struct vme_dma_resource), GFP_KERNEL);
		if(dma_ctrlr == NULL) {
			dev_err(&pdev->dev,
				"Failed to allocate memory for dma resource structure\n");
			retval = -ENOMEM;
			goto err_dma;
		}
		dma_ctrlr->parent = vivo_bridge;
		mutex_init(&dma_ctrlr->mtx);
		dma_ctrlr->locked = 0;
		dma_ctrlr->number = i;
		dma_ctrlr->route_attr = VME_DMA_VME_TO_MEM |
			VME_DMA_MEM_TO_VME | VME_DMA_VME_TO_VME | VME_DMA_MEM_TO_MEM;
		INIT_LIST_HEAD(&dma_ctrlr->pending);
		INIT_LIST_HEAD(&dma_ctrlr->running);
		list_add_tail(&dma_ctrlr->list, &vivo_bridge->dma_resources);
	}

	vivo_bridge->slave_get = vivo_slave_get;
	vivo_bridge->slave_set = vivo_slave_set;
	vivo_bridge->master_get = vivo_master_get;
	vivo_bridge->master_set = vivo_master_set;
	vivo_bridge->master_read = vivo_master_read;
	vivo_bridge->master_write = vivo_master_write;
	vivo_bridge->master_rmw = NULL;
	vivo_bridge->dma_list_add = vivo_dma_list_add;
	vivo_bridge->dma_list_exec = vivo_dma_list_exec;
	vivo_bridge->dma_list_empty = vivo_dma_list_empty;
	vivo_bridge->irq_set = vivo_irq_set;
	vivo_bridge->irq_generate = vivo_irq_generate;
	vivo_bridge->lm_set = NULL;
	vivo_bridge->lm_get = NULL;
	vivo_bridge->lm_attach = NULL;
	vivo_bridge->lm_detach = NULL;
	vivo_bridge->slot_get = vivo_slot_get;
	vivo_bridge->alloc_consistent = vivo_alloc_consistent;
	vivo_bridge->free_consistent = vivo_free_consistent;
	vivo_bridge->bridge_mmap = vivo_bridge_mmap;
	vivo_bridge->bridge_info = vivo_bridge_info;

	data = ioread32(vivo_device->base + VIVO_SYS_CTRL);
	dev_info(&pdev->dev, "Board is%s the VME system controller\n",
		(data & VIVO_SYS_CTRL_SYSCTRL) ? "" : " not");
	if(!geoid)
		dev_info(&pdev->dev, "VME geographical address is %d\n", data & VIVO_SYS_CTRL_GA);
	else
		dev_info(&pdev->dev, "VME geographical address is set to %d\n", geoid);

	dev_info(&pdev->dev, "VME error check is %s\n", err_chk ? "enabled" : "disabled");

	retval = vivo_crcsr_init(vivo_bridge, pdev);
	if(retval) {
		dev_err(&pdev->dev, "CR/CSR configuration failed.\n");
		goto err_crcsr;
	}

	retval = vme_register_bridge(vivo_bridge);
	if(retval != 0) {
		dev_err(&pdev->dev, "Chip Registration failed.\n");
		goto err_reg;
	}

	pci_set_drvdata(pdev, vivo_bridge);

	return 0;

err_reg:
	vivo_crcsr_exit(vivo_bridge, pdev);
err_crcsr:
err_dma:
	/* resources are stored in link list */
	list_for_each_safe(pos, n, &vivo_bridge->dma_resources) {
		dma_ctrlr = list_entry(pos, struct vme_dma_resource, list);
		list_del(pos);
		kfree(dma_ctrlr);
	}
err_slave:
	/* resources are stored in link list */
	list_for_each_safe(pos, n, &vivo_bridge->slave_resources) {
		slave_image = list_entry(pos, struct vme_slave_resource, list);
		list_del(pos);
		kfree(slave_image);
	}
err_master:
	/* resources are stored in link list */
	list_for_each_safe(pos, n, &vivo_bridge->master_resources) {
		master_image = list_entry(pos, struct vme_master_resource, list);
		list_del(pos);
		kfree(master_image);
	}

	vivo_irq_exit(vivo_bridge, pdev);
err_irq:
err_test:
	iounmap(vivo_device->base);
err_remap:
	pci_release_regions(pdev);
err_resource:
	pci_disable_device(pdev);
err_enable:
	kfree(vivo_device);
err_driver:
	kfree(vivo_bridge);
err_struct:
	return retval;
}

static void vivo_remove(struct pci_dev *pdev)
{
	struct list_head *pos = NULL;
	struct list_head *tmplist;
	struct vme_master_resource *master_image;
	struct vme_slave_resource *slave_image;
	struct vme_dma_resource *dma_ctrlr;
	struct vivo_driver *bridge;
	struct vme_bridge *vivo_bridge = pci_get_drvdata(pdev);

	bridge = vivo_bridge->driver_priv;

	dev_dbg(&pdev->dev, "Driver is being unloaded.\n");

	/* Terminate device */
	vivo_device_init(vivo_bridge);

	vivo_irq_exit(vivo_bridge, pdev);

	vme_unregister_bridge(vivo_bridge);

	vivo_crcsr_exit(vivo_bridge, pdev);

	/* resources are stored in link list */
	list_for_each_safe(pos, tmplist, &vivo_bridge->dma_resources) {
		dma_ctrlr = list_entry(pos, struct vme_dma_resource, list);
		list_del(pos);
		kfree(dma_ctrlr);
	}

	/* resources are stored in link list */
	list_for_each_safe(pos, tmplist, &vivo_bridge->slave_resources) {
		slave_image = list_entry(pos, struct vme_slave_resource, list);
		list_del(pos);
		kfree(slave_image);
	}

	/* resources are stored in link list */
	list_for_each_safe(pos, tmplist, &vivo_bridge->master_resources) {
		master_image = list_entry(pos, struct vme_master_resource, list);
		list_del(pos);
		kfree(master_image);
	}

	iounmap(bridge->base);

	pci_release_regions(pdev);

	pci_disable_device(pdev);

	kfree(vivo_bridge->driver_priv);

	kfree(vivo_bridge);
}

module_pci_driver(vivo_driver);

MODULE_PARM_DESC(err_chk, "Check for VME errors on reads and writes");
module_param(err_chk, bool, 0);

MODULE_PARM_DESC(geoid, "Override geographical addressing");
module_param(geoid, int, 0);

MODULE_PARM_DESC(berr_irq_enable, "Enable IRQ for BERR");
module_param(berr_irq_enable, int, 0);

MODULE_PARM_DESC(dma_irq_enable, "Enable IRQ for DMA Completion");
module_param(dma_irq_enable, int, 0);

MODULE_DESCRIPTION("VME driver for the Vivo VME bridge");
MODULE_LICENSE("GPL");
