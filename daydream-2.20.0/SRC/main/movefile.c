#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <daydream.h>
#include <ddcommon.h>

static int getdp(conference_t *, char *, size_t, int)
	__attr_bounded__ (__string__, 2, 3);
static char *getfe(char *);
static int ltosdesc(char *, size_t, char *) __attr_bounded__ (__string__, 1, 2);
static int mff(char *, const char *, char *, int);
static int stoldesc(char *, size_t, char *) __attr_bounded__ (__string__, 1, 2);

extern char monthlist[]; /* FIXME! should be declared in daydream.h */

int movefile(char *params, int mode)
{
	char parbuf[1024];
	char vbuf[1024];
	const char *srcstrh;
	
	srcstrh = params;

	if (!params || !*params) {
		struct FFlag *myf;

		myf = (struct FFlag *) flaggedfiles->lh_Head;
		if (myf->fhead.ln_Succ) {
			DDPut(sd[move1str]);
			if (HotKey(HOT_YESNO) == 1) {
				while (myf->fhead.ln_Succ) {
					if (myf->f_conf == conference()->conf.CONF_NUMBER) {
						mf(myf->f_filename, mode);
					}
					myf = (struct FFlag *) myf->fhead.ln_Succ;
				}

			}
		}
		DDPut(sd[move2str]);
		*vbuf = 0;
		if (!(Prompt(vbuf, 80, PROMPT_FILE)))
			return 0;
		if (!*vbuf)
			return 0;
		srcstrh = vbuf;
	}
	for (;;) {
		if (strtoken(parbuf, &srcstrh, sizeof parbuf) > sizeof parbuf)
			continue;
		if (!*parbuf)
			break;
		mf(parbuf, mode);
	}
	return 1;
}

int mf(char *file, int mode)
{
	int dirsleft, currdir;
	char *fpath;

	char fb[1024];
	if (!(fpath = find_file(file, NULL))) {
		ddprintf(sd[moveerrstr], file);
		return 0;
	}
	dirsleft = conference()->conf.CONF_FILEAREAS;
	currdir = dirsleft;
	snprintf(fb, sizeof fb, "%s/data/directory.%3.3d", 
		conference()->conf.CONF_PATH, 
		conference()->conf.CONF_UPLOADAREA);

	if (mff(fb, filepart(fpath), fpath, mode))
		return 1;

	while (dirsleft) {
		if (currdir == conference()->conf.CONF_UPLOADAREA) {
			currdir--;
			dirsleft--;
		} else {
			snprintf(fb, sizeof fb, "%s/data/directory.%3.3d", 
				conference()->conf.CONF_PATH, currdir);
			if (mff(fb, filepart(fpath), fpath, mode))
				return 1;
			currdir--;
			dirsleft--;
		}
	}
	return 0;
}

