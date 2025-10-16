#include <ddlib.h>
#include <dd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>

#include <ddcommon.h>

static void die(void);
static void nuketmp(void);
static void CpyToLAINA(char *, char *);
static char *ecfg(char *hay, char *need);
static int makeqwk(void);
static int getqwk(void);
static int maketmp(void);
static int findtmpnc(char *, char *);
static int get_conf_virt(int);
static void skipmsg(int, char *);
static void mystrcpy(char *, char *, int);
static void removespaces(char *);
static int myjoin(int);
static int get_virtual_conf(int, int);
static int procqwkm(struct DayDream_Message *);
static void setlrps(void);
static int inttoms(int, char[4]);
static char *fgetsnolf(char *, int, FILE *) __attr_bounded__ (__string__, 1, 2);

static struct dif *d;
static char *cfg;
static int node;

static unsigned short mcount = 0;

static struct DayDream_MainConfig maincfg;
static char bbsid[20];
static char unzip[128];
static struct DayDream_LRP lp;
static struct DayDream_MsgPointers mp;
static struct DayDream_Conference *confs;
static struct DayDream_Conference *conf;
static struct DayDream_MsgBase *base;
static FILE *msgdfd;
static int ndxfd;

static int blk_cnt = 2;
static int cconf;


static int qwkcom(char *buf)
{
	if (toupper(*buf) == 'D') {
		makeqwk();
	} else if (toupper(*buf) == 'U') {
		getqwk();
	} else {
		return 0;
	}
	return 1;
}

int main(int argc, char *argv[])
{
	char buf[1024];
	int i;
	struct stat st;
	int cfgfd;
	char *cptr;

	if (argc == 1) {
		printf("This program requires MS Windows!\n");
		exit(1);
	}
	d = dd_initdoor(argv[1]);
	if (d == 0) {
		printf("Couldn't find socket!\n");
		exit(1);
	}
	cconf = dd_getintval(d, VCONF_NUMBER);
	dd_changestatus(d, "Grabbing messages");

	atexit(die);
	dd_sendstring(d, "\n[35mDD-Grab " versionstring " by Antti H�yrynen.\n\n");
	node = atoi(argv[1]);

	sprintf(buf, "%s/configs/dd-grab.cfg", getenv("DAYDREAM"));

	if (stat(buf, &st) == -1) {
		dd_sendstring(d, "[37mCouldn't find configfile!\n\n");
		exit(0);
	}
	cfg = (char *) malloc(st.st_size + 2);

	cfgfd = open(buf, O_RDONLY);
	i = read(cfgfd, cfg, st.st_size);
	close(cfgfd);
	cfg[i] = 0;

	if ((cptr = ecfg(cfg, "BBSID \""))) {
		CpyToLAINA(cptr, bbsid);
	} else
		exit(0);

	if ((cptr = ecfg(cfg, "UNZIP \""))) {
		CpyToLAINA(cptr, unzip);
	} else {
		strcpy(unzip, "unzip -jqq");
	}

	sprintf(buf, "%s/data/daydream.dat", getenv("DAYDREAM"));
	cfgfd = open(buf, O_RDONLY);
	read(cfgfd, &maincfg, sizeof(struct DayDream_MainConfig));
	close(cfgfd);

	*buf = 0;
	dd_getstrval(d, buf, DOOR_PARAMS);
	if (*buf && qwkcom(buf))
		exit(0);

	dd_sendstring(d, "[36mDo you want to [0mD[34m)[36mownload or [0mU[34m)[36mpload QWK packet?: [0m");
	if (!(dd_prompt(d, buf, 3, 0)))
		exit(0);
	qwkcom(buf);
	
	return 0;
}

