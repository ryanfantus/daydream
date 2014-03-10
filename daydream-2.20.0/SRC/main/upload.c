#include <config.h>

#include <dirent.h>
#include <sys/param.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/mman.h>
#include <signal.h>
#include <syslog.h>
#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#include <sys/mount.h>

#include <daydream.h>
#include <ddcommon.h>
#include <utility.h>
#include <console.h>

struct bgcheckinfo {
	struct userbase bguser;
	int totbytes;
	int totfiles;
	int dupes;
	int okfiles;
	int okbytes;
};

static void bghan(int);
static int checkbgdone(int);
static int checkfilename(char *);
static int dupecheck(char *);
static int getdszuname(char *, char *);
static uint64_t get_free_space(const char *);
static int getpath(conference_t *, char *, size_t, int)
	__attr_bounded__ (__string__, 2, 3);
static int makelc(char *);
static int runexamine(char *);
static int extract_desc(char *, size_t, time_t *)
	__attr_bounded__ (__string__, 1, 2);

static int bgdone;
static struct bgcheckinfo bi;
static char finname[255];
static int quickmode;
static int ul_totulf;
static int ul_dupes;
static int ul_totulb;
static int ul_okf;
static int ul_okb;

struct DayDream_Archiver *arc; /* FIXME! where does this belong? */

int bgrun = 0; 
int wasbg; 

static int addulbytes(int by)
{
	int dnod;
	struct dd_nodemessage ddn;

	if ((dnod = isonline(user.user_account_id))) {
		dnod--;
		if (dnod < node) {
			ddn.dn_command = 12;
			ddn.dn_data1 = by;
			sendtosock(dnod, &ddn);
			ddn.dn_command = 10;
			ddn.dn_data1 = 1;
			sendtosock(dnod, &ddn);
		}
	}
	user.user_ulbytes += by;
	user.user_ulfiles++;
	return 1;
}

void recfiles(char *path, char *confp)
{
	char olddir[1024];
	char buf2[80];
	char udbuf[400];

	getcwd(olddir, 1024);
	chdir(path);

	snprintf(buf2, sizeof buf2, "%s/dszlog.%d", DDTMP, node);
	unlink(buf2);

	if (lmode) {
		HotKey(0);
	} else if (protocol->PROTOCOL_TYPE == 1) {
		if (confp) {
			snprintf(udbuf, sizeof udbuf, "%s/utils/ddrz -vv -b -r -f %s/data/paths.dat -F %s/tmplist%d -l %s -g %s -n %s/nodeinfo%d.data", origdir, conference()->conf.CONF_PATH, DDTMP, node, buf2, ttyname(serhandle), DDTMP, node);
		} else {
			snprintf(udbuf, sizeof udbuf, "%s/utils/ddrz -vv -b -r -l %s -g %s -n %s/nodeinfo%d.data", origdir, buf2, ttyname(serhandle), DDTMP, node);
		}
		runstdio(udbuf, -1, 2);
	} 
        else if (protocol->PROTOCOL_TYPE == 4) {
		checkforftp(2, path, buf2);
	}

	analyzedszlog(buf2, udbuf, sizeof buf2);
	chdir(olddir);
}

static void terminate_background_checker(void)
{
	if (bgrun) {
		wasbg = 1;

		DDPut(sd[w4bgcheckstr]);
		kill(bgrun, SIGUSR1);
		for (;;) {
			if (checkbgdone(0))
				break;
			sleep(1);
			if (!ispid(bgrun)) {
				checkbgdone(0);

				break;
			}
		}
		bgrun = 0;
	}
}

/* mode = 0    normal
 *        1    local upload
 *        2    ftp
 *        3    rz
 */
