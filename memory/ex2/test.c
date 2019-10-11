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
	int fd;
	int ret;
	int num = LED2;

	fd = open("/dev/led", O_RDWR);
	if (fd == -1)
		goto fail;

	while (1) {
		ret = ioctl(fd, FSLED_ON, num);
		if (ret == -1)
			goto fail;
		usleep(500000);
		ret = ioctl(fd, FSLED_OFF, num);
		if (ret == -1)
			goto fail;
		usleep(500000);

		num = (num + 1) % 4;
	}
fail:
	perror("led test");
	exit(EXIT_FAILURE);
}

