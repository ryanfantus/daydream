#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <inttypes.h>

#include <daydream.h>
#include <ddcommon.h>
#include <console.h>

static volatile int child_initialized;

static void childhan(int);
static void readhan(int);
static int getpty(char *, size_t) __attr_bounded__ (__string__, 1, 2);

void stdioout(const char *s)
{
	char fname[15] = "";
	int fd;

	strlcpy(fname, "/tmp/dd.XXXXXX", sizeof fname);
	if ((fd = mkstemp(fname)) == -1) {
		fprintf(stderr, "%s: %s\n", fname, strerror(errno));
		return;
	}
	if (runstdio(s, fd, 3)) 
		TypeFile(fname, TYPE_NOCODES);
	close(fd);
	unlink(fname);
}

/* Splits the command line at whitespaces for execv(). */
static void chop_args(char *cmd, char **args)
{
	int quoteon, i = 1;
	char *s;
	
	s = cmd;
	args[0] = s;
	quoteon = 0;

	while (*s) {
		if (quoteon) {
			if (*s == '\"') {
				quoteon = 0;
				*s = 0;

			}
			s++;
		} else if (*s == '\"') {
			args[i - 1] = s + 1;
			s++;
			quoteon = 1;
		} else if (*s == ' ') {
			*s = 0;
			s++;
			args[i] = s;
			i++;
		} else
			s++;
	}
	args[i] = 0;
}

#define max(_a, _b) ((_a) > (_b) ? (_a) : (_b))

#define PTY_MODE_STDIO	1
#define PTY_MODE_PIPE	2
#define PTY_MODE_SMODEM 4

static void do_comm(int pipefd[2], int ptyfd, int mode, int outfd, pid_t kid,
	time_t *hangup_time)
{
	int highfd;
	struct timeval tv;
	fd_set rset;
	int saved_ansi;
	int saved_userinput;
	unsigned char ch;
	ssize_t bread;
	char buf[BUFSIZ];

	FD_ZERO(&rset);
	highfd = 0;
	/* Serial line should not be read if it is handled by
	 * programs like ZModem (PTY_MODE_PIPE) or SModem.
	 */
	if (!lmode && mode == 1) {
		FD_SET(serhandle, &rset);
		highfd = max(highfd, serhandle);
	}

	if (console_active()) 
	  highfd = console_select_input(highfd, &rset);

	if (mode == PTY_MODE_PIPE) {
		FD_SET(pipefd[0], &rset);
		highfd = max(highfd, pipefd[0]);
	} else {
		FD_SET(ptyfd, &rset);
		highfd = max(highfd, ptyfd);
	}
		
	tv.tv_sec = 0;
	tv.tv_usec = 50e3;

	if (select(highfd + 1, &rset, NULL, NULL, &tv) == -1) {
		if (errno != EINTR && hangup_time) {
			/* soft termination anyway */
			*hangup_time = time(NULL);
			kill(kid, SIGHUP);
		}
		return;
	}

	if (!checkcarrier() && hangup_time) {
		/* soft termination anyway */
		*hangup_time = time(NULL);
		kill(kid, SIGHUP);
	}
	
	if (!lmode && mode == 1 && FD_ISSET(serhandle, &rset)) {
		read(serhandle, &ch, 1);
		safe_write(ptyfd, &ch, 1);
		keysrc = 1;
	}
	
	if (mode == PTY_MODE_PIPE && FD_ISSET(pipefd[0], &rset)) {
		if ((bread = read(pipefd[0], buf, sizeof buf)) == -1) {
			if (errno != EINTR && hangup_time) {
				/* probably the child terminated */
				*hangup_time = time(NULL);
				kill(kid, SIGHUP);
			}
		} else if (bread > 0 && console_active()) 
			console_putsn(buf, bread);
	}

	if (console_active() && console_pending_input(&rset)) {
		ch = console_getc();
		if (mode != 2 && mode != 3)
			safe_write(ptyfd, &ch, 1);
		keysrc = 2;
		if (ch == 2) 
			handlectrl(0);
	}

	if (mode != 2 && FD_ISSET(ptyfd, &rset)) {
		if ((bread = read(ptyfd, &buf, sizeof buf)) == -1) {
			if (errno != EINTR && hangup_time) {
				/* probably the child terminated */
				*hangup_time = time(NULL);
				kill(kid, SIGHUP);
			}
		} else if (bread > 0) {
			if (mode == 4) {
				saved_ansi = ansi;
				saved_userinput = userinput;
				ansi = 1;
				userinput = 0;

				ddput(buf, bread);

				ansi = saved_ansi;
				userinput = saved_userinput;
			} else if (mode != 3)
				ddput(buf, bread);

			/* if mode == 3, dup2(outfd, child_std{out,err}) */
			if (mode != 3 && outfd != -1) 
				safe_write(outfd, &buf, bread);
		}
	}
}