int upload(int mode)
{
	char udbuf[500];
	int autooff = 0;
	int maxlen;
	struct dirent *dent;
	DIR *dh;
	int gotfiles = 0;
	char olddir[1024];
	int starttimeleft;
	time_t timenow;

	ul_totulf = ul_dupes = ul_totulb = ul_okf = ul_okb = 0;

	setprotocol();

	changenodestatus("Uploading");

	if (user.user_toggles & (1L << 14))
		quickmode = 1;
	else
		quickmode = 0;

	if (conference()->conf.CONF_FILEAREAS == 0) {
		DDPut(sd[nofileareasstr]);
		return 0;
	}
	if (conference()->conf.CONF_UPLOADAREA == 0) {
		DDPut(sd[nouploadsstr]);
		return 0;
	}
	if (mode == 0 || mode == 1 || mode == 3) {
		bgrun = 0;
		wasbg = 0;

		if (cleantemp() == -1) {
			DDPut(sd[tempcleanerrstr]);
			return 0;
		}
		if (mode != 3)
			TypeFile("upload", TYPE_MAKE | TYPE_CONF);
		if (!freespace())
			return 0;
		if (conference()->conf.CONF_ATTRIBUTES & (1L << 3)) {
			maxlen = 31;
		} else {
			maxlen = 12;
		}

		if (mode != 3) {
			checkforpartialuploads(1);
			if (protocol->PROTOCOL_TYPE != 4) {
				char logn[512];

				snprintf(logn, sizeof logn,
					"%s/dszlog.%d", DDTMP, node);
				unlink(logn);

				if (checkforftp(1, currnode->MULTI_TEMPORARY, logn)) {
					analyzedszlog(logn, udbuf, sizeof udbuf);
					goto ftpgo;
				}
			}
			ddprintf(sd[fnamelenstr], maxlen);

			for (;;) {
				DDPut(sd[proceedtransferstr]);
				udbuf[0] = 0;
				if (!(Prompt(udbuf, 1, 0)))
					return 0;
				if (!strcasecmp(udbuf, "a")) {
					return 0;
				} else if (!strcasecmp(udbuf, "d")) {
					autooff = 1;
					break;
				} else if (!strcasecmp(udbuf, "p") || udbuf[0] == 0) {
					break;
				}
			}
		}
	}

	if (mode == 1) {
		ddprintf(sd[localulstr], currnode->MULTI_TEMPORARY);
		HotKey(0);
	} else if (mode == 0 || mode == 3) {
		starttimeleft = timeleft;
		maketmplist();

		if ((!(user.user_toggles & (1L << 15))) &&
		    (maincfg.CFG_FLAGS & (1L << 11)) &&
		    protocol->PROTOCOL_TYPE != 4) {
			initbgchecker();
		}
		recfiles(currnode->MULTI_TEMPORARY, 
			 conference()->conf.CONF_PATH);
		timenow = time(0);
		timeleft = starttimeleft;
		endtime = timenow + timeleft;
	}
       	       	
	if (autooff) {
		if (autodisconnect() == 1)
			quickmode = 1;
		else
			autooff = 0;
	}
	
	terminate_background_checker();

ftpgo:	
	if (conference()->conf.CONF_ATTRIBUTES & (1L << 3)) {
		DDPut(sd[longfstr]);
	} else {
		DDPut(sd[shortfstr]);
	}

	if ((dh = opendir(currnode->MULTI_TEMPORARY))) {

		getcwd(olddir, 1024);
		chdir(currnode->MULTI_TEMPORARY);
		while ((dent = readdir(dh))) {
			if (dent->d_name[0] == '.' && (dent->d_name[1] == '\0' || (dent->d_name[1] == '.' && dent->d_name[2] == '\0')))
				continue;
			if (!strcmp(".packtmp", dent->d_name))
				continue;
			gotfiles++;
			deldir(".packtmp");
			if (!handleupload(dent->d_name))
				makelc(finname);
		}
		chdir(olddir);
		closedir(dh);
	}
	if (autooff)
		return 2;
/*
   Upload statistics
   =================
   Type              Files         Bytes       Dupes      OK Files    OK Bytes
   ---------------------------------------------------------------------------
   Normal              232       5443456           4           238   543452542
   Background           10      54333345           2            22     3444432
   ---------------------------------------------------------------------------
   TOTAL               242      59000000           6           245   567000000
 */
	if (gotfiles == 0 && !ul_totulf && !bi.totfiles) {
		DDPut(sd[tempemptystr]);
	} else {
		DDPut(sd[ulstathdrstr]);
		if (ul_totulf) 
			ddprintf(sd[ulstatlinestr],
				sd[ulstatnormstr], ul_totulf, ul_totulb,
				ul_dupes, ul_okf, ul_okb);
		if (bi.totfiles && wasbg) 
			ddprintf(sd[ulstatlinestr],
			     sd[ulstatbackstr], bi.totfiles, bi.totbytes,
				bi.dupes, bi.okfiles, bi.okbytes);
		if (ul_totulf && bi.totfiles && wasbg) 
			ddprintf(sd[ulstattailstr],
				bi.totfiles + ul_totulf, 
				bi.totbytes + ul_totulb,
				bi.dupes + ul_dupes, bi.okfiles + ul_okf,
				bi.okbytes + ul_okb);
	}
	return 0;
}

static int makelc(char *fn)
{
	char buf[1024];

	struct lcfile lc;
	int fd;

	memset(&lc, 0, sizeof(struct lcfile));

	lc.lc_conf = conference()->conf.CONF_NUMBER;
	strlcpy(lc.lc_name, fn, sizeof lc.lc_name);

	snprintf(buf, sizeof buf, "%s/users/%d/lcfiles.dat", origdir,
		 user.user_account_id);
	fd = open(buf, O_WRONLY | O_CREAT, 0666);
	if (fd == -1) {
		unlink(fn);
		return 0;
	}
	fsetperm(fd, 0666);
	lseek(fd, 0, SEEK_END);
	safe_write(fd, &lc, sizeof(struct lcfile));
	snprintf(buf, sizeof buf, "%s/users/%d/lcfiles/%s", origdir, 
		user.user_account_id, fn);
	close(fd);
	newrename(fn, buf);
	return 1;
}

int genstdiocmdline(char *dest, const char *src, 
		    const char *arc, const char *no)
{
	char tbuffer[60];
	char *s;

	for (;;) {
		if (*src == '%') {
			if (*(src + 1) == 'A' || *(src + 1) == 'a') {
				while (arc && *arc)
					*dest++ = *arc++;
				src += 2;
			} else if (*(src + 1) == 'N' || *(src + 1) == 'n') {
				s = tbuffer;
				if (!no) {
					sprintf(s, "%d", node);
				} else {
					strcpy(s, no);
				}
				while (*s)
					*dest++ = *s++;
				src += 2;
			} else if (*(src + 1) == 'R' || *(src + 1) == 'r') {
				s = user.user_realname;
				while (*s)
					*dest++ = *s++;
				src += 2;
			} else if (*(src + 1) == 'H' || *(src + 1) == 'h') {
				s = user.user_handle;
				while (*s)
					*dest++ = *s++;
				src += 2;
			} else if (*(src + 1) == 'O' || *(src + 1) == 'o') {
				s = user.user_organization;
				while (*s)
					*dest++ = *s++;
				src += 2;
			} else if (*(src + 1) == 'Z' || *(src + 1) == 'z') {
				s = user.user_zipcity;
				while (*s)
					*dest++ = *s++;
				src += 2;
			} else if (*(src + 1) == 'S' || *(src + 1) == 's') {
				s = tbuffer;;
				snprintf(s, sizeof tbuffer, "%d", user.user_securitylevel);
				while (*s)
					*dest++ = *s++;
				src += 2;
			} else {
				*dest++ = *src++;
			}
		} else {
			if (!*src)
				break;
			*dest++ = *src++;
		}
	}
	*dest = 0;
	return 0;
}

static int update_filelist(char *fname, char *desc)
{
	int fd;	
	if ((fd = open(fname, O_RDWR | O_CREAT | O_APPEND, 0666)) == -1)
		return -1;
	fsetperm(fd, 0666);
	if (safe_write(fd, desc, strlen(desc)) != strlen(desc)) {
		close(fd);
		return -1;
	}
	return close(fd);
}

