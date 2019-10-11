#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>

#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>

#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include "mpu6050.h"

#define FSRTC_MAJOR	256
#define FSRTC_MINOR	9
#define FSRTC_DEV_NAME	"mpu6050"

#define SMPLRT_DIV	0x19
#define CONFIG		0x1A
#define GYRO_CONFIG	0x1B
#define ACCEL_CONFIG	0x1C
#define ACCEL_XOUT_H	0x3B
#define ACCEL_XOUT_L	0x3C
#define ACCEL_YOUT_H	0x3D
#define ACCEL_YOUT_L	0x3E
#define ACCEL_ZOUT_H	0x3F
#define ACCEL_ZOUT_L	0x40
#define TEMP_OUT_H	0x41
#define TEMP_OUT_L	0x42
#define GYRO_XOUT_H	0x43
#define GYRO_XOUT_L	0x44
#define GYRO_YOUT_H	0x45
#define GYRO_YOUT_L	0x46
#define GYRO_ZOUT_H	0x47
#define GYRO_ZOUT_L	0x48
#define PWR_MGMT_1	0x6B

struct mpu6050_dev {
	struct i2c_client *client;
	atomic_t available;
	struct cdev cdev;
};

static int mpu6050_open(struct inode *inode, struct file *filp)
{
	struct mpu6050_dev *mpu6050 = container_of(inode->i_cdev, struct mpu6050_dev, cdev);

	filp->private_data = mpu6050;
	if (atomic_dec_and_test(&mpu6050->available))
		return 0;
	else {
		atomic_inc(&mpu6050->available);
		return -EBUSY;
	}
}

static int mpu6050_release(struct inode *inode, struct file *filp)
{
	struct mpu6050_dev *mpu6050 = filp->private_data;

	atomic_inc(&mpu6050->available);
	return 0;
}

static long mpu6050_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct mpu6050_dev *mpu6050 = filp->private_data;
	struct atg_val val;

	if (_IOC_TYPE(cmd) != MPU6050_MAGIC)
		return -ENOTTY;

	switch (cmd) {
	case MPU6050_GET_VAL:
		val.accelx = i2c_smbus_read_word_data(mpu6050->client, ACCEL_XOUT_H);
		val.accely = i2c_smbus_read_word_data(mpu6050->client, ACCEL_YOUT_H);
		val.accelz = i2c_smbus_read_word_data(mpu6050->client, ACCEL_ZOUT_H);
		val.temp   = i2c_smbus_read_word_data(mpu6050->client, TEMP_OUT_H);
		val.gyrox  = i2c_smbus_read_word_data(mpu6050->client, GYRO_XOUT_H);
		val.gyroy  = i2c_smbus_read_word_data(mpu6050->client, GYRO_YOUT_H);
		val.gyroz  = i2c_smbus_read_word_data(mpu6050->client, GYRO_ZOUT_H);
		val.accelx = be16_to_cpu(val.accelx);
		val.accely = be16_to_cpu(val.accely);
		val.accelz = be16_to_cpu(val.accelz);
		val.temp   = be16_to_cpu(val.temp);
		val.gyrox  = be16_to_cpu(val.gyrox);
		val.gyroy  = be16_to_cpu(val.gyroy);
		val.gyroz  = be16_to_cpu(val.gyroz);
		if (copy_to_user((struct atg_val __user *)arg, &val, sizeof(struct atg_val)))
			return -EFAULT;
		break;
	default:
		return -ENOTTY;
	}

	return 0;
}

static struct file_operations mpu6050_ops = {
	.owner = THIS_MODULE,
	.open = mpu6050_open,
	.release = mpu6050_release,
	.unlocked_ioctl = mpu6050_ioctl,
};

static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	dev_t dev;
	struct mpu6050_dev *mpu6050;

	dev = MKDEV(FSRTC_MAJOR, FSRTC_MINOR);
	ret = register_chrdev_region(dev, 1, FSRTC_DEV_NAME);
	if (ret)
		goto reg_err;

	mpu6050 = kzalloc(sizeof(struct mpu6050_dev), GFP_KERNEL);
	if (!mpu6050) {
		ret = -ENOMEM;
		goto mem_err;
	}
	i2c_set_clientdata(client, mpu6050);
	mpu6050->client = client;

	cdev_init(&mpu6050->cdev, &mpu6050_ops);
	mpu6050->cdev.owner = THIS_MODULE;
	ret = cdev_add(&mpu6050->cdev, dev, 1);
	if (ret)
		goto add_err;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_WORD_DATA)) {
		ret = -ENOSYS;
		goto fun_err;
	}

	i2c_smbus_write_byte_data(client, PWR_MGMT_1, 0x80);
	msleep(200);
	i2c_smbus_write_byte_data(client, PWR_MGMT_1, 0x40);
	i2c_smbus_write_byte_data(client, PWR_MGMT_1, 0x00);

	i2c_smbus_write_byte_data(client, SMPLRT_DIV,   0x7);
	i2c_smbus_write_byte_data(client, CONFIG,       0x6);
	i2c_smbus_write_byte_data(client, GYRO_CONFIG,  0x3 << 3); 
	i2c_smbus_write_byte_data(client, ACCEL_CONFIG, 0x3 << 3);

	atomic_set(&mpu6050->available, 1);

	return 0;

fun_err:
	cdev_del(&mpu6050->cdev);
add_err:
	kfree(mpu6050);
mem_err:
	unregister_chrdev_region(dev, 1);
reg_err:
	return ret;
}

static int mpu6050_remove(struct i2c_client *client)
{
	dev_t dev;
	struct mpu6050_dev *mpu6050 = i2c_get_clientdata(client);

	dev = MKDEV(FSRTC_MAJOR, FSRTC_MINOR);

	cdev_del(&mpu6050->cdev);
	kfree(mpu6050);
	unregister_chrdev_region(dev, 1);
	return 0;
}

static const struct i2c_device_id mpu6050_id[] = {
	{"mpu6050", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, mpu6050_id);

static struct i2c_driver mpu6050_driver = {
	.probe          =       mpu6050_probe,
	.remove         =       mpu6050_remove,
	.id_table       =       mpu6050_id,
	.driver = {
		.owner  =       THIS_MODULE,
		.name   =       "mpu6050",
	},
};

module_i2c_driver(mpu6050_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Jiang <jiangxg@farsight.com.cn>");
MODULE_DESCRIPTION("MPU6050 driver");
