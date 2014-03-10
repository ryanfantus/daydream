/*
 * Copyright (c) 1985, 1988, 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#define	FTP_NAMES
#include <ftp.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>

#include <stdarg.h>
#include <stdio.h>
#include <grp.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#ifdef SHADOW_PWD
#include <shadow.h>
#endif
#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#endif
#include <pwd.h>
#include <setjmp.h>
#include <netdb.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>

#include <global.h>
#include <md5.h>
#include <dd.h>
#include <ddftp.h>
#include <ddcommon.h>

#define MD_CTX MD5_CTX
#define MDInit MD5Init
#define MDUpdate MD5Update
#define MDFinal MD5Final

extern FILE *ftpd_popen();
extern int ftpd_pclose();
extern char cbuf[];
extern off_t restart_point;

static struct sockaddr_in ctrl_addr;
static struct sockaddr_in data_source;
struct sockaddr_in data_dest;
static struct sockaddr_in his_addr;
static struct sockaddr_in pasv_addr;

int data;
jmp_buf errcatch;
static jmp_buf urgcatch;
int logged_in;
struct userbase *pw;
int debug;
int timeout = 1800;		
int maxtimeout = 7200;		
static int restricted_data_ports = 1;
int type;
int form;
int stru;		
static int mode;
int usedefault = 1;	
int pdata = -1;	
int ulconf = 0;
int transflag;
static off_t file_size;
static off_t byte_count;
char tmpline[7];
char hostname[MAXHOSTNAMELEN];
char remotehost[MAXHOSTNAMELEN];

/*
 * Timeout intervals for retrying connections
 * to hosts that don't accept PORT cmds.  This
 * is a kludge, but given the problems with TCP...
 */
#define	SWAITMAX	90	/* wait at most 90 seconds */
#define	SWAITINT	5	/* interval between retries */

static int swaitmax = SWAITMAX;
static int swaitint = SWAITINT;

static void lostconn(int);
static void myoob(int);
static FILE *getdatasock(char *);
static FILE *dataconn(char *, off_t, char *);

#ifndef HAVE_SETPROCTITLE
static char **Argv = NULL;	/* pointer to argument vector */
static char *LastArgv = NULL;	/* end of argv */
#endif
char proctitle[BUFSIZ];		/* initial part of title */

static char *datadir;
static struct DayDream_MainConfig mcfg;
static struct DayDream_Conference *confs = 0;

static char myroot[1024];
static void end_login(void);
static char *fgetsnolf(char *, int, FILE *) __attr_bounded__ (__string__, 1, 2);
static int findfile(const char *file, char *de,
		    struct DayDream_Conference *tc);
static void _splitpath(char *file, char *path);
static int cmppasswds(char *passwd, unsigned char *thepw);
static int dupecheck(const char *name);
static int receive_data(FILE * instr, FILE * outstr);
static void send_data(FILE * instr, FILE * outstr, off_t blksize);
static int checkpath(char *pa);
static char *gunique(char *);
static int check_archive(const char *);

static int login_attempts;	/* number of failed login attempts */
static int askpasswd;		/* had user command, ask for passwd */
static struct sockaddr_in passive_ip;
static int forced_passive_ip = 0;

static struct ftpinfo minfo;
static char *infoname = 0;
static char *daydream_path = NULL;

static int getddconfig(void)
{
	char buf[1024];
	int fd;
	struct stat fib;
	unsigned char *s;

	if (ssnprintf(buf, "%s/data/daydream.dat", datadir))
		return -1;
	if ((fd = open(buf, O_RDONLY)) == -1)
		return -1;
	if (read(fd, &mcfg, sizeof(struct DayDream_MainConfig)) !=
	    sizeof(struct DayDream_MainConfig))
		return -1;
	close(fd);

	if (ssnprintf(buf, "%s/data/conferences.dat", datadir))
		return -1;
	if ((fd = open(buf, O_RDONLY)) == -1)
		return -1;
	if (fstat(fd, &fib) == -1)
		return -1;
	if (fib.st_size % sizeof(struct DayDream_Conference))
		return -1;
	confs = (struct DayDream_Conference *) malloc(fib.st_size + 2);
	if (confs == NULL)
		return -1;
	if (read(fd, confs, fib.st_size) != fib.st_size)
		return -1;
	close(fd);
	s = (unsigned char *) confs;
	s[fib.st_size] = 255;

	memset(&minfo, 0, sizeof(struct ftpinfo));
	return 0;
}

static void show_upload_insns(void)
{
	FILE *fi;
	char buf[1024];
	char *s;

	if (ssnprintf(buf, "%s/display/ftp-upload.txt", datadir))
		return;
	if ((fi = fopen(buf, "r")) == NULL)
		return;
	while (fgets(buf, 1024, fi)) {
		if ((s = strchr(buf, '\n')) != NULL)
			*s = '\0';
		lreply(250, buf);
	}
	lreply(250, "");
	fclose(fi);
}

static int doinitgroups(uid_t uid, gid_t gid)
{
	struct passwd *pwd;

	if ((pwd = getpwuid(uid)) == NULL)
		return -1;
	return initgroups(pwd->pw_name, gid);
}

static int check_daydream_path(void)
{
	struct stat st;

	if (lstat(daydream_path, &st) || !S_ISREG(st.st_mode)) 
		return -1;
	if (!strchr(daydream_path, '/'))
		return -1;

	return 0;
}

