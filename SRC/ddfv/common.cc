#include <stdlib.h>
#include <string.h>
#include "entrypack.h"
#include "common.h"

char *DirColors[2][2][2]={
	{
		{"\e[0;36m", "\e[0;41;33;1m"},
		{"\e[0;37;1m", "\e[0;42;37;1m"}},
	{
		{"\e[44;37;1m", "\e[0;42;33;1m"},
		{"", ""}}
};
		
char *stdcolor="\e[0;37m";

/****************************************************************************/

char *basename(char *buffer, char *path)
{
	char *p;
	if (path==NULL)
		return NULL;
	if ((p=strrchr(path, '/'))==NULL)
		p=path;
	else p++;
	if (buffer==NULL)
		return strdup(p);
	else {
		// Use safer string copy with bounds checking
		size_t len = strlen(p);
		if (len >= 256) { // Assume reasonable buffer size limit
			return NULL; // Buffer too small
		}
		strncpy(buffer, p, 255);
		buffer[255] = '\0'; // Ensure null termination
		return buffer;
	}
}

/****************************************************************************/

int ext_atoi(char *s, int len)
{
	// Add bounds checking to prevent buffer overrun
	if (s == NULL || len < 0) {
		return 0;
	}
	
	size_t str_len = strlen(s);
	if (len > (int)str_len) {
		return 0; // len exceeds string length
	}
	
	char tmp = s[len];
	s[len] = 0;
	int i = atoi(s);
	s[len] = tmp;
	return i;
}

/****************************************************************************/

FVEntryPack::FVEntryPack(void) : dirflag(0), tagged(0)
{
};

/****************************************************************************/

void FVEntryPack::SetTagStatus(int t) 
{
	tagged=t;
};

/****************************************************************************/

void FVEntryPack::ToggleTag(void) 
{
	tagged=tagged?0:1;
};
