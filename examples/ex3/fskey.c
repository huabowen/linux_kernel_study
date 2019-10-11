#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/slab.h>
#include <linux/delay.h>

#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/platform_device.h>

#define MAX_KEYS_NUM		(8)
#define SCAN_INTERVAL		(50)	/* ms */
#define KB_ACTIVATE_DELAY	(20)	/* us */
#define KBDSCAN_STABLE_COUNT	(3)

struct fskey_dev {
	unsigned int count;
	unsigned int kstate[MAX_KEYS_NUM];
	unsigned int kcount[MAX_KEYS_NUM];
	unsigned char keycode[MAX_KEYS_NUM];
	int gpio[MAX_KEYS_NUM];
	struct input_polled_dev *polldev;
};

static void fskey_poll(struct input_polled_dev *dev)
{
	unsigned int index;
	unsigned int kstate;
	struct fskey_dev *fskey = dev->private;

	for (index = 0; index < fskey->count; index++)
		fskey->kcount[index] = 0;

	index = 0;
	do {
		udelay(KB_ACTIVATE_DELAY);
		kstate = gpio_get_value(fskey->gpio[index]);
		if (kstate != fskey->kstate[index]) {
			fskey->kstate[index] = kstate;
			fskey->kcount[index] = 0;
		} else {
			if (++fskey->kcount[index] >= KBDSCAN_STABLE_COUNT) {
				input_report_key(dev->input, fskey->keycode[index], !kstate);
				index++;
			}
		}
	} while (index < fskey->count);

	input_sync(dev->input);
}

static int fskey_probe(struct platform_device *pdev)
{
	int ret;
	int index;
	struct fskey_dev *fskey;

	fskey = kzalloc(sizeof(struct fskey_dev), GFP_KERNEL);
	if (!fskey)
		return -ENOMEM;

	platform_set_drvdata(pdev, fskey);

	for (index = 0; index < MAX_KEYS_NUM; index++) {
		ret = of_get_gpio(pdev->dev.of_node, index);
		if (ret < 0)
			break;
		else
			fskey->gpio[index] = ret;
	}

	if (!index)
		goto gpio_err;
	else
		fskey->count = index;

	for (index = 0; index < fskey->count; index++) {
		ret = gpio_request(fskey->gpio[index], "KEY");
		if (ret)
			goto req_err;

		gpio_direction_input(fskey->gpio[index]);
		fskey->keycode[index] = KEY_2 + index;
		fskey->kstate[index] = 1;
	}

	fskey->polldev = input_allocate_polled_device();
	if (!fskey->polldev) {
		ret = -ENOMEM;
		goto req_err;
	}

	fskey->polldev->private = fskey;
	fskey->polldev->poll = fskey_poll;
	fskey->polldev->poll_interval = SCAN_INTERVAL;

	fskey->polldev->input->name = "FS4412 Keyboard";
	fskey->polldev->input->phys = "fskbd/input0";
	fskey->polldev->input->id.bustype = BUS_HOST;
	fskey->polldev->input->id.vendor = 0x0001;
	fskey->polldev->input->id.product = 0x0001;
	fskey->polldev->input->id.version = 0x0100;
	fskey->polldev->input->dev.parent = &pdev->dev;

	fskey->polldev->input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);
	fskey->polldev->input->keycode = fskey->keycode;
	fskey->polldev->input->keycodesize = sizeof(unsigned char);
	fskey->polldev->input->keycodemax = index;

	for (index = 0; index < fskey->count; index++)
		__set_bit(fskey->keycode[index], fskey->polldev->input->keybit);
	__clear_bit(KEY_RESERVED, fskey->polldev->input->keybit);

	ret = input_register_polled_device(fskey->polldev);
	if (ret)
		goto reg_err;

	return 0;

reg_err:
	input_free_polled_device(fskey->polldev);
req_err:
	for (index--; index >= 0; index--)
		gpio_free(fskey->gpio[index]);
gpio_err:
	kfree(fskey);
	return ret;
}

static int fskey_remove(struct platform_device *pdev)
{
	unsigned int index;
	struct fskey_dev *fskey = platform_get_drvdata(pdev);

	input_unregister_polled_device(fskey->polldev);
	input_free_polled_device(fskey->polldev);
	for (index = 0; index < fskey->count; index++)
		gpio_free(fskey->gpio[index]);
	kfree(fskey);

	return 0;
}

static const struct of_device_id fskey_of_matches[] = {
	{ .compatible = "fs4412,fskey", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, fskey_of_matches);

struct platform_driver fskey_drv = { 
	.driver = { 
		.name    = "fskey",
		.owner   = THIS_MODULE,
		.of_match_table = of_match_ptr(fskey_of_matches),
	},  
	.probe   = fskey_probe,
	.remove  = fskey_remove,
};

module_platform_driver(fskey_drv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("A simple device driver for Keys on FS4412 board");
