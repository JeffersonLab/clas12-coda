#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#include "dam_drv.h"
#include "dam_hw.h"
#include "dam_ioctl.h"
#include "version.h"

MODULE_AUTHOR("J. Kuczewski <jkuczewski@bnl.gov>");
MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_MODULE_VERSION);

char *dev_fname = DRV_MODULE_NAME;
dev_t first_dev;
struct class *dam_class;
static struct cdev dam_cdev[MAX_PCIE_ENDPOINTS];
struct dam_private *dam_endpoints[MAX_PCIE_ENDPOINTS];
int total_endpoints;

typedef struct {
    struct dam_private *dev_priv;
    u_int slot;
} file_params_t;

static int dam_fopen(struct inode *inode, struct file *file)
{
    file_params_t *fdata;
    int minor = MINOR(inode->i_rdev);
    pr_info("%s, minor=%i\n", __func__, minor);

    fdata = (file_params_t *)kmalloc(sizeof(file_params_t), GFP_KERNEL);
    if (fdata == NULL) {
        return ENOMEM;
    }

    fdata->dev_priv = dam_endpoints[minor];
    fdata->slot = minor; 
    file->private_data = (char *)fdata;

    return 0;
}

static int dam_frelease(struct inode *inode, struct file *file)
{
    pr_info("%s\n", __func__);
    kfree(file->private_data);
    return 0;
}

ssize_t dam_fread(struct file *file_p, char __user *user_buff,
        size_t len_bytes, loff_t *offset_bytes)
{
    ssize_t ret = 0;
    file_params_t *fdata = NULL;
    struct dam_private *dev_priv = NULL;
    wupper_regs_t *m_bar0 = NULL;
    uint64_t start_ptr = 0;
    uint64_t read_ptr = 0;
    uint64_t write_ptr = 0;
    size_t n_bytes = 0;
    size_t total_bytes = 0;

    fdata = (file_params_t *)file_p->private_data;
    dev_priv = fdata->dev_priv;
    m_bar0 = (wupper_regs_t *)dev_priv->wupper_io;

    start_ptr = m_bar0->DMA_DESC[0].start_address;
    read_ptr = m_bar0->DMA_DESC[0].read_ptr;
    write_ptr = m_bar0->DMA_DESC_STATUS[0].current_address;

    if (write_ptr > read_ptr) {
        n_bytes = write_ptr - read_ptr;
        if (n_bytes > len_bytes)
            n_bytes = len_bytes;

        //ret = copy_to_user(&user_buff[total_bytes >> 2], &dev_priv->kbuf_dma[(write_ptr - read_ptr) >> 2], n_bytes);
        ret = copy_to_user(&user_buff[total_bytes >> 2], &dev_priv->kbuf_dma[(read_ptr - start_ptr) >> 2], n_bytes);
        if (ret) {
            pr_err("copy_to_user fault: ret=%li\n", ret);
            return -EFAULT;
        }

        total_bytes += n_bytes;
        m_bar0->DMA_DESC[0].read_ptr = read_ptr + n_bytes;
    }
    else {
        if (file_p->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }
    }

    if (file_p->f_flags & O_NONBLOCK) {
        return total_bytes;
    }


    return total_bytes;
}

ssize_t dam_fwrite(struct file *file, const char __user *user_buffer,
        size_t count, loff_t *offset)
{
    pr_info("%s\n", __func__);
    return count;
}

loff_t dam_llseek(struct file *file, loff_t off, int whence)
{
    struct dam_private *dev_priv = NULL;
    file_params_t *fdata = NULL;
    wupper_regs_t *m_bar0 = NULL;
    uint64_t start_ptr = 0, end_ptr = 0;
    uint64_t read_ptr = 0, write_ptr =0;

    fdata = (file_params_t *)file->private_data;
    dev_priv = fdata->dev_priv;
    m_bar0 = (wupper_regs_t *)dev_priv->wupper_io;

    start_ptr = m_bar0->DMA_DESC[0].start_address;
    read_ptr  = m_bar0->DMA_DESC[0].read_ptr;
    write_ptr = m_bar0->DMA_DESC_STATUS[0].current_address;
    end_ptr   = m_bar0->DMA_DESC[0].end_address;

    switch (whence) {
        case 0: // SEEK_SET
            read_ptr = dev_priv->dma_addr + off;
            if (read_ptr < start_ptr || read_ptr > end_ptr) {
                pr_err("%s: unsupported SEEK_SET offset %llx\n", __func__, off);
                return -EINVAL;
            }
            m_bar0->DMA_DESC[0].read_ptr = read_ptr;
            break;
        case 1: // SEEK_CUR
            read_ptr += off;
            if (read_ptr < start_ptr || read_ptr > end_ptr) {
                pr_err("%s: unsupported SEEK_CUR offset %llx\n", __func__, off);
                return -EINVAL;
            }
            m_bar0->DMA_DESC[0].read_ptr = read_ptr;
            break;
        case 2: // SEEK_END
            m_bar0->DMA_DESC[0].read_ptr = end_ptr;
            break;
    }

    read_ptr    = m_bar0->DMA_DESC[0].read_ptr;
    file->f_pos = read_ptr;
    return file->f_pos;
}

