#include <stdarg.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <pwd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <syslog.h>

#include <ddcommon.h>
#include <daydream.h>
#include <symtab.h>

struct dayfilestat {
	uint64_t tbytes;
	int tfiles;
	uint64_t ybytes;
	int yfiles;
	uint64_t tgbytes;
	int tgfiles;
	uint64_t ygbytes;
	int ygfiles;
};

static char initstring[512];
static char ringstring[512];
static char okstring[512];
static char connectstring[512];
static char answerstring[512];
char hangupstring[512];
int hupmode = 0;
static char lockfile[512];

static int checklock(void);
static void getfstats(struct dayfilestat *, int);
static int initmodem(void);
static int listlast(void);
static int lockserial(void);
static int readmodem(char *, int, int);
static void wqcb(int);

#define LOCKPATH "/var/spool/uucp/"

static struct speedtab {
#ifdef POSIX_TERMIOS
	speed_t cbaud;
#else
	unsigned short cbaud;	/* baud rate, e.g. B9600 */
#endif
	int nspeed;		/* speed in numeric format */
	const char *speed;	/* speed in display format */
} speedtab[] = {

	{
		B50, 50, "50"
	},
	{
		B75, 75, "75"
	},
	{
		B110, 110, "110"
	},
	{
		B134, 134, "134"
	},
	{
		B150, 150, "150"
	},
	{
		B200, 200, "200"
	},
	{
		B300, 300, "300"
	},
	{
		B600, 600, "600"
	},
#ifdef  B900
	{
		B900, 900, "900"
	},
#endif
	{
		B1200, 1200, "1200"
	},
	{
		B1800, 1800, "1800"
	},
	{
		B2400, 2400, "2400"
	},
#ifdef  B3600
	{
		B3600, 3600, "3600"
	},
#endif
	{
		B4800, 4800, "4800"
	},
#ifdef  B7200
	{
		B7200, 7200, "7200"
	},
#endif
	{
		B9600, 9600, "9600"
	},
#ifdef  B14400
	{
		B14400, 14400, "14400"
	},
#endif
#ifdef  B19200
	{
		B19200, 19200, "19200"
	},
#endif				/* B19200 */
#ifdef  B28800
	{
		B28800, 28800, "28800"
	},
#endif
#ifdef  B38400
	{
		B38400, 38400, "38400"
	},
#endif				/* B38400 */
#ifdef  EXTA
	{
		EXTA, 19200, "EXTA"
	},
#endif
#ifdef  EXTB
	{
		EXTB, 38400, "EXTB"
	},
#endif
#ifdef  B57600
	{
		B57600, 57600, "57600"
	},
#endif
#ifdef  B115200
	{
		B115200, 115200, "115200"
	},
#endif
#ifdef B230400
	{
		B230400, 230400, "230400"
	},
#endif
#ifdef B460800
	{
		B460800, 460800, "460800"
	},
#endif
	{
		0, 0, ""
	}
};

static int wquit = 0;

static void wqcb(int sig)
{
	wquit = 1;
}

void log_error(const char *filename, int line, const char *fmt, ...)
{
	va_list args;
	char msg[1024], msg2[1024];
	va_start(args, fmt);
	vsnprintf(msg2, sizeof msg, fmt, args);
	va_end(args);
	/* do not remove "%s" or you will cause a security hazard */
	snprintf(msg, sizeof msg, "file %s, line %d: %s", 
		filename, line, msg2);
	syslog(LOG_ERR, "%s", msg);
	fputs(msg, stderr);
	fputs("\r\n", stderr);
}

