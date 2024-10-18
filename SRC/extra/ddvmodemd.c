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
 */

#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <pty.h>

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>

#include <syslog.h>

#include <arpa/telnet.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <net/if.h>

#define BUFSIZE 4096

#define OUTFD 1
#define INFD 0

#define FALSE 0
#define TRUE 1

static char *argv_init[] = {NULL, NULL, NULL, NULL};

void show_usage(void)
{
	printf("Usage: telnetd [-l loginprogram] [-u user]\n");
	printf("\n");
	printf("   -l loginprogram  program started by the server\n");
	printf("   -u user  logins user without password auth\n");
	printf("\n");         
	exit(1);
}

int main(int argc, char **argv)
{
	int new_offset = 0;
	int pid;
	int shell_pid;
	int ptyfd;
	
	unsigned char buf[BUFSIZE];
	char loginpath[256];
	
	char* user = NULL;
	char* login = NULL;
	
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
		strcpy(loginpath, "/bin/login");
	}
	
	argv_init[0] = strdup(loginpath);
	
	if(user) {
		argv_init[1] = strdup("-f");
		argv_init[2] = strdup(user);
		
		free(user);
		user = NULL;
	}

        openlog("ddvmodemd", LOG_PID, LOG_SYSLOG);

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
	    		int r;
	    		
	    		r = read(ptyfd, buf, BUFSIZE);
	    		
			if (r <= 0) {
				break;
			}
				
			if(write(OUTFD, buf, r) != r) {
			   break;
			}
			

		}
		
		if (FD_ISSET(0, &rdfdset)) {
			int r;
			
			r = read(INFD, buf, BUFSIZE);
				
			if (r <= 0) {
				break;
			}
			
			if(write(ptyfd, buf, r) != r) {
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
