#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>

#include <daydream.h>
#include <symtab.h>
#include <ddcommon.h>

static int doorcnt = 0;

static int write_dropfile(char *, int);

list_t *child_pid_list = NULL;

static int accept_nonblock(int sockfd, struct sockaddr *addr, 
	socklen_t *addrlen)
{
	fd_set rset;
	struct timeval tv;

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	if (select(sockfd + 1, &rset, NULL, NULL, &tv) <= 0)
		return -1;
		
	return accept(sockfd, addr, addrlen);
}	
	
int runpython(const char *cmd, const char *params)
{
	char buf[1024];
	if (!*maincfg.CFG_PYTHON)
		return 0;
	snprintf(buf, sizeof buf, "%s %s %%N", maincfg.CFG_PYTHON, cmd);
	return rundoor(buf, params);
}

static void door_loop(int sockfd, pid_t child_pid, const char *door_params)
{
	struct DayDream_DoorMsg msg;
	char extstatus[50];

	if (!ispid(child_pid))
		return;
	sockfd = accept_nonblock(sockfd, NULL, NULL);
	if (sockfd == -1) {
		kill_child(child_pid);
		return;
	}
	
	child_pid_list = push(child_pid_list, (void *) child_pid);
	doorcnt++;
	
	for (;;) {
		fd_set setti;
		int re;
		int maxfd;
		struct timeval tv;
		
		while ((waitpid(-1, NULL, WNOHANG)) > 0);
		
		FD_ZERO(&setti);
		FD_SET(sockfd, &setti);
		maxfd = sockfd;
		if (!bgmode) {
			FD_SET(dsockfd, &setti);
			if (dsockfd > sockfd)
				maxfd = dsockfd;
		}
		tv.tv_usec = 0;
		tv.tv_sec = 2;
		
		select(maxfd + 1, &setti, 0, 0, &tv);
		
		if (!bgmode && FD_ISSET(dsockfd, &setti)) {
			struct dd_nodemessage ddn;
			read(dsockfd, &ddn, sizeof(struct dd_nodemessage));
			processmsg(&ddn);
		} else if (FD_ISSET(sockfd, &setti)) {
			re = read(sockfd, &msg, sizeof(struct DayDream_DoorMsg));
			if (!re)
				break;
			
			if (msg.ddm_command == 1)
				break;
			
			switch (msg.ddm_command) {
			case 2:
				DDPut(msg.ddm_string);
				break;
			case 3:
				msg.ddm_data1 = Prompt(msg.ddm_string, msg.ddm_data1, msg.ddm_data2);
				break;
			case 4:
				msg.ddm_data1 = (uint16_t) HotKey(msg.ddm_data1);
				break;
			case 5:
				msg.ddm_data1 = TypeFile(msg.ddm_string, msg.ddm_data1);
				break;
			case 6:
				msg.ddm_data1 = flagsingle(msg.ddm_string, msg.ddm_data1);
				break;
			case 7:
				msg.ddm_data1 = findusername(msg.ddm_string);
				break;
			case 8:
				runstdio(msg.ddm_string, -1, msg.ddm_data1);
				break;
			case 9:
				docmd(msg.ddm_string, 
					strlen(msg.ddm_string), 0);
				break;
			case 10:
				writelog(msg.ddm_string);
				break;
			case 11:
				/* FIXME! */
				strlcpy(extstatus, msg.ddm_string, 
					sizeof extstatus);
				changenodestatus(extstatus);
				break;
			case 12:
				dpause();
				break;
			case 13:
				msg.ddm_data1 = joinconf(msg.ddm_data1, msg.ddm_data2);
				break;
			case 14:
				msg.ddm_data1 = isfreedl(msg.ddm_string);
				break;
			case 15:
				msg.ddm_data1 = flagfile(msg.ddm_string, msg.ddm_data1);
				break;
			case 16:
				msg.ddm_data1 = lrp;
				msg.ddm_data2 = lsp;
				break;
			case 17:
				lrp = msg.ddm_data1;
				lsp = msg.ddm_data2;
				break;
			case 18:
				msg.ddm_data1 = checkconfaccess(msg.ddm_data1, &user);
				break;
			case 19:
				msg.ddm_data1 = isanybasestagged(msg.ddm_data1);
				break;
			case 20:
				msg.ddm_data1 = isconftagged(msg.ddm_data1);
				break;
			case 21:
				msg.ddm_data1 = isbasetagged(msg.ddm_data1, msg.ddm_data2);
				break;
			case 22:
				getmsgptrs();
				msg.ddm_data1 = highest;
				msg.ddm_data2 = lowest;
				break;
			case 23:
				highest = msg.ddm_data1;
				lowest = msg.ddm_data2;
				setmsgptrs();
				break;
			case 24:
				msg.ddm_data1 = changemsgbase(msg.ddm_data1, msg.ddm_data2);
				break;
			case 25:
				sendfiles(msg.ddm_string, 0, sizeof msg.ddm_string);
				break;
			case 26:
				recfiles(msg.ddm_string, 0);
				break;
			case 27:
				fileattach();
				break;
			case 28:
				msg.ddm_data1 = unflagfile(msg.ddm_string);
				break;
			case 29:
				msg.ddm_data1 = findfilestolist(msg.ddm_string, &msg.ddm_string[50]);
				break;
			case 30:
				msg.ddm_data1 = isfiletagged(msg.ddm_string);
				break;
			case 31:
				msg.ddm_data1 = dumpfilestofile(msg.ddm_string);
				break;
			case 100:
				strlcpy(msg.ddm_string, maincfg.CFG_BOARDNAME, sizeof msg.ddm_string);
				break;
				
			case 101:
				strlcpy(msg.ddm_string, maincfg.CFG_SYSOPNAME, sizeof msg.ddm_string);
				break;
				
			case 102:
				if (msg.ddm_data1) {
					strlcpy(user.user_realname, msg.ddm_string, sizeof user.user_realname);
				} else {
					strlcpy(msg.ddm_string, user.user_realname, sizeof msg.ddm_string);
				}
				break;
				
			case 103:
				if (msg.ddm_data1) {
					strlcpy(user.user_handle, msg.ddm_string, sizeof user.user_handle);
				} else {
					strlcpy(msg.ddm_string, user.user_handle, sizeof msg.ddm_string);
				}
				break;
				
			case 104:
				if (msg.ddm_data1) {
					strlcpy(user.user_organization, msg.ddm_string, sizeof user.user_organization);
				} else {
					strlcpy(msg.ddm_string, user.user_organization, sizeof msg.ddm_string);
				}
				break;
			case 105:
				if (msg.ddm_data1) {
					strlcpy(user.user_zipcity, msg.ddm_string, sizeof user.user_zipcity);
				} else {
					strlcpy(msg.ddm_string, user.user_zipcity, sizeof msg.ddm_string);
				}
				break;
			case 106:
				if (msg.ddm_data1) {
					strlcpy(user.user_voicephone, msg.ddm_string, sizeof user.user_voicephone);
				} else {
					strlcpy(msg.ddm_string, user.user_voicephone, sizeof msg.ddm_string);
				}
				break;
			case 107:
				if (msg.ddm_data1) {
					strlcpy(user.user_computermodel, msg.ddm_string, sizeof user.user_computermodel);
				} else {
					strlcpy(msg.ddm_string, user.user_computermodel, sizeof msg.ddm_string);
				}
				break;
			case 108:
				if (msg.ddm_data1) {
					strlcpy(user.user_signature, msg.ddm_string, sizeof user.user_signature);
				} else {
					strlcpy(msg.ddm_string, user.user_signature, sizeof msg.ddm_string);
				}
				break;
			case 109:
				if (msg.ddm_data1) {
					user.user_screenlength = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_screenlength;
				}
				break;
				
			case 110:
				if (msg.ddm_data1) {
					user.user_toggles = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_toggles;
				}
				break;
				
			case 111:
				/*                                              
				 if (msg.ddm_data1) {
				 user.user_ulbytes=msg.ddm_data2;
				 } else {
				 msg.ddm_data1=user.user_ulbytes;
				 } Disabled */
				
				break;
			case 112:
				/*                                              if (msg.ddm_data1) {
				 user.user_dlbytes=msg.ddm_data2;
				 } else {
				 msg.ddm_data1=user.user_dlbytes;
				 } Disabled */
				break;
			case 113:
				if (msg.ddm_data1) {
					user.user_ulfiles = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_ulfiles;
				}
				break;
			case 114:
				if (msg.ddm_data1) {
					user.user_dlfiles = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_dlfiles;
				}
				break;
			case 115:
				if (msg.ddm_data1) {
					user.user_pubmessages = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_pubmessages;
				}
				break;
			case 116:
				if (msg.ddm_data1) {
					user.user_pvtmessages = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_pvtmessages;
				}
				break;
			case 117:
				if (msg.ddm_data1) {
					user.user_connections = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_connections;
				}
				break;
			case 118:
				if (msg.ddm_data1) {
					user.user_fileratio = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_fileratio;
				}
				break;
			case 119:
				if (msg.ddm_data1) {
					user.user_byteratio = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_byteratio;
				}
				break;
			case 120:
				if (msg.ddm_data1) {
					user.user_freedlbytes = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_freedlbytes;
				}
				break;
			case 121:
				if (msg.ddm_data1) {
					user.user_freedlfiles = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_freedlfiles;
				}
				break;
			case 122:
				if (msg.ddm_data1) {
					user.user_securitylevel = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_securitylevel;
				}
				break;
			case 123:
				if (msg.ddm_data1) {
					user.user_joinconference = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_joinconference;
				}
				break;
			case 124:
				if (msg.ddm_data1) {
					user.user_conferenceacc1 = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_conferenceacc1;
				}
				break;
			case 125:
				if (msg.ddm_data1) {
					user.user_conferenceacc2 = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_conferenceacc2;
				}
				break;
			case 126:
				if (msg.ddm_data1) {
					user.user_dailytimelimit = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_dailytimelimit;
				}
				break;
			case 127:
				msg.ddm_data1 = user.user_account_id;
				break;
			case 128:
				if (msg.ddm_data1) {
					time_t curr;
					timeleft = msg.ddm_data2;
					curr = time(0);
					endtime = curr + timeleft;
					user.user_timeremaining = timeleft / 60;
				} else {
					msg.ddm_data1 = timeleft;
				}
				break;
			case 129:
				if (door_params) {
					strncpy(msg.ddm_string, door_params, 300);
				} else {
					msg.ddm_string[0] = 0;
				}
				break;
			case 130:
				strlcpy(msg.ddm_string, origdir, sizeof msg.ddm_string);
				break;
			case 131:
				msg.ddm_data1 = conference()->conf.CONF_NUMBER;
				break;
			case 132:
				if (msg.ddm_data1) {
					user.user_ulbytes = msg.ddm_ldata;
				} else {
					msg.ddm_ldata = user.user_ulbytes;
				}
				
				break;
			case 133:
				if (msg.ddm_data1) {
					user.user_dlbytes = msg.ddm_ldata;
				} else {
					msg.ddm_ldata = user.user_dlbytes;
				}
				break;
			case 134:
				if (msg.ddm_data1) {
					user.user_firstcall = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_firstcall;
				}
				break;
			case 135:
				if (msg.ddm_data1) {
					user.user_flines = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_flines;
				}
				break;
			case 136:
				if (msg.ddm_data1) {
					user.user_lastcall = msg.ddm_data2;
				} else {
					msg.ddm_data1 = last;
				}
				break;
			case 137:
				if (msg.ddm_data1) {
					user.user_protocol = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_protocol;
				}
				break;
			case 138:
				if (msg.ddm_data1) {
					user.user_fakedfiles = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_fakedfiles;
				}
				break;
			case 139:
				if (msg.ddm_data1) {
					user.user_fakedbytes = msg.ddm_data2;
				} else {
					msg.ddm_data1 = user.user_fakedbytes;
				}
				break;
			case 140:
				recountfiles();
				msg.ddm_data1 = filestagged;
				break;
			case 141:
				recountfiles();
				msg.ddm_data1 = bytestagged;
				break;
			case 142:
				recountfiles();
				msg.ddm_data1 = flagerror;
				break;
				
			case 143:
				msg.ddm_data1 = current_msgbase->MSGBASE_NUMBER;
				break;
			}
			safe_write(sockfd, &msg, sizeof(struct DayDream_DoorMsg));
		}
		if (!ispid(car(int, child_pid_list)))
			break;
	}		

	kill_child(shift(int, child_pid_list));	
	doorcnt--;
	
	close(sockfd);
}