static long dam_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    dam_drv_io_t p;
    wupper_regs_t *m_bar0;
    int ret = 0;
    file_params_t *fdata = NULL;
    struct dam_private *dev_priv = NULL;

    fdata = (file_params_t *)file->private_data;
    dev_priv = fdata->dev_priv;

    switch (cmd)
    {
        case DEBUG_DMA:
            m_bar0 = (wupper_regs_t *)dev_priv->wupper_io;
            pr_info("Start Ptr:   0x%016lx\n", m_bar0->DMA_DESC[0].start_address);
            pr_info("End Ptr:     0x%016lx\n", m_bar0->DMA_DESC[0].end_address);
            pr_info("Enable:      0x%0x\n", m_bar0->DMA_DESC_ENABLE);
            pr_info("Read Ptr:    0x%016lx\n", m_bar0->DMA_DESC[0].read_ptr);
            pr_info("Write Ptr:   0x%016lx\n", m_bar0->DMA_DESC_STATUS[0].current_address);
            pr_info("Descriptor done DMA0: 0x%x\n", m_bar0->DMA_DESC_STATUS[0].descriptor_done);
            pr_info("Even Addr. DMA  DMA0: 0x%x\n", m_bar0->DMA_DESC_STATUS[0].even_addr_dma);
            pr_info("Even Addr. PC   DMA0: 0x%x\n", m_bar0->DMA_DESC_STATUS[0].even_addr_pc);
            pr_info("Descriptor done DMA1: 0x%x\n", m_bar0->DMA_DESC_STATUS[1].descriptor_done);
            pr_info("Even Addr. DMA  DMA1: 0x%x\n", m_bar0->DMA_DESC_STATUS[1].even_addr_dma);
            pr_info("Even Addr. PC   DMA1: 0x%x\n", m_bar0->DMA_DESC_STATUS[1].even_addr_pc);

            pr_info("Start Addr: %016lx\nEnd Addr:  %016lx\nRead Ptr: %016lx\n", m_bar0->DMA_DESC[0].start_address, m_bar0->DMA_DESC[0].end_address, m_bar0->DMA_DESC[0].read_ptr);
            break;

        case RESET_DMA:
            m_bar0 = (wupper_regs_t *)dev_priv->wupper_io;
            m_bar0->DMA_DESC_ENABLE = 0;

            m_bar0->DMA_DESC[0].read_ptr      = dev_priv->dma_addr;

            if (m_bar0->DMA_DESC_STATUS[0].even_addr_pc == m_bar0->DMA_DESC_STATUS[0].even_addr_dma) {
                // Make 'even_addr_pc' unequal to 'even_addr_dma', or a (circular) DMA won't start!?
                --m_bar0->DMA_DESC[0].read_ptr;
                ++m_bar0->DMA_DESC[0].read_ptr;
            }

            m_bar0->DMA_DESC_ENABLE |= 1 << 0;
            break;

        case READ_REG:
            if (copy_from_user(&p, (dam_drv_io_t *)arg, sizeof(dam_drv_io_t))) {
                ret = -EACCES;
                break;
            }

            p.data = DAM_PCIE_REG_READ(p.address);

            if (copy_to_user((dam_drv_io_t *)arg, &p, sizeof(dam_drv_io_t))) {
                ret = -EACCES;
                break;
            }
            break;

        case WRITE_REG:
            if (copy_from_user(&p, (dam_drv_io_t *)arg, sizeof(dam_drv_io_t))) {
                ret = -EACCES;
                break;
            }
            DAM_PCIE_REG_WRITE(p.data, p.address);
            break;
    }

    return ret;
}

