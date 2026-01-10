#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grp.h>
#include <pwd.h>

#ifdef USE_SGTTY
#include <sgtty.h>
#endif

#include <daydream.h>
#include <ddcommon.h>

#define LOGINATTEMPTS 10

int lmode = 0;

int serhandle;
char keysrc;
int ansi;
int carrier;
int timeleft;
time_t endtime;
int lrp;
int lsp;
int oldlrp;
int oldlsp;
int highest;
int lowest;
int bytestagged, fbytestagged;
int delayt;
uint16_t filestagged, ffilestagged;

struct DayDream_MainConfig maincfg;

msgbase_t *current_msgbase;

struct DayDream_AccessPreset *presets;
struct DayDream_DisplayMode *displays;
struct DayDream_DisplayMode *display;
struct DayDream_Multinode *nodes;
struct DayDream_Multinode *currnode;
struct DayDream_Archiver *arcs;
struct DD_ExternalCommand *exts;
struct DayDream_Protocol *protocols;
struct DayDream_Protocol *protocol;
struct callerslog clog;
struct DD_Seclevel *secs;

int ul_user;
int ul_conf;
char *ul_file;

struct List *olms;

uint8_t selcfg[2056];

int pages;
int access1;
int access2;
struct userbase user;
int onlinestat;
int forcebps = 0;


char lrpdatname[80];
int userinput;
char *origdir;

char *sd[MAXSTRS];
unsigned char inconvtab[256];
unsigned char outconvtab[256];

static void carrieroff(int);
static int createflaglist(void);
static int getin(void);
static int ispw(void);
static int syspw(void);
static int visit_bbs(int);
static int matrix(void);

static void readdatafile(void **dest, const char *filename, int endmarker)
{
	struct stat st;
	int fd;

	if ((fd = open(filename, O_RDONLY)) < 0) {
		fprintf(stderr, "Can't open %s.\n", filename);
		exit(1);
	}
	fstat(fd, &st);
	*dest = xmalloc(st.st_size + 2);
	read(fd, *dest, st.st_size);
	close(fd);
	*((unsigned char *) *dest + st.st_size) = endmarker;
}

int main(int argc, char *argv[])
{
	int datafd;

	/* FIXME: the facility should be configurable */
	openlog("daydream", LOG_PID, LOG_LOCAL2);

	if (!(origdir = getenv("DAYDREAM"))) {
		syslog(LOG_ERR, "environment variable DAYDREAM not set");
		fputs("Environment variable DAYDREAM is not set.\n", stderr);
		exit(1);
	}
	if (chdir(origdir) == -1) {
		syslog(LOG_ERR, "cannot chdir to home directory");
		fputs("Cannot chdir to BBS home directory.\n", stderr);
		exit(1);
	}

	umask(007);

	datafd = open("data/daydream.dat", O_RDONLY);
	if (datafd == -1) {
		syslog(LOG_WARNING, "cannot open \"daydream.dat\": %s",
			strerror(errno));
		fprintf(stderr, "cannot open \"daydream.dat\": %s\n", 
			strerror(errno));
		exit(1);
	}
	if (read(datafd, &maincfg, sizeof(struct DayDream_MainConfig)) !=
		sizeof(struct DayDream_MainConfig)) {
		syslog(LOG_WARNING, "bad \"daydream.dat\": %s", 
			strerror(errno));
		fprintf(stderr, "bad \"daydream.dat\": %s\n", strerror(errno));
		exit(1);
	}
	close(datafd);
	
	if (create_directory(DDTMP, maincfg.CFG_BBSUID, 
		maincfg.CFG_BBSGID, 0770) == -1)  {
		syslog(LOG_ERR, "cannot create temporary directory: %m");
		fputs("Cannot create temporary directory.\n", stderr);
		exit(1);
	}

	initstringspace();
	srand(time(NULL));

	serhandle = 0;
	protocol = 0;
	onlinestat = 0;
	userinput = 1;
	lrp = lsp = oldlrp = oldlsp = 0;
	lrpdatname[0] = 0;
	ffilestagged = filestagged = bytestagged = fbytestagged = 0;
	*reason = 0;
	memset(&clog, 0, sizeof(struct callerslog));

	if (!getcmdline(argc, argv))
		return 0;
	

	read_conference_data();
	readdatafile((void **) &nodes, "data/multinode.dat", 0);
	readdatafile((void **) &presets, "data/access.dat", 0);
	readdatafile((void **) &secs, "data/security.dat", 0);
	readdatafile((void **) &displays, "data/display.dat", 0);
	readdatafile((void **) &exts, "data/externalcommands.dat", 0);
	readdatafile((void **) &arcs, "data/archivers.dat", 255);
	readdatafile((void **) &protocols, "data/protocols.dat", 0);

	display = 0;

	/* Check if we know which TTY we're using.. */

	olms = NewList();

	if (ul_user != -1) {
		cmdlineupload();
		return 0;
	}

	initterm();
	/* Initialize carrier loss handler */

	if (mkdir(currnode->MULTI_TEMPORARY, 0770) == -1 && errno != EEXIST) {
		syslog(LOG_ERR, "cannot create temporary directory %s (%d)",
			currnode->MULTI_TEMPORARY, errno);
		fprintf(stderr, "cannot create temporary directory %s (%d)\r\n",
			currnode->MULTI_TEMPORARY, errno);
		return 0;
	}

	signal(SIGHUP, carrieroff);
	if (!fnode) {
		signal(SIGTERM, carrieroff);
		signal(SIGINT, carrieroff);
	}


	if (fnode) {
		int ir = 0;
		while (waitforcall(ir++) != -1) {			
			visitbbs(0);
			if (forcebps)
				break;
		}
		tcflush(serhandle, TCIOFLUSH);
	} else 
		visitbbs(0);
		
	fini_keyboard();
	return 0;
}