static void set_library_path(void)
{
	char *lpath, *ldlibpath;

	if (!strlen(maincfg.CFG_DOORLIBPATH))
		return;

	/* This is used for doors only, so we're not leaking
	 * any memory here in putenv().
	 */
	ldlibpath = getenv("LD_LIBRARY_PATH");

	if (ldlibpath) {
		lpath = (char *) xmalloc(strlen(ldlibpath) + 
			strlen(maincfg.CFG_DOORLIBPATH) + 17);
		strcpy(lpath, "LD_LIBRARY_PATH=");
		strcat(lpath, ldlibpath);
		strcat(lpath, ":");
	} else {
		lpath = (char *) xmalloc(strlen(maincfg.CFG_DOORLIBPATH) + 16);
		strcpy(lpath, "LD_LIBRARY_PATH=");
	}	
	strcat(lpath, maincfg.CFG_DOORLIBPATH);
	putenv(lpath);
}

int rundoor(const char *command, const char *params)
{
	int sockfd;
	char sockname[80];
	char *args[10];
	char dcmd[250];
	struct sockaddr_un doorsock;
	pid_t pid;
	int i = 1;
	char *s;
	char doom[10];

	syslog(LOG_DEBUG, "running door %.200s", command);
	changenodestatus("Running a door...");

	if (command == NULL)
		return 0;

	if (strspace(dcmd, command, sizeof dcmd) > sizeof dcmd) {
		syslog(LOG_WARNING, "rundoor(): command too long");
		return 0;
	}
	if (access(dcmd, R_OK | X_OK) == -1) {
		syslog(LOG_WARNING, "rundoor(): door not found: %m");
		return 0;
	}

	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		DDPut("Can't create socket...\n\n");
		return 0;
	}
	
	if (!doorcnt) {
		snprintf(sockname, sizeof sockname, 
			"%s/dd_door%d", DDTMP, node);
		snprintf(doom, sizeof doom, "%d", node);
	} else {
		snprintf(sockname, sizeof sockname,
			 "%s/dd_door%d_%d", DDTMP, node, doorcnt);
		snprintf(doom, sizeof doom, "%d_%d", node, doorcnt);
	}
	unlink(sockname);
	strlcpy(doorsock.sun_path, sockname, sizeof doorsock.sun_path);
	doorsock.sun_family = AF_UNIX;
	if (bind(sockfd, (struct sockaddr *) &doorsock, sizeof doorsock) < 0) {
		DDPut("Can't bind socket...\n\n");
		close(sockfd);
		return 0;
	}
	listen(sockfd, 2);

	genstdiocmdline(dcmd, command, 0, doom);

	s = dcmd;
