/* ddtelnetd.c
 *
 * Simple telnet server for Daydream BBS, 100% fast and 100% 8-bit clean.
 * Bo Simonsen <bo@geekworld.dk>

 * Based on utelnetd, see credits below.
 
 * ---------------------------------------------------------------------------
 * (C) 2000, 2001, 2002, 2003 by the authors mentioned below
 * ---------------------------------------------------------------------------

 * Artur Bajor, Centrum Informatyki ROW   Joerg Schmitz-Linneweber, Aston GmbH
 * <abajor@cirow.pl>                      <schmitz-linneweber@aston-technologie.de>
 *
 * Vladimir Oleynik                       Robert Schwebel, Pengutronix
 * <dzo@simtreas.ru>                      <r.schwebel@pengutronix.de>
 *
 * Bjorn Wesen, Axis Communications AB    Sepherosa Ziehau
 * <bjornw@axis.com>                      <sepherosa@myrealbox.com>
 * 
 * This file is distributed under the GNU General Public License (GPL),
 * please see the file LICENSE for further information.
 * 
 * Recent revisions include provisions for logging IP and hostname lookups.
 * These additions were included in ddtelnetd in 2015 thanks to
 * Michael Griffin and Frank Linhares.
 *
 */

#include <config.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_PTY_H
#include <pty.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>

#include <syslog.h>

#include <arpa/telnet.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFSIZE 4096

#define OUTFD 1
#define INFD 0

#define FALSE 0
#define TRUE 1

static char *argv_init[] = {NULL, NULL, NULL, NULL};

void show_usage(void)
{
	printf("Usage: ddtelnetd [-l loginprogram] [-u user]\n");
	printf("\n");
	printf("   -l loginprogram  program started by the server\n");
	printf("   -u user  logins user without password auth\n");
	printf("\n");         
	exit(1);
}

int send_iac(unsigned char command, int option);

int
remove_iacs(unsigned char *in, int len, unsigned char* out, int* out_count) {
	unsigned char *end = in + len;
	unsigned char* orig_in = in;
	static int ignore = FALSE;
   
	while (in < end) {
		if (*in != IAC) {
			if(ignore == FALSE) {
				*out = *in; in++; out++;
				*out_count = *out_count + 1;
			}
			else {
				in++;
			}
		}
		else {
			if((in+1) == end) {
				*orig_in = IAC;
				return 1;
			}
			
			if(ignore && *(in+1) != SE) {
				send_iac(*(in+1), -1);
				in += 2;
				continue;
			}

			switch(*(in+1)) {
			case IAC:
				*out = IAC; ++out;
				*out_count = *out_count + 1;
				in += 2;
				break;
			case WILL:
			case WONT:
			case DO:
			case DONT:
				if((in+2) == end) {
					*orig_in = IAC;
					*(orig_in+1) = *(end-1);
					return 2;
				}
				in += 3;
				break;
			case SB:
				ignore = TRUE;
				syslog(LOG_INFO, "Negotiation began");
				in += 2;
				break;
			case SE:
				ignore = FALSE;
				syslog(LOG_INFO, "Negotiation completed");
				in += 2;
				break;
			case NOP:
			case DM:
			case BREAK:
			case IP:
			case AO:
			case AYT:
			case GA:
				in += 2;
				break;
			case EC:
				in += 2;
				*out = '\b';
				++out;
				*out_count = *out_count + 1;
				break;
			default:
				syslog(LOG_INFO, "Unknown IAC arg: %d", *(in+1));
				in += 2;
				break;
			}
		}

	}

	return 0;
}

int send_iac(unsigned char command, int option)
{
	char buf[3];
	int w;
	
	buf[0] = IAC;
	buf[1] = command;
	buf[2] = option;
	
	w = write(OUTFD, buf, option == -1 ? 2 : 3);
	
	return w;
}

