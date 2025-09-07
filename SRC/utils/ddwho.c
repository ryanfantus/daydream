#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <dd.h>

static struct DayDream_Multinode *mn;
static struct DayDream_MainConfig maincfg;
static size_t mn_size;

static const char *whlinestr = "%-2.2d %-20.20s %-25.25s %-20.20s %s\n";

static int isnode(int, struct DayDream_NodeInfo *);
static void shownodes(void);
static void who_show(struct DayDream_Multinode *, int);

int main(int argc, char *argv[])
{
	char *env;
	char bufa[1024];
	struct stat st;
	int fd;

	env=getenv("DAYDREAM");
	
	sprintf(bufa,"%s/data/multinode.dat",env);
	if (stat(bufa,&st)<0) {
		perror("open $DAYDREAM/data/multinode.dat");
		return -1;
	}
	mn=malloc(st.st_size+2);
	memset(mn,0,st.st_size+2);
	mn_size = st.st_size;
	fd=open(bufa,O_RDONLY);
	if (fd < 0) {
		perror("open $DAYDREAM/data/multinode.dat");
		return -1;
	}
	read(fd,mn,st.st_size);
	close(fd);
	
	sprintf(bufa,"%s/data/daydream.dat",env);
	fd=open(bufa,O_RDONLY);
	if (fd < 0) {
		perror("open $DAYDREAM/data/daydream.dat");
		return -1;
	}
	read(fd,&maincfg,sizeof(struct DayDream_MainConfig));
	close(fd);
	
	shownodes();
	return 0;
}

static void shownodes(void)
{
	struct DayDream_Multinode *cn;

	cn=mn;
	
	printf("N  Account Name         Org. / Location           Activity             Bps\n-----------------------------------------------------------------------------\n");

	/* Process all nodes in the configuration file, not just until first zero.
	 * The nodes array is terminated by padding at the end, so we continue
	 * until we've processed all entries based on the file size.
	 */
	while ((char *)cn < (char *)mn + mn_size)
	{
		/* Skip completely zero entries that might be padding */
		if (*(unsigned char *)cn == 0)
			break;
			
		if (cn->MULTI_NODE == 254) {
			int j;
			int i=maincfg.CFG_TELNET1ST;
			j=maincfg.CFG_TELNETMAX;
			
			while(j) {
				j--;
				who_show(cn,i);
				i++;
			}
		} else if (cn->MULTI_NODE == 253) {
			int j;
			int i=maincfg.CFG_LOCAL1ST;
			j=maincfg.CFG_LOCALMAX;
			
			while(j) {
				j--;
				who_show(cn,i);
				i++;
			}
		} else if (cn->MULTI_NODE != 252 && cn->MULTI_NODE != 0) {
			who_show(cn,cn->MULTI_NODE);
		}
		cn++;
	}
	printf("-----------------------------------------------------------------------------\n\n");
}

static void who_show(struct DayDream_Multinode *cn, int num)
{
	struct DayDream_NodeInfo myn;
	const char *usern, *orgn, *activity;
	char dabps[30];
	
	int nodefd;
	struct userbase nuser;
	char blabuf[500];       
	int showent=1;
		
	memset(&myn,0,sizeof(struct DayDream_NodeInfo));
	
	if (isnode(num,&myn)) {
		if (cn->MULTI_OTHERFLAGS & (1L<<4)) {
			showent=0;
		} else {
			if (myn.ddn_userslot > -1) {
				sprintf(blabuf,"%s/data/userbase.dat",getenv("DAYDREAM"));
				nodefd=open(blabuf,O_RDONLY);
				lseek(nodefd,myn.ddn_userslot*sizeof(struct userbase),SEEK_SET);
				read(nodefd,&nuser,sizeof(struct userbase));
				close(nodefd);
				if (maincfg.CFG_FLAGS & (1L<<1)) usern=nuser.user_handle; else usern=nuser.user_realname;
				if (maincfg.CFG_FLAGS & (1L<<2)) orgn=nuser.user_organization; else orgn=nuser.user_zipcity;
				activity=myn.ddn_activity;
				sprintf(dabps,"%d",myn.ddn_bpsrate);
			} else {
				usern=" "; orgn=" ";
				activity=myn.ddn_activity;
				sprintf(dabps,"%d",myn.ddn_bpsrate);
			}
		} 
	} else {
		if (cn->MULTI_OTHERFLAGS & (1L<<3)) showent=0;
		else {
			usern=" "; orgn=" "; activity="Waiting for a call...";
			sprintf(dabps,"%d",0);
		}
	}
	if (showent) {
		/* Determine connection type based on node number range */
		if (num >= maincfg.CFG_TELNET1ST && num < maincfg.CFG_TELNET1ST + maincfg.CFG_TELNETMAX) {
			/* Node is in telnet range */
			if (isnode(num,&myn) && myn.ddn_bpsrate > 0) {
				sprintf(dabps,"%d",myn.ddn_bpsrate);
			} else {
				strcpy(dabps,"TELNET");
			}
		} else if (num >= maincfg.CFG_LOCAL1ST && num < maincfg.CFG_LOCAL1ST + maincfg.CFG_LOCALMAX) {
			/* Node is in local range */
			strcpy(dabps,"LOCAL");
		} else {
			/* Fall back to original logic for other node types */
			if (isnode(num,&myn) && myn.ddn_bpsrate == 0) {
				/* Zero bps rate typically indicates telnet connection */
				strcpy(dabps,"TELNET");
			} else {
				switch (cn->MULTI_TTYTYPE)
				{
					case 1:
						strcpy(dabps,"LOCAL");
					break;
					case 2:
						if (myn.ddn_flags & (1L<<1)) {
							sprintf(dabps,"%d",myn.ddn_bpsrate);
						} else {
							strcpy(dabps,"TELNET");
						}
					break;
					default:
						if (myn.ddn_bpsrate > 0) {
							sprintf(dabps,"%d",myn.ddn_bpsrate);
						} else {
							strcpy(dabps,"TELNET");
						}
					break;
				}
			}
		}
		printf(whlinestr,num,usern,orgn,activity,dabps);
	}
	return;
}

static int ispid(pid_t pid)
{
	if (kill(pid, 0) == 0)
		return 1;
	if (errno == EPERM)
		return 1;
	return 0;
}

static int isnode(int nod, struct DayDream_NodeInfo *ndnfo)
{
	char infoname[1024];
	int nodefd;
		
	sprintf(infoname, "%s/nodeinfo%d.data", DDTMP, nod);
	nodefd=open(infoname,O_RDONLY);
	if (nodefd!=-1) {
		read(nodefd,ndnfo,sizeof(struct DayDream_NodeInfo));
		close(nodefd);
		if(ispid(ndnfo->ddn_pid)) return 1;
	}
	return 0;
}
