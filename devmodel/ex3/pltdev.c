#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/platform_device.h>

static void pdev_release(struct device *dev)
{
}

struct platform_device pdev0 = {
	.name = "pdev",
	.id = 0,
	.num_resources = 0,
	.resource = NULL,
	.dev = {
		.release = pdev_release,
	},
};

struct platform_device pdev1 = {
	.name = "pdev",
	.id = 1,
	.num_resources = 0,
	.resource = NULL,
	.dev = {
		.release = pdev_release,
	},
};

static int __init pltdev_init(void)
{
	platform_device_register(&pdev0);
	platform_device_register(&pdev1);

	return 0;
}

static void __exit pltdev_exit(void)
{
	platform_device_unregister(&pdev1);
	platform_device_unregister(&pdev0);
}

module_init(pltdev_init);
module_exit(pltdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("register a platfom device");
