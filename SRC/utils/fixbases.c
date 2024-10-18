#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <ddlib.h>
#include <ddcommon.h>

int main(int argc, char *argv[])
{
	struct DayDream_Conference *confs;
	struct DayDream_Conference *conf;
	struct DayDream_MsgBase *base;
		
	confs=(struct DayDream_Conference *)dd_getconfdata();
	
	if (!confs) exit (0);
	conf=confs;

	while(1)
	{
		struct DayDream_Message msg;
		int bcnt;
		
		if (conf->CONF_NUMBER==255) break;
		
		/* FIXME: Very suspicious code */
		base = (struct DayDream_MsgBase *) conf + 1;
		bcnt=conf->CONF_MSGBASES;

		for (bcnt = conf->CONF_MSGBASES; bcnt ;bcnt--, base++) {
			int basefd;
			struct DayDream_MsgPointers mp;
			
			basefd = ddmsg_open_base(conf->CONF_PATH, base->MSGBASE_NUMBER, O_RDONLY, 0666);
			if (basefd < 0) 
				continue;
			
			memset(&msg, '\0', sizeof(struct DayDream_Message));
			read(basefd,&msg,sizeof(struct DayDream_Message));
			mp.msp_low=msg.MSG_NUMBER;
			dd_lseek(basefd, -sizeof(struct DayDream_Message), SEEK_END);
			read(basefd,&msg,sizeof(struct DayDream_Message));
			mp.msp_high=msg.MSG_NUMBER;

			ddmsg_close_base(basefd);

			ddmsg_setptrs(conf->CONF_PATH, base->MSGBASE_NUMBER, &mp);
		}
		/* FIXME: Very suspicious code */
		conf = (struct DayDream_Conference *) base;
	}
	return 0;
}
