#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#include <global.h>
#include <dd.h>
#include <ddlib.h>
#include <ddcommon.h>
#include <md5.h>

#include <syslog.h>

#define MD_CTX MD5_CTX
#define MDInit MD5Init
#define MDUpdate MD5Update
#define MDFinal MD5Final

struct DayDream_Conference *current_conf;
struct DayDream_MsgBase *current_base;

//static struct DayDream_MainConfig mcfg;
char* datadir;
int auth_ok;

int generate_temp_message(int messagenumber);
int count_lines_in_temp_message();
int count_bytes_in_temp_message();

static struct userbase *ddgetpwnam(char *name)
{
  int fd;
  char buf[PATH_MAX];
  static struct userbase ub;

  if (ssnprintf(buf, "%s/data/userbase.dat", datadir))
    return NULL;

  if ((fd = open(buf, O_RDONLY)) != -1) {
    while (read(fd, &ub, sizeof(struct userbase))) {
      if ((ub.user_toggles & (1L << 30))
          && ((ub.user_toggles & (1L << 31)) == 0))
        continue;
      if (!strcasecmp(ub.user_handle, name)
          || !strcasecmp(ub.user_realname, name)) {
        close(fd);
        return (&ub);
      }
    }
    close(fd);
  } else
    syslog(LOG_ERR, "cannot open userbase");
  return NULL;
}

static int cmppasswds(char *passwd, unsigned char *thepw)
{
  MD_CTX context;
  unsigned char digest[16];
  char newpw[30];
  int i;

  strcpy(newpw, passwd);
  strupr(newpw);

  MDInit(&context);
  MDUpdate(&context, (unsigned char*) newpw, strlen(newpw));
  MDFinal(digest, &context);

  for (i = 0; i < 16; i++) {
    if (thepw[i] != digest[i])
      return (0);
  }
  return (1);
}


int generate_list() {
	struct DayDream_Conference *confs;
	struct DayDream_Conference *conf;
	struct DayDream_MsgBase *base;
	struct DayDream_MsgPointers ptrs;
	int bcnt;

	confs=(struct DayDream_Conference *)dd_getconfdata();
	
	if (!confs) exit (0);
	conf=confs;

	printf("215 Newsgroups in form \"group high low flags\".\r\n");

	while(1) {
		if (conf->CONF_NUMBER==255) break;

		base = (struct DayDream_MsgBase *) conf + 1;
		bcnt=conf->CONF_MSGBASES;
		for (bcnt = conf->CONF_MSGBASES; bcnt ;bcnt--, base++) {
			if(base->MSGBASE_FN_TAG[0] == 0)
				continue;
			ddmsg_getptrs(conf->CONF_PATH, base->MSGBASE_NUMBER, &ptrs); 
			
			printf("%s.%s %10.10d %10.10d y\r\n", conf->CONF_NAME, 
				base->MSGBASE_FN_TAG, (unsigned int) ptrs.msp_high, (unsigned int) ptrs.msp_low);
		}
		
		conf = (struct DayDream_Conference *) base;
	}
	printf(".\r\n");
	
	return 0;
}

int generate_list_with_desc() {
	struct DayDream_Conference *confs;
	struct DayDream_Conference *conf;
	struct DayDream_MsgBase *base;
	int bcnt;

	confs=(struct DayDream_Conference *)dd_getconfdata();
	
	if (!confs) exit (0);
	conf=confs;

	printf("215 Descriptions in form \"group description\".\r\n");

	while(1) {
		if (conf->CONF_NUMBER==255) break;

		base = (struct DayDream_MsgBase *) conf + 1;
		bcnt=conf->CONF_MSGBASES;
		for (bcnt = conf->CONF_MSGBASES; bcnt ;bcnt--, base++) {
			if(base->MSGBASE_FN_TAG[0] == 0)
				continue;
			
			printf("%s.%s %s\r\n", conf->CONF_NAME, 
				base->MSGBASE_FN_TAG, base->MSGBASE_NAME);
		}
		
		conf = (struct DayDream_Conference *) base;
	}
	printf(".\r\n");
	
	return 0;
}


