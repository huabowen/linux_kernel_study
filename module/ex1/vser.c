#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

int init_module(void)
{
	printk("module init\n");
	return 0;
}

void cleanup_module(void)
{
	printk("cleanup module\n");
}
