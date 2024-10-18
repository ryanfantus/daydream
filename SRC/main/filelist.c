#include <syslog.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <daydream.h>
#include <ddcommon.h>
#include <symtab.h>

static int screenl; 
char monthlist[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

static void ShowFile(char *, char *, int);
static void strcupr(char *, char *);
static char *mystrc(char *, char *);
static char *mystrcn(char *, char *, int);

/* Filescan

   mode : 1 = Regular, 2 = New Files, 3 = Zippy Search, 4 = Global new scan

 */

struct cursordat {
	int cd_line;
	char cd_file[40];
};

static list_t *area_list = NULL;

static int fileprompt(int, list_t **, struct cursordat *, 
		      struct cursordat *, int);

int filelist(int mode, char *params)
{
	struct tm *myt = 0;
	struct tm teem;
	struct cursordat *cdat;
	const char *srcstrh;
	char parbuf[500];
	char databuf[600];
	char zippystrh[500];
	char *zipmatch = 0;
	int ziplen = 0;
	time_t scannewer;
	int gopr = 1;
	int entries = 0, entriesinarea = 1;
	int contmode = 0;
	list_t *iterator;

	changenodestatus("Scanning filedirs");
	if (!(conference()->conf.CONF_FILEAREAS)) {
		DDPut(sd[flnoareasstr]);
		return 0;

	}
	srcstrh = params;

	if (mode == 2 || mode == 4) {
		time_t temptime;

		if (!(srcstrh = strspa(srcstrh, parbuf, 500))) {

			myt = localtime(&last);
			ddprintf(sd[fldatestr], myt->tm_mday, myt->tm_mon + 1, myt->tm_year % 100);
			databuf[0] = 0;
			if (!(Prompt(databuf, 44, 0)))
				return 0;
			srcstrh = databuf;
			*parbuf = 0;
			srcstrh = strspa(srcstrh, parbuf, 500);
		}
		if (parbuf[0] == 0 || (!strcasecmp(parbuf, "s"))) {
			myt = localtime(&last);
			myt->tm_sec = 0;
			myt->tm_min = 0;
			myt->tm_hour = 0;
			scannewer = mktime(myt);
		} else if (!strcasecmp(parbuf, "t")) {
			time(&temptime);
			myt = localtime(&temptime);
			myt->tm_sec = myt->tm_min = myt->tm_hour = 0;
			scannewer = mktime(myt);
			myt = localtime(&scannewer);
		} else if (parbuf[0] == '-') {
			int days;

			days = atoi(&parbuf[1]);
			time(&temptime);
			myt = localtime(&temptime);
			myt->tm_sec = 0;
			myt->tm_min = 0;
			myt->tm_hour = 0;
			scannewer = mktime(myt);
			scannewer = scannewer - (days * 86400);
			myt = localtime(&scannewer);
		} else {
			char numb[4];

			if (strlen(parbuf) != 6)
				return 0;
			numb[0] = parbuf[0];
			numb[1] = parbuf[1];
			numb[2] = 0;
			teem.tm_sec = 0;
			teem.tm_min = 0;
			teem.tm_hour = 0;
			teem.tm_mday = atoi(numb);
			numb[0] = parbuf[2];
			numb[1] = parbuf[3];
			teem.tm_mon = atoi(numb) - 1;
			numb[0] = parbuf[4];
			numb[1] = parbuf[5];
			teem.tm_year = atoi(numb);
			/* This is a Y2K hack, but since the beginning
			 * of the UNIX epoch was 1970, it's OK.
			 */
			if (teem.tm_year < 70)
				teem.tm_year += 100;
			scannewer = mktime(&teem);
			myt = &teem;
		}
	} else if (mode == 3) {
		if (!(srcstrh = strspa(srcstrh, zippystrh, 500))) {
			DDPut(sd[flzippystr]);
			databuf[0] = 0;
			if (!(Prompt(databuf, 44, 0)))
				return 0;
			srcstrh = databuf;
			if (!(srcstrh = strspa(srcstrh, zippystrh, 500)))
				return 0;
		}
		strupr(zippystrh);
		ziplen = strlen(zippystrh);
	} else if (mode == 1) {
		if (!(strspace(zippystrh, srcstrh, sizeof zippystrh) <=
		      sizeof zippystrh && *zippystrh))
			TypeFile("filecatalogs", TYPE_MAKE | TYPE_WARN | TYPE_CONF);
	}
	while (gopr) {
		gopr = 1;
		if (!(srcstrh = strspa(srcstrh, parbuf, 500))) {
			ddprintf(sd[fldirsstr], conference()->conf.CONF_FILEAREAS);
			databuf[0] = 0;

			if (!(Prompt(databuf, 70, 0))) {
				while (area_list)
					shift(int, area_list);
				return 0;
			}
			srcstrh = databuf;
			if (!(srcstrh = strspa(srcstrh, parbuf, 500))) {
				while (area_list)
					shift(int, area_list);
				return 0;
			}
		}

		do {
			if (!strcasecmp(parbuf, "a")) {
				int i;
				for (i = 1; i <= conference()->conf.CONF_FILEAREAS; i++)
					area_list = sorted_insert(area_list, (void *) i, int_sortfn);
				
				gopr = 0;
				break;
			} else if (!strcasecmp(parbuf, "h")) {
				TypeFile("filecataloghelp", TYPE_MAKE | TYPE_WARN | TYPE_CONF);
				gopr = 2;
				break;
			} else if (!strcasecmp(parbuf, "l")) {
				TypeFile("filecatalogs", TYPE_MAKE | TYPE_WARN | TYPE_CONF);
				gopr = 2;
				break;
			} else {
				int newarea;

				if (!strcasecmp(parbuf, "u")) {
					newarea = conference()->conf.CONF_UPLOADAREA;
				} else
					newarea = atoi(parbuf);
				
				if (newarea && !exists_in_list(area_list, (void *) newarea, int_sortfn))
					area_list = sorted_insert(area_list, (void *) newarea, int_sortfn);
			}
		} while ((srcstrh = strspa(srcstrh, parbuf, 500)));
		if (gopr == 1)
			break;
	}

	cdat = (struct cursordat *) xmalloc((user.user_screenlength + 2) * sizeof(struct cursordat));
	
	for (iterator = area_list; iterator; iterator = cdr(iterator)) {
		char *listm;
		int listfd;
		int listsize;
		struct stat st;
		char descbuf[6000];
		char *endp;
		char *s, *t;
		int nono;
		
		struct cursordat *currc;
		int ml;


		nono = 1;
		if (entriesinarea) 
			ddprintf(sd[flscanningstr], car(int, iterator));
		else 
			ddprintf("[3D%3.3d", car(int, iterator));
		entriesinarea = 0;

		snprintf(parbuf, sizeof parbuf, "%s/data/directory.%3.3d", 
			conference()->conf.CONF_PATH, car(int, iterator));
		listfd = open(parbuf, O_RDONLY);
		
		if (listfd == -1) {
			syslog(LOG_WARNING, "cannot open %s: %m", parbuf);
			continue;
		}

		screenl = 4;
		currc = cdat;
		currc->cd_line = 0;
		
		if (fstat(listfd, &st) == -1) {
			syslog(LOG_WARNING, "cannot stat: %m");
			close(listfd);
			continue;
		}
		listsize = st.st_size;
		
                if(listsize == 0) {
                   close(listfd);
                   continue;
                }

  		listm = (char *) mmap(0, listsize, PROT_READ, 
			MAP_SHARED, listfd, 0);
		if (listm == (char *) -1) {
			syslog(LOG_WARNING, "cannot mmap: %m");
			close(listfd);
			DDPut("Cannot mmap();\n");
			break;
		}
		
		close(listfd);
		
		endp = listm + listsize;
		s = listm;
		
		while (s < endp) {
			int flins = 1;
			if (!user.user_flines)
				ml = 500;
			else {
				ml = user.user_flines;
				if (conference()->conf.CONF_ATTRIBUTES & (1L << 3))
					ml++;
			}
			t = descbuf;
			for (;;) {
				if (s == endp)
					break;
				if (*s == 10 && ((s + 1) == endp || *(s + 1) != ' '))
					break;
				if (*s == 10) {
					if (ml) {
						flins++;
						ml--;
					}
				}
				if (ml) {
					*t++ = *s++;
				} else
					s++;
			}
			s++;
			*t++ = 10;
			*t = 0;
			
			if (mode == 3) {
				char upbuf[8000];
				strcupr(upbuf, descbuf);
				if (!(zipmatch = strstr(upbuf, zippystrh)))
					continue;
				zipmatch = zipmatch - upbuf + descbuf;
			}
			if (mode == 2 || mode == 4) {
				struct tm teemh;
				
				if (conference()->conf.CONF_ATTRIBUTES & (1L << 3)) {
					char *s;
					char numb[5];
					
					s = monthlist;
					for (teemh.tm_mon = 0; teemh.tm_mon < 13; teemh.tm_mon++) {
						if (!strncmp(s, &descbuf[52], 3))
							break;
						s = &s[3];
					}
					numb[0] = descbuf[56];
					numb[1] = descbuf[57];
					numb[2] = 0;
					teemh.tm_sec = 0;
					teemh.tm_min = 0;
					teemh.tm_hour = 0;
					teemh.tm_mday = atoi(numb);
					memcpy(numb, descbuf + 68, 4);
					numb[4] = 0;
					teemh.tm_year = atoi(numb) - 1900;
				} else {
					char numb[4];
					numb[0] = descbuf[26];
					numb[1] = descbuf[27];
					numb[2] = 0;
					teemh.tm_sec = 0;
					teemh.tm_min = 0;
					teemh.tm_hour = 0;
					teemh.tm_mday = atoi(numb);
					numb[0] = descbuf[29];
					numb[1] = descbuf[30];
					teemh.tm_mon = atoi(numb) - 1;
					numb[0] = descbuf[32];
					numb[1] = descbuf[33];
					teemh.tm_year = atoi(numb);
					/* Y2K hack */
					if (teemh.tm_year < 70)
						teemh.tm_year += 100;
				}
				if (teemh.tm_year < myt->tm_year)
					continue;
				if (teemh.tm_mon < myt->tm_mon && teemh.tm_year == myt->tm_year)
					continue;
				if (teemh.tm_mday < myt->tm_mday && teemh.tm_mon == myt->tm_mon && teemh.tm_year == myt->tm_year)
					continue;
			}
			if (entriesinarea == 0) {
				DDPut("\n\n");
			}
			entries++;
			entriesinarea++;
			if (screenl + flins > user.user_screenlength) {
				int hotres;
				hotres = fileprompt(contmode, &iterator, cdat, currc, 1);
				if (hotres == 3)
					contmode = 1;
				if (hotres == 0) {
					nono = 0;
					break;
				}
				currc = cdat;
				screenl = 1;
				currc->cd_line = 0;
			}
			if (!contmode) {
				currc->cd_line = screenl;
				strspa(descbuf, currc->cd_file, 6000);
				screenl += flins;
				currc++;
				currc->cd_line = 0;
			} else {
				if (HotKey(HOT_QUICK) == 3) {
					currc = cdat;
					currc->cd_line = 0;
					contmode = 0;
					screenl = user.user_screenlength;
				}
			}
			if (!checkcarrier())
				break;
			ShowFile(descbuf, zipmatch, ziplen);
		}
		if (entriesinarea && nono) {
			int hotres;
			hotres = fileprompt(contmode, &iterator, cdat, currc, 0);
			if (hotres == 3)
				contmode = 1;
		}
		
		munmap(listm, listsize);
	}

	if (entries == 0 && mode != 4) {
		DDPut(sd[flnoentriesstr]);
	} else {
		DDPut("\n");
	}
	free(cdat);
	
	while (area_list)
		shift(int, area_list);
	return 1;
}

static void fl_getbot(void)
{
	ddprintf("[%d;1H", screenl);
}

static void fl_restore(struct cursordat *cda)
{
	fl_getbot();
	DDPut("\r                                                                             \r");
	DDPut(sd[flmorestr]);
	ddprintf("[%d;%dH", cda->cd_line, strlen(cda->cd_file));
}

static int fileprompt(int contmode, list_t **curarea, struct cursordat *cdat, 
		      struct cursordat *currc, int clear)
{
	int hotres = 1;
	char parbuf[512];
	char fbuf[512];
	const char *s;
	int i;

	if (contmode == 0) {
		DDPut(sd[flmorestr]);
	} else {
		hotres = 3;
	}
	while (hotres == 1) {
		if (!checkcarrier())
			hotres = 0;
		switch (HotKey(HOT_CURSOR)) {
		case 0:
		case 'n':
		case 'N':
		case 'q':
		case 'Q':
			*curarea = NULL;
			hotres = 0;
			break;
		case 13:
		case 10:
		case 'y':
		case 'Y':
		case 251:
		case ' ':
			if (clear)
				DDPut("[2J[H");
			hotres = 2;
			break;
		case 250:
			if (cdat->cd_line) {
				DDPut(sd[flflagstr]);
			}
			currc--;
			while (currc->cd_line) {
				ddprintf(sd[fltagstr], currc->cd_line, currc->cd_file);
				switch (HotKey(HOT_CURSOR)) {
					int ii;

				case 13:
				case 10:
					ii = flagsingle(currc->cd_file, 0);
					if (ii == 0 || ii == 4) {
						DDPut(sd[starstr]);
						DDPut("[D");
						ddprintf("[%d;1H[36m%s[0m", currc->cd_line, currc->cd_file);
						currc++;
					} else if (ii == 3) {
						if (unflagfile(currc->cd_file)) {
							DDPut(" [D");
							ddprintf("[%d;1H[36m%s[0m", currc->cd_line, currc->cd_file);
							currc++;
						}
					} else if (ii == 1) {
						fl_getbot();
						DDPut(sd[flfrater2str]);
						HotKey(0);
						fl_restore(currc);
					} else if (ii == 2) {
						fl_getbot();
						DDPut(sd[flbrater2str]);
						HotKey(0);
						fl_restore(currc);
					} else if (ii == -1) {
						fl_getbot();
						DDPut(sd[flexister2str]);
						HotKey(0);
						fl_restore(currc);
					}
					break;
				case 250:
					if (currc == cdat)
						break;
					ddprintf(sd[fltagoffstr], currc->cd_line, currc->cd_file);
					currc--;
					break;
				case 251:
					ddprintf(sd[fltagoffstr], currc->cd_line, currc->cd_file);
					currc++;
					break;
				case 'q':
				case 'Q':
					ddprintf(sd[fltagoffstr], currc->cd_line, currc->cd_file);
					while (currc->cd_line)
						currc++;
					break;
				}
				if (!checkcarrier())
					break;
			}
			ddprintf("[%d;1H", screenl);
			DDPut(sd[flmorestr]);
			break;
		case 'c':
		case 'C':
			contmode = 1;
			hotres = 3;
			break;
		case 'f':
		case 'F':
		case 't':
		case 'T':
			DDPut(sd[flflag2str]);
			*parbuf = 0;
			if (!(Prompt(parbuf, 512, PROMPT_FILE | PROMPT_NOCRLF)))
				return 0;
			s = parbuf;
			i = 0;
			
			for (;;) {
				if (strtoken(fbuf, &s, sizeof fbuf) > sizeof fbuf)
					continue;
				if (!*fbuf)
					break;
				i += flagfile(fbuf, 0);
			}
			
			if (i || flagerror == -1) 
				ddprintf(sd[flresstr], i);
			else {
				if (flagerror == 1) {
					fl_getbot();
					DDPut(sd[flfrater2str]);
				} else if (flagerror == 2) {
					fl_getbot();
					DDPut(sd[flbrater2str]);
				}
			}
			HotKey(0);
			fl_getbot();
			DDPut("\r                                                                             \r");
			DDPut(sd[flmorestr]);
			break;

		}
	}
	return hotres;
}

static char *mystrc(char *de, char *so)
{
	while (*so)
		*de++ = *so++;
	*de = 0;
	return de;
}

static char *mystrcn(char *de, char *so, int s)
{
	int cnt = 0;

	while (*so) {
		*de++ = *so++;
		cnt++;
		if (cnt == s)
			break;
	}
	*de = 0;
	return de;
}

static void ShowFile(char *src, char *zipmatch, int ziplen)
{
	char showb[8000];
	char *s, *t;
	int zoffset;
	char fname[256];

	s = src;
	t = fname;
	while (*s != ' ')
		*t++ = *s++;
	*t = 0;

	s = src;
	t = showb;

	if (ziplen == 0)
		zoffset = 344444444;
	else
		zoffset = zipmatch - src;
	t = mystrc(t, sd[flcol1str]);
	if (conference()->conf.CONF_ATTRIBUTES & (1L << 3)) {
		if (zoffset < 34) {
			int cnt = 0;
			while (s != zipmatch) {
				*t++ = *s++;
				cnt++;
			}
			t = mystrc(t, sd[flz1str]);
			for (; ziplen; ziplen--) {
				*t++ = *s++;
				cnt++;
			}
			t = mystrc(t, sd[flz2str]);
			t = mystrcn(t, s, 35 - cnt);
			s = &s[35 - cnt];
		} else {
			if (isfiletagged(fname)) {
				int cnt;
				t = mystrc(t, fname);
				t = mystrc(t, sd[starstr]);

				for (cnt = 35 - (strlen(fname) + 1); cnt; cnt--) {
					*t++ = ' ';
				}
				*t = 0;
				s = &s[35];
			} else {
				t = mystrcn(t, s, 35);
				s = &s[35];
			}
		}
		t = mystrc(t, sd[flcol2str]);
		t = mystrcn(t, s, 4);
		s = &s[4];
		t = mystrc(t, sd[flcol3str]);
		t = mystrcn(t, s, 9);
		s = &s[9];
		t = mystrc(t, sd[flcol4str]);
		t = mystrcn(t, s, 25);
		s = &s[25];
	} else {
		if (zoffset < 13) {
			int cnt = 0;
			while (s != zipmatch) {
				*t++ = *s++;
				cnt++;
			}
			t = mystrc(t, sd[flz1str]);
			for (; ziplen; ziplen--) {
				*t++ = *s++;
				cnt++;
			}
			t = mystrc(t, sd[flz2str]);
			t = mystrcn(t, s, 13 - cnt);
			s = &s[13 - cnt];
		} else {
			if (isfiletagged(fname)) {
				int cnt;
				t = mystrc(t, fname);
				t = mystrc(t, sd[starstr]);

				for (cnt = 13 - (strlen(fname) + 1); cnt; cnt--) {
					*t++ = ' ';
				}
				*t = 0;
				s = &s[13];
			} else {
				t = mystrcn(t, s, 13);
				s = &s[13];
			}
		}
		t = mystrc(t, sd[flcol2str]);
		t = mystrcn(t, s, 5);
		s = &s[5];
		t = mystrc(t, sd[flcol3str]);
		t = mystrcn(t, s, 8);
		s = &s[8];
		t = mystrc(t, sd[flcol4str]);
		t = mystrcn(t, s, 9);
		s = &s[9];
	}
	t = mystrc(t, sd[flcol5str]);

	if (!ziplen) {
		strcpy(t, s);
	} else {
		while (s != zipmatch && *s) {
			*t++ = *s++;
		}
		*t = 0;
		if (*s) {
			t = mystrc(t, sd[flz3str]);
			for (; ziplen; ziplen--) {
				*t++ = *s++;
			}
			t = mystrc(t, sd[flz4str]);
			strcpy(t, s);
		}
	}
	DDPut(showb);
}

int globalnewscan(void)
{
	conference_t *mc;
	int oldc;
	struct iterator *iterator;
	char gbuf[300];

	oldc = conference()->conf.CONF_NUMBER;

	DDPut(sd[flglobhstr]);

	iterator = conference_iterator();
	while ((mc = (conference_t *) iterator_next(iterator))) {
		if (mc->conf.CONF_FILEAREAS && *mc->conf.CONF_NEWSCANAREAS && checkconfaccess(mc->conf.CONF_NUMBER, &user) && isconftagged(mc->conf.CONF_NUMBER)) {
			ddprintf(sd[flconfstr], mc->conf.CONF_NAME);
			joinconf(mc->conf.CONF_NUMBER, JC_SHUTUP | JC_QUICK | JC_NOUPDATE);
			snprintf(gbuf, sizeof gbuf, "S %s", 
				mc->conf.CONF_NEWSCANAREAS);
			filelist(4, gbuf);
		}
	}
	iterator_discard(iterator);
	joinconf(oldc, JC_SHUTUP | JC_QUICK);
	return 1;
}

static void strcupr(char *dest, char *strh)
{
	while (*strh) 
		*dest++ = toupper(*strh++);
	*dest = 0;
}