static int getqwk(void)
{
	char qbuf[1024];
	char tname[1024];
	char olddir[1024];
	int msgfd;
	int bolloks;

	char qwkblock[128];
	int repfd;

	char* ptr;

	if (!maketmp())
		return 0;
	atexit(nuketmp);

	confs = (struct DayDream_Conference *) dd_getconfdata();
	if (!confs)
		return 0;

	sprintf(qbuf, "%s/dd-grab%d/", DDTMP, node);
	dd_getfiles(d, qbuf);

	getcwd(olddir, 1024);
	sprintf(qbuf, "%s/dd-grab%d/", DDTMP, node);
	chdir(qbuf);
	sprintf(qbuf, "%s.rep", bbsid);
	if (findtmpnc(qbuf, tname)) {
		sprintf(qbuf, "%s %s </dev/null >/dev/null 2>/dev/null", unzip, tname);
		system(qbuf);
	}
	sprintf(qbuf, "%s.msg", bbsid);
	if (findtmpnc(qbuf, tname)) {
		repfd = open(tname, O_RDONLY);
		if (repfd > -1) {
			read(repfd, &qwkblock, 128);
			if (!strncasecmp(qwkblock, bbsid, strlen(bbsid))) {
				while (read(repfd, &qwkblock, 128)) {
					int confn;
					struct DayDream_Message msg;
					char usr[80];
					char receiver[80];
					char ebuf[1024];

					confn = atoi(&qwkblock[1]);
					memset(&msg, 0, sizeof(struct DayDream_Message));
					if (!get_conf_virt(confn)) {
						dd_sendstring(d, "Unknown conf!\n");
						skipmsg(repfd, qwkblock);
						continue;
					}
					if (!dd_joinconf(d, conf->CONF_NUMBER, JC_QUICK | JC_SHUTUP)) {
						skipmsg(repfd, qwkblock);
						continue;
					}
					dd_changemsgbase(d, base->MSGBASE_NUMBER, MC_QUICK | MC_NOSTAT);
					if (base->MSGBASE_FLAGS & (1L << 2)) {
						dd_getstrval(d, usr, USER_HANDLE);
					} else {
						dd_getstrval(d, usr, USER_REALNAME);
					}
					strcpy(msg.MSG_AUTHOR, usr);
					mystrcpy(receiver, &qwkblock[21], 25);
					if (!strcasecmp(receiver, "All")) {
						*msg.MSG_RECEIVER = 0;
					} else if (!strcasecmp(receiver, "EAll")) {
						*msg.MSG_RECEIVER = 255;
					} else {
						strncpy(msg.MSG_RECEIVER, receiver, 25);
					}
					mystrcpy(msg.MSG_SUBJECT, &qwkblock[71], 25);
					msg.MSG_CREATION = time(0);
					if (*qwkblock == ' ' || *qwkblock == '-' || base->MSGBASE_FLAGS & (1L << 1)) {

					} else {
						msg.MSG_FLAGS |= (1L << 0);
					}
					dd_getmprs(d, &mp);
					mp.msp_high++;
					msg.MSG_NUMBER = mp.msp_high;
					dd_setmprs(d, &mp);

					msgfd = ddmsg_open_base(conf->CONF_PATH, base->MSGBASE_NUMBER, O_RDWR|O_CREAT, 0664);

					if (msgfd < 0) {
						skipmsg(repfd, qwkblock);
					}
					lseek(msgfd, 0, SEEK_END);
					if (toupper(base->MSGBASE_FN_FLAGS) == 'E') {
						msg.MSG_FN_ORIG_ZONE = base->MSGBASE_FN_ZONE;
						msg.MSG_FN_ORIG_NET = base->MSGBASE_FN_NET;
						msg.MSG_FN_ORIG_NODE = base->MSGBASE_FN_NODE;
						msg.MSG_FN_ORIG_POINT = base->MSGBASE_FN_POINT;
						msg.MSG_FLAGS |= (1L << 2);
					}
					write(msgfd, &msg, sizeof(struct DayDream_Message));
					ddmsg_close_base(msgfd);

					msgfd = ddmsg_open_msg(conf->CONF_PATH, base->MSGBASE_NUMBER, msg.MSG_NUMBER, O_CREAT|O_WRONLY, 0644);

					if (toupper(base->MSGBASE_FN_FLAGS) == 'E') {
						char ub[128];
						char ebuf[1024];
						int uq;

						strcpy(ub, base->MSGBASE_FN_TAG);
						strupr(ub);
						sprintf(ebuf, "AREA:%s\n", ub);
						write(msgfd, ebuf, strlen(ebuf));
						if ((uq = dd_getfidounique())) {
							sprintf(ebuf, "\001MSGID: %d:%d/%d.%d %8.8x\n", base->MSGBASE_FN_ZONE, base->MSGBASE_FN_NET, base->MSGBASE_FN_NODE, base->MSGBASE_FN_POINT, uq);
							write(msgfd, ebuf, strlen(ebuf));
						}
					}
					bolloks = atoi(&qwkblock[116]) - 1;
					while (bolloks--) {
						unsigned char qwb[130];
						int i;

						read(repfd, &qwb, 128);

						for (i = 0; i < 129; i++)
							if (qwb[i] == 227)
								qwb[i] = 10;

						if (toupper(base->MSGBASE_FN_FLAGS) == 'E') {
							if((ptr = strstr((char*) qwb, "\n--- "))) {
								/* Replace the existing tearline */
								memcpy(ptr+1, "___", 3); 
							}
						}

						if (!bolloks) {
							qwb[128] = 0;
							removespaces((char*) qwb);
							write(msgfd, &qwb, strlen((char*) qwb));
						} 
						else
							write(msgfd, &qwb, 128);
					}
					if (toupper(base->MSGBASE_FN_FLAGS) == 'E') {
						sprintf(ebuf, "\n--- DayDream BBS/Linux %s (Grab QWK Door)\n * Origin: %s (%d:%d/%d)\nSEEN-BY: %d/%d\n", versionstring, base->MSGBASE_FN_ORIGIN, base->MSGBASE_FN_ZONE, base->MSGBASE_FN_NET, base->MSGBASE_FN_NODE, base->MSGBASE_FN_NET, base->MSGBASE_FN_NODE);
						write(msgfd, ebuf, strlen(ebuf));
					}
					ddmsg_close_msg(msgfd);
					sprintf(qbuf, "[36mMessage posted to [0m%s[34m/[0m%s[36m.\n", conf->CONF_NAME, base->MSGBASE_NAME);
					dd_sendstring(d, qbuf);
				}
			}
		}
	} else {
		dd_sendstring(d, "\n[35mCan't find message packet!\n\n");
	}
	chdir(olddir);
	return 1;
}

