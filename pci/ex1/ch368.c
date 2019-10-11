#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/uaccess.h>

#include "ch368.h"

#define CH368_MAJOR	256
#define CH368_MINOR	11
#define CH368_DEV_NAME	"ch368"

struct ch368_dev {
	void __iomem *io_addr;
	void __iomem *mem_addr;
	unsigned long io_len;
	unsigned long mem_len;
	struct pci_dev *pdev;
	struct cdev cdev;
	dev_t dev;
};

static unsigned int minor = CH368_MINOR;

static int ch368_open(struct inode *inode, struct file *filp)
{
	struct ch368_dev *ch368;

	ch368 = container_of(inode->i_cdev, struct ch368_dev, cdev);
	filp->private_data = ch368;

	return 0;
}

static int ch368_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t ch368_read(struct file *filp, char __user *buf, size_t count, loff_t *f_ops)
{
	int ret;
	struct ch368_dev *ch368 = filp->private_data;

	count = count > ch368->mem_len ? ch368->mem_len : count;
	ret = copy_to_user(buf, ch368->mem_addr, count);

	return count - ret;
}

static ssize_t ch368_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_ops)
{
	int ret;
	struct ch368_dev *ch368 = filp->private_data;

	count = count > ch368->mem_len ? ch368->mem_len : count;
	ret = copy_from_user(ch368->mem_addr, buf, count);

	return count - ret;
}

static long ch368_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	union addr_data ad;
	struct ch368_dev *ch368 = filp->private_data;

	if (_IOC_TYPE(cmd) != CH368_MAGIC)
		return -ENOTTY;

	if (copy_from_user(&ad, (union addr_data __user *)arg, sizeof(union addr_data)))
		return -EFAULT;

	switch (cmd) {
	case CH368_RD_CFG:
		if (ad.addr > 0x3F)
			return -ENOTTY;
		pci_read_config_byte(ch368->pdev, ad.addr, &ad.data);
		if (copy_to_user((union addr_data __user *)arg, &ad, sizeof(union addr_data)))
			return -EFAULT;
		break;
	case CH368_WR_CFG:
		if (ad.addr > 0x3F)
			return -ENOTTY;
		pci_write_config_byte(ch368->pdev, ad.addr, ad.data);
		break;
	case CH368_RD_IO:
		ad.data = ioread8(ch368->io_addr + ad.addr);
		if (copy_to_user((union addr_data __user *)arg, &ad, sizeof(union addr_data)))
			return -EFAULT;
		break;
	case CH368_WR_IO:
		iowrite8(ad.data, ch368->io_addr + ad.addr);
		break;
	default:
		return -ENOTTY;
	}

	return 0;
}

static struct file_operations ch368_ops = {
	.owner = THIS_MODULE,
	.open = ch368_open,
	.release = ch368_release,
	.read = ch368_read,
	.write = ch368_write,
	.unlocked_ioctl = ch368_ioctl,
};

static int ch368_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int ret;

	unsigned long io_start;
	unsigned long io_end;
	unsigned long io_flags;
	unsigned long io_len;
	void __iomem *io_addr = NULL;

	unsigned long mem_start;
	unsigned long mem_end;
	unsigned long mem_flags;
	unsigned long mem_len;
	void __iomem *mem_addr = NULL;

	struct ch368_dev *ch368;

	ret = pci_enable_device(pdev);
	if(ret)
		goto enable_err;

	io_start  = pci_resource_start(pdev, 0);
	io_end    = pci_resource_end(pdev, 0);
	io_flags  = pci_resource_flags(pdev, 0);
	io_len    = pci_resource_len(pdev, 0);

	mem_start = pci_resource_start(pdev, 1);
	mem_end   = pci_resource_end(pdev, 1);
	mem_flags = pci_resource_flags(pdev, 1);
	mem_len   = pci_resource_len(pdev, 1);

	if (!(io_flags & IORESOURCE_IO) || !(mem_flags & IORESOURCE_MEM)) {
		ret = -ENODEV;
		goto res_err;
	}

	ret = pci_request_regions(pdev, "ch368");
	if (ret)
		goto res_err;

	io_addr = ioport_map(io_start, io_len);
	if (io_addr == NULL) {
		ret = -EIO;
		goto ioport_map_err;
	}

	mem_addr = ioremap(mem_start, mem_len);
	if (mem_addr == NULL) {
		ret = -EIO;
		goto ioremap_err;
	}

	ch368 = kzalloc(sizeof(struct ch368_dev), GFP_KERNEL);
	if (!ch368) {
		ret = -ENOMEM;
		goto mem_err;
	}
	pci_set_drvdata(pdev, ch368);

	ch368->io_addr = io_addr;
	ch368->mem_addr = mem_addr;
	ch368->io_len = io_len;
	ch368->mem_len = mem_len;
	ch368->pdev = pdev;

	ch368->dev = MKDEV(CH368_MAJOR, minor++);
	ret = register_chrdev_region (ch368->dev, 1, CH368_DEV_NAME);
	if (ret < 0)
		goto region_err;

	cdev_init(&ch368->cdev, &ch368_ops);
	ch368->cdev.owner = THIS_MODULE;
	ret = cdev_add(&ch368->cdev, ch368->dev, 1); 
	if (ret)
		goto add_err;

	return 0;

add_err:
	unregister_chrdev_region(ch368->dev, 1);
region_err:
	kfree(ch368);
mem_err:
	iounmap(mem_addr);
ioremap_err:
	ioport_unmap(io_addr);
ioport_map_err:
	pci_release_regions(pdev);
res_err:
	pci_disable_device(pdev);
enable_err:
	return ret;
}

static void ch368_remove(struct pci_dev *pdev)
{
	struct ch368_dev *ch368 = pci_get_drvdata(pdev);

	cdev_del(&ch368->cdev);
	unregister_chrdev_region(ch368->dev, 1);
	iounmap(ch368->mem_addr);
	ioport_unmap(ch368->io_addr);
	kfree(ch368);
	pci_release_regions(pdev);
	pci_disable_device(pdev);
}

static struct pci_device_id ch368_id_table[] =	
{
	{0x1C00, 0x5834, 0x1C00, 0x5834, 0, 0, 0},
	{0,}
};
MODULE_DEVICE_TABLE(pci, ch368_id_table);

static struct pci_driver ch368_driver = {
	.name = "ch368",
	.id_table = ch368_id_table,
	.probe = ch368_probe,
	.remove = ch368_remove,
};

module_pci_driver(ch368_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("CH368 driver");
