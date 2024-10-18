#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>

#include <daydream.h>
#include <ddcommon.h>

static int checkforlcfiles(void);
static int copypartial(void);
static int createuserdir(void);
static int loadflaglist(void);

time_t last;
struct List *flaggedfiles;

void enterbbs(void)
{
	char *t2;
	char aikas[40];
	int i, j = 0;
	time_t curr;
	int mmode = 1;
	int selfd;
	char entbuf[200];
	int floodcntr = 0;
	int scanfm;

	changenodestatus("Logging in...");

	snprintf(entbuf, sizeof entbuf, "Login: %s @ %s (Security: %d)\nFrom : %s @ %s\n", user.user_realname, user.user_handle, user.user_securitylevel, user.user_zipcity, user.user_organization);
	writelog(entbuf);

	if ((!(user.user_toggles & (1UL << 30))) && (user.user_toggles & (1UL << 31))) {
		TypeFile("accountfrozen", TYPE_MAKE);
		return;
	}
	if (user.user_connections == 0) {
		writelog("** New User **\n\n");
	} else {
		writelog("\n");
	}

	flaggedfiles = NewList();
	
	if (!getsec()) {
		DDPut(sd[secerrstr]);
		return;
	}
	
	if (currnode->MULTI_OTHERFLAGS & (1L << 1)) {
		char mesbuf[1024];
		snprintf(mesbuf, sizeof mesbuf, "\n[0m%s from %s logged on to node #%d\n",
			maincfg.CFG_FLAGS & (1L << 1) ? user.user_handle : user.user_realname,
			maincfg.CFG_FLAGS & (1L << 2) ? user.user_organization : user.user_zipcity,
			node);

		olmall(2, mesbuf);
	}

	createuserdir();
	setprotocol();

	snprintf(entbuf, sizeof entbuf, 
		"users/%d/selected.dat", user.user_account_id);
	selfd = open(entbuf, O_RDWR);
	if (selfd == -1) {
		selfd = open("data/selected.dat", O_RDONLY);
		if (selfd == -1) {
			DDPut(sd[selerrstr]);
			return;
		}
	}
	read(selfd, &selcfg, 2056);
	close(selfd);

	timeleft = user.user_timeremaining * 60;

	last = user.user_lastcall;
	curr = time(0);
	strlcpy(aikas, ctime(&last), sizeof strlcpy);
	t2 = ctime(&curr);

	for (i = 0; i < 10; i++)
		if (aikas[i] != t2[i])
			j = 1;

	if (j)
		timeleft = user.user_dailytimelimit * 60;

	endtime = curr + timeleft;

	user.user_lastcall = time(0);
	if (user.user_firstcall == 0)
		user.user_firstcall = user.user_lastcall;

	user.user_connections++;
	user.user_failedlogins = 0;

	TypeFile("welcome", TYPE_SEC | TYPE_MAKE | TYPE_WARN);

	rundoorbatch("data/logindoors.dat", 0);

	scanfm = 0;
	if (user.user_toggles & (1L << 12)) {
		DDPut(sd[scanformailstr]);
		if (HotKey(HOT_YESNO) == 1)
			scanfm = 1;
	} else if (!(user.user_toggles & (1L << 5)))
		scanfm = 1;

	if (scanfm)
		scanfornewmail();

	scanfm = 0;

	if (user.user_toggles & (1L << 13)) {
		DDPut(sd[scanforfilesstr]);
		if (HotKey(HOT_YESNO) == 1)
			scanfm = 1;
	} else if (user.user_toggles & (1L << 6))
		scanfm = 1;

	if (scanfm)
		globalnewscan();

	checkforlcfiles();
	checkforpartialuploads(0);
	checkforftp(0, 0, 0);
	loadflaglist();

	if (!joinconf(user.user_joinconference, JC_SHUTUP)) {
		if (!joinconf(maincfg.CFG_JOINIFAUTOJOINFAILS, JC_SHUTUP)) {
			DDPut(sd[joinconferrstr]);
			return;
		}
	}
	floodcntr = maincfg.CFG_FLOODKILLTRIG;

	while (mmode) {
		if ((mmode = domenu(mmode)) == 3) {
			floodcntr = maincfg.CFG_FLOODKILLTRIG;
			DDPut(sd[mmunknowncmd]);
			mmode = 2;
		} else if (mmode == 2) {
			if (!--floodcntr) {
				DDPut(sd[mmfloodkillstr]);
				break;
			}
		} else
			floodcntr = maincfg.CFG_FLOODKILLTRIG;
	}
}

int setprotocol(void)
{
	if (user.user_protocol) {
		struct DayDream_Protocol *pr;
		pr = protocols;
		for (;;) {
			if (pr->PROTOCOL_ID == 0) {
				protocol = protocols;
				user.user_protocol = protocol->PROTOCOL_ID;
				break;
			}
			if (pr->PROTOCOL_ID == user.user_protocol) {
				protocol = pr;
				break;
			}
			pr++;
		}
	} else {
		protocol = protocols;
		user.user_protocol = protocol->PROTOCOL_ID;
	}
	return 1;
}

static int createuserdir(void)
{
	char crbuf[100];

	snprintf(crbuf, sizeof crbuf, "%s/users/%d", 
		origdir, user.user_account_id);
	mkdir(crbuf, 0777);
	setperm(crbuf, 0777);
	snprintf(crbuf, sizeof crbuf, "%s/users/%d/lcfiles", 
		origdir, user.user_account_id);
	mkdir(crbuf, 0777);
	setperm(crbuf, 0777);
	return 1;
}

