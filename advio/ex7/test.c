#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <linux/input.h>
#include <aio.h>

#include "vser.h"

void aiow_completion_handler(sigval_t sigval)
{
	int ret;
	struct aiocb *req;

	req = (struct aiocb *)sigval.sival_ptr;

	if (aio_error(req) == 0) {
		ret = aio_return(req);
		printf("aio write %d bytes\n", ret);
	}

	return;
}

void aior_completion_handler(sigval_t sigval)
{
	int ret;
	struct aiocb *req;

	req = (struct aiocb *)sigval.sival_ptr;

	if (aio_error(req) == 0) {
		ret = aio_return(req);
		if (ret)
			printf("aio read: %s\n", (char *)req->aio_buf);
	}

	return;
}

int main(int argc, char *argv[])
{
	int ret;
	int fd;
	struct aiocb aiow, aior;

	fd = open("/dev/vser0", O_RDWR);
	if (fd == -1) 
		goto fail;

	memset(&aiow, 0, sizeof(aiow));
	memset(&aior, 0, sizeof(aior));

	aiow.aio_fildes = fd;
	aiow.aio_buf = malloc(32);
	strcpy((char *)aiow.aio_buf, "aio test");
	aiow.aio_nbytes = strlen((char *)aiow.aio_buf) + 1;
	aiow.aio_offset = 0;
	aiow.aio_sigevent.sigev_notify = SIGEV_THREAD;
	aiow.aio_sigevent.sigev_notify_function = aiow_completion_handler;
	aiow.aio_sigevent.sigev_notify_attributes = NULL;
	aiow.aio_sigevent.sigev_value.sival_ptr = &aiow;

	aior.aio_fildes = fd;
	aior.aio_buf = malloc(32);
	aior.aio_nbytes = 32;
	aior.aio_offset = 0;
	aior.aio_sigevent.sigev_notify = SIGEV_THREAD;
	aior.aio_sigevent.sigev_notify_function = aior_completion_handler;
	aior.aio_sigevent.sigev_notify_attributes = NULL;
	aior.aio_sigevent.sigev_value.sival_ptr = &aior;

	while (1) {
		if (aio_write(&aiow) == -1)
			goto fail;
		if (aio_read(&aior) == -1)
			goto fail;
		sleep(1);
	}

fail:
	perror("aio test");
	exit(EXIT_FAILURE);
}
