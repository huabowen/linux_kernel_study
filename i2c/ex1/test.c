#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "i2c-dev.h"

#define	SMPLRT_DIV	0x19
#define	CONFIG		0x1A
#define	GYRO_CONFIG	0x1B
#define	ACCEL_CONFIG	0x1C
#define	ACCEL_XOUT_H	0x3B
#define	ACCEL_XOUT_L	0x3C
#define	ACCEL_YOUT_H	0x3D
#define	ACCEL_YOUT_L	0x3E
#define	ACCEL_ZOUT_H	0x3F
#define	ACCEL_ZOUT_L	0x40
#define	TEMP_OUT_H	0x41
#define	TEMP_OUT_L	0x42
#define	GYRO_XOUT_H	0x43
#define	GYRO_XOUT_L	0x44
#define	GYRO_YOUT_H	0x45
#define	GYRO_YOUT_L	0x46
#define	GYRO_ZOUT_H	0x47
#define	GYRO_ZOUT_L	0x48
#define	PWR_MGMT_1	0x6B

void swap_int16(short *val)
{
	*val = (*val << 8) | (*val >> 8);
}

int main(int argc, char *argv[])
{
	int fd;
	int ret;
	short accelx, accely, accelz;
	short temp;
	short gyrox, gyroy, gyroz;
	unsigned char *p;

	fd = open("/dev/i2c-5", O_RDWR);
	if (fd == -1)
		goto fail;

	if (ioctl(fd, I2C_SLAVE, 0x68) < 0)
		goto fail;

	i2c_smbus_write_byte_data(fd, PWR_MGMT_1, 0x80);
	usleep(200000);
	i2c_smbus_write_byte_data(fd, PWR_MGMT_1, 0x40);
	i2c_smbus_write_byte_data(fd, PWR_MGMT_1, 0x00);

	i2c_smbus_write_byte_data(fd, SMPLRT_DIV,   0x7);
	i2c_smbus_write_byte_data(fd, CONFIG,       0x6);
	i2c_smbus_write_byte_data(fd, GYRO_CONFIG,  0x3 << 3);
	i2c_smbus_write_byte_data(fd, ACCEL_CONFIG, 0x3 << 3);

	while (1) {
		accelx = i2c_smbus_read_word_data(fd, ACCEL_XOUT_H);
		swap_int16(&accelx);
		accely = i2c_smbus_read_byte_data(fd, ACCEL_YOUT_H);
		swap_int16(&accely);
		accelz = i2c_smbus_read_byte_data(fd, ACCEL_ZOUT_H);
		swap_int16(&accelz);

		printf("accelx: %.2f\n", accelx / 2048.0);
		printf("accely: %.2f\n", accely / 2048.0);
		printf("accelz: %.2f\n", accelz / 2048.0);

		temp = i2c_smbus_read_word_data(fd, TEMP_OUT_H);
		swap_int16(&temp);
		printf("temp: %.2f\n", temp / 340.0 + 36.53);

		gyrox = i2c_smbus_read_word_data(fd, GYRO_XOUT_H);
		swap_int16(&gyrox);
		gyroy = i2c_smbus_read_byte_data(fd, GYRO_YOUT_H);
		swap_int16(&gyroy);
		gyroz = i2c_smbus_read_byte_data(fd, GYRO_ZOUT_H);
		swap_int16(&gyroz);

		printf("gyrox: %.2f\n", gyrox / 16.4);
		printf("gyroy: %.2f\n", gyroy / 16.4);
		printf("gyroz: %.2f\n", gyroz / 16.4);

		sleep(1);
	}

fail:
	perror("i2c test");
	exit(EXIT_FAILURE);
}