static void process_options(int argc, char *argv[])
{
	char opt;
	
	for (;;) {
		switch (opt = getopt(argc, argv, "vdUt:T:D:P:p:")) {
		case -1:
			return;
		case '?':
			/* no interactive help */
			syslog(LOG_ERR, "invalid parameter -%c", opt);
			exit(1);
		case 'v':
			debug = 1;
			break;
		case 'd':
			debug = 1;
			break;
		case 'U':
			restricted_data_ports = 0;
			break;
		case 't':
			timeout = atoi(optarg);
			if (maxtimeout < timeout)
				maxtimeout = timeout;
			break;
		case 'T':
			maxtimeout = atoi(optarg);
			if (timeout > maxtimeout)
				timeout = maxtimeout;
			break;
		case 'D':
			if (!(datadir = strdup(optarg))) {
				syslog(LOG_ERR, "%m");
				exit(1);
			}
			if (getddconfig() == -1) {
				syslog(LOG_ERR, "cannot load config\n");
				exit(1);
			}
			break;
		case 'P':
			if (inet_pton(AF_INET, optarg, &passive_ip.sin_addr) <= 0) {
				syslog(LOG_ERR, "invalid forced passive IP %.200s", optarg);
				exit(1);
			}
			forced_passive_ip = 1;
			break;
		case 'p':
			if (!(daydream_path = strdup(optarg))) {
				syslog(LOG_ERR, "%m");
				exit(1);
			}
			break;
		}
	}
}

int main(int argc, char *argv[], char *envp[])
{
	int addrlen, on = 1, tos;
	/*
	 * LOG_NDELAY sets up the logging connection immediately,
	 * necessary for anonymous ftp's that chroot and can't do it later.
	 */
	openlog("ddftpd", LOG_PID | LOG_NDELAY, LOG_LOCAL2);
	addrlen = sizeof his_addr;
	if (getpeername(0, (struct sockaddr *) &his_addr, &addrlen) < 0) {
		syslog(LOG_ERR, "getpeername (%s): %m", argv[0]);
		exit(1);
	}
	addrlen = sizeof ctrl_addr;
	if (getsockname(0, (struct sockaddr *) &ctrl_addr, &addrlen) < 0) {
		syslog(LOG_ERR, "getsockname (%s): %m", argv[0]);
		exit(1);
	}
#ifdef IP_TOS
	tos = IPTOS_LOWDELAY;
	if (setsockopt(0, IPPROTO_IP, IP_TOS, (char *) &tos, sizeof(int)) <
	    0)
		syslog(LOG_WARNING, "setsockopt (IP_TOS): %m");
#endif
	data_source.sin_port = htons(ntohs(ctrl_addr.sin_port) - 1);
	debug = 0;
#ifndef HAVE_SETPROCTITLE
	/*
	 *  Save start and extent of argv for setproctitle.
	 */
	Argv = argv;
	while (*envp)
		envp++;
	LastArgv = envp[-1] + strlen(envp[-1]);
#endif

	process_options(argc, argv);

	if (!confs) {
		syslog(LOG_ERR, "no configuration loaded");
		exit(1);
	}
	if (!daydream_path) {
		syslog(LOG_ERR, "you must specify the -p switch");
		exit(1);
	}
	if (check_daydream_path() == -1) {
		syslog(LOG_ERR, "%.200s is not an absolute pathname of DayDream executable", daydream_path);
		exit(1);
	}
	if (create_directory(DDTMP, mcfg.CFG_BBSUID, mcfg.CFG_BBSGID, 0770)) {
		syslog(LOG_ERR, "cannot create temporary directory");
		exit(1);
	}
	if (doinitgroups(mcfg.CFG_BBSUID, mcfg.CFG_BBSGID) == -1) {
		syslog(LOG_ERR, "initgroups() failed");
		exit(1);
	}

	freopen(_PATH_DEVNULL, "w", stderr);
	signal(SIGPIPE, lostconn);
	if ((int) signal(SIGURG, myoob) < 0)
		syslog(LOG_ERR, "signal: %m");

	/* Try to handle urgent data inline */
#ifdef SO_OOBINLINE
	if (setsockopt
	    (0, SOL_SOCKET, SO_OOBINLINE, (char *) &on, sizeof on) < 0)
		syslog(LOG_ERR, "setsockopt: %m");
#endif

#ifdef	F_SETOWN
	if (fcntl(fileno(stdin), F_SETOWN, getpid()) == -1)
		syslog(LOG_ERR, "fcntl F_SETOWN: %m");
#endif
	dolog(&his_addr);
	/*
	 * Set up default state
	 */
	data = -1;
	type = TYPE_I;
	form = FORM_N;
	stru = STRU_F;
	mode = MODE_S;
	tmpline[0] = '\0';
	gethostname(hostname, sizeof hostname);
	reply(220, "Service ready for new user.");
	setjmp(errcatch);

	umask(007);

	for (;;)
		yyparse();
	/* NOTREACHED */
}

#ifndef HAVE_SETPROCTITLE
void setproctitle(const char *fmt, ...)
{
	char *p, *bp, ch;
	int i;
	char buf[BUFSIZ];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buf, sizeof buf, fmt, args);
	va_end(args);

	/* make ps print our process name */
	p = Argv[0];
	*p++ = '-';

	i = strlen(buf);
	if (i > LastArgv - p - 2) {
		i = LastArgv - p - 2;
		buf[i] = '\0';
	}
	bp = buf;
	while ((ch = *bp++) != 0)
		if (ch != '\n' && ch != '\r')
			*p++ = ch;
	while (p < LastArgv)
		*p++ = '\0';
}
#endif

static void lostconn(int signum)
{
	if (debug)
		syslog(LOG_DEBUG, "lost connection");
	dologout(-1);
}

static const char *filepart(const char *s)
{
	const char *t;

	if ((t = (const char *) strrchr(s, '/')))
		return t + 1;
	else
		return s;
}

/*
 * Save the result of a getpwnam.  Used for USER command, since
 * the data returned must not be clobbered by any other command
 * (e.g., globbing).
 */
static struct userbase *ddgetpwnam(char *name)
{
	int fd;
	char buf[PATH_MAX];
	static struct userbase ub;

	if (ssnprintf(buf, "%s/data/userbase.dat", datadir))
		return NULL;

