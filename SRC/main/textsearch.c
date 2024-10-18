/* FIXME! reaudit this file */

#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <daydream.h>

#define TS_INCORRECT_PARAMS	0
#define TS_STRING_PROMPT	1
#define TS_CANCEL		2
#define TS_NO_BASES_TAGGED	3
#define TS_SCAN_STRING		4
#define TS_INDICATOR_CHARS	5
#define TS_SCAN_OK		6
#define TS_RM_HEADER1		7
#define TS_RM_HEADER2		8
#define TS_RM_HEADER3		9
#define TS_RM_HEADER4		10
#define TS_RM_HEADER5		11
#define TS_RM_ALLSTR		12
#define TS_RM_EALLSTR		13
#define TS_ATTACH_HEADER	14
#define TS_ATTACH_FOOTER	15
#define TS_ATTACH_LINE		16
#define TS_MOREPROMPT		17
#define TS_CMDPROMPT		18

#define INDICATOR_ROTATION_SPEED	32

#define ATTACH_HEADER_LENGTH	2
#define ATTACH_LINE_LENGTH	1
#define ATTACH_FOOTER_LENGTH	1

static char search_str[1024];

static const char *errstr[19] = {
	"\a\033[0;35mIncorrect params!\033[0m\n\n",
	"\033[0;36mText to search: \033[0m",
	"", /* displayed if search cancelled by hitting enter */
	"\033[0;35mNo messagebases tagged in this conference\033[0m\n\n",
	"\033[0;32mSearching \033[0;33m%.21s \033[0;35m",
	"-\\|/",
	"\033[D\033[0m\033[K \033[32mOK\n",
	"\033[2J\033[H\n\033[0;32mAuthor \033[33m : \033[0m%-26.26s  \033[32mStatus  \033[33m: \033[0m%s\n",
	"\033[32mReceiver\033[33m: \033[0m%-26.26s  \033[32mCreation\033[33m: \033[0m%s",
	"\033[32mMsgBase \033[33m: \033[0m%-26.26s  \033[32mReceived\033[33m: \033[0m%s",
	"\033[32mR. Count\033[33m: \033[0m%5.5ld\n\n",
	"\033[34m--\033[36m[ \033[0m%s \033[36m]\033[34m",
	"All users",
	"<*> ALL USERS <*>",
	"\n\033[36mFollowing files have been attached to this message:\n\033[34m==========================================================================\n",
	"\033[34m==========================================================================\n\033[0m\033[32m(\033[33mR\033[32m)\033[36meview, \033[32m(\033[33mP\033[32m)\033[36mrevious, \033[32m(\033[33mN\033[32m)\033[36mext, \033[32m(\033[33mD\033[32m)\033[36mownload attachs, \033[32m(\033[33mQ\033[32m)\033[36muit\033[32m)\033[36m: ",
	"\033[0m%s \033[32m(\033[33m%d \033[32mbytes)\n",
	"\033[0mMore: \033[35mY\033[36m)\033[0mes, \033[35mN\033[36m)\033[0mo, \033[35mC\033[36m)\033[0montinuous?:",
	"\033[32m(\033[33mR\033[32m)\033[36meview, \033[32m(\033[33mP\033[32m)\033[36mrevious, \033[32m(\033[33mN\033[32m)\033[36mext, \033[32m(\033[33mQ\033[32m)\033[36muit\033[32m)\033[36m: "
};

/* 0 = quoted ascii, 1 = like 0 but with c escape sequences */
static int pattern_type = 0;

static int read_all = 1;

/* tolower() is applied, too */
static void shrink_spaces(char *s)
{
	char *r, *p, *t = s;
	r = p = (char *) xmalloc(strlen(s) + 1);
	while (*s) {
		if (isspace(*s)) {
			*r++ = tolower(*s);
			while (isspace(*++s));
		} else
			*r++ = tolower(*s++);
	}
	*r = 0;
	strcpy(t, p);
	free(p);
}

