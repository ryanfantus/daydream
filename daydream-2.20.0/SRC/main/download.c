#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <daydream.h>
#include <ddcommon.h>

static int ftpdl(char *, char *);
static int makepartial(char *);
static int sflagfile(char *);
static void typedlprompt(void);

int download(const char *params)
{
	char parbuf[512];
	char bigbuf[10000];
	const char *srcstrh;
	int discon = 0;
	struct FFlag *myf;
	FILE *listh;
	char lastfile[100];
	int keepc = 1;

	bgrun = 0;
	wasbg = 0;

	setprotocol();
	changenodestatus("Downloading");
	TypeFile("download", TYPE_MAKE | TYPE_CONF | TYPE_WARN);

	if (!conference()->conf.CONF_FILEAREAS) {
		DDPut(sd[dlnoareasstr]);
		return 0;
	}
	if ((protocol->PROTOCOL_TYPE == 3 || protocol->PROTOCOL_TYPE == 2) && !conference()->conf.CONF_UPLOADAREA) {
		DDPut(sd[dlnouploadsstr]);
		return 0;
	}
	if (protocol->PROTOCOL_TYPE == 2 || protocol->PROTOCOL_TYPE == 3) {
		if (cleantemp() == -1) {
			DDPut(sd[tempcleanerrstr]);
			return 0;
		}
		if (!freespace())
			return 0;
		maketmplist();
	}
	srcstrh = params;

	for (;;) {
		if (strtoken(parbuf, &srcstrh, sizeof parbuf) > sizeof parbuf)
			continue;
		if (!*parbuf)
			break;
		flagfile(parbuf, 1);
	}
	for (;;) {
		typedlprompt();
		bigbuf[0] = 0;
		if (!(Prompt(bigbuf, 200, 0)))
			return 0;
		if (!bigbuf[0]) {
			break;
		} else if (!strcasecmp(bigbuf, "a")) {
			return 0;
		} else {
			srcstrh = bigbuf;
			for (;;) {
				if (strtoken(parbuf, &srcstrh, 
					     sizeof parbuf) > sizeof parbuf)
					continue;
				if (!*parbuf)
					break;
				flagfile(parbuf, 1);
			}
		}
	}
	if (!filestagged)
		return 0;
	listtags();
	if (estimsecs(bytestagged) > timeleft) {
		DDPut(sd[dlnotimestr]);
		return 0;
	}
	for (;;) {
		DDPut(sd[dlproceedstr]);
		bigbuf[0] = 0;
		if (!(Prompt(bigbuf, 3, 0)))
			return 0;
		if (!bigbuf[0] || bigbuf[0] == 'p' || bigbuf[0] == 'P')
			break;
		else if (bigbuf[0] == 'e' || bigbuf[0] == 'E') {
			taged(0);
		} else if (bigbuf[0] == 'd' || bigbuf[0] == 'D') {
			discon = 1;
			break;
		} else if (bigbuf[0] == 'a' || bigbuf[0] == 'A') {
			return 0;
		}
	}
	snprintf(parbuf, sizeof parbuf, "%s/dszlog.%d", DDTMP, node);

	sprintf(&parbuf[250], "%s/ddfilelist.%d", DDTMP, node);
	unlink(&parbuf[250]);

	if (!(listh = fopen(&parbuf[250], "w")))
		return 0;

	myf = (struct FFlag *) flaggedfiles->lh_Head;
	while (myf->fhead.ln_Succ) {
		char tbu[256];
		snprintf(tbu, sizeof tbu, "%s%s\n", 
			myf->f_path, myf->f_filename);
		fputs(tbu, listh);
		myf = (struct FFlag *) myf->fhead.ln_Succ;
	}
	fclose(listh);
	*lastfile = 0;

	if (protocol->PROTOCOL_TYPE == 2 || protocol->PROTOCOL_TYPE == 3) {
		if ((!(user.user_toggles & (1L << 15))) && (maincfg.CFG_FLAGS & (1L << 11))) {
			initbgchecker();

		}
	}
	sendfiles(&parbuf[250], lastfile, sizeof lastfile);


	if (protocol->PROTOCOL_TYPE == 2 || protocol->PROTOCOL_TYPE == 3) {
		upload(2);
	}
	if (*lastfile) {
		myf = (struct FFlag *) flaggedfiles->lh_Head;
		while (myf->fhead.ln_Succ && keepc) {
			struct FFlag *oldf;
			struct DD_DownloadLog ddl;
			char lbuf[100];
			int logfd;

			snprintf(lbuf, sizeof lbuf, 
				"%s/logfiles/downloadlog.dat", origdir);
			logfd = open(lbuf, O_WRONLY | O_CREAT, 0666);
			if (logfd != -1) {
				fsetperm(logfd, 0666);
				memset((char *) &ddl, 0, sizeof(struct DD_DownloadLog));
				ddl.DL_SLOT = user.user_account_id;
				strlcpy(ddl.DL_FILENAME, myf->f_filename, sizeof ddl.DL_FILENAME);
				ddl.DL_FILESIZE = myf->f_size;
				ddl.DL_TIME = time(0);
				ddl.DL_BPSRATE = bpsrate;
				ddl.DL_NODE = node;
				ddl.DL_CONF = (unsigned char) myf->f_conf;
				lseek(logfd, 0, SEEK_END);
				safe_write(logfd, &ddl, sizeof(struct DD_DownloadLog));
				close(logfd);
			}
			if (!(myf->f_flags & FLAG_FREE)) {
				user.user_dlbytes += myf->f_size;
				user.user_dlfiles++;
			}
			if (!strcasecmp(lastfile, myf->f_filename))
				keepc = 0;
			Remove((struct Node *) myf);
			oldf = myf;
			myf = (struct FFlag *) myf->fhead.ln_Succ;
			free(oldf);
		}
	}
	recountfiles();

	unlink(&parbuf[250]);

	if (discon) {
		if (autodisconnect())
			return 2;
	}
	return 1;
}

