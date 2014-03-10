#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <daydream.h>
#include <ddcommon.h>
#include <utility.h>

/* FIXME! what do all these things store in case the next caller dials? */
static int show_klugdes = 0;
static int seekpoint = 0;
static int oldseekpoint;
static char rbuffer[500];
static int msgbasesize;
static struct DayDream_Message *msgbuf;
static int bread;
static int msgnum;
static struct DayDream_Message *daheader = 0;

static int editfile (FILE *);
static int editmsg(int);
static int editmsghdr(int);
static int getfilesize(char *);
static int showmsg(int, int);
static int deletemsg(int);
static int replymsg(int);
static void readmsgdatas(void);
static int keepmsg(int);
static void listmsgs(void);

int readmessages(int cseekp, int premsg, char *mask)
{
	int dir;
	int tnum;
	int oldmsg;
	int rval = 0;
	int i;
	oldseekpoint = -1;
	
	if (!conference()->conf.CONF_MSGBASES) 
		return 0;
	    
	changenodestatus("Reading messages");

	daheader = 0;

	getmsgptrs();

	snprintf(rbuffer, sizeof rbuffer, "%s/messages/base%3.3d/msgbase.dat", 
		conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER);

	msgbasesize = getfilesize(rbuffer);
	if (!msgbasesize) {
		if (!premsg)
			DDPut(sd[rmnomsgsstr]);
		return 0;
	}
	msgbuf = (struct DayDream_Message *) xmalloc(sizeof(struct DayDream_Message) * 101);

	if (cseekp != -1)
		seekpoint = cseekp;
	else
		seekpoint = msgbasesize - (100 * sizeof(struct DayDream_Message));
	if (seekpoint < 0)
		seekpoint = 0;

	readmsgdatas();
	dir = 1;

	if (premsg > 0)
		msgnum = premsg;
	else
		msgnum = lrp;

	for (;;) {
		char dirc;

		if (dir == 1)
			dirc = '+';
		else
			dirc = '-';

		if (premsg > 0)
			showmsg(msgnum, 1);

		if (premsg == -1) {
			rbuffer[0] = 0;
			premsg = 0;
		} else {
			ddprintf(sd[rmpromptstr], msgnum, highest, dirc);
			rbuffer[0] = 0;
			if (!(Prompt(rbuffer, 5, PROMPT_NOCRLF)))
				return 0;
			premsg = 0;
		}
		if (rbuffer[0] == 0) {
			if (cseekp != -1) {
				lsp = msgnum;
				free(msgbuf);
				return 0;
			}

			do {
				int saved = msgnum;
				
				while (msgnum >= lowest && msgnum <= highest) {
					msgnum += dir;
					if (!mask) 
						break;
					if (mask[msgnum - lowest])
						break;
				}

				if (msgnum < lowest) {
					msgnum = saved;
					break;
				}

				if (msgnum > highest)
					goto pois;
				
			} while (showmsg(msgnum, 0) != 1);
		} else if (!strcasecmp(rbuffer, "a")) {
			showmsg(msgnum, 0);
		} else if (!strcasecmp(rbuffer, "d")) {
			deletemsg(msgnum);
		} else if (!strcasecmp(rbuffer, "e")) {
			editmsg(msgnum);
		} else if (!strcasecmp(rbuffer, "eh")) {
			editmsghdr(msgnum);
		} else if (!strcasecmp(rbuffer, "k")) {
			keepmsg(msgnum);
		} else if ((!strcasecmp(rbuffer, "r")) || (!strcasecmp(rbuffer, "re"))) {
			replymsg(msgnum);
		} else if (!strcasecmp(rbuffer, "c")) {
			DDPut("\n");
			TypeFile("msgreadcommands", TYPE_MAKE | TYPE_WARN);
		} else if (!strcasecmp(rbuffer, "q")) {
			rval = 2;
			break;
		} else if (!strcasecmp(rbuffer, "-")) {
			dir = -1;
		} else if (!strcasecmp(rbuffer, "+")) {
			dir = 1;
		} else if (!strcasecmp(rbuffer, "]")) {
			changemsgbase(current_msgbase->MSGBASE_NUMBER + 1, 
					MC_QUICK | MC_NOSTAT);
			msgnum = lrp;
			readmsgdatas();
		} else if (!strcasecmp(rbuffer, "[")) {
			changemsgbase(current_msgbase->MSGBASE_NUMBER - 1,
					MC_QUICK | MC_NOSTAT);
			msgnum = lrp;
			readmsgdatas();
		} else if(!strcasecmp(rbuffer, "!")) {
			show_klugdes = show_klugdes ? 0 : 1;
			showmsg(msgnum, 0);
		} else if(!strcasecmp(rbuffer, "l")) {
			listmsgs();
		} else {		       
			tnum = atoi(rbuffer);
			if (tnum) {
				if (tnum > highest || tnum < lowest) {
					DDPut(sd[rmoutstr]);

				} else {
					struct DayDream_Message *oldhead;
					if (mask && !mask[tnum - lowest])
						continue;
					if (cseekp != -1) {
						if (tnum > lsp)
							lsp = tnum;
						free(msgbuf);
						return 0;
					}
					oldmsg = msgnum;
					oldhead = daheader;
					msgnum = tnum;
					switch (showmsg(msgnum, 0)) {
					case 0:
						DDPut(sd[rmoutstr]);
						msgnum = oldmsg;
						daheader = oldhead;
						break;
					case 1:
						break;
					case 2:
						DDPut(sd[rmdeletestr]);
						msgnum = oldmsg;
						daheader = oldhead;
						break;
					case 3:
						DDPut(sd[rmprivatestr]);
						msgnum = oldmsg;
						daheader = oldhead;
						break;

					}
				}
			}
		}
	}
      pois:
	DDPut("\n\n");
	free(msgbuf);
	return rval;
}

