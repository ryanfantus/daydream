#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ddlib.h>
#include <dd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

struct FakerDataFile {
	int16_t df_Type;
	char df_FileName[32];
	uint32_t df_FileSize;
	int16_t df_CreditFactor;
};

struct Fakerdata {
	uint16_t FD_Conf;
	char FD_Filename[26];
	char FD_Informer[26];
};

static char *ExamineCfg(char *, char *);

static void strupr(char *);
static void closeMe(void);
static void CpyToEOL(char *, char *);
static void CpyToLAINA(char *, char *);
static int ProcessLine(char *);
static void ProcessFile(void);
static int FindFile(char *);
static char *GetFe(char *);
static void GenComline(const char *source, char *dest);
static void SaveDir(char *dirikka);
static void Delete(int);
static void FreeFile(void);
static void NukeFile(void);
static void copytonuke(char *);
static void CommentFile(void);
static void ReturnCreds(void);
static void Reward(void);
static int fakelist(void);
static int LookForFile(char *);
static char *fgetsnolf(char *, int, FILE *) __attr_bounded__ (__string__, 1, 2);

static char username[40];
static char tbuf[200];
static char ubname[200];
static char ullog[200];
static char dllog[200];

static int userlen;
static int node;

static int quitme = 0;
static char *fptr;
static char *feptr;
static char *DirMem = 0;
static int DirSize;
static char *CfgMem = 0;
static struct dif *d;

struct StringMem {
	char descbuffer[10000];
	char comline1[200];
	char comline2[200];
	char comline3[200];
	char delline[500];
	char freeline[100];
	char credline[500];
	struct DD_UploadLog logbuffer[401];
};


static char BUFFER[2048];
static char BUFFER2[2048];
static uint16_t SysOpAccess;
static uint16_t RewardPercentage;
static char CurrFile[40];
static char CurrPath[260];
static uint32_t CurrSize;
static char rewarded[30];

static struct DayDream_Conference *thisconf;

static char origdir[130];
static int updatedir;

static struct StringMem *sta;

