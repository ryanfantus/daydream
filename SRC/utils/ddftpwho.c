#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <dd.h>

static struct DayDream_MainConfig maincfg;

static void showftp(void);

static int ispid(pid_t pid)
{
	if (kill(pid, 0) == 0)
		return 1;
	if (errno == EPERM)
		return 1;
	return 0;
}

int main(int argc, char *argv[])
{
	char *env;
	char bufa[1024];
	int fd;

	env=getenv("DAYDREAM");
	
	sprintf(bufa,"%s/data/daydream.dat",env);
	fd=open(bufa,O_RDONLY);
	if (fd < 0) {
		perror("open $DAYDREAM/data/daydream.dat");
		return -1;
	}
	read(fd,&maincfg,sizeof(struct DayDream_MainConfig));
	close(fd);
	
	showftp();
	return 0;
}

static void showftp(void)
{
	int i;
	int fd;
	char buf[1024];
	char acti[512];
	struct ftpinfo mi;
	struct userbase nuser;
	char *usern, *orgn;
	int cps;

	printf("Account Name            Org. / Location           Activity                 CPS\n------------------------------------------------------------------------------\n");

	for(i=0;i<maincfg.CFG_MAXFTPUSERS;i++) {
		sprintf(buf, "%s/ftpinfo%d.dat", DDTMP, i + 1);
		fd=open(buf,O_RDONLY);
		if (fd<0) continue;
		read(fd,&mi,sizeof(struct ftpinfo));
		close(fd);
		if (!ispid(mi.pid)) continue;
		
		sprintf(buf,"%s/data/userbase.dat",getenv("DAYDREAM"));
		fd=open(buf,O_RDONLY);
		lseek(fd,mi.userid*sizeof(struct userbase),SEEK_SET);
		read(fd,&nuser,sizeof(struct userbase));
		close(fd);

		if (maincfg.CFG_FLAGS & (1L<<1)) usern=nuser.user_handle; else usern=nuser.user_realname;
		if (maincfg.CFG_FLAGS & (1L<<2)) orgn=nuser.user_organization; else orgn=nuser.user_zipcity;
		if (mi.mode==1) {
			cps=mi.cps;
			sprintf(acti,"UL: %s",mi.filename);
		} else if (mi.mode==2) {
			cps=mi.cps;
			sprintf(acti,"DL: %s",mi.filename);
		} else {
			cps=0;
			strcpy(acti,"Idle");
		}
		printf("%-23.23s %-25.25s %-20.20s %7d\n",
			usern,orgn,acti,cps);
		if (mi.mode) {
			if (mi.filesize==-1) {
				strcpy(acti,"????????");
			} else {
				sprintf(acti,"%d",mi.filesize);
			}
			printf("%-23.23s %-25.25s (%d/%s)\n"," "," ",mi.transferred,acti);
		}
	}
	printf("------------------------------------------------------------------------------\n");

}


/*
design:

Hydra                   HiRMU                   UL: warezelite.tgz      1234567
                                                (12345678/????????)
Marko                   Jumala.                 Idle                          0
Väinö                   The 31337 godz          DL: porno.tar.gz          54445
                                                (12345/67874324)

						*/
