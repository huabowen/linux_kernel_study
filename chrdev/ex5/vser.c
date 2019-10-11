#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>

#define VSER_MAJOR	256
#define VSER_MINOR	0
#define VSER_DEV_CNT	2
#define VSER_DEV_NAME	"vser"

static DEFINE_KFIFO(vsfifo0, char, 32);
static DEFINE_KFIFO(vsfifo1, char, 32);

struct vser_dev {
	struct kfifo *fifo;
	struct cdev cdev;
};

static struct vser_dev vsdev[2];

static int vser_open(struct inode *inode, struct file *filp)
{
	filp->private_data = container_of(inode->i_cdev, struct vser_dev, cdev);
	return 0;
}

static int vser_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t vser_read(struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
	unsigned int copied = 0;
	struct vser_dev *dev = filp->private_data;

	kfifo_to_user(dev->fifo, buf, count, &copied);

	return copied;
}

static ssize_t vser_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{
	unsigned int copied = 0;
	struct vser_dev *dev = filp->private_data;

	kfifo_from_user(dev->fifo, buf, count, &copied);

	return copied;
}

static struct file_operations vser_ops = {
	.owner = THIS_MODULE,
	.open = vser_open,
	.release = vser_release,
	.read = vser_read,
	.write = vser_write,
};

static int __init vser_init(void)
{
	int i;
	int ret;
	dev_t dev;

	dev = MKDEV(VSER_MAJOR, VSER_MINOR);
	ret = register_chrdev_region(dev, VSER_DEV_CNT, VSER_DEV_NAME);
	if (ret)
		goto reg_err;

	for (i = 0; i < VSER_DEV_CNT; i++) {
		cdev_init(&vsdev[i].cdev, &vser_ops);
		vsdev[i].cdev.owner = THIS_MODULE;
		vsdev[i].fifo = i == 0 ? (struct kfifo *) &vsfifo0 : (struct kfifo*)&vsfifo1;

		ret = cdev_add(&vsdev[i].cdev, dev + i, 1);
		if (ret)
			goto add_err;
	}

	return 0;

add_err:
	for (--i; i > 0; --i)
		cdev_del(&vsdev[i].cdev);
	unregister_chrdev_region(dev, VSER_DEV_CNT);
reg_err:
	return ret;
}

static void __exit vser_exit(void)
{
	int i;
	dev_t dev;

	dev = MKDEV(VSER_MAJOR, VSER_MINOR);

	for (i = 0; i < VSER_DEV_CNT; i++)
		cdev_del(&vsdev[i].cdev);
	unregister_chrdev_region(dev, VSER_DEV_CNT);
}

module_init(vser_init);
module_exit(vser_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("A simple character device driver");
MODULE_ALIAS("virtual-serial");