int waitforcall(int mode)
{
	FILE *modfd;
	char wbuf[1024];
	int succbits = 0;
	int i;
	struct winsize ws;
	userinput = 0;
	snprintf(wbuf, sizeof wbuf, "configs/modem%d.cfg", fnode);

	if ((modfd = fopen(wbuf, "r")) == NULL) {
		log_error(__FILE__, __LINE__,
			"cannot open configuration file \"%s\": %s (%d)", 
			wbuf, strerror(errno), errno);
		return -1;
	}
	ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);

	maincfg.CFG_LOCALSCREEN = ws.ws_row;

	/* Exit on these signals. */
	signal(SIGTERM, wqcb);
	signal(SIGINT, wqcb);

	if (!mode) {
		struct termios tty;

		if (!checklock())
			return -1;

		/* this may get stuck without O_NONBLOCK */
		serhandle = open(currnode->MULTI_TTYNAME, O_RDWR | O_NONBLOCK);
		if (serhandle == -1) {
			log_error(__FILE__, __LINE__, 
				"cannot open serial: %s (%d)",
				 strerror(errno), errno);
			return -1;
		}
		
		/* get rid of O_NONBLOCK */
		if (set_blocking_mode(serhandle, 0) == -1) {
			log_error(__FILE__, __LINE__,
				"cannot clear non-blocking mode: %s (%d)",
				strerror(errno), errno);
			return -1;
		}
		
		if (tcgetattr(serhandle, &tty) == -1) {
			log_error(__FILE__, __LINE__,
				"cannot get terminal attributes: %s (%d)",
				strerror(errno), errno);
			return -1;
		}
		
		tty.c_iflag &= ~(IGNBRK | IGNCR | INLCR | ICRNL |
				 IXANY | IXON | IXOFF | INPCK | ISTRIP);
#ifdef HAVE_IUCLC
		tty.c_iflag &= ~IUCLC;
#endif
		tty.c_iflag |= (BRKINT | IGNPAR);
		tty.c_oflag &= ~OPOST;
		tty.c_lflag &= ~(ECHONL | NOFLSH);
#ifdef HAVE_XCASE
		tty.c_lflag &= ~XCASE;
#endif
		tty.c_lflag &= ~(ICANON | ISIG | ECHO);
		tty.c_cflag |= CREAD | CRTSCTS;
		tty.c_cflag &= ~(HUPCL);
		tty.c_cc[VTIME] = 5;
		tty.c_cc[VMIN] = 1;

		for (i = 0; speedtab[i].cbaud != 0; i++) {
			if (speedtab[i].nspeed == currnode->MULTI_TTYSPEED) {
				cfsetispeed(&tty, speedtab[i].cbaud);
				cfsetospeed(&tty, speedtab[i].cbaud);
			}
		}

		tcsetattr(serhandle, TCSANOW, &tty);
		if (!forcebps)
			lockserial();
	}
	while (fgetsnolf(wbuf, 1024, modfd)) {
		if (!strncasecmp(wbuf, "INIT ", 5)) {
			strlcpy(initstring, &wbuf[5], sizeof initstring);
			succbits |= (1L << 0);
		} else if (!strncasecmp(wbuf, "OK ", 3)) {
			strlcpy(okstring, &wbuf[3], sizeof okstring);
			succbits |= (1L << 1);
		} else if (!strncasecmp(wbuf, "RING ", 5)) {
			strlcpy(ringstring, &wbuf[5], sizeof ringstring);
			succbits |= (1L << 2);
		} else if (!strncasecmp(wbuf, "CONNECT ", 8)) {
			strlcpy(connectstring, &wbuf[8], sizeof connectstring);
			succbits |= (1L << 3);
		} else if (!strncasecmp(wbuf, "ANSWER ", 7)) {
			strlcpy(answerstring, &wbuf[7], sizeof answerstring);
			succbits |= (1L << 4);
		} else if (!strncasecmp(wbuf, "HANGUP ", 7)) {
			strlcpy(hangupstring, &wbuf[7], sizeof hangupstring);
			hupmode = 1;
		}
	}
	fclose(modfd);
	if (forcebps) {
		bpsrate = forcebps;
		userinput = 1;
		return 0;
	}
      ans:

	snprintf(wbuf, sizeof wbuf,
		"DayDream BBS %s written by Antti Häyrynen", versionstring);
	ansi = 1;
	ddprintf("\033[2J\033[H\033[0;44;36m%-78s\033[0m\n\n", wbuf);
	DDPut("              \033[0;44;36m L \033[0m - \033[35mLocal logon        \033[44;36m U \033[0m - \033[35mUser Editor \n");
	DDPut("              \033[0;44;36m S \033[0m - \033[35mSysOp logon        \033[44;36m I \033[0m - \033[35mRe-initialize modem\n");
	DDPut("              \033[0;44;36m A \033[0m - \033[35mImmediate answer   \033[44;36m Q \033[0m - \033[35mQuit\n\n");

	DDPut("                  \033[0;44;36m Control-B + H \033[0m - \033[35mhelp while user online\n\n");

	ddprintf("\033[0;44;36m%-78s\033[0m\n", "  Bps Name                Organization            Logon    On Upkb:Dnkb Flags ");
	listlast();

	changenodestatus("Waiting for a call...");

	if (succbits & (1L << 0)) {
		if (initmodem() == -1)
			return -1;
	}
	while ((i = readmodem(wbuf, sizeof wbuf, 0))) {
		char *s;

		if (i == -1) {
			unlink(lockfile);
			DDPut("Exiting..\n\033[0m");
			return -1;
		} else if (i == -2) {
			carrier = 1;
			lmode = 1;
			usered();
			lmode = 0;
			carrier = 0;
			goto ans;
		} else if (i == -3) {
			lmode = 1;  
			visitbbs(0);
			lmode = 0;
			goto ans;
		} else if (i == -4) {
			lmode++;
			visitbbs(1);
			lmode = 0;
			goto ans;
		} else if (i == -6) {
			goto ans;
		}
		if (i == -5 || strstr(wbuf, ringstring)) {
			DDPut("[36mRinging.. ");
			writetomodem(answerstring);
			DDPut("[35mAnswering.. ");
			readmodem(wbuf, sizeof wbuf, 2);
			s = strstr(wbuf, connectstring);
			if (s) {
				s += strlen(connectstring);
				while (*s == ' ')
					s++;
				bpsrate = atoi(s);
				if (bpsrate == 0)
					bpsrate = 300;
				DDPut("[0mCONNECT ");
				DDPut(s);
				sleep(2);
				break;
			}
			DDPut("[31mNO CARRIER!");
			sleep(1);
			DDPut("\r                                              \r");
		}
	}
	userinput = 1;
	return 0;
}

