#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/platform_device.h>

static int pdrv_suspend(struct device *dev)
{
	printk("pdev: suspend\n");
	return 0;
}

static int pdrv_resume(struct device *dev)
{
	printk("pdev: resume\n");
	return 0;
}

static const struct dev_pm_ops pdrv_pm_ops = {
	.suspend = pdrv_suspend,
	.resume  = pdrv_resume,
};

static int pdrv_probe(struct platform_device *pdev)
{
	return 0;
}

static int pdrv_remove(struct platform_device *pdev)
{
	return 0;
}

struct platform_driver pdrv = {
	.driver = {
		.name    = "pdev",
		.owner   = THIS_MODULE,
		.pm      = &pdrv_pm_ops,
	},
	.probe   = pdrv_probe,
	.remove  = pdrv_remove,
};

module_platform_driver(pdrv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("A simple platform driver");
MODULE_ALIAS("platform:pdev");