	if ((fd = open(buf, O_RDONLY)) != -1) {
		while (read(fd, &ub, sizeof(struct userbase))) {
			if ((ub.user_toggles & (1L << 30))
			    && ((ub.user_toggles & (1L << 31)) == 0))
				continue;
			if (!strcasecmp(ub.user_handle, name)
			    || !strcasecmp(ub.user_realname, name)) {
				close(fd);
				return (&ub);
			}
		}
		close(fd);
	} else
		syslog(LOG_ERR, "cannot open userbase");

	return NULL;
}

void user(char *name)
{
	if (logged_in) 
		end_login();

	pw = ddgetpwnam(name);
	if (!pw) {
		reply(530, "User %s unknown.", name);
		return;
	}
	reply(331, "Password required for %s.", name);
	askpasswd = 1;
	/*
	 * Delay before reading passwd after first failed
	 * attempt to slow down passwd-guessing programs.
	 */
	if (login_attempts)
		sleep((unsigned) login_attempts);
}

static int ispid(pid_t pid)
{
	return kill(pid, 0) != -1;
}

static int checkconfaccess(int confn)
{
	int newcn = confn - 1;

	if (newcn < 32) {
		if (pw->user_conferenceacc1 & (1L << newcn))
			return 1;
	} else {
		newcn -= 32;
		if (pw->user_conferenceacc2 & (1L << newcn))
			return 1;
	}
	return 0;
}

int setulconf(int cn)
{
	struct DayDream_Conference *tconf = confs;
	struct DayDream_MsgBase *tbase;
	int bcnt;

	if (!checkconfaccess(cn))
		return 0;
	while (tconf->CONF_NUMBER != 255 && tconf->CONF_NUMBER) {
		if (tconf->CONF_UPLOADAREA && cn == tconf->CONF_NUMBER) {
			ulconf = cn;
			return 1;
		}
		/* FIXME: Very suspicious code */
		tbase = (struct DayDream_MsgBase *) tconf + 1;
		bcnt = tconf->CONF_MSGBASES;
		while (bcnt) {
			tbase++;
			bcnt--;
		}
		tconf = (struct DayDream_Conference *) tbase;
	}
	return 0;
}

