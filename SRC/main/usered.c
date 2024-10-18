#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <daydream.h>
#include <ddcommon.h>
#include <symtab.h>

int current_id = 0;

static int usereditor(struct userbase *, int);
void search_by_number(int);
static void uemove(int, int);
static void setseclevel(struct userbase *);

static void confstostr(int, char *);
static int seceditor(struct DD_Seclevel *);
static int strtoconfs(char *);
int uedask(struct userbase *);

struct flagstr {
	const char *fl_desc;
	int fl_num;
	int fl_field;
	int fl_bit;
	int fl_x;
	int fl_y;
};

static struct flagstr fl[] = {
	{"Download", 7, 1, 0, 22, 7},
	{"Upload", 8, 1, 1, 48, 7},
	{"Read Message", 9, 1, 2, 74, 7},
	{"Enter Message", 10, 1, 3, 22, 8},
	{"Page SysOp", 11, 1, 4, 48, 8},
	{"Comment", 12, 1, 5, 74, 8},
	{"Bulletins", 13, 1, 6, 22, 9},
	{"List Files", 14, 1, 7, 48, 9},
	{"New Files", 15, 1, 8, 74, 9},
	{"Zippy Search", 16, 1, 9, 22, 10},
	{"Run Door", 17, 1, 10, 48, 10},
	{"Join Conference", 18, 1, 11, 74, 10},
	{"Change Msg Area", 19, 1, 12, 22, 11},
	{"Change Info", 20, 1, 13, 48, 11},
	{"Relogin", 21, 1, 14, 74, 11},
	{"Tag Editor", 22, 1, 15, 22, 12},
	{"User Stats", 23, 1, 16, 48, 12},
	{"View Time", 24, 1, 17, 74, 12},
	{"--", 25, 1, 18, 22, 13},
	{"Expert Mode", 26, 1, 19, 48, 13},
	{"Eall-Message", 27, 1, 20, 74, 13},
	{"Fidomessage", 28, 1, 21, 22, 14},
	{"Public Message", 29, 1, 22, 48, 14},
	{"Privateread", 30, 1, 23, 74, 14},
	{"User Editor", 31, 1, 24, 22, 15},
	{"View Log", 32, 1, 25, 48, 15},
	{"SysOp DL", 33, 1, 26, 74, 15},
	{"Userlist", 34, 1, 27, 22, 16},
	{"Delete Any Msg", 35, 1, 28, 48, 16},
	{"Remote Shell", 36, 1, 29, 74, 16},
	{"Who is online", 37, 1, 30, 22, 17},
	{"Move file", 38, 1, 31, 48, 17},
	{"Select FConfs", 39, 2, 0, 74, 17},
	{"Select Bases", 40, 2, 1, 22, 18},
	{"Send Netmail", 41, 2, 2, 48, 18},
	{"Online Message", 42, 2, 3, 74, 18},
	{"Prv file attach", 43, 2, 4, 22, 19},
	{"Pub file attach", 44, 2, 5, 48, 19},
	{"View File", 45, 2, 6, 74, 19},
	{"Edit real name", 46, 2, 7, 22, 20},
	{"Edit handle", 47, 2, 8, 48, 20},
	{"Crash", 48, 2, 9, 22, 21},
	{0, 0, 0, 0, 0, 0},
};

int edit_new_users(void)
{
	struct userbase tmpuser;
	int account_id = 0, retcode;

	for (;; account_id++) {
		if (getubentbyid(account_id, &tmpuser) == -1)
			return;
		if ((tmpuser.user_toggles & UBENT_STAT_MASK) != UBENT_STAT_NEW)
			continue;
		retcode = uedask(&tmpuser);
		if (retcode == 2) 
			return;
		if (retcode == 0)
			continue;
		if(!usereditor(&tmpuser, account_id))
			continue;

		saveuserbase(&tmpuser);
		if (tmpuser.user_account_id == user.user_account_id)
			memcpy(&user, &tmpuser, sizeof(struct userbase));
	}
	return account_id;
}


