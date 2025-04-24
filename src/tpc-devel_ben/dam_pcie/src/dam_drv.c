#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)
#include <linux/pci-aspm.h>
#endif
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/wait.h>

#include "dam_drv.h"
#include "dam_hw.h"

extern struct dam_private *dam_endpoints[MAX_PCIE_ENDPOINTS];
extern int total_endpoints;

volatile wupper_regs_t    *m_bar0;

static int dam_buffer_init(struct dam_private *dev_priv)
{
    int dma_id = 0;
    m_bar0 = (wupper_regs_t *)dev_priv->wupper_io;

    m_bar0->DMA_RESET = 1;
    m_bar0->SOFT_RESET = 1;
    m_bar0->DMA_DESC_ENABLE = 0;
    m_bar0->TOHOST_FULL_THRESH.THRESHOLD_NEGATE = 4000/2;
    m_bar0->TOHOST_FULL_THRESH.THRESHOLD_ASSERT = 4096/2;

    dev_priv->dma_size = 256 * PAGE_SIZE * /*1024*/4;
    // anything above 4MB seems does not work ?
 // 3 MiB => ((256 * 4096 bytes) = 1 MiB) * 3 

    dev_priv->kbuf_dma = dma_alloc_coherent(&dev_priv->pdev->dev, dev_priv->dma_size, &dev_priv->dma_addr, GFP_KERNEL);

    if ( ! dev_priv->kbuf_dma) {
        dev_err(&dev_priv->pdev->dev, "Failed to request DMA memory\n");
        return -ENOMEM;
    }

    dev_info(&dev_priv->pdev->dev, "got DMA address block: [%pad : 0x%llx], size: %lu bytes\n",
            &dev_priv->dma_addr,
            (dev_priv->dma_addr + dev_priv->dma_size),
            dev_priv->dma_size);

    dev_priv->kbuf_dma[2] = 0xbeef;

    // Load up the DMA descriptors
    m_bar0->DMA_DESC[dma_id].start_address = dev_priv->dma_addr;
    m_bar0->DMA_DESC[dma_id].end_address   = (dev_priv->dma_addr + dev_priv->dma_size);
    m_bar0->DMA_DESC[dma_id].tlp           = 64; // FIXME: Read TLP properly
    m_bar0->DMA_DESC[dma_id].read          = 0;
    m_bar0->DMA_DESC[dma_id].wrap_around   = 0;
    m_bar0->DMA_DESC[dma_id].read_ptr      = dev_priv->dma_addr;

    if (m_bar0->DMA_DESC_STATUS[dma_id].even_addr_pc == m_bar0->DMA_DESC_STATUS[dma_id].even_addr_dma) {
        // Make 'even_addr_pc' unequal to 'even_addr_dma', or a (circular) DMA won't start!?
        --m_bar0->DMA_DESC[dma_id].read_ptr;
        ++m_bar0->DMA_DESC[dma_id].read_ptr;
    }

    m_bar0->DMA_DESC_ENABLE |= 1 << dma_id;

    //pr_info("Bar 0: 0x%x", ioread32(dev_priv->wupper_io));

    return 0;
}

static int dam_buffer_exit(struct dam_private *dev_priv)
{
    m_bar0->DMA_DESC_ENABLE = 0x0;
    if (dev_priv->kbuf_dma) {
        dma_free_coherent(&dev_priv->pdev->dev, dev_priv->dma_size, dev_priv->kbuf_dma, dev_priv->dma_addr);
    }

    return 0;
}