static void updatemsgdatas(void)
{
	int msgdatfd;

	msgdatfd = ddmsg_open_base(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, O_WRONLY, 0);
	lseek(msgdatfd, oldseekpoint, SEEK_SET);
	safe_write(msgdatfd, msgbuf, bread);
	ddmsg_close_base(msgdatfd);
}

static int showmsg(int showme, int mode)
{
	int hinkmode = 0;
	int msghandle = 0;
	int lowmode = 0;
	const char *msgstatus;
	int sublen;
	FILE *msgfd;
	int screenl;
	int l;
	char from[48];
	char to[48];
	char msgstr[32];

	daheader = msgbuf;

	if (!showme)
		return 0;
	for (;;) {
		if (daheader->MSG_NUMBER == showme)
			break;
		if (daheader->MSG_NUMBER == 65535 && daheader->MSG_NEXTREPLY == 65535) {
			if ((lowest > showme) || (highest < showme)) {
				return 0;
			}
			if (!hinkmode) {
				seekpoint = msgbasesize - ((10 + (highest - msgnum)) * sizeof(struct DayDream_Message));
				if (seekpoint < 0)
					seekpoint = 0;
				readmsgdatas();
				hinkmode = 1;
			} else {
				msghandle = ddmsg_open_msg(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, showme, O_RDONLY, 0);
				if (msghandle == -1)
					return 2;
				ddmsg_close_msg(msghandle);
				if (showme < msgbuf->MSG_NUMBER) {
					lowmode = 1;
					if (!oldseekpoint)
						return 2;
					seekpoint = seekpoint - 100 * sizeof(struct DayDream_Message);
					if (seekpoint == oldseekpoint)
						return 2;
					if (seekpoint < 0)
						seekpoint = 0;
					readmsgdatas();
					if (!bread)
						return 2;
				} else {
					if (seekpoint == oldseekpoint)
						return 2;
					if (lowmode)
						return 2;
					readmsgdatas();
					if (!bread)
						return 2;
				}
			}
			daheader = msgbuf;
		} else
			daheader++;
	}

	if (daheader->MSG_FLAGS & MSG_FLAGS_DELETED)
		return 2;

	if (lrp < showme && (mode == 0)) {
		lrp = showme;
	}
	strcpy(msgstr, "Pub");
	if (daheader->MSG_FLAGS & MSG_FLAGS_PRIVATE) {
		strcpy(msgstr, "Pvt");

		if (!((!strcasecmp(daheader->MSG_AUTHOR, user.user_realname)) || (!strcasecmp(daheader->MSG_AUTHOR, user.user_handle)) || (!strcasecmp(daheader->MSG_RECEIVER, user.user_handle)) || (!strcasecmp(daheader->MSG_RECEIVER, user.user_realname)) || (access1 & (1L << SECB_READALL)))) {
			return 3;
		}
	}

	if(daheader->MSG_FLAGS & MSG_FLAGS_LOCAL) {
		strcat(msgstr, ",Loc");
	}
	if(daheader->MSG_FLAGS & MSG_FLAGS_KILL_SENT) {
		strcat(msgstr, ",K/S");
	}
	if(daheader->MSG_FLAGS & MSG_FLAGS_CRASH) {
		strcat(msgstr, ",Cra");
	}
	if(daheader->MSG_FLAGS & MSG_FLAGS_ATTACH) {
		strcat(msgstr, ",Att");
	}
	if(daheader->MSG_FLAGS & MSG_FLAGS_EXPORTED) {
		strcat(msgstr, ",Ex");
	}

	msghandle = ddmsg_open_msg(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, showme, O_RDONLY, 0);

	if(msghandle == -1) {
		return 2;
	}

	msgfd = fdopen(msghandle, "r");

	if (lrp > showme)
		daheader->MSG_READCOUNT++;

	if(toupper(current_msgbase->MSGBASE_FN_FLAGS) == 'N') {
		snprintf(from, sizeof from, "%s, %d:%d/%d.%d", daheader->MSG_AUTHOR, daheader->MSG_FN_ORIG_ZONE, daheader->MSG_FN_ORIG_NET, daheader->MSG_FN_ORIG_NODE, daheader->MSG_FN_ORIG_POINT);
		snprintf(to, sizeof to, "%s, %d:%d/%d.%d", daheader->MSG_RECEIVER, daheader->MSG_FN_DEST_ZONE, daheader->MSG_FN_DEST_NET, daheader->MSG_FN_DEST_NODE, daheader->MSG_FN_DEST_POINT);
	} else {
		strncpy(from, daheader->MSG_AUTHOR, sizeof from);
		strncpy(to, daheader->MSG_RECEIVER, sizeof to);
	}

	ddprintf(sd[rmhead1str], from, msgstr);

	if (daheader->MSG_RECEIVER[0] == 0) {
		msgstatus = sd[rmallstr];
	} else if (daheader->MSG_RECEIVER[0] == -1) {
		msgstatus = sd[rmeallstr];
	} else {
		//msgstatus = daheader->MSG_RECEIVER;
		msgstatus=to;
	}

	ddprintf(sd[rmhead2str], msgstatus, ctime(&daheader->MSG_CREATION));
	if (daheader->MSG_RECEIVED == 0) {
		msgstatus = "-\n";
	} else {
		msgstatus = ctime(&daheader->MSG_RECEIVED);
	}
	ddprintf(sd[rmhead3str], current_msgbase->MSGBASE_NAME, msgstatus);
	ddprintf(sd[rmhead4str], daheader->MSG_READCOUNT);
	snprintf(rbuffer, sizeof rbuffer, sd[rmhead5str], 
		daheader->MSG_SUBJECT);
	sublen = strlen(daheader->MSG_SUBJECT);
	sublen = 73 - sublen;
	while (sublen) {
		strlcat(rbuffer, "-", sizeof rbuffer);
		sublen--;
	}
	strlcat(rbuffer, "\n\n[0m", sizeof rbuffer);
	DDPut(rbuffer);

	screenl = user.user_screenlength - 8;

	l = 0;
	while (fgets(rbuffer, 490, msgfd)) {
		char *s;
		int ih = 0;

		s = rbuffer;

		if(toupper(current_msgbase->MSGBASE_FN_FLAGS) != 'L' && !show_klugdes) {
			if(*rbuffer == 1) {
				continue;
			}
			else if(!strncmp("AREA:", rbuffer, 5)) {
				continue;
			}
			else if (!strncmp("SEEN-BY:", rbuffer, 8) && !show_klugdes)
				break;
		} 
		else {
			if(*rbuffer == 1) {
				*rbuffer = '@';
			}
		}

		l++;
//		if (toupper(current_msgbase->MSGBASE_FN_FLAGS) != 'L')
//		place holder for strip pipes if daydream.cfg says so
		parsepipes(s);
		DDPut(s);
		screenl--;

		if (screenl == 1) {
			int hot;

			DDPut(sd[morepromptstr]);
			hot = HotKey(0);
			DDPut("\r                                                         \r");
			if (hot == 'N' || hot == 'n' || !checkcarrier())
				break;
			if (hot == 'C' || hot == 'c') {
				screenl = 20000000;
			} else {
				screenl = user.user_screenlength;
			}
		}
	}

	fclose(msgfd);
	ddmsg_close_msg(msghandle);

	if (*daheader->MSG_ATTACH) {
		char fabuf[1024];
		char buf2[1024];
		FILE *atlist;

		snprintf(fabuf, sizeof fabuf, "%s/messages/base%3.3d/msf%5.5d",
			conference()->conf.CONF_PATH, 
			current_msgbase->MSGBASE_NUMBER, daheader->MSG_NUMBER);
		atlist = fopen(fabuf, "r");
		if (atlist) {
			if (screenl < 5) {
				int hot;

				DDPut(sd[morepromptstr]);
				hot = HotKey(0);
				DDPut("\r                                                         \r");
				if (hot == 'N' || hot == 'n' || !checkcarrier())
					goto pom;
				if (hot == 'C' || hot == 'c') {
					screenl = 20000000;
				} else {
					screenl = user.user_screenlength;
				}
			}
			DDPut(sd[attachhdstr]);
			screenl -= 3;
			while (fgetsnolf(fabuf, 1024, atlist)) {
				int hot;
				struct stat st;

				snprintf(buf2, sizeof buf2, 
					"%s/messages/base%3.3d/fa%5.5d/%s", 
					conference()->conf.CONF_PATH, 
					current_msgbase->MSGBASE_NUMBER, 
					daheader->MSG_NUMBER, fabuf);
				if (stat(buf2, &st) == -1)
					continue;

				ddprintf(sd[attachestr], fabuf, st.st_size);
				screenl--;

				if (screenl < 2) {
					DDPut(sd[morepromptstr]);
					hot = HotKey(0);
					DDPut("\r                                                         \r");
					if (hot == 'N' || hot == 'n' || !checkcarrier())
						break;
					if (hot == 'C' || hot == 'c') {
						screenl = 20000000;
					} else {
						screenl = user.user_screenlength;
					}
				}
			}
			DDPut(sd[attachtstr]);
			if (HotKey(HOT_NOYES) == 1) {
				char olddir[1024];

				getcwd(olddir, 1024);
				snprintf(fabuf, sizeof fabuf, 
					"%s/messages/base%3.3d/fa%5.5d", 
					conference()->conf.CONF_PATH, 
					current_msgbase->MSGBASE_NUMBER, 
					daheader->MSG_NUMBER);
				chdir(fabuf);
				snprintf(fabuf, sizeof fabuf, "../msf%5.5d", 
					daheader->MSG_NUMBER);
				sendfiles(fabuf, 0, sizeof fabuf);
				chdir(olddir);
			}
		pom:
			fclose(atlist);
		}
	}
	DDPut("\n");

	return 1;
}
static void listmsgs(void) {
	int i=0;
	int screenl = user.user_screenlength - 3;

	if(!msgbuf) {
		return;
	}
	
	DDPut(sd[msllheadstr]);

	for(i=0; i < (bread/sizeof(struct DayDream_Message)); i++) {
	
		if (msgbuf[i].MSG_FLAGS & MSG_FLAGS_DELETED)
			continue;

		if (msgbuf[i].MSG_FLAGS & MSG_FLAGS_PRIVATE) {

			if (!((!strcasecmp(msgbuf[i].MSG_AUTHOR, user.user_realname)) || 
			     (!strcasecmp(msgbuf[i].MSG_AUTHOR, user.user_handle)) || 
					 (!strcasecmp(msgbuf[i].MSG_RECEIVER, user.user_handle)) || 
					 (!strcasecmp(msgbuf[i].MSG_RECEIVER, user.user_realname)) || 
					 (access1 & (1L << SECB_READALL)))) {
				continue;
			}
		}

		ddprintf(sd[msllliststr], msgbuf[i].MSG_NUMBER, msgbuf[i].MSG_AUTHOR, msgbuf[i].MSG_RECEIVER, msgbuf[i].MSG_SUBJECT);

		screenl--;

		if (screenl == 1) {
			int hot;

			DDPut(sd[morepromptstr]);
			hot = HotKey(0);
			DDPut("\r                                                         \r");
			if (hot == 'N' || hot == 'n' || !checkcarrier())
				break;
			if (hot == 'C' || hot == 'c') {
				screenl = 20000000;
			} else {
				screenl = user.user_screenlength;
			}
		}
	}

}

