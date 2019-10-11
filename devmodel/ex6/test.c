#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "fsled.h"

int main(int argc, char *argv[])
{
	int fd[4];
	int ret;
	int num = 0;

	fd[0] = open("/dev/led2", O_RDWR);
	if (fd[0] == -1)
		goto fail;
	fd[1] = open("/dev/led3", O_RDWR);
	if (fd[1] == -1)
		goto fail;
	fd[2] = open("/dev/led4", O_RDWR);
	if (fd[2] == -1)
		goto fail;
	fd[3] = open("/dev/led5", O_RDWR);
	if (fd[3] == -1)
		goto fail;

	while (1) {
		ret = ioctl(fd[num], FSLED_ON);
		if (ret == -1)
			goto fail;
		usleep(500000);
		ret = ioctl(fd[num], FSLED_OFF);
		if (ret == -1)
			goto fail;
		usleep(500000);

		num = (num + 1) % 4;
	}
fail:
	perror("led test");
	exit(EXIT_FAILURE);
}