static int dam_pci_reserve_mem(struct dam_private *dev_priv)
{
    unsigned long start;
    unsigned long len;
    int ret;

    /* Reserve resources */
    ret = pci_request_region(dev_priv->pdev, DAM_PCIE_WUPPER_BAR, "Wupper DMA");
    if (ret) {
        dev_err(&dev_priv->pdev->dev, "Failed to request S2 IO\n");
        return ret;
    }

    ret = pci_request_region(dev_priv->pdev, DAM_PCIE_MSIX_INT_BAR, "MSIX Interrupts");
    if (ret) {
        dev_err(&dev_priv->pdev->dev, "Failed to request ISP IO\n");
        pci_release_region(dev_priv->pdev, DAM_PCIE_WUPPER_BAR);
        return ret;
    }

    ret = pci_request_region(dev_priv->pdev, DAM_PCIE_CONTROL_BAR, "Firmware Control");
    if (ret) {
        pci_release_region(dev_priv->pdev, DAM_PCIE_MSIX_INT_BAR);
        pci_release_region(dev_priv->pdev, DAM_PCIE_WUPPER_BAR);
        return ret;
    }

    /* Wupper DMA Control */
    start = pci_resource_start(dev_priv->pdev, DAM_PCIE_WUPPER_BAR);
    len = pci_resource_len(dev_priv->pdev, DAM_PCIE_WUPPER_BAR);
    dev_priv->wupper_io = ioremap_nocache(start, len);
    dev_priv->wupper_io_len = len;

    /* MSIX Intterupt Control */
    start = pci_resource_start(dev_priv->pdev, DAM_PCIE_MSIX_INT_BAR);
    len = pci_resource_len(dev_priv->pdev, DAM_PCIE_MSIX_INT_BAR);
    dev_priv->msix_mem = ioremap_nocache(start, len);
    dev_priv->msix_mem_len = len;

    /* Frimware Control */
    start = pci_resource_start(dev_priv->pdev, DAM_PCIE_CONTROL_BAR);
    len = pci_resource_len(dev_priv->pdev, DAM_PCIE_CONTROL_BAR);
    dev_priv->pcie_io = ioremap_nocache(start, len);
    dev_priv->pcie_io_len = len;
    dev_priv->pcie_io_baseaddr = start;

    pr_debug("Allocated Wupper DMA regs (BAR %d). %u bytes at 0x%p\n",
            DAM_PCIE_WUPPER_BAR, dev_priv->wupper_io_len, dev_priv->wupper_io);

    pr_debug("Allocated MSIX control regs (BAR %d). %u bytes at 0x%p\n",
            DAM_PCIE_MSIX_INT_BAR, dev_priv->msix_mem_len, dev_priv->msix_mem);

    pr_debug("Allocated PCIe control regs (BAR %d). %u bytes at 0x%p\n",
            DAM_PCIE_CONTROL_BAR, dev_priv->pcie_io_len, dev_priv->pcie_io);

    return 0;
}

static void dam_handle_irq(struct dam_private *dev_priv, struct fw_channel *chan)
{
    u32 entry;
    int ret;
    /*
       if (chan == dev_priv->channel_io) {
       pr_debug("IO channel ready\n");
       wake_up_interruptible(&chan->wq);
       return;
       }

       if (chan == dev_priv->channel_buf_h2t) {
       pr_debug("H2T channel ready\n");
       wake_up_interruptible(&chan->wq);
       return;
       }

       if (chan == dev_priv->channel_debug) {
       pr_debug("DEBUG channel ready\n");
       wake_up_interruptible(&chan->wq);
       return;
       }
       */
    return;
}

static void dam_irq_work(struct work_struct *work)
{
    struct dam_private *dev_priv = container_of(work, struct dam_private, irq_work);
    struct fw_channel *chan;

    u32 pending;
    int i = 0;

    return;
}

static irqreturn_t dam_irq_handler(int irq, void *arg)
{
    struct dam_private *dev_priv = arg;
    u32 pending;
    unsigned long flags;

    spin_lock_irqsave(&dev_priv->io_lock, flags);
    //pending = DAM_ISP_REG_READ(ISP_IRQ_STATUS);
    spin_unlock_irqrestore(&dev_priv->io_lock, flags);

    //if (!(pending & 0xf0))
    //	return IRQ_NONE;

    schedule_work(&dev_priv->irq_work);

    return IRQ_HANDLED;
}

static int dam_irq_enable(struct dam_private *dev_priv, int int_n)
{
    int vec_n, int_pba;

    if (int_n > dev_priv->msix_int_available) {
        dev_err(&dev_priv->pdev->dev,
                "IRQ out of range %i\n", int_n);
        return -1;
    }

    vec_n = pci_irq_vector(dev_priv->pdev, int_n);
    enable_irq(vec_n);

    int_pba = ioread32(dev_priv->msix_mem + dev_priv->msix_int_available);
    int_pba |= (1 << int_n);
    iowrite32(int_pba, dev_priv->msix_mem + dev_priv->msix_int_available);

    return 0;
}

static int dam_irq_disable(struct dam_private *dev_priv, int int_n)
{
    int vec_n, int_pba;

    if (int_n > dev_priv->msix_int_available) {
        dev_err(&dev_priv->pdev->dev,
                "IRQ out of range %i\n", int_n);
        return -1;
    }

    int_pba = ioread32(dev_priv->msix_mem + dev_priv->msix_int_available);
    int_pba |= (0 << int_n);
    iowrite32(int_pba, dev_priv->msix_mem + dev_priv->msix_int_available);

    vec_n = pci_irq_vector(dev_priv->pdev, int_n);
    disable_irq(vec_n);

    return 0;
}

