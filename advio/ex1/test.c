#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "vser.h"

int main(int argc, char *argv[])
{
	int fd;
	int ret;
	unsigned int baud;
	struct option opt = {8,1,1};

	fd = open("/dev/vser0", O_RDWR);
	if (fd == -1)
		goto fail;

	baud = 9600;
	ret = ioctl(fd, VS_SET_BAUD, baud);
	if (ret == -1)
		goto fail;

	ret = ioctl(fd, VS_GET_BAUD, baud);
	if (ret == -1)
		goto fail;

	ret = ioctl(fd, VS_SET_FFMT, &opt);
	if (ret == -1)
		goto fail;

	ret = ioctl(fd, VS_GET_FFMT, &opt);
	if (ret == -1)
		goto fail;

	printf("baud rate: %d\n", baud);
	printf("frame format: %d%c%d\n", opt.datab, opt.parity == 0 ? 'N' : \
			opt.parity == 1 ? 'O' : 'E', \
			opt.stopb);

	close(fd);
	exit(EXIT_SUCCESS);

fail:
	perror("ioctl test");
	exit(EXIT_FAILURE);
}

