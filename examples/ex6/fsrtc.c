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

#include <linux/of.h>
#include <linux/clk.h>
#include <linux/pinctrl/consumer.h>
#include <linux/bcd.h>

#include "fsrtc.h"

#define FSRTC_MAJOR	256
#define FSRTC_MINOR	8
#define FSRTC_DEV_NAME	"fsrtc"

struct fsrtc_dev {
	unsigned int __iomem *rtccon;
	unsigned int __iomem *bcdsec;
	unsigned int __iomem *bcdmin;
	unsigned int __iomem *bcdhour;
	unsigned int __iomem *bcdday;
	unsigned int __iomem *bcdmon;
	unsigned int __iomem *bcdyear;
	struct clk *clk;
	atomic_t available;
	struct cdev cdev;
};

static int fsrtc_open(struct inode *inode, struct file *filp)
{
	struct fsrtc_dev *fsrtc = container_of(inode->i_cdev, struct fsrtc_dev, cdev);

	filp->private_data = fsrtc;
	if (atomic_dec_and_test(&fsrtc->available))
		return 0;
	else {
		atomic_inc(&fsrtc->available);
		return -EBUSY;
	}
}

static int fsrtc_release(struct inode *inode, struct file *filp)
{
	struct fsrtc_dev *fsrtc = filp->private_data;

	atomic_inc(&fsrtc->available);
	return 0;
}

static long fsrtc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct fsrtc_dev *fsrtc = filp->private_data;
	struct rtc_time time;

	if (_IOC_TYPE(cmd) != FSRTC_MAGIC)
		return -ENOTTY;

	switch (cmd) {
	case FSRTC_SET:
		if (copy_from_user(&time, (struct rtc_time __user *)arg, sizeof(struct rtc_time)))
			return -ENOTTY;
		writel(readl(fsrtc->rtccon) | 0x1, fsrtc->rtccon);

		writel(bin2bcd(time.tm_sec ),  fsrtc->bcdsec);
		writel(bin2bcd(time.tm_min ),  fsrtc->bcdmin);
		writel(bin2bcd(time.tm_hour),  fsrtc->bcdhour);
		writel(bin2bcd(time.tm_mday),  fsrtc->bcdday);
		writel(bin2bcd(time.tm_mon ),  fsrtc->bcdmon);
		writel(bin2bcd(time.tm_year - 2000),  fsrtc->bcdyear);

		writel(readl(fsrtc->rtccon) & ~0x1, fsrtc->rtccon);
		break;
	case FSRTC_GET:
		time.tm_sec  = bcd2bin(readl(fsrtc->bcdsec));
		time.tm_min  = bcd2bin(readl(fsrtc->bcdmin));
		time.tm_hour = bcd2bin(readl(fsrtc->bcdhour));
		time.tm_mday = bcd2bin(readl(fsrtc->bcdday));
		time.tm_mon  = bcd2bin(readl(fsrtc->bcdmon));
		time.tm_year = bcd2bin(readl(fsrtc->bcdyear)) + 2000;

		if (copy_to_user((struct rtc_time __user *)arg, &time, sizeof(struct rtc_time)))
			return -ENOTTY;
		break;
	default:
		return -ENOTTY;
	}

	return 0;
}

static struct file_operations fsrtc_ops = {
	.owner = THIS_MODULE,
	.open = fsrtc_open,
	.release = fsrtc_release,
	.unlocked_ioctl = fsrtc_ioctl,
};

static int fsrtc_probe(struct platform_device *pdev)
{
	int ret;
	dev_t dev;
	struct fsrtc_dev *fsrtc;
	struct resource *res;
	unsigned int __iomem *regbase;

	dev = MKDEV(FSRTC_MAJOR, FSRTC_MINOR);
	ret = register_chrdev_region(dev, 1, FSRTC_DEV_NAME);
	if (ret)
		goto reg_err;

	fsrtc = kzalloc(sizeof(struct fsrtc_dev), GFP_KERNEL);
	if (!fsrtc) {
		ret = -ENOMEM;
		goto mem_err;
	}
	platform_set_drvdata(pdev, fsrtc);

	cdev_init(&fsrtc->cdev, &fsrtc_ops);
	fsrtc->cdev.owner = THIS_MODULE;
	ret = cdev_add(&fsrtc->cdev, dev, 1);
	if (ret)
		goto add_err;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -ENOENT;
		goto res_err;
	}

	regbase = ioremap(res->start, resource_size(res));
	if (!regbase) {
		ret = -EBUSY;
		goto map_err;
	}
	fsrtc->rtccon     = regbase + 16;
	fsrtc->bcdsec     = regbase + 28;
	fsrtc->bcdmin     = regbase + 29;
	fsrtc->bcdhour    = regbase + 30;
	fsrtc->bcdday     = regbase + 31;
	fsrtc->bcdmon     = regbase + 33;
	fsrtc->bcdyear    = regbase + 34;

	fsrtc->clk = clk_get(&pdev->dev, "rtc");
	if (IS_ERR(fsrtc->clk)) {
		ret =  PTR_ERR(fsrtc->clk);
		goto get_clk_err;
	}

	ret = clk_prepare_enable(fsrtc->clk);
	if (ret < 0)
		goto enable_clk_err;

	writel(0, fsrtc->rtccon);

	atomic_set(&fsrtc->available, 1);

	return 0;

enable_clk_err:
	clk_put(fsrtc->clk);
get_clk_err:
	iounmap(fsrtc->rtccon - 16);
map_err:
res_err:
	cdev_del(&fsrtc->cdev);
add_err:
	kfree(fsrtc);
mem_err:
	unregister_chrdev_region(dev, 1);
reg_err:
	return ret;
}

static int fsrtc_remove(struct platform_device *pdev)
{
	dev_t dev;
	struct fsrtc_dev *fsrtc = platform_get_drvdata(pdev);

	dev = MKDEV(FSRTC_MAJOR, FSRTC_MINOR);

	clk_disable_unprepare(fsrtc->clk);
	clk_put(fsrtc->clk);
	iounmap(fsrtc->rtccon - 16);
	cdev_del(&fsrtc->cdev);
	kfree(fsrtc);
	unregister_chrdev_region(dev, 1);
	return 0;
}

static const struct of_device_id fsrtc_of_matches[] = {
	{ .compatible = "fs4412,fsrtc", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, fsrtc_of_matches);

struct platform_driver fsrtc_drv = { 
	.driver = { 
		.name    = "fsrtc",
		.owner   = THIS_MODULE,
		.of_match_table = of_match_ptr(fsrtc_of_matches),
	},  
	.probe   = fsrtc_probe,
	.remove  = fsrtc_remove,
};

module_platform_driver(fsrtc_drv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("RTC driver");