static int check_archive(void)
{
	char arccmd[1024];
	char arcbuf[4096];
	char fname[PATH_MAX];
	char tmpname[15] = "";
	FILE *checkd;
	int arcok = 1;
	int fd;

	if (strlcpy(fname, currnode->MULTI_TEMPORARY,
			sizeof fname) >= sizeof fname ||
		strlcat(fname, "/", sizeof fname) >= sizeof fname ||
		strlcat(fname, finname, sizeof fname) >= sizeof fname)
		return -1;
	genstdiocmdline(arccmd, arc->ARC_CMD_TEST, fname, 0);
	strlcpy(tmpname, "/tmp/dd.XXXXXX", sizeof tmpname);
	if ((fd = mkstemp(tmpname)) == -1)
		return -1;

	DDPut(sd[testingarcstr]);
	if (arc->ARC_FLAGS & (1L << 4)) {
		DDPut(sd[testsepastr]);
		runstdio(arccmd, fd, 0);
		DDPut(sd[testsepastr]);
	} else {
		runstdio(arccmd, fd, 3);
	}
	if ((checkd = fdopen(fd, "w+")) != NULL) {
		fseek(checkd, 0, SEEK_SET);
		while (fgets(arcbuf, sizeof arcbuf, checkd) && arcok) {
			if (*arc->ARC_CORRUPTED1 && 
				strstr(arcbuf, arc->ARC_CORRUPTED1))
				arcok = 0;
			if (*arc->ARC_CORRUPTED2 && 
				strstr(arcbuf, arc->ARC_CORRUPTED2))
				arcok = 0;
			if (*arc->ARC_CORRUPTED3 && 
				strstr(arcbuf, arc->ARC_CORRUPTED3))
				arcok = 0;
		}
		fclose(checkd);
	} else {
		/* since there's no fclose() to close the descriptor,
		 * we have to close it explicitly.
		 */
		close(fd);
	}
	unlink(tmpname);
	return arcok ? 0 : -1;
}

static int extract_desc(char *dbuf, size_t dbuflen, time_t *desctime)
{
	struct dirent *dent;
	DIR *dir;
	char pname[PATH_MAX];
	char fname[PATH_MAX];
	char buf[PATH_MAX];
	char extdiz[1024]; 
	FILE *fp;
	int cwdfd;
	char kelabuf[256];
	int dizcnt;
	struct stat st;
	int fd;

	if (!dbuf || !dbuflen)
		return -1;
	if (!arc || !arc->ARC_EXTRACTFILEID[0])
		return -1;
	
	/* syslog only those errors which the SysOp can fix easily */
	if (pathcat2(pname, sizeof pname, 
		currnode->MULTI_TEMPORARY, ".packtmp") == -1) 
		return -1;
	if (deldir(pname) == -1 && errno != ENOENT) {
		syslog(LOG_ERR, "cannot delete %.200s: %m", pname);
		return -1;
	}
	/* create_directory() does needed syslogging */
	if (create_directory(pname, maincfg.CFG_BBSUID, 
		maincfg.CFG_ZIPGID, 0770 | S_ISGID) == -1)
		return -1;

	if (pathcat2(fname, sizeof fname,
		currnode->MULTI_TEMPORARY, finname) == -1) 
		return -1;
	if ((cwdfd = open(".", O_RDONLY)) == -1) 
		return -1;
	if (chdir(".packtmp") == -1) 
		return -1;
	
	genstdiocmdline(extdiz, arc->ARC_EXTRACTFILEID, fname, 0);
	runstdio(extdiz, -1, 3);

	/* "hypothetical" situation */
	if (fchdir(cwdfd) == -1) {
		close(cwdfd);
		return -1;
	}
	close(cwdfd);

	if (!(dir = opendir(".packtmp")))
		return -1;

	fd = -1;	
	while ((dent = readdir(dir))) {
		if (strcasecmp("file_id.diz", dent->d_name))
			continue;
		if (pathcat2(buf, sizeof buf, pname, dent->d_name) == -1)
			continue;
		/* we can check whether the opened file is a regular
		 * file, but O_NOFOLLOW prevents anybody doing nasty
		 * stuff (imagine old tape drives...)
		 */
#ifdef HAVE_O_NOFOLLOW
		fd = open(buf, O_RDONLY | O_NOFOLLOW);
#else
		fd = open(buf, O_RDONLY);
#endif
		if (fd != -1 && fstat(fd, &st) == 0 && S_ISREG(st.st_mode)) 
			break;

		close(fd);
		fd = -1;
	}
	*desctime = st.st_mtime;
	closedir(dir);

	if (fd == -1 || !(fp = fdopen(fd, "r"))) {
		close(fd);
		return -1;
	}

	DDPut(sd[gotdizstr]);

	dizcnt = 0;
	*dbuf = 0;
	while (dbuflen >= 45) {
		if (!fgetsnolf(kelabuf, 250, fp))
			break;
		if (!*kelabuf)
			continue;
		stripansi(kelabuf);
		snprintf(dbuf, dbuflen, "%-44.44s", kelabuf);
		ddprintf(sd[dizlinestr], dizcnt + 1, dbuf);
		dizcnt++;
		dbuf += 45;
		dbuflen -= 45;
	}
	fclose(fp);

	return dizcnt;
}

static int set_permissions(const char *fname)
{
	char buf[PATH_MAX];

	if (pathcat2(buf, sizeof buf, currnode->MULTI_TEMPORARY, fname) == -1) 
		return -1;
	if (chmod(buf, 0770) == -1)
		return -1;

	return 0;
}

