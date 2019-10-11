#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

int main(int argc, char * argv[])
{
	int fd;
	char *start;
	int i;
	char buf[32];

	fd = open("/dev/vfb0", O_RDWR);
	if (fd == -1)
		goto fail;

	start = mmap(NULL, 32, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (start == MAP_FAILED)
		goto fail;
	
	for (i = 0; i < 26; i++)
		*(start + i) = 'a' + i;
	*(start + i) = '\0';

	if (read(fd, buf, 27) == -1)
		goto fail;

	puts(buf);

	munmap(start, 32);
	return 0;

fail:
	perror("mmap test");
	exit(EXIT_FAILURE);
}