void search_by_number(int account_id)
{
	struct userbase tmpuser;
	int retcode;

	for (;; ) {
		current_id = account_id;
		if (getubentbyid(account_id, &tmpuser) == -1)
			return;
		if ((tmpuser.user_toggles & UBENT_STAT_MASK) ==
			UBENT_STAT_DELETED)
			continue;
		if (!usereditor(&tmpuser, account_id))
			continue;
		saveuserbase(&tmpuser);
		if (tmpuser.user_account_id == user.user_account_id)
			memcpy(&user, &tmpuser, sizeof(struct userbase));
			return;
	}
}


int search_and_edit(const char *sstring)
{
	struct userbase tmpuser;
	int account_id = 0, retcode;

	for (;; account_id++) {
		if (getubentbyid(account_id, &tmpuser) == -1)
			return;
		if ((tmpuser.user_toggles & UBENT_STAT_MASK) == 
			UBENT_STAT_DELETED)
			continue;
		if (!wildcmp(tmpuser.user_realname, sstring) &&
		    !wildcmp(tmpuser.user_handle, sstring))
			continue;

		retcode = uedask(&tmpuser);
		if (retcode == 2) 
			return;
		if (!retcode)
			continue;
		if (!usereditor(&tmpuser, account_id))
			continue;

		saveuserbase(&tmpuser);
		if (tmpuser.user_account_id == user.user_account_id)
			memcpy(&user, &tmpuser, sizeof(struct userbase));
	}
	return account_id;
}

void usered(void)
{
	struct userbase euser;
	char uedbuf[300];

	int go = 1;
	int show_help = 1;
	int account_id = 0;

	while (go) {
		if(show_help) {
			DDPut("[2J[H\n[0;36mDayDream BBS On-Line User Editor\n[0m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n[32mC[0m)urrent User\n[32mS[0m)earch from UserBase\n[32mN[0m)ew accounts scan\n\e[32mL\e[0m)ist users\n\e[32mF\e[0m)ind by user number\n[32mQ[0m)uit\n");
			show_help = 0;
		}

		DDPut("\n[33mOn-Line user editor> [0m");

		uedbuf[0] = 0;
		if (!(Prompt(uedbuf, 3, 0)))
			break;

		if (!strcasecmp(uedbuf, "c")) {
			if (!onlinestat) {
				DDPut("\n[35mNo user online!\n");
				continue;
			}
			euser = user;
// FIX ME
// Right now it's just user number 0 since the sysop should be both user
// number 0, and should be the only one with usered access editing his
// own info ;)
			if (usereditor(&euser, 0)) {
				user = euser;
				getsec();
				saveuserbase(&user);
			}
                        while (current_id != account_id) {
                                account_id = current_id;
                                search_by_number(account_id);
                        }

		} else if (!strcasecmp(uedbuf, "q")) {
			go = 0;
		} else if (!strcasecmp(uedbuf, "l")) {
			sysop_userlist();
		} else if (!strcasecmp(uedbuf, "?")) {
			show_help = 1;
		} else if (!strcasecmp(uedbuf, "n")) {
			account_id = edit_new_users();
			while (current_id != account_id) {
				account_id = current_id;
				search_by_number(account_id);
			}
		} else if (!strcasecmp(uedbuf, "s")) {
			char sstring[25];

			DDPut("\n[0mEnter search string: [36m");
			sstring[0] = 0;
			if (!Prompt(sstring, 25, 0))
				continue;

			if (!sstring[0])
				continue;

			account_id = search_and_edit(sstring);
			while (current_id != account_id) {
				account_id = current_id;
				search_by_number(account_id);
			}
		} else if (!strcasecmp(uedbuf, "f")) {
			char sstring[3];
			int usernum;

			DDPut("\n\e[0mEnter user number: \e[36m");
			sstring[0] = 0;
			usernum = 0;
			if (!Prompt(sstring, 3, 0))
				continue;
			usernum = atoi(&sstring);
			ddprintf("The search number is %i\n", usernum);
			search_by_number(usernum);
			while (current_id != usernum) {
				usernum = current_id;
				search_by_number(usernum);
			}
		}
	}
}

