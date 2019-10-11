#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/uaccess.h>

#include "pdiusbd12.h"

#define PDIUSBD12_MAJOR		256
#define PDIUSBD12_MINOR		10
#define PDIUSBD12_DEV_NAME	"pdiusbd12"

struct pdiusbd12_dev {
	int pipe_ep1_out;
	int pipe_ep1_in;
	int pipe_ep2_out;
	int pipe_ep2_in;
	int maxp_ep1_out;
	int maxp_ep1_in;
	int maxp_ep2_out;
	int maxp_ep2_in;
	struct urb *ep2inurb;
	int errors;
	unsigned int ep2inlen;
	unsigned char ep1inbuf[16];
	unsigned char ep1outbuf[16];
	unsigned char ep2inbuf[64];
	unsigned char ep2outbuf[64];
	struct usb_device *usbdev;
	wait_queue_head_t wq;
	struct cdev cdev;
	dev_t dev;
};

static unsigned int minor = PDIUSBD12_MINOR;

static int pdiusbd12_open(struct inode *inode, struct file *filp)
{
	struct pdiusbd12_dev *pdiusbd12;

	pdiusbd12 = container_of(inode->i_cdev, struct pdiusbd12_dev, cdev);
	filp->private_data = pdiusbd12;

	return 0;
}

static int pdiusbd12_release(struct inode *inode, struct file *filp)
{
	struct pdiusbd12_dev *pdiusbd12;

	pdiusbd12 = container_of(inode->i_cdev, struct pdiusbd12_dev, cdev);
	usb_kill_urb(pdiusbd12->ep2inurb);

	return 0;
}

void usb_read_complete(struct urb * urb)
{
	struct pdiusbd12_dev *pdiusbd12 = urb->context;

	switch (urb->status) {
		case 0:
			pdiusbd12->ep2inlen = urb->actual_length;
			break;
		case -ECONNRESET:
		case -ENOENT:
		case -ESHUTDOWN:
		default:
			pdiusbd12->ep2inlen = 0;
			break;
	}
	pdiusbd12->errors = urb->status;
	wake_up_interruptible(&pdiusbd12->wq);
}

static ssize_t pdiusbd12_read(struct file *filp, char __user *buf, size_t count, loff_t *f_ops)
{
	int ret;
	struct usb_device *usbdev;
	struct pdiusbd12_dev *pdiusbd12 = filp->private_data;

	count = count > sizeof(pdiusbd12->ep2inbuf) ? sizeof(pdiusbd12->ep1inbuf) : count;

	ret = count;
	usbdev = pdiusbd12->usbdev;
	usb_fill_bulk_urb(pdiusbd12->ep2inurb, usbdev, pdiusbd12->pipe_ep2_in, pdiusbd12->ep2inbuf, ret, usb_read_complete, pdiusbd12);
	if (usb_submit_urb(pdiusbd12->ep2inurb, GFP_KERNEL))
		return -EIO;
	interruptible_sleep_on(&pdiusbd12->wq);

	if (pdiusbd12->errors)
		return pdiusbd12->errors;
	else {
		if (copy_to_user(buf, pdiusbd12->ep2inbuf, pdiusbd12->ep2inlen))
			return -EFAULT;
		else
			return pdiusbd12->ep2inlen;
	}
}

static ssize_t pdiusbd12_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_ops)
{
	int len;
	ssize_t ret = 0;
	struct pdiusbd12_dev *pdiusbd12 = filp->private_data;

	count = count > sizeof(pdiusbd12->ep2outbuf) ? sizeof(pdiusbd12->ep2outbuf) : count;
	if (copy_from_user(pdiusbd12->ep2outbuf, buf, count))
		return -EFAULT;

	ret = usb_bulk_msg(pdiusbd12->usbdev, pdiusbd12->pipe_ep2_out, pdiusbd12->ep2outbuf, count, &len, 10 * HZ);
	if (ret)
		return ret;
	else
		return len;
}

long pdiusbd12_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret;
	int len;
	struct pdiusbd12_dev *pdiusbd12 = filp->private_data;

	if (_IOC_TYPE(cmd) != PDIUSBD12_MAGIC)
		return -ENOTTY;

	switch (cmd) {
	case PDIUSBD12_GET_KEY:
		ret = usb_interrupt_msg(pdiusbd12->usbdev, pdiusbd12->pipe_ep1_in, pdiusbd12->ep1inbuf, 8, &len, 10 * HZ);
		if (ret)
			return ret;
		else {
			if (copy_to_user((unsigned char __user *)arg, pdiusbd12->ep1inbuf, len))
				return -EFAULT;
			else
				return 0;
		}
		break;
	case PDIUSBD12_SET_LED:
		if (copy_from_user(pdiusbd12->ep1outbuf, (unsigned char __user *)arg, 8))
			return -EFAULT;
		ret = usb_interrupt_msg(pdiusbd12->usbdev, pdiusbd12->pipe_ep1_out, pdiusbd12->ep1outbuf, 8, &len, 10 * HZ);
		if (ret)
			return ret;
		else
			return 0;
	default:
		return -ENOTTY;
	}

	return 0;
}

static struct file_operations pdiusbd12_ops = {
	.owner = THIS_MODULE,
	.open = pdiusbd12_open,
	.release = pdiusbd12_release,
	.read = pdiusbd12_read,
	.write = pdiusbd12_write,
	.unlocked_ioctl = pdiusbd12_ioctl,
};

