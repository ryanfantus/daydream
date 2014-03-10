/*
 * DayDream filelist cleaning tool. Code by Hydra.
 */

#include <dd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ddlib.h>
#include <stdlib.h>
#include <string.h>

static int killf=0;
static int quick=0;

static int confn;
static int basen;

static char buf[1024];

static struct DayDream_Conference *mconf;

static char *fgetsnolf(char *, int, FILE *) __attr_bounded__ (__string__, 1, 2);
static int findfile(char *, char *);

static void usage(void);

int main(int argc, char *argv[]) 
{
	char *s, *cp;
	FILE *inf, *outf;
	int filen=0;
	int delc=0;
	char buf2[1024];
	int writeon=1;
	char tmpname[256];
	
	if (argc < 3) {
		usage();
		exit(1);
	}
	confn=atoi(argv[1]);
	basen=atoi(argv[2]);

	while (--argc) {
		cp = *++argv;
		if (*cp == '-') {
			while( *++cp) {
				switch(*cp) {
				 case 'k':
					killf=1; break;
				 case 'q':
					quick=1;  break;
				}
				
			}
		}	
	}

	if (argc==4 && !strcasecmp(argv[3],"-k")) killf=1;
	
	mconf=dd_getconf(confn);
	if (!mconf) {
		printf("Unknown conference.\n");
		exit(2);
	}
	
	sprintf(buf, "%s/data/directory.%3.3d", mconf->CONF_PATH, basen);
	inf=fopen(buf,"r");
	if (!inf) {
		printf("Can't open filelist.\n");
		exit(3);
	}
	s=tmpnam(tmpname);
	outf=fopen(s,"w");
	
	while(fgets(buf,1024,inf)) {
		if (*buf!=' ') {
			char fn[80];
			char *s, *t;
			char kala[1024];
			int doh=1;
			writeon=1;

			if (mconf->CONF_ATTRIBUTES & (1L<<3)) {
				if (!killf && buf[36]=='D') doh=0;
			} else {
				if (!killf && buf[14]=='D') doh=0;
			}

			sprintf(buf2,"\rFile number: %d",++filen);
			write(STDOUT_FILENO,buf2,strlen(buf2));
			
			s=buf;
			t=fn;
			while(*s!=' ') *t++=*s++;
			*t=0;
			if (doh && !findfile(fn,kala)) {
				sprintf(buf2,"\rFile missing: %s          \n",fn);
				write(STDOUT_FILENO,buf2,strlen(buf2));
				delc++;
				if (killf) {
					writeon=0;
				} else {
					if (mconf->CONF_ATTRIBUTES & (1L<<3)) {
						buf[36]='D';
					} else {
						buf[14]='D';
					}
		
				}
			}
		}
		if (writeon) {
			fputs(buf,outf);
		}
     	}
	fclose(inf);
	fclose(outf);

       	inf=fopen(tmpname,"r");
	if (!inf) {
		printf("FATAL ERROR.\n");
		exit(5);
	}
	
	sprintf(buf, "%s/data/directory.%3.3d", mconf->CONF_PATH, basen);
	outf=fopen(buf,"w");
	if (!outf) {
		printf("FATAL ERROR.\n");
		exit(4);
	}

	while(fgets(buf,1024,inf)) fputs(buf,outf);
	fclose(inf); fclose(outf);
	unlink(tmpname);
	
	printf("\rDone. %d files processed, %d deleted from the list.\n",filen,delc);
	return 0;
}

static void usage()
{
	printf("DayDream filelist cleaner. Usage: listclean [conf] [base] [-kq].\n"
	       "  -k to remove files completely from the list.\n"
	       "  -q to disable case sensitive search (BE CAREFUL!).\n");
}

static int findfile(char *file, char *de)
{
	char buf1[1024];
	char buf2[1024];
	int fd1;
	FILE *plist;
	char *s;
	DIR *dh;
	struct dirent *dent;
	
	*de=0;
	s=file;
	while(*s) {
		if (*s=='/') return 0;
		s++;
	}	
	sprintf(buf1, "%s/data/paths.dat", mconf->CONF_PATH);
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

	sprintf(buf1, "%s/data/paths.dat", mconf->CONF_PATH);
	if (!quick && (plist=fopen(buf1,"r"))) {
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

static char *fgetsnolf(char *buf, int n, FILE *fh)
{
	char *hih;
	char *s;
	
	hih=fgets(buf,n,fh);
	if (!hih) return 0;
	s=buf;
	while (*s)
	{
		if (*s==13 || *s==10) {
			*s=0;
			break;
		}
		s++;
	}
	return hih;
}