static void quoted2plain(char **quoted, char *plain) 
{
	int quote_on = 0;
	char *p, *s = *quoted;	
	for (p = plain; *s; s++) {
		if (isspace(*s) && !quote_on)
			break;
		if ((!pattern_type || *s != '\\') && 
		    *s != '"' && *s != '\'') {
			*p++ = *s;
			continue;
		}	     				
		if ((*s == '"' || *s == '\'') &&
		    (!quote_on || quote_on == *s)) {
			if (quote_on)
				quote_on = 0;
			else 
				quote_on = *s;
			continue;
		}
		if (pattern_type && !*++s) {
			*plain = 0;
			ddprintf("%s", errstr[TS_INCORRECT_PARAMS]);
			return;
		}
		*p++ = *s;
	}
	*p = 0;
	*quoted = s;
}

static void handle_params(char *params)
{
	char *s;
	
	search_str[0] = 0;

	if (!params)
		return;
	/* skip whitespaces */
	for (s = params; isspace(*s); s++);
	if (!*s)
		return;
	quoted2plain(&s, search_str);
}
	
static void ask_params(void)
{
	if (!*search_str) {
		char buffer[512], *s = buffer;
		ddprintf("%s", errstr[TS_STRING_PROMPT]);
		buffer[0] = 0;
		if (!Prompt(buffer, 512, 0))
			return;
		
		quoted2plain(&s, search_str);
		if (!*search_str) {
			ddprintf("%s", errstr[TS_CANCEL]);
			return;
		}
	}
	
	if (pattern_type < 2)
		shrink_spaces(search_str);
}

static char *get_headers(void)
{
	char *ofs;
	int fd;
	struct DayDream_Message msg;
	char fname[PATH_MAX + 1];
	
	fd = ddmsg_open_base(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, O_RDONLY, 0);

	if (fd == -1)
		return NULL;

	ofs = (char *) calloc(1 + highest - lowest, 1);
	
	for (;;) { 
		if (read(fd, &msg, sizeof(struct DayDream_Message)) !=
		    sizeof(struct DayDream_Message)) 
			break;
		
		if (msg.MSG_NUMBER < lowest ||
		    msg.MSG_NUMBER > highest)
			continue;
		/* no deleted messages */
		if (msg.MSG_FLAGS & (1L << 1))
			continue;
		
		/* keep privacy */
		if (msg.MSG_FLAGS & (1L << 0) && !read_all &&
		    findusername(msg.MSG_RECEIVER) != user.user_account_id) 
			continue;

		ofs[msg.MSG_NUMBER - lowest] = 1;
	}
	ddmsg_close_base(fd);
	return ofs;
}	       

static int simple_search(int num)
{
	int fd, retcode;
	struct stat st;
	char fname[PATH_MAX + 1], *contents;

	fd = ddmsg_open_msg(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, num, O_RDONLY, 0);

	if(fd == -1)
		return 0;

	fstat(fd, &st);
	contents = (char *) xmalloc(st.st_size + 1);
	contents[st.st_size] = 0;
	read(fd, contents, st.st_size);

	ddmsg_close_msg(fd);

	if (pattern_type < 2)
		shrink_spaces(contents);
	retcode = strstr(contents, search_str) != NULL;
	free(contents);
	return retcode;
}

static void scan_msgbase(int num)
{
	int i, first = -1;
	char *headers;
		
	if (!changemsgbase(num, MC_QUICK | MC_NOSTAT))
		return;
	
	getmsgptrs();
	
	if (!(headers = get_headers()))
		return;

	for (i = lowest; i <= highest; i++) {
		if (!headers[i - lowest]) 
			continue;
		if (!simple_search(i))
			headers[i - lowest] = 0;
		else if (first == -1)
			first = i;
	}

	if (first == -1)
		return;
	
	readmessages(-1, first, headers);
		       
	free(headers);
}
	
static void do_conference_wide_scan(void)
{
	int i, cm;
		
	if (!isanybasestagged(conference()->conf.CONF_NUMBER)) {
		ddprintf("%s", errstr[TS_NO_BASES_TAGGED]);
		return;
	}
	
	cm = current_msgbase->MSGBASE_NUMBER;
		
	for (i = 1; i <= conference()->conf.CONF_MSGBASES; i++) {
		if (!isbasetagged(conference()->conf.CONF_NUMBER, i))
			continue;
		scan_msgbase(i);
	}
	
	changemsgbase(cm, MC_QUICK | MC_NOSTAT);
}

int textsearch(char *params)
{
	handle_params(params);
	ask_params();
	if (!*search_str)
		return 1;

	do_conference_wide_scan();
	return 1;
}

