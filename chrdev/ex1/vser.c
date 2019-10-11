#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>

#define VSER_MAJOR	256
#define VSER_MINOR	0
#define VSER_DEV_CNT	1
#define VSER_DEV_NAME	"vser"

static int __init vser_init(void)
{
	int ret;
	dev_t dev;

	dev = MKDEV(VSER_MAJOR, VSER_MINOR);
	ret = register_chrdev_region(dev, VSER_DEV_CNT, VSER_DEV_NAME);
	if (ret)
		goto reg_err;
	return 0;

reg_err:
	return ret;
}

static void __exit vser_exit(void)
{
	
	dev_t dev;

	dev = MKDEV(VSER_MAJOR, VSER_MINOR);
	unregister_chrdev_region(dev, VSER_DEV_CNT);
}

module_init(vser_init);
module_exit(vser_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("A simple character device driver");
MODULE_ALIAS("virtual-serial");