int do_group_command(char* group, int print) {
	char* conf_ptr;
	char* base_ptr;
	char tmpbuf[128];
	struct DayDream_Conference *conf;
	struct DayDream_MsgBase *base;
	int bcnt;
	struct DayDream_MsgPointers ptrs;
	int number_of_messages;

	strncpy(tmpbuf, group, 128);
	
	conf_ptr = strchr(tmpbuf, '.');
	
	if(!conf_ptr) {
		return -1;
	}
	
	*conf_ptr = '\0';
	
	base_ptr = conf_ptr + 1;
	conf_ptr = tmpbuf;
	
	conf=(struct DayDream_Conference *)dd_getconfdata();
	
	while(1) {
		if (conf->CONF_NUMBER==255) {
			return -1;
		}
		
		base = (struct DayDream_MsgBase *) conf + 1;
		
		if(!strcasecmp(conf->CONF_NAME, conf_ptr)) {
			current_conf = conf;
			break;
		}
		conf = (struct DayDream_Conference*) base;
	}	

	base = (struct DayDream_MsgBase *) conf + 1;
	for (bcnt = conf->CONF_MSGBASES; bcnt ;bcnt--, base++) {
		if(!strcasecmp(base_ptr, base->MSGBASE_FN_TAG)) {
			current_base = base;
			break;
		}
	}
	
	if(bcnt == 0) {
	    return -1;
	}
	
	if(print) {
		ddmsg_getptrs(current_conf->CONF_PATH, current_base->MSGBASE_NUMBER, &ptrs); 

		number_of_messages = ptrs.msp_high - ptrs.msp_low;
		if(ptrs.msp_high > 0) {
			number_of_messages++;
		}

		printf("211 %d %d %d %s\r\n", number_of_messages, ptrs.msp_low, ptrs.msp_high, group);
	}
	syslog(LOG_INFO, "Group %s selected", group);
	return 0;
}

char* generate_message_id(int messagenumber) {
	static char buf[128];
	
	sprintf(buf, "<%d$%s.%s@localhost>", messagenumber, current_conf->CONF_NAME, current_base->MSGBASE_FN_TAG);
	return buf;
}

unsigned char* strip_hibit(unsigned char* str) {
	unsigned char* orig_str = str;
	
	while(*str != '\0') {
		if(*str > 127) {
			*str = '?';
		}
		++str;
	}
	
	return orig_str;
}

int print_message_headers(int messagenumber) {
	int fd;
	int ok;
	int number_of_lines;
	char tmpbuf[128];
	struct DayDream_Message dd_msg;
	struct tm* t;
	
	fd = ddmsg_open_base(current_conf->CONF_PATH, current_base->MSGBASE_NUMBER, O_RDONLY, 0660);
	if(fd < 0) {
		return -1;
	}
	
	ok = 0;
	
	while(read(fd, &dd_msg, sizeof(struct DayDream_Message)) == sizeof(struct DayDream_Message)) {
		if(dd_msg.MSG_NUMBER == messagenumber) {
			ok = 1;
			break;
		}
	}
	
	ddmsg_close_base(fd);
	
	if(!ok) {
		return -1;
	}
	
	generate_temp_message(messagenumber);
	number_of_lines = count_lines_in_temp_message();
	
	t = gmtime(&dd_msg.MSG_CREATION);
	strftime(tmpbuf, 128, "%a, %d %b %Y %H:%M:%S GMT", t);
	
	strip_hibit((unsigned char*) dd_msg.MSG_AUTHOR);
	strip_hibit((unsigned char*) dd_msg.MSG_RECEIVER);
	
	printf("Path: localhost!not-for-mail\r\n");
	printf("From: %s <dummy@localhost>\r\n", dd_msg.MSG_AUTHOR);
	printf("Newsgroups: %s.%s\r\n", current_conf->CONF_NAME, current_base->MSGBASE_FN_TAG);
	printf("Message-ID: %s\r\n", generate_message_id(messagenumber));
	printf("Subject: %s\r\n", dd_msg.MSG_SUBJECT);
	printf("Date: %s\r\n", tmpbuf);
	printf("Lines: %d\r\n", number_of_lines);
	printf("X-FTN-TO: %s\r\n", dd_msg.MSG_RECEIVER);
	
	return 0;
}