static void mystrcpy(char *d, char *s, int l)
{
	char* tmp = 0;
	
	strncpy(d, s, l);
	
	tmp = d + (l - 1);
	
	while(*tmp == ' ' && tmp != d) {
	    *tmp = 0;
	    tmp--;
	}
	
	if(tmp == d) {
	    *(tmp) = 0;
	}
	else {
	    *(tmp + 1) = 0;
	}
}

static void removespaces(char *strh)
{
	char *s;
	s = strh;
	if (*s == 0) {
		return;
	}
	while (*s != 0) {
		s++;
	}
	s--;
	while (*s == ' ') {
		s--;
	}
	*(s + 1) = 0;
}

static void skipmsg(int fd, char *qbl)
{
	lseek(fd, (atoi(&qbl[116]) - 1) * 128, SEEK_CUR);
}

static int findtmpnc(char *file, char *dname)
{
	char tbuf[1024];
	DIR *dh;
	struct dirent *dent;

	sprintf(tbuf, "%s/dd-grab%d/", DDTMP, node);

	if ((dh = opendir(tbuf))) {
		while ((dent = readdir(dh))) {
			if (!strcmp(dent->d_name, ".") || (!strcmp(dent->d_name, "..")))
				continue;
			if (!strcasecmp(dent->d_name, file)) {

				closedir(dh);
				strcpy(dname, dent->d_name);
				return 1;
			}
		}
		closedir(dh);
	}
	return 0;
}

