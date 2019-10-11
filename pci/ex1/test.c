#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "ch368.h"

int main(int argc, char *argv[])
{
	int i;
	int fd;
	int ret;
	union addr_data ad;
	unsigned char id[4];

	fd = open("/dev/ch368", O_RDWR);
	if (fd == -1)
		goto fail;

	for (i = 0; i < sizeof(id); i++) {
		ad.addr = i;
		if (ioctl(fd, CH368_RD_CFG, &ad))
			goto fail;
		id[i] = ad.data;
	}

	printf("VID: 0x%02x%02x, DID: 0x%02x%02x\n", id[1], id[0], id[3], id[2]);

	i = 0;
	ad.addr = 0;
	while (1) {
		ad.data = i++;
		if (ioctl(fd, CH368_WR_IO, &ad))
			goto fail;
		i %= 15;
		sleep(1);
	}
fail:
	perror("pci test");
	exit(EXIT_FAILURE);
}

