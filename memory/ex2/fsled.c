#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>

#include <linux/ioctl.h>
#include <linux/uaccess.h>

#include <linux/io.h>
#include <linux/ioport.h>

#include "fsled.h"

#define FSLED_MAJOR	256
#define FSLED_MINOR	0
#define FSLED_DEV_CNT	1
#define FSLED_DEV_NAME	"fsled"

#define GPX2_BASE	0x11000C40
#define GPX1_BASE	0x11000C20
#define GPF3_BASE	0x114001E0

struct fsled_dev {
	unsigned int __iomem *gpx2con;
	unsigned int __iomem *gpx2dat;
	unsigned int __iomem *gpx1con;
	unsigned int __iomem *gpx1dat;
	unsigned int __iomem *gpf3con;
	unsigned int __iomem *gpf3dat;
	atomic_t available;
	struct cdev cdev;
};

static struct fsled_dev fsled;

static int fsled_open(struct inode *inode, struct file *filp)
{
	if (atomic_dec_and_test(&fsled.available))
		return 0;
	else {
		atomic_inc(&fsled.available);
		return -EBUSY;
	}
}

static int fsled_release(struct inode *inode, struct file *filp)
{
	writel(readl(fsled.gpx2dat) & ~(0x1 << 7), fsled.gpx2dat);
	writel(readl(fsled.gpx1dat) & ~(0x1 << 0), fsled.gpx1dat);
	writel(readl(fsled.gpf3dat) & ~(0x3 << 4), fsled.gpf3dat);

	atomic_inc(&fsled.available);
	return 0;
}

static long fsled_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	if (_IOC_TYPE(cmd) != FSLED_MAGIC)
		return -ENOTTY;

	switch (cmd) {
	case FSLED_ON:
		switch(arg) {
		case LED2:
			writel(readl(fsled.gpx2dat) | (0x1 << 7), fsled.gpx2dat);
			break;
		case LED3:
			writel(readl(fsled.gpx1dat) | (0x1 << 0), fsled.gpx1dat);
			break;
		case LED4:
			writel(readl(fsled.gpf3dat) | (0x1 << 4), fsled.gpf3dat);
			break;
		case LED5:
			writel(readl(fsled.gpf3dat) | (0x1 << 5), fsled.gpf3dat);
			break;
		default:
			return -ENOTTY;
		}
		break;
	case FSLED_OFF:
		switch(arg) {
		case LED2:
			writel(readl(fsled.gpx2dat) & ~(0x1 << 7), fsled.gpx2dat);
			break;
		case LED3:
			writel(readl(fsled.gpx1dat) & ~(0x1 << 0), fsled.gpx1dat);
			break;
		case LED4:
			writel(readl(fsled.gpf3dat) & ~(0x1 << 4), fsled.gpf3dat);
			break;
		case LED5:
			writel(readl(fsled.gpf3dat) & ~(0x1 << 5), fsled.gpf3dat);
			break;
		default:
			return -ENOTTY;
		}
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

static int __init fsled_init(void)
{
	int ret;
	dev_t dev;

	dev = MKDEV(FSLED_MAJOR, FSLED_MINOR);
	ret = register_chrdev_region(dev, FSLED_DEV_CNT, FSLED_DEV_NAME);
	if (ret)
		goto reg_err;

	memset(&fsled, 0, sizeof(fsled));
	atomic_set(&fsled.available, 1);
	cdev_init(&fsled.cdev, &fsled_ops);
	fsled.cdev.owner = THIS_MODULE;

	ret = cdev_add(&fsled.cdev, dev, FSLED_DEV_CNT);
	if (ret)
		goto add_err;

	fsled.gpx2con = ioremap(GPX2_BASE, 8);
	fsled.gpx1con = ioremap(GPX1_BASE, 8);
	fsled.gpf3con = ioremap(GPF3_BASE, 8);

	if (!fsled.gpx2con || !fsled.gpx1con || !fsled.gpf3con) {
		ret = -EBUSY;
		goto map_err;
	}

	fsled.gpx2dat = fsled.gpx2con + 1;
	fsled.gpx1dat = fsled.gpx1con + 1;
	fsled.gpf3dat = fsled.gpf3con + 1;

	writel((readl(fsled.gpx2con) & ~(0xF  << 28)) | (0x1  << 28), fsled.gpx2con);
	writel((readl(fsled.gpx1con) & ~(0xF  <<  0)) | (0x1  <<  0), fsled.gpx1con);
	writel((readl(fsled.gpf3con) & ~(0xFF << 16)) | (0x11 << 16), fsled.gpf3con);

	writel(readl(fsled.gpx2dat) & ~(0x1 << 7), fsled.gpx2dat);
	writel(readl(fsled.gpx1dat) & ~(0x1 << 0), fsled.gpx1dat);
	writel(readl(fsled.gpf3dat) & ~(0x3 << 4), fsled.gpf3dat);

	return 0;

map_err:
	if (fsled.gpf3con)
		iounmap(fsled.gpf3con);
	if (fsled.gpx1con)
		iounmap(fsled.gpx1con);
	if (fsled.gpx2con)
		iounmap(fsled.gpx2con);
add_err:
	unregister_chrdev_region(dev, FSLED_DEV_CNT);
reg_err:
	return ret;
}

static void __exit fsled_exit(void)
{
	dev_t dev;

	dev = MKDEV(FSLED_MAJOR, FSLED_MINOR);

	iounmap(fsled.gpf3con);
	iounmap(fsled.gpx1con);
	iounmap(fsled.gpx2con);
	cdev_del(&fsled.cdev);
	unregister_chrdev_region(dev, FSLED_DEV_CNT);
}

module_init(fsled_init);
module_exit(fsled_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("A simple character device driver for LEDs on FS4412 board");