static int makeqwk(void)
{
	char qbuf[1024];
	time_t t;
	struct tm *tm;
	char username[80];
	FILE *ctrl, *doorid;
	int ccount = 0;
	int vc = 0;

	t = time(0);
	tm = localtime(&t);

	dd_getstrval(d, username, USER_HANDLE);


	if (!maketmp())
		return 0;
	atexit(nuketmp);

	confs = (struct DayDream_Conference *) dd_getconfdata();
	if (!confs)
		return 0;
	conf = confs;

	sprintf(qbuf, "%s/dd-grab%d/messages.dat", DDTMP, node);
	msgdfd = fopen(qbuf, "w");
	if (msgdfd == 0)
		return 0;

	fprintf(msgdfd, "%-128.128s", "Produced by Qmail...Copyright (c) 1987 by Sparkware.  All rights Reserved");

	sprintf(qbuf, "%s/dd-grab%d/door.id", DDTMP, node);

	doorid = fopen(qbuf, "w");

	fprintf(doorid, "DOOR = Grab QWK\r\n");
	fprintf(doorid, "VERSION = 0.1\r\n");
	fprintf(doorid, "SYSTEM = Daydream BBS/Linux %s\r\n", versionstring);
	fprintf(doorid, "MIXEDCASE = YES\r\n");

	fclose(doorid);

	sprintf(qbuf, "%s/dd-grab%d/control.dat", DDTMP, node);
	ctrl = fopen(qbuf, "w");

	fprintf(ctrl, "%s\r\n", maincfg.CFG_BOARDNAME);
	fprintf(ctrl, "%s\r\n", "Location");
	fprintf(ctrl, "%s\r\n", "Phone");
	fprintf(ctrl, "%s\r\n", maincfg.CFG_SYSOPNAME);

	fprintf(ctrl, "00000,%s\r\n", bbsid);
	fprintf(ctrl, "%-2.2d-%2.2d-%-4.4d,%-2.2d:%-2.2d:%-2.2d\r\n",tm->tm_mon + 1, 
																															 tm->tm_mday, 
																															 1900 + tm->tm_year, 
																															 tm->tm_hour, 
																															 tm->tm_min, 
																															 tm->tm_sec);

	fprintf(ctrl, "%s\r\n", username);
	fprintf(ctrl, " \r\n");
	fprintf(ctrl, "0\r\n");

	dd_sendstring(d, "\n[35mPacking new messages\n\nConference                           Message Base          Progress\n[34m---------------------------------------------------------------------------\n");

	for (;;) {
		int newcnt;
		struct stat st;
		char mailbuf[1024];
		int bcnt;

		if (conf->CONF_NUMBER == 255)
			break;


		if (myjoin(conf->CONF_NUMBER)) {
			int confv = 0;

			/* FIXME: Very suspicious code */
			base = (struct DayDream_MsgBase *) conf + 1;
			for (bcnt = conf->CONF_MSGBASES; bcnt; bcnt--, base++) {
				int basefd;
				char *cn;
				newcnt = 0;
				ccount++;
				dd_changemsgbase(d, base->MSGBASE_NUMBER, MC_QUICK | MC_NOSTAT);
				if (dd_isbasetagged(d, conf->CONF_NUMBER, base->MSGBASE_NUMBER)) {

					newcnt = 0;


					if (!confv) {
						cn = conf->CONF_NAME;
						confv = 1;
					} else {
						cn = " ";
					}
					sprintf(mailbuf, "[33m%-37.37s[32m%-21.21s[36m ", cn, base->MSGBASE_NAME);
					dd_sendstring(d, mailbuf);
					dd_getmprs(d, &mp);
					dd_getlprs(d, &lp);

					if (!*base->MSGBASE_FN_TAG) {
						dd_sendstring(d, "[31mNo tag!\n");
						continue;
					}
					sprintf(qbuf, "%s/dd-grab%d/%3.3d.ndx", DDTMP, node, get_virtual_conf(conf->CONF_NUMBER, base->MSGBASE_NUMBER));
					ndxfd = open(qbuf, O_TRUNC | O_CREAT | O_WRONLY, 0644);
					if (mp.msp_high > lp.lrp_read) {
						struct DayDream_Message msg;
						int seekp;

						basefd = ddmsg_open_base(conf->CONF_PATH, base->MSGBASE_NUMBER, O_RDONLY, 0);
						if (basefd == -1)
							continue;

						fstat(basefd, &st);
						seekp = st.st_size - (mp.msp_high - lp.lrp_read + 2) * sizeof(struct DayDream_Message);
						if (seekp < 0)
							seekp = 0;
						lseek(basefd, seekp, SEEK_SET);
						while (read(basefd, &msg, sizeof(struct DayDream_Message))) {
							if (procqwkm(&msg))
								newcnt++;
						}
						ddmsg_close_base(basefd);


					}
					close(ndxfd);

					if (newcnt) {
						sprintf(mailbuf, "%d new msgs\n", newcnt);
						dd_sendstring(d, mailbuf);
					} else {
						sprintf(qbuf, "%s/dd-grab%d/%3.3d.ndx", DDTMP, node, get_virtual_conf(conf->CONF_NUMBER, base->MSGBASE_NUMBER));
						unlink(qbuf);
						dd_sendstring(d, "No new messages\n");
					}
				}
			}
			/* FIXME: Very suspicious code */
			conf = (struct DayDream_Conference *) base;
		} else {
			/* FIXME: Very suspicious code */
			base = (struct DayDream_MsgBase *) conf + 1;
			bcnt = conf->CONF_MSGBASES;

			while (bcnt) {
				ccount++;
				base++;
				bcnt--;
			}
			/* FIXME: Very suspicious code */
			conf = (struct DayDream_Conference *) base;
		}
	}
	fprintf(ctrl, "%d\r\n%d\r\n", mcount, ccount - 1);
	conf = confs;

	while (1) {
		int bcnt;

		if (conf->CONF_NUMBER == 255)
			break;

		/* FIXME: Very suspicious code */
		base = (struct DayDream_MsgBase *) conf + 1;
		bcnt = conf->CONF_MSGBASES;

		for (bcnt = conf->CONF_MSGBASES; bcnt; bcnt--, base++) {
			vc++;
			fprintf(ctrl, "%d\r\n%-13.13s\r\n", vc, base->MSGBASE_FN_TAG);
		}
		/* FIXME: Very suspicious code */
		conf = (struct DayDream_Conference *) base;
	}
	fprintf(ctrl, "\r\n\r\n\r\n");
	fclose(ctrl);
	fclose(msgdfd);
	if (mcount) {
		char cdir[1024];
		FILE *fl;

		sprintf(qbuf, "[34m---------------------------------------------------------------------------\n[36m%d messages archived. Compressing with zip...", mcount);
		dd_sendstring(d, qbuf);
		getcwd(cdir, 1024);
		sprintf(qbuf, "%s/dd-grab%d", DDTMP, node);
		chdir(qbuf);
		sprintf(qbuf, "zip >/dev/null 2>/dev/null </dev/null %s.qwk *", bbsid);
		system(qbuf);
		dd_sendstring(d, "OK!\n");

		sprintf(qbuf, "%s/dd-grab%d/dltemp.lst", DDTMP, node);
		fl = fopen(qbuf, "w");
		fprintf(fl, "%s/dd-grab%d/%s.qwk\n", DDTMP, node, bbsid);
		fclose(fl);
		dd_sendfiles(d, qbuf);
		chdir(cdir);

		setlrps();

	}
	return 1;
}

