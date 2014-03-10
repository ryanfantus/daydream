#include <ddlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static struct dif *d;

int main(int argc, char *argv[])
{
	struct DayDream_LRP lp;
	char buf[1024];

	if (argc == 1) {
		printf("This program requires MS Windows!\n");
		exit(1);
	}
	d = dd_initdoor(argv[1]);
	if (d == 0) {
		printf("Couldn't find socket!\n");
		exit(1);
	}
	dd_sendstring(d, "\n[35mWU-LRP! V1.0\n\n[0mGimme new lrp, nigga! (preferably 187!): [36m");
	dd_getlprs(d, &lp);
	sprintf(buf, "%d", lp.lrp_read);
	dd_prompt(d, buf, 5, 0);
	lp.lrp_read = atoi(buf);
	dd_setlprs(d, &lp);
	dd_sendstring(d, "[35m\nThank you for using WU-LRP! WU-Tang clan is nuthing to fuck w/!!!! <grin>\n");
	dd_close(d);
	
	return 0;
}