int sysopdownload(const char *params)
{
	const char *srcstrh;
	char parbuf[1024];
	char bigbuf[4096];
	int discon = 0;
	struct FFlag *myf;
	FILE *listh;
	char lastfile[100];
	int keepc = 1;

	changenodestatus("SysOp download");
	TypeFile("sysopdownload", TYPE_MAKE);

	srcstrh = params;

	for (;;) {
		if (strtoken(parbuf, &srcstrh, sizeof parbuf) > sizeof parbuf)
			continue;
		if (!*parbuf)
			break;
		sflagfile(parbuf);
	}
	for (;;) {
		typedlprompt();
		bigbuf[0] = 0;
		if (!(Prompt(bigbuf, 200, 0)))
			return 0;
		if (!bigbuf[0]) {
			break;
		} else if (!strcasecmp(bigbuf, "a")) {
			return 0;
		} else {
			srcstrh = bigbuf;
			for (;;) {
				if (strtoken(parbuf, &srcstrh,
					     sizeof parbuf) > sizeof parbuf)
					continue;
				if (!*parbuf)
					break;
				sflagfile(parbuf);
			}
		}
	}
	if (!filestagged)
		return 0;
	listtags();
	if (estimsecs(bytestagged) > timeleft) {
		DDPut(sd[dlnotimestr]);
		return 0;
	}
	for (;;) {
		DDPut(sd[dlproceedstr]);
		bigbuf[0] = 0;
		if (!(Prompt(bigbuf, 3, 0)))
			return 0;
		if (!bigbuf[0] || bigbuf[0] == 'p' || bigbuf[0] == 'P')
			break;
		else if (bigbuf[0] == 'e' || bigbuf[0] == 'E') {
			taged(0);
		} else if (bigbuf[0] == 'd' || bigbuf[0] == 'D') {
			discon = 1;
			break;
		} else if (bigbuf[0] == 'a' || bigbuf[0] == 'A') {
			return 0;
		}
	}

	if (estimsecs(bytestagged) > timeleft) {
		DDPut(sd[dlnotimestr]);
		return 0;
	}
	snprintf(parbuf, sizeof parbuf - 250, "%s/dszlog.%d", DDTMP, node);

	snprintf(&parbuf[250], sizeof parbuf - 250, "%s/ddfilelist.%d", DDTMP, node);
	unlink(&parbuf[250]);

	if (!(listh = fopen(&parbuf[250], "w")))
		return 0;

	myf = (struct FFlag *) flaggedfiles->lh_Head;
	while (myf->fhead.ln_Succ) {
		char tbu[256];
		snprintf(tbu, sizeof tbu, "%s%s\n", 
			myf->f_path, myf->f_filename);
		fputs(tbu, listh);
		myf = (struct FFlag *) myf->fhead.ln_Succ;
	}
	fclose(listh);
	*lastfile = 0;
	sendfiles(&parbuf[250], lastfile, sizeof lastfile);

	if (*lastfile) {
		myf = (struct FFlag *) flaggedfiles->lh_Head;
		while (myf->fhead.ln_Succ && keepc) {
			struct FFlag *oldf;

			if (!strcasecmp(lastfile, myf->f_filename))
				keepc = 0;
			Remove((struct Node *) myf);
			oldf = myf;
			myf = (struct FFlag *) myf->fhead.ln_Succ;
			free(oldf);
		}
	}
	recountfiles();

	unlink(&parbuf[250]);

	if (protocol->PROTOCOL_TYPE == 2 || protocol->PROTOCOL_TYPE == 3) {
		upload(2);
	}
	if (discon) {
		if (autodisconnect())
			return 2;
	}
	return 1;

}