static char *getddinfo(void)
{
	static char nam[256];
	int infd;
	struct ftpinfo mi;
	int i;
	char *res = 0;
	int freeh = 0;
	for (i = 0; i < mcfg.CFG_MAXFTPUSERS; i++) {
		sprintf(nam, "%s/ftpinfo%d.dat", DDTMP, i + 1);
		infd = open(nam, O_RDONLY);
		if (infd < 0) {
		      kamake:
			freeh = i + 1;
			res = nam;
		} else {
			read(infd, &mi, sizeof(struct ftpinfo));
			close(infd);
			if (!ispid(mi.pid)) {
				goto kamake;
			}
			if (((mcfg.CFG_FLAGS & (1L << 12)) == 0)
			    && mi.userid == pw->user_account_id)
				return (char *) -1;
		}
	}
	if (res) {
		sprintf(nam, "%s/ftpinfo%d.dat", DDTMP, freeh);
		infd = open(nam, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		minfo.pid = getpid();
		minfo.userid = pw->user_account_id;
		if (infd < 0)
			return 0;
		write(infd, &minfo, sizeof(struct ftpinfo));
		close(infd);
	}
	return res;
}

static void updateinfo(void)
{
	int infd = open(infoname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	minfo.pid = getpid();
	minfo.userid = pw->user_account_id;
	if (infd < 0)
		return;
	write(infd, &minfo, sizeof(struct ftpinfo));
	close(infd);
}

/*
 * Terminate login as previous user, if any, resetting state;
 * used when USER command is given or login fails.
 */
static void end_login(void)
{

	seteuid((uid_t) 0);
	pw = NULL;
	logged_in = 0;
}

void pass(char *passwd)
{
	char buf[1024];
	if (logged_in || askpasswd == 0) {
		reply(503, "Login with USER first.");
		return;
	}
	askpasswd = 0;
	if (!cmppasswds(passwd, pw->user_password)) {
		reply(530, "Login incorrect.");
		pw = NULL;
		if (login_attempts++ >= 5) {
			syslog(LOG_NOTICE, "repeated login failures from %s",
			       remotehost);
			exit(0);
		}
		return;
	}

	login_attempts = 0;	/* this time successful */
	logged_in = 1;

	if (ssnprintf(buf, "%s/users/%d/ftp", datadir, pw->user_account_id))
		goto internal_error;
	if (create_directory(buf, mcfg.CFG_BBSUID, mcfg.CFG_BBSGID, 0770))
		goto internal_error;
	if (ssnprintf(buf, "%s/users/%d/ftp/dl", datadir, pw->user_account_id))
		goto internal_error;
	if (create_directory(buf, mcfg.CFG_BBSUID, mcfg.CFG_BBSGID, 0770))
		goto internal_error;
	if (ssnprintf(buf, "%s/users/%d/ftp/ul", datadir, pw->user_account_id))
		goto internal_error;
	if (create_directory(buf, mcfg.CFG_BBSUID, mcfg.CFG_BBSGID, 0770))
		goto internal_error;

	sprintf(buf, "%s/users/%d/ftp", datadir, pw->user_account_id);
	if (chdir(buf) < 0) {
		reply(550, "Can't get user directory.");
		goto bad;
	}
	getcwd(myroot, sizeof myroot);

	seteuid((uid_t) 0);
	if (setegid((gid_t) mcfg.CFG_BBSGID) == -1 ||
		seteuid((uid_t) mcfg.CFG_BBSUID) == -1) {
		syslog(LOG_ERR, "cannot set credentials");
		goto internal_error;
	}

	infoname = getddinfo();
	if (!infoname) {
		reply(550, "No free ftp-slots. Max %d. Try again later.",
		      mcfg.CFG_MAXFTPUSERS);
		goto bad;
	} else if ((int) infoname == -1) {
		reply(550, "No multiple ftp-connections allowed!");
		goto bad;
	}

	lreply(230, "Help");
	printf("Upload files to ul/ dir. After uploading log on to the BBS and hit U\nin the destination conference.\n\nFiles to DL are in dl/ dir. Please remove them after you've leeched them.\n\n");
	reply(230, "User %s logged in.", pw->user_handle);

	snprintf(proctitle, sizeof proctitle, "%s: %s", 
		remotehost, pw->user_handle);
 	setproctitle("%s", proctitle);
	return;

bad:
	end_login();
	return;

internal_error:
	reply(421, "Service not available, closing control connection.");
	exit(1);
}

void retrieve(char *cmd, char *name)
{
	FILE *fin, *dout;
	struct stat st;
	int (*closefunc)(FILE *);
	char line[BUFSIZ];

	if (!checkpath(name)) {
		reply(550, "Illegal filename");
		return;
	}
	strncpy(minfo.filename, filepart(name), 256);
	if (cmd == 0) {
		fin = fopen(name, "r");
		closefunc = fclose;
		st.st_size = 0;
	} else {
		snprintf(line, sizeof line, cmd, name);
		name = line;
		fin = ftpd_popen(line, "r");
		closefunc = ftpd_pclose;
		st.st_size = -1;
		st.st_blksize = BUFSIZ;
	}
	if (fin == NULL) {
		if (errno != 0)
			perror_reply(550, name);
		return;
	}
	if (!cmd && (fstat(fileno(fin), &st) == -1 || !S_ISREG(st.st_mode))) {
		reply(550, "%s: not a plain file.", name);
		goto done;
	}
	if (restart_point) {
		if (type == TYPE_A) {
			int i, n, c;

			n = restart_point;
			i = 0;
			while (i++ < n) {
				if ((c = getc(fin)) == EOF) {
					perror_reply(550, name);
					goto done;
				}
				if (c == '\n')
					i++;
			}
		} else if (lseek(fileno(fin), restart_point, SEEK_SET) < 0) {
			perror_reply(550, name);
			goto done;
		}
	}
	dout = dataconn(name, st.st_size, "w");
	if (dout == NULL)
		goto done;
	send_data(fin, dout, st.st_blksize);
	fclose(dout);
	data = -1;
	pdata = -1;
done:
	closefunc(fin);
}

void _store(char *name, const char *mode, int unique)
{
	FILE *fout, *din;
	struct stat st;
	int (*closefunc)(FILE *);
	char buf[1024];
	char oldbuf[1024];

	if (!checkpath(name)) {
		reply(550, "Illegal filename");
		goto err;
	}

	if (dupecheck(name)) {
		reply(550, "File %s is already online.", name);
		goto err;
	}

	if (unique && stat(name, &st) == 0 &&
		(name = gunique(name)) == NULL)
		goto err;

	if (restart_point)
		mode = "r+w";
	fout = fopen(name, mode);
	closefunc = fclose;
	if (fout == NULL) {
		perror_reply(553, name);
		goto err;
	}
	if (restart_point) {
		if (type == TYPE_A) {
			int i, n, c;

			n = restart_point;
			i = 0;
			while (i++ < n) {
				if ((c = getc(fout)) == EOF) {
					perror_reply(550, name);
					goto done;
				}
				if (c == '\n')
					i++;
			}
			/*
			 * We must do this seek to "current" position
			 * because we are changing from reading to
			 * writing.
			 */
			if (fseek(fout, 0L, SEEK_CUR) < 0) {
				perror_reply(550, name);
				goto done;
			}
		} else if (lseek(fileno(fout), restart_point, SEEK_SET) <
			   0) {
			perror_reply(550, name);
			goto done;
		}
	}
	din = dataconn(name, (off_t) - 1, "r");
	strncpy(minfo.filename, filepart(name), 256);

	if (din == NULL)
		goto done;
	if (receive_data(din, fout) == 0) {
		fclose(din);
// 2.14.9 code below.  Gonna attempt 2.14.7 and see if it works better.
		if (ulconf && (!daydream_path || check_archive(name) == -1))
			syslog(LOG_DEBUG, "archive check failed");
		if (unique)
			reply(226,
			      "Transfer complete (unique file name:%s).",
			      name);
		else
			reply(226, "Transfer complete.");
	} else {
		fclose(din);
	}
	data = -1;
	pdata = -1;
      done:
	(*closefunc) (fout);
	return;

      err:
	close(data);
	close(pdata);
	data = -1;
	pdata = -1;
}

static int check_archive(const char *name)
{
	char id[16];
	char uc[16];
	char pname[PATH_MAX];
	char *execname;
	pid_t pid;
	int status;

	syslog(LOG_DEBUG, "executing archive check");

	snprintf(id, sizeof id, "%hu", pw->user_account_id);
	snprintf(uc, sizeof uc, "%d", ulconf);
	if (!getcwd(pname, sizeof pname)) {
		syslog(LOG_ERR, "getcwd failed: %m");
		return -1;
	}
	if (strlcat(pname, "/", sizeof pname) >= sizeof pname)
		return -1;
	if (strlcat(pname, filepart(name), sizeof pname) >= sizeof pname)
		return -1;
	if (!(execname = strrchr(daydream_path, '/')))
		return -1;
	switch (pid = fork()) {
	case -1:
		syslog(LOG_ERR, "fork failed: %m");
		return -1;
	case 0:
		setenv("DAYDREAM", datadir, 1);
		execl(daydream_path, execname, "-u", id, uc, pname, NULL);
		syslog(LOG_ERR, "execl failed: %m");
		_exit(1);
	default:
		if (waitpid(pid, &status, 0) == -1)
			return -1;
		if (!WIFEXITED(status) || WEXITSTATUS(status))
			return -1;
		return 0;
	}
}

static FILE *getdatasock(char *mode)
{
	int s, on = 1, tries;

	if (data >= 0)
		return (fdopen(data, mode));
	seteuid((uid_t) 0);
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		goto bad;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
		       (char *) &on, sizeof on) < 0)
		goto bad;
	/* anchor socket to avoid multi-homing problems */
	data_source.sin_family = AF_INET;
	data_source.sin_addr = ctrl_addr.sin_addr;
	for (tries = 1;; tries++) {
		if (bind(s, (struct sockaddr *) &data_source,
			 sizeof data_source) >= 0)
			break;
		if (errno != EADDRINUSE || tries > 10)
			goto bad;
		sleep(tries);
	}
	seteuid((uid_t) mcfg.CFG_BBSUID);
#ifdef IP_TOS
	on = IPTOS_THROUGHPUT;
	if (setsockopt(s, IPPROTO_IP, IP_TOS, (char *) &on, sizeof(int)) < 0)
		syslog(LOG_WARNING, "setsockopt (IP_TOS): %m");
#endif
	return (fdopen(s, mode));

bad:
	seteuid((uid_t) mcfg.CFG_BBSUID);
	close(s);
	return NULL;
}