static void setlrps(void)
{
	dd_sendstring(d, "[36mSet last read pointers? [32m([33mYes[32m/[33mno[32m)[34m: [0m");
	conf = confs;

	if (dd_hotkey(d, HOT_YESNO) == 1) {

		while (1) {
			int bcnt;

			if (conf->CONF_NUMBER == 255)
				break;

			if (myjoin(conf->CONF_NUMBER)) {
				/* FIXME: Very suspicious code */
				base = (struct DayDream_MsgBase *) conf + 1;
				bcnt = conf->CONF_MSGBASES;
				for (bcnt = conf->CONF_MSGBASES; bcnt; bcnt--, base++) {
					dd_changemsgbase(d, base->MSGBASE_NUMBER, MC_QUICK | MC_NOSTAT);
					if (dd_isbasetagged(d, conf->CONF_NUMBER, base->MSGBASE_NUMBER)) {
						dd_getmprs(d, &mp);
						dd_getlprs(d, &lp);

						lp.lrp_read = lp.lrp_scan = mp.msp_high;

						dd_setlprs(d, &lp);
					}
				}
				/* FIXME: Very suspicious code */
				conf = (struct DayDream_Conference *) base;
			} else {
				/* FIXME: Very suspicious code */
				base = (struct DayDream_MsgBase *) conf + 1;
				bcnt = conf->CONF_MSGBASES;

				while (bcnt) {
					base++;
					bcnt--;
				}
				/* FIXME: Very suspicious code */
				conf = (struct DayDream_Conference *) base;
			}
		}
	}
}

