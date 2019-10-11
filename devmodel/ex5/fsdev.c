#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/platform_device.h>

static void fsdev_release(struct device *dev)
{
}

static struct resource led2_resources[] = {
	[0] = DEFINE_RES_MEM(0x11000C40, 4),
};

static struct resource led3_resources[] = {
	[0] = DEFINE_RES_MEM(0x11000C20, 4),
};

static struct resource led4_resources[] = {
	[0] = DEFINE_RES_MEM(0x114001E0, 4),
};

static struct resource led5_resources[] = {
	[0] = DEFINE_RES_MEM(0x114001E0, 4),
};

unsigned int led2pin = 7;
unsigned int led3pin = 0;
unsigned int led4pin = 4;
unsigned int led5pin = 5;

struct platform_device fsled2 = {
	.name = "fsled",
	.id = 2,
	.num_resources = ARRAY_SIZE(led2_resources),
	.resource = led2_resources,
	.dev = {
		.release = fsdev_release,
		.platform_data = &led2pin,
	},
};

struct platform_device fsled3 = {
	.name = "fsled",
	.id = 3,
	.num_resources = ARRAY_SIZE(led3_resources),
	.resource = led3_resources,
	.dev = {
		.release = fsdev_release,
		.platform_data = &led3pin,
	},
};

struct platform_device fsled4 = {
	.name = "fsled",
	.id = 4,
	.num_resources = ARRAY_SIZE(led4_resources),
	.resource = led4_resources,
	.dev = {
		.release = fsdev_release,
		.platform_data = &led4pin,
	},
};

struct platform_device fsled5 = {
	.name = "fsled",
	.id = 5,
	.num_resources = ARRAY_SIZE(led5_resources),
	.resource = led5_resources,
	.dev = {
		.release = fsdev_release,
		.platform_data = &led5pin,
	},
};

static struct platform_device *fsled_devices[]  = {
	&fsled2,
	&fsled3,
	&fsled4,
	&fsled5,
};

static int __init fsdev_init(void)
{
	return platform_add_devices(fsled_devices, ARRAY_SIZE(fsled_devices));
}

static void __exit fsdev_exit(void)
{
	platform_device_unregister(&fsled5);
	platform_device_unregister(&fsled4);
	platform_device_unregister(&fsled3);
	platform_device_unregister(&fsled2);
}

module_init(fsdev_init);
module_exit(fsdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("register LED devices");