int handleupload(const char *upname)
{
	char linebuf[20 * 45];
	char buf[200];
	char daline[86];
	struct stat st;
	struct tm *muntm;
	time_t currtime;
	int linecnt = 0;
	char *s;
	int sta = 1;
	int i = 0;
	char ulsize[10];
	char finalbuf[80 * 25];
	int frees;
	int lcn;
	char dizbuf[45 * 20];
	int dizcnt;
	time_t diztime;

	strlcpy(finname, upname, sizeof finname);
	if (set_permissions(finname) == -1) {
		syslog(LOG_ERR, "cannot chmod uploaded file: %m");
		ddprintf("Error handling uploaded file (%s).\n", finname);
		return 0;
	}

	stat(upname, &st);
	if (!bgmode) {
		ul_totulb += st.st_size;
		ul_totulf++;
	}

	currtime = time(0);
	muntm = localtime(&currtime);

	while (checkfilename(finname) == 0) {
		if (bgmode)
			return 0;
		i = 1;
		if (conference()->conf.CONF_ATTRIBUTES & (1L << 3)) {
			ddprintf(sd[linvalidstr], finname, " ");
			buf[0] = 0;
			if (!(Prompt(buf, 34, PROMPT_NOCRLF | PROMPT_FILE)))
				return 0;
			strlcpy(finname, buf, sizeof finname);
		} else {
			ddprintf(sd[sinvalidstr], finname);
			buf[0] = 0;
			if (!(Prompt(buf, 12, PROMPT_NOCRLF | PROMPT_FILE)))
				return 0;
			strlcpy(finname, buf, sizeof finname);
		}

	}
	if (i)
		rename(upname, finname);

	getarchiver(finname);
	/* background checker accepts only packets with file_id.diz */
	if ((!arc || !arc->ARC_EXTRACTFILEID[0]) && bgmode)
		return 0;

	DDPut("\r                                                                          \r");

	if (st.st_size >= 10000000) {
		snprintf(ulsize, sizeof ulsize, "%6dk", 
			(int) st.st_size / 1024);
	} else {
		snprintf(ulsize, sizeof ulsize, "%7d", (int) st.st_size);
	}

	if (conference()->conf.CONF_ATTRIBUTES & (1L << 3)) {
		snprintf(daline, sizeof daline, "%-34.34s ", finname);
	} else {
		snprintf(daline, sizeof daline, "%-12.12s ", finname);
	}
	DDPut(daline);
	if ((conference()->conf.CONF_ATTRIBUTES & (1L << 9)) && dupecheck(finname)) {
		ul_dupes++;
		return 1;
	}
	if (conference()->conf.CONF_ATTRIBUTES & (1L << 3)) {
		snprintf(&daline[35], sizeof daline - 35, "N--- %s %s", 
		    ulsize, ctime(&currtime));
		DDPut(&daline[35]);
		DDPut(sd[ullinestr]);
	} else {		
		snprintf(&daline[13], sizeof daline - 13, 
		    "N--- %s %2.2d.%2.2d.%2.2d ", 
		    ulsize, muntm->tm_mday, muntm->tm_mon + 1, 
		    muntm->tm_year % 100);
		DDPut(&daline[13]);
	}

	memset(linebuf, 0, sizeof linebuf);

	DDPut(sd[checkdizstr]);
	if ((dizcnt = extract_desc(dizbuf, sizeof dizbuf, &diztime)) <= 0)
		multibackspace(strlena(sd[checkdizstr]));

	if (dizcnt > 0) {
		/* we don't have controlling terminal, from which
		 * we could ask additions to file_id.diz, so we
		 * default to the quick mode.
		 */
		if (bgmode)
			quickmode = 1;

		if (checkcarrier() && !quickmode) {
			DDPut(sd[dizwaitstr]);
		      keke:
			delayt = 3;
			if (!checkcarrier()) {
				quickmode = 1;
			} else
				switch (HotKey(HOT_DELAY)) {
				case 'q':
				case 'Q':
					quickmode = 1;
				case 0:
				case 255:
				case 's':
				case 'S':
				case 13:
				case 10:
					sta = 2;
					memcpy(linebuf, dizbuf, dizcnt * 45);
					linecnt = dizcnt;
					break;
				case 'H':
				case 'h':
					sta = 3;
					memcpy(linebuf, dizbuf, dizcnt * 45);
					linecnt = dizcnt;
					break;
				case 'e':
				case 'E':
					memcpy(linebuf, dizbuf, dizcnt * 45);
					linecnt = dizcnt;
				case 'n':
				case 'N':
					DDPut("\r                                                                      \r");
					ddprintf(sd[lineistr], linecnt + 1);
					break;
				default:
					goto keke;
				}
			if (sta != 1)
				DDPut("\r                                                                      \r");
		} else {
			sta = 2;
			memcpy(linebuf, dizbuf, dizcnt * 45);
			linecnt = dizcnt;
		}
	} else if (bgmode)
		return 0;

	while (sta == 1) {
		s = linecnt * 45 + linebuf;
		if (!(Prompt(s, 44, 0)))
			return 0;
		if (s[0] == 0 || linecnt == 14) {
			for (;;) {
				DDPut(sd[entrymenustr]);
				buf[0] = 0;
				if (!(Prompt(buf, 2, 0)))
					return 0;
				if (buf[0] == 0 || (!strcasecmp(buf, "s"))) {
					sta = 2;
					break;
				} else if (!strcasecmp(buf, "w")) {
					sta = 4;
					break;
				} else if (!strcasecmp(buf, "r")) {
					ddprintf("                          Line #%2.2d ", linecnt + 1);
					break;
				} else if (!strcasecmp(buf, "e")) {
					int destlin;
					DDPut(sd[ullineedstr]);
					buf[0] = 0;
					if (!(Prompt(buf, 2, 0)))
						return 0;
					destlin = atoi(buf);
					if (destlin > 0 && destlin < (linecnt + 1)) {
						DDPut(sd[ullineeditingstr]);
						s = linebuf + (destlin - 1) * 45;
						if (!(Prompt(s, 44, 0)))
							return 0;
					} else {
						DDPut(sd[unknownlinestr]);
					}
				} else if ((!strcasecmp(buf, "?")) || (!strcasecmp(buf, "c"))) {
					TypeFile("uploadcommands", TYPE_WARN | TYPE_MAKE);
				} else if (!strcasecmp(buf, "l")) {
					int j;
					DDPut(sd[descliststr]);
					if (linecnt == 14)
						j = 15;
					else
						j = linecnt;
					i = 0;
					while (i != linecnt) {
						s = linebuf + i * 45;
						ddprintf(sd[dizlinestr], i + 1, s);
						i++;
					}
				} else if (!strcasecmp(buf, "h")) {
					sta = 3;
					break;
				} else if (!strcasecmp(buf, "d")) {
					unlink(finname);
					return 1;
				}
			}
		} else {
			linecnt++;
			ddprintf(sd[lineistr], linecnt + 1);
		}
	}

	s = linebuf;
	lcn = linecnt;

	finalbuf[0] = 0;
	if (conference()->conf.CONF_ATTRIBUTES & (1L << 3)) {
		snprintf(finalbuf, sizeof finalbuf, "%s", daline);
	} else {
		snprintf(finalbuf, sizeof finalbuf, "%s%s\n", daline, s);
		s = &s[45];
	}

	while (*s && linecnt) {
		strlcat(finalbuf, "                                   ", sizeof finalbuf);
		strlcat(finalbuf, s, sizeof finalbuf);
		strlcat(finalbuf, "\n", sizeof finalbuf);
		s = &s[45];
		linecnt--;
	}

	if (arc && arc->ARC_FLAGS & (1L << 2) && dizcnt) {
		struct tm *tm;
		char db[100];
		tm = localtime(&diztime);
		snprintf(db, sizeof db, "                                   [ Date from file_id.diz: %2.2d.%2.2d.%2.2d  (%2.2d:%2.2d) ]\n", tm->tm_mday, tm->tm_mon + 1, tm->tm_year % 100, tm->tm_hour, tm->tm_min);
		strlcat(finalbuf, db, sizeof finalbuf);
	}
	if ((conference()->conf.CONF_ATTRIBUTES & (1L << 2)) && sta == 2) {
		strlcat(finalbuf, "                                   ", sizeof finalbuf);
		strlcat(finalbuf, user.user_signature, sizeof finalbuf);
		strlcat(finalbuf, "\n", sizeof finalbuf);

	}
	frees = isfreedl(finname);
	if (frees) {
		if (frees == 1) {
			strlcat(finalbuf, "                                   ", sizeof finalbuf);
			strlcat(finalbuf, maincfg.CFG_FREEDLLINE, sizeof finalbuf);
			strlcat(finalbuf, "\n", sizeof finalbuf);
		}
		if (conference()->conf.CONF_ATTRIBUTES & (1L << 3)) {
			finalbuf[36] = 'F';
		} else {
			finalbuf[14] = 'F';
		}
	}
/* daline -> filedesc, buf -> destination name */

	if (sta == 3) {
		snprintf(daline, sizeof daline, "%sDescriptions", 
		     maincfg.CFG_HOLDDIR);
		snprintf(buf, sizeof buf, "%s%s", 
			maincfg.CFG_HOLDDIR, finname);
	} else {
		if ((conference()->conf.CONF_ATTRIBUTES & (1L << 8)) == 0 && arc && *arc->ARC_CMD_TEST) {
			if (check_archive() != -1) {
				if (conference()->conf.CONF_ATTRIBUTES & (1L << 3)) {
					finalbuf[35] = 'P';
				} else {
					finalbuf[13] = 'P';
				}
				DDPut(sd[arcokstr]);
			} else {
				if (arc->ARC_FLAGS & (1L << 3)) {
					DDPut(sd[arcbrokenstr]);
					unlink(finname);
					return 1;
				} else {
					DDPut(sd[arcbroken2str]);
					if (conference()->conf.CONF_ATTRIBUTES & (1L << 3)) {
						finalbuf[35] = 'F';
					} else {
						finalbuf[13] = 'F';
					}
				}
			}
		}
		if (conference()->conf.CONF_ATTRIBUTES & (1L << 4)) {
			char db[80];
			char upath[PATH_MAX];

			int ps;

		      askbag:
			snprintf(upath, sizeof upath, sd[uldeststr], 
				conference()->conf.CONF_FILEAREAS);
			DDPut(upath);
			*db = 0;
			/* again, we don't have controlling terminal, so
			 * let's select the default upload area.
			 */
			if (!bgmode) 
				ps = Prompt(db, 3, 0);
			else
				ps = 0;
			if (!ps || toupper(*db) == 'U' || !checkcarrier()) {
				snprintf(daline, sizeof daline, 
				    "%s/data/directory.%3.3d", 
				    conference()->conf.CONF_PATH, 
				    conference()->conf.CONF_UPLOADAREA);
				if (conference()->conf.CONF_ULPATH[0] == '@') {
					getfreeulp(&conference()->conf.CONF_ULPATH[1], upath, sizeof upath, 0);
					snprintf(buf, sizeof buf, "%s%s", 
						upath, finname);
				} else {
					snprintf(buf, sizeof buf, "%s%s", 
						conference()->conf.CONF_ULPATH,
						finname);
				}
			} else if (toupper(*db) == 'L') {
				TypeFile("filecatalogs", TYPE_MAKE | TYPE_WARN | TYPE_CONF);
				goto askbag;
			} else {
				ps = atoi(db);
				if (ps < 1 || ps > conference()->conf.CONF_FILEAREAS)
					goto askbag;
				snprintf(daline, sizeof daline, 
				     "%s/data/directory.%3.3d", 
				     conference()->conf.CONF_PATH, ps);
				getpath(conference(), upath, sizeof upath, ps);
				snprintf(buf, sizeof buf, "%s%s", 
					upath, finname);
			}
		} else {
			char upath[1024];
			snprintf(daline, sizeof daline, "%s/data/directory.%3.3d", 
			     conference()->conf.CONF_PATH, 
			     conference()->conf.CONF_UPLOADAREA);
			if (conference()->conf.CONF_ULPATH[0] == '@') {
				getfreeulp(&conference()->conf.CONF_ULPATH[1], upath, sizeof upath, 0);
				snprintf(buf, sizeof buf, "%s%s", 
					upath, finname);
			} else {
				snprintf(buf, sizeof buf, "%s%s", 
					conference()->conf.CONF_ULPATH, 
					finname);
			}
		}
	}

	runexamine(finname);

	DDPut(sd[savingulstr]);

	if ((sta == 2 || sta == 4) && !dizcnt && arc && *arc->ARC_ADDFILEID) {
		FILE *fid;

		fid = fopen("file_id.diz", "w");
		if (fid) {
			char adddiz[1024];
			char buu[1024];
			s = linebuf;

			while (*s && lcn) {
				fprintf(fid, "%-44.44s\n", s);
				s = &s[45];
				lcn--;
			}
			fclose(fid);
			snprintf(buu, sizeof buu, "%s/%s", 
				currnode->MULTI_TEMPORARY, finname);
			genstdiocmdline(adddiz, arc->ARC_ADDFILEID, buu, 0);
			runstdio(adddiz, -1, 3);
			unlink("file_id.diz");
		}
	}

	if (update_filelist(daline, finalbuf)) {
		DDPut(sd[ulerrwritedescstr]);
		return 1;
	}

	newrename(finname, buf);

	DDPut(sd[ulokstr]);

	if ((!(conference()->conf.CONF_ATTRIBUTES & (1L << 1))) && sta != 3) {
		addulbytes(st.st_size);
/*              user.user_ulfiles++;
   user.user_ulbytes+=st.st_size; */
	}
	ul_okf++;
	ul_okb += st.st_size;
	if (bgmode) {
		ul_totulb += st.st_size;
		ul_totulf++;
	}
	{
		char lbuf[100];
		int logfd;
		struct DD_UploadLog ddl;
		snprintf(lbuf, sizeof lbuf, "%s/logfiles/uploadlog.dat", 
			origdir);
		logfd = open(lbuf, O_WRONLY | O_CREAT, 0666);
		if (logfd != -1) {
			fsetperm(logfd, 0666);
			memset((char *) &ddl, 0, sizeof(struct DD_UploadLog));
			ddl.UL_SLOT = user.user_account_id;
			strlcpy(ddl.UL_FILENAME, finname, sizeof ddl.UL_FILENAME);
			ddl.UL_FILESIZE = st.st_size;
			ddl.UL_TIME = time(0);
			ddl.UL_BPSRATE = bpsrate;
			ddl.UL_NODE = node;
			ddl.UL_CONF = conference()->conf.CONF_NUMBER;
			lseek(logfd, 0, SEEK_END);
			safe_write(logfd, &ddl, sizeof(struct DD_UploadLog));
			close(logfd);
		}
	}
	return 1;
}

