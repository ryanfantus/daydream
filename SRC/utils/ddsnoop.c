#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <grp.h>
#include <utmp.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#include <dd.h>

#define max(__a__, __b__) (((__a__) > (__b__)) ? (__a__) : (__b__))

static int outfd;
static struct termios saved_tty;

static int raw_tty(struct termios *);
static void restore_tty(void);
static void closeout(void);
static void sighandler(int);

int main(int argc, char *argv[])
{
	fd_set readset;
	int infd, fdmax, quit = 0, n;
	struct winsize ws;
	char buf[BUFSIZ];
	char sockname[128];

	if (argc < 2) {
		fputs("Usage: ddsnoop <node>\n", stderr);
		exit(1);
	}
	sprintf(sockname, "%s/daydream%sr", DDTMP, argv[1]);
	if ((outfd = open(sockname, O_WRONLY | O_NONBLOCK)) == -1) {
		fputs("Node not running!\n", stderr);
		exit(1);
	}

	sprintf(sockname, "%s/daydream%sw", DDTMP, argv[1]);
	if ((infd = open(sockname, O_RDONLY | O_NONBLOCK)) == -1) {
		fputs("Node not running!\n", stderr);
		exit(1);
	}

	puts("DDSnoop coded by Antti Häyrynen. ctrl-b-h for help!\n");

	if (raw_tty(&saved_tty) == -1) {
		fputs("cannot initialize terminal\n", stderr);
		exit(1);
	}
	atexit(restore_tty);

	ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);

	buf[0] = 2;
	buf[1] = 1;
	buf[2] = ws.ws_row;

	write(outfd, buf, 3);

	atexit(closeout);
	signal(SIGINT, sighandler);
	signal(SIGHUP, sighandler);
	signal(SIGTERM, sighandler);
	fdmax = max(STDIN_FILENO, infd);

	while (!quit) {
		FD_ZERO(&readset);
		FD_SET(STDIN_FILENO, &readset);
		FD_SET(infd, &readset);

		if (select(fdmax + 1, &readset, NULL, NULL, NULL) == -1) {
			if (errno == EINTR)
				continue;
			break;
		}

		if (FD_ISSET(STDIN_FILENO, &readset)) {
			read(STDIN_FILENO, buf, 1);

			if (buf[0] == 3) 
				exit(0);
			else {
				if (write(outfd, buf, 1) < 0)
					quit = 1;
			}
		}

		if (FD_ISSET(infd, &readset)) {
			if ((n = read(infd, buf, sizeof buf)) < 1)
				quit = 1;

			if (n > 0)
				write(STDOUT_FILENO, buf, n);
		}
	}

	return 0;
}

static void closeout(void)
{
	char buf[2];

	buf[0] = 2;
	buf[1] = 4;
	write(outfd, buf, 2);
}

static void sighandler(int signum)
{
	closeout();
	printf("\r\nBack at local tty.\r\n");
	exit(0);
}

static int raw_tty(struct termios *saved_tty)
{
	struct termios tty;

	if (tcgetattr(STDIN_FILENO, &tty) < 0)
		return -1;
	memcpy(saved_tty, &tty, sizeof(struct termios));

	tty.c_lflag &= ~(ICANON | IEXTEN | ISIG | ECHO);
	tty.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON | BRKINT);
	tty.c_oflag &= ~OPOST;
	tty.c_cflag |= CS8;

	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty);
	return 0;
}

static void restore_tty(void)
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_tty);
}