int visitbbs(int m)
{
	int retcode;
	reset_history();
	init_keyboard();
	init_menu_system();
	retcode = visit_bbs(m);
	dropcarrier();
	fini_menu_system();
	fini_keyboard();
	return retcode;
}

static int create_new_account(void)
{
	DDPut(sd[newucstr]);
	switch (HotKey(HOT_NOYES)) {
	case 1:
		if (CreateNewAccount()) {
			clog.cl_userid = user.user_account_id;
			clog.cl_firstcall = user.user_firstcall;
			clog.cl_logon = time(0);
			if (user.user_connections == 0)
				clog.cl_flags |= CL_NEWUSER;
			clog.cl_bpsrate = bpsrate;

			getin();
			return 1;
		}
		return 0;
	case 2:
		DDPut("\n");
		return 0;
	default:
		return 1;
	}
}

static int try_login(void)
{
	char username[300];
	int retvalue, passwdcnt;
	
	DDPut(sd[usernamestr]);
	username[0] = 0;
	
	Prompt(username, 25, 0);
	removespaces(username);
	if (!checkcarrier())
		return -1;
	if (!username[0]) {		
		DDPut("");
		return -1;
	}
	if (!strcasecmp("new", username) && 
		!(maincfg.CFG_FLAGS & (1L << 17))) {
		CreateNewAccount();
		return -1;
	}
	if (!strcasecmp("logoff", username))
		return 0;
	if (!strcasecmp("chat", username)) {
		pagesysop(0);
		return -1;
	}

	retvalue = checklogon(username);
	if (!retvalue && !(maincfg.CFG_FLAGS & (1L << 17))) {
		if (maincfg.CFG_FLAGS & (1L << 9))
			return create_new_account() ? 0 : -1;
		else {
			DDPut(sd[unknownuserstr]);
			return -1;
		}
	} else {
		if (retvalue != 1 && !(maincfg.CFG_FLAGS & (1L << 18)))
			return -1;
		for (passwdcnt = 0; passwdcnt < 3; passwdcnt++) {
			username[0] = 0;
			if (ispw() || retvalue != 1) {
				DDPut(sd[passwordstr]);
				Prompt(username, 25, PROMPT_SECRET);
			}
			if (!checkcarrier()) 
				return -1;
			if (retvalue > 0 && (!ispw() || 
				cmppasswds(username, user.user_password))) {
				if (retvalue == 2) 
					DDPut(sd[alreadyonlinestr]);
				else
					getin();
				return 0;
			} else {
				if (passwdcnt != 2)
					DDPut(sd[tryagainstr]);
				clog.cl_flags |= CL_PASSWDFAIL;
			}
		}
		if (retvalue != 2) {
			TypeFile("passwordfailure", TYPE_MAKE);
			DDPut(sd[excessivepwfailstr]);
			return 0;
		} 
	}

	return -1;
}