/*      while (*s!=' ') s++;
   *s=0;
   s++; */

	args[0] = s;

	while (*s) {
		if (*s == ' ') {
			*s = 0;
			s++;
			args[i] = s;
			i++;
		} else
			s++;
	}
	args[i] = 0;

	switch (pid = fork()) {
	case -1:
		syslog(LOG_ERR, "fork failed: %m");
		return 0;
	case 0:
		set_library_path();				
		execv(args[0], args);
		syslog(LOG_ERR, "execv failed: %m");
		DDPut("Command failed.\n");
		_exit(1);
	default:
		door_loop(sockfd, pid, params);		
		break;
	}

	close(sockfd);
	unlink(sockname);

	/* cancel the timeout alarm */
	if (timeleft < 0) {
		alarm(0);
		dropcarrier();
		return 0;
	}
	return 1;
}

int rundosdoor(const char *command, int dropfile) 
{
	char *ptr; 
	char *s;
	char *args[10];
	char dcmd[250]; 
	char doom[10];
	pid_t pid;
	int i = 1;
	char pname[PATH_MAX];
	int status;

	syslog(LOG_DEBUG, "running DOS door %.200s", command);
	changenodestatus("Running a door...");
		
	if (strlcpy(pname, command, sizeof pname) >= sizeof pname)
		return 0;
	if (!(ptr = strrchr(pname, '/')))
		*pname = '\0';
	else 
		ptr[1] = '\0';

	if (dropfile == 1)
		snprintf(dcmd, sizeof dcmd, "dorinfo%d.def", node);
	else
		strlcpy(dcmd, "door.sys", sizeof dcmd);

	if (strlcat(pname, dcmd, sizeof pname) >= sizeof pname)
		return 0;
		
	if (write_dropfile(pname, dropfile) == -1)
		return 0;

	if (!doorcnt)
		snprintf(doom, sizeof doom, "%d", node);
	else
		snprintf(doom, sizeof doom, "%d_%d", node, doorcnt);

	genstdiocmdline(dcmd, command, 0, doom);

	snprintf(doom, sizeof doom, " %d", node);
	if (strlcat(dcmd, doom, sizeof dcmd) >= sizeof dcmd)
		return 0;
	
	runstdio(dcmd, -1, 1);

	return 1;
}