int pdiusbd12_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	static struct pdiusbd12_dev *pdiusbd12;
	struct usb_device *usbdev;
	struct usb_host_interface *interface;
	struct usb_endpoint_descriptor *endpoint;
	int ret = 0;

	pdiusbd12 = kmalloc(sizeof(struct pdiusbd12_dev), GFP_KERNEL);
	if (!pdiusbd12)
		return -ENOMEM;

	usbdev = interface_to_usbdev(intf);
	interface = intf->cur_altsetting;
	if (interface->desc.bNumEndpoints != 4) {
		ret = -ENODEV;
		goto out_no_dev;
	}

	/* EP1 Interrupt IN */
	endpoint = &interface->endpoint[0].desc;
	if (!(endpoint->bEndpointAddress & 0x80)) {	/* IN */
		ret = -ENODEV;
		goto out_no_dev;
	}
	if ((endpoint->bmAttributes & 0x7F) != 3) {	/* Interrupt */
		ret = -ENODEV;
		goto out_no_dev;
	}
	pdiusbd12->pipe_ep1_in = usb_rcvintpipe(usbdev, endpoint->bEndpointAddress);
	pdiusbd12->maxp_ep1_in = usb_maxpacket(usbdev, pdiusbd12->pipe_ep1_in, usb_pipeout(pdiusbd12->pipe_ep1_in));

	/* EP1 Interrupt Out */
	endpoint = &interface->endpoint[1].desc;
	if (endpoint->bEndpointAddress & 0x80) {	/* OUT */
		ret = -ENODEV;
		goto out_no_dev;
	}
	if ((endpoint->bmAttributes & 0x7F) != 3) {	/* Interrupt */
		ret = -ENODEV;
		goto out_no_dev;
	}
	pdiusbd12->pipe_ep1_out = usb_sndintpipe(usbdev, endpoint->bEndpointAddress);
	pdiusbd12->maxp_ep1_out = usb_maxpacket(usbdev, pdiusbd12->pipe_ep1_out, usb_pipeout(pdiusbd12->pipe_ep1_out));

	/* EP2 Bulk IN */
	endpoint = &interface->endpoint[2].desc;
	if (!(endpoint->bEndpointAddress & 0x80)) {	/* IN */
		ret = -ENODEV;
		goto out_no_dev;
	}
	if ((endpoint->bmAttributes & 0x7F) != 2) {	/* Bulk */
		ret = -ENODEV;
		goto out_no_dev;
	}
	pdiusbd12->pipe_ep2_in = usb_rcvintpipe(usbdev, endpoint->bEndpointAddress);
	pdiusbd12->maxp_ep2_in = usb_maxpacket(usbdev, pdiusbd12->pipe_ep2_in, usb_pipeout(pdiusbd12->pipe_ep2_in));

	endpoint = &interface->endpoint[3].desc;
	if (endpoint->bEndpointAddress & 0x80) {	/* OUT */
		ret = -ENODEV;
		goto out_no_dev;
	}
	if ((endpoint->bmAttributes & 0x7F) != 2) {	/* Bulk */
		ret = -ENODEV;
		goto out_no_dev;
	}
	pdiusbd12->pipe_ep2_out = usb_sndintpipe(usbdev, endpoint->bEndpointAddress);
	pdiusbd12->maxp_ep2_out = usb_maxpacket(usbdev, pdiusbd12->pipe_ep2_out, usb_pipeout(pdiusbd12->pipe_ep2_out));

	pdiusbd12->ep2inurb = usb_alloc_urb(0, GFP_KERNEL);
	pdiusbd12->usbdev = usbdev;
	usb_set_intfdata(intf, pdiusbd12);

	pdiusbd12->dev = MKDEV(PDIUSBD12_MAJOR, minor++);
	ret = register_chrdev_region (pdiusbd12->dev, 1, PDIUSBD12_DEV_NAME);
	if (ret < 0)
		goto out_reg_region;

	cdev_init(&pdiusbd12->cdev, &pdiusbd12_ops);
	pdiusbd12->cdev.owner = THIS_MODULE;
	ret = cdev_add(&pdiusbd12->cdev, pdiusbd12->dev, 1);
	if (ret)
		goto out_cdev_add;

	init_waitqueue_head(&pdiusbd12->wq);

	return 0;

out_cdev_add:
	unregister_chrdev_region(pdiusbd12->dev, 1);
out_reg_region:
	usb_free_urb(pdiusbd12->ep2inurb);
out_no_dev:
	kfree(pdiusbd12);
	return ret;
}

void pdiusbd12_disconnect(struct usb_interface *intf)
{
	struct pdiusbd12_dev *pdiusbd12 = usb_get_intfdata(intf);

	cdev_del(&pdiusbd12->cdev);
	unregister_chrdev_region(pdiusbd12->dev, 1);
	usb_kill_urb(pdiusbd12->ep2inurb);
	usb_free_urb(pdiusbd12->ep2inurb);
	kfree(pdiusbd12);
}

static struct usb_device_id id_table [] = {
	{ USB_DEVICE(0x8888, 0x000b) }, 
	{ }
};
MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver pdiusbd12_driver = 
{
	.name  = "pdiusbd12",
	.id_table = id_table,
	.probe = pdiusbd12_probe,
	.disconnect = pdiusbd12_disconnect,
};

module_usb_driver(pdiusbd12_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("PDIUSBD12 driver");
