#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>

#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>

#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>

#include "fsled.h"

#define FSLED_MAJOR	256
#define FSLED_DEV_NAME	"fsled"

struct fsled_dev {
	unsigned int __iomem *con;
	unsigned int __iomem *dat;
	unsigned int pin;
	atomic_t available;
	struct cdev cdev;
};

static int fsled_open(struct inode *inode, struct file *filp)
{
	struct fsled_dev *fsled = container_of(inode->i_cdev, struct fsled_dev, cdev);

	filp->private_data = fsled;
	if (atomic_dec_and_test(&fsled->available))
		return 0;
	else {
		atomic_inc(&fsled->available);
		return -EBUSY;
	}
}

static int fsled_release(struct inode *inode, struct file *filp)
{
	struct fsled_dev *fsled = filp->private_data;

	writel(readl(fsled->dat) & ~(0x1 << fsled->pin), fsled->dat);

	atomic_inc(&fsled->available);
	return 0;
}

static long fsled_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct fsled_dev *fsled = filp->private_data;

	if (_IOC_TYPE(cmd) != FSLED_MAGIC)
		return -ENOTTY;

	switch (cmd) {
	case FSLED_ON:
		writel(readl(fsled->dat) | (0x1 << fsled->pin), fsled->dat);
		break;
	case FSLED_OFF:
		writel(readl(fsled->dat) & ~(0x1 << fsled->pin), fsled->dat);
		break;
	default:
		return -ENOTTY;
	}

	return 0;
}

static struct file_operations fsled_ops = {
	.owner = THIS_MODULE,
	.open = fsled_open,
	.release = fsled_release,
	.unlocked_ioctl = fsled_ioctl,
};

static int fsled_probe(struct platform_device *pdev)
{
	int ret;
	dev_t dev;
	struct fsled_dev *fsled;
	struct resource *res;
	unsigned int pin = *(unsigned int*)pdev->dev.platform_data;

	dev = MKDEV(FSLED_MAJOR, pdev->id);
	ret = register_chrdev_region(dev, 1, FSLED_DEV_NAME);
	if (ret)
		goto reg_err;

	fsled = kzalloc(sizeof(struct fsled_dev), GFP_KERNEL);
	if (!fsled) {
		ret = -ENOMEM;
		goto mem_err;
	}

	cdev_init(&fsled->cdev, &fsled_ops);
	fsled->cdev.owner = THIS_MODULE;
	ret = cdev_add(&fsled->cdev, dev, 1);
	if (ret)
		goto add_err;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -ENOENT;
		goto res_err;
	}

	fsled->con = ioremap(res->start, resource_size(res));
	if (!fsled->con) {
		ret = -EBUSY;
		goto map_err;
	}
	fsled->dat = fsled->con + 1;

	fsled->pin = pin;
	atomic_set(&fsled->available, 1);
	writel((readl(fsled->con) & ~(0xF  << 4 * fsled->pin)) | (0x1  << 4 * fsled->pin), fsled->con);
	writel(readl(fsled->dat) & ~(0x1 << fsled->pin), fsled->dat);
	platform_set_drvdata(pdev, fsled);

	return 0;

map_err:
res_err:
	cdev_del(&fsled->cdev);
add_err:
	kfree(fsled);
mem_err:
	unregister_chrdev_region(dev, 1);
reg_err:
	return ret;
}

static int fsled_remove(struct platform_device *pdev)
{
	dev_t dev;
	struct fsled_dev *fsled = platform_get_drvdata(pdev);

	dev = MKDEV(FSLED_MAJOR, pdev->id);

	iounmap(fsled->con);
	cdev_del(&fsled->cdev);
	kfree(fsled);
	unregister_chrdev_region(dev, 1);

	return 0;
}

struct platform_driver fsled_drv = { 
	.driver = { 
		.name    = "fsled",
		.owner   = THIS_MODULE,
	},  
	.probe   = fsled_probe,
	.remove  = fsled_remove,
};

module_platform_driver(fsled_drv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("A simple character device driver for LEDs on FS4412 board");
