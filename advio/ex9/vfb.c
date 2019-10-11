#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define VFB_MAJOR	256
#define VFB_MINOR	1
#define VFB_DEV_CNT	1
#define VFB_DEV_NAME	"vfbdev"

struct vfb_dev {
	unsigned char *buf;
	struct cdev cdev;
};

static struct vfb_dev vfbdev;

static int vfb_open(struct inode * inode, struct file * filp)
{
	return 0;
}

static int vfb_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static int vfb_mmap(struct file *filp, struct vm_area_struct *vma)
{
	if (remap_pfn_range(vma, vma->vm_start, virt_to_phys(vfbdev.buf) >> PAGE_SHIFT, \
		vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;
	return 0;
}

ssize_t vfb_read(struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
	int ret;
	size_t len = (count > PAGE_SIZE) ? PAGE_SIZE : count;

	ret = copy_to_user(buf, vfbdev.buf, len);

	return len - ret;
}

static struct file_operations vfb_fops = {
	.owner = THIS_MODULE,
	.open = vfb_open,
	.release = vfb_release,
	.mmap = vfb_mmap,
	.read = vfb_read,
};

static int __init vfb_init(void)
{
	int ret;
	dev_t dev;
	unsigned long addr;

	dev = MKDEV(VFB_MAJOR, VFB_MINOR);
	ret = register_chrdev_region(dev, VFB_DEV_CNT, VFB_DEV_NAME);
	if (ret)
		goto reg_err;

	cdev_init(&vfbdev.cdev, &vfb_fops);
	vfbdev.cdev.owner = THIS_MODULE;
	ret = cdev_add(&vfbdev.cdev, dev, VFB_DEV_CNT);
	if (ret)
		goto add_err;

	addr = __get_free_page(GFP_KERNEL);
	if (!addr)
		goto get_err;

	vfbdev.buf = (unsigned char *)addr;
	memset(vfbdev.buf, 0, PAGE_SIZE);

	return 0;

get_err:
	cdev_del(&vfbdev.cdev);
add_err:
	unregister_chrdev_region(dev, VFB_DEV_CNT);
reg_err:
	return ret;
}

static void __exit vfb_exit(void)
{
	dev_t dev;

	dev = MKDEV(VFB_MAJOR, VFB_MINOR);

	free_page((unsigned long)vfbdev.buf);
	cdev_del(&vfbdev.cdev);
	unregister_chrdev_region(dev, VFB_DEV_CNT);
}

module_init(vfb_init);
module_exit(vfb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("This is an example for mmap");