static int listlast(void)
{
	int listnum;
	char buf1[512];
	int logfd, offset;
	struct callerslog clg;
	struct stat st;
	struct dayfilestat dls;
	struct dayfilestat uls;

	listnum = maincfg.CFG_LOCALSCREEN - 13;

	snprintf(buf1, sizeof buf1, 
		"%s/logfiles/callerslog%d.dat", origdir, node);
	logfd = open(buf1, O_RDONLY);
	if (logfd == -1) 
		return 0;

	if (fstat(logfd, &st) == -1)
		return 0;
	if (st.st_size / sizeof(struct callerslog) < listnum)
		 listnum = st.st_size / sizeof(struct callerslog);

	offset = st.st_size;

	while (listnum--) {
		char bpsr[128];
		char upb[10];
		char dnb[10];
		
		char user_location[26];
		char user_name[26];
		
		struct userbase tmpuser;
		struct tm *tm;
		struct tm tem1;
		time_t tk;
				
		offset -= sizeof(struct callerslog);
		if (lseek(logfd, offset, SEEK_SET) == -1) 
			break;

		if (read(logfd, &clg, sizeof(struct callerslog)) !=
			sizeof(struct callerslog)) 
			break;

		if (getubentbyid(clg.cl_userid, &tmpuser) == -1) {
			strlcpy(user_location, "N/A", sizeof user_location);
			strlcpy(user_name, "N/A", sizeof user_location);
		} else {
			strlcpy(user_location, maincfg.CFG_FLAGS & (1L << 2) ?
				tmpuser.user_organization : 
				tmpuser.user_zipcity, sizeof user_location);
			strlcpy(user_name, maincfg.CFG_FLAGS & (1L << 1) ?
				tmpuser.user_handle : tmpuser.user_realname, 
				sizeof user_name);
		}

		tm = localtime(&clg.cl_logon);
		memcpy(&tem1, tm, sizeof(struct tm));
		tk = clg.cl_logoff - clg.cl_logon;
		tm = gmtime(&tk);

		if ((clg.cl_ulbytes / 1024) > 9999) {
			snprintf(upb, sizeof upb, "%3.3dM", 
				clg.cl_ulbytes / (1024 * 1024));
		} else if (clg.cl_ulbytes) {
			snprintf(upb, sizeof upb, "%4.4d", 
				clg.cl_ulbytes / 1024);
		} else {
			strlcpy(upb, "----", sizeof upb);
		}

		if ((clg.cl_dlbytes / 1024) > 9999) {
			snprintf(dnb, sizeof dnb, "%3.3dM", 
				clg.cl_dlbytes / (1024 * 1024));
		} else if (clg.cl_dlbytes) {
			snprintf(dnb, sizeof dnb, "%4.4d", 
				clg.cl_dlbytes / 1024);
		} else {
			strlcpy(dnb, "----", sizeof dnb);
		}

		snprintf(&bpsr[100], sizeof bpsr - 100, "%-5d", clg.cl_bpsrate);
		if (strlen(&bpsr[100]) > 5) {
			bpsr[0] = bpsr[100];
			bpsr[1] = bpsr[101];
			bpsr[2] = bpsr[102];
			bpsr[3] = 'k';
			bpsr[4] = bpsr[103];
			bpsr[5] = 0;
		} else
			strlcpy(bpsr, &bpsr[100], sizeof bpsr);

		ddprintf("%s %-19.19s %-23.23s %2.2d:%2.2d %2.2d:%2.2d %s:%s %c%c%c%c%c%c\n",
			bpsr, user_name, user_location,
			tem1.tm_hour, tem1.tm_min, tm->tm_hour, tm->tm_min,
			upb, dnb,
			clg.cl_ulbytes ? 'U' : '-',
			clg.cl_dlbytes ? 'D' : '-',
			(clg.cl_flags & CL_CARRIERLOST) ? 'C' : '-',
			(clg.cl_flags & CL_NEWUSER) ? 'N' : '-',
			(clg.cl_flags & CL_PAGEDSYSOP) ? 'P' : '-',
			(clg.cl_flags & CL_PASSWDFAIL) ? 'H' : '-');
	}	
	
	close(logfd);
	getfstats(&uls, 1);
	getfstats(&dls, 2);
	ddprintf("\033[44;36mT: ULs [ %4.4d/%4.4d ] ULMB [ %4.4Lu/%4.4Lu ]   DLs [ %4.4d/%4.4d ] DLMB [ %4.4Lu/%4.4Lu ]\n",
		 uls.tfiles, uls.tgfiles, uls.tbytes / (1024 * 1024),
		 uls.tgbytes / (1024 * 1024), dls.tfiles, dls.tgfiles,
		 dls.tbytes / (1024 * 1024), dls.tgbytes / (1024 * 1024));
	ddprintf("Y: ULs [ %4.4d/%4.4d ] ULMB [ %4.4Lu/%4.4Lu ]   DLs [ %4.4d/%4.4d ] DLMB [ %4.4Lu/%4.4Lu ]\n\033[0m",
		 uls.yfiles, uls.ygfiles, uls.ybytes / (1024 * 1024),
		 uls.ygbytes / (1024 * 1024), dls.yfiles, dls.ygfiles,
		 dls.ybytes / (1024 * 1024), dls.ygbytes / (1024 * 1024));

	return 1;
}

