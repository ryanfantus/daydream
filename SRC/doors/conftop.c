/*
   ConfTop V1.0 for DayDream BBS
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ddlib.h>

#define APTR char *
#define MEMF_CLEAR (1L<<0)
#define WORD short
#define SendString dd_sendstring
#define Open open
#define Read read
#define Close close
#define BPTR int
#define Seek lseek
#define OFFSET_END SEEK_END
#define OFFSET_BEGINNING SEEK_SET
#define MODE_NEWFILE O_RDWR | O_CREAT | O_TRUNC, 0664
#define Write write
#define DeleteFile unlink
#define ChangeActivity dd_changestatus
#define Pause dd_pause
#define JoinConference dd_joinconf

struct DayDream_Conference *thisconf;

struct ConfTopData {
	uint64_t ULBytes;
	uint64_t ULFiles;
	uint32_t FirstC;
};

struct ConfTopDates {
	uint32_t ctd_Start;
	uint32_t ctd_LastWS;
	uint32_t ctd_CurrWS;
};

static void AddData(char *, int, uint32_t, uint32_t);
static char *ExamineCfg(char *, char *);
static int get_file_size(char *);
static void GenTop(char *file, uint32_t, uint32_t, uint32_t);
static char *StyleNum(char *bufferi, uint64_t num);
static void CpyToLAINA(char *src, char *dest);
static void CpyToEOL(char *src, char *dest);
static void CpyToHIPSU(char *src, char *dest);
static void *AllocMem(int, int);
static int enddate(int);

static char *CfgMem = 0;
static struct dif *d;
static int ConfigSize = 0;
static int maxlines = 15;

static char Header[2000];
static char Line[500];
static char Tail[2000];

static void CloseMe(void);

static struct ConfTopDates ctdates;

static char BUFFER[2048];
static char BUFFER2[2048];
static char origdir[500];
static struct userbase *UserBase = 0;
static int UBSize = 0;
static struct stat st;

static char ubname[1024];


int main(int argc, char *argv[])
{
	BPTR CfgHandle;
	char cmdline[512];
	BPTR ULHandle;
	int ULSize;
	char *p;
	time_t stamp;

	if (argc == 1) {
		printf("Brr.. Requires DD!\n");
		exit(0);
	}
	d = dd_initdoor(argv[1]);	/* Establish link with DD */
	dd_changestatus(d, "Running ConfTop..");

	sprintf(ubname, "%s/data/userbase.dat", getenv("DAYDREAM"));
	if (d == NULL) {
		printf("Hey Asswipe, this program requires DayDream BBS!\n");
		exit(10);
	}
	dd_getstrval(d, cmdline, DOOR_PARAMS);

	atexit(CloseMe);


	thisconf = dd_getconf(dd_getintval(d, VCONF_NUMBER));
	if (!thisconf) {
		SendString(d, "[37mCouldn't get conference!\n\n");
		exit(0);
	}
	if (thisconf->CONF_FILEAREAS == 0) {
		SendString(d, "[37mNo files in this conference!\n\n");
		exit(0);
	}
	dd_getstrval(d, BUFFER, DD_ORIGDIR);
	sprintf(origdir, "%s/", BUFFER);
	strcat(BUFFER, "/configs/dd-conftop.cfg");

	stamp = time(0);
	stamp = stamp / (60 * 60 * 24);
	if (stat(BUFFER, &st) == -1) {
		SendString(d, "[37mCouldn't find configfile!\n\n");
		exit(0);
	}
	ConfigSize = st.st_size;
	CfgMem = (char *) malloc(st.st_size + 2);

	CfgHandle = Open(BUFFER, O_RDONLY);
	Read(CfgHandle, CfgMem, ConfigSize);
	Close(CfgHandle);


	if ((p = ExamineCfg(CfgMem, "HEADER \"")))
		CpyToLAINA(p, Header);
	else
		strcpy(Header, "\n[44;33m No | Name                  | Organization             | Upload bytes | Files[0m\n\n");

	if ((p = ExamineCfg(CfgMem, "LINE \"")))
		CpyToLAINA(p, Line);
	else
		strcpy(Line, "[0m %02ld.  [33m%-23.23s [32m%-25.25s [36m%15.15s [35m%5.5s\n");

	if ((p = ExamineCfg(CfgMem, "TAIL \"")))
		CpyToLAINA(p, Tail);
	else
		strcpy(Tail, "\n[44;33m UP: %15.15s kbytes / %7.7s files | DD-ConfTop v1.0   Hydra/insane [0m\n[44;33m Start Date : %s                                 End Date : %s [0m\n\n");

	if ((p = ExamineCfg(CfgMem, "MAXLINES "))) {
		CpyToEOL(p, BUFFER);
		maxlines = atoi(BUFFER);
	}
	sprintf(BUFFER, "%s/data/dd-conftopdates.dat", thisconf->CONF_PATH);
	ULHandle = Open(BUFFER, O_RDONLY);
	if (ULHandle != -1) {
		Read(ULHandle, &ctdates, sizeof(struct ConfTopDates));
		Close(ULHandle);
	} else {
		ctdates.ctd_Start = stamp;
		ctdates.ctd_CurrWS = stamp;
	}

	sprintf(BUFFER, "%s/data/dd-conftopall.dat", thisconf->CONF_PATH);

	if (argc == 2) {

		if (stat(cmdline, &st) == -1) {
			SendString(d, "[37mCan't open uploaded file!\n\n");
			exit(0);
		}
		ULSize = st.st_size;

		AddData(BUFFER, dd_getintval(d, USER_ACCOUNT_ID), ULSize, dd_getintval(d, USER_FIRSTCALL));

		if (enddate(ctdates.ctd_CurrWS) < stamp) {

			UBSize = get_file_size(ubname);

			if ((UserBase = AllocMem(UBSize, 0))) {
				ULHandle = Open(ubname, O_RDONLY);
				Read(ULHandle, UserBase, UBSize);
				Close(ULHandle);

				sprintf(BUFFER, "%s/data/dd-conftopthisweek.dat", thisconf->CONF_PATH);
				GenTop(BUFFER, ctdates.ctd_CurrWS, enddate(ctdates.ctd_CurrWS), 1);
				sprintf(BUFFER, "%s/data/dd-conftoplastweek.dat", thisconf->CONF_PATH);
				sprintf(BUFFER2, "%s/data/dd-conftopthisweek.dat", thisconf->CONF_PATH);
				DeleteFile(BUFFER);
				rename(BUFFER2, BUFFER);
				ctdates.ctd_LastWS = ctdates.ctd_CurrWS;
				ctdates.ctd_CurrWS = stamp;
			}
		}
		sprintf(BUFFER, "%s/data/dd-conftopthisweek.dat", thisconf->CONF_PATH);
		AddData(BUFFER, dd_getintval(d, USER_ACCOUNT_ID), ULSize, dd_getintval(d, USER_FIRSTCALL));

	} else {
		UBSize = get_file_size(ubname);

		UserBase = AllocMem(UBSize, 0);
		ULHandle = Open(ubname, O_RDONLY);
		Read(ULHandle, UserBase, UBSize);
		Close(ULHandle);

		if (!(strcasecmp("last", argv[2]))) {
			sprintf(BUFFER, "%s/data/dd-conftoplastweek.dat", thisconf->CONF_PATH);
			GenTop(BUFFER, ctdates.ctd_LastWS, enddate(ctdates.ctd_LastWS), 0);
		}
		if (!(strcasecmp("this", argv[2]))) {
			sprintf(BUFFER, "%s/data/dd-conftopthisweek.dat", thisconf->CONF_PATH);
			GenTop(BUFFER, ctdates.ctd_CurrWS, enddate(ctdates.ctd_CurrWS), 0);
		}
		if (!(strcasecmp("all", argv[2]))) {
			GenTop(BUFFER, ctdates.ctd_Start, stamp, 0);
		}
	}
	sprintf(BUFFER, "%s/data/dd-conftopdates.dat", thisconf->CONF_PATH);
	if ((ULHandle = Open(BUFFER, O_RDWR | O_CREAT | O_TRUNC, 0660)) > -1) {
		Write(ULHandle, &ctdates, sizeof(struct ConfTopDates));
		Close(ULHandle);
	}
	return 0;
}

