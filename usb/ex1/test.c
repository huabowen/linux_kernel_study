#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "pdiusbd12.h"

int main(int argc, char *argv[])
{
	int fd;
	int ret;
	unsigned char key[8];
	unsigned char led[8];
	unsigned int count, i;

	fd = open("/dev/pdiusbd12", O_RDWR);
	if (fd == -1)
		goto fail;

	while (1) {
		ret = ioctl(fd, PDIUSBD12_GET_KEY, key);
		if (ret == -1) {
			if (errno == ETIMEDOUT)
				continue;
			else
				goto fail;
		}
		switch (key[0]) {
		case 0x00:
			puts("KEYn released");
			break;
		case 0x01:
			puts("KEY2 pressed");
			break;
		case 0x02:
			puts("KEY3 pressed");
			break;
		case 0x04:
			puts("KEY4 pressed");
			break;
		case 0x08:
			puts("KEY5 pressed");
			break;
		case 0x10:
			puts("KEY6 pressed");
			break;
		case 0x20:
			puts("KEY7 pressed");
			break;
		case 0x40:
			puts("KEY8 pressed");
			break;
		case 0x80:
			puts("KEY9 pressed");
			break;
		}

		if (key[0] != 0) {
			led[0] = key[0];
			ret = ioctl(fd, PDIUSBD12_SET_LED, key);
			if (ret == -1)
				goto fail;
		}
	}
fail:
	perror("usb test");
	exit(EXIT_FAILURE);
}

