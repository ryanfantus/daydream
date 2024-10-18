/* Messagebase optimizer/maintainer for daydream */
#include <dd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include <ddlib.h>
#include <ddcommon.h>

static int massmode=0;
static int eallage=0;
static int confn, basen;

static struct DayDream_MainConfig maincfg;
static void usage(void);
static int optbase(struct DayDream_Conference *, struct DayDream_MsgBase *);

int main(int argc, char *argv[])
{
	int cfgfd;
	char buf[512];

	if (argc<2) {
		usage();
		return 0;
	}
	if (!strcasecmp(argv[1],"all")) {
		massmode++;
		if (argc==3) eallage=atoi(argv[2]);
	} else if (argc>2) {
		confn=atoi(argv[1]);
		basen=atoi(argv[2]);
		if (argc==4) eallage=atoi(argv[3]);
	} else {
		usage();
	}

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

	eallage=eallage*60*60*24;

	if (massmode) {
		struct DayDream_Conference *cd;
		struct DayDream_Conference *confs;
		struct DayDream_MsgBase *bd;

		confs=(struct DayDream_Conference *)dd_getconfdata();
	
		if (!confs) exit (0);
		cd=confs;

		for (;;) {
			int bcnt;
		
			if (cd->CONF_NUMBER==255) break;
		
			/* FIXME: Very suspicious code */
			bd = (struct DayDream_MsgBase *) cd + 1;
			bcnt=cd->CONF_MSGBASES;

			for(bcnt=cd->CONF_MSGBASES;bcnt;bcnt--,bd++)
				optbase(cd,bd);
			
			/* FIXME: Very suspicious code */
			cd = (struct DayDream_Conference *) bd;
		}
	} else {
		struct DayDream_Conference *cd;
		struct DayDream_MsgBase *bd;

		cd=dd_getconf(confn);
		bd=dd_getbase(confn,basen);
		if(!bd || !cd) {
			fprintf(stderr,"No such msgbase!\n");
			return 0;
		}
		optbase(cd,bd);
	}
	return 0;
}

static void usage(void)
{
	fprintf(stderr,"Usage:\n"
		"Single base mode: msgbaseopt [conf] [base] {max age in days for eall msgs}\n"
		"Mass mode:        msgbaseopt all {max age in days for eall msgs}\n");
}

static int optbase(struct DayDream_Conference *conf, 
	struct DayDream_MsgBase *base)
{
	char buf1[512];
	char buf2[512];
	char buf3[512];
	struct stat st;
	int i;
	int dc=0;
	int oldf, newf;
	struct DayDream_Message msg;
	struct DayDream_MsgPointers mp;
	struct DayDream_LRP lrp;
	
	printf("%s/%s: ",conf->CONF_NAME,base->MSGBASE_NAME);

	sprintf(buf1, "%s/messages/base%3.3d/msgbase.dat", conf->CONF_PATH, base->MSGBASE_NUMBER);
	oldf=open(buf1,O_RDONLY);
	
	if (oldf<0) {
		printf("can't open msgbase.\n");
		return 0;
	}
	
	lock_file(oldf, O_RDONLY);

	sprintf(buf2, "%s/messages/base%3.3d/msgbase.new", conf->CONF_PATH, base->MSGBASE_NUMBER);
	newf=lock_and_open(buf2,O_WRONLY|O_CREAT|O_TRUNC,0666);
	
	if (newf<0) {
		close(oldf);
		printf("can't open msgbase.\n");
		return 0;
	}

	/* remove deleted msgs */
	while(read(oldf,&msg,sizeof(struct DayDream_Message))) {
		sprintf(buf3, "%s/messages/base%3.3d/msg%5.5d", conf->CONF_PATH, base->MSGBASE_NUMBER, msg.MSG_NUMBER);
		if (stat(buf3,&st)==-1) {
			continue;
		}
		if (*msg.MSG_RECEIVER==-1 && eallage) {
			if (msg.MSG_CREATION < (time(0)-eallage)) *msg.MSG_RECEIVER=0;
		}
		write(newf,&msg,sizeof(struct DayDream_Message));
	}
	unlock_file(oldf);
	close(oldf);
	unlock_and_close(newf);
	unlink(buf1);
	rename(buf2,buf1);