static int get_file_size(char *fileh)
{
	struct stat sta;

	if (!stat(fileh, &sta)) {
		return sta.st_size;
	} else
		return 0;
}

static void GenTop(char *file, uint32_t startd, uint32_t endd, uint32_t mode)
{
	struct ConfTopData *ctd;
	int users;
	int dbsize;
	BPTR dbhandle;
	struct ConfTopData *currbest;
	struct ConfTopData *curr;
	int dohowmany;
	int etsi;
	struct userbase *TheUser;
	int bestnum;
	int currnum;
	char ubbuf[40];
	char ufbuf[40];
	uint64_t totk = 0;
	uint32_t totf = 0;
	FILE *msghandle;
	char startds[30];
	char endds[30];
	char buff[200];
	char MsgCom[300];
	char *p;
	uint32_t usersinconf = 0;
	uint32_t conf;
	time_t nahkadt;
	struct tm *stime;

	if (mode) {
		sprintf(buff, "MSGCOM%d '", thisconf->CONF_NUMBER);
		if ((p = ExamineCfg(CfgMem, buff)))
			CpyToHIPSU(p, MsgCom);
		else
			return;

		sprintf(buff, "/tmp/msg.%d", dd_getintval(d, USER_ACCOUNT_ID));
		if (!(msghandle = fopen(buff, "w")))
			return;
	}
	conf = thisconf->CONF_NUMBER - 1;

	nahkadt = startd * 60 * 60 * 24;
	stime = gmtime(&nahkadt);
	sprintf(startds, "%2.2d.%2.2d.%2.2d", stime->tm_mday, stime->tm_mon + 1, stime->tm_year % 100);

	nahkadt = endd * 60 * 60 * 24;
	stime = gmtime(&nahkadt);
	sprintf(endds, "%2.2d.%2.2d.%2.2d", stime->tm_mday, stime->tm_mon + 1, stime->tm_year % 100);


	dbsize = get_file_size(file);
	if (!(ctd = AllocMem(dbsize, 0)))
		return;

	dbhandle = Open(file, O_RDONLY);
	Read(dbhandle, ctd, dbsize);
	Close(dbhandle);

	users = dbsize / sizeof(struct ConfTopData);

	curr = ctd;

	if (ExamineCfg(CfgMem, "NOOPERATOR"))
		ctd->ULBytes = 0;

	currnum = 0;
	for (etsi = UBSize / sizeof(struct userbase); etsi; etsi--) {
		TheUser = UserBase + currnum;

		if (conf < 32) {
			if (TheUser->user_conferenceacc1 & (1L << conf))
				usersinconf++;
		} else {
			if (TheUser->user_conferenceacc2 & (1L << (conf - 32)))
				usersinconf++;
		}
		currnum++;
	}

	for (etsi = users; etsi; etsi--) {
		totk = totk + (curr->ULBytes / 1024);
		totf = totf + curr->ULFiles;
		curr = curr + 1;
	}

	sprintf(BUFFER2, Header, thisconf->CONF_NAME, usersinconf);
	if (mode)
		fputs(BUFFER2, msghandle);
	else
		SendString(d, BUFFER2);

	for (dohowmany = 1; dohowmany < maxlines + 1; dohowmany++) {
		curr = ctd;
		currbest = 0;
		bestnum = currnum = 0;
		for (etsi = users; etsi; etsi--) {
			if (curr->ULBytes) {
				if (currbest) {
					if (curr->ULBytes > currbest->ULBytes) {
						bestnum = currnum;
						currbest = curr;
					}
				} else {
					currbest = curr;
					bestnum = currnum;
				}
			}
			curr = curr + 1;
			currnum++;
		}
		if (currbest) {
			TheUser = UserBase + bestnum;
			sprintf(BUFFER2, Line, dohowmany, TheUser->user_handle, TheUser->user_organization, StyleNum(ubbuf, currbest->ULBytes), StyleNum(ufbuf, currbest->ULFiles));
			if (mode)
				fputs(BUFFER2, msghandle);
			else
				SendString(d, BUFFER2);
			currbest->ULBytes = 0;
		} else
			dohowmany = 600;
	}

	sprintf(BUFFER2, Tail, StyleNum(ubbuf, totk), StyleNum(ufbuf, totf), startds, endds);
	if (mode)
		fputs(BUFFER2, msghandle);
	else
		SendString(d, BUFFER2);

	if (mode) {
		fclose(msghandle);

		sprintf(BUFFER2, MsgCom, buff);
		system(BUFFER2);
		DeleteFile(buff);
	}
	free(ctd);
}