static FILE *dataconn(char *name, off_t size, char *mode)
{
	char sizebuf[32];
	FILE *file;
	int retry = 0, tos;

	minfo.filesize = size;
	file_size = size;
	byte_count = 0;
	if (size != (off_t) - 1)
		sprintf(sizebuf, " (%ld bytes)", size);
	else
		strcpy(sizebuf, "");
	if (pdata >= 0) {
		struct sockaddr_in from;
		int s, fromlen = sizeof from;

		s = accept(pdata, (struct sockaddr *) &from, &fromlen);
		if (s < 0) {
			reply(425, "Can't open data connection.");
			close(pdata);
			pdata = -1;
			return (NULL);
		}
		close(pdata);
		pdata = s;
#ifdef IP_TOS
		tos = IPTOS_LOWDELAY;
		setsockopt(s, IPPROTO_IP, IP_TOS, (char *) &tos, sizeof(int));
#endif
		reply(150, "Opening %s mode data connection for %s%s.",
		      type == TYPE_A ? "ASCII" : "BINARY", name, sizebuf);
		return (fdopen(pdata, mode));
	}
	if (data >= 0) {
		reply(125, "Using existing data connection for %s%s.",
		      name, sizebuf);
		usedefault = 1;
		return (fdopen(data, mode));
	}
	if (usedefault)
		data_dest = his_addr;
	usedefault = 1;
	file = getdatasock(mode);
	if (file == NULL) {
		reply(425, "Can't create data socket (%s,%d): %s.",
		      inet_ntoa(data_source.sin_addr),
		      ntohs(data_source.sin_port), strerror(errno));
		return (NULL);
	}
	data = fileno(file);
	while (connect(data, (struct sockaddr *) &data_dest, 
		sizeof data_dest) < 0) {
		if (errno == EADDRINUSE && retry < swaitmax) {
			sleep((unsigned) swaitint);
			retry += swaitint;
			continue;
		}
		perror_reply(425, "Can't build data connection");
		fclose(file);
		data = -1;
		return (NULL);
	}
	reply(150, "Opening %s mode data connection for %s%s.",
	      type == TYPE_A ? "ASCII" : "BINARY", name, sizebuf);
	return (file);
}

/*
 * Tranfer the contents of "instr" to
 * "outstr" peer using the appropriate
 * encapsulation of the data subject
 * to Mode, Structure, and Type.
 *
 * NB: Form isn't handled.
 */
static void send_data(FILE * instr, FILE * outstr, off_t blksize)
{
	int c, cnt;
	char *buf;
	int netfd, filefd;
	time_t startt, t2;
	startt = time(0);
	transflag++;
	if (setjmp(urgcatch)) {
		transflag = 0;
		return;
	}
	switch (type) {

	case TYPE_A:
		while ((c = getc(instr)) != EOF) {
			byte_count++;
			if (c == '\n') {
				if (ferror(outstr))
					goto data_err;
				putc('\r', outstr);
			}
			putc(c, outstr);
		}
		fflush(outstr);
		transflag = 0;
		if (ferror(instr))
			goto file_err;
		if (ferror(outstr))
			goto data_err;
		reply(226, "Transfer complete.");
		return;

	case TYPE_I:
	case TYPE_L:
		minfo.mode = 2;
		if ((buf = malloc((u_int) blksize)) == NULL) {
			transflag = 0;
			perror_reply(451,
				     "Local resource failure: malloc");
			return;
		}
		netfd = fileno(outstr);
		filefd = fileno(instr);
		minfo.transferred = 0;
		while ((cnt = read(filefd, buf, (u_int) blksize)) > 0) {
			int offset = 0, sent;

			while (cnt > 0) {
				sent = write(netfd, buf + offset, cnt);
				minfo.transferred += sent;
				t2 = time(0);
				if (t2 - startt)
					minfo.cps =
					    minfo.transferred / (t2 -
								 startt);
				updateinfo();
				if (sent <= 0)
					break;
				offset += sent;
				cnt -= sent;
			}
			byte_count += cnt;
			if (cnt)
				break;
		}
		transflag = 0;
		free(buf);
		minfo.mode = 0;
		updateinfo();
		if (cnt != 0) {
			if (cnt < 0)
				goto file_err;
			goto data_err;
		}
		reply(226, "Transfer complete.");
		return;
	default:
		transflag = 0;
		reply(550, "Unimplemented TYPE %d in send_data", type);
		return;
	}

      data_err:
	transflag = 0;
	perror_reply(426, "Data connection");
	return;

      file_err:
	transflag = 0;
	perror_reply(551, "Error on input file");
}

/*
 * Transfer data from peer to
 * "outstr" using the appropriate
 * encapulation of the data subject
 * to Mode, Structure, and Type.
 *
 * N.B.: Form isn't handled.
 */
