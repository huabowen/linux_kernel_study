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
	char rbuf[32] = {0};

	fd = open("/dev/vser0", O_RDWR | O_NONBLOCK);
	if (fd == -1)
		goto fail;

	ret = read(fd, rbuf, sizeof(rbuf));
	if (ret < 0)
		perror("read");

	close(fd);
	exit(EXIT_SUCCESS);

fail:
	perror("ioctl test");
	exit(EXIT_FAILURE);
}