static int initmodem(void)
{
	int i;
	char buf[1024];

	for (i = 1; i < 6; i++) {
		ddprintf("\r\033[0mInitializing modem - try %d - ", i);
		if (tcflush(serhandle, TCIFLUSH) == -1) {
			log_error(__FILE__, __LINE__,
				"cannot flush modem: %s (%d)",
				strerror(errno), errno);
			return -1;
		} 
		if (writetomodem(initstring) == -1)
			return -1;
		if (readmodem(buf, sizeof buf, 1) && strstr(buf, okstring)) {
			DDPut("\033[36mOK!\n");
			return 0;
		}
		sleep(1);
	}
	DDPut("[31mERROR!\n[0m");
	return -1;

}

static int readmodem(char *s, int length, int mode)
{
	fd_set readset;
	struct timeval tv;
	int havestuff = 0;
	int tcount = 0;
	char *t = s;
	char *u;
	char k;
	*s = 0;

	for (;;) {
		FD_ZERO(&readset);
		FD_SET(serhandle, &readset);
		if (!mode)
			FD_SET(STDIN_FILENO, &readset);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		select(serhandle + 1, &readset, 0, 0, &tv);
		if (wquit)
			return -1;

		if (FD_ISSET(serhandle, &readset)) {
			if (havestuff < length - 1) {
				read(serhandle, s, 1);
				*++s = 0;
				havestuff++;
			}
		} else {
			if (mode == 2) {
				if (tcount > 30)
					return 0;
				u = strstr(t, connectstring);
				if (u) {
					while (*u++)
						if (*u == '\r') 
							return havestuff;
				}
			} else if (!mode && FD_ISSET(STDIN_FILENO, &readset)) {
				read(STDIN_FILENO, &k, 1);
				switch (toupper(k)) {
				case 'Q':
					return -1;
				case 'U':
					return -2;
				case 'L':
					return -3;
				case 'S':
					return -4;
				case 'A':
					return -5;
				case 'I':
					return -6;
				}
			} else if (havestuff || mode == 1) {
				return havestuff;
			} else if (mode == 0 && tcount > 600) {
				DDPut("\033[A                                                               ");
				if (initmodem() == -1)
					return -1; 
				tcount = 0;
			}
			tcount++;
		}
	}
}

