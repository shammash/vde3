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


#include <libvdeplug.h>
#include <sys/poll.h>

static inline unsigned long long
gettimeofdayms(void) {
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (unsigned long long) tv.tv_sec * 1000ULL + (unsigned long long) tv.tv_usec / 1000;
}

#define BUFSIZE 1540ULL

static unsigned long
	long t_start, t_end, counter, reordered, lost;


void print_statistics(int signo)
{
	static int test = 0;
	t_end = gettimeofdayms();
	printf("Sent %llu packets in %llu seconds, Bandwidth: %lld MB/s - out of order: %llu, lost: %llu\n", counter,
		(t_end - t_start)/1000,
		((counter*BUFSIZE)/(1000*(t_end - t_start))),
		reordered, lost);
	t_start = t_end;
	counter = 0;
	if (test++ == 5)
		exit(0);
	else
		alarm(5);
}

int main (int argc, char **argv) {

	int n, p;
	char buf[BUFSIZE];
	VDECONN *plug;
	struct pollfd pfd[1]= {
		[0]={.fd=-1}
	};
	unsigned long long *pkt, last = 0;
	if (argc != 2) {
		printf("Usage: %s sock\n", argv[0]);
		exit(1);
	}


	plug = vde_open(argv[1], "test_recv", NULL);
	if (!plug) {
		perror("vde_open");
		exit(2);
	}

	pfd[0].fd=vde_datafd(plug);
	pfd[0].events=POLLIN | POLLHUP;
	signal(SIGALRM, print_statistics);
	p = poll(pfd, 1, -1);
	if (pfd[0].revents & POLLIN) {
		alarm(5);
	} else {
		exit(0);
	}
	t_start=gettimeofdayms();

	while(1) {
		p = poll(pfd, 1, 1);
		if (pfd[0].revents & POLLHUP)
			exit(0);
		if (pfd[0].revents & POLLIN) {
			n=vde_recv(plug, buf, BUFSIZE, 0);
			pkt = (unsigned long long *)(&buf[14]);
			//printf ("Received pkt %llu\n", *pkt);
			if (!last)
				last = *pkt;
			else {
				if (*pkt > last + 1)
					lost++;
				if (*pkt < last) {
					reordered++;
					lost--;
				}
				last = *pkt;
			}
			counter++;
		}
	}
}

