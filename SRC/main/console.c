#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>

#include <console.h>
#include <daydream.h>
#include <ddcommon.h>

static int conin = 0;
static int conout = 0;
static int conon = 0;
static int dummyfd = 0;

int console_active(void)
{
	return conon != 0;
}

int init_console(void)
{
	char buffer[4096];
	struct sigaction sigact;
	sigset_t sigset;

	/* Writing to pipe with no readers on the other side raises
	 * SIGPIPE and its default action is to terminate the program.
	 */
	sigact.sa_handler = SIG_IGN;
	sigemptyset(&sigset);
	sigact.sa_mask = sigset;
	sigact.sa_flags = SA_RESTART;
	if (sigaction(SIGPIPE, &sigact, NULL) == -1)
		abort();

	if (lmode == 1 || fnode) {
		conin = STDIN_FILENO;
		conout = STDOUT_FILENO;
		conon = 2;
		return 0;
	} 
	
	snprintf(buffer, sizeof buffer, "%s/daydream%dw", DDTMP, node);
	unlink(buffer);
	if (mkfifo(buffer, 0777) == -1) {
		syslog(LOG_ERR, "cannot mkfifo(\"%.200s\"): %m", buffer); 
		fputs("Cannot create communication FIFO\r\n", stderr);
		exit(1);
	}
	/* We must first open the writing FIFO as O_RDONLY, if we're
	 * ever going to get it opened as O_WRONLY. Without this trick  
	 * open() would always return ENXIO.
	 *
	 * We can still know whether there are no readers on the other 
	 * side of pipe, since safe_write() will return EPIPE on such case.
	 */
	if ((dummyfd = open(buffer, O_RDONLY | O_NONBLOCK)) == -1 ||
		(conout = open(buffer, O_WRONLY | O_NONBLOCK)) == -1)
		abort();
	close(dummyfd);
	set_blocking_mode(conout, 0);
	
	snprintf(buffer, sizeof buffer, "%s/daydream%dr", DDTMP, node);
	unlink(buffer);
	if (mkfifo(buffer, 0777) == -1) {
		syslog(LOG_ERR, "cannot mkfifo(\"%.200s\"): %m", buffer);
		fputs("Cannot create communication FIFO\r\n", stderr);
		exit(1);
	}
	/* Opening a FIFO for O_RDONLY in non blocking mode should
	 * work on all systems.
	 */
	if ((conin = open(buffer, O_RDONLY | O_NONBLOCK)) == -1 ||
		(dummyfd = open(buffer, O_WRONLY | O_NONBLOCK)) == -1)
		abort();

	set_blocking_mode(conin, 0);
		
	return 0;	
}

void finalize_console(void)
{
	close(conin);
	close(conout);
	close(dummyfd);
	conon = 0;
}

void open_console(void)
{
	if (conon != 2)
		conon = 1;
}
	
void close_console(void)
{
	if (conon != 2)
		conon = 0;
}

int console_select_input(int maxfd, fd_set *set)
{
	FD_SET(conin, set);
	return maxfd < conin ? conin : maxfd;
}

int console_pending_input(fd_set *set)
{
	return FD_ISSET(conin, set);
}

int console_getc(void)
{
	unsigned char ch;

	switch (read(conin, &ch, 1)) {
	case 0:
		return EOF;
	case -1:
		if (errno == EPIPE) {
			conon = 0;
			return EOF;
		}
		return -1;
	default:
		return ch;
	}
}

int console_putsn(void *str, size_t n)
{
	int writecnt;
	
	if ((writecnt = safe_write(conout, str, n)) == -1 && errno == EPIPE)
		return 0;
	else
		return writecnt;
}