static int deletemsg(int delme)
{
	if (!daheader) {
		DDPut(sd[rmdeletewhatstr]);
		return 0;
	}
	if (daheader->MSG_FLAGS & (1L << 1)) {
		DDPut(sd[rmdeletealrstr]);
		return 0;
	}
	if ((access1 & (1L << SECB_DELETEANY)) || (!strcasecmp(user.user_handle, daheader->MSG_AUTHOR)) || (!strcasecmp(user.user_realname, daheader->MSG_AUTHOR)) || (!strcasecmp(user.user_handle, daheader->MSG_RECEIVER)) || (!strcasecmp(user.user_realname, daheader->MSG_RECEIVER))) {

		daheader->MSG_FLAGS |= (1L << 1);
		ddmsg_delete_msg(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, daheader->MSG_NUMBER);
		updatemsgdatas();
		DDPut(sd[rmdeletedstr]);
	} else {
		DDPut(sd[rmdelnostr]);
	}
	return 0;
}

static int editmsg(int editme)
{
	int fd;
	FILE* fh;
	if (!daheader || (daheader->MSG_FLAGS & (1L << 1)))
		return 0;

	if ((!strcasecmp(user.user_handle, daheader->MSG_AUTHOR)) || (!strcasecmp(user.user_realname, daheader->MSG_AUTHOR)) || user.user_securitylevel == 255) {

		fd = ddmsg_open_msg(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, daheader->MSG_NUMBER, O_RDWR, 0664);

		if(fd == -1) {
			return 0;
		}

		fh = fdopen(fd, "r+");

		DDPut("\r                                                                 \r");
		editfile(fh);

		fclose(fh);
		ddmsg_close_msg(fd);
	}
	return 1;
}