int main(int argc, char *argv[])
{
	int CfgHandle;
	char cmdline[512];
	int i;
	char *CfgPtr;
	int goon = 1;
	struct FakerDataFile dfentry;
	struct Fakerdata fdentry;
	struct stat st;

	if (argc == 1) {
		printf("Brr.. Requires DD!\n");
		exit(0);
	}
	d = dd_initdoor(argv[1]);	/* Establish link with DD */

	node = atoi(argv[1]);

	if (d == NULL) {
		printf("Hey Asswipe, this program requires DayDream BBS!\n");
		exit(10);
	}
	dd_changestatus(d, "Faking Files");

	dd_getstrval(d, cmdline, DOOR_PARAMS);

	atexit(closeMe);

	dd_getstrval(d, BUFFER, DD_ORIGDIR);
	sprintf(origdir, "%s/", BUFFER);
	strcat(BUFFER, "/configs/dreamfaker.cfg");

	if (stat(BUFFER, &st) == -1) {
		dd_sendstring(d, "[37mCouldn't find configfile!\n\n");
		exit(0);
	}
	CfgMem = (char *) malloc(st.st_size + 2);
	sprintf(tbuf, "%s/data/dreamfaker.dat", origdir);
	sprintf(ubname, "%s/data/userbase.dat", origdir);
	sprintf(ullog, "%s/logfiles/uploadlog.dat", origdir);
	sprintf(dllog, "%s/logfiles/downloadlog.dat", origdir);

	CfgHandle = open(BUFFER, O_RDONLY);
	i = read(CfgHandle, CfgMem, st.st_size);
	close(CfgHandle);
	CfgMem[i] = 0;

	if (!(CfgPtr = ExamineCfg(CfgMem, "ACCESS "))) {
		dd_sendstring(d, "[37mCan't find ACCESS-variable in configfile!\n\n");
	}
	CpyToEOL(CfgPtr, BUFFER2);
	SysOpAccess = atoi(BUFFER2);

	if (!(CfgPtr = ExamineCfg(CfgMem, "REWARD "))) {
		RewardPercentage = 0;
	} else {
		CpyToEOL(CfgPtr, BUFFER2);
		RewardPercentage = atoi(BUFFER2);
	}

	sta = (void *) malloc(sizeof(struct StringMem));
	memset(sta, 0, sizeof(struct StringMem));

	dd_getstrval(d, username, USER_HANDLE);
	strupr(username);
	username[14] = 0;
	userlen = strlen(username);

	if (argc == 3) {
		sprintf(BUFFER, "%s/users/%d/faker.dat", origdir, dd_getintval(d, USER_ACCOUNT_ID));
		CfgHandle = open(BUFFER, O_RDONLY);
		if (CfgHandle != -1) {
			dd_sendstring(d, "\n[0mYour up/download status has changed since the last call!\n\n[36mFilename               Size  Credits  U/L bytes change  D/L bytes change\n[35m========================================================================\n");
			while (read(CfgHandle, &dfentry, sizeof(struct FakerDataFile))) {
				if (dfentry.df_Type == 0) {
					sprintf(BUFFER, "[36m%-18s %8d      %3d          %8d          %8d\n", dfentry.df_FileName, dfentry.df_FileSize, dfentry.df_CreditFactor, dfentry.df_CreditFactor * dfentry.df_FileSize, 0);
				} else if (dfentry.df_Type == 1) {
					sprintf(BUFFER, "[36m%-18s %8d      %3d          %8d          %8d\n", dfentry.df_FileName, dfentry.df_FileSize, 0, 0, -dfentry.df_FileSize);
				} else {
					sprintf(BUFFER, "[36m%-18s %8d     0.%2d          %8d          %8d\n", dfentry.df_FileName, dfentry.df_FileSize, RewardPercentage, (dfentry.df_FileSize / 100) * RewardPercentage, 0);
				}
				dd_sendstring(d, BUFFER);
			}
			close(CfgHandle);
			dd_sendstring(d, "[35m========================================================================\n[34m                                                         DreamFaker V3.1[0m\n");
			dd_pause(d);
			dd_sendstring(d, "\n");
			sprintf(BUFFER, "%s/users/%d/faker.dat", origdir, dd_getintval(d, USER_ACCOUNT_ID));
			unlink(BUFFER);
		}
		if (SysOpAccess > dd_getintval(d, USER_SECURITYLEVEL)) {
			exit(0);
		} else {
			CfgHandle = open(tbuf, O_RDONLY);
			if (CfgHandle != -1) {
				dd_sendstring(d, "\n[0mFollowing files have been commented!\n\n[36mFilename               Conf      Who                                        \n[35m========================================================================\n");
				while (read(CfgHandle, &fdentry, sizeof(struct Fakerdata))) {
					sprintf(BUFFER, "[36m%-18s %8d      %s\n", fdentry.FD_Filename, fdentry.FD_Conf, fdentry.FD_Informer);
					dd_sendstring(d, BUFFER);
				}
				close(CfgHandle);
				dd_sendstring(d, "[35m========================================================================\n[34m                                                         DreamFaker V3.1[0m\n");
				dd_pause(d);
				dd_sendstring(d, "\n");
			}
			exit(0);
		}
	}
	thisconf = dd_getconf(dd_getintval(d, VCONF_NUMBER));
	if (!thisconf) {
		dd_sendstring(d, "[37mCouldn't get conference!\n\n");
		exit(0);
	}
	dd_sendstring(d, "\n                    [36mDreamFaker [0mV3.1 [36m- Code by Hydra[0m/[36mSelleri\n\n");

	dd_changestatus(d, "Faking files");

	if (thisconf->CONF_FILEAREAS == 0) {
		dd_sendstring(d, "[37mNo files in this conference!\n\n");
		exit(0);
	}
	if (SysOpAccess <= dd_getintval(d, USER_SECURITYLEVEL)) {
		int datahandle;
		struct Fakerdata datamem;
		int oldconf;

		oldconf = thisconf->CONF_NUMBER;

		datahandle = open(tbuf, O_RDONLY);
		if (datahandle != -1) {
			dd_sendstring(d, "[36mAutoprompt faked files? [35m(Yes/no) [0m: ");

			if (dd_hotkey(d, HOT_YESNO) == 1) {

				while (read(datahandle, &datamem, sizeof(struct Fakerdata))) {
					strcpy(rewarded, datamem.FD_Informer);
					dd_joinconf(d, datamem.FD_Conf, JC_SHUTUP | JC_QUICK);
					thisconf = dd_getconf(dd_getintval(d, VCONF_NUMBER));
					ProcessLine(datamem.FD_Filename);
				}
				close(datahandle);
				unlink(tbuf);
				dd_joinconf(d, oldconf, JC_SHUTUP | JC_QUICK);
			} else {
				close(datahandle);
				dd_sendstring(d, "\n[36mDelete list? [35m(Yes/no) [0m: ");

				if (dd_hotkey(d, HOT_YESNO) == 1) {
					unlink(tbuf);
				}
				dd_sendstring(d, "\n\n");
			}
		}
	}
	rewarded[0] = 0;
	fakelist();

	ProcessLine(cmdline);
	if (quitme)
		exit(0);

	while (goon) {
		dd_sendstring(d, "[36mEnter Filename(s) to Fake[34m: [0m");

		*cmdline = 0;
		if (!(dd_prompt(d, cmdline, 80, PROMPT_FILE)))
			goon = 0;
		else {
			if (cmdline[0]) {
				dd_sendstring(d, "\n");
				ProcessLine(cmdline);
				if (quitme)
					goon = 0;
			} else
				goon = 0;
		}
	}
	return 0;
}

static int fakelist(void)
{
	char buf[1024];
	int fd;
	struct FFlag fl;

	if (!dd_getintval(d, SYS_FLAGGEDFILES))
		return 0;

	dd_sendstring(d, "[36mAutoprompt flagged files? [35m(Yes/no) [0m: ");

	if (dd_hotkey(d, HOT_YESNO) == 1) {
		sprintf(buf, "%s/fakerlist.%d", DDTMP, node);
		if (dd_dumpfilestofile(d, buf)) {
			fd = open(buf, O_RDONLY);
			if (fd < 0)
				return 0;
			while (read(fd, &fl, sizeof(struct FFlag)) && !quitme) {
				if (fl.f_conf == thisconf->CONF_NUMBER) {
					ProcessLine(fl.f_filename);
				}
			}
			close(fd);
			unlink(buf);
		}
	}
	return 1;
}

