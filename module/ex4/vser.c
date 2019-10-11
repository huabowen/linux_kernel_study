#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

static int baudrate = 9600;
static int port[4] = {0, 1, 2, 3};
static char *name = "vser";

module_param(baudrate, int, S_IRUGO);
module_param_array(port, int, NULL, S_IRUGO);
module_param(name, charp, S_IRUGO);

static int __init vser_init(void)
{
	int i;

	printk("vser_init\n");
	printk("baudrate: %d\n", baudrate);
	printk("port: ");
	for (i = 0; i < ARRAY_SIZE(port); i++)
		printk("%d ", port[i]);
	printk("\n");
	printk("name: %s\n", name);

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