int writetomodem(const char *s)
{
	char ch;

	for (; *s; s++) {
		if (*s == '~')
			usleep(500000);
		else {
			if (*s == '\\') {
				switch (*++s) {
				case 'r':	
					ch = '\r';
					break;
				case 'n':
					ch = '\n';
					break;
				default:
					ch = *s;
					break;
				}
			} else
				ch = *s;
			if (safe_write(serhandle, &ch, 1) != 1) {
				log_error(__FILE__, __LINE__, 
					"cannot write to modem: %s (%d)",
					strerror(errno), errno);
				return -1;
			}
		}
	}
	return 0;
}

static int lockserial(void)
{
	int mask, fd;
	struct passwd *pwd;
	char buf[512];

	mask = umask(S_IRWXG | S_IRWXO);

	if ((pwd = getpwuid(getuid())) == (struct passwd *) 0) {
		fprintf(stderr, "You don't exist. Go away.\n");
		return 0;
	}
	if ((fd = open(lockfile, O_WRONLY | O_CREAT | O_EXCL, 0666)) < 0) {
		return 0;
	}
	(void) umask(mask);
	snprintf(buf, sizeof buf, 
		"%10ld daydream %.20s\n", (long) getpid(), pwd->pw_name);
	safe_write(fd, buf, strlen(buf));
	close(fd);
	return 1;
}

static int checklock(void)
{
	char buf[128];
	int fd;
	pid_t pid;
	int n;

	snprintf(lockfile, sizeof lockfile, "%sLCK..%s", LOCKPATH, filepart(currnode->MULTI_TTYNAME));
	fd = open(lockfile, O_RDONLY);
	if (fd > -1) {
		n = read(fd, buf, 127);
		close(fd);
		if (n > 0) {
			pid = -1;
			if (n == 4)
				/* Kermit-style lockfile. */
				pid = *(int *) buf;
			else {
				/* Ascii lockfile. */
				buf[n] = 0;
				sscanf(buf, "%d", &pid);
			}
			if (pid > 0 && kill((pid_t) pid, 0) < 0 &&
			    errno == ESRCH) {
				sleep(1);
				unlink(lockfile);
			} else
				n = 0;
		}
		if (n == 0) {
			fprintf(stderr, "Device %s is locked.\r\n", currnode->MULTI_TTYNAME);
			return 0;
		}
	}
	return 1;
}

static void getfstats(struct dayfilestat *dfs, int mode)
{
	char buf[1024];
	int fd;
	int lentries;
	struct stat st;
	struct DD_UploadLog lok;

	int day = time(0) / (60 * 60 * 24);

	memset(dfs, 0, sizeof(struct dayfilestat));

	if (mode == 1) {
		snprintf(buf, sizeof buf, 
			"%s/logfiles/uploadlog.dat", origdir);
	} else {
		snprintf(buf, sizeof buf,
			"%s/logfiles/downloadlog.dat", origdir);
	}
	fd = open(buf, O_RDONLY);
	if (fd < 0)
		return;
	fstat(fd, &st);
	lentries = st.st_size / sizeof(struct DD_UploadLog);
	dd_lseek(fd, -sizeof(struct DD_UploadLog), SEEK_END);
	while (lentries--) {
		read(fd, &lok, sizeof(struct DD_UploadLog));
		lseek(fd, -2 * sizeof(struct DD_UploadLog), SEEK_CUR);
		if ((lok.UL_TIME / (60 * 60 * 24)) < (day - 1))
			break;
		if ((lok.UL_TIME / (60 * 60 * 24)) == day) {
			dfs->tgbytes += lok.UL_FILESIZE;
			dfs->tgfiles++;
			if (lok.UL_NODE == node) {
				dfs->tbytes += lok.UL_FILESIZE;
				dfs->tfiles++;
			}
		} else {
			dfs->ygbytes += lok.UL_FILESIZE;
			dfs->ygfiles++;
			if (lok.UL_NODE == node) {
				dfs->ybytes += lok.UL_FILESIZE;
				dfs->yfiles++;
			}
		}
	}
	close(fd);
}
