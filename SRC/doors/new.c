#include <ddlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

static struct dif *d;

static char namecol[20];
static char flagcol[20];
static char sizecol[20];
static char datecol[20];
static char desccol[20];
static char dateline[1024];
static char cmdline[512];
static char BUFFER[2048];
static char origdir[256];
static char olddate[40];
static char *CfgMem;

static struct stat st;

static int datelines;
static int screenl;
static int listsize;
static char *listm;
static char *endp;

static struct DayDream_Conference *thisconf;


struct cursordat {
	int cd_line;
	char cd_file[80];
};

static struct cursordat *cdat;
static struct cursordat *currc;

static char *mystrc(char *, char *);
static char *mystrcn(char *, char *, int);
static char *strspa(char *, char *);
static void closeme(void);
static void CpyToLAINA(char *, char *);
static char *ExamineCfg(char *, char *);
static int showfile(char *, char *, int);
static int moreprompt(void);
static void changeact(int);
static void changelact(int);
static void fl_restore(struct cursordat *);
static void fl_getbot();
static void fakefile(char *);
static void viewfile(char *);

static int cont;
static int maxl;

static char monthlist[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

int main(int argc, char *argv[])
{
	int i;
	int arena;
	int listfd;
	char *s;
	int goon = 1;
	int cfgfd;
	char *cptr;

	*dateline = 0;
	*olddate = 0;
	cont = 0;

	if (argc == 1) {
		printf("This program requires MS Windows!\n");
		exit(1);
	}
	d = dd_initdoor(argv[1]);
	if (d == 0) {
		printf("Couldn't find socket!\n");
		exit(1);
	}
	dd_changestatus(d, "Reversing filelist");
	atexit(closeme);

	thisconf = dd_getconf(dd_getintval(d, VCONF_NUMBER));
	cdat = (struct cursordat *) malloc((dd_getintval(d, USER_SCREENLENGTH) + 2) * sizeof(struct cursordat));

	if (!thisconf) {
		dd_sendstring(d, "[37mCouldn't get conference!\n\n");
		exit(0);
	}
	if (thisconf->CONF_FILEAREAS == 0) {
		dd_sendstring(d, "[37mNo files in this conference!\n\n");
		exit(0);
	}
	dd_sendstring(d, "[2J[H[36m           DreamNew V1.0 By Hydra/Selleri. [0mLoading Filelist...");

	dd_getstrval(d, cmdline, DOOR_PARAMS);
	dd_getstrval(d, BUFFER, DD_ORIGDIR);
	sprintf(origdir, "%s/", BUFFER);
	strcat(BUFFER, "/configs/dreamnew.cfg");

	if (stat(BUFFER, &st) == -1) {
		dd_sendstring(d, "[37mCouldn't find configfile!\n\n");
		exit(0);
	}
	CfgMem = (char *) malloc(st.st_size + 2);

	cfgfd = open(BUFFER, O_RDONLY);
	i = read(cfgfd, CfgMem, st.st_size);
	close(cfgfd);
	CfgMem[i] = 0;

	i = 0;

	if ((cptr = ExamineCfg(CfgMem, "COLORNAME '"))) {
		CpyToLAINA(cptr, namecol);
	} else
		i = 1;

	if ((cptr = ExamineCfg(CfgMem, "COLORFLAGS '"))) {
		CpyToLAINA(cptr, flagcol);
	} else
		i = 1;

	if ((cptr = ExamineCfg(CfgMem, "COLORSIZE '"))) {
		CpyToLAINA(cptr, sizecol);
	} else
		i = 1;

	if ((cptr = ExamineCfg(CfgMem, "COLORDATE '"))) {
		CpyToLAINA(cptr, datecol);
	} else
		i = 1;

	if ((cptr = ExamineCfg(CfgMem, "COLORDESC '"))) {
		CpyToLAINA(cptr, desccol);
	} else
		i = 1;

	if (i) {
		dd_sendstring(d, "\nError occured when parsing config file!\n");
		exit(0);
	}
	*dateline = 0;

	if ((cptr = ExamineCfg(CfgMem, "DATELINE '"))) {
		CpyToLAINA(cptr, dateline);
		s = dateline;
		while (*s) {
			if (*s == 10)
				datelines++;
			s++;
		}
	}
	arena = atoi(cmdline);
	if (!arena)
		arena = thisconf->CONF_UPLOADAREA;

	maxl = dd_getintval(d, USER_FLINES);

	sprintf(BUFFER, "%s/data/directory.%3.3d", thisconf->CONF_PATH, arena);
	listfd = open(BUFFER, O_RDONLY);
	if (listfd != -1) {
		screenl = 3;
		currc = cdat;
		currc->cd_line = 0;

		fstat(listfd, &st);
		listsize = st.st_size;
		if (listsize < 30)
			exit(0);
		listm = mmap(0, listsize, PROT_READ, MAP_SHARED, listfd, 0);
		if (listm == (char *) -1) {
			close(listfd);
			dd_sendstring(d, "Cannot mmap();\n");
			exit(0);
		}
		close(listfd);
		endp = listm + listsize;
		s = &listm[listsize - 2];
		dd_sendstring(d, "[36m Done!\n[34m-  - - -- ----------------------------------------------------------- -- - -  -\n");

		while (goon) {
			int fli = 0;
			int ml;
			char *fend;
			char *cend;
			fend = s;

			while (1) {
				if (--s == listm) {
					--s;
					fli++;
					goon = 0;
					break;
				}
				if (*s == 10) {
					fli++;
					if (*(s + 1) != ' ') {
						break;
					}
				}
			}
			cend = &s[1];
			if (maxl) {
				char *mo;
				ml = maxl;
				if (thisconf->CONF_ATTRIBUTES & (1L << 3))
					ml++;
				fli = 0;

				mo = cend;
				while (mo != fend + 1) {
					if (*mo == 10 || mo == endp - 2) {
						ml--;
						fli++;
						if (!ml)
							break;

					}
					mo++;
				}
			}
			if (showfile(cend, fend, fli) == 0)
				break;
			if (dd_hotkey(d, HOT_QUICK) == 3)
				break;
			if (!goon)
				moreprompt();

			screenl += fli;
		}
		munmap(listm, listsize);

	}
	return 0;
}

static int moreprompt(void)
{
	int i;
	int act = 0;
	dd_sendstring(d, "[33mCursor keys to move or [0;44m MORE [0m Quit  Cont  Flag ");
	while (1) {
		i = dd_hotkey(d, HOT_CURSOR);
		if (((i == 10 || i == 13) && act == 1) || i == 'q' || i == 'Q' || i == 0 || i == -1) {
			dd_sendstring(d, "\n");
			return 0;
		} else if (((i == 10 || i == 13) && act == 0) || i == 'm' || i == 'M' || i == 251) {
			dd_sendstring(d, "[2J[H");
			screenl = 1;
			currc = cdat;
			currc->cd_line = 0;
			break;
		} else if (((i == 10 || i == 13) && act == 2) || i == 'c' || i == 'C') {
			cont = 1;
			break;
		} else if (((i == 10 || i == 13) && act == 3) || i == 'f' || i == 'F') {
			char fbuf1[80];
			char fbuf2[300];
			char *s;
			int success = 0;

			dd_sendstring(d, "\r                                                                    \r[32mEnter Filename(s) to Flag: [36m");
			*fbuf1 = 0;
			if (!(dd_prompt(d, fbuf1, 50, PROMPT_FILE | PROMPT_NOCRLF)))
				return 0;
			s = fbuf1;
			while ((s = strspa(s, fbuf2))) {
				success += dd_flagfile(d, fbuf2, 0);
			}
			sprintf(fbuf2, "\r                                                                    \r[36m%d [0mfiles successfully flagged! [35m<ENTER>[35m", success);
			dd_sendstring(d, fbuf2);
			dd_hotkey(d, 0);
			dd_sendstring(d, "\r                                                                   \r[33mCursor keys to move or ");
			changeact(act);
		} else if (i == 252) {
			if (act != 3) {
				act++;
				changeact(act);
			}
		} else if (i == 253) {
			if (act) {
				act--;
				changeact(act);
			}
		} else if (i == 250 && cdat->cd_line) {
			int lact = 0;
			char parbuf[120];

			dd_sendstring(d, "\r[33mCursor keys to move or [0;44m TAG [0m Fake  View  Quit   ");
			currc--;
			while (currc->cd_line) {
				sprintf(parbuf, "[%d;1H[44;36m%s[0m", currc->cd_line, currc->cd_file);
				dd_sendstring(d, parbuf);
				switch (dd_hotkey(d, HOT_CURSOR)) {
				case 250:
					if (currc == cdat)
						break;
					sprintf(parbuf, "[%d;1H[36m%s[0m", currc->cd_line, currc->cd_file);
					dd_sendstring(d, parbuf);
					currc--;
					break;
				case 251:
					sprintf(parbuf, "[%d;1H[36m%s[0m", currc->cd_line, currc->cd_file);
					dd_sendstring(d, parbuf);
					currc++;
					break;
				case 252:
					if (lact != 3) {
						lact++;
						changelact(lact);
					}
					break;
				case 253:
					if (lact) {
						lact--;
						changelact(lact);
					}
					break;
				case 10:
				case 13:
					switch (lact) {
						int v;
					case 0:
						v = dd_flagsingle(d, currc->cd_file, 0);
						if (v == 0 || v == 4) {
							dd_sendstring(d, "*[D");
							sprintf(parbuf, "[%d;1H[36m%s[0m", currc->cd_line, currc->cd_file);
							dd_sendstring(d, parbuf);
							currc++;
						} else if (v == 3) {
							if (dd_unflagfile(d, currc->cd_file)) {
								dd_sendstring(d, " [D");
								sprintf(parbuf, "[%d;1H[36m%s[0m", currc->cd_line, currc->cd_file);
								dd_sendstring(d, parbuf);
								currc++;
							}
						} else if (v == 1) {
							fl_getbot();
							dd_sendstring(d, "\e[32mYour fileratio doesn't allow downloading. \e[35mUpload! \e[36m<PAUSE>");
							dd_hotkey(d, 0);
							fl_restore(currc);
						} else if (v == 2) {
							fl_getbot();
							dd_sendstring(d, "\e[32mYour byteratio doesn't allow downloading. \e[35mUpload! \e[36m<PAUSE>");
							dd_hotkey(d, 0);
							fl_restore(currc);
						} else if (v == -1) {
							fl_getbot();
							dd_sendstring(d, "\e[32mFile doesn't exist on hard disk. \e[36m<PAUSE>                ");
							dd_hotkey(d, 0);
							fl_restore(currc);
						}
						break;
					case 1:
						currc->cd_line = 0;
						fakefile(currc->cd_file);
						break;
					case 2:
						currc->cd_line = 0;
						viewfile(currc->cd_file);
						break;
					case 3:
						sprintf(parbuf, "[%d;1H[36m%s[0m", currc->cd_line, currc->cd_file);
						dd_sendstring(d, parbuf);
						while (currc->cd_line)
							currc++;
						break;
					}
					break;
				case 'q':
				case 'Q':
				case 27:
					sprintf(parbuf, "[%d;1H[36m%s[0m", currc->cd_line, currc->cd_file);
					dd_sendstring(d, parbuf);
					while (currc->cd_line)
						currc++;
					break;


				case 'f':
				case 'F':
					currc->cd_line = 0;
					fakefile(currc->cd_file);
					break;
				case 'v':
				case 'V':
					currc->cd_line = 0;
					viewfile(currc->cd_file);
					break;
				case 0:
				case -1:
					return 0;
					break;

				}
			}
			sprintf(parbuf, "[%d;1H", screenl);
			dd_sendstring(d, parbuf);
			dd_sendstring(d, "[33mCursor keys to move or ");
			changeact(act);
		}
	}
	return 1;
}

static void viewfile(char *s)
{
	char buf[512];
	sprintf(buf, "v %s", s);
	dd_sendstring(d, "[2J[H");
	dd_docmd(d, buf);
	cdat->cd_line = 0;
}

static void fakefile(char *s)
{
	char buf[512];
	sprintf(buf, "FAKER %s", s);
	dd_sendstring(d, "[2J[H");
	dd_docmd(d, buf);
	cdat->cd_line = 0;
}

static void fl_getbot()
{
	char buf[32];
	sprintf(buf, "[%d;1H", screenl);
	dd_sendstring(d, buf);
}

static void fl_restore(struct cursordat *cda)
{
	char buf[32];

	fl_getbot();
	dd_sendstring(d, "\r                                                                             \r");
	dd_sendstring(d, "[33mCursor keys to move or ");
	changelact(0);

	sprintf(buf, "[%d;%dH", cda->cd_line, strlen(cda->cd_file));
	dd_sendstring(d, buf);
}

static void changeact(int a)
{
	char lbuf[80];
	switch (a) {
	case 0:
		sprintf(lbuf, "[%d;24H[0;44m MORE [0m Quit  Cont  Flag ", screenl);
		break;
	case 1:
		sprintf(lbuf, "[%d;24H[0m More [44m QUIT [0m Cont  Flag ", screenl);
		break;
	case 2:
		sprintf(lbuf, "[%d;24H[0m More  Quit [44m CONT [0m Flag ", screenl);
		break;
	case 3:
		sprintf(lbuf, "[%d;24H[0m More  Quit  Cont [44m FLAG [0m", screenl);
		break;
	}
	dd_sendstring(d, lbuf);
}

static void changelact(int a)
{
	char lbuf[80];
	switch (a) {
	case 0:
		sprintf(lbuf, "[%d;24H[0;44m TAG [0m Fake  View  Quit ", screenl);
		break;
	case 1:
		sprintf(lbuf, "[%d;24H[0m Tag [44m FAKE [0m View  Quit ", screenl);
		break;
	case 2:
		sprintf(lbuf, "[%d;24H[0m Tag  Fake [44m VIEW [0m Quit ", screenl);
		break;
	case 3:
		sprintf(lbuf, "[%d;24H[0m Tag  Fake  View [44m QUIT [0m", screenl);
		break;
	}
	dd_sendstring(d, lbuf);
}

static int showfile(char *s, char *en, int li)
{
	char myb[400];
	char *t;
	int dls = 0;
	char fname[256];

	t = myb;
	t = mystrc(t, namecol);

	if (!*olddate) {
		if (thisconf->CONF_ATTRIBUTES & (1L << 3)) {
			strncpy(olddate, &s[48], 10);
		} else {
			strncpy(olddate, &s[26], 8);
		}
	} else if (*dateline) {
		if (thisconf->CONF_ATTRIBUTES & (1L << 3)) {
			if (strncmp(&s[48], olddate, 10)) {
				dls = datelines + 1;
				strncpy(olddate, &s[48], 10);
			}
		} else {
			if (strncmp(&s[26], olddate, 8)) {
				dls = datelines + 1;
				strncpy(olddate, &s[26], 8);
			}
		}
	}
	if ((screenl + li + dls > dd_getintval(d, USER_SCREENLENGTH)) && cont == 0) {
		if (moreprompt() == 0)
			return 0;
	}
	if (dls) {
		char tmp[10];
		int da, mo, ye;
		char day[10];
		char date[20];
		char *r, *t, *u;
		char fbuf[600];
		struct tm tm;

		if (thisconf->CONF_ATTRIBUTES & (1L << 3)) {
			strncpy(day, &s[48], 3);
			day[3] = 0;
			strncpy(tmp, &s[56], 2);
			da = atoi(tmp);
			r = monthlist;
			for (mo = 1; mo < 13; mo++) {
				if (!strncmp(r, &s[52], 3))
					break;
				r = &r[3];
			}
			strncpy(tmp, &s[70], 2);
			ye = atoi(tmp);
			sprintf(date, "%2.2d-%2.2d-%2.2d", da, mo, ye);
		} else {
			time_t ti;
			strncpy(tmp, &s[26], 2);
			tm.tm_mday = atoi(tmp);

			strncpy(tmp, &s[29], 2);
			tm.tm_mon = atoi(tmp) - 1;

			strncpy(tmp, &s[32], 2);
			tm.tm_year = atoi(tmp);
			/* Y2K hack */
			if (tm.tm_year < 70)
				tm.tm_year += 100;
			
			tm.tm_sec = tm.tm_min = tm.tm_hour = tm.tm_wday = 0;
			ti = mktime(&tm);
			t = ctime(&ti);
			strncpy(day, t, 3);
			day[3] = 0;
			sprintf(date, "%2.2d-%2.2d-%2.2d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year % 100);
		}
		r = fbuf;
		t = dateline;
		while (*t) {
			if (*t == '%' && *(t + 1) == 'a') {
				u = day;
				while (*u)
					*r++ = *u++;
				t += 2;
			} else if (*t == '%' && *(t + 1) == 'b') {
				u = date;
				while (*u)
					*r++ = *u++;
				t += 2;
			} else
				*r++ = *t++;
		}
		*r = 0;
		dd_sendstring(d, fbuf);
		dd_sendstring(d, "\n");
		screenl += dls;
	}
	if (!cont) {
		currc->cd_line = screenl;
		strspa(s, currc->cd_file);
		currc++;
		currc->cd_line = 0;
	} {
		char *s1 = s;
		char *s2 = fname;
		while (*s1 != ' ')
			*s2++ = *s1++;
		*s2 = 0;
	}
	if (thisconf->CONF_ATTRIBUTES & (1L << 3)) {
		if (dd_isfiletagged(d, fname)) {
			int cnt;
			t = mystrc(t, fname);
			t = mystrc(t, "[0m*");

			if (thisconf->CONF_ATTRIBUTES & (1L << 11)) {
			  for (cnt = 115 - (strlen(fname) + 1); cnt; cnt--) {
				*t++ = ' ';
			  } 
			} else {
			  for (cnt = 35 - (strlen(fname) + 1); cnt; cnt--) {
				*t++ = ' ';
			  }
			}
			*t = 0;
			if (thisconf->CONF_ATTRIBUTES & (1L << 11)) {
				s = &s[115];
			} else {
				s = &s[35];
			}
		} else {
			if (thisconf->CONF_ATTRIBUTES & (1L << 11)) {
				t = mystrcn(t, s, 115);
				s = &s[115];
			} else {
				t = mystrcn(t, s, 35);
				s = &s[35];
			}
		}
		t = mystrc(t, flagcol);
		t = mystrcn(t, s, 4);
		s = &s[4];
		t = mystrc(t, sizecol);
		t = mystrcn(t, s, 9);
		s = &s[9];
		t = mystrc(t, datecol);
		t = mystrcn(t, s, 25);
		s = &s[25];
		t = mystrc(t, desccol);
		dd_sendstring(d, myb);
		li--;
	} else {
		if (dd_isfiletagged(d, fname)) {
			int cnt;
			t = mystrc(t, fname);
			t = mystrc(t, "[0m*");

			for (cnt = 13 - (strlen(fname) + 1); cnt; cnt--) {
				*t++ = ' ';
			}
			*t = 0;
			s = &s[13];
		} else {
			t = mystrcn(t, s, 13);
			s = &s[13];
		}
		t = mystrc(t, flagcol);
		t = mystrcn(t, s, 5);
		s = &s[5];
		t = mystrc(t, sizecol);
		t = mystrcn(t, s, 8);
		s = &s[8];
		t = mystrc(t, datecol);
		t = mystrcn(t, s, 9);
		s = &s[9];
		dd_sendstring(d, myb);

	}

	while (li) {
		t = myb;
		t = mystrc(t, desccol);
		while (*s != 10 && s != endp) {
			*t++ = *s++;
		}
		*t = 0;
		s++;
		li--;
		dd_sendstring(d, myb);
		dd_sendstring(d, "\n");
		if (s == (endp + 1))
			break;

	}
	return 1;
}

static void closeme(void)
{
	if (CfgMem)
		free(CfgMem);
	dd_close(d);
}

static char *ExamineCfg(char *hay, char *need)
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

	while (src[i] != 39) {
		dest[i] = src[i];
		i++;
	}
	dest[i] = 0;
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

static char *strspa(char *src, char *dest)
{
	int go = 1;

	if (src == 0)
		return 0;

	while (*src == ' ')
		src++;
	if (*src == 0)
		return 0;
	while (go) {
		if (*src == ' ' || *src == 0)
			go = 0;
		else
			*dest++ = *src++;
	}
	*dest = 0;
	return src;
}