static int dam_msix_init(struct dam_private *dev_priv)
{
    int offset, data, bar, vec_offset, pba_offset, interrupt;
    int nvec;

    offset = pci_find_capability(dev_priv->pdev, PCI_CAP_ID_MSIX);
    if (offset == 0) {
        dev_err(&dev_priv->pdev->dev, "%s: Failed to map MSI-X BAR for device id 0x%x\n",
                __FUNCTION__, dev_priv->pdev->device);
        return ENODEV;
    }

    // Extract bar and vector table location
    pci_read_config_dword(dev_priv->pdev, offset+4, &data);
    bar = data & 0xf;
    vec_offset = data & 0xfffffff0;
    pr_debug("%s: MSI-X Bar @ %i, vector offset 0x%x", __FUNCTION__, bar, vec_offset);

    // Get PBA offset
    pci_read_config_dword(dev_priv->pdev, offset+8, &data);
    bar = data & 0xf;
    pba_offset = data & 0xfffffff0;
    dev_priv->msix_enable_offset = pba_offset/sizeof(uint32_t);
    pr_debug("%s: MSI-X Bar @ %i, pba offset 0x%x", __FUNCTION__, bar, pba_offset);

    // Clear the MSI-X enable table
    iowrite32(0x0, dev_priv->msix_mem + dev_priv->msix_enable_offset);

    nvec = pci_alloc_irq_vectors(dev_priv->pdev, 1, MAX_MSIX, PCI_IRQ_MSIX);
    dev_priv->msix_int_available = nvec;
    if (nvec < 0) {
        dev_err(&dev_priv->pdev->dev, "%s: Failed to enable MSI-X BAR for device id 0x%x\n",
                __FUNCTION__, dev_priv->pdev->device);
        return ENODEV;
    }
    dev_info(&dev_priv->pdev->dev, "%s: Enabled %i MSI-X vectors\n",
            __FUNCTION__, nvec);

    return 0;
}

static int dam_msix_irq_install(struct dam_private *dev_priv)
{
    int ret=0, interrupt, vec_n;

    for (interrupt = 0; interrupt < dev_priv->msix_int_available; interrupt++) {
        vec_n = pci_irq_vector(dev_priv->pdev, interrupt);
        pr_debug("%s: pci_irq_vector got 0x%x for %i", __FUNCTION__, vec_n, interrupt);
        ret = request_irq(vec_n, dam_irq_handler, 0,
                KBUILD_MODNAME, (void *)dev_priv);
        if (ret) {
            dev_err(&dev_priv->pdev->dev, "Failed to request IRQ\n");
            break;
        }
        disable_irq(vec_n);
    }

    return ret;
}

static void dam_msix_irq_uninstall(struct dam_private *dev_priv)
{
    int i, vec_n;
    for (i = 0; i < dev_priv->msix_int_available; i++) {
        vec_n = pci_irq_vector(dev_priv->pdev, i);
        dam_irq_disable(dev_priv, i);
        free_irq(vec_n, dev_priv);
    }
    pci_free_irq_vectors(dev_priv->pdev);
}

static int dam_pci_set_dma_mask(struct dam_private *dev_priv,
        unsigned int mask)
{
    int ret;

    ret = dma_set_mask_and_coherent(&dev_priv->pdev->dev, DMA_BIT_MASK(mask));
    if (ret) {
        dev_err(&dev_priv->pdev->dev, "Failed to set %u pci dma mask\n",
                mask);
        return ret;
    }

    dev_priv->dma_mask = mask;

    return 0;
}

static void dam_stop_firmware(struct dam_private *dev_priv)
{
    return;
}

void dam_pci_remove(struct pci_dev *pdev)
{
    struct dam_private *dev_priv;

    dev_priv = pci_get_drvdata(pdev);
    if (!dev_priv)
        goto out;

    //dam_debugfs_exit(dev_priv);

    //dam_stop_firmware(dev_priv);

    dam_msix_irq_uninstall(dev_priv);

    cancel_work_sync(&dev_priv->irq_work);

    //dam_hw_deinit(dev_priv);

    dam_buffer_exit(dev_priv);

    if (dev_priv->wupper_io)
        iounmap(dev_priv->wupper_io);
    if (dev_priv->msix_mem)
        iounmap(dev_priv->msix_mem);
    if (dev_priv->pcie_io)
        iounmap(dev_priv->pcie_io);

    pci_release_region(pdev, DAM_PCIE_WUPPER_BAR);
    pci_release_region(pdev, DAM_PCIE_MSIX_INT_BAR);
    pci_release_region(pdev, DAM_PCIE_CONTROL_BAR);
out:
    pci_disable_device(pdev);
}