static int editmsghdr(int editme)
{
	if (!daheader || (daheader->MSG_FLAGS & (1L << 1)))
		return 0;

	if ((!strcasecmp(user.user_handle, daheader->MSG_AUTHOR)) || (!strcasecmp(user.user_realname, daheader->MSG_AUTHOR)) || user.user_securitylevel == 255) {
		char foobuf[1024];
		DDPut("\n\n");
		if (user.user_securitylevel == 255) {
			DDPut(sd[edhfstr]);
			if (!(Prompt(daheader->MSG_AUTHOR, 25, 0)))
				return 0;
		}
		DDPut(sd[edhtstr]);
		if (*daheader->MSG_RECEIVER == -1) {
			strlcpy(foobuf, "EAll", sizeof foobuf);
		} else if (*daheader->MSG_RECEIVER == 0) {
			strlcpy(foobuf, "All", sizeof foobuf);
		} else {
			strlcpy(foobuf, daheader->MSG_RECEIVER, sizeof foobuf);
		}
		if (!(Prompt(foobuf, 25, 0)))
			return 0;
		if (!strcasecmp(foobuf, "eall")) {
			if (!(access1 & (1L << SECB_EALLMESSAGE))) {
				*daheader->MSG_RECEIVER = 0;
			} else {
				*daheader->MSG_RECEIVER = -1;
			}
		} else if (!strcasecmp(foobuf, "all")) {
			*daheader->MSG_RECEIVER = 0;
		} else {
			strlcpy(daheader->MSG_RECEIVER, foobuf, sizeof daheader->MSG_RECEIVER);
		}
		DDPut(sd[edhsstr]);
		if (!(Prompt(daheader->MSG_SUBJECT, 67, 0)))
			return 0;
		if (((current_msgbase->MSGBASE_FLAGS & (1L << 0)) == 0) && ((current_msgbase->MSGBASE_FLAGS & (1L << 1)) == 0)) {
			DDPut(sd[edhpstr]);
			if (HotKey(HOT_NOYES) == 2) {
				daheader->MSG_FLAGS &= ~(1L << 0);
			} else {
				daheader->MSG_FLAGS |= (1L << 0);
			}
		}
		updatemsgdatas();
	}
	return 1;
}