static int procqwkm(struct DayDream_Message *daheader)
{
	char realname[40];
	char handle[40];
	char msgstatus = ' ';
	struct stat st;
	unsigned short vc;
	unsigned char vc2;
	char mspervo[4];
	int end_offset;
	int n;
	int out_bytes;
	int fd;

	char *rec;
	char msgbuf[1024];
	FILE *msgf;

	struct tm *tm;


	dd_getstrval(d, realname, USER_REALNAME);
	dd_getstrval(d, handle, USER_HANDLE);

	if (daheader->MSG_NUMBER > lp.lrp_read) {
		if (daheader->MSG_FLAGS & (1L << 0)) {
			msgstatus = '+';

			if ((!strcasecmp(daheader->MSG_AUTHOR, realname)) || (!strcasecmp(daheader->MSG_AUTHOR, handle)) || (!strcasecmp(daheader->MSG_RECEIVER, handle)) || (!strcasecmp(daheader->MSG_RECEIVER, realname))) {

			} else {
				return 0;
			}
		} else {
			if (daheader->MSG_RECEIVED)
				msgstatus = '-';
		}

	} else
		return 0;

	fd = ddmsg_open_msg(conf->CONF_PATH, base->MSGBASE_NUMBER, daheader->MSG_NUMBER, O_RDONLY, 0);

	if(fd == -1)
		return 0;

	if(fstat(fd, &st) == -1) {
		return -1;
	}

	msgf = fdopen(fd, "r");

	lock_file(fileno(msgf), O_RDONLY);

	end_offset = st.st_size;

	tm = localtime(&daheader->MSG_CREATION);
	switch (*daheader->MSG_RECEIVER) {
	case 0:
		rec = "All users";
		break;
	case -1:
		rec = "<*> All users <*>";
		break;
	default:
		rec = daheader->MSG_RECEIVER;
		break;
	}
	fprintf(msgdfd, "%c%-7d%2.2d-%2.2d-%2.2d%-2.2d:%-2.2d", msgstatus, 
																													daheader->MSG_NUMBER, 
																													tm->tm_mon + 1, 
																													tm->tm_mday, 
																													tm->tm_year % 100, 
																													tm->tm_hour, 
																													tm->tm_min);

	fprintf(msgdfd, "%-25.25s%-25.25s%-25.25s%-12.12s%-8.8s", rec, 
																														daheader->MSG_AUTHOR, 
																														daheader->MSG_SUBJECT, 
																														" ", 
																														" ");

	fprintf(msgdfd, "%-6d�", 2 + end_offset / 128);
	vc = get_virtual_conf(conf->CONF_NUMBER, base->MSGBASE_NUMBER);
	vc2 = vc;

	fwrite(&vc, 2, 1, msgdfd);
	mcount++;
	fwrite(&mcount, 2, 1, msgdfd);
	fputc(' ', msgdfd);

	out_bytes = 0;

	while (NULL != fgetsnolf(msgbuf, 1024, msgf)) {
		n = strlen(msgbuf);
		fwrite(msgbuf, n, 1, msgdfd);
		out_bytes += n;

		if (n < 1024 - 1) {
			fputc(227, msgdfd);
			out_bytes++;
		}
	}
	fclose(msgf);
	ddmsg_close_msg(fd);
	/* Pad block as necessary */
	n = out_bytes % 128;
	for (; n < 128; n++)
		fputc(' ', msgdfd);

	inttoms(blk_cnt, mspervo);
	blk_cnt += 2 + end_offset / 128;
	write(ndxfd, &mspervo, 4);
	write(ndxfd, &vc2, 1);
	return 1;
}