/* FIXME! this seems kludgy. Is it correct? */
static int getpath(conference_t *con, char *s, size_t slen, int b)
{
	char buf[1024];
	FILE *pf;
	int i;

	snprintf(buf, sizeof buf, "%s/data/paths.dat", con->conf.CONF_PATH);
	pf = fopen(buf, "r");
	if (!pf) {
		if (con->conf.CONF_ULPATH[0] == '@') {
			getfreeulp(&con->conf.CONF_ULPATH[1], s, slen, 0);
		} else {
			strlcpy(s, con->conf.CONF_ULPATH, slen);
		}
		return 1;
	}
	i = 1;
	while (fgetsnolf(buf, 1024, pf)) {
		if (i == b) {
			strlcpy(s, buf, slen);
			fclose(pf);
			return 1;
		}
		i++;
	}
	fclose(pf);
	if (con->conf.CONF_ULPATH[0] == '@') {
		getfreeulp(&con->conf.CONF_ULPATH[1], s, slen, 0);
	} else {
		strlcpy(s, con->conf.CONF_ULPATH, slen);
	}
	return 1;
}

static int runexamine(char *filen)
{
	char exbuf[1024];

	snprintf(exbuf, sizeof exbuf, "%s/data/examine.dat", 
		conference()->conf.CONF_PATH);
	rundoorbatch(exbuf, filen);
	return 1;
}