static int dam_pci_init(struct dam_private *dev_priv)
{
    struct pci_dev *pdev = dev_priv->pdev;
    int ret;

    ret = pci_enable_device(pdev);
    if (ret) {
        dev_err(&pdev->dev, "Failed to enable device\n");
        return ret;
    }

    ret = dam_pci_reserve_mem(dev_priv);
    if (ret)
        goto fail_enable;

    ret = dam_msix_init(dev_priv);
    if (ret) {
        dev_err(&pdev->dev, "Failed to enable MSI-X\n");
        goto fail_reserve;
    }

    ret = dam_msix_irq_install(dev_priv);
    if (ret)
        goto fail_reserve;

    ret = dam_pci_set_dma_mask(dev_priv, 64);
    if (ret)
        ret = dam_pci_set_dma_mask(dev_priv, 32);

    dev_info(&pdev->dev, "Setting %u-bit DMA mask\n", dev_priv->dma_mask);
    pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(dev_priv->dma_mask));

    pci_set_master(pdev);

    pci_set_drvdata(pdev, dev_priv);
    dam_endpoints[0] = dev_priv;
    total_endpoints++;

    return 0;

fail_irq:
    pci_free_irq_vectors(pdev);
fail_reserve:
    pci_release_region(pdev, DAM_PCIE_WUPPER_BAR);
    pci_release_region(pdev, DAM_PCIE_MSIX_INT_BAR);
    pci_release_region(pdev, DAM_PCIE_CONTROL_BAR);
fail_enable:
    pci_disable_device(pdev);
    return ret;
}

static int dam_firmware_start(struct dam_private *dev_priv)
{
    int ret = 0;
    return ret;
}

int dam_pci_probe(struct pci_dev *pdev,
        const struct pci_device_id *entry)
{
    struct dam_private *dev_priv;
    int ret;

    dev_info(&pdev->dev, "%s: found DAM/FELIX PCIe card with device id: 0x%x\n",
            __FUNCTION__, pdev->device);

    dev_priv = kzalloc(sizeof(struct dam_private), GFP_KERNEL);
    if (!dev_priv) {
        dev_err(&pdev->dev, "Failed to allocate memory\n");
        return -ENOMEM;
    }

    spin_lock_init(&dev_priv->io_lock);

    mutex_init(&dev_priv->ioctl_lock);
    INIT_LIST_HEAD(&dev_priv->buffer_queue);
    INIT_WORK(&dev_priv->irq_work, dam_irq_work);

    dev_priv->pdev = pdev;

    ret = dam_pci_init(dev_priv);
    if (ret)
        goto fail_work;

    dev_priv->flx_model  = DAM_PCIE_REG_READ(0x830);
    dev_priv->block_size = DAM_PCIE_REG_READ(0x890);
    dev_priv->reg_map    = DAM_PCIE_REG_READ(0x000);
    dev_info(&pdev->dev, "Frimware reports: FLX Model: %i, Reg Map Verison: %i\n", dev_priv->flx_model, dev_priv->reg_map);

    ret = dam_buffer_init(dev_priv);
    if (ret)
    	goto fail_pci;

    //ret = dam_hw_init(dev_priv);
    //if (ret)
    //	goto fail_buffer;

    ret = dam_firmware_start(dev_priv);
    if (ret)
        goto fail_hw;

    //ret = dam_debugfs_init(dev_priv);
    //if (ret)
    //	goto fail_firmware;
    return 0;

fail_firmware:
    dam_stop_firmware(dev_priv);
fail_hw:
    //dam_hw_deinit(dev_priv);
fail_buffer:
    dam_buffer_exit(dev_priv);
fail_pci:
    dam_msix_irq_uninstall(dev_priv);
    pci_release_region(pdev, DAM_PCIE_WUPPER_BAR);
    pci_release_region(pdev, DAM_PCIE_MSIX_INT_BAR);
    pci_release_region(pdev, DAM_PCIE_CONTROL_BAR);
    pci_disable_device(pdev);

fail_work:
    cancel_work_sync(&dev_priv->irq_work);
    kfree(dev_priv);

    return ret;
}

#ifdef CONFIG_PM
int dam_pci_suspend(struct pci_dev *pdev, pm_message_t state)
{
    dam_pci_remove(pdev);

    return 0;
}

int dam_pci_resume(struct pci_dev *pdev)
{
    dam_pci_probe(pdev, NULL);

    return 0;
}
#endif /* CONFIG_PM */

//module_pci_driver(dam_pci_driver);