/* inttoms ripped from uqwk */
static int inttoms(int i, char c[4])
/*
 *  Convert an integer into the Microsoft Basic floating format.
 *  This is the dumbest thing in the whole QWK standard.  Why in
 *  the world store block offsets as floating point numbers?
 *  Stupid!
 */
{
	int m, e;

	if (i == 0) {
		c[0] = c[1] = c[2] = 0;
		c[3] = 0x80;
		return 0;
	}
	e = 152;
	m = 0x7fffff & i;

	while (!(0x800000 & m)) {
		m <<= 1;
		e--;
	}
	c[0] = 0xff & m;
	c[1] = 0xff & (m >> 8);
	c[2] = 0x7f & (m >> 16);
	c[3] = 0xff & e;
	return 0;
}

static int myjoin(int confn)
{
	if (!dd_isanybasestagged(d, confn))
		return 0;
	if (!dd_joinconf(d, confn, JC_QUICK | JC_SHUTUP))
		return 0;
	return 1;
}

static void nuketmp(void)
{
	char tb[1024];
	sprintf(tb, "%s/dd-grab%d", DDTMP, node);
	deldir(tb);
	rmdir(tb);
}

static int maketmp(void)
{
	char tb[1024];
	sprintf(tb, "%s/dd-grab%d", DDTMP, node);
	nuketmp();
	if (mkdir(tb, 0755) == 0)
		return 1;
	return 0;
}

static void die(void)
{
	dd_joinconf(d, cconf, JC_QUICK | JC_SHUTUP);
	dd_close(d);
}

static char *ecfg(char *hay, char *need)
{
	char *s;
	while (1) {
		s = need;
		if (*hay == 0)
			return 0;
		if (*hay == ';') {
			while (*hay != 10) {
				if (*hay == 0)
					return 0;
				hay++;
			}
			continue;
		}
		while (1) {
			if (*s++ == *hay++) {
				if (*s == 0)
					return hay;
			} else {
				break;
			}
		}
	}
}

static void CpyToLAINA(char *src, char *dest)
{
	int i = 0;

	while (src[i] != '\"') {
		dest[i] = src[i];
		i++;
	}
	dest[i] = 0;
}

static int get_virtual_conf(int confn, int basen)
{
	struct DayDream_Conference *mconf;
	struct DayDream_MsgBase *mbase;

	int i = 1;
	mconf = confs;

	while (1) {
		int bcnt;
		if (mconf->CONF_NUMBER == 255)
			break;

		/* FIXME: Very suspicious code */
		mbase = (struct DayDream_MsgBase *) mconf + 1;
		bcnt = mconf->CONF_MSGBASES;
		for (bcnt = mconf->CONF_MSGBASES; bcnt; bcnt--, mbase++) {
			if (mbase->MSGBASE_NUMBER == basen && mconf->CONF_NUMBER == confn)
				return i;
			i++;
		}
		/* FIXME: Very suspicious code */
		mconf = (struct DayDream_Conference *) mbase;
	}
	return 0;
}

static int get_conf_virt(int virt)
{
	struct DayDream_Conference *mconf;
	struct DayDream_MsgBase *mbase;

	int i = 1;
	mconf = confs;

	while (1) {
		int bcnt;
		if (mconf->CONF_NUMBER == 255)
			break;

		/* FIXME: Very suspicious code */
		mbase = (struct DayDream_MsgBase *) mconf + 1;
		for (bcnt = mconf->CONF_MSGBASES; bcnt; bcnt--, mbase++) {
			if (virt == i) {
				conf = mconf;
				base = mbase;
				return 1;
			}
			i++;
		}
		/* FIXME: Very suspicious code */
		mconf = (struct DayDream_Conference *) mbase;
	}
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