void dev_vmopen(struct vm_area_struct *vma) { }


void dev_vmclose(struct vm_area_struct *vma) { }

// Virtual memory operations
static struct vm_operations_struct dev_vm_ops = {
    open:  dev_vmopen,
    close: dev_vmclose,
};

int dam_mmap(struct file *file, struct vm_area_struct *vma) 
{
    file_params_t *fdata = NULL;
    struct dam_private *dev_priv = NULL;
    unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long vsize = vma->vm_end - vma->vm_start;
    int result;

    fdata = (file_params_t *)file->private_data;
    dev_priv = fdata->dev_priv;

    pr_info("vma->vm_pgoff=%lx\n", vma->vm_pgoff);
    // Check bounds of memory map
    if (offset & ~PAGE_MASK) {
        pr_err("%s: offest %lu not aligned to %lu\n", __func__, offset, ~PAGE_MASK);
        return -EINVAL;
    }

    // mark no cache for mmap pages
    //set_memory_uc(dev_priv->kbuf_dma, (dev_priv->dma_size/PAGE_SIZE));
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    vma->vm_flags |= VM_LOCKED;
    result = remap_pfn_range(vma, vma->vm_start, (dev_priv->pcie_io_baseaddr >> PAGE_SHIFT) + vma->vm_pgoff,
            vsize, vma->vm_page_prot);

    if (result) return -EAGAIN;

    vma->vm_ops = &dev_vm_ops;
    dev_vmopen(vma);

    return 0;  
}

struct file_operations fops = {
    .owner          = THIS_MODULE,
    .mmap           = dam_mmap,
    .unlocked_ioctl = dam_ioctl,
    .open           = dam_fopen,
    .read           = dam_fread,
    .write          = dam_fwrite,
    .llseek          = dam_llseek,
    .release        = dam_frelease,
};

static int dam_init(void)
{
    int ret, i = 0;
    int major_n;
    dev_t curr_dev;

    pr_info_once("%s: %s version: %s git: %s\n", __func__, DRV_DESCRIPTION, DRV_MODULE_VERSION, GIT_COMMIT);

    ret = alloc_chrdev_region(&first_dev, FIRST_MINOR, MAX_PCIE_ENDPOINTS, dev_fname);
    if (ret) {
       pr_err("%s: alloc chrdev failed %i\n", __FUNCTION__, ret);
       return ret;
    }

    major_n = MAJOR(first_dev);
    dam_class = class_create(THIS_MODULE, DRV_MOD_CLASS);

    for (i = 0; i < MAX_PCIE_ENDPOINTS; i++) {
        curr_dev = MKDEV(major_n, i);
        cdev_init(&dam_cdev[i], &fops);
        ret = cdev_add(&dam_cdev[i], curr_dev, 1);
        if (ret) {
           pr_err("%s: alloc chrdev failed %i\n", __FUNCTION__, ret);
           goto cleanup;
        }
        device_create(dam_class, NULL, curr_dev, NULL, "dam%d", i);
    }

    total_endpoints = 0;

    // Register PCI driver
    ret = pci_register_driver(&dam_pci_driver);
    if (ret < 0) {
        pr_err("Failed to register pci driver, pci_register_driver = %d\n", ret);
        goto cleanup;
    }


    return 0;

cleanup:
    major_n = MAJOR(first_dev);
    for (i = 0; i < MAX_PCIE_ENDPOINTS; i++) {
        curr_dev = MKDEV(major_n, i);
        cdev_del(&dam_cdev[i]);
        device_destroy(dam_class, curr_dev);
    }
    class_destroy(dam_class);
    unregister_chrdev_region(first_dev, MAX_PCIE_ENDPOINTS);
    return -1;
}

static void dam_exit(void)
{
    int i = 0;
    int major = MAJOR(first_dev);
    dev_t curr_dev;

    pr_info("unregister driver\n");

    pci_unregister_driver(&dam_pci_driver);

    for (i = 0; i < MAX_PCIE_ENDPOINTS; i++) {
        curr_dev = MKDEV(major, i);
        cdev_del(&dam_cdev[i]);
        device_destroy(dam_class, curr_dev);
    }
    class_destroy(dam_class);
    unregister_chrdev_region(first_dev, MAX_PCIE_ENDPOINTS);
}

module_init(dam_init);
module_exit(dam_exit);