int count_lines_in_temp_message() {
	char* ptr;
	FILE* fp;
	char filename[128];
	char tmpbuf[128];
	int count;

	sprintf(filename, "/tmp/msg.%d", getpid());
	fp = fopen(filename, "r");
	
	count = 0;
	
	while(fgets(tmpbuf, 128, fp) != NULL) {
		ptr = tmpbuf;
		while(*ptr) {
			if(*ptr == '\n') {
				++count;
			}
			++ptr;
		}
	}
	
	fclose(fp);
	
	return count;
}

int count_bytes_in_temp_message() {
	char filename[128];
	struct stat s;
	
	sprintf(filename, "/tmp/msg.%d", getpid());
	stat(filename, &s);
	return s.st_size;
}

int generate_temp_message(int messagenumber) {
	int i;
	int j;
	int fd;
	char tmpbuf[128];
	FILE* fp;
	int ignore;
	
	sprintf(tmpbuf, "/tmp/msg.%d", getpid());
	fp = fopen(tmpbuf, "w");

	fd = ddmsg_open_msg(current_conf->CONF_PATH, current_base->MSGBASE_NUMBER, messagenumber, O_RDONLY, 0660);
	if(fd < 0) {
		return -1;
	}

	ignore = 0;
	
	while((i = read(fd, tmpbuf, 128)) > 0) {
		for(j = 0; j < i; ++j) {
			if(tmpbuf[j] == '\1' || 
			   !strncasecmp(tmpbuf+j, "SEEN-BY:", 8) ||
			   !strncasecmp(tmpbuf+j, "AREA:", 5)) {
				ignore = 1;
			} 
			else if(tmpbuf[j] == '\n' && ignore) {
				ignore = 0;
			}
			else if(tmpbuf[j] == '\n') {
				fputc('\r', fp);
				fputc('\n', fp);
			}
			else if(!ignore) {
				fputc(tmpbuf[j], fp);
			}
			
		}
	}

	ddmsg_close_msg(fd);
	fclose(fp);
	
	return 0;

}

int print_message(int messagenumber) {
	char tmpbuf[128];
	char filename[128];
	FILE* fp;

	generate_temp_message(messagenumber);
	sprintf(filename, "/tmp/msg.%d", getpid());
	fp = fopen(filename, "r");
	
	while(fgets(tmpbuf, 128, fp) != NULL) {
		fputs(tmpbuf, stdout);
	}
	
	fclose(fp);
	
	return 0;
}

int message_num_ok(int message_number) {
	struct DayDream_MsgPointers ptrs;
	struct DayDream_Message dd_msg;
	int retval;
	int fd;

	ddmsg_getptrs(current_conf->CONF_PATH, current_base->MSGBASE_NUMBER, &ptrs); 
	if(message_number < ptrs.msp_low || message_number > ptrs.msp_high) {
		return 0;
	}
	
	fd = ddmsg_open_base(current_conf->CONF_PATH, current_base->MSGBASE_NUMBER, O_RDONLY, 0660);
	if(fd < 0) {
		return 0;
	}
	
	retval = 0;

	while(read(fd, &dd_msg, sizeof(struct DayDream_Message)) == sizeof(struct DayDream_Message)) {
		if(dd_msg.MSG_NUMBER == message_number) {
			if(!(dd_msg.MSG_FLAGS & MSG_FLAGS_PRIVATE)) {
				retval = 1;
			}

			break;
		}
	}
	
	ddmsg_close_base(fd);
	
	return retval;

}

