#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include <dd.h>

static int sock;
static int ponce = 0;
FILE *outfd;

static void killsock(int);

static int ispid(pid_t pid)
{
	return kill(pid, 0) != -1;
}

int main(int argc, char *argv[])
{
	struct DayDream_PageMsg pm;
	int beep=0;
	char output[1024];
	char sample[1024];
	int i;
	struct sockaddr_un name;
	char buf[1024];
	umask(0);
	
	output[0]=sample[0]=0;
	
	if (argc < 2) {
		printf("DayDream BBS SysOp yelling daemon.\n\nUsage:\n -b                - use beeping.\n -o [file/tty]     - message output. \n -s [sample]       - AU-file to play\n -1         - play sample only ONCE\n\n");
		exit(0);
	}
	
	for (i=0;i!=argc;i++)
	{
		if (!strcasecmp(argv[i],"-b")) {
			beep=1;
			break;
		}
	}

	for (i=0;i!=argc;i++)
	{
		if (!strcasecmp(argv[i],"-o")) {
			strcpy(output,argv[i+1]);
			break;
		}
	}

	for (i=0;i!=argc;i++)
	{
		if (!strcasecmp(argv[i],"-s")) {
			strcpy(sample,argv[i+1]);
			break;
		}
	}

	for (i=0;i!=argc;i++)
	{
		if (!strcasecmp(argv[i],"-1")) {
			ponce=1;
			break;
		}
	}

	if (!output[0]) {
		fprintf(stderr,"No output specified -- exiting!\n\n");
		exit(-1);
	}

	outfd=fopen(YELLDLOCK,"r");
	if (outfd) {
		fgets(buf,1024,outfd);
		fclose(outfd);
		if (ispid(atoi(buf))) {
			fprintf(stderr,"Yelld already active!\n");
			exit(-2);
		}
	}

	signal(SIGTERM,killsock);
	signal(SIGHUP,killsock);
	signal(SIGINT,killsock);

	if (fork()==0)
	{
		int  samplefd;
		char *samplem;
		struct stat st;

		if (sample[0]) {
			samplefd=open(sample,O_RDONLY);
			if (samplefd==-1) {
				perror("Can't open sample!\n\n");
				exit(-4);
			}

			fstat(samplefd,&st);
			samplem=(char *)malloc((unsigned int)st.st_size);
			read(samplefd,samplem,(unsigned int)st.st_size);
			close(samplefd);
		
			samplefd=open("/dev/audio",O_WRONLY);
			if (samplefd==-1) {
				perror("Can't open /dev/audio!\n\n");
				exit(-5);
			}
			close(samplefd);
		}
		outfd=fopen(YELLDLOCK,"w");
		if (!outfd) {
			perror("Can't create lockfile");
			exit(-6);
		}
		fprintf(outfd,"%d\n",getpid());
		fclose(outfd);

		outfd=fopen(output,"a");
		if (!outfd) {
			perror("Can't open output file/tty!\n\n");
			exit(-3);
		}

		sock=socket(AF_UNIX, SOCK_DGRAM, 0);
		if (sock < 1) {
			perror("error opening socket!\n\n");
			exit(-7);
		}

		unlink(YELLDSOCK);
		strcpy(name.sun_path, YELLDSOCK);
		name.sun_family=AF_UNIX;
		if (bind(sock,(struct sockaddr *)&name, sizeof(struct sockaddr_un))) {
			perror("Can't bind socket!\n\n");
			exit(0);
		}

		puts("\nyelld active!\n\n");
		
		while (1)
		{
			read(sock,&pm,sizeof(struct DayDream_PageMsg));
			if (pm.pm_cmd==1) {
				if (beep) {
     			                system("/usr/bin/beep -f 100");
					system("/usr/bin/beep -f 500");
					system("/usr/bin/beep -f 1000");
					sleep(2);
				}
				if (sample[0] && ponce < 2) {
					samplefd=open("/dev/audio",O_WRONLY);
					if (samplefd!=-1) {
					write(samplefd,samplem,(unsigned int)st.st_size);
					if (ponce==1) ponce=2;
					close(samplefd);
					}
				}
			} else if (pm.pm_cmd==2) {
				fputs(pm.pm_string,outfd);
				fflush(outfd);
			}
		
		}	
	}
	return 0;
}

static void killsock(int sig)
{
	sig++; /* Make the compiler happy */
	unlink(YELLDSOCK);
	unlink(YELLDLOCK);
	fclose(outfd);
	exit(0);
}
