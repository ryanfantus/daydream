#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <daydream.h>
#include <ddcommon.h>
#include <symtab.h>

static int getfidounique(void);
static int str2addr(char *, unsigned short *, unsigned short *, 
		    unsigned short *, unsigned short *);

int entermsg(struct DayDream_Message *msg, int reply, char *params)
{
	char ebuf[1024];
	int hola;
	int msgfd;
	char *s;
	struct DayDream_Message header;
	char *lineedmem;

	int recoff;

	if (toupper(current_msgbase->MSGBASE_FN_FLAGS) == 'E' && ((access1 & (1L << SECB_FIDOMESSAGE)) == 0)) {
		DDPut(sd[emnofidomsgstr]);
		return 0;
	}
	if (toupper(current_msgbase->MSGBASE_FN_FLAGS) == 'N' && ((access2 & (1L << SECB_SENDNETMAIL)) == 0)) {
		DDPut(sd[emnonetmsgstr]);
		return 0;
	}
	changenodestatus("Entering a message");
	if (msg) {
		memcpy(&header, msg, sizeof(struct DayDream_Message));
	} else {
		s = (char *) &header;
		memset(&header, 0, sizeof(struct DayDream_Message));
		if (params)
			strncpy(header.MSG_RECEIVER, params, 25);
	}

	if (current_msgbase->MSGBASE_FLAGS & (1L << 0) && current_msgbase->MSGBASE_FLAGS & (1L << 1)) {
		DDPut(sd[emnomsgsstr]);
		return 0;
	}
	DDPut(sd[emhead1str]);
	DDPut(current_msgbase->MSGBASE_NAME);
	ebuf[0] = 0;
	strlcat(ebuf, sd[emhead2str], sizeof ebuf);
	hola = 61 - strlen(current_msgbase->MSGBASE_NAME);
	while (hola) {
		strlcat(ebuf, "-", sizeof ebuf);
		hola--;
	}
	DDPut(ebuf);
	ddprintf(sd[emhead3str], highest);
	DDPut(sd[emhead4str]);

	if ((current_msgbase->MSGBASE_FLAGS & (1L << 0)) || (header.MSG_FLAGS & (1L << 0))) {
		DDPut(sd[emprvstr]);
		header.MSG_FLAGS |= (1L << 0);
	} else {
		DDPut(sd[empubstr]);
	}
	header.MSG_CREATION = time(0);
	DDPut(sd[emhead5str]);
	DDPut(ctime(&header.MSG_CREATION));

	DDPut(sd[emhead6str]);
	if (current_msgbase->MSGBASE_FLAGS & (1L << 2)) {
		strlcpy(header.MSG_AUTHOR, user.user_handle, sizeof header.MSG_AUTHOR);
	} else {
		strlcpy(header.MSG_AUTHOR, user.user_realname, sizeof header.MSG_AUTHOR);
	}
	DDPut(header.MSG_AUTHOR);
	for (;;) {
	      askrec:
		DDPut(sd[emhead7str]);
		if (!(Prompt(header.MSG_RECEIVER, 25, 0)))
			return 0;
		if (header.MSG_RECEIVER[0] == 0 || (!strcasecmp(header.MSG_RECEIVER, "all")) || (!strcasecmp(header.MSG_RECEIVER, "all users"))) {
			if (current_msgbase->MSGBASE_FLAGS & (1L << 0)) {
				DDPut(sd[emhead8str]);
				HotKey(0);
				header.MSG_RECEIVER[0] = 0;
			} else {
				DDPut(sd[emhead9str]);
				header.MSG_RECEIVER[0] = 0;
				break;
			}
		} else if (!strcasecmp(header.MSG_RECEIVER, "eall")) {
			if (current_msgbase->MSGBASE_FLAGS & (1L << 0)) {
				DDPut(sd[emhead8str]);
				HotKey(0);
				header.MSG_RECEIVER[0] = 0;
			} else if (access1 & (1L << SECB_EALLMESSAGE)) {
				header.MSG_RECEIVER[0] = -1;
				DDPut(sd[emhead10str]);
				break;
			} else {
				DDPut(sd[emnopoststr]);
				HotKey(0);
				header.MSG_RECEIVER[0] = 0;
			}
		} else {
			if (toupper(current_msgbase->MSGBASE_FN_FLAGS) == 'L') {
				struct userbase user;
				
				if (!strcasecmp(header.MSG_RECEIVER, "sysop")) {
					recoff = 0;
				} else {
					recoff = findusername(header.MSG_RECEIVER);
				}
				if (recoff == -1 ||
					getubentbyid(recoff, &user) == -1) {
					DDPut(sd[emnouserstr]);
					HotKey(0);
					goto askrec;
				}
				
				if (!checkconfaccess(conference()->conf.CONF_NUMBER, &user)) {
					DDPut(sd[emnoaccessstr]);
					HotKey(0);
				}
				DDPut("[5;12H                     [5;12H");
				if (current_msgbase->MSGBASE_FLAGS & (1L << 2)) {
					strlcpy(header.MSG_RECEIVER, 
						user.user_handle,
						sizeof header.MSG_RECEIVER);
				} else {
					strlcpy(header.MSG_RECEIVER, 
						user.user_realname,
						sizeof header.MSG_RECEIVER);
				}
				DDPut(header.MSG_RECEIVER);
				break;
			} else
				break;
		}
	}
	DDPut(sd[emsubjectstr]);
	if (!(Prompt(header.MSG_SUBJECT, 67, 0)))
		return 0;
	if (header.MSG_SUBJECT[0] == 0) {
		DDPut(sd[emabortedstr]);
		return 0;
	}
	DDPut("[11;1H                                                                       [11;1H");
	if (header.MSG_RECEIVER[0] == 0 || header.MSG_RECEIVER[0] == -1 || header.MSG_FLAGS & (1L << 0) || current_msgbase->MSGBASE_FLAGS & (1L << 1)) {

	} else {
		DDPut(sd[emisprivatestr]);
		hola = HotKey(HOT_NOYES);
		if (hola == 0)
			return 0;
		if (hola == 1)
			header.MSG_FLAGS |= MSG_FLAGS_PRIVATE;
	}

	if ((header.MSG_FLAGS & (1L << 0)) == 0 && (access1 & (1L << SECB_PUBLICMESSAGE)) == 0) {
		DDPut(sd[emnopubstr]);
		return 0;
	}
	if (toupper(current_msgbase->MSGBASE_FN_FLAGS) == 'N') {
		if (header.MSG_FN_DEST_NET) {
			snprintf(ebuf, sizeof ebuf, "%d:%d/%d.%d", 
				header.MSG_FN_DEST_ZONE,
				header.MSG_FN_DEST_NET, 
				header.MSG_FN_DEST_NODE,
				header.MSG_FN_DEST_POINT);
		} else {
			*ebuf = 0;
		}
		DDPut(sd[emnetaddstr]);
		if (!(Prompt(ebuf, 30, 0)))
			return 0;
		if (!str2addr(ebuf, &header.MSG_FN_DEST_ZONE, &header.MSG_FN_DEST_NET,
		    &header.MSG_FN_DEST_NODE, &header.MSG_FN_DEST_POINT))
			return 0;
		if(access2 & (1L << SECB_CRASH)) {
			DDPut(sd[emnetcrashstr]);
			if(HotKey(HOT_NOYES) == 1) {
				header.MSG_FLAGS |= MSG_FLAGS_CRASH;
			}
		}
		DDPut(sd[emnetkillstr]);
		if(HotKey(HOT_YESNO) == 1) {
			header.MSG_FLAGS |= MSG_FLAGS_KILL_SENT;
		}
	}
	*header.MSG_ATTACH = 0;

	if (current_msgbase->MSGBASE_FLAGS & (1L << 5)) {
		if ((header.MSG_FLAGS & (1L << 0)) && (access2 & (1L << SECB_PVTATTACH))) {
			*header.MSG_ATTACH = 1;
		} else if (((header.MSG_FLAGS & (1L << 0)) == 0) && (access2 & (1L << SECB_PUBATTACH))) {
			*header.MSG_ATTACH = 1;
		}
	}
	if (reply) {
		if (!askqlines()) {
			snprintf(ebuf, sizeof ebuf, "%s/daydream%d.msg", 
				DDTMP, node);
			unlink(ebuf);
		}
		DDPut("\n\n");
	}
	/* XXX: size should be replaced by a constant! */
	lineedmem = (char *) xmalloc(80 * 500);
	hola = edfile(lineedmem, 80 * 500, reply, &header);
	if (hola == 0) {
		char fabuf[1024];

		DDPut(sd[emaborted2str]);
		free(lineedmem);
		if (cleantemp() == -1) {
			DDPut(sd[tempcleanerrstr]);
			return 0;
		}
		snprintf(fabuf, sizeof fabuf, "%s/attachs.%d", DDTMP, node);
		unlink(fabuf);

		return 0;
	}
	DDPut(sd[emsavingstr]);

	getmsgptrs();
	highest++;
	header.MSG_NUMBER = highest;
	if (setmsgptrs() == 0) {
		free(lineedmem);
		return 0;
	}
	if (*header.MSG_ATTACH) {
		char fabuf[1024];
		FILE *fd;

		snprintf(fabuf, sizeof fabuf, "%s/attachs.%d", DDTMP, node);
		if ((fd = fopen(fabuf, "r"))) {
			char hoobab[1024];

			snprintf(hoobab, sizeof hoobab, "%s/messages/base%3.3d/fa%5.5d", conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, header.MSG_NUMBER);
			mkdir(hoobab, 0777);
			setperm(hoobab, 0777);

			while (fgetsnolf(hoobab, 1024, fd)) {
				char sr[1024];
				char de[1024];
				snprintf(sr, sizeof sr, "%s/%s", currnode->MULTI_TEMPORARY, hoobab);
				snprintf(de, sizeof de, "%s/messages/base%3.3d/fa%5.5d/%s", conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, header.MSG_NUMBER, hoobab);
				newrename(sr, de);
			}
			fclose(fd);
			snprintf(hoobab, sizeof hoobab, "%s/messages/base%3.3d/msf%5.5d", conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, header.MSG_NUMBER);
			newrename(fabuf, hoobab);
		} else {
			*header.MSG_ATTACH = 0;
		}
	}

	if (toupper(current_msgbase->MSGBASE_FN_FLAGS) != 'L') {
		header.MSG_FN_ORIG_ZONE = current_msgbase->MSGBASE_FN_ZONE;
		header.MSG_FN_ORIG_NET = current_msgbase->MSGBASE_FN_NET;
		header.MSG_FN_ORIG_NODE = current_msgbase->MSGBASE_FN_NODE;
		header.MSG_FN_ORIG_POINT = current_msgbase->MSGBASE_FN_POINT;
		header.MSG_FLAGS |= (1L << 2);
	}
	if ((msgfd = ddmsg_open_base(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, O_RDWR | O_CREAT, 0666)) == -1) {
		DDPut(sd[emwriteerrstr]);
		free(lineedmem);
		return 0;
	}
	fsetperm(msgfd, 0666);
	lseek(msgfd, 0, SEEK_END);
	safe_write(msgfd, &header, sizeof(struct DayDream_Message));
	ddmsg_close_base(msgfd);

	if ((msgfd = ddmsg_open_msg(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, header.MSG_NUMBER, O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1) {
		DDPut(sd[emwriteerrstr]);
		free(lineedmem);
		return 0;
	}
	fsetperm(msgfd, 0666);
	if (toupper(current_msgbase->MSGBASE_FN_FLAGS) == 'E') {
		char ub[128];
		int uq;

		strlcpy(ub, current_msgbase->MSGBASE_FN_TAG, sizeof ub);
		strupr(ub);
		snprintf(ebuf, sizeof ebuf, "AREA:%s\n", ub);
		safe_write(msgfd, ebuf, strlen(ebuf));
		if ((uq = getfidounique())) {
			snprintf(ebuf, sizeof ebuf, "\001MSGID: %d:%d/%d.%d %8.8x\n", current_msgbase->MSGBASE_FN_ZONE, current_msgbase->MSGBASE_FN_NET, current_msgbase->MSGBASE_FN_NODE, current_msgbase->MSGBASE_FN_POINT, uq);
			safe_write(msgfd, ebuf, strlen(ebuf));
			if (header.MSG_ORIGINAL) {
				if (getreplyid(header.MSG_ORIGINAL, ebuf, sizeof ebuf))
					safe_write(msgfd, ebuf, strlen(ebuf));
			}
		}
	} else if (toupper(current_msgbase->MSGBASE_FN_FLAGS) == 'N') {
		snprintf(ebuf, sizeof ebuf, "\001INTL %d:%d/%d %d:%d/%d\n",
			 header.MSG_FN_DEST_ZONE, header.MSG_FN_DEST_NET,
			 header.MSG_FN_DEST_NODE, header.MSG_FN_ORIG_ZONE,
			 header.MSG_FN_ORIG_NET, header.MSG_FN_ORIG_NODE);
		safe_write(msgfd, ebuf, strlen(ebuf));

		if (header.MSG_FN_DEST_POINT) {
			snprintf(ebuf, sizeof ebuf, "\001TOPT %d\n", header.MSG_FN_DEST_POINT);
			safe_write(msgfd, ebuf, strlen(ebuf));
		}
		if (header.MSG_FN_ORIG_POINT) {
			snprintf(ebuf, sizeof ebuf, "\001FMPT %d\n", header.MSG_FN_ORIG_POINT);
			safe_write(msgfd, ebuf, strlen(ebuf));
		}
	}
	s = lineedmem;
	while (hola) {
		snprintf(ebuf, sizeof ebuf, "%s\n", s);
		safe_write(msgfd, ebuf, strlen(ebuf));
		hola--;
		s = &s[80];
	}

//	place holder, user auto-sig goes here
//	snprintf(ebuf, sizeof ebuf, "signature here");
//	safe_write(msgfd, ebuf, strlen(ebuf));

	if (toupper(current_msgbase->MSGBASE_FN_FLAGS) == 'E') {
		snprintf(ebuf, sizeof ebuf, "\n--- DayDream BBS/UNIX (" UNAME ") %s\n * Origin: %s (%d:%d/%d)\nSEEN-BY: %d/%d\n", versionstring, current_msgbase->MSGBASE_FN_ORIGIN, current_msgbase->MSGBASE_FN_ZONE, current_msgbase->MSGBASE_FN_NET, current_msgbase->MSGBASE_FN_NODE, current_msgbase->MSGBASE_FN_NET, current_msgbase->MSGBASE_FN_NODE);
		safe_write(msgfd, ebuf, strlen(ebuf));
	} else if (toupper(current_msgbase->MSGBASE_FN_FLAGS) != 'L') {
		snprintf(ebuf, sizeof ebuf, "\n--- DayDream BBS/UNIX (" UNAME ") %s\n", versionstring);
		safe_write(msgfd, ebuf, strlen(ebuf));
	}
	ddmsg_close_msg(msgfd);

	DDPut(sd[emdonestr]);
	free(lineedmem);

	if (header.MSG_FLAGS & (1L << 0)) {
		user.user_pvtmessages++;
		clog.cl_pvtmessages++;
	} else {
		user.user_pubmessages++;
		clog.cl_pubmessages++;
	}
	return 1;
}

int fsed(char *buffer, size_t buflen, int mode, struct DayDream_Message *header)
{
	char buf[200];
	FILE *myf;
	int rc = 0;
	char goo[1024];
	int head;

	if (!mode) {
		snprintf(buf, sizeof buf, "%s/daydream%d.msg", DDTMP, node);
		unlink(buf);
	}
	if (header) {
		snprintf(goo, sizeof buf, "%s/msgheader.%d", DDTMP, node);
		head = open(goo, O_WRONLY | O_TRUNC | O_CREAT, 0666);
		if (head != -1) {
			fsetperm(head, 0666);
			safe_write(head, header, sizeof(struct DayDream_Message));
			close(head);
		}
	}
	rundoor(maincfg.CFG_FSEDCOMMAND, 0);
	snprintf(buf, sizeof buf, "%s/fsed%d.txt", DDTMP, node);

	if ((myf = fopen(buf, "r"))) {
		/* FIXME: rethink */
		while (fgetsnolf(buf, 79, myf)) {
			strlcpy(buffer + rc * 80, buf, buflen - rc * 80);
			rc++;
		}
		fclose(myf);
		snprintf(buf, sizeof buf, "%s/fsed%d.txt", DDTMP, node);
		unlink(buf);
		return rc;
	}
	return 0;
}

int comment(void)
{
	int oldb;

	if (!conference()->conf.CONF_COMMENTAREA)
		return 0;

	oldb = current_msgbase->MSGBASE_NUMBER;

	if (changemsgbase(conference()->conf.CONF_COMMENTAREA, MC_QUICK | MC_NOSTAT)) {
		entermsg(0, 0, maincfg.CFG_SYSOPNAME);
		changemsgbase(oldb, MC_QUICK | MC_NOSTAT);
		return 1;
	}
	return 0;
}


void getmsgptrs(void)
{
	struct DayDream_MsgPointers ptrs;

	ddmsg_getptrs(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, &ptrs);

	highest = ptrs.msp_high;
	lowest = ptrs.msp_low;

}

int setmsgptrs(void)
{
	int msgfd;
	struct DayDream_MsgPointers ptrs;

	ptrs.msp_high = highest;
	ptrs.msp_low = lowest;

	if(ddmsg_setptrs(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, &ptrs) < 0) {
		DDPut(sd[setptrserrstr]);
		return 0;
	}

	return 1;
}


int edfile(char *lineedmem, size_t memsize, int reply, struct DayDream_Message *header)
{
	int edtype = 0;
	int hola;

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
	memset(lineedmem, 0, memsize);

	if (edtype)
		return fsed(lineedmem, memsize, reply, header);
	else 
		return lineed(lineedmem, memsize, reply, header);
}

static int getfidounique(void)
{
	return ddmsg_getfidounique(origdir);
}

static int str2addr(char *s, unsigned short *zone, unsigned short *net, 
		    unsigned short *node, unsigned short *point)
{
	char cb[1024];
	char *t;
	*zone = *net = *node = *point = 0;

	if (*s == '-' || *s == 10 || *s == 13 || *s < '0' || *s > '9') {
		return 0;
	}
	t = cb;
	while (*s != ':' && *s)
		*t++ = *s++;
	*t = 0;
	*zone = atoi(cb);
	if (!*s)
		return 0;
	else
		s++;

	t = cb;
	while (*s != '/' && *s)
		*t++ = *s++;
	*t = 0;
	*net = atoi(cb);
	if (!*s)
		return 0;
	else
		s++;

	t = cb;
	while (*s != '.' && *s && *s != 13 && *s != 10)
		*t++ = *s++;
	*t = 0;
	*node = atoi(cb);
	if (!*s || *s == 13 || *s == 10)
		return 1;
	else
		s++;

	t = cb;
	while (*s && *s != 10 && *s != 13)
		*t++ = *s++;
	*t = 0;
	*point = atoi(cb);
	return 1;

}
