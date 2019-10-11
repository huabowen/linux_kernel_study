#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "fsadc.h"

int main(int argc, char *argv[])
{
	int fd;
	int ret;
	union chan_val cv;

	fd = open("/dev/adc", O_RDWR);
	if (fd == -1)
		goto fail;

	while (1) {
		cv.chan = 3;
		ret = ioctl(fd, FSADC_GET_VAL, &cv);
		if (ret == -1)
			goto fail;
		printf("current volatage is: %.2fV\n", 1.8 * cv.val / 4095.0);
		sleep(1);
	}
fail:
	perror("adc test");
	exit(EXIT_FAILURE);
}

