/*
 * DD-Echo
 * Date formatting routines
 * Author: Bo Simonsen <bo@geekworld.dk>
 */

#include <stdio.h>
#include <time.h>
#include <string.h>

static const char* text_m[] = {"Jan", "Feb", "Mar", "Apr",
												"May", "Jun", "Jul", "Aug", "Sep", "Oct", 
												"Nov", "Dec"};
static const char* unkn_m = "???";
static const int num_m = sizeof(text_m) / sizeof(char*);

struct tm* StrToTm(char* buf, struct tm* st)
{
	char tmp[4];
	int i;

	sscanf(buf, "%02d %03s %02d  %02d:%02d:%02d", &st->tm_mday, 
																								tmp, 
																								&st->tm_year,
																								&st->tm_hour,
																								&st->tm_min, 
																								&st->tm_sec);
    
	st->tm_year += 100;
 
	for(i=0; i < num_m; i++)
		if(!strcasecmp(text_m[i], tmp)) {
			st->tm_mon = i;
			break;
		}
 
	return st;
}

char* TmToStr(struct tm* st, char* buf)
{
	static char tmp[64];
	char* ptr;

	if(!st)
		return NULL;
	
	if(buf == NULL) 
		buf = tmp;

	if(st->tm_mon >= 0 && st->tm_mon < num_m) {
		ptr = (char*) text_m[st->tm_mon];
	}
	else {
		printf("Bad month %d\n", st->tm_mon);
		ptr = (char*) unkn_m;
	}
	
	sprintf(buf, "%02d %s %02d  %02d:%02d:%02d", st->tm_mday,
																							 ptr,
																							 st->tm_year-100,
																							 st->tm_hour,
																							 st->tm_min,
																							 st->tm_sec);
 
	return buf;
}