static int editfile(FILE* fh_msg)
{
	char *s;
	int edtype = 0;
	int hola;
	char *lineedmem;
	int res;
	int rep = 0;
	FILE *fh_tmp;
	char mbuf[1024];

	edtype = 0;
	if (user.user_toggles & (1L << 11)) {
		DDPut(sd[emfsedstr]);
		hola = HotKey(HOT_YESNO);
		if (hola == 0)
			return 0;
		if (hola == 1)
			edtype = 1;
	} else if (user.user_toggles & (1L << 0)) {
		edtype = 1;
	}
	/* XXX: argh! constants! */
	lineedmem = (char *) xmalloc(40000);

	if (fh_msg) {
		rep = 1;
		snprintf(mbuf, sizeof mbuf, "%s/daydream%d.msg", DDTMP, node);
		fh_tmp = fopen(mbuf, "w");
		while (fgets(mbuf, 80, fh_msg))
			fputs(mbuf, fh_tmp);
		fclose(fh_tmp);
	}
	memset(lineedmem, 0, 40000);

	if (edtype) 
		res = fsed(lineedmem, 40000, rep, 0);
	else 
		res = lineed(lineedmem, 40000, rep, 0);

	if (res) {
		if (fh_msg) {
			rewind(fh_msg);
			s = lineedmem;
			while (res--) {
				fprintf(fh_msg, "%s\n", s);
				s = &s[80];
			}
		}
	}
	free(lineedmem);
	return 1;
}

