#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "libddc++.h"
#include "common.h"

void exec_cmd(char *cmdline)
{
	char *args[64];
	int i;
	for (i=0; i<64; i++) {
		args[i]=cmdline;
		for (; *cmdline&&!isspace(*cmdline); cmdline++);
		if (!*cmdline)
			break;
		*cmdline++=0;
	}
	args[++i]=NULL;
	execv(args[0], args);
}

int genstdiocmdline(char *dest, char *src, char *arc)
{
//	char tbuffer[60];
//	char *s;
	for (;;) 
		if (*src=='%') {
			if (*(src+1)=='A' || *(src+1)=='a') {
				while (*arc) *dest++=*arc++;
				src+=2;
			} else *dest++=*src++;
			
		} else {
			if (!*src)
				break;
			*dest++=*src++;
		}
	*dest=0;
	return 0;
}

/* filename is supplied, path is returned */

int extract_packet(char *filename, char *path)
{
	int ret_value=EX_FAIL;
	
	if (getarchiver(filename) && (arc&&arc->ARC_EXTRACTFILEID[0])) {
		int child_pid;
		if (tmpnam(path)==NULL)
			goto ret;
		
		char *curdir=getcwd(NULL, 0);
		mkdir(path, 0770);
		chdir(path);
		char *cmdline=new char[512];
		genstdiocmdline(cmdline, arc->ARC_EXTRACTFILEID, filename);
		sprintf(cmdline+strlen(cmdline), " *");
		switch (child_pid=fork()) {
		 case 0:
			exec_cmd(cmdline);
			kill(getpid(), SIGKILL);
			break;
		 case -1:
			break;
		 default:
			dprintf("\e[2H\e[0;42;37;1m\e[K                        Unpacking... press ESC to abort");
			for (;;) {
				if (waitpid(-1, NULL, WNOHANG)!=0)
					break;
				if (HotKey(HOT_QUICK)==0x1b) {				       
					kill(child_pid, SIGTERM);
					wait(NULL);
					ret_value=EX_INTR;
					break;
				}
			}
			break;
		}
		chdir(curdir);
		delete [] cmdline;
		delete [] curdir;
		
		if (ret_value!=EX_INTR)
			ret_value=EX_SUCCESS;
	}
	
	ret:
	return ret_value;
}
			
