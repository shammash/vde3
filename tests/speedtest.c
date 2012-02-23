/* Test for vde_switch */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <libgen.h>
#include <syslog.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <math.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>

#include <libvdeplug.h>
#define BUFSIZE 2048

void exiting(int signo)
{
	exit(0);
}


int main (int argc, char **argv) {
	const unsigned char eth_hdr[14] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00};
	char buf[BUFSIZE];
	VDECONN *plug;
	int randomfd, n;
	unsigned long long *count;
	int rate = 0;
	unsigned long long interval = 0;

	if ((argc != 2) && (argc != 3)) {
		printf("Usage: %s sock [rate in frame/s]\n", argv[0]);
		exit(1);
	}

	if (argc == 3) {
		rate = atoi(argv[2]);
		if (rate < 1) {
			printf("Invalid frame rate %s\n", argv[2]);
			exit(1);
		}
		interval = 1000000 / rate;
		if (interval < 1) {
			printf("Warning: frame rate %s is too high, framerate is unlimited.\n", argv[2]);
		} else {
			printf ("Inter-frame interval %llu μs\n", interval);
		}
	}


	plug = vde_open(argv[1], "test_send", NULL);
	if (!plug) {
		perror("vde_open");
		exit(2);
	}
	randomfd=open("/dev/urandom", O_RDONLY);
	if (randomfd < 0)
		exit(3);

	n = read(randomfd, buf, BUFSIZE);
	memcpy(buf, eth_hdr, 14);
	count = (unsigned long long *)(&buf[14]);
	*count = 0;
	signal(SIGALRM, exiting);
	alarm(30);
	while(1) {
		vde_send(plug, buf, n, 0);
		(*count)++;
		if (interval)
			usleep(interval);
	}
}


