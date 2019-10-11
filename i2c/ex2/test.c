#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>

#include "mpu6050.h"


int main(int argc, char *argv[])
{
	int fd;
	struct atg_val val;


	fd = open("/dev/mpu6050", O_RDWR);

	while (2) {
		ioctl(fd, MPU6050_GET_VAL, &val);

		printf("accelx: %.2f\n", val.accelx / 2048.0);
		printf("accely: %.2f\n", val.accely / 2048.0);
		printf("accelz: %.2f\n", val.accelz / 2048.0);
		printf("temp: %.2f\n", val.temp / 340.0 + 36.53); 
		printf("gyrox: %.2f\n", val.gyrox / 16.4);
		printf("gyroy: %.2f\n", val.gyroy / 16.4);
		printf("gyroz: %.2f\n", val.gyroz / 16.4);


		sleep(1);
	}
}
