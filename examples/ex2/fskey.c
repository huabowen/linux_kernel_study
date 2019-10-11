#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>

#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>

#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

struct resource *key2_res;
struct resource *key3_res;

static irqreturn_t fskey_handler(int irq, void *dev_id)
{
	if (irq == key2_res->start)
		printk("K2 pressed\n");
	else
		printk("K3 pressed\n");

	return IRQ_HANDLED;
}

static int fskey_probe(struct platform_device *pdev)
{
	int ret;

	key2_res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	key3_res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);

	if (!key2_res || !key3_res) {
		ret = -ENOENT;
		goto res_err;
	}

	ret = request_irq(key2_res->start, fskey_handler, key2_res->flags & IRQF_TRIGGER_MASK, "key2", NULL);
	if (ret)
		goto key2_err;
	ret = request_irq(key3_res->start, fskey_handler, key3_res->flags & IRQF_TRIGGER_MASK, "key3", NULL);
	if (ret)
		goto key3_err;

	return 0;

key3_err:
	free_irq(key2_res->start, NULL);
key2_err:
res_err:
	return ret;
}

static int fskey_remove(struct platform_device *pdev)
{
	free_irq(key3_res->start, NULL);
	free_irq(key2_res->start, NULL);

	return 0;
}

static const struct of_device_id fskey_of_matches[] = {
	{ .compatible = "fs4412,fskey", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, fskey_of_matches);

struct platform_driver fskey_drv = { 
	.driver = { 
		.name    = "fskey",
		.owner   = THIS_MODULE,
		.of_match_table = of_match_ptr(fskey_of_matches),
	},  
	.probe   = fskey_probe,
	.remove  = fskey_remove,
};

module_platform_driver(fskey_drv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("A simple device driver for Keys on FS4412 board");
