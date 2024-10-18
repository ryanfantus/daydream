#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#include <ddlib.h>
#include <ddcommon.h>

const char* format = "                 \x1b[1;32m %03d \x1b[0;33m ..... \x1b[0;35m %s\n";

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
		char buf[128];
		FILE* fp;
		DIR* dir;
		
		if (conf->CONF_NUMBER==255) break;

		sprintf(buf, "%s/display/iso", conf->CONF_PATH);
		dir = opendir(buf);
		if(dir == NULL) {
			mkdir(buf, 0775);
		} 
		else {
			closedir(dir);
		}
	
		sprintf(buf, "%s/display/iso/messagebases.gfx", conf->CONF_PATH);
		fp = fopen(buf, "w");

		if(fp != NULL) {
		
			/* FIXME: Very suspicious code */
			base = (struct DayDream_MsgBase *) conf + 1;
			bcnt=conf->CONF_MSGBASES;

			for (bcnt = conf->CONF_MSGBASES; bcnt ;bcnt--, base++) {
				fprintf(fp, format, base->MSGBASE_NUMBER, base->MSGBASE_NAME);

			}
			fclose(fp);
		}
		else {
			printf("Couldn't write %s\n", buf);
			bcnt = 	conf->CONF_MSGBASES;
			base = (struct DayDream_MsgBase *) conf + bcnt + 1;
		}

		/* FIXME: Very suspicious code */
		conf = (struct DayDream_Conference *) base;

	}
	return 0;
}
