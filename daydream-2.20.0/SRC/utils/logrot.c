/* 
   DayDream BBS binary log file rotator, Code by Hydra.
   */
#include <dd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <ddcommon.h>

static struct DayDream_MainConfig maincfg;

static void rotul(int maxents, const char *logn)
{
	char buf[1024];
	char buf2[1024];
	struct stat st;
	int fd1,  fd2;
	struct DD_UploadLog foo;

        sprintf(buf,"%s/logfiles/%s",getenv("DAYDREAM"),logn);
	if (stat(buf,&st) == -1) {
		perror("Couldn't open logfile");
		return;
	}
	if (maxents < (st.st_size/sizeof(struct DD_UploadLog))) {
		fd1=open(buf,O_RDONLY);
		sprintf(buf2,"%s.new",buf);
		fd2=open(buf2,O_WRONLY|O_CREAT|O_TRUNC,0664);
		if (fd1<0 || fd2<0) {
			perror("Couldn't open logfile!");
			exit(0);
		}
		dd_lseek(fd1,-maxents*sizeof(struct DD_UploadLog),SEEK_END);
		while(read(fd1,&foo,sizeof(struct DD_UploadLog))) 
			write(fd2,&foo,sizeof(struct DD_UploadLog));
		close(fd1);
		close(fd2);
		unlink(buf);
		rename(buf2,buf);
	}
}

static void rotcl(int maxents, const char *logn)
{
	char buf[1024];
	char buf2[1024];
	struct stat st;
	int fd1,  fd2;
	struct callerslog foo;

        sprintf(buf, "%s/logfiles/%s", getenv("DAYDREAM"), logn);
	if (stat(buf, &st) == -1) {
		fputs("cannot stat logfile", stderr);
		return;
	}
	if (maxents < (st.st_size / sizeof(struct callerslog))) {
		fd1 = open(buf,O_RDONLY);
		sprintf(buf2, "%s.new", buf);
		fd2 = open(buf2, O_WRONLY|O_CREAT|O_TRUNC, 0664);
		if (fd1 < 0 || fd2 < 0) {
			perror("Couldn't open logfile!");
			exit(0);
		}
		dd_lseek(fd1,-maxents*sizeof(struct callerslog),SEEK_END);
		while(read(fd1,&foo,sizeof(struct callerslog))) 
			write(fd2,&foo,sizeof(struct callerslog));
		close(fd1);
		close(fd2);
		unlink(buf);
		rename(buf2,buf);
	}
}

static void showhelp(void)
{
/*
DayDream BBS Logfile rotator -- Code by Hydra

Usage:
logrot -u [max upload log entries] -d [max download log entries]
       -c [max callerslog entries]

*/
	printf("DayDream BBS Logfile rotator -- Code by Hydra\n\n"
	       "Usage:\n"
	       "logrot -u [max upload log entries] -d [max download log entries]\n"
	       "       -c [max callerslog entries]\n");
	exit(0);
}

int main(int argc, char **argv)
{
	char *cp;
	int cfgfd;
	char buf[1024];

	if (argc < 2) showhelp();

        sprintf(buf,"%s/data/daydream.dat",getenv("DAYDREAM"));
        cfgfd=open(buf,O_RDONLY);
        if (cfgfd < 0) {
                perror("Can't open DD datafiles");
                exit(0);
        }
        read(cfgfd,&maincfg,sizeof(struct DayDream_MainConfig));
        close(cfgfd);

        setgid(maincfg.CFG_BBSGID);
        setegid(maincfg.CFG_BBSGID);
        setuid(maincfg.CFG_BBSUID);
        seteuid(maincfg.CFG_BBSUID);

	while (--argc) {
	cp = *++argv;
		if (*cp == '-') {
			while( *++cp) {
				switch(*cp) {
				case 'u':
					if (--argc < 1) {
						showhelp();
						exit(1);
					}
					rotul(atoi(*++argv),"uploadlog.dat");
					break;
				case 'd':
					if (--argc < 1) {
						showhelp();
						exit(1);
					}
					rotul(atoi(*++argv),"downloadlog.dat");
					break;
				case 'c':
					if (--argc < 1) {
						showhelp();
						exit(1);
					}
					rotcl(atoi(*++argv),"callerslog.dat");
					break;
				}
				
			}
		}	
	}
	return 0;
}
