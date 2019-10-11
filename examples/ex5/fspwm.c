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

#include "fspwm.h"

#define FSPWM_MAJOR	256
#define FSPWM_MINOR	7
#define FSPWM_DEV_NAME	"fspwm"

struct fspwm_dev {
	unsigned int __iomem *tcfg0;
	unsigned int __iomem *tcfg1;
	unsigned int __iomem *tcon;
	unsigned int __iomem *tcntb0;
	unsigned int __iomem *tcmpb0;
	unsigned int __iomem *tcnto0;
	struct clk *clk;
	unsigned long freq;
	struct pinctrl	*pctrl;
	atomic_t available;
	struct cdev cdev;
};

static int fspwm_open(struct inode *inode, struct file *filp)
{
	struct fspwm_dev *fspwm = container_of(inode->i_cdev, struct fspwm_dev, cdev);

	filp->private_data = fspwm;
	if (atomic_dec_and_test(&fspwm->available))
		return 0;
	else {
		atomic_inc(&fspwm->available);
		return -EBUSY;
	}
}

static int fspwm_release(struct inode *inode, struct file *filp)
{
	struct fspwm_dev *fspwm = filp->private_data;

	atomic_inc(&fspwm->available);
	return 0;
}

static long fspwm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct fspwm_dev *fspwm = filp->private_data;
	unsigned int div;

	if (_IOC_TYPE(cmd) != FSPWM_MAGIC)
		return -ENOTTY;

	switch (cmd) {
	case FSPWM_START:
		writel(readl(fspwm->tcon) | 0x1, fspwm->tcon);
		break;
	case FSPWM_STOP:
		writel(readl(fspwm->tcon) & ~0x1, fspwm->tcon);
		break;
	case FSPWM_SET_FREQ:
		if (arg > fspwm->freq || arg == 0)
			return -ENOTTY;
		div = fspwm->freq / arg - 1;
		writel(div, fspwm->tcntb0);
		writel(div / 2, fspwm->tcmpb0);
		writel(readl(fspwm->tcon) | 0x2, fspwm->tcon);
		writel(readl(fspwm->tcon) & ~0x2, fspwm->tcon);
		break;
	default:
		return -ENOTTY;
	}

	return 0;
}

static struct file_operations fspwm_ops = {
	.owner = THIS_MODULE,
	.open = fspwm_open,
	.release = fspwm_release,
	.unlocked_ioctl = fspwm_ioctl,
};

static int fspwm_probe(struct platform_device *pdev)
{
	int ret;
	dev_t dev;
	struct fspwm_dev *fspwm;
	struct resource *res;
	unsigned int prescaler0;

	dev = MKDEV(FSPWM_MAJOR, FSPWM_MINOR);
	ret = register_chrdev_region(dev, 1, FSPWM_DEV_NAME);
	if (ret)
		goto reg_err;

	fspwm = kzalloc(sizeof(struct fspwm_dev), GFP_KERNEL);
	if (!fspwm) {
		ret = -ENOMEM;
		goto mem_err;
	}
	platform_set_drvdata(pdev, fspwm);

	cdev_init(&fspwm->cdev, &fspwm_ops);
	fspwm->cdev.owner = THIS_MODULE;
	ret = cdev_add(&fspwm->cdev, dev, 1);
	if (ret)
		goto add_err;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -ENOENT;
		goto res_err;
	}

	fspwm->tcfg0 = ioremap(res->start, resource_size(res));
	if (!fspwm->tcfg0) {
		ret = -EBUSY;
		goto map_err;
	}
	fspwm->tcfg1  = fspwm->tcfg0 + 1;
	fspwm->tcon   = fspwm->tcfg0 + 2;
	fspwm->tcntb0 = fspwm->tcfg0 + 3;
	fspwm->tcmpb0 = fspwm->tcfg0 + 4;
	fspwm->tcnto0 = fspwm->tcfg0 + 5;

	fspwm->clk = clk_get(&pdev->dev, "timers");
	if (IS_ERR(fspwm->clk)) {
		ret =  PTR_ERR(fspwm->clk);
		goto get_clk_err;
	}

	ret = clk_prepare_enable(fspwm->clk);
	if (ret < 0)
		goto enable_clk_err;
	fspwm->freq = clk_get_rate(fspwm->clk);

	prescaler0 = readl(fspwm->tcfg0) & 0xFF;
	writel((readl(fspwm->tcfg1) & ~0xF) | 0x4, fspwm->tcfg1); 	/* 1/16 */
	fspwm->freq /= (prescaler0 + 1) * 16;				/* 3125000 */
	writel((readl(fspwm->tcon) & ~0xF) | 0x8, fspwm->tcon);		/* auto-reload */

	fspwm->pctrl = devm_pinctrl_get_select_default(&pdev->dev);

	atomic_set(&fspwm->available, 1);

	return 0;

enable_clk_err:
	clk_put(fspwm->clk);
get_clk_err:
	iounmap(fspwm->tcfg0);
map_err:
res_err:
	cdev_del(&fspwm->cdev);
add_err:
	kfree(fspwm);
mem_err:
	unregister_chrdev_region(dev, 1);
reg_err:
	return ret;
}

static int fspwm_remove(struct platform_device *pdev)
{
	dev_t dev;
	struct fspwm_dev *fspwm = platform_get_drvdata(pdev);

	dev = MKDEV(FSPWM_MAJOR, FSPWM_MINOR);

	clk_disable_unprepare(fspwm->clk);
	clk_put(fspwm->clk);
	iounmap(fspwm->tcfg0);
	cdev_del(&fspwm->cdev);
	kfree(fspwm);
	unregister_chrdev_region(dev, 1);
	return 0;
}

static const struct of_device_id fspwm_of_matches[] = {
	{ .compatible = "fs4412,fspwm", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, fspwm_of_matches);

struct platform_driver fspwm_drv = { 
	.driver = { 
		.name    = "fspwm",
		.owner   = THIS_MODULE,
		.of_match_table = of_match_ptr(fspwm_of_matches),
	},  
	.probe   = fspwm_probe,
	.remove  = fspwm_remove,
};

module_platform_driver(fspwm_drv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("PWM driver");