static void login_loop(void)
{
	int attempts;
	for (attempts = 0; attempts < 10; attempts++) {
		if (!try_login())
			return;
		if (!checkcarrier())
			return;
	}
	DDPut(sd[maxattemptsstr]);
}

static int visit_bbs(int m)
{
	time_t aika;
	char *aikas;
	char puskur[300];

	lrp = lsp = oldlrp = oldlsp = 0;
	lrpdatname[0] = 0;
	ffilestagged = filestagged = bytestagged = fbytestagged = 0;
	*reason = 0;
	memset(&clog, 0, sizeof(struct callerslog));
	memset(&user, 0, sizeof(struct userbase));
	onlinestat = 0;
	display = 0;
	clearlist(olms);
	clearlist(flaggedfiles);

	carrier = 1;
	aika = time(0);
	aikas = ctime(&aika);
	aikas[24] = 0;

	snprintf(puskur, sizeof puskur, "===============================================[ %s ]===\n%d BPS connection on line #%d\n\n", aikas, bpsrate, node);
	writelog(puskur);

	changenodestatus("Logging in...");

	ddprintf("[0m[2J[HDayDream BBS/UNiX %s\nProgramming by Antti H�yrynen 1996, 1997,\n    DayDream Development Team 1998, 1999, 2000, 2001, 2002, 2003, 2004,\n    Bo Simonsen 2008, 2009, 2010\nCurrently maintained by Ryan Fantus\nYou are connected to node #%d at %d BPS.\n\n", versionstring, node, bpsrate);

	rundoorbatch("data/frontends.dat", NULL);

	if (!display)
		if (getdisplaymode(0, 0) < 1) {
			DDPut("Failed to load displaymode... disconnecting!\n\n");
			return 0;
		}
	if (!syspw()) {
		DDPut(sd[disconnectingstr]);
		return 0;
	}

	// default option of matrix_result being 0 is to continue login process as normal

	TypeFile("banner", TYPE_MAKE | TYPE_WARN);

	if (m) {
		switch (checklogon("sysop")) {
		case 1:
			sleep(2);
			getin();
			break;
		}
		return 1;
	}
	login_loop();

	DDPut(sd[disconnectingstr]);
	dropcarrier();
	return 1;
}

static int ispw(void)
{
	int i;
	for (i = 0; i < 15; i++) {
		if (user.user_password[i])
			return 1;
	}
	return 0;
}

int clearlist(struct List *l)
{
	struct Node *nodeh;
	struct Node *onode;

	if (!l)
		return 0;

	nodeh = l->lh_Head;

	while (nodeh->ln_Succ) {
		Remove((struct Node *) nodeh);
		onode = nodeh;
		nodeh = nodeh->ln_Succ;
		free(onode);
	}
	return 1;
}