static int lookdata(char *file)
{
	int datahandle;
	int pos = 0;
	struct Fakerdata datamem;

	datahandle = open(tbuf, O_RDONLY);
	if (datahandle != -1) {
		while (read(datahandle, &datamem, sizeof(struct Fakerdata))) {
			if (!strcasecmp(file, datamem.FD_Filename)) {
				close(datahandle);
				return pos;
			}
			pos++;
		}
		close(datahandle);
	}
	return -1;
}

static void adddata(int confn, char *file, char *info)
{
	int datahandle;
	struct Fakerdata datamem;

	if (lookdata(file) != -1)
		return;

	datahandle = open(tbuf, O_RDWR | O_CREAT, 0664);
	if (datahandle != -1) {
		datamem.FD_Conf = confn;
		strcpy(datamem.FD_Filename, file);
		strcpy(datamem.FD_Informer, info);
		lseek(datahandle, 0, SEEK_END);
		write(datahandle, &datamem, sizeof(struct Fakerdata));
		close(datahandle);
	}
}

static int ProcessLine(char *line)
{
	char *token;

	token = strtok(line, " ");
	while (token != NULL) {

		CurrFile[0] = 0;
		if (LookForFile(token)) {
/*                      ProcessFile();*/
		} else {
			dd_sendstring(d, "\n[35mFile not found!\n\n");
		}
		if (quitme)
			return 0;
		token = strtok(0, " ");
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

static void _splitpath(char *file, char *drive, char *path, 
	char *name, char *ext)
{
	char *s, *t;
	if (ext)
		*ext = 0;
	if (drive)
		*drive = 0;

	s = file;
	while (*s)
		s++;

	while (*s != '/') {
		if (s == file) {
			*path = 0;
			strcpy(name, file);
			return;
		} else {
			s--;
		}
	}
	s++;
	t = file;
	while (t != s) {
		*path++ = *t++;
	}
	*path = 0;
	strcpy(name, t);
}

static int LookForFile(char *fname)
{
	char buf1[512];
	char buf2[512];
	FILE *plist;
	struct stat st;
	int fi = 0;

	sprintf(buf1, "%s/faker%d", DDTMP, node);
	if (dd_findfilestolist(d, fname, buf1)) {
		plist = fopen(buf1, "r");
		if (plist) {
			while (fgetsnolf(buf2, 512, plist) && !quitme) {
				_splitpath(buf2, 0, CurrPath, CurrFile, 0);
				stat(buf2, &st);
				CurrSize = st.st_size;
				ProcessFile();
				fi++;
			}
			fclose(plist);
		}
	} else {
		unlink(buf1);
	}
	return fi;
}

static void ProcessFile(void)
{
	int currdir;
	int dirsleft;

	updatedir = 0;
	sta->comline1[0] = 0;
	sta->comline2[0] = 0;
	sta->comline3[0] = 0;
	sta->delline[0] = 0;
	sta->credline[0] = 0;

	dirsleft = thisconf->CONF_FILEAREAS;
	currdir = dirsleft;
	sprintf(BUFFER, "%s/data/directory.%3.3d", thisconf->CONF_PATH, thisconf->CONF_UPLOADAREA);

	if (FindFile(BUFFER))
		return;

	while (dirsleft) {
		if (currdir == thisconf->CONF_UPLOADAREA) {
			currdir--;
			dirsleft--;
		} else {
			sprintf(BUFFER, "%s/data/directory.%3.3d", thisconf->CONF_PATH, currdir);
			if (FindFile(BUFFER))
				return;
			currdir--;
			dirsleft--;
		}
	}
}

static int FindFile(char *dirikka)
{
	int dirhandle;
	char fndbuf[40];
	int Hot;
	int nukeloop = 1;
	struct stat st;

	if (stat(dirikka, &st) == -1)
		return 0;

	DirSize = st.st_size;

	if (!(DirMem = malloc(DirSize + 2))) {
		dd_sendstring(d, "\n[35mOut of memory!\n\n");
		return 0;
	}
	dirhandle = open(dirikka, O_RDONLY);
	read(dirhandle, &DirMem[1], DirSize);
	close(dirhandle);
	DirMem[0] = 10;

/*      strupr(CurrFile); */

	sprintf(fndbuf, "\n%s ", CurrFile);
	if ((fptr = strstr(DirMem, fndbuf))) {
		fptr++;
		feptr = GetFe(fptr);

		strcpy(sta->descbuffer, fptr);

		dd_sendstring(d, "[0m\n");
		dd_sendstring(d, fptr);
		dd_sendstring(d, "\n\n");
		free(DirMem);

		if (SysOpAccess > dd_getintval(d, USER_SECURITYLEVEL)) {
			CommentFile();
		} else {
			nukeloop = 1;
			while (nukeloop) {
				dd_sendstring(d, "[0mWhat shall I do for this file [35m[[36mn/d/r/c/f/o/W/s/q/?[35m] [0m?: ");
			      uglycoding:
				Hot = dd_hotkey(d, 0);
				switch (Hot) {
				case 13:
				case 10:
				case 'w':
				case 'W':
					dd_sendstring(d, "write it!\n\n");
					nukeloop = 0;
					break;
				case 'N':
				case 'n':
					dd_sendstring(d, "Nuke it!\n\n");
					NukeFile();
					break;
				case 'R':
				case 'r':
					dd_sendstring(d, "Return credits!\n\n");
					ReturnCreds();
					break;
				case 'o':
				case 'O':
					dd_sendstring(d, "Reward informer!\n");
					Reward();
					break;
				case 's':
				case 'S':
					dd_sendstring(d, "Skip it!\n\n");
					updatedir = 0;
					nukeloop = 0;
					break;
				case 0:
				case -1:
				case 'q':
				case 'Q':
					dd_sendstring(d, "Quit!\n\n");
					nukeloop = 0;
					quitme = 1;
					break;
				case '?':
					dd_sendstring(d, "Help!\n\n[36mN [35m- [0m Nuke file. Use this to reward/punish user.\n[36md [35m- [0m Delete file (Uppercase D to rm)\n[36mR [35m- [0m Return credits to downloaders.\n[36mC [35m- [0m Comment file.\n[36mO [35m- [0m Reward Informer.\n[36mF [35m- [0m Make file free download.\n[36mW [35m- [0m write file. (Saves modifications)\n[36mS [35m- [0m Skip current file. (Abandon changes)\n[36mQ [35m- [0m Quit.\n\n");
					break;
				case 'c':
				case 'C':
					dd_sendstring(d, "Comment!\n\n");
					CommentFile();
					break;
				case 'D':
					dd_sendstring(d, "Delete!\n\n");
					Delete(1);
					break;
				case 'd':
					dd_sendstring(d, "Delete!\n\n");
					Delete(0);
					break;
				case 'f':
				case 'F':
					dd_sendstring(d, "Free it!\n\n");
					FreeFile();
					break;
				default:
					goto uglycoding;
				}
			}
		}
		SaveDir(dirikka);
		return 1;
	} else {
		free(DirMem);
		return 0;
	}
}

static void Reward(void)
{
	int NukeHandle;
	int offset;
	struct userbase NukeUser;
	struct FakerDataFile dfentry;
	char nukebuf[500];

	if (rewarded[0] == 0) {
		dd_sendstring(d, "[0mNo-one to be rewarded!\n\n");
		return;
	}
	if (!RewardPercentage) {
		dd_sendstring(d, "[0mRewarding is not in use!\n\n");
		return;
	}
	offset = dd_findusername(d, rewarded);
	if (offset == -1) {
		dd_sendstring(d, "[0mError - couldn't find user to reward!\n\n");
		return;
	}
	NukeHandle = open(ubname, O_RDWR);
	if (NukeHandle == -1) {
		dd_sendstring(d, "[31mCouldn't access userbase!!\n\n");
		return;
	}
	lseek(NukeHandle, offset * sizeof(struct userbase), SEEK_SET);
	read(NukeHandle, &NukeUser, sizeof(struct userbase));
	close(NukeHandle);

	sprintf(nukebuf, "\n[36mHandle   [35m:[0m %-23s     [36mOrg.  [35m:[0m %s\n", NukeUser.user_handle, NukeUser.user_organization);
	dd_sendstring(d, nukebuf);
	sprintf(nukebuf, "\n[36mUploads  [35m:[0m %8Lu kB (%5d files)   [36mFakes [35m:[0m %7d kB (%5d files)\n", NukeUser.user_ulbytes / 1024, NukeUser.user_ulfiles, NukeUser.user_fakedbytes / 1024, NukeUser.user_fakedfiles);
	dd_sendstring(d, nukebuf);
	sprintf(nukebuf, "[36mDownloads[35m:[0m %8Lu kB (%5d files)   [36mRatios[35m:[0m 1:%d (byte), 1:%d (file)\n\n", NukeUser.user_dlbytes / 1024, NukeUser.user_dlfiles, NukeUser.user_byteratio, NukeUser.user_fileratio);
	dd_sendstring(d, nukebuf);

	dd_sendstring(d, "[36mReward user? [35m(Yes/no) [0m: ");

	if (dd_hotkey(d, HOT_YESNO) == 1) {

		sprintf(nukebuf, "\n[36mUploaded bytes             [35m:[0m %12Lu   [36mUploaded files             [35m:[0m %5d \n", NukeUser.user_ulbytes, NukeUser.user_ulfiles);
		dd_sendstring(d, nukebuf);
		sprintf(nukebuf, "[36m   Changed bytes           [35m:[0m %12d   [36m   Changed files           [35m:[0m %5d \n", (CurrSize / 100) * RewardPercentage, 0);
		dd_sendstring(d, nukebuf);
		dd_sendstring(d, "                             [35m------------                                -----\n");
		sprintf(nukebuf, "[36m     Bytes after           [35m:[0m %12Lu   [36m     Files after           [35m:[0m %5d \n\n", NukeUser.user_ulbytes + (CurrSize / 100) * RewardPercentage, NukeUser.user_ulfiles);
		dd_sendstring(d, nukebuf);

		NukeUser.user_ulbytes = NukeUser.user_ulbytes + (CurrSize / 100) * RewardPercentage;


		NukeHandle = open(ubname, O_RDWR);
		if (NukeHandle == -1) {
			dd_sendstring(d, "[31mCouldn't access userbase!!\n\n");
			return;
		}
		lseek(NukeHandle, offset, SEEK_SET);
		write(NukeHandle, &NukeUser, sizeof(struct userbase));
		close(NukeHandle);

		sprintf(nukebuf, "%s/users/%d/faker.dat", origdir, NukeUser.user_account_id);
		NukeHandle = open(nukebuf, O_RDWR | O_CREAT, 0664);
		if (NukeHandle != -1) {

			dfentry.df_Type = 2;
			strcpy(dfentry.df_FileName, CurrFile);
			dfentry.df_FileSize = CurrSize;
			lseek(NukeHandle, 0, SEEK_END);
			write(NukeHandle, &dfentry, sizeof(struct FakerDataFile));
			close(NukeHandle);
		}
	}
}

static void ReturnCreds(void)
{
	int NukeHandle;
	int LogHandle;
	char nukebuf[256];
	int loglen;
	struct DD_UploadLog *dllogentry;
	struct userbase NukeUser;
	struct FakerDataFile dfentry;

	dd_sendstring(d, "[0mLooking for downloaders...");

	LogHandle = open(dllog, O_RDONLY);
	if (LogHandle == -1) {
		dd_sendstring(d, "[31m Error! Could not access log file!!\n\n");
		return;
	}
	while ((loglen = read(LogHandle, sta->logbuffer, sizeof(struct DD_UploadLog) * 350))) {
		dllogentry = sta->logbuffer + loglen / (sizeof(struct DD_UploadLog));
		dllogentry->UL_SLOT = 65535;

		dllogentry = sta->logbuffer;

		while (dllogentry->UL_SLOT != 65535) {
			if (!(strcasecmp(dllogentry->UL_FILENAME, CurrFile))) {
				NukeHandle = open(ubname, O_RDWR);

				if (NukeHandle == -1) {
					dd_sendstring(d, "[31mCouldn't access userbase!!\n\n");
					return;
				}
				lseek(NukeHandle, sizeof(struct userbase) * dllogentry->UL_SLOT, SEEK_SET);
				read(NukeHandle, &NukeUser, sizeof(struct userbase));
				close(NukeHandle);

				sprintf(nukebuf, "\n\n[36mHandle   [35m:[0m %-23s     [36mOrg.  [35m:[0m %s\n", NukeUser.user_handle, NukeUser.user_organization);
				dd_sendstring(d, nukebuf);
				sprintf(nukebuf, "\n[36mUploads  [35m:[0m %8Lu kB (%5d files)   [36mFakes [35m:[0m %7d kB (%5d files)\n", NukeUser.user_ulbytes / 1024, NukeUser.user_ulfiles, NukeUser.user_fakedbytes / 1024, NukeUser.user_fakedfiles);
				dd_sendstring(d, nukebuf);
				sprintf(nukebuf, "[36mDownloads[35m:[0m %8Lu kB (%5d files)   [36mRatios[35m:[0m 1:%d (byte), 1:%d (file)\n\n", NukeUser.user_dlbytes / 1024, NukeUser.user_dlfiles, NukeUser.user_byteratio, NukeUser.user_fileratio);
				dd_sendstring(d, nukebuf);

				dd_sendstring(d, "[36mReturn credits? [35m(Yes/no) [0m: ");

				if (dd_hotkey(d, HOT_YESNO) == 1) {

					sprintf(nukebuf, "\n[36mDownloaded bytes           [35m:[0m %12Lu   [36mDownloaded files           [35m:[0m %5d \n", NukeUser.user_dlbytes, NukeUser.user_dlfiles);
					dd_sendstring(d, nukebuf);
					sprintf(nukebuf, "[36m   Changed bytes           [35m:[0m %12d   [36m   Changed files           [35m:[0m %5d \n", dllogentry->UL_FILESIZE, 1);
					dd_sendstring(d, nukebuf);
					dd_sendstring(d, "                             [35m------------                                -----\n");
					sprintf(nukebuf, "[36m     Bytes after           [35m:[0m %12Lu   [36m     Files after           [35m:[0m %5d \n\n", (int64_t) (NukeUser.user_dlbytes - (int64_t) dllogentry->UL_FILESIZE), NukeUser.user_dlfiles - 1);
					dd_sendstring(d, nukebuf);

					NukeUser.user_dlbytes = NukeUser.user_dlbytes - dllogentry->UL_FILESIZE;

					NukeUser.user_dlfiles--;

					NukeHandle = open(ubname, O_RDWR);
					if (NukeHandle == -1) {
						dd_sendstring(d, "[31mCouldn't access userbase!!\n\n");
						return;
					}
					lseek(NukeHandle, sizeof(struct userbase) * dllogentry->UL_SLOT, SEEK_SET);
					write(NukeHandle, &NukeUser, sizeof(struct userbase));
					close(NukeHandle);

					sprintf(nukebuf, "%s/users/%d/faker.dat", origdir, dllogentry->UL_SLOT);
					NukeHandle = open(nukebuf, O_RDWR | O_CREAT, 0664);
					if (NukeHandle != -1) {

						dfentry.df_Type = 1;
						strcpy(dfentry.df_FileName, CurrFile);
						dfentry.df_FileSize = dllogentry->UL_FILESIZE;
						lseek(NukeHandle, 0, SEEK_END);
						write(NukeHandle, &dfentry, sizeof(struct FakerDataFile));
						close(NukeHandle);
					}
				}
			}
			dllogentry++;
		}
	}
	close(LogHandle);
	dd_sendstring(d, "\n\n");
}

static void NukeFile(void)
{
	int NukeHandle;
	char nukebuf[512];
	int loglen;
	struct DD_UploadLog *ullogentry;
	struct userbase NukeUser;
	int creditfaktor;
	int64_t newbytes;
	struct FakerDataFile dfentry;

	char *cptr;

	dd_sendstring(d, "[0mLooking for uploader...");

	NukeHandle = open(ullog, O_RDONLY);
	if (NukeHandle == -1) {
		dd_sendstring(d, "[31m Error! Could not access log file!!\n\n");
		return;
	}
	while ((loglen = read(NukeHandle, sta->logbuffer, sizeof(struct DD_UploadLog) * 350))) {
		ullogentry = sta->logbuffer + loglen / (sizeof(struct DD_UploadLog));
		ullogentry->UL_SLOT = 65535;

		ullogentry = sta->logbuffer;

		while (ullogentry->UL_SLOT != 65535) {
			if (!(strcasecmp(ullogentry->UL_FILENAME, CurrFile))) {
				close(NukeHandle);
				NukeHandle = open(ubname, O_RDWR | O_CREAT, 0664);
				if (NukeHandle == -1) {
					dd_sendstring(d, "[31mCouldn't access userbase!!\n\n");
					return;
				}
				lseek(NukeHandle, sizeof(struct userbase) * ullogentry->UL_SLOT, SEEK_SET);
				read(NukeHandle, &NukeUser, sizeof(struct userbase));
				close(NukeHandle);

				sprintf(nukebuf, "\n\n[36mHandle   [35m:[0m %-23s     [36mOrg.  [35m:[0m %s\n", NukeUser.user_handle, NukeUser.user_organization);
				dd_sendstring(d, nukebuf);
				sprintf(nukebuf, "\n[36mUploads  [35m:[0m %8Lu kB (%5d files)   [36mFakes [35m:[0m %7d kB (%5d files)\n", NukeUser.user_ulbytes / 1024, NukeUser.user_ulfiles, NukeUser.user_fakedbytes / 1024, NukeUser.user_fakedfiles);
				dd_sendstring(d, nukebuf);
				sprintf(nukebuf, "[36mDownloads[35m:[0m %8Lu kB (%5d files)   [36mRatios[35m:[0m 1:%d (byte), 1:%d (file)\n\n", NukeUser.user_dlbytes / 1024, NukeUser.user_dlfiles, NukeUser.user_byteratio, NukeUser.user_fileratio);
				dd_sendstring(d, nukebuf);

				dd_sendstring(d, "[36mGive how many times extra credits [35m(Negative=deduct) [0m: ");
				*nukebuf = 0;
				if (!(dd_prompt(d, nukebuf, 3, 0)))
					return;
				creditfaktor = atoi(nukebuf);

				if (!creditfaktor) {
					dd_sendstring(d, "\n");
					return;
				}
				sprintf(nukebuf, "\n[36m  Uploaded bytes           [35m:[0m %12Lu   [36m  Uploaded files           [35m:[0m %5d \n", NukeUser.user_ulbytes, NukeUser.user_ulfiles);
				dd_sendstring(d, nukebuf);
				sprintf(nukebuf, "[36m   Changed bytes           [35m:[0m %12d   [36m   Changed files           [35m:[0m %5d \n", ullogentry->UL_FILESIZE * creditfaktor, creditfaktor);
				dd_sendstring(d, nukebuf);
				dd_sendstring(d, "                             [35m------------                                -----\n");
				sprintf(nukebuf, "[36m     Bytes after           [35m:[0m %12Lu   [36m     Files after           [35m:[0m %5d \n\n", NukeUser.user_ulbytes + (int64_t) ullogentry->UL_FILESIZE * creditfaktor, NukeUser.user_ulfiles + creditfaktor);
				dd_sendstring(d, nukebuf);

				if ((cptr = ExamineCfg(CfgMem, "CREDSTRING '"))) {
					CpyToLAINA(cptr, sta->credline);
				}
				if (creditfaktor > 0)
					sprintf(nukebuf, "%d * EXTRA CREDITS!!!", creditfaktor);
				if (creditfaktor > 9)
					sprintf(nukebuf, "%d * EXTRA CREDITS!!", creditfaktor);

				if (creditfaktor < 0)
					sprintf(nukebuf, "%d * CREDITS TAKEN!!!", creditfaktor);
				if (creditfaktor < -9)
					sprintf(nukebuf, "%d * CREDITS TAKEN!!", creditfaktor);

				if (creditfaktor > 0)
					copytonuke(nukebuf);
				else
					copytonuke(&nukebuf[1]);

				if (creditfaktor < 0) {
					NukeUser.user_fakedbytes = NukeUser.user_fakedbytes + ullogentry->UL_FILESIZE;
					newbytes = -(creditfaktor * ullogentry->UL_FILESIZE);
					if (newbytes > NukeUser.user_ulbytes)
						newbytes = 0;
					else
						newbytes = NukeUser.user_ulbytes + (int64_t) creditfaktor *ullogentry->UL_FILESIZE;
				} else
					newbytes = NukeUser.user_ulbytes + (int64_t) (creditfaktor * ullogentry->UL_FILESIZE);

				NukeUser.user_ulbytes = newbytes;

				if (creditfaktor < 0) {
					NukeUser.user_fakedfiles++;
					newbytes = -(creditfaktor);
					if (newbytes > NukeUser.user_ulfiles)
						newbytes = 0;
					else
						newbytes = NukeUser.user_ulfiles + creditfaktor;
				} else
					newbytes = NukeUser.user_ulfiles + creditfaktor;

				NukeUser.user_ulfiles = newbytes;

				NukeHandle = open(ubname, O_RDWR | O_CREAT, 0664);
				if (NukeHandle == -1) {
					dd_sendstring(d, "[31mCouldn't access userbase!!\n\n");
					return;
				}
				lseek(NukeHandle, sizeof(struct userbase) * ullogentry->UL_SLOT, SEEK_SET);
				write(NukeHandle, &NukeUser, sizeof(struct userbase));
				close(NukeHandle);

				sprintf(nukebuf, "%s/users/%d/faker.dat", origdir, ullogentry->UL_SLOT);
				NukeHandle = open(nukebuf, O_RDWR | O_CREAT, 0664);
				if (NukeHandle != -1) {
					dfentry.df_Type = 0;
					strcpy(dfentry.df_FileName, CurrFile);
					dfentry.df_FileSize = ullogentry->UL_FILESIZE;
					dfentry.df_CreditFactor = creditfaktor;
					lseek(NukeHandle, 0, SEEK_END);
					write(NukeHandle, &dfentry, sizeof(struct FakerDataFile));
					close(NukeHandle);
				}
				updatedir = 1;
				return;
			}
			ullogentry++;
		}
	}
	close(NukeHandle);
}

static void copytonuke(char *src)
{
	char *dest;

	dest = &sta->credline[12];

	while (*src)
		*dest++ = *src++;
}

static void FreeFile(void)
{
	FILE *FreeHandle;
	char freebuf[256];

	sprintf(freebuf, "%s/data/freedownloads.dat", thisconf->CONF_PATH);

	if ((FreeHandle = fopen(freebuf, "a"))) {
		sprintf(freebuf, "%s\n", CurrFile);
		fputs(freebuf, FreeHandle);
		fclose(FreeHandle);
	}
	if (thisconf->CONF_ATTRIBUTES & (1 << 3)) {
		sta->descbuffer[36] = 'F';
	} else {
		sta->descbuffer[14] = 'F';
	}

/*      strcpy(sta->freeline,Pointers->dp_DayDream->CFG_FREEDLLINE); */
	updatedir = 1;
}

static void Delete(int tapa)
{
	int DelHandle;
	char delbuf[256];
	char *cptr;

	sprintf(delbuf, "%s%s", CurrPath, CurrFile);

	DelHandle = open(delbuf, O_RDWR | O_CREAT | O_TRUNC, 0664);
	if (DelHandle != -1) {
		write(DelHandle, "NUKED!\n", 7);
		close(DelHandle);

		if (tapa)
			unlink(delbuf);

		if (thisconf->CONF_ATTRIBUTES & (1 << 3)) {
			sta->descbuffer[36] = 'D';
		} else {
			sta->descbuffer[14] = 'D';
		}

		if ((cptr = ExamineCfg(CfgMem, "DELLINE '"))) {
			CpyToLAINA(cptr, sta->delline);
		}
		updatedir = 1;
	}
}

static void SaveDir(char *dirikka)
{
	int dirhandle;
	char fndbuf[40];
	struct stat st;
	char str[80];
	if (!updatedir)
		return;

	dd_getstrval(d, str, USER_HANDLE);

	if (SysOpAccess > dd_getintval(d, USER_SECURITYLEVEL)) {
		adddata(thisconf->CONF_NUMBER, CurrFile, str);
	}
	if (stat(dirikka, &st) == -1)
		return;

	DirSize = st.st_size;

	if (!(DirMem = malloc(DirSize + 2))) {
		dd_sendstring(d, "\n[35mOut of memory!\n\n");
		return;
	}
	dirhandle = open(dirikka, O_RDWR | O_CREAT, 0664);
	if (dirhandle != -1) {

		read(dirhandle, &DirMem[1], DirSize);
		DirMem[0] = 10;

		sprintf(fndbuf, "\n%s ", CurrFile);
		if ((fptr = strstr(DirMem, fndbuf))) {
			fptr++;
			feptr = GetFe(fptr);

			lseek(dirhandle, (fptr - 1) - DirMem, SEEK_SET);
			write(dirhandle, sta->descbuffer, strlen(sta->descbuffer));
			write(dirhandle, "\n", 1);
			if (sta->comline1[0]) {
				sprintf(BUFFER2, "                                   %s", sta->comline1);
				write(dirhandle, BUFFER2, strlen(BUFFER2));
			}
			if (sta->comline2[0]) {
				sprintf(BUFFER2, "                                   %s", sta->comline2);
				write(dirhandle, BUFFER2, strlen(BUFFER2));
			}
			if (sta->comline3[0]) {
				sprintf(BUFFER2, "                                   %s", sta->comline3);
				write(dirhandle, BUFFER2, strlen(BUFFER2));
			}
			if (sta->credline[0]) {
				sprintf(BUFFER2, "                                   %s\n", sta->credline);
				write(dirhandle, BUFFER2, strlen(BUFFER2));
			}
			if (sta->delline[0]) {
				sprintf(BUFFER2, "                                   %s\n", sta->delline);
				write(dirhandle, BUFFER2, strlen(BUFFER2));
			}
			if (sta->freeline[0]) {
				sprintf(BUFFER2, "                                   %s\n", sta->freeline);
				write(dirhandle, BUFFER2, strlen(BUFFER2));
			}
			write(dirhandle, feptr + 1, strlen(feptr + 1));
		}
		close(dirhandle);
	}
	free(DirMem);
}

static void CommentFile(void)
{
	char combuff[200];
	char *cptr;
	int i = 0;

	dd_sendstring(d, "[36mPlease enter your comment now. You have three lines.\nMacros [35m: [0mA[35m)[36mncient [0mB[35m)[36mroken [0mC[35m)[36mrap [0mF[35m)[36make [0mL[35m)[36mame [0mO[35m)[36mld [0mR[35m)[36menamed\n[0m");

	cptr = combuff;

	for (i = 0; i != 34; i++)
		*cptr++ = ' ';
	*cptr++ = '[';
	for (i = 0; i != 35 - userlen; i++)
		*cptr++ = '-';
	*cptr++ = ']';
	*cptr++ = 10;
	*cptr++ = 0;

	dd_sendstring(d, combuff);

	dd_sendstring(d, "                                  [0m:[35m");
	*combuff = 0;
	dd_prompt(d, combuff, 35 - userlen, 0);
	if (combuff[0] == 0)
		return;

	if (strlen(combuff) == 1) {
		switch (*combuff) {
		case 'a':
		case 'A':
			GenComline("Ancient!", sta->comline1);
			return;
		case 'b':
		case 'B':
			GenComline("Broken!", sta->comline1);
			return;
		case 'c':
		case 'C':
			GenComline("Crap!", sta->comline1);
			return;
		case 'f':
		case 'F':
			GenComline("Fake!", sta->comline1);
			return;
		case 'l':
		case 'L':
			GenComline("Lame!", sta->comline1);
			return;
		case 'o':
		case 'O':
			GenComline("Old!", sta->comline1);
			return;
		case 'r':
		case 'R':
			GenComline("Rename!", sta->comline1);
			return;
		}
	}
	GenComline(combuff, sta->comline1);
	dd_sendstring(d, "                                  [0m:[35m");
	*combuff = 0;
	dd_prompt(d, combuff, 35 - userlen, 0);
	if (combuff[0] == 0)
		return;
	GenComline(combuff, sta->comline2);
	dd_sendstring(d, "                                  [0m:[35m");
	*combuff = 0;
	dd_prompt(d, combuff, 35 - userlen, 0);
	if (combuff[0] == 0)
		return;
	GenComline(combuff, sta->comline3);

}

static void GenComline(const char *source, char *dest)
{
	int i = 0;
	int j = 0;

	if ((i = strlen(source))) {
		updatedir = 1;

		strcpy(dest, "``                                        ''\n");
		j = 23 - (3 + (userlen + i) / 2);
		i = 0;
		while (username[i]) {
			dest[j] = username[i];
			j++;
			i++;
		}
		i = 0;
		dest[j] = ' ';
		j++;
		dest[j] = ':';
		j++;
		dest[j] = ' ';
		j++;

		while (source[i]) {
			dest[j] = source[i];
			j++;
			i++;
		}
		if (ExamineCfg(CfgMem, "UPPERCASE"))
			strupr(dest);
	} else
		dest[0] = 0;
}

static char *GetFe(char *entry)
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

	while (src[i] != 39) {
		dest[i] = src[i];
		i++;
	}
	dest[i] = 0;
}

static void closeMe(void)
{
	if (sta)
		free(sta);
	if (CfgMem)
		free(CfgMem);
	dd_close(d);
}

static void strupr(char *str)
{
	while (*str) {
		*str = toupper(*str);
		str++;
	}
}

static char *ExamineCfg(char *hay, char *need)
{
	char *s;
	for (;;) {
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
		for (;;) {
			if (*s++ == *hay++) {
				if (*s == 0)
					return hay;
			} else {
				break;
			}
		}
	}
}
