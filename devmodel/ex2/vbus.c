#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/device.h>

static int vbus_match(struct device *dev, struct device_driver *drv)
{
	return 1;
}

static struct bus_type vbus = {
	.name = "vbus",
	.match = vbus_match,
};

EXPORT_SYMBOL(vbus);

static int __init vbus_init(void)
{
	return bus_register(&vbus);
}

static void __exit vbus_exit(void)
{
	bus_unregister(&vbus);
}

module_init(vbus_init);
module_exit(vbus_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("A virtual bus");