static int master_loop(int pipefd[2], int ptyfd, int mode, int outfd, pid_t kid)
{
	time_t hangup_time = 0;
	struct sigaction saved_sigchld, sigchld;
	sigset_t saved_mask, mask;
	int sigterm_raised = 0;
	int status;
	int rval;

	sigchld.sa_handler = childhan;
	sigemptyset(&sigchld.sa_mask);
	sigchld.sa_flags = 0;
	if (sigaction(SIGCHLD, &sigchld, &saved_sigchld))
		return -1;
	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
	if (sigprocmask(SIG_UNBLOCK, &mask, &saved_mask)) {
		sigaction(SIGCHLD, &saved_sigchld, NULL);
		return -1;	
	}
	
	if (mode == PTY_MODE_PIPE)
		close(pipefd[1]);

	/* wait until child finishes PTY setup, but fall through if 
	 * the child ceases to exist, we'll deal with its zombie at
	 * the beginning of the loop.
	 */
	while (!child_initialized && waitpid(kid, NULL, WNOHANG | WUNTRACED))
		usleep(50000);

	for (;;) {
		do_comm(pipefd, ptyfd, mode, outfd, kid,
			hangup_time ? NULL : &hangup_time);

		/* we don't need no stopped children */
		rval = waitpid(kid, &status, WNOHANG | WUNTRACED);
		if (rval > 0 && WIFSTOPPED(status)) {
			kill(kid, SIGKILL);
			continue;
		}
		if (rval)	
			break;

		/* allow child to terminate properly in 20 seconds */
		if (hangup_time) {
			/* stay awhile here not to hog the cpu */
				usleep(50000);
			if (!sigterm_raised && hangup_time < time(NULL) - 10) {
				sigterm_raised = 1;
				kill(kid, SIGTERM);
				/* no continue here, try to read more stuff */
			}
			if (hangup_time < time(NULL) - 20) {
				/* use force */	
				kill(kid, SIGKILL);
				continue;
			}
		}
	}
	
	/* reap zombies */
	waitpid(-1, 0, WNOHANG);
	sigprocmask(SIG_SETMASK, &saved_mask, NULL);
	sigaction(SIGCHLD, &saved_sigchld, NULL);
	return 0;
}