void strip_crlf(char* str) {
	while(*str != '\0') {
		if(*str == '\n' || *str == '\r') {
			*str = '\0';
			break;
		}
		++str;
	}
}

int process_xover_msg(struct DayDream_Message* dd_msg) {
	int number_of_lines;
	int number_of_bytes;
	struct tm* t;
	char tmpbuf[128];
	
	generate_temp_message(dd_msg->MSG_NUMBER);
	
	number_of_lines = count_lines_in_temp_message();
	number_of_bytes = count_bytes_in_temp_message();

	t = gmtime(&dd_msg->MSG_CREATION);
	strftime(tmpbuf, 128, "%a, %d %b %Y %H:%M:%S -0000", t);
	
	printf("%d\t%s\t%s <dummy@localhost>\t%s\t%s\t%s\t%d\t%d\t\r\n", 
		dd_msg->MSG_NUMBER, 
		dd_msg->MSG_SUBJECT, 
		strip_hibit((unsigned char*) dd_msg->MSG_AUTHOR),
		tmpbuf,
		generate_message_id(dd_msg->MSG_NUMBER),
		"",
		number_of_bytes,
		number_of_lines
		);
	
	return 0;

}

int process_xover(int start, int end) {
	int fd;
	struct DayDream_Message dd_msg;
	
	fd = ddmsg_open_base(current_conf->CONF_PATH, current_base->MSGBASE_NUMBER, O_RDONLY, 0660);
	if(fd < 0) {
		return -1;
	}
	
	while(read(fd, &dd_msg, sizeof(struct DayDream_Message)) == sizeof(struct DayDream_Message)) {
		if(dd_msg.MSG_NUMBER >= start && dd_msg.MSG_NUMBER <= end) {
			if(!(dd_msg.MSG_FLAGS & MSG_FLAGS_PRIVATE)) { 
				process_xover_msg(&dd_msg);
			}
		}
	}
	
	ddmsg_close_base(fd);
	
	return 0;
}

void parse_input_message_id(char* input_str, int* messageid) {
	char tmpbuf[128];
	if(*input_str == '<') {
	    sscanf(input_str, "<%d@%s>", messageid, tmpbuf);
	}
	else {
	    *messageid = atoi(input_str);
	}
}

void find_reply_msgid(int ref_num, char* reply_str, char* to_str) {
	int fd;
	FILE* fh;
	char tmpbuf[128];
	struct DayDream_Message dd_msg;
	
	fd = ddmsg_open_base(current_conf->CONF_PATH, 
			current_base->MSGBASE_NUMBER, 
			O_RDWR | O_CREAT, 0666);
			
	while(read(fd, &dd_msg, sizeof(struct DayDream_Message)) == sizeof(struct DayDream_Message)) {
		if(dd_msg.MSG_NUMBER == ref_num) {
			strcpy(to_str, dd_msg.MSG_AUTHOR);
			break;
		}
	
	}
	
	ddmsg_close_base(fd);

	fd = ddmsg_open_msg(current_conf->CONF_PATH, 
			current_base->MSGBASE_NUMBER, 
			ref_num,
			O_RDWR | O_CREAT, 0666);
	fh = fdopen(fd, "r");
	
	while(fgets(tmpbuf, 128, fh) != NULL) {
		strip_crlf(tmpbuf);
		if(!strncasecmp(tmpbuf, "\1MSGID:", 7)) {
			strcpy(reply_str, tmpbuf+8);
			break;
		}
	}
	
	fclose(fh);
	ddmsg_close_msg(fd);
}

