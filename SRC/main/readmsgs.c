#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include <daydream.h>
#include <ddcommon.h>
#include <utility.h>
#include <ddlib.h>

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
static void word_wrap(const char *, char *, int, int *);
static bool contains_ansi_codes(const char *);
static char *read_entire_message(FILE *msgfd, long *total_size);
static char **split_into_lines(const char *buffer, int *line_count);
static char **process_flowed_format(char **lines, int line_count, int *out_line_count);
static char *wrap_lines(char **lines, int line_count, int wrap_length);
static void free_line_array(char **lines, int line_count);
static void display_scrollable_message(char **display_lines, int total_lines, int screenl);
static char **prepare_display_lines(const char *wrapped_message, int *total_lines);
static void draw_lightbar(int selected, int top_line, int max_lines, int total_lines);

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

	// New scrollable message display with arrow key navigation
	{
		long msg_size;
		char *message_buffer;
		char **lines;
		int line_count;
		char *wrapped_message;
		char **display_lines;
		int total_display_lines;
		
		// Step 1: Read entire message into dynamically allocated buffer
		message_buffer = read_entire_message(msgfd, &msg_size);
		if (!message_buffer) {
			DDPut("Error reading message.\n");
			goto cleanup_msg;
		}
		
		// Step 2: Split buffer into array of lines
		lines = split_into_lines(message_buffer, &line_count);
		if (!lines) {
			free(message_buffer);
			DDPut("Error processing message.\n");
			goto cleanup_msg;
		}
		
		// Step 2.5: Process format=flowed if applicable
		char **processed_lines;
		int processed_line_count;
		int needs_free_processed = 0;
		
		processed_lines = process_flowed_format(lines, line_count, &processed_line_count);
		if (processed_lines != lines) {
			// Flowed format was processed, we got new line array
			needs_free_processed = 1;
		}
		
		// Step 3: Wrap the lines
		wrapped_message = wrap_lines(processed_lines, processed_line_count, 80);
		if (!wrapped_message) {
			free_line_array(lines, line_count);
			free(message_buffer);
			DDPut("Error wrapping message.\n");
			goto cleanup_msg;
		}
		
        // Step 4: Prepare display lines from wrapped message
        display_lines = prepare_display_lines(wrapped_message, &total_display_lines);
        if (!display_lines) {
            free(wrapped_message);
            free_line_array(lines, line_count);
            free(message_buffer);
            DDPut("Error preparing display.\n");
            goto cleanup_msg;
        }
		
		// Step 5: Display with scrollable interface
		display_scrollable_message(display_lines, total_display_lines, screenl);
		
		// Cleanup
		free_line_array(display_lines, total_display_lines);
		free(wrapped_message);
		if (needs_free_processed) {
			free_line_array(processed_lines, processed_line_count);
		}
		free_line_array(lines, line_count);
		free(message_buffer);
		
		cleanup_msg:;
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

static void word_wrap(const char *input, char *output, int wrap_length, int *screenl) {
    int len = 0;
    const char *word_start = input;
    char *out_ptr = output;

    while (*input) {
        if (*input == ' ' || *input == '\n' || *(input + 1) == '\0') {
            int word_len = input - word_start + (*(input + 1) == '\0' ? 1 : 0);

            if (len + word_len > wrap_length) {
                *out_ptr++ = '\n';
                len = 0;
                (*screenl)--;
            } else if (len > 0) {
                *out_ptr++ = ' ';
                len++;
            }

            strncpy(out_ptr, word_start, word_len);
            out_ptr += word_len;
            len += word_len;
            word_start = input + 1;
        }

        input++;

        if (*screenl == 1) {
            DDPut(sd[morepromptstr]);
            int hot = HotKey(0);
            DDPut("\r                                                         \r");
            if (hot == 'N' || hot == 'n' || !checkcarrier())
                break;
            if (hot == 'C' || hot == 'c') {
                *screenl = 20000000; // Infinite lines
            } else {
                *screenl = user.user_screenlength;
            }
        }
    }

    *out_ptr = '\0'; // Null-terminate the output
}

static bool contains_ansi_codes(const char *s) {
    while (*s) {
        if (*s == '\033' && *(s + 1) == '[') {
            const char *p = s + 2;

            // Validate the format of ANSI escape sequences
            while (*p && ((*p >= '0' && *p <= '9') || *p == ';')) {
                p++; // Skip numeric parameters and semicolons
            }

            if (*p && ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z'))) {
                return true; // Found a valid ANSI escape sequence
            }
        }
        s++;
    }
    return false;
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

	sizefd = open(pathi, O_RDONLY);
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

// Step 1: Read entire message into dynamically allocated buffer
static char *read_entire_message(FILE *msgfd, long *total_size) {
    long start_pos = ftell(msgfd);
    char *buffer = NULL;
    long size = 0;
    long capacity = 1024;
    char line_buffer[500];
    
    buffer = (char *) xmalloc(capacity);
    buffer[0] = '\0';
    
    while (fgets(line_buffer, sizeof(line_buffer), msgfd)) {
        long line_len = strlen(line_buffer);
        
        // Check if we need to expand buffer
        if (size + line_len + 1 > capacity) {
            capacity *= 2;
            buffer = (char *) realloc(buffer, capacity);
            if (!buffer) {
                *total_size = 0;
                return NULL;
            }
        }
        
        strcat(buffer, line_buffer);
        size += line_len;
    }
    
    *total_size = size;
    return buffer;
}

// Step 2: Split buffer into array of lines
static char **split_into_lines(const char *buffer, int *line_count) {
    int capacity = 100;
    int count = 0;
    char **lines = (char **) xmalloc(capacity * sizeof(char *));
    const char *start = buffer;
    const char *current = buffer;
    
    while (*current) {
        if (*current == '\n') {
            int line_len = current - start;
            
            // Expand array if needed
            if (count >= capacity) {
                capacity *= 2;
                lines = (char **) realloc(lines, capacity * sizeof(char *));
            }
            
            // Allocate and copy line (include empty lines to preserve blank lines)
            lines[count] = (char *) xmalloc(line_len + 1);
            strncpy(lines[count], start, line_len);
            lines[count][line_len] = '\0';
            
            count++;
            start = current + 1;
        }
        current++;
    }
    
    // Handle case where buffer doesn't end with newline
    if (start < current) {
        if (count >= capacity) {
            capacity++;
            lines = (char **) realloc(lines, capacity * sizeof(char *));
        }
        int line_len = current - start;
        lines[count] = (char *) xmalloc(line_len + 1);
        strncpy(lines[count], start, line_len);
        lines[count][line_len] = '\0';
        count++;
    }
    
    *line_count = count;
    return lines;
}

// Helper: Check if a string contains only formatting codes and whitespace (no visible text)
static int is_only_formatting(const char *str) {
    if (!str || !*str) return 1; // Empty string
    
    while (*str) {
        // Skip whitespace
        if (*str == ' ' || *str == '\t' || *str == '\r') {
            str++;
            continue;
        }
        
        // Skip pipe codes (|XX where XX are digits)
        if (*str == '|' && isdigit(*(str+1)) && isdigit(*(str+2))) {
            str += 3;
            continue;
        }
        
        // Skip ANSI escape sequences (\e[...m)
        if (*str == '\033' && *(str+1) == '[') {
            str += 2;
            while (*str && !((*str >= 'A' && *str <= 'Z') || (*str >= 'a' && *str <= 'z'))) {
                str++;
            }
            if (*str) str++; // Skip the final letter
            continue;
        }
        
        // Found a visible character
        return 0;
    }
    
    return 1; // Only formatting codes found
}

// Step 2.5: Process format=flowed messages (RFC 3676)
static char **process_flowed_format(char **lines, int line_count, int *out_line_count) {
    // First, check if this message uses format=flowed
    int is_flowed = 0;
    for (int i = 0; i < line_count && i < 20; i++) {
        // Check for FORMAT: flowed or FORMAT:flowed (with SOH prefix)
        char *line = lines[i];
        if (*line == 1) line++; // Skip SOH character
        if (!strncmp("FORMAT: flowed", line, 14) || 
            !strncmp("FORMAT:flowed", line, 13)) {
            is_flowed = 1;
            break;
        }
    }
    
    if (!is_flowed) {
        // Not a flowed message, return original lines
        *out_line_count = line_count;
        return lines;
    }
    
    // Process flowed format
    int capacity = line_count;
    char **result = (char **) xmalloc(capacity * sizeof(char *));
    int result_count = 0;
    
    int i = 0;
    while (i < line_count) {
        char *line = lines[i];
        int line_len = strlen(line);
        
        // Skip kludge lines (they're not part of the body)
        if (*line == 1 || !strncmp("AREA:", line, 5) || 
            !strncmp("SEEN-BY:", line, 8) || !strncmp("PATH:", line, 5)) {
            i++;
            continue;
        }
        
        // Count quote depth (number of '>' characters at start)
        int quote_depth = 0;
        int pos = 0;
        while (pos < line_len && (line[pos] == '>' || line[pos] == ' ')) {
            if (line[pos] == '>') quote_depth++;
            pos++;
        }
        
        // Remove space-stuffing (leading space after quotes)
        const char *text_start = line;
        if (pos < line_len && pos > 0 && line[pos-1] == ' ') {
            text_start = line; // Keep as-is for now
        }
        
        // Check if line is empty
        if (line_len == 0 || (pos >= line_len)) {
            // Empty line = paragraph break, always include it
            if (result_count >= capacity) {
                capacity *= 2;
                result = (char **) realloc(result, capacity * sizeof(char *));
            }
            result[result_count] = strdup(line);
            result_count++;
            i++;
            continue;
        }
        
        // Build a paragraph by joining flowed lines
        long para_capacity = line_len + 1;
        char *paragraph = (char *) xmalloc(para_capacity);
        strcpy(paragraph, line);
        int para_len = line_len;
        i++;
        
        // Join lines that end with a space (soft break)
        while (i < line_count && para_len > 0 && paragraph[para_len-1] == ' ') {
            char *next_line = lines[i];
            int next_len = strlen(next_line);
            
            // Stop if next line is kludge
            if (*next_line == 1 || !strncmp("AREA:", next_line, 5) || 
                !strncmp("SEEN-BY:", next_line, 8) || !strncmp("PATH:", next_line, 5)) {
                break;
            }
            
            // Stop if next line is empty (paragraph break)
            if (next_len == 0) {
                break;
            }
            
            // Check quote depth of next line
            int next_quote_depth = 0;
            int next_pos = 0;
            while (next_pos < next_len && (next_line[next_pos] == '>' || next_line[next_pos] == ' ')) {
                if (next_line[next_pos] == '>') next_quote_depth++;
                next_pos++;
            }
            
            // Stop if quote depth changes
            if (next_quote_depth != quote_depth) {
                break;
            }
            
            // Remove the trailing space from current paragraph
            paragraph[para_len-1] = '\0';
            para_len--;
            
            // Append next line
            if (para_len + next_len + 2 > para_capacity) {
                para_capacity = para_len + next_len + 100;
                paragraph = (char *) realloc(paragraph, para_capacity);
            }
            
            strcat(paragraph, " ");
            strcat(paragraph, next_line);
            para_len += 1 + next_len;
            i++;
        }
        
        // Add the completed paragraph to results
        if (result_count >= capacity) {
            capacity *= 2;
            result = (char **) realloc(result, capacity * sizeof(char *));
        }
        result[result_count] = paragraph;
        result_count++;
    }
    
    // Post-process: Remove empty lines between quoted text at same quote level
    // and collapse consecutive empty lines
    int final_capacity = result_count;
    char **final_result = (char **) xmalloc(final_capacity * sizeof(char *));
    int final_count = 0;
    int prev_was_empty = 0;
    
    for (int i = 0; i < result_count; i++) {
        // Check if line is empty based on VISIBLE content (not raw length)
        // Lines with only ANSI/pipe codes should be treated as empty
        int is_empty = is_only_formatting(result[i]);
        
        // Skip this line if both current and previous were empty
        if (is_empty && prev_was_empty) {
            free(result[i]); // Free the skipped line
            continue;
        }
        
        // Special case: Skip empty lines in certain contexts
        if (is_empty && i > 0 && i < result_count - 1) {
            char *prev = result[i-1];
            char *next = result[i+1];
            int skip_this_empty = 0;
            
            // Case 1: Empty lines between quoted text with same prefix
            // Look for patterns like " Sh>>", " pa>", " >", etc.
            int prev_is_quote = (strchr(prev, '>') != NULL);
            int next_is_quote = (strchr(next, '>') != NULL);
            
            if (prev_is_quote && next_is_quote) {
                // Extract quote prefix from previous line (text before '>>')
                char prev_prefix[20] = {0};
                char *prev_gt = strstr(prev, ">>");
                if (prev_gt == NULL) prev_gt = strchr(prev, '>');
                if (prev_gt != NULL) {
                    int prefix_len = (prev_gt - prev) + 2; // Include '>>' or '>'
                    if (prefix_len > 0 && prefix_len < 20) {
                        strncpy(prev_prefix, prev, prefix_len);
                        prev_prefix[prefix_len] = '\0';
                    }
                }
                
                // Extract quote prefix from next line
                char next_prefix[20] = {0};
                char *next_gt = strstr(next, ">>");
                if (next_gt == NULL) next_gt = strchr(next, '>');
                if (next_gt != NULL) {
                    int prefix_len = (next_gt - next) + 2; // Include '>>' or '>'
                    if (prefix_len > 0 && prefix_len < 20) {
                        strncpy(next_prefix, next, prefix_len);
                        next_prefix[prefix_len] = '\0';
                    }
                }
                
                // If both lines have same quote prefix, skip the empty line
                if (strlen(prev_prefix) > 0 && strcmp(prev_prefix, next_prefix) == 0) {
                    skip_this_empty = 1;
                }
            }
            
            // Case 2: Empty lines between ASCII art / formatted text
            // Lines that start with pipe codes (|XX) or have lots of ANSI/box drawing chars
            int prev_has_pipes = (prev[0] == '|' && strlen(prev) > 2);
            int next_has_pipes = (next[0] == '|' && strlen(next) > 2);
            
            // Also check for box drawing characters (UTF-8 or CP437)
            int prev_has_boxdraw = (strstr(prev, "┌") || strstr(prev, "─") || strstr(prev, "┐") ||
                                     strstr(prev, "│") || strstr(prev, "└") || strstr(prev, "┘") ||
                                     strstr(prev, "█") || strstr(prev, "▄"));
            int next_has_boxdraw = (strstr(next, "┌") || strstr(next, "─") || strstr(next, "┐") ||
                                     strstr(next, "│") || strstr(next, "└") || strstr(next, "┘") ||
                                     strstr(next, "█") || strstr(next, "▄"));
            
            // If both lines are ASCII art, skip the empty line between them
            if ((prev_has_pipes && next_has_pipes) || 
                (prev_has_boxdraw && next_has_boxdraw)) {
                skip_this_empty = 1;
            }
            
            if (skip_this_empty) {
                free(result[i]);
                continue;
            }
        }
        
        final_result[final_count] = result[i];
        final_count++;
        prev_was_empty = is_empty;
    }
    
    free(result); // Free the array (but not the strings, they're in final_result)
    
    *out_line_count = final_count;
    return final_result;
}

// Step 3: Wrap the lines
static char *wrap_lines(char **lines, int line_count, int wrap_length) {
    long total_capacity = 1024;
    char *result = (char *) xmalloc(total_capacity);
    long result_len = 0;
    result[0] = '\0';
    
    for (int i = 0; i < line_count; i++) {
        char *line = lines[i];
        int line_len = strlen(line);
        int visible_len = dd_strlenansi(line);
        
        // DEBUG: Log line length to help diagnose
        // ddprintf("[DEBUG: Line %d, len=%d, visible=%d]\n", i, line_len, visible_len);
        
        // Skip kludge lines for FidoNet messages
        if (*line == 1 || !strncmp("AREA:", line, 5) || 
            (!strncmp("SEEN-BY:", line, 8) && !show_klugdes)) {
            continue;
        }
        
        // Handle @ character replacement
        if (*line == 1) {
            *line = '@';
        }
        
        // For lines that don't need wrapping, just copy and add ONE newline
        if (visible_len <= wrap_length) {
            // Ensure we have enough space
            if (result_len + line_len + 2 > total_capacity) {
                total_capacity *= 2;
                result = (char *) realloc(result, total_capacity);
            }
            
            // Copy the line
            strcat(result, line);
            result_len += line_len;
            
            // Add exactly ONE newline
            strcat(result, "\n");
            result_len++;
            
            continue;
        }
        
        // For lines that need wrapping, process them based on VISIBLE characters
        int pos = 0;  // Actual character position in string
        int visible_pos = 0;  // Visible character position on screen
        
        while (pos < line_len) {
            int chunk_start = pos;
            int chunk_visible = 0;
            int chunk_actual_len = 0;
            
            // Find how many actual characters we need to get wrap_length visible characters
            while (pos < line_len && chunk_visible < wrap_length) {
                // Check if we're at the start of a pipe code (|XX)
                if (line[pos] == '|' && pos + 2 < line_len && 
                    isdigit(line[pos+1]) && isdigit(line[pos+2])) {
                    // Skip pipe code (doesn't add to visible length)
                    pos += 3;
                    chunk_actual_len += 3;
                } 
                // Check if we're at the start of an ANSI escape sequence
                else if (line[pos] == '\033' && pos + 1 < line_len && line[pos+1] == '[') {
                    // Skip ANSI sequence (doesn't add to visible length)
                    pos += 2;
                    chunk_actual_len += 2;
                    while (pos < line_len && !(line[pos] >= 'A' && line[pos] <= 'Z') && 
                           !(line[pos] >= 'a' && line[pos] <= 'z')) {
                        pos++;
                        chunk_actual_len++;
                    }
                    if (pos < line_len) {
                        pos++;
                        chunk_actual_len++;
                    }
                } else {
                    // Regular character - adds to visible length
                    pos++;
                    chunk_actual_len++;
                    chunk_visible++;
                }
            }
            
            // Find last space for word boundary if we're not at end of line
            if (pos < line_len && chunk_visible == wrap_length) {
                int test_pos = pos - 1;
                int backtrack = 0;
                while (test_pos > chunk_start && line[test_pos] != ' ' && backtrack < 20) {
                    test_pos--;
                    backtrack++;
                }
                if (line[test_pos] == ' ' && test_pos > chunk_start) {
                    chunk_actual_len -= backtrack;
                    pos = test_pos;
                }
            }
            
            // Ensure we have enough space in result buffer
            if (result_len + chunk_actual_len + 2 > total_capacity) {
                total_capacity *= 2;
                result = (char *) realloc(result, total_capacity);
            }
            
            // Copy chunk to result
            strncat(result, line + chunk_start, chunk_actual_len);
            result_len += chunk_actual_len;
            
            // Add newline after each chunk
            strcat(result, "\n");
            result_len++;
            
            visible_pos += chunk_visible;
            
            // Skip space at beginning of next chunk if we're mid-line
            if (pos < line_len && line[pos] == ' ') {
                pos++;
            }
        }
    }
    
    return result;
}

// Step 4: Free line array
static void free_line_array(char **lines, int line_count) {
    for (int i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);
}

// Prepare display lines from wrapped message - splits into individual lines for display
static char **prepare_display_lines(const char *wrapped_message, int *total_lines) {
    int capacity = 100;
    int count = 0;
    char **lines = (char **) xmalloc(capacity * sizeof(char *));
    const char *start = wrapped_message;
    const char *current = wrapped_message;
    
    while (*current) {
        if (*current == '\n') {
            int line_len = current - start;
            
            // Expand array if needed
            if (count >= capacity) {
                capacity *= 2;
                lines = (char **) realloc(lines, capacity * sizeof(char *));
            }
            
            // Allocate and copy line
            lines[count] = (char *) xmalloc(line_len + 1);
            strncpy(lines[count], start, line_len);
            lines[count][line_len] = '\0';
            
            count++;
            start = current + 1;
        }
        current++;
    }
    
    // Handle case where buffer doesn't end with newline
    if (start < current) {
        if (count >= capacity) {
            capacity++;
            lines = (char **) realloc(lines, capacity * sizeof(char *));
        }
        int line_len = current - start;
        lines[count] = (char *) xmalloc(line_len + 1);
        strncpy(lines[count], start, line_len);
        lines[count][line_len] = '\0';
        count++;
    }
    
    *total_lines = count;
    return lines;
}

// Draw the lightbar menu at the bottom of the screen
static void draw_lightbar(int selected, int top_line, int max_lines, int total_lines) {
    const int num_options = 6;
    const char *menu_items[] = {
        "Continue",
        "Reply",
        "Re-display",
        "Forward",
        "Previous",
        "Quit"
    };
    
    DDPut("\r\e[K");  // Clear line
    for (int i = 0; i < num_options; i++) {
        if (i == selected) {
            // Highlighted option (inverse video)
            ddprintf("\e[0;7m[%s]\e[0m ", menu_items[i]);
        } else {
            // Normal option
            ddprintf("\e[0;36m[%s]\e[0m ", menu_items[i]);
        }
    }
    // Add scroll position indicator
    ddprintf("\e[0;33m(%d-%d/%d)\e[0m", 
             top_line + 1,
             (top_line + max_lines < total_lines) ? top_line + max_lines : total_lines,
             total_lines);
}

// Display message with scrollable interface using arrow keys
static void display_scrollable_message(char **display_lines, int total_lines, int screenl) {
    int top_line = 0;  // Index of the top line currently displayed
    int max_lines = screenl - 1;  // Reserve one line for status/control prompt
    int done = 0;
    int selected_option = 0;  // Currently selected lightbar option (0-5)
    const int num_options = 6;
    
    // If message fits on one screen, just display it and return
    if (total_lines <= max_lines) {
        for (int i = 0; i < total_lines; i++) {
            int visible_len = dd_strlenansi(display_lines[i]);
            if (visible_len == 0) {
                // Empty line - just output newline
                DDPut("\n");
            } else {
                DDPut(display_lines[i]);
                // Only output newline if line is NOT exactly 80 chars (terminal auto-wraps at 80)
                if (visible_len != 80) {
                    DDPut("\n");
                }
            }
        }
        return;
    }
    
    // Initial display
    for (int i = 0; i < max_lines && i < total_lines; i++) {
        int visible_len = dd_strlenansi(display_lines[i]);
        if (visible_len == 0) {
            // Empty line - just output newline
            DDPut("\n");
        } else {
            DDPut(display_lines[i]);
            // Only output newline if line is NOT exactly 80 chars (terminal auto-wraps at 80)
            if (visible_len != 80) {
                DDPut("\n");
            }
        }
    }
    
    // Show initial lightbar menu
    draw_lightbar(selected_option, top_line, max_lines, total_lines);
    
    while (!done && checkcarrier()) {
        int key = HotKey(HOT_CURSOR);
        int needs_redraw = 0;
        int needs_menu_redraw = 0;
        char inject_key[3];
        
        switch(key) {
            case 250:  // Arrow Up
                if (top_line > 0) {
                    top_line--;
                    needs_redraw = 1;
                }
                break;
                
            case 251:  // Arrow Down
                if (top_line + max_lines < total_lines) {
                    top_line++;
                    needs_redraw = 1;
                }
                break;
                
            case 252:  // Arrow Right - move to next menu item
                selected_option = (selected_option + 1) % num_options;
                needs_menu_redraw = 1;
                break;
                
            case 253:  // Arrow Left - move to previous menu item
                selected_option = (selected_option - 1 + num_options) % num_options;
                needs_menu_redraw = 1;
                break;
                
            case 13:   // Enter - activate selected menu item
            case 10:   // Line feed
                switch(selected_option) {
                    case 0:  // Continue - page down or exit if at end
                        if (top_line + max_lines < total_lines) {
                            top_line += max_lines;
                            if (top_line + max_lines > total_lines) {
                                top_line = total_lines - max_lines;
                            }
                            needs_redraw = 1;
                        } else {
                            done = 1;  // At end, exit viewer
                        }
                        break;
                    case 1:  // Reply
                        inject_key[0] = 'r';
                        inject_key[1] = '\0';
                        keyboard_stuff(inject_key);
                        done = 1;
                        break;
                    case 2:  // Re-display
                        inject_key[0] = 'a';
                        inject_key[1] = '\0';
                        keyboard_stuff(inject_key);
                        done = 1;
                        break;
                    case 3:  // Forward
                        inject_key[0] = '+';
                        inject_key[1] = '\0';
                        keyboard_stuff(inject_key);
                        done = 1;
                        break;
                    case 4:  // Previous
                        inject_key[0] = '-';
                        inject_key[1] = '\0';
                        keyboard_stuff(inject_key);
                        done = 1;
                        break;
                    case 5:  // Quit
                        done = 1;
                        break;
                }
                break;
                
            case ' ':  // Space - page down
                if (top_line + max_lines < total_lines) {
                    top_line += max_lines;
                    if (top_line + max_lines > total_lines) {
                        top_line = total_lines - max_lines;
                    }
                    needs_redraw = 1;
                }
                break;
                
            case 'b':  // 'b' for page up
            case 'B':
                if (top_line > 0) {
                    top_line -= max_lines;
                    if (top_line < 0) {
                        top_line = 0;
                    }
                    needs_redraw = 1;
                }
                break;
                
            // Legacy hotkey support (still works)
            case 'a':  // Re-display message
            case 'A':
                inject_key[0] = 'a';
                inject_key[1] = '\0';
                keyboard_stuff(inject_key);
                done = 1;
                break;
                
            case 'l':  // List messages
            case 'L':
                inject_key[0] = 'l';
                inject_key[1] = '\0';
                keyboard_stuff(inject_key);
                done = 1;
                break;
                
            case '+':  // Set forward direction
                inject_key[0] = '+';
                inject_key[1] = '\0';
                keyboard_stuff(inject_key);
                done = 1;
                break;
                
            case '-':  // Set backward direction
                inject_key[0] = '-';
                inject_key[1] = '\0';
                keyboard_stuff(inject_key);
                done = 1;
                break;
                
            case '!':  // Toggle kludges
                inject_key[0] = '!';
                inject_key[1] = '\0';
                keyboard_stuff(inject_key);
                done = 1;
                break;
                
            case 'r':  // Reply
            case 'R':
                inject_key[0] = 'r';
                inject_key[1] = '\0';
                keyboard_stuff(inject_key);
                done = 1;
                break;
                
            case 'd':  // Delete
            case 'D':
                inject_key[0] = 'd';
                inject_key[1] = '\0';
                keyboard_stuff(inject_key);
                done = 1;
                break;
                
            case 'e':  // Edit message
                inject_key[0] = 'e';
                inject_key[1] = '\0';
                keyboard_stuff(inject_key);
                done = 1;
                break;
                
            case 'k':  // Keep message
            case 'K':
                inject_key[0] = 'k';
                inject_key[1] = '\0';
                keyboard_stuff(inject_key);
                done = 1;
                break;
                
            case '[':  // Previous base
                inject_key[0] = '[';
                inject_key[1] = '\0';
                keyboard_stuff(inject_key);
                done = 1;
                break;
                
            case ']':  // Next base
                inject_key[0] = ']';
                inject_key[1] = '\0';
                keyboard_stuff(inject_key);
                done = 1;
                break;
                
            case 'n':  // Non-stop (NS) - handle two-char command
            case 'N':
                {
                    int next_key = HotKey(0);
                    if (next_key == 's' || next_key == 'S') {
                        // Pass empty string through for now (NS not fully implemented)
                        inject_key[0] = '\r';
                        inject_key[1] = '\0';
                        keyboard_stuff(inject_key);
                        done = 1;
                    }
                }
                break;
                
            case 'c':  // Show commands
            case 'C':
                inject_key[0] = 'c';
                inject_key[1] = '\0';
                keyboard_stuff(inject_key);
                done = 1;
                break;
                
            case 'q':  // Quit
            case 'Q':
            case 27:   // ESC
                done = 1;
                break;
                
            default:
                // Ignore other keys
                break;
        }
        
        if (needs_redraw) {
            // Clear the current status line first
            DDPut("\r\e[K");
            
            // Calculate how many lines to show
            int lines_to_show = max_lines;
            if (top_line + lines_to_show > total_lines) {
                lines_to_show = total_lines - top_line;
            }
            
            // Move cursor up to the start of the message content area
            // We need to go up by the number of lines displayed (including newlines)
            for (int i = 0; i < max_lines; i++) {
                DDPut("\e[A");  // Move cursor up one line
            }
            DDPut("\r");  // Move to beginning of line
            
            // Redraw all lines in the viewport
            for (int i = 0; i < lines_to_show; i++) {
                DDPut("\e[K");  // Clear current line
                int visible_len = dd_strlenansi(display_lines[top_line + i]);
                if (visible_len == 0) {
                    // Empty line - just output newline
                    DDPut("\n");
                } else {
                    DDPut(display_lines[top_line + i]);
                    // Only output newline if line is NOT exactly 80 chars (terminal auto-wraps at 80)
                    if (visible_len != 80) {
                        DDPut("\n");
                    }
                }
            }
            
            // Clear any remaining lines if we're showing fewer lines now
            for (int i = lines_to_show; i < max_lines; i++) {
                DDPut("\e[K\n");
            }
            
            // Show updated lightbar menu
            draw_lightbar(selected_option, top_line, max_lines, total_lines);
        } else if (needs_menu_redraw) {
            // Just redraw the menu without redrawing the message content
            draw_lightbar(selected_option, top_line, max_lines, total_lines);
        }
    }
    
    // Clear the menu line and add a newline
    DDPut("\r\e[K\n");
}