static int write_dropfile(char *file, int type) 
{
	FILE *fp;
	char buffer1[26], buffer2[26], *ptr;
	struct tm *time_p;
	int tmp;
	
	if ((fp = fopen(file, "w")) == NULL)
		return -1;
	
	if (type == 1) { /* DORINFO%N.DEF */
		ptr = strrchr(user.user_realname, ' ');

		if (ptr == NULL) {
			strcpy(buffer1, user.user_realname);
			strcpy(buffer2, "NLN");
		} else {
			memset(buffer1, 0, 26);
			strncpy(buffer1, user.user_realname, (ptr - user.user_realname));
			strcpy(buffer2, user.user_realname + (ptr - user.user_realname) + 1); 
		}
		
		for (tmp = 0; tmp < 26; tmp++) { /* names have to be UPPERCASE */
			buffer1[tmp] = toupper(buffer1[tmp]);
			buffer2[tmp] = toupper(buffer2[tmp]);
		}
		fprintf(fp, "%s\r\n", maincfg.CFG_BOARDNAME);
		fprintf(fp, "%s\r\n", maincfg.CFG_SYSOPFIRST);
		fprintf(fp, "%s\r\n", maincfg.CFG_SYSOPLAST);
		fprintf(fp, "COM1\r\n");
		fprintf(fp, "%d BAUD,8,N,1\r\n", bpsrate);
		fprintf(fp, " 0\r\n");
		fprintf(fp, "%s\r\n", buffer1);
		fprintf(fp, "%s\r\n", buffer2);
		if (user.user_zipcity[0] == 0)
			fprintf(fp, "No City Supplied\r\n");
		else
			fprintf(fp, "%s\r\n", user.user_zipcity);
		if (ansi == 1)
			fprintf(fp, "1\r\n");
		else
			fprintf(fp, "0\r\n");
		fprintf(fp, "%d\r\n", user.user_securitylevel);
		fprintf(fp, "%d\r\n", user.user_timeremaining);
		fprintf(fp, "-1\r\n");
	} else if (type == 2) { /* DOOR.SYS */
		time_p = localtime(&user.user_lastcall);
		strftime(buffer1, 26, "%m/%d/%y", time_p);
				
		fprintf(fp, "COM1:\r\n");
		fprintf(fp, "%d\r\n", bpsrate);
		fprintf(fp, "8\r\n");
		fprintf(fp, "%d\r\n", node);
		fprintf(fp, "%d\r\n", bpsrate);
		fprintf(fp, "Y\r\n");
		fprintf(fp, "Y\r\n");
		fprintf(fp, "Y\r\n");
		fprintf(fp, "Y\r\n");
		fprintf(fp, "%s\r\n", user.user_realname);
		
		if (user.user_zipcity[0] == 0)
			fprintf(fp, "No City Supplied\r\n");
		else
			fprintf(fp, "%s\r\n", user.user_zipcity);
		if (user.user_voicephone[0] == 0) {
			fprintf(fp, "No Phone Supplied\r\n");
			fprintf(fp, "No Phone Supplied\r\n");		
		} else {
			fprintf(fp, "%s\r\n", user.user_voicephone);
			fprintf(fp, "%s\r\n", user.user_voicephone);
		}
		fprintf(fp, "<encrypted>\r\n");
		fprintf(fp, "%d\r\n", user.user_securitylevel);
		fprintf(fp, "%d\r\n", user.user_connections);
		fprintf(fp, "%s\r\n", buffer1);
		fprintf(fp, "%d\r\n", user.user_timeremaining * 60);
		fprintf(fp, "%d\r\n", user.user_timeremaining);
		if (ansi == 1)
			fprintf(fp, "GR\r\n");
		else
			fprintf(fp, "NG\r\n");
		fprintf(fp, "%d\r\n", user.user_screenlength);
		fprintf(fp, "%c\r\n", (user.user_toggles & (1L << 4) ? 'Y' : 'N'));
		fprintf(fp, "1,2,3,4,6,7\r\n");
		fprintf(fp, "1\r\n");
		fprintf(fp, "01/01/99\r\n");
		fprintf(fp, "%d\r\n", user.user_account_id);
		fprintf(fp, "X\r\n");
		fprintf(fp, "%d\r\n", user.user_ulfiles);
		fprintf(fp, "%d\r\n", user.user_dlfiles);
		fprintf(fp, "0\r\n");
		fprintf(fp, "9999\r\n");
	}

	fclose(fp);
	return 0;
}