void sysop_userlist(void)
{
	struct userbase tmpuser;
	int lcount;
	int account_id = 0;

	char userbuf[500];
	int usercnt = 0, frozcnt = 0, sysopcnt = 0, cosysopcnt = 0, noobcnt = 0;

	lcount = user.user_screenlength - 3;

	DDPut("\n\e[35mNUM NAME                  HANDLE                ORGANIZATION              SEC\n");
	DDPut("\e[34m-------------------------------------------------------------------------------\n");

	for (;; account_id++) {
		if (getubentbyid(account_id, &tmpuser) == -1)
			break;
		if ((tmpuser.user_toggles & UBENT_STAT_MASK) ==
			UBENT_STAT_DELETED)
			continue;

		snprintf(userbuf, sizeof userbuf,
			"%-3.3ld %-21.21s %-21.21s %-25.25s %3.3ld\n",
			account_id, tmpuser.user_realname, tmpuser.user_handle,
			tmpuser.user_organization, tmpuser.user_securitylevel);

		usercnt++;
		if ((tmpuser.user_toggles & (1L << 31)) &&
			((tmpuser.user_toggles & (1L << 31)) == 0)) {
			DDPut("\e[34m");
			frozcnt++;
		} else if (tmpuser.user_securitylevel == 255) {
			DDPut("\e[32m");
			sysopcnt++;
		} else if ((tmpuser.user_toggles & UBENT_STAT_MASK) == UBENT_STAT_NEW) {
			DDPut("\e[35m");
			noobcnt++;
		} else {
			DDPut("\e[0m");
		}
		DDPut(userbuf);
		lcount--;

		if (lcount == 0) {
			int hot;

			DDPut(sd[morepromptstr]);
			hot = HotKey(0);
			DDPut("\r                                                         \r");
			if (hot == 'N' || hot == 'n')
				break;
			if (hot == 'C' || hot == 'c') {
				lcount = -1;
			} else {
				lcount = user.user_screenlength;
			}
		}
	}
	ddprintf(sd[userltailstr], sysopcnt, usercnt, cosysopcnt, frozcnt);
	ddprintf("\e[32mUser still has NEW flag \e[33m: \e[0m%5.5ld", noobcnt);
}

int uedask(struct userbase *auser)
{
	unsigned char hot;
	
	ddprintf("\r                                                      \r[0m%s [32m/ [0m%s [35m([36mYes[35m/[36mno[35m/[36mquit[35m)[0m: ", auser->user_realname, auser->user_handle);

	for (;;) {
		if ((hot = HotKey(0))) {
			if (hot == 'q' || hot == 'Q') {
				DDPut("Quit!");
				return 2;
			} else if (hot == 'n' || hot == 'N') {
				DDPut("No!");
				return 0;
			} else if (hot == 'y' || hot == 'Y' || hot == 13 || hot == 10) {
				DDPut("Yes!");
				return 1;
			}
		} else
			return 2;
	}
}

