#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <linux/input.h>


#define KEY_DEV_PATH "/dev/input/event0"

int fs4412_get_key(void)
{
	int fd; 
	struct input_event event;

	fd = open(KEY_DEV_PATH, O_RDONLY);
	if(fd == -1) {
		perror("fskey->open");
		return -1;
	}

	while (1) {
		if(read(fd, &event, sizeof(event)) == sizeof(event)) {
			if (event.type == EV_KEY && event.value == 1) {
				close(fd);
				return event.code;
			} else
				continue;
		} else {
			close(fd);
			fprintf(stderr, "fskey->read: read failed\n");
			return -1;
		}
	}
}

int main(int argc, char *argv[])
{
	while (1)
		printf("key value: %d\n", fs4412_get_key());
}