static int mff(char *dr, const char *file, char *fpath, int mode)
{
	int dirhandle;
	char fndbuf[40];
	struct stat st;
	char *DirMem;
	char *fptr, *feptr;
	char descbuffer[80 * 40];
	char mfb[1024];
	int dconf;
	int darea;
	char dpath[1024];
	int fd;
	conference_t *dc;


	if (stat(dr, &st) == -1)
		return 0;

	if (!(DirMem = (char *) xmalloc(st.st_size + 2)))
		return 0;

	dirhandle = open(dr, O_RDONLY);
	read(dirhandle, &DirMem[1], st.st_size);
	close(dirhandle);
	DirMem[0] = 10;
	DirMem[st.st_size + 1] = 0;

	snprintf(fndbuf, sizeof fndbuf, "\n%s ", file);
	if ((fptr = strstr(DirMem, fndbuf))) {
		fptr++;
		feptr = getfe(fptr);

		strlcpy(descbuffer, fptr, sizeof descbuffer);

		DDPut("[0m\n");
		DDPut(fptr);
		DDPut("\n");
		free(DirMem);

		for (;;) {
			DDPut(sd[movedstr]);
			*mfb = 0;
			if (!(Prompt(mfb, 2, 0)))
				return 0;
			if (*mfb == 0)
				return 0;
			if (toupper(*mfb) == 'L') {
				TypeFile("joinconference", TYPE_MAKE | TYPE_WARN);
				continue;
			}
			dconf = atoi(mfb);
			dc = findconf(dconf);
			if (!dc) {
				DDPut(sd[movenodstr]);
				continue;
			}
			if (dc->conf.CONF_FILEAREAS == 0) {
				DDPut(sd[movenodcstr]);
				continue;
			}
			break;
		}
		if (dc->conf.CONF_FILEAREAS == 1) {
			darea = 1;
		} else {
			for (;;) {
				ddprintf(sd[movedfstr], dc->conf.CONF_FILEAREAS);
				*mfb = 0;
				if (!(Prompt(mfb, 2, 0)))
					return 0;
				if (*mfb == 0)
					return 0;
				if (toupper(*mfb) == 'L') {
					int old;
					old = conference()->conf.CONF_NUMBER;
					joinconf(dconf, JC_SHUTUP | JC_QUICK | JC_NOUPDATE);
					TypeFile("filecatalogs", TYPE_MAKE | TYPE_CONF | TYPE_WARN);
					joinconf(old, JC_SHUTUP | JC_QUICK | JC_NOUPDATE);
					continue;
				} else if (toupper(*mfb) == 'U') {
					darea = dc->conf.CONF_UPLOADAREA;
					break;
				}
				darea = atoi(mfb);
				if (darea < 1 || darea > dc->conf.CONF_FILEAREAS)
					continue;
				break;
			}
		}
		if (dc->conf.CONF_ATTRIBUTES & (1L << 3) && (!(conference()->conf.CONF_ATTRIBUTES & (1L << 3)))) {
			char newdesc[40 * 80];
			stoldesc(newdesc, sizeof newdesc, descbuffer);
			strlcpy(descbuffer, newdesc, sizeof descbuffer);
		} else if ((!(dc->conf.CONF_ATTRIBUTES & (1L << 3))) && (conference()->conf.CONF_ATTRIBUTES & (1L << 3))) {
			char newdesc[40 * 80];
			ltosdesc(newdesc, sizeof newdesc, descbuffer);
			strlcpy(descbuffer, newdesc, sizeof descbuffer);
		}
		snprintf(mfb, sizeof mfb, "%s/data/directory.%3.3d", 
			dc->conf.CONF_PATH, darea);
		getdp(dc, dpath, sizeof dpath, darea);
		strlcat(dpath, file, sizeof dpath);
		if (!mode) {
			newrename(fpath, dpath);
		} else if (mode == 1) {
			symlink(fpath, dpath);
		} else {
			newcopy(fpath, dpath);
		}
		fd = open(mfb, O_WRONLY | O_CREAT, 0666);
		if (fd == -1)
			return 1;
		fsetperm(fd, 0666);
		lseek(fd, 0, SEEK_END);
		safe_write(fd, descbuffer, strlen(descbuffer));
		safe_write(fd, "\n", 1);
		close(fd);

		if (stat(dr, &st) == -1)
			return 0;

		if (!(DirMem = (char *) xmalloc(st.st_size + 2)))
			return 0;
		DirMem[st.st_size + 1] = 0;

		dirhandle = open(dr, O_RDONLY);
		if (dirhandle != -1) {
			read(dirhandle, &DirMem[1], st.st_size);
			DirMem[0] = 10;
			close(dirhandle);

			snprintf(fndbuf, sizeof fndbuf, "\n%s ", file);
			if ((fptr = strstr(DirMem, fndbuf))) {
				fptr++;
				feptr = getfe(fptr);

				if (!mode) {
					dirhandle = open(dr, O_WRONLY | O_TRUNC | O_CREAT, 0666);
					fsetperm(dirhandle, 0666);
					/* FIXME: dirhandle == -1 ? */

					safe_write(dirhandle, &DirMem[1], fptr - DirMem - 1);
					safe_write(dirhandle, feptr + 1, strlen(feptr + 1));
					close(dirhandle);
				}
			}
		}
		free(DirMem);
		return 1;
	}
	return 0;

}

