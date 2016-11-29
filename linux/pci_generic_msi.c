/*
 * This software is Copyright by the Board of Trustees of Michigan
 * State University (c) Copyright 2016.
 */
/* Similar to uio_pci_generic.c in stock kernel,
 * But uses PCI MSI instead of INT disable.
 * This driver has no ID list and must be explicitly bound.
 *
 * # echo "8086 10f5" > /sys/bus/pci/drivers/pci_generic_msi/new_id
 * # echo -n 0000:00:19.0 > /sys/bus/pci/drivers/e1000e/unbind
 * # echo -n 0000:00:19.0 > /sys/bus/pci/drivers/pci_generic_msi/bind
 * # ls -l /sys/bus/pci/devices/0000:00:19.0/driver
 * .../0000:00:19.0/driver -> ../../../bus/pci/drivers/pci_generic_msi
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/uio_driver.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/msi.h>

#ifndef VM_RESERVED
#define VM_RESERVED 0
#endif

#ifndef module_pci_driver
/* added in 3.4 */
#define module_pci_driver(__pci_driver) \
    module_driver(__pci_driver, pci_register_driver, \
               pci_unregister_driver)
#endif

#define DRV_NAME "pci_generic_msi"
#define DRV_VERSION "0.0"

struct pci_generic_msi {
    struct uio_info uio;
    struct pci_dev *pdev;
    unsigned maskable:1;
};

/** Linux 3.12 adds a size test when mapping UIO_MEM_PHYS ranges
 *  to fix an clear security issue.  7314e613d5ff9f0934f7a0f74ed7973b903315d1
 *
 *  Unfortunately this makes it impossible to map ranges less than a page,
 *  such as the control registers for the PLX bridges (128 bytes).
 *  A further change in b65502879556d041b45104c6a35abbbba28c8f2d
 *  prevents mapping of ranges which don't start on a page boundary,
 *  which is also true of the PLX chips (offset 0xc00 on my test system).
 *
 *  This remains the case though the present (4.1 in May 2015).
 *
 *  These patches have been applied to the Debian 3.2.0 kernel.
 *
 *  The following is based uio_mmap_physical() from 3.2.0.
 */
static
int mmap_generic_msi(struct uio_info *info, struct vm_area_struct *vma)
{
    struct pci_dev *dev = info->priv;
    int mi = vma->vm_pgoff; /* bounds check already done in uio_mmap() */

    if (vma->vm_end - vma->vm_start > PAGE_ALIGN(info->mem[mi].size)) {
        dev_err(&dev->dev, "mmap alignment/size test fails %lx %lx %u\n",
                vma->vm_start, vma->vm_end, (unsigned)PAGE_ALIGN(info->mem[mi].size));
        return -EINVAL;
    }

    vma->vm_flags |= VM_IO | VM_RESERVED;
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    return remap_pfn_range(vma,
                           vma->vm_start,
                           info->mem[mi].addr >> PAGE_SHIFT,
                           vma->vm_end - vma->vm_start,
                           vma->vm_page_prot);
}

static
irqreturn_t
handle_generic_msi(int irq, struct uio_info *info)
{
    //struct pci_generic_msi *priv = container_of(info, struct pci_generic_msi, uio);
    /*TODO: automatically mask this IRQ when it occurs?
    if (priv->maskable) {
        mask_msi_irq();
    }
    */
    return IRQ_HANDLED;
}

static
int control_generic_msi(struct uio_info *info, s32 onoff)
{
    //struct pci_generic_msi *priv = container_of(info, struct pci_generic_msi, uio);
    /* (un)mask MSI */
    return 0;
}

#define ERR(COND, LABEL, MSG) if(COND) { dev_err(&pdev->dev, DRV_NAME ": %s", MSG); goto LABEL; }

static
const char *mem_names[IORESOURCE_MEM] = {
    "BAR0",
    "BAR1",
    "BAR2",
    "BAR3",
    "BAR4",
    "BAR5",
};

static int probe_generic_msi(struct pci_dev *pdev,
                             const struct pci_device_id *id)
{
    struct pci_generic_msi *priv;
    int err;
    unsigned i;

    priv = kzalloc(sizeof(*priv), GFP_KERNEL);
    if (!priv) {
        err = -ENOMEM;
    }
    priv->pdev = pdev;

    priv->uio.name = DRV_NAME;
    priv->uio.version = DRV_VERSION;
    priv->uio.priv = pdev;
    priv->uio.mmap = mmap_generic_msi;

    pci_set_drvdata(pdev, priv);

    err = pci_enable_device(pdev);
    ERR(err, err_free, "Can't enable device\n");

    err = pci_enable_msi(pdev);
    ERR(err, err_disable_dev, "Device does not support MSI\n");

    priv->uio.irq = pdev->irq;
    priv->uio.handler = handle_generic_msi;
    priv->uio.irqcontrol = control_generic_msi;
    priv->uio.irq_flags = 0;

    for (i=0; i<PCI_STD_RESOURCES; i++)
    {
        unsigned long flags = pci_resource_flags(pdev, i);
        resource_size_t size = pci_resource_len(pdev, i);

        priv->uio.mem[i].name = mem_names[i];

        if ((flags & IORESOURCE_MEM) && size>0)
        {
            priv->uio.mem[i].addr = pci_resource_start(pdev, i);
            priv->uio.mem[i].size = size;
            priv->uio.mem[i].memtype = UIO_MEM_PHYS;
        } else {
            priv->uio.mem[i].size = 1; /* Otherwise UIO will stop searching... */
            priv->uio.mem[i].memtype = UIO_MEM_NONE; /* prevent mapping */
        }
    }

    {
        struct msi_desc *desc = irq_get_msi_desc(pdev->irq);
        priv->maskable = desc ? desc->msi_attrib.maskbit : 0;
        dev_info(&pdev->dev, "MSI is %smaskable\n", priv->maskable ? "" : "not ");
    }

    err = uio_register_device(&pdev->dev, &priv->uio);
    ERR(err, err_disable_msi, "Can't add UIO dev\n");

    return 0;

err_disable_msi:
    pci_disable_msi(pdev);
err_disable_dev:
    pci_disable_device(pdev);
err_free:
    pci_set_drvdata(pdev, NULL);
    kfree(priv);
    return err;
}

static void remove_generic_msi(struct pci_dev *pdev)
{
    struct uio_info *info = pci_get_drvdata(pdev);
    struct pci_generic_msi *priv = container_of(info, struct pci_generic_msi, uio);

    uio_unregister_device(info);
    pci_disable_msi(pdev);
    pci_disable_device(pdev);
    pci_set_drvdata(pdev, NULL);
    kfree(priv);
}

static struct pci_driver pci_generic_msi_driver = {
    .name = "pci_generic_msi",
    .id_table = NULL,
    .probe = probe_generic_msi,
    .remove = remove_generic_msi,
};

module_pci_driver(pci_generic_msi_driver);
MODULE_VERSION(DRV_VERSION);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Davidsaver <mdavidsaver@gmail.com>");
MODULE_DESCRIPTION("UIO Generic driver for PCI devices supporting MSI");
