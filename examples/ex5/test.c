#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "fspwm.h"
#include "music.h"

#define ARRAY_SIZE(a)	(sizeof(a) / sizeof(a[0]))

int main(int argc, char *argv[])
{
	int i;
	int fd;
	int ret;
	unsigned int freq;

	fd = open("/dev/pwm", O_RDWR);
	if (fd == -1)
		goto fail;

	ret = ioctl(fd, FSPWM_START);
	if (ret == -1)
		goto fail;

	for (i = 0; i < ARRAY_SIZE(HappyNewYear); i++) {
		ret = ioctl(fd, FSPWM_SET_FREQ, HappyNewYear[i].pitch);
		if (ret == -1)
			goto fail;
		usleep(HappyNewYear[i].dimation);
	}

	ret = ioctl(fd, FSPWM_STOP);
	if (ret == -1)
		goto fail;

	exit(EXIT_SUCCESS);
fail:
	perror("pwm test");
	exit(EXIT_FAILURE);
}