static int receive_data(FILE * instr, FILE * outstr)
{
	int c;
	int cnt, bare_lfs = 0;
	char buf[BUFSIZ];
	time_t startt = time(0);
	time_t t2;

	transflag++;
	if (setjmp(urgcatch)) {
		transflag = 0;
		return (-1);
	}
	switch (type) {

	case TYPE_I:
	case TYPE_L:
		minfo.mode = 1;
		while ((cnt = read(fileno(instr), buf, sizeof buf)) > 0) {
			if (write(fileno(outstr), buf, cnt) != cnt)
				goto file_err;
			byte_count += cnt;
			minfo.transferred = byte_count;
			t2 = time(0);
			if (t2 - startt) {
				minfo.cps = byte_count / (t2 - startt);
			}
			updateinfo();
		}
		if (cnt < 0)
			goto data_err;
		transflag = 0;
		minfo.mode = 0;
		updateinfo();
		return (0);

	case TYPE_E:
		reply(553, "TYPE E not implemented.");
		transflag = 0;
		return (-1);

	case TYPE_A:
		while ((c = getc(instr)) != EOF) {
			byte_count++;
			if (c == '\n')
				bare_lfs++;
			while (c == '\r') {
				if (ferror(outstr))
					goto data_err;
				if ((c = getc(instr)) != '\n') {
					putc('\r', outstr);
					if (c == '\0' || c == EOF)
						goto contin2;
				}
			}
			putc(c, outstr);
		      contin2:;
		}
		fflush(outstr);
		if (ferror(instr))
			goto data_err;
		if (ferror(outstr))
			goto file_err;
		transflag = 0;
		if (bare_lfs) {
			lreply(230,
			       "WARNING! %d bare linefeeds received in ASCII mode",
			       bare_lfs);
			printf
			    ("   File may not have transferred correctly.\r\n");
		}
		return (0);
	default:
		reply(550, "Unimplemented TYPE %d in receive_data", type);
		transflag = 0;
		return (-1);
	}

      data_err:
	transflag = 0;
	perror_reply(426, "Data Connection");
	return (-1);

      file_err:
	transflag = 0;
	perror_reply(452, "Error writing file");
	return (-1);
}

void statfilecmd(char *filename)
{
	char line[BUFSIZ];
	FILE *fin;
	int c;

	sprintf(line, "/bin/ls -lgA %s", filename);
	fin = ftpd_popen(line, "r");
	lreply(211, "status of %s:", filename);
	while ((c = getc(fin)) != EOF) {
		if (c == '\n') {
			if (ferror(stdout)) {
				perror_reply(421, "control connection");
				ftpd_pclose(fin);
				dologout(1);
				/* NOTREACHED */
			}
			if (ferror(fin)) {
				perror_reply(551, filename);
				ftpd_pclose(fin);
				return;
			}
			putc('\r', stdout);
		}
		putc(c, stdout);
	}
	ftpd_pclose(fin);
	reply(211, "End of Status");
}

void statcmd(void)
{
	struct sockaddr_in *sin;
	u_char *a, *p;

	lreply(211, "%s FTP server status:", hostname);
	printf("     Connected to %s", remotehost);
	if (!isdigit(remotehost[0]))
		printf(" (%s)", inet_ntoa(his_addr.sin_addr));
	printf("\r\n");
	if (logged_in) 
		printf("     Logged in as %s\r\n", pw->user_handle);
	else if (askpasswd)
		printf("     Waiting for password\r\n");
	else
		printf("     Waiting for user name\r\n");
	printf("     TYPE: %s", typenames[type]);
	if (type == TYPE_A || type == TYPE_E)
		printf(", FORM: %s", formnames[form]);
	if (type == TYPE_L)
#if NBBY == 8
		printf(" %d", NBBY);
#else
		printf(" %d", bytesize);	/* need definition! */
#endif
	printf("; STRUcture: %s; transfer MODE: %s\r\n",
	       strunames[stru], modenames[mode]);
	if (data != -1)
		printf("     Data connection open\r\n");
	else if (pdata != -1) {
		printf("     in Passive mode");
		sin = &pasv_addr;
		goto printaddr;
	} else if (usedefault == 0) {
		printf("     PORT");
		sin = &data_dest;
	      printaddr:
		a = (u_char *) & sin->sin_addr;
		p = (u_char *) & sin->sin_port;
#define UC(b) (((int) b) & 0xff)
		printf(" (%d,%d,%d,%d,%d,%d)\r\n", UC(a[0]),
		       UC(a[1]), UC(a[2]), UC(a[3]), UC(p[0]), UC(p[1]));
#undef UC
	} else
		printf("     No data connection\r\n");
	reply(211, "End of status");
}

void fatal(char *s)
{
	reply(451, "Error in server: %s\n", s);
	reply(221, "Closing connection due to server error.");
	dologout(0);
	/* NOTREACHED */
}

void reply(int n, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	printf("%d ", n);
	vprintf(fmt, args);
	printf("\r\n");
	fflush(stdout);
	va_end(args);
}

void lreply(int n, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	printf("%d- ", n);
	vprintf(fmt, args);
	printf("\r\n");
	fflush(stdout);
	va_end(args);
}

void ack(char *s)
{
	reply(250, "%s command successful.", s);
}

void nack(const char *s)
{
	reply(502, "%s command not implemented.", s);
}

/* ARGSUSED */
void yyerror(char *s)
{
	char *cp;

	if ((cp = strchr(cbuf, '\n')) != 0)
		*cp = '\0';
	reply(500, "'%s': command not understood.", cbuf);
}

void _delete(char *name)
{
	struct stat st;

	if (!checkpath(name)) {
		reply(550, "Illegal filename");
		return;
	}

	if (stat(name, &st) < 0) {
		perror_reply(550, name);
		return;
	}

	if (unlink(name) < 0) {
		perror_reply(550, name);
		return;
	}
	ack("DELE");
}

void cwd(char *path)
{
	if (checkpath(path)) {
		chdir(path);
		if (strstr(path, "ul"))
			show_upload_insns();
		ack("CWD");
	} else
		reply(550, "Don't even try it.");
}

void makedir(char *name)
{
	if (!checkpath(name)) {
		reply(550, "Illegal filename");
		return;
	}
	mkdir(name, 0755);
}

