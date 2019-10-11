#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/device.h>

extern struct bus_type vbus;

static struct device_driver vdrv = {
	.name = "vdrv",
	.bus = &vbus,
};

static int __init vdrv_init(void)
{
	return driver_register(&vdrv);
}

static void __exit vdrv_exit(void)
{
	driver_unregister(&vdrv);
}

module_init(vdrv_init);
module_exit(vdrv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("A virtual device driver");
