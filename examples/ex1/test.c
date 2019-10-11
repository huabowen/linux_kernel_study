#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#define LED_DEV_PATH	"/sys/class/leds/led%d/brightness"
#define ON		1
#define OFF		0

int fs4412_set_led(unsigned int lednum, unsigned int mode)
{
	int fd;
	int ret;
	char devpath[128];
	char *on = "1\n";
	char *off = "0\n";
	char *m = NULL;

	snprintf(devpath, sizeof(devpath), LED_DEV_PATH, lednum);
	fd = open(devpath, O_WRONLY);
	if (fd == -1) {
		perror("fsled->open");
		return -1;
	}

	if (mode == ON)
		m = on;
	else
		m = off;

	ret = write(fd, m, strlen(m));
	if (ret == -1) {
		perror("fsled->write");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int main(int argc, char *argv[])
{
	unsigned int lednum = 2;

	while (1) {
		fs4412_set_led(lednum, ON);
		usleep(500000);
		fs4412_set_led(lednum, OFF);
		usleep(500000);

		lednum++;
		if (lednum > 5)
			lednum = 2;
	}
}