static int getin(void)
{
	int clfd;
	char buffer[256];
	struct gcallerslog gcl;

	onlinestat = 1;
	enterbbs();
	
	if (current_msgbase)
		changemsgbase(current_msgbase->MSGBASE_NUMBER, MC_QUICK | MC_NOSTAT);

	createflaglist();
	user.user_timeremaining = timeleft / 60;
	saveuserbase(&user);
	clog.cl_logoff = time(0);
	snprintf(buffer, sizeof buffer, "%s/logfiles/callerslog%d.dat", 
		origdir, node);
	clfd = open(buffer, O_WRONLY | O_CREAT, 0666);
	if (clfd != -1) {
		fsetperm(clfd, 0666);
		lseek(clfd, 0, SEEK_END);
		safe_write(clfd, &clog, sizeof(struct callerslog));
		(void) close(clfd);
	}
	snprintf(buffer, sizeof buffer, "%s/logfiles/callerslog.dat", 
		origdir);
	clfd = open(buffer, O_WRONLY | O_CREAT, 0666);
	if (clfd != -1) {
		fsetperm(clfd, 0666);
		gcl.cl_node = node;
		gcl.cl = clog;
		lseek(clfd, 0, SEEK_END);
		safe_write(clfd, &gcl, sizeof(struct gcallerslog));
		(void) close(clfd);
	} 
	if (currnode->MULTI_OTHERFLAGS & (1L << 1)) {
		char mesbuf[1024];
		snprintf(mesbuf, sizeof mesbuf, 
			"\n[0m%s from %s logged off from node #%d\n",
			maincfg.CFG_FLAGS & (1L << 1) ? user.user_handle : user.user_realname,
			maincfg.CFG_FLAGS & (1L << 2) ? user.user_organization : user.user_zipcity,
			node);

		olmall(2, mesbuf);
	}
	runlogoffbatch();
	onlinestat = 0;
	return 1;
}

static int createflaglist(void)
{
	char buf[1024];

	struct savedflag sl;
	struct FFlag *myf;
	int fd;

	recountfiles();
	if (!filestagged)
		return 1;

	myf = (struct FFlag *) flaggedfiles->lh_Head;
	if (myf->fhead.ln_Succ) {
		snprintf(buf, sizeof buf, "%s/users/%d/flaggedfiles.dat", 
				origdir, user.user_account_id);
		fd = open(buf, O_WRONLY | O_TRUNC | O_CREAT, 0666);
		if (fd == -1)
			return 0;
		fsetperm(fd, 0666);
		while (myf->fhead.ln_Succ) {
			strlcpy(sl.fname, myf->f_filename, sizeof sl.fname);
			sl.conf = myf->f_conf;
			safe_write(fd, &sl, sizeof(struct savedflag));
			myf = (struct FFlag *) myf->fhead.ln_Succ;
		}
		(void) close(fd);
	}
	return 1;
}

static void carrieroff(int sig)
{
	char buf[80];

	signal(sig, carrieroff);
	if (!carrier)
		return;
	carrier = 0;

	snprintf(buf, sizeof buf, "Carrier lost at %s\n", currt());
	writelog(buf);
	clog.cl_flags |= CL_CARRIERLOST;
}

static int syspw(void)
{
	char b[80];
	int i;

	if (*maincfg.CFG_SYSTEMPW) {
		TypeFile("systempassword", TYPE_MAKE);
		for (i = 2; i; i--) {
			*b = 0;
			DDPut(sd[syspwstr]);
			if (!(Prompt(b, 16, PROMPT_SECRET)))
				return 0;
			if (!strcasecmp(b, maincfg.CFG_SYSTEMPW))
				return 1;
		}
		return 0;
	}
	return 1;
}

int checkcarrier(void)
{
	int kelmas;
	char buf[1024];

	if (lmode)
		return carrier;
	if (!carrier)
		return 0;
	if (fnode) {
		if (ioctl(serhandle, TIOCMGET, &kelmas) < 0) {
			(void) close(serhandle);
			serhandle = open(currnode->MULTI_TTYNAME, O_RDWR | O_NOCTTY);
			return 1;
		}
		if (kelmas & TIOCM_CD)
			return 1;

		snprintf(buf, sizeof buf, "Carrier lost at %s\n", currt());
		writelog(buf);
		clog.cl_flags |= CL_CARRIERLOST;
		carrier = 0;

		return 0;
	} else 
		return ttyname(serhandle) ? 1 : 0;
}

int dropcarrier(void)
{
	if (!checkcarrier())
		return 0;

	if (hupmode) {
		writetomodem(hangupstring);
	} else if (!lmode && !is_telnet_connection()) {
		struct termios tty;
		speed_t sp;

		tcgetattr(serhandle, &tty);
		sp = cfgetospeed(&tty);
		cfsetospeed(&tty, B0);
		tcsetattr(serhandle, TCSANOW, &tty);
		sleep(2);
		cfsetospeed(&tty, sp);
		tcsetattr(serhandle, TCSANOW, &tty);
	}
	carrier = 0;
	return 1;
}