int process_post() {
	int fd;
	int fd2;
	char buf[128];
	char from_str[128];
	char to_str[128];
	char area_str[128];
	char subject_str[128];
	char reply_str[128];
	char filename[128];
	int ref_num = 0;
	struct DayDream_Message dd_msg;
	struct DayDream_MsgPointers ptrs;
	char* ptr;
	int header_mode = 1;
	FILE* fh;
	int uq;
	int i;
	
	fh = NULL;
	
	memset(&dd_msg, '\0', sizeof(struct DayDream_Message));

	while(1) {
		if(fgets(buf, 128, stdin) == NULL) {
			break;
		}
		strip_crlf(buf);

		if(header_mode) {
			if(!strncasecmp(buf, "From:", 5)) {
				/* From: Bo Simonsen <bo@geekworld.dk>
				         ^^^^^^^^^^^^ This is interesting
				*/
				ptr = strchr(buf+6, '<');
				if(ptr != NULL) {
					--ptr;
					*ptr = '\0';
				}
				strncpy(from_str, buf+6, 128);
			}
			else if(!strncasecmp(buf, "References:", 11)) {
				/* References: <1$AREA@localhost>
				                ^ This is interesting 
				*/
				parse_input_message_id(buf+12, &ref_num);
			}
			else if(!strncasecmp(buf, "Newsgroups:", 11)) {
				/* Newsgroups: Fidonet.FIDONEWS */
				strncpy(area_str, buf+12, 128);
			}
			else if(!strncasecmp(buf, "Subject:", 8)) {
				strncpy(subject_str, buf+9, 128);
			}
			else if(!strcasecmp(buf, "")) {
				do_group_command(area_str, 0);
				
				if(current_conf == NULL || current_base == NULL)
					return -1;
				
				reply_str[0] = 0;
				to_str[0] = 0;
				find_reply_msgid(ref_num, reply_str, to_str);
			
				strncpy(dd_msg.MSG_RECEIVER, to_str, 25);
				strncpy(dd_msg.MSG_AUTHOR, from_str, 25);
				strncpy(dd_msg.MSG_SUBJECT, subject_str, 25);
				dd_msg.MSG_CREATION = time(NULL);
				dd_msg.MSG_RECEIVED = time(NULL);
				
				dd_msg.MSG_FN_ORIG_ZONE = current_base->MSGBASE_FN_ZONE;
				dd_msg.MSG_FN_ORIG_NET = current_base->MSGBASE_FN_NET;
				dd_msg.MSG_FN_ORIG_NODE = current_base->MSGBASE_FN_NODE;
				dd_msg.MSG_FN_ORIG_POINT = current_base->MSGBASE_FN_POINT;
				
				dd_msg.MSG_FLAGS = MSG_FLAGS_LOCAL;
				
				header_mode = 0;
				
				sprintf(filename, "/tmp/msg.%d", getpid());
				fh = fopen(filename, "w");

				fprintf(fh, "AREA:%s\n", current_base->MSGBASE_FN_TAG);
				if((uq = dd_getfidounique())) {
					fprintf(fh, "\1MSGID: %d:%d/%d.%d %08x\n", 
						current_base->MSGBASE_FN_ZONE,
						current_base->MSGBASE_FN_NET,
						current_base->MSGBASE_FN_NODE,
						current_base->MSGBASE_FN_POINT,
						uq);
				}
				if(reply_str[0] != 0) {
					fprintf(fh, "\1REPLY: %s\n", reply_str);
				}
				fputc('\n', fh);
			}
		}
		else {
			if(!strcasecmp(buf, ".")) {
				break;
			}
			else {
				if(!fh) {
					return -1;
				}
				
				fputs(buf, fh);
				fputc('\n', fh);
			}
		
		}
	}
	
	fprintf(fh, "\n--- DayDream BBS/Linux %s (via NNTP)\n", versionstring);
	fprintf(fh, " * Origin: %s (%d:%d/%d.%d)\n", current_base->MSGBASE_FN_ORIGIN, 
		current_base->MSGBASE_FN_ZONE,
		current_base->MSGBASE_FN_NET,
		current_base->MSGBASE_FN_NODE,
		current_base->MSGBASE_FN_POINT);
	fprintf(fh, "SEEN-BY: %d/%d\n", current_base->MSGBASE_FN_NET,
		current_base->MSGBASE_FN_NODE);
	
	fclose(fh);
	
	fd = ddmsg_open_base(current_conf->CONF_PATH, 
		current_base->MSGBASE_NUMBER, 
		O_WRONLY, 0666);
		
	if(fd == -1) {
		return -1;
	}
	
	lseek(fd, 0, SEEK_END);
				
	ddmsg_getptrs(current_conf->CONF_PATH, 
		current_base->MSGBASE_NUMBER, 
		&ptrs);
					
	ptrs.msp_high++;
				
	ddmsg_setptrs(current_conf->CONF_PATH, 
		current_base->MSGBASE_NUMBER, 
		&ptrs);
		
	dd_msg.MSG_NUMBER = ptrs.msp_high;

	if(write(fd, &dd_msg, sizeof(struct DayDream_Message)) != sizeof(struct DayDream_Message)) {
		ddmsg_close_base(fd);
		return -1;
	}
				
	ddmsg_close_base(fd);
	
	fd = ddmsg_open_msg(current_conf->CONF_PATH, 
		current_base->MSGBASE_NUMBER,
		dd_msg.MSG_NUMBER, 
		O_RDWR | O_CREAT, 0666);
		
	if(fd == -1) {
		return -1;
	}

	sprintf(filename, "/tmp/msg.%d", getpid());
	fd2 = open(filename, O_RDONLY);
	
	while((i = read(fd2, buf, 128)) == 128) {
		write(fd, buf, 128);
	}
	
	if(i > 0) {
		write(fd, buf, i);
	}
	
	close(fd2);
	ddmsg_close_msg(fd);
	
	return 0;
}

