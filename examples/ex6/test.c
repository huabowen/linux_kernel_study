#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "fsrtc.h"

int main(int argc, char *argv[])
{
	int fd;
	int ret;
	struct rtc_time time;

	fd = open("/dev/rtc", O_RDWR);
	if (fd == -1)
		goto fail;

	time.tm_sec  = 59;
	time.tm_min  = 59;
	time.tm_hour = 23;
	time.tm_mday = 8;
	time.tm_mon  = 8;
	time.tm_year = 2016;

	ret = ioctl(fd, FSRTC_SET, &time);
	if (ret == -1)
		goto fail;

	while (1) {
		ret = ioctl(fd, FSRTC_GET, &time);
		if (ret == -1)
			goto fail;

		printf("%d-%d-%d %d:%d:%d\n", time.tm_year, time.tm_mon, time.tm_mday,\
				time.tm_hour, time.tm_min, time.tm_sec);
		sleep(1);
	}

	exit(EXIT_SUCCESS);
fail:
	perror("adc test");
	exit(EXIT_FAILURE);
}

