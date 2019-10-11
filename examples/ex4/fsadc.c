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
#include <linux/interrupt.h>

#include "fsadc.h"

#define FSADC_MAJOR	256
#define FSADC_MINOR	6
#define FSADC_DEV_NAME	"fsadc"

struct fsadc_dev {
	unsigned int __iomem *adccon;
	unsigned int __iomem *adcdat;
	unsigned int __iomem *clrint;
	unsigned int __iomem *adcmux;

	unsigned int adcval;
	struct completion completion;
	atomic_t available;
	unsigned int irq;
	struct cdev cdev;
};

static int fsadc_open(struct inode *inode, struct file *filp)
{
	struct fsadc_dev *fsadc = container_of(inode->i_cdev, struct fsadc_dev, cdev);

	filp->private_data = fsadc;
	if (atomic_dec_and_test(&fsadc->available))
		return 0;
	else {
		atomic_inc(&fsadc->available);
		return -EBUSY;
	}
}

static int fsadc_release(struct inode *inode, struct file *filp)
{
	struct fsadc_dev *fsadc = filp->private_data;

	atomic_inc(&fsadc->available);
	return 0;
}

static long fsadc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct fsadc_dev *fsadc = filp->private_data;
	union chan_val cv;

	if (_IOC_TYPE(cmd) != FSADC_MAGIC)
		return -ENOTTY;

	switch (cmd) {
	case FSADC_GET_VAL:
		if (copy_from_user(&cv, (union chan_val __user *)arg, sizeof(union chan_val)))
			return -EFAULT;
		if (cv.chan > AIN3)
			return -ENOTTY;
		writel(cv.chan, fsadc->adcmux);
		writel(readl(fsadc->adccon) | 1, fsadc->adccon);
		if (wait_for_completion_interruptible(&fsadc->completion))
			return -ERESTARTSYS;
		cv.val = fsadc->adcval & 0xFFF;
		if (copy_to_user( (union chan_val __user *)arg, &cv, sizeof(union chan_val)))
			return -EFAULT;
		break;
	default:
		return -ENOTTY;
	}

	return 0;
}

static irqreturn_t fsadc_isr(int irq, void *dev_id)
{
	struct fsadc_dev *fsadc = dev_id;

	fsadc->adcval = readl(fsadc->adcdat);
	writel(1, fsadc->clrint);
	complete(&fsadc->completion);

	return IRQ_HANDLED;
}

static struct file_operations fsadc_ops = {
	.owner = THIS_MODULE,
	.open = fsadc_open,
	.release = fsadc_release,
	.unlocked_ioctl = fsadc_ioctl,
};

static int fsadc_probe(struct platform_device *pdev)
{
	int ret;
	dev_t dev;
	struct fsadc_dev *fsadc;
	struct resource *res;

	dev = MKDEV(FSADC_MAJOR, FSADC_MINOR);
	ret = register_chrdev_region(dev, 1, FSADC_DEV_NAME);
	if (ret)
		goto reg_err;

	fsadc = kzalloc(sizeof(struct fsadc_dev), GFP_KERNEL);
	if (!fsadc) {
		ret = -ENOMEM;
		goto mem_err;
	}
	platform_set_drvdata(pdev, fsadc);

	cdev_init(&fsadc->cdev, &fsadc_ops);
	fsadc->cdev.owner = THIS_MODULE;
	ret = cdev_add(&fsadc->cdev, dev, 1);
	if (ret)
		goto add_err;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -ENOENT;
		goto res_err;
	}

	fsadc->adccon = ioremap(res->start, resource_size(res));
	if (!fsadc->adccon) {
		ret = -EBUSY;
		goto map_err;
	}
	fsadc->adcdat = fsadc->adccon + 3;
	fsadc->clrint = fsadc->adccon + 6;
	fsadc->adcmux = fsadc->adccon + 7;

	fsadc->irq = platform_get_irq(pdev, 0);
	if (fsadc->irq < 0) {
		ret = fsadc->irq;
		goto irq_err;
	}

	ret = request_irq(fsadc->irq, fsadc_isr, 0, "adc", fsadc);
	if (ret)
		goto irq_err;

	writel((1 << 16) | (1 << 14) | (19 << 6), fsadc->adccon);

	init_completion(&fsadc->completion);
	atomic_set(&fsadc->available, 1);

	return 0;
irq_err:
	iounmap(fsadc->adccon);
map_err:
res_err:
	cdev_del(&fsadc->cdev);
add_err:
	kfree(fsadc);
mem_err:
	unregister_chrdev_region(dev, 1);
reg_err:
	return ret;
}

static int fsadc_remove(struct platform_device *pdev)
{
	dev_t dev;
	struct fsadc_dev *fsadc = platform_get_drvdata(pdev);

	dev = MKDEV(FSADC_MAJOR, FSADC_MINOR);

	writel((readl(fsadc->adccon) & ~(1 << 16)) | (1 << 2), fsadc->adccon);
	free_irq(fsadc->irq, fsadc);
	iounmap(fsadc->adccon);
	cdev_del(&fsadc->cdev);
	kfree(fsadc);
	unregister_chrdev_region(dev, 1);
	return 0;
}

static const struct of_device_id fsadc_of_matches[] = {
	{ .compatible = "fs4412,fsadc", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, fsadc_of_matches);

struct platform_driver fsadc_drv = { 
	.driver = { 
		.name    = "fsadc",
		.owner   = THIS_MODULE,
		.of_match_table = of_match_ptr(fsadc_of_matches),
	},  
	.probe   = fsadc_probe,
	.remove  = fsadc_remove,
};

module_platform_driver(fsadc_drv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("ADC driver");