static int sflagfile(char *file)
{
	struct stat st;
	char fpath[1024];
	const char *s;

	memset(fpath, 0, 1024);
	if (stat(file, &st) == -1)
		return 0;
	s = filepart(file);
	strncpy(fpath, file, s - file);

	if (!S_ISREG(st.st_mode))
		return 0;

	return flagres(addtag(fpath, s, 0, st.st_size, FLAG_FREE), s, st.st_size);

}

int autodisconnect(void)
{
	int count;
	char autodb[80];
	int i;

	DDPut(sd[dlautodcstr]);

	for (count = 9; count; count--) {
		sleep(1);

		i = HotKey(HOT_QUICK);

		if (i == 3) {
			DDPut("\n\n");
			return 0;
		}
		
		if (!checkcarrier())
			return 0;

		ddprintf("%2.2d", count);
	}
	DDPut("\n\n");
	snprintf(autodb, sizeof autodb, 
		"Connection closed by automatic disconnection at %s\n", 
		currt());
	writelog(autodb);
	dropcarrier();
	return 1;
}

int makeflist(char *listn, char *list)
{
	const char *s;
	char buf[1024];
	FILE *lh;
	int cnt = 0;

	if ((lh = fopen(listn, "w"))) {
		s = list;
		for (;;) {
			int fd;
			
			if (strtoken(buf, &s, sizeof buf) > sizeof buf)
				continue;
			if (!*buf)
				break;

			fd = open(buf, O_RDONLY);
			if (fd == -1)
				continue;
			close(fd);
			cnt++;
			fprintf(lh, "%s\n", buf);
		}
		fclose(lh);
		return cnt;
	} else {
		return 0;
	}
}

int sendfiles(char *list, char *lastf, size_t flen)
{
	char parbuf[150];
	char bigbuf[4002];

	snprintf(parbuf, sizeof parbuf, "%s/dszlog.%d", DDTMP, node);
	unlink(parbuf);

	if (protocol->PROTOCOL_TYPE == 1) {
		snprintf(bigbuf, sizeof bigbuf, "%s/utils/ddsz -vv -r -b -g %s -H %s -@ %s -I %s/nodeinfo%d.data", origdir, ttyname(serhandle), parbuf, list, DDTMP, node);
		runstdio(bigbuf, -1, 2);
	} 
        else if (protocol->PROTOCOL_TYPE == 4) {
		ftpdl(list, parbuf);
	}

	analyzedszlog(parbuf, lastf, flen);

	snprintf(parbuf, sizeof parbuf, "%s/dszlog.%d", DDTMP, node);
	unlink(parbuf);
	return 1;
}

int analyzedszlog(char *log, char *lastf, size_t lastf_len)
{
	char parbuf[150];
	char bigbuf[4002];
	FILE *listh;
	char mode;

	if (!(listh = fopen(log, "r")))
		return 0;

	while (fgetsnolf(bigbuf, 4000, listh)) {
		char fnam[1024];
		int fsize;
		int cps;
		char *s;
		char *t;

		if (*bigbuf == 'H' || *bigbuf == 's')
			mode = 'D';
		else if (*bigbuf == 'S' || *bigbuf == 'R')
			mode = 'U';
		else if (*bigbuf == 'E')
			mode = 'E';
		else
			continue;

		s = &bigbuf[2];
		t = parbuf;

		while (*s == ' ')
			s++;
		while (*s != ' ')
			*t++ = *s++;
		*t = 0;
		fsize = atoi(parbuf);

		while (*s == ' ')
			s++;
		while (*s != ' ')
			s++;

		while (*s == ' ')
			s++;
		while (*s != ' ')
			s++;

		t = parbuf;
		while (*s == ' ')
			s++;
		while (*s != ' ')
			*t++ = *s++;
		*t = 0;
		cps = atoi(parbuf);

		s = &s[4];
		while (*s == ' ')
			s++;
		while (*s != ' ')
			s++;

		s = &s[7];

		while (*s == ' ')
			s++;
		while (*s != ' ')
			s++;

		while (*s == ' ')
			s++;
		while (*s != ' ')
			s++;

		t = fnam;

		while (*s == ' ')
			s++;
		while (*s != ' ')
			*t++ = *s++;
		*t = 0;

		if (mode == 'U') {
			clog.cl_ulbytes += fsize;
			clog.cl_ulfiles++;
		} else if (mode == 'D') {
			clog.cl_dlbytes += fsize;
			clog.cl_dlfiles++;
		} else {
			makepartial(fnam);
			continue;
		}

		snprintf(parbuf, sizeof parbuf,
			"%cLoad: %s, %d bytes, average of %d CPS\n", 
			mode, fnam, fsize, cps);
		writelog(parbuf);

		if (mode == 'D' && lastf)
			strlcpy(lastf, fnam, lastf_len);
	}
	fclose(listh);
	return 1;
}

