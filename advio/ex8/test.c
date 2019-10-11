#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>

#include "vser.h"

int fd;

void sigio_handler(int signum, siginfo_t *siginfo, void *act)
{
	int ret;
	char buf[32];

	if (signum == SIGIO) {
		if (siginfo->si_band & POLLIN) {
			printf("FIFO is not empty\n");
			if ((ret = read(fd, buf, sizeof(buf))) != -1) {
				buf[ret] = '\0';
				puts(buf);
			}
		}
		if (siginfo->si_band & POLLOUT)
			printf("FIFO is not full\n");
	}
}

int main(int argc, char *argv[])
{
	int ret;
	int flag;
	struct sigaction act, oldact;

	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGIO);
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = sigio_handler;
	if (sigaction(SIGIO, &act, &oldact) == -1)
		goto fail;

	fd = open("/dev/vser0", O_RDWR | O_NONBLOCK);
	if (fd == -1) 
		goto fail;

	if (fcntl(fd, F_SETOWN, getpid()) == -1)
		goto fail;
	if (fcntl(fd, F_SETSIG, SIGIO) == -1)
		goto fail;
	if ((flag = fcntl(fd, F_GETFL)) == -1)
		goto fail;
	if (fcntl(fd, F_SETFL, flag | FASYNC) == -1)
		goto fail;

	while (1)
		sleep(1);

fail:
	perror("fasync test");
	exit(EXIT_FAILURE);
}