int rundoorbatch(const char *batch, char *fn)
{
	FILE *doorlist;
	char exbuf[1024];

	doorlist = fopen(batch, "r");
	if (doorlist) {
		while (fgetsnolf(exbuf, 1024, doorlist)) {
			if (!*exbuf)
				continue;
			rundoor(exbuf, fn);
		}
		fclose(doorlist);
	}
	return 1;
}

int newrename(const char *old, const char *newf)
{
	if (!strcmp(old, newf))
		return -1;
	if (rename(old, newf) == -1) {
		int oldfd;
		int newfd;
		char *rwbuf;

		/* Obviously this is the only case we should handle. */
		if (errno != EXDEV) 
			return -1;
		
		oldfd = open(old, O_RDONLY);
		if (oldfd != -1) {
			newfd = open(newf, O_WRONLY | O_CREAT | O_TRUNC, 0666);
			if (newfd != -1) {
				int cnt;
				fsetperm(newfd, 0666);
				rwbuf = (char *) xmalloc(60000);

				while ((cnt = read(oldfd, rwbuf, 60000))) {
					safe_write(newfd, rwbuf, cnt);
				}
				free(rwbuf);
			}
			close(newfd);
		}
		close(oldfd);
		unlink(old);
	}
	return 0;
}

int newcopy(const char *old, const char *newf)
{
	int oldfd;
	int newfd;
	char *rwbuf;

	oldfd = open(old, O_RDONLY);
	if (oldfd != -1) {
		newfd = open(newf, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (newfd != -1) {
			int cnt;
			fsetperm(newfd, 0666);
			rwbuf = (char *) xmalloc(60000);

			while ((cnt = read(oldfd, rwbuf, 60000))) {
				safe_write(newfd, rwbuf, cnt);
			}
			free(rwbuf);
		}
		close(newfd);
	}
	close(oldfd);
	return 1;
}

static int checkfilename(char *name)
{
	int len = 0;
	int hips = 0;
	while (*name) {
		len++;
		if (*name == 39) {
			hips++;
		}
		name++;
	}

	if (hips > 1)
		return 0;

	if (conference()->conf.CONF_ATTRIBUTES & (1L << 3)) {
		if (len > 34)
			return 0;
	} else {
		if (len > 12)
			return 0;
	}

	if (len < 3)
		return 0;

	return 1;

}

int getarchiver(const char *file)
{
	arc = arcs;

	while (arc->ARC_FLAGS != 255) {
		if (wildcmp(file, arc->ARC_PATTERN)) 
			return 1;
		arc++;
	}
	arc = 0;
	return 0;
}

int cleantemp(void)
{
	if (deldir(currnode->MULTI_TEMPORARY) == -1 && errno != ENOENT) 
		return -1;
	if (create_directory(currnode->MULTI_TEMPORARY, maincfg.CFG_BBSUID,
		maincfg.CFG_BBSGID, 0770) == -1)
		return -1;
	return 0;	
}

int freespace(void)
{
	uint64_t freesp, freesp2;
	char buf[PATH_MAX];

	if (conference()->conf.CONF_ULPATH[0] == '@') {
		freesp = getfreeulp(&conference()->conf.CONF_ULPATH[1], buf, sizeof buf, 1);
	} else {
		freesp = get_free_space(conference()->conf.CONF_ULPATH);
	}
	freesp2 = get_free_space(currnode->MULTI_TEMPORARY);
	if (freesp < maincfg.CFG_FREEHDDSPACE || 
		freesp2 < maincfg.CFG_FREEHDDSPACE ||
		freesp == (uint64_t) -1LL ||
		freesp2 == (uint64_t) -1LL) {
		DDPut(sd[notfreespacestr]);
		return 0;
	} else {
		ddprintf(sd[freespacestr], freesp, freesp2);
		return 1;
	}
}

#ifdef HAVE_STATVFS
static uint64_t get_free_space(const char *path)
{
	struct statvfs st;
	if (statvfs(path, &st) == -1)
		return (uint64_t) -1LL;
	return ((uint64_t) st.f_bsize * (uint64_t) st.f_bavail);
}
#else
static uint64_t get_free_space(const char *path)
{
	struct statfs st;
	if (statfs(path, &st) == -1) 
		return (uint64_t) -1LL;
	return ((uint64_t) st.f_bsize * (uint64_t) st.f_bavail);
}
#endif

int localupload(void)
{
	return upload(1);
}

static int dupecheck(char *fname)
{
	conference_t *mc;
	struct iterator *iterator;

	DDPut(sd[dckstr]);

	if (maincfg.CFG_FLAGS & (1L << 10)) {
		struct DD_UploadLog *ddl, *lo;

		char lbuf[1024];
		int logfd;
		struct stat st;
		int df = 0;
		int i;

		snprintf(lbuf, sizeof lbuf, 
			"%s/logfiles/uploadlog.dat", origdir);
		logfd = open(lbuf, O_RDONLY);
		if (logfd != -1) {
			fstat(logfd, &st);
			ddl = (struct DD_UploadLog *) mmap(0, st.st_size, PROT_READ, MAP_SHARED, logfd, 0);
			if ((char *) ddl == (char *) -1) {
				multibackspace(strlena(sd[dckstr]));
				close(logfd);
				return 0;
			}
			i = st.st_size / sizeof(struct DD_UploadLog);
			for (lo = ddl; i; lo++, i--) {
				if (!strcasecmp(lo->UL_FILENAME, fname)) {
					df = 1;
					break;
				}
			}
			munmap((void *) ddl, st.st_size);
			close(logfd);
			if (df) {
				multibackspace(strlena(sd[dckstr]));
				DDPut(sd[dckfailedstr]);
				unlink(fname);
				return 1;
			}
		}
		multibackspace(strlena(sd[dckstr]));
		return 0;
	}

	iterator = conference_iterator();
	while ((mc = (conference_t *) iterator_next(iterator))) {
		if (!(mc->conf.CONF_ATTRIBUTES & (1L << 7))) {
			if (find_file(fname, mc)) {
				multibackspace(strlena(sd[dckstr]));
				DDPut(sd[dckfailedstr]);
				unlink(fname);
				iterator_discard(iterator);
				return 1;
			}
		}


	}
	multibackspace(strlena(sd[dckstr]));
	iterator_discard(iterator);
	
	return 0;
}

/* prior to 2.14.6, MULTI_TEMPORARY was expected to end with 
 * a trailing slash. Therefore we have to supply the slash.
 */
int maketmplist(void)
{
	char buf[1024];
	FILE *fi;
	struct DayDream_Multinode *cn;

	snprintf(buf, sizeof buf, "%s/tmplist%d", DDTMP, node);
	fi = fopen(buf, "w");
	if (fi) {
		for (cn = nodes; cn->MULTI_NODE; cn++) {
			if (cn->MULTI_NODE == 253) {
				int j;
				int i = maincfg.CFG_TELNET1ST;
				j = maincfg.CFG_TELNETMAX;

				while (j) {
					j--;
					if (i != node) {
						fprintf(fi, cn->MULTI_TEMPORARY, i);
						fputs("/\n", fi);
					}
					i++;
				}
			} else if (cn->MULTI_NODE == 254) {
				int j;
				int i = maincfg.CFG_LOCAL1ST;
				j = maincfg.CFG_LOCALMAX;

				while (j) {
					j--;
					if (i != node) {
						fprintf(fi, cn->MULTI_TEMPORARY, i);
						fputs("/\n", fi);
					}
					i++;
				}
			} else if (cn->MULTI_NODE != 252) {
				if (cn->MULTI_NODE != node) {
					fprintf(fi, cn->MULTI_TEMPORARY, cn->MULTI_NODE);
					fputs("/\n", fi);
				}
			}
		}
		fclose(fi);
	}
	return 1;
}

static int checkbgdone(int mode)
{
	char buf[1024];
	int efdii;
	int rere;

	while ((waitpid(-1, NULL, WNOHANG)) > 0);

	snprintf(buf, sizeof buf, "%s/bgdone.%d", DDTMP, node);
	if (mode == 1) {
		unlink(buf);
	} else if (mode == 0) {
		int tries = 0;
	      doom:
		efdii = open(buf, O_RDONLY);
		if (efdii == -1)
			return 0;
		rere = read(efdii, &bi, sizeof(struct bgcheckinfo));
		close(efdii);

		if (rere != sizeof(struct bgcheckinfo)) {
			usleep(100000);
			if (++tries < 50)
				goto doom;
			log_error(__FILE__, __LINE__, 
				"background checker failed");
		} else 
			user = bi.bguser;

		unlink(buf);
		return 1;
	} else {
		bi.bguser = user;
		bi.totbytes = ul_totulb;
		bi.dupes = ul_dupes;
		bi.totfiles = ul_totulf;
		bi.okfiles = ul_okf;
		bi.okbytes = ul_okb;

		efdii = open(buf, O_WRONLY | O_CREAT, 0666);	/* If this fails -> trouble */
		if (efdii == -1) {
			log_error(__FILE__, __LINE__,
				"open() failed: %s (%d)",
				strerror(errno), errno);
			return 0;
		}
		fsetperm(efdii, 0666);
		safe_write(efdii, &bi, sizeof(struct bgcheckinfo));
		close(efdii);
	}
	return 1;
}

static void bghan(int sig)
{
	bgdone = 1;
}

/* FIXME! check this */
int initbgchecker(void)
{
	int lastf;
	char buf[1024];
	FILE *log;
	int cnt;
	char fname[256];
	lastf = 0;

	ul_totulf = ul_dupes = ul_totulb = ul_okf = ul_okb = 0;

	bgrun = fork();
	if (bgrun < 0) {
		bgrun = 0;
		return 0;
	}
	if (bgrun > 0)
		return 0;

	signal(SIGUSR1, bghan);

	bgmode = 1;
	bgdone = 0;
	carrier = 0;

	/* get rid of filedescriptors */
	finalize_console();
	close(serhandle);
	close(dsockfd);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	while (!bgdone) {
		snprintf(buf, sizeof buf, "%s/dszlog.%d", DDTMP, node);
		log = fopen(buf, "r");
		if (log) {
			for (cnt = 0; cnt < lastf; cnt++) {
				fgetsnolf(buf, 1024, log);
			}
			if (fgetsnolf(buf, 1024, log)) {
				if (getdszuname(buf, fname)) {
					char oldwd[1024];

					getcwd(oldwd, 1024);
					chdir(currnode->MULTI_TEMPORARY);
					handleupload(filepart(fname));
					chdir(oldwd);
				}
				lastf++;
			}
			fclose(log);
		}
		sleep(1);
	}
	checkbgdone(2);
	/* Commit suicide with style */
	kill(getpid(), SIGKILL);
	return 0;
}

static int getdszuname(char *loge, char *name)
{
	char *s, *t;
	s = loge;
	t = name;
	if (*s == 'S' || *s == 'R') {
		s = &s[2];
		while (*s == ' ')
			if (!*s)
				return 0;
			else
				s++;
		while (*s != ' ')
			if (!*s)
				return 0;
			else
				s++;

		while (*s == ' ')
			if (!*s)
				return 0;
			else
				s++;
		while (*s != ' ')
			if (!*s)
				return 0;
			else
				s++;

		while (*s == ' ')
			if (!*s)
				return 0;
			else
				s++;
		while (*s != ' ')
			if (!*s)
				return 0;
			else
				s++;

		while (*s == ' ')
			if (!*s)
				return 0;
			else
				s++;
		while (*s != ' ')
			if (!*s)
				return 0;
			else
				s++;

		if (strlen(s) < 4)
			return 0;
		s = &s[4];

		while (*s == ' ')
			if (!*s)
				return 0;
			else
				s++;
		while (*s != ' ')
			if (!*s)
				return 0;
			else
				s++;

		if (strlen(s) < 7)
			return 0;
		s = &s[7];
		while (*s == ' ')
			if (!*s)
				return 0;
			else
				s++;
		while (*s != ' ')
			if (!*s)
				return 0;
			else
				s++;

		while (*s == ' ')
			if (!*s)
				return 0;
			else
				s++;
		while (*s != ' ')
			if (!*s)
				return 0;
			else
				s++;

		while (*s == ' ')
			if (!*s)
				return 0;
			else
				s++;
		while (*s != ' ')
			if (!*s)
				return 0;
			else
				*t++ = *s++;
		*t = 0;

		return 1;
	}
	return 0;
}

int getfreeulp(const char *lname, char *ulpath, size_t ulpathlen, int mode)
{
	FILE *fp;
	uint64_t total = 0;
	uint64_t fspace;

	if (!(fp = fopen(lname, "r")))
		return 0;

	while (fgetsnolf(ulpath, ulpathlen, fp)) {
		fspace = get_free_space(ulpath);
		if (fspace == (uint64_t) -1LL) {
			syslog(LOG_ERR, "cannot statfs %.200s: %m", ulpath);
			continue;
		}

		if (mode) 
			total += fspace;
		else {
			if (fspace > maincfg.CFG_FREEHDDSPACE) {
				fclose(fp);
				return 1;
			}
		}
	}
	fclose(fp);
	if (mode)
		return total;

	return 0;
}
