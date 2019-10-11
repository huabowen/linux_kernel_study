#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/slab.h>
#include <linux/kobject.h>

static struct kset *kset;
static struct kobject *kobj1;
static struct kobject *kobj2;
static unsigned int val = 0;

static ssize_t val_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t val_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	char *endp;

	printk("size = %d\n", count);
	val = simple_strtoul(buf, &endp, 10);

	return count;
}

static struct kobj_attribute kobj1_val_attr = __ATTR(val, 0666, val_show, val_store);
static struct attribute *kobj1_attrs[] = {
	&kobj1_val_attr.attr,
	NULL,
};

static struct attribute_group kobj1_attr_group = { 
	        .attrs = kobj1_attrs,
};

static int __init model_init(void)
{
	int ret;

	kset = kset_create_and_add("kset", NULL, NULL);
	kobj1 = kobject_create_and_add("kobj1", &kset->kobj);
	kobj2 = kobject_create_and_add("kobj2", &kset->kobj);

	ret = sysfs_create_group(kobj1, &kobj1_attr_group);
	ret = sysfs_create_link(kobj2, kobj1, "kobj1");

	return 0;
}

static void __exit model_exit(void)
{
	sysfs_remove_link(kobj2, "kobj1");
	sysfs_remove_group(kobj1, &kobj1_attr_group);
	kobject_del(kobj2);
	kobject_del(kobj1);
	kset_unregister(kset);
}

module_init(model_init);
module_exit(model_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("A simple module for device model");