static int ltosdesc(char *n, size_t nlen, char *o)
{
	int i;
	struct tm teemh;
	char numb[4];
	char *s;

	s = monthlist;
	for (teemh.tm_mon = 0; teemh.tm_mon < 13; teemh.tm_mon++) {
		if (!strncmp(s, &o[52], 3))
			break;
		s = &s[3];
	}
	numb[0] = o[56];
	numb[1] = o[57];
	numb[2] = 0;
	teemh.tm_mday = atoi(numb);
	numb[0] = o[70];
	numb[1] = o[71];
	teemh.tm_year = atoi(numb);
	/* Y2K hack */
	if (teemh.tm_year >= 100)
		teemh.tm_year -= 100;

	for (i = 0; i < 13; i++)
		*n++ = o[i];
	o = &o[35];
	for (i = 0; i < 13; i++)
		*n++ = *o++;
	snprintf(n, nlen, "%2.2d.%2.2d.%2.2d ", teemh.tm_mday, teemh.tm_mon + 1, teemh.tm_year);
	while (*n)
		n++;
	while (*o != 10)
		o++;
	o++;
	while (*o == ' ')
		o++;
	strlcpy(n, o, nlen);
	return 1;
}

static int stoldesc(char *n, size_t nlen, char *o)
{
	int i;
	struct tm teemh;
	char numb[4];
	char *s;
	numb[0] = o[26];
	numb[1] = o[27];
	numb[2] = 0;
	teemh.tm_sec = 0;
	teemh.tm_min = 0;
	teemh.tm_hour = 0;
	teemh.tm_mday = atoi(numb);
	numb[0] = o[29];
	numb[1] = o[30];
	teemh.tm_mon = atoi(numb) - 1;
	numb[0] = o[32];
	numb[1] = o[33];
	teemh.tm_year = atoi(numb);
	/* Y2K hack */
	if (teemh.tm_year < 70)
		teemh.tm_year += 100;	
	teemh.tm_wday = 0;
	teemh.tm_yday = 0;
	teemh.tm_isdst = -1;

	for (i = 0; i < 13; i++)
		n[i] = *o++;
	for (i = 13; i < 35; i++)
		n[i] = ' ';
	n = &n[i];
	for (i = 0; i < 13; i++)
		*n++ = *o++;
	while (*o != ' ')
		o++;
	o++;
	s = asctime(&teemh);
	while (*s)
		*n++ = *s++;
	for (i = 0; i < 35; i++)
		*n++ = ' ';
	while (*o)
		*n++ = *o++;
	*n = 0;
	return 1;
}

static int getdp(conference_t *dc, char *s, size_t slen, int ar)
{
	char mb[1024];
	FILE *dp;

	if (dc->conf.CONF_ULPATH[0] == '@') {
		getfreeulp(&dc->conf.CONF_ULPATH[1], s, slen, 0);
	} else {
		strlcpy(s, dc->conf.CONF_ULPATH, slen);
	}
	snprintf(mb, sizeof mb, "%s/data/paths.dat", dc->conf.CONF_PATH);
	dp = fopen(mb, "r");
	if (!dp) 
		return 1;
	
	while (ar) {
		if (!fgetsnolf(s, 1024, dp)) {
			fclose(dp);
			return 1;
		}
		ar--;
	}
	fclose(dp);
	return 1;
}

static char *getfe(char *entry)
{
	for (;;) {
		while (*entry != 10)
			entry++;
		entry++;
		switch (*entry) {
		case ' ':
			break;
		default:
			entry--;
			*entry = 0;
			return entry;
		}
	}
}