	/* Recycle */
	if (stat(buf1,&st)==-1) {
		return 0;
	}
	
	if (st.st_size/sizeof(struct DayDream_Message) > base->MSGBASE_MSGLIMIT) {

		oldf=open(buf1,O_RDONLY);
		lock_file(oldf, O_RDONLY);
		newf=lock_and_open(buf2,O_WRONLY|O_CREAT|O_TRUNC,0666);

		for( (i=st.st_size/sizeof(struct DayDream_Message));i>base->MSGBASE_MSGLIMIT ; i--) {
			if (read(oldf,&msg,sizeof(struct DayDream_Message))==sizeof(struct DayDream_Message)) {
				sprintf(buf3, "%s/messages/base%3.3d/msg%5.5d", conf->CONF_PATH, base->MSGBASE_NUMBER, msg.MSG_NUMBER);
				unlink(buf3);
				if (*msg.MSG_ATTACH == 1) {
					sprintf(buf3, "%s/messages/base%3.3d/fa%5.5d", conf->CONF_PATH, base->MSGBASE_NUMBER, msg.MSG_NUMBER);
					deldir(buf3);
					unlink(buf3);
					sprintf(buf3, "%s/messages/base%3.3d/msf%5.5d", conf->CONF_PATH, base->MSGBASE_NUMBER, msg.MSG_NUMBER);
					unlink(buf3);
				}
				dc++;
			}
			

		}
		while(read(oldf,&msg,sizeof(struct DayDream_Message))) {
			write(newf,&msg,sizeof(struct DayDream_Message));
		}
		unlock_file(oldf);
		close(oldf);
		unlock_and_close(newf);
		unlink(buf1);
		rename(buf2,buf1);

	}
	
	oldf=open(buf1,O_RDONLY);
	
	if (oldf < 0) {
		return 0;
	}
			
	lock_file(oldf, O_RDONLY);
	
	if(read(oldf,&msg,sizeof(struct DayDream_Message)) > 0) {
		mp.msp_low=msg.MSG_NUMBER;
	} else {
		mp.msp_low=0;
	}
	
	dd_lseek(oldf,-(sizeof(struct DayDream_Message)),SEEK_END);
	
	if(read(oldf,&msg,sizeof(struct DayDream_Message)) > 0) {
		mp.msp_high=msg.MSG_NUMBER;
	} else {
		mp.msp_high=0;
	}

	unlock_file(oldf);
	close(oldf);

	sprintf(buf3, "%s/messages/base%3.3d/msgbase.ptr", conf->CONF_PATH, base->MSGBASE_NUMBER);
	newf=lock_and_open(buf3,O_WRONLY|O_TRUNC|O_CREAT,0666);
	
	if (newf < 0) {
		return dc;
	}
	
	write(newf,&mp,sizeof(struct DayDream_MsgPointers));
	
	unlock_and_close(newf);
	
	/* Fix lrps */

	sprintf(buf3, "%s/messages/base%3.3d/msgbase.lrp", conf->CONF_PATH, base->MSGBASE_NUMBER);
	
	oldf=open(buf3,O_RDWR);
	
	if (oldf < 0) {
		return 0;
	}
	
	lock_file(oldf, O_RDWR);
	
	while(read(oldf,&lrp,sizeof(struct DayDream_LRP))) {
		if (lrp.lrp_read > mp.msp_high) lrp.lrp_read=mp.msp_high;
		if (lrp.lrp_scan > mp.msp_high) lrp.lrp_scan=mp.msp_high;
		if (lrp.lrp_read < mp.msp_low) lrp.lrp_read = mp.msp_low;
		if (lrp.lrp_scan < mp.msp_low) lrp.lrp_scan = mp.msp_low;
		lseek(oldf,-sizeof(struct DayDream_LRP),SEEK_CUR);
		write(oldf,&lrp,sizeof(struct DayDream_LRP));
	}
	
	unlock_file(oldf);
	close(oldf);
	
	printf("%d deleted\n",dc);
	return dc;
}