void removedir(char *name)
{
	if (!checkpath(name)) {
		reply(550, "Illegal filename");
		return;
	}
	rmdir(name);
}

void pwd(void)
{
	char path[MAXPATHLEN + 1];

	if (getcwd(path, sizeof path) == (char *) NULL)
		reply(550, "%s.", path);
	else
		reply(257, "\"%s\" is current directory.", path);
}

char *renamefrom(char *name)
{
	struct stat st;

	if (!checkpath(name)) {
		reply(550, "Illegal filename");
		return 0;
	}
	if (stat(name, &st) < 0) {
		perror_reply(550, name);
		return ((char *) 0);
	}
	reply(350, "File exists, ready for destination name");
	return (name);
}

/* FIXME: 'to' should not contain '/' */
void renamecmd(char *from, char *to)
{
	if (!checkpath(from) || !checkpath(to)) {
		reply(550, "Illegal filename");
		return;
	}
	if (rename(from, to) < 0)
		perror_reply(550, "rename");
	else
		ack("RNTO");
}

void dolog(struct sockaddr_in *sin)
{
	struct hostent *hp;
	time_t t;

	hp = gethostbyaddr((char *) &sin->sin_addr,
		sizeof(struct in_addr), AF_INET);

	if (hp)
		strncpy(remotehost, hp->h_name, sizeof remotehost);
	else
		strncpy(remotehost, inet_ntoa(sin->sin_addr), 
			sizeof remotehost);

	snprintf(proctitle, sizeof proctitle, "%s: connected", remotehost);
	setproctitle("%s", proctitle);

	t = time(NULL);
	syslog(LOG_INFO, "connection from %s at %s", remotehost, ctime(&t));
}

/*
 * Exit with supplied status.
 */
void dologout(int status)
{
	if (logged_in) 
		seteuid((uid_t) 0);
	if (infoname)
		unlink(infoname);

	/* beware of flushing buffers after a SIGPIPE */
	_exit(status);
}

static void myoob(int signum)
{
	char *cp;

	/* only process if transfer occurring */
	if (!transflag)
		return;
	cp = tmpline;
	if (ftpgetline(cp, 7, stdin) == NULL) {
		reply(221, "You could at least say goodbye.");
		dologout(0);
	}
	upper(cp);
	if (strcmp(cp, "ABOR\r\n") == 0) {
		tmpline[0] = '\0';
		reply(426, "Transfer aborted. Data connection closed.");
		reply(226, "Abort successful");
		longjmp(urgcatch, 1);
	}
	if (strcmp(cp, "STAT\r\n") == 0) {
		if (file_size != (off_t) - 1)
			reply(213, "Status: %lu of %lu bytes transferred",
			      byte_count, file_size);
		else
			reply(213, "Status: %lu bytes transferred",
			      byte_count);
	}
}

/*
 * Note: a response of 425 is not mentioned as a possible response to
 * 	the PASV command in RFC959. However, it has been blessed as
 * 	a legitimate response by Jon Postel in a telephone conversation
 *	with Rick Adams on 25 Jan 89.
 */
void passive(void)
{
	int len;
	u_short port;
	char *p, *a;

	if (!logged_in) {
		reply(530, "You are not logged in");
		return;
	}

	pdata = socket(AF_INET, SOCK_STREAM, 0);
	if (pdata < 0) {
		perror_reply(425, "Can't open passive connection");
		return;
	}

	if (restricted_data_ports) {
		for (port = FTP_DATA_BOTTOM; port <= FTP_DATA_TOP; port++) {
			pasv_addr = ctrl_addr;
			pasv_addr.sin_port = htons(port);
			seteuid((uid_t) 0);
			if (bind(pdata, (struct sockaddr *) &pasv_addr,
				 sizeof pasv_addr) < 0) {
				seteuid((uid_t) mcfg.CFG_BBSUID);
				if (errno == EADDRINUSE)
					continue;
				else
					goto pasv_error;
			}
			seteuid((uid_t) mcfg.CFG_BBSUID);
			break;
		}
		if (port > FTP_DATA_TOP)
			goto pasv_error;
	} else {
		pasv_addr = ctrl_addr;
		pasv_addr.sin_port = 0;
		seteuid((uid_t) 0);
		if (bind(pdata, (struct sockaddr *) &pasv_addr,
			 sizeof pasv_addr) < 0) {
			seteuid((uid_t) mcfg.CFG_BBSUID);
			goto pasv_error;
		}
		seteuid((uid_t) mcfg.CFG_BBSUID);
	}

	len = sizeof pasv_addr;
	if (getsockname(pdata, (struct sockaddr *) &pasv_addr, &len) < 0)
		goto pasv_error;
	if (listen(pdata, 1) < 0)
		goto pasv_error;
	a = (char *) &pasv_addr.sin_addr;
	if (forced_passive_ip)
		a = (char *) &passive_ip.sin_addr;
	p = (char *) &pasv_addr.sin_port;

#define UC(b) (((int) b) & 0xff)

	reply(227, "Entering Passive Mode (%d,%d,%d,%d,%d,%d)", UC(a[0]),
	      UC(a[1]), UC(a[2]), UC(a[3]), UC(p[0]), UC(p[1]));
	return;

pasv_error:
	close(pdata);
	pdata = -1;
	perror_reply(425, "Can't open passive connection");
	return;
}

/*
 * Generate unique name for file with basename "local".
 * The file named "local" is already known to exist.
 * Generates failure reply on error.
 */
static char *gunique(char *local)
{
	static char new[MAXPATHLEN];
	struct stat st;
	char *cp = strrchr(local, '/');
	int count = 0;

	if (cp)
		*cp = '\0';
	if (stat(cp ? local : ".", &st) < 0) {
		perror_reply(553, cp ? local : ".");
		return ((char *) 0);
	}
	if (cp)
		*cp = '/';
	strcpy(new, local);
	cp = new + strlen(new);
	*cp++ = '.';
	for (count = 1; count < 100; count++) {
		sprintf(cp, "%d", count);
		if (stat(new, &st) < 0)
			return (new);
	}
	reply(452, "Unique file name cannot be created.");
	return ((char *) 0);
}