static int makepartial(char *fn)
{
	char buf[1024];
	char goobuf[60000];

	int fd, fd2;
	int i;

	fd = open(fn, O_RDONLY);
	if (fd < 0)
		return 0;

	snprintf(buf, sizeof buf, "%s/users/%d/badxfer.dat", 
		origdir, user.user_account_id);
	fd2 = open(buf, O_WRONLY | O_CREAT, 0666);
	if (fd2 == -1) {
		close(fd);
		snprintf(buf, sizeof buf, "%s/%s", 
			currnode->MULTI_TEMPORARY, fn);
		unlink(buf);
		return 0;
	}
	fsetperm(fd2, 0666);
	strlcpy(buf, filepart(fn), sizeof buf);
	safe_write(fd2, buf, 256);
	while ((i = read(fd, &goobuf, 60000))) {
		safe_write(fd2, goobuf, i);
	}
	close(fd);
	close(fd2);
	unlink(fn);
	return 1;
}

static void typedlprompt(void)
{
	char buffer1[256];
	char buffer2[256];

	recountfiles();
	freefstr(buffer1, sizeof buffer1);
	freebstr(buffer2, sizeof buffer2);
	ddprintf(sd[dlpromptstr], filestagged, buffer1, bytestagged, buffer2);
}

void freefstr(char *db, size_t len)
{
	if (user.user_fileratio)
		snprintf(db, len, "%d", user.user_fileratio * user.user_ulfiles + user.user_freedlfiles - user.user_dlfiles - filestagged + ffilestagged);
	else
		strlcpy(db, sd[dlunlimitedstr], len);
}

void freebstr(char *db, size_t len)
{
	if (user.user_byteratio)
		snprintf(db, len, "%d", (int) (user.user_byteratio * user.user_ulbytes + user.user_freedlbytes - user.user_dlbytes - bytestagged + fbytestagged));
	else
		strlcpy(db, sd[dlunlimitedstr], len);
}

void killflood(void)
{
	DDPut(sd[dlkillfloodstr]);
	delayt = 3;
	for (;;) {
		int i;
		if (!checkcarrier())
			break;
		i = HotKey(HOT_DELAY);
		if (!checkcarrier())
			break;
		if (i == 0 || i == 255 || i == 'y' || i == 'Y')
			break;
	}
	DDPut("\n");
}

static int ftpdl(char *list, char *loog)
{
	FILE *listh;
	FILE *logh;

	char buf[1024];
	char buf2[1024];

	snprintf(buf2, sizeof buf2, "%s/users/%d/ftp",
		origdir, user.user_account_id);
	mkdir(buf2, 0777);
	setperm(buf2, 0777);
	snprintf(buf2, sizeof buf2, "%s/users/%d/ftp/dl",
		origdir, user.user_account_id);
	mkdir(buf2, 0777);
	setperm(buf2, 0777);

	if (!(listh = fopen(list, "r")))
		return 0;

	logh = fopen(loog, "w");

	while (fgetsnolf(buf, 1024, listh)) {
		snprintf(buf2, sizeof buf2, "%s/users/%d/ftp/dl/%s",
			origdir, user.user_account_id, filepart(buf));
		//symlink(buf, buf2);
		link(buf, buf2);
		if (logh) {
			struct stat st;
			stat(buf, &st);
			fprintf(logh,
				"s %6d     0 bps    0 cps   0 errors     0 1024 %s 0\n",
				(int) st.st_size, filepart(buf));

		}
	}
	fclose(listh);
	if (logh)
		fclose(logh);
	return 1;
}