void view_user_data(struct userbase *euser)
{
	static const char *acct_stat[4] = {
		"Active",
		"Deleted",
		"Frozen",
		"New"
	};
	
	DDPut("[2J[H\n[0m");
	
	ddprintf("[32mA[0m) Name         : [33m%-29.29s[32mL[0m) Security    : [33m%d\n", euser->user_realname, euser->user_securitylevel);
	ddprintf("[32mB[0m) Handle       : [33m%-29.29s[32mN[0m) Time Left   : [33m%d\n", euser->user_handle, euser->user_timeremaining);
	ddprintf("[32mC[0m) Organization : [33m%-29.29s[32mQ[0m) Calls Made  : [33m%d\n", euser->user_organization, euser->user_connections);
	ddprintf("[32mD[0m) Zip & City   : [33m%-29.29s[32mR[0m) Protocol    : [33m%s\n", euser->user_zipcity, "unused");
	ddprintf("[32mE[0m) Voice Phone  : [33m%-29.29s[32mS[0m) Auto-Join   : [33m%d\n", euser->user_voicephone, euser->user_joinconference);
	ddprintf("[32mF[0m) PassWord     : [33m%-29.29s[32mT[0m) Free Bytes  : [33m%d\n", "(encrypted)", euser->user_freedlbytes);
	ddprintf("[32mG[0m) CPU Model    : [33m%-29.29s[32mU[0m) Free Files  : [33m%d\n\n", euser->user_computermodel, euser->user_freedlfiles);
	       
	/* For some strange reason, this had to be split in order
	 * to avoid segfaults.
	 */
	ddprintf("[32mH[0m) Uploaded     : [33m%12Lu", euser->user_ulbytes); 
	ddprintf("[0m | [33m%-5u         [32mW[0m) Acc. Status : [33m", euser->user_ulfiles);
	ddprintf("%s\n", acct_stat[(euser->user_toggles >> 30) & 3]);
	
	ddprintf("[0m   Downloaded   : [33m%12Lu[0m | [33m%-5u\n", euser->user_dlbytes, euser->user_dlfiles);
	ddprintf("                                               [0mSlot number : [33m%d\n", euser->user_account_id);
	
	{
		struct tm *tm1;
		struct tm tmt;
		struct tm *tm2;
		
		tm1 = localtime(&euser->user_firstcall);
		memcpy(&tmt, tm1, sizeof(struct tm));
		tm1 = &tmt;
		tm2 = localtime(&euser->user_lastcall);
		ddprintf("[0m   Logins       : [33mFirst %d.%d.%d, Last %d.%d.%d\n", tm1->tm_mday, tm1->tm_mon + 1, tm1->tm_year + 1900, tm2->tm_mday, tm2->tm_mon + 1, tm2->tm_year + 1900);
	}
	ddprintf("[32mX[0m) Access Presets                              [32mZ[0m) Edit Security\n\n");
	ddprintf("[32mI[0m) UL Signature : [33m%s\n\n", euser->user_signature);
}

int change_account_status(struct userbase *euser)
{
	char uebuf[300];
	for (;;) {
		DDPut("[A[33mChange account status: [36mN[0m)ew, [36mA[0m)ctive, [36mD[0m)eleted, [36mF[0m)rozen or [36mQ)[0muit:  [D");
		uebuf[0] = 0;
		if (!(Prompt(uebuf, 1, 0)))
			return 0;
		if (!strcasecmp(uebuf, "q")) {
			DDPut("[A                                                                  ");
			return 1;
		} else if (!strcasecmp(uebuf, "a")) {
			euser->user_toggles &= ~(1L << 30);
			euser->user_toggles &= ~(1L << 31);
			DDPut("[A                                                                     ");
			uemove(65, 10);
			DDPut("Active  ");
			return 1;
		} else if (!strcasecmp(uebuf, "n")) {
			euser->user_toggles |= (1L << 30);
			euser->user_toggles |= (1L << 31);
			DDPut("[A                                                                     ");
			uemove(65, 10);
			DDPut("New user");
			return 1;
		} else if (!strcasecmp(uebuf, "f")) {
			euser->user_toggles &= ~(1L << 30);
			euser->user_toggles |= (1L << 31);
			DDPut("[A                                                                     ");
			uemove(65, 10);
			DDPut("Frozen  ");
			return 1;
		} else if (!strcasecmp(uebuf, "d")) {
			euser->user_toggles &= ~(1L << 31);
			euser->user_toggles |= (1L << 30);
			DDPut("[A                                                                     ");
			uemove(65, 10);
			DDPut("Deleted");
			
			/* FIXME: deletion lacks nuking of user's dir. */
			return 1;
		}
	}
}
	
