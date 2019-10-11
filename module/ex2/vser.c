#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

static int __init vser_init(void)
{
	printk("vser_init\n");
	return 0;
}

static void __exit vser_exit(void)
{
	printk("vser_exit\n");
}

module_init(vser_init);
module_exit(vser_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("A simple module");
MODULE_ALIAS("virtual-serial");