int check_auth() {
	if(!auth_ok) {
		printf("480 Authentication required\r\n");
		return 0;
	}
	
	return 1;

}

int main(int argc, char *argv[])
{
	char buf[128];
	char filename[128];
	char dd[128];
	char user_str[128];
	char* ptr;
	
	int tmp;
	int first_arg;
	int second_arg;
	
	int i;

	auth_ok = 0;

	for(i=0; i < argc; ++i) {
		if(!strcmp(argv[i], "-a")) {
			auth_ok = 1;
		}
	}
	
	current_conf = NULL;
	current_base = NULL;
	
	ptr = getenv("DAYDREAM");
	if(!ptr) {
		syslog(0, "Can't get DAYDREAM env");
		exit(1);
	}

	strncpy(dd, ptr, 128);
	datadir = dd;
	
	printf("200 DDNNTP ready to serve\r\n");
	fflush(stdout);
	while(1) {
		if(fgets(buf, 128, stdin) == NULL) {
			break;
		}
		strip_crlf(buf);
		
		if(!strncasecmp(buf, "LIST", 4)) {
			if(check_auth()) {
				if(strlen(buf) > 5) {
					if(!strncasecmp(buf+5, "NEWSGROUPS", 10)) {
						generate_list_with_desc();
					}
					else {
						generate_list();
	
					}
				} 
				else {
					generate_list();
				}
			}
		}
		else if(!strncasecmp(buf, "GROUP", 5)) {
			if(check_auth()) {
				if(strlen(buf) < 7) {
					printf("411 no such news group\r\n");
				}
				else {
					if(do_group_command(buf+6, 1) < 0) {
						printf("411 no such news group\r\n");
					}
				}
			}
		}
		else if(!strncasecmp(buf, "HEAD", 4) ||
		        !strncasecmp(buf, "BODY", 4) ||
		        !strncasecmp(buf, "ARTICLE", 7)) {
		        
			if(check_auth()) {
				if(!current_base || !current_conf) {
					printf("412 No newsgroup has been selected.\r\n");
				}
				else {
					if(!strncasecmp(buf, "ARTICLE", 7)) {
						parse_input_message_id(buf+8, &tmp);
					}
					else {
						parse_input_message_id(buf+5, &tmp);
					}
				
					if(message_num_ok(tmp)) {
						if(!strncasecmp(buf, "HEAD", 4)) {
							printf("221 %d %s article retrieved - head follows\r\n", tmp, generate_message_id(tmp));
							print_message_headers(tmp);
							printf(".\r\n");
						}
						else if(!strncasecmp(buf, "BODY", 4)) {
							printf("222 %d %s article retrieved - body follows\r\n", tmp, generate_message_id(tmp));
							print_message(tmp);
							printf(".\r\n");
						}
						else {
							printf("220 %d %s article retrieved - head and body follow\r\n", tmp, generate_message_id(tmp));
							print_message_headers(tmp);
							printf("\r\n");
							print_message(tmp);
							printf(".\r\n");
						}
					}
					else {
						printf("423 No such article number in this group.\r\n");
					}
				}
			}
		}
		else if(!strncasecmp(buf, "QUIT", 4)) {
			printf("205 Closing connection - Goodbye!\r\n");
			break;
		}
		else if(!strncasecmp(buf, "MODE READER", 11)) {
			printf("200 Hello, you can post.\r\n");
		}
		else if(!strncasecmp(buf, "XOVER", 5)) {
			if(check_auth()) {
				if(!current_base || !current_conf) {
					printf("412 No news group current selected.\r\n");
				}
				else {
					ptr = strchr(buf+5, '-');
					if(ptr == NULL) {
						printf("420 No article(s) selected.\r\n");
					}
					else {
						*ptr = '\0';
						first_arg = atoi(buf+6);
						second_arg = atoi(ptr+1);
						printf("224 %d-%d fields follow\r\n", first_arg, second_arg);
						process_xover(first_arg, second_arg);
						printf(".\r\n");
					}
				}	
			}
		}
		else if(!strncasecmp(buf, "POST", 4)) {
			if(check_auth()) {
				printf("340 Ok\r\n");
				fflush(stdout);
				if(!process_post()) {
					printf("240 Article posted.\r\n");
					syslog(LOG_INFO, "Message posted\n");
				}
				else {
					printf("441 posting failed.\r\n");
					syslog(LOG_INFO, "Message post failed\n");
				}
			}
		}
		else if(!strncasecmp(buf, "IHAVE", 5)) {
			if(check_auth()) {
				printf("335 Go ahead\r\n");
				fflush(stdout);
				if(!process_post()) {
					printf("235 Article posted.\r\n");
				}
				else {
					printf("435 posting failed.\r\n");
				}
			}
		}
		else if(!strncasecmp(buf, "AUTHINFO", 8)) {
			if(!strncasecmp(buf+9, "USER", 4)) {
				strncpy(user_str, buf+14, 128);
				printf("381 More authentication information required.\r\n");
			}
			else if(!strncasecmp(buf+9, "PASS", 4)) {
				struct userbase* u = ddgetpwnam(user_str);	
				if(!u) {
					printf("482 Authentication rejected.\r\n");
				}
				else if(cmppasswds(buf+14, u->user_password)) {
					syslog(LOG_INFO, "User %s logged in", user_str);
					printf("281 Authentication accepted.\r\n");
					auth_ok = 1;
				}
				else {
					printf("482 Authentication rejected.\r\n");
				}
			}
		}
		else {
			printf("500 Command not recognized\r\n");
			syslog(LOG_INFO, "Unknown command: %s", buf);
		}
		fflush(stdout);
	}

	sprintf(filename, "/tmp/msg.%d", getpid());
	unlink(filename);
	
	return 0;
}