int usereditor(struct userbase *euser, int account_id)
{
	struct userbase tmpuser;
	int go = 1, redraw = 1;
	char uebuf[300];

	current_id = account_id;

	while (go) {

		if (redraw) {
			redraw = 0;
			view_user_data(euser);
		}

		uebuf[0] = 0;
		ddprintf("[18H[0mUser number \e[36m%3.3ld\e[0m - \e[36m [\e[0m,\e[36m]\e[0m Change #\nSelect field to be altered ([36mSA[0mve, [36mAB[0mort or [36mVI[0mew):     [4D", account_id);

		if (!(Prompt(uebuf, 3, 0)))
			return 0;

		if (!strcasecmp(uebuf, "sa")) {
			/* get rid of deleted users */
			if (euser->user_toggles & (1L << 30) && !(euser->user_toggles & (1L << 31))) {
				/* editor protection */
				if (euser->user_account_id != user.user_account_id) {
					memset(euser->user_handle, 0, 26);
					memset(euser->user_realname, 0, 26);
				}
			}
			return 1;
		} else if (!strcasecmp(uebuf, "[")) {
			current_id--;
			return 1;
		} else if (!strcasecmp(uebuf, "]")) {
			current_id++;
			return 1;
		} else if (!strcasecmp(uebuf, "ab")) {
			return 1;
		} else if (!strcasecmp(uebuf, "vi")) {
			redraw = 1;
		} else if (!strcasecmp(uebuf, "a")) {
			uemove(19, 2);
			if (!(Prompt(euser->user_realname, 25, 0)))
				return 0;
		} else if (!strcasecmp(uebuf, "b")) {
			uemove(19, 3);
			if (!(Prompt(euser->user_handle, 25, 0)))
				return 0;
		} else if (!strcasecmp(uebuf, "c")) {
			uemove(19, 4);
			if (!(Prompt(euser->user_organization, 25, 0)))
				return 0;
		} else if (!strcasecmp(uebuf, "d")) {
			uemove(19, 5);
			if (!(Prompt(euser->user_zipcity, 20, 0)))
				return 0;
		} else if (!strcasecmp(uebuf, "e")) {
			uemove(19, 6);
			if (!(Prompt(euser->user_voicephone, 20, 0)))
				return 0;
		} else if (!strcasecmp(uebuf, "f")) {
			MD_CTX context;

			uemove(19, 7);
			DDPut("              ");
			uemove(19, 7);
			uebuf[0] = 0;
			if (!(Prompt(uebuf, 15, 0)))
				return 0;
			strupr(uebuf);
			MDInit(&context);
			MDUpdate(&context, (unsigned char *) uebuf, 
				strlen(uebuf));
			MDFinal(euser->user_password, &context);
		} else if (!strcasecmp(uebuf, "g")) {
			uemove(19, 8);
			if (!(Prompt(euser->user_computermodel, 20, 0)))
				return 0;
		} else if (!strcasecmp(uebuf, "h")) {
			uemove(19, 10);
			DDPut("            ");
			uemove(19, 10);
			snprintf(uebuf, sizeof uebuf, "%Lu", euser->user_ulbytes);
			if (!(Prompt(uebuf, 12, 0)))
				return 0;
			sscanf(uebuf, "%Lu", &euser->user_ulbytes);

			uemove(34, 10);
			DDPut("     ");
			uemove(34, 10);
			snprintf(uebuf, sizeof uebuf, "%u", euser->user_ulfiles);
			if (!(Prompt(uebuf, 5, 0)))
				return 0;
			euser->user_ulfiles = atoi(uebuf);

			uemove(19, 11);
			DDPut("            ");
			uemove(19, 11);
			snprintf(uebuf, sizeof uebuf, "%Lu", euser->user_dlbytes);
			if (!(Prompt(uebuf, 12, 0)))
				return 0;
			sscanf(uebuf, "%Lu", &euser->user_dlbytes);

			uemove(34, 11);
			DDPut("     ");
			uemove(34, 11);
			snprintf(uebuf, sizeof uebuf, "%u", euser->user_dlfiles);
			if (!(Prompt(uebuf, 5, 0)))
				return 0;
			euser->user_dlfiles = atoi(uebuf);

			redraw = 1;
		} else if (!strcasecmp(uebuf, "i")) {
			uemove(19, 16);
			if (!(Prompt(euser->user_signature, 44, 0)))
				return 0;
		} else if (!strcasecmp(uebuf, "l")) {
			uemove(65, 2);
			snprintf(uebuf, sizeof uebuf, "%d", euser->user_securitylevel);
			if (!(Prompt(uebuf, 3, 0)))
				return 0;
			euser->user_securitylevel = atoi(uebuf);
			setseclevel(euser);
			if (euser->user_account_id == user.user_account_id) {
				getsec();
			}
		} else if (!strcasecmp(uebuf, "n")) {
			time_t curr;
			uemove(65, 3);
			snprintf(uebuf, sizeof uebuf, "%d", euser->user_timeremaining);
			if (!(Prompt(uebuf, 5, 0)))
				return 0;
			euser->user_timeremaining = atoi(uebuf);
			if (euser->user_account_id == user.user_account_id) {
				timeleft = euser->user_timeremaining * 60;
				curr = time(0);
				endtime = curr + timeleft;
			}
		} else if (!strcasecmp(uebuf, "q")) {
			uemove(65, 4);
			snprintf(uebuf, sizeof uebuf, "%d", euser->user_connections);
			if (!(Prompt(uebuf, 5, 0)))
				return 0;
			euser->user_connections = atoi(uebuf);
		} else if (!strcasecmp(uebuf, "s")) {
			uemove(65, 6);
			snprintf(uebuf, sizeof uebuf, "%d", euser->user_joinconference);
			if (!(Prompt(uebuf, 3, 0)))
				return 0;
			euser->user_joinconference = atoi(uebuf);
		} else if (!strcasecmp(uebuf, "t")) {
			uemove(65, 7);
			snprintf(uebuf, sizeof uebuf, "%d", euser->user_freedlbytes);
			if (!(Prompt(uebuf, 10, 0)))
				return 0;
			euser->user_freedlbytes = atoi(uebuf);
		} else if (!strcasecmp(uebuf, "u")) {
			uemove(65, 8);
			snprintf(uebuf, sizeof uebuf, "%d", euser->user_freedlfiles);
			if (!(Prompt(uebuf, 5, 0)))
				return 0;
			euser->user_freedlfiles = atoi(uebuf);
		} else if (!strcasecmp(uebuf, "v")) {
			uemove(65, 9);
			snprintf(uebuf, sizeof uebuf, "%d", euser->user_screenlength);
			if (!(Prompt(uebuf, 3, 0)))
				return 0;
			euser->user_screenlength = atoi(uebuf);
		} else if (!strcasecmp(uebuf, "w")) {
			if (!change_account_status(euser))
				return 0;
		} else if (!strcasecmp(uebuf, "z")) {
			struct DD_Seclevel secl;
			struct DD_Seclevel *cs;
			char sbuffer[80];
			int secfd;
			int succ = 0;
			snprintf(sbuffer, sizeof sbuffer, 
				"%s/users/%d/security.dat", origdir, 
				euser->user_account_id);
			secfd = open(sbuffer, O_RDONLY);
			if (secfd != -1) {
				read(secfd, &secl, sizeof(struct DD_Seclevel));
				close(secfd);
			} else {
				cs = secs;
				while (cs->SEC_SECLEVEL) {
					if (cs->SEC_SECLEVEL == euser->user_securitylevel) {
						memcpy(&secl, cs, sizeof(struct DD_Seclevel));
						succ = 1;
						break;
					}
					cs++;
				}
				if (!succ)
					continue;
			}
			if (seceditor(&secl)) {
				secfd = open(sbuffer, O_WRONLY | O_CREAT, 0666);
				if (secfd != -1) {
					fsetperm(secfd, 0666);
					safe_write(secfd, &secl, sizeof(struct DD_Seclevel));
					close(secfd);
				}
				getsec();
			}
			redraw = 1;
		} else if (!strcasecmp(uebuf, "x")) {
			DDPut("[A");
			for (;;) {
				DDPut("[33mSelect preset: [36m1[0m-[36m255[0m, [36mL[0mist or [36mQ[0m)uit:               [14D");
				uebuf[0] = 0;
				if (!(Prompt(uebuf, 3, 0)))
					return 0;
				if (!strcasecmp(uebuf, "l")) {
					struct DayDream_AccessPreset *currpre;

					currpre = presets;
					DDPut("[2J[H\n[32mPreset ID      Description\n[0m================================================================\n");
					while (currpre->ACCESS_PRESETID != 0) {
						ddprintf("[33m   %3.3d         [36m%s\n", currpre->ACCESS_PRESETID, currpre->ACCESS_DESCRIPTION);
						currpre++;
					}
				} else if (!strcasecmp(uebuf, "q")) {
					break;
				} else {
					int newacc;

					if ((newacc = atoi(uebuf))) {
						if (setpreset(newacc, euser)) {
							setseclevel(euser);
							if (euser->user_account_id == user.user_account_id) {
								time_t curr;

								getsec();
								euser->user_timeremaining = user.user_dailytimelimit;
								timeleft = euser->user_timeremaining * 60;
								curr = time(0);
								endtime = curr + timeleft;
							}
							break;
						}
					}
				}
			}
			redraw = 1;
		}
	}
	return 0;
}