int main(int argc, char **argv)
{
	int new_offset = 0;
	int pid;
	int shell_pid;
	int ptyfd;
	
	unsigned char buf[BUFSIZE];
	unsigned char tmpbuf[BUFSIZE];
	char loginpath[256];
	char host_string[256];
	char ip_string[256];
	
	char* user = NULL;
	char* login = NULL;
	
	struct sockaddr_in from;
	socklen_t fromlen;
	const char *ip;
	const char *hostname;
	struct hostent *hp;
	
	for (;;) {
		int c;
		c = getopt( argc, argv, "l:u:");
		if (c == EOF) break;
		switch (c) {
			case 'u':
				user = strdup(optarg);
				break;
			case 'l':
				login = strdup(optarg);
				break;
			default:
				printf("%c\n", c);
				show_usage();
				exit(1);
		}
	}

	if(login) {
		strcpy(loginpath, login);
		
		free(login);
		login = NULL;
	} else {
		strcpy(loginpath, LOGIN_PROGRAM);
	}
	
	argv_init[0] = strdup(loginpath);
	
	/* Get host address from remote connection, pass like in.telnetd
	Code snippets below from Michael Griffin */
	
	fromlen = sizeof (from);
	if (getpeername(0, struct sockaddr *)&from, &fromlen) <0) {
		perror("Error: getpeername");
	}
	
	/* Map IP address to host name. */
	hp = gethostbyaddr((char *)&from.sin_addr, sizeof(struct in_addr), from.sin_family);
	if (hp) {
		hostname = hp->h_name;
	} else {
		hostname = inet_ntoa(from.sin_addr);
	}
	
	ip = inet_ntoa(from.sin_addr);
	
	sprintf(ip_string,"-IP%s",strdup(ip));
	sprintf(host_string,"-HOST%s",strdup(hostname));
	
	syslog(LOG_INFO, "incoming connection from %s / %s", strdup(ip), strdup(hostname));
	
	if(user) {
		argv_init[1] = strdup("-f");
		argv_init[2] = strdup(user);
		//argv_init[3] = ip_string;	// unimplemented feature in daydream
		//argv_init[4] = host_string;	// unimplemented feature in daydream
		free(user);
		user = NULL;
	}

        /* These options are taken from maximus ipcomm.c (max 3.03b unix) */

	if(send_iac(DONT, TELOPT_OLD_ENVIRON) < 0) {
	  exit(1);
	}
	if(send_iac(DO, TELOPT_SGA) < 0) {
	  exit(1);
	}
	if(send_iac(WILL, TELOPT_ECHO) < 0) {
	  exit(1);
	}
	if(send_iac(WILL, TELOPT_SGA) < 0) {
	  exit(1);
	}
	if(send_iac(WONT, TELOPT_NAWS) < 0) {
	  exit(1);
	}
	if(send_iac(WILL, TELOPT_BINARY) < 0) {
	  exit(1);
	}
	if(send_iac(DO, TELOPT_BINARY) < 0) {
	  exit(1);
	}

        openlog("ddtelnetd", LOG_PID, LOG_SYSLOG);

	pid = forkpty(&ptyfd, NULL, NULL, NULL);

	if (pid == 0) {
		setsid();
		tcsetpgrp(0, getpid());

		/* exec shell, with correct argv and env */
		execv(loginpath, argv_init);
		exit(1);
	} 
	else if(pid == -1) {
	  closelog();
	  exit(1);
	}
	else { 
	  shell_pid = pid;
	}

	do {
		int selret;
		fd_set rdfdset;
		
		FD_ZERO(&rdfdset);
		FD_SET(ptyfd, &rdfdset);
		FD_SET(INFD, &rdfdset);
    
		selret = select(ptyfd + 1, &rdfdset, NULL, 0, 0);
		
		if (selret <= 0)
			break;
		
		if (FD_ISSET(ptyfd, &rdfdset)) {
			unsigned char* ptr1, *ptr2;	
			int iacs = 0;
	    		int r;
	    		
	    		r = read(ptyfd, tmpbuf, BUFSIZE / 2);
	    		
			if (r <= 0) {
				break;
			}
				
			ptr1 = tmpbuf;
			ptr2 = buf;
				
			while(ptr1 != tmpbuf + r && ptr2 != buf + BUFSIZE) {
				if(*ptr1 == IAC) {
					*ptr2 = IAC; ptr2++;
					*ptr2 = IAC; ptr2++;
					ptr1++; iacs++;
				}
				else {
					*ptr2 = *ptr1;
					ptr1++; ptr2++;
				}
			}
			
			if(write(OUTFD, buf, r + iacs) != (r + iacs)) {
			   break;
			}
			

		}
		
		if (FD_ISSET(0, &rdfdset)) {
			int out_count = 0;
			int r;
			
			r = read(INFD, buf + new_offset, BUFSIZE - new_offset);
				
			if (r <= 0) {
				break;
			}
			
			new_offset = remove_iacs(buf, r + new_offset, tmpbuf, &out_count);
			
			if(write(ptyfd, tmpbuf, out_count) != out_count) {
			   break;
			}
		}

	} while (1);

	close(ptyfd);

	syslog(LOG_INFO, "Closed connection");
	closelog();
	
	_exit(0);
	
	return 1;
}
