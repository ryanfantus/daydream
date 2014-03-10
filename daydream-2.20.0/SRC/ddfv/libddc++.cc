#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <libddc++.h>
#include <ddcommon.h>

struct dif *d;
struct DayDream_Archiver *arcs;
struct DayDream_Archiver *arc;

void die(void)
{
	dd_close(d);
}

void GetStrVal(char *buf, size_t bufsize, int type)
{
	dd_getstrlval(d, buf, bufsize, type);
}

int GetIntVal(int w)
{
	return dd_getintval(d, w);
}

int FindFile(char *file, char *de)
{
	char buf1[1024];
	char buf2[1024];
	int fd1;
	FILE *plist;
	char *s;
	DIR *dh;
	struct dirent *dent;
	struct DayDream_Conference *co;
	
	co=GetConf();
	
	*de=0;
	s=file;
	while(*s) {
		if (*s=='/') return 0;
		s++;
	}	
	sprintf(buf1, "%s/data/paths.dat", co->CONF_PATH);
	if ((plist=fopen(buf1,"r"))) {
		while(fgetsnolf(buf1,512,plist))
		{
			sprintf(buf2,"%s%s",buf1,file);
			if (*buf1 && ((fd1=open(buf2,O_RDONLY)) > -1)) {
				fclose(plist);
				close(fd1);

				strcpy(de,buf2);
				return 1;
			}
		}
		fclose(plist);
	}

	sprintf(buf1, "%s/data/paths.dat", co->CONF_PATH);
	if ((plist=fopen(buf1,"r"))) {
		while(fgetsnolf(buf1,512,plist))
		{
			if (*buf1 && (dh=opendir(buf1)))
			{
				while((dent=readdir(dh)))
				{
					if (!strcmp(dent->d_name,".") || (!strcmp(dent->d_name,".."))) continue;
					if (!strcasecmp(dent->d_name,file)) {
						sprintf(de,"%s%s",buf1,dent->d_name);
						break;
					}
				}
				closedir(dh);
			}
		}
		fclose(plist);
	}
	if (*de) return 1;
	return 0;
}

int HotKey(int i)
{
	return dd_hotkey(d, i);
}

DayDream_Conference *GetConf(int n)
{
	if (n==-1) 
		n=dd_getintval(d, SYS_CONF);
	return dd_getconf(n);
}

char *fgetsnolf(char *buffer, int maxlen, FILE *fp)
{
	if (fgets(buffer, maxlen, fp)==NULL)
		return NULL;
	for (char *p=buffer; *p; p++)
		if (*p==0x0d||*p==0x0a) {
			*p=0;
			break;
		}
	return buffer;
}

void loadarchivers()
{
	unsigned char *s;
	struct stat fib;
	int datafd=open("data/archivers.dat", O_RDONLY);
	if (datafd==-1) {
		printf("Can't open archivers.dat. Aiee!\n");
		exit(0);
	}
	fstat(datafd, &fib);
	arcs=(struct DayDream_Archiver *)malloc(fib.st_size+2);
	read(datafd,arcs,fib.st_size);
	close(datafd);
	s=(unsigned char *)arcs;
	s[fib.st_size]=255;
}

int getarchiver(char *file)
{
	arc=arcs;
	
	while(arc->ARC_FLAGS!=255) {
		if (wildcmp(file,arc->ARC_PATTERN)) {
			return 1;
		}
		arc++;
	}
	arc=0;
	return 0;
}

int wildcmp(char *nam, char *pat)
{
	char *p;              /* Thu Jan 16 14:50:30 1992 */

	for (;;)
	{
		if (tolower(*nam) == tolower(*pat)) {
			if(*nam++ == '\0')  return(1);
			pat++;
		} else if (*pat == '?' && *nam != 0) {
		    	nam++;
		    	pat++;
		} else	break;
	}

	if (*pat != '*') return(0);

	while (*pat == '*') {
		if (*++pat == '\0')  return(1);
	}

	for (p=nam+strlen(nam)-1;p>=nam;p--) {
		if (tolower(*p) == tolower(*pat))
			if (wildcmp(p,pat) == 1) return(1);
	}
	return(0);
}


int main(int argc, char *argv[])
{
	if (argc==1) {
		printf("DayDream required.\n");
		exit(1);
	}
	if ((d=dd_initdoor(argv[1]))==NULL) {
		printf("Can't find socket.\n");
		exit(1);
	}
	atexit(die);
	DoorCode();
	return 0;
}

void dprintf(const char *fmt, ...)
{
	char buffer[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, 1024, fmt, args);
	va_end(args);
	dd_sendstring(d, buffer);
}
		      