static int seceditor(struct DD_Seclevel *sech)
{
	char cbuf1[40];
	char cbuf2[40];
	char uebuf[80];
	int ret = 0;
	int i;
	struct DD_Seclevel se;
	struct flagstr *cf;

	memcpy(&se, sech, sizeof(struct DD_Seclevel));

	while (!ret) {
		int ec = 0;
		cf = fl;
		confstostr(se.SEC_CONFERENCEACC1, cbuf1);
		confstostr(se.SEC_CONFERENCEACC2, cbuf2);
		ddprintf("[2J[H\n[32m1[0m) Security Level  : [33m%-3d            [32m2[0m) Daily Time      : [33m%d\n[32m3[0m) Byte Ratio      : [33m%-3d            [32m4[0m) File Ratio      : [33m%d\n[32m5[0m) Conference Access (01-32): [33m%s\n[32m6[0m) Conference Access (33-64): [33m%s\n\n", se.SEC_SECLEVEL, se.SEC_DAILYTIME, se.SEC_BYTERATIO, se.SEC_FILERATIO, cbuf1, cbuf2);
		while (cf->fl_desc) {
			int bfield;
			if (cf->fl_field == 1) {
				bfield = se.SEC_ACCESSBITS1;
			} else
				bfield = se.SEC_ACCESSBITS2;
			ddprintf("[32m%2d[0m) %-15.15s: [33m%s", cf->fl_num, cf->fl_desc, (bfield & (1L << cf->fl_bit)) ? "Yes  " : "No   ");
			ec++;
			if (ec == 3) {
				DDPut("\n");
				ec = 0;
			}
			cf++;
		}

		for (;;) {
			DDPut("[22;1H[0mSelect field to be altered ([36mSA[0mve, [36mAB[0mort or [36mVI[0mew):     [4D");
			*uebuf = 0;
			if (!(Prompt(uebuf, 3, 0)))
				return 0;

			if (!strcasecmp(uebuf, "sa")) {
				ret = 1;
				break;
			} else if (!strcasecmp(uebuf, "ab")) {
				ret = 2;
				break;
			} else if (!strcasecmp(uebuf, "vi")) {
				break;
			} else if ((i = atoi(uebuf))) {
				switch (i) {
				case 1:
					uemove(22, 2);
					snprintf(uebuf, sizeof uebuf, "%d", se.SEC_SECLEVEL);
					if (!(Prompt(uebuf, 3, 0)))
						return 0;
					se.SEC_SECLEVEL = atoi(uebuf);
					break;

				case 2:
					uemove(58, 2);
					snprintf(uebuf, sizeof uebuf, "%u", se.SEC_DAILYTIME);
					if (!(Prompt(uebuf, 5, 0)))
						return 0;
					se.SEC_DAILYTIME = atoi(uebuf);
					break;

				case 3:
					uemove(22, 3);
					snprintf(uebuf, sizeof uebuf, "%u", se.SEC_BYTERATIO);
					if (!(Prompt(uebuf, 3, 0)))
						return 0;
					se.SEC_BYTERATIO = atoi(uebuf);
					break;

				case 4:
					uemove(58, 3);
					snprintf(uebuf, sizeof uebuf, "%u", se.SEC_FILERATIO);
					if (!(Prompt(uebuf, 3, 0)))
						return 0;
					se.SEC_FILERATIO = atoi(uebuf);
					break;

				case 5:
					uemove(31, 4);
					if (!(Prompt(cbuf1, 32, 0)))
						return 0;
					se.SEC_CONFERENCEACC1 = strtoconfs(cbuf1);
					break;

				case 6:
					uemove(31, 5);
					if (!(Prompt(cbuf2, 32, 0)))
						return 0;
					se.SEC_CONFERENCEACC2 = strtoconfs(cbuf2);
					break;

				}
				if (i > 6 && i < 48) {
					cf = fl;
					while (cf->fl_num != i)
						cf++;
					if (cf->fl_field == 1) {
						if (se.SEC_ACCESSBITS1 & (1L << cf->fl_bit)) {
							se.SEC_ACCESSBITS1 &= ~(1L << cf->fl_bit);
							uemove(cf->fl_x, cf->fl_y);
							DDPut("No ");
						} else {
							se.SEC_ACCESSBITS1 |= (1L << cf->fl_bit);
							uemove(cf->fl_x, cf->fl_y);
							DDPut("Yes");
						}
					} else {
						if (se.SEC_ACCESSBITS2 & (1L << cf->fl_bit)) {
							se.SEC_ACCESSBITS2 &= ~(1L << cf->fl_bit);
							uemove(cf->fl_x, cf->fl_y);
							DDPut("No ");
						} else {
							se.SEC_ACCESSBITS2 |= (1L << cf->fl_bit);
							uemove(cf->fl_x, cf->fl_y);
							DDPut("Yes");
						}
					}
				}
			}
		}
	}
	if (ret == 1) {
		memcpy(sech, &se, sizeof(struct DD_Seclevel));
		return 1;
	} else {
		return 0;
	}
}

static void confstostr(int confss, char *str)
{
	int i;
	for (i = 0; i != 32; i++) {
		if (confss & (1L << i))
			*str++ = 'X';
		else
			*str++ = '_';
	}
	*str = 0;
}

static int strtoconfs(char *str)
{
	int i;
	int confd = 0;
	for (i = 0; i != 32; i++) {
		if (*str++ == 'X')
			confd |= (1L << i);
	}
	return confd;
}

static void setseclevel(struct userbase *suser)
{
	char sbuffer[50];

	snprintf(sbuffer, sizeof sbuffer, "%s/users/%d/security.dat", 
		origdir, suser->user_account_id);
	unlink(sbuffer);
}

static void uemove(int x, int y)
{
	ddprintf("[33m[%d;%dH", y, x);
}