static int replymsg(int delme)
{
	updatemsgdatas();

	if (!daheader) {
		DDPut(sd[rmreplynostr]);
		return 0;
	}
	replymessage(daheader);
	if ((daheader->MSG_FLAGS & (1L << 0)) &&
	    ((!strcasecmp(daheader->MSG_RECEIVER, user.user_handle)) ||
	     (!strcasecmp(daheader->MSG_RECEIVER, user.user_realname)))) {
		DDPut(sd[delorigstr]);
		if (HotKey(HOT_YESNO) == 1) {
			deletemsg(daheader->MSG_NUMBER);
		}
	}
	seekpoint = oldseekpoint;
	readmsgdatas();

	return 1;
}

static void readmsgdatas(void)
{
	int msgdatfd;
	struct DayDream_Message *kalamsg;
	int moffset;

	msgdatfd = ddmsg_open_base(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, O_RDONLY, 0);

	lseek(msgdatfd, seekpoint, SEEK_SET);
	bread = read(msgdatfd, msgbuf, 100 * sizeof(struct DayDream_Message));
	moffset = bread / sizeof(struct DayDream_Message);
	kalamsg = msgbuf + moffset;
	kalamsg->MSG_NUMBER = 65535;
	kalamsg->MSG_NEXTREPLY = 65535;
	oldseekpoint = seekpoint;
	seekpoint = seekpoint + bread;

	ddmsg_close_base(msgdatfd);
}

