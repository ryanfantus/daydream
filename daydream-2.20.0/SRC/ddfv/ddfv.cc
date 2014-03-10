#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include "libddc++.h"
#include "lightbar.h"
#include "entrypack.h"
#include "listviewer.h"
#include "common.h"

int current_area;

int GetArea()
{
	char buffer[128];
	char *endptr;

	GetStrVal(buffer, sizeof buffer, DOOR_PARAMS);

	int area=strtol(buffer, &endptr, 10);
	
	if (*endptr||!*buffer)
		area=1;
	
	if (area<1||area>GetConf()->CONF_FILEAREAS)
		area=1;
	
	return area;
}
	
void DoorCode(void)
{
	char buffer[256]; FILE *fp;
	sprintf(buffer, "%s/configs/ddfv.cfg", getenv("DAYDREAM"));
	if ((fp=fopen(buffer, "r"))!=NULL) {
		if (parse_file(fp)) {			
			dprintf("Syntax error in \"ddfv.cfg\".\n");
			return;
		}
		fclose(fp);
	}
	
	loadarchivers();

	current_area=GetArea();
	for (;;) {
		FileList l(GetIntVal(USER_SCREENLENGTH), current_area);
		
		l.Print(P_REFRESH);
		if (!l.Handler())
			break;
	}
		       	       
	dprintf("\e[0m\e[2J\e[H");
}
