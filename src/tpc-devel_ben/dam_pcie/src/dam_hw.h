#ifndef _DAM_HW_H
#define _DAM_HW_H

#include <linux/pci.h>

/* Used after most PCI Link IO writes */
static inline void dam_hw_pci_post(struct dam_private *dev_priv)
{
    pci_write_config_dword(dev_priv->pdev, 0, 0x12345678);
}

#define DAM_WUPPER_REG_READ(offset) _DAM_WUPPER_REG_READ(dev_priv, (offset))
#define DAM_WUPPER_REG_WRITE(val, offset) _DAM_WUPPER_REG_WRITE(dev_priv, (val), (offset))

#define DAM_PCIE_REG_READ(offset) _DAM_PCIE_REG_READ(dev_priv, (offset))
#define DAM_PCIE_REG_WRITE(val, offset) _DAM_PCIE_REG_WRITE(dev_priv, (val), (offset))
#define DAM_PCIE_MEMCPY_TOIO(offset, buf, len) _DAM_PCIE_MEMCPY_TOIO(dev_priv, (buf), (offset), (len))
#define DAM_PCIE_MEMCPY_FROMIO(buf, offset, len) _DAM_PCIE_MEMCPY_FROMIO(dev_priv, (buf), (offset), (len))

static inline u32 _DAM_WUPPER_REG_READ(struct dam_private *dev_priv, u32 offset)
{
    if (offset >= dev_priv->wupper_io_len) {
        dev_err(&dev_priv->pdev->dev,
                "Read out of range at %u\n", offset);
        return 0;
    }

    return ioread32(dev_priv->wupper_io + offset);
}

static inline void _DAM_WUPPER_REG_WRITE(struct dam_private *dev_priv, u32 val,
        u32 offset)
{
    if (offset >= dev_priv->wupper_io_len) {
        dev_err(&dev_priv->pdev->dev,
                "Write out of range at %u\n", offset);
        return;
    }

    iowrite32(val, dev_priv->wupper_io + offset);
}

static inline u32 _DAM_PCIE_REG_READ(struct dam_private *dev_priv, u32 offset)
{
    if (offset >= dev_priv->pcie_io_len) {
        dev_err(&dev_priv->pdev->dev,
                "Read out of range at %u\n", offset);
        return 0;
    }

    return ioread32(dev_priv->pcie_io + offset);
}

static inline void _DAM_PCIE_REG_WRITE(struct dam_private *dev_priv, u32 val,
        u32 offset)
{
    if (offset >= dev_priv->pcie_io_len) {
        dev_err(&dev_priv->pdev->dev,
                "Write out of range at %u\n", offset);
        return;
    }

    iowrite32(val, dev_priv->pcie_io + offset);
    //fthd_hw_pci_post(dev_priv);
}

static inline void _DAM_PCIE_MEMCPY_TOIO(struct dam_private *dev_priv, const void *buf,
        u32 offset, int len)
{
    memcpy_toio(dev_priv->pcie_io + offset, buf, len);
}


static inline void _DAM_PCIE_MEMCPY_FROMIO(struct dam_private *dev_priv, void *buf,
        u32 offset, int len)
{
    memcpy_fromio(buf, dev_priv->pcie_io + offset, len);
}

#endif