int globalread(void)
{
	int oldconf;
	conference_t *mc;
	struct iterator *iterator;

	oldconf = user.user_joinconference;
	
	iterator = conference_iterator();
	while ((mc = (conference_t *) iterator_next(iterator))) {
		int i;
		
		if (!joinconf(mc->conf.CONF_NUMBER, JC_QUICK | JC_SHUTUP | JC_NOUPDATE))
			continue;

		for (i = 0; i < conference()->conf.CONF_MSGBASES; i++) {
			msgbase_t *mbase = conference()->msgbases[i];
			
			changemsgbase(mbase->MSGBASE_NUMBER, MC_QUICK | MC_NOSTAT);
			if (highest > lrp && isbasetagged(conference()->conf.CONF_NUMBER, current_msgbase->MSGBASE_NUMBER)) {
				if (readmessages(-1, -1, NULL) == 2) {
					/* TODO: Ask if the user will containue global read */
					break;
				}
			}
		}	       
	}
	iterator_discard(iterator);
	joinconf(oldconf, JC_QUICK | JC_SHUTUP | JC_NOUPDATE);
	return 1;
}

static int getfilesize(char *pathi)
{
	struct stat fib;
	int sizefd;

	sizefd = open(rbuffer, O_RDONLY);
	if (sizefd == -1)
		return 0;
	fstat(sizefd, &fib);
	close(sizefd);
	return fib.st_size;
}

int strlena(char *s)
{
	int sz = 0;
	char *p;

	while (*s) {
		if (s[0] == '\033' && s[1] == '[') {
			p = strpbrk(s + 2, "@AbBcCdDgGHiIJKLmMnPRSTXZ");
			if (p == NULL) {
				sz += strlen(s);
				break;
			} else 
				s = p + 1;
		} else {
			s++;
			sz++;
		}
	}

	return sz;
}

static int keepmsg(int msgnum)
{
	struct DayDream_Message msg;
	int msgfd;

	updatemsgdatas();

	if (!daheader) {
		return 0;
	}
	if ((!strcasecmp(daheader->MSG_RECEIVER, user.user_handle))
	    || (!strcasecmp(daheader->MSG_RECEIVER, user.user_realname))
	    || user.user_securitylevel >= maincfg.CFG_COSYSOPLEVEL) {

		msg = *daheader;

		msg.MSG_RECEIVED = 0;
		daheader->MSG_FLAGS |= (1L << 1);

		msg.MSG_FLAGS |= (1L << 2);
		getmsgptrs();
		highest++;
		msg.MSG_NUMBER = highest;
		setmsgptrs();

		ddmsg_rename_msg(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, daheader->MSG_NUMBER, msg.MSG_NUMBER);

		msgfd = ddmsg_open_base(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, O_RDWR|O_CREAT, 0666);

		if(msgfd == -1) {
			return 0;
		}

		fsetperm(msgfd, 0666);
		lseek(msgfd, 0, SEEK_END);
		safe_write(msgfd, &msg, sizeof(struct DayDream_Message));
		
		ddmsg_close_base(msgfd);

		seekpoint = oldseekpoint;
		readmsgdatas();
		return 1;
	}
	return 0;
}