int runstdio(const char *command, int outfd, int inp)
{
	int ptyfd = 0;
	int pipefd[2];
	char dcmd[400];
	char *args[100];
	char ptyname[20];
	pid_t kid;

	genstdiocmdline(dcmd, command, NULL, NULL);

	chop_args(dcmd, args);
	
	if (inp != 2)
		if ((ptyfd = getpty(ptyname, sizeof ptyname)) < 0) {
			DDPut("*AAAARGH* fatal error: Tell Sysop to make more accessable pty's!\n");
			return 0;
		}
	child_initialized = 0;

	if (inp == 2) {
		pipe(pipefd);
	} else {

		struct winsize ws;

		ws.ws_col = 80;
		if (inp == 4) {
			ws.ws_row = maincfg.CFG_LOCALSCREEN;
		} else {
			ws.ws_row = user.user_screenlength;
		}
		ws.ws_xpixel = ws.ws_ypixel = 0;
		ioctl(ptyfd, TIOCSWINSZ, &ws);

	}
	signal(SIGUSR2, readhan);
	kid = fork();

	switch (kid) {
	case 0:
		if (inp != 2) {
			close(ptyfd);
			/* setsid() cannot fail, we're not session leader */
			setsid();
			
			if ((ptyfd = open(ptyname, O_RDWR | O_NONBLOCK)) == -1)
				_exit(1);
			
			set_blocking_mode(ptyfd, 0);
		}

		if (inp == 2) {
			fcntl(serhandle, F_SETFD, 0);
			dup2(pipefd[1], 2);
			setbuf(stderr, 0);
			close(pipefd[0]);
		} else if (inp == 3 && outfd != -1) {
			dup2(ptyfd, STDIN_FILENO);
			dup2(outfd, STDOUT_FILENO);
			dup2(outfd, STDERR_FILENO);
		} else {
			dup2(ptyfd, STDIN_FILENO);
			dup2(ptyfd, STDOUT_FILENO);
			dup2(ptyfd, STDERR_FILENO);
		}

#ifdef HAVE_TIOCSCTTY
		if (inp != 2 && ioctl(ptyfd, TIOCSCTTY, 0) == -1) 
			syslog(LOG_WARNING, "TIOCSCTTY failed for PTY");
#endif

		kill(getppid(), SIGUSR2);
		execv(args[0], &args[0]);		
		syslog(LOG_ERR, "cannot execute \"%.200s\": %s", args[0],
			strerror(errno));
		DDPut("Error running program.\r\n");
		kill(getpid(), SIGKILL);
		_exit(1);
		break;
	case -1:
		syslog(LOG_ERR, "fork() failed: %s", strerror(errno));
		DDPut("Error running program.\r\n");
		break;
	default:
		master_loop(pipefd, ptyfd, inp, outfd, kid);
		break;
	}
	if (inp != 2) 
		close(ptyfd);
	else
		close(pipefd[0]);

	return 1;

}

static void childhan(int signum)
{
}

static void readhan(int signum)
{
	child_initialized = 1;
}

#ifdef HAVE_PTY_UTILS
static int getpty_unix98(char *name, size_t namelen)
{
	int pty;

	pty = getpt();
	if (pty < 0)
		return -1;

	if (grantpt(pty) < 0 || unlockpt(pty) < 0)
		goto close_pty;

	if (ptsname_r(pty, name, namelen) <0)
		goto close_pty;

	return pty;

close_pty:
	close(pty);
	return -1;
}
#endif 
static int getpty(char *name, size_t namelen)
{
	int pty, tty;
	char *pty_dev = "/dev/ptc", *tt;

#ifdef HAVE_PTY_UTILS
	/* first try to use Unix98 allocation */
	 if ((pty = getpty_unix98(name, namelen)) != -1) {
		set_blocking_mode(pty, 0);
		return pty;
	}
#endif 
	/* look for a SYSV-type pseudo device */
	if ((pty = open(pty_dev, O_RDWR | O_NONBLOCK)) >= 0) {
		if ((tt = ttyname(pty)) != NULL) {
			strlcpy(name, tt, namelen);
			set_blocking_mode(pty, 0);
			return pty;
		}
		close(pty);
	}

	/* scan Berkeley-style */
	strlcpy(name, "/dev/ptyp0", namelen);
	while (access(name, 0) == 0) {
		if ((pty = open(name, O_RDWR | O_NONBLOCK)) >= 0) {
			name[5] = 't';
			if ((tty = open(name, O_RDWR | O_NONBLOCK)) >= 0) {
				close(tty);
				set_blocking_mode(pty, 0);
				return pty;
			}
			name[5] = 'p';
			close(pty);
		}

		/* get next pty name */
		if (name[9] == 'f') {
			name[8]++;
			name[9] = '0';
		} else if (name[9] == '9')
			name[9] = 'a';
		else
			name[9]++;
	}
	return -1;
}