int checkforpartialuploads(int mode)
{
	int fd;
	char buf[1024];
	char buf2[256];
	snprintf(buf, sizeof buf, "%s/users/%d/badxfer.dat", 
		origdir, user.user_account_id);
	fd = open(buf, O_RDONLY);

	if (fd < 0)
		return 0;
	read(fd, &buf2, 256);
	close(fd);
	ddprintf(sd[partstr], buf2);
	if (!mode) {
		DDPut(sd[res1str]);
	} else {
		DDPut(sd[res2str]);
		switch (HotKey(HOT_YESNO)) {
		case 1:
			copypartial();
			break;
		case 2:
			DDPut(sd[resdelstr]);
			switch (HotKey(HOT_YESNO)) {
			case 1:
				snprintf(buf, sizeof buf,
					"%s/users/%d/badxfer.dat", origdir, 
					user.user_account_id);
				unlink(buf);
				break;
			case 0:
			case -1:
				return 0;
			}
			break;
		case 0:
		case -1:
			return 0;
		}
	}
	return 1;
}

int checkforftp(int mode, const char *dp, const char *log)
{
	FILE *logf = 0;
	int fcount = 0;
	struct dirent *dent;
	DIR *dh;
	char buf[1024];
	char buf2[1024];

	snprintf(buf, sizeof buf, "%s/users/%d/ftp/ul",
		origdir, user.user_account_id);

	if ((dh = opendir(buf))) {
		if (mode == 2) {
			logf = fopen(log, "w");
		}
		while ((dent = readdir(dh))) {
			if (dent->d_name[0] == '.' && (dent->d_name[1] == '\0' || (dent->d_name[1] == '.' && dent->d_name[2] == '\0')))
				continue;
			if (mode == 2) {
				struct stat st;

				snprintf(buf, sizeof buf,
					"%s/users/%d/ftp/ul/%s", origdir, 
					user.user_account_id, dent->d_name);
				snprintf(buf2, sizeof buf2, "%s/%s", dp, 
					dent->d_name);
				stat(buf, &st);
				newrename(buf, buf2);
				if (logf)
					fprintf(logf,
						"S %6d %5d bps %4d cps   0 errors     0 %4d %s %u\n",
						(int) st.st_size, 0, 0, 1024, dent->d_name, 0);
			}
			fcount++;
		}
		if (logf)
			fclose(logf);
		closedir(dh);
	}
	if (fcount) {
		if (!mode) {
			DDPut(sd[ftp1str]);
		} else if (mode == 1) {
			DDPut(sd[ftp2str]);
			if (HotKey(HOT_YESNO) == 1) {
				checkforftp(2, dp, log);
				return 1;
			}
		}
	}
	return 0;
}


static int copypartial(void)
{
	int fd, fd2;
	char buf[1024];
	char buf2[256];
	char cbuf[60000];
	int i;

	snprintf(buf, sizeof buf, "%s/users/%d/badxfer.dat", origdir, 
		user.user_account_id);
	fd = open(buf, O_RDONLY);
	if (fd == 0)
		return 0;
	read(fd, &buf2, 256);

	snprintf(buf, sizeof buf, "%s/%s", currnode->MULTI_TEMPORARY, buf2);

	if ((fd2 = open(buf, O_WRONLY | O_CREAT, 0666)) == -1)
		return 0;
	fsetperm(fd2, 0666);
	while ((i = read(fd, &cbuf, 60000))) {
		safe_write(fd2, cbuf, i);
	}
	close(fd);
	close(fd2);
	snprintf(buf, sizeof buf, "%s/users/%d/badxfer.dat", 
		origdir, user.user_account_id);
	unlink(buf);
	return 1;
}

static int checkforlcfiles(void)
{
	int fd;
	char buf[1024];
	struct lcfile lc;
	int oldc;

	oldc = user.user_joinconference;

	snprintf(buf, sizeof buf, "%s/users/%d/lcfiles.dat", 	
		origdir, user.user_account_id);
	fd = open(buf, O_RDONLY);
	if (fd < 0)
		return 0;

	DDPut(sd[lcfstr]);

	while (read(fd, &lc, sizeof(struct lcfile))) {
		char buf1[1024];
		char buf2[1024];

		snprintf(buf1, sizeof buf1, "%s/%s", 
			currnode->MULTI_TEMPORARY, lc.lc_name);
		snprintf(buf2, sizeof buf2, "%s/users/%d/lcfiles/%s", 
			origdir, user.user_account_id, lc.lc_name);
		newrename(buf2, buf1);
	}
	close(fd);
	unlink(buf);
	joinconf(lc.lc_conf, JC_QUICK | JC_SHUTUP | JC_NOUPDATE);
	upload(2);
	joinconf(oldc, JC_QUICK | JC_SHUTUP | JC_NOUPDATE);
	return 1;
}

static int loadflaglist(void)
{
	char buf[1024];
	struct savedflag sl;
	int fd;
	int res;
	int tot = 0;
	recountfiles();

	snprintf(buf, sizeof buf, "%s/users/%d/flaggedfiles.dat", 
		origdir, user.user_account_id);
	fd = open(buf, O_RDONLY);
	if (fd < 0)
		return 1;
	DDPut(sd[alreadyflagstr]);

	while (read(fd, &sl, sizeof(struct savedflag))) {
		if (joinconf(sl.conf, JC_SHUTUP | JC_QUICK | JC_NOUPDATE)) {
			res = flagsingle(sl.fname, 0);
			if (res == 0 || res == 4)
				tot++;
		}
	}
	close(fd);
	unlink(buf);
	ddprintf(sd[alrdonestr], tot);
	return 1;
}