void perror_reply(int code, char *string)
{
	reply(code, "%s: %s.", string, strerror(errno));
}

void send_file_list(char *dname)
{
	struct stat st;
	DIR *dir;
	struct dirent *dent;
	FILE *dout;

	if (setjmp(urgcatch)) {
		transflag = 0;
		return;
	}

	if (stat(dname, &st) == -1) {
		reply(450, "No such file or directory.");
		return;
	}
	if (!S_ISDIR(st.st_mode)) {
		reply(501, "Not a directory.");
		return;
	}
	
	if (!(dir = opendir(dname))) {
		reply(451, "Internal server error.");
		return;
	}

	if (!(dout = dataconn("file list", (off_t) - 1, "w")))
		return;
	transflag++;

	while ((dent = readdir(dir))) {
		char nbuf[PATH_MAX];

		if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
			continue;

		snprintf(nbuf, sizeof nbuf, "%s/%s", dname, dent->d_name);

		/* we have to do a stat to insure it's not a directory or
		 * special file. */
		if (stat(nbuf, &st) != 0 || !S_ISREG(st.st_mode)) 
			continue;

		/* it's user's responsibility to switch to TYPE A */
		if (nbuf[0] == '.' && nbuf[1] == '/')
			fprintf(dout, "%s\r\n", nbuf + 2);
		else
			fprintf(dout, "%s\r\n", nbuf);
		
		/* FIXME: compute correct byte count */
		byte_count += strlen(nbuf) + 1;
	}
	closedir(dir);

	if (ferror(dout))
		reply(426, "Data connection.");
	else
		reply(226, "Transfer complete.");

	fclose(dout);
	transflag = 0;
	data = -1;
	pdata = -1;
}

static int cmppasswds(char *passwd, unsigned char *thepw)
{
	MD_CTX context;
	unsigned char digest[16];
	char newpw[30];
	int i;

	strcpy(newpw, passwd);
	strupr(newpw);

	MDInit(&context);
	MDUpdate(&context, newpw, strlen(newpw));
	MDFinal(digest, &context);

	for (i = 0; i < 16; i++) {
		if (thepw[i] != digest[i])
			return (0);
	}
	return (1);
}

static int dupecheck(const char *name)
{
	struct DayDream_Conference *tconf = confs;
	struct DayDream_MsgBase *tbase;

	while (tconf->CONF_NUMBER != 255 && tconf->CONF_NUMBER) {
		int bcnt;
		if (!(tconf->CONF_ATTRIBUTES & (1L << 7))) {
			char de[1024];
			if (findfile(name, de, tconf)) {
				return 1;
			}
		}
		/* FIXME: very suspicious code */
		tbase = (struct DayDream_MsgBase *) tconf + 1;
		bcnt = tconf->CONF_MSGBASES;
		while (bcnt) {
			tbase++;
			bcnt--;
		}
		tconf = (struct DayDream_Conference *) tbase;
	}
	return 0;
}

static int findfile(const char *file, char *de,
		    struct DayDream_Conference *tc)
{
	char buf1[1024];
	char buf2[1024];
	int fd1;
	FILE *plist;
	const char *s;
	DIR *dh;
	struct dirent *dent;
	struct DayDream_Conference *co = tc;

	*de = 0;
	s = file;
	while (*s) {
		if (*s == '/')
			return 0;
		s++;
	}
	sprintf(buf1, "%s/data/paths.dat", co->CONF_PATH);
	if ((plist = fopen(buf1, "r"))) {
		while (fgetsnolf(buf1, 512, plist)) {
			sprintf(buf2, "%s%s", buf1, file);
			if (*buf1 && ((fd1 = open(buf2, O_RDONLY)) > -1)) {
				fclose(plist);
				close(fd1);

				strcpy(de, buf2);
				return 1;
			}
		}
		fclose(plist);
	}

	sprintf(buf1, "%s/data/paths.dat", co->CONF_PATH);
	if ((plist = fopen(buf1, "r"))) {
		while (fgetsnolf(buf1, 512, plist)) {
			if (*buf1 && (dh = opendir(buf1))) {
				while ((dent = readdir(dh))) {
					if (!strcmp(dent->d_name, ".")
					    ||
					    (!strcmp(dent->d_name, "..")))
						continue;
					if (!strcasecmp
					    (dent->d_name, file)) {
						sprintf(de, "%s%s", buf1,
							dent->d_name);
						break;
					}
				}
				closedir(dh);
			}
		}
		fclose(plist);
	}
	if (*de)
		return 1;
	return 0;
}

static char *fgetsnolf(char *buf, int n, FILE * fh)
{
	char *hih;
	char *s;

	hih = fgets(buf, n, fh);
	if (!hih)
		return 0;
	s = buf;
	while (*s) {
		if (*s == 13 || *s == 10) {
			*s = 0;
			break;
		}
		s++;
	}
	return hih;
}

static int checkpath(char *pa)
{
	char olddir[1024];
	struct stat st;
	char buf[1024];

	getcwd(olddir, sizeof olddir);
	if ((stat(pa, &st) < 0) || (S_ISDIR(st.st_mode) == 0)) {
		_splitpath(pa, buf);
		if (!*buf)
			return 1;
	} else {
		strcpy(buf, pa);
	}
	if (chdir(buf) > 0)
		return 0;
	getcwd(buf, sizeof olddir);
	if (!strncmp(buf, myroot, strlen(myroot))) {
		chdir(olddir);
		return 1;
	}
	chdir(olddir);
	return 0;
}

static void _splitpath(char *file, char *path)
{
	char *s, *t;

	s = file;
	while (*s)
		s++;

	while (*s != '/') 
		if (s == file) {
			*path = 0;
			return;
		} else 
			s--;

	s++;
	t = file;
	while (t != s) 
		*path++ = *t++;
	*path = 0;
}