static char *StyleNum(char *bufferi, uint64_t num)
{
	char tempbuf[60];
	char *tp;
	char *p;
	int buflen;
	int silmacount;

	sprintf(tempbuf, "%Lu", num);
	tp = &tempbuf[50];
	buflen = strlen(tempbuf);
	p = &tempbuf[buflen - 1];
	tp[1] = 0;
	silmacount = 3;

	while (p != tempbuf) {
		if (!silmacount) {
			*tp = '.';
			tp--;
			silmacount = 3;
		}
		*tp-- = *p--;
		silmacount--;
	}
	if (!silmacount) {
		*tp = '.';
		tp--;
	}
	*tp = *p;

	strcpy(bufferi, tp);
	return (bufferi);
}

static void *AllocMem(int size, int flags)
{
	void *mem;
	mem = malloc(size);
	if (mem && (flags & MEMF_CLEAR)) {
		memset(mem, 0, size);
	}
	return mem;
}

static void AddData(char *file, int user, uint32_t bytes, uint32_t call)
{
	int dbsize;
	BPTR dbhandle;
	APTR nullmem;
	struct ConfTopData ctd;

	dbsize = get_file_size(file);
	dbhandle = Open(file, O_RDWR | O_CREAT, 0660);
	if (dbhandle == -1)
		return;

	if (user * sizeof(struct ConfTopData) > dbsize) {
		nullmem = AllocMem(user * sizeof(struct ConfTopData) - dbsize, MEMF_CLEAR);
		Seek(dbhandle, 0, OFFSET_END);
		Write(dbhandle, nullmem, user * sizeof(struct ConfTopData) - dbsize);
		free(nullmem);
	}
	Seek(dbhandle, user * sizeof(struct ConfTopData), OFFSET_BEGINNING);
	Read(dbhandle, &ctd, sizeof(struct ConfTopData));
	Seek(dbhandle, user * sizeof(struct ConfTopData), OFFSET_BEGINNING);
	if (call == ctd.FirstC) {
		ctd.ULBytes = ctd.ULBytes + bytes;
		ctd.ULFiles++;
	} else {
		ctd.ULBytes = bytes;
		ctd.ULFiles = 1;
		ctd.FirstC = call;
	}
	Write(dbhandle, &ctd, sizeof(struct ConfTopData));
	Close(dbhandle);
}

static void CpyToEOL(char *src, char *dest)
{
	int i = 0;

	while (src[i] != 10) {
		dest[i] = src[i];
		i++;
	}
	dest[i] = 0;
}

static void CpyToLAINA(char *src, char *dest)
{
	int i = 0;

	while (src[i] != 34) {
		dest[i] = src[i];
		i++;
	}
	dest[i] = 0;
}

static void CpyToHIPSU(char *src, char *dest)
{
	int i = 0;

	while (src[i] != 39) {
		dest[i] = src[i];
		i++;
	}
	dest[i] = 0;
}

static void CloseMe(void)
{
	if (UBSize)
		free(UserBase);
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

static int enddate(int da)
{
	int i;
	da -= (252460800 / (60 * 60 * 24));
	i = ((6 - (da - 1) % 7) + da);
	return i + (252460800 / (60 * 60 * 24));
}
