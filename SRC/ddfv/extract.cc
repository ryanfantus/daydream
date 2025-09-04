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
#include <cstdlib>
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

int genstdiocmdline(char *dest, char *src, char *arc, size_t dest_size)
{
	size_t dest_pos = 0;
	
	if (!dest || !src || dest_size == 0) {
		return -1;
	}
	
	for (;;) {
		if (dest_pos >= dest_size - 1) {
			// Buffer full, terminate and return error
			dest[dest_pos] = '\0';
			return -1;
		}
		
		if (*src=='%') {
			if (*(src+1)=='A' || *(src+1)=='a') {
				// Copy archive name with bounds checking
				while (*arc && dest_pos < dest_size - 1) {
					dest[dest_pos++] = *arc++;
				}
				if (*arc) {
					// Arc name was truncated
					dest[dest_pos] = '\0';
					return -1;
				}
				src+=2;
			} else {
				dest[dest_pos++] = *src++;
			}
		} else {
			if (!*src)
				break;
			dest[dest_pos++] = *src++;
		}
	}
	dest[dest_pos] = '\0';
	return 0;
}

/* filename is supplied, path is returned */

int extract_packet(char *filename, char *path)
{
	int ret_value=EX_FAIL;
	
	if (!filename || !path) {
		return EX_FAIL;
	}
	
	if (getarchiver(filename) && (arc&&arc->ARC_EXTRACTFILEID[0])) {
		int child_pid;
		
		// Use safer mkdtemp instead of tmpnam
		strcpy(path, "/tmp/ddfv_XXXXXX");
		if (mkdtemp(path) == NULL) {
			goto ret;
		}
		
		char *curdir=getcwd(NULL, 0);
		if (!curdir) {
			goto ret;
		}
		
		if (chdir(path) != 0) {
			free(curdir);
			goto ret;
		}
		
		char *cmdline=new char[512];
		if (genstdiocmdline(cmdline, arc->ARC_EXTRACTFILEID, filename, 512) != 0) {
			// Command line generation failed
			delete [] cmdline;
			chdir(curdir);
			free(curdir);
			goto ret;
		}
		
		// Safe append with bounds checking
		size_t current_len = strlen(cmdline);
		if (current_len + 3 < 512) {  // " *" + null terminator
			strcat(cmdline, " *");
		} else {
			// Command line too long
			delete [] cmdline;
			chdir(curdir);
			free(curdir);
			goto ret;
		}
		
		switch (child_pid=fork()) {
		 case 0:
			exec_cmd(cmdline);
			_exit(1);  // Use _exit instead of kill(SIGKILL)
			break;
		 case -1:
			break;
		 default:
			dprintf("\e[2H\e[0;42;37;1m\e[K                        Unpacking... press ESC to abort");
			for (;;) {
				int status;
				if (waitpid(child_pid, &status, WNOHANG) > 0)
					break;
				if (HotKey(HOT_QUICK)==0x1b) {				       
					kill(child_pid, SIGTERM);
					waitpid(child_pid, NULL, 0);  // Wait for specific child
					ret_value=EX_INTR;
					break;
				}
			}
			break;
		}
		chdir(curdir);
		delete [] cmdline;
		free(curdir);  // Use free instead of delete for malloc'd memory
		
		if (ret_value!=EX_INTR)
			ret_value=EX_SUCCESS;
	}
	
	ret:
	return ret_value;
}
			
