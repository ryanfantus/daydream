#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <ddlib.h>
#include <dd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <config.h>
#include <ddcommon.h>

static void showhelp(void);

static int dconf;
static int dbase;
static int msgfd;
static char *receiver="\0";
static char *recv_addr="0:0/0";
static char *author="Robowriter";
static char *subject="--unknown--";
static char *mfile=0;
static int private=0;
static FILE *inf;
static FILE *outf;

static struct DayDream_Conference *confd;
static struct DayDream_MsgBase *based;

int main(int argc, char *argv[])
{
	char *cp;
	struct DayDream_Message ddm;
	struct DayDream_MsgPointers ptrs;
	
	while (--argc) {
	cp = *++argv;
		if (*cp == '-') {
			while( *++cp) {
				switch(*cp) {
				case 'p':
					private=1; break;
				case 'a':
					if (--argc < 1) {
						showhelp();
						exit(1);
					}
					recv_addr=*++argv;
					break;
				case 'r':
					if (--argc < 1) {
						showhelp();
						exit(1);
					}
					receiver=*++argv;
					break;
				case 'f':
					if (--argc < 1) {
						showhelp();
						exit(1);
					}
					author=*++argv;
					break;
				case 's':
					if (--argc < 1) {
						showhelp();
						exit(1);
					}
					subject=*++argv;
					break;
				case 'c':
					if (--argc < 1) {
						showhelp();
						exit(1);
					}
					dconf=atoi(*++argv);
					break;
				case 'b':
					if (--argc < 1) {
						showhelp();
						exit(1);
					}
					dbase=atoi(*++argv);
					break;
				}
				
			}
		} else {
			mfile=cp;
		}	
	}
	if (!dconf || !dbase || !mfile) {
		showhelp();
		exit(1);
	}
	confd=dd_getconf(dconf);
	if (!confd) {
		printf("Can't get conference!\n");
		exit(0);
	}
	based=dd_getbase(dconf,dbase);
	if (!based) {
		printf("Can't get messagebase!\n");
		exit(0);
	}
	inf=fopen(mfile,"r");
	if (!inf) {
		printf("Can't open message file!\n");
		exit(0);
	}
	memset(&ddm,0,sizeof(struct DayDream_Message));
	if (private) ddm.MSG_FLAGS |= (1L<<0);
	strncpy(ddm.MSG_AUTHOR,author,25);
	if (!strcasecmp(receiver,"All")) {
		*ddm.MSG_RECEIVER=0;
	} else if (!strcasecmp(receiver,"EAll")) {
		*ddm.MSG_RECEIVER=255;
	} else {
		strncpy(ddm.MSG_RECEIVER,receiver,25);
	}
	strncpy(ddm.MSG_SUBJECT,subject,25);
	ddm.MSG_CREATION=time(0);

	ddmsg_getptrs(confd->CONF_PATH, based->MSGBASE_NUMBER, &ptrs);
	ptrs.msp_high++;

	ddm.MSG_NUMBER=ptrs.msp_high;

	if (!ddmsg_setptrs(confd->CONF_PATH, based->MSGBASE_NUMBER, &ptrs)) {
		char outbuf[4096];

		msgfd = ddmsg_open_base(confd->CONF_PATH, based->MSGBASE_NUMBER, O_RDWR|O_CREAT, 0664);

		if (msgfd < 0) {
			printf("*FATAL* error.. Can't write message!\n\n");
			exit(1);
		}

		lseek(msgfd,0,SEEK_END);
		if (toupper(based->MSGBASE_FN_FLAGS)=='E' || toupper(based->MSGBASE_FN_FLAGS) == 'N') {
			ddm.MSG_FN_ORIG_ZONE=based->MSGBASE_FN_ZONE;
			ddm.MSG_FN_ORIG_NET=based->MSGBASE_FN_NET;
			ddm.MSG_FN_ORIG_NODE=based->MSGBASE_FN_NODE;
			ddm.MSG_FN_ORIG_POINT=based->MSGBASE_FN_POINT;
			ddm.MSG_FLAGS |= (1L<<2);
		}
		if (toupper(based->MSGBASE_FN_FLAGS)=='N') {
			sscanf(recv_addr, "%d:%d/%d.%d", &ddm.MSG_FN_DEST_ZONE, 
			&ddm.MSG_FN_DEST_NET, &ddm.MSG_FN_DEST_NODE, &ddm.MSG_FN_DEST_POINT);
		}

		write(msgfd,&ddm,sizeof(struct DayDream_Message));
		ddmsg_close_base(msgfd);
		
		msgfd = ddmsg_open_msg(confd->CONF_PATH, based->MSGBASE_NUMBER, ddm.MSG_NUMBER, O_WRONLY | O_CREAT, 0664);

		outf=fdopen(msgfd,"w");
		if (!outf) {
			printf("*FATAL* error.. Can't write message!\n\n");
			exit(1);
		}
		if (toupper(based->MSGBASE_FN_FLAGS)=='E') {
			char ub[128];
			int uq;
		
			strcpy(ub,based->MSGBASE_FN_TAG);
			strupr(ub);
			fprintf(outf,"AREA:%s\n",ub);
			if ((uq=dd_getfidounique())) {
				fprintf(outf,"\001MSGID: %d:%d/%d.%d %8.8x\n",based->MSGBASE_FN_ZONE,based->MSGBASE_FN_NET,based->MSGBASE_FN_NODE,based->MSGBASE_FN_POINT,uq);
			}
		}
		else if(toupper(based->MSGBASE_FN_FLAGS) == 'N') {
			fprintf(outf, "\1INTL %d:%d/%d.%d %d:%d/%d.%d\n", ddm.MSG_FN_DEST_ZONE, 
			ddm.MSG_FN_DEST_NET, ddm.MSG_FN_DEST_NODE, ddm.MSG_FN_DEST_POINT, 
			ddm.MSG_FN_ORIG_ZONE, ddm.MSG_FN_ORIG_NET, ddm.MSG_FN_ORIG_NODE, ddm.MSG_FN_ORIG_POINT);

			if(ddm.MSG_FN_DEST_POINT > 0) {
				fprintf(outf, "\1TOPT %d\n", ddm.MSG_FN_DEST_POINT);
			}
			if(ddm.MSG_FN_ORIG_POINT > 0) {
				fprintf(outf, "\1FMPT %d\n", ddm.MSG_FN_ORIG_POINT);
			}
		}

		while(fgets(outbuf,4096,inf)) {
			fputs(outbuf,outf);
		}
		if (toupper(based->MSGBASE_FN_FLAGS)=='E') {
			fprintf(outf,"\n--- DayDream Robowriter/UNIX (" UNAME ") %s\n * Origin: %s (%d:%d/%d)\nSEEN-BY: %d/%d\n",versionstring,based->MSGBASE_FN_ORIGIN,based->MSGBASE_FN_ZONE,based->MSGBASE_FN_NET,based->MSGBASE_FN_NODE,based->MSGBASE_FN_NET,based->MSGBASE_FN_NODE);
		} else if(toupper(based->MSGBASE_FN_FLAGS) == 'N') {
			fprintf(outf, "\n--- DayDream Robowriter/UNIX (" UNAME ") %s\n", versionstring);
		}
		fclose(outf);
		ddmsg_close_msg(msgfd);
	}
	fclose(inf);
	return 0;
}

static void showhelp(void)
{
	printf("DD-Robowriter v1.0 - written by Antti Häyrynen\n\n");
	printf("Usage: robowriter [-f from] [-r receiver] [-a recv_addr] [-s subject] [-c conference]\n");
	printf("                  [-b base] [-p] file...\n\n -p == private\n");
	printf(" Note that you must supply at least -c, -b and file\n");
}


